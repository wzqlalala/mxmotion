#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MPreData_global.h"

//QT
#include <QObject>
#include <QHash>
#include <QFile>
#include <QTextStream>

namespace MechPreData
{
	class mPythonInterpreter;

	class MPREDATA_EXPORT mPythonAgent :public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mPythonAgent* getInstance();

		/*
		* 初始化
		*/
		bool initPythonAgent();

		/*
		* 提交Python命令
		*/
		void submitCommand(const QString& cmd);

		/*
		* 提交Python命令
		*/
		void submitCommand(const QStringList& cmds);

		/*
		* 仅将命令写入日志文件并立即落盘（不执行、不入历史、不回显）。
		* 用于"先落盘再执行API"的防闪退场景：崩溃时肇事命令已在文件中
		*/
		void logCommandToFile(const QString& cmd);

		/*
		* 仅入命令历史（不写文件、不执行、不回显）
		*/
		void addCommandToHistory(const QString& cmd);

		/*
		* 脚本文件批量执行模式：执行脚本文件期间为 true（界面批量创建不改变模型树选中节点）
		*/
		void setScriptFileRunning(bool running);
		bool isScriptFileRunning() const;

		/*
		* 初始化语言
		*/
		void initLanguage();

	private:
		/*
		* 打开命令日志文件（软件运行目录下 py_command_record.py，覆盖模式，用于闪退后复现）
		*/
		void openCommandLogFile();

		/*
		* 追加一条命令到日志文件并立即落盘
		*/
		void appendCommandLog(const QString& cmd);

	private:
		/*
		* 默认构造函数
		*/
		mPythonAgent();

		/*
		* 析构函数
		*/
		~mPythonAgent();

	private:
		//单例指针
		static mPythonAgent* _instance;

		//解释器
		mPythonInterpreter* _pyInterpreter;

		//脚本文件批量执行模式标志
		bool _scriptFileRunning = false;

		//解释器初始化期间不回显 import 等内部命令
		bool _initializing = false;

		friend class mPythonInterpreter;

		//命令日志文件（闪退后可通过该文件复现操作）
		QFile _cmdLogFile;
		QTextStream _cmdLogStream;

		QHash<QString, QString> _English;
		QHash<QString, QString> _Chinese;
	};
}