#include "mIMxCaeFile.h"

//QT
#include <mGlobalSignals.h>
#include <QFileInfo>
#include <QFile>
#include <QSet>
#include <QPair>
#include <QVector2D>
#include <QPointer>
#include "mGlobalVarible.h"
#include <QTextCodec>
#include <set>
#include <QMutexLocker>

//BasicData
#include "mModelData.h"
#include "mMeshData.h"
#include "mPartDataStruct.h"
#include "mParseMeshData.h"
//MechCommon
#include "mGlobalSignals.h"
#include "mOutputMessage.h"
#include "mWidgetOperation.h"
#include "mLanguageVarible.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mModelInOutSteam.h"

//PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace MxFunctions;
using namespace MechPreData;
using namespace MechCommon;
using namespace MechPreData;
namespace MechIOFile
{
    mIMxCaeFile::mIMxCaeFile(const QString& name) :_filename(name)
    {
        _isSuccessRead = true;
        //初始化数据库
        _modelData = nullptr;
    }

    mIMxCaeFile::~mIMxCaeFile()
    {
    }

    QString mIMxCaeFile::getFileName()
    {
        return _filename;
    }

    bool mIMxCaeFile::isSuccessReadFile()
    {
        return _isSuccessRead;
    }

    void mIMxCaeFile::run()
    {
        mGlobalSignals::getInstance()->outputMessageSig(QString("开始读取文件： %1").arg(_filename));
        QFile file(_filename);
        if (!file.open(QIODevice::ReadOnly)) {
            emit readMxcaeErrorSig(FileNotOpen);
            return;
        }

        QDataStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        in.setVersion(QDataStream::Qt_5_13);
#else
        in.setVersion(QDataStream::Qt_5_12);
#endif

        in >> FILE_VERSION;
        if (FILE_VERSION > 0x3000) {
            emit readMxcaeErrorSig(FileVersionFalse);
            return;
        }

        //取消检查点：后续整文件读取不可分段中断，读取前先响应取消请求
        if (_cancelRequested) {
            _isSuccessRead = false;
            file.close();
            return;
        }

        QByteArray compressedData;
        in >> compressedData;
        file.close();

        //解压缩数据
        QByteArray input = compressedData;
        QByteArray decompressed;
        int pos = 0;
        while (pos + 4 <= input.size()) {
            //取消检查点：逐压缩块响应，中断后按读取失败处理
            if (_cancelRequested) {
                _isSuccessRead = false;
                return;
            }
            quint32 blockLen;
            memcpy(&blockLen, input.constData() + pos, 4);
            pos += 4;
            if (pos + blockLen > input.size()) break;
            QByteArray block = input.mid(pos, blockLen);
            QByteArray original = qUncompress(block);
            decompressed.append(original);
            pos += blockLen;
        }

        //从 QByteArray 反序列化数据
        QDataStream dataStream(decompressed);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        dataStream.setVersion(QDataStream::Qt_5_13);
#else
        dataStream.setVersion(QDataStream::Qt_5_12);
#endif

        try {
            QMutexLocker locker(&mutex);
            _modelData = getCVM->getModelData();

            //单模型格式：直接读取一个模型数据块
            QByteArray chunk;
            dataStream >> chunk;
            //数据块读取失败或为空说明文件格式不匹配/损坏
            if (dataStream.status() != QDataStream::Ok || chunk.isEmpty()) {
                emit readMxcaeErrorSig(FileVersionFalse);
                return;
            }
            //取消检查点：反序列化前最后响应一次取消请求
            if (_cancelRequested) {
                _isSuccessRead = false;
                return;
            }
            if (_modelData != nullptr) {
                QDataStream chunkStream(chunk);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
                chunkStream.setVersion(QDataStream::Qt_5_13);
#else
                chunkStream.setVersion(QDataStream::Qt_5_12);
#endif

                //记录当前模型的名称，反序列化后恢复——
                //打开文件时模型名称使用当前模型原本的，不被文件中的值覆盖
                //（模型ID不参与序列化，构造时分配后不会改变）
                const QString curModelDataName = _modelData->getModelDataName();
                chunkStream >> *_modelData;
                _modelData->setModelDataName(curModelDataName);
            }
        }
        catch (...) {
            emit readMxcaeErrorSig(UnknownError);
        }
    }

}
