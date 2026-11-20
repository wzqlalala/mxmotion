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
	* .mxses 会话文件写线程（仿 mOMxCaeFile）。
	* 会话数据（含各模型 mModelData 序列化chunk）由界面线程采集完成后传入，
	* 本线程只负责压缩与原子落盘，不触碰 GUI/模型对象。
	*/
	class MIOFILE_EXPORT mOMxsesFile : public QThread
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		* name：目标文件路径
		* data：界面线程采集完成的会话数据（线程接管所有权）
		* compressLevel：压缩级别（qCompress），9为最高压缩
		*/
		mOMxsesFile(const QString& name, const MxsesFileData& data, int compressLevel = 9);

		/*
		* 析构函数
		*/
		~mOMxsesFile();

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
		void writeMxsesErrorSig(MechCommon::IOFileErrorType type = MechCommon::UnknownError);

	private:
		//文件名
		QString _fileName;

		//会话数据
		MxsesFileData _data;

		//压缩级别（0不压缩~9最高压缩）
		int _compressLevel = 9;

		//取消标志（退出程序时由界面线程置位，run()在检查点响应）
		std::atomic_bool _cancelRequested{ false };

		QMutex mutex;
	};
}
