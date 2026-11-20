#include "mConnectorBearingUnits.h"

namespace MechConnectorData
{
	mConnectorBearingUnits::mConnectorBearingUnits(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{
		type_ = 0;
	
		rotateFace_ = 0;
		defMode_ = 0;
		node1_.first = MxFunctions::PickDataType::None;
		node1_.second = -1;

		node2_.first = MxFunctions::PickDataType::None;
		node2_.second = -1;
	}
	mConnectorBearingUnits::~mConnectorBearingUnits()
	{
	}
	std::shared_ptr<mConnectorBase> mConnectorBearingUnits::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorBearingUnits> conn = std::make_shared<mConnectorBearingUnits>(id, getConnectorType(), name);
		conn->type_ = type_;
		conn->node1_ = node1_;
		conn->node2_ = node2_;
		conn->rotateFace_ = rotateFace_;
		conn->defMode_ = defMode_;
		conn->bearingPara_ = bearingPara_;
		conn->curveRotate_ = curveRotate_;
		return conn;
	}
	MxFunctions::BrowserTreeStatus mConnectorBearingUnits::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if (node1_.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"节点1未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		if (node2_.first == MxFunctions::PickDataType::None)
		{
			info.append({ tip,"节点2未选择" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		if (bearingPara_.size() == 0 || curveRotate_.size() == 0)
		{
			info.append({ tip,"轴承参数未定义" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}
		return status;
	}
	std::vector<int> mConnectorBearingUnits::getAllNodesInConnector()
	{
		std::vector<int>res;
		if (node1_.first == MxFunctions::PickDataType::Node)
		{
			res.push_back(node1_.second.result_id);
		}
		if (node2_.first == MxFunctions::PickDataType::Node)
		{
			res.push_back(node2_.second.result_id);
		}
		return res;
	}

}
