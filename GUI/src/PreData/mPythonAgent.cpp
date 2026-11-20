// 先包含 Windows 头：rpcndr.h 的全局 byte 与 C++17 的 std::byte 冲突
// （mGlobalSignals.h 中有全局 using namespace std），需在 using 生效前完成解析
#ifdef _WIN32
#include <Windows.h>
#endif
//Python
#include <Python.h>
#include "mPythonAgent.h"
#include "mPythonInterpreter.h"

//MBasicFunction
#include "mGlobalSignals.h"
#include "mOutputMessage.h"
#include "mBasicEnum.h"
#include "mGlobalVarible.h"
#include "mWidgetOperation.h"
//QT
#include <QCoreApplication>
#include "mAppConfiguration.h"
#include <QDir>

#include <wchar.h>
#ifdef Q_OS_WIN32
#include <atlconv.h>
#endif
#include <locale.h>

using namespace MxFunctions;

namespace MechPreData
{
    mPythonAgent* mPythonAgent::_instance = nullptr;

    mPythonAgent* mPythonAgent::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mPythonAgent;
        }
        return _instance;
    }

    mPythonAgent::mPythonAgent()
    {
        _pyInterpreter = new mPythonInterpreter;
        initLanguage();
        openCommandLogFile();
    }

    mPythonAgent::~mPythonAgent()
    {
        if (_cmdLogFile.isOpen())
        {
            _cmdLogStream.flush();
            _cmdLogFile.close();
        }
        if (_pyInterpreter != nullptr)
        {
            delete _pyInterpreter;
            _pyInterpreter = nullptr;
        }
    }

    void mPythonAgent::setScriptFileRunning(bool running)
    {
        _scriptFileRunning = running;
    }

    bool mPythonAgent::isScriptFileRunning() const
    {
        return _scriptFileRunning;
    }

    void mPythonAgent::openCommandLogFile()
    {
        // 日志文件放在软件运行目录，覆盖模式：每次启动重新记录本次会话的命令
        const QString logPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../LogFiles/py_command_record.py");
        _cmdLogFile.setFileName(logPath);
        if (_cmdLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            _cmdLogStream.setDevice(&_cmdLogFile);
            _cmdLogStream << "# MxSim python command record (for crash replay)\n";
            _cmdLogStream.flush();
        }
    }

    void mPythonAgent::appendCommandLog(const QString& cmd)
    {
        if (!_cmdLogFile.isOpen())
                return;
        _cmdLogStream << cmd << "\n";
        _cmdLogStream.flush();  // 立即落盘，保证闪退时文件内容完整
    }

    bool mPythonAgent::initPythonAgent()
    {
        QString pyPath;
#ifdef Q_OS_WIN32
        // Windows Python37 路径逻辑不变
        pyPath = qApp->applicationDirPath() + "/python";
        if (!QDir(pyPath).exists())
        {
            pyPath = qApp->applicationDirPath() + "/../tools/python";
        }
#else
        // Linux 使用 Python3.9
        pyPath = "/usr/bin/python3.9";
#endif
        pyPath = QDir::cleanPath(pyPath);

#ifdef Q_OS_WIN32
        char* ch;
        QByteArray ba = pyPath.toLocal8Bit();
        ch = ba.data();
        wchar_t* wc;
        USES_CONVERSION;
        wc = A2W(ch);
        Py_SetPythonHome(wc);
#else
// 与链接的 libpython3.9 一致：优先使用包内的 python 环境（与 bin 同级目录），回退到 /usr/local/python3.9
pyPath = qApp->applicationDirPath() + "/python";
pyPath = QDir::cleanPath(pyPath);
QString pymodules = pyPath + "/lib/python3.8:" + pyPath + "/lib/python3.8/lib-dynload:" + pyPath + "/lib/python3.8/site-packages";
QByteArray pymodulesba = pymodules.toLocal8Bit();
QByteArray ba = pyPath.toLocal8Bit();
#endif

        QDir pyDir(pyPath);
        if (pyDir.exists())
        {
            // 统一老式初始化API，移除PyStatus/PyConfig（3.11专属）
            Py_Initialize();

            if (_pyInterpreter == nullptr)
            {
                    return false;
            }

            //初始化Python解释器（初始化期间不回显 import 等内部命令）
            _initializing = true;
            bool isInit = _pyInterpreter->initPythonInterpreter();
            _initializing = false;
            if (isInit)
            {
                    return true;
            }

            return false;
        }
        return false;
    }

    void mPythonAgent::submitCommand(const QString& cmd)
    {
        if (_pyInterpreter == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, mWidgetOperation::FindLanguageValue(_English, _Chinese, "Python初始化失败"));
            return;
        }

        // 先落盘：即使执行中闪退，肇事命令也已记录在文件中
        appendCommandLog(cmd);

        // 执行（成功/失败的信息窗口回显由 execPythonCommand 内部处理）
        if (_pyInterpreter->execPythonCommand(cmd))
        {
            //保存Python命令
            _pythonCommands.append(cmd);
        }
    }

    void mPythonAgent::submitCommand(const QStringList& cmds)
    {
        const int ncmd = cmds.size();
        for (int i = 0; i < ncmd; ++i)
        {
            this->submitCommand(cmds.at(i));
        }
    }

    void mPythonAgent::logCommandToFile(const QString& cmd)
    {
        appendCommandLog(cmd);
    }

    void mPythonAgent::addCommandToHistory(const QString& cmd)
    {
        _pythonCommands.append(cmd); 
    }

    void mPythonAgent::initLanguage()
    {
        _Chinese.insert("Python初始化失败", "Python初始化失败!");
        _English.insert("Python初始化失败", "Python initialization failed!");
    }
}