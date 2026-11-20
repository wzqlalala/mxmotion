#include "mConnectorBase.h"

using namespace MxFunctions;
namespace MechConnectorData
{
	mConnectorBase::mConnectorBase(int id, ConnectorType type, const QString& name)
		: _connectorType(type), _connectorName(name), _id(id), _isSuppressed(false)
	{
		QString connectorname = name;
		if (name.isEmpty())
		{
			connectorname = "Connector" + QString("_%1").arg(_id);
		}
		_connectorName = connectorname;
		userRenamed_ = false;
	}

	mConnectorBase::~mConnectorBase()
	{
	}

	void mConnectorBase::setConnectorId(int id)
	{
		_id = id;
	}

	int mConnectorBase::getConnectorId()
	{
		return _id;
	}

	void mConnectorBase::setConnectorName(const QString& name)
	{
		_connectorName = name;
	}

	QString mConnectorBase::getConnectorName()
	{
		return _connectorName;
	}

	void mConnectorBase::setConnectorType(MxFunctions::ConnectorType type)
	{
		_connectorType = type;
	}

	ConnectorType mConnectorBase::getConnectorType()
	{
		return _connectorType;
	}

	void mConnectorBase::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mConnectorBase::isSuppressed()
	{
		return _isSuppressed;
	}

	QString mConnectorBase::getConnectorTypeToStringEn()
	{
		switch (_connectorType)
		{
		case MxFunctions::Connector_Bind:
			return "Tie";
		case MxFunctions::Connector_Rigid:
			return "Rigid";
		case MxFunctions::Connector_Flexible:
			return "Flex";
		case MxFunctions::Connector_MPC:
			return "MPC";
		case MxFunctions::Connector_Disperse:
			return "DiscreteRigid";
		case MxFunctions::Connector_Bolt:
			return "BoltConstraint";
		case MxFunctions::Connector_DistributedCoupling:
			return "DistributedCoupling";
		case MxFunctions::Connector_BearingUnits:
			return "BearingUnits";
		case MxFunctions::Connector_Rigidbody:
			return "Rigidbody";
		default:
			break;
		}

		return QString();
	}
}
