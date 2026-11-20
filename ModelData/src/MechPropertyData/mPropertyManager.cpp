#include "mPropertyManager.h"
#include "mPropertyBase.h"

#include "mGlobalSignals.h"
#include <vector>

namespace MechPropertyData
{
	mPropertyManager::mPropertyManager()
	{
		_maxID = 0;
	}

	mPropertyManager::~mPropertyManager()
	{
		for (auto it : _propertyData)
		{
			mPropertyBase * base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}

		_propertyData.clear();
	}

	mPropertyBase * mPropertyManager::getPropertyData(int id)
	{
		auto it = _propertyData.find(id);
		if (it != _propertyData.end())
		{
			return _propertyData[id];
		}

		return nullptr;
	}

	mPropertyBase* mPropertyManager::getPropertyData(const QString& name)
	{
		for (auto it : _propertyData)
		{
			mPropertyBase* base = it.second;
			if (base != nullptr)
			{
				if (base->getPropertyName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mPropertyManager::getPropertyNameByID(int id)
	{
		if (_propertyData.find(id) != _propertyData.cend())
			return _propertyData[id]->getPropertyName();

		return QString();
	}

	int mPropertyManager::getIDByPropertyName(QString name)
	{
		for (auto& it : _propertyData)
		{
			if (it.second && it.second->getPropertyName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mPropertyManager::AddPropertyData(mPropertyBase * base)
	{
		_propertyData.insert({ base->getPropertyId(), base });
		//if (sendSignal)
		//{
		//	MxFunctions::mGlobalSignals::getInstance()->updateModelData(MxFunctions::Physical_Property);
		//}
	}

	bool mPropertyManager::ExistPropertyName(const QString& name)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getPropertyName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mPropertyManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _propertyData)
		{
			if (it.second != nullptr && it.second->getPropertyName() == oldName)
			{
				it.second->setPropertyName(newName);
				break;
			}
		}
	}

	int mPropertyManager::copyPropertyData(const int & id)
	{
		mPropertyBase* mat = getPropertyData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getPropertyName() + "-copy";
		while (ExistPropertyName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mPropertyBase*copyProperty = mat->copyProperty(createID(),copyName);
		AddPropertyData(copyProperty);
		return copyProperty->getPropertyId();
	}

	void mPropertyManager::deleteAllProperty()
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

	void mPropertyManager::deleteProperty(const int& id)
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

	std::vector<int> mPropertyManager::getAllProperyID()
	{
		std::vector<int> keys;
		for (auto pair : _propertyData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mPropertyManager::getAllPropertyNames()
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getPropertyName());
		}
		return names;
	}

	QStringList mPropertyManager::getAllPropertyNamesByTypes(MxFunctions::PropertyType type)
	{
		QStringList names;
		for (auto it : _propertyData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getPropertyType() == type)
			{
				names.append(it.second->getPropertyName());
			}
		}
		return names;
	}
	int mPropertyManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

}