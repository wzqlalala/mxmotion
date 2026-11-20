//解决中文乱码
#pragma execution_character_set("utf-8")
#if defined(_WIN32) || defined(_WIN64)
#include "winsock_config.h"
#include <QFile>
//【关闭挂起探针·临时】追加写关闭链路日志（定位完成后整体删除）
static void shutdownTrace(const QString& tag) {
    QFile file(QStringLiteral("E:/Mech_GUI/shutdown_trace.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        file.write(tag.toUtf8());
        file.write(QByteArray(1, 0x0A));
        file.close();
    }
}
#endif
//MxMech
#include "mAppInitializer.h"
#include "mOpenSoftWare.h"
#include "mLicense.h"
#include "mClient.h"
#include "mAppConfiguration.h"
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"
#include "mLoginDialog.h"
#include "mMechMainWindow.h"
#include "TaskbarRelaunch.h"
//UI截图工具模式（--ui-shot，界面对照自动化，见 Auto_Postpage skill §1.2）
#include "mUiShotController.h"
#include "mMpcommBridge.h"
#include "mMechGlobalSignals.h"
//UI截图工具模式（--ui-shot，界面对照自动化，见 Auto_Postpage skill §1.2）
#include "mUiShotController.h"
//QT
#include <QApplication>
#include <QStyleFactory>
#include <QWidget>
#include <QProcess>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStyleFactory>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QIcon>
#include <qprocess.h>
#include <QDateTime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QFontDatabase>
#include "UiMetrics.h"

#include <QWebEngineView>
#include <qprocess.h>
#ifdef _WIN32
#include <Windows.h>
#include <iostream>
#include <Dbghelp.h>
#include <process.h>
#else
#include <xcb/xcb.h>
#endif

#include "ip_utils.h"
#include "client.h"
#include "mLicenseNew.h"
#include <QtGlobal>
#include "LicenseShenSi.h"
#define AUTHENTICATE
//#include <unistd.h>

using namespace MxFunctions;
#ifdef _WIN32
// 将宽字符字符串转换为多字节字符字符串
std::string WideCharToMultiByteString(const wchar_t* wideString) {
	int length = WideCharToMultiByte(CP_ACP, 0, wideString, -1, NULL, 0, NULL, NULL);
	std::string result(length, '\0');
	WideCharToMultiByte(CP_ACP, 0, wideString, -1, &result[0], length, NULL, NULL);
	return result;
}

//程式异常捕获
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException) {
	const wchar_t* wideFileName = L"crash.dmp";
	std::string multiByteFileName = WideCharToMultiByteString(wideFileName);
	//创建 Dump 文件  
	HANDLE hDumpFile = CreateFileA(multiByteFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {
		//Dump信息  
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;

		//写入Dump文件内容  
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}
    
	//剪切文件到指定位置 
	QDateTime time = QDateTime::currentDateTime();
	QString filename = time.toString("yyyyMMddhhmmss");//格式化时间
	QString filePath_old = QApplication::applicationDirPath() + QString("/crash.dmp"); //原文件位置
	QString filePath_new = QApplication::applicationDirPath() + QString("/../LogFiles/%1.dmp").arg(filename); //新文件位置
	QFile::rename(filePath_old, filePath_new);

	//删除文件数据
	int pid = (int)getpid();
	QString exePath = QDir::toNativeSeparators(qApp->applicationDirPath().toLocal8Bit() + QString("/temp"));
	QDir dir(exePath);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	dir.setSorting(QDir::Name | QDir::Reversed);

	QStringList filters;
	filters << QString("*.dat");
	dir.setNameFilters(filters);

	QStringList listTmp = dir.entryList();
	for (QString item : listTmp) {
		QString basename = item;
		basename.remove(".dat");
		QStringList list = basename.split("_");
		if (list.size() >= 1 && list.last().toInt() == pid) {
			QFile fileTemp(QDir::toNativeSeparators(exePath + "/" + item));
			fileTemp.remove();
		}
	}

	//这里运行错误窗口程序
	QProcess *process = new QProcess;
	QStringList list;
	list.append("hnmx888");
	list.append("0");
	process->startDetached("ErrorBackClient.exe", list);
	return EXCEPTION_EXECUTE_HANDLER;

}

#endif

//获取当前屏幕分辨率
qreal GetScreenDpi() {
	qreal pix;
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#pragma comment(lib,"User32.lib")
	pix = GetSystemMetrics(SM_CXSCREEN);
	return pix;
#else
	int i, screenNum;
	xcb_connection_t *connection = xcb_connect(NULL, &screenNum);

	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	for (i = 0; i < screenNum; ++i) {
		xcb_screen_next(&iter);
	}

	xcb_screen_t *screen = iter.data;
	pix = screen->width_in_pixels;
	return pix;
#endif
}

enum class LicenseType {
	MxSim = 0x0001,
	GCLM = 0x0010
};



void getLicenseType(LicenseType& licenseType) {
	QString configFile = QCoreApplication::applicationDirPath() + QString("/../ConfigFiles/mxcaefile.config");
	QFile file(configFile);
	if (!file.exists()) {
		// 创建并且写入一行LicenseType
		if (file.open(QIODevice::WriteOnly)) {
			QTextStream out(&file);
			//QString licenseType = MAppConfiguration::mAppConfiguration::getInstance()->_licenseType;
			QString MxSim = QString("MxSim");
			MxSim = QString("*MXLICENSETYPE = %1\r\n").arg(MxSim);
			file.write(MxSim.toUtf8());//转为uft-8写文件即可
			file.close();
		}
		else {
		}
	}
	else
	{
		// 配置文件存在
		//QFile file(defaultDir);
		QString currentline;
		QStringList data;
		bool isRead{ false };
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))//以读的方式打开文件
		{
			while (!file.atEnd()) {
				currentline = file.readLine().simplified();
				if (currentline.contains("*MXLICENSETYPE")) {
					data = currentline.split("=");
					if (data.size() == 2)
					{
						isRead = true;
						MAppConfiguration::mAppConfiguration::getInstance()->_licenseType = data.last().simplified();
						if (MAppConfiguration::mAppConfiguration::getInstance()->_licenseType == QString("MxSim")) {
							licenseType = LicenseType::MxSim;
						}
						else if (MAppConfiguration::mAppConfiguration::getInstance()->_licenseType == QString("GCLM")) {
							licenseType = LicenseType::GCLM;
						}
					}
				}
			}
			file.close();
		}
		// 如果没有写过*MXRECENTFILE  默认为MxSim
		if (!isRead) {
			if (file.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
				QTextStream out(&file);
				//QString licenseType = MAppConfiguration::mAppConfiguration::getInstance()->_licenseType;
				QString MxSim = QString("MxSim");
				MxSim = QString("*MXLICENSETYPE = %1\r\n").arg(MxSim);
				file.write(MxSim.toUtf8());//转为uft-8写文件即可
				file.close();
			}
		}
	}
}

#ifdef _WIN32
bool IsWindows7()
{
    OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
    DWORDLONG dwlConditionMask = 0;
    // 修改：使用精确相等
    int op = VER_EQUAL;

    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);

    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 1;

    return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != FALSE;
}

static void setSystemDpiScaling()
{
	// 1. 取自己 exe 全路径
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);

	// 2. 注册表键
	const wchar_t* key =
		LR"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";

	// 3. 标志含义：
	//    ~ GDIDPISCALING  -> 强制 GDI 缩放（即“系统”选项）
	//    DPIUNAWARE       -> 让进程声明自己 DPI-unaware
	const wchar_t* flags = L"~ GDIDPISCALING DPIUNAWARE ";
	// 4. 写进去（HKCU 不需要管理员）
	RegSetKeyValueW(HKEY_CURRENT_USER, key, path,
		REG_SZ, flags, (wcslen(flags) + 1) * sizeof(wchar_t));
}
#endif

static void InitDirectory()
{
    // 使用QT路径创建相应的文件夹
    auto mk_dir = [](QString dir_str)
    {
        QDir dir;
        if (!dir.exists(dir_str))
        {
            bool res = dir.mkpath(dir_str);
        }
    };
    QString dir_str = qApp->applicationDirPath() + "/MeshResult";
    mk_dir(dir_str);
    mk_dir(dir_str + "/Logger");
}

// 自定义翻译，不依赖 .qm 文件
class ChineseTranslator : public QTranslator
{
public:
    QString translate(const char* context,
        const char* sourceText,
        const char* disambiguation = nullptr,
        int n = -1) const override
    {
        // QColorDialog 相关文字映射
        static const QHash<QByteArray, QString> dict = {
            {"&Basic colors",         "基本颜色"},
            {"&Custom colors",        "自定义颜色"},
            {"&Add to Custom Colors", "添加到自定义颜色"},
            {"&Pick Screen Color",    "拾取屏幕颜色"},
            {"OK",                   "确定"},
            {"Cancel",               "取消"},
            {"&Red:",                 "红(&R)"},
            {"&Green:",               "绿(&G)"},
            {"Bl&ue:",                "蓝(&U)"},
            {"A&lpha channel:",      "透明度(&L):"},
            {"&HTML:",               "HTML(&H):"},
            {"Cursor at %1, %2\n"
             "Press ESC to cancel",  "光标在 %1, %2\n按 ESC 取消"},
            {"Select Color",         "选择颜色"},
            {"&Sat:",                "饱和度(&S):"},
            {"&Val:",                "明度(&V):"},
            {"Hu&e:",                "色调(&E):"},
            // QLineEdit 右键菜单（表格编辑框等）
          {"&Undo",                "撤销(&U)"},
          {"&Redo",                "恢复(&R)"},
          {"Cu&t",                 "剪切(&T)"},
          {"&Copy",                "复制(&C)"},
          {"&Paste",               "粘贴(&P)"},
          {"Delete",               "删除"},
          {"Select All",           "全选"},
        };

        auto it = dict.find(QByteArray(sourceText));
        if (it != dict.end()) {
            return it.value();
        }
        return QString(); // 返回空表示使用原文
    }
};

// 如果默认没有打开则默认不开，但是要写入文件
int getfloat(const QString& str);

int new_license()
{
    //判断许可证
    LicenseNew* x = new LicenseNew(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
    int state = x->Licensecheck();
    if (state == 1)
    {
        cout << "写入配置文件失败，请联系软件管理员！\n";
        return 0;
    }
    else if (state == 2)
    {
        //cout << "本机无激活码文件，请联系软件管理员！\n";
        return 0;
    }
    else if (state == 3)
    {
        cout << "激活码无效！\n";
        return 0;
    }
    else if (state == 4)
    {
        //cout << "激活码已过期！\n";
        return 0;
    }
    else if (state == 5)
    {
        cout << "许可证类型不正确，请申请单机许可！\n";
        return 0;
    }
    else if (state == 6)
    {
        cout << "许可证mac地址验证存在问题！\n";
        return 0;
    }
    //cout << "程序正常启动" << endl;
    //cout << "激活码剩余天数:" << lincese->getRemainDay() << endl;
    return 1;
}

int main(int argc, char* argv[]) {

//手动缩放
//	setSystemDpiScaling();
//#if(QT_VERSION >= QT_VERSION_CHECK(5,6,0))
//	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
//#endif
//
//	int w = GetSystemMetrics(SM_CXSCREEN); // 主屏像素宽
//	int x = 1920;
//	qreal scale = qMax(1.0, qreal( w) / x);
//	qputenv("QT_SCALE_FACTOR", QByteArray::number(scale));
#if defined(_WIN32) || defined(_WIN64)
    if(IsWindows7())
        qputenv("QT_OPENGL", "desktop");
#endif

	//自动缩放
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);


	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QApplication::setStyle(QStyleFactory::create("plastique"));
	QApplication::setApplicationName("MxSimV1.0-beta");
	QApplication::setApplicationVersion("1.0");
	// 进程级 AUMID：必须先于一切窗口创建（决定任务栏固定项的应用身份）
	TaskbarRelaunch::SetProcessAppUserModelId();
	QApplication app(argc, argv);

    // mpcomm 平台通信接入（GUI 软件角色；独立启动时 start() 为 no-op）
    MxMech::MpcommBridge mpcommBridge;
    mpcomm::simple::SoftwareAdapter mpcommAdapter("MxMech");
    mpcommAdapter.on_command(&MxMech::handleMpcommCommand, &mpcommBridge);
    const bool mpcommStarted = mpcommAdapter.start(argc, argv);
    if (!mpcommStarted && mpcommAdapter.platform_mode()) {
        std::cerr << "[mpcomm] start failed in platform mode: " << mpcommAdapter.error() << std::endl;
        return 2;
    }
    mpcommBridge.setAdapter(&mpcommAdapter);

    // 软件退出（用户主动关闭/任意路径退出）时通知平台关闭节点会话；
    // 与平台 CLOSE 命令的 onClose 路径互补，覆盖用户直接关窗口的场景。
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &mpcommBridge,
        [&mpcommBridge]() {
            mpcommBridge.onClose();
        });

    // 平台模式下仅允许单模型：禁用"新建"（在 slot_New 里据此拦截）
    MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode =
        mpcommAdapter.platform_mode();


    // 安装自定义翻译器（translator 必须堆分配：installTranslator 不接管所有权，
    // 栈对象会在进入事件循环前析构导致翻译失效——ADS 右键菜单仍英文即此原因）
    {
        ChineseTranslator* translator = new ChineseTranslator();
        app.installTranslator(translator);

        QTranslator* adsTranslator = new QTranslator();
        if (adsTranslator->load(":/mMainWidgets/Translations/ads_zh_CN.qm"))
        {
            app.installTranslator(adsTranslator);
            qDebug() << "ads translator installed";
        }
        else
        {
            qWarning() << "ads_zh_CN.qm load failed";
            delete adsTranslator;
        }

        QTranslator* translator_cn = new QTranslator();
        translator_cn->load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        app.installTranslator(translator_cn);
    }

	app.setStyle("Fusion");
	//思源黑体（字体已移出 qrc——15.8MB 内嵌导致 qrc 源文件 185MB，编译器堆耗尽 C1060；
	//发布包由打包器拷贝到 Application/fonts，开发环境由 POST_BUILD 拷贝到 exe 目录）
	QString fontPath = QCoreApplication::applicationDirPath() + "/SourceHanSansSC-Normal.otf";
	int fontId = QFontDatabase::addApplicationFont(fontPath);
	if (fontId != -1) {
		QStringList families = QFontDatabase::applicationFontFamilies(fontId);
		qDebug() << "loaded families:" << families;   // 看看它到底叫啥
	}
	else {
		qWarning() << "load font failed!";
	}
	// 全局默认字体：按优先级探测 微软雅黑→黑体→微软正黑体→等线→内置思源黑体，
	// 覆盖无显式 font-family 的控件/消息框/提示（QSS 里的字体族由 ThemeManager
	// 下发前统一替换，取值同源）。uiFontFamily 首调即缓存探测结果，必须晚于
	// 上面的 addApplicationFont，保底思源黑体才可见
	QFont appFont(UiMetrics::uiFontFamily());
	qApp->setFont(appFont);
	qDebug() << "ui font family:" << UiMetrics::uiFontFamily();
	//MechMainWindow::mOpenMXSoftWare s;
	//s.run();s

	//if (s.getOpenSig() != 1)
	//{
	//  MechMainWindow::mFramelessMessageBox::warning(new QWidget(), 
	//		mLanguageVarible::FindLanguageValue("警告"), 
	//		mLanguageVarible::FindLanguageValue("访问网段失败！"),
	//		mLanguageVarible::FindLanguageValue("确定"));
	//	return 1;
	//}

#ifdef _WIN32
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);//注冊异常捕获函数	//这里运行错误窗口程
#endif
	QSurfaceFormat format;
	format.setMajorVersion(3);
	format.setMinorVersion(3);
	format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
#ifdef _WIN32
	format.setSamples(4);
#endif
	QSurfaceFormat::setDefaultFormat(format);

    // 创建初始文件夹
    InitDirectory();

	//默认许可方式
	//默认许可方式
	LicenseType licensetype = LicenseType::MxSim;
	getLicenseType(licensetype);
	if (licensetype == LicenseType::MxSim) {
		MAppConfiguration::mAppConfiguration::getInstance()->_licenseType = QString("MxSim");
		//判断许可证
		Common::mLicense *license = new Common::mLicense(MAppConfiguration::mAppConfiguration::getInstance()->_workingDir);
		//0正确  1配置文件失败  2文件不存在 3不正确 4许可证过期
		int state = license->checkLicense();
		int Day=license->getRemainDay();

        //获取深思相关配置
#ifdef _WIN32
        virbox::Config config;
        virbox::LicenseManager manager;
        //获取深思单机许可
        config.type = virbox::LicenseTypeShensi::Standalone;
        virbox::Status started = manager.Start(config);
#else
        //Linux 暂不支持深思许可：视为未获取，走下方迈曦许可回退逻辑
        bool started = false;
#endif
        if (!started) {
            //深思单机许可获取不成功则尝试迈曦单机许可
            //内部人员开发时，可以自己设为0，跳过判断，外发需要许可证时需要将下一行注释掉
            //state = 0; 不要打开
            if (state == 0) {

                //浮动许可
                //Common::mClient::getInstance()->setIsUseFloatLicnese(true);
                //Common::mClient::getInstance()->start();


                //显示主窗口
                //ApplicationInit::mLoginDialog*LoginDialog = new ApplicationInit::mLoginDialog;

                //if (LoginDialog->exec() == QDialog::Accepted)
                //{
                int batchExitCode = MechMainWindow::mMechMainWindow::runBatchMeshIfRequested(app.arguments());
                if (batchExitCode != -1)
                    return batchExitCode;

                MxMech::mAppInitializer* ini = new MxMech::mAppInitializer(argc, argv, 1);
                QTranslator translator;
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    translator.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                    app.installTranslator(&translator);
                }
                //UI截图工具模式（--ui-shot）：主窗就绪后自动截图并退出；未请求时为空操作
                PostWindow::mUiShotController::scheduleIfRequested(app.arguments());
                int val = app.exec();

                //是否重启动
                if (val == -123456) {
                    QProcess::startDetached(qApp->applicationFilePath(), QStringList());
                    return 0;
                }
                delete ini;
                return val;
                //}
                //else
                //{
                //	app.exit(0);
                //	return 0;
                //}
            }
            else if (state == 1) {
                MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证文件配置失败，请联系软件管理员！"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                app.exit(0);
                return 0;
            }
            else if (state == 2) {
                //MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证文件不存在，请联系软件管理员！"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                //app.exit(0);

                //获取深思浮动许可
#ifdef _WIN32
                config.type = virbox::LicenseTypeShensi::Floating;
                virbox::Status started = manager.Start(config);
#else
                bool started = false;   //Linux 暂不支持深思许可
#endif
                if (!started) {
                    //深思浮动许可获取不成功则尝试迈曦浮动许可
                    int state1 = new_license();
                    if (state1 == 0) {
                        int requested_resources = 1;
                        auto client = initializeClient();
                        if (client->connectAndAuthenticate()) {
                            int batchExitCode = MechMainWindow::mMechMainWindow::runBatchMeshIfRequested(app.arguments());
                            if (batchExitCode != -1)
                                return batchExitCode;

                            MxMech::mAppInitializer* ini = new MxMech::mAppInitializer(argc, argv, 1);
                            QTranslator translator;
                            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                            {
                                translator.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                                app.installTranslator(&translator);
                            }

                            int val = app.exec();

                            //是否重启动
                            if (val == -123456) {
                                QProcess::startDetached(qApp->applicationFilePath(), QStringList());
                                return 0;
                            }
                        }
                        else
                        {
                            if (state1 == 3) {
                                MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证文件不支持此产品，请联系软件管理员！"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                            }
                            else if (state1 == 6) {
                                MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证文件验证存在问题"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                            }
                            app.exit(0);
                            return 0;
                        }
                    }
                    else
                    {
                        int batchExitCode = MechMainWindow::mMechMainWindow::runBatchMeshIfRequested(app.arguments());
                        if (batchExitCode != -1)
                            return batchExitCode;

                        MxMech::mAppInitializer* ini = new MxMech::mAppInitializer(argc, argv, 1);
                        QTranslator translator;
                        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                        {
                            translator.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                            app.installTranslator(&translator);
                        }

                        int val = app.exec();

                        //是否重启动
                        if (val == -123456) {
                            QProcess::startDetached(qApp->applicationFilePath(), QStringList());
                            return 0;
                        }
                    }
                }
                else {
                    //深思浮动许可成功则启动界面
                    int batchExitCode = MechMainWindow::mMechMainWindow::runBatchMeshIfRequested(app.arguments());
                    if (batchExitCode != -1)
                        return batchExitCode;
                    MxMech::mAppInitializer* ini = new MxMech::mAppInitializer(argc, argv, 1);
                    QTranslator translator;
                    if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                    {
                        translator.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                        app.installTranslator(&translator);
                    }

                    int val = app.exec();

                    //是否重启动
                    if (val == -123456) {
                        QProcess::startDetached(qApp->applicationFilePath(), QStringList());
                        return 0;
                    }
                    delete ini;
                    return val;
                }
            }
            else if (state == 3) {
                MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证文件不正确，请联系软件管理员！"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                app.exit(0);
                return 0;
            }
            else if (state == 4) {
                MechMainWindow::mFramelessMessageBox::warning(new QWidget(), mWidgetOperation::FindLanguageValue(English, Chinese, "警告"), mWidgetOperation::FindLanguageValue(English, Chinese, "许可证已过期，请联系软件管理员！"), mWidgetOperation::FindLanguageValue(English, Chinese, "确定"));
                app.exit(0);
                return 0;
            }
        }
        else {
            //深思单机许可获取成功则启动界面
            int batchExitCode = MechMainWindow::mMechMainWindow::runBatchMeshIfRequested(app.arguments());
            if (batchExitCode != -1)
                return batchExitCode;

            MxMech::mAppInitializer* ini = new MxMech::mAppInitializer(argc, argv, 1);
            QTranslator translator;
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                translator.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
                app.installTranslator(&translator);
            }

            int val = app.exec();

            //是否重启动
            if (val == -123456) {
                QProcess::startDetached(qApp->applicationFilePath(), QStringList());
                return 0;
            }
            delete ini;
            return val;
        }              
	}
	// TODO: raise error when no valid license type
}


int getfloat(const QString& str)
{
	int res;
	// 获取release目录的路径
#ifdef _WIN32
	QString releasePath = QCoreApplication::applicationDirPath();
#else
	QString releasePath = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
#endif

	// 构建ConfigFiles文件夹路径
	QString configFolderPath = releasePath + "/../FloatLicense/floatelicense.config";

	QString dirPath = QFileInfo(configFolderPath).absolutePath();
	QDir dir;

	QFileInfo checkFile(configFolderPath);
	if (checkFile.exists() && checkFile.isFile()) {
		// 构建mxcaefile.config文件路径
		QString configFilepath = configFolderPath;

		QFile configFile(configFilepath);

		if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return -1;
		}

		QTextStream in(&configFile);
		while (!in.atEnd())
		{
			QString line = in.readLine();
			if (line.contains(str))
			{
				if (str == QString("*MXFLOATLICENSE"))
				{
					int equalsIndex = line.indexOf('=');
					if (equalsIndex != -1)
					{
						QString valueStr = line.mid(equalsIndex + 1).trimmed();
						bool ok;
						int value = valueStr.toInt(&ok);
						res = value;
						if (ok) {}
						else {}
					}
				}
				else if (str == QString("*MXNETWORKCONTROL"))
				{
					int equalsIndex = line.indexOf('=');
					if (equalsIndex != -1)
					{
						QString valueStr = line.mid(equalsIndex + 1).trimmed();
						bool ok;
						int value = valueStr.toInt(&ok);
						res = value;
					}
				}
			}
		}
		configFile.close();
		return res;
	}
	else {
		if (!dir.exists(dirPath)) {
			if (!dir.mkpath(dirPath)) {
				return -1;
			}
			QFile file(configFolderPath);
			if (file.open(QIODevice::ReadWrite)) {
				file.close();
			}
			if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QTextStream out(&file);
				out << QString("*MXFLOATLICENSE = 1");
				file.close();
			}
			return 1;
		}
		return 0;
	}


}