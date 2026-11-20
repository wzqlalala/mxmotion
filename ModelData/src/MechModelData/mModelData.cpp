#include "mModelData.h"
#include <QFile>
#include <QDir>
#include <QDataStream> //【S7】工程几何段
#include "mGeoEngineData.h" //【头链卫生】前向声明化后 cpp 自含（组合层本位）
#include "mGeoEngineServices.h" //【OCC摘除·S10/P0-1】engine() 同语法服务视图（零 OCC）
//#include "mGeoEngineAttach.h" //【OCC摘除·S10/P0-1】组合已下沉 OCCAdapter；该头携带后端 OCC 头，本仓禁含
#include "MXGEODataMgr.h"
#include "mMeshData.h"
#include "mDataElement.h"
#include "mGeoDataBase.h"
//#include "mGeoDataModelSet.h" //【OCC摘除·S9】partReName 已改 GN 元数据，不再需要完整类型（含 OCC 头）
#include "mGeoManager.h"
#include "mPropertyBase.h"
#include "mNonStructuralQuality.h"
#include "mMaterialBase.h"
#include "mLoadBoundaryBase.h"
#include "mLoadForce.h"
#include "mLoadPressure.h"
#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"
#include "mLoadSurface.h"
#include "mPredefinedFieldInitTemp.h"
#include "mPredefinedFieldTempLoad.h"
#include "mSetBase.h"
#include "mConnectorBase.h"
#include "mDeputyBase.h"
#include "mContactBase.h"
#include "mPostDataBase.h"
#include "mMixSet.h"
#include "mGeoSet.h"
#include "mElementSurface.h"
#include "mMotionBody.h"
#include "mMotionJointBase.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
#include "mMotionScenario.h"
#include "mMotionMarker.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include "mMotionScalarTorque.h"
#include "mMotionBushing.h"
#include "mMotionContact.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionBodySensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"
#include "mMotionPIDController.h"
#include "mMotionForceElementManager.h"
#include "mMotionCouplingGearRack.h"
#include "mGlobalSignals.h"
#include "mStructOption.h"
#include "mOptimizationDesignData.h"
#include "mOptimizationConstraintData.h"
#include "mRunnerOptimizationConstraintData.h"
#include "mRunnerOptimizationDesignData.h"
#include "mRunnerStructOption.h"
#include "mFreqSamplPointBase.h"
#include "mPlyLaminate.h"
#include "mGlobalParam.h"
#include "mGenericAssignData.h"
#include "mGenericMatrixData.h"
#include "mGenericParamData.h"
#include "mCaseImplicitFrequency.h"
#include "mCaseImplASCoupling.h"
#include "mPredefinedFieldBase.h"
#include "mRoatateAnalyseBase.h"
#include "mDiscreteField.h"
#include "mPeriodicBoundary.h"
#include "mPeriodicBoundaryManager.h"
#include "mModelFixDataBase.h"
#include "mBoundaryDisplacement.h"
#include "mConnectorBind.h"


using namespace MechMaterialData;
using namespace MechCaseData;
using namespace MechElementData;
using namespace MxFunctions;
using namespace MechMapRelation;
using namespace MechStructOptionData;
using namespace MechPropertyData;
using namespace MechGlobalParamData;
using namespace MechModelFixData;


namespace MechModelData
{
	int mModelData::_modelGlobalID = 1;

	void mModelData::resetModelGlobalID()
	{
		_modelGlobalID = 1;
	}

	mModelData::mModelData()
	{
		_modelDataID = _modelGlobalID++;
		_meshData = new mMeshData();
		_elementData = new mDataElement;
		_unitSystemManager = new mUnitSystemManager;
		_mGlobalParam = new mGlobalParam;
		_geoDataBase = new MGeoData::GeoDataBase(_meshData);
		_geoManager = new MGeoManager::GeoManager(_geoDataBase,_meshData);

		_geoEngineData = std::make_unique<GeoEngineConfig::GeoEngineData>(_meshData);
		_geoManager->setGeoPartMgr(&_geoEngineData->engine().context().PartManager());

		//【OCC摘除·S10/P0-1】engine() 即零 OCC 句柄（组合在 OCCAdapter 内完成），
		// 注入 MeshData 侧 MXGEODataMgr 与 GeoManager
		_meshData->getMXGEODataMgr()->attachEngine(_geoEngineData->engine());
		//【老类退役·S1】同一句柄注入 GeoManager：导入/导出主链改走引擎 dataExchange
		_geoManager->attachEngine(_geoEngineData->engine());
	}

	mModelData::~mModelData()
	{
		if (_meshData != nullptr)
		{
			delete _meshData;
			_meshData = nullptr;
		}

        if (_geoManager != nullptr)
        {
            delete _geoManager;
            _geoManager = nullptr;
        }

        if (_geoDataBase != nullptr)
        {
            delete _geoDataBase;
			_geoDataBase = nullptr;
        }

		if (_unitSystemManager != nullptr)
		{
			delete _unitSystemManager;
			_unitSystemManager = nullptr;
		}

		if (_mGlobalParam != nullptr)
		{
			delete _mGlobalParam;
			_mGlobalParam = nullptr;
		}

		if (_mStructOptionData != nullptr)
		{
			delete _mStructOptionData;
			_mStructOptionData = nullptr;
		}

		if (_mRunnerStructOptionData != nullptr)
		{
			delete _mRunnerStructOptionData;
			_mRunnerStructOptionData = nullptr;
		}
	}

	int mModelData::getModelDataID()
	{
		return _modelDataID;
	}

	void mModelData::setModelDataName(const QString& name)
	{
		_modelDataName = name;
	}

	QString mModelData::getModelDataName() const
	{
		return _modelDataName;
	}

	QString mModelData::createNameByType(MxFunctions::PhysicalFieldType type, const QString& text)
	{
		int cnt = 1;
		QString name = text + "_" + QString::number(cnt++);;
		while (isExistName(type, name))
		{
			name = text + "_" + QString::number(cnt++);
		}
		return name;
	}

	bool mModelData::isExistName(MxFunctions::PhysicalFieldType type, const QString & name, bool isInternal)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
			return _meshData->havePart(name);
		case MxFunctions::Physical_Material:
			return _materialManager.ExistMaterialName(name);
		case MxFunctions::Physical_Case:
			return _caseManager.ExistCaseName(name);
		case MxFunctions::Physical_LoadBoundary:
			return _loadboundaryManager.ExistLoadBoundaryName(name);
		case MxFunctions::Physical_PredefinedField:
			return _predefinedFieldManager.ExistPredefinedFieldName(name);
		case MxFunctions::Physical_RotateAnalyse:
			return _rotateAnalyseManager.ExistRotateAnalyseName(name);
		case MxFunctions::Physical_Connector:
			return _connectorManager.ExistConnectorName(name);
		case MxFunctions::Physical_Deputy:
			return _deputyManager.ExistDeputyName(name);
		case MxFunctions::Physical_Set:
			return _setManager.ExistSetName(name, isInternal);
		case MxFunctions::Physical_Coordinate:
			return _coordinateManager.ExistCoordinateName(name);
		case MxFunctions::Physical_Function:
			return _functionManager.ExistFunctionName(name);
		case MxFunctions::Physical_Property:
			return _propertyManager.ExistPropertyName(name);
		case MxFunctions::Physical_Contact:
			return _contactManager.ExistContactName(name);
		case MxFunctions::Physical_PeriodicBoundary:
			return _periodicBoundaryManager.ExistPeriodicBoundaryName(name);
		case MxFunctions::Physical_ModelFix:
			return _modelFixManager.ExistModelFixName(name);
		case MxFunctions::Physical_PropertyShape:
			return _sectionShapeManager.ExistSectionShapeName(name);
		case MxFunctions::Physical_MeshPanel:
			return _meshPanelManager.ExistMeshPanelName(name);
		case MxFunctions::Physical_MeshOptPanel:
			return _meshOptPanelManager.ExistMeshPanelName(name);
		case MxFunctions::Physical_OptimationDesign:
			return _mStructOptionData->existOptimizationDesignName(name);
		case MxFunctions::Physical_OptimationConstraint:
			return _mStructOptionData->existOptimizationConstraintName(name);
		case MxFunctions::Physical_RunnerOptimationDesign:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->existOptimizationDesignName(name) : false;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->existOptimizationConstraintName(name) : false;
		case MxFunctions::Physical_GenericParam:
			return _mGlobalParam->existGenericParamName(name);
		case MxFunctions::Physical_FreqSamplPoint:
			return _FreqSamplPointManager.ExistFreqSamplPointName(name);
		case MxFunctions::Physical_Laminate:
			return _LaminateManager.ExistLaminateName(name);
		case MxFunctions::Physical_PeakFilter:
			return _mGlobalParam->existPeakFilterName(name);
		case MxFunctions::Physical_NonStructuralQuality:
			return _nonStructuralQualityManager.ExistNsmName(name);
		case MxFunctions::Physical_Field:
			return _fieldManager.existFieldName(name, MxFunctions::FieldType::Discrete)
				|| _fieldManager.existFieldName(name, MxFunctions::FieldType::Analysis);
		case MxFunctions::Physical_Assembly:
			return _assemblyManager.existAssemblyName(name);
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.existName(name);
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.existName(name);
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.existName(name);
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.existName(name);
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.existName(name);
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.existName(name);
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.existName(name);
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.existName(name);
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.existName(name);
		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.existName(name);
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.existName(name);
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.existName(name);
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.existName(name);
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.existName(name);
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.existName(name);
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.existName(name);
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.existName(name);
		default:
			break;
		}

		return false;
	}

	QStringList mModelData::getAllNames(MxFunctions::PhysicalFieldType type, const int& caseID)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
			return _meshData->getAllPartNames();
		case MxFunctions::Physical_Material:
			return _materialManager.getAllMatNames();
		case MxFunctions::Physical_Case:
			return _caseManager.getAllCaseNames();
		case MxFunctions::Physical_LoadBoundary:
		{
			if (caseID > 0)
			{
				QStringList allName;
				std::set<int> all = _mapRelationManager.getMapRelation<std::set<int>>({ Physical_Case ,caseID }, Physical_LoadBoundary);
				for (auto id : all)
				{
					allName.append(_loadboundaryManager.getLoadBoundaryNameByID(id));
				}
				return allName;
			}
			else
			{
				return _loadboundaryManager.getAllLoadBoundaryName();
			}
		}
		case MxFunctions::Physical_Connector:
			return _connectorManager.getAllConnectorNames();
		case MxFunctions::Physical_Deputy:
			return _deputyManager.getAllDeputyNames();
		case MxFunctions::Physical_Set:
			return _setManager.getAllSetNames();
		case MxFunctions::Physical_Coordinate:
			return _coordinateManager.getAllCoordinateNames();
		case MxFunctions::Physical_Function:
			return _functionManager.getAllFunctionNames();
		case MxFunctions::Physical_Property:
			return _propertyManager.getAllPropertyNames();
		case MxFunctions::Physical_Contact:
			return _contactManager.getAllContactNames();
		case MxFunctions::Physical_PeriodicBoundary:
			return _periodicBoundaryManager.getAllPeriodicBoundaryNames();
		case MxFunctions::Physical_ModelFix:
			return _modelFixManager.getAllModelFixNames();
		case MxFunctions::Physical_OptimationDesign:
			return _mStructOptionData->getAllOptimizationDesignNames();
		case MxFunctions::Physical_OptimationConstraint:
			return _mStructOptionData->getAllOptimizationConstraintNames();
		case MxFunctions::Physical_RunnerOptimationDesign:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getAllOptimizationDesignNames() : QStringList{};
		case MxFunctions::Physical_RunnerOptimationConstraint:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getAllOptimizationConstraintNames() : QStringList{};
		case MxFunctions::Physical_GenericParam:
			return _mGlobalParam->getAllGenericParamNames();
		case MxFunctions::Physical_FreqSamplPoint:
			return _FreqSamplPointManager.getAllFreqSamplPointNames();
		case MxFunctions::Physical_Laminate:
			return _LaminateManager.getAllLaminateName();
		case MxFunctions::Physical_PredefinedField:
			return _predefinedFieldManager.getAllPredefinedFieldNames();
		case MxFunctions::Physical_RotateAnalyse:
			return _rotateAnalyseManager.getAllRotateAnalyseNames();
		case MxFunctions::Physical_PeakFilter:
			return _mGlobalParam->getAllPeakFilterNames();
		case MxFunctions::Physical_NonStructuralQuality:
			return _nonStructuralQualityManager.getAllNsmNames();
		case MxFunctions::Physical_Field:
			return _fieldManager.getAllFieldNames(MxFunctions::FieldType::Discrete)
				+ _fieldManager.getAllFieldNames(MxFunctions::FieldType::Analysis);
		case MxFunctions::Physical_Assembly:
			return _assemblyManager.getAllAssemblyNames();
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.getAllNames();
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.getAllNames();
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.getAllNames();
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.getAllNames();
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.getAllNames();
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.getAllNames();
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.getAllNames();
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.getAllNames();
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.getAllNames();
		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.getAllNames();
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.getAllNames();
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.getAllNames();
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.getAllNames();
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.getAllNames();
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.getAllNames();
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.getAllNames();
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.getAllNames();
		default:
			break;
		}

		return {};
	}

	std::vector<int> mModelData::getAllPhyID(MxFunctions::PhysicalFieldType type, const int& caseID)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
			return _meshData->getAllPartIds();
		case MxFunctions::Physical_Material:
			return _materialManager.getAllMatID();
		case MxFunctions::Physical_Case:
			return _caseManager.getAllCaseID();
		case MxFunctions::Physical_LoadBoundary:
		{
			if (caseID > 0)
			{
				std::vector<int> allID;
				std::set<int> all = _mapRelationManager.getMapRelation<std::set<int>>({ Physical_Case ,caseID }, Physical_LoadBoundary);
				for (auto id : all)
				{
					allID.push_back(id);
				}
				return allID;
			}
			else
			{
				return _loadboundaryManager.getAllLoadBoundaryID();
			}
		}
		case MxFunctions::Physical_Connector:
			return _connectorManager.getAllConnectorID();
		case MxFunctions::Physical_Deputy:
			return _deputyManager.getAllDeputyID();
		case MxFunctions::Physical_Set:
			return _setManager.getAllSetID();
		case MxFunctions::Physical_Coordinate:
			return _coordinateManager.getAllCoordinateID();
		case MxFunctions::Physical_Function:
			return _functionManager.getAllFunctionID();
		case MxFunctions::Physical_Property:
			return _propertyManager.getAllProperyID();
		case MxFunctions::Physical_Contact:
			return _contactManager.getAllContactID();
		case MxFunctions::Physical_ModelFix:
			return _modelFixManager.getAllModelFixID();
		case MxFunctions::Physical_PeriodicBoundary:
			return _periodicBoundaryManager.getAllPeriodicBoundaryID();
		case MxFunctions::Physical_PropertyShape:
			return _sectionShapeManager.getAllSectionShapeID();
		case MxFunctions::Physical_MeshPanel:
			return _meshPanelManager.getAllMeshPanelID();
		case MxFunctions::Physical_MeshOptPanel:
			return _meshOptPanelManager.getAllMeshPanelID();
		case MxFunctions::Physical_OptimationDesign:
			return _mStructOptionData->getAllOptimizationDesignIDs();
		case MxFunctions::Physical_OptimationConstraint:
			return _mStructOptionData->getAllOptimizationConstraintIDs();
		case MxFunctions::Physical_RunnerOptimationDesign:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getAllOptimizationDesignIDs() : std::vector<int>{};
		case MxFunctions::Physical_RunnerOptimationConstraint:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getAllOptimizationConstraintIDs() : std::vector<int>{};
		case MxFunctions::Physical_GenericParam:
			return _mGlobalParam->getAllGenericParamIDs();
		case MxFunctions::Physical_SolveControl:
		{
			if (_mGlobalParam->getSolveControlData())
				return { 0 };
			return {};
		}
		case MxFunctions::Physical_FreqSamplPoint:
			return _FreqSamplPointManager.getAllFreqSamplPointID();
		case MxFunctions::Physical_Laminate:
			return _LaminateManager.getAllLaminateID();
		case MxFunctions::Physical_PredefinedField:
			return _predefinedFieldManager.getAllPredefinedFieldID();
		case MxFunctions::Physical_RotateAnalyse:
			return _rotateAnalyseManager.getAllRotateAnalyseID();
		case MxFunctions::Physical_PeakFilter:
			return _mGlobalParam->getAllPeakFilterIDs();
		case MxFunctions::Physical_NonStructuralQuality:
			return _nonStructuralQualityManager.getAllNonSturcturalQualityID();
		case MxFunctions::Physical_Field:
		{
			std::vector<int> ids = _fieldManager.getAllFieldID(MxFunctions::FieldType::Discrete);
			std::vector<int> analysisIds = _fieldManager.getAllFieldID(MxFunctions::FieldType::Analysis);
			ids.insert(ids.end(), analysisIds.begin(), analysisIds.end());
			return ids;
		}
		case MxFunctions::Physical_Assembly:
		{
			const QVector<int> ids = _assemblyManager.getAllAssemblyIDs();
			return std::vector<int>(ids.begin(), ids.end());
		}
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.getAllIDs();
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.getAllIDs();
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.getAllIDs();
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.getAllIDs();
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.getAllIDs();
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.getAllIDs();
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.getAllIDs();
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.getAllIDs();
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.getAllIDs();
		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.getAllIDs();
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.getAllIDs();
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.getAllIDs();
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.getAllIDs();
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.getAllIDs();
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.getAllIDs();
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.getAllIDs();
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.getAllIDs();
		default:
			break;
		}
		return {};
	}

	QString mModelData::getNameByPhyID(MxFunctions::PhysicalFieldType type, int id)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
			return _meshData->getPartNameByPartId(id);
		case MxFunctions::Physical_Material:
			return _materialManager.getMatNameByID(id);
		case MxFunctions::Physical_Case:
			return _caseManager.getCaseNameByID(id);
		case MxFunctions::Physical_LoadBoundary:
			return _loadboundaryManager.getLoadBoundaryNameByID(id);
		case MxFunctions::Physical_Connector:
			return _connectorManager.getConnectorNameByID(id);
		case MxFunctions::Physical_Deputy:
			return _deputyManager.getDeputyNameByID(id);
		case MxFunctions::Physical_Set:
			return _setManager.getSetNameByID(id);
		case MxFunctions::Physical_Coordinate:
			return _coordinateManager.getCoordinateNameByID(id);
		case MxFunctions::Physical_Function:
			return _functionManager.getFunctionNameByID(id);
		case MxFunctions::Physical_Property:
			return _propertyManager.getPropertyNameByID(id);
		case MxFunctions::Physical_Contact:
			return _contactManager.getContactNameByID(id);
		case MxFunctions::Physical_ModelFix:
			return _modelFixManager.getModelFixNameByID(id);
		case MxFunctions::Physical_PeriodicBoundary:
			return _periodicBoundaryManager.getPeriodicBoundaryNameByID(id);
		case MxFunctions::Physical_PropertyShape:
			return _sectionShapeManager.getSectionShapeNameByID(id);
		case MxFunctions::Physical_MeshPanel:
			return _meshPanelManager.getMeshPanelNameByID(id);
		case MxFunctions::Physical_OptimationGoal:
			return "优化目标";
		case MxFunctions::Physical_MeshOptPanel:
			return _meshOptPanelManager.getMeshPanelNameByID(id);
		case MxFunctions::Physical_OptimationDesign:
			return _mStructOptionData->getOptimizationDesignNameByID(id);
		case MxFunctions::Physical_OptimationConstraint:
			return _mStructOptionData->getOptimizationConstraintNameByID(id);
		case MxFunctions::Physical_RunnerOptimationGoal:
			return "流道优化目标";
		case MxFunctions::Physical_RunnerOptimationDesign:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationDesignNameByID(id) : "";
		case MxFunctions::Physical_RunnerOptimationConstraint:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationConstraintNameByID(id) : "";
		case MxFunctions::Physical_GenericParam:
			return _mGlobalParam->getGenericParamNameByID(id);
		case MxFunctions::Physical_GenericAssign:
			return "超单元调用";
		case MxFunctions::Physical_GenericMatrix:
			return "超单元矩阵";
		case MxFunctions::Physical_SolveControl:
			return "求解控制";
		case MxFunctions::Physical_FreqSamplPoint:
			return _FreqSamplPointManager.getFreqSamplPointNameByID(id);
		case MxFunctions::Physical_Laminate:
			return _LaminateManager.getLaminateNameByID(id);
		case MxFunctions::Physical_PredefinedField:
			return _predefinedFieldManager.getPredefinedFieldNameByID(id);
		case MxFunctions::Physical_RotateAnalyse:
			return _rotateAnalyseManager.getRotateAnalyseNameByID(id);
		case MxFunctions::Physical_ModalContri:
			return "模态贡献量";
		case MxFunctions::Physical_NodeContri:
			return "节点贡献量";
		case MxFunctions::Physical_TPContri:
			return "传递路径贡献量";
		case MxFunctions::Physical_PanelContri:
			return "面板贡献量";
		case MxFunctions::Physical_ERP:
			return "等效辐射功率";
		case MxFunctions::Physical_PeakFilter:
			return _mGlobalParam->getPeakFilterNameByID(id);
		case MxFunctions::Physical_NonStructuralQuality:
			return _nonStructuralQualityManager.getNonStructuralQualityNameByID(id);
		case MxFunctions::Physical_Field:
			return _fieldManager.getFieldNameByID(id);
		case MxFunctions::Physical_Assembly:
			return _assemblyManager.getAssemblyName(id);
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.getNameByID(id);
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.getNameByID(id);
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.getNameByID(id);
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.getNameByID(id);
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.getNameByID(id);
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.getNameByID(id);
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.getNameByID(id);
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.getNameByID(id);
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.getNameByID(id);
		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.getNameByID(id);
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.getNameByID(id);
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.getNameByID(id);
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.getNameByID(id);
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.getNameByID(id);
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.getNameByID(id);
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.getNameByID(id);
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.getNameByID(id);
		default:
			break;
		}
		return "";
	}

	int mModelData::getPhyIDByName(MxFunctions::PhysicalFieldType type, QString name, bool isContainAutoCreate)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
			return _meshData->getPartIdByPartName(name);
		case MxFunctions::Physical_Material:
			return _materialManager.getIDByMatName(name);
		case MxFunctions::Physical_Case:
			return _caseManager.getIDByCaseName(name);
		case MxFunctions::Physical_LoadBoundary:
			return _loadboundaryManager.getIDByLoadBoundaryName(name);
		case MxFunctions::Physical_Connector:
			return _connectorManager.getIDByConnectorName(name);
		case MxFunctions::Physical_Contact:
			return _contactManager.getContactIdByName(name);
		case MxFunctions::Physical_ModelFix:
			return _modelFixManager.getIDByModelFixName(name);
		case MxFunctions::Physical_PeriodicBoundary:
			return _periodicBoundaryManager.getPeriodicBoundaryIdByName(name);
		case MxFunctions::Physical_Deputy:
			return _deputyManager.getIDByDeputyName(name);
		case MxFunctions::Physical_Set:
			return _setManager.getIDBySetName(name, isContainAutoCreate);
		case MxFunctions::Physical_Coordinate:
			return _coordinateManager.getIDByCoordinateName(name);
		case MxFunctions::Physical_Function:
			return _functionManager.getIDByFunctionName(name);
		case MxFunctions::Physical_Property:
			return _propertyManager.getIDByPropertyName(name);
		case MxFunctions::Physical_PropertyShape:
			return _sectionShapeManager.getIDByPropertyShapeName(name);
		case MxFunctions::Physical_MeshPanel:
			return _meshPanelManager.getIDByMeshPanelName(name);
		case MxFunctions::Physical_MeshOptPanel:
			return _meshOptPanelManager.getIDByMeshPanelName(name);
		case MxFunctions::Physical_OptimationDesign:
			return _mStructOptionData->getOptimizationDesignIDByName(name);
		case MxFunctions::Physical_OptimationConstraint:
			return _mStructOptionData->getOptimizationConstraintIDByName(name);
		case MxFunctions::Physical_RunnerOptimationDesign:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationDesignIDByName(name) : -1;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			return _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationConstraintIDByName(name) : -1;
		case MxFunctions::Physical_GenericParam:
			return _mGlobalParam->getGenericParamIDByName(name);
		case MxFunctions::Physical_SolveControl:
			return _mGlobalParam->getSolveControlData() ? 0 : -1;
		case MxFunctions::Physical_FreqSamplPoint:
			return _FreqSamplPointManager.getIDByFreqSamplPointName(name);
		case MxFunctions::Physical_Laminate:
			return _LaminateManager.getIDByLaminateName(name);
		case MxFunctions::Physical_PredefinedField:
			return _predefinedFieldManager.getIDByPredefinedFieldName(name);
		case MxFunctions::Physical_RotateAnalyse:
			return _rotateAnalyseManager.getIDByRotateAnalyseName(name);
		case MxFunctions::Physical_PeakFilter:
			return _mGlobalParam->getPeakFilterIDByName(name);
		case MxFunctions::Physical_NonStructuralQuality:
			return _nonStructuralQualityManager.getIDByNsmName(name);
		case MxFunctions::Physical_Field:
			return _fieldManager.getIDByFieldName(name);
		case MxFunctions::Physical_Assembly:
			return _assemblyManager.getIDByAssemblyName(name);
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.getIDByName(name);
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.getIDByName(name);
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.getIDByName(name);
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.getIDByName(name);
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.getIDByName(name);
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.getIDByName(name);
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.getIDByName(name);
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.getIDByName(name);
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.getIDByName(name);
		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.getIDByName(name);
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.getIDByName(name);
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.getIDByName(name);
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.getIDByName(name);
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.getIDByName(name);
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.getIDByName(name);
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.getIDByName(name);
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.getIDByName(name);
		default:
			break;
		}
		return -1;
	}

	void mModelData::resetName(MxFunctions::PhysicalFieldType type, const QString& oldName, const QString& newName)
	{
		switch (type)
		{
		case MxFunctions::Physical_Part:
		{
			_meshData->partReName(oldName, newName);
			//【OCC摘除·S9】旧栈 set 改名随冻结岛退役，几何侧改名走引擎 GN 部件元数据
			/*MGeoData::GeoDataModelSet* geoSet = _geoDataBase->getSetByPartName(oldName);
			if (geoSet == nullptr)
				return;
			QString name = newName;
			geoSet->setName(name, true);*/
			_geoEngineData->engine().context().PartManager().resetName(oldName, newName);
			break;
		}
		case MxFunctions::Physical_Material:
			_materialManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Case:
			_caseManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_LoadBoundary:
			_loadboundaryManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Connector:
			_connectorManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Deputy:
			_deputyManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Set:
			_setManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Coordinate:
			_coordinateManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Property:
			_propertyManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Contact:
			_contactManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_PeriodicBoundary:
			_periodicBoundaryManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Function:
			_functionManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_PropertyShape:
			_sectionShapeManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MeshPanel:
			_meshPanelManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MeshOptPanel:
			_meshOptPanelManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_OptimationDesign:
		    _mStructOptionData->resetOptimizationDesignName(oldName, newName);
			break;
		case MxFunctions::Physical_OptimationConstraint:
			_mStructOptionData->resetOptimizationConstraintName(oldName, newName);
			break;
		case MxFunctions::Physical_RunnerOptimationDesign:
			if (_mRunnerStructOptionData) _mRunnerStructOptionData->resetOptimizationDesignName(oldName, newName);
			break;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			if (_mRunnerStructOptionData) _mRunnerStructOptionData->resetOptimizationConstraintName(oldName, newName);
			break;
		case MxFunctions::Physical_GenericParam:
			_mGlobalParam->resetGenericParamName(oldName, newName);
			break;
		case MxFunctions::Physical_FreqSamplPoint:
			_FreqSamplPointManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Laminate:
			_LaminateManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_PredefinedField:
			_predefinedFieldManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_RotateAnalyse:
			_rotateAnalyseManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_ModelFix:
			_modelFixManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_PeakFilter:
			_mGlobalParam->resetPeakFilterName(oldName, newName);
			break;
		case MxFunctions::Physical_NonStructuralQuality:
			_nonStructuralQualityManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_Field:
		{
			//根据旧名称命中子类型后重命名
			int id = _fieldManager.getIDByFieldName(oldName);
			MechDiscreteFieldData::mFieldBase* field = _fieldManager.getFieldData(id);
			if (field != nullptr)
				_fieldManager.resetName(oldName, newName, field->getFieldType());
			break;
		}
		case MxFunctions::Physical_Assembly:
			_assemblyManager.renameAssembly(_assemblyManager.getIDByAssemblyName(oldName), newName);
		case MxFunctions::Physical_MotionBody:
			_motionBodyManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionJoint:
			_motionJointManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionCoupling:
			_motionCouplingManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionDrive:
			_motionDriveManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionScenario:
			_motionScenarioManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionMarker:
			_motionMarkerManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionVectorForce:
			_motionForceElementManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionContact:
			_motionContactManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionSpring:
			_motionSpringManager.resetName(oldName, newName);
			break;
			break;
		case MxFunctions::Physical_MotionControlInput:
			_motionControlInputManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionElectroImport:
			_motionElectroImportManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionElectroExport:
			_motionElectroExportManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionHydraulic:
			_motionHydraulicManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionRL:
			_motionRLManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionSensor:
			_motionSensorManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionController:
			_motionControllerManager.resetName(oldName, newName);
			break;
		case MxFunctions::Physical_MotionAutoFlex:
			_motionAutoFlexManager.resetName(oldName, newName);
			break;
		default:
			break;
		}
	}

	int mModelData::copyPhysicalField(MxFunctions::PhysicalFieldType type, const int & id, const int& caseID)
	{
		int copyId = -1;
		switch (type)
		{
		case MxFunctions::Physical_Material:
			copyId = _materialManager.copyMaterialData(id);
			break;
		case MxFunctions::Physical_Case:
		{
			copyId = _caseManager.copyCaseData(id);
			if (copyId != -1)
			{
				// 同步拷贝该工况关联的 Physical_RotateAnalyse 数据及其映射
				std::set<int> rotateIDs = _mapRelationManager.getMapRelation<std::set<int>>(
					{ Physical_Case, id }, Physical_RotateAnalyse);
				std::map<int, int> oldToNewRotateID;
				for (int rotateID : rotateIDs)
				{
					int newRotateID = _rotateAnalyseManager.copyRotateAnalyseData(rotateID);
					if (newRotateID != 0)
					{
						oldToNewRotateID[rotateID] = newRotateID;
						_mapRelationManager.appendMapRelation(
							{ Physical_RotateAnalyse, newRotateID }, { Physical_Case, copyId });
					}
				}
				// 拷贝 Physical_RotateAnalyse -> Physical_Set 映射
				for (auto& kv : oldToNewRotateID)
				{
					std::set<int> setIDs = _mapRelationManager.getMapRelation<std::set<int>>(
						{ Physical_RotateAnalyse, kv.first }, Physical_Set);
					for (int setID : setIDs)
					{
						_mapRelationManager.appendMapRelation(
							{ Physical_RotateAnalyse, kv.second }, { Physical_Set, setID });
					}
				}
			}
		}
			break;
		case MxFunctions::Physical_LoadBoundary:
			copyId = _loadboundaryManager.copyLoadBoundaryData(id);
			break;
		case MxFunctions::Physical_Connector:
			copyId = _connectorManager.copyConnectorData(id);
			break;
		case MxFunctions::Physical_Deputy:
			copyId = _deputyManager.copyDeputyData(id);
			break;
		case MxFunctions::Physical_Set:
			copyId = _setManager.copySetData(id);
			break;
		case MxFunctions::Physical_Coordinate:
			copyId = _coordinateManager.copyCoordinateData(id);
			break;
		case MxFunctions::Physical_Property:
			copyId = _propertyManager.copyPropertyData(id);
			break;
		case MxFunctions::Physical_Contact:
			copyId = _contactManager.copyContactData(id);
			break;
		case MxFunctions::Physical_PeriodicBoundary:
			copyId = _periodicBoundaryManager.copyPeriodicBoundaryData(id);
			break;
		case MxFunctions::Physical_ModelFix:
			copyId = _modelFixManager.copyModelFixData(id);
			break;
		case MxFunctions::Physical_Function:
			copyId = _functionManager.copyFunctionData(id);
			break;
		case MxFunctions::Physical_PropertyShape:
			copyId = _sectionShapeManager.copySectionShapeData(id);
			break;
		case MxFunctions::Physical_OptimationDesign:
			copyId = _mStructOptionData->copyOptimizationDesignData(id);
			break;
		case MxFunctions::Physical_OptimationConstraint:
			copyId = _mStructOptionData->copyOptimizationConstraintData(id);
			break;
		case MxFunctions::Physical_RunnerOptimationDesign:
			copyId = _mRunnerStructOptionData ? _mRunnerStructOptionData->copyOptimizationDesignData(id) : -1;
			break;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			copyId = _mRunnerStructOptionData ? _mRunnerStructOptionData->copyOptimizationConstraintData(id) : -1;
			break;
		case MxFunctions::Physical_GenericParam:
			copyId = _mGlobalParam->copyGenericParamData(id);
			break;
		case MxFunctions::Physical_FreqSamplPoint:
			copyId = _FreqSamplPointManager.copyFreqSamplPointData(id);
			break;
		case MxFunctions::Physical_Laminate:
			copyId = _LaminateManager.copyLaminateData(id);
			break;
		case MxFunctions::Physical_PredefinedField:
			copyId = _predefinedFieldManager.copyPredefinedFieldData(id);
			break;
		case MxFunctions::Physical_RotateAnalyse:
			copyId = _rotateAnalyseManager.copyRotateAnalyseData(id);
			break;
		case MxFunctions::Physical_PeakFilter:
			copyId = _mGlobalParam->copyPeakFilterData(id);
			break;
		case MxFunctions::Physical_NonStructuralQuality:
			copyId = _nonStructuralQualityManager.copyNsmData(id);
			break;
		case MxFunctions::Physical_Field:
			copyId = _fieldManager.copyFieldData(id);
			break;
		case MxFunctions::Physical_MotionBody:
			return _motionBodyManager.copyData(id);
		case MxFunctions::Physical_MotionJoint:
			return _motionJointManager.copyData(id);
		case MxFunctions::Physical_MotionCoupling:
			return _motionCouplingManager.copyData(id);
		case MxFunctions::Physical_MotionDrive:
			return _motionDriveManager.copyData(id);
		case MxFunctions::Physical_MotionScenario:
			return _motionScenarioManager.copyData(id);
		case MxFunctions::Physical_MotionMarker:
			return _motionMarkerManager.copyData(id);
		case MxFunctions::Physical_MotionVectorForce:
			return _motionForceElementManager.copyData(id);
		case MxFunctions::Physical_MotionContact:
			return _motionContactManager.copyData(id);
		case MxFunctions::Physical_MotionSpring:
			return _motionSpringManager.copyData(id);

		case MxFunctions::Physical_MotionControlInput:
			return _motionControlInputManager.copyData(id);
		case MxFunctions::Physical_MotionElectroImport:
			return _motionElectroImportManager.copyData(id);
		case MxFunctions::Physical_MotionElectroExport:
			return _motionElectroExportManager.copyData(id);
		case MxFunctions::Physical_MotionHydraulic:
			return _motionHydraulicManager.copyData(id);
		case MxFunctions::Physical_MotionRL:
			return _motionRLManager.copyData(id);
		case MxFunctions::Physical_MotionSensor:
			return _motionSensorManager.copyData(id);
		case MxFunctions::Physical_MotionController:
			return _motionControllerManager.copyData(id);
		case MxFunctions::Physical_MotionAutoFlex:
			return _motionAutoFlexManager.copyData(id);
		default:
			break;
		}

		//映射数据拷贝
		if(copyId != -1)  _mapRelationManager.copyMapRelation(type, id, copyId, caseID);
		return copyId;
	}

	void mModelData::deleteAllByPhysicalType(MxFunctions::PhysicalFieldType type, const int& caseID)
	{
		vector<int> phyIDs;
		switch (type)
		{
		case MxFunctions::Physical_Material:
			phyIDs = _materialManager.getAllMatID();
			_materialManager.deleteAllMat();
			break;
		case MxFunctions::Physical_Connector:
		{
			phyIDs = _connectorManager.getAllConnectorID();
			std::vector<int>allnodes = _connectorManager.getAllNodesInConn();
			_meshData->deleteConnectNodes(allnodes);
			_connectorManager.deleteAllConnector();
		}
			break;
		case MxFunctions::Physical_Deputy:
		{
			phyIDs = _deputyManager.getAllDeputyID();
			std::vector<int>allnodes = _deputyManager.getAllNodesInDeputy();
			_meshData->deleteConnectNodes(allnodes);
			_deputyManager.deleteAllDeputy();
		}
			break;
		case MxFunctions::Physical_Set:
			phyIDs = _setManager.getAllSetID();
			_setManager.deleteAllSet();
			break;
		case MxFunctions::Physical_Coordinate:
			phyIDs = _coordinateManager.getAllCoordinateID();
			_coordinateManager.deleteAllCoordinate();
			break;
		case MxFunctions::Physical_Property:
			phyIDs = _propertyManager.getAllProperyID();
			_propertyManager.deleteAllProperty();
			break;
		case MxFunctions::Physical_Contact:
			phyIDs = _contactManager.getAllContactID();
			_contactManager.deleteAllContact();
			break;
		case MxFunctions::Physical_ModelFix:
			phyIDs = _modelFixManager.getAllModelFixID();
			_modelFixManager.deleteAllModelFix();
			break;
		case MxFunctions::Physical_PeriodicBoundary:
			phyIDs = _periodicBoundaryManager.getAllPeriodicBoundaryID();
			_periodicBoundaryManager.deleteAllPeriodicBoundary();
			break;
		case MxFunctions::Physical_Function:
			phyIDs = _functionManager.getAllFunctionID();
			_functionManager.deleteAllFunction();
			break;
		case MxFunctions::Physical_MeshPanel:
			_meshPanelManager.deleteAllMeshPanel();
			break;
		case MxFunctions::Physical_PropertyShape:
			phyIDs = _sectionShapeManager.getAllSectionShapeID();
			_sectionShapeManager.deleteAllSectionShape();
			break;
		case MxFunctions::Physical_MeshOptPanel:
			_meshOptPanelManager.deleteAllMeshPanel();
			break;
		case MxFunctions::Physical_OptimationDesign:
			phyIDs = _mStructOptionData->getAllOptimizationDesignIDs();
			_mStructOptionData->deleteAllOptimizationDesignData();
			break;
		case MxFunctions::Physical_OptimationConstraint:
			phyIDs = _mStructOptionData->getAllOptimizationConstraintIDs();
			_mStructOptionData->deleteAllOptimizationConstraintData();
			break;
		case MxFunctions::Physical_FreqSamplPoint:
			phyIDs = _FreqSamplPointManager.getAllFreqSamplPointID();
			_FreqSamplPointManager.deleteAllFreqSamplPoint();
			break;
		case MxFunctions::Physical_Laminate:
			phyIDs = _LaminateManager.getAllLaminateID();
			_LaminateManager.deleteAllLaminate();
			break;
		case MxFunctions::Physical_PredefinedField:
			phyIDs = _predefinedFieldManager.getAllPredefinedFieldID();
			_predefinedFieldManager.deleteAllPredefinedField();
			break;
		case MxFunctions::Physical_RotateAnalyse:
			phyIDs = _rotateAnalyseManager.getAllRotateAnalyseID();
			_rotateAnalyseManager.deleteAllRotateAnalyse();
			break;
		case MxFunctions::Physical_RunnerOptimationDesign:
			phyIDs = _mRunnerStructOptionData->getAllOptimizationDesignIDs();
			_mRunnerStructOptionData->deleteAllOptimizationDesignData();
			break;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			phyIDs = _mRunnerStructOptionData->getAllOptimizationConstraintIDs();
			_mRunnerStructOptionData->deleteAllOptimizationConstraintData();
			break;
		case MxFunctions::Physical_GenericParam:
			phyIDs = _mGlobalParam->getAllGenericParamIDs();
			_mGlobalParam->deleteAllGenericParamData();
			break;
		case MxFunctions::Physical_PeakFilter:
			phyIDs = _mGlobalParam->getAllPeakFilterIDs();
			_mGlobalParam->deleteAllPeakFilterData();
			break;
		case MxFunctions::Physical_NonStructuralQuality:
			phyIDs = _nonStructuralQualityManager.getAllNonSturcturalQualityID();
			_nonStructuralQualityManager.deleteAllNsm();
			break;
		case MxFunctions::Physical_Field:
		{
			phyIDs = _fieldManager.getAllFieldID(MxFunctions::FieldType::Discrete);
			std::vector<int> analysisIds = _fieldManager.getAllFieldID(MxFunctions::FieldType::Analysis);
			phyIDs.insert(phyIDs.end(), analysisIds.begin(), analysisIds.end());
			_fieldManager.deleteAllField(MxFunctions::FieldType::Discrete);
			_fieldManager.deleteAllField(MxFunctions::FieldType::Analysis);
			break;
		}
		case MxFunctions::Physical_Assembly:
			_assemblyManager.clear();
			break;
		case MxFunctions::Physical_MotionBody:
			phyIDs = _motionBodyManager.getAllIDs();
			_motionBodyManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionJoint:
			phyIDs = _motionJointManager.getAllIDs();
			_motionJointManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionCoupling:
			phyIDs = _motionCouplingManager.getAllIDs();
			_motionCouplingManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionDrive:
			phyIDs = _motionDriveManager.getAllIDs();
			_motionDriveManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionScenario:
			phyIDs = _motionScenarioManager.getAllIDs();
			_motionScenarioManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionMarker:
			phyIDs = _motionMarkerManager.getAllIDs();
			_motionMarkerManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionVectorForce:
			phyIDs = _motionForceElementManager.getAllIDs();
			_motionForceElementManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionContact:
			phyIDs = _motionContactManager.getAllIDs();
			_motionContactManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionSpring:
			phyIDs = _motionSpringManager.getAllIDs();
			_motionSpringManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionControlInput:
			phyIDs = _motionControlInputManager.getAllIDs();
			_motionControlInputManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionElectroImport:
			phyIDs = _motionElectroImportManager.getAllIDs();
			_motionElectroImportManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionElectroExport:
			phyIDs = _motionElectroExportManager.getAllIDs();
			_motionElectroExportManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionHydraulic:
			phyIDs = _motionHydraulicManager.getAllIDs();
			_motionHydraulicManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionRL:
			phyIDs = _motionRLManager.getAllIDs();
			_motionRLManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionSensor:
			phyIDs = _motionSensorManager.getAllIDs();
			_motionSensorManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionController:
			phyIDs = _motionControllerManager.getAllIDs();
			_motionControllerManager.deleteAll();
			break;
		case MxFunctions::Physical_MotionAutoFlex:
			phyIDs = _motionAutoFlexManager.getAllIDs();
			_motionAutoFlexManager.deleteAll();
			break;
		default:
			break;
		}

		for (int id : phyIDs)
		{
			_mapRelationManager.deleteMapRelation({ type ,id });
		}
	}

	void mModelData::deleteByPhysicalType(MxFunctions::PhysicalFieldType type, const int& id, const int& caseID)
	{
		switch (type)
		{
		case MxFunctions::Physical_Material:
			_materialManager.deleteMat(id);
			break;
		case MxFunctions::Physical_Case:
		{
			_caseManager.deleteCase(id);
			// 同步删除该工况关联的 Physical_RotateAnalyse 数据对象
			std::set<int> rotateIDs = _mapRelationManager.getMapRelation<std::set<int>>(
				{ Physical_Case, id }, Physical_RotateAnalyse);
			for (int rotateID : rotateIDs)
			{
				_mapRelationManager.deleteMapRelation({ Physical_RotateAnalyse, rotateID });
				_rotateAnalyseManager.deleteRotateAnalyse(rotateID);
			}
		}
		break;
		case MxFunctions::Physical_LoadBoundary:
			_loadboundaryManager.deleteLoadBoundary(id);
			break;
		case MxFunctions::Physical_Connector:
		{
			std::vector<int>allnodes = _connectorManager.getAllNodesInConn(id);
			_meshData->deleteConnectNodes(allnodes);
			_connectorManager.deleteConnector(id);
		}
			break;
		case MxFunctions::Physical_Deputy:
		{
			std::vector<int>allnodes = _deputyManager.getAllNodesInDeputy(id);
			_meshData->deleteConnectNodes(allnodes);
			_deputyManager.deleteDeputy(id);
		}
			break;
		case MxFunctions::Physical_Set:
			_setManager.deleteSet(id);
			break;
		case MxFunctions::Physical_Coordinate:
			_coordinateManager.deleteCoordinate(id);
			break;
		case MxFunctions::Physical_Property:
			_propertyManager.deleteProperty(id);
			break;
		case MxFunctions::Physical_Contact:
			_contactManager.deleteContact(id);
			break;
		case MxFunctions::Physical_ModelFix:
			_modelFixManager.deleteModelFix(id);
			break;
		case MxFunctions::Physical_PeriodicBoundary:
			_periodicBoundaryManager.deletePeriodicBoundary(id);
			break;
		case MxFunctions::Physical_Post:
			_postDataManager.deletePostDatabase(id);
			break;
		case MxFunctions::Physical_Function:
			_functionManager.deleteFunction(id);
			break;
		case MxFunctions::Physical_MeshPanel:
			_meshPanelManager.deleteMeshPanel(id);
			break;
		case MxFunctions::Physical_PropertyShape:
			_sectionShapeManager.deleteSectionShape(id);
			break;
		case MxFunctions::Physical_MeshOptPanel:
			_meshOptPanelManager.deleteMeshPanel(id);
			break;
		case MxFunctions::Physical_OptimationDesign:
			_mStructOptionData->deleteOptimizationDesignData(id);
			break;
		case MxFunctions::Physical_OptimationConstraint:
			_mStructOptionData->deleteOptimizationConstraintData(id);
			break;
		case MxFunctions::Physical_RunnerOptimationDesign:
			if (_mRunnerStructOptionData) _mRunnerStructOptionData->deleteOptimizationDesignData(id);
			break;
		case MxFunctions::Physical_RunnerOptimationConstraint:
			if (_mRunnerStructOptionData) _mRunnerStructOptionData->deleteOptimizationConstraintData(id);
			break;
		case MxFunctions::Physical_GenericParam:
			_mGlobalParam->deleteGenericParamData(id);
			break;
		case MxFunctions::Physical_GenericAssign:
			_mGlobalParam->deleteGenericAssignData();
			break;
		case MxFunctions::Physical_GenericMatrix:
			_mGlobalParam->deleteGenericMatrixData();
			break;
		case MxFunctions::Physical_ModalContri:
			_mGlobalParam->deleteModalContriData();
			break;
		case MxFunctions::Physical_NodeContri:
			_mGlobalParam->deleteNodeContriData();
			break;
		case MxFunctions::Physical_TPContri:
			_mGlobalParam->deleteTPContriData();
			break;
		case MxFunctions::Physical_PanelContri:
			_mGlobalParam->deletePanelContriData();
			break;
		case MxFunctions::Physical_ERP:
			_mGlobalParam->deleteERPData();
			break;
		case MxFunctions::Physical_PeakFilter:
			_mGlobalParam->deletePeakFilterData(id);
			break;
		case MxFunctions::Physical_GenericPart:
			_mGlobalParam->deleteGenericPartData();
			break;
		case MxFunctions::Physical_ASCouplingSearch:
			_mGlobalParam->deleteASCouplingSearchData();
			break;
		case MxFunctions::Physical_SolveControl:
			_mGlobalParam->deleteSolveControlData();
			break;
		case MxFunctions::Physical_FreqSamplPoint:
			_FreqSamplPointManager.deleteFreqSamplPoint(id);
			break;
		case MxFunctions::Physical_Laminate:
			_LaminateManager.deleteLaminateById(id);
			break;
		case MxFunctions::Physical_PredefinedField:
			_predefinedFieldManager.deletePredefinedField(id);
			break;
		case MxFunctions::Physical_RotateAnalyse:
			_rotateAnalyseManager.deleteRotateAnalyse(id);
			break;
		case MxFunctions::Physical_NonStructuralQuality:
			_nonStructuralQualityManager.deleteNsm(id);
			break;
		case MxFunctions::Physical_Field:
			_fieldManager.deleteField(id);
			break;
		case MxFunctions::Physical_FRCMaterial:
			_mFrpManager.clear();
		case MxFunctions::Physical_Assembly:
			_assemblyManager.removeAssembly(id);
			break;
		case MxFunctions::Physical_MotionBody:
			_motionBodyManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionJoint:
			_motionJointManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionCoupling:
			_motionCouplingManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionDrive:
			_motionDriveManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionScenario:
			_motionScenarioManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionMarker:
			_motionMarkerManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionVectorForce:
			_motionForceElementManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionContact:
			_motionContactManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionSpring:
			_motionSpringManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionControlInput:
			_motionControlInputManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionElectroImport:
			_motionElectroImportManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionElectroExport:
			_motionElectroExportManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionHydraulic:
			_motionHydraulicManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionRL:
			_motionRLManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionSensor:
			_motionSensorManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionController:
			_motionControllerManager.deleteData(id);
			break;
		case MxFunctions::Physical_MotionAutoFlex:
			_motionAutoFlexManager.deleteData(id);
			break;
		default:
			break;
		}

		_mapRelationManager.deleteMapRelation({ type ,id });
	}

	mMeshData * mModelData::getMeshData()
	{
		return _meshData;
	}

    MGeoData::GeoDataBase* mModelData::getGeoData()
    {
		return _geoDataBase;
    }

	MechElementData::mDataElement* mModelData::getElementData()
	{
		return _elementData;
	}

    MGeoManager::GeoManager* mModelData::getGeoManager()
    {
		return _geoManager;
    }

	GeoEngineConfig::GeoEngineData* mModelData::getGeoEngineDataMgr()
	{
		return _geoEngineData.get();
	}

	//装配层级段魔数（旧版本文件无此段，atEnd 判别；'AS01'）
	static constexpr quint32 kAssemblySectionMagic = 0x41533031u;

	void mModelData::saveAssemblySection(QDataStream& out)
	{
		out << kAssemblySectionMagic << _assemblyManager;
	}

	void mModelData::loadAssemblySection(QDataStream& in)
	{
		if (in.atEnd())
		{
			return;
		}
		quint32 magic = 0;
		in >> magic;
		if (magic != kAssemblySectionMagic)
		{
			return;
		}
		in >> _assemblyManager;
	}

	//【老类退役·S7】工程文件(.mxcae)引擎几何段：魔数（旧版本文件无此段，atEnd 判别）
	static constexpr quint32 kEngineGeoSectionMagic = 0x47453031u; // 'GE01'

	void mModelData::saveEngineGeometrySection(QDataStream& out)
	{
		if (_geoManager == nullptr)
		{
			return;
		}
		const QString tempGeo = QDir::tempPath() + "/mxsim_engine_geo_save.mxcae";
		QFile::remove(tempGeo);
		if (!_geoManager->exportProjectGeometrySync(tempGeo))
		{
			return;
		}
		QFile tempFile(tempGeo);
		if (!tempFile.open(QIODevice::ReadOnly))
		{
			return;
		}
		const QByteArray engineBlob = tempFile.readAll();
		tempFile.close();
		QFile::remove(tempGeo);
		out << kEngineGeoSectionMagic << engineBlob;
	}

	void mModelData::loadEngineGeometrySection(QDataStream& in)
	{
		if (in.atEnd() || _geoManager == nullptr)
		{
			return;
		}
		quint32 magic = 0;
		in >> magic;
		if (magic != kEngineGeoSectionMagic)
		{
			return;
		}
		QByteArray engineBlob;
		in >> engineBlob;
		if (engineBlob.isEmpty())
		{
			return;
		}
		const QString tempGeo = QDir::tempPath() + "/mxsim_engine_geo_load.mxcae";
		QFile tempFile(tempGeo);
		if (!tempFile.open(QIODevice::WriteOnly))
		{
			return;
		}
		tempFile.write(engineBlob);
		tempFile.close();
		_geoManager->importGeoFileSync(tempGeo);
		QFile::remove(tempGeo);
	}

	void mModelData::adoptLegacyGeometry()
	{
		if (_geoManager != nullptr)
		{
			_geoManager->adoptLoadedLegacySets();
		}
	}

	mMapRelationManager& mModelData::getMapRelationManager()
	{
		return _mapRelationManager;
	}

	AssemblyData::mAssemblyManager& mModelData::getAssemblyManager()
	{
		return _assemblyManager;
	}

	mUnitSystemManager* mModelData::getUnitSystemManager()
	{
		return _unitSystemManager;
	}

	void mModelData::updateDataFromUnitSystem(const int& unitidx)
	{
		////材料数据更新
		//_materialManager.responseUnitChanged(unitidx,_unitSystemManager);
		////载荷数据更新
		//_loadManager.responseUnitChanged(unitidx, _unitSystemManager);
	}

	bool mModelData::checkModelData()
	{
		bool isOk = true;

		//材料
		vector<int> ids = getAllPhyID(Physical_Material);
		if (ids.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig("材料未定义！");
			isOk = false;
		}
			
		//截面属性
		ids = getAllPhyID(Physical_Property);
		if (ids.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig("截面属性未定义！");
			isOk = false;
		}

		//分析工况
		ids = getAllPhyID(Physical_Case);
		if (ids.empty())
		{
			mGlobalSignals::getInstance()->outputMessageSig("分析工况未定义！");
			isOk = false;
		}

		//网格


		return isOk;
	}

	MxFunctions::BrowserTreeStatus mModelData::getDataStatus(QVector<QPair<QString, QString>>& info, MxFunctions::PhysicalFieldType type, int id)
	{
		QString tip = "警告";
		switch (type)
		{
		case MxFunctions::Physical_Material:
		{
			//检查截面属性
			auto data = _materialManager.getMaterialData(id);
			if (data == nullptr)
			{
				info.append({ tip,"材料不存在" });
				return Tree_Failure;
			}
			return data->getNodeStatu(info);

			break;
		}
		case MxFunctions::Physical_LoadBoundary:
		{
			auto data = _loadboundaryManager.getLoadBoundaryData(id);
			if (data == nullptr)
				return Tree_Failure;
			if (data->isSuppressed()) //如果被抑制 直接返回Tree_Wrong状态 不再进行其他判断 此优先级最高
				return Tree_Wrong;

			//力 力矩 压强 热流密度 热辐射 对流换热 位移/转角 表面载荷在独立赋值时不需要判断作用对象集合
			bool isIndependentAssignment = false;
			if (data->getLoadBoundaryType() == LOAD_IM_FORCE || data->getLoadBoundaryType() == LOAD_IM_TORQUE)
			{
				auto force = dynamic_cast<MechCaseData::mLoadForce*>(data.get());
				if (data->getLoadBoundaryType() == LOAD_IM_FORCE)
				{
					// 集中力：0=各向分量, 1=方向与大小, 2=独立赋值, 3=文件（独立赋值/文件模式无作用对象集合）
					if (force && (force->way_ == 2 || force->way_ == 3))
						isIndependentAssignment = true;

				}
				else if (data->getLoadBoundaryType() == LOAD_IM_TORQUE)
				{
					// 力矩：0=各向分量, 1=独立赋值, 2=文件（独立赋值/文件模式无作用对象集合）
					if (force && (force->way_ == 1 || force->way_ == 2))
						isIndependentAssignment = true;
				}
			}
			else if (data->getLoadBoundaryType() == BOUNDARY_IM_DISPLACEMENT ||
				data->getLoadBoundaryType() == BOUNDARY_IM_SPEED || data->getLoadBoundaryType() == BOUNDARY_IM_ACC)
			{
				auto displace = dynamic_cast<MechCaseData::mBoundaryDisplacement*>(data.get());
				if (displace && displace->_definitionMode == 1)
					isIndependentAssignment = true;
			}
			else if (data->getLoadBoundaryType() == LOAD_IM_PRESSURE)
			{
				auto pressure = dynamic_cast<MechCaseData::mLoadPressure*>(data.get());
				if (pressure && (pressure->way_ == 1 || pressure->way_ == 2))
					isIndependentAssignment = true;
			}
			else if (data->getLoadBoundaryType() == LOAD_IM_HEATFLUX)
			{
				auto heatflux = dynamic_cast<MechCaseData::mLoadHeatFlux*>(data.get());
				if (heatflux && heatflux->defineType_ == 1)
					isIndependentAssignment = true;
			}
			else if (data->getLoadBoundaryType() == LOAD_IM_RADIATION)
			{
				auto radiation = dynamic_cast<MechCaseData::mLoadRadiation*>(data.get());
				if (radiation && radiation->defineType_ == 1)
					isIndependentAssignment = true;
			}
			else if (data->getLoadBoundaryType() == LOAD_IM_CONVECTION)
			{
				auto convection = dynamic_cast<MechCaseData::mLoadConvection*>(data.get());
				if (convection && (convection->defineType_ == 1 || convection->defineType_ == 2))
					isIndependentAssignment = true;
			}
			else if (data->getLoadBoundaryType() == LOAD_IM_SURFACE)
			{
				auto surface = dynamic_cast<MechCaseData::mLoadSurface*>(data.get());
				if (surface && (surface->way_ == 1))
					isIndependentAssignment = true;
			}

			//重力 时域载荷 频域载荷 组合载荷 螺栓载荷 声学频域载荷不管什么模式都不需要判断作用对象
			bool needCheckCollection = true;
			if (data->getLoadBoundaryType() == LOAD_IM_GRAVITY ||
				data->getLoadBoundaryType() == LOAD_IM_TRANSIENT ||
				data->getLoadBoundaryType() == LOAD_IM_FREQDOMAIN ||
				data->getLoadBoundaryType() == LOAD_IM_COMBINED ||
				data->getLoadBoundaryType() == LOAD_IM_BOLT ||
				data->getLoadBoundaryType() == LOAD_IM_ACOFREQDOMAIN ||
				data->getLoadBoundaryType() == LOAD_IM_SPECTRUM)

			{
				needCheckCollection = false;
			}

			if (!isIndependentAssignment && needCheckCollection) //非独立模式 且是需要检查集合的面板才会进入此处
			{
				bool hasSelectTarget = false;
				//首先判断数据类中的标志位
				if (data->getPickState() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_Set);

					if (setId != -1) hasSelectTarget = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (data->getPickState() == 2) hasSelectTarget = true;
				}

				if (!hasSelectTarget)
				{
					info.append({ tip,"未选择作用对象" });
				}
			}
			//时域载荷
			if (data->getLoadBoundaryType() == LOAD_IM_TRANSIENT)
			{
				int loadboundid = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_LoadBoundary);
				if (loadboundid == -1)
				{
					info.append({ tip,"未选择调用载荷!" });
				}
			}

			//频域载荷需要判断选取载荷情况以及第一个曲线（函数）
			if (data->getLoadBoundaryType() == LOAD_IM_FREQDOMAIN)
			{
				int loadboundid = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_LoadBoundary);
				if (loadboundid == -1)
				{
					info.append({ tip,"未选择调用载荷!" });
				}

				int funcid1 = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_Function, MapRelationType::MapRelation_AmplitudeFunction);
				if (funcid1 == -1)
				{
					info.append({ tip,"实部/幅值曲线未给定!" });
				}
			}
			//组合载荷也需判断调用载荷情况
			if (data->getLoadBoundaryType() == LOAD_IM_COMBINED)
			{
				QList<int> loadIDs = _mapRelationManager.getMapRelation<QList<int>>({ Physical_LoadBoundary, id }, Physical_LoadBoundary);
				if (loadIDs.size() == 0)
				{
					info.append({ tip,"未选择调用载荷!" });
				}
			}
			//螺栓载荷需要判断选取螺栓情况
			if (data->getLoadBoundaryType() == LOAD_IM_BOLT)
			{
				int loadboundid = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_Connector);
				if (loadboundid == -1)
				{
					info.append({ tip,"未选择调用螺栓!" });
				}
			}
			//声学频域载荷需判断调用载荷（声学载荷），以及曲线（函数）
			if (data->getLoadBoundaryType() == LOAD_IM_ACOFREQDOMAIN)
			{
				int id = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_LoadBoundary);
				if (id != -1)
				{
					info.append({ tip,"未选择调用声学载荷!" });
				}

				int funcid = _mapRelationManager.getMapRelation<int>({ Physical_LoadBoundary,id }, Physical_Function);
				if (funcid != -1)
				{
					info.append({ tip,"未选择调用曲线!" });
				}
			}

			data->getNodeStatu(info);
			return info.isEmpty() ? Tree_Yes : Tree_Failure;
		}
		case MxFunctions::Physical_Connector:
		{
			auto conn = _connectorManager.getConnectorData(id);
			if (conn == nullptr)
			{
				info.append({ tip,"连接不存在" });
				return Tree_Failure;
			}
			if (conn->isSuppressed())
				return Tree_Wrong;

			if (conn->getConnectorType() == Connector_Bind) //绑定需要判断主从面
			{
				auto bind = std::dynamic_pointer_cast<MechConnectorData::mConnectorBind>(conn);
				if (!bind) return Tree_Failure;
				//**********************************主面************************************************
				bool hasMaster = false;
				//首先判断数据类中的标志位
				if (bind->getPickStateMaster() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Connector,id }, Physical_Set, MapRelation_Master);

					if (setId != -1) hasMaster = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (bind->getPickStateMaster() == 2) hasMaster = true;
				}

				if (!hasMaster)
				{
					info.append({ tip,"接触没有主面" });
				}

				//**********************************从面************************************************
				bool hasSlave = false;
				//首先判断数据类中的标志位
				if (bind->getPickStateSlave() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Connector,id }, Physical_Set, MapRelation_Slave);

					if (setId != -1) hasSlave = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (bind->getPickStateSlave() == 2) hasSlave = true;
				}

				if (!hasSlave)
				{
					info.append({ tip,"接触没有从面" });
				}
			}

			if (conn->getConnectorType() == Connector_Bolt)
			{//螺栓约束需判断集合映射
				int setid = _mapRelationManager.getMapRelation<int>({ Physical_Connector,conn->getConnectorId() }, Physical_Set);
				if (setid == -1)
				{
					info.append({ tip,"未选择集合" });
					return Tree_Failure;
				}
			}
			return conn->getNodeStatu(info);
		}
		case MxFunctions::Physical_Deputy:
		{
			auto deputy = _deputyManager.getDeputyData(id);
			if (deputy == nullptr)
			{
				info.append({ tip,"连接副不存在" });
				return Tree_Failure;
			}
			if (deputy->isSuppressed())
				return Tree_Wrong;
			return deputy->getNodeStatu(info);
		}
		case MxFunctions::Physical_Part:
		{
			//多体产品线：部件只是几何载体——材料在自动柔性体等入口按需给定，网格由
			//AutoFlex 按需剖分，隐式工作流的"部件没有赋予截面属性/部件未剖分网格"
			//两条告警不再检查（结构隐式产品沿上游 dev_Mech 原逻辑，合并回上游时
			//应改为按菜单栏形态开关门控）
			break;
		}
		case MxFunctions::Physical_Property:
		{
			auto data = _propertyManager.getPropertyData(id);
			if (data == nullptr)
			{
				info.append({ tip,"截面属性不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			PropertyType type = data->getPropertyType();
			if (type != LayeredShellProperty && type != Elas1Property && type != Spring1Property
				&& type != Spring2Property && type != SpringAProperty && type != ViscousDampProperty && type != CbushProperty
				&& type != PlyLaminateProperty && type != GapuniProperty)
			{
				//检查材料
				int matId = _mapRelationManager.getMapRelation<int>({ Physical_Property, id }, Physical_Material);
				if (matId == -1)
				{
					info.append({ tip,"截面属性没有选择材料" });
				}
			}
			//检查截面形状
			if (type == ResultantProperty)
			{
				int propShapeId = _mapRelationManager.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
				if (propShapeId == -1)
				{
					info.append({ tip,"截面属性没有选择截面形状" });
				}
			}
			else if (type == CrossProperty)
			{
				QList<int> propShapeIds = _mapRelationManager.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_PropertyShape);
				if (propShapeIds.size() >= 2)
				{
					for (int i = 0; i < propShapeIds.size(); i++)
					{
						if (propShapeIds.at(i) == -1)
						{
							info.append({ tip,"变截面梁属性存在未定义的截面形状" });
							break;
						}
					}
				}
			}

			//铺层额外的完整性检查
			if (type == PlyLaminateProperty)
			{
				//data是数据类
				auto property = dynamic_cast<MechPropertyData::mPlyLaminate*>(data);

				if (property->_plyDefType == 0) //直接定义
				{
					QList<int> matIDs = _mapRelationManager.getMapRelation<QList<int>>({ Physical_Property,  property->getPropertyId() }, Physical_Material);
					if (matIDs.size() == 0)
					{
						info.append({ tip,"请至少定义一个层合结构" });
					}
				}
				else if (property->_plyDefType == 1) //层合板
				{
					int ID = _mapRelationManager.getMapRelation<int>({ Physical_Property, id }, Physical_Laminate);
					if (ID == -1)
					{
						info.append({ tip,"请选择层合板" });
					}
				}
			}

			break;
		}
		case MxFunctions::Physical_Contact:
		{
			auto data = _contactManager.getContactData(id);
			if (data == nullptr)
			{
				info.append({ tip,"接触不存在" });
				return Tree_Failure;
			}
			if (data->isSuppressed()) //抑制直接返回Tree_Wrong 此状态优先级最高 不再往下判断
				return Tree_Wrong;

			ContactType type = data->getContactType();
			if (type == SurfaceToSurfaceContact || type == NodeToSurfaceContact
				|| type == TwoDimensionalContact || type == LineToSurfaceContact)
			{
				//**********************************主面************************************************
				bool hasMaster = false;
				//首先判断数据类中的标志位
				if (data->getPickStateMaster() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Contact,id }, Physical_Set, MapRelation_Master);

					if (setId != -1) hasMaster = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (data->getPickStateMaster() == 2) hasMaster = true;
				}

				if (!hasMaster)
				{
					info.append({ tip,"接触没有主面" });
				}

				//**********************************从面************************************************
				bool hasSlave = false;
				//首先判断数据类中的标志位
				if (data->getPickStateSlave() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Contact,id }, Physical_Set, MapRelation_Slave);

					if (setId != -1) hasSlave = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (data->getPickStateSlave() == 2) hasSlave = true;
				}

				if (!hasSlave)
				{
					info.append({ tip,"接触没有从面" });
				}
			}
			else if (type == SelfContact)
			{
				bool hasMaster = false;
				//首先判断数据类中的标志位
				if (data->getPickStateMaster() == 0) //标志位等于0时 判断集合映射
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Contact,id }, Physical_Set, MapRelation_Master);

					if (setId != -1) hasMaster = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (data->getPickStateMaster() == 2) hasMaster = true;
				}

				if (!hasMaster)
				{
					//检查作用对象集合映射
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_Contact,id }, Physical_Set, MapRelation_Master);

					if (setId != -1) hasMaster = true;
				}
				else //标志位不为0时 判断标志位 面板中对标志位进行设置
				{
					//1表示无临时数据 2表示有临时数据
					if (data->getPickStateMaster() == 2) hasMaster = true;
				}

				if (!hasMaster)
				{
					info.append({ tip,"接触没有接触面" });
				}
			}
			else if (type == InterferenceContact)
			{
				//检查过盈接触是否选择了有效的接触对
				int contactPairId = _mapRelationManager.getMapRelation<int>({ Physical_Contact, id }, Physical_Contact);
				if (contactPairId == -1)
				{
					info.append({ tip,"过盈接触没有选择接触对" });
				}
				else
				{
					auto* pairContact = _contactManager.getContactData(contactPairId);
					if (!pairContact)
					{
						info.append({ tip,"过盈接触的接触对不存在" });
					}
					else if (pairContact->isSuppressed())
					{
						info.append({ tip,"过盈接触的接触对已被抑制" });
					}
					else
					{
						const ContactType pairType = pairContact->getContactType();
						if (pairType != SurfaceToSurfaceContact && pairType != NodeToSurfaceContact
							&& pairType != TwoDimensionalContact)
						{
							info.append({ tip,"过盈接触的接触对类型不支持" });
						}
					}
				}
				//当clr_fit_typ=1（逐步消除间隙配合量）时，检查是否有函数映射
				if (data->clr_fit_typ == 1)
				{
					int funcId = _mapRelationManager.getMapRelation<int>({ Physical_Contact, id }, Physical_Function);
					if (funcId == -1)
					{
						info.append({ tip,"过盈接触没有选择函数" });
					}
				}
			}
			break;
		}
		case MxFunctions::Physical_PeriodicBoundary:
		{
			auto data = _periodicBoundaryManager.getPeriodicBoundaryData(id);
			if (data == nullptr)
			{
				info.append({ tip,"周期性边界不存在" });
				return Tree_Failure;
			}
			if (data->isSuppressed())
				return Tree_Wrong;


			//**********************************主面************************************************
			bool hasMaster = false;
			//首先判断数据类中的标志位
			if (data->getPickStateMaster() == 0) //标志位等于0时 判断集合映射
			{
				//检查作用对象集合映射
				int setId = _mapRelationManager.getMapRelation<int>({ Physical_PeriodicBoundary,id }, Physical_Set, MapRelation_Master);

				if (setId != -1) hasMaster = true;
			}
			else //标志位不为0时 判断标志位 面板中对标志位进行设置
			{
				//1表示无临时数据 2表示有临时数据
				if (data->getPickStateMaster() == 2) hasMaster = true;
			}

			if (!hasMaster)
			{
				info.append({ tip,"周期性边界没有主面" });
			}

			//**********************************从面************************************************
			bool hasSlave = false;
			//首先判断数据类中的标志位
			if (data->getPickStateSlave() == 0) //标志位等于0时 判断集合映射
			{
				//检查作用对象集合映射
				int setId = _mapRelationManager.getMapRelation<int>({ Physical_PeriodicBoundary,id }, Physical_Set, MapRelation_Slave);

				if (setId != -1) hasSlave = true;
			}
			else //标志位不为0时 判断标志位 面板中对标志位进行设置
			{
				//1表示无临时数据 2表示有临时数据
				if (data->getPickStateSlave() == 2) hasSlave = true;
			}

			if (!hasSlave)
			{
				info.append({ tip,"周期性边界没有从面" });
			}

			data->getNodeStatu(info);
			break;
		}
		case MxFunctions::Physical_Set:
		{
			//检查截面属性
			auto data = _setManager.getSetData(id);
			if (data == nullptr)
			{
				info.append({ tip,"集合不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_OptimationGoal:
		{
			auto data = _mStructOptionData->_goalData;

			if (!(data->_responseType == "综合柔度" || (data->_responseType == "柔度指数" && !data->_isFrequency)))
			{
				int caseID = _mapRelationManager.getMapRelation<int>({ Physical_OptimationGoal, id }, Physical_Case);
				if (caseID == -1)
				{
					info.append({ tip,"拓扑优化目标没有分析工况" });
				}
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_OptimationDesign:
		{
			auto data = _mStructOptionData->getOptimizationDesignData(id);
			if (data == nullptr)
			{
				info.append({ tip,"拓扑优化设计域不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			//检查设计域的截面属性
			int propertyId = _mapRelationManager.getMapRelation<int>({ Physical_OptimationDesign, id }, Physical_Property);
			if (propertyId == -1)
			{
				info.append({ tip,"拓扑优化设计域没有指定设计域" });
			}
			//如果开启模式重复了为从设计域
			if (data->_patternRepStatus && data->_domainType == "从设计域")
			{
				int designId = _mapRelationManager.getMapRelation<int>({ Physical_OptimationDesign, id }, Physical_OptimationDesign);
				if (designId == -1)
				{
					info.append({ tip,"拓扑优化设计域没有选择参考设计域" });
				}
			}
			break;
		}
		case MxFunctions::Physical_OptimationConstraint:
		{
			auto data = _mStructOptionData->getOptimizationConstraintData(id);
			if (data == nullptr)
			{
				info.append({ tip,"拓扑优化设计域不存在" });
				return Tree_Failure;
			}
			//检查设计域的截面属性

			//如果开启模式重复了为从设计域
			if (data->_isArea)
			{
				int propertyId = _mapRelationManager.getMapRelation<int>({ Physical_OptimationConstraint, id }, Physical_Property);
				if (propertyId == -1)
				{
					info.append({ tip,"拓扑优化约束没有指定区域" });
				}
			}

			//响应类型约束：检查分析工况与响应参数(与优化目标保持一致)
			bool isResponseType = data->_constraintType == "柔度" || data->_constraintType == "位移"
				|| data->_constraintType == "频率" || data->_constraintType == "应力"
				|| data->_constraintType == "综合柔度" || data->_constraintType == "综合频率"
				|| data->_constraintType == "柔度指数" || data->_constraintType == "热柔度"
				|| data->_constraintType == "节点温度" || data->_constraintType == "PSD位移"
				|| data->_constraintType == "频响位移";
			if (isResponseType)
			{
				if (!(data->_constraintType == "综合柔度" || (data->_constraintType == "柔度指数" && !data->_isFrequency)))
				{
					int caseID = _mapRelationManager.getMapRelation<int>({ Physical_OptimationConstraint, id }, Physical_Case);
					if (caseID == -1)
					{
						info.append({ tip,"拓扑优化约束没有分析工况" });
					}
				}
				data->getNodeStatus(info);
			}
			break;
		}
		case MxFunctions::Physical_RunnerOptimationGoal:
		{
			if (!_mRunnerStructOptionData || !_mRunnerStructOptionData->_goalData)
			{
				info.append({ tip,"流道优化目标不存在" });
				return Tree_Failure;
			}
			break;
		}
		case MxFunctions::Physical_RunnerOptimationDesign:
		{
			auto data = _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationDesignData(id) : nullptr;
			if (data == nullptr)
			{
				info.append({ tip,"流道优化设计域不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			int propertyId = _mapRelationManager.getMapRelation<int>({ Physical_RunnerOptimationDesign, id }, Physical_Property);
			if (propertyId == -1)
			{
				info.append({ tip,"流道优化设计域没有指定设计域" });
			}
			break;
		}
		case MxFunctions::Physical_RunnerOptimationConstraint:
		{
			auto data = _mRunnerStructOptionData ? _mRunnerStructOptionData->getOptimizationConstraintData(id) : nullptr;
			if (data == nullptr)
			{
				info.append({ tip,"流道优化约束不存在" });
				return Tree_Failure;
			}
			if (data->_isArea)
			{
				int propertyId = _mapRelationManager.getMapRelation<int>({ Physical_RunnerOptimationConstraint, id }, Physical_Property);
				if (propertyId == -1)
				{
					info.append({ tip,"流道优化约束没有指定区域" });
				}
			}
			break;
		}
		case MxFunctions::Physical_FreqSamplPoint:
		{
			auto data = _FreqSamplPointManager.getFreqSamplPointData(id);
			if (data == nullptr)
			{
				info.append({ tip,"频率采样点方式不存在" });
				return Tree_Failure;
			}
			data->getNodeStatu(info);
			break;
		}
		case MxFunctions::Physical_Laminate:
		{
			auto data = _LaminateManager.getLaminateData(id);
			if (data == nullptr)
			{
				info.append({ tip,"层合结构不存在" });
				return Tree_Failure;
			}

			if (data->getLaminateType() == SglLayerPlate)
			{
				//检查材料
				int matId = _mapRelationManager.getMapRelation<int>({ Physical_Laminate, id }, Physical_Material);
				if (matId == -1)
				{
					info.append({ tip,"单层板没有选择材料" });
				}

				//检查单元集合
				int setId = _mapRelationManager.getMapRelation<int>({ Physical_Laminate, id }, Physical_Set);
				if (setId == -1)
				{
					info.append({ tip,"单层板没有选择集合" });
				}
			}
			else if (data->getLaminateType() == GlobalSglLam)
			{
				//检查材料
				int matId = _mapRelationManager.getMapRelation<int>({ Physical_Laminate, id }, Physical_Material);
				if (matId == -1)
				{
					info.append({ tip,"单层板没有选择材料" });
				}
			}
			else if (data->getLaminateType() == LamPlate)
			{
				//检查表格映射
				QList<int> subLamIds = _mapRelationManager.getMapRelation<QList<int>>({ Physical_Laminate, id }, Physical_Laminate);
				if (subLamIds.size() == 0)
				{
					info.append({ tip,"层合板没有引用单层板" });
				}
			}

			data->getNodeStatu(info);
			break;
		}
		case MxFunctions::Physical_GenericParam:
		{
			auto data = _mGlobalParam->getGenericParamData(id);
			if (data == nullptr)
			{
				info.append({ tip,"超单元参数不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_GenericAssign:
		{
			if (_mGlobalParam->getGenericAssignData() == nullptr)
			{
				info.append({ tip,"超单元调用不存在" });
				return Tree_Failure;
			}
			_mGlobalParam->getGenericAssignData()->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_GenericMatrix:
		{
			if (_mGlobalParam->getGenericMatrixData() == nullptr)
			{
				info.append({ tip,"超单元矩阵不存在" });
				return Tree_Failure;
			}
			_mGlobalParam->getGenericMatrixData()->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_MotionBody:
		{
			auto data = _motionBodyManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"运动体不存在" });
				return Tree_Failure;
			}
			if (_mapRelationManager.getMapRelation<std::set<int>>({ Physical_MotionBody ,id }, Physical_Part).empty())
			{
				info.append({ tip,"运动体没有部件" });
			}

			break;
		}
		case MxFunctions::Physical_MotionJoint:
		{
			auto data = _motionJointManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"运动副不存在" });
				return Tree_Failure;
			}
			// 基本体 -1 表示大地（合法），仅校验作用体
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionJoint ,id }, Physical_MotionBody, MapRelation_MotionActionBody) == -1)
			{
				info.append({ tip,"运动副没有动作实体" });
			}

			break;
		}
		case MxFunctions::Physical_MotionCoupling:
		{
			auto data = _motionCouplingManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"耦合副不存在" });
				return Tree_Failure;
			}
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionCoupling ,id }, Physical_MotionJoint, MapRelation_MotionFirstJoint) == -1)
			{
				info.append({ tip,"耦合副没有第一个运动副" });
			}
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionCoupling ,id }, Physical_MotionJoint, MapRelation_MotionSecondJoint) == -1)
			{
				info.append({ tip,"耦合副没有第二个运动副" });
			}

			break;
		}
		case MxFunctions::Physical_MotionDrive:
		{
			auto data = _motionDriveManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"驱动不存在" });
				return Tree_Failure;
			}
			if (data->getTargetType() == DriveTargetType::Joint)
			{
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionDrive ,id }, Physical_MotionJoint) == -1)
				{
					info.append({ tip,"驱动没有运动副" });
				}
			}

			break;
		}
		case MxFunctions::Physical_MotionVectorForce:
		{
			auto data = _motionForceElementManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"力元不存在" });
				return Tree_Failure;
			}
			MechMotionData::mMotionVectorForce* vectorForce = dynamic_cast<MechMotionData::mMotionVectorForce*>(data);
			if (vectorForce) {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionVectorForce ,id }, Physical_MotionBody, MapRelation_MotionActionBody) == -1)
				{
					info.append({ tip,"力元没有实体" });
				}
				// 衬套基本体 -1 表示大地（合法），不告警
			}
			else if (dynamic_cast<MechMotionData::mMotionScalarForce*>(data)) {
				// 基本体 -1 表示大地（合法），仅校验作用体
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionVectorForce ,id }, Physical_MotionBody, MapRelation_MotionBodyI) == -1)
				{
					info.append({ tip,"标量力没有作用体" });
				}
			}
			else if (dynamic_cast<MechMotionData::mMotionScalarTorque*>(data)) {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionVectorForce ,id }, Physical_MotionBody, MapRelation_MotionBodyI) == -1)
				{
					info.append({ tip,"标量力矩没有作用体" });
				}
			}
			else if (dynamic_cast<MechMotionData::mMotionThreePointForce*>(data)) {
				// 出发/目标体 -1 表示大地（合法），仅校验应用体
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionVectorForce ,id }, Physical_MotionBody, MapRelation_MotionBodyI) == -1)
				{
					info.append({ tip,"三点力没有应用体" });
				}
			}
			else if (dynamic_cast<MechMotionData::mMotionThreePointTorque*>(data)) {
				// 出发/目标体 -1 表示大地（合法），仅校验应用体
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionVectorForce ,id }, Physical_MotionBody, MapRelation_MotionBodyI) == -1)
				{
					info.append({ tip,"三点力矩没有应用体" });
				}
			}

			break;
		}
		case MxFunctions::Physical_MotionScenario:
		{
			auto data = _motionScenarioManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"方案不存在" });
				return Tree_Failure;
			}
			if (!data->hasResult())
			{
				info.append({ tip,"多体方案未求解" });
			}

			break;
		}
		case MxFunctions::Physical_MotionMarker:
		{
			auto data = _motionMarkerManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"标记点不存在" });
				return Tree_Failure;
			}
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionMarker ,id }, Physical_MotionBody) == -1)
			{
				info.append({ tip,"标记点没有关联运动体" });
			}
			break;
		}
		case MxFunctions::Physical_MotionContact:
		{
			auto data = _motionContactManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"接触不存在" });
				return Tree_Failure;
			}
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionContact ,id }, Physical_MotionBody, MapRelation_MotionActionBody) == -1)
			{
				info.append({ tip,"接触没有作用体" });
			}
			if (_mapRelationManager.getMapRelation<int>({ Physical_MotionContact ,id }, Physical_MotionBody, MapRelation_MotionBaseBody) == -1)
			{
				info.append({ tip,"接触没有基本体" });
			}
			break;
		}
		case MxFunctions::Physical_MotionSpring:
		{
			auto data = _motionSpringManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"弹簧不存在" });
				return Tree_Failure;
			}
			if (data->getType() == MechMotionData::SpringType::BodyPoint) {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionSpring ,id }, Physical_MotionBody, MapRelation_MotionBodyI) == -1
					|| _mapRelationManager.getMapRelation<int>({ Physical_MotionSpring ,id }, Physical_MotionBody, MapRelation_MotionBodyJ) == -1)
				{
					info.append({ tip,"弹簧两端体未设置" });
				}
			}
			else {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionSpring ,id }, Physical_MotionJoint) == -1)
				{
					info.append({ tip,"扭转弹簧没有目标运动副" });
				}
			}
			break;
		}
		case MxFunctions::Physical_MotionSensor:
		{
			auto data = _motionSensorManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"传感器不存在" });
				return Tree_Failure;
			}
			MechMotionData::mMotionBodySensor* bodySensor = dynamic_cast<MechMotionData::mMotionBodySensor*>(data);
			if (bodySensor) {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionSensor ,id }, Physical_MotionBody) == -1)
				{
					info.append({ tip,"传感器没有目标运动体" });
				}
			}
			else if (MechMotionData::mMotionDistanceSensor* distSensor = dynamic_cast<MechMotionData::mMotionDistanceSensor*>(data)) {
				//距离传感器端点为体+局部位置强绑定结构（SensorPointRef），bodyId 保留直存
				if (distSensor->getPointA().bodyId == -1 || distSensor->getPointB().bodyId == -1)
				{
					info.append({ tip,"距离传感器两端体未设置" });
				}
			}
			else if (dynamic_cast<MechMotionData::mMotionAngleSensor*>(data)) {
				if (_mapRelationManager.getMapRelation<int>({ Physical_MotionSensor ,id }, Physical_MotionBody, MapRelation_MotionBodyA) == -1
					|| _mapRelationManager.getMapRelation<int>({ Physical_MotionSensor ,id }, Physical_MotionBody, MapRelation_MotionBodyB) == -1)
				{
					info.append({ tip,"角度传感器两端体未设置" });
				}
			}
			break;
		}
		case MxFunctions::Physical_MotionController:
		{
			auto data = _motionControllerManager.getData(id);
			if (data == nullptr)
			{
				info.append({ tip,"控制器不存在" });
				return Tree_Failure;
			}
			// PID 控制器：传感器反馈源必须挂一个存在的传感器（类型/启用性由求解侧细分校验）
			MechMotionData::mMotionPIDController* pid = dynamic_cast<MechMotionData::mMotionPIDController*>(data);
			if (pid && pid->getFeedbackSource() == MechMotionData::PidFeedbackSource::Sensor) {
				const int sensorId = _mapRelationManager.getMapRelation<int>({ Physical_MotionController ,id }, Physical_MotionSensor);
				if (sensorId == -1 || _motionSensorManager.getData(sensorId) == nullptr)
				{
					info.append({ tip,"PID控制器没有有效的反馈传感器" });
				}
			}
			break;
		}
		case MxFunctions::Physical_PeakFilter:
		{//判断表格数据至少有一行，且三个值的前两个值有数字
			auto data = _mGlobalParam->getPeakFilterData(id);
			if (data == nullptr)
			{
				info.append({ tip,"峰值筛选不存在" });
				return Tree_Failure;
			}
			if (data->_tableData.size() < 1)
			{
				info.append({ tip,"峰值筛选表格数据至少有一行" });
			}
			else
			{
				for (int i = 0; i < data->_tableData.size(); ++i)
				{
					if (data->_tableData[i].size() < 2)
					{
						info.append({ tip, QString("峰值筛选表格第%1行数据不完整").arg(i + 1) });
						continue;
					}
					if (data->_tableData[i][0] == 0)
					{
						info.append({ tip, QString("峰值筛选表格第%1行响应点未填写").arg(i + 1) });
					}
					if (data->_tableData[i][1] == 0)
					{
						info.append({ tip, QString("峰值筛选表格第%1行自由度未填写").arg(i + 1) });
					}
				}
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_ModalContri:
		{
			mModalContriData* data = nullptr;
			if (id == 0)
			{
				data = _mGlobalParam->getModalContriData();
			}
			else
			{
				auto caseBase = _caseManager.getCaseData(id);
				if (caseBase)
				{
					if (caseBase->getCaseType() == ImplicitFrequency)
					{
						auto analysis = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
						if (analysis) data = analysis->getModalContriData();
					}
					else if (caseBase->getCaseType() == ASCoupling)
					{
						auto analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
						if (analysis) data = analysis->getModalContriData();
					}
				}
			}
			if (data == nullptr)
			{
				info.append({ tip,"模态贡献量不存在" });
				return Tree_Failure;
			}
			//开启峰值筛选时输出集合可不选
			if (!data->_checkBoxPkScr)
			{
				//判断输出类型是structure还是fluid
				if (data->_outPutType == "structure")
				{
					//structure判断是否有输出集合映射
					int outSetId = _mapRelationManager.getMapRelation<int>({ Physical_ModalContri, data->getId() }, Physical_Set, MapRelation_OutSet);
					if (outSetId == -1)
					{
						info.append({ tip,"未选择结构输出集合" });
					}
				}
				else if (data->_outPutType == "fluid")
				{
					//fluid判断是否有输出集合映射
					int outSetId = _mapRelationManager.getMapRelation<int>({ Physical_ModalContri, data->getId() }, Physical_Set, MapRelation_OutSet);
					if (outSetId == -1)
					{
						info.append({ tip,"未选择声腔输出集合" });
					}
				}
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_NodeContri:
		{
			mNodeContriData* data = nullptr;
			if (id == 0)
			{
				data = _mGlobalParam->getNodeContriData();
			}
			else
			{
				auto caseBase = _caseManager.getCaseData(id);
				if (caseBase && caseBase->getCaseType() == ASCoupling)
				{
					auto analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
					if (analysis) data = analysis->getNodeContriData();
				}
			}
			if (data == nullptr)
			{
				info.append({ tip,"节点贡献量不存在" });
				return Tree_Failure;
			}
			//响应点集合是否有映射（开启峰值筛选时可不选）
			if (!data->_checkBoxPkScr)
			{
				int responseGridSetId = _mapRelationManager.getMapRelation<int>({ Physical_NodeContri, data->getId() }, Physical_Set, MapRelation_ResponseGridSet);
				if (responseGridSetId == -1)
				{
					info.append({ tip,"未选择响应点集合" });
				}
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_TPContri:
		{
			mTPContriData* data = nullptr;
			if (id == 0)
			{
				data = _mGlobalParam->getTPContriData();
			}
			// TPContri 目前仅存在于模型树下
			if (data == nullptr)
			{
				info.append({ tip,"传递路径贡献量不存在" });
				return Tree_Failure;
			}
			//判断连接点集合是否有映射
			int conptSetId = _mapRelationManager.getMapRelation<int>({ Physical_TPContri, data->getId() }, Physical_Set, MapRelation_ConptSet);
			if (conptSetId == -1)
			{
				info.append({ tip,"未选择连接点集合" });
			}
			//判断响应点集合是否有映射
			int responseGridSetId = _mapRelationManager.getMapRelation<int>({ Physical_TPContri, data->getId() }, Physical_Set, MapRelation_ResponseGridSet);
			if (responseGridSetId == -1)
			{
				info.append({ tip,"未选择响应点集合" });
			}
			//判断连接单元集合是否有映射
			int conelSetId = _mapRelationManager.getMapRelation<int>({ Physical_TPContri, data->getId() }, Physical_Set, MapRelation_ConelSet);
			if (conelSetId == -1)
			{
				info.append({ tip,"未选择连接单元集合" });
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_PanelContri:
		{
			mPanelContriData* data = nullptr;
			if (id == 0)
			{
				data = _mGlobalParam->getPanelContriData();
			}
			else
			{
				auto caseBase = _caseManager.getCaseData(id);
				if (caseBase && caseBase->getCaseType() == ASCoupling)
				{
					auto analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
					if (analysis) data = analysis->getPanelContriData();
				}
			}
			if (data == nullptr)
			{
				info.append({ tip,"面板贡献量不存在" });
				return Tree_Failure;
			}
			//全部可选 无需判断
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_SolveControl:
		{
			mSolveControlData* data = _mGlobalParam->getSolveControlData();
			if (data == nullptr)
			{
				info.append({ tip,"求解控制不存在" });
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_ERP:
		{
			mERPData* data = nullptr;
			if (id == 0)
			{
				data = _mGlobalParam->getERPData();
			}
			// ERP 目前仅存在于模型树下
			if (data == nullptr)
			{
				info.append({ tip,"等效辐射功率不存在" });
				return Tree_Failure;
			}
			//全部可选 无需判断
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_RotateAnalyse:
		{
			auto data = _rotateAnalyseManager.getRotateAnalyseData(id);
			if (data == nullptr)
			{
				info.append({ tip,"旋转分析不存在" });
				return Tree_Failure;
			}
			data->getNodeStatu(info);
			break;
		}
		case MxFunctions::Physical_PredefinedField:
		{
			auto data = _predefinedFieldManager.getPredefinedFieldData(id);
			if (data == nullptr)
			{
				info.append({ tip,"预定义场不存在" });
				return Tree_Failure;
			}
			// 初始温度和温度载荷在统一赋值时需要检查集合映射
			if (data->getPredefinedFieldType() == PREDEFINED_FIELD_INIT_TEMP)
			{
				auto initTemp = dynamic_cast<MechCaseData::mPredefinedFieldInitTemp*>(data.get());
				if (initTemp && initTemp->_defineType == 0)
				{
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_PredefinedField, id }, Physical_Set);
					if (setId == -1)
					{
						info.append({ tip,"未选择集合" });
					}
				}
			}
			else if (data->getPredefinedFieldType() == PREDEFINED_FIELD_TEMP_LOAD)
			{
				auto tempLoad = dynamic_cast<MechCaseData::mPredefinedFieldTempLoad*>(data.get());
				if (tempLoad && tempLoad->_defineType == 0)
				{
					int setId = _mapRelationManager.getMapRelation<int>({ Physical_PredefinedField, id }, Physical_Set);
					if (setId == -1)
					{
						info.append({ tip,"未选择集合" });
					}
				}
			}
			data->getNodeStatu(info);
			break;
		}
		case MxFunctions::Physical_NonStructuralQuality:
		{
			auto data = _nonStructuralQualityManager.getNonStructuralQualityData(id);
			if (data == nullptr)
			{
				info.append({tip, "非结构质量不存在"});
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_Field:
		{
			auto data = _fieldManager.getFieldData(id);
			if (data == nullptr)
			{
				info.append({tip, "场数据不存在"});
				return Tree_Failure;
			}
			data->getNodeStatus(info);
			break;
		}
		case MxFunctions::Physical_ModelFix:
		{
			int setid = _mapRelationManager.getMapRelation<int>({ Physical_ModelFix,id }, Physical_Set);
			if (setid == -1)
			{
				info.append({ tip,"未选择集合" });
				return Tree_Failure;
			}
			auto data = _modelFixManager.getModelFixData(id);
			data->getNodeStatus(info);
			break;
		}
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
		
	MechStructOptionData::mStructOption* mModelData::getStructOptionBase()
	{
		return _mStructOptionData;
	}

	MechStructOptionData::mStructOption* mModelData::createStructOption()
	{ 
		if (!_mStructOptionData)
		{
			_mStructOptionData = new mStructOption;
		}
		return _mStructOptionData;
	}

	void mModelData::deleteStructOption()
	{
		deleteAllByPhysicalType(Physical_OptimationConstraint);
		deleteAllByPhysicalType(Physical_OptimationDesign);
		_mapRelationManager.deleteMapRelation({ Physical_OptimationGoal ,_modelDataID });
		if (_mStructOptionData)
		{
			delete _mStructOptionData;
			_mStructOptionData = nullptr;
		}
	}

	MechStructOptionData::mRunnerStructOption* mModelData::getRunnerStructOption()
	{
		return _mRunnerStructOptionData;
	}

	MechStructOptionData::mRunnerStructOption* mModelData::createRunnerStructOption()
	{
		if (!_mRunnerStructOptionData)
		{
			_mRunnerStructOptionData = new mRunnerStructOption;
		}
		return _mRunnerStructOptionData;
	}

	void mModelData::deleteRunnerStructOption()
	{
		deleteAllByPhysicalType(Physical_RunnerOptimationGoal);
		deleteAllByPhysicalType(Physical_RunnerOptimationDesign);
		_mapRelationManager.deleteMapRelation({ Physical_RunnerOptimationGoal ,_modelDataID });
		if (_mRunnerStructOptionData)
		{
			delete _mRunnerStructOptionData;
			_mRunnerStructOptionData = nullptr;
		}
	}

	MechGlobalParamData::mGlobalParam* mModelData::getGlobalParam()
	{
		return _mGlobalParam;
	}

	void mModelData::deleteGlobalParam()
	{
		deleteAllByPhysicalType(Physical_GenericParam);
		deleteByPhysicalType(Physical_GenericAssign, _modelDataID);
		deleteByPhysicalType(Physical_GenericMatrix, _modelDataID);
		deleteByPhysicalType(Physical_ModalContri, _modelDataID);
		deleteByPhysicalType(Physical_NodeContri, _modelDataID);
		deleteByPhysicalType(Physical_TPContri, _modelDataID);
		deleteByPhysicalType(Physical_PanelContri, _modelDataID);
		deleteByPhysicalType(Physical_ERP, _modelDataID);
		deleteByPhysicalType(Physical_PeakFilter, _modelDataID);
		deleteByPhysicalType(Physical_GenericPart, _modelDataID);
		deleteByPhysicalType(Physical_ASCouplingSearch, _modelDataID);
		deleteByPhysicalType(Physical_SolveControl, _modelDataID);
	}

	QString mModelData::getRenderNameByTypeId(MxFunctions::PhysicalFieldType type, const int& id)
	{
		switch (type)
		{
		case MxFunctions::Physical_Coordinate:
			return QString("coordinate_%1").arg(id);
		case MxFunctions::Physical_LoadBoundary:
		{
			auto data = _loadboundaryManager.getLoadBoundaryData(id);
			if (!data)
				return QString();
			if (data->isLoadOrBoundary() == 0)
				return QString("load_%1").arg(id);
			else if (data->isLoadOrBoundary() == 1)
				return QString("boundary_%1").arg(id);
			return QString();
		}
		case MxFunctions::Physical_PredefinedField:
			return QString("predefinedfield_%1").arg(id);
		case MxFunctions::Physical_RotateAnalyse:
			return QString("rotateanalyse_%1").arg(id);
		case MxFunctions::Physical_Connector:
			return QString("connector_%1").arg(id);
		case MxFunctions::Physical_Deputy:
			return QString("deputy_%1").arg(id);
		case MxFunctions::Physical_Contact:
			return QString("contact_%1").arg(id);
		case MxFunctions::Physical_MeshPanel:
		{
			auto data = _meshPanelManager.getMeshPanelData(id);
			if (data && data->getMeshPanelType() == MxFunctions::CustomEncryption)
			{
				// 自定义加密网格符号名称直接存储在 _Renderlabel 中
				if (!data->_Renderlabel.empty())
					return data->_Renderlabel.front();
			}
			return QString();
		}
		default:
			break;
		}
		return QString();
	}

	bool mModelData::getContactSetDataStatus(int setId)
	{
		if (setId == -1)
		{
			return false;
		}
		MechSetData::mSetBase* set = _setManager.getSetData(setId);
		if (set == nullptr)
		{
			return false;
		}

		if (set->getSetType() == SetType::Mix)
		{
			auto mixset = dynamic_cast<MechSetData::mMixSet*>(set);
			if (mixset == nullptr)
			{
				return false;
			}
			std::vector<RenderPickResultData> data;
			std::vector<int> ids;
			PickDataType pickDataType;
			mixset->getMixSetData(pickDataType, data, ids);
			if (pickDataType == PickDataType::None)
			{
				return false;
			}
		}
		else if (set->getSetType() == GeoFace)
		{
			auto geoset = dynamic_cast<MechSetData::mGeoSet*>(set);
			if (geoset == nullptr || geoset->getGeoSetData().empty())
			{
				return false;
			}
		}
		else if (set->getSetType() == ElementSurface)
		{
			auto elesurfaceset = dynamic_cast<MechSetData::mElementSurface*>(set);
			if (elesurfaceset == nullptr || elesurfaceset->getElementSurfaceData().empty())
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}


	void mModelData::addPhysicalFieldP(std::shared_ptr<MechMaterialData::mMaterialBase> mat)
	{
		_materialManager.AddMaterialData(mat);
	}

	void mModelData::addPhysicalFieldP(std::shared_ptr<MechCaseData::mLoadBoundaryBase> load, int caseID) //边界与载荷映射统一
	{
		if (caseID != -1)
		{
			_mapRelationManager.appendMapRelation({ Physical_LoadBoundary,load->getLoadBoundaryId() }, { Physical_Case,caseID });
		}
		_loadboundaryManager.AddLoadBoundaryData(load);
	}

	void mModelData::addPhysicalFieldP(std::shared_ptr<MechCaseData::mPredefinedFieldBase> field)
	{
		_predefinedFieldManager.AddPredefinedFieldData(field);
	}

	void mModelData::addPhysicalFieldP(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateAnalyse, int caseID)
	{
		if (caseID != -1)
		{
			_mapRelationManager.appendMapRelation({ Physical_RotateAnalyse, rotateAnalyse->getRotateAnalyseId() }, { Physical_Case, caseID });
		}
		_rotateAnalyseManager.AddRotateAnalyseData(rotateAnalyse);
	}
		
	void mModelData::addPhysicalFieldP(std::shared_ptr<MechConnectorData::mConnectorBase> conn)
	{
		_connectorManager.AddConnectorData(conn);
	}

	void mModelData::addPhysicalFieldP(std::shared_ptr<MechConnectorData::mDeputyBase> deputy)
	{
		_deputyManager.AddDeputyData(deputy);
	}

	void mModelData::addPhysicalFieldP(std::shared_ptr<MechModelFixData::mModelFixDataBase> modelFix)
	{
		_modelFixManager.AddModelFixData(modelFix);
	}

	void mModelData::addPhysicalFieldP(QPointer<PostData::mPostDataBase> post)
	{
		_postDataManager.addPostDatabase(post);
	}

}