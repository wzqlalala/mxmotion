#include "mSetManager.h"
#include "mSetBase.h"
#include <vector>

namespace MechSetData
{
	mSetManager::mSetManager()
	{
		_maxID = 0;
	}

	mSetManager::~mSetManager()
	{
		for (auto it : _setData)
		{
			if (it.second != nullptr)
			{
				delete it.second;
				it.second = nullptr;
			}
		}

		_setData.clear();
	}

	mSetBase* mSetManager::getSetData(int id)
	{
		auto it = _setData.find(id);
		if (it != _setData.end())
		{
			return _setData[id];
		}

		return nullptr;
	}

	mSetBase* mSetManager::getSetData(const QString& name)
	{
		for (auto it : _setData)
		{
			if (it.second != nullptr && it.second->getSetName() == name && !it.second->getInternal())//内部集合不能通过名字拿
			{
				return it.second;
			}
		}

		return nullptr;
	}

	QString mSetManager::getSetNameByID(int id)
	{
		if (_setData.find(id) != _setData.cend())
			return _setData[id]->getSetName();

		return QString();
	}

	int mSetManager::getIDBySetName(QString name, bool isContainAutoCreate)
	{
		for (auto& it : _setData)
		{
			if (it.second && it.second->getSetName() == name &&
				(isContainAutoCreate || !it.second->getInternal()))
			{
				return it.first;
			}
		}

		return -1;
	}

	void mSetManager::AddSetData(mSetBase* base)
	{
		_setData.insert({ base->getSetId(), base });
	}

	void mSetManager::replaceSetData(mSetBase* base)
	{
		deleteSet(base->getSetId());
		_setData.insert({ base->getSetId(), base });
	}

	bool mSetManager::ExistSetName(const QString& name, bool isInternal)
	{
		for (auto it : _setData)
		{
			if (it.second != nullptr && it.second->getSetName() == name && (!it.second->getInternal() || isInternal))
			{
				return true;
			}
		}

		return false;
	}

	void mSetManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _setData)
		{
			if (it.second != nullptr && it.second->getSetName() == oldName && !it.second->getInternal())//内部集合不判断
			{
				it.second->setSetName(newName);
				break;
			}
		}
	}

	int mSetManager::copySetData(const int& id)
	{
		mSetBase* mat = getSetData(id);
		if (mat == nullptr)
			return 0;

		QString copyName = mat->getSetName() + "-copy";
		while (ExistSetName(copyName))
		{
			copyName = copyName + "-copy";
		}

		mSetBase* copySet = mat->copySet(createID(), copyName);
		AddSetData(copySet);
		return copySet->getSetId();
	}

	void mSetManager::deleteAllSet()
	{
		for (auto it : _setData)
		{
			if (it.second == nullptr)
				continue;

			delete it.second;
			it.second = nullptr;
		}
		_setData.clear();
	}

	void mSetManager::deleteSet(const int& id)
	{
		if (_setData.find(id) != _setData.cend())
		{
			if (_setData[id] != nullptr)
			{
				delete _setData[id];
				_setData[id] = nullptr;
			}
			_setData.erase(id);
		}
	}

	bool mSetManager::containsSet(const mSetBase* base) const
	{
		if (base == nullptr)
		{
			return false;
		}
		for (const auto& it : _setData)
		{
			if (it.second == base)
			{
				return true;
			}
		}
		return false;
	}

	std::vector<int> mSetManager::getAllSetID(bool isContainAutoCreate)
	{
		std::vector<int> keys;
		for (auto pair : _setData)
		{
			if (!pair.second)
			{
				continue;
			}
			if (!isContainAutoCreate && pair.second->getInternal())//不包含自动创建的
			{
				continue;
			}
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mSetManager::getAllSetNames(bool isContainAutoCreate)
	{
		QStringList names;
		for (auto pair : _setData)
		{
			if (!pair.second)
			{
				continue;
			}
			if (!isContainAutoCreate && pair.second->getInternal())//不包含自动创建的
			{
				continue;
			}
			names.push_back(pair.second->getSetName());
		}
		return names;
	}

	QStringList mSetManager::getAllSetNamesByTypes(MxFunctions::SetType type, bool isContainAutoCreate)
	{
		QStringList names;
		for (auto pair : _setData)
		{
			if (!pair.second)
			{
				continue;
			}
			if (!isContainAutoCreate && pair.second->getInternal())//不包含自动创建的
			{
				continue;
			}
			if (pair.second->getSetType() == type)
			{
				names.append(pair.second->getSetName());
			}
		}
		return names;
	}

	int mSetManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	bool mSetManager::beingUsed(const MxFunctions::SetType& settype, const MxFunctions::LoadBoundaryType& loadtype)
	{
		switch (settype)
		{
		case MxFunctions::SetType::Node:
		{
			if (
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_FORCE ||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FIXED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_DISPLACEMENT ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_SPEED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_ACC ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FORWARD ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_VIRTUAL)
			{
				return true;
			}
		}
		case MxFunctions::SetType::GeoPoint:
		{
			if (
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_FORCE ||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FIXED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_DISPLACEMENT ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_SPEED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_ACC ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FORWARD ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_VIRTUAL
				)
			{
				return true;
			}
		}
		case MxFunctions::SetType::GeoEdge:
		{
			if (
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_FORCE ||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FIXED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_DISPLACEMENT ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_ACC ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_SPEED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FORWARD ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_VIRTUAL
				)
			{
				return true;
			}
		}
		case MxFunctions::SetType::GeoFace:
		{
			if (
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_FORCE ||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_PRESSURE||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FIXED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_DISPLACEMENT ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_SPEED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_ACC ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FORWARD ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_VIRTUAL
				)
			{
				return true;
			}
		}
		case MxFunctions::SetType::GeoSolid:
		{
			if (
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_FORCE ||
				loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_TORQUE||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FIXED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_DISPLACEMENT ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_SPEED ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_ACC ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_FORWARD ||
				loadtype == MxFunctions::LoadBoundaryType::BOUNDARY_IM_VIRTUAL
				)
			{
				return true;
			}
		}
		case MxFunctions::SetType::ElementSurface:
		{
			if (loadtype == MxFunctions::LoadBoundaryType::LOAD_IM_PRESSURE)
			{
				return true;
			}
		}
		default:
			break;
		}

		return false;
	}

	/*
	bool mSetManager::beingUsed(const MxFunctions::SetType& settype, const MxFunctions::LoadBoundaryType& boundarytype)
	{
		switch (settype)
		{
		case MxFunctions::SetType::Node:
		{
			
			return true;
		}
		case MxFunctions::SetType::GeoPoint:
		{
			return true;
		}
		case MxFunctions::SetType::GeoEdge:
		{
			return true;
		}
		case MxFunctions::SetType::GeoFace:
		{
			return true;
		}
		case MxFunctions::SetType::GeoSolid:
		{
			return true;
		}
		default:
			break;
		}

		return false;
	}*/

	bool mSetManager::beingUsed(const MxFunctions::SetType& settype, const MxFunctions::ContactType& boundarytype)
	{
		switch (settype)
		{
		case MxFunctions::SetType::ElementSurface:
		{
			return true;
		}
		case MxFunctions::SetType::GeoFace:
		{
			return true;
		}
		default:
			break;
		}

		return false;
	}
}