#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
//Qt
#include <QThread>
#include <QStringList>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QPair>
#include <QHash>
#include <QMultiHash>
#include <set>
#include <iostream>
#include <fstream>
#include <QMutex>
#include <QTreeWidgetItem>

#include "miofile_global.h"
#include "mIOEnum.h"
#include "mElementFunction.h"
#include "mMeshIO.h"

namespace MechModelData
{
	class mModelData;
}


class RenderPickResultData;
class mMeshData;
namespace MechIOFile
{
	class MIOFILE_EXPORT mIMeshIOFile: public QThread
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
        mIMeshIOFile(const QString& name);

		/*
		* 析构函数
		*/
		~mIMeshIOFile();

		/*
		* 获取文件名
		*/
		QString getFileName();

        /*
        * 获取文件名
        */
        QString getFileType();

        /*
        * 获取读取到的部件数据
        */
        QVector<meshReadPart> getMeshParts();

		/*
		* 获取读取状态
		*/
		bool isSuccessReadFile();

	private:
        bool praseParts();
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

        /*
        * 发送信号，反馈成功读取的文件名
        */
        void readMeshSuccessSig(const QString& name);
	private:
		//文件名
		QString _filename;

        //文件类型
        QString _fileType;

        //部件信息
        QVector<meshReadPart> _parts;

		//读取文件是否成功
		bool _isSuccessRead;
	};
}