#include "mAppInitializer.h"
#include "TaskbarRelaunch.h"
#include "mStartPage.h"
#include "mAppTranslators.h"

//MechMainWindow
#include "mMechMainWindow.h"
#include "mDirectModelingController.h"
//PostWindow模块装配（组合根）：后处理菜单页经IRibbonHost注册到宿主菜单栏
#include "IRibbonHost.h"
#include "mPostMenuContributor.h"
#include "mPostAutoTestAPIRegistrar.h"
#include "ThemeManager.h"
//MBasicFunction
#include "mGlobalSignals.h"

//MechCommon
#include "mAppConfiguration.h"
#include "mCommonLanguage.h"

//MechPreData
#include "mViewportModelManager.h"

//MPythonScript
#include "mPythonAgent.h"

//APIServer
#include "mAPIScriptExecutor.h"
#include "mAPISchedulingServer.h"
#include "mAPIServer.h"

//Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QTimer>
#include <QDir>
#include <QScreen>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDebug>

//IOFile
#include "mIConfigurationFile.h"
#include "mLanguageVarible.h"
#include "mWidgetOperation.h"

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>

#endif
using namespace MechPreData;
using namespace MxFunctions;

namespace
{
    // 调试时只需要修改这一处 JSON 路径。Windows 路径建议使用正斜杠。
    const QString kDebugScriptPath = QStringLiteral("");

    // 同时写入 Qt 日志和软件消息窗口，脚本异常不再通过退出程序反馈。
    void outputScriptInfo(const QString& message)
    {
        qInfo().noquote() << message;
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            0, message, true);
    }

    void outputScriptError(const QString& message)
    {
        qCritical().noquote() << message;
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            2, message, true);
    }

    QStringList buildStartupArguments()
    {
        QStringList arguments = QCoreApplication::arguments();

#if defined(_DEBUG) || defined(QT_DEBUG)
        // Debug 构建自动等价于：MxSim.exe -e C:/sp.json
        if (!kDebugScriptPath.isEmpty())
        {
            const QFileInfo scriptFile(kDebugScriptPath);
            if (scriptFile.exists() && scriptFile.isFile())
            {
                arguments.clear();
                arguments << QCoreApplication::applicationFilePath()
                    << QStringLiteral("-e")
                    << scriptFile.absoluteFilePath();
            }
            else
            {
                // 调试路径无效时保留原始命令行，避免程序启动后立即退出。
                qCritical().noquote()
                    << QStringLiteral("CAE 调试脚本不存在或不是文件：%1")
                    .arg(QDir::toNativeSeparators(kDebugScriptPath));
            }
        }
#endif

        return arguments;
    }
}

namespace MxMech
{
    mAppInitializer::mAppInitializer(int argc, char* argv[], int ramainDay) :_remainDay(ramainDay)
    {
        //打开启动页
        _startPage = new mStartPage;

        //初始化predata中英互译
        mLanguageVarible::initLanguageVarible();
        //初始化common中英互译
        MechCommon::mCommonLanguage::initLanguageVarible();

        //读取配置文件，存储配置信息
        this->readConfigFile();

        //打开主窗口
        _mainWindow = new MechMainWindow::mMechMainWindow;

        //PostWindow模块装配：注册后处理菜单页，与(int remainDay)构造保持一致；
        //深思许可重构(e5423969)后main.cpp只走本构造，缺失此调用会导致
        //结果/云图等Post页签查无页面对象，点击页签下方Ribbon不切换
        PostWindow::mPostMenuContributor::getInstance()->registerMenus(_mainWindow->getRibbonHost());

        // 后注册页也必须走同一套 Ribbon 输入框/按钮主题几何规则。
        if (auto* host = dynamic_cast<QWidget*>(_mainWindow->getRibbonHost()))
            MechMainWindow::ThemeManager::instance()->applyAll(host);


        /*
        * 全局基础控件样式改经 ThemeManager 令牌化下发（applyGlobalQss）：
        * 文件内硬值已渐进替换为 @{...} 占位符，按当前 UiPalette/UiMetrics
        * 填空后应用，切主题/切档随 applyAllInternal 同代数重发；
        * 此处不再静态读取一次性压顶（时序上后于 applyAll 会覆盖令牌样式）
        */
        MechMainWindow::ThemeManager::instance()->applyGlobalQss();

        //初始化语言
        MxFunctions::mGlobalSignals::getInstance()->updateTranslator(MAppConfiguration::mAppConfiguration::getInstance()->_language);


        if (argc > 1)
        {
            for (int i = 1; i < argc; ++i)
                _fileNames.append(QString::fromLocal8Bit(argv[i]));
        }

        // 构造函数返回后 main() 才会进入 app.exec()。在此之后显示主窗，
        // 确保 Qt 能及时处理首帧布局、主题与 dock 收尾任务。
        QTimer::singleShot(0, this, &mAppInitializer::start);
    }

    void mAppInitializer::start()
    {
        if (!_mainWindow)
            return;

        // 任务栏固定/重启动属性必须在窗口显示前设置（winId() 先建原生窗口）。
        TaskbarRelaunch::ApplyForMainWindow(_mainWindow);
        //启动永远最大化（不保存/恢复窗口大小状态）
        _mainWindow->showMaximized();
        _startPage->finish(_mainWindow);

        // 等到主窗首帧真正画完再同步初始化 Python；零延时定时器本身不保证
        // WM_PAINT 已处理，提前 import 仍可能让客户区停留在窗口底色。
        auto* firstPaintWatcher = new QObject(_mainWindow);
        auto* paintState = new bool(false);
        _mainWindow->installEventFilter(firstPaintWatcher);
        connect(firstPaintWatcher, &QObject::eventFilter, _mainWindow,
            [this, firstPaintWatcher, paintState](QObject* watched, QEvent* event)
        {
            if (watched != _mainWindow || event->type() != QEvent::Paint)
                return false;
            *paintState = true;
            _mainWindow->removeEventFilter(firstPaintWatcher);
            firstPaintWatcher->deleteLater();
            QTimer::singleShot(0, this, &mAppInitializer::finishStartup);
            return false;
        });
        // 无界面平台或系统暂缓绘制时，不能令脚本/批量入口永久等待 Paint。
        QTimer::singleShot(3000, this,
            [this, firstPaintWatcher, paintState]()
        {
            if (*paintState)
                return;
            _mainWindow->removeEventFilter(firstPaintWatcher);
            firstPaintWatcher->deleteLater();
            finishStartup();
        });
    }

    void mAppInitializer::finishStartup()
    {
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                0, QStringLiteral("软件初始化完成!"), true);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                0, QStringLiteral("Software initialization completed!"), true);
        }

        const bool py = mPythonAgent::getInstance()->initPythonAgent();
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            0, mLanguageVarible::FindLanguageValue(
                py ? "Python初始化成功" : "Python初始化失败"));

        const QStringList arguments = buildStartupArguments();
        if (APIServer::mAPIScriptExecutor::commandLineRequestsScript(arguments))
        {
            // 脚本模式由执行器消费 -e/--script 参数，不能再作为普通文件打开。
            initScriptExecutor(arguments);
        }
        else if (!_fileNames.isEmpty())
        {
            // Base 没有 Mech 的命令行批量求解入口，保留原有行为：只解析参数。
        }
    }

    void mAppInitializer::initScriptExecutor(const QStringList& arguments)
    {
        // 沿用业务 API 注册模块使用的调度器入口。如实际工程名称不同，只替换此行。
        auto* scheduler = getAPISchedulingServer;
        if (scheduler == nullptr)
        {
            outputScriptError(QStringLiteral("CAE 脚本启动失败：API 调度器未初始化"));
            return;
        }

        // 执行器必须持续存活到整个脚本结束，不能创建为构造函数局部变量。
        _scriptExecutor = new APIServer::mAPIScriptExecutor(scheduler, _mainWindow);

        // 预留异步几何状态监听；当前同步策略不会查询该状态。
        QObject::connect(
            MxFunctions::mGlobalSignals::getInstance(),
            &MxFunctions::mGlobalSignals::modelDataThreadStatusChangedSig,
            _scriptExecutor,
            [this](bool busy) {
                _modelBusy = busy;
            });

        _scriptExecutor->registerCompletionCondition(
            QStringLiteral("modelIdle"),
            [this](const APIServer::APIParams&) {
                if (_modelBusy)
                {
                    return APIServer::ScriptConditionResult::pending(
                        QStringLiteral("正在执行几何操作"));
                }

                return APIServer::ScriptConditionResult::completed(
                    QStringLiteral("几何操作完成"));
            });

        // 当前所有 API 暂按同步调用处理，实际执行清单完全由 JSON steps 决定。
        APIServer::ScriptExecutionPolicy defaultPolicy;
        // 完成条件为空时，以 executeAPI() 成功返回作为当前步骤完成标志。
        defaultPolicy.completionCondition.clear();
        defaultPolicy.timeoutMs = 120000;
        defaultPolicy.pollIntervalMs = 100;
        defaultPolicy.retries = 0;
        defaultPolicy.retryDelayMs = 500;
        defaultPolicy.onError = QStringLiteral("stop");

        QString error;
        if (!_scriptExecutor->setDefaultExecutionPolicy(defaultPolicy, &error))
        {
            outputScriptError(
                QStringLiteral("CAE 脚本策略配置失败：%1").arg(error));
            return;
        }

        // importGeoFile 实际为异步操作，必须等 modelIdle 后才能推进下一步。
        // 这里只声明 API 的完成语义，执行哪些 API 仍完全由 JSON steps 决定。
        APIServer::ScriptExecutionPolicy importPolicy = defaultPolicy;
        importPolicy.completionCondition = QStringLiteral("modelIdle");
        if (!_scriptExecutor->registerExecutionPolicy(
            QStringLiteral("importGeoFile"), importPolicy, &error))
        {
            outputScriptError(
                QStringLiteral("CAE 导入 API 策略配置失败：%1").arg(error));
            return;
        }

        // 后处理自动化测试 API（mx-autotest 平台数据抽取原语，只读/低副作用）：
        // 注册 API 本体 + 结果导入的异步完成语义（postImportIdle）
        if (!PostWindow::mPostAutoTestAPIRegistrar::getInstance()->ensureRegistered())
        {
            outputScriptError(QStringLiteral("CAE 脚本启动失败：后处理测试 API 注册失败"));
            return;
        }
        _scriptExecutor->registerCompletionCondition(
            QStringLiteral("postImportIdle"),
            [](const APIServer::APIParams&) {
                if (!PostWindow::mPostAutoTestAPIRegistrar::getInstance()->importSettled())
                {
                    return APIServer::ScriptConditionResult::pending(
                        QStringLiteral("结果文件导入进行中"));
                }
                return APIServer::ScriptConditionResult::completed(
                    QStringLiteral("结果文件导入完成"));
            });
        APIServer::ScriptExecutionPolicy postImportPolicy = defaultPolicy;
        postImportPolicy.completionCondition = QStringLiteral("postImportIdle");
        postImportPolicy.timeoutMs = 120000;
        if (!_scriptExecutor->registerExecutionPolicy(
            QStringLiteral("API_Result_OpenFiles"), postImportPolicy, &error))
        {
            outputScriptError(
                QStringLiteral("CAE 结果打开 API 策略配置失败：%1").arg(error));
            return;
        }

        // 输出一行一个 JSON 事件，便于 cmd 或日志采集程序读取。
        QObject::connect(
            _scriptExecutor,
            &APIServer::mAPIScriptExecutor::eventReady,
            _mainWindow,
            [](const QJsonObject& event) {
                qInfo().noquote()
                    << QJsonDocument(event).toJson(QJsonDocument::Compact);
            });

        QObject::connect(
            _scriptExecutor,
            &APIServer::mAPIScriptExecutor::finished,
            _mainWindow,
            [this](int exitCode, const QJsonObject& report) {
                // 初始点/线/面/体 ID 映射只在一次脚本运行期间有效。无论成功、失败
                // 还是取消，都按 executionId 精确释放，避免保留已被数据层删除的
                // PartSet 指针。
                    if (report.value(QStringLiteral("geometryIdMode")).toString()
                        == QStringLiteral("original"))
                    {
                        MechMainWindow::DirectModelingController::getInstance()
                            ->clearScriptOriginalFaceIdContext(
                                report.value(QStringLiteral("scriptExecutionId")).toString());
                    }


                const QString message = report.value(
                    QStringLiteral("message")).toString();
                const QString reportError = report.value(
                    QStringLiteral("reportError")).toString();

                if (exitCode == APIServer::mAPIScriptExecutor::Succeeded)
                {
                    outputScriptInfo(QStringLiteral("CAE 脚本执行完成：%1")
                        .arg(message));
                }
                else
                {
                    QString errorMessage = QStringLiteral(
                        "CAE 脚本执行失败（错误码 %1）：%2")
                        .arg(exitCode)
                        .arg(message);
                    if (!reportError.isEmpty())
                    {
                        errorMessage += QStringLiteral("；报告写入错误：%1")
                            .arg(reportError);
                    }
                    outputScriptError(errorMessage);
                }

                // 脚本声明 closeOnFinish 时跑完自退（自动化回归由外层按退出码判定）
                if (_scriptExecutor != nullptr && _scriptExecutor->closeOnFinish())
                {
                    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
                }
            });

        if (!_scriptExecutor->configureFromCommandLine(arguments, &error))
        {
            outputScriptError(
                QStringLiteral("CAE 脚本加载失败：%1").arg(error));
            return;
        }

        // 几何导入依赖 Qt 事件循环，必须在构造函数返回后再启动第一步。
        QTimer::singleShot(0, _scriptExecutor, [this]() {
            QString startError;
            if (!_scriptExecutor->start(&startError))
            {
                outputScriptError(
                    QStringLiteral("CAE 脚本预检失败：%1").arg(startError));
            }
            });
    }

    mAppInitializer::~mAppInitializer()
    {
        if (_startPage != nullptr)
        {
            delete _startPage;
            _startPage = nullptr;
        }
        if (_mainWindow != nullptr)
        {
            delete _mainWindow;
            _mainWindow = nullptr;
        }
    }

    void mAppInitializer::readConfigFile()
    {
        MechIOFile::mIConfigurationFile* iconfig = new MechIOFile::mIConfigurationFile();
        iconfig->readConfigureData();
        delete iconfig;
    }
}
