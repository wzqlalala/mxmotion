#include "mStructOption.h"

#include "mOptimizationGoalData.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationConstraintData.h"
namespace MechStructOptionData
{
	mStructOption::mStructOption()
	{
		_goalData = new mOptimizationGoalData();
		_maxOptimizationDesignID = 0;
		_maxOptimizationConstraintID = 0;
	}

	mStructOption::~mStructOption()
	{
		if (_goalData)
		{
			delete _goalData;
			_goalData = nullptr;
		}
		for (int i = 0; i < _designData.size(); i++)
		{
			auto data = _designData[i];
			if (data)
			{
				delete data;
			}
		}
		_designData.clear();

		for (int i = 0; i < _constraintData.size(); i++)
		{
			auto data = _constraintData[i];
			if (data)
			{
				delete data;
			}
		}
		_constraintData.clear();
	}


	mOptimizationDesignData* mStructOption::getOptimizationDesignData(int id)
	{
		auto it = _designData.find(id);
		if (it != _designData.end())
		{
			return _designData[id];
		}

		return nullptr;
	}

	mOptimizationDesignData* mStructOption::getOptimizationDesignData(const QString& name) const
	{
		for (auto it : _designData)
		{
			mOptimizationDesignData* base = it.second;
			if (base != nullptr)
			{
				if (base->getName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mStructOption::getOptimizationDesignNameByID(int id) const
	{
		if (_designData.find(id) != _designData.cend())
			return _designData.at(id)->getName();

		return QString();
	}

	int mStructOption::getOptimizationDesignIDByName(const QString& name) const
	{
		for (const auto& pair : _designData) {
			if (pair.second && pair.second->getName() == name) 
				return pair.first;
		}
		return -1;
	}

	void mStructOption::addOptimizationDesignData(mOptimizationDesignData* base)
	{
		_designData.insert({ base->getId(), base });
	}

	bool mStructOption::existOptimizationDesignName(const QString& name) const
	{
		for (auto it : _designData)
		{
			if (it.second != nullptr && it.second->getName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mStructOption::resetOptimizationDesignName(const QString& oldName, const QString& newName)
	{
		for (auto it : _designData)
		{
			if (it.second != nullptr && it.second->getName() == oldName)
			{
				it.second->setName(newName);
				break;
			}
		}
	}

	int mStructOption::copyOptimizationDesignData(const int& id)
	{
		mOptimizationDesignData* mat = getOptimizationDesignData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existOptimizationDesignName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mOptimizationDesignData* copy = mat->copyData(createOptimizationDesignID(), copyName);
		addOptimizationDesignData(copy);
		return copy->getId();
	}

	void mStructOption::deleteAllOptimizationDesignData()
	{
		for (auto it : _designData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_designData.clear();
	}

	void mStructOption::deleteOptimizationDesignData(int id)
	{
		if (_designData.find(id) != _designData.cend())
		{
			if (_designData[id] != nullptr)
			{
				delete _designData[id];
				_designData[id] = nullptr;
			}
			_designData.erase(id);
		}
	}

	std::vector<int> mStructOption::getAllOptimizationDesignIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _designData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mStructOption::getAllOptimizationDesignNames() const
	{
		QStringList names;
		for (auto it : _designData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getName());
		}
		return names;
	}

	QStringList mStructOption::getAllOptimizationMasterDesignNames() const
	{
		QStringList names;
		for (auto it : _designData)
		{
			if (it.second == nullptr)
				continue;

			if (!it.second->_patternRepStatus || it.second->_domainType == "从设计域")
				continue;
			
			names.append(it.second->getName());
		}
		return names;
	}

	int mStructOption::createOptimizationDesignID()
	{
		_maxOptimizationDesignID += 1;
		return _maxOptimizationDesignID;
	}

	mOptimizationConstraintData* mStructOption::getOptimizationConstraintData(int id)
	{
		auto it = _constraintData.find(id);
		if (it != _constraintData.end())
		{
			return _constraintData[id];
		}

		return nullptr;
	}

	mOptimizationConstraintData* mStructOption::getOptimizationConstraintData(const QString& name) const
	{
		for (auto it : _constraintData)
		{
			mOptimizationConstraintData* base = it.second;
			if (base != nullptr)
			{
				if (base->getName() == name)
				{
					return base;
				}
			}
		}
		return nullptr;
	}

	QString mStructOption::getOptimizationConstraintNameByID(int id) const
	{
		if (_constraintData.find(id) != _constraintData.cend())
			return _constraintData.at(id)->getName();

		return QString();
	}

	int mStructOption::getOptimizationConstraintIDByName(const QString& name) const
	{
		for (const auto& pair : _constraintData) {
			if (pair.second && pair.second->getName() == name)
				return pair.first;
		}
		return -1;
	}

	void mStructOption::addOptimizationConstraintData(mOptimizationConstraintData* base)
	{
		_constraintData.insert({ base->getId(), base });
	}

	bool mStructOption::existOptimizationConstraintName(const QString& name) const
	{
		for (auto it : _constraintData)
		{
			if (it.second != nullptr && it.second->getName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mStructOption::resetOptimizationConstraintName(const QString& oldName, const QString& newName)
	{
		for (auto it : _constraintData)
		{
			if (it.second != nullptr && it.second->getName() == oldName)
			{
				it.second->setName(newName);
				break;
			}
		}
	}

	int mStructOption::copyOptimizationConstraintData(const int& id)
	{
		mOptimizationConstraintData* mat = getOptimizationConstraintData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existOptimizationConstraintName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mOptimizationConstraintData* copy = mat->copyData(createOptimizationConstraintID(), copyName);
		addOptimizationConstraintData(copy);
		return copy->getId();
	}

	void mStructOption::deleteAllOptimizationConstraintData()
	{
		for (auto it : _constraintData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_constraintData.clear();
	}

	void mStructOption::deleteOptimizationConstraintData(int id)
	{
		if (_constraintData.find(id) != _constraintData.cend())
		{
			if (_constraintData[id] != nullptr)
			{
				delete _constraintData[id];
				_constraintData[id] = nullptr;
			}
			_constraintData.erase(id);
		}
	}

	std::vector<int> mStructOption::getAllOptimizationConstraintIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _constraintData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mStructOption::getAllOptimizationConstraintNames() const
	{
		QStringList names;
		for (auto it : _constraintData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getName());
		}
		return names;
	}

	int mStructOption::createOptimizationConstraintID()
	{
		_maxOptimizationConstraintID += 1;
		return _maxOptimizationConstraintID;
	}
}