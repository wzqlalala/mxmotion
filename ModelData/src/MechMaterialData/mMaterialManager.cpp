#include "mMaterialManager.h"
#include "mMaterialBase.h"


namespace MechMaterialData
{
	mMaterialManager::mMaterialManager()
	{
		_maxID = 0;
	}

	mMaterialManager::~mMaterialManager()
	{
		_materialData.clear();
	}

	std::shared_ptr<mMaterialBase> mMaterialManager::getMaterialData(const int& id)
	{
		auto it = _materialData.find(id);
		if (it != _materialData.end())
		{
			return _materialData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mMaterialBase> mMaterialManager::getMaterialData(const QString& name)
	{
		for (auto it : _materialData)
		{
			if (it.second->getMaterialName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mMaterialManager::getMatNameByID(int id)
	{
		if (_materialData.find(id) != _materialData.cend())
			return _materialData[id]->getMaterialName();

		return QString();
	}

	int mMaterialManager::getIDByMatName(QString name)
	{
		for (auto& it : _materialData)
		{
			if (it.second && it.second->getMaterialName() == name)
			{
				return it.first;
			}
		}

		return -1;
	}

	void mMaterialManager::AddMaterialData(mMaterialBase* mat)
	{
		if (!mat) return; 
		auto matptr = std::shared_ptr<mMaterialBase>(mat);
		_materialData[mat->getMaterialId()] = std::move(matptr);
		
	}

	void mMaterialManager::AddMaterialData(std::shared_ptr<mMaterialBase>mat)
	{
		_materialData[mat->getMaterialId()] = std::move(mat);
	}

	bool mMaterialManager::ExistMaterialName(const QString & name)
	{
		for (auto it : _materialData)
		{
			if (it.second != nullptr && it.second->getMaterialName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mMaterialManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _materialData)
		{
			if (it.second != nullptr && it.second->getMaterialName() == oldName)
			{
				it.second->setMaterialName(newName);
				break;
			}
		}
	}

	int mMaterialManager::copyMaterialData(const int& id)
	{
		std::shared_ptr<mMaterialBase> mat = getMaterialData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getMaterialName() + "-copy";
		while (ExistMaterialName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mMaterialBase>copyMat = mat->copyMaterial(createID(), copyName);
		AddMaterialData(copyMat);
		return copyMat->getMaterialId();
	}

	void mMaterialManager::deleteAllMat()
	{
		_materialData.clear();
	}

	void mMaterialManager::deleteMat(const int& id)
	{
		if (_materialData.find(id) != _materialData.cend())
		{
			_materialData.erase(id);
		}
	}

	std::vector<int> mMaterialManager::getAllMatID()
	{
		std::vector<int> keys;
		for (auto pair : _materialData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mMaterialManager::getAllMatNames()
	{
		QStringList names;
		for (auto pair : _materialData)
		{
			names.push_back(pair.second->getMaterialName());
		}
		return names;
	}

	QStringList mMaterialManager::getAllMatNamesByTypes(MxFunctions::MaterialType type)
	{
		QStringList names;
		for (auto it : _materialData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getMaterialType() == type)
			{
				names.append(it.second->getMaterialName());
			}
		}
		return names;
	}
	int mMaterialManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	void mMaterialManager::responseUnitChanged(const int& unit, mUnitSystemManager* unitsystemmanager)
	{
		for (auto& it : _materialData)
		{
			it.second->responseUnitChanged(unit,unitsystemmanager);
		}
	}
}