#include "mIOFileManager.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QDataStream>
#include <QByteArray>
#include <mAppConfiguration.h>
#include <QCoreApplication>

#include "mIMxCaeFile.h"
#include "mOMxCaeFile.h"
#include "mIMxsesFile.h"
#include "mOMxsesFile.h"
#include "mIThirdFile.h"
#include "mOThirdFile.h"
#include "mIMotionJsonFile.h"
#include "mOMotionJsonFile.h"
#include "mMechGlobalSignals.h"
#include "mIMeshIOFile.h"
#include "mIMxFile.h"
#include "mOMxFile.h"
#include "mModelInOutSteam.h"
#include "mIOMfFile.h"

#include <QDataStream>
//MechPreData
#include "mElementFunction.h"
//MechCommon
#include "mOutputMessage.h"
#include "mOtherFunction.h"
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
//BasicData
#include "mModelInOutSteam.h"


using namespace MAppConfiguration;
using namespace MxFunctions;
using namespace MechPreData;
using namespace MechCommon;
namespace MechIOFile
{
    mIOFileManager* mIOFileManager::_instance = nullptr;

    mIOFileManager* mIOFileManager::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mIOFileManager;
        }
        return _instance;
    }
    bool mIOFileManager::hasRunningWrtiteMxcaeThread()
    {
        if (_omxcaeThread != nullptr)
        {
            return true;
        }
        return false;
    }

    bool mIOFileManager::hasRunningThread()
    {
        if (_imxcaeThread != nullptr && _imxcaeThread->isRunning())
        {
            return true;
        }
        if (_omxcaeThread != nullptr && _omxcaeThread->isRunning())
        {
            return true;
        }
        if (_ithirdThread != nullptr && _ithirdThread->isRunning())
        {
            return true;
        }
        if (_othirdThread != nullptr && _othirdThread->isRunning())
        {
            return true;
        }
        if (_imeshThread != nullptr && _imeshThread->isRunning())
        {
            return true;
        }
        if (_imxsesThread != nullptr && _imxsesThread->isRunning())
        {
            return true;
        }
        if (_omxsesThread != nullptr && _omxsesThread->isRunning())
        {
            return true;
        }
        if (_oMfThread != nullptr && _oMfThread->isRunning())
        {
            return true;
        }
        if (_iMfThread != nullptr && _iMfThread->isRunning())
        {
            return true;
        }
        return false;
    }

    void mIOFileManager::killFileThread()
    {
        //取消并等待线程结束。两点说明：
        //1）等待前先断开该线程到管理器的全部连接，防止等待期间分派事件时
        //   完成槽（release*Thread/slot_*Finished）重入删除线程对象；
        //2）等待期间分派非用户输入事件，使写线程起始的requestPrepareWriteFile
        //   （BlockingQueuedConnection，阻塞等待界面线程）能被处理完毕，避免互等死锁
        auto cancelAndWait = [this](QThread* thread)
            {
                QObject::disconnect(thread, nullptr, this, nullptr);
                while (!thread->wait(50))
                {
                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
                }
            };

        if (_imxcaeThread != nullptr)
        {
            _imxcaeThread->requestCancel();
            cancelAndWait(_imxcaeThread);
            delete _imxcaeThread;
            _imxcaeThread = nullptr;
        }
        if (_omxcaeThread != nullptr)
        {
            _omxcaeThread->requestCancel();
            cancelAndWait(_omxcaeThread);
            delete _omxcaeThread;
            _omxcaeThread = nullptr;
        }
        if (_imxsesThread != nullptr)
        {
            _imxsesThread->requestCancel();
            cancelAndWait(_imxsesThread);
            delete _imxsesThread;
            _imxsesThread = nullptr;
        }
        if (_omxsesThread != nullptr)
        {
            _omxsesThread->requestCancel();
            cancelAndWait(_omxsesThread);
            delete _omxsesThread;
            _omxsesThread = nullptr;
        }
        bool threadFlagSet = false;
        if (_ithirdThread != nullptr)
        {
            MAppConfiguration::mAppConfiguration::getInstance()->threadFlag = false;
            threadFlagSet = true;
            cancelAndWait(_ithirdThread);
            delete _ithirdThread;
            _ithirdThread = nullptr;
        }
        if (_othirdThread != nullptr)
        {
            MAppConfiguration::mAppConfiguration::getInstance()->threadFlag = false;
            threadFlagSet = true;
            cancelAndWait(_othirdThread);
            delete _othirdThread;
            _othirdThread = nullptr;
        }
        if (_oMfThread != nullptr)
        {
            MAppConfiguration::mAppConfiguration::getInstance()->threadFlag = false;
            threadFlagSet = true;
            cancelAndWait(_oMfThread);
            delete _oMfThread;
            _oMfThread = nullptr;
        }
        if (_imeshThread != nullptr)
        {
            _imeshThread->quit();
            _imeshThread->wait();
            _imeshThread = nullptr;
        }
        if (_iMfThread != nullptr)
        {
            MAppConfiguration::mAppConfiguration::getInstance()->threadFlag = false;
            threadFlagSet = true;
            cancelAndWait(_iMfThread);
            delete _iMfThread;
            _iMfThread = nullptr;
        }
        //threadFlag置false后必须复位，否则后续读写线程一进入循环就判为已取消
        if (threadFlagSet)
        {
            MAppConfiguration::mAppConfiguration::getInstance()->threadFlag = true;
        }
    }

    void mIOFileManager::readMxcaeFile(const QString& filename)
    {
        _imxcaeThread = new mIMxCaeFile(filename);
        //在读线程释放前记录本次读取结果：该连接先于下方的finished转发建立，
        //保证 slot_readMxCaeFinished 释放线程后 lastReadMxcaeSucceeded 仍可查询
        connect(_imxcaeThread, &QThread::finished, this, [this]()
            {
                _lastReadMxcaeSuccess = (_imxcaeThread != nullptr && _imxcaeThread->isSuccessReadFile());
            });
        connect(_imxcaeThread, SIGNAL(readMxcaeErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
        connect(_imxcaeThread, SIGNAL(finished()), this, SIGNAL(readMxcaeFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _imxcaeThread->start();
    }

    void mIOFileManager::writeCustomMaterialFile(const QString& filename, std::shared_ptr<MechMaterialData::mMaterialBase> mat)
    {
        mOMxFile* writer = new mOMxFile(filename);
        writer->writeMaterialToMatFile(filename, mat);
    }

    void mIOFileManager::readCustomMaterialFile(const QString& filename, std::shared_ptr<MechMaterialData::mMaterialBase>& mat)
    {
        mIMxFile* reader = new mIMxFile();
        mat = reader->ReadCustomMaterialData(filename);
    }

    bool mIOFileManager::isSuccessReadMxcae()
    {
        if (_imxcaeThread != nullptr)
        {
            return _imxcaeThread->isSuccessReadFile();
        }
        return false;
    }

    QString mIOFileManager::getReadMxcaeFileName()
    {
        if (_imxcaeThread != nullptr)
        {
            return _imxcaeThread->getFileName();
        }
        return QString();
    }

    void mIOFileManager::writeMxcaeFile(const QString& filename, int compressLevel)
    {
        _omxcaeThread = new mOMxCaeFile(filename, compressLevel);
        connect(_omxcaeThread, SIGNAL(writeMxcaeErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
        connect(_omxcaeThread, SIGNAL(finished()), this, SIGNAL(writeMxcaeFinishedSig()));
        //写工程文件期间不置灰界面（原modelDataThreadStatusChangedSig(true)已取消，
        //读取/求解等其他操作的置灰逻辑不受影响）
        _omxcaeThread->start();
    }

    QString mIOFileManager::getWriteMxcaeFileName()
    {
        if (_omxcaeThread != nullptr)
        {
            return _omxcaeThread->getFileName();
        }
        return QString();
    }

    void mIOFileManager::releaseReadMxcaeThread()
    {
        if (_imxcaeThread != nullptr)
        {
            disconnect(_imxcaeThread);
            _imxcaeThread->quit();
            delete _imxcaeThread;
            _imxcaeThread = nullptr;
        }
    }

    void mIOFileManager::releaseWriteMxcaeThread()
    {
        if (_omxcaeThread != nullptr)
        {
            disconnect(_omxcaeThread);
            _omxcaeThread->quit();
            delete _omxcaeThread;
            _omxcaeThread = nullptr;
        }
    }

    void mIOFileManager::readMxsesFile(const QString& filename)
    {
        //上一个会话读线程未释放（运行中或完成信号尚未处理）时拒绝新读取：
        //继续会覆盖 _imxsesThread 指针，完成槽会误删新线程导致闪退
        if (_imxsesThread != nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("当前有未完成的会话文件操作"));
            return;
        }
        _imxsesThread = new mIMxsesFile(filename);
        connect(_imxsesThread, SIGNAL(readMxsesErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
        connect(_imxsesThread, SIGNAL(finished()), this, SIGNAL(readMxsesFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _imxsesThread->start();
    }

    void mIOFileManager::writeMxsesFile(const QString& filename, const MxsesFileData& data, int compressLevel)
    {
        //上一个会话写线程未释放时拒绝新写入（防连点保存覆盖指针后误删新线程）
        if (_omxsesThread != nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("当前有未完成的会话文件保存"));
            return;
        }
        _omxsesThread = new mOMxsesFile(filename, data, compressLevel);
        connect(_omxsesThread, SIGNAL(writeMxsesErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
        connect(_omxsesThread, SIGNAL(finished()), this, SIGNAL(writeMxsesFinishedSig()));
        _omxsesThread->start();
    }

    bool mIOFileManager::isSuccessReadMxses()
    {
        if (_imxsesThread != nullptr)
        {
            return _imxsesThread->isSuccessReadFile();
        }
        return false;
    }

    QString mIOFileManager::getReadMxsesFileName()
    {
        if (_imxsesThread != nullptr)
        {
            return _imxsesThread->getFileName();
        }
        return QString();
    }

    QString mIOFileManager::getWriteMxsesFileName()
    {
        if (_omxsesThread != nullptr)
        {
            return _omxsesThread->getFileName();
        }
        return QString();
    }

    const MxsesFileData& mIOFileManager::getMxsesFileData() const
    {
        return _imxsesThread->getFileData();
    }

    void mIOFileManager::releaseReadMxsesThread()
    {
        if (_imxsesThread != nullptr)
        {
            disconnect(_imxsesThread);
            _imxsesThread->quit();
            delete _imxsesThread;
            _imxsesThread = nullptr;
        }
    }

    void mIOFileManager::releaseWriteMxsesThread()
    {
        if (_omxsesThread != nullptr)
        {
            disconnect(_omxsesThread);
            _omxsesThread->quit();
            delete _omxsesThread;
            _omxsesThread = nullptr;
        }
    }

    bool mIOFileManager::isSuccessReadSolve()
    {
        if (_ithirdThread != nullptr)
        {
            return _ithirdThread->isSuccessReadFile();
        }
        return false;
    }

	QString mIOFileManager::getReadSolveFileName()
	{
		if (_ithirdThread != nullptr)
		{
			return _ithirdThread->getReadFileName();
		}
		//读取线程已释放，返回持久记录的最近一次导入文件名
		return _lastReadSolveFileName;
	}

    void mIOFileManager::releaseReadSolveThread()
    {
        if (_ithirdThread != nullptr)
        {
            disconnect(_ithirdThread);
            _ithirdThread->quit();
            delete _ithirdThread;
            _ithirdThread = nullptr;
        }
    }

    bool mIOFileManager::isSuccessWriteSolve()
    {
        if (_othirdThread != nullptr)
        {
            return _othirdThread->isWriteSuccess();
        }
        return false;
    }

    QString mIOFileManager::getWriteSolveFileName()
    {
        if (_othirdThread != nullptr)
        {
            return _othirdThread->getOutFilePath();
        }
        return QString();
    }


    void mIOFileManager::releaseWriteSolveThread()
    {
        if (_othirdThread != nullptr)
        {
            disconnect(_othirdThread);
            _othirdThread->quit();
            delete _othirdThread;
            _othirdThread = nullptr;
        }
    }


    mIOFileManager::mIOFileManager()
    {
        _imxcaeThread = nullptr;
        _omxcaeThread = nullptr;
        _ithirdThread = nullptr;
        _othirdThread = nullptr;
        _imeshThread = nullptr;
        _oMfThread = nullptr;
        _iMfThread = nullptr;
        //获取几何装配树，用于写二进制文件
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(AddGeoTreeItemSig(QTreeWidgetItem*)), this, SLOT(slot_AddGeoTreeItem(QTreeWidgetItem*)));
    }

    mIOFileManager::~mIOFileManager()
    {
        this->killFileThread();
    }

    void mIOFileManager::importFile(const QString& fname, bool isMeshFile)
    {
        //判断当前是否有读文件线程
        if (_ithirdThread != nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "当前读文件线程未结束");
            return;
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "正在读取文件......");
        }

		//持久记录导入的计算文件名（读取线程释放后仍可查询）
		_lastReadSolveFileName = fname;

		//读取文件
		_ithirdThread = new mIThirdFile(fname, isMeshFile);
		connect(_ithirdThread, SIGNAL(readSolveErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
		connect(_ithirdThread, SIGNAL(finished()), this, SIGNAL(readSolveFinishedSig()));

        //判断文件类型和当前分析类型是否一致
        AnalysisModule filetype = _ithirdThread->getMxFileType();
        if (filetype == ImplicitAnalysis)
        {
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
            //开始读文件
            _ithirdThread->start();
        }
        else
        {
            if (filetype == UnknownAnalysis)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "文件格式不符合规范");
                delete _ithirdThread;
                _ithirdThread = nullptr;
            }
            else
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, "该文件格式与当前分析类型不匹配！");
                delete _ithirdThread;
                _ithirdThread = nullptr;
            }

        }
    }

    void mIOFileManager::slot_exportMfFile(const QString& fname, const QString& modelName)
    {
        _oMfThread = new mOMfFile(fname);
        if (!modelName.isEmpty())
            _oMfThread->setExportModelName(modelName);
        connect(_oMfThread, SIGNAL(writeFileErrorSig(IOFileErrorType)), this, SLOT(slot_ioFileError(IOFileErrorType)));
        connect(_oMfThread, SIGNAL(finished()), this, SIGNAL(writeMfFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _oMfThread->start();
        emit exportFileNameSig(fname);
    }

    bool mIOFileManager::isSuccessWriteMf()
    {
        if (_oMfThread != nullptr)
        {
            return _oMfThread->isSuccessWrite();
        }
        return false;
    }

    QString mIOFileManager::getWriteMfFileName()
    {
        if (_oMfThread != nullptr)
        {
            return _oMfThread->getOutFilePath();
        }
        return QString();
    }

    void mIOFileManager::releaseWriteMfThread()
    {
        if (_oMfThread != nullptr)
        {
            disconnect(_oMfThread);
            _oMfThread->quit();
            delete _oMfThread;
            _oMfThread = nullptr;
        }
    }

    bool mIOFileManager::isSuccessReadMf()
    {
        if (_iMfThread != nullptr)
        {
            return _iMfThread->isSuccessRead();
        }
        return false;
    }

    QString mIOFileManager::getReadMfFileName()
    {
        if (_iMfThread != nullptr)
        {
            return _iMfThread->getInFilePath();
        }
        return QString();
    }

    void mIOFileManager::releaseReadMfThread()
    {
        if (_iMfThread != nullptr)
        {
            disconnect(_iMfThread);
            _iMfThread->quit();
            delete _iMfThread;
            _iMfThread = nullptr;
        }
    }

    void mIOFileManager::slot_importMfFile(const QString& fname)
    {
        //判断当前是否有读文件线程
        if (_iMfThread != nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "当前读文件线程未结束");
            return;
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "正在读取文件......");
        }

        //读取文件
        _iMfThread = new mIMfFile(fname);
        connect(_iMfThread, SIGNAL(readFileErrorSig(IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(IOFileErrorType)));
        connect(_iMfThread, SIGNAL(finished()), this, SIGNAL(readMfFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _iMfThread->start();
    }

    void mIOFileManager::slot_importSolveFile(const QString& fname)
    {
        importFile(fname);
    }

    void mIOFileManager::slot_importMeshFile(const QString& fname)
    {
		importFile(fname,true);
    }

    void mIOFileManager::readMeshIOFile(const QString& filename)
    {
        //判断当前是否有读文件线程
        if (_imeshThread != nullptr && _imeshThread->isRunning())
        {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "当前读文件线程未结束");
            return;
        }

        _imeshThread = new mIMeshIOFile(filename);
        connect(_imeshThread, SIGNAL(readMeshErrorSig(MechCommon::IOFileErrorType)), this, SIGNAL(ioFileErrorManagerSig(MechCommon::IOFileErrorType)));
        connect(_imeshThread, SIGNAL(finished()), this, SIGNAL(readMeshFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _imeshThread->start();
    }

    bool mIOFileManager::isSuccessReadMesh()
    {
        if (_imeshThread != nullptr)
        {
            return _imeshThread->isSuccessReadFile();
        }
        return false;
    }

    QString mIOFileManager::getReadMeshFileName()
    {
        if (_imeshThread != nullptr)
        {
            return _imeshThread->getFileName();
        }
        return QString();
    }

    QVector<meshReadPart> mIOFileManager::getMeshParts()
    {
        if (_imeshThread != nullptr)
        {
            return _imeshThread->getMeshParts();
        }
        return QVector<meshReadPart>();
    }

    void mIOFileManager::releaseReadMeshThread()
    {
        if (_imeshThread != nullptr)
        {
            disconnect(_imeshThread);
            _imeshThread->quit();
            delete _imeshThread;
            _imeshThread = nullptr;
        }
    }


	void mIOFileManager::slot_exportSolveFile(const QString & fname)
    {
        _othirdThread = new mOThirdFile(fname);
        connect(_othirdThread, SIGNAL(writeFileErrorSig(IOFileErrorType)), this, SLOT(slot_ioFileError(IOFileErrorType)));
        connect(_othirdThread, SIGNAL(finished()), this, SIGNAL(writeSolveFinishedSig()));
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        _othirdThread->start();
        emit exportFileNameSig(fname);
    }

    void mIOFileManager::slot_AddGeoTreeItem(QTreeWidgetItem* item)
    {
        _modelItem = item;
    }

    void mIOFileManager::slot_importMotionJsonFile(const QString& fname, bool fromPlatform)
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "模型数据为空，无法导入运动数据");
            return;
        }
        _motionJsonPath = fname;
        _motionJsonFromPlatform = fromPlatform;
        _imotionJsonThread = new mIMotionJsonFile(fname, modelData);
        connect(_imotionJsonThread, SIGNAL(importFinished(bool, QString)), this, SLOT(slot_motionJsonImportFinished(bool, QString)));
        connect(_imotionJsonThread, SIGNAL(finished()), this, SIGNAL(readMotionJsonFinishedSig()));
        connect(_imotionJsonThread, &QThread::finished, _imotionJsonThread, &QObject::deleteLater);
        _imotionJsonThread->start();
    }

    void mIOFileManager::slot_exportMotionJsonFile(const QString& fname, bool autoSave)
    {
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData) {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, "模型数据为空，无法导出运动数据");
            return;
        }
        _motionJsonExportPath = fname;
        _omotionJsonThread = new mOMotionJsonFile(fname, modelData);
        connect(_omotionJsonThread, SIGNAL(exportFinished(bool, QString)), this, SLOT(slot_motionJsonExportFinished(bool, QString)));
        connect(_omotionJsonThread, SIGNAL(finished()), this, SIGNAL(readMotionJsonFinishedSig()));
        connect(_omotionJsonThread, &QThread::finished, _omotionJsonThread, &QObject::deleteLater);
        _omotionJsonThread->start();
        emit exportFileNameSig(fname);
    }

    void mIOFileManager::slot_motionJsonImportFinished(bool success, const QString& msg)
    {
        if (success) {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, msg);
            mGlobalSignals::getInstance()->updateModelTree();
        }
        else {
            mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, msg);
        }

        // mpcomm 平台会话中，运动数据导入完成回传 committed(mesh+setup)。
        // fromPlatform 标记本次是否平台触发，桥接据此前置不回传（防回声）。
        auto* sigs = MechCommon::mMechGlobalSignals::getInstance();
        const QString path = _motionJsonPath;
        const bool fromPlatform = _motionJsonFromPlatform;
        QMetaObject::invokeMethod(sigs, [sigs, success, path, fromPlatform]() {
            emit sigs->mpcommImportMotionDataFinished(success, path, fromPlatform);
            }, Qt::QueuedConnection);
    }

    void mIOFileManager::slot_motionJsonExportFinished(bool success, const QString& msg)
    {
        if (success) {
            mGlobalSignals::getInstance()->outputMessageSig(NormalMessage, msg);
        }
        else {
            mGlobalSignals::getInstance()->outputMessageSig(ErrorMessage, msg);
        }

        // 导出（手动或求解前自动）完成后，回传 committed(mesh+setup)：
        // 导出的 json 就是当前的网格+设置输入。
        if (success && !_motionJsonExportPath.isEmpty()) {
            auto* sigs = MechCommon::mMechGlobalSignals::getInstance();
            const QString path = _motionJsonExportPath;
            QMetaObject::invokeMethod(sigs, [sigs, path]() {
                emit sigs->mpcommImportMotionDataFinished(true, path, false);
                }, Qt::QueuedConnection);
        }
    }

    void mIOFileManager::copyModelData(MechModelData::mModelData*& oldModel, MechModelData::mModelData*& newModel) {
        QByteArray dataBuffer;
        QDataStream dataStream(&dataBuffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        dataStream.setVersion(QDataStream::Qt_5_13);
#else
        dataStream.setVersion(QDataStream::Qt_5_12);
#endif
        dataStream << *oldModel;
        QDataStream outdataStream(&dataBuffer, QIODevice::ReadOnly);
        outdataStream >> *newModel;

    }
}
