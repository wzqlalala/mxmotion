#include "mPeriodicBoundaryManager.h"
#include "mPeriodicBoundary.h"
#include <vector>

namespace MechContactData
{
	mPeriodicBoundaryManager::mPeriodicBoundaryManager()
	{
		_maxID = 0;
	}

	mPeriodicBoundaryManager::~mPeriodicBoundaryManager()
	{
		for (auto it : _propertyData)
		{
			mPeriodicBoundary* base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}
		_propertyData.clear();
	}

	mPeriodicBoundary* mPeriodicBoundaryManager::getPeriodicBoundaryData(int id)
	{
		auto it = _propertyData.find(id);
		if (it != _propertyData.end())
			return _propertyData[id];
		return nullptr;
	}

	mPeriodicBoundary* mPeriodicBoundaryManager::getPeriodicBoundaryData(const QString& name)
	{
		for (auto it : _propertyData)
		{
			mPeriodicBoundary* base = it.second;
			if (base != nullptr && base->getPeriodicBoundaryName() == name)
				return base;
		}
		return nullptr;
	}

	QString mPeriodicBoundaryManager::getPeriodicBoundaryNameByID(int id)
	{
		if (_propertyData.find(id) != _propertyData.cend())
			return _propertyData[id]->getPeriodicBoundaryName();
		return QString();
	}

	int mPeriodicBoundaryManager::getPeriodicBoundaryIdByName(const QString& name)
	{
		for (auto& it : _propertyData)
		{
			if (it.second && it.second->getPeriodicBoundaryName() == name)
				return it.first;
		}
		return -1;
	}

	void mPeriodicBoundaryManager::AddPeriodicBoundaryData(mPeriodicBoundary* base)
	{
		_propertyData.insert({ base->getPeriodicBoundaryId(), base });
	}

	bool mPeriodicBoundaryManager::ExistPeriodicBoundaryName(const QString& name)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getPeriodicBoundaryName() == name)
				return true;
		}
		return false;
	}

	void mPeriodicBoundaryManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getPeriodicBoundaryName() == oldName)
			{
				it.second->setPeriodicBoundaryName(newName);
				break;
			}
		}
	}

	int mPeriodicBoundaryManager::copyPeriodicBoundaryData(const int& id)
	{
		mPeriodicBoundary* data = getPeriodicBoundaryData(id);
		if (data == nullptr)
			return 0;

		QString copyName = data->getPeriodicBoundaryName() + "-copy";
		while (ExistPeriodicBoundaryName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mPeriodicBoundary* copyData = data->copyPeriodicBoundary(createID(), copyName);
		AddPeriodicBoundaryData(copyData);
		return copyData->getPeriodicBoundaryId();
	}

	void mPeriodicBoundaryManager::deleteAllPeriodicBoundary()
	{
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			delete it.second;
			it.second = nullptr;
		}
		_propertyData.clear();
	}

	void mPeriodicBoundaryManager::deletePeriodicBoundary(const int& id)
	{
		if (_propertyData.find(id) != _propertyData.cend())
		{
			if (_propertyData[id] != nullptr)
				{
					delete _propertyData[id];
					_propertyData[id] = nullptr;
				}
			_propertyData.erase(id);
		}
	}

	std::vector<int> mPeriodicBoundaryManager::getAllPeriodicBoundaryID()
	{
		std::vector<int> keys;
		for (auto pair : _propertyData)
			keys.push_back(pair.first);
		return keys;
	}

	QStringList mPeriodicBoundaryManager::getAllPeriodicBoundaryNames()
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			names.append(it.second->getPeriodicBoundaryName());
		}
		return names;
	}

	QStringList mPeriodicBoundaryManager::getAllPeriodicBoundaryNamesByTypes(MxFunctions::PeriodicBoundaryType type)
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getPeriodicBoundaryType() == type)
				names.append(it.second->getPeriodicBoundaryName());
		}
		return names;
	}

	std::vector<int> mPeriodicBoundaryManager::getAllPeriodicBoundaryIdsByTypes(MxFunctions::PeriodicBoundaryType type)
	{
		std::vector<int> ids;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getPeriodicBoundaryType() == type)
				ids.push_back(it.second->getPeriodicBoundaryId());
		}
		return ids;
	}

	int mPeriodicBoundaryManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}
