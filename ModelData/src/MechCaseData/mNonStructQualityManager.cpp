#include "mNonStructQualityManager.h"
#include "mNonStructuralQuality.h"

#include "mGlobalSignals.h"
#include <vector>

namespace MechCaseData
{
	mNonStructQualityManager::mNonStructQualityManager()
	{
		_maxID = 0;
	}

	mNonStructQualityManager::~mNonStructQualityManager()
	{
		for (auto it : _nsmDataMap)
		{
			mNonStructuralQuality* base = it.second;
			if (base != nullptr)
			{
				delete base;
				base = nullptr;
			}
		}

		_nsmDataMap.clear();
	}

	mNonStructuralQuality* mNonStructQualityManager::getNonStructuralQualityData(int id)
	{
		auto it = _nsmDataMap.find(id);
		if (it != _nsmDataMap.end())
		{
			return _nsmDataMap[id];
		}

		return nullptr;
	}

	mNonStructuralQuality* mNonStructQualityManager::getNonStructuralQualityData(const QString& name)
	{
		for (auto it : _nsmDataMap)
		{
			mNonStructuralQuality* base = it.second;
			if (base != nullptr)
			{
				if (base->getNonStructuralQualityName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mNonStructQualityManager::getNonStructuralQualityNameByID(int id)
	{
		if (_nsmDataMap.find(id) != _nsmDataMap.cend())
			return _nsmDataMap[id]->getNonStructuralQualityName();

		return QString();
	}

	int mNonStructQualityManager::getIDByNsmName(QString name)
	{
		for (auto& it : _nsmDataMap)
		{
			if (it.second && it.second->getNonStructuralQualityName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mNonStructQualityManager::AddNsmData(mNonStructuralQuality* base)
	{
		_nsmDataMap.insert({ base->getNonStructuralQualityId(), base });
		//if (sendSignal)
		//{
		//	MxFunctions::mGlobalSignals::getInstance()->updateModelData(MxFunctions::Physical_Property);
		//}
	}

	bool mNonStructQualityManager::ExistNsmName(const QString& name)
	{
		for (auto it : _nsmDataMap)
		{
			if (it.second != nullptr && it.second->getNonStructuralQualityName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mNonStructQualityManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _nsmDataMap)
		{
			if (it.second != nullptr && it.second->getNonStructuralQualityName() == oldName)
			{
				it.second->setNsmName(newName);
				break;
			}
		}
	}

	int mNonStructQualityManager::copyNsmData(const int & id)
	{
		mNonStructuralQuality* mat = getNonStructuralQualityData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getNonStructuralQualityName() + "-copy";
		while (ExistNsmName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mNonStructuralQuality*copyProperty = mat->copyNonStructuralQuality(createID(),copyName);
		AddNsmData(copyProperty);
		return copyProperty->getNonStructuralQualityId();
	}

	void mNonStructQualityManager::deleteAllNsm()
	{
		for (auto it : _nsmDataMap)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_nsmDataMap.clear();
	}

	void mNonStructQualityManager::deleteNsm(const int& id)
	{
		if (_nsmDataMap.find(id) != _nsmDataMap.cend())
		{
			if (_nsmDataMap[id] != nullptr)
			{
				delete _nsmDataMap[id];
				_nsmDataMap[id] = nullptr;
			}
			_nsmDataMap.erase(id);
		}
	}

	std::vector<int> mNonStructQualityManager::getAllNonSturcturalQualityID()
	{
		std::vector<int> keys;
		for (auto pair : _nsmDataMap)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mNonStructQualityManager::getAllNsmNames()
	{
		QStringList names;
		for (auto it : _nsmDataMap)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getNonStructuralQualityName());
		}
		return names;
	}

	//QStringList mNonStructQualityManager::getAllPropertyNamesByTypes(MxFunctions::PropertyType type)
	//{
	//	QStringList names;
	//	for (auto it : _propertyData)
	//	{
	//		if (it.second == nullptr)
	//			continue;
	//		if (it.second->getPropertyType() == type)
	//		{
	//			names.append(it.second->getNonStructuralQualityName());
	//		}
	//	}
	//	return names;
	//}
	int mNonStructQualityManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

}