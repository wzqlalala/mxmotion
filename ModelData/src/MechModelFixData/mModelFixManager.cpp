#include "mModelFixManager.h"

#include "mEleSetCtrl.h"

namespace MechModelFixData
{
	mModelFixManager::mModelFixManager()
	{
		_maxID = 0;
	}

	mModelFixManager::~mModelFixManager()
	{
		_modelFixDataMap.clear();
	}

	std::shared_ptr<mModelFixDataBase> mModelFixManager::getModelFixData(int id)
	{
		auto it = _modelFixDataMap.find(id);
		if (it != _modelFixDataMap.end())
		{
			return it->second;
		}

		return nullptr;
	}

	std::shared_ptr<mModelFixDataBase> mModelFixManager::getModelFixData(const QString& name)
	{
		for (auto it : _modelFixDataMap)
		{
			if (it.second != nullptr && it.second->getModelFixName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mModelFixManager::getModelFixNameByID(int id)
	{
		auto it = _modelFixDataMap.find(id);
		if (it != _modelFixDataMap.cend() && it->second != nullptr)
			return it->second->getModelFixName();

		return QString();
	}

	int mModelFixManager::getIDByModelFixName(QString name)
	{
		for (auto& it : _modelFixDataMap)
		{
			if (it.second && it.second->getModelFixName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mModelFixManager::AddModelFixData(std::shared_ptr<mModelFixDataBase> base)
	{
		if (base != nullptr)
			_modelFixDataMap[base->getId()] = std::move(base);
	}

	bool mModelFixManager::ExistModelFixName(const QString& name)
	{
		for (auto it : _modelFixDataMap)
		{
			if (it.second != nullptr && it.second->getModelFixName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mModelFixManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto& it : _modelFixDataMap)
		{
			if (it.second != nullptr && it.second->getModelFixName() == oldName)
			{
				it.second->setModelFixName(newName);
				break;
			}
		}
	}

	int mModelFixManager::copyModelFixData(const int& id)
	{
		std::shared_ptr<mModelFixDataBase> mat = getModelFixData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getModelFixName() + "-copy";
		while (ExistModelFixName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mModelFixDataBase> copyMat = mat->copyModelFixData(createID(), copyName);
		AddModelFixData(copyMat);
		return copyMat->getId();
	}

	void mModelFixManager::deleteAllModelFix()
	{
		_modelFixDataMap.clear();
	}

	void mModelFixManager::deleteModelFix(const int& id)
	{
		auto it = _modelFixDataMap.find(id);
		if (it != _modelFixDataMap.cend())
		{
			_modelFixDataMap.erase(it);
		}
	}

	std::vector<int> mModelFixManager::getAllModelFixID()
	{
		std::vector<int> keys;
		for (auto pair : _modelFixDataMap)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mModelFixManager::getAllModelFixNames()
	{
		QStringList names;
		for (auto it : _modelFixDataMap)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getModelFixName());
		}
		return names;
	}

	int mModelFixManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}
}
