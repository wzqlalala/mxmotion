#include "mConnectorMPC.h"

namespace MechConnectorData
{
	mConnectorMPC::mConnectorMPC(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{
		for (int i = 0; i < 6; ++i)
		{
			defaultweights_[i] = 1;
		}
		subnodes_.first = MxFunctions::PickDataType::None;
		subnodes_.second = -1;
		subDof_ = 0;
		subWeights_ = 1;
		coefficient_ = QVariant();
	}

	mConnectorMPC::~mConnectorMPC()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorMPC::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorMPC> conn = std::make_shared<mConnectorMPC>(id, getConnectorType(), name);
		conn->mainnodes_ = mainnodes_;
		conn->subnodes_ = subnodes_;
		conn->mainweights_ = mainweights_;
		conn->defaultweights_ = defaultweights_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorMPC::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if (subnodes_.first == MxFunctions::PickDataType::None)
		{
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
			info.append({ tip,"从节点未选择" });
		}
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
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
			info.append({ tip,"主节点未选择" });
		}

		return status;
	}

	std::vector<int> mConnectorMPC::getAllNodesInConnector()
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