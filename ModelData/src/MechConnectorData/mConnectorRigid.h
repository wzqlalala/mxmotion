#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorRigid : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorRigid(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorRigid() {}

		/*
		* 析构函数
		*/
		~mConnectorRigid();

		/*
		* 拷贝
		*/
		std::shared_ptr<mConnectorBase> copyConnector(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

		/*
		* 获取连接的所有节点
		*/
		std::vector<int> getAllNodesInConnector()override;

		friend QDataStream& operator>>(QDataStream& in, mConnectorRigid& mgr);
		friend QDataStream& InStreammConnectorRigidV3(QDataStream& in, mConnectorRigid& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorRigid& mgr);
	public:
		//方式
		int way_;

		//主节点
		std::pair<MxFunctions::PickDataType, RenderPickResultData> mainnodes_;

		//从节点
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> subnodes_;

		//(主节点)自由度
		std::set<int> degreeoffreedom_main;

		//容差
		QVariant tolerance_;
	};
}