#pragma once
#pragma execution_character_set("utf-8")

//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include <QMouseEvent>
#include <QProcess>

namespace Ui
{
	class mTransFileInfoDialog;
}

namespace MechMainWindow
{
	class mTransFileInfoDialog : public mFramelessDialog
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mTransFileInfoDialog(QWidget *parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mTransFileInfoDialog();

		/*
		* 转换文件
		*/
		void transMxFile(const QString& fileName, bool isMeshFile = false);

	protected:
		/*
		* 鼠标中键
		*/
		void mousePressEvent(QMouseEvent *event);

		/*
		* 开始读取mx文件
		*/
		void sendReadMxFileSig();

	private slots:
		/*
		* 是
		*/
		void slot_okButton();

		/*
		* 否
		*/
		void slot_noButton();

		/*
		* 求解器进程完成
		*/
		void slot_processFinished(int exitCode, QProcess::ExitStatus exitStatus);

	signals:
		/*
		* 发送信号，读取转换后的mx文件
		*/
		void readMxFileSig(const QString& fname);

		/*
		* 发送信号，读取转换网格文件
		*/
		void readMeshFileSig(const QString& fname);

	private:
		//UI
		Ui::mTransFileInfoDialog* _ui;

		//转换文件
		QString _fname;

		//转化的mx文件
		QString _mxFileName;

		//求解器进程
		QProcess* _solverProcess = nullptr;

		//是否为网格文件
		bool _isMeshFile = false;
	};
}