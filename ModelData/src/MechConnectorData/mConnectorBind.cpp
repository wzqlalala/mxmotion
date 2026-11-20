#include "mConnectorBind.h"

namespace MechConnectorData
{
	mConnectorBind::mConnectorBind(int id, MxFunctions::ConnectorType type, const QString& name )
		:mConnectorBase(id, type, name)
	{
		type_ = 0;
		allowrancetype_ = 1;
		adjust_ = false;
		allowrance_ = QVariant();
		_pickStateMaster = 0;
		_pickStateSlave = 0;
	}

	mConnectorBind::~mConnectorBind()
	{

	}

	std::shared_ptr<mConnectorBase> mConnectorBind::copyConnector(const int& id, const QString& name)
	{
		std::shared_ptr<mConnectorBind> conn = std::make_shared<mConnectorBind>(id, getConnectorType(), name);
		conn->allowrance_ = allowrance_;
		conn->type_ = type_;
		return conn;
	}

	MxFunctions::BrowserTreeStatus mConnectorBind::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		return info.isEmpty()? MxFunctions::BrowserTreeStatus::Tree_Yes :MxFunctions::BrowserTreeStatus::Tree_Failure;
	}

	void mConnectorBind::setPickStateMaster(int pickstate)
	{
		_pickStateMaster = pickstate;
	}

	int mConnectorBind::getPickStateMaster()
	{
		return _pickStateMaster;
	}

	void mConnectorBind::setPickStateSlave(int pickstate)
	{
		_pickStateSlave = pickstate;
	}

	int mConnectorBind::getPickStateSlave()
	{
		return _pickStateSlave;
	}
}