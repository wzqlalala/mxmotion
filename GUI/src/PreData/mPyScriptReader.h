#pragma once
#include "MPreData_global.h"

//QT
#include <QObject>

namespace MechPreData
{
	class MPREDATA_EXPORT mPyScriptReader : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mPyScriptReader(const QString& filename);

		/*
		* 析构函数
		*/
		~mPyScriptReader();

		/*
		* 开始运行脚本
		*/
		void startRun();

	private:
		//读取的文件名
		QString _fileName;
	};

}