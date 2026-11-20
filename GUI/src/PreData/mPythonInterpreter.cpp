//Python
#include <Python.h>
#include "mPythonInterpreter.h"
#include "mPythonAgent.h"


//MBasicFunction
#include "mOutputMessage.h"
#include "mGlobalSignals.h"
#include "mBasicEnum.h"
#include "mAppConfiguration.h"

//QT
#include <QDir>
#include <QCoreApplication>
#include <QFileInfoList>

using namespace MxFunctions;

namespace MechPreData
{
	mPythonInterpreter::mPythonInterpreter()
	{
	}

	mPythonInterpreter::~mPythonInterpreter()
	{
	}

	bool mPythonInterpreter::initPythonInterpreter()
	{
		if (!Py_IsInitialized())
		{
			return false;
		}

		//异常处理函数
		PyErr_PrintEx(1);
        
		//指定Python文件所在的文件夹

		this->execPythonCommand("import sys");
		this->execPythonCommand("print(sys.path)");
		this->execPythonCommand("import os");

		// py 目录优先级：运行目录下的 py/ 子目录，
		// 未找到则回退源码目录 src/Py（开发调试时直接使用源码脚本）
		QString pyPath = QDir::cleanPath(qApp->applicationDirPath() + "/py");
		if (!QDir(pyPath).exists())
		{
			const QString srcPyPath = QDir::cleanPath(qApp->applicationDirPath() + "/../../src/Py");
			if (QDir(srcPyPath).exists())
				pyPath = srcPyPath;
		}
		QString pyCmd = QString("sys.path.append(\"%1\")").arg(pyPath);
		this->execPythonCommand(pyCmd);

		//加载Python文件
		QDir pyDir(pyPath);
		QStringList pySuffix = { "*.py" };
		pyDir.setNameFilters(pySuffix);
		QFileInfoList pyFileInfos = pyDir.entryInfoList(pySuffix);
		const int fileCount = pyFileInfos.size();
		for (int i = 0; i < fileCount; ++i)
		{
			QFileInfo fileInfo = pyFileInfos.at(i);
			QString fileName = fileInfo.baseName();
			QString cmd = QString("import %1").arg(fileName);
			this->execPythonCommand(cmd);
		}
		return true;
	}

	bool mPythonInterpreter::execPythonCommand(QString cmd)
	{
		std::string str = cmd.toStdString();
		const char* ch = str.c_str();

		//判断Python命令是否执行成功
		int flag = PyRun_SimpleStringFlags(ch, NULL);
 		if (flag == -1)
		{
			// 取 Python 真实异常信息输出到信息窗口（PyErr_Print 只打 stderr，VS 调试时看不到）
			QString pyErr;
			PyObject *ptype = nullptr, *pvalue = nullptr, *ptraceback = nullptr;
			PyErr_Fetch(&ptype, &pvalue, &ptraceback);
			if (ptype)
			{
				PyObject* typeStr = PyObject_Str(ptype);
				PyObject* valueStr = pvalue ? PyObject_Str(pvalue) : nullptr;
				const char* typeCStr = typeStr ? PyUnicode_AsUTF8(typeStr) : "";
				const char* valueCStr = valueStr ? PyUnicode_AsUTF8(valueStr) : "";
				pyErr = QString::fromUtf8("%1: %2").arg(typeCStr, valueCStr ? valueCStr : "");
				Py_XDECREF(typeStr);
				Py_XDECREF(valueStr);
				Py_XDECREF(ptype);
				Py_XDECREF(pvalue);
				Py_XDECREF(ptraceback);
			}
			PyErr_Clear();

			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage,
					pyErr.isEmpty() ? tr("未声明的命令行:%1").arg(cmd)
					                : tr("命令执行失败:%1\n%2").arg(cmd, pyErr));
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage,
					pyErr.isEmpty() ? tr("Undeclared Command Line:%1").arg(cmd)
					                : tr("Command failed:%1\n%2").arg(cmd, pyErr));
			}

			return false;
		}

		// 执行成功：命令行单条执行回显到信息窗口；脚本批量执行与解释器初始化不回显
		mPythonAgent* agent = mPythonAgent::getInstance();
		if (!agent->isScriptFileRunning() && !agent->_initializing)
		{
			mGlobalSignals::getInstance()->outputMessageSig(PythonCommand, cmd);
		}
		return true;
	}
}