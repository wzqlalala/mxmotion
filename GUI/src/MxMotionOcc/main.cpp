#include "MotionWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QEventLoop>
#include <QLocale>
#include <QMessageBox>
#include <QResource>
#include <QScreen>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTimer>
#include <QTranslator>
#include <QWidget>
#include <algorithm>

#ifdef MX_OCC_DESKTOP_SMOKE
int runMotionOccDesktopSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccNativeDesktopSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccPickingSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccOriginalPickerSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccOriginalBackendSmoke();
int runMotionOccOriginalPlotSmoke(QApplication& app);
int runMotionOccOriginalNativeCurvesSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccOriginalLoadExportSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccModalDesktopSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccNavigationSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccRenderFeedbackSmoke(QApplication& app, const QString& outputDirectory);

namespace {
class HiddenSmokeWindows final : public QObject
{
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (event->type() == QEvent::Polish || event->type() == QEvent::Show) {
            if (auto* widget = qobject_cast<QWidget*>(object); widget && widget->isWindow()) {
                widget->setAttribute(Qt::WA_DontShowOnScreen);
                widget->setAttribute(Qt::WA_ShowWithoutActivating);
            }
        }
        return false;
    }
};
}
#endif

int main(int argc, char* argv[])
{
    // The OCC profile supplies a desktop core viewport, not the full SDK's
    // OSG, licensing, post-processing or solver startup path.
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("MxSim"));
    app.setApplicationName(QStringLiteral("MxMotionOcc"));
    app.setApplicationVersion(QStringLiteral("1.0"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    QCommandLineParser args;
    args.setApplicationDescription(QStringLiteral("MxSim.Motion — OCC / OpenGL / Netgen"));
    args.addHelpOption();
    args.addVersionOption();
    args.addPositionalArgument(QStringLiteral("CAD"), QStringLiteral("MxMotion project, STEP, IGES or BREP."), QStringLiteral("[CAD]"));
    const QCommandLineOption motion(QStringLiteral("motion"), QStringLiteral("Import the original Motion JSON after the CAD file."), QStringLiteral("file"));
    args.addOption(motion);
#ifdef MX_OCC_DESKTOP_SMOKE
    const QCommandLineOption smoke(QStringLiteral("desktop-smoke"), QStringLiteral("Run the local desktop smoke test."), QStringLiteral("directory"));
    args.addOption(smoke);
    const QCommandLineOption nativeSmoke(QStringLiteral("native-desktop-smoke"), QStringLiteral("Run the native motion desktop regression."), QStringLiteral("directory"));
    args.addOption(nativeSmoke);
    const QCommandLineOption pickingSmoke(QStringLiteral("picking-smoke"), QStringLiteral("Run CAD and mesh picking regression."), QStringLiteral("directory"));
    args.addOption(pickingSmoke);
    const QCommandLineOption originalPickerSmoke(QStringLiteral("original-picker-smoke"), QStringLiteral("Run the uploaded original picker controls against real geometry and meshes."), QStringLiteral("directory"));
    args.addOption(originalPickerSmoke);
    const QCommandLineOption originalBackendSmoke(QStringLiteral("original-backend-smoke"), QStringLiteral("Run original data and real mesh/solver compatibility regression."));
    const QCommandLineOption originalPlotSmoke(QStringLiteral("original-plot-smoke"), QStringLiteral("Run the original CurveWindow against real Qwt."));
    args.addOption(originalBackendSmoke);
    args.addOption(originalPlotSmoke);
    const QCommandLineOption originalNativeCurvesSmoke(QStringLiteral("original-native-curves-smoke"), QStringLiteral("Verify saved native channels in the original curve panel."), QStringLiteral("directory"));
    const QCommandLineOption originalLoadExportSmoke(QStringLiteral("original-load-export-smoke"), QStringLiteral("Verify the original load-export tabs against real accepted results."), QStringLiteral("directory"));
    args.addOption(originalNativeCurvesSmoke);
    args.addOption(originalLoadExportSmoke);
    const QCommandLineOption modalSmoke(QStringLiteral("modal-desktop-smoke"),QStringLiteral("Run real modal inspector regression."),QStringLiteral("directory"));
    args.addOption(modalSmoke);
    const QCommandLineOption navigationSmoke(QStringLiteral("navigation-smoke"),QStringLiteral("Run real viewport navigation regression."),QStringLiteral("directory"));
    const QCommandLineOption renderFeedbackSmoke(QStringLiteral("render-feedback-smoke"),QStringLiteral("Run CAD selection depth regression."),QStringLiteral("directory"));
    args.addOption(navigationSmoke);
    args.addOption(renderFeedbackSmoke);
#endif
    args.process(app);
    bool smokeMode = false;
#ifdef MX_OCC_DESKTOP_SMOKE
    HiddenSmokeWindows hiddenWindows;
    const auto optionNames = args.optionNames();
    smokeMode = std::any_of(optionNames.cbegin(), optionNames.cend(), [](const QString& name) {
        return name.endsWith(QStringLiteral("-smoke"));
    });
    if (smokeMode) {
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        app.installEventFilter(&hiddenWindows);
    }
#endif
    const auto resources = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("mMainWidgets.rcc"));
    if (!QResource::registerResource(resources)) {
        const auto message = QStringLiteral("缺少界面资源。请从完整的 bin 目录运行 MxMotion.exe。");
        if (smokeMode) qCritical().noquote() << message;
        else QMessageBox::critical(nullptr, QStringLiteral("MxSim.Motion"), message);
        return 2;
    }
    QTranslator qt;
    if (qt.load(QStringLiteral(":/mMainWidgets/Translations/qt_zh_CN.qm"))) app.installTranslator(&qt);
    QTranslator ads;
    if (ads.load(QStringLiteral(":/mMainWidgets/Translations/ads_zh_CN.qm"))) app.installTranslator(&ads);

#ifdef MX_OCC_DESKTOP_SMOKE
    if (args.isSet(smoke)) return runMotionOccDesktopSmoke(app, args.value(smoke));
    if (args.isSet(nativeSmoke)) return runMotionOccNativeDesktopSmoke(app, args.value(nativeSmoke));
    if (args.isSet(pickingSmoke)) return runMotionOccPickingSmoke(app, args.value(pickingSmoke));
    if (args.isSet(originalPickerSmoke)) return runMotionOccOriginalPickerSmoke(app,args.value(originalPickerSmoke));
    if (args.isSet(originalBackendSmoke)) return runMotionOccOriginalBackendSmoke();
    if (args.isSet(originalPlotSmoke)) return runMotionOccOriginalPlotSmoke(app);
    if (args.isSet(originalNativeCurvesSmoke)) return runMotionOccOriginalNativeCurvesSmoke(app,args.value(originalNativeCurvesSmoke));
    if (args.isSet(originalLoadExportSmoke)) return runMotionOccOriginalLoadExportSmoke(app,args.value(originalLoadExportSmoke));
    if (args.isSet(modalSmoke)) return runMotionOccModalDesktopSmoke(app,args.value(modalSmoke));
    if (args.isSet(navigationSmoke)) return runMotionOccNavigationSmoke(app,args.value(navigationSmoke));
    if (args.isSet(renderFeedbackSmoke)) return runMotionOccRenderFeedbackSmoke(app,args.value(renderFeedbackSmoke));
#endif

    auto* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    const auto size = screen->availableGeometry().size();
    const QPixmap artwork(QStringLiteral(":/mMainWidgets/Icon/start_Motion.png"));
    QSplashScreen splash(screen, artwork.scaled(size.width() * 7 / 24, size.height() * 5 / 12,
        Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::WindowDoesNotAcceptFocus);
    splash.setAttribute(Qt::WA_ShowWithoutActivating);
    QElapsedTimer startup;
    startup.start();
    splash.show();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    MotionOcc::MotionWindow window;
    if (!args.positionalArguments().isEmpty()) {
        const auto path = args.positionalArguments().first();
        if (path.endsWith(QStringLiteral(".mxmotion"), Qt::CaseInsensitive)) window.openRigidProject(path);
        else window.importCad(path);
    }
    if (args.isSet(motion)) window.importMotionJson(args.value(motion));
    QTimer::singleShot(int(qMax<qint64>(0, 750 - startup.elapsed())), &window, [&] {
        window.showMaximized();
        splash.finish(&window);
    });
    return app.exec();
}
