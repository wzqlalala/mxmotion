#include "mConnectorDisperse.h"

namespace MechConnectorData
{
	mConnectorDisperse::mConnectorDisperse(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{

	}

	mConnectorDisperse::~mConnectorDisperse()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorDisperse::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorDisperse> conn = std::make_shared<mConnectorDisperse>(id, getConnectorType(), name);
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorDisperse::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}