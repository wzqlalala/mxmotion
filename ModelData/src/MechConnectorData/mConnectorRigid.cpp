#include "mConnectorRigid.h"

namespace MechConnectorData
{
	mConnectorRigid::mConnectorRigid(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{
		way_ = 0;
		tolerance_ = QVariant();
		degreeoffreedom_main.insert(1);
		degreeoffreedom_main.insert(2);
		degreeoffreedom_main.insert(3);
		degreeoffreedom_main.insert(4);
		degreeoffreedom_main.insert(5);
		degreeoffreedom_main.insert(6);
		mainnodes_.first = MxFunctions::PickDataType::None;
		mainnodes_.second = -1;
	}

	mConnectorRigid::~mConnectorRigid()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorRigid::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorRigid> conn = std::make_shared<mConnectorRigid>(id, getConnectorType(), name);
		conn->way_ = way_;
		conn->mainnodes_ = mainnodes_;
		conn->subnodes_ = subnodes_;
		conn->degreeoffreedom_main = degreeoffreedom_main;
		//conn->mainnodes_id = mainnodes_id;
		//conn->subnodes_id = subnodes_id;
		//conn->main2subs_ = main2subs_;
		conn->tolerance_ = tolerance_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorRigid::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (mainnodes_.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"主节点未选择" });
		}
		bool subnodesEmpty = true;
		for (auto it = subnodes_.begin(); it != subnodes_.end(); ++it)
		{
			if (!it->second.empty())
			{
				subnodesEmpty = false;
				break;
			}
		}
		if (subnodesEmpty)
		{
			info.append({ tip,"从节点未选择" });
		}
		if (way_ == 3)
		{
			if(tolerance_.isNull()||!tolerance_.isValid())
				info.append({ tip,"容差值未输入" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}

	std::vector<int> mConnectorRigid::getAllNodesInConnector()
	{
		std::vector<int>res;
		if (mainnodes_.first == MxFunctions::PickDataType::Node)
		{
			res.push_back(mainnodes_.second.result_id);
		}
		if (subnodes_.find(MxFunctions::PickDataType::Node) != subnodes_.end())
		{
			std::vector<RenderPickResultData> subnodes = subnodes_[MxFunctions::PickDataType::Node];
			for (int i = 0; i < subnodes.size(); ++i)
			{
				res.push_back(subnodes[i].result_id);
			}
		}
		return res;
	}
}