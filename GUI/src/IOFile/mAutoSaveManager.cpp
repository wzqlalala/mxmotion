#include "mAutoSaveManager.h"

//QT
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//IOFile
#include "mIOFileManager.h"

using namespace MxFunctions;

namespace MechIOFile
{
	mAutoSaveManager* mAutoSaveManager::_instance = nullptr;

	bool mAutoSaveManager::hasRecoveryFile()
	{
		QFileInfo info(autoSaveFilePath());
		return info.exists() && info.isFile() && info.size() > 0;
	}

	void mAutoSaveManager::clearRecoveryFile()
	{
		QFile file(autoSaveFilePath());
		if (file.exists())
		{
			file.remove();
		}
	}

	mAutoSaveManager* mAutoSaveManager::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mAutoSaveManager;
		}
		return _instance;
	}

	void mAutoSaveManager::setAutoSaveEnabled(bool enable)
	{
		_enabled = enable;
		if (enable)
		{
			//开启时重启定时器，保证间隔立即生效
			_timer->start(_intervalMinutes * 60 * 1000);
		}
		else
		{
			_timer->stop();
		}
	}

	void mAutoSaveManager::setIntervalMinutes(int minutes)
	{
		if (minutes < 1)
		{
			minutes = 1;
		}
		_intervalMinutes = minutes;
		if (_enabled)
		{
			_timer->start(_intervalMinutes * 60 * 1000);
		}
	}

	void mAutoSaveManager::triggerAutoSave()
	{
		if (!_enabled)
		{
			return;
		}
		//当前有未完成的读/写文件任务时跳过本轮，等待下一个周期
		if (mIOFileManager::getInstance()->hasRunningThread())
		{
			return;
		}
		//无当前模型、模型数据为空或处于后处理模型时不保存
		if (getCVM == nullptr || getCVM->getModelData() == nullptr || getCVM->isPostModel())
		{
			return;
		}
		//确保自动保存文件夹存在
		QDir().mkpath(autoSaveDir());
		//复用工程文件写线程（mOMxCaeFile），不压缩（级别0）、不置灰界面、不输出提示，
		//避免定时保存干扰用户操作；写线程的释放由 mMenuWidget::slot_writeMxCaeFinished 统一处理
		mIOFileManager::getInstance()->writeMxcaeFile(autoSaveFilePath(), autoSaveCompressLevel());
	}

	void mAutoSaveManager::slot_timeout()
	{
		triggerAutoSave();
	}

	void mAutoSaveManager::slot_aboutToQuit()
	{
		_enabled = false;
		if (_timer != nullptr)
		{
			_timer->stop();
		}
		//正常退出时清除自动保存文件，下次启动不再弹恢复询问
		//（恢复询问只用于异常退出场景）
		clearRecoveryFile();
	}

	mAutoSaveManager::mAutoSaveManager()
	{
		_timer = new QTimer(this);
		_timer->setInterval(_intervalMinutes * 60 * 1000);
		connect(_timer, &QTimer::timeout, this, &mAutoSaveManager::slot_timeout);
		//软件正常退出（close/quit）时走该槽清理现场；崩溃退出不会触发，
		//自动保存文件得以保留供下次启动恢复
		connect(qApp, &QCoreApplication::aboutToQuit, this, &mAutoSaveManager::slot_aboutToQuit);
	}

	mAutoSaveManager::~mAutoSaveManager()
	{
		if (_instance == this)
		{
			_instance = nullptr;
		}
	}
}
