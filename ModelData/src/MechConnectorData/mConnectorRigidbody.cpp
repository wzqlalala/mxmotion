#include "mConnectorRigidbody.h"

namespace MechConnectorData
{
	mConnectorRigidbody::mConnectorRigidbody(int id, MxFunctions::ConnectorType type, const QString& name)
		:mConnectorBase(id, type, name)
	{

	}

	mConnectorRigidbody::~mConnectorRigidbody()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorRigidbody::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorRigidbody> conn = std::make_shared<mConnectorRigidbody>(id, getConnectorType(), name);
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorRigidbody::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
