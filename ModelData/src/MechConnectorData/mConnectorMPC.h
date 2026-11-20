#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXConnectorData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include "mConnectorBase.h"

namespace MechConnectorData
{
	class MXCONNECTORDATA_EXPORT mConnectorMPC : public mConnectorBase
	{
	public:
		/*
		* 构造函数
		*/
		mConnectorMPC(int id, MxFunctions::ConnectorType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConnectorMPC() {}

		/*
		* 析构函数
		*/
		~mConnectorMPC();

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

		friend QDataStream& operator>>(QDataStream& in, mConnectorMPC& mgr);
		friend QDataStream& InStreammConnectorMPCV3(QDataStream& in, mConnectorMPC& obj);
		friend QDataStream& operator<<(QDataStream& out, mConnectorMPC& mgr);
	public:
		//从节点
		std::pair<MxFunctions::PickDataType, RenderPickResultData> subnodes_;
		//从节点自由度(从0开始，为下拉框索引)
		int subDof_;
		//从节点权重
		double subWeights_;

		//主节点
		std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes_;
		//主节点自由度(0 - 5)
		QHash<int, double> defaultweights_;
		//节点自由度及权重  里面size默认大小为6，索引对应主节点的节点类型vector索引
		std::vector<std::vector<QVariant>> mainweights_;

		QVariant coefficient_;
	};
}