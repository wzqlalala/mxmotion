#pragma once
#include "miofile_global.h"
#include "mIOEnum.h"
#include "mBasicEnum.h"

//QT
#include <QThread>
#include <vector>

namespace MechIOFile
{
	class mOMxFile;
	class MIOFILE_EXPORT mOThirdFile : public QThread
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mOThirdFile(const QString& filename);

		/*
		* 析构函数
		*/
		~mOThirdFile();

		/*
		* 是否写入成功
		*/
		bool isWriteSuccess();

		/*
		* 获取文件名
		*/
		QString getOutFilePath();

		/*
		* 设置工况过滤列表（只写这些工况；为空=写全部工况），
		* 必须在start()之前调用（写文件在子线程中进行）
		*/
		void setCaseFilter(const std::vector<int>& caseIDs);

	private:
		/*
		* 重载run函数
		*/
		void run() override;

	signals:
		/*
		* 发送信号，反馈文件写入错误情况
		*/
		void writeFileErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

		/*
		* 发送信号，反馈文件写入成功情况
		*/
		void writeFileSuccessSig(const QString& name);

	private:
		//写入的文件名
		QString _fileName;

		//是否导出成功
		bool _isSuccessWrite;

		//场的名称
		QString _fieldName;

		//工况过滤列表（空=写全部工况）
		std::vector<int> _caseFilter;

		//写入mx文件(隐式模块)
		mOMxFile* _mxFile;
	};
}