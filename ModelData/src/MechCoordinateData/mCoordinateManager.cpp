#include "mCoordinateManager.h"
#include "mCoordinateBase.h"

#include "mPhysicalFieldEnum.h"

namespace MechCoordinateData
{
	mCoordinateManager::mCoordinateManager()
	{
		_maxID = 0;
		auto globalCoordinate = new mCoordinateBase(-1, MxFunctions::CoordinateType::Cartesian, "全局坐标系");
		_coordinateData[globalCoordinate->getCoordinateId()] = globalCoordinate;
	}

	mCoordinateManager::~mCoordinateManager()
	{
		for (auto it : _coordinateData)
		{
			if (it.second != nullptr)
			{
				delete it.second;
				it.second = nullptr;
			}
		}

		_coordinateData.clear();
	}

	mCoordinateBase * mCoordinateManager::getCoordinateData(int id)
	{
		auto it = _coordinateData.find(id);
		if (it != _coordinateData.end())
		{
			return _coordinateData[id];
		}

		return nullptr;
	}

	mCoordinateBase* mCoordinateManager::getCoordinateData(const QString& name)
	{
		for (auto it : _coordinateData)
		{
			if (it.second != nullptr && it.second->getCoordinateName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mCoordinateManager::getCoordinateNameByID(int id)
	{
		if (_coordinateData.find(id) != _coordinateData.cend())
			return _coordinateData[id]->getCoordinateName();

		return QString();
	}

	int mCoordinateManager::getIDByCoordinateName(QString name)
	{
		for (auto& it : _coordinateData)
		{
			if (it.second && it.second->getCoordinateName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mCoordinateManager::AddCoordinateData(mCoordinateBase * base)
	{
		_coordinateData.insert({ base->getCoordinateId(), base });
	}

	bool mCoordinateManager::ExistCoordinateName(const QString& name)
	{
		for (auto it : _coordinateData)
		{
			if (it.second != nullptr && it.second->getCoordinateName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mCoordinateManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _coordinateData)
		{
			if (it.second != nullptr && it.second->getCoordinateName() == oldName)
			{
				it.second->setCoordinateName(newName);
				break;
			}
		}
	}

	int mCoordinateManager::copyCoordinateData(const int & id)
	{
		mCoordinateBase* mat = getCoordinateData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getCoordinateName() + "-copy";
		while (ExistCoordinateName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mCoordinateBase*copyCoordinate = mat->copyCoordinate(createID(), copyName);
		AddCoordinateData(copyCoordinate);
		return copyCoordinate->getCoordinateId();
	}

	void mCoordinateManager::deleteAllCoordinate()
	{
		for (auto it = _coordinateData.begin(); it != _coordinateData.end(); )
		{
			if (it->first == -1)  // 保留全局坐标系
			{
				++it;
				continue;
			}

			if (it->second != nullptr)
			{
				delete it->second;
				it->second = nullptr;
			}

			it = _coordinateData.erase(it);  // 删除非 -1 项
		}
	}

	void mCoordinateManager::deleteCoordinate(const int& id)
	{
		if (id == -1)//全局坐标系不能删除
		{
			return;
		}
		if (_coordinateData.find(id) != _coordinateData.cend())
		{
			if (_coordinateData[id] != nullptr)
			{
				delete _coordinateData[id];
				_coordinateData[id] = nullptr;
			}
			_coordinateData.erase(id);
		}
	}

	std::vector<int> mCoordinateManager::getAllCoordinateID(bool containGlobalCoord)
	{
		std::vector<int> keys;
		if (containGlobalCoord)
		{
			for (auto pair : _coordinateData)
			{
				keys.push_back(pair.first);
			}
		}
		else
		{
			for (auto pair : _coordinateData)
			{
				if (pair.first != -1)
				{
					keys.push_back(pair.first);
				}
			}
		}
		return keys;
	}

	QStringList mCoordinateManager::getAllCoordinateNames()
	{
		QStringList names;
		for (auto pair : _coordinateData)
		{
			names.push_back(pair.second->getCoordinateName());
		}
		return names;
	}

	std::vector<QString> mCoordinateManager::getAllCoorinateIDStr()
	{
		std::vector<QString> keys;
		for (auto pair : _coordinateData)
		{
			keys.push_back(QString("coordinate_%1").arg(pair.first));
		}
		return keys;
	}

	std::vector<QString> mCoordinateManager::getAllCoorinateIDStrExceptGlobal()
	{
		std::vector<QString> keys;
		for (auto pair : _coordinateData)
		{
			if (pair.first == -1)
			{
				continue;
			}
			keys.push_back(QString("coordinate_%1").arg(pair.first));
		}
		return keys;
	}

	int mCoordinateManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	void mCoordinateManager::setUnifiedNodeCoordinate(const std::vector<int>& nodeIds, int refId, int anaId)
	{
		_nodeCoordinateData.clear();
		for (int nodeId : nodeIds)
		{
			_nodeCoordinateData[nodeId] = std::make_pair(refId, anaId);
		}
	}

	void mCoordinateManager::addNodeCoordinate(int nodeId, int refId, int anaId)
	{
		_nodeCoordinateData[nodeId] = std::make_pair(refId, anaId);
	}

	std::map<int, std::pair<int, int>> mCoordinateManager::getNodeCoordinateData() const
	{
		return _nodeCoordinateData;
	}

	std::map<int, std::pair<QString, QString>> mCoordinateManager::getNodeCoordinateNameData()
	{
		std::map<int, std::pair<QString, QString>> nameData;
		for (const auto& pair : _nodeCoordinateData)
		{
			nameData[pair.first] = std::make_pair(getCoordinateNameByID(pair.second.first), getCoordinateNameByID(pair.second.second));
		}
		return nameData;
	}

	void mCoordinateManager::clearNodeCoordinateData()
	{
		_nodeCoordinateData.clear();
	}

}