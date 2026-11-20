#include "mClient.h"
#include "mFramelessMessageBox.h"
#include "ui_mClient.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QCoreApplication>
#include <qmessagebox.h>
#include <QtNetwork>
#include <qtcpsocket.h>
#include <QString>
#include <mutex>

namespace Common
{
	mClient* mClient::_instance = nullptr;
	std::mutex mClient::mutex_mClient;
	mClient::mClient(QWidget *parent)
		: QMainWindow(parent)
	{
		_ui = new Ui::mClient();
		_ui->setupUi(this);
		getIPandHostName();
		socket = new QTcpSocket;
		this->setWindowTitle("客户端");
		_sendTimer = new QTimer(socket);
		_isConnectTimer = new QTimer(socket);
		_sendTimer->setInterval(10000);
		_isConnectTimer->setInterval(30000);
		connect(socket, &QTcpSocket::disconnected, this, &mClient::disconnectReact);
		setWindowTitle("浮动许可");
		setWindowIcon(QIcon(":/mApplicationStart/Icon/logo.png"));
		connect(socket, &QTcpSocket::connected, this, &mClient::onConnected);
		connect(socket, &QTcpSocket::disconnected, this, &mClient::onDisconnected);
	}


	void mClient::start() {
		if (!_isUseFloatingLicense) return;
		QString exeFilePath = QCoreApplication::applicationDirPath();
		qDebug() << exeFilePath;
		QString floatpath = exeFilePath + "/../FloatLicense";
		QDir dir(floatpath);
		if (dir.exists()) {
			qDebug() << "文件夹已存在";
		}
		else
		{
			if (dir.mkpath(floatpath)) { qDebug() << "文件夹创建成功"; }
			else { qDebug() << "文件夹创建失败"; }
		}
		QString floatpath01 = floatpath + "/floateServer.txt";
		qDebug() << floatpath01;
		QFile file(floatpath01);
		if (!file.exists()) {
			file.open(QIODevice::ReadWrite);
			file.close();
		}
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			if (file.size() == 0 || file.atEnd())
			{
				mFloatingLicenseTest floatTest;
				floatTest.exec();
			}
			else
			{
				QTextStream in(&file);
				QString ip = in.readLine();
				QString port = in.readLine();
				startConnectServer(ip, port);
			}
		}
	}
	mClient::~mClient()
	{
		int testDeconstruct = 0;
		delete _ui;
	}

	void mClient::startConnectServer(QString ip, QString port)
	{
		//防止重连
		if (socket->state() == QTcpSocket::ConnectingState || socket->state() == QTcpSocket::ConnectedState)
			return;
		socket->connectToHost(QHostAddress(ip), port.toInt());
		connect(socket, &QTcpSocket::readyRead, this, &mClient::readData);
		_loginTime = nowTime();
		QString connectedInfo = "LOGGINN#" + _hostName + "#" + _IPaddr + "#" + _loginTime + "#" + _moduleName + "#" + _userName + "#";
		QByteArray byteArray = connectedInfo.toUtf8();
		socket->write(byteArray);
		emit setStatusBarMessage_sig("正在连接服务器...");
		if (socket->waitForConnected(1000))
		{
			qDebug() << "连接服务器成功！";
			emit setStatusBarMessage_sig("连接服务器成功!");
			connect(_sendTimer, SIGNAL(timeout()), this, SLOT(writeHeartBeat()));
			connect(_isConnectTimer, SIGNAL(timeout()), this, SLOT(disconnectFromServer()));
			_sendTimer->start();
			_isConnectTimer->start();
		}
		else
		{
			qDebug() << "连接服务器失败！";
			emit setStatusBarMessage_sig("连接服务器失败!");
			MechMainWindow::mFramelessMessageBox::warning(new QWidget(), QString("警告"), QString("未连接浮动许可服务器，请联系管理员！"), QString("确定"));
			exit(0);
		}
	}

	void mClient::getIPandHostName()
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

		_userName = getCurrentUsername();
		_IPaddr = getLocalIP();
		if (_hostName.isEmpty())
			_hostName = "default";
		if (_IPaddr.isEmpty())
			_IPaddr = "0.0.0.0";
	}

	QString mClient::nowTime()
	{
		QDateTime currentDateTime = QDateTime::currentDateTime();
		QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
		return currentDateTimeString;
	}

	QString mClient::getLocalIP()
	{
		QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
		foreach(const QNetworkInterface &interface, interfaces)
		{
			QList<QNetworkAddressEntry> addressEntries = interface.addressEntries();
			foreach(const QNetworkAddressEntry &addressEntry, addressEntries)
			{
				QHostAddress ip = addressEntry.ip();
				if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip.toString().startsWith("192.168."))
					return ip.toString();  // 返回找到的第一个符合条件的IP地址
			}
		}
		return QString();  // 如果没有找到符合条件的IP地址，返回空字符串
	}



	void mClient::readData()
	{
		QByteArray data = socket->readAll();
		if (data == "HeatBe")
			_isConnectTimer->start();
		else
		{
			if (data != "MAX")
			{
			}
			if (data == "MAX")
			{
				MechMainWindow::mFramelessMessageBox::warning(new QWidget(), QString("警告"), QString("局域网已达最大数量，请先关闭其他客户端！"), QString("确定"));
				exit(0);
			}
		}
	}

	void mClient::disconnectReact()
	{
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), QString("警告"), QString("服务器已停止服务，请联系管理员！"), QString("确定"));
		exit(0);
	}

	void mClient::setStatusBarMessage_slot(QString message)
	{
		_ui->statusBar->showMessage(message);
	}

	void mClient::onConnected()
	{
		if (socket->state() == QTcpSocket::ConnectingState || socket->state() == QTcpSocket::ConnectedState) {
			return;
		}

		socket->connectToHost(QHostAddress(IP), PORT);
		_loginTime = nowTime();
		QString connectedInfo = "LOGGINN#" + _hostName + "#" + _IPaddr + "#" + _loginTime + "#";
		QByteArray byteArray = connectedInfo.toUtf8();
		socket->write(byteArray);
	}

	void mClient::onDisconnected()
	{
		_logoutTime = nowTime();
		QString disconnectedInfo = "LOGOUTT#" + _hostName + "#" + _IPaddr + "#" + _logoutTime + "#";
		QByteArray byteArray = disconnectedInfo.toUtf8();
		socket->write(byteArray);
		socket->deleteLater();
	}

	void mClient::writeHeartBeat()
	{
		QByteArray data = "HeartBe";
		socket->write(data);
		_isConnectTimer->start();
	}

	void mClient::disconnectFromServer()
	{
		MechMainWindow::mFramelessMessageBox::warning(new QWidget(), QString("警告"), QString("检测到你与服务器断开连接，请联系管理员！"), QString("确定"));
		exit(0);
	}
	QString Common::mClient::getCurrentUsername()
	{
		QString homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
		QStringList dirNames = homePath.split("/");
		if (dirNames.size() == 0) return QString();
		return dirNames[dirNames.size() - 1];
	}

	void Common::mClient::changeToMaterial()
	{
		sendInfotoServer(QString("DynaMaterial"));
	}

	void Common::mClient::changeToProperty()
	{
		sendInfotoServer(QString("DynaProperty"));
	}

	void Common::mClient::changeToElement()
	{
		sendInfotoServer(QString("DynaElement"));
	}

	void Common::mClient::changeToAnalyse()
	{
		sendInfotoServer(QString("DynaAnalyse"));
	}

	void Common::mClient::changeToLoad()
	{
		sendInfotoServer(QString("DynaLoad"));
	}

	void Common::mClient::changeToBoundary()
	{
		sendInfotoServer(QString("DynaBoundary"));
	}

	void Common::mClient::changeToCondition()
	{
		sendInfotoServer(QString("DynaCondition"));
	}

	void Common::mClient::changeToConnect()
	{
		sendInfotoServer(QString("DynaConnect"));
	}

	void Common::mClient::changeToContact()
	{
		sendInfotoServer(QString("DynaContact"));
	}

	void Common::mClient::changeToRigid()
	{
		sendInfotoServer(QString("DynaRigid"));
	}

	void Common::mClient::changeToAirbag()
	{
		sendInfotoServer(QString("DynaAirbag"));
	}

	void Common::mClient::changeToSensor()
	{
		sendInfotoServer(QString("DynaSensor"));
	}

	void Common::mClient::changeToOutput()
	{
		sendInfotoServer(QString("DynaOutput"));
	}

	void Common::mClient::changeToJob()
	{
		sendInfotoServer(QString("DynaJob"));
	}

	void Common::mClient::changeToSet()
	{
		sendInfotoServer(QString("DynaSet"));
	}

	void Common::mClient::changeToXYPlot()
	{
		sendInfotoServer(QString("DynaXYPlot"));
	}

	void Common::mClient::changeToCoordinate()
	{
		sendInfotoServer(QString("DynaCoordinate"));
	}

	void Common::mClient::changeToFace()
	{
		sendInfotoServer(QString("DynaFace"));
	}

	void Common::mClient::changeToRestart()
	{
		sendInfotoServer(QString("DynaRestart"));
	}

	void Common::mClient::DynaSubmit()
	{
		sendInfotoServer(QString("DynaSubmit"));
	}

	void Common::mClient::changeToGeometry()
	{
		sendInfotoServer(QString("DynaGeometry"));
	}

	void Common::mClient::changeToMesh()
	{
		sendInfotoServer(QString("DynaMesh"));
	}

	void Common::mClient::changeToTool()
	{
		sendInfotoServer(QString("DynaTool"));
	}

	void Common::mClient::submitAndSlover(const QString & jobName)
	{
		//if (!_isUseFloatingLicense) return;
		//if (_currentModule == moudle) return;
		//_currentModule = moudle;
		//QString String = QString("mMoudle#%1#").arg(moudle);
		//QByteArray byteArray = String.toUtf8();
		//socket->write(byteArray);
		if (!_isUseFloatingLicense) return;
		//if (_currentModule == moudle) return;
		//_currentModule = moudle;
		QString String = QString("mSubmit#%1#").arg(jobName);
		QByteArray byteArray = String.toUtf8();
		socket->write(byteArray);

	}

	void Common::mClient::sendInfotoServer(const QString & moudle)
	{
		if (!_isUseFloatingLicense) return;
		if (_currentModule == moudle) return;
		_currentModule = moudle;
		QString String = QString("mMoudle#%1#").arg(moudle);
		QByteArray byteArray = String.toUtf8();
		socket->write(byteArray);
	}

	mClient * Common::mClient::getInstance()
	{
		std::lock_guard<std::mutex> lock(mutex_mClient);
		if (_instance == nullptr)
		{
			_instance = new mClient;
		}
		return _instance;
	}

	void Common::mClient::setIsUseFloatLicnese(bool flag) {
		_isUseFloatingLicense = flag;
	}


}

