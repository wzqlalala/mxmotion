#include "mOMxCaeFile.h"
#include "mAutoSaveManager.h"

//QT
#include <QFileInfo>
#include <QSaveFile>
#include <QMutexLocker>

//全局信号
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"
#include "mBasicEnum.h"
#include "mGlobalVarible.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mPartDataStruct.h"
#include "mParseMeshData.h"
#include "mModelInOutSteam.h"
#include "mUnitIoCodec.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace MechCommon;
using namespace std;

const quint32 WARE_VERSION = 0x3000;
namespace MechIOFile
{
    mOMxCaeFile::mOMxCaeFile()
    {
        //转化成绝对路径
        QFileInfo finfo(_fileName);
        _fileName = finfo.absoluteFilePath();
    }

    mOMxCaeFile::mOMxCaeFile(const QString& name, int compressLevel)
        :_fileName(name), _compressLevel(compressLevel)
    {
        //转化成绝对路径
        QFileInfo finfo(_fileName);
        _fileName = finfo.absoluteFilePath();
    }

    mOMxCaeFile::~mOMxCaeFile()
    {
    }

    QString mOMxCaeFile::getFileName()
    {
        return _fileName;
    }


    void mOMxCaeFile::run()
    {
        //取消检查点：已请求取消则不再执行（尤其须跳过下方BlockingQueuedConnection
        //阻塞等待界面线程的发射，避免与界面线程的等待逻辑互锁）
        if (_cancelRequested) {
            return;
        }

        //写前通知界面，阻塞等待界面处理完成（与mOThirdFile写mx文件一致）：
        //提交当前面板未保存的拾取数据，保证工程文件包含最新数据
        emit MechCommon::mMechGlobalSignals::getInstance()->requestPrepareWriteFile();

        QMutexLocker locker(&mutex);

        //仅保存当前模型数据
        MechModelData::mModelData* modelData = nullptr;
        if (getCVM != nullptr && getCVM->getModelData() != nullptr)
        {
            modelData = getCVM->getModelData();
        }
        if (modelData == nullptr)
        {
            emit writeMxcaeErrorSig(UnknownError);
            return;
        }

        QByteArray dataBuffer;
        QDataStream dataStream(&dataBuffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        dataStream.setVersion(QDataStream::Qt_5_13);
#else
        dataStream.setVersion(QDataStream::Qt_5_12);
#endif

        if (_cancelRequested) {
            mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
            return;
        }
        QByteArray chunk;
        QDataStream chunkStream(&chunk, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        chunkStream.setVersion(QDataStream::Qt_5_13);
#else
        chunkStream.setVersion(QDataStream::Qt_5_12);
#endif

        chunkStream << *modelData;
        dataStream << chunk;

        //压缩数据（自动保存时压缩级别为0，即不压缩的zlib存储模式，格式与读取端兼容）
        const int blockSize = 4 * 1024 * 1024;
        int numBlocks = (dataBuffer.size() + blockSize - 1) / blockSize;
        QVector<QByteArray> compressedBlocks(numBlocks);

#pragma omp parallel for
        for (int i = 0; i < numBlocks; ++i) {
            //取消检查点：已请求取消则跳过剩余块的压缩，尽快结束线程
            if (_cancelRequested) {
                continue;
            }
            QByteArray chunk = dataBuffer.mid(i * blockSize, blockSize);
            QByteArray compressedData = qCompress(chunk, _compressLevel);
            compressedBlocks[i] = compressedData;
        }

        //压缩阶段被取消，不再组装写入
        if (_cancelRequested) {
            mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
            return;
        }

        QByteArray allCompressed;
        for (const QByteArray& block : compressedBlocks) {
            quint32 blen = block.size();
            allCompressed.append(reinterpret_cast<const char*>(&blen), sizeof(blen));
            allCompressed.append(block);
        }


        QSaveFile file(_fileName);
        //取消检查点：开写前最后确认，取消则不产生任何落盘动作
        if (_cancelRequested || !file.open(QIODevice::WriteOnly)) {
            if (_cancelRequested) {
                mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
                return;
            }
            emit writeMxcaeErrorSig(FileNotOpen);
            return;
        }

        QDataStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        out.setVersion(QDataStream::Qt_5_13);
#else
        out.setVersion(QDataStream::Qt_5_12);
#endif

        // 写入文件头
        out << WARE_VERSION;

        // 写入压缩后的数据块
        out << allCompressed;

        //取消检查点：不commit，QSaveFile析构时自动丢弃临时文件，原文件保持完整
        if (_cancelRequested) {
            mGlobalSignals::getInstance()->outputMessageSig(QString("写入任务已取消： %1").arg(_fileName));
            return;
        }

        if (!file.commit()) {
            qWarning("Failed to commit save file: %s", qPrintable(file.errorString()));
            return;
        }
    }
}
