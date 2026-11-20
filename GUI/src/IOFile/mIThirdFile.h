#pragma once
#include "miofile_global.h"
#include "mIOEnum.h"
#include "mBasicEnum.h"
//QT
#include <QThread>

using namespace MxFunctions;
namespace MechIOFile
{
	class mIMxFile;
	class MIOFILE_EXPORT mIThirdFile : public QThread
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mIThirdFile(const QString& filename, bool isMeshFile);

		/*
		* 析构函数
		*/
		~mIThirdFile();
		
		/*
		* 获取是否成功读取文件
		*/
		bool isSuccessReadFile();

		/*
		* 获取文件名
		*/
		QString getReadFileName();
		/*
		* 获取mx文件类型
		*/
		AnalysisModule getMxFileType();

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
		void readSolveErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

		/*
		* 发送信号，反馈成功读取的文件名
		*/
		void readFileSuccessSig(const QString& name);

	private:
		//打开的文件名
		QString _fileName;

		bool _isMeshFile = false;

		//读取文件是否成功
		bool _isSuccessRead;

		//读取mx文件的对象(隐式模块)
		mIMxFile* _mxFile;
	};
}
