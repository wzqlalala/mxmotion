#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorBolt : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorBolt(int id, MxFunctions::ConnectorType type, const QString& name = "");
		mConnectorBolt() {};
		/*
		* 析构函数
		*/
		~mConnectorBolt();

		/*
		* 拷贝
		*/
		std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mConnectorBolt& obj);
		friend QDataStream& InStreammConnectorBoltV3(QDataStream& in, mConnectorBolt& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorBolt& obj);
	public:


		//螺栓类型 0一维螺栓 1三维螺栓
		int type_;

		//预紧方向是否开启
		bool isOpen_;
		//矢量size 分量xyz
		int way_; //0矢量，1分量

		QVariant dir_;

		//预紧方向控件完整UI状态（单例面板反显用）
		DirectionPickState dirPickState_;

		QVariant x_;
		QVariant y_;
		QVariant z_;

		//节点id
		int node_;
	};
}