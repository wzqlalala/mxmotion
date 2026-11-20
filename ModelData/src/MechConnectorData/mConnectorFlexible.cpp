#include "mConnectorFlexible.h"

namespace MechConnectorData
{
	mConnectorFlexible::mConnectorFlexible(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{
		way_ = 0;
		singleweight_ = 1;
		subnodes_.first = MxFunctions::PickDataType::None;
		subnodes_.second = -1;
		isTableMode_ = false;
	}

	mConnectorFlexible::~mConnectorFlexible()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorFlexible::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorFlexible> conn = std::make_shared<mConnectorFlexible>(id, getConnectorType(), name);
		conn->way_ = way_;
		conn->mainnodes_ = mainnodes_;
		conn->subnodes_ = subnodes_;
		conn->singleweight_ = singleweight_;
		conn->degreeoffreedom_main = degreeoffreedom_main;
		conn->degreeoffreedom_sub = degreeoffreedom_sub;
		conn->dof_ = dof_;
		conn->w_ = w_;
		conn->isTableMode_ = isTableMode_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorFlexible::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		bool mainnodesEmpty = true;
		for (auto it = mainnodes_.begin(); it != mainnodes_.end(); ++it)
		{
			if (!it->second.empty())
			{
				mainnodesEmpty = false;
				break;
			}
		}
		if (mainnodesEmpty)
		{
			info.append({ tip,"主节点未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		if (subnodes_.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"从节点未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		if (singleweight_ == -1 && w_.empty())
		{
			info.append({ tip,"主节点权重未输入" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		return status;
	}

	std::vector<int> mConnectorFlexible::getAllNodesInConnector()
	{
		std::vector<int>res;
		if (subnodes_.first == MxFunctions::PickDataType::Node)
		{
			res.push_back(subnodes_.second.result_id);
		}
		if (mainnodes_.find(MxFunctions::PickDataType::Node) != mainnodes_.end())
		{
			std::vector<RenderPickResultData> subnodes = mainnodes_[MxFunctions::PickDataType::Node];
			for (int i = 0; i < subnodes.size(); ++i)
			{
				res.push_back(subnodes[i].result_id);
			}
		}
		return res;
	}
}