#pragma once
//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include <QTcpSocket>
#include "mCommon_global.h"
#pragma execution_character_set("utf-8")

namespace Ui { class mFloatingLicenseTest; };
namespace Common
{
	class  COMMON_EXPORT mFloatingLicenseTest : public MechMainWindow::mFramelessDialog
	{
		Q_OBJECT

	public:

		/** 构造函数 */
		mFloatingLicenseTest(QWidget *parent = Q_NULLPTR);

		/** 析构函数 */
		~mFloatingLicenseTest();

		/** 获取信息 */
		void getIPandHostName();

		/** 获取时间 */
		QString nowTime();

		/** 获取用户名 */
		QString getCurrentUsername();

	public slots:

		/** 保存 */
		void on_pushButton_save_clicked();

		/** 测试 */
		void on_pushButton_test_clicked();

		/** 读取数据 */
		void readData();

	protected:
		/** 重写关闭事件 */
		void closeEvent(QCloseEvent* event) override
		{
			exit(0);
		}
	public:
		/** 获取本地IP */
		QString getLoclIP1();

		/** 获取本地IP */
		QString getLocalIP();


	private:
		Ui::mFloatingLicenseTest*		ui;
		QString							floatlicenseFile{};
		QTcpSocket*						m_tcp{};
		bool							isTest{};
		QString							_hostName{};
		QString							_useName{};
		QString							_IPaddr{};
		QString							_loginTime{};
	};

}
