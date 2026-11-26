#include "MotionWindow.h"
#include "compat/model/LegacyMotionContext.h"
#include "MotionTreeIcons.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"
#include "RigidPlayback.h"
#include "NativeMotionPresentation.h"
#include "NativeMotionSymbols.h"
#include "NativeModeInspector.h"
#include "ThemeManager.h"
#include "UiMetrics.h"
#include "mFoldSection.h"
#include "mUiMenu.h"
#include "mMotionScenarioPanel.h"
#include "mCurveWindow.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMatrix4x4>
#include <QProgressBar>
#include <QPushButton>
#include <QPointer>
#include <QQuaternion>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

namespace MotionOcc {
namespace {
constexpr int idRole = Qt::UserRole;
constexpr int kindRole = Qt::UserRole + 1;
using MechMainWindow::ThemeManager;
using MechMainWindow::FoldSection;
using MxFunctions::mUiMenu;

QIcon motionIcon(const char* file)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/Motion/") + QString::fromLatin1(file));
}

template<class T> const T* byId(const std::vector<T>& list, int id)
{
    const auto it = std::find_if(list.begin(), list.end(), [id](const auto& x) { return x.id == id; });
    return it == list.end() ? nullptr : &*it;
}

template<class T> int nextId(const std::vector<T>& list)
{
    for (int id = 1; id < 1000000; ++id) if (!byId(list, id)) return id;
    return 0;
}

template<class T> void replace(std::vector<T>& list, const T& item)
{
    const auto it = std::find_if(list.begin(), list.end(), [&](const T& x) { return x.id == item.id; });
    if (it == list.end()) list.push_back(item);
    else *it = item;
}

template<class T, class F> void eraseIf(std::vector<T>& list, F predicate)
{
    list.erase(std::remove_if(list.begin(), list.end(), predicate), list.end());
}

QString jointLabel(JointKind kind)
{
    switch (kind) {
    case JointKind::Revolute: return QStringLiteral("旋转副");
    case JointKind::Prismatic: return QStringLiteral("移动副");
    case JointKind::Fixed: return QStringLiteral("固定副");
    }
    return {};
}

QDoubleSpinBox* number(QFormLayout* form, const QString& name, double value,
                       double min = -1e8, double max = 1e8, int decimals = 6)
{
    auto* spin = new QDoubleSpinBox;
    spin->setDecimals(decimals);
    spin->setRange(min, max);
    spin->setKeyboardTracking(false);
    spin->setValue(value);
    form->addRow(name, spin);
    return spin;
}

std::array<QDoubleSpinBox*, 3> vectorFields(QFormLayout* form, const QString& name, const Vector3d& value)
{
    std::array<QDoubleSpinBox*, 3> fields{};
    for (int axis = 0; axis < 3; ++axis)
        fields[axis] = number(form, name + QLatin1Char(' ') + QLatin1Char('X' + axis), value[axis]);
    return fields;
}

Vector3d vectorValue(const std::array<QDoubleSpinBox*, 3>& fields)
{
    return {fields[0]->value(), fields[1]->value(), fields[2]->value()};
}

void selectData(QComboBox* combo, int id)
{
    const int index = combo->findData(id);
    if (index >= 0) combo->setCurrentIndex(index);
}

void finishForm(QFormLayout* form, MotionWindow* owner, const std::function<void()>& apply, const std::function<void()>& cancel)
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* yes = new QPushButton(QStringLiteral("确定"), row);
    yes->setObjectName(QStringLiteral("motionApplyButton"));
    auto* no = new QPushButton(QStringLiteral("取消"), row);
    layout->addWidget(yes);
    layout->addWidget(no);
    form->addRow(row);
    QObject::connect(yes, &QPushButton::clicked, owner, apply);
    QObject::connect(no, &QPushButton::clicked, owner, cancel);
}

void openDock(ads::CDockWidget* dock)
{
    dock->toggleView(true);
    if (auto* area = dock->dockAreaWidget()) area->setCurrentDockWidget(dock);
}
}

void MotionWindow::createRigidCommands()
{
    auto add = [this](Command id, const QString& text, const char* image, const std::function<void()>& fn) {
        auto* action = new QAction(motionIcon(image), text, this);
        m_commands.emplace(id, action);
        connect(action, &QAction::triggered, this, fn);
    };
    add(Command::AddBody, tr("运动体"), "movingBody.png", [this] { editBody(); });
    add(Command::BatchBodies, tr("一键生成运动体"), "bodyBatch.png", [this] { batchBodies(); });
    add(Command::AddRevolute, tr("旋转副"), "joint_revolute.png", [this] { editJoint(JointKind::Revolute); });
    add(Command::AddPrismatic, tr("移动副"), "joint_prismatic.png", [this] { editJoint(JointKind::Prismatic); });
    add(Command::AddFixed, tr("固定副"), "joint_fixed.png", [this] { editJoint(JointKind::Fixed); });
    add(Command::AddSpeedDrive, tr("速度驱动"), "drive.png", [this] { editDrive(DriveKind::Speed); });
    add(Command::AddPositionDrive, tr("位移驱动"), "drive.png", [this] { editDrive(DriveKind::Displacement); });
    add(Command::NewScenario, tr("新建求解方案"), "drive.png", [this] { editScenario(); });
    add(Command::Solve, tr("求解"), "solve.png", [this] {
        int id = m_model.scenarios.empty() ? -1 : m_model.scenarios.back().id;
        if (m_selectionNode.kind >= ModelNode::Scenario && m_selectionNode.kind <= ModelNode::Curves) id = m_selectionNode.id;
        if (id >= 0 && m_model.nativeDocument) {
            editOriginalMotionEntity(QStringLiteral("scenarios"),id,{});
            if(m_editor)if(auto* panel=m_editor->findChild<MechMainWindow::mMotionScenarioPanel*>())
                panel->onStartSolve(id);
        }
        else if (id >= 0) startRigidSolve(id);
        else editScenario();
    });
    add(Command::StopSolve, tr("停止求解"), "solve.png", [this] {
        if (m_running) m_running->stop.store(true, std::memory_order_relaxed);
        updateActions();
    });
    add(Command::ModelCheck, tr("模型检查"), "joint_fixed.png", [this] { showModelDiagnostics(false, false); });
    add(Command::ConstraintDiagnostics, tr("约束诊断"), "joint_fixed.png", [this] { showModelDiagnostics(true, false); });
    add(Command::HydraulicCircuitCheck, tr("回路检查"), "controlCalculation.png", [this] { showModelDiagnostics(false, true); });
    add(Command::LoadRigidExample, tr("刚体示例"), "movingBody.png", [this] { loadRigidExample(); });
    const auto addResult = [this](Command id, const QString& text, const char* icon, ModelNode::Kind kind) {
        auto* action = new QAction(TreeIcons::item(QString::fromLatin1(icon)), text, this);
        m_commands.emplace(id, action);
        connect(action, &QAction::triggered, this, [this, kind] {
            const int scenarioId = scenarioForResultCommand();
            if (m_projectActive && scenarioId >= 0) showRigidResult({kind, scenarioId});
        });
    };
    addResult(Command::ShowAnimation, tr("动画"), "animation", ModelNode::Animation);
    addResult(Command::ShowResultCurves, tr("运动曲线"), "curves", ModelNode::Curves);
    addResult(Command::ExportResult, tr("结果导出"), "export", ModelNode::Export);
    createNativeCommands();
}

void MotionWindow::createRigidDocks(ads::CDockAreaWidget* bottom)
{
    m_rigidPlot = new RigidPlot;
    m_curveDock = addPane("Curve", tr("函数与曲线"), m_rigidPlot);
    m_docks->addDockWidgetTabToArea(m_curveDock, bottom);
    m_curveDock->closeDockWidget();
    auto* controls = new QWidget;
    auto* layout = new QVBoxLayout(controls);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);
    auto* playback = new FoldSection(tr("播放控制"), controls);
    m_playToggle = new QPushButton(tr("播放"), controls);
    m_playToggle->setObjectName(QStringLiteral("motionPlayButton"));
    m_pausePlayback = new QPushButton(tr("暂停"), controls);
    m_pausePlayback->setObjectName(QStringLiteral("motionPauseButton"));
    m_stopPlayback = new QPushButton(tr("停止"), controls);
    m_stopPlayback->setObjectName(QStringLiteral("motionStopButton"));
    // Fusion's default QPushButton minimum-size hint is about 80 px. Three
    // such buttons force the original 250 px parameter column to scroll
    // horizontally. These short labels should share the available row width.
    for (auto* button : {m_playToggle, m_pausePlayback, m_stopPlayback}) {
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        button->setMinimumWidth(button->fontMetrics().horizontalAdvance(button->text())
                                + 2 * MxFunctions::UiMetrics::fieldInset());
    }
    playback->addRow(m_playToggle, m_pausePlayback, m_stopPlayback, QString());
    layout->addWidget(playback);
    auto* timeline = new FoldSection(tr("时间轴"), controls);
    m_timeline = new QSlider(Qt::Horizontal, controls);
    m_timeline->setObjectName(QStringLiteral("motionTimeline"));
    m_timeline->setRange(0, 0);
    timeline->addRow(m_timeline);
    layout->addWidget(timeline);
    auto* settings = new FoldSection(tr("播放设置"), controls);
    m_timeLabel = new QLabel(tr("时间: 0.000 s / 0.000 s"), controls);
    m_timeLabel->setObjectName(QStringLiteral("motionPlaybackTime"));
    settings->addRow(m_timeLabel);
    m_playbackSpeed = new QComboBox(controls);
    m_playbackSpeed->setObjectName(QStringLiteral("motionPlaybackSpeed"));
    for (double speed : {0.5, 1.0, 2.0, 5.0, 50.0})
        m_playbackSpeed->addItem(QStringLiteral("%1x").arg(speed, 0, 'f', 1), speed);
    m_playbackSpeed->setCurrentIndex(1);
    settings->addRow(tr("播放速度:"), m_playbackSpeed);
    m_loopPlayback = new QCheckBox(tr("循环播放"), controls);
    m_loopPlayback->setObjectName(QStringLiteral("motionLoopPlayback"));
    settings->addRow(m_loopPlayback);
    layout->addWidget(settings);
    auto* curveLink = new FoldSection(tr("曲线联动"), controls);
    m_followCurveMarker = new QCheckBox(tr("曲线上跟随动画帧标记"), controls);
    m_followCurveMarker->setObjectName(QStringLiteral("motionFollowCurveMarker"));
    m_followCurveMarker->setChecked(true);
    m_clickCurveJump = new QCheckBox(tr("点击曲线跳转动画帧"), controls);
    m_clickCurveJump->setObjectName(QStringLiteral("motionClickCurveJump"));
    m_clickCurveJump->setChecked(true);
    curveLink->addRow(m_followCurveMarker);
    curveLink->addRow(m_clickCurveJump);
    layout->addWidget(curveLink);
    auto* flexDisplay = new FoldSection(tr("柔性体云图"), controls);
    m_flexContour = new QComboBox(controls); m_flexContour->setObjectName("motionFlexContour");
    m_flexContour->addItem(tr("关闭云图"),-1);
    m_flexContour->addItem(tr("弹性位移 (mm)"),int(FlexContourKind::ElasticDisplacement));
    m_flexContour->addItem(tr("整体位移 (mm)"),int(FlexContourKind::TotalDisplacement));
    m_flexContour->addItem(tr("原自由模态投影 (mm，仅自动柔性体)"),int(FlexContourKind::LegacyFreeModeProjection));
    m_flexContour->setToolTip(tr("弹性位移去除刚体运动。整体位移相对初始几何。原自由模态投影沿用原界面的近似投影公式。"));
    m_flexRange = new QComboBox(controls); m_flexRange->setObjectName("motionFlexRange");
    m_flexRange->addItem(tr("当前帧自适应"),0); m_flexRange->addItem(tr("整个时程统一范围"),1);
    flexDisplay->addRow(tr("显示量"),m_flexContour); flexDisplay->addRow(tr("色标范围"),m_flexRange);
    m_modeInspectButton=new QPushButton(tr("检查真实模态…"));m_modeInspectButton->setObjectName("motionInspectModes");
    flexDisplay->addRow(m_modeInspectButton);
    connect(m_modeInspectButton,&QPushButton::clicked,this,[this]{
        const auto run=m_results.find(m_resultScenario);
        if(run==m_results.end() || !run->second->input.nativeInput)return;
        setPlayback(false);
        openNativeModeInspector(this,m_document,run->second->input.nativeInput->document,run->second->result.flexBodies);
    });
    layout->addWidget(flexDisplay);
    connect(m_flexContour,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int) { seekRigidTime(m_playbackTime); });
    connect(m_flexRange,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int) { seekRigidTime(m_playbackTime); });
    layout->addStretch();
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(controls);
    m_animationDock = addPane("MotionAnimation", tr("动画"), scroll);
    // The original animation editor lives in the shared parameter column.
    m_docks->addDockWidgetTabToArea(m_animationDock, m_parameterDock->dockAreaWidget());
    m_animationDock->closeDockWidget();
    connect(m_playToggle, &QPushButton::clicked, this, [this] { setPlayback(true); });
    connect(m_pausePlayback, &QPushButton::clicked, this, [this] { setPlayback(false); });
    connect(m_stopPlayback, &QPushButton::clicked, this, &MotionWindow::stopRigidPlayback);
    connect(m_timeline, &QSlider::valueChanged, this, [this](int frameIndex) {
        const auto run = m_results.find(m_resultScenario);
        if (!m_animationActive || run == m_results.end() || frameIndex < 0
            || frameIndex >= int(run->second->result.frames.size())) return;
        if (m_playbackTimer->isActive()) m_playbackClock.restart();
        seekRigidTime(run->second->result.frames[std::size_t(frameIndex)].seconds);
    });
    connect(m_playbackSpeed, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        if (m_playbackTimer && m_playbackTimer->isActive()) m_playbackClock.restart();
    });
    connect(m_followCurveMarker, &QCheckBox::toggled, this, [this](bool on) {
        m_rigidPlot->setTimeMarkerVisible(m_animationActive && on);
        if (m_animationActive && on) m_rigidPlot->setTime(m_playbackTime);
    });
    connect(m_rigidPlot, &RigidPlot::timeRequested, this, [this](double time) {
        if (!m_animationActive || !m_clickCurveJump->isChecked()) return;
        if (m_playbackTimer->isActive()) m_playbackClock.restart();
        seekRigidTime(time);
    });
    connect(m_rigidPlot, &RigidPlot::curvesApplied, this, [this] {
        if (m_rigidPlot->hasVisibleCurves()) openDock(m_curveDock);
    });
    connect(m_rigidPlot, &RigidPlot::curveRequestsChanged, this, [this](int scenarioId) {
        const auto requests = m_rigidPlot->curveRequests(scenarioId);
        for (auto& scenario : m_model.scenarios)
            if (scenario.id == scenarioId) { scenario.curveRequests = requests; setProjectModified(true); }
        syncNativeScenarioCurves(m_model, scenarioId);
        const auto result = m_results.find(scenarioId);
        if (result != m_results.end()) result->second->input.scenario.curveRequests = requests;
        updateActions();
    });
    m_rigidPlot->setTimeMarkerVisible(false);
}

void MotionWindow::createRigidControls()
{
    m_rigidPoll = new QTimer(this);
    m_rigidPoll->setInterval(50);
    connect(m_rigidPoll, &QTimer::timeout, this, &MotionWindow::pollRigidSolve);
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(16);
    connect(m_playbackTimer, &QTimer::timeout, this, [this] {
        const auto run = m_results.find(m_resultScenario);
        if (!m_animationActive || run == m_results.end() || run->second->result.frames.empty())
            { setPlayback(false); return; }
        const auto& frames = run->second->result.frames;
        const auto first = frames.front().seconds, end = frames.back().seconds;
        double time = m_playbackTime + m_playbackClock.restart() / 1000.0 * m_playbackSpeed->currentData().toDouble();
        if (time >= end) {
            if (m_loopPlayback->isChecked() && end > first) time = first;
            else { stopRigidPlayback(); return; }
        }
        seekRigidTime(time);
    });
    connect(m_animationDock, &ads::CDockWidget::closed, this, [this] {
        if (m_projectActive && !m_animationDock->property("mxOriginalAnimationControllerActive").toBool())
            leaveRigidAnimation();
    });
    auto* animationArea = m_animationDock->dockAreaWidget();
    connect(animationArea, &ads::CDockAreaWidget::currentChanged, this, [this, animationArea](int) {
        if (!m_projectActive || m_animationDock->property("mxOriginalAnimationControllerActive").toBool()) return;
        if (m_animationActive && m_animationDock->dockAreaWidget() == animationArea
            && !m_animationDock->isCurrentTab()) leaveRigidAnimation();
    });
    connect(m_animationDock, &ads::CDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!m_projectActive || m_animationDock->property("mxOriginalAnimationControllerActive").toBool()) return;
        // Switching the shared parameter column to another editor ends the
        // animation session just like uneditAnimationPanel in the original UI.
        // ADS emits Hide before changing its current-widget pointer. The area
        // signal above handles the normal switch synchronously; check again
        // after reparenting for dock areas created by a later drag operation.
        // Minimizing the window keeps the animation as its area's current tab.
        if (!visible && m_animationActive) QTimer::singleShot(0, this, [this] {
            if (!m_projectActive || m_animationDock->property("mxOriginalAnimationControllerActive").toBool()) return;
            if (m_animationActive && !m_animationDock->isCurrentTab()) leaveRigidAnimation();
        });
    });
    updatePlaybackControls();
}

bool MotionWindow::motionBusy() const { return m_running || m_importingGeometry; }

bool MotionWindow::canRemovePart(int id) const
{
    if (m_model.nativeDocument) {
        for (const auto& parts : m_model.nativeDocument->bodyPartBindings) if (parts.contains(id)) return false;
        return true;
    }
    return std::none_of(m_model.bodies.begin(), m_model.bodies.end(), [id](const auto& b) { return b.partId == id; });
}

void MotionWindow::updateRigidActions()
{
    const bool idle = !motionBusy() && m_jobPart < 0;
    const bool usablePart = std::any_of(m_document.parts().begin(), m_document.parts().end(), [&](const auto& p) { return canRemovePart(p.id); });
    const bool moving = std::any_of(m_model.bodies.begin(), m_model.bodies.end(), [](const auto& b) { return !b.fixed; });
    const bool canDrive = std::any_of(m_model.joints.begin(), m_model.joints.end(), [&](const auto& j) {
        return j.kind != JointKind::Fixed && std::none_of(m_model.drives.begin(), m_model.drives.end(), [&](const auto& d) { return d.jointId == j.id; });
    });
    for (const auto id : {Command::AddBody, Command::BatchBodies}) command(id)->setEnabled(idle && usablePart);
    for (const auto id : {Command::AddRevolute, Command::AddPrismatic, Command::AddFixed, Command::Solve})
        command(id)->setEnabled(idle && moving);
    command(Command::AddSpeedDrive)->setEnabled(idle && canDrive);
    command(Command::AddPositionDrive)->setEnabled(idle && canDrive);
    command(Command::NewScenario)->setEnabled(idle);
    command(Command::LoadRigidExample)->setEnabled(idle);
    command(Command::ModelCheck)->setEnabled(idle && (!m_document.parts().empty() || !m_model.bodies.empty()));
    command(Command::ConstraintDiagnostics)->setEnabled(idle && (!m_document.parts().empty() || !m_model.bodies.empty()));
    command(Command::StopSolve)->setEnabled(m_running && !m_running->stop.load());
    const bool hasResult = scenarioForResultCommand() >= 0;
    for (const auto id : {Command::ShowAnimation, Command::ShowResultCurves, Command::ExportResult})
        command(id)->setEnabled(hasResult);
    command(Command::ImportMotionJson)->setEnabled(idle);
    command(Command::ExportMotionJson)->setEnabled(idle && !m_model.bodies.empty());
    for (const auto id : {Command::AddNativeJoint, Command::AddCoupling, Command::AddSpring,
            Command::AddVectorForce, Command::AddScalarForce, Command::AddScalarTorque,
            Command::AddThreePointForce, Command::AddThreePointTorque, Command::AddContact,
            Command::AddMarker, Command::AddSensor, Command::AddController, Command::AddControlInput,
            Command::AddAutoFlex, Command::AddSoftStop, Command::AddTensionCable,
            Command::AddHydraulicCylinder, Command::AddHydraulicMotor})
        command(id)->setEnabled(idle && !m_model.bodies.empty());
    command(Command::EditHydraulicFluid)->setEnabled(idle);
    command(Command::HydraulicCircuitCheck)->setEnabled(idle);
    if (m_model.nativeDocument) {
        const bool joints = !m_model.nativeDocument->collection(QStringLiteral("joints")).isEmpty();
        command(Command::AddSpeedDrive)->setEnabled(idle && joints);
        command(Command::AddPositionDrive)->setEnabled(idle && joints);
    }
    updatePlaybackControls();
}

void MotionWindow::appendRigidNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups)
{
    if (m_model.nativeDocument) { appendNativeNodes(root, groups); return; }
    auto insert = [this](QTreeWidgetItem* parent, const QString& label, ModelNode::Kind kind, int id, const QIcon& image) {
        auto* item = new QTreeWidgetItem(parent, {label});
        item->setIcon(0, image);
        item->setData(0, idRole, id);
        item->setData(0, kindRole, int(kind));
        parent->setExpanded(true);
        if (m_selectionNode.kind == kind && m_selectionNode.id == id) m_tree->setCurrentItem(item);
        return item;
    };
    for (const auto& b : m_model.bodies) insert(groups.at(tr("运动体")), b.name, ModelNode::Body, b.id, TreeIcons::body(b.fixed));
    for (const auto& j : m_model.joints) insert(groups.at(tr("运动副")), j.name, ModelNode::Joint, j.id,
        TreeIcons::joint(j.kind == JointKind::Revolute ? "Revolute" : j.kind == JointKind::Prismatic ? "Translational" : "Fixed"));
    for (const auto& d : m_model.drives) insert(groups.at(tr("驱动")), d.name, ModelNode::Drive, d.id, motionIcon("drive.png"));
    for (const auto& sc : m_model.scenarios) {
        auto* item = insert(root, sc.name, ModelNode::Scenario, sc.id, TreeIcons::item("scenarios"));
        const bool ready = m_results.count(sc.id) != 0;
        for (const auto& x : {std::pair{tr("动画"), ModelNode::Animation}, {tr("结果导出"), ModelNode::Export}, {tr("运动曲线"), ModelNode::Curves}}) {
            auto* child = insert(item, x.first, x.second, sc.id, TreeIcons::item(
                x.second == ModelNode::Animation ? "animation" : x.second == ModelNode::Curves ? "curves" : "export"));
            if (!ready) child->setDisabled(true);
        }
    }
}

void MotionWindow::selectRigidNode(ModelNode node)
{
    m_viewport->clearViewportSelection();
    m_selectionNode = node;
    m_partId = -1;
    m_viewport->selectPart(-1);
    closeEditor();
    m_meshForm->hide();
    updateParameters();
    updateActions();
    if (motionBusy() || m_jobPart >= 0) {
        if (!m_animationActive) refreshMotionSymbols();
        return;
    }
    switch (node.kind) {
    case ModelNode::Native: editNativeEntity(node.collection, node.id); break;
    case ModelNode::Body: editBody(node.id); break;
    case ModelNode::Joint: if (const auto* j = byId(m_model.joints, node.id)) editJoint(j->kind, j->id); break;
    case ModelNode::Drive: if (const auto* d = byId(m_model.drives, node.id)) editDrive(d->kind, d->id); break;
    case ModelNode::Scenario: editScenario(node.id); break;
    case ModelNode::Animation:
    case ModelNode::Curves: showRigidResult(node); break;
    case ModelNode::Export:
        // The original mTreeView opens LoadExport from its selection handler,
        // just like Animation and Curves. The legacy flat model still opens
        // its direct CSV chooser only on an explicit export activation.
        if (!showOriginalMotionResult(node)) leaveRigidAnimation();
        break;
    default: break;
    }
    if (!m_animationActive) refreshMotionSymbols();
}

void MotionWindow::showRigidMenu(ModelNode node, const QPoint& globalPosition)
{
    mUiMenu menu(this);
    if (node.kind >= ModelNode::Animation && node.kind <= ModelNode::Curves) {
        const auto text = node.kind == ModelNode::Animation ? tr("动画") : node.kind == ModelNode::Curves ? tr("运动曲线") : tr("结果导出");
        auto* action = menu.addAction(text);
        action->setEnabled(m_results.count(node.id) > 0);
        connect(action, &QAction::triggered, this, [this, node] { showRigidResult(node); });
    } else {
        const bool idle = !motionBusy() && m_jobPart < 0;
        auto* edit = menu.addAction(tr("编辑"));
        edit->setEnabled(idle);
        connect(edit, &QAction::triggered, this, [this, node] { selectRigidNode(node); });
        if (node.kind == ModelNode::Scenario) {
            auto* solve = menu.addAction(tr("求解"));
            solve->setEnabled(idle);
            connect(solve, &QAction::triggered, this, [this, node] { startRigidSolve(node.id); });
        }
        auto* remove = menu.addAction(tr("删除"));
        remove->setEnabled(idle);
        connect(remove, &QAction::triggered, this, [this, node] { removeRigidNode(node); });
    }
    ThemeManager::applyMenuStyle(&menu);
    menu.exec(globalPosition);
}

void MotionWindow::showOriginalParameterPanel(QWidget* panel, const QString& scrollName)
{
    if (auto* layout = panel->layout()) {
        layout->setContentsMargins(8, 7, 8, 7);
        layout->setSpacing(4);
    }
    auto* scroll = new QScrollArea;
    scroll->setObjectName(scrollName);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(panel);
    for (auto* surface : {static_cast<QWidget*>(scroll), scroll->viewport(), panel}) {
        surface->setProperty("mxSurface", true);
        surface->setAttribute(Qt::WA_StyledBackground);
    }
    scroll->setProperty("mxOriginalMotionPanel", true);
    showEditor(scroll);
}

void MotionWindow::showEditor(QWidget* editor)
{
    leaveRigidAnimation();
    closeEditor();
    m_meshForm->hide();
    m_parameterTitle->hide();
    m_parameterDetails->hide();
    m_partSettings->hide();
    m_editor = editor;
    const bool originalPanel = editor->property("mxOriginalMotionPanel").toBool();
    if (originalPanel) m_parameterLayout->setContentsMargins(0, 0, 0, 0);
    const bool scrollable = qobject_cast<QScrollArea*>(editor) != nullptr;
    m_parameterLayout->insertWidget(m_parameterLayout->count() - 1, m_editor, scrollable ? 1 : 0);
    m_parameterLayout->setStretch(m_parameterLayout->count() - 1, scrollable ? 0 : 1);
    openDock(m_parameterDock);
    m_editor->show();
    if (originalPanel || editor->property("mxFitParameterDock").toBool()) {
        QPointer<QWidget> guard(editor);
        QTimer::singleShot(0, this, [this, guard] {
            if (!guard || guard != m_editor)return;
            auto* scroll = qobject_cast<QScrollArea*>(guard.data());
            QWidget* content = scroll ? scroll->widget() : guard.data();
            if (!content)return;
            content->ensurePolished();
            if (content->layout())content->layout()->activate();
            const auto margins = m_parameterLayout->contentsMargins();
            const int scrollWidth = scroll ? scroll->verticalScrollBar()->sizeHint().width() + 2 * scroll->frameWidth() : 0;
            const int required = content->minimumSizeHint().expandedTo(content->minimumSize()).width()
                + margins.left() + margins.right() + scrollWidth + 2;
            const int suggested = 12 * MxFunctions::UiMetrics::baseH()
                + 8 * qMax(0, 6 - MxFunctions::UiMetrics::baseG()) + 8;
            // Respect the original content's own measured minimum. In
            // particular, the picker + Apply row must not be hidden behind an
            // extra horizontal scroll area added by the compatibility host.
            m_parameterDock->setMinimumWidth(qMax(suggested - 40, required));
            if (auto* area = m_parameterDock->dockAreaWidget()) {
                auto sizes = m_docks->splitterSizes(area);
                const int preferred = qMax(suggested, required);
                if (sizes.size() == 3 && sizes[1] < preferred) {
                    const int growth = preferred - sizes[1];
                    sizes[1] = preferred;sizes[2] = qMax(1, sizes[2] - growth);
                    m_docks->setSplitterSizes(area, sizes);
                }
            }
        });
    }
}

void MotionWindow::closeEditor(bool immediate)
{
    if (m_original && m_editor && m_editor->property("mxOriginalMotionPanel").toBool()) m_original->endEdit();
    if (m_viewport && m_viewport->isPicking()) { m_viewport->endPick(); m_viewport->setProperty("nativePickOwner",QVariant{}); }
    if (!m_editor) return;
    auto* editor = m_editor;
    m_editor = nullptr;
    editor->hide();
    m_parameterLayout->removeWidget(editor);
    if (immediate) delete editor;
    else editor->deleteLater();
    m_parameterLayout->setContentsMargins(8, 7, 8, 7);
    const int suggested = 12 * MxFunctions::UiMetrics::baseH()
        + 8 * qMax(0, 6 - MxFunctions::UiMetrics::baseG()) + 8;
    m_parameterDock->setMinimumWidth(suggested - 40);
    m_parameterLayout->setStretch(m_parameterLayout->count() - 1, 1);
    m_parameterTitle->show();
    m_parameterDetails->show();
    m_partSettings->setVisible(m_document.part(m_partId) && m_meshForm->isHidden());
}

void MotionWindow::editBody(int id)
{
    if (!m_model.nativeDocument && m_model.bodies.empty() && !ensureNativeModel()) return;
    if (m_model.nativeDocument) { editNativeEntity(QStringLiteral("bodies"), id); return; }
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.bodies, id);
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(tr("运动体 · 质量和惯性矩")));
    const int key = current ? current->id : nextId(m_model.bodies);
    auto* name = new QLineEdit(current ? current->name : tr("运动体-%1").arg(key));
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* part = new QComboBox;
    for (const auto& p : m_document.parts()) if (canRemovePart(p.id) || (current && current->partId == p.id)) part->addItem(p.name, p.id);
    if (part->count() == 0) { delete panel; delete part; setNotice(tr("没有可分配的 CAD 实体。"), true); return; }
    selectData(part, current ? current->partId : m_partId);
    form->addRow(tr("部件"), part);
    auto* fixed = new QCheckBox(tr("固定参考体"));
    fixed->setChecked(current && current->fixed);
    form->addRow(fixed);
    auto* source = new QComboBox;
    source->addItems({tr("指定质量"), tr("按密度计算")});
    source->setCurrentIndex(current && current->densityKgM3 > 0 ? 1 : 0);
    form->addRow(tr("质量来源"), source);
    auto* mass = number(form, tr("质量 (kg)"), current ? current->massKg : 1.0, 1e-8, 1e8, 8);
    auto* density = number(form, tr("密度 (kg/m³)"), current && current->densityKgM3 > 0 ? current->densityKgM3 : 7800, 1e-5, 1e8);
    auto toggle = [=](int index) { mass->setEnabled(index == 0); density->setEnabled(index == 1); };
    connect(source, qOverload<int>(&QComboBox::currentIndexChanged), panel, toggle);
    toggle(source->currentIndex());
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidBody body{key, name->text().trimmed(), part->currentData().toInt(), fixed->isChecked(), mass->value(),
                       source->currentIndex() == 1 ? density->value() : 0.0};
        BodyProperties properties;
        QString error;
        if (!rigidBodyProperties(m_document, body, properties, error)) { setNotice(error, true); return; }
        replace(model.bodies, body);
        commitRigid(std::move(model), {ModelNode::Body, key});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::batchBodies()
{
    if (!m_model.nativeDocument && m_model.bodies.empty() && !ensureNativeModel()) return;
    if (m_model.nativeDocument) { batchNativeBodies(); return; }
    if (motionBusy() || m_jobPart >= 0) return;
    RigidModel next = m_model;
    int added = 0, skipped = 0;
    for (const auto& p : m_document.parts()) {
        if (!canRemovePart(p.id)) continue;
        RigidBody body{nextId(next.bodies), p.name.left(128), p.id, false, 1, 0};
        BodyProperties properties;
        QString error;
        if (rigidBodyProperties(m_document, body, properties, error)) { next.bodies.push_back(body); ++added; }
        else ++skipped;
    }
    if (added) commitRigid(std::move(next), {});
    setNotice(tr("生成 %1 个运动体，跳过 %2 个非实体或无有效惯量的部件。").arg(added).arg(skipped), added == 0);
}

void MotionWindow::editJoint(JointKind kind, int id)
{
    if (m_model.nativeDocument) {
        editNativeEntity(QStringLiteral("joints"), id, kind == JointKind::Revolute ? QStringLiteral("Revolute")
            : kind == JointKind::Prismatic ? QStringLiteral("Translational") : QStringLiteral("Fixed"));
        return;
    }
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.joints, id);
    const RigidJoint initial = current ? *current : RigidJoint{nextId(m_model.joints), {}, kind};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(jointLabel(kind)));
    auto* name = new QLineEdit(initial.name.isEmpty() ? jointLabel(kind) + QStringLiteral("-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* moving = new QComboBox;
    auto* base = new QComboBox;
    base->addItem(tr("大地"), 0);
    for (const auto& b : m_model.bodies) {
        if (!b.fixed) moving->addItem(b.name, b.id);
        base->addItem(b.name, b.id);
    }
    selectData(moving, initial.movingBodyId);
    selectData(base, initial.baseBodyId);
    form->addRow(tr("动作体"), moving);
    form->addRow(tr("基本体"), base);
    const auto origin = vectorFields(form, tr("原点 (mm)"), initial.originMm);
    const auto axis = kind == JointKind::Fixed ? std::array<QDoubleSpinBox*, 3>{}
                                              : vectorFields(form, tr("轴向"), initial.axis);
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        auto joint = initial;
        joint.name = name->text().trimmed();
        joint.movingBodyId = moving->currentData().toInt();
        joint.baseBodyId = base->currentData().toInt();
        joint.originMm = vectorValue(origin);
        if (kind != JointKind::Fixed) joint.axis = vectorValue(axis);
        replace(model.joints, joint);
        commitRigid(std::move(model), {ModelNode::Joint, joint.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::editDrive(DriveKind kind, int id)
{
    if (m_model.nativeDocument) {
        editNativeEntity(QStringLiteral("drives"), id, kind == DriveKind::Speed ? QStringLiteral("Velocity") : QStringLiteral("Displacement"));
        return;
    }
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.drives, id);
    const RigidDrive initial = current ? *current : RigidDrive{nextId(m_model.drives), {}, 0, kind, 0};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    const auto label = kind == DriveKind::Speed ? tr("速度驱动") : tr("位移驱动");
    form->addRow(new QLabel(label));
    auto* name = new QLineEdit(initial.name.isEmpty() ? label + QStringLiteral("-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* joint = new QComboBox;
    for (const auto& j : m_model.joints) {
        if (j.kind == JointKind::Fixed) continue;
        if (std::any_of(m_model.drives.begin(), m_model.drives.end(), [&](const auto& d) { return d.id != id && d.jointId == j.id; })) continue;
        joint->addItem(j.name, j.id);
    }
    selectData(joint, initial.jointId);
    form->addRow(tr("运动副"), joint);
    auto* value = number(form, kind == DriveKind::Speed ? tr("恒定速度") : tr("末时刻位移"), initial.value);
    auto showUnit = [=] {
        const auto* target = byId(m_model.joints, joint->currentData().toInt());
        if (target) value->setSuffix(target->kind == JointKind::Revolute
            ? (kind == DriveKind::Speed ? QStringLiteral(" °/s") : QStringLiteral(" °"))
            : (kind == DriveKind::Speed ? QStringLiteral(" mm/s") : QStringLiteral(" mm")));
    };
    connect(joint, qOverload<int>(&QComboBox::currentIndexChanged), panel, showUnit);
    showUnit();
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidDrive drive{initial.id, name->text().trimmed(), joint->currentData().toInt(), kind, value->value()};
        replace(model.drives, drive);
        commitRigid(std::move(model), {ModelNode::Drive, drive.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::editScenario(int id)
{
    if (m_model.nativeDocument) { editNativeEntity(QStringLiteral("scenarios"), id); return; }
    if (motionBusy() || m_jobPart >= 0) return;
    const auto* current = byId(m_model.scenarios, id);
    const RigidScenario initial = current ? *current : RigidScenario{nextId(m_model.scenarios)};
    auto* panel = new QWidget;
    auto* form = new QFormLayout(panel);
    form->addRow(new QLabel(tr("求解方案")));
    auto* name = new QLineEdit(initial.name.isEmpty() ? tr("求解方案-%1").arg(initial.id) : initial.name);
    name->setMaxLength(128);
    form->addRow(tr("名称"), name);
    auto* end = number(form, tr("终止时间 (s)"), initial.duration, 1e-6, 3600);
    auto* step = number(form, tr("步长 (s)"), initial.step, 1e-6, 10);
    const auto gravity = vectorFields(form, tr("重力 (m/s²)"), initial.gravityMS2);
    finishForm(form, this, [=] {
        RigidModel model = m_model;
        RigidScenario scenario{initial.id, name->text().trimmed(), end->value(), step->value(), vectorValue(gravity)};
        scenario.curveRequests = initial.curveRequests;
        replace(model.scenarios, scenario);
        commitRigid(std::move(model), {ModelNode::Scenario, scenario.id});
    }, [this] { closeEditor(); updateParameters(); });
    showEditor(panel);
}

void MotionWindow::removeRigidNode(ModelNode node)
{
    if (motionBusy()) return;
    if (m_model.nativeDocument) {
        removeNativeEntity(node.kind == ModelNode::Scenario ? QStringLiteral("scenarios") : node.collection, node.id);
        return;
    }
    RigidModel next = m_model;
    if (node.kind == ModelNode::Body) {
        if (std::any_of(next.joints.begin(), next.joints.end(), [&](const auto& j) { return j.movingBodyId == node.id || j.baseBodyId == node.id; })) {
            setNotice(tr("此运动体已用于运动副，请先删除相关运动副。"), true);
            return;
        }
        eraseIf(next.bodies, [&](const auto& b) { return b.id == node.id; });
    } else if (node.kind == ModelNode::Joint) {
        eraseIf(next.drives, [&](const auto& d) { return d.jointId == node.id; });
        eraseIf(next.joints, [&](const auto& j) { return j.id == node.id; });
    } else if (node.kind == ModelNode::Drive) eraseIf(next.drives, [&](const auto& d) { return d.id == node.id; });
    else if (node.kind == ModelNode::Scenario) eraseIf(next.scenarios, [&](const auto& sc) { return sc.id == node.id; });
    else return;
    for (auto& scenario : next.scenarios) {
        eraseIf(scenario.curveRequests, [&](const auto& request) {
            const bool joint = request.metric == RigidCurveMetric::JointCoordinate || request.metric == RigidCurveMetric::JointRate;
            return joint ? !byId(next.joints, request.sourceId) : !byId(next.bodies, request.sourceId);
        });
    }
    commitRigid(std::move(next), {});
}

void MotionWindow::commitRigid(RigidModel model, ModelNode selection)
{
    if (motionBusy() || m_jobPart >= 0) return;
    QString error;
    if (!validateRigidModel(m_document, model, error)) { setNotice(error, true); return; }
    closeEditor();
    clearResults();
    m_model = std::move(model);
    setProjectModified(true);
    m_selectionNode = selection;
    if (selection.kind != ModelNode::Part) { m_partId = -1; m_viewport->selectPart(-1); }
    updateTree();
    updateParameters();
    updateActions();
    refreshMotionSymbols();
    setNotice(tr("已更新运动模型。"));
}

void MotionWindow::clearResults()
{
    leaveRigidAnimation();
    m_rigidPlot->setResult(nullptr, nullptr);
    if(m_curveDock)if(auto* originalCurve=qobject_cast<MechMainWindow::mCurveWindow*>(m_curveDock->widget()))
        originalCurve->clearAllPlot();
    m_resultScenario = -1;
    m_playbackTime = 0.0;
    m_results.clear();
    if(m_original) {
        auto& scenarios=m_original->model()->getManager<MechMotionData::mMotionScenarioManager>(MxFunctions::Physical_MotionScenario);
        for(int id:scenarios.getAllIDs())if(auto* scenario=scenarios.getData(id))scenario->attachResult(nullptr);
    }
    m_viewport->clearPartPoses();
    const QSignalBlocker blocker(m_timeline);
    m_timeline->setRange(0, 0);
    m_timeLabel->setText(tr("时间: 0.000 s / 0.000 s"));
    m_animationDock->closeDockWidget();
    m_curveDock->closeDockWidget();
    updatePlaybackControls();
}

void MotionWindow::startRigidSolve(int scenarioId)
{
    if (motionBusy() || m_jobPart >= 0) { publishOriginalSolveFinished(scenarioId,false,tr("当前有求解或网格任务正在运行。"));return; }
    if (m_original) m_original->flush();
    snapshotNativeFlexMeshes();
    auto run = std::make_shared<RigidRun>();
    QString error;
    if (!prepareRigidModel(m_document, m_model, scenarioId, run->input, error)) { setNotice(error, true);publishOriginalSolveFinished(scenarioId,false,error);return; }
    if (!m_editor || !m_editor->property("mxOriginalMotionPanel").toBool()) closeEditor();
    leaveRigidAnimation();
    try {
        m_solverThread = std::thread([run] {
            try {
                run->status = solveRigid(run->input, run->result, run->error, {
                    [run] { return run->stop.load(std::memory_order_relaxed); },
                    [run](double value) { run->progress.store(value, std::memory_order_relaxed); }});
            } catch (const std::exception& e) { run->error = QString::fromUtf8(e.what()); }
            catch (...) { run->error = QStringLiteral("MotionEngine 求解异常。"); }
            run->done.store(true, std::memory_order_release);
        });
    } catch (const std::exception& e) { const auto error=QString::fromUtf8(e.what());setNotice(error,true);publishOriginalSolveFinished(scenarioId,false,error);return; }
    m_running = std::move(run);
    m_runningInputChanged = false;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->show();
    m_rigidPoll->start();
    updateActions();
    setNotice(tr("正在求解…"));
}

void MotionWindow::pollRigidSolve()
{
    auto run = m_running;
    if (!run) return;
    m_progress->setValue(int(std::clamp(run->progress.load(), 0.0, 1.0) * 100));
    if(run->inspectionFlexId<0)publishOriginalSolveProgress(run->input.scenario.id,m_progress->value());
    if (!run->done.load(std::memory_order_acquire)) return;
    if (run->inspectionFlexId >= 0 && !m_projectActive) {
        m_rigidPoll->stop();
        return;
    }
    m_solverThread.join();
    m_running.reset();
    m_rigidPoll->stop();
    m_progress->hide();
    m_progress->setRange(0, 0);
    if(m_runningInputChanged) {
        m_runningInputChanged=false;
        const auto message=tr("模型输入已更改，已停止旧输入的求解。修改已保留，请重新求解。");
        setNotice(message,true);
        if(run->inspectionFlexId<0)publishOriginalSolveFinished(run->input.scenario.id,false,message);
        updateParameters();updateActions();return;
    }
    if(run->inspectionFlexId>=0){
        if(run->status==SolveStatus::Completed && !run->result.flexBodies.empty()){
            const auto& basis=run->result.flexBodies.front().modalBasis;
            setNotice(tr("真实模态检查完成：请求 %1 阶，保留 %2 阶。").arg(basis.requestedCount).arg(basis.frequenciesHz.size()));
            openNativeModeInspector(this,m_document,run->input.nativeInput->document,run->result.flexBodies);
        }else setNotice(run->status==SolveStatus::Stopped?tr("已停止模态检查。"):tr("模态检查失败：%1").arg(run->error),run->status!=SolveStatus::Stopped);
        updateParameters();updateActions();return;
    }
    if (run->status == SolveStatus::Completed && !run->result.frames.empty()) {
        const int id = run->result.scenarioId;
        // Replacing this result must release the plot's pointers first. A solve
        // does not implicitly enter animation or create a result curve.
        if (m_resultScenario == id) {
            m_rigidPlot->setResult(nullptr, nullptr);
            m_resultScenario = -1;
        }
        m_results[id] = run;
        m_selectionNode = {ModelNode::Scenario, id};
        updateTree();
        const auto message = run->result.termination == ResultTermination::StaticEquilibrium
            ? tr("静力平衡完成。已保存平衡构型与载荷，可打开动画查看变形或导出结果。")
            : run->result.termination != ResultTermination::EndTime
            ? (run->result.termination == ResultTermination::FlexDivergence
                ? tr("求解因柔性体模态发散终止：已保留 %1 帧 / %2 s 的有效结果。")
                : tr("求解因到达几何边界终止：已保留 %1 帧 / %2 s 的有效结果。"))
                .arg(run->result.frames.size()).arg(run->result.frames.back().seconds)
            : tr("求解完成：%1 帧 / %2 s。请从方案下打开动画或运动曲线。")
                .arg(run->result.frames.size()).arg(run->result.frames.back().seconds);
        const bool truncated = run->result.termination != ResultTermination::EndTime
            && run->result.termination != ResultTermination::StaticEquilibrium;
        setStatusNotice(message);
        appendOutput(message, truncated ? NoticeLevel::Warning : NoticeLevel::Normal);
        for (const auto& diagnostic : run->result.diagnostics) appendOutput(diagnostic);
        publishOriginalSolveFinished(id,true,message);
        if (!run->result.diagnostics.isEmpty()) openDock(m_outputDock);
    } else {
        setNotice(run->status == SolveStatus::Stopped ? tr("已停止求解。") : tr("求解失败：%1").arg(run->error), run->status != SolveStatus::Stopped);
        publishOriginalSolveFinished(run->input.scenario.id,false,run->status==SolveStatus::Stopped ? tr("已停止求解。") : run->error);
    }
    updateParameters();
    updateActions();
}

int MotionWindow::scenarioForResultCommand() const
{
    if (motionBusy() || m_jobPart >= 0) return -1;
    if (m_selectionNode.kind >= ModelNode::Scenario && m_selectionNode.kind <= ModelNode::Curves)
        return m_results.count(m_selectionNode.id) ? m_selectionNode.id : -1;
    // 没有选中求解方案时，只能使用项目中唯一一份已完成的结果。
    return m_results.size() == 1 ? m_results.begin()->first : -1;
}

void MotionWindow::showRigidResult(ModelNode node)
{
    if (showOriginalMotionResult(node)) return;
    const auto it = m_results.find(node.id);
    if (motionBusy() || m_jobPart >= 0 || it == m_results.end() || it->second->result.frames.empty()) return;
    const auto& run = *it->second;
    if (node.kind == ModelNode::Animation) {
        QString error;
        if (!validateRigidPlayback(run.input, run.result, error)) { setNotice(error, true); return; }
    }
    if (node.kind == ModelNode::Export) {
        if (run.input.nativeInput) { editNativeLoadExport(node.id); return; }
        leaveRigidAnimation();
        auto path = QFileDialog::getSaveFileName(this, tr("导出求解结果"), run.input.scenario.name + ".csv", tr("CSV 表格 (*.csv)"));
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += ".csv";
        QString error;
        if (exportRigidCsv(path, run.input, run.result, error)) setNotice(tr("已导出结果：%1").arg(QFileInfo(path).fileName()));
        else setNotice(error, true);
        return;
    }
    if (m_resultScenario != node.id) {
        leaveRigidAnimation();
        m_resultScenario = node.id;
        m_rigidPlot->setResult(&run.input, &run.result);
        m_playbackTime = run.result.frames.front().seconds;
    }
    if (node.kind == ModelNode::Curves) {
        showEditor(m_rigidPlot->createConfigurationPanel());
        if (m_rigidPlot->hasVisibleCurves()) openDock(m_curveDock);
        return;
    }
    if (node.kind != ModelNode::Animation) return;
    closeEditor();
    setPlayback(false);
    m_animationActive = true;
    {
        const QSignalBlocker blocker(m_timeline);
        m_timeline->setRange(0, int(run.result.frames.size()) - 1);
    }
    m_rigidPlot->setTimeMarkerVisible(m_followCurveMarker->isChecked());
    seekRigidTime(run.result.frames.front().seconds);
    openDock(m_animationDock);
    if (m_rigidPlot->hasVisibleCurves()) openDock(m_curveDock);
    updatePlaybackControls();
}

void MotionWindow::seekRigidTime(double seconds)
{
    const auto it = m_results.find(m_resultScenario);
    if (!m_animationActive || it == m_results.end()) return;
    const auto& run = *it->second;
    const auto& frames = run.result.frames;
    RigidPlaybackSample sample;
    QString error;
    if (!sampleRigidPlayback(run.input, run.result, seconds, sample, error)) {
        setPlayback(false);
        setNotice(error, true);
        return;
    }
    if (run.input.nativeInput) for (const auto& flex : run.result.flexBodies) {
        const auto parts = run.input.nativeInput->document.bodyPartBindings.value(flex.bodyId);
        if (parts.isEmpty() || !m_hidden.count(parts.front())) continue;
        const auto visible = std::find_if(parts.begin(), parts.end(), [this](int part) { return !m_hidden.count(part); });
        auto mesh = sample.flexMeshes.extract(parts.front());
        if (visible != parts.end() && !mesh.empty()) {
            mesh.key() = *visible; sample.flexMeshes.insert(std::move(mesh));
            sample.flexHiddenParts.erase(*visible); sample.flexHiddenParts.insert(parts.front());
        }
    }
    m_viewport->setPartPoses(sample.partPoses);
    if (run.input.nativeInput) m_viewport->setMotionSymbols(nativeMotionSymbols(m_document,run.input.nativeInput->document,sample.partPoses,{},-1,sample.bodyPoses));
    m_viewport->setDeformedMeshes(sample.flexMeshes, sample.flexHiddenParts);
    if (m_flexContour->currentData().toInt()>=0 && !run.result.flexBodies.empty()) {
        const auto kind=FlexContourKind(m_flexContour->currentData().toInt());
        const bool globalRange = m_flexRange->currentData().toInt() == 1;
        std::map<int,std::vector<double>> scalars;
        const double infinity = std::numeric_limits<double>::infinity();
        double minimum = infinity, maximum = -infinity;
        const auto next=std::upper_bound(frames.begin(),frames.end(),sample.seconds,[](double t,const RigidFrame& f) { return t<f.seconds; });
        const auto left=next==frames.begin() ? std::size_t(0) : std::size_t(next-frames.begin()-1);
        const auto right=std::min(left+1,frames.size()-1);
        const double fraction=right==left ? 0 : (sample.seconds-frames[left].seconds)/(frames[right].seconds-frames[left].seconds);
        for (const auto& flex:run.result.flexBodies) {
            if (kind == FlexContourKind::LegacyFreeModeProjection && flex.modalBasis.kind == "MNF") continue;
            const auto parts=run.input.nativeInput->document.bodyPartBindings.value(flex.bodyId);
            const auto visible=std::find_if(parts.begin(),parts.end(),[&](int part) { return sample.flexMeshes.count(part)!=0; });
            if (visible==parts.end()) continue;
            std::vector<double> values;
            if (!flexNodeScalarsAt(flex,left,right,fraction,kind,values,error)) {
                setPlayback(false); setNotice(error,true); m_viewport->clearMeshScalars(); return;
            }
            const auto [first, last] = std::minmax_element(values.begin(), values.end());
            double bodyMinimum = *first, bodyMaximum = *last;
            if (globalRange) {
                const auto key = std::make_pair(flex.bodyId, int(kind));
                auto cached = m_flexGlobalRanges.find(key);
                if (cached == m_flexGlobalRanges.end()) {
                    double low = infinity, high = -infinity;
                    std::vector<double> frame;
                    for (std::size_t i=0;i<frames.size();++i) {
                        if (!flexNodeScalars(flex,i,kind,frame,error)) {
                            setPlayback(false); setNotice(error,true); m_viewport->clearMeshScalars(); return;
                        }
                        const auto [minNode, maxNode] = std::minmax_element(frame.begin(), frame.end());
                        low = std::min(low, *minNode);
                        high = std::max(high, *maxNode);
                    }
                    cached = m_flexGlobalRanges.emplace(key, std::make_pair(low, high)).first;
                }
                bodyMinimum = cached->second.first;
                bodyMaximum = cached->second.second;
            }
            minimum = std::min(minimum, bodyMinimum);
            maximum = std::max(maximum, bodyMaximum);
            scalars.emplace(*visible,std::move(values));
        }
        if (scalars.empty()) m_viewport->clearMeshScalars();
        else m_viewport->setMeshScalars(scalars,minimum,maximum,m_flexContour->currentText());
    } else m_viewport->clearMeshScalars();
    m_playbackTime = sample.seconds;
    const QSignalBlocker block(m_timeline);
    m_timeline->setValue(sample.frameIndex);
    m_timeLabel->setText(tr("时间: %1 s / %2 s").arg(sample.seconds, 0, 'f', 3).arg(frames.back().seconds, 0, 'f', 3));
    if (m_followCurveMarker->isChecked()) m_rigidPlot->setTime(sample.seconds);
}

void MotionWindow::setPlayback(bool play)
{
    if (!m_playbackTimer) return;
    if (play) {
        const auto it = m_results.find(m_resultScenario);
        if (!m_animationActive || motionBusy() || m_jobPart >= 0 || it == m_results.end()
            || it->second->result.frames.size() < 2) return;
        const auto& frames = it->second->result.frames;
        if (frames.back().seconds <= frames.front().seconds) return;
        if (m_playbackTime >= frames.back().seconds) seekRigidTime(frames.front().seconds);
        m_playbackClock.restart();
        m_playbackTimer->start();
    } else m_playbackTimer->stop();
    updatePlaybackControls();
}

void MotionWindow::stopRigidPlayback()
{
    setPlayback(false);
    const auto it = m_results.find(m_resultScenario);
    if (m_animationActive && it != m_results.end() && !it->second->result.frames.empty())
        seekRigidTime(it->second->result.frames.front().seconds);
}

void MotionWindow::refreshMotionSymbols()
{
    if (!m_viewport) return;
    m_viewport->setMotionSymbols(m_model.nativeDocument
        ? nativeMotionSymbols(m_document, *m_model.nativeDocument, {}, m_selectionNode.collection, m_selectionNode.id)
        : QVector<MotionSymbol>{});
}

void MotionWindow::leaveRigidAnimation()
{
    m_flexGlobalRanges.clear();
    if (m_viewport) m_viewport->clearMeshScalars();
    setPlayback(false);
    if (m_animationActive) {
        const auto it = m_results.find(m_resultScenario);
        if (it != m_results.end() && !it->second->result.frames.empty()) {
            const auto& frames = it->second->result.frames;
            m_playbackTime = frames.front().seconds;
            const QSignalBlocker blocker(m_timeline);
            m_timeline->setValue(0);
            m_timeLabel->setText(tr("时间: %1 s / %2 s").arg(m_playbackTime, 0, 'f', 3).arg(frames.back().seconds, 0, 'f', 3));
        }
    }
    m_animationActive = false;
    if (m_viewport) m_viewport->clearPartPoses();
    refreshMotionSymbols();
    if (m_rigidPlot) m_rigidPlot->setTimeMarkerVisible(false);
    if (m_animationDock && !m_animationDock->isClosed()) m_animationDock->closeDockWidget();
    updatePlaybackControls();
}

void MotionWindow::updatePlaybackControls()
{
    if (!m_playToggle) return;
    const auto it = m_results.find(m_resultScenario);
    const bool available = m_animationActive && !motionBusy() && m_jobPart < 0
        && it != m_results.end() && !it->second->result.frames.empty();
    const bool playing = available && m_playbackTimer && m_playbackTimer->isActive();
    const bool multiple = available && it->second->result.frames.size() > 1;
    m_playToggle->setEnabled(multiple && !playing);
    m_pausePlayback->setEnabled(playing);
    m_stopPlayback->setEnabled(available);
    m_timeline->setEnabled(multiple);
    m_playbackSpeed->setEnabled(available);
    m_loopPlayback->setEnabled(multiple);
    m_followCurveMarker->setEnabled(available);
    m_clickCurveJump->setEnabled(available);
    if(m_modeInspectButton)m_modeInspectButton->setEnabled(available && !it->second->result.flexBodies.empty()
        && std::any_of(it->second->result.flexBodies.begin(),it->second->result.flexBodies.end(),[](const auto& flex){return !flex.modalBasis.frequenciesHz.empty();}));
}

bool MotionWindow::saveProject(bool choosePath)
{
    if(m_original)m_original->flush();
    snapshotNativeFlexMeshes();
    QString path = m_projectPath;
    if (choosePath || path.isEmpty()) path = QFileDialog::getSaveFileName(window(), tr("保存项目"),
        path.isEmpty() ? QStringLiteral("model.mxmotion") : path, tr("MxMotion 项目 (*.mxmotion)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(QStringLiteral(".mxmotion"), Qt::CaseInsensitive)) path += ".mxmotion";
    QString error;
    if (!saveRigidProject(path, m_document, m_model, error)) { setNotice(error, true); return false; }
    m_projectPath = path;
    setProjectModified(false);
    setNotice(tr("已保存项目：%1").arg(QFileInfo(path).fileName()));
    return true;
}

bool MotionWindow::openRigidProject(const QString& path)
{
    if (!confirmReplace() || m_jobPart >= 0) return false;
    CadDocument cad;
    RigidModel model;
    QString error;
    if (!loadRigidProject(path, cad, model, error)) { setNotice(error, true); return false; }
    closeEditor();
    clearResults();
    m_original.reset();
    m_document = std::move(cad);
    m_model = std::move(model);
    m_meshes.clear();
    restoreNativeFlexMeshes();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath = path;
    setProjectModified(false);
    m_selectionNode = {};
    m_partId = -1;
    updateTree();
    syncViewport(true);
    selectPart(-1);
    for (const auto& note : m_document.importNotes()) appendOutput(note);
    setNotice(tr("已打开项目：%1").arg(QFileInfo(path).fileName()));
    return true;
}

void MotionWindow::loadRigidExample()
{
    if (!confirmReplace() || m_jobPart >= 0) return;
    CadDocument cad;
    int part = 0;
    QString error;
    if (!cad.addPrimitive(Primitive::Box, 80, 14, 12, part, error)) { setNotice(error, true); return; }
    closeEditor();
    clearResults();
    m_document = std::move(cad);
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath.clear();
    RigidModel model;
    model.bodies.push_back({1, tr("摇臂"), part, false, 1, 0});
    model.joints.push_back({1, tr("摇臂铰链"), JointKind::Revolute, 1, 0, {0, 7, 6}, {0, 0, 1}});
    model.drives.push_back({1, tr("恒速驱动"), 1, DriveKind::Speed, 90});
    model.scenarios.push_back({1, tr("刚体示例 · 90°/s"), 1.005, 0.01, {0, 0, 0}});
    commitRigid(std::move(model), {ModelNode::Scenario, 1});
    syncViewport(true);
    setNotice(tr("已载入刚体示例。点击「求解」运行。"));
}
} // namespace MotionOcc
