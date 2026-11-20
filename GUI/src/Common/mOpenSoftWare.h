#pragma once
#include "mCommon_global.h"
#include <QObject>
#include <QTcpSocket>
#include <QThread>

namespace MechMainWindow
{
	class COMMON_EXPORT mOpenMXSoftWare :
		public QObject
	{
		Q_OBJECT
	public:
		explicit mOpenMXSoftWare();

		//获取打开信息
		int getOpenSig();

		void run();

		//断开服务器
		void disconnectServer();

		//解析ip地址
		static QString getIpv4();

	signals:
		//失败信号
		void submitFail();
	private:
		QTcpSocket* m_tcp;

		int ip = 0;

		bool isright = false;
	};
}

