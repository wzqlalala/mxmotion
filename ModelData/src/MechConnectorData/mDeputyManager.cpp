#include "mDeputyManager.h"
#include "mDeputyBase.h"
#include <vector>

namespace MechConnectorData
{
	mDeputyManager::mDeputyManager()
	{
		_maxID = 0;
	}

	mDeputyManager::~mDeputyManager()
	{
		_deputyData.clear();
	}

	std::shared_ptr<mDeputyBase> mDeputyManager::getDeputyData(const int& id)
	{
		auto it = _deputyData.find(id);
		if (it != _deputyData.end())
		{
			return _deputyData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mDeputyBase> mDeputyManager::getDeputyData(const QString& name)
	{
		for (auto it : _deputyData)
		{
			if (it.second->getDeputyName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mDeputyManager::getDeputyNameByID(const int& id)
	{
		if (_deputyData.find(id) != _deputyData.cend())
			return _deputyData[id]->getDeputyName();

		return QString();
	}

	int mDeputyManager::getIDByDeputyName(const QString& name)
	{
		for (auto& it : _deputyData)
		{
			if (it.second && it.second->getDeputyName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mDeputyManager::AddDeputyData(std::shared_ptr<mDeputyBase> mat)
	{
		_deputyData[mat->getDeputyId()] = std::move(mat);
	}

	bool mDeputyManager::ExistDeputyName(const QString& name)
	{
		for (auto it : _deputyData)
		{
			if (it.second != nullptr && it.second->getDeputyName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mDeputyManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _deputyData)
		{
			if (it.second != nullptr && it.second->getDeputyName() == oldName)
			{
				it.second->setDeputyName(newName);
				break;
			}
		}
	}

	int mDeputyManager::copyDeputyData(const int& id)
	{
		std::shared_ptr<mDeputyBase> mat = getDeputyData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getDeputyName() + "-copy";
		while (ExistDeputyName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mDeputyBase> copyMat = mat->copyDeputy(createID(), copyName);
		AddDeputyData(copyMat);
		return copyMat->getDeputyId();
	}

	void mDeputyManager::deleteAllDeputy()
	{
		_deputyData.clear();
	}

	void mDeputyManager::deleteDeputy(const int& id)
	{
		if (_deputyData.find(id) != _deputyData.cend())
		{
			_deputyData.erase(id);
		}
	}

	std::vector<int> mDeputyManager::getAllNodesInDeputy(int id)
	{
		if (_deputyData.find(id) != _deputyData.cend())
		{
			return _deputyData[id]->getAllNodesInDeputy();
		}
		return std::vector<int>();
	}

	std::vector<int> mDeputyManager::getAllNodesInDeputy()
	{
		std::vector<int> res;
		for (auto it = _deputyData.begin(); it != _deputyData.end(); it++)
		{
			auto nodes = it->second->getAllNodesInDeputy();
			for (int i = 0; i < nodes.size(); ++i)
			{
				res.push_back(nodes[i]);
			}
		}
		return res;
	}

	std::vector<int> mDeputyManager::getAllDeputyID()
	{
		std::vector<int> keys;
		for (auto pair : _deputyData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mDeputyManager::getAllDeputyNames()
	{
		QStringList names;
		for (auto pair : _deputyData)
		{
			names.push_back(pair.second->getDeputyName());
		}
		return names;
	}

	QSet<QString> mDeputyManager::getAllDeputyNamesDup()
	{
		QSet<QString> names;
		for (auto pair : _deputyData)
		{
			names.insert(pair.second->getDeputyName());
		}
		return names;
	}

	QStringList mDeputyManager::getAllDeputyNamesByTypes(MxFunctions::DeputyType type)
	{
		QStringList names;
		for (auto it : _deputyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getDeputyType() == type)
			{
				names.append(it.second->getDeputyName());
			}
		}
		return names;
	}

	QString mDeputyManager::getDeputyTypeNameByType(MxFunctions::DeputyType type)
	{
		switch (type)
		{
		case MxFunctions::Deputy_Connector:
			return "连接器";
		case MxFunctions::Deputy_Fastener:
			return "焊接点";
		default:
			return QString();
		}
	}

	int mDeputyManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}
