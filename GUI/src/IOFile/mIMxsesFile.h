#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "miofile_global.h"
#include "mIOEnum.h"
#include "mMxsesFormat.h"

//Qt
#include <QThread>
#include <QString>
#include <atomic>
#include <QMutex>

namespace MechIOFile
{
	/*
	* .mxses 会话文件读线程（仿 mIMxCaeFile）。
	* 只负责读盘、解压、反序列化到内存 MxsesFileData（模型数据保留为chunk，
	* 不实例化 mModelData），恢复编排由界面线程在收到完成信号后执行。
	*/
	class MIOFILE_EXPORT mIMxsesFile : public QThread
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mIMxsesFile(const QString& name);

		/*
		* 析构函数
		*/
		~mIMxsesFile();

		/*
		* 获取文件名
		*/
		QString getFileName();

		/*
		* 获取读取状态
		*/
		bool isSuccessReadFile();

		/*
		* 取走读取到的会话数据（仅读取成功后调用）
		*/
		const MxsesFileData& getFileData() const { return _data; }

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
		*/
		void readMxsesErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

	private:
		//文件名
		QString _filename;

		//读取文件是否成功
		bool _isSuccessRead = false;

		//读取到的会话数据
		MxsesFileData _data;

		//取消标志（退出程序时由界面线程置位，run()在检查点响应）
		std::atomic_bool _cancelRequested{ false };

		QMutex mutex;
	};
}
