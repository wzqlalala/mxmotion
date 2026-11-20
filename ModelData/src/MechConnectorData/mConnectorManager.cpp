#include "mConnectorManager.h"
#include "mConnectorBase.h"
#include <vector>

namespace MechConnectorData
{
	mConnectorManager::mConnectorManager()
	{
		_maxID = 0;
	}

	mConnectorManager::~mConnectorManager()
	{
		_connectorData.clear();
	}

	std::shared_ptr<mConnectorBase> mConnectorManager::getConnectorData(const int& id)
	{
		auto it = _connectorData.find(id);
		if (it != _connectorData.end())
		{
			return _connectorData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mConnectorBase> mConnectorManager::getConnectorData(const QString& name)
	{
		for (auto it : _connectorData)
		{
			if (it.second->getConnectorName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mConnectorManager::getConnectorNameByID(const int& id)
	{
		if (_connectorData.find(id) != _connectorData.cend())
			return _connectorData[id]->getConnectorName();

		return QString();
	}

	int mConnectorManager::getIDByConnectorName(const QString& name)
	{
		for (auto& it : _connectorData)
		{
			if (it.second && it.second->getConnectorName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mConnectorManager::AddConnectorData(std::shared_ptr<mConnectorBase>mat)
	{
		_connectorData[mat->getConnectorId()] = std::move(mat);
	}

	bool mConnectorManager::ExistConnectorName(const QString& name)
	{
		for (auto it : _connectorData)
		{
			if (it.second != nullptr && it.second->getConnectorName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mConnectorManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _connectorData)
		{
			if (it.second != nullptr && it.second->getConnectorName() == oldName)
			{
				it.second->setConnectorName(newName);
				break;
			}
		}
	}

	int mConnectorManager::copyConnectorData(const int& id)
	{
		std::shared_ptr<mConnectorBase> mat = getConnectorData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getConnectorName() + "-copy";
		while (ExistConnectorName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mConnectorBase>copyMat = mat->copyConnector(createID(), copyName);
		AddConnectorData(copyMat);
		return copyMat->getConnectorId();
	}

	void mConnectorManager::deleteAllConnector()
	{
		_connectorData.clear();
	}

	void mConnectorManager::deleteConnector(const int& id)
	{
		if (_connectorData.find(id) != _connectorData.cend())
		{
			_connectorData.erase(id);
		}
	}

	std::vector<int> mConnectorManager::getAllNodesInConn(int id)
	{
		if (_connectorData.find(id) != _connectorData.cend())
		{
			return _connectorData[id]->getAllNodesInConnector();
		}
		return std::vector<int>();
	}

	std::vector<int> mConnectorManager::getAllNodesInConn()
	{
		std::vector<int>res;
		for (auto it = _connectorData.begin(); it != _connectorData.end(); it++)
		{
			auto nodes = it->second->getAllNodesInConnector();
			for (int i = 0; i < nodes.size(); ++i)
			{
				res.push_back(nodes[i]);
			}
		}
		return res;
	}

	std::vector<int> mConnectorManager::getAllConnectorID()
	{
		std::vector<int> keys;
		for (auto pair : _connectorData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mConnectorManager::getAllConnectorNames()
	{
		QStringList names;
		for (auto pair : _connectorData)
		{
			names.push_back(pair.second->getConnectorName());
		}
		return names;
	}

	QSet<QString> mConnectorManager::getAllConnectorNamesDup()
	{
		QSet<QString> names;
		for (auto pair : _connectorData)
		{
			names.insert(pair.second->getConnectorName());
		}
		return names;
	}

	QStringList mConnectorManager::getAllConnectorNamesByTypes(MxFunctions::ConnectorType type)
	{
		QStringList names;
		for (auto it : _connectorData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getConnectorType() == type)
			{
				names.append(it.second->getConnectorName());
			}
		}
		return names;
	}

	QString mConnectorManager::getConnectorTypeNameByType(MxFunctions::ConnectorType type)
	{
		switch (type)
		{
		case MxFunctions::Connector_Bind:
			return "绑定";
		case MxFunctions::Connector_Rigid:
			return "刚性连接";
		case MxFunctions::Connector_Flexible:
			return "柔性连接";
		case MxFunctions::Connector_MPC:
			return "MPC约束";
		case MxFunctions::Connector_Disperse:
			return "离散刚体";
		case MxFunctions::Connector_Bolt:
			return "螺栓约束";
		case MxFunctions::Connector_DistributedCoupling:
			return "分布式耦合连接";
		case MxFunctions::Connector_BearingUnits:
			return "轴承单元";
		case MxFunctions::Connector_Rigidbody:
			return "解析刚体";
		default:
			return QString();
		}
	}

	int mConnectorManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}