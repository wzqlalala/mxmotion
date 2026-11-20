#include "mConnectorBolt.h"

namespace MechConnectorData
{
	mConnectorBolt::mConnectorBolt(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{
		isOpen_ = false;
		type_ = 0;
		way_ = 0;
		dir_ = QVariant();
		x_ = QVariant();
		y_ = QVariant();
		z_ = QVariant();
	}

	mConnectorBolt::~mConnectorBolt()
	{
	}

	std::shared_ptr<mConnectorBase> mConnectorBolt::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorBolt> conn = std::make_shared<mConnectorBolt>(id, getConnectorType(), name);
		conn->type_ = type_;
		conn->way_ = way_;
		conn->dir_ = dir_;
		conn->x_ = x_;
		conn->y_ = y_;
		conn->z_ = z_;
		conn->isOpen_ = isOpen_;
		conn->dirPickState_ = dirPickState_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorBolt::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		return MxFunctions::BrowserTreeStatus::Tree_Yes;
	}
}