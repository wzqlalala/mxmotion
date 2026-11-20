#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "miofile_global.h"
#include "mIOEnum.h"
#include "mElementFunction.h"
//QT
#include <QThread>
#include <QStringList>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QPair>
#include <QHash>
#include <QMultiHash>
#include <QSet>
#include <set>
#include <iostream>
#include <fstream>
#include <atomic>
#include <QMutex>
#include <QTreeWidgetItem>

namespace MechModelData
{
	class mModelData;
}

class RenderPickResultData;
class mMeshData;
namespace MechIOFile
{
    class MIOFILE_EXPORT mOMxCaeFile : public QThread
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数，用于保存
        */
        mOMxCaeFile();

        /*
        * 重载构造函数，用于另存为
        * compressLevel：压缩级别（qCompress），9为最高压缩（默认，正常保存），
        * 0为不压缩（zlib存储模式，自动保存使用，文件格式与读取端完全兼容）
        */
        mOMxCaeFile(const QString& name, int compressLevel = 9);

        /*
        *析构函数
        */
        ~mOMxCaeFile();

        /*
        * 获取文件名
        */
        QString getFileName();

        /*
        * 请求取消写入（退出程序时中断写线程，run()在检查点响应后放弃commit，
        * QSaveFile自动丢弃临时文件，磁盘上原文件不受影响）
        */
        void requestCancel() { _cancelRequested = true; }

        /*
        * 是否已请求取消
        */
        bool isCancelRequested() const { return _cancelRequested; }
    private:
        /*
        * 重载run函数
        */
        void run() override;

    signals:
        /*
        * 反馈文件写入错误情况
        */
        void writeMxcaeErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

    private:
        //文件名
        QString _fileName;

        //压缩级别（0不压缩~9最高压缩）
        int _compressLevel = 9;

        //取消标志（退出程序时由界面线程置位，run()在检查点响应）
        std::atomic_bool _cancelRequested{ false };

        QMutex mutex;
    };
}