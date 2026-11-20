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
	class MXCONNECTORDATA_EXPORT mConnectorFlexible : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorFlexible(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorFlexible() {}

		/*
		* 析构函数
		*/
		~mConnectorFlexible();

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

		friend QDataStream& operator>>(QDataStream& in, mConnectorFlexible& mgr);
		friend QDataStream& InStreammConnectorFlexibleV3(QDataStream& in, mConnectorFlexible& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorFlexible& mgr);
	public:
		//方式
		int way_;

		//主节点
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes_;

		//从节点
		std::pair<MxFunctions::PickDataType, RenderPickResultData> subnodes_;

		//权重
		std::vector<double>  w_;

		//自由度
		std::vector<std::set<int>>  dof_;

		//权重
		double singleweight_;

		//(主节点)自由度
		std::set<int> degreeoffreedom_main;

		//从节点自由度
		std::set<int> degreeoffreedom_sub;

		// 是否使用表格差异化设置模式
		bool isTableMode_;
	};
}