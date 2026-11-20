#include "mLaminateManager.h"
#include "mLaminateBase.h"
#include "mGlobalSignals.h"
#include <vector>

namespace MechPropertyData
{
	mLaminateManager::mLaminateManager()
	{
		_maxID = 0;
	}

	mLaminateManager::~mLaminateManager()
	{
		deleteAllLaminate();
	}

	mLaminateBase* mLaminateManager::getLaminateData(int id)
	{
		auto it = _LaminateData.find(id);
		if (it != _LaminateData.end())
		{
			return _LaminateData[id];
		}
		return nullptr;
	}

	mLaminateBase* mLaminateManager::getLaminateData(const QString& name)
	{
		for (auto it : _LaminateData)
		{
			if (it.second != nullptr && it.second->getLaminateName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mLaminateManager::getLaminateNameByID(int id)
	{
		if (_LaminateData.find(id) != _LaminateData.cend())
			return _LaminateData[id]->getLaminateName();

		return QString();
	}

	int mLaminateManager::getIDByLaminateName(QString name)
	{
		for (auto& it : _LaminateData)
		{
			if (it.second && it.second->getLaminateName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mLaminateManager::AddLaminateData(mLaminateBase* base)
	{
		_LaminateData.insert({ base->getLaminateId(), base });
	}

	bool mLaminateManager::ExistLaminateName(const QString& name)
	{
		for (auto it : _LaminateData)
		{
			if (it.second != nullptr && it.second->getLaminateName() == name)
			{
				return true;
			}
		}
		return false;
	}

	void mLaminateManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _LaminateData)
		{
			if (it.second != nullptr && it.second->getLaminateName() == oldName)
			{
				it.second->setLaminateName(newName);
				break;
			}
		}
	}

	int mLaminateManager::copyLaminateData(const int& id)
	{
		mLaminateBase* laminate = getLaminateData(id);
		if (laminate == nullptr)
			return 0;

		QString copyName = laminate->getLaminateName() + "-copy";
		while (ExistLaminateName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mLaminateBase* copylaminate = laminate->copyLaminate(createID(), copyName);
		AddLaminateData(copylaminate);
		return copylaminate->getLaminateId();
	}

	void mLaminateManager::deleteAllLaminate()
	{
		for (auto it : _LaminateData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_LaminateData.clear();
	}

	void mLaminateManager::deleteLaminateById(const int& id)
	{
		if (_LaminateData.find(id) != _LaminateData.cend())
		{
			if (_LaminateData[id] != nullptr)
			{
				delete _LaminateData[id];
			}
			_LaminateData.erase(id);
		}
	}

	std::vector<int> mLaminateManager::getAllLaminateID()
	{
		std::vector<int> keys;
		for (auto pair : _LaminateData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mLaminateManager::getAllLaminateName()
	{
		QStringList names;
		for (auto it : _LaminateData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getLaminateName());
		}
		return names;
	}

	QStringList mLaminateManager::getAllLaminateByTypes(MxFunctions::LaminateType type)
	{
		QStringList names;
		for (auto it : _LaminateData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getLaminateType() == type)
			{
				names.append(it.second->getLaminateName());
			}
		}
		return names;
	}

	int mLaminateManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}