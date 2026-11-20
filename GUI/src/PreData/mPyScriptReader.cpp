#include "mPyScriptReader.h"
#include "mPythonAgent.h"

//MBasicFunction
#include "mOutputMessage.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

//QT
#include <QFile>
#include <QTextStream>

using namespace MxFunctions;
namespace MechPreData
{
	mPyScriptReader::mPyScriptReader(const QString& filename) :_fileName(filename)
	{

	}

	mPyScriptReader::~mPyScriptReader()
	{

	}

	void mPyScriptReader::startRun()
	{
		QFile file(_fileName);
		if (file.open(QIODevice::Text | QIODevice::ReadOnly))
		{
			// 脚本文件批量执行期间置标志：创建的树节点不改变当前选中节点
			mPythonAgent::getInstance()->setScriptFileRunning(true);
			QTextStream reader(&file);
			while (!reader.atEnd())
			{
				QString pycmd = reader.readLine().simplified();
				if (pycmd.isEmpty())
				{
					continue;
				}

				//执行Python命令
				mPythonAgent::getInstance()->submitCommand(pycmd);
			}
			mPythonAgent::getInstance()->setScriptFileRunning(false);
			file.close();

			//发送信息
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, tr("脚本文件运行完成!"));
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, tr("Script file run completed!"));
			}
			
		}
		else
		{
			//发送信息
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, tr("脚本文件运行失败!"));
			}
			else
			{
				mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, tr("Script file failed to run!"));
			}
			
		}
	}
}