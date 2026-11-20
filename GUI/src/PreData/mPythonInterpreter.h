#pragma once
#include <QObject>

namespace MechPreData
{
	class mPythonInterpreter : public QObject
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mPythonInterpreter();

		/*
		* 析构函数
		*/
		~mPythonInterpreter();

		/*
		* 初始化
		*/
		bool initPythonInterpreter();

		/*
		* 执行单个命令
		*/
		bool execPythonCommand(QString cmd);
	};
}