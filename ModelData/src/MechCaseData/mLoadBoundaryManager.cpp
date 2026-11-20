#include "mLoadBoundaryManager.h"
#include "mLoadBoundaryBase.h"
#include "mGlobalSignals.h"
#include "mLoadBolt.h"


namespace MechCaseData
{
	mLoadBoundaryManager::mLoadBoundaryManager()
	{
		_maxID = 0;
	}

	mLoadBoundaryManager::~mLoadBoundaryManager()
	{
		_loadBoundaryData.clear();
	}

	std::shared_ptr<mLoadBoundaryBase> mLoadBoundaryManager::getLoadBoundaryData(int id)
	{
		auto it = _loadBoundaryData.find(id);
		if (it != _loadBoundaryData.end())
		{
			return _loadBoundaryData[id];
		}

		return nullptr;
	}

	std::shared_ptr<mLoadBoundaryBase> mLoadBoundaryManager::getLoadBoundaryData(const QString& name)
	{
		for (auto it : _loadBoundaryData)
		{
			if (it.second != nullptr && it.second->getLoadBoundaryName() == name)
			{
				return it.second;
			}
		}
		return nullptr;
	}

	QString mLoadBoundaryManager::getLoadBoundaryNameByID(int id)
	{
		if (_loadBoundaryData.find(id) != _loadBoundaryData.cend())
			return _loadBoundaryData[id]->getLoadBoundaryName();

		return QString();
	}

	QString mLoadBoundaryManager::getLoadBoundaryTypeNameByID(int id)
	{
		LoadBoundaryType type;
		if (_loadBoundaryData.find(id) != _loadBoundaryData.cend())
			type = _loadBoundaryData[id]->getLoadBoundaryType();

		QString typeName;

		switch (type)
		{
		case MxFunctions::LOAD_IM_FORCE:
			typeName = "力(力学载荷)";
			break;
		case MxFunctions::LOAD_IM_GRAVITY:
			typeName = "重力(力学载荷)";
			break;
		case MxFunctions::LOAD_IM_TORQUE:
			typeName = "力矩(力学载荷)";
			break;
		case MxFunctions::LOAD_IM_PRESSURE:
			typeName = "压力(力学载荷)";
			break;
		case MxFunctions::LOAD_IM_HEATFLUX:
			typeName = "热流密度(热学载荷)";
			break;
		case MxFunctions::LOAD_IM_RADIATION:
			typeName = "热辐射(热学载荷)";
			break;
		case MxFunctions::LOAD_IM_CONVECTION:
			typeName = "对流换热(热学载荷)";
			break;
		case MxFunctions::LOAD_IM_TRANSIENT:
			typeName = "时域载荷(动态载荷)";
			break;
		case MxFunctions::LOAD_IM_FREQDOMAIN:
			typeName = "频域载荷(动态载荷)";
			break;
		case MxFunctions::LOAD_IM_COMBINED:
			typeName = "组合载荷(动态载荷)";
			break;		
		case MxFunctions::LOAD_IM_BOLT:
			typeName = "螺栓载荷(力学载荷)";
			break;		
		case MxFunctions::LOAD_IM_ACOCONCFORCE:
			typeName = "声学载荷(动态载荷)";
			break;		
		case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
			typeName = "声学频域载荷(动态载荷)";
			break;
		case MxFunctions::LOAD_IM_SPECTRUM:
			typeName = "谱载荷(动态载荷)";
			break;		
		case MxFunctions::LOAD_IM_SURFACE:
			typeName = "表面载荷(力学载荷)";
			break;		
		case MxFunctions::LOAD_IM_EDGE:
			typeName = "边载荷(力学载荷)";
			break;
		case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
			typeName = "旋转体力(力学载荷)";
			break;
		case MxFunctions::BOUNDARY_IM_FIXED:
			typeName = "约束";
			break;
		case MxFunctions::BOUNDARY_IM_TEMP:
			typeName = "温度边界";
			break;
		case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
			 typeName = "位移/转角";
			break;
		case MxFunctions::BOUNDARY_IM_SPEED:
			 typeName = "速度";
			break;
		case MxFunctions::BOUNDARY_IM_ACC:
			 typeName = "加速度";
			break;
		case MxFunctions::BOUNDARY_MOVE:
			 typeName = "位移边界";
			break;
		case MxFunctions::BOUNDARY_TEMP_NEW:
			 typeName = "温度边界";
			break;
		case MxFunctions::BOUNDARY_IM_VIRTUAL:
			 typeName = "虚拟约束";
			break;
		case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
			 typeName = "超单元边界";
			break;
		default:
			 typeName = QString();
			break;
		}
		return typeName;
	}

	QString mLoadBoundaryManager::getLoadBoundaryTypeNameByType(LoadBoundaryType type)
	{
		QString typeName;

		switch (type)
		{
		case MxFunctions::LOAD_IM_FORCE:
			typeName = "集中力";
			break;
		case MxFunctions::LOAD_IM_GRAVITY:
			typeName = "重力";
			break;
		case MxFunctions::LOAD_IM_TORQUE:
			typeName = "力矩";
			break;
		case MxFunctions::LOAD_IM_PRESSURE:
			typeName = "压强";
			break;
		case MxFunctions::LOAD_IM_HEATFLUX:
			typeName = "热流密度";
			break;
		case MxFunctions::LOAD_IM_RADIATION:
			typeName = "热辐射";
			break;
		case MxFunctions::LOAD_IM_CONVECTION:
			typeName = "对流换热";
			break;
		case MxFunctions::LOAD_IM_TRANSIENT:
			typeName = "时域载荷";
			break;
		case MxFunctions::LOAD_IM_FREQDOMAIN:
			typeName = "频域载荷";
			break;
		case MxFunctions::LOAD_IM_COMBINED:
			typeName = "组合载荷";
			break;		
		case MxFunctions::LOAD_IM_BOLT:
			typeName = "螺栓载荷";
			break;		
		case MxFunctions::LOAD_IM_ACOCONCFORCE:
			typeName = "声学载荷";
			break;		
		case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
			typeName = "声学频域载荷";
			break;
		case MxFunctions::LOAD_IM_SPECTRUM:
			typeName = "谱载荷";
			break;		
		case MxFunctions::LOAD_IM_SURFACE:
			typeName = "表面载荷";
			break;		
		case MxFunctions::LOAD_IM_EDGE:
			typeName = "边载荷";
			break;
		case MxFunctions::BOUNDARY_IM_FIXED:
			typeName = "固定约束";
			break;
		case MxFunctions::BOUNDARY_IM_TEMP:
			typeName = "温度边界";
			break;
		case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
			 typeName = "位移/转角";
			break;
		case MxFunctions::BOUNDARY_IM_SPEED:
			 typeName = "速度";
			break;
		case MxFunctions::BOUNDARY_IM_ACC:
			typeName = "加速度";
			break;
		case MxFunctions::BOUNDARY_MOVE:
			 typeName = "位移边界";
			break;
		case MxFunctions::BOUNDARY_TEMP_NEW:
			 typeName = "温度边界";
			break;
		case MxFunctions::BOUNDARY_IM_VIRTUAL:
			 typeName = "虚拟约束";
			break;
		case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
			typeName = "超单元边界";
			break;
		case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
			typeName = "旋转体力";
			break;
		default:
			typeName = QString();
			break;
		}
		return typeName;
	}

	int mLoadBoundaryManager::getIDByLoadBoundaryName(QString name)
	{
		for (auto& it : _loadBoundaryData)
		{
			if (it.second && it.second->getLoadBoundaryName() == name)
			{
				return it.first;
			}
		}
		return -1;
	}

	void mLoadBoundaryManager::AddLoadBoundaryData(std::shared_ptr<mLoadBoundaryBase> base)
	{
		_loadBoundaryData.insert({ base->getLoadBoundaryId(), base });
	}

	bool mLoadBoundaryManager::ExistLoadBoundaryName(const QString& name)
	{
		for (auto it : _loadBoundaryData)
		{
			if (it.second != nullptr && it.second->getLoadBoundaryName() == name)
			{
				return true;
			}
		}

		return false;
	}

	void mLoadBoundaryManager::resetName(const QString& oldName, const QString& newName)
	{
		for (auto it : _loadBoundaryData)
		{
			if (it.second != nullptr && it.second->getLoadBoundaryName() == oldName)
			{
				it.second->setLoadBoundaryName(newName);
				break;
			}
		}
	}

	int mLoadBoundaryManager::copyLoadBoundaryData(const int& id)
	{
		std::shared_ptr<mLoadBoundaryBase> loadBoundary = getLoadBoundaryData(id);
		if (loadBoundary == nullptr)
			return 0;

		QString copyName = loadBoundary->getLoadBoundaryName() + "-copy";
		while (ExistLoadBoundaryName(copyName))
		{
			copyName = copyName + "-copy";
		}

		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary = loadBoundary->copyLoadBoundary(createID(), copyName);
		AddLoadBoundaryData(copyLoadBoundary);
		return copyLoadBoundary->getLoadBoundaryId();
	}

	void mLoadBoundaryManager::deleteAllLoad() //删除所有载荷，需要根据类型筛选，todo
	{
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;
			if (
				it.second->getLoadBoundaryType() == LOAD_IM_FORCE ||
				it.second->getLoadBoundaryType() == LOAD_IM_GRAVITY ||
				it.second->getLoadBoundaryType() == LOAD_IM_TORQUE ||
				it.second->getLoadBoundaryType() == LOAD_IM_PRESSURE ||
				it.second->getLoadBoundaryType() == LOAD_IM_F1 ||
				it.second->getLoadBoundaryType() == LOAD_IM_F ||
				it.second->getLoadBoundaryType() == LOAD_IM_T ||
				it.second->getLoadBoundaryType() == LOAD_IM_D ||
				it.second->getLoadBoundaryType() == LOAD_IM_S ||
				it.second->getLoadBoundaryType() == LOAD_IM_ACSRCE ||
				it.second->getLoadBoundaryType() == LOAD_IM_HEATFLUX ||
				it.second->getLoadBoundaryType() == LOAD_IM_RADIATION ||
				it.second->getLoadBoundaryType() == LOAD_IM_CONVECTION ||
				it.second->getLoadBoundaryType() == LOAD_IM_TRANSIENT ||
				it.second->getLoadBoundaryType() == LOAD_IM_FREQDOMAIN ||
				it.second->getLoadBoundaryType() == LOAD_IM_COMBINED||
				it.second->getLoadBoundaryType() == LOAD_IM_BOLT||
				it.second->getLoadBoundaryType() == LOAD_IM_ACOCONCFORCE ||
				it.second->getLoadBoundaryType() == LOAD_IM_ACOFREQDOMAIN||
				it.second->getLoadBoundaryType() == LOAD_IM_COMBINED ||
				it.second->getLoadBoundaryType() == LOAD_IM_SPECTRUM|| 
				it.second->getLoadBoundaryType() == LOAD_ROTATE_PHYSICAL_POWER||
				it.second->getLoadBoundaryType() == LOAD_IM_SURFACE||
				it.second->getLoadBoundaryType() == LOAD_IM_EDGE
				)
			{
				_loadBoundaryData.erase(it.first);
			}
		}
	}

	void mLoadBoundaryManager::deleteAllBoundary()  //删除所有边界，需要根据类型筛选，todo
	{
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;
			if (
				it.second->getLoadBoundaryType() == BOUNDARY_IM_FIXED ||
				it.second->getLoadBoundaryType() == BOUNDARY_IM_TEMP ||
				it.second->getLoadBoundaryType() == BOUNDARY_IM_DISPLACEMENT ||
				it.second->getLoadBoundaryType() == BOUNDARY_MOVE ||
				it.second->getLoadBoundaryType() == BOUNDARY_TEMP_NEW ||
				it.second->getLoadBoundaryType() == BOUNDARY_IM_FORWARD ||
				it.second->getLoadBoundaryType() == BOUNDARY_IM_VIRTUAL||
				it.second->getLoadBoundaryType() == BOUNDARY_IM_SUPERELEMENT
				)
			{
				_loadBoundaryData.erase(it.first);
			}
		}
	}

	void mLoadBoundaryManager::deleteLoadBoundary(const int& id) //根据id删除载荷边界，无需区分
	{
		if (_loadBoundaryData.find(id) != _loadBoundaryData.cend())
		{
			_loadBoundaryData.erase(id);
		}
	}

	std::vector<int> mLoadBoundaryManager::getAllLoadID() //获取所有载荷id，需要根据类型区分，todo
	{
		std::vector<int> keys;
		for (auto pair : _loadBoundaryData)
		{
			if (pair.second->isLoadOrBoundary() == 0)
			{
				keys.push_back(pair.first);
			}
		}
		return keys;
	}

	std::vector<int> mLoadBoundaryManager::getAllBoundaryID() //获取所有边界id，需要根据类型区分，todo
	{
		std::vector<int> keys;
		for (auto pair : _loadBoundaryData)
		{
			if (pair.second->isLoadOrBoundary() == 1)
			{
				keys.push_back(pair.first);
			}
		}
		return keys;
	}

	std::vector<int> mLoadBoundaryManager::getAllLoadBoundaryID() //获取所有id，无需区分
	{
		std::vector<int> keys;
		for (auto pair : _loadBoundaryData)
		{
			keys.push_back(pair.first);
		}
		return keys;
	}

	QStringList mLoadBoundaryManager::getAllLoadNames() //获取所有载荷名称，需要根据类型区分，todo
	{
		QStringList names;
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->isLoadOrBoundary() == 0)
			{
				names.append(it.second->getLoadBoundaryName());
			}
		}
		return names;
	}

	QStringList mLoadBoundaryManager::getAllBoundaryNames() //获取所有边界名称，需要根据类型区分，todo
	{
		QStringList names;
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->isLoadOrBoundary() == 1)
			{
				names.append(it.second->getLoadBoundaryName());
			}
		}
		return names;
	}

	QStringList mLoadBoundaryManager::getAllLoadBoundaryName() //获取所有载荷边界名称，无需区分
	{
		QStringList names;
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;

			names.append(it.second->getLoadBoundaryName());
		}
		return names;
	}

	QStringList mLoadBoundaryManager::getAllLoadBoundaryNamesByTypes(MxFunctions::LoadBoundaryType type)
	{
		QStringList names;
		for (auto it : _loadBoundaryData)
		{
			if (it.second == nullptr)
				continue;
			if (it.second->getLoadBoundaryType() == type)
			{
				names.append(it.second->getLoadBoundaryName());
			}
		}
		return names;
	}
	int mLoadBoundaryManager::createID()
	{
		_maxID += 1;
		return _maxID;
	}

	void mLoadBoundaryManager::responseUnitChanged(const int& unit, mUnitSystemManager* unitsystemmanager) //载荷独有，需根据类型区分，todo
	{
		//for (auto& it : _loadBoundaryData)
		//{
		//	it.second->responseUnitChanged(unit, unitsystemmanager);
		//}
	}

	void mLoadBoundaryManager::differentiateLoadBoundary(const std::set<int>& loadBcIDs, std::vector<int>& load, std::vector<int>& boundary)
	{
		for (int loadBcID : loadBcIDs)
		{
			if (_loadBoundaryData.find(loadBcID) == _loadBoundaryData.cend())
				continue;

			if (_loadBoundaryData[loadBcID]->isLoadOrBoundary() == 0)
			{
				load.push_back(loadBcID);
			}
			else if (_loadBoundaryData[loadBcID]->isLoadOrBoundary() == 1)
			{
				boundary.push_back(loadBcID);
			}
		}
	}

	void mLoadBoundaryManager::differentiateLoadFlag(const std::set<int>& loadIDs, std::vector<int>& mechanicalLoad, std::vector<int>& dynamicLoad, bool onlyUnsuppressed)
	{
		for (int loadID : loadIDs)
		{
			if (_loadBoundaryData.find(loadID) == _loadBoundaryData.cend())
				continue;

			if (onlyUnsuppressed && _loadBoundaryData[loadID]->isSuppressed())
				continue;

			switch (_loadBoundaryData[loadID]->getLoadBoundaryType())
			{
			case MxFunctions::LOAD_IM_FORCE:
			case MxFunctions::LOAD_IM_GRAVITY:
			case MxFunctions::LOAD_IM_TORQUE:
			case MxFunctions::LOAD_IM_PRESSURE:
			case MxFunctions::LOAD_IM_HEATFLUX:
			case MxFunctions::LOAD_IM_RADIATION:
			case MxFunctions::LOAD_IM_CONVECTION:
			case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
			case MxFunctions::LOAD_IM_SURFACE:
			case MxFunctions::LOAD_IM_EDGE:
				mechanicalLoad.push_back(loadID);
				break;
			case MxFunctions::LOAD_IM_TRANSIENT:
			case MxFunctions::LOAD_IM_FREQDOMAIN:
			case MxFunctions::LOAD_IM_COMBINED:
			case MxFunctions::LOAD_IM_ACOCONCFORCE:
			case MxFunctions::LOAD_IM_ACOFREQDOMAIN:			
				dynamicLoad.push_back(loadID);
				break;
			case MxFunctions::LOAD_IM_BOLT: //螺栓载荷类型不是螺栓锁止时 放入载荷
			{
				auto data = dynamic_cast<MechCaseData::mLoadBolt*>(_loadBoundaryData[loadID].get());
				if (data->way_ != 2)
				{
					mechanicalLoad.push_back(loadID);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void mLoadBoundaryManager::differentiateBoundaryFlag(const std::set<int>& bcDs, std::vector<int>& spc, std::vector<int>& suport1, bool onlyUnsuppressed)
	{
		for (int bcID : bcDs)
		{
			if (_loadBoundaryData.find(bcID) == _loadBoundaryData.cend())
				continue;

			if (onlyUnsuppressed && _loadBoundaryData[bcID]->isSuppressed())
				continue;

			switch (_loadBoundaryData[bcID]->getLoadBoundaryType())
			{
			case MxFunctions::BOUNDARY_IM_VIRTUAL:
				suport1.push_back(bcID);
				break;
			case MxFunctions::BOUNDARY_IM_FIXED:
			case MxFunctions::BOUNDARY_IM_TEMP:
			case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
			case MxFunctions::BOUNDARY_IM_SPEED:
			case MxFunctions::BOUNDARY_IM_ACC:
			case MxFunctions::BOUNDARY_MOVE:
			case MxFunctions::BOUNDARY_TEMP_NEW:
			case MxFunctions::BOUNDARY_IM_FORWARD:
			case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
				spc.push_back(bcID);
				break;
			case MxFunctions::LOAD_IM_BOLT: //螺栓载荷类型是螺栓锁止时 放入边界
			{
				auto data = dynamic_cast<MechCaseData::mLoadBolt*>(_loadBoundaryData[bcID].get());
				if (data->way_ == 2)
				{
					spc.push_back(bcID);
				}
				break;
			}
			default:
				break;
			}
		}
	}
	bool mLoadBoundaryManager::isExistGenericBoundary()
	{
		for (auto load : _loadBoundaryData)
		{
			LoadBoundaryType type = load.second->getLoadBoundaryType();
			if (type == BOUNDARY_IM_SUPERELEMENT)
				return true;
		}

		return false;
	}
}