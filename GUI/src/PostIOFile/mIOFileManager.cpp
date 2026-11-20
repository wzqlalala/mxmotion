#include "mIOFileManager.h"
#include "mIMxdbFile1.h"
#include "mIF06File.h"
#include "mGlobalSignals.h"
#include "mDataPost1.h"

using namespace MxFunctions;
namespace MIOFile
{
	mIOFileManager* mIOFileManager::_instance = nullptr;
	std::mutex mIOFileManager::_mutex_mIOFileManager;
	mIOFileManager* mIOFileManager::getInstance()
	{
		std::lock_guard<std::mutex> lock(_mutex_mIOFileManager);
		if (_instance == nullptr)
			_instance = new mIOFileManager;
		return _instance;
	}

	bool mIOFileManager::hasRunningThread()
	{
		return false;
	}

	void mIOFileManager::killFileThread()
	{}

	void mIOFileManager::readMxdbFile(const QString & initfile, const QStringList & framefiles, QPointer<mPostDataBase> modeldb)
	{
		mIMxdbFile1* mxdbThread1 = new mIMxdbFile1(initfile, framefiles, modeldb);
		QThread *thread = new QThread;
		mxdbThread1->moveToThread(thread);
		connect(this, &mIOFileManager::readMxdbFileManagerSig, mxdbThread1, &mIMxdbFile1::slot_importAllMxDbFile, Qt::QueuedConnection);
		connect(mxdbThread1, SIGNAL(readMxdbErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
		connect(mxdbThread1, SIGNAL(readFileSuccessSig(const QString&)), this, SIGNAL(readFileSuccessSig(const QString&)));
		connect(mxdbThread1, SIGNAL(finishedMxdbFileSig(const QString&)), this, SIGNAL(finishedMxdbManagerSig(const QString&)));
		connect(mxdbThread1, SIGNAL(finishedAllMxdbSig(const QString&)), this, SIGNAL(finishedAllMxdbManagerSig(const QString&)));
		thread->start();
		_mxdbThreads.insert(mxdbThread1, thread);

	}

	QHash<mIMxdbFile1*, QThread*> mIOFileManager::getMxdbThreads()
	{
		return _mxdbThreads;
	}

	mIMxdbFile1 * mIOFileManager::getMxdb(const QString & postname)
	{
		foreach(mIMxdbFile1* mxdbFile, _mxdbThreads.keys())
		{
			if (mxdbFile == nullptr)
				return nullptr;
			if (mxdbFile->getPostDataName() == postname)
				return mxdbFile;
		}
		return nullptr;
	}


	QThread * mIOFileManager::getThread1(mIMxdbFile1 * mxdb)
	{
		return _mxdbThreads.value(mxdb, nullptr);
	}

	void MIOFile::mIOFileManager::appendDataPost(PostData::mDataPost1 * datapost)
	{
		if (datapost == nullptr)
			return;
		for (PostData::mDataPost1* dp : _dataPosts)
		{

			if (dp != nullptr && dp->getModelName() == datapost->getModelName())
			{
				signal_updateHightTreeByRepeatName(dp->getModelName());
				_dataPosts.replace(_dataPosts.indexOf(dp), datapost);
				return;
			}
		}
		_dataPosts.append(datapost);
	}

	PostData::mDataPost1 * MIOFile::mIOFileManager::getDataPostFromName(const QString & name)
	{
		for (PostData::mDataPost1* dp : _dataPosts)
		{
			if (dp != nullptr&&dp->getModelName() == name)
				return dp;
		}
		return nullptr;
	}

	QStringList MIOFile::mIOFileManager::getPostDataNames()
	{
		QStringList names;
		for (PostData::mDataPost1* dp : _dataPosts)
		{
			if (dp != nullptr)
				names.append(dp->getModelName());
		}
		return names;
	}

	void MIOFile::mIOFileManager::deleteDataPost(const QString & name)
	{
		for (PostData::mDataPost1* dp : _dataPosts)
		{
			if (dp != nullptr && dp->getModelName() == name)
			{
				_dataPosts.removeOne(dp);
				return;
			}
		}
	}
	bool MIOFile::mIOFileManager::isDatabaseEmpty()
	{
		if (_dataPosts.isEmpty())
			return true;
		return false;
	}
	bool MIOFile::mIOFileManager::isExist(const QString & postname)
	{
		for (auto iter : _dataPosts)
		{
			if (iter->getModelName() == postname)
				return true;
		}
		return false;
	}

	PostData::mDataPost1 * MIOFile::mIOFileManager::getmDataPostByName(const QString & postname)
	{
		for (int i = 0; i < _dataPosts.size(); i++)
		{
			if (_dataPosts[i]->getModelName() == postname)
			{
				_dataPosts.removeOne(_dataPosts[i]);
				return _dataPosts[i];
			}
		}
		return nullptr;
	}

	void MIOFile::mIOFileManager::addmDataPost(PostData::mDataPost1 * postdata)
	{
		if (postdata != nullptr)
			_dataPosts.append(postdata);
	}

	void MIOFile::mIOFileManager::inital()
	{
		//connect(mViewportManager::getInstance(), SIGNAL(DataPosttoTemp(const QString&)), this, SLOT(StorageMoveToTemporary(const QString&)));
		//connect(mViewportManager::getInstance(), SIGNAL(TemptoDataPost(const QString&)), this, SLOT(TemporaryMoveToStorage(const QString&)));
	}

	void MIOFile::mIOFileManager::StorageMoveToTemporary(const QString & name)
	{
		for (int i = 0; i < _dataPosts.size(); i++)
		{
			if (_dataPosts[i]->getModelName() == name)
			{
				_tempList.append(_dataPosts[i]);
				_dataPosts.removeOne(_dataPosts[i]);
				return;
			}
		}
	}

	void MIOFile::mIOFileManager::TemporaryMoveToStorage(const QString & name)
	{
		for (int i = 0; i < _tempList.size(); i++)
		{
			if (_tempList[i]->getModelName() == name)
			{
				_dataPosts.append(_tempList[i]);
				_tempList.removeOne(_tempList[i]);
				return;
			}
		}

	}

	void MIOFile::mIOFileManager::deleteDataPost()
	{
		_dataPosts.clear();
	}


	void mIOFileManager::saveToDataPosttoRend()
	{
		QString postname;
		mIMxdbFile1* mxdb = qobject_cast<mIMxdbFile1*>(sender());
		if (mxdb != nullptr)
		{
			postname = mxdb->getPostDataName();

			QThread* thread = _mxdbThreads.value(mxdb);
			if (thread != nullptr)
			{
				disconnect(mxdb);
				thread->exit(1);
				thread->wait(2);
				delete mxdb;
				mxdb = nullptr;
				delete thread;
				thread = nullptr;
			}
		}
		mGlobalSignals::getInstance()->finishedReadPostFileSig(postname);
	}

	mIOFileManager::mIOFileManager()
	{
		inital();
		_dataPosts.clear();
	}

	mIOFileManager::~mIOFileManager()
	{
		this->killFileThread();
	}
}