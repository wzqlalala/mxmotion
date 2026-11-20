#include "mDataStatusManager.h"

#include <QFileInfo>
#include <QTextStream>
#include <memory>

#include "mModelData.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
#include "mMapRelationManager.h"
#include "mPropertyManager.h"
#include "mPropertyBase.h"
#include "mLoadBoundaryBase.h"
#include "mLoadBoundaryManager.h"
#include "mConnectorManager.h"
#include "mConnectorBase.h"
#include "mMeshData.h"
#include "mOptimizationConstraintData.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationGoalData.h"
#include "mStructOption.h"

#include "LowCPlusDefine.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mOtherFunction.h"

using namespace MechMaterialData;
using namespace MechPropertyData;
using namespace MechCommon;
using namespace MxFunctions;
using namespace MechCaseData;
using namespace MechConnectorData;
using namespace MechStructOptionData;

namespace MechPreData
{
	QHash<PhysicalFieldType, QString> phyTypeToStr
	{
		{Physical_Part, "部件"},
		{Physical_Assembly, "装配"},
		{Physical_Material, "材料"},
		{Physical_FRCMaterial, "纤维增强复合材料"},
		{Physical_Case, "分析工况"},
		{Physical_LoadBoundary, "载荷边界"},
		{Physical_Set, "集合"},
		{Physical_Coordinate, "坐标系"},
		{Physical_Function, "函数"},
		{Physical_Property, "截面属性"},
		{Physical_PropertyShape, "截面形状"},
		{Physical_MeshPanel, "网格"},
		{Physical_Contact, "接触"},
		{Physical_Connector,"连接关系"},
		{Physical_OptimationDesign,"结构优化设计域"},
		{Physical_OptimationConstraint,"结构优化约束"},
        {Physical_OptimationGoal,"结构优化目标"},
        {Physical_FreqSamplPoint,"采样频率点"},
        {Physical_Laminate,"层合结构"},
        {Physical_GenericParam,"超单元参数" },
        {Physical_GenericAssign,"超单元调用" },
        {Physical_GenericMatrix,"超单元矩阵名称" },
        {Physical_MotionBody,"运动体"},
        {Physical_MotionJoint,"运动副"},
        {Physical_MotionCoupling,"耦合副"},
        {Physical_MotionDrive,"驱动"},
        {Physical_MotionVectorForce,"力元"},
        {Physical_MotionScenario,"方案"},
        // 多体其余类型：名称与模型树父节点文案一致
        {Physical_MotionMarker,"标记点"},
        {Physical_MotionContact,"接触"},
        {Physical_MotionSpring,"弹簧"},
        {Physical_MotionSensor,"传感器"},
        {Physical_MotionControlInput,"控制输入"},
        {Physical_MotionController,"PID控制器"},
        {Physical_MotionElectroImport,"机电导入"},
        {Physical_MotionElectroExport,"机电导出"},
        {Physical_MotionHydraulic,"液压协同"},
        {Physical_MotionRL,"强化学习"},
        {Physical_MotionAutoFlex,"自动柔性体"},
        { Physical_ModalContri,"模态贡献量" },
        { Physical_NodeContri,"节点贡献量" },
        { Physical_TPContri,"传递路径贡献量" },
        { Physical_PanelContri,"面板贡献量" },
        { Physical_ERP,"等效辐射功率" },
        { Physical_PeakFilter,"峰值筛选" },
        {Physical_GenericMatrix,"超单元矩阵名称" },
        {Physical_NonStructuralQuality, "非结构体质量"},
        {Physical_ASCouplingSearch, "声固耦合对搜索"},
        {Physical_Field, "场"},
        {Physical_PredefinedField, "预定义场"},
        {Physical_Deputy, "连接副"},
        {Physical_PeriodicBoundary, "周期性边界"},
        {Physical_ModelFix, "模型修改"}
 };


	void mDataStatusManager::clearAllData()
	{
		_normalInfoData.clear();
		_dynamicInfoData.clear();
	}
	void mDataStatusManager::clearNormalInfoData()
	{
		_normalInfoData.clear();
	}
	QList<outputInfoData>& mDataStatusManager::updataDynamicInfoData()
	{
		_dynamicInfoData.clear();
		return _dynamicInfoData;
	}
	QString mDataStatusManager::moduleOf(MxFunctions::PhysicalFieldType phyType)
	{
		return phyTypeToStr.value(phyType);
	}
	void mDataStatusManager::updateDynamicInfoData(MxFunctions::PhysicalFieldType phyType, int phyID)
	{
		if (!phyTypeToStr.contains(phyType))
			return;

		QVector<QPair<QString, QString>> info;
		QHash<QString, QSet<int>> deleteInfoData;
		QString module = phyTypeToStr[phyType];
		QString phyName = getCVM->getModelData()->getNameByPhyID(phyType, phyID);
		BrowserTreeStatus status = getCVM->getModelData()->getDataStatus(info, phyType, phyID);

		for (int i = _dynamicInfoData.size() - 1; i >= 0; --i)
		{
			if (_dynamicInfoData[i]._module == module && _dynamicInfoData[i]._ID == phyID)
			{
				_dynamicInfoData.removeAt(i);
			}
		}

		deleteInfoData[module].insert(phyID);
		QList<outputInfoData> tabelDynamic;
		AddListOutputInfoData(tabelDynamic, info, module, phyName, phyID);
		_dynamicInfoData += tabelDynamic;
	
		mMechGlobalSignals::getInstance()->updateBrowserTreeStatus(phyType, { { phyID, status } });
		mMechGlobalSignals::getInstance()->updateOutPutInfoTable(deleteInfoData, tabelDynamic);
	}
	void mDataStatusManager::updateDynamicInfoData(QHash<MxFunctions::PhysicalFieldType, std::vector<int>> phyData)
	{
		QHash<QString, QSet<int>> deleteInfoData;
		QList<outputInfoData> tabelDynamic;

		for (auto it = phyData.begin(); it != phyData.end(); ++it)
		{
			PhysicalFieldType phyType = it.key();
			if (!phyTypeToStr.contains(phyType))
				continue;

			QString module = phyTypeToStr[phyType];
			QHash<int, MxFunctions::BrowserTreeStatus> phyIDToStatus;
			for (auto& phyID : it.value())
			{
				QVector<QPair<QString, QString>> info;
				QString phyName = getCVM->getModelData()->getNameByPhyID(phyType, phyID);
				BrowserTreeStatus status = getCVM->getModelData()->getDataStatus(info, phyType, phyID);

				for (int i = _dynamicInfoData.size() - 1; i >= 0; --i)
				{
					if (_dynamicInfoData[i]._module == module && _dynamicInfoData[i]._ID == phyID)
					{
						_dynamicInfoData.removeAt(i);
					}
				}

				deleteInfoData[module].insert(phyID);
				AddListOutputInfoData(tabelDynamic, info, module, phyName, phyID);
				phyIDToStatus[phyID] = status;
			}

			mMechGlobalSignals::getInstance()->updateBrowserTreeStatus(phyType, phyIDToStatus);
		}
		_dynamicInfoData += tabelDynamic;
		mMechGlobalSignals::getInstance()->updateOutPutInfoTable(deleteInfoData, tabelDynamic);
	}
	void mDataStatusManager::deleteDynamicInfoData(MxFunctions::PhysicalFieldType phyType, std::vector<int> phyIDs)
	{
		if (!phyTypeToStr.contains(phyType))
			return;

		QString module = phyTypeToStr[phyType];
		QHash<QString, QSet<int>> deleteInfoData;

		for (int& id : phyIDs)
		{
			deleteInfoData[module].insert(id);
		}

		removeDynamicInfoData(module, phyIDs);
		getDeleteMapRelation(deleteInfoData);
	}
	void mDataStatusManager::resetName(MxFunctions::PhysicalFieldType phyType, const QString& oldName, const QString& newName, const int& ID)
	{
		if (!phyTypeToStr.contains(phyType))
			return;

		QString module = phyTypeToStr[phyType];
		for (int i = _dynamicInfoData.size() - 1; i >= 0; --i)
		{
			if (_dynamicInfoData[i]._module == module && _dynamicInfoData[i]._ID == ID)
			{
				_dynamicInfoData[i]._object = newName;
			}
		}

		getCVM->getModelData()->resetName(phyType, oldName, newName);
		mMechGlobalSignals::getInstance()->resetNameUpdateOutPutInfoTable(module, ID, newName);
	}

	void mDataStatusManager::readFileErrorInfo(const QString& filename)
	{
		QFileInfo finfo(filename);
		QFile file(filename);
		QList<outputInfoData> tabelDynamic;
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QString linetext;
			QTextStream reader(&file);
			reader.setCodec("UTF-8");
			while (!reader.atEnd())
			{
				linetext = reader.readLine();
				if (linetext.contains("*ERROR"))
				{
					linetext = reader.readLine();
					QString lineindex = mOtherFunction::getParenthesesContent(linetext);
					QString content;
					QStringList linelist = linetext.split(")");
					if (linelist.size() > 1)
					{
						QString str1 = linelist.at(1).simplified();
						QStringList str1list = str1.split(":");
						if (str1list.size() > 1)
						{
							content = str1list.at(1).simplified();
						}
						else
						{
							content = str1;
						}
					}
					else
					{
						content = linetext;
					}

					linetext = reader.readLine();

					outputInfoData data;
					data._type = "警告";
					data._module = "文件";
					data._object = content;
					data._explain = linetext;
					data._ID = -1;
					tabelDynamic.append(data);
					_normalInfoData.append(data);
				}
			}
		}

		mMechGlobalSignals::getInstance()->updataNormalInfo(tabelDynamic);
	}
	void mDataStatusManager::getDeleteMapRelation(QHash<QString, QSet<int>> deleteInfoData)
	{
		std::unordered_map<MxFunctions::PhysicalFieldType, std::set<int>> deleteDataMap = getCVM->getModelData()->getMapRelationManager().getDeleteDataMap();
		BrowserTreeStatus status = Tree_Yes;
		QList<outputInfoData> tabelDynamic;
        QString phyName;
		for (auto& it : deleteDataMap)
		{
			QHash<int, MxFunctions::BrowserTreeStatus> phyIDToStatus;
			vector<int> phyIDs;
            QString module = phyTypeToStr[it.first];
			switch (it.first)
			{
			case MxFunctions::Physical_Part:
			{
				mMeshData* meshData = getCVM->getModelData()->getMeshData();
				for (auto& id : it.second)
				{
					mPartData* part = meshData->getPartData(id);
					if (part == nullptr)
						continue;

                    phyName = part->getPartName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
				}
				break;
			}
			case MxFunctions::Physical_Material:
				break;
			case MxFunctions::Physical_Case:
				break;
			case MxFunctions::Physical_LoadBoundary:
			{
                mLoadBoundaryManager& loadboundaryManager = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
				for (auto& id : it.second)
				{
					shared_ptr<mLoadBoundaryBase> base = loadboundaryManager.getLoadBoundaryData(id);
					if (base == nullptr)
						continue;

					phyName = base->getLoadBoundaryName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
				}
				break;
			}
			case MxFunctions::Physical_Connector:
			{
				mConnectorManager& connectorManager = getCVM->getModelData()->getManager<mConnectorManager>(Physical_Connector);
				for (auto& id : it.second)
				{
					shared_ptr<mConnectorBase> base = connectorManager.getConnectorData(id);
					if (base == nullptr)
						continue;

					phyName = base->getConnectorName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
				}
				break;
			}
			case MxFunctions::Physical_Set:
				break;
			case MxFunctions::Physical_Coordinate:
				break;
			case MxFunctions::Physical_Function:
				break;
			case MxFunctions::Physical_Property:
			{
				mPropertyManager& proManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
				for (auto& id : it.second)
				{
					mPropertyBase* base = proManager.getPropertyData(id);
					if (base == nullptr)
						continue;

					phyName = base->getPropertyName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
				}
				break;
			}
            case MxFunctions::Physical_OptimationGoal:
            {
                if (getCVM->getModelData()->getStructOptionBase() == nullptr) break;

                int id = getCVM->getModelData()->getModelDataID();
                phyName = "优化目标";
                stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
                break;
            }
            case MxFunctions::Physical_OptimationDesign:
            {
                if (getCVM->getModelData()->getStructOptionBase() == nullptr) break;
                for (auto& id : it.second)
                {
                    mOptimizationConstraintData* base = getCVM->getModelData()->getStructOptionBase()->getOptimizationConstraintData(id);
                    if (base == nullptr)
                        continue;

                    phyName = base->getName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
                }
                break;
            }
            case MxFunctions::Physical_OptimationConstraint:
            {
                if (getCVM->getModelData()->getStructOptionBase() == nullptr) break;
                for (auto& id : it.second)
                {
                    mOptimizationDesignData* base = getCVM->getModelData()->getStructOptionBase()->getOptimizationDesignData(id);
                    if (base == nullptr)
                        continue;

                    phyName = base->getName();
                    stateInfoFunction(it.first, tabelDynamic, deleteInfoData, phyIDToStatus, phyIDs, phyName, id);
                }
                break;
            }
			case MxFunctions::Physical_PropertyShape:
				break;
			case MxFunctions::Physical_MeshPanel:
				break;
			case MxFunctions::Physical_Post:
				break;
			default:
				break;
			}

			removeDynamicInfoData(module, phyIDs);
			mMechGlobalSignals::getInstance()->updateBrowserTreeStatus(it.first, phyIDToStatus);
		}

		_dynamicInfoData += tabelDynamic;
		mMechGlobalSignals::getInstance()->updateOutPutInfoTable(deleteInfoData, tabelDynamic);
		getCVM->getModelData()->getMapRelationManager().clearDeleteDataMap();
	}
	void mDataStatusManager::removeDynamicInfoData(const QString& module, const vector<int>& phyIDs)
	{
		for (int i = _dynamicInfoData.size() - 1; i >= 0; --i)
		{
			if (_dynamicInfoData[i]._module == module &&
				std::find(phyIDs.begin(), phyIDs.end(), _dynamicInfoData[i]._ID) != phyIDs.end())
			{
				_dynamicInfoData.removeAt(i);
			}
		}
	}
    void mDataStatusManager::stateInfoFunction(MxFunctions::PhysicalFieldType phyType, QList<outputInfoData>& tabelDynamic, QHash<QString, QSet<int>>& deleteInfoData,
        QHash<int, MxFunctions::BrowserTreeStatus>& phyIDToStatus, std::vector<int>& phyIDs, const QString& phyName, int id)
    {
        QVector<QPair<QString, QString>> info;
        BrowserTreeStatus status = getCVM->getModelData()->getDataStatus(info, phyType, id);
        phyIDToStatus[id] = status;
        phyIDs.push_back(id);
        QString module = phyTypeToStr[phyType];
        AddListOutputInfoData(tabelDynamic, info, module, phyName, id);
        deleteInfoData[module].insert(id);
    }
	void mDataStatusManager::AddListOutputInfoData(QList<outputInfoData>& tabelDynamic, const QVector<QPair<QString, QString>>& info, const QString& module, const QString& phyName, const int& ID)
	{
		for (auto& its : info)
		{
			outputInfoData data;
			data._type = its.first;
			data._module = module;
			data._object = phyName;
			data._explain = its.second;
			data._ID = ID;
			tabelDynamic.append(data);
		}
	}
}