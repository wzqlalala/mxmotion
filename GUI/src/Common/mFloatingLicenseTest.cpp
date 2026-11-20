#include "mFloatingLicenseTest.h"
#include "ui_mFloatingLicenseTest.h"
#include <QDebug>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QHostAddress>
#include <QFile>
#include <QHostInfo>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkInterface>
namespace Common
{
	mFloatingLicenseTest::mFloatingLicenseTest(QWidget *parent)
		: MechMainWindow::mFramelessDialog(parent)
	{

		ui = new Ui::mFloatingLicenseTest();
		//.ui 顶层布局装进内容容器再交给基类（基类 this 的布局已被标题栏占用）
		QWidget* content = new QWidget(this);
		ui->setupUi(content);
		setContentWidget(content);
		//on_ 自动连接经 connectSlotsByName(content) 已失效（槽在本类不在容器上），
		//这里显式补连
		connect(ui->pushButton_save, &QPushButton::clicked, this, &mFloatingLicenseTest::on_pushButton_save_clicked);
		setWindowTitle("浮动管理IP/端口连接----本机IP:" + getLocalIP());
		QString exepath = QCoreApplication::applicationDirPath();
		qDebug() << exepath;
		QString FloatDir = exepath + "/../FloatLicense";
		QDir folder(FloatDir);
		if (!folder.exists())
		{
			if (folder.mkdir(FloatDir)) {}
		}
		floatlicenseFile = exepath + "/../FloatLicense/floateServer.txt";
		getIPandHostName();
		connect(ui->lineEdit_IP, &QLineEdit::textChanged, this, [=](const QString &text) {
			isTest = false;
		});
		connect(ui->lineEdit_PORT, &QLineEdit::textChanged, this, [=](const QString &text) {
			isTest = false;
		});
	}

	mFloatingLicenseTest::~mFloatingLicenseTest()
	{
		if (m_tcp != nullptr)
			delete m_tcp;
		delete ui;
	}

	void mFloatingLicenseTest::getIPandHostName()
	{
		_hostName = QHostInfo::localHostName();
		QHostInfo info = QHostInfo::fromName(_hostName);
		QList<QHostAddress> list = info.addresses();
		for (int i = 0; i < list.size(); i++)
		{
			QHostAddress aHost = list.at(i);
			if (QAbstractSocket::IPv4Protocol == aHost.protocol())
			{
				_IPaddr = aHost.toString();
				break;
			}
		}
		_IPaddr = getLoclIP1();
		if (_hostName.isEmpty())
			_hostName = "default";
		if (_IPaddr.isEmpty())
			_IPaddr = "0.0.0.0";
	}

	QString mFloatingLicenseTest::nowTime()
	{
		QDateTime currentDateTime = QDateTime::currentDateTime();
		QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
		return currentDateTimeString;
	}

	void mFloatingLicenseTest::on_pushButton_save_clicked()
	{
		if (ui->lineEdit_IP->text().isEmpty() || ui->lineEdit_PORT->text().isEmpty())
		{
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "未输入服务器的IP或端口号！", QMessageBox::Ok);
			return;
		}
		if (isTest == true)
		{
			QFile file(floatlicenseFile);
			if (file.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				QTextStream out(&file);
				out << ui->lineEdit_IP->text() << endl;
				out << ui->lineEdit_PORT->text() << endl;
				file.close();
				MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "保存成功,重启即可！", QMessageBox::Ok);
				exit(0);
			}
			else
				MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "无法写入文件，请重试！", QMessageBox::Ok);
		}
		else
		{
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "请先测试并测试成功进行保存！", QMessageBox::Ok);
			return;
		}
	}

	void mFloatingLicenseTest::on_pushButton_test_clicked()
	{
		if (ui->lineEdit_IP->text().isEmpty() || ui->lineEdit_PORT->text().isEmpty())
		{
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "请输入IP和端口号！", QMessageBox::Ok);
			return;
		}
		m_tcp = new QTcpSocket;
		connect(m_tcp, &QTcpSocket::readyRead, this, &mFloatingLicenseTest::readData);
		m_tcp->connectToHost(QHostAddress(ui->lineEdit_IP->text()), ui->lineEdit_PORT->text().toInt());
		_useName = getCurrentUsername();
		if (m_tcp->waitForConnected(1000))
		{
			_loginTime = nowTime();
			QString connectedInfo = "TESTTTT#" + _hostName + "#" + _IPaddr + "#" + _loginTime + "#" + _useName + "#";
			QByteArray byteArray = connectedInfo.toUtf8();
			m_tcp->write(byteArray);
		}
		else
		{
			delete m_tcp;
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "连接失败，请联系管理员！", QMessageBox::Ok);
			return;
		}
	}

	QString mFloatingLicenseTest::getCurrentUsername()
	{
		QString homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
		QStringList dirNames = homePath.split("/");
		if (dirNames.size() == 0) return QString();
		return dirNames[dirNames.size() - 1];
	}


	void mFloatingLicenseTest::readData()
	{
		QByteArray data = m_tcp->readAll();
		if (data == "Connect")
		{
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "成功连接，保存重启即可！", QMessageBox::Ok);
			isTest = true;
			m_tcp->close();
			return;
		}
		else
			MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "连接失败，请联系管理员！", QMessageBox::Ok);
	}
	QString mFloatingLicenseTest::getLoclIP1()
	{
		QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
		foreach(QNetworkInterface interface, interfaces)
		{
			if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
				!interface.flags().testFlag(QNetworkInterface::IsLoopBack))
			{
				QList<QNetworkAddressEntry> entries = interface.addressEntries();
				foreach(QNetworkAddressEntry entry, entries)
				{
					if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
						return entry.ip().toString();
				}
			}
		}
		return QString();
	}
	QString mFloatingLicenseTest::getLocalIP()
	{
		QString hostName = QHostInfo::localHostName();
		QHostInfo   hostInfo = QHostInfo::fromName(hostName);
		QString   localIP = "";
		QList<QHostAddress> addList = hostInfo.addresses();
		if (!addList.isEmpty())
			for (int i = 0; i < addList.count(); i++)
			{
				QHostAddress aHost = addList.at(i);
				if (QAbstractSocket::IPv4Protocol == aHost.protocol())
				{
					localIP = aHost.toString();
					break;
				}
			}
		return localIP;
	}
}



