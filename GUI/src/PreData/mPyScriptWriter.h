#pragma once
#include "MPreData_global.h"

//QT
#include <QThread>

namespace MechPreData
{
	class MPREDATA_EXPORT mPyScriptWriter : public QThread
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mPyScriptWriter(const QString& filename);

		/*
		* 析构函数
		*/
		~mPyScriptWriter();

	private:
		/*
		* 重载run函数
		*/
		void run() override;

	private:
		//文件名
		QString _fileName;
	};
}