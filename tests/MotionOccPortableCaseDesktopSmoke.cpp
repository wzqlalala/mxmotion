#include "CadDocument.h"
#include "MotionWindow.h"
#include "NativeMotionDocument.h"
#include "OccViewport.h"
#include "compat/mViewportModelManager.h"
#include "compat/model/LegacyMotionContext.h"
#include "mMotionAnimationPanel.h"
#include "mMotionResultData.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QLineEdit>
#include <QLineF>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;

namespace {

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toUtf8().constData());
}

template<class Predicate>
bool waitFor(Predicate ready, int timeout)
{
    if (ready()) return true;
    QEventLoop wait;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &wait, [&] {
        if (ready()) wait.quit();
    });
    QTimer::singleShot(timeout, &wait, &QEventLoop::quit);
    poll.start(20);
    wait.exec();
    return ready();
}

bool saveProject(MotionWindow& window, const QString& path)
{
    QTimer selectFile;
    QObject::connect(&selectFile, &QTimer::timeout, &window, [&] {
        auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        dialog->selectFile(path);
        if (auto* filename = dialog->findChild<QLineEdit*>("fileNameEdit")) filename->setText(path);
        if (dialog->selectedFiles().contains(path))
            QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &window, [] {
        if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()))
            dialog->reject();
    });
    selectFile.start(20);
    deadline.start(10000);
    window.command(Command::SaveAs)->trigger();
    return QFileInfo(path).size() > 0;
}

LegacyMotionContext& currentContext()
{
    require(getCVM && getCVM->getModelData()->context, "Original model facade is unavailable");
    return *getCVM->getModelData()->context;
}

void selectNode(MotionWindow& window, ModelNode::Kind kind, int id)
{
    auto* tree = window.findChild<QTreeWidget*>("mTreeView");
    require(tree != nullptr, "Original model tree is missing");
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        auto* item = *it;
        if (item->data(0, Qt::UserRole + 1).toInt() != kind || item->data(0, Qt::UserRole).toInt() != id) continue;
        require(!item->isDisabled(), "The requested scenario node is disabled");
        tree->setCurrentItem(item);
        QApplication::processEvents();
        return;
    }
    throw std::runtime_error("The requested scenario node is missing");
}

const MechMotionData::mMotionResultData* resultFor(const LegacyMotionContext& context, int scenarioId)
{
    const auto& scenarios = context.model()->getManager<MechMotionData::mMotionScenarioManager>(MxFunctions::Physical_MotionScenario);
    const auto* scenario = scenarios.getData(scenarioId);
    return scenario && scenario->hasResult() ? scenario->getResult() : nullptr;
}

QString outputTail(MotionWindow& window)
{
    const auto* console = window.findChild<QPlainTextEdit*>();
    return console ? console->toPlainText().right(4000) : QString{};
}

void verifyDriveTables(const NativeMotionDocument& source, const NativeMotionDocument& opened)
{
    for (const auto& driveValue : source.collection("drives")) {
        const auto drive = driveValue.toObject();
        const int id = drive.value("id").toInt();
        const auto actual = opened.entity("drives", id);
        require(!actual.isEmpty() && actual.value("driveTableX") == drive.value("driveTableX")
            && actual.value("driveTableY") == drive.value("driveTableY"),
            QStringLiteral("Opening the portable input changed drive table %1").arg(id));
    }
}

double verifyPose(OccViewport& viewport, const LegacyMotionContext& context,
    const MechMotionData::mMotionResultData& result, int index)
{
    constexpr std::size_t probeCount = 8;
    double maxDisplacement = 0;
    bool compared = false;
    const auto& initial = result.m_frames.front().m_bodyTransforms;
    const auto& frame = result.m_frames.at(index).m_bodyTransforms;
    const auto& bindings = context.document().bodyPartBindings;
    for (auto binding = bindings.cbegin(); binding != bindings.cend(); ++binding) {
        const auto first = initial.find(binding.key());
        const auto current = frame.find(binding.key());
        if (first == initial.end() || current == frame.end()) continue;
        for (int partId : binding.value()) {
            const auto* part = context.cadDocument().part(partId);
            if (!part || part->display.vertices.empty()) continue;
            const auto& vertices = part->display.vertices;
            for (std::size_t probe = 0; probe < probeCount; ++probe) {
                // Sample across the mesh since a single vertex can lie on the hinge axis.
                const auto& point = vertices[(vertices.size() - 1) * probe / (probeCount - 1)];
                const auto transformPoint = [&](const QMatrix4x4& transform) {
                    const auto mapped = transform.map(point);
                    return viewport.projectPoint({mapped.x(), mapped.y(), mapped.z()});
                };
                const auto expected = transformPoint(current->second);
                const auto shown = viewport.projectPoint({point.x(), point.y(), point.z()}, partId);
                const auto error = QLineF(expected, shown).length();
                require(std::isfinite(error) && error < 0.5,
                    QStringLiteral("Frame %1 does not display the accepted CAD pose of part %2 (%3 px)")
                        .arg(index).arg(partId).arg(error));
                maxDisplacement = std::max(maxDisplacement, QLineF(transformPoint(first->second), shown).length());
                compared = true;
            }
        }
    }
    require(compared, "The accepted frame contains no transforms for visible CAD parts");
    return maxDisplacement;
}

} // namespace

int runMotionOccPortableCaseDesktopSmoke(const QString& outputDirectory, const QString& motionPath)
{
    const QDir output(outputDirectory);
    if (!QDir().mkpath(output.absolutePath())) return 2;
    QFile report(output.filePath("portable-case-smoke.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) return 2;
    QTextStream log(&report);
    try {
        require(!motionPath.isEmpty(), "Supply a portable input JSON with --motion");
        NativeMotionDocument source;
        QString error;
        require(readNativeMotionDocument(motionPath, source, error), error);
        const bool mapped = source.root.contains("geometryExchange");
        const auto association = mapped ? source.root.value("geometryExchange").toObject()
            : source.root.value("generatorNotes").toObject().value("geometryBinding").toObject();
        const auto step = QDir::cleanPath(QDir::fromNativeSeparators(association.value("stepFile").toString()));
        require(!step.isEmpty() && QDir::isRelativePath(step) && !step.contains(':')
            && step != ".." && !step.startsWith("../"), "The source JSON requires a relative STEP path within its package");
        require(!association.value(mapped ? "sha256" : "stepSHA256").toString().isEmpty(),
            "The source JSON is missing the associated STEP hash");
        const auto scenarios = source.collection("scenarios");
        require(scenarios.size() == 1, "The portable smoke input requires exactly one scenario");
        const auto scenario = scenarios.first().toObject();
        const auto scenarioId = scenario.value("id").toInt(-1);
        const double endTime = scenario.value("endTime").toDouble();
        require(endTime > 0, "The portable smoke input requires a positive scenario duration");

        QTemporaryDir movedInput(output.absoluteFilePath("relative-input-XXXXXX"));
        QTemporaryDir staging(output.absoluteFilePath("saved-XXXXXX"));
        QTemporaryDir relocated(output.absoluteFilePath("reopened-XXXXXX"));
        require(movedInput.isValid() && staging.isValid() && relocated.isValid(), "Could not create isolated project directories");
        const QFileInfo originalInput(motionPath);
        const QString movedMotion = movedInput.filePath(originalInput.fileName());
        const QString movedStep = movedInput.filePath(step);
        require(QDir().mkpath(QFileInfo(movedStep).absolutePath())
            && QFile::copy(originalInput.absoluteDir().filePath(step), movedStep)
            && QFile::copy(originalInput.absoluteFilePath(), movedMotion), "Could not copy the unchanged JSON and STEP into the new input directory");
        const QString stagedProject = staging.filePath("portable-case.mxmotion");
        const QString relocatedProject = relocated.filePath("portable-case.mxmotion");

        bool projectSaved = false;
        QMap<int, QVector<int>> bindings;
        {
            MotionWindow window;
            window.resize(1440, 960);
            window.show();
            const bool imported = window.importMotionJson(movedMotion, false);
            require(imported, QStringLiteral("Portable JSON import failed\n%1").arg(outputTail(window)));
            selectNode(window, ModelNode::Scenario, scenarioId);
            const auto& opened = currentContext().document();
            bindings = opened.bodyPartBindings;
            require(!bindings.isEmpty(), "The imported motion bodies were not bound to CAD");
            verifyDriveTables(source, opened);
            log << "PASS relocated JSON + STEP import with " << bindings.size() << " bound bodies and unchanged drive tables\n";
            log.flush();
            projectSaved = saveProject(window, stagedProject);
            if (!projectSaved) log << "WARN standalone Save As failed\n" << outputTail(window) << '\n';
        }
        MotionWindow window;
        window.resize(1440, 960);
        window.show();
        bool projectReopened = false;
        if (projectSaved && QFile::rename(stagedProject, relocatedProject)) {
            projectReopened = window.openRigidProject(relocatedProject);
            if (!projectReopened) log << "WARN relocated standalone project failed to reopen\n" << outputTail(window) << '\n';
        } else if (projectSaved) {
            log << "WARN could not relocate the saved standalone project\n";
        }
        if (!projectReopened) {
            const bool imported = window.importMotionJson(movedMotion, false);
            require(imported, QStringLiteral("Portable JSON could not be reopened for solve\n%1").arg(outputTail(window)));
        }
        selectNode(window, ModelNode::Scenario, scenarioId);
        auto& context = currentContext();
        require(context.document().bodyPartBindings == bindings, "Reopen changed the CAD body bindings");
        verifyDriveTables(source, context.document());
        log << (projectReopened ? "PASS standalone Save As and reopen" : "PASS reopened relocated JSON + STEP")
            << " with unchanged body bindings and drive tables\n";
        log.flush();

        auto* viewportModel = getCVM;
        require(viewportModel && viewportModel->solveMotion, "Original scenario solve adapter is unavailable");
        require(!resultFor(context, scenarioId), "The reopened project started with a cached result");
        bool finished = false;
        bool solved = false;
        QObject::connect(viewportModel, &MechPreData::mViewportModel::motionSolveFinished, &window,
            [&](int id, bool success) {
                if (id != scenarioId) return;
                finished = true;
                solved = success;
            });
        QElapsedTimer solveTimer;
        solveTimer.start();
        // Use the original panel's backend callback. Its platform export normally
        // writes to the shared WorkingDir beside the executable.
        viewportModel->solveMotion(scenarioId);
        if (!waitFor([&] { return finished; }, 300000)) {
            if (viewportModel->stopMotion) viewportModel->stopMotion();
            throw std::runtime_error("Timed out waiting for the real desktop solver");
        }
        require(solved, QStringLiteral("The desktop solver failed\n%1").arg(outputTail(window)));
        selectNode(window, ModelNode::Animation, scenarioId);
        const auto& animationContext = currentContext();
        const auto* result = resultFor(animationContext, scenarioId);
        require(result && result->getFrameCount() > 1, "Original animation did not receive solver frames");
        require(std::abs(result->m_frames.front().m_time) < 1e-9
            && std::abs(result->getTotalTime() - endTime) < 1e-7, "Accepted results do not span the full scenario");
        for (int i = 1; i < result->getFrameCount(); ++i)
            require(result->m_frames[i].m_time > result->m_frames[i - 1].m_time, "Accepted time grid is not increasing");
        log << "PASS real solve in " << solveTimer.elapsed() << " ms with " << result->getFrameCount()
            << " accepted frames through " << result->getTotalTime() << " s\n";
        log.flush();

        auto* viewport = window.findChild<OccViewport*>();
        require(viewport && waitFor([&] { return viewport->glReady(); }, 10000), "Hidden OpenGL viewport did not initialize");
        window.command(Command::Fit)->trigger();
        const auto animations = window.findChildren<MechMainWindow::mMotionAnimationPanel*>();
        QSlider* timeline = nullptr;
        MechMainWindow::mMotionAnimationPanel* animation = nullptr;
        for (auto* panel : animations) {
            if (!panel->isVisible()) continue;
            animation = panel;
            timeline = panel->findChild<QSlider*>();
            break;
        }
        const int last = result->getFrameCount() - 1;
        require(timeline && timeline->isEnabled() && timeline->maximum() == last,
            "Original playback slider does not cover every accepted frame");
        QImage firstImage;
        bool imageChanged = false;
        double mostMovement = 0;
        for (const int index : std::array<int, 5>{0, last / 4, last / 2, last - last / 4, last}) {
            timeline->setValue(index);
            QApplication::processEvents();
            const double displacement = verifyPose(*viewport, animationContext, *result, index);
            mostMovement = std::max(mostMovement, displacement);
            const auto image = viewport->grabFramebuffer();
            if (firstImage.isNull()) firstImage = image;
            else imageChanged |= image != firstImage;
            const auto name = QStringLiteral("pose-%1.png").arg(index, 6, 10, QLatin1Char('0'));
            require(!image.isNull() && image.save(output.filePath(name)), "Could not capture the hidden rendered pose");
            log << "PASS frame " << index << " at " << result->m_frames[index].m_time
                << " s, maximum sampled displacement " << displacement << " px, " << name << '\n';
        }
        require(mostMovement > 1, "Playback did not move any bound CAD parts at the sampled positions");
        require(imageChanged, "The hidden OpenGL framebuffer did not change during playback");
        require(window.grab().save(output.filePath("portable-case-animation.png")), "Could not capture the hidden animation panel");
        timeline->setValue(0);
        QPushButton* play = nullptr;
        QPushButton* pause = nullptr;
        for (auto* button : animation->findChildren<QPushButton*>()) {
            if (button->text() == QStringLiteral("播放")) play = button;
            if (button->text() == QStringLiteral("暂停")) pause = button;
        }
        require(play && pause, "Original playback controls are unavailable");
        play->click();
        require(waitFor([&] { return timeline->value() > 0; }, 3000), "Playback did not advance the timeline");
        pause->click();

        log << "PASS original playback controls\n";
        if (projectReopened) {
            relocated.setAutoRemove(false);
            log << "PROJECT " << output.relativeFilePath(relocatedProject) << '\n';
        }
        return 0;
    } catch (const std::exception& exception) {
        log << "FAIL " << QString::fromUtf8(exception.what()) << '\n';
        return 1;
    }
}
