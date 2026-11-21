#include "MotionWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QResource>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QTranslator>

#ifdef MX_OCC_DESKTOP_SMOKE
int runMotionOccDesktopSmoke(QApplication& app, const QString& outputDirectory);
int runMotionOccNativeDesktopSmoke(QApplication& app, const QString& outputDirectory);
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
    const auto resources = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("mMainWidgets.rcc"));
    if (!QResource::registerResource(resources)) {
        QMessageBox::critical(nullptr, QStringLiteral("MxSim.Motion"),
            QStringLiteral("缺少界面资源。请从完整的 bin 目录运行 MxMotion.exe。"));
        return 2;
    }
    QTranslator qt;
    if (qt.load(QStringLiteral(":/mMainWidgets/Translations/qt_zh_CN.qm"))) app.installTranslator(&qt);
    QTranslator ads;
    if (ads.load(QStringLiteral(":/mMainWidgets/Translations/ads_zh_CN.qm"))) app.installTranslator(&ads);

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
#endif
    args.process(app);
#ifdef MX_OCC_DESKTOP_SMOKE
    if (args.isSet(smoke)) return runMotionOccDesktopSmoke(app, args.value(smoke));
    if (args.isSet(nativeSmoke)) return runMotionOccNativeDesktopSmoke(app, args.value(nativeSmoke));
#endif

    MotionOcc::MotionWindow window;
    if (!args.positionalArguments().isEmpty()) {
        const auto path = args.positionalArguments().first();
        if (path.endsWith(QStringLiteral(".mxmotion"), Qt::CaseInsensitive)) window.openRigidProject(path);
        else window.importCad(path);
    }
    if (args.isSet(motion)) window.importMotionJson(args.value(motion));
    window.showMaximized();
    return app.exec();
}
