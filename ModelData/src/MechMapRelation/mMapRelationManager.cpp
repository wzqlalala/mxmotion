#include "mMapRelationManager.h"

#include <QSet>
#include <algorithm>

using namespace MxFunctions;
using namespace std;

namespace MechMapRelation
{
	mMapRelationManager::mMapRelationManager()
	{
	}

	mMapRelationManager::~mMapRelationManager()
	{
	}

	void mMapRelationManager::appendMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, MxFunctions::MapRelationType type, double scale)
	{
		int phyID = id1.second;
		int resultID = id2.second;
		switch (id1.first)
		{
		case Physical_LoadBoundary:
			appendLoadBoundary(phyID, resultID, id2.first, type, scale);
			break;
		case Physical_Property:
			appendProperty(phyID, resultID, id2.first, type);
			break;
		case Physical_Connector:
			appendConnector(phyID, resultID, id2.first, type);
			break;
		case Physical_Part:
			appendPart(phyID, resultID, id2.first);
			break;
		case Physical_Contact:
			appendContact(phyID, resultID, id2.first, type);
			break;
		case Physical_Case:
			appendCase(phyID, resultID, id2.first, type);
			break;
		case Physical_OptimationGoal:
			appendOptimationGoal(phyID, resultID, id2.first);
			break;
		case Physical_OptimationDesign:
			appendOptimationDesign(phyID, resultID, id2.first);
			break;
		case Physical_OptimationConstraint:
			appendOptimationConstraint(phyID, resultID, id2.first);
			break;
		case Physical_Laminate:
			appendLaminate(phyID, resultID, id2.first);
			break;
		case Physical_ModalContri:
			appendModalContri(phyID, resultID, id2.first, type);
			break;
		case Physical_NodeContri:
			appendNodeContri(phyID, resultID, id2.first, type);
			break;
		case Physical_TPContri:
			appendTPContri(phyID, resultID, id2.first, type);
			break;
		case Physical_PanelContri:
			appendPanelContri(phyID, resultID, id2.first, type);
			break;
		case Physical_Deputy:
			appendDeputy(phyID, resultID, id2.first, type);
			break;
		case Physical_PredefinedField:
			appendPredefinedField(phyID, resultID, id2.first);
			break;
		case Physical_Coordinate:
			appendCoordinate(phyID, resultID, id2.first);
			break;
		case Physical_Field:
			appendField(phyID, resultID, id2.first);
			break;
		case Physical_PeriodicBoundary:
			appendPeriodicBoundary(phyID, resultID, id2.first, type);
			break;
		case Physical_ModelFix:
			appendModelFix(phyID, resultID, id2.first, type);
			break;
		case Physical_Microconstituent:
			appendMicroconstituent(phyID, resultID, id2.first, type);
			break;
		case Physical_RotateAnalyse:
			appendRotateAnalyse(phyID, resultID, id2.first);
			break;
		default:
			//多体实体引用走通用映射存储
			if (isMotionSource(id1.first))
				appendMotionRef(id1.first, phyID, id2.first, type, resultID);
			break;
		}

	}

	void mMapRelationManager::appendTableMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row, MxFunctions::MapRelationType type)
	{
		int phyID = id1.second;
		int resultID = id2.second;
		switch (id1.first)
		{
		case Physical_Property:
		{
			if (id2.first == Physical_Material)
			{
				appendTableMapByRow(_propertyToListMat, id1.first, phyID, resultID, _matMap, row);
			}
			else if (id2.first == Physical_PropertyShape)
			{
				appendTableMapByRow(_propertyToListShape, id1.first, phyID, resultID, _propertyShapeMap, row);
			}

			break;
		}
		case Physical_LoadBoundary:
		{
			if (id2.first == Physical_LoadBoundary)
			{
				appendTableMapByRow(_loadToListLoad, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_Function)
			{
				appendTableMapByRow(_loadBcToListFunction, id1.first, phyID, resultID, _functionMap, row);
			}
			break;
		}
		case Physical_Case:
		{
			if (type == MapRelation_TableCase1 || type == MapRelation_TableCase2 || type == MapRelation_TableFunction)
			{
				QList<mRandomCaseMappingData> datas;
				if (_RandomCase.find(phyID) != _RandomCase.cend())
				{
					datas = _RandomCase[phyID];
				}

				mRandomCaseMappingData data;
				switch (type)
				{
				case MxFunctions::MapRelation_TableCase1:
					data.case1 = resultID;
					break;
				case MxFunctions::MapRelation_TableCase2:
					data.case2 = resultID;
					break;
				case MxFunctions::MapRelation_TableFunction:
					data.function = resultID;
					break;
				default:
					break;
				}

				if (datas.size() > row)
				{
					datas.insert(row, data);
				}
				else
				{
					datas.append(data);
				}
				_RandomCase[phyID] = datas;

				if (resultID > 0)
				{
					unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> set;
					switch (type)
					{
					case MxFunctions::MapRelation_TableCase1:
					case MxFunctions::MapRelation_TableCase2:
					{					
						if (_caseMap.find(resultID) != _caseMap.cend())
						{
							set = _caseMap[resultID];
						}
						set[Physical_Case].insert(phyID);
						_caseMap[resultID] = set;
						break;
					}
					case MxFunctions::MapRelation_TableFunction:
					{
						if (_functionMap.find(resultID) != _functionMap.cend())
						{
							set = _functionMap[resultID];
						}
						set[Physical_Case].insert(phyID);
						_functionMap[resultID] = set;
						break;
					}
					default:
						break;
					}
				}
			}
			else if (id2.first == Physical_LoadBoundary)
			{
				appendTableMapByRow(_caseToListLoadBC, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_FreqSamplPoint)
			{
				appendTableMapByRow(_caseToListFreq, id1.first, phyID, resultID, _freqMap, row);
			}
			else if (id2.first == Physical_NonStructuralQuality)
			{
				appendTableMapByRow(_caseToListNSM, id1.first, phyID, resultID, _nsmMap, row);
			}
			else if (id2.first == Physical_ModelFix)
			{
				appendTableMapByRow(_caseToListModelFix, id1.first, phyID, resultID, _modelFixMap, row);
			}
			else if (id2.first == Physical_PeakFilter)
			{
				appendTableMapByRow(_caseToListPeakFilter, id1.first, phyID, resultID, _peakFilterMap, row);
			}
			break;
		}
		case Physical_Laminate:
		{
			if (id2.first == Physical_Laminate)
			{
				appendTableMapByRow(_laminateToLayerList, id1.first, phyID, resultID, _laminateMap, row);
			}
			break;
		}
		case Physical_Set:
		{
			if (id2.first == Physical_Set)
			{
				appendTableMapByRow(_setToListSet, id1.first, phyID, resultID, _setMap, row);
			}
			break;
		}
		case Physical_GenericPart:
		{
			if (id2.first == Physical_Part)
			{
				appendTableMapByRow(_genericPartToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		case Physical_Deputy:
		{
			if (id2.first == Physical_Part)
			{
				appendTableMapByRow(_deputyToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id)
	{
		int phyID = id.second;
		switch (id.first)
		{
		case Physical_LoadBoundary:
			deleteLoadBoundary(phyID);
			break;
		case Physical_Case:
			deleteCase(phyID);
			break;
		case Physical_Property:
			deleteProperty(phyID);
			break;
		case Physical_Part:
			deletePart(phyID);
			break;
		case Physical_Material:
			deleteMaterial(phyID);
			break;
		case Physical_Function:
			deleteFunction(phyID);
			break;
		case Physical_Coordinate:
			deleteCoordinate(phyID);
			break;
		case Physical_Contact:
			deleteContact(phyID);
			break;
		case Physical_Connector:
			deleteConnector(phyID);
			break;
		case Physical_Set:
			deleteSet(phyID);
			break;
		case Physical_PropertyShape:
			deletePropertyShape(phyID);
			break;
		case Physical_OptimationGoal:
			deleteOptimationGoal(phyID);
			break;
		case Physical_OptimationDesign:
			deleteOptimationDesign(phyID);
			break;
		case Physical_OptimationConstraint:
			deleteOptimationConstraint(phyID);
			break;
		case Physical_FreqSamplPoint:
			deleteFreq(phyID);
			break;
		case Physical_Laminate:
			deleteLaminate(phyID);
			break;
		case Physical_ModalContri:
			deleteModalContri(phyID);
			break;
		case Physical_NodeContri:
			deleteNodeContri(phyID);
			break;
		case Physical_TPContri:
			deleteTPContri(phyID);
			break;
		case Physical_PanelContri:
			deletePanelContri(phyID);
			break;
		case Physical_Deputy:
			deleteDeputy(phyID);
			break;
		case Physical_PeriodicBoundary:
			deletePeriodicBoundary(phyID);
			break;
		case Physical_ModelFix:
			deleteModelFix(phyID);
			break;
		case Physical_NonStructuralQuality:
			deleteNSM(phyID);
			break;
		case Physical_PeakFilter:
			deletePeakFilter(phyID);
			break;
		case Physical_GenericParam:
			deleteGenericParam(phyID);
			break;
		case Physical_GenericPart:
			deleteGenericPart(phyID);
			break;
		case Physical_PredefinedField:
			deletePredefinedField(phyID);
			break;
		case Physical_Field:
			deleteDiscreteField(phyID);
			break;
		case Physical_Microconstituent:
			deleteMicroconstituent(phyID);
			break;
		case Physical_RotateAnalyse:
			deleteRotateAnalyse(phyID);
			break;
		default:
			//多体实体引用：删除该源实体的全部引用
			if (isMotionSource(id.first))
				deleteMotionRefs(id.first, phyID);
			break;
		}

	}

	void mMapRelationManager::deleteMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> phyData, std::pair<MxFunctions::PhysicalFieldType, int> resultData)
	{
		int phyID = phyData.second;
		int resultID = resultData.second;
		PhysicalFieldType type = resultData.first;
		switch (phyData.first)
		{
		case Physical_LoadBoundary:
		{
			if (type == Physical_Set)
			{
				if (_setMap.find(resultID) == _setMap.cend())
					return;
				_setMap[resultID][Physical_LoadBoundary].erase(phyID);
				_loadBcToSet.erase(phyID);
			}
			break;
		}
		case Physical_Contact:
		{
			if (type == Physical_Set)
			{
				if (_setMap.find(resultID) == _setMap.cend())
					return;
				_setMap[resultID][Physical_Contact].erase(phyID);
				if (_contactToSet.find(phyID) != _contactToSet.cend())
				{
					if (_contactToSet[phyID].master == resultID)
						_contactToSet[phyID].master = -1;
					else if (_contactToSet[phyID].slave == resultID)
						_contactToSet[phyID].slave = -1;
				}
			}
			break;
		}
		case Physical_OptimationDesign:
		{
			if (type == Physical_OptimationDesign)
			{
				for (auto it = _designToDesign.begin(); it != _designToDesign.end(); ) {
					if (it->second == phyID) {
						it = _designToDesign.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
			break;
		}
		case Physical_Case:
		{
			if (type == Physical_LoadBoundary)
			{
				if (_loadBcToCase.find(resultID) == _loadBcToCase.cend())
					return;
				if (_caseMap.find(phyID) != _caseMap.cend())
				{
					_caseMap[phyID][Physical_LoadBoundary].erase(resultID);
				}
				_loadBcToCase[resultID].erase(phyID);
				if (_loadBcToCase[resultID].empty())
				{
					_loadBcToCase.erase(resultID);
				}
			}
			else if (type == Physical_PredefinedField)
			{
				if (_predefinedFieldToCase.find(resultID) == _predefinedFieldToCase.cend())
					return;
				if (_caseMap.find(phyID) != _caseMap.cend())
				{
					_caseMap[phyID][Physical_PredefinedField].erase(resultID);
				}
				_predefinedFieldToCase[resultID].erase(phyID);
				if (_predefinedFieldToCase[resultID].empty())
				{
					_predefinedFieldToCase.erase(resultID);
				}
			}
			else if (type == Physical_Contact)
			{
				if (_contactToCase.find(resultID) == _contactToCase.cend())
					return;
				if (_caseMap.find(phyID) != _caseMap.cend())
				{
					_caseMap[phyID][Physical_Contact].erase(resultID);
				}
				_contactToCase[resultID].erase(phyID);
				if (_contactToCase[resultID].empty())
				{
					_contactToCase.erase(resultID);
				}
			}
		}
		default:
			//多体实体引用：删除源实体下全部角色中指向该目标的引用
			if (isMotionSource(phyData.first))
			{
				auto itType = _motionRefMap.find(phyData.first);
				if (itType != _motionRefMap.cend())
				{
					auto itSrc = itType->second.find(phyID);
					if (itSrc != itType->second.cend())
					{
						for (auto& rolePair : itSrc->second)
							rolePair.second.erase(resultID);
					}
				}
			}
			break;
		}

	}

	void mMapRelationManager::updateTabelMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row, MxFunctions::MapRelationType type)
	{
		int phyID = id1.second;
		int resultID = id2.second;
		//更新后，反映射未移除
		switch (id1.first)
		{
		case Physical_Property:
		{
			if (id2.first == Physical_Material)
			{
				updateTableMapByRow(_propertyToListMat, id1.first, phyID, resultID, _matMap, row);
			}
			else if (id2.first == Physical_PropertyShape)
			{
				updateTableMapByRow(_propertyToListShape, id1.first, phyID, resultID, _propertyShapeMap, row);
			}
			break;
		}
		case Physical_LoadBoundary:
		{
			if (id2.first == Physical_LoadBoundary)
			{
				updateTableMapByRow(_loadToListLoad, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_Function)
			{
				updateTableMapByRow(_loadBcToListFunction, id1.first, phyID, resultID, _functionMap, row);
			}
			break;
		}
		case Physical_Laminate:
		{
			if (id2.first == Physical_Laminate)
			{
				updateTableMapByRow(_laminateToLayerList, id1.first, phyID, resultID, _laminateMap, row);
			}
			break;
		}
		case Physical_Set:
		{
			if (id2.first == Physical_Set)
			{
				updateTableMapByRow(_setToListSet, id1.first, phyID, resultID, _setMap, row);
			}
			break;
		}
		case Physical_GenericPart:
		{
			if (id2.first == Physical_Part)
			{
				updateTableMapByRow(_genericPartToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		case Physical_Deputy:
		{
			if (id2.first == Physical_Part)
			{
				updateTableMapByRow(_deputyToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		case Physical_Case:
		{
			if (type == MapRelation_TableCase1 || type == MapRelation_TableCase2 || type == MapRelation_TableFunction)
			{
				if (_RandomCase.find(phyID) == _RandomCase.cend())
					return;

				if (_RandomCase[phyID].size() > row)
				{
					switch (type)
					{
					case MxFunctions::MapRelation_TableCase1:
						_RandomCase[phyID][row].case1 = resultID;
						break;
					case MxFunctions::MapRelation_TableCase2:
						_RandomCase[phyID][row].case2 = resultID;
						break;
					case MxFunctions::MapRelation_TableFunction:
						_RandomCase[phyID][row].function = resultID;
						break;
					default:
						break;
					}
				}

				if (resultID > 0)
				{
					unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> set;
					switch (type)
					{
					case MxFunctions::MapRelation_TableCase1:
					case MxFunctions::MapRelation_TableCase2:
					{
						if (_caseMap.find(resultID) != _caseMap.cend())
						{
							set = _caseMap[resultID];
						}
						set[Physical_Case].insert(phyID);
						_caseMap[resultID] = set;
						break;
					}
					case MxFunctions::MapRelation_TableFunction:
					{
						if (_functionMap.find(resultID) != _functionMap.cend())
						{
							set = _functionMap[resultID];
						}
						set[Physical_Case].insert(phyID);
						_functionMap[resultID] = set;
						break;
					}
					default:
						break;
					}
				}
			}
			else if (id2.first == Physical_LoadBoundary)
			{
				updateTableMapByRow(_caseToListLoadBC, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_FreqSamplPoint)
			{
				updateTableMapByRow(_caseToListFreq, id1.first, phyID, resultID, _freqMap, row);
			}
			else if (id2.first == Physical_NonStructuralQuality)
			{
				updateTableMapByRow(_caseToListNSM, id1.first, phyID, resultID, _nsmMap, row);
			}
			else if (id2.first == Physical_ModelFix)
			{
				updateTableMapByRow(_caseToListModelFix, id1.first, phyID, resultID, _modelFixMap, row);
			}
			else if (id2.first == Physical_PeakFilter)
			{
				updateTableMapByRow(_caseToListPeakFilter, id1.first, phyID, resultID, _peakFilterMap, row);
			}
			break;
		}
		default:
			break;
		}

	}

	void mMapRelationManager::deleteTabelMapRelation(std::pair<MxFunctions::PhysicalFieldType, int> id1, std::pair<MxFunctions::PhysicalFieldType, int> id2, int row)
	{
		int phyID = id1.second;
		int resultID = id2.second;
		switch (id1.first)
		{
		case Physical_Property:
		{
			if (id2.first == Physical_Material)
			{
				deleteTableMapByRow(_propertyToListMat, id1.first, phyID, resultID, _matMap, row);
			}
			else if (id2.first == Physical_PropertyShape)
			{
				deleteTableMapByRow(_propertyToListShape, id1.first, phyID, resultID, _propertyShapeMap, row);
			}
			break;
		}
		case Physical_LoadBoundary:
		{
			if (id2.first == Physical_LoadBoundary)
			{
				deleteTableMapByRow(_loadToListLoad, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_Function)
			{
				deleteTableMapByRow(_loadBcToListFunction, id1.first, phyID, resultID, _functionMap, row);
			}
			break;
		}
		case Physical_Case:
		{
			if (id2.first == Physical_Case)
			{
				if (_RandomCase.find(phyID) == _RandomCase.cend())
					return;

				if (_RandomCase[phyID].size() > row)
				{
					_RandomCase[phyID].removeAt(row);
				}

				//暂不移除
				//if (resultID > 0 && !_RandomCase[phyID].contains(resultID))
				//{
				//	if (_loadBcMap.find(resultID) != _loadBcMap.cend())
				//	{
				//		_loadBcMap[resultID][Physical_Property].erase(phyID);
				//	}
				//}
			}
			else if (id2.first == Physical_LoadBoundary)
			{
				deleteTableMapByRow(_caseToListLoadBC, id1.first, phyID, resultID, _loadBcMap, row);
			}
			else if (id2.first == Physical_FreqSamplPoint)
			{
				deleteTableMapByRow(_caseToListFreq, id1.first, phyID, resultID, _freqMap, row);
			}
			else if (id2.first == Physical_NonStructuralQuality)
			{
				deleteTableMapByRow(_caseToListNSM, id1.first, phyID, resultID, _nsmMap, row);
			}
			else if (id2.first == Physical_ModelFix)
			{
				deleteTableMapByRow(_caseToListModelFix, id1.first, phyID, resultID, _modelFixMap, row);
			}
			else if (id2.first == Physical_PeakFilter)
			{
				deleteTableMapByRow(_caseToListPeakFilter, id1.first, phyID, resultID, _peakFilterMap, row);
			}
			break;
		}
		case Physical_Laminate:
		{
			if (id2.first == Physical_Laminate)
			{
				deleteTableMapByRow(_laminateToLayerList, id1.first, phyID, resultID, _laminateMap, row);
			}
			break;
		}
		case Physical_GenericPart:
		{
			if (id2.first == Physical_Part)
			{
				deleteTableMapByRow(_genericPartToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		case Physical_Set:
		{
			if (id2.first == Physical_Set)
			{
				deleteTableMapByRow(_setToListSet, id1.first, phyID, resultID, _setMap, row);
			}
			break;
		}
		case Physical_Deputy:
		{
			if (id2.first == Physical_Part)
			{
				deleteTableMapByRow(_deputyToListPart, id1.first, phyID, resultID, _partMap, row);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::setScaleByCase(std::pair<MxFunctions::PhysicalFieldType, int> phyData, int caseID, double scale)
	{
		int phyID = phyData.second;
		if (phyData.first == Physical_LoadBoundary)
		{
			if (_loadBcToCase.find(phyID) != _loadBcToCase.cend() && _loadBcToCase[phyID].find(caseID) != _loadBcToCase[phyID].cend())
				_loadBcToCase[phyID][caseID] = scale;
		}
	}

	double mMapRelationManager::getScaleByCase(std::pair<MxFunctions::PhysicalFieldType, int> phyData, int caseID)
	{
		int phyID = phyData.second;
		if (phyData.first == Physical_LoadBoundary)
		{
			if (_loadBcToCase.find(phyID) != _loadBcToCase.cend() && _loadBcToCase[phyID].find(caseID) != _loadBcToCase[phyID].cend())
				return _loadBcToCase[phyID][caseID];
		}

		return 1.0;
	}

	std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> mMapRelationManager::getCoordinateMap(int coorID)
	{
		if(_coordinateMap.find(coorID) != _coordinateMap.cend())
		{
			auto& innerMap = _coordinateMap[coorID];
			std::vector<MxFunctions::PhysicalFieldType> emptyKeys;
			for (auto& it : innerMap)
			{
				it.second.erase(-1);
				if (it.second.empty())
				{
					emptyKeys.push_back(it.first);
				}
			}

			for (auto key : emptyKeys)
			{
				innerMap.erase(key);
			}

			if (innerMap.empty())
			{
				_coordinateMap.erase(coorID);
				return std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>();
			}

			return innerMap;
		}

		return std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>();
	}

	std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> mMapRelationManager::getPhysicalFieldMap(MxFunctions::PhysicalFieldType type, int phyID)
	{
		std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>> _map;
		switch (type)
		{
		case MxFunctions::Physical_Part:
			_map = _partMap;
			break;
		case MxFunctions::Physical_Material:
			_map = _matMap;
			break;
		case MxFunctions::Physical_Case:
			_map = _caseMap;
			break;
		case MxFunctions::Physical_Set:
			_map = _setMap;
			break;
		case MxFunctions::Physical_Coordinate:
			_map = _coordinateMap;
			break;
		case MxFunctions::Physical_Function:
			_map = _functionMap;
			break;
		case MxFunctions::Physical_Property:
			_map = _propertyMap;
			break;
		default:
			break;
		}

		if (_map.find(phyID) != _map.cend())
		{
			return _map[phyID];
		}

		return std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>();
	}

	void mMapRelationManager::appendTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type, int phyID,
		int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map, int row)
	{
		QList<int> layer;
		if (mapList.find(phyID) != mapList.cend())
		{
			layer = mapList[phyID];
		}

		if (layer.size() > row)
		{
			layer.insert(row, resultID);
		}
		else
		{
			layer.append(resultID);
		}
		mapList[phyID] = layer;

		if (resultID > 0)
		{
			unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> set;
			if (map.find(resultID) != map.cend())
			{
				set = map[resultID];
			}
			set[type].insert(phyID);
			map[resultID] = set;
		}
	}

	void mMapRelationManager::updateTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type, int phyID,
		int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map, int row)
	{
		if (mapList.find(phyID) == mapList.cend())
			return;

		if (mapList[phyID].size() > row)
		{
			mapList[phyID][row] = resultID;
		}

		if (resultID > 0)
		{
			unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> set;
			if (map.find(resultID) != map.cend())
			{
				set = map[resultID];
			}
			set[type].insert(phyID);
			map[resultID] = set;
		}
	}

	void mMapRelationManager::deleteTableMapByRow(std::unordered_map<int, QList<int>>& mapList, MxFunctions::PhysicalFieldType type, int phyID, 
		int resultID, std::unordered_map<int, std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>>& map, int row)
	{
		if (mapList.find(phyID) == mapList.cend())
			return;

		if (mapList[phyID].size() > row)
		{
			resultID = mapList[phyID][row];
			mapList[phyID].removeAt(row);
		}

		if (resultID > 0 && !mapList[phyID].contains(resultID))
		{
			if (map.find(resultID) != map.cend())
			{
				map[resultID][type].erase(phyID);
			}
		}
	}


	void mMapRelationManager::appendProperty(int propertyID, int matID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		int oldID;
		switch (phyType)
		{
		case MxFunctions::Physical_Material:
		{
			if (_propertyToMat.find(propertyID) != _propertyToMat.cend())
			{
				oldID = _propertyToMat[propertyID];
				if (_matMap.find(oldID) != _matMap.cend())
				{
					_matMap[oldID][Physical_Property].erase(propertyID);
				}
			}
			_propertyToMat[propertyID] = matID;
			if (matID > 0)
			{
				_matMap[matID][Physical_Property].insert(propertyID);
			}
			break;
		}
		case MxFunctions::Physical_PropertyShape:
		{
			if (_propertyToShape.find(propertyID) != _propertyToShape.cend())
			{
				oldID = _propertyToShape[propertyID];
				if (_propertyShapeMap.find(oldID) != _propertyShapeMap.cend())
				{
					_propertyShapeMap[oldID][Physical_Property].erase(propertyID);
				}
			}
			_propertyToShape[propertyID] = matID;
			if (matID > 0)
			{
				_propertyShapeMap[matID][Physical_Property].insert(propertyID);
			}
			break;
		}
		case MxFunctions::Physical_Coordinate:
		{
			if (_propertyToCoordinate.find(propertyID) != _propertyToCoordinate.cend())
			{
				oldID = _propertyToCoordinate[propertyID];
				if (_coordinateMap.find(oldID) != _coordinateMap.cend())
				{
					_coordinateMap[oldID][Physical_Property].erase(propertyID);
				}
			}
			_propertyToCoordinate[propertyID] = matID;
			if (matID > 0)
			{
				_coordinateMap[matID][Physical_Property].insert(propertyID);
			}
			break;
		}
		case MxFunctions::Physical_Laminate:
		{
			if (_propertyToLaminate.find(propertyID) != _propertyToLaminate.cend())
			{
				oldID = _propertyToLaminate[propertyID];
				if (_laminateMap.find(oldID) != _laminateMap.cend())
				{
					_laminateMap[oldID][Physical_Property].erase(propertyID);
				}
			}
			_propertyToLaminate[propertyID] = matID;
			if (matID > 0)
			{
				_laminateMap[matID][Physical_Property].insert(propertyID);
			}
			break;
		}
		case MxFunctions::Physical_Field:
		{
			if (_propertyToDiscreteField.find(propertyID) != _propertyToDiscreteField.cend())
			{
				if (type == MapRelation_ThickField)
				{
					oldID = _propertyToDiscreteField[propertyID].thickField;
				}
				else if (type == MapRelation_RotationField)
				{
					oldID = _propertyToDiscreteField[propertyID].rotationField;
				}

				if (_discreteFieldMap.find(oldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[oldID][Physical_Property].erase(propertyID);
				}
			}

			if (type == MapRelation_ThickField)
			{
				_propertyToDiscreteField[propertyID].thickField = matID;
			}
			else if (type == MapRelation_RotationField)
			{
				_propertyToDiscreteField[propertyID].rotationField = matID;
			}

			if (matID > 0)
			{
				_discreteFieldMap[matID][Physical_Property].insert(propertyID);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteProperty(int propertyID)
	{
		//删除材料
		if (_propertyToMat.find(propertyID) != _propertyToMat.cend())
		{
			int matID = _propertyToMat[propertyID];
			if (_matMap.find(matID) != _matMap.cend())
			{
				_matMap[matID][Physical_Property].erase(propertyID);
			}
			_propertyToMat.erase(propertyID);
		}

		if (_propertyToListMat.find(propertyID) != _propertyToListMat.cend())
		{
			QSet<int> matID = _propertyToListMat[propertyID].toSet();
			for (const auto& it : matID)
			{
				if (_matMap.find(it) != _matMap.cend())
				{
					_matMap[it][Physical_Property].erase(propertyID);
				}
			}
			_propertyToListMat.erase(propertyID);
		}

		//删除截面类型
		if (_propertyToShape.find(propertyID) != _propertyToShape.cend())
		{
			int shapeID = _propertyToShape[propertyID];
			if (_propertyShapeMap.find(shapeID) != _propertyShapeMap.cend())
			{
				_propertyShapeMap[shapeID][Physical_Property].erase(propertyID);
			}
			_propertyToShape.erase(propertyID);
		}

		if (_propertyToListShape.find(propertyID) != _propertyToListShape.cend())
		{
			QSet<int> shapeID = _propertyToListShape[propertyID].toSet();
			for (const auto& it : shapeID)
			{
				if (_propertyShapeMap.find(it) != _propertyShapeMap.cend())
				{
					_propertyShapeMap[it][Physical_Property].erase(propertyID);
				}
			}
			_propertyToListShape.erase(propertyID);
		}

		//删除坐标系
		if (_propertyToCoordinate.find(propertyID) != _propertyToCoordinate.cend())
		{
			int coordinateID = _propertyToCoordinate[propertyID];
			if (_coordinateMap.find(coordinateID) != _coordinateMap.cend())
			{
				_coordinateMap[coordinateID][Physical_Property].erase(propertyID);
			}
			_propertyToCoordinate.erase(propertyID);
		}

		//删除层合板
		if (_propertyToLaminate.find(propertyID) != _propertyToLaminate.cend())
		{
			int laminateID = _propertyToLaminate[propertyID];
			if (_laminateMap.find(laminateID) != _laminateMap.cend())
			{
				_laminateMap[laminateID][Physical_Property].erase(propertyID);
			}
			_propertyToLaminate.erase(propertyID);
		}

		//删除离散场
		if (_propertyToDiscreteField.find(propertyID) != _propertyToDiscreteField.cend())
		{
			if (_discreteFieldMap.find(_propertyToDiscreteField[propertyID].rotationField) != _discreteFieldMap.cend())
			{
				_discreteFieldMap[_propertyToDiscreteField[propertyID].rotationField][Physical_Property].erase(propertyID);
			}
			if (_discreteFieldMap.find(_propertyToDiscreteField[propertyID].thickField) != _discreteFieldMap.cend())
			{
				_discreteFieldMap[_propertyToDiscreteField[propertyID].thickField][Physical_Property].erase(propertyID);
			}
			_propertyToDiscreteField.erase(propertyID);
		}

		//删除部件
		if (_propertyMap.find(propertyID) != _propertyMap.cend())
		{
			std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> map = _propertyMap[propertyID];
			if (map.find(Physical_Part) != map.cend())
			{
				for (const auto& partID : map[Physical_Part])
				{
					if (_partToProperty.find(partID) != _partToProperty.cend())
					{
						_partToProperty.erase(partID);
						deleteDataMap[Physical_Part].insert(partID);
					}
				}
			}
			if (map.find(Physical_OptimationDesign) != map.cend())
			{
				for (const auto& designID : map[Physical_OptimationDesign])
				{
					if (_designToProperty.find(designID) != _designToProperty.cend())
					{
						_designToProperty.erase(designID);
						deleteDataMap[Physical_OptimationDesign].insert(designID);
					}
				}
			}
			if (map.find(Physical_OptimationConstraint) != map.cend())
			{
				for (const auto& constraintID : map[Physical_OptimationConstraint])
				{
					if (_constraintToProperty.find(constraintID) != _constraintToProperty.cend())
					{
						_constraintToProperty.erase(constraintID);
						deleteDataMap[Physical_OptimationConstraint].insert(constraintID);
					}
				}
			}
			_propertyMap.erase(propertyID);
		}
	}

	int mMapRelationManager::getPropertyMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		if (phyType == MxFunctions::Physical_Material)
		{
			if (_propertyToMat.find(phyID) == _propertyToMat.cend())
				return -1;

			return _propertyToMat[phyID];
		}
		else if (phyType == MxFunctions::Physical_PropertyShape)
		{
			if (_propertyToShape.find(phyID) == _propertyToShape.cend())
				return -1;

			return _propertyToShape[phyID];
		}
		else if (phyType == MxFunctions::Physical_Coordinate)
		{
			if (_propertyToCoordinate.find(phyID) == _propertyToCoordinate.cend())
				return -1;

			return _propertyToCoordinate[phyID];
		}
		else if (phyType == MxFunctions::Physical_Laminate)
		{
			if (_propertyToLaminate.find(phyID) == _propertyToLaminate.cend())
				return -1;

			return _propertyToLaminate[phyID];
		}
		else if (phyType == MxFunctions::Physical_Field)
		{
			if (_propertyToDiscreteField.find(phyID) == _propertyToDiscreteField.cend())
				return -1;

			if (type == MxFunctions::MapRelation_ThickField)
			{
				return _propertyToDiscreteField[phyID].thickField;
			}
			else if (type == MxFunctions::MapRelation_RotationField)
			{
				return _propertyToDiscreteField[phyID].rotationField;
			}
		}

		return -1;
	}

	void mMapRelationManager::appendPart(int partID, int resultID, MxFunctions::PhysicalFieldType phyType)
	{
		int oldID;
		if (phyType == Physical_Property)
		{
			if (_partToProperty.find(partID) != _partToProperty.cend())
			{
				oldID = _partToProperty[partID];
				if (_propertyMap.find(oldID) != _propertyMap.cend())
				{
					_propertyMap[oldID][Physical_Part].erase(partID);
				}
			}
			_partToProperty[partID] = resultID;
			if (resultID > 0)
			{
				_propertyMap[resultID][Physical_Part].insert(partID);
			}
		}

	}

	void mMapRelationManager::deletePart(int phyID)
	{
		if (_partToProperty.find(phyID) != _partToProperty.cend())
		{
			int propertyID = _partToProperty[phyID];
			if (_propertyMap.find(propertyID) != _propertyMap.cend())
			{
				_propertyMap[propertyID][Physical_Part].erase(phyID);
			}
			_partToProperty.erase(phyID);
		}

		if (_partMap.find(phyID) != _partMap.cend())
		{
			for (auto& it : _partMap[phyID][Physical_GenericPart])
			{
				if (_genericPartToListPart.find(it) != _genericPartToListPart.cend())
				{
					_genericPartToListPart[it].removeAll(phyID);
				}
			}
			for (auto& it : _partMap[phyID][Physical_Deputy])
			{
				if (_deputyToListPart.find(it) != _deputyToListPart.cend())
				{
					_deputyToListPart[it].removeAll(phyID);
				}
			}
			for (auto& it : _partMap[phyID][Physical_RotateAnalyse])
			{
				if (_rotateToPart.find(it) != _rotateToPart.cend())
				{
					_rotateToPart.erase(it);
					deleteDataMap[Physical_RotateAnalyse].insert(it);
				}
			}
			_partMap.erase(phyID);
		}
	}

	void mMapRelationManager::appendLoadBoundary(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type, double scale)
	{
		int oldID;
		if (phyType == Physical_Set)
		{
			if (_loadBcToSet.find(phyID) != _loadBcToSet.cend())
			{
				oldID = _loadBcToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToSet[phyID] = resultID;
			if (resultID > 0)
			{
				_setMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_Coordinate)
		{
			if (_loadBcToCoordinate.find(phyID) != _loadBcToCoordinate.cend())
			{
				oldID = _loadBcToCoordinate[phyID];
				if (_coordinateMap.find(oldID) != _coordinateMap.cend())
				{
					_coordinateMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToCoordinate[phyID] = resultID;
			if (resultID > 0)
			{
				_coordinateMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_Function)
		{
			if (_loadBcToFunction.find(phyID) != _loadBcToFunction.cend())
			{

				switch (type)
				{
				case MxFunctions::MapRelation_convFunction:
					oldID = _loadBcToFunction[phyID].conv_function;
					break;
				case MxFunctions::MapRelation_AmplitudeFunction:
					oldID = _loadBcToFunction[phyID].amplitude_function;
					break;
				case MxFunctions::MapRelation_PhaseFunction:
					oldID = _loadBcToFunction[phyID].phase_function;
					break;
				default:
					oldID = _loadBcToFunction[phyID].default_function;
					break;
				}

				if (_functionMap.find(oldID) != _functionMap.cend())
				{
					_functionMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}

			switch (type)
			{
			case MxFunctions::MapRelation_convFunction:
				_loadBcToFunction[phyID].conv_function = resultID;
				break;
			case MxFunctions::MapRelation_AmplitudeFunction:
				_loadBcToFunction[phyID].amplitude_function = resultID;
				break;
			case MxFunctions::MapRelation_PhaseFunction:
				_loadBcToFunction[phyID].phase_function = resultID;
				break;
			default:
				_loadBcToFunction[phyID].default_function = resultID;
				break;
			}

			if (resultID > 0)
			{
				_functionMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_Case)
		{
			_loadBcToCase[phyID][resultID] = scale;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_LoadBoundary)
		{
			if (_loadBcToLoadBC.find(phyID) != _loadBcToLoadBC.cend())
			{
				oldID = _loadBcToLoadBC[phyID];
				if (_loadBcMap.find(oldID) != _loadBcMap.cend())
				{
					_loadBcMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToLoadBC[phyID] = resultID;
			if (resultID > 0)
			{
				_loadBcMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_Connector)
		{
			if (_loadBcToConnector.find(phyID) != _loadBcToConnector.cend())
			{
				oldID = _loadBcToConnector[phyID];
				if (_connectorMap.find(oldID) != _connectorMap.cend())
				{
					_connectorMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToConnector[phyID] = resultID;
			if (resultID > 0)
			{
				_connectorMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
		else if (phyType == Physical_Field)
		{
			if (_loadBcToAnalysisField.find(phyID) != _loadBcToAnalysisField.cend())
			{
				oldID = (type == MapRelation_convAnalysisField) ? _loadBcToAnalysisField[phyID].conv_field : _loadBcToAnalysisField[phyID].field;
				if (_discreteFieldMap.find(oldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[oldID][Physical_LoadBoundary].erase(phyID);
				}
			}

			if (type == MapRelation_convAnalysisField)
				_loadBcToAnalysisField[phyID].conv_field = resultID;
			else
				_loadBcToAnalysisField[phyID].field = resultID;

			if (resultID > 0)
			{
				_discreteFieldMap[resultID][Physical_LoadBoundary].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteLoadBoundary(int phyID)
	{
		//删除集合
		if (_loadBcToSet.find(phyID) != _loadBcToSet.cend())
		{
			int setID = _loadBcToSet[phyID];
			if (_setMap.find(setID) != _setMap.cend())
			{
				_setMap[setID][Physical_LoadBoundary].erase(phyID);
			}
			_loadBcToSet.erase(phyID);
		}

		//删除坐标系
		if (_loadBcToCoordinate.find(phyID) != _loadBcToCoordinate.cend())
		{
			int coordID = _loadBcToCoordinate[phyID];
			if (_coordinateMap.find(coordID) != _coordinateMap.cend())
			{
				_coordinateMap[coordID][Physical_LoadBoundary].erase(phyID);
			}
			_loadBcToCoordinate.erase(phyID);
		}

		//删除连接
		if (_loadBcToConnector.find(phyID) != _loadBcToConnector.cend())
		{
			int connectID = _loadBcToConnector[phyID];
			if (_connectorMap.find(connectID) != _connectorMap.cend())
			{
				_connectorMap[connectID][Physical_LoadBoundary].erase(phyID);
			}
			_loadBcToConnector.erase(phyID);
		}

		//删除分析场
		if (_loadBcToAnalysisField.find(phyID) != _loadBcToAnalysisField.cend())
		{
			if (_loadBcToAnalysisField[phyID].field > 0)
			{
				int fieldID = _loadBcToAnalysisField[phyID].field;
				if (_discreteFieldMap.find(fieldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[fieldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			if (_loadBcToAnalysisField[phyID].conv_field > 0)
			{
				int convFieldID = _loadBcToAnalysisField[phyID].conv_field;
				if (_discreteFieldMap.find(convFieldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[convFieldID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToAnalysisField.erase(phyID);
		}

		//删除函数
		if (_loadBcToFunction.find(phyID) != _loadBcToFunction.cend())
		{
			vector<int> functionIDs;
			functionIDs.reserve(4);
			functionIDs.push_back(_loadBcToFunction[phyID].default_function);
			functionIDs.push_back(_loadBcToFunction[phyID].conv_function);
			functionIDs.push_back(_loadBcToFunction[phyID].amplitude_function);
			functionIDs.push_back(_loadBcToFunction[phyID].phase_function);

			for (int functionID : functionIDs)
			{
				if (functionID != -1)
				{
					_functionMap[functionID][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToFunction.erase(phyID);
		}

		//删除分析工况
		if (_loadBcToCase.find(phyID) != _loadBcToCase.cend())
		{
			for (auto& it : _loadBcToCase[phyID])
			{
				if (_caseMap.find(it.first) != _caseMap.cend())
				{
					_caseMap[it.first][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToCase.erase(phyID);
		}


		//删除载荷边界
		if (_loadBcToLoadBC.find(phyID) != _loadBcToLoadBC.cend())
		{
			int ID = _loadBcToLoadBC[phyID];
			if (_loadBcMap.find(ID) != _loadBcMap.cend())
			{
				_loadBcMap[ID][Physical_LoadBoundary].erase(phyID);
			}
			_loadBcToLoadBC.erase(phyID);
		}

		if (_loadToListLoad.find(phyID) != _loadToListLoad.cend())
		{
			QSet<int> loadID = _loadToListLoad[phyID].toSet();
			for (const auto& it : loadID)
			{
				if (_loadBcMap.find(it) != _loadBcMap.cend())
				{
					_loadBcMap[it][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadToListLoad.erase(phyID);
		}

		if (_loadBcToListFunction.find(phyID) != _loadBcToListFunction.cend())
		{
			QSet<int> functionID = _loadBcToListFunction[phyID].toSet();
			for (const auto& it : functionID)
			{
				if (_functionMap.find(it) != _functionMap.cend())
				{
					_functionMap[it][Physical_LoadBoundary].erase(phyID);
				}
			}
			_loadBcToListFunction.erase(phyID);
		}

		//删除载荷边界映射
		if (_loadBcMap.find(phyID) != _loadBcMap.cend())
		{
			//删除载荷边界
			for (auto& it : _loadBcMap[phyID][Physical_LoadBoundary])
			{
				if (_loadBcToLoadBC.find(it) != _loadBcToLoadBC.cend())
				{
					deleteDataMap[Physical_LoadBoundary].insert(it);
					_loadBcToLoadBC.erase(it);
				}

				if (_loadToListLoad.find(it) != _loadToListLoad.cend())
				{
					std::replace(_loadToListLoad[it].begin(), _loadToListLoad[it].end(), phyID, -1);
				}
			}

			//删除分析
			for (auto& it : _loadBcMap[phyID][Physical_Case])
			{
				if (_caseToListLoadBC.find(it) != _caseToListLoadBC.cend())
				{
					std::replace(_caseToListLoadBC[it].begin(), _caseToListLoadBC[it].end(), phyID, -1);
				}
			}

			_loadBcMap.erase(phyID);
		}
	}

	int mMapRelationManager::getLoadMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		int resultID = -1;
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_loadBcToSet.find(phyID) == _loadBcToSet.cend())
				return -1;

			resultID = _loadBcToSet[phyID];
		}
		else if (phyType == MxFunctions::Physical_Coordinate)
		{
			if (_loadBcToCoordinate.find(phyID) == _loadBcToCoordinate.cend())
				return -1;

			resultID = _loadBcToCoordinate[phyID];
		}
		else if (phyType == MxFunctions::Physical_Function)
		{
			if (_loadBcToFunction.find(phyID) == _loadBcToFunction.cend())
				return -1;

			switch (type)
			{
			case MxFunctions::MapRelation_convFunction:
				resultID = _loadBcToFunction[phyID].conv_function;
				break;
			case MxFunctions::MapRelation_AmplitudeFunction:
				resultID = _loadBcToFunction[phyID].amplitude_function;
				break;
			case MxFunctions::MapRelation_PhaseFunction:
				resultID = _loadBcToFunction[phyID].phase_function;
				break;
			default:
				resultID = _loadBcToFunction[phyID].default_function;
				break;
			}
		}
		else if (phyType == MxFunctions::Physical_LoadBoundary)
		{
			if (_loadBcToLoadBC.find(phyID) == _loadBcToLoadBC.cend())
				return -1;

			resultID = _loadBcToLoadBC[phyID];
		}
		else if (phyType == MxFunctions::Physical_Connector)
		{
			if (_loadBcToConnector.find(phyID) == _loadBcToConnector.cend())
				return -1;

			resultID = _loadBcToConnector[phyID];
		}
		else if (phyType == MxFunctions::Physical_Field)
		{
			if (_loadBcToAnalysisField.find(phyID) == _loadBcToAnalysisField.cend())
				return -1;

			switch (type)
			{
			case MxFunctions::MapRelation_convAnalysisField:
				resultID = _loadBcToAnalysisField[phyID].conv_field;
				break;
			default:
				resultID = _loadBcToAnalysisField[phyID].field;
				break;
			}
		}

		return resultID;
	}

	void mMapRelationManager::deleteMaterial(int phyID)
	{
		if (_matMap.find(phyID) != _matMap.cend())
		{
			std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> map = _matMap[phyID];
			if (map.find(Physical_Property) != map.cend())
			{
				for (const auto& proID : map[Physical_Property])
				{
					if (_propertyToMat.find(proID) != _propertyToMat.cend())
					{
						_propertyToMat.erase(proID);
						deleteDataMap[Physical_Property].insert(proID);
					}

					if (_propertyToListMat.find(proID) != _propertyToListMat.cend())
					{
						std::replace(_propertyToListMat[proID].begin(), _propertyToListMat[proID].end(), phyID, -1);
					}
				}
			}
			if (map.find(Physical_Connector) != map.cend())
			{
				for (const auto& proID : map[Physical_Connector])
				{
					if (_connectorToMat.find(proID) != _connectorToMat.cend())
					{
						_connectorToMat.erase(proID);
						deleteDataMap[Physical_Connector].insert(proID);
					}
				}
			}
			if (map.find(Physical_Laminate) != map.cend())
			{
				for (const auto& proID : map[Physical_Laminate])
				{
					if (_laminateToMat.find(proID) != _laminateToMat.cend())
					{
						_laminateToMat.erase(proID);
						deleteDataMap[Physical_Laminate].insert(proID);
					}
				}
			}
			_matMap.erase(phyID);
		}
	}

	void mMapRelationManager::appendContact(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		int oldID;
		if (phyType == Physical_Set)
		{
			if (_contactToSet.find(phyID) != _contactToSet.cend())
			{
				if (type == MapRelation_Master)
				{
					oldID = _contactToSet[phyID].master;
				}
				else
				{
					oldID = _contactToSet[phyID].slave;
				}

			if (_setMap.find(oldID) != _setMap.cend())
			{
				_setMap[oldID][Physical_Contact].erase(phyID);
			}
			}

			if (type == MapRelation_Master)
			{
				_contactToSet[phyID].master = resultID;
			}
			else
			{
				_contactToSet[phyID].slave = resultID;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_Contact].insert(phyID);
			}
		}
		else if (phyType == Physical_Contact)
		{
			if (_contactToContact.find(phyID) != _contactToContact.cend())
			{
				oldID = _contactToContact[phyID];
				if (_contactMap.find(oldID) != _contactMap.cend())
				{
					_contactMap[oldID][Physical_Contact].erase(phyID);
				}
			}

			_contactToContact[phyID] = resultID;
			if (resultID > 0)
			{
				_contactMap[resultID][Physical_Contact].insert(phyID);
			}
		}
		else if (phyType == Physical_Case)
		{
			_contactToCase[phyID][resultID] = 1.0;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_Contact].insert(phyID);
			}
		}
		else if (phyType == Physical_Function)
		{
			if (_contactToFunction.find(phyID) != _contactToFunction.cend())
			{
				oldID = _contactToFunction[phyID];
				if (_functionMap.find(oldID) != _functionMap.cend())
				{
					_functionMap[oldID][Physical_Contact].erase(phyID);
				}
			}
			_contactToFunction[phyID] = resultID;
			if (resultID > 0)
			{
				_functionMap[resultID][Physical_Contact].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteContact(int phyID)
	{
		//删除集合
		if (_contactToSet.find(phyID) != _contactToSet.cend())
		{
			if (_setMap.find(_contactToSet[phyID].master) != _setMap.cend())
			{
				_setMap[_contactToSet[phyID].master][Physical_Contact].erase(phyID);
			}
			if (_setMap.find(_contactToSet[phyID].slave) != _setMap.cend())
			{
				_setMap[_contactToSet[phyID].slave][Physical_Contact].erase(phyID);
			}
			_contactToSet.erase(phyID);
		}

		//删除接触
		if (_contactToContact.find(phyID) != _contactToContact.cend())
		{
			if (_contactMap.find(_contactToContact[phyID]) != _contactMap.cend())
			{
				_contactMap[_contactToContact[phyID]][Physical_Contact].erase(phyID);
			}
			_contactToContact.erase(phyID);
		}

		//删除分析映射
		if (_contactToCase.find(phyID) != _contactToCase.cend())
		{
			for (auto& it : _contactToCase[phyID])
			{
				if (_caseMap.find(it.first) != _caseMap.cend())
				{
					_caseMap[it.first][Physical_Contact].erase(phyID);
				}
			}
			_contactToCase.erase(phyID);
		}

		//删除函数
		if (_contactToFunction.find(phyID) != _contactToFunction.cend())
		{
			int resultID = _contactToFunction[phyID];
			if (_functionMap.find(resultID) != _functionMap.cend())
			{
				_functionMap[resultID][Physical_Contact].erase(phyID);
			}
			_contactToFunction.erase(phyID);
		}

		//删除接触映射
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _contactMap[phyID];
		for (auto& it : map)
		{
			set<int> ids = it.second;
			switch (it.first)
			{
			case Physical_Contact:
				for (int id : ids)
				{
					_contactToContact.erase(id);
					deleteDataMap[Physical_Contact].insert(id);
				}
				break;
			default:
				break;
			}
		}
		_contactMap.erase(phyID);

	}

	int mMapRelationManager::getContactMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_contactToSet.find(phyID) == _contactToSet.cend())
				return -1;

			if (type == MxFunctions::MapRelation_Master)
			{
				return _contactToSet[phyID].master;
			}
			else if (type == MxFunctions::MapRelation_Slave)
			{
				return _contactToSet[phyID].slave;
			}
		}
		else if (phyType == MxFunctions::Physical_Contact)
		{
			if (_contactToContact.find(phyID) == _contactToContact.cend())
				return -1;

			return _contactToContact[phyID];
		}
		else if (phyType == MxFunctions::Physical_Function)
		{
			if (_contactToFunction.find(phyID) == _contactToFunction.cend())
				return -1;

			return _contactToFunction[phyID];
		}

		return -1;
	}

	void mMapRelationManager::appendPeriodicBoundary(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int oldID;
		if (phyType == Physical_Set)
		{
			if (_periodicBoundaryToSet.find(phyID) != _periodicBoundaryToSet.cend())
			{
				if (typee == MapRelation_Master)
				{
					oldID = _periodicBoundaryToSet[phyID].master;
				}
				else
				{
					oldID = _periodicBoundaryToSet[phyID].slave;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_PeriodicBoundary].erase(phyID);
				}
			}

			if (typee == MapRelation_Master)
			{
				_periodicBoundaryToSet[phyID].master = resultID;
			}
			else
			{
				_periodicBoundaryToSet[phyID].slave = resultID;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_PeriodicBoundary].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deletePeriodicBoundary(int phyID)
	{
		//删除集合
		if (_periodicBoundaryToSet.find(phyID) != _periodicBoundaryToSet.cend())
		{
			if (_setMap.find(_periodicBoundaryToSet[phyID].master) != _setMap.cend())
			{
				_setMap[_periodicBoundaryToSet[phyID].master][Physical_PeriodicBoundary].erase(phyID);
			}
			if (_setMap.find(_periodicBoundaryToSet[phyID].slave) != _setMap.cend())
			{
				_setMap[_periodicBoundaryToSet[phyID].slave][Physical_PeriodicBoundary].erase(phyID);
			}
			_periodicBoundaryToSet.erase(phyID);
		}
	}

	int mMapRelationManager::getPeriodicBoundaryMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_periodicBoundaryToSet.find(phyID) == _periodicBoundaryToSet.cend())
				return -1;

			if (typee == MxFunctions::MapRelation_Master)
			{
				return _periodicBoundaryToSet[phyID].master;
			}
			else if (typee == MxFunctions::MapRelation_Slave)
			{
				return _periodicBoundaryToSet[phyID].slave;
			}
		}

		return -1;
	}

	void mMapRelationManager::appendModelFix(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == Physical_Set)
		{
			if (_modelFixToSet.find(phyID) != _modelFixToSet.cend())
			{
				int oldID = _modelFixToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_ModelFix].erase(phyID);
				}
			}

			_modelFixToSet[phyID] = resultID;

			if (resultID > 0)
			{
				_setMap[resultID][Physical_ModelFix].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteModelFix(int phyID)
	{
		//删除集合
		if (_modelFixToSet.find(phyID) != _modelFixToSet.cend())
		{
			int oldID = _modelFixToSet[phyID];
			if (_setMap.find(oldID) != _setMap.cend())
			{
				_setMap[oldID][Physical_ModelFix].erase(phyID);
			}
			_modelFixToSet.erase(phyID);
		}
	}

	int mMapRelationManager::getModelFixMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_modelFixToSet.find(phyID) == _modelFixToSet.cend())
				return -1;

			return _modelFixToSet[phyID];
		}

		return -1;
	}

	void mMapRelationManager::deleteSet(int phyID)
	{
		//删除集合
		if (_setToListSet.find(phyID) != _setToListSet.cend())
		{
			_setToListSet.erase(phyID);
		}

		if (_setMap.find(phyID) == _setMap.cend())
			return;

		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _setMap[phyID];
		for (auto& it : map)
		{
			set<int> ids = it.second;
			switch (it.first)
			{
			case Physical_LoadBoundary:
				for (int id : ids)
				{
					_loadBcToSet.erase(id);
					deleteDataMap[Physical_LoadBoundary].insert(id);
				}
				break;
			case Physical_Contact:
				for (int id : ids)
				{
					if (_contactToSet.find(id) == _contactToSet.cend())
						continue;
					if (_contactToSet[id].master == phyID)
					{
						_contactToSet[id].master = -1;
					}
					else if (_contactToSet[id].slave == phyID)
					{
						_contactToSet[id].slave = -1;
					}
					else
					{
						continue;
					}
					deleteDataMap[Physical_Contact].insert(id);
				}
				break;
			case Physical_Connector:
				for (int id : ids)
				{
					if (_connectorToSet.find(id) == _connectorToSet.cend())
						continue;
					if (_connectorToSet[id].defaultSet == phyID) _connectorToSet[id].defaultSet = -1;
					else if (_connectorToSet[id].master == phyID) _connectorToSet[id].master = -1;
					else if (_connectorToSet[id].slave == phyID) _connectorToSet[id].slave = -1;
					else if (_connectorToSet[id].nodeSet == phyID) _connectorToSet[id].nodeSet = -1;
					else continue;
					deleteDataMap[Physical_Connector].insert(id);
				}
				break;
			case Physical_Case:
				for (int id : ids)
				{
					if (_caseToSet.find(id) == _caseToSet.cend())
						continue;
					if (_caseToSet[id].history_set == phyID)
					{
						_caseToSet[id].history_set = -1;
					}
					if (_caseToSet[id].elforce_set == phyID)
					{
						_caseToSet[id].elforce_set = -1;
					}
					if (_caseToSet[id].gpforce_set == phyID)
					{
						_caseToSet[id].gpforce_set = -1;
					}
				}
				break;
			case Physical_Laminate:
				for (int id : ids)
				{
					_laminateToSet.erase(id);
					deleteDataMap[Physical_Laminate].insert(id);
				}
				break;
			case Physical_OptimationDesign:
				for (int id : ids)
				{
					if (_designToSet.find(id) != _designToSet.cend())
					{
						_designToSet.erase(id);
						deleteDataMap[Physical_OptimationDesign].insert(id);
					}
				}
				break;
			case Physical_ModalContri:
				for (int id : ids)
				{
					if (_modalContriToSet.find(id) == _modalContriToSet.cend())
						continue;
					if (_modalContriToSet[id].outSet == phyID)
					{
						_modalContriToSet[id].outSet = -1;
					}
					if (_modalContriToSet[id].frequencySet == phyID)
					{
						_modalContriToSet[id].frequencySet = -1;
					}
				}
				break;
			case Physical_NodeContri:
				for (int id : ids)
				{
					if (_nodeContriToSet.find(id) == _nodeContriToSet.cend())
						continue;
					if (_nodeContriToSet[id].gridSet == phyID)
					{
						_nodeContriToSet[id].gridSet = -1;
					}
					if (_nodeContriToSet[id].gridfSet == phyID)
					{
						_nodeContriToSet[id].gridfSet = -1;
					}
					if (_nodeContriToSet[id].responseGridSet == phyID)
					{
						_nodeContriToSet[id].responseGridSet = -1;
					}
					if (_nodeContriToSet[id].frequencySet == phyID)
					{
						_nodeContriToSet[id].frequencySet = -1;
					}
				}
				break;
			case Physical_TPContri:
				for (int id : ids)
				{
					if (_tpContriToSet.find(id) == _tpContriToSet.cend())
						continue;
					if (_tpContriToSet[id].responseGridSet == phyID)
					{
						_tpContriToSet[id].responseGridSet = -1;
					}
					if (_tpContriToSet[id].conptSet == phyID)
					{
						_tpContriToSet[id].conptSet = -1;
					}
					if (_tpContriToSet[id].conelSet == phyID)
					{
						_tpContriToSet[id].conelSet = -1;
					}
					if (_tpContriToSet[id].conrelSet == phyID)
					{
						_tpContriToSet[id].conrelSet = -1;
					}
					if (_tpContriToSet[id].convolSet == phyID)
					{
						_tpContriToSet[id].convolSet = -1;
					}
				}
				break;
			case Physical_PanelContri:
				for (int id : ids)
				{
					if (_panelContriToSet.find(id) == _panelContriToSet.cend())
						continue;
					if (_panelContriToSet[id].frequencySet == phyID)
					{
						_panelContriToSet[id].frequencySet = -1;
					}
					if (_panelContriToSet[id].responseGridSet == phyID)
					{
						_panelContriToSet[id].responseGridSet = -1;
					}
				}
				break;
			case Physical_Set:
				for (int id : ids)
				{
					if (_setToListSet.find(id) != _setToListSet.cend())
					{
						_setToListSet[id].removeAll(phyID);
					}
				}
				break;
			case Physical_PredefinedField:
				for (int id : ids)
				{
					if (_predefinedFieldToSet.find(id) != _predefinedFieldToSet.cend())
					{
						_predefinedFieldToSet.erase(id);
						deleteDataMap[Physical_PredefinedField].insert(id);
					}
				}
				break;
			case Physical_PeriodicBoundary:
				for (int id : ids)
				{
					if (_periodicBoundaryToSet.find(id) == _periodicBoundaryToSet.cend())
						continue;
					if (_periodicBoundaryToSet[id].master == phyID)
					{
						_periodicBoundaryToSet[id].master = -1;
					}
					if (_periodicBoundaryToSet[id].slave == phyID)
					{
						_periodicBoundaryToSet[id].slave = -1;
					}
					deleteDataMap[Physical_PeriodicBoundary].insert(id);
				}
				break;
				case Physical_ModelFix:
				for (int id : ids)
				{
					if (_modelFixToSet.find(id) == _modelFixToSet.cend())
						continue;
					if (_modelFixToSet[id] == phyID)
					{
						_modelFixToSet[id] = -1;
					}
					deleteDataMap[Physical_ModelFix].insert(id);
				}
				break;
			case Physical_Field:
				for (int id : ids)
				{
					if (_fieldToSet.find(id) != _fieldToSet.cend())
					{
						_fieldToSet[id] = -1;
					}
					deleteDataMap[Physical_Field].insert(id);
				}
				break;
			default:
				break;
			}
		}
		_setMap.erase(phyID);
	}

	void mMapRelationManager::deletePropertyShape(int phyID)
	{
		if (_propertyShapeMap.find(phyID) == _propertyShapeMap.cend())
			return;

		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _propertyShapeMap[phyID];
		for (auto& it : map)
		{
			set<int> ids = it.second;
			for (int id : ids)
			{
				if (_propertyToShape.find(id) != _propertyToShape.cend())
				{
					_propertyToShape.erase(id);
					deleteDataMap[Physical_Property].insert(id);
				}

				if (_propertyToListShape.find(id) != _propertyToListShape.cend())
				{
					std::replace(_propertyToListShape[id].begin(), _propertyToListShape[id].end(), phyID, -1);
				}
			}
		}
		_propertyShapeMap.erase(phyID);
	}

	void mMapRelationManager::deleteFunction(int phyID)
	{
		if (_functionMap.find(phyID) == _functionMap.cend())
			return;

		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _functionMap[phyID];
		for (auto& it : map)
		{
			set<int> ids = it.second;
			switch (it.first)
			{
		case Physical_LoadBoundary:
			for (int id : ids)
			{
				if (_loadBcToFunction.find(id) != _loadBcToFunction.cend())
				{
					if (_loadBcToFunction[id].default_function == phyID)
					{
						_loadBcToFunction[id].default_function = -1;
					}
					if (_loadBcToFunction[id].conv_function == phyID)
					{
						_loadBcToFunction[id].conv_function = -1;
					}
					if (_loadBcToFunction[id].amplitude_function == phyID)
					{
						_loadBcToFunction[id].amplitude_function = -1;
					}
					if (_loadBcToFunction[id].phase_function == phyID)
					{
						_loadBcToFunction[id].phase_function = -1;
					}
				}

				if (_loadBcToListFunction.find(id) != _loadBcToListFunction.cend())
				{
					std::replace(_loadBcToListFunction[id].begin(), _loadBcToListFunction[id].end(), phyID, -1);
				}
			}
			break;
			case Physical_PredefinedField:
				for (int id : ids)
				{
					if (_predefinedFieldToFunction.find(id) != _predefinedFieldToFunction.cend())
					{
						_predefinedFieldToFunction.erase(id);
					}
				}
				break;
			case Physical_Case:
				for (int id : ids)
				{
					if (_caseToFunction.find(id) != _caseToFunction.cend())
					{
						_caseToFunction.erase(id);
					}

					if (_RandomCase.find(id) != _RandomCase.cend())
					{
						for(int i = 0; i < _RandomCase[id].size(); i++)
						{
							if (_RandomCase[id][i].function == phyID) _RandomCase[id][i].function = -1;
						}
					}
				}
				break;
			case Physical_Contact:
				for (int id : ids)
				{
					if (_contactToFunction.find(id) != _contactToFunction.cend())
					{
						_contactToFunction.erase(id);
					}
				}
				break;
			default:
				break;
			}
		}
		_functionMap.erase(phyID);
	}

	void mMapRelationManager::appendCoordinate(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType)
	{
		int oldID;
		if (phyType == Physical_Field)
		{
			if (_coordinateToDiscreteField.find(phyID) != _coordinateToDiscreteField.cend())
			{
				oldID = _coordinateToDiscreteField[phyID];
				if (_discreteFieldMap.find(oldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[oldID][Physical_Coordinate].erase(phyID);
				}
			}
			_coordinateToDiscreteField[phyID] = resultID;
			if (resultID > 0)
			{
				_discreteFieldMap[resultID][Physical_Coordinate].insert(phyID);
			}
		}
	}

	int mMapRelationManager::getCoordinateMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		int resultID = -1;
		if (phyType == MxFunctions::Physical_Field)
		{
			if (_coordinateToDiscreteField.find(phyID) == _coordinateToDiscreteField.cend()) return -1;
			resultID = _coordinateToDiscreteField[phyID];
		}
		return resultID;
	}

	void mMapRelationManager::appendField(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType)
	{
		if (phyType == Physical_Coordinate)
		{
			if (_fieldToCoordinate.find(phyID) != _fieldToCoordinate.cend())
			{
				int oldID = _fieldToCoordinate[phyID];
				if (_coordinateMap.find(oldID) != _coordinateMap.cend())
				{
					_coordinateMap[oldID][Physical_Field].erase(phyID);
				}
			}
			_fieldToCoordinate[phyID] = resultID;
			if (resultID > 0)
			{
				_coordinateMap[resultID][Physical_Field].insert(phyID);
			}
		}
		else if (phyType == Physical_Set)
		{
			if (_fieldToSet.find(phyID) != _fieldToSet.cend())
			{
				int oldID = _fieldToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_Field].erase(phyID);
				}
			}
			_fieldToSet[phyID] = resultID;
			if (resultID > 0)
			{
				_setMap[resultID][Physical_Field].insert(phyID);
			}
		}
	}

	int mMapRelationManager::getFieldMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		if (phyType == Physical_Coordinate)
		{
			if (_fieldToCoordinate.find(phyID) == _fieldToCoordinate.cend())
				return -1;
			return _fieldToCoordinate[phyID];
		}
		if (phyType == Physical_Set)
		{
			if (_fieldToSet.find(phyID) == _fieldToSet.cend())
				return -1;
			return _fieldToSet[phyID];
		}
		return -1;
	}

	void mMapRelationManager::deleteCoordinate(int phyID)
	{
		//删除离散场映射
		if (_coordinateToDiscreteField.find(phyID) != _coordinateToDiscreteField.cend())
		{
			int resultID = _coordinateToDiscreteField[phyID];
			if (_discreteFieldMap.find(resultID) != _discreteFieldMap.cend())
			{
				_discreteFieldMap[resultID][Physical_Coordinate].erase(phyID);
			}
			_coordinateToDiscreteField.erase(phyID);
		}


		if (_coordinateMap.find(phyID) != _coordinateMap.cend())
		{
			std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _coordinateMap[phyID];
			for (auto& it : map)
			{
				set<int> ids = it.second;
				switch (it.first)
				{
				case Physical_LoadBoundary:
					for (int id : ids)
					{
						if (_loadBcToCoordinate.find(id) != _loadBcToCoordinate.cend())
						{
							_loadBcToCoordinate.erase(id);
						}
					}
					break;
				case Physical_Property:
					for (int id : ids)
					{
						if (_propertyToCoordinate.find(id) != _propertyToCoordinate.cend())
						{
							_propertyToCoordinate.erase(id);
						}
					}
					break;
				case Physical_Deputy:
					for (int id : ids)
					{
						if (_deputyToCoordinate.find(id) != _deputyToCoordinate.cend())
						{
							_deputyToCoordinate.erase(id);
						}
					}
					break;
				case Physical_Connector:
					for (int id : ids)
					{
						if (_connectorToCoordinate.find(id) != _connectorToCoordinate.cend())
						{
							if (_connectorToCoordinate[id].defaultCoord == phyID)
							{
								_connectorToCoordinate[id].defaultCoord = -1;
							}
							if (_connectorToCoordinate[id].coor2 == phyID)
							{
								_connectorToCoordinate[id].coor2 = -1;
							}
						}
					}
					break;
				case Physical_Field:
					for (int id : ids)
					{
						if (_fieldToCoordinate.find(id) != _fieldToCoordinate.cend())
						{
							_fieldToCoordinate[id] = -1;
						}
					}
					break;
				default:
					break;
				}
			}
			_coordinateMap.erase(phyID);
		}
	}

	void mMapRelationManager::deleteCase(int phyID)
	{
		if (_caseToFunction.find(phyID) != _caseToFunction.cend())
		{
			_functionMap[_caseToFunction[phyID]][Physical_Case].erase(phyID);
			_caseToFunction.erase(phyID);
		}

		if (_caseToCase.find(phyID) != _caseToCase.cend())
		{
			_caseMap[_caseToCase[phyID]][Physical_Case].erase(phyID);
			_caseToCase.erase(phyID);
		}

		if (_caseToGenericParam.find(phyID) != _caseToGenericParam.cend())
		{
			_genericParamMap[_caseToGenericParam[phyID]][Physical_Case].erase(phyID);
			_caseToGenericParam.erase(phyID);
		}

		if (_caseToSet.find(phyID) != _caseToSet.cend())
		{
			vector<int> setIDs;
			setIDs.reserve(4);
			setIDs.push_back(_caseToSet[phyID].history_set);
			setIDs.push_back(_caseToSet[phyID].elforce_set);
			setIDs.push_back(_caseToSet[phyID].gpforce_set);

			for (int setID : setIDs)
			{
				if (setID != -1)
				{
					_setMap[setID][Physical_Case].erase(phyID);
				}
			}
			_caseToSet.erase(phyID);
		}

		if (_RandomCase.find(phyID) != _RandomCase.cend())
		{
			//删除分析 函数
			for (auto& it : _RandomCase[phyID])
			{
				if (it.case1 != -1 && _caseMap.find(it.case1) != _caseMap.cend())
					_caseMap[it.case1][Physical_Case].erase(phyID);
				if (it.case2 != -1 && _caseMap.find(it.case2) != _caseMap.cend())
					_caseMap[it.case2][Physical_Case].erase(phyID);
				if (it.function != -1 && _functionMap.find(it.function) != _functionMap.cend())
					_functionMap[it.function][Physical_Case].erase(phyID);
			}
			_RandomCase.erase(phyID);
		}

		if (_caseToListLoadBC.find(phyID) != _caseToListLoadBC.cend())
		{
			for (auto& it : _caseToListLoadBC[phyID])
			{
				if (it != -1 && _loadBcMap.find(it) != _loadBcMap.cend())
				{
					_loadBcMap[it][Physical_Case].erase(phyID);
				}
			}
			_caseToListLoadBC.erase(phyID);
		}

		if (_caseToListFreq.find(phyID) != _caseToListFreq.cend())
		{
			for (auto& it : _caseToListFreq[phyID])
			{
				if (it != -1 && _freqMap.find(it) != _freqMap.cend())
				{
					_freqMap[it][Physical_Case].erase(phyID);
				}
			}
			_caseToListFreq.erase(phyID);
		}

		if (_caseToListNSM.find(phyID) != _caseToListNSM.cend())
		{
			for (auto& it : _caseToListNSM[phyID])
			{
				if (it != -1 && _nsmMap.find(it) != _nsmMap.cend())
				{
					_nsmMap[it][Physical_Case].erase(phyID);
				}
			}
			_caseToListNSM.erase(phyID);
		}

		if (_caseToListModelFix.find(phyID) != _caseToListModelFix.cend())
		{
			for (auto& it : _caseToListModelFix[phyID])
			{
				if (it != -1 && _modelFixMap.find(it) != _modelFixMap.cend())
				{
					_modelFixMap[it][Physical_Case].erase(phyID);
				}
			}
			_caseToListModelFix.erase(phyID);
		}

		if (_caseToListPeakFilter.find(phyID) != _caseToListPeakFilter.cend())
		{
			for (auto& it : _caseToListPeakFilter[phyID])
			{
				if (it != -1 && _peakFilterMap.find(it) != _peakFilterMap.cend())
				{
					_peakFilterMap[it][Physical_Case].erase(phyID);
				}
			}
			_caseToListPeakFilter.erase(phyID);
		}

		if (_caseMap.find(phyID) != _caseMap.cend())
		{
			//删除载荷
			for (auto& it : _caseMap[phyID][Physical_LoadBoundary])
			{
				if (_loadBcToCase.find(it) != _loadBcToCase.cend())
				{
					_loadBcToCase[it].erase(phyID);
					if (_loadBcToCase[it].empty())
					{
						_loadBcToCase.erase(it);
					}
				}
			}

			//删除预定义场
			for (auto& it : _caseMap[phyID][Physical_PredefinedField])
			{
				if (_predefinedFieldToCase.find(it) != _predefinedFieldToCase.cend())
				{
					_predefinedFieldToCase[it].erase(phyID);
					if (_predefinedFieldToCase[it].empty())
					{
						_predefinedFieldToCase.erase(it);
					}
				}
			}

			//删除接触
			for (auto& it : _caseMap[phyID][Physical_Contact])
			{
				if (_contactToCase.find(it) != _contactToCase.cend())
				{
					_contactToCase[it].erase(phyID);
					if (_contactToCase[it].empty())
					{
						_contactToCase.erase(it);
					}
				}
			}

			//删除转子动力
			for (auto& it : _caseMap[phyID][Physical_RotateAnalyse])
			{
				if (_rotateToCase.find(it) != _rotateToCase.cend())
				{
					_rotateToCase[it].erase(phyID);
					if (_rotateToCase[it].empty())
					{
						_rotateToCase.erase(it);
					}
				}
			}

			//删除分析
			for (auto& it : _caseMap[phyID][Physical_Case])
			{
				deleteDataMap[Physical_Case].insert(it);
				_caseToCase.erase(it);

				if (_RandomCase.find(it) != _RandomCase.end())
				{
					for (int i = 0; i < _RandomCase[it].size(); i++)
					{
						if (_RandomCase[it][i].case1 == phyID) _RandomCase[it][i].case1 = -1;
						if (_RandomCase[it][i].case2 == phyID) _RandomCase[it][i].case2 = -1;
					}
				}
			}

			//删除优化目标
			for (auto& it : _caseMap[phyID][Physical_OptimationGoal])
			{
				if (_goalToCase.find(it) != _goalToCase.cend())
				{
					_goalToCase.erase(it);
					deleteDataMap[Physical_OptimationGoal].insert(it);
				}
			}

			//删除优化约束
			for (auto& it : _caseMap[phyID][Physical_OptimationConstraint])
			{
				if (_constraintToCase.find(it) != _constraintToCase.cend())
				{
					_constraintToCase.erase(it);
					deleteDataMap[Physical_OptimationConstraint].insert(it);
				}
			}

			_caseMap.erase(phyID);
		}
	}

	void mMapRelationManager::appendCase(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType type)
	{
		int oldID;
		if (phyType == Physical_Function)
		{
			if (_caseToFunction.find(phyID) != _caseToFunction.cend())
			{
				oldID = _caseToFunction[phyID];
				if (_functionMap.find(oldID) != _functionMap.cend())
				{
					_functionMap[oldID][Physical_Case].erase(phyID);
				}
			}
			_caseToFunction[phyID] = resultID;
			if (resultID > 0)
			{
				_functionMap[resultID][Physical_Case].insert(phyID);
			}
		}
		else if (phyType == Physical_Case)
		{
			if (_caseToCase.find(phyID) != _caseToCase.cend())
			{
				oldID = _caseToCase[phyID];
				if (_caseMap.find(oldID) != _caseMap.cend())
				{
					_caseMap[oldID][Physical_Case].erase(phyID);
				}
			}
			_caseToCase[phyID] = resultID;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_Case].insert(phyID);
			}
		}
		else if (phyType == Physical_GenericParam)
		{
			if (_caseToGenericParam.find(phyID) != _caseToGenericParam.cend())
			{
				oldID = _caseToGenericParam[phyID];
				if (_genericParamMap.find(oldID) != _genericParamMap.cend())
				{
					_genericParamMap[oldID][Physical_Case].erase(phyID);
				}
			}
			_caseToGenericParam[phyID] = resultID;
			if (resultID > 0)
			{
				_genericParamMap[resultID][Physical_Case].insert(phyID);
			}
		}
		else if (phyType == Physical_Set)
		{
			if (_caseToSet.find(phyID) != _caseToSet.cend())
			{
				switch (type)
				{
				case MxFunctions::MapRelation_HistorySet:
					oldID = _caseToSet[phyID].history_set;
					break;
				case MxFunctions::MapRelation_ElforceSet:
					oldID = _caseToSet[phyID].elforce_set;
					break;
				case MxFunctions::MapRelation_GpforceSet:
					oldID = _caseToSet[phyID].gpforce_set;
					break;
				default:
					return;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_Case].erase(phyID);
				}
			}

			switch (type)
			{
			case MxFunctions::MapRelation_HistorySet:
				_caseToSet[phyID].history_set = resultID;
				break;
			case MxFunctions::MapRelation_ElforceSet:
				_caseToSet[phyID].elforce_set = resultID;
				break;
			case MxFunctions::MapRelation_GpforceSet:
				_caseToSet[phyID].gpforce_set = resultID;
				break;
			default:
				return;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_Case].insert(phyID);
			}
		}
	}

	QList<int> mMapRelationManager::getCaseListMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		QList<int> list;
		if (typee == MapRelation_TableCase1 || typee == MapRelation_TableCase2 || typee == MapRelation_TableFunction)
		{
			if (_RandomCase.find(phyID) == _RandomCase.cend())
				return list;

			switch (typee)
			{
			case MxFunctions::MapRelation_TableCase1:
			{
				for (auto& it : _RandomCase[phyID])
				{
					list.append(it.case1);
				}
				break;
			}
			case MxFunctions::MapRelation_TableCase2:
			{
				for (auto& it : _RandomCase[phyID])
				{
					list.append(it.case2);
				}
				break;
			}
			case MxFunctions::MapRelation_TableFunction:
			{
				for (auto& it : _RandomCase[phyID])
				{
					list.append(it.function);
				}
				break;
			}
			default:
				break;
			}
		}
		else if (phyType == Physical_LoadBoundary)
		{
			if (_caseToListLoadBC.find(phyID) != _caseToListLoadBC.cend())
			{
				return _caseToListLoadBC[phyID];
			}
		}
		else if (phyType == Physical_FreqSamplPoint)
		{
			if (_caseToListFreq.find(phyID) != _caseToListFreq.cend())
			{
				return _caseToListFreq[phyID];
			}
		}
		else if (phyType == Physical_NonStructuralQuality)
		{
			if (_caseToListNSM.find(phyID) != _caseToListNSM.cend())
			{
				return _caseToListNSM[phyID];
			}
		}
		else if (phyType == Physical_ModelFix)
		{
			if (_caseToListModelFix.find(phyID) != _caseToListModelFix.cend())
			{
				return _caseToListModelFix[phyID];
			}
		}
		else if (phyType == Physical_PeakFilter)
		{
			if (_caseToListPeakFilter.find(phyID) != _caseToListPeakFilter.cend())
			{
				return _caseToListPeakFilter[phyID];
			}
		}

		return list;
	}

	int mMapRelationManager::getCaseMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Case)
		{
			if (_caseToCase.find(phyID) == _caseToCase.cend())
				return -1;

			return _caseToCase[phyID];
		}
		else if (phyType == MxFunctions::Physical_Set)
		{
			if (_caseToSet.find(phyID) == _caseToSet.cend())
				return -1;

			switch (typee)
			{
			case MxFunctions::MapRelation_HistorySet:
				return _caseToSet[phyID].history_set;
			case MxFunctions::MapRelation_ElforceSet:
				return _caseToSet[phyID].elforce_set;
			case MxFunctions::MapRelation_GpforceSet:
				return _caseToSet[phyID].gpforce_set;
			default:
				break;
			}
		}
		else if (phyType == MxFunctions::Physical_Function)
		{
			if (_caseToFunction.find(phyID) == _caseToFunction.cend())
				return -1;

			return _caseToFunction[phyID];
		}
		else if (phyType == MxFunctions::Physical_GenericParam)
		{
			if (_caseToGenericParam.find(phyID) == _caseToGenericParam.cend())
				return -1;

			return _caseToGenericParam[phyID];
		}

		return -1;
	}

	void mMapRelationManager::appendConnector(int connectID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int oldID;
		switch (phyType)
		{
		case MxFunctions::Physical_Material:
		{
			if (_connectorToMat.find(connectID) != _connectorToMat.cend())
			{
				oldID = _connectorToMat[connectID];
				if (_matMap.find(oldID) != _matMap.cend())
				{
					_matMap[oldID][Physical_Connector].erase(connectID);
				}
			}
			_connectorToMat[connectID] = resultID;
			if (resultID > 0)
			{
				_matMap[resultID][Physical_Connector].insert(connectID);
			}
			break;
		}
		case MxFunctions::Physical_Set:
		{
			if (_connectorToSet.find(connectID) != _connectorToSet.cend())
			{
				if (typee == MapRelation_Master)
				{
					oldID = _connectorToSet[connectID].master;
				}
				else if (typee == MapRelation_Slave)
				{
					oldID = _connectorToSet[connectID].slave;
				}
				else if (typee == MapRelation_GpforceSet)
				{
					oldID = _connectorToSet[connectID].nodeSet;
				}
				else
				{
					oldID = _connectorToSet[connectID].defaultSet;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_Connector].erase(connectID);
				}
			}

			if (typee == MapRelation_Master)
			{
				_connectorToSet[connectID].master = resultID;
			}
			else if (typee == MapRelation_Slave)
			{
				_connectorToSet[connectID].slave = resultID;
			}
			else if (typee == MapRelation_GpforceSet)
			{
				_connectorToSet[connectID].nodeSet = resultID;
			}
			else
			{
				_connectorToSet[connectID].defaultSet = resultID;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_Connector].insert(connectID);
			}
			break;
		}
		case MxFunctions::Physical_Coordinate:
		{
			if (_connectorToCoordinate.find(connectID) != _connectorToCoordinate.cend())
			{
				if (typee == MapRelation_Coord2)
				{
					oldID = _connectorToCoordinate[connectID].coor2;
				}
				else
				{
					oldID = _connectorToCoordinate[connectID].defaultCoord;
				}

				if (_coordinateMap.find(oldID) != _coordinateMap.cend())
				{
					_coordinateMap[oldID][Physical_Connector].erase(connectID);
				}
			}

			if (typee == MapRelation_Coord2)
			{
				_connectorToCoordinate[connectID].coor2 = resultID;
			}
			else
			{
				_connectorToCoordinate[connectID].defaultCoord = resultID;
			}

			if (resultID > 0)
			{
				_coordinateMap[resultID][Physical_Connector].insert(connectID);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteConnector(int phyID)
	{
		int resultID;
		//删除材料
		if (_connectorToMat.find(phyID) != _connectorToMat.cend())
		{
			resultID = _connectorToMat[phyID];
			if (_matMap.find(resultID) != _matMap.cend())
			{
				_matMap[resultID][Physical_Connector].erase(phyID);
			}
			_connectorToMat.erase(phyID);
		}
		//删除集合
		if (_connectorToSet.find(phyID) != _connectorToSet.cend())
		{
			resultID = _connectorToSet[phyID].defaultSet;
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_Connector].erase(phyID);
			}
			resultID = _connectorToSet[phyID].nodeSet;
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_Connector].erase(phyID);
			}
			resultID = _connectorToSet[phyID].master;
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_Connector].erase(phyID);
			}
			resultID = _connectorToSet[phyID].slave;
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_Connector].erase(phyID);
			}
			_connectorToSet.erase(phyID);
		}
		//删除坐标系
		if (_connectorToCoordinate.find(phyID) != _connectorToCoordinate.cend())
		{
			resultID = _connectorToCoordinate[phyID].defaultCoord;
			if (_coordinateMap.find(resultID) != _coordinateMap.cend())
			{
				_coordinateMap[resultID][Physical_Connector].erase(phyID);
			}
			resultID = _connectorToCoordinate[phyID].coor2;
			if (_coordinateMap.find(resultID) != _coordinateMap.cend())
			{
				_coordinateMap[resultID][Physical_Connector].erase(phyID);
			}
			_connectorToCoordinate.erase(phyID);
		}

		//删除载荷边界
		if (_connectorMap.find(phyID) != _connectorMap.cend())
		{
			for (auto& it : _connectorMap[phyID])
			{
				set<int> ids = it.second;
				switch (it.first)
				{
				case Physical_LoadBoundary:
					for (int id : ids)
					{
						_loadBcToConnector.erase(id);
						deleteDataMap[Physical_LoadBoundary].insert(id);
					}
					break;
				}
			}
			_connectorMap.erase(phyID);
		}
	}

	int mMapRelationManager::getConnectorMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int resultID = -1;
		if (phyType == MxFunctions::Physical_Material)
		{
			if (_connectorToMat.find(phyID) == _connectorToMat.cend()) return -1;
			resultID = _connectorToMat[phyID];
		}
		else if (phyType == MxFunctions::Physical_Set)
		{
			if (_connectorToSet.find(phyID) == _connectorToSet.cend()) return -1;
			if (typee == MxFunctions::MapRelation_Master)	resultID = _connectorToSet[phyID].master;
			else if (typee == MxFunctions::MapRelation_Slave) resultID = _connectorToSet[phyID].slave;
			else if (typee == MxFunctions::MapRelation_GpforceSet) resultID = _connectorToSet[phyID].nodeSet;
			else resultID = _connectorToSet[phyID].defaultSet;
		}
		else if (phyType == MxFunctions::Physical_Coordinate)
		{
			if (_connectorToCoordinate.find(phyID) == _connectorToCoordinate.cend()) return -1;
			if (typee == MxFunctions::MapRelation_Coord2)
			{
				resultID = _connectorToCoordinate[phyID].coor2;
			}
			else
			{
				resultID = _connectorToCoordinate[phyID].defaultCoord;
			}
		}
		return resultID;
	}

	void mMapRelationManager::appendDeputy(int deputyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int oldID;
		switch (phyType)
		{
		case MxFunctions::Physical_Coordinate:
		{
			if (_deputyToCoordinate.find(deputyID) != _deputyToCoordinate.cend())
			{
				oldID = _deputyToCoordinate[deputyID];
				if (_coordinateMap.find(oldID) != _coordinateMap.cend())
				{
					_coordinateMap[oldID][Physical_Deputy].erase(deputyID);
				}
			}
			_deputyToCoordinate[deputyID] = resultID;
			if (resultID > 0)
			{
				_coordinateMap[resultID][Physical_Deputy].insert(deputyID);
			}
			break;
		}
		case MxFunctions::Physical_Deputy:
		{
			if (_deputyToDeputy.find(deputyID) != _deputyToDeputy.cend())
			{
				oldID = _deputyToDeputy[deputyID];
				if (_deputyMap.find(oldID) != _deputyMap.cend())
				{
					_deputyMap[oldID][Physical_Deputy].erase(deputyID);
				}
			}
			_deputyToDeputy[deputyID] = resultID;
			if (resultID > 0)
			{
				_deputyMap[resultID][Physical_Deputy].insert(deputyID);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteDeputy(int phyID)
	{
		int resultID;
		//删除坐标系
		if (_deputyToCoordinate.find(phyID) != _deputyToCoordinate.cend())
		{
			resultID = _deputyToCoordinate[phyID];
			if (_coordinateMap.find(resultID) != _coordinateMap.cend())
			{
				_coordinateMap[resultID][Physical_Deputy].erase(phyID);
			}
			_deputyToCoordinate.erase(phyID);
		}

		//删除连接副
		if (_deputyToDeputy.find(phyID) != _deputyToDeputy.cend())
		{
			resultID = _deputyToDeputy[phyID];
			if (_deputyMap.find(resultID) != _deputyMap.cend())
			{
				_deputyMap[resultID][Physical_Deputy].erase(phyID);
			}
			_deputyToDeputy.erase(phyID);
		}

		//删除部件
		if (_deputyToListPart.find(phyID) != _deputyToListPart.cend())
		{
			for (int partID : _deputyToListPart[phyID])
			{
				if (partID > 0 && _partMap.find(partID) != _partMap.cend())
				{
					_partMap[partID][Physical_Deputy].erase(phyID);
				}
			}
			_deputyToListPart.erase(phyID);
		}

		//删除连接副映射
		if (_deputyMap.find(phyID) != _deputyMap.cend())
		{
			for (auto& it : _deputyMap[phyID])
			{
				set<int> ids = it.second;
				switch (it.first)
				{
				case Physical_Deputy:
					for (int id : ids)
					{
						_deputyToDeputy.erase(id);
						deleteDataMap[Physical_Deputy].insert(id);
					}
					break;
				}
			}
			_deputyMap.erase(phyID);
		}
	}

	int mMapRelationManager::getDeputyMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int resultID = -1;
		if (phyType == MxFunctions::Physical_Coordinate)
		{
			if (_deputyToCoordinate.find(phyID) == _deputyToCoordinate.cend()) return -1;
			resultID = _deputyToCoordinate[phyID];
		}
		else if (phyType == MxFunctions::Physical_Deputy)
		{
			if (_deputyToDeputy.find(phyID) == _deputyToDeputy.cend()) return -1;
			resultID = _deputyToDeputy[phyID];
		}
		return resultID;
	}

	void mMapRelationManager::appendPredefinedField(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType)
	{
		int oldID;
		if (phyType == Physical_Set)
		{
			if (_predefinedFieldToSet.find(phyID) != _predefinedFieldToSet.cend())
			{
				oldID = _predefinedFieldToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_PredefinedField].erase(phyID);
				}
			}
			_predefinedFieldToSet[phyID] = resultID;
			if (resultID > 0)
			{
				_setMap[resultID][Physical_PredefinedField].insert(phyID);
			}
		}
		else if (phyType == Physical_Function)
		{
			if (_predefinedFieldToFunction.find(phyID) != _predefinedFieldToFunction.cend())
			{
				oldID = _predefinedFieldToFunction[phyID];
				if (_functionMap.find(oldID) != _functionMap.cend())
				{
					_functionMap[oldID][Physical_PredefinedField].erase(phyID);
				}
			}
			_predefinedFieldToFunction[phyID] = resultID;
			if (resultID > 0)
			{
				_functionMap[resultID][Physical_PredefinedField].insert(phyID);
			}
		}
		else if (phyType == Physical_Case)
		{
			_predefinedFieldToCase[phyID][resultID] = 1.0;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_PredefinedField].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deletePredefinedField(int phyID)
	{
		int resultID;
		//删除集合
		if (_predefinedFieldToSet.find(phyID) != _predefinedFieldToSet.cend())
		{
			resultID = _predefinedFieldToSet[phyID];
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_PredefinedField].erase(phyID);
			}
			_predefinedFieldToSet.erase(phyID);
		}

		//删除函数
		if (_predefinedFieldToFunction.find(phyID) != _predefinedFieldToFunction.cend())
		{
			resultID = _predefinedFieldToFunction[phyID];
			if (_functionMap.find(resultID) != _functionMap.cend())
			{
				_functionMap[resultID][Physical_PredefinedField].erase(phyID);
			}
			_predefinedFieldToFunction.erase(phyID);
		}

		//删除分析映射
		if (_predefinedFieldToCase.find(phyID) != _predefinedFieldToCase.cend())
		{
			for (auto& it : _predefinedFieldToCase[phyID])
			{
				if (_caseMap.find(it.first) != _caseMap.cend())
				{
					_caseMap[it.first][Physical_PredefinedField].erase(phyID);
				}
			}
			_predefinedFieldToCase.erase(phyID);
		}
		
	}

	int mMapRelationManager::getPredefinedFieldMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int resultID = -1;
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_predefinedFieldToSet.find(phyID) == _predefinedFieldToSet.cend()) return -1;
			resultID = _predefinedFieldToSet[phyID];
		}
		else if (phyType == MxFunctions::Physical_Function)
		{
			if (_predefinedFieldToFunction.find(phyID) == _predefinedFieldToFunction.cend()) return -1;
			resultID = _predefinedFieldToFunction[phyID];
		}
		return resultID;
	}

	void mMapRelationManager::appendOptimationGoal(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp)
	{
		int oldID;
		switch (phyTyp)
		{
		case MxFunctions::Physical_Case:
		{
			if (_goalToCase.find(phyID) != _goalToCase.cend())
			{
				oldID = _goalToCase[phyID];
				if (_caseMap.find(oldID) != _caseMap.cend())
				{
					_caseMap[oldID][Physical_OptimationGoal].erase(phyID);
				}
			}
			_goalToCase[phyID] = resultID;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_OptimationGoal].insert(phyID);
			}
			break;
		}
		default:
			break;
		}

	}

	void mMapRelationManager::deleteOptimationGoal(int phyID)
	{
		int resultID;
		//删除分析
		if (_goalToCase.find(phyID) != _goalToCase.cend())
		{
			resultID = _goalToCase[phyID];
			if (_caseMap.find(resultID) != _caseMap.cend())
			{
				_caseMap[resultID][Physical_OptimationGoal].erase(phyID);
			}
			_goalToCase.erase(phyID);
		}

		//if (_goalToListCase.find(phyID) != _goalToListCase.cend())
		//{
		//	QSet<int> caseID = _goalToListCase[phyID].toSet();
		//	for (const auto& it : caseID)
		//	{
		//		if (_caseMap.find(it) != _caseMap.cend())
		//		{
		//			_caseMap[it][Physical_OptimationGoal].erase(phyID);
		//		}
		//	}
		//	_goalToListCase.erase(phyID);
		//}

	}

	int mMapRelationManager::getOptimationGoalMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		switch (phyType)
		{
		case MxFunctions::Physical_Case:
			if (_goalToCase.find(phyID) == _goalToCase.cend())
				return -1;
			return _goalToCase[phyID];
		default:
			return -1;
		}

		return -1;
	}

	void mMapRelationManager::appendOptimationDesign(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp)
	{
		int oldID;
		switch (phyTyp)
		{
		case MxFunctions::Physical_Property:
		{
			if (_designToProperty.find(phyID) != _designToProperty.cend())
			{
				oldID = _designToProperty[phyID];
				if (_propertyMap.find(oldID) != _propertyMap.cend())
				{
					_propertyMap[oldID][Physical_OptimationDesign].erase(phyID);
				}
			}
			_designToProperty[phyID] = resultID;
			if (resultID > 0)
			{
				_propertyMap[resultID][Physical_OptimationDesign].insert(phyID);
			}
			break;
		}
		case MxFunctions::Physical_Set:
		{
			if (_designToSet.find(phyID) != _designToSet.cend())
			{
				oldID = _designToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_OptimationDesign].erase(phyID);
				}
			}
			_designToSet[phyID] = resultID;
			if (resultID > 0)
			{
				_setMap[resultID][Physical_OptimationDesign].insert(phyID);
			}
			break;
		}
		case MxFunctions::Physical_OptimationDesign:
		{
			_designToDesign[phyID] = resultID;
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteOptimationDesign(int phyID)
	{
		int resultID;
		//删除截面属性
		if (_designToProperty.find(phyID) != _designToProperty.cend())
		{
			resultID = _designToProperty[phyID];
			if (_propertyMap.find(resultID) != _propertyMap.cend())
			{
				_propertyMap[resultID][Physical_OptimationDesign].erase(phyID);
			}
			_designToProperty.erase(phyID);
		}

		//删除集合
		if (_designToSet.find(phyID) != _designToSet.cend())
		{
			resultID = _designToSet[phyID];
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_OptimationDesign].erase(phyID);
			}
			_designToSet.erase(phyID);
		}

		if (_designToDesign.find(phyID) != _designToDesign.cend())
		{
			_designToDesign.erase(phyID);
		}

		for (auto it = _designToDesign.begin(); it != _designToDesign.end(); )
		{
			if (it->second == phyID)
			{
				deleteDataMap[Physical_OptimationDesign].insert(it->first);
				it = _designToDesign.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	int mMapRelationManager::getOptimationDesignMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		switch (phyType)
		{
		case MxFunctions::Physical_Property:
			if (_designToProperty.find(phyID) == _designToProperty.cend())
				return -1;
			return _designToProperty[phyID];
		case MxFunctions::Physical_Set:
			if (_designToSet.find(phyID) == _designToSet.cend())
				return -1;
			return _designToSet[phyID];
		case MxFunctions::Physical_OptimationDesign:
			if (_designToDesign.find(phyID) == _designToDesign.cend())
				return -1;
			return _designToDesign[phyID];
		default:
			return -1;
		}

		return -1;
	}

	void mMapRelationManager::appendOptimationConstraint(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp)
	{
		int oldID;
		switch (phyTyp)
		{
		case MxFunctions::Physical_Property:
		{
			if (_constraintToProperty.find(phyID) != _constraintToProperty.cend())
			{
				oldID = _constraintToProperty[phyID];
				if (_propertyMap.find(oldID) != _propertyMap.cend())
				{
					_propertyMap[oldID][Physical_OptimationConstraint].erase(phyID);
				}
			}
			_constraintToProperty[phyID] = resultID;
			if (resultID > 0)
			{
				_propertyMap[resultID][Physical_OptimationConstraint].insert(phyID);
			}
			break;
		}
		case MxFunctions::Physical_Case:
		{
			if (_constraintToCase.find(phyID) != _constraintToCase.cend())
			{
				oldID = _constraintToCase[phyID];
				if (_caseMap.find(oldID) != _caseMap.cend())
				{
					_caseMap[oldID][Physical_OptimationConstraint].erase(phyID);
				}
			}
			_constraintToCase[phyID] = resultID;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_OptimationConstraint].insert(phyID);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteOptimationConstraint(int phyID)
	{
		int resultID;
		//删除截面属性
		if (_constraintToProperty.find(phyID) != _constraintToProperty.cend())
		{
			resultID = _constraintToProperty[phyID];
			if (_propertyMap.find(resultID) != _propertyMap.cend())
			{
				_propertyMap[resultID][Physical_OptimationConstraint].erase(phyID);
			}
			_constraintToProperty.erase(phyID);
		}

		//删除分析工况
		if (_constraintToCase.find(phyID) != _constraintToCase.cend())
		{
			resultID = _constraintToCase[phyID];
			if (_caseMap.find(resultID) != _caseMap.cend())
			{
				_caseMap[resultID][Physical_OptimationConstraint].erase(phyID);
			}
			_constraintToCase.erase(phyID);
		}
	}

	int mMapRelationManager::getOptimationConstraintMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		switch (phyType)
		{
		case MxFunctions::Physical_Property:
			if (_constraintToProperty.find(phyID) == _constraintToProperty.cend())
				return -1;
			return _constraintToProperty[phyID];
		case MxFunctions::Physical_Case:
			if (_constraintToCase.find(phyID) == _constraintToCase.cend())
				return -1;
			return _constraintToCase[phyID];
		default:
			return -1;
		}

		return -1;
	}

	void mMapRelationManager::deleteFreq(int phyID)
	{
		int resultID;
		//删除分析
		if (_freqMap.find(phyID) != _freqMap.cend())
		{
			for (auto& it : _freqMap[phyID][Physical_Case])
			{
				if (_caseToListFreq.find(it) != _caseToListFreq.cend())
				{
					std::replace(_caseToListFreq[it].begin(), _caseToListFreq[it].end(), phyID, -1);
				}
			}
			_freqMap.erase(phyID);
		}
	}

	void mMapRelationManager::appendLaminate(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp)
	{
		int oldID;
		switch (phyTyp)
		{
		case MxFunctions::Physical_Material:
		{
			if (_laminateToMat.find(phyID) != _laminateToMat.cend())
			{
				oldID = _laminateToMat[phyID];
				if (_matMap.find(oldID) != _matMap.cend())
				{
					_matMap[oldID][Physical_Laminate].erase(phyID);
				}
			}
			_laminateToMat[phyID] = resultID;
			if (resultID > 0)
			{
				_matMap[resultID][Physical_Laminate].insert(phyID);
			}
			break;
		}
		case MxFunctions::Physical_Set:
		{
			if (_laminateToSet.find(phyID) != _laminateToSet.cend())
			{
				oldID = _laminateToSet[phyID];
				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_Laminate].erase(phyID);
				}
			}
			_laminateToSet[phyID] = resultID;
			if (resultID > 0)
			{
				_setMap[resultID][Physical_Laminate].insert(phyID);
			}
			break;
		}
		case MxFunctions::Physical_Field:
		{
			if (_laminateToDiscreteField.find(phyID) != _laminateToDiscreteField.cend())
			{
				oldID = _laminateToDiscreteField[phyID];
				if (_discreteFieldMap.find(oldID) != _discreteFieldMap.cend())
				{
					_discreteFieldMap[oldID][Physical_Laminate].erase(phyID);
				}
			}
			_laminateToDiscreteField[phyID] = resultID;
			if (resultID > 0)
			{
				_discreteFieldMap[resultID][Physical_Laminate].insert(phyID);
			}
			break;
		}
		default:
			break;
		}
	}

	void mMapRelationManager::deleteLaminate(int phyID)
	{
		int resultID;

		//删除材料
		if (_laminateToMat.find(phyID) != _laminateToMat.cend())
		{
			resultID = _laminateToMat[phyID];
			if (_matMap.find(resultID) != _matMap.cend())
			{
				_matMap[resultID][Physical_Laminate].erase(phyID);
			}
			_laminateToMat.erase(phyID);
		}

		//删除集合
		if (_laminateToSet.find(phyID) != _laminateToSet.cend())
		{
			resultID = _laminateToSet[phyID];
			if (_setMap.find(resultID) != _setMap.cend())
			{
				_setMap[resultID][Physical_Laminate].erase(phyID);
			}
			_laminateToSet.erase(phyID);
		}
	
		//删除离散场
		if (_laminateToDiscreteField.find(phyID) != _laminateToDiscreteField.cend())
		{
			resultID = _laminateToDiscreteField[phyID];
			if (_discreteFieldMap.find(resultID) != _discreteFieldMap.cend())
			{
				_discreteFieldMap[resultID][Physical_Laminate].erase(phyID);
			}
			_laminateToDiscreteField.erase(phyID);
		}
	
			//删除层合板
		if (_laminateToLayerList.find(phyID) != _laminateToLayerList.cend())
		{
			QSet<int> layerID = _laminateToLayerList[phyID].toSet();
			for (const auto& it : layerID)
			{
				if (_laminateMap.find(it) != _laminateMap.cend())
				{
					_laminateMap[it][Physical_Laminate].erase(phyID);
				}
			}
			_laminateToLayerList.erase(phyID);
		}

		//删除层合板映射
		if (_laminateMap.find(phyID) != _laminateMap.cend())
		{
			for (auto it : _laminateMap[phyID])
			{
				set<int> ids = _laminateMap[phyID][it.first];
				switch (it.first)
				{
				case Physical_Laminate:
					for (auto id : ids)
					{
						if (_laminateToLayerList.find(id) != _laminateToLayerList.cend())
						{
							_laminateToLayerList[id].removeAll(phyID);
						}
					}
					break;
				case Physical_Property:
					for (auto id : ids)
					{
						if (_propertyToLaminate.find(id) != _propertyToLaminate.cend())
						{
							deleteDataMap[Physical_Property].insert(id);
							_propertyToLaminate.erase(id);
						}
					}
					break;
				default:
					break;
				}


			}
			_laminateMap.erase(phyID);
		}
	}

	int mMapRelationManager::getLaminateMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		switch (phyType)
		{
		case MxFunctions::Physical_Material:
		{
			if (_laminateToMat.find(phyID) != _laminateToMat.cend())
			{
				return _laminateToMat[phyID];
			}
			break;
		}
		case MxFunctions::Physical_Set:
		{
			if (_laminateToSet.find(phyID) != _laminateToSet.cend())
			{
				return _laminateToSet[phyID];
			}
			break;
		}
		case MxFunctions::Physical_Field:
		{
			if (_laminateToDiscreteField.find(phyID) != _laminateToDiscreteField.cend())
			{
				return _laminateToDiscreteField[phyID];
			}
			break;
		}
		default:
			break;
		}

		return -1;
	}

	void mMapRelationManager::appendModalContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee)
	{
		int oldID;
		if (phyTyp == Physical_Set)
		{
			if (_modalContriToSet.find(phyID) != _modalContriToSet.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_OutSet:
					oldID = _modalContriToSet[phyID].outSet;
					break;
				case MxFunctions::MapRelation_FrequencySet:
					oldID = _modalContriToSet[phyID].frequencySet;
					break;
				default:
					return;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_ModalContri].erase(phyID);
				}
			}

			switch (typee)
			{
			case MxFunctions::MapRelation_OutSet:
				_modalContriToSet[phyID].outSet = resultID;
				break;
			case MxFunctions::MapRelation_FrequencySet:
				_modalContriToSet[phyID].frequencySet = resultID;
				break;
			default:
				return;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_ModalContri].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteModalContri(int phyID)
	{
		//删除集合
		if (_modalContriToSet.find(phyID) != _modalContriToSet.cend())
		{
			vector<int> setIDs;
			setIDs.reserve(2);
			setIDs.push_back(_modalContriToSet[phyID].outSet);
			setIDs.push_back(_modalContriToSet[phyID].frequencySet);

			for (int setID : setIDs)
			{
				if (setID != -1)
				{
					_setMap[setID][Physical_ModalContri].erase(phyID);
				}
			}
			_modalContriToSet.erase(phyID);
		}
	}

	int mMapRelationManager::getModalContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_modalContriToSet.find(phyID) == _modalContriToSet.cend())
				return -1;

			switch (typee)
			{
			case MxFunctions::MapRelation_OutSet:
				return _modalContriToSet[phyID].outSet;
			case MxFunctions::MapRelation_FrequencySet:
				return _modalContriToSet[phyID].frequencySet;
			default:
				break;
			}
		}

		return -1;
	}

	void mMapRelationManager::appendNodeContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee)
	{
		int oldID;
		if (phyTyp == Physical_Set)
		{
			if (_nodeContriToSet.find(phyID) != _nodeContriToSet.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_GridSet:
					oldID = _nodeContriToSet[phyID].gridSet;
					break;
				case MxFunctions::MapRelation_GridfSet:
					oldID = _nodeContriToSet[phyID].gridfSet;
					break;
				case MxFunctions::MapRelation_ResponseGridSet:
					oldID = _nodeContriToSet[phyID].responseGridSet;
					break;
				case MxFunctions::MapRelation_FrequencySet:
					oldID = _nodeContriToSet[phyID].frequencySet;
					break;
				default:
					return;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_NodeContri].erase(phyID);
				}
			}

			switch (typee)
			{
			case MxFunctions::MapRelation_GridSet:
				_nodeContriToSet[phyID].gridSet = resultID;
				break;
			case MxFunctions::MapRelation_GridfSet:
				_nodeContriToSet[phyID].gridfSet = resultID;
				break;
			case MxFunctions::MapRelation_ResponseGridSet:
				_nodeContriToSet[phyID].responseGridSet = resultID;
				break;
			case MxFunctions::MapRelation_FrequencySet:
				_nodeContriToSet[phyID].frequencySet = resultID;
				break;
			default:
				return;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_NodeContri].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteNodeContri(int phyID)
	{
		if (_nodeContriToSet.find(phyID) != _nodeContriToSet.cend())
		{
			vector<int> setIDs;
			setIDs.reserve(4);
			setIDs.push_back(_nodeContriToSet[phyID].gridSet);
			setIDs.push_back(_nodeContriToSet[phyID].gridfSet);
			setIDs.push_back(_nodeContriToSet[phyID].responseGridSet);
			setIDs.push_back(_nodeContriToSet[phyID].frequencySet);

			for (int setID : setIDs)
			{
				if (setID != -1)
				{
					_setMap[setID][Physical_NodeContri].erase(phyID);
				}
			}
			_nodeContriToSet.erase(phyID);
		}
	}

	int mMapRelationManager::getNodeContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_nodeContriToSet.find(phyID) == _nodeContriToSet.cend())
				return -1;

			switch (typee)
			{
			case MxFunctions::MapRelation_GridSet:
				return _nodeContriToSet[phyID].gridSet;
			case MxFunctions::MapRelation_GridfSet:
				return _nodeContriToSet[phyID].gridfSet;
			case MxFunctions::MapRelation_ResponseGridSet:
				return _nodeContriToSet[phyID].responseGridSet;
			case MxFunctions::MapRelation_FrequencySet:
				return _nodeContriToSet[phyID].frequencySet;
			default:
				break;
			}
		}

		return -1;
	}

	void mMapRelationManager::appendTPContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee)
	{
		int oldID;
		if (phyTyp == Physical_Set)
		{
			if (_tpContriToSet.find(phyID) != _tpContriToSet.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_ResponseGridSet:
					oldID = _tpContriToSet[phyID].responseGridSet;
					break;
				case MxFunctions::MapRelation_ConptSet:
					oldID = _tpContriToSet[phyID].conptSet;
					break;
				case MxFunctions::MapRelation_ConelSet:
					oldID = _tpContriToSet[phyID].conelSet;
					break;
				case MxFunctions::MapRelation_ConrelSet:
					oldID = _tpContriToSet[phyID].conrelSet;
					break;
				case MxFunctions::MapRelation_ConvolSet:
					oldID = _tpContriToSet[phyID].convolSet;
					break;
				default:
					return;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_TPContri].erase(phyID);
				}
			}

			switch (typee)
			{
			case MxFunctions::MapRelation_ResponseGridSet:
				_tpContriToSet[phyID].responseGridSet = resultID;
				break;
			case MxFunctions::MapRelation_ConptSet:
				_tpContriToSet[phyID].conptSet = resultID;
				break;
			case MxFunctions::MapRelation_ConelSet:
				_tpContriToSet[phyID].conelSet = resultID;
				break;
			case MxFunctions::MapRelation_ConrelSet:
				_tpContriToSet[phyID].conrelSet = resultID;
				break;
			case MxFunctions::MapRelation_ConvolSet:
				_tpContriToSet[phyID].convolSet = resultID;
				break;
			default:
				return;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_TPContri].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteTPContri(int phyID)
	{
		if (_tpContriToSet.find(phyID) != _tpContriToSet.cend())
		{
			vector<int> setIDs;
			setIDs.reserve(5);
			setIDs.push_back(_tpContriToSet[phyID].responseGridSet);
			setIDs.push_back(_tpContriToSet[phyID].conptSet);
			setIDs.push_back(_tpContriToSet[phyID].conelSet);
			setIDs.push_back(_tpContriToSet[phyID].conrelSet);
			setIDs.push_back(_tpContriToSet[phyID].convolSet);

			for (int setID : setIDs)
			{
				if (setID != -1)
				{
					_setMap[setID][Physical_TPContri].erase(phyID);
				}
			}
			_tpContriToSet.erase(phyID);
		}
	}

	int mMapRelationManager::getTPContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_tpContriToSet.find(phyID) == _tpContriToSet.cend())
				return -1;

			switch (typee)
			{
			case MxFunctions::MapRelation_ResponseGridSet:
				return _tpContriToSet[phyID].responseGridSet;
			case MxFunctions::MapRelation_ConptSet:
				return _tpContriToSet[phyID].conptSet;
			case MxFunctions::MapRelation_ConelSet:
				return _tpContriToSet[phyID].conelSet;
			case MxFunctions::MapRelation_ConrelSet:
				return _tpContriToSet[phyID].conrelSet;
			case MxFunctions::MapRelation_ConvolSet:
				return _tpContriToSet[phyID].convolSet;
			default:
				break;
			}
		}

		return -1;
	}

	void mMapRelationManager::appendPanelContri(int phyID, int resultID, MxFunctions::PhysicalFieldType phyTyp, MxFunctions::MapRelationType typee)
	{
		int oldID;
		if (phyTyp == Physical_Set)
		{
			if (_panelContriToSet.find(phyID) != _panelContriToSet.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_FrequencySet:
					oldID = _panelContriToSet[phyID].frequencySet;
					break;
				case MxFunctions::MapRelation_ResponseGridSet:
					oldID = _panelContriToSet[phyID].responseGridSet;
					break;
				default:
					return;
				}

				if (_setMap.find(oldID) != _setMap.cend())
				{
					_setMap[oldID][Physical_PanelContri].erase(phyID);
				}
			}

			switch (typee)
			{
			case MxFunctions::MapRelation_FrequencySet:
				_panelContriToSet[phyID].frequencySet = resultID;
				break;
			case MxFunctions::MapRelation_ResponseGridSet:
				_panelContriToSet[phyID].responseGridSet = resultID;
				break;
			default:
				return;
			}

			if (resultID > 0)
			{
				_setMap[resultID][Physical_PanelContri].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deletePanelContri(int phyID)
	{
		if (_panelContriToSet.find(phyID) != _panelContriToSet.cend())
		{
			vector<int> setIDs;
			setIDs.reserve(2);
			setIDs.push_back(_panelContriToSet[phyID].frequencySet);
			setIDs.push_back(_panelContriToSet[phyID].responseGridSet);

			for (int setID : setIDs)
			{
				if (setID != -1)
				{
					_setMap[setID][Physical_PanelContri].erase(phyID);
				}
			}
			_panelContriToSet.erase(phyID);
		}
	}

	void mMapRelationManager::deleteGenericParam(int phyID)
	{
		//删除分析工况
		if (_genericParamMap.find(phyID) != _genericParamMap.cend())
		{
			for (auto& it : _genericParamMap[phyID][Physical_Case])
			{
				if (_caseToGenericParam.find(it) != _caseToGenericParam.cend())
				{
					_caseToGenericParam.erase(it);
				}
			}
			_genericParamMap.erase(phyID);
		}
	}

	void mMapRelationManager::deleteGenericPart(int phyID)
	{
		//删除部件
		if (_genericPartToListPart.find(phyID) != _genericPartToListPart.cend())
		{
			for (auto& it : _genericPartToListPart[phyID])
			{
				_partMap[it][Physical_GenericPart].erase(phyID);
			}
			_genericPartToListPart.erase(phyID);
		}
	}

	void mMapRelationManager::deleteDiscreteField(int phyID)
	{
		//删除坐标系
		if (_fieldToCoordinate.find(phyID) != _fieldToCoordinate.cend())
		{
			int coorID = _fieldToCoordinate[phyID];
			if (_coordinateMap.find(coorID) != _coordinateMap.cend())
			{
				_coordinateMap[coorID][Physical_Field].erase(phyID);
			}
			_fieldToCoordinate.erase(phyID);
		}

		//删除目标集合
		if (_fieldToSet.find(phyID) != _fieldToSet.cend())
		{
			int setID = _fieldToSet[phyID];
			if (_setMap.find(setID) != _setMap.cend())
			{
				_setMap[setID][Physical_Field].erase(phyID);
			}
			_fieldToSet.erase(phyID);
		}

		if (_discreteFieldMap.find(phyID) != _discreteFieldMap.cend())
		{
			std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>>& map = _discreteFieldMap[phyID];
			for (auto& it : map)
			{
				set<int> ids = it.second;
				switch (it.first)
				{
				case Physical_Coordinate:
					for (int id : ids)
					{
						if (_coordinateToDiscreteField.find(id) != _coordinateToDiscreteField.cend())
						{
							_coordinateToDiscreteField.erase(id);
							deleteDataMap[it.first].insert(id);
						}
					}
					break;
				case Physical_Property:
					for (int id : ids)
					{
						if (_propertyToDiscreteField.find(id) == _propertyToDiscreteField.cend())
							continue;

						if (_propertyToDiscreteField[id].rotationField == phyID)
						{
							_propertyToDiscreteField[id].rotationField = -1;
						}
						if (_propertyToDiscreteField[id].thickField == phyID)
						{
							_propertyToDiscreteField[id].thickField = -1;
						}
						else
						{
							continue;
						}
						deleteDataMap[Physical_Property].insert(id);
					}
					break;
				case Physical_Laminate:
					for (int id : ids)
					{
						if (_laminateToDiscreteField.find(id) != _laminateToDiscreteField.cend())
						{
							_laminateToDiscreteField.erase(id);
							deleteDataMap[Physical_Laminate].insert(id);
						}
					}
					break;
				case Physical_LoadBoundary:
					for (int id : ids)
					{
						if (_loadBcToAnalysisField.find(id) != _loadBcToAnalysisField.cend())
						{
							bool changed = false;
							if (_loadBcToAnalysisField[id].field == phyID)
							{
								_loadBcToAnalysisField[id].field = -1;
								changed = true;
							}
							if (_loadBcToAnalysisField[id].conv_field == phyID)
							{
								_loadBcToAnalysisField[id].conv_field = -1;
								changed = true;
							}
							if (changed)
							{
								deleteDataMap[Physical_LoadBoundary].insert(id);
							}
						}
					}
					break;
				default:
					break;
				}
			}
			_discreteFieldMap.erase(phyID);
		}
	}

	void mMapRelationManager::appendMicroconstituent(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		int oldID;
		switch (phyType)
		{
		case Physical_Material:
			if (_microconstituentToMat.find(phyID) != _microconstituentToMat.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_Mat2:
					oldID = _microconstituentToMat[phyID].mat2;
					break;
				default:
					oldID = _microconstituentToMat[phyID].defaultMat;
					break;
				}

				if (_matMap.find(oldID) != _matMap.cend())
				{
					_matMap[oldID][Physical_Microconstituent].erase(phyID);
				}
			}

			switch (typee)
			{
			case MxFunctions::MapRelation_Mat2:
				_microconstituentToMat[phyID].mat2 = resultID;
				break;
			default:
				_microconstituentToMat[phyID].defaultMat = resultID;
				break;
			}

			if (resultID > 0)
			{
				_matMap[resultID][Physical_Microconstituent].insert(phyID);
			}
			break;
		case Physical_Function:
			if (_microconstituentToFunction.find(phyID) != _microconstituentToFunction.cend())
			{
				oldID = _microconstituentToFunction[phyID];
				if (_functionMap.find(oldID) != _functionMap.cend())
				{
					_functionMap[oldID][Physical_Microconstituent].erase(phyID);
				}
			}

			_microconstituentToFunction[phyID] = resultID;
			if (resultID > 0)
			{
				_functionMap[resultID][Physical_Microconstituent].insert(phyID);
			}
			break;
		default:
			break;
		}
	}

	void mMapRelationManager::deleteMicroconstituent(int phyID)
	{
		// 清理材料/函数对微观组分的正向映射，并同步删除反向映射 _matMap/_functionMap 中的记录
		if (_microconstituentToMat.find(phyID) != _microconstituentToMat.cend())
		{
			int defaultMat = _microconstituentToMat[phyID].defaultMat;
			int mat2 = _microconstituentToMat[phyID].mat2;
			if (defaultMat > 0 && _matMap.find(defaultMat) != _matMap.cend())
			{
				_matMap[defaultMat][Physical_Microconstituent].erase(phyID);
			}
			if (mat2 > 0 && _matMap.find(mat2) != _matMap.cend())
			{
				_matMap[mat2][Physical_Microconstituent].erase(phyID);
			}
			_microconstituentToMat.erase(phyID);
		}

		if (_microconstituentToFunction.find(phyID) != _microconstituentToFunction.cend())
		{
			int functionID = _microconstituentToFunction[phyID];
			if (functionID > 0 && _functionMap.find(functionID) != _functionMap.cend())
			{
				_functionMap[functionID][Physical_Microconstituent].erase(phyID);
			}
			_microconstituentToFunction.erase(phyID);
		}
	}

	int mMapRelationManager::getMicroconstituentMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		switch (phyType)
		{
		case Physical_Material:
			if (_microconstituentToMat.find(phyID) != _microconstituentToMat.cend())
			{
				switch (typee)
				{
				case MxFunctions::MapRelation_Mat2:
					return _microconstituentToMat[phyID].mat2;
				default:
					return _microconstituentToMat[phyID].defaultMat;
				}
			}
			break;
		case Physical_Function:
			if (_microconstituentToFunction.find(phyID) != _microconstituentToFunction.cend())
			{
				return _microconstituentToFunction[phyID];
			}
			break;
		default:
			break;
		}
		return -1;
	}

	void mMapRelationManager::appendRotateAnalyse(int phyID, int resultID, MxFunctions::PhysicalFieldType phyType)
	{
		int oldID;
		if (phyType == Physical_Part)
		{
			if (_rotateToPart.find(phyID) != _rotateToPart.cend())
			{
				oldID = _rotateToPart[phyID];
				if (_partMap.find(oldID) != _partMap.cend())
				{
					_partMap[oldID][Physical_RotateAnalyse].erase(phyID);
				}
			}
			_rotateToPart[phyID] = resultID;
			if (resultID > 0)
			{
				_partMap[resultID][Physical_RotateAnalyse].insert(phyID);
			}
		}
		else if (phyType == Physical_Case)
		{
			_rotateToCase[phyID][resultID] = 1.0;
			if (resultID > 0)
			{
				_caseMap[resultID][Physical_RotateAnalyse].insert(phyID);
			}
		}
	}

	void mMapRelationManager::deleteRotateAnalyse(int phyID)
	{
		//删除部件
		if (_rotateToPart.find(phyID) != _rotateToPart.cend())
		{
			int partID = _rotateToPart[phyID];
			if (_partMap.find(partID) != _partMap.cend())
			{
				_partMap[partID][Physical_RotateAnalyse].erase(phyID);
			}
			_rotateToPart.erase(phyID);
		}

		//删除分析工况
		if (_rotateToCase.find(phyID) != _rotateToCase.cend())
		{
			for (auto& it : _rotateToCase[phyID])
			{
				if (_caseMap.find(it.first) != _caseMap.cend())
				{
					_caseMap[it.first][Physical_RotateAnalyse].erase(phyID);
				}
			}
			_rotateToCase.erase(phyID);
		}
	}

	int mMapRelationManager::getRotateAnalyseMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType)
	{
		if (phyType == MxFunctions::Physical_Part)
		{
			if (_rotateToPart.find(phyID) == _rotateToPart.cend())
				return -1;

			return _rotateToPart[phyID];
		}

		return -1;
	}

	void mMapRelationManager::deletePeakFilter(int phyID)
	{
		//删除分析工况
		if (_peakFilterMap.find(phyID) != _peakFilterMap.cend())
		{
			for (auto& it : _peakFilterMap[phyID][Physical_Case])
			{
				if (_caseToListPeakFilter.find(it) != _caseToListPeakFilter.cend())
				{
					_caseToListPeakFilter[it].removeAll(phyID);
				}
			}
			_peakFilterMap.erase(phyID);
		}
	}

	int mMapRelationManager::getPanelContriMapRelation(int phyID, MxFunctions::PhysicalFieldType phyType, MxFunctions::MapRelationType typee)
	{
		if (phyType == MxFunctions::Physical_Set)
		{
			if (_panelContriToSet.find(phyID) == _panelContriToSet.cend())
				return -1;

			switch (typee)
			{
			case MxFunctions::MapRelation_FrequencySet:
				return _panelContriToSet[phyID].frequencySet;
			case MxFunctions::MapRelation_ResponseGridSet:
				return _panelContriToSet[phyID].responseGridSet;
			default:
				break;
			}
		}

		return -1;
	}

	void mMapRelationManager::deleteNSM(int phyID)
	{
		//删除分析工况
		if (_nsmMap.find(phyID) != _nsmMap.cend())
		{
			for (auto& it : _nsmMap[phyID][Physical_Case])
			{
				if (_caseToListNSM.find(it) != _caseToListNSM.cend())
				{
					_caseToListNSM[it].removeAll(phyID);
				}
			}
			_nsmMap.erase(phyID);
		}
	}

	void mMapRelationManager::copyMapRelation(MxFunctions::PhysicalFieldType type, int oldID, int copyID, int caseID)
	{
		switch (type)
		{
		case Physical_LoadBoundary:
		{
			//集合
			if (_loadBcToSet.find(oldID) != _loadBcToSet.cend())
				appendLoadBoundary(copyID, _loadBcToSet[oldID], Physical_Set, MapRelation_None, 1.0);
			//坐标系
			if (_loadBcToCoordinate.find(oldID) != _loadBcToCoordinate.cend())
				appendLoadBoundary(copyID, _loadBcToCoordinate[oldID], Physical_Coordinate, MapRelation_None, 1.0);
			//函数
			if (_loadBcToFunction.find(oldID) != _loadBcToFunction.cend())
			{
				auto funcData = _loadBcToFunction[oldID];
				if (funcData.default_function != -1)
					appendLoadBoundary(copyID, funcData.default_function, Physical_Function, MapRelation_None, 1.0);
				if (funcData.conv_function != -1)
					appendLoadBoundary(copyID, funcData.conv_function, Physical_Function, MapRelation_convFunction, 1.0);
				if (funcData.amplitude_function != -1)
					appendLoadBoundary(copyID, funcData.amplitude_function, Physical_Function, MapRelation_AmplitudeFunction, 1.0);
				if (funcData.phase_function != -1)
					appendLoadBoundary(copyID, funcData.phase_function, Physical_Function, MapRelation_PhaseFunction, 1.0);
			}
			//分析工况
			if (caseID > 0)
			{
				if (_loadBcToCase.find(oldID) != _loadBcToCase.cend())
				{
					for (auto& it : _loadBcToCase[oldID])
					{
						if (caseID == it.first) appendLoadBoundary(copyID, it.first, Physical_Case, MapRelation_None, it.second);
					}
				}
			}
			else
			{
				if (_loadBcToCase.find(oldID) != _loadBcToCase.cend())
				{
					for (auto& it : _loadBcToCase[oldID])
					{
						appendLoadBoundary(copyID, it.first, Physical_Case, MapRelation_None, it.second);
					}
				}
			}

			//载荷边界
			if (_loadBcToLoadBC.find(oldID) != _loadBcToLoadBC.cend())
				appendLoadBoundary(copyID, _loadBcToLoadBC[oldID], Physical_LoadBoundary, MapRelation_None, 1.0);
			//连接
			if (_loadBcToConnector.find(oldID) != _loadBcToConnector.cend())
				appendLoadBoundary(copyID, _loadBcToConnector[oldID], Physical_Connector, MapRelation_None, 1.0);
			//分析场
			if (_loadBcToAnalysisField.find(oldID) != _loadBcToAnalysisField.cend())
			{
				auto fieldData = _loadBcToAnalysisField[oldID];
				if (fieldData.field != -1)
					appendLoadBoundary(copyID, fieldData.field, Physical_Field, MapRelation_AnalysisField, 1.0);
				if (fieldData.conv_field != -1)
					appendLoadBoundary(copyID, fieldData.conv_field, Physical_Field, MapRelation_convAnalysisField, 1.0);
			}
			//多个载荷边界
			if (_loadToListLoad.find(oldID) != _loadToListLoad.cend())
			{
				for (int i = 0; i < _loadToListLoad[oldID].size(); ++i)
					appendTableMapRelation({ Physical_LoadBoundary, copyID }, { Physical_LoadBoundary, _loadToListLoad[oldID][i] }, i);
			}
			//多个函数
			if (_loadBcToListFunction.find(oldID) != _loadBcToListFunction.cend())
			{
				for (int i = 0; i < _loadBcToListFunction[oldID].size(); ++i)
					appendTableMapRelation({ Physical_LoadBoundary, copyID }, { Physical_Function, _loadBcToListFunction[oldID][i] }, i);
			}
			break;
		}
		case Physical_Property:
		{
			if (_propertyToMat.find(oldID) != _propertyToMat.cend())
				appendProperty(copyID, _propertyToMat[oldID], Physical_Material);
			if (_propertyToShape.find(oldID) != _propertyToShape.cend())
				appendProperty(copyID, _propertyToShape[oldID], Physical_PropertyShape);
			if (_propertyToCoordinate.find(oldID) != _propertyToCoordinate.cend())
				appendProperty(copyID, _propertyToCoordinate[oldID], Physical_Coordinate);
			if (_propertyToLaminate.find(oldID) != _propertyToLaminate.cend())
				appendProperty(copyID, _propertyToLaminate[oldID], Physical_Laminate);
			if (_propertyToListMat.find(oldID) != _propertyToListMat.cend())
			{
				for (int i = 0; i < _propertyToListMat[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Property, copyID }, { Physical_Material, _propertyToListMat[oldID][i] }, i);
			}
			if (_propertyToListShape.find(oldID) != _propertyToListShape.cend())
			{
				for (int i = 0; i < _propertyToListShape[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Property, copyID }, { Physical_PropertyShape, _propertyToListShape[oldID][i] }, i);
			}
			if (_propertyToDiscreteField.find(oldID) != _propertyToDiscreteField.cend())
			{
				auto disData = _propertyToDiscreteField[oldID];
				if (disData.rotationField != -1)
					appendContact(copyID, disData.rotationField, Physical_Field, MapRelation_RotationField);
				if (disData.thickField != -1)
					appendContact(copyID, disData.thickField, Physical_Field, MapRelation_ThickField);
			}
			break;
		}
		case Physical_Part:
		{
			if (_partToProperty.find(oldID) != _partToProperty.cend())
				appendPart(copyID, _partToProperty[oldID], Physical_Property);
			break;
		}
		case Physical_Case:
		{
			if (_caseToFunction.find(oldID) != _caseToFunction.cend())
				appendCase(copyID, _caseToFunction[oldID], Physical_Function, MapRelation_None);
			if (_caseToCase.find(oldID) != _caseToCase.cend())
				appendCase(copyID, _caseToCase[oldID], Physical_Case, MapRelation_None);
			if (_caseToGenericParam.find(oldID) != _caseToGenericParam.cend())
				appendCase(copyID, _caseToGenericParam[oldID], Physical_GenericParam, MapRelation_None);
			if (_caseToSet.find(oldID) != _caseToSet.cend())
			{
				auto setData = _caseToSet[oldID];
				if (setData.history_set != -1)
					appendCase(copyID, setData.history_set, Physical_Set, MapRelation_HistorySet);
				if (setData.elforce_set != -1)
					appendCase(copyID, setData.elforce_set, Physical_Set, MapRelation_ElforceSet);
				if (setData.gpforce_set != -1)
					appendCase(copyID, setData.gpforce_set, Physical_Set, MapRelation_GpforceSet);
			}
			//RandomCase需要直接拷贝，不能分字段调用appendTableMapRelation
			if (_RandomCase.find(oldID) != _RandomCase.cend())
			{
				_RandomCase[copyID] = _RandomCase[oldID];
				for (auto& data : _RandomCase[copyID])
				{
					if (data.case1 != -1)
					{
						if (_caseMap.find(data.case1) == _caseMap.cend())
							_caseMap[data.case1] = unordered_map<PhysicalFieldType, set<int>>();
						_caseMap[data.case1][Physical_Case].insert(copyID);
					}
					if (data.case2 != -1)
					{
						if (_caseMap.find(data.case2) == _caseMap.cend())
							_caseMap[data.case2] = unordered_map<PhysicalFieldType, set<int>>();
						_caseMap[data.case2][Physical_Case].insert(copyID);
					}
					if (data.function != -1)
					{
						if (_functionMap.find(data.function) == _functionMap.cend())
							_functionMap[data.function] = unordered_map<PhysicalFieldType, set<int>>();
						_functionMap[data.function][Physical_Case].insert(copyID);
					}
				}
			}
			if (_caseToListLoadBC.find(oldID) != _caseToListLoadBC.cend())
			{
				for (int i = 0; i < _caseToListLoadBC[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Case, copyID }, { Physical_LoadBoundary, _caseToListLoadBC[oldID][i] }, i);
			}
			if (_caseToListFreq.find(oldID) != _caseToListFreq.cend())
			{
				for (int i = 0; i < _caseToListFreq[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Case, copyID }, { Physical_FreqSamplPoint, _caseToListFreq[oldID][i] }, i);
			}
			if (_caseToListNSM.find(oldID) != _caseToListNSM.cend())
			{
				for (int i = 0; i < _caseToListNSM[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Case, copyID }, { Physical_NonStructuralQuality, _caseToListNSM[oldID][i] }, i);
			}
			if (_caseToListModelFix.find(oldID) != _caseToListModelFix.cend())
			{
				for (int i = 0; i < _caseToListModelFix[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Case, copyID }, { Physical_ModelFix, _caseToListModelFix[oldID][i] }, i);
			}
			if (_caseToListPeakFilter.find(oldID) != _caseToListPeakFilter.cend())
			{
				for (int i = 0; i < _caseToListPeakFilter[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Case, copyID }, { Physical_PeakFilter, _caseToListPeakFilter[oldID][i] }, i);
			}
			break;
		}
		case Physical_Connector:
		{
			if (_connectorToMat.find(oldID) != _connectorToMat.cend())
				appendConnector(copyID, _connectorToMat[oldID], Physical_Material, MapRelation_None);
			if (_connectorToSet.find(oldID) != _connectorToSet.cend())
			{
				auto setData = _connectorToSet[oldID];
				if (setData.defaultSet != -1)
					appendConnector(copyID, setData.defaultSet, Physical_Set, MapRelation_None);
				if (setData.master != -1)
					appendConnector(copyID, setData.master, Physical_Set, MapRelation_Master);
				if (setData.slave != -1)
					appendConnector(copyID, setData.slave, Physical_Set, MapRelation_Slave);
				if (setData.nodeSet != -1)
					appendConnector(copyID, setData.nodeSet, Physical_Set, MapRelation_GpforceSet);
			}
			if (_connectorToCoordinate.find(oldID) != _connectorToCoordinate.cend())
			{
				auto coordData = _connectorToCoordinate[oldID];
				if (coordData.defaultCoord != -1)
					appendConnector(copyID, coordData.defaultCoord, Physical_Coordinate, MapRelation_None);
				if (coordData.coor2 != -1)
					appendConnector(copyID, coordData.coor2, Physical_Coordinate, MapRelation_Coord2);
			}
			break;
		}
		case Physical_Contact:
		{
			if (_contactToSet.find(oldID) != _contactToSet.cend())
			{
				auto setData = _contactToSet[oldID];
				if (setData.master != -1)
					appendContact(copyID, setData.master, Physical_Set, MapRelation_Master);
				if (setData.slave != -1)
					appendContact(copyID, setData.slave, Physical_Set, MapRelation_Slave);
			}
			if (_contactToContact.find(oldID) != _contactToContact.cend())
				appendContact(copyID, _contactToContact[oldID], Physical_Contact, MapRelation_None);
			if (_contactToFunction.find(oldID) != _contactToFunction.cend())
				appendContact(copyID, _contactToFunction[oldID], Physical_Function, MapRelation_None);

			//分析工况
			if (caseID > 0)
			{
				if (_contactToCase.find(oldID) != _contactToCase.cend())
				{
					for (auto& it : _contactToCase[oldID])
					{
						if (caseID == it.first) appendContact(copyID, it.first, Physical_Case, MapRelation_None);
					}
				}
			}
			else
			{
				if (_contactToCase.find(oldID) != _contactToCase.cend())
				{
					for (auto& it : _contactToCase[oldID])
					{
						appendContact(copyID, it.first, Physical_Case, MapRelation_None);
					}
				}
			}
			break;
		}
		case Physical_PeriodicBoundary:
		{
			if (_periodicBoundaryToSet.find(oldID) != _periodicBoundaryToSet.cend())
			{
				auto setData = _periodicBoundaryToSet[oldID];
				if (setData.master != -1)
					appendPeriodicBoundary(copyID, setData.master, Physical_Set, MapRelation_Master);
				if (setData.slave != -1)
					appendPeriodicBoundary(copyID, setData.slave, Physical_Set, MapRelation_Slave);
			}
			break;
		}
		case Physical_ModelFix:
		{
			if (_modelFixToSet.find(oldID) != _modelFixToSet.cend())
			{
				int setID = _modelFixToSet[oldID];
				if (setID != -1)
					appendModelFix(copyID, setID, Physical_Set, MapRelation_Master);
			}
			break;
		}
		case Physical_Laminate:
		{
			if (_laminateToMat.find(oldID) != _laminateToMat.cend())
				appendLaminate(copyID, _laminateToMat[oldID], Physical_Material);
			if (_laminateToSet.find(oldID) != _laminateToSet.cend())
				appendLaminate(copyID, _laminateToSet[oldID], Physical_Set);
			if (_laminateToDiscreteField.find(oldID) != _laminateToDiscreteField.cend())
				appendLaminate(copyID, _laminateToDiscreteField[oldID], Physical_Field);
			if (_laminateToLayerList.find(oldID) != _laminateToLayerList.cend())
			{
				for (int i = 0; i < _laminateToLayerList[oldID].size(); ++i)
					appendTableMapRelation({ Physical_Laminate, copyID }, { Physical_Laminate, _laminateToLayerList[oldID][i] }, i);
			}
			break;
		}
		case Physical_ModalContri:
		{
			if (_modalContriToSet.find(oldID) != _modalContriToSet.cend())
			{
				auto setData = _modalContriToSet[oldID];
				if (setData.outSet != -1)
					appendModalContri(copyID, setData.outSet, Physical_Set, MapRelation_OutSet);
				if (setData.frequencySet != -1)
					appendModalContri(copyID, setData.frequencySet, Physical_Set, MapRelation_FrequencySet);
			}
			break;
		}
		case Physical_OptimationGoal:
		{
			if (_goalToCase.find(oldID) != _goalToCase.cend())
				appendOptimationGoal(copyID, _goalToCase[oldID], Physical_Case);
			break;
		}
		case Physical_OptimationDesign:
		{
			if (_designToProperty.find(oldID) != _designToProperty.cend())
				appendOptimationDesign(copyID, _designToProperty[oldID], Physical_Property);
			if (_designToSet.find(oldID) != _designToSet.cend())
				appendOptimationDesign(copyID, _designToSet[oldID], Physical_Set);
			if (_designToDesign.find(oldID) != _designToDesign.cend())
				_designToDesign[copyID] = _designToDesign[oldID];
			break;
		}
		case Physical_OptimationConstraint:
		{
			if (_constraintToProperty.find(oldID) != _constraintToProperty.cend())
				appendOptimationConstraint(copyID, _constraintToProperty[oldID], Physical_Property);
			if (_constraintToCase.find(oldID) != _constraintToCase.cend())
				appendOptimationConstraint(copyID, _constraintToCase[oldID], Physical_Case);
			break;
		}
		case Physical_Deputy:
		{
			if (_deputyToCoordinate.find(oldID) != _deputyToCoordinate.cend())
				appendDeputy(copyID, _deputyToCoordinate[oldID], Physical_Coordinate, MapRelation_None);
			if (_deputyToDeputy.find(oldID) != _deputyToDeputy.cend())
				appendDeputy(copyID, _deputyToDeputy[oldID], Physical_Deputy, MapRelation_None);
			if (_deputyToListPart.find(oldID) != _deputyToListPart.cend())
			{
				for (int i = 0; i < _deputyToListPart[oldID].size(); ++i)
					appendDeputy(copyID, _deputyToListPart[oldID][i], Physical_Part, MapRelation_None);
			}
			break;
		}
		case Physical_PredefinedField:
		{
			if (_predefinedFieldToSet.find(oldID) != _predefinedFieldToSet.cend())
				appendPredefinedField(copyID, _predefinedFieldToSet[oldID], Physical_Set);
			if (_predefinedFieldToFunction.find(oldID) != _predefinedFieldToFunction.cend())
				appendPredefinedField(copyID, _predefinedFieldToFunction[oldID], Physical_Function);

			//分析工况
			if (caseID > 0)
			{
				if (_predefinedFieldToCase.find(oldID) != _predefinedFieldToCase.cend())
				{
					for (auto& it : _predefinedFieldToCase[oldID])
					{
						if (caseID == it.first) appendPredefinedField(copyID, it.first, Physical_Case);
					}
				}
			}
			else
			{
				if (_predefinedFieldToCase.find(oldID) != _predefinedFieldToCase.cend())
				{
					for (auto& it : _predefinedFieldToCase[oldID])
					{
						appendPredefinedField(copyID, it.first, Physical_Case);
					}
				}
			}

			break;
		}
		case Physical_Coordinate:
		{
			if (_coordinateToDiscreteField.find(oldID) != _coordinateToDiscreteField.cend())
				appendCoordinate(copyID, _coordinateToDiscreteField[oldID], Physical_Field);
			break;
		}
		case Physical_Microconstituent:
		{
			if (_microconstituentToMat.find(oldID) != _microconstituentToMat.cend())
			{
				auto matData = _microconstituentToMat[oldID];
				if (matData.defaultMat > 0)
					appendMicroconstituent(copyID, matData.defaultMat, Physical_Material, MapRelation_None);
				if (matData.mat2 > 0)
					appendMicroconstituent(copyID, matData.mat2, Physical_Material, MapRelation_Mat2);
			}
			if (_microconstituentToFunction.find(oldID) != _microconstituentToFunction.cend())
				appendMicroconstituent(copyID, _microconstituentToFunction[oldID], Physical_Function, MapRelation_None);
			break;
		}
		case Physical_RotateAnalyse:
		{
			if (_rotateToPart.find(oldID) != _rotateToPart.cend())
				appendRotateAnalyse(copyID, _rotateToPart[oldID], Physical_Part);

			if (_rotateToCase.find(oldID) != _rotateToCase.cend())
			{
				for (auto& it : _rotateToCase[oldID])
				{
					appendRotateAnalyse(copyID, it.first, Physical_Case);
				}
			}
			break;
		}
		default:
			//多体实体引用：整体拷贝到新ID
			if (isMotionSource(type))
				copyMotionRefs(type, oldID, copyID);
			break;
		}
	}

	/******************** 多体实体引用映射（通用存储）********************/

	bool mMapRelationManager::isMotionSource(MxFunctions::PhysicalFieldType type)
	{
		switch (type)
		{
		case Physical_MotionBody:
		case Physical_MotionJoint:
		case Physical_MotionCoupling:
		case Physical_MotionDrive:
		case Physical_MotionScenario:
		case Physical_MotionMarker:
		case Physical_MotionVectorForce:
		case Physical_MotionContact:
		case Physical_MotionSpring:
		case Physical_MotionControlInput:
		case Physical_MotionElectroImport:
		case Physical_MotionElectroExport:
		case Physical_MotionHydraulic:
		case Physical_MotionRL:
		case Physical_MotionSensor:
		case Physical_MotionController:
		case Physical_MotionAutoFlex:
			return true;
		default:
			return false;
		}
	}

	void mMapRelationManager::appendMotionRef(MxFunctions::PhysicalFieldType srcType, int srcID,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstID)
	{
		if (dstID < 0)
			return;
		MotionRoleKey key(dstType, role);
		_motionRefMap[srcType][srcID][key].insert(dstID);
	}

	void mMapRelationManager::deleteMotionRefs(MxFunctions::PhysicalFieldType srcType, int srcID)
	{
		auto itType = _motionRefMap.find(srcType);
		if (itType != _motionRefMap.end())
			itType->second.erase(srcID);
	}

	void mMapRelationManager::deleteMotionRef(MxFunctions::PhysicalFieldType srcType, int srcID,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstID)
	{
		auto itType = _motionRefMap.find(srcType);
		if (itType == _motionRefMap.end())
			return;
		auto itSrc = itType->second.find(srcID);
		if (itSrc == itType->second.end())
			return;
		auto itRole = itSrc->second.find(MotionRoleKey(dstType, role));
		if (itRole != itSrc->second.end())
			itRole->second.erase(dstID);
	}

	void mMapRelationManager::clearMotionRole(MxFunctions::PhysicalFieldType srcType, int srcID,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role)
	{
		auto itType = _motionRefMap.find(srcType);
		if (itType == _motionRefMap.end())
			return;
		auto itSrc = itType->second.find(srcID);
		if (itSrc == itType->second.end())
			return;
		itSrc->second[MotionRoleKey(dstType, role)].clear();
	}

	const std::set<int>& mMapRelationManager::getMotionRefs(MxFunctions::PhysicalFieldType srcType, int srcID,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role) const
	{
		static const std::set<int> emptySet;
		auto itType = _motionRefMap.find(srcType);
		if (itType == _motionRefMap.cend())
			return emptySet;
		auto itSrc = itType->second.find(srcID);
		if (itSrc == itType->second.cend())
			return emptySet;
		auto itRole = itSrc->second.find(MotionRoleKey(dstType, role));
		if (itRole == itSrc->second.cend())
			return emptySet;
		return itRole->second;
	}

	void mMapRelationManager::copyMotionRefs(MxFunctions::PhysicalFieldType srcType, int oldID, int copyID)
	{
		auto itType = _motionRefMap.find(srcType);
		if (itType == _motionRefMap.end())
			return;
		auto itSrc = itType->second.find(oldID);
		if (itSrc == itType->second.end())
			return;
		auto& target = _motionRefMap[srcType][copyID];
		for (const auto& rolePair : itSrc->second)
			target[rolePair.first].insert(rolePair.second.begin(), rolePair.second.end());
	}

}



