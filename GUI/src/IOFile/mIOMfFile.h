#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include <QThread>
#include <QString>
#include "mIOEnum.h"

namespace MechModelData
{
    class mModelData;
}
namespace MechIOFile
{

    class MIOFILE_EXPORT mOMfFile : public QThread
    {
        Q_OBJECT
    public:
        mOMfFile();
        explicit mOMfFile(QString fname);
        virtual ~mOMfFile();

        void run() override;

        // 是否导出成功
        bool isSuccessWrite() const;
        // 指定只导出哪些均质化模型（按模型名）；不设置/置空 = 导出全部
        void setExportModelNames(const QStringList& names);
        void setExportModelName(const QString& name);


        // 输出文件全路径
        QString getOutFilePath();

    signals:
        /*
         * 发送信号，反馈文件写入错误情况
         */
        void writeFileErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

    private:
        MechModelData::mModelData* _modelData;

        //写出的文件名
        QString _outputFileName;
        // 仅导出这些模型名；为空 = 导出全部
        QStringList _exportModelNames;

        //是否导出成功
        bool _isSuccessWrite;
    };

    /************************************* 导入 *****************************************/
    class MIOFILE_EXPORT mIMfFile : public QThread
    {
        Q_OBJECT
    public:
        mIMfFile();
        explicit mIMfFile(QString fname);
        virtual ~mIMfFile();
        void run() override;

        bool isSuccessRead() const;
        QString getInFilePath();
        QString errorString() const;
        QStringList takeWarnings();

    signals:
        void readFileErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

    private:
        QString _inputFileName;
        bool _isSuccessRead;
        QString _errorString;
        QStringList _warnings;
    };
} // namespace MechIOFile
