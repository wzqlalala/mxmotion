#include "MotionWindow.h"
#include "OccViewport.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVector3D>
#include <array>
#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

bool waitForOpenGl(const MotionOcc::OccViewport& viewport)
{
    if (viewport.glReady()) return true;
    QEventLoop wait;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &wait, [&] {
        if (viewport.glReady()) wait.quit();
    });
    QTimer::singleShot(10000, &wait, &QEventLoop::quit);
    poll.start(20);
    wait.exec(QEventLoop::ExcludeUserInputEvents);
    return viewport.glReady();
}

void focusCamera(MotionOcc::OccViewport& viewport, const QString& focus)
{
    const auto fields = focus.split(QLatin1Char(','));
    require(fields.size() == 4, QStringLiteral("聚焦参数必须是 x,y,z,span"));
    std::array<double, 4> values{};
    for (int i = 0; i < fields.size(); ++i) {
        bool ok = false;
        values[i] = fields.at(i).toDouble(&ok);
        require(ok && std::isfinite(values[i]), QStringLiteral("聚焦参数不是有限数值：%1").arg(fields.at(i)));
    }
    const double span = values[3];
    require(span > 0, QStringLiteral("聚焦范围必须为正数"));
    const std::array<double, 3> target{values[0], values[1], values[2]};
    const auto pixel = viewport.projectPoint(target);
    viewport.panView(QStringLiteral("right"), 200.0 * (0.5 - pixel.x() / viewport.width()));
    viewport.panView(QStringLiteral("down"), 200.0 * (0.5 - pixel.y() / viewport.height()));
    const double currentSpan = viewport.queryCameraState().ortho_zoom;
    viewport.zoomView(currentSpan >= span, currentSpan >= span ? currentSpan / span : span / currentSpan);
}

} // namespace

int runMotionOccCadImportSmoke(const QString& outputDirectory, const QString& cadPath,
                              const QStringList& expectedParts, const QString& focus, const QString& motionPath)
{
    using namespace MotionOcc;
    const QDir output(outputDirectory);
    if (!QDir().mkpath(output.absolutePath())) return 2;
    QFile report(output.filePath(QStringLiteral("cad-import-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) return 2;
    QTextStream log(&report);
    log.setCodec("UTF-8");

    try {
        const QFileInfo input(cadPath);
        require(input.isFile(), QStringLiteral("缺少 CAD 文件：%1").arg(cadPath));
        log << "path=" << input.absoluteFilePath() << "\nfile_bytes=" << input.size() << '\n';
        log.flush();

        QString graphicsError;
        MotionWindow window;
        window.resize(1600, 1000);
        window.show();
        auto* viewport = window.findChild<OccViewport*>(QStringLiteral("occViewport"));
        auto* tree = window.findChild<QTreeWidget*>(QStringLiteral("mTreeView"));
        require(viewport && tree, QStringLiteral("找不到 CAD 视口或模型树"));
        QObject::connect(viewport, &OccViewport::graphicsError, &window,
                         [&graphicsError](const QString& message) { graphicsError = message; });
        require(waitForOpenGl(*viewport), QStringLiteral("OpenGL 初始化失败：%1").arg(graphicsError));
        const QImage empty = viewport->grabFramebuffer();
        require(!empty.isNull(), QStringLiteral("无法获取导入前的 OpenGL 帧"));

        QElapsedTimer timer;
        timer.start();
        QEventLoop wait;
        QTimer heartbeat;
        int ticks = 0;
        qint64 lastTick = 0, longestGap = 0;
        bool done = false, imported = false;
        QObject::connect(&heartbeat, &QTimer::timeout, &wait, [&] {
            const auto now = timer.elapsed();
            longestGap = qMax(longestGap, now - lastTick);
            lastTick = now;
            ++ticks;
        });
        QObject::connect(&window, &MotionWindow::geometryImportFinished, &wait, [&](bool success) {
            done = true;
            imported = success;
            wait.quit();
        });
        const bool started = motionPath.isEmpty() ? window.startCadImport(input.absoluteFilePath())
                                                  : window.startMotionJsonImport(motionPath);
        const auto dispatch = timer.elapsed();
        require(started && window.geometryImportInProgress() && !window.command(Command::Import)->isEnabled(),
            QStringLiteral("CAD 导入没有进入后台任务状态"));
        heartbeat.start(30);
        QTimer::singleShot(600000, &wait, &QEventLoop::quit);
        wait.exec();
        heartbeat.stop();
        longestGap = qMax(longestGap, timer.elapsed() - lastTick);
        log << "dispatch_ms=" << dispatch << "\nimport_ms=" << timer.elapsed()
            << "\nui_heartbeat=" << ticks << "\nui_max_gap_ms=" << longestGap << '\n';
        require(done && !window.geometryImportInProgress(), QStringLiteral("等待后台 CAD 导入超时"));
        if (timer.elapsed() > 2000)
            require(dispatch < 1000 && ticks >= 10 && longestGap < 2000,
                QStringLiteral("大型 CAD 导入期间界面事件循环长时间停顿"));
        const auto* console = window.findChild<QTextEdit*>(QStringLiteral("motionOutput"));
        const QString notices = console ? console->toPlainText().right(4000) : QString{};
        if (!notices.isEmpty()) log << notices << '\n';
        require(imported, QStringLiteral("CAD 导入失败"));

        int partCount = 0;
        for (QTreeWidgetItemIterator item(tree); *item; ++item)
            if ((*item)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Part)) ++partCount;
        require(partCount > 0, QStringLiteral("模型树没有导入的部件"));
        log << "parts=" << partCount << '\n';
        for (const auto& name : expectedParts) {
            QTreeWidgetItemIterator item(tree);
            for (; *item; ++item) {
                if ((*item)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Part) && (*item)->text(0) == name)
                    break;
            }
            require(*item, QStringLiteral("没有找到部件：%1").arg(name));
            log << "part=" << name << " id=" << (*item)->data(0, Qt::UserRole).toInt() << '\n';
            if (name == expectedParts.front()) tree->scrollToItem(*item);
        }
        window.command(Command::ClearSelection)->trigger();
        QVector3D minimum, maximum;
        require(viewport->queryVisibleBounds(minimum, maximum), QStringLiteral("导入后视口没有显示边界"));
        const auto size = maximum - minimum;
        require(std::isfinite(size.length()) && size.length() > 0, QStringLiteral("导入后视口显示边界无效"));
        log << "bounds_mm=" << size.x() << ',' << size.y() << ',' << size.z() << '\n';
        log.flush();

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        const QImage frame = viewport->grabFramebuffer();
        require(graphicsError.isEmpty(), QStringLiteral("OpenGL 绘制失败：%1").arg(graphicsError));
        require(!frame.isNull() && frame != empty, QStringLiteral("导入后的 OpenGL 帧未显示几何"));
        require(frame.save(output.filePath(QStringLiteral("cad-viewport.png"))), QStringLiteral("无法保存视口截图"));
        require(window.grab().save(output.filePath(QStringLiteral("cad-window.png"))), QStringLiteral("无法保存窗口截图"));
        if (!focus.isEmpty()) {
            focusCamera(*viewport, focus);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            const QImage focused = viewport->grabFramebuffer();
            require(!focused.isNull() && focused != frame, QStringLiteral("聚焦后的 OpenGL 帧未更新"));
            require(graphicsError.isEmpty(), QStringLiteral("聚焦后的 OpenGL 绘制失败：%1").arg(graphicsError));
            require(focused.save(output.filePath(QStringLiteral("cad-focus-viewport.png"))), QStringLiteral("无法保存聚焦截图"));
            require(window.grab().save(output.filePath(QStringLiteral("cad-focus-window.png"))), QStringLiteral("无法保存聚焦窗口截图"));
            log << "focus_mm=" << focus << '\n';
        }
        log << "PASS CAD import, model tree and OpenGL frame\n";
        return 0;
    } catch (const std::exception& error) {
        log << "FAIL " << QString::fromUtf8(error.what()) << '\n';
        return 1;
    }
}
