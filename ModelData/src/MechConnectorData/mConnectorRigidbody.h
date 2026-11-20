#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorRigidbody : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorRigidbody(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
        * 仅用于输入输出流
        */
		mConnectorRigidbody() {}
		/*
		* 析构函数
		*/
		~mConnectorRigidbody();

		/*
		* 拷贝
		*/
		std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mConnectorRigidbody& mgr);
		friend QDataStream& InStreammConnectorRigidbodyV3(QDataStream& in, mConnectorRigidbody& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorRigidbody& mgr);
	};
}
