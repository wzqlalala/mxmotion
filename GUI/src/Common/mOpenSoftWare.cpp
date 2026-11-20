#pragma execution_character_set("utf-8")
#include "mOpenSoftWare.h"
#include <QHostAddress>
#include "mCloudeEnum.h"
#include <QApplication>
#include <QFile>
#include <QRegExp>

namespace MechMainWindow
{

	mOpenMXSoftWare::mOpenMXSoftWare()
	{
		m_tcp = new QTcpSocket(this);
		connect(m_tcp, &QTcpSocket::readyRead, this, [=]() {
			QByteArray data = m_tcp->readAll();
			if (data.at(0) != 0x0c && data.at(data.size() - 1) != 0x0c)
			{
				//qDebug() << "文件头包错误";
				disconnectServer();
				emit submitFail();
				return;
			}
			if (data.at(1) == IPUsable && data.mid(2, 16) == "&&OpenSoftWare&&")
			{
				ip = 1;
			}
			else
			{
				ip = 2;
			}
			disconnectServer();

		});
	}

	void mOpenMXSoftWare::run()
	{
		QString Ipv4 = mOpenMXSoftWare::getIpv4();
		if (Ipv4.isEmpty())
		{
			return;
		}

		m_tcp->connectToHost(QHostAddress(Ipv4), 8008);

		if (m_tcp->waitForConnected(1000)) {
			qDebug() << "yese";
		}
		else
		{
			ip = 2;
			return;
		}
		QThread::usleep(10);
		char buf[255] = { 0 };
		int i = 0;
		//包头
		buf[i++] = 0x0c;
		//命令号
		buf[i++] = OpenSoftWare;
		std::string str = "&&OpenSoftWare&&";
		for (auto it : str)
		{
			buf[i++] = it;
		}
		buf[i++] = 0x0c;

		QByteArray byteArray(buf, strlen(str.c_str()) + 3);

		m_tcp->write(byteArray);

		if (m_tcp->waitForReadyRead() == false)
		{
			ip = 2;
			return;
		}


	}



	int mOpenMXSoftWare::getOpenSig()
	{
		return ip;
	}

	void mOpenMXSoftWare::disconnectServer()
	{
		//断开连接
		m_tcp->disconnectFromHost();
	}
	QString mOpenMXSoftWare::getIpv4()
	{
		QString filePath = QApplication::applicationDirPath() + QString("/Network.log");
		QFile filepath;
		filepath.setFileName(filePath);
		if (!filepath.open(QIODevice::ReadOnly))
		{
			return QString("");
		}
		//第一行注释舍去
		QByteArray fileData = filepath.readLine();
		//第二行服务器ip
		fileData = filepath.readLine();
		QString Ipv4(fileData);
		Ipv4 = Ipv4.remove(QRegExp("\\s"));
		int bpos = Ipv4.indexOf("\"") + 1;
		int epos = Ipv4.lastIndexOf("\"");
		Ipv4 = Ipv4.mid(bpos, epos - bpos);
		return Ipv4;
	}
}
