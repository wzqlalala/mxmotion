#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"
#include <map>

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorDistributedCoupling : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorDistributedCoupling(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorDistributedCoupling() {}

		/*
		* 析构函数
		*/
		~mConnectorDistributedCoupling();

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

		friend QDataStream& operator>>(QDataStream& in, mConnectorDistributedCoupling& mgr);
		friend QDataStream& InStreammConnectorDistributedCouplingV3(QDataStream& in, mConnectorDistributedCoupling& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorDistributedCoupling& mgr);
	public:
		//方式
		int way_;

		//主节点（耦合点）
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes_;

		//从节点（参考点）
		std::pair<MxFunctions::PickDataType, RenderPickResultData> subnodes_;

		//差异化权重
		std::vector<double> w_;

		//差异化自由度
		std::vector<std::set<int>> dof_;

		//统一权重
		double singleweight_;

		//主节点自由度（U1/U2/U3固定，UR1/UR2/UR3可选）
		std::set<int> degreeoffreedom_main;
	};
}
