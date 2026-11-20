#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include <QtSql/QSqlDatabase>

namespace Ui
{
	class mLoginDialog;
}

namespace ApplicationInit
{
	class  mLoginDialog : public MechMainWindow::mFramelessDialog
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mLoginDialog(QWidget *parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mLoginDialog();

	private slots:


		void slot_loginButton();

	private:
		//UI
		Ui::mLoginDialog* _ui;
		QSqlDatabase db;
	};
}