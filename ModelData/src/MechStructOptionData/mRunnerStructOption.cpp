#include "mRunnerStructOption.h"

#include "mRunnerOptimizationGoalData.h"
#include "mRunnerOptimizationDesignData.h"
#include "mRunnerOptimizationConstraintData.h"
namespace MechStructOptionData
{
	mRunnerStructOption::mRunnerStructOption()
	{
		_goalData = new mRunnerOptimizationGoalData();
		_maxOptimizationDesignID = 0;
		_maxOptimizationConstraintID = 0;
	}

	mRunnerStructOption::~mRunnerStructOption()
	{
		if (_goalData)
		{
			delete _goalData;
			_goalData = nullptr;
		}
		for (auto it = _designData.begin(); it != _designData.end(); ++it)
		{
			auto data = it->second;
			if (data)
			{
				delete data;
			}
		}
		_designData.clear();

		for (auto it = _constraintData.begin(); it != _constraintData.end(); ++it)
		{
			auto data = it->second;
			if (data)
			{
				delete data;
			}
		}
		_constraintData.clear();
	}


	mRunnerOptimizationDesignData* mRunnerStructOption::getOptimizationDesignData(int id)
	{
		auto it = _designData.find(id);
		if (it != _designData.end())
		{
			return _designData[id];
		}

		return nullptr;
	}

	mRunnerOptimizationDesignData* mRunnerStructOption::getOptimizationDesignData(const QString& name) const
	{
		for (auto it : _designData)
		{
			mRunnerOptimizationDesignData* base = it.second;
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

	QString mRunnerStructOption::getOptimizationDesignNameByID(int id) const
	{
		if (_designData.find(id) != _designData.cend())
			return _designData.at(id)->getName();

		return QString();
	}

	int mRunnerStructOption::getOptimizationDesignIDByName(const QString& name) const
	{
		for (const auto& pair : _designData) {
			if (pair.second && pair.second->getName() == name) 
				return pair.first;
		}
		return -1;
	}

	void mRunnerStructOption::addOptimizationDesignData(mRunnerOptimizationDesignData* base)
	{
		_designData.insert({ base->getId(), base });
	}

	bool mRunnerStructOption::existOptimizationDesignName(const QString& name) const
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

	void mRunnerStructOption::resetOptimizationDesignName(const QString& oldName, const QString& newName)
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

	int mRunnerStructOption::copyOptimizationDesignData(const int& id)
	{
		mRunnerOptimizationDesignData* mat = getOptimizationDesignData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existOptimizationDesignName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mRunnerOptimizationDesignData* copy = mat->copyData(createOptimizationDesignID(), copyName);
		addOptimizationDesignData(copy);
		return copy->getId();
	}

	void mRunnerStructOption::deleteAllOptimizationDesignData()
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

	void mRunnerStructOption::deleteOptimizationDesignData(int id)
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

	std::vector<int> mRunnerStructOption::getAllOptimizationDesignIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _designData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mRunnerStructOption::getAllOptimizationDesignNames() const
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

	QStringList mRunnerStructOption::getAllOptimizationMasterDesignNames() const
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

	int mRunnerStructOption::createOptimizationDesignID()
	{
		_maxOptimizationDesignID += 1;
		return _maxOptimizationDesignID;
	}

	mRunnerOptimizationConstraintData* mRunnerStructOption::getOptimizationConstraintData(int id)
	{
		auto it = _constraintData.find(id);
		if (it != _constraintData.end())
		{
			return _constraintData[id];
		}

		return nullptr;
	}

	mRunnerOptimizationConstraintData* mRunnerStructOption::getOptimizationConstraintData(const QString& name) const
	{
		for (auto it : _constraintData)
		{
			mRunnerOptimizationConstraintData* base = it.second;
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

	QString mRunnerStructOption::getOptimizationConstraintNameByID(int id) const
	{
		if (_constraintData.find(id) != _constraintData.cend())
			return _constraintData.at(id)->getName();

		return QString();
	}

	int mRunnerStructOption::getOptimizationConstraintIDByName(const QString& name) const
	{
		for (const auto& pair : _constraintData) {
			if (pair.second && pair.second->getName() == name)
				return pair.first;
		}
		return -1;
	}

	void mRunnerStructOption::addOptimizationConstraintData(mRunnerOptimizationConstraintData* base)
	{
		_constraintData.insert({ base->getId(), base });
	}

	bool mRunnerStructOption::existOptimizationConstraintName(const QString& name) const
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

	void mRunnerStructOption::resetOptimizationConstraintName(const QString& oldName, const QString& newName)
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

	int mRunnerStructOption::copyOptimizationConstraintData(const int& id)
	{
		mRunnerOptimizationConstraintData* mat = getOptimizationConstraintData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getName() + "-copy";
		while (existOptimizationConstraintName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mRunnerOptimizationConstraintData* copy = mat->copyData(createOptimizationConstraintID(), copyName);
		addOptimizationConstraintData(copy);
		return copy->getId();
	}

	void mRunnerStructOption::deleteAllOptimizationConstraintData()
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

	void mRunnerStructOption::deleteOptimizationConstraintData(int id)
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

	std::vector<int> mRunnerStructOption::getAllOptimizationConstraintIDs() const
	{
		std::vector<int> keys;
		for (auto pair : _constraintData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mRunnerStructOption::getAllOptimizationConstraintNames() const
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

	int mRunnerStructOption::createOptimizationConstraintID()
	{
		_maxOptimizationConstraintID += 1;
		return _maxOptimizationConstraintID;
	}
}
