#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
//Qt
#include <QThread>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QPair>
#include <QHash>
#include <QMultiHash>
#include <set>
#include <iostream>
#include <fstream>
#include <atomic>
#include <QMutex>
#include <QTreeWidgetItem>

#include "miofile_global.h"
#include "mIOEnum.h"
#include "mElementFunction.h"

namespace MechModelData
{
	class mModelData;
}


class RenderPickResultData;
class mMeshData;
namespace MechIOFile
{
    class MIOFILE_EXPORT mIMxCaeFile : public QThread
    {
        Q_OBJECT
    public:
        /*
        * 默认构造函数
        */
        mIMxCaeFile(const QString& name);

        /*
        * 析构函数
        */
        ~mIMxCaeFile();

        /*
        * 获取文件名
        */
        QString getFileName();

        /*
        * 获取读取状态
        */
        bool isSuccessReadFile();

        /*
        * 请求取消读取（退出程序时中断读线程，run()在检查点响应后置读取失败并返回）
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
        * 发送信号，反馈文件读取错误情况
        * 参数type表示错误的类型
        */
        void readMxcaeErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);


    private:
        //文件名
        QString _filename;

        //读取文件是否成功
        bool _isSuccessRead;

        //数据库
        MechModelData::mModelData* _modelData;

        //文件大小
        quint64 _fileSize;

        //取消标志（退出程序时由界面线程置位，run()在检查点响应）
        std::atomic_bool _cancelRequested{ false };

        QMutex mutex;
    };
}