#include "mFunctionManager.h"
#include "mFunctionBase.h"


namespace MechFunctionData
{
	mFunctionManager::mFunctionManager()
	{
		_maxID = 0;
	}

	mFunctionManager::~mFunctionManager()
	{
	
	}

	mFunctionBase* mFunctionManager::getFunctionData(int id)
	{
		auto it = _functionData.find(id);
		if (it != _functionData.end())
		{
			return _functionData[id];
		}

		return nullptr;
	}

	QString mFunctionManager::getFunctionNameByID(int id)
	{
		if (_functionData.find(id) != _functionData.cend())
			return _functionData[id]->getFunctionName();

		return QString();
	}

	int mFunctionManager::getIDByFunctionName(QString name)
	{
		for (auto& it : _functionData)
		{
			if (it.second && it.second->getFunctionName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mFunctionManager::AddFunctionData(mFunctionBase* func)
	{
		if (!func) return;
		_functionData[func->getFunctionId()] = std::move(func);
	}

	bool mFunctionManager::ExistFunctionName(const QString& name)
	{
		for (auto it : _functionData)
		{
			if (it.second != nullptr && it.second->getFunctionName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mFunctionManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _functionData)
		{
			if (it.second != nullptr && it.second->getFunctionName() == oldName)
			{
				it.second->setFunctionName(newName);
				break;
			}
		}
	}

	int mFunctionManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	QStringList mFunctionManager::getAllFunctionNames()
	{
		QStringList names;
		for (auto pair : _functionData)
		{
			names.push_back(pair.second->getFunctionName());
		}
		return names;
	}

	std::vector<int> mFunctionManager::getAllFunctionID()
	{
		std::vector<int> keys;
		for (auto pair : _functionData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	void mFunctionManager::deleteAllFunction()
	{
		for (auto it : _functionData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_functionData.clear();
	}

	void mFunctionManager::deleteFunction(const int& id)
	{
		if (_functionData.find(id) != _functionData.cend())
		{
			if (_functionData[id] != nullptr)
			{
				delete _functionData[id];
				_functionData[id] = nullptr;
			}
			_functionData.erase(id);
		}
	}

	int mFunctionManager::copyFunctionData(const int& id)
	{
		mFunctionBase* func = getFunctionData(id);
		if (func == nullptr)
			return 0;

		QString copyName = func->getFunctionName() + "-copy";
		while (ExistFunctionName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mFunctionBase* copyFunc = func->copyFunction(createID(), copyName);
		AddFunctionData(copyFunc);
		return copyFunc->getFunctionId();
	}

	int mFunctionManager::getMaxID()
	{
		return _maxID;
	}
}