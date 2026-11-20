#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "MXModelData_global.h"

#include "mUnitSystemManager.h"
#include "mMapRelationManager.h"
#include "mMaterialManager.h"
#include "mCaseManager.h"
#include "mSetManager.h"
#include "mCoordinateManager.h"
#include "mPropertyManager.h"
#include "mNonStructQualityManager.h"
#include "mFieldManager.h"
#include "mDiscreteField.h"
#include "mAnalysisField.h"
#include "mMeshPanelManager.h"
#include "mMeshOptPanelManager.h"
#include "mPostDataManager.h"
#include "mFunctionManager.h"
#include "mSectionShapeManager.h"
#include "mConnectorManager.h"
#include "mDeputyManager.h"
#include "mContactManager.h"
#include "mPeriodicBoundaryManager.h"
#include "mLoadBoundaryManager.h"
#include "mMotionBodyManager.h"
#include "mMotionDriveManager.h"
#include "mMotionJointManager.h"
#include "mMotionCouplingManager.h"
#include "mMotionScenarioManager.h"
#include "mMotionMarkerManager.h"
#include "mMotionForceElementManager.h"
#include "mMotionVectorForceManager.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionContactManager.h"
#include "mMotionSpringManager.h"
#include "mMotionSpringBase.h"
#include "mMotionSensorManager.h"
#include "mMotionControllerManager.h"
#include "mMotionAutoFlexManager.h"
#include "mMotionControlInputManager.h"
#include "mMotionElectroImportManager.h"
#include "mMotionElectroExportManager.h"
#include "mMotionHydraulicManager.h"
#include "mMotionRLManager.h"
#include "mPredefinedFieldManager.h"
#include "mRotateAnalyseManager.h"
#include "mFreqSamplPointManager.h"
#include "mLaminateManager.h"
#include "mModelFixManager.h"
#include "mFrpManager.h"
#include "mSolveTaskManager.h"
//装配层级关系管理（纯引用组织层，不参与求解）
#include "mAssemblyManager.h"
//【OCC摘除·S10/P0-1】GeoEngineData 已 pimpl 化收口：本 include 零 OCC（仅句柄），
// server()/模板类型不再暴露，GUI getGEOServer 宏已切 engine() 同语法视图。
#include "mGeoEngineData.h"

#include "mPhysicalFieldEnum.h"
#include "mGlobalSignals.h"
#include <stdexcept>
#include <memory>

class mMeshData;

namespace MechElementData
{
	class mDataElement;
}
namespace MGeoData {
	class GeoDataBase;
}

namespace MGeoManager {
	class GeoManager;
}
namespace MechStructOptionData
{
	class mStructOption;
	class mRunnerStructOption;
}

namespace MechGlobalParamData
{
	class mGlobalParam;
}

using namespace CommonData;
namespace MechModelData
{
	class MXMODELDATA_EXPORT mModelData
	{
	public:
		mModelData();

		~mModelData();

		/*
		* 获取模型数据id
		*/
		int getModelDataID();

		/*
		* 重置模型数据全局ID计数（会话恢复等"清空整个软件数据"场景使用：
		* 全部模型删除后调用，使新建模型的ID从初始值重新分配，避免ID无限累加。
		* 仅在没有存活 mModelData 实例时调用才安全）
		*/
		static void resetModelGlobalID();

		/*
		* 设置/获取模型名称（模型树名称与视口命名前缀的唯一来源）
		*/
		void setModelDataName(const QString& name);
		QString getModelDataName() const;

		/*
		* 获取物理场管理类
		*/
		template<typename T>
		T& getManager(MxFunctions::PhysicalFieldType type);

		/*
		* 创建名称（根据物理场数据类型和前缀）
		*/
		QString createNameByType(MxFunctions::PhysicalFieldType type, const QString &text);

		/*
		* 判断名称是否存在
		*/
		bool isExistName(MxFunctions::PhysicalFieldType type, const QString& name, bool isInternal = false);

		/*
		* 获取所有名称
		*/
		QStringList getAllNames(MxFunctions::PhysicalFieldType type, const int& caseID = -1);

		/*
		* 获取所有ID
		*/
		std::vector<int> getAllPhyID(MxFunctions::PhysicalFieldType type, const int& caseID = -1);

		/*
		* 通过ID获取名称
		*/
		QString getNameByPhyID(MxFunctions::PhysicalFieldType type, int id);

		/*
		* 通过名称获取ID
		*/
		int getPhyIDByName(MxFunctions::PhysicalFieldType type, QString name, bool isContainAutoCreate = false);

		/*
		* 添加数据
		*/
		template<typename T>
		void addPhysicalField(T* t);

		/*
		* 添加智能指针数据
		*/
		void addPhysicalFieldP(std::shared_ptr<MechMaterialData::mMaterialBase> mat);
		void addPhysicalFieldP(std::shared_ptr<MechCaseData::mLoadBoundaryBase> load,int caseID = -1);
		void addPhysicalFieldP(std::shared_ptr<MechCaseData::mPredefinedFieldBase> field);
		void addPhysicalFieldP(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateAnalyse, int caseID = -1);
		void addPhysicalFieldP(std::shared_ptr<MechConnectorData::mConnectorBase> conn);
		void addPhysicalFieldP(std::shared_ptr<MechConnectorData::mDeputyBase> deputy);
		void addPhysicalFieldP(std::shared_ptr<MechModelFixData::mModelFixDataBase> modelFix);
		void addPhysicalFieldP(QPointer<PostData::mPostDataBase> post);

		/*
		* 重命名
		*/
		void resetName(MxFunctions::PhysicalFieldType type, const QString& oldName, const QString& newName);

		/*
		* 拷贝物理场数据
		*/
		int copyPhysicalField(MxFunctions::PhysicalFieldType type,const int& id, const int& caseID = -1);

		/*
		* 删除某种类型所有物理场数据
		*/
		void deleteAllByPhysicalType(MxFunctions::PhysicalFieldType type, const int& caseID = -1);

		/*
		* 删除某种类型物理场数据
		*/
		void deleteByPhysicalType(MxFunctions::PhysicalFieldType type, const int& id, const int& caseID = -1);

		/*
		* 获取网格数据指针
		*/
		mMeshData* getMeshData();

		/*
		* 获取几何数据
		*/
		MGeoData::GeoDataBase* getGeoData();

		/*
		* 获取单元数据
		*/
		MechElementData::mDataElement* getElementData();

		/**
		 * 获取几何数据的Manager
		 */
		MGeoManager::GeoManager* getGeoManager();

		/*
		* 获取几何引擎数据mgr
		*/
		GeoEngineConfig::GeoEngineData* getGeoEngineDataMgr();
		//【老类退役·S7】工程文件(.mxcae)几何段支持（实现在 cpp，避免导出头拉重型类型）：
		// saveEngineGeometrySection：GN 部件经引擎 MXCAE 格式嵌入工程流（含段魔数）；
		// loadEngineGeometrySection：读段并经引擎同步导入恢复；
		// adoptLegacyGeometry：旧世界 set 桥回 GN 世界并触发显示。
		void saveEngineGeometrySection(QDataStream& out);
		void loadEngineGeometrySection(QDataStream& in);
		void adoptLegacyGeometry();
		//装配层级段（魔数 + atEnd 判别，旧版本文件无此段时装配分组为空，行为等同现状）
		void saveAssemblySection(QDataStream& out);
		void loadAssemblySection(QDataStream& in);

		/*
		* 获取映射关系
		*/
		MechMapRelation::mMapRelationManager& getMapRelationManager();

		/*
		* 获取装配关系管理器（装配层级唯一可信源）
		*/
		AssemblyData::mAssemblyManager& getAssemblyManager();

		/*
		* 获取单位制系统
		*/
		mUnitSystemManager* getUnitSystemManager();

		/*
		* 单位制切换，数据更新
		*/
		void updateDataFromUnitSystem(const int& unitidx);

		/*
		* 模型检查
		*/
		bool checkModelData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getDataStatus(QVector<QPair<QString, QString>>& info, MxFunctions::PhysicalFieldType type, int id);

		/*
		* 获取结构优化指针
		*/
		MechStructOptionData::mStructOption* getStructOptionBase();

		/*
		* 创建结构优化
		*/
		MechStructOptionData::mStructOption* createStructOption();

		/*
		* 删除结构优化
		*/
		void deleteStructOption();

		/*
		* 获取流道优化指针
		*/
		MechStructOptionData::mRunnerStructOption* getRunnerStructOption();

		/*
		* 创建流道优化
		*/
		MechStructOptionData::mRunnerStructOption* createRunnerStructOption();

		/*
		* 删除流道优化
		*/
		void deleteRunnerStructOption();

		/*
		* 获取全局参数设置指针
		*/
		MechGlobalParamData::mGlobalParam* getGlobalParam();

		/*
		* 删除全局参数设置
		*/
		void deleteGlobalParam();

		/*
		* 根据该类型，ID获取渲染名称
		*/
		QString getRenderNameByTypeId(MxFunctions::PhysicalFieldType type, const int& id);

		/*
		* 输入输出流
		*/
		friend QDataStream& operator>>(QDataStream& in, mModelData& obj);
		friend QDataStream& operator<<(QDataStream& out, mModelData& obj);
		friend QDataStream& InStreammModelDataV3(QDataStream& in, mModelData& obj);


	private:

		//返回集合是否包含有效的几何/混合集数据
		bool getContactSetDataStatus(int setId);

	private:
		static int _modelGlobalID;
		int _modelDataID;
		QString _modelDataName;

		mMeshData* _meshData = nullptr;
		MechElementData::mDataElement* _elementData = nullptr;
		MGeoData::GeoDataBase* _geoDataBase = nullptr;
		MGeoManager::GeoManager* _geoManager = nullptr;
		/*几何引擎*/
		std::unique_ptr<GeoEngineConfig::GeoEngineData> _geoEngineData;
		/*几何引擎*/
		MechMapRelation::mMapRelationManager _mapRelationManager;
		mUnitSystemManager* _unitSystemManager;
		MechStructOptionData::mStructOption* _mStructOptionData = nullptr;
		MechStructOptionData::mRunnerStructOption* _mRunnerStructOptionData = nullptr;
		MechGlobalParamData::mGlobalParam* _mGlobalParam = nullptr;
		MechMaterialData::mMaterialManager _materialManager;
		MechCaseData::mCaseManager _caseManager;
		MechCaseData::mLoadBoundaryManager _loadboundaryManager;
		MechSetData::mSetManager _setManager;
		MechCoordinateData::mCoordinateManager _coordinateManager;
		MechFunctionData::mFunctionManager _functionManager;
		MechPropertyData::mPropertyManager _propertyManager;
		MechCaseData::mNonStructQualityManager _nonStructuralQualityManager;
		//场管理类（离散场/分析场统一存储）
		MechDiscreteFieldData::mFieldManager _fieldManager;

		MeshPanelData::mMeshPanelManager _meshPanelManager;
		MeshPanelData::mMeshOptPanelManager _meshOptPanelManager;
		PostData::mPostDataManager _postDataManager;
		MechSectionShapeData::mSectionShapeManager _sectionShapeManager;
		MechConnectorData::mConnectorManager _connectorManager;
		MechConnectorData::mDeputyManager _deputyManager;
		MechContactData::mContactManager _contactManager;
		MechContactData::mPeriodicBoundaryManager _periodicBoundaryManager;
		MechCaseData::mPredefinedFieldManager _predefinedFieldManager;
		MechCaseData::mRotateAnalyseManager _rotateAnalyseManager;
		MechCaseData::mFreqSamplPointManager _FreqSamplPointManager;
		MechPropertyData::mLaminateManager _LaminateManager;
		MechMotionData::mMotionBodyManager _motionBodyManager;
		MechMotionData::mMotionJointManager _motionJointManager;
		MechMotionData::mMotionCouplingManager _motionCouplingManager;
		MechMotionData::mMotionDriveManager _motionDriveManager;
		MechMotionData::mMotionScenarioManager _motionScenarioManager;
		MechMotionData::mMotionMarkerManager _motionMarkerManager;
		MechMotionData::mMotionForceElementManager _motionForceElementManager;
		MechMotionData::mMotionContactManager _motionContactManager;
		MechMotionData::mMotionSpringManager _motionSpringManager;
		MechMotionData::mMotionSensorManager _motionSensorManager;
		MechMotionData::mMotionControllerManager _motionControllerManager;
		MechMotionData::mMotionAutoFlexManager _motionAutoFlexManager;
		MechMotionData::mMotionControlInputManager _motionControlInputManager;
		MechMotionData::mMotionElectroImportManager _motionElectroImportManager;
		MechMotionData::mMotionElectroExportManager _motionElectroExportManager;
		MechMotionData::mMotionHydraulicManager _motionHydraulicManager;
		MechMotionData::mMotionRLManager _motionRLManager;
		MechModelFixData::mModelFixManager _modelFixManager;
		MechFrpData::mFrpManager _mFrpManager;
		MechSolveTaskData::mSolveTaskManager _solveTaskManager;
		AssemblyData::mAssemblyManager _assemblyManager;
	};



	//材料管理类
	template<>
	inline MechMaterialData::mMaterialManager& mModelData::getManager<MechMaterialData::mMaterialManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Material)
			return _materialManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMaterialData::mMaterialBase>(MechMaterialData::mMaterialBase* t)
	{
		_materialManager.AddMaterialData(t);
	};

	//连接管理类
	template<>
	inline MechConnectorData::mConnectorManager& mModelData::getManager<MechConnectorData::mConnectorManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Connector)
			return _connectorManager;
		throw std::runtime_error("type error");
	}

	//连接副管理类
	template<>
	inline MechConnectorData::mDeputyManager& mModelData::getManager<MechConnectorData::mDeputyManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Deputy)
			return _deputyManager;
		throw std::runtime_error("type error");
	}

	//分析工况管理类
	template<>
	inline MechCaseData::mCaseManager& mModelData::getManager<MechCaseData::mCaseManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Case)
			return _caseManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechCaseData::mCaseBase>(MechCaseData::mCaseBase* t)
	{
		_caseManager.AddCaseData(t);
	};

	//载荷边界管理类
	template<>
	inline MechCaseData::mLoadBoundaryManager& mModelData::getManager<MechCaseData::mLoadBoundaryManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_LoadBoundary)
			return _loadboundaryManager;
		throw std::runtime_error("type error");
	}

	//预定义场管理类
	template<>
	inline MechCaseData::mPredefinedFieldManager& mModelData::getManager<MechCaseData::mPredefinedFieldManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_PredefinedField)
			return _predefinedFieldManager;
		throw std::runtime_error("type error");
	}

	//旋转分析管理类
	template<>
	inline MechCaseData::mRotateAnalyseManager& mModelData::getManager<MechCaseData::mRotateAnalyseManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_RotateAnalyse)
			return _rotateAnalyseManager;
		throw std::runtime_error("type error");
	}
	//频率采样点管理类
	template<>
	inline MechCaseData::mFreqSamplPointManager& mModelData::getManager<MechCaseData::mFreqSamplPointManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_FreqSamplPoint)
			return _FreqSamplPointManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechCaseData::mFreqSamplPointBase>(MechCaseData::mFreqSamplPointBase* t)
	{
		_FreqSamplPointManager.AddFreqSamplPointData(t);
	};

	//集合管理类
	template<>
	inline MechSetData::mSetManager& mModelData::getManager<MechSetData::mSetManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Set)
			return _setManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechSetData::mSetBase>(MechSetData::mSetBase* t)
	{
		_setManager.AddSetData(t);
	};

	//坐标系管理类
	template<>
	inline MechCoordinateData::mCoordinateManager& mModelData::getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Coordinate)
			return _coordinateManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechCoordinateData::mCoordinateBase>(MechCoordinateData::mCoordinateBase* t)
	{
		_coordinateManager.AddCoordinateData(t);
	};

	//函数管理类
	template<>
	inline MechFunctionData::mFunctionManager& mModelData::getManager<MechFunctionData::mFunctionManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Function)
			return _functionManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechFunctionData::mFunctionBase>(MechFunctionData::mFunctionBase* t)
	{
		_functionManager.AddFunctionData(t);
	};

	//截面形状管理类
	template<>
	inline MechSectionShapeData::mSectionShapeManager& mModelData::getManager<MechSectionShapeData::mSectionShapeManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_PropertyShape)
			return _sectionShapeManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechSectionShapeData::mSectionShapeBase>(MechSectionShapeData::mSectionShapeBase* t)
	{
		_sectionShapeManager.AddSectionShapeData(t);
	};

	//层合结构管理类
	template<>
	inline MechPropertyData::mLaminateManager& mModelData::getManager<MechPropertyData::mLaminateManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Laminate)
			return _LaminateManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechPropertyData::mLaminateBase>(MechPropertyData::mLaminateBase* t)
	{
		_LaminateManager.AddLaminateData(t);
	};

	//属性管理类
	template<>
	inline MechPropertyData::mPropertyManager& mModelData::getManager<MechPropertyData::mPropertyManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Property)
			return _propertyManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechPropertyData::mPropertyBase>(MechPropertyData::mPropertyBase* t)
	{
		_propertyManager.AddPropertyData(t);
	};

	//非结构质量管理类
	template<>
	inline MechCaseData::mNonStructQualityManager& mModelData::getManager<MechCaseData::mNonStructQualityManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_NonStructuralQuality)
			return _nonStructuralQualityManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechCaseData::mNonStructuralQuality>(MechCaseData::mNonStructuralQuality* t)
	{
		_nonStructuralQualityManager.AddNsmData(t);
	};

	//场管理类（离散场/分析场统一）
	template<>
	inline MechDiscreteFieldData::mFieldManager& mModelData::getManager<MechDiscreteFieldData::mFieldManager>
		(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Field)
			return _fieldManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechDiscreteFieldData::mDiscreteField>(MechDiscreteFieldData::mDiscreteField* t)
	{
		_fieldManager.addFieldData(t);
	};
	template<>
	inline void mModelData::addPhysicalField<MechDiscreteFieldData::mAnalysisField>(MechDiscreteFieldData::mAnalysisField* t)
	{
		_fieldManager.addFieldData(t);
	};

	//接触管理类
	template<>
	inline MechContactData::mContactManager& mModelData::getManager<MechContactData::mContactManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Contact)
			return _contactManager;
		throw std::runtime_error("type error");
	}

	//模型修复管理类
	template<>
	inline MechModelFixData::mModelFixManager& mModelData::getManager<MechModelFixData::mModelFixManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_ModelFix)
			return _modelFixManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechModelFixData::mModelFixDataBase>(MechModelFixData::mModelFixDataBase* t)
	{
		_modelFixManager.AddModelFixData(std::shared_ptr<MechModelFixData::mModelFixDataBase>(t));
	};
	template<>
	inline void mModelData::addPhysicalField<MechContactData::mContactBase>(MechContactData::mContactBase* t)
	{
		_contactManager.AddContactData(t);
	};

	//多体动力学运动体管理类
	template<>
	inline MechMotionData::mMotionBodyManager& mModelData::getManager<MechMotionData::mMotionBodyManager>(MxFunctions::PhysicalFieldType type) {
		return _motionBodyManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionBody>(MechMotionData::mMotionBody* t)
	{
		_motionBodyManager.addData(t);
	};

	//接触管理类
	template<>
	inline MechMotionData::mMotionJointManager& mModelData::getManager<MechMotionData::mMotionJointManager>(MxFunctions::PhysicalFieldType type) {
		return _motionJointManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionJointBase>(MechMotionData::mMotionJointBase* t)
	{
		_motionJointManager.addData(t);
	};

	//耦合副管理类
	template<>
	inline MechMotionData::mMotionCouplingManager& mModelData::getManager<MechMotionData::mMotionCouplingManager>(MxFunctions::PhysicalFieldType type) {
		return _motionCouplingManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionCouplingBase>(MechMotionData::mMotionCouplingBase* t)
	{
		_motionCouplingManager.addData(t);
	};

	//驱动管理类
	template<>
	inline MechMotionData::mMotionDriveManager& mModelData::getManager<MechMotionData::mMotionDriveManager>(MxFunctions::PhysicalFieldType type) {
		return _motionDriveManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionDriveBase>(MechMotionData::mMotionDriveBase* t)
	{
		_motionDriveManager.addData(t);
	};

	//
	template<>
	inline MechMotionData::mMotionScenarioManager& mModelData::getManager<MechMotionData::mMotionScenarioManager>(MxFunctions::PhysicalFieldType type) {
		return _motionScenarioManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionScenario>(MechMotionData::mMotionScenario* t)
	{
		_motionScenarioManager.addData(t);
	};

	//标记点管理类
	template<>
	inline MechMotionData::mMotionMarkerManager& mModelData::getManager<MechMotionData::mMotionMarkerManager>(MxFunctions::PhysicalFieldType type) {
		return _motionMarkerManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionMarker>(MechMotionData::mMotionMarker* t)
	{
		_motionMarkerManager.addData(t);
	};

	//力元管理类（统一管理矢量力/力矩/衬套/标量力）
	template<>
	inline MechMotionData::mMotionForceElementManager& mModelData::getManager<MechMotionData::mMotionForceElementManager>(MxFunctions::PhysicalFieldType type) {
		return _motionForceElementManager;
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionForceElement>(MechMotionData::mMotionForceElement* t)
	{
		_motionForceElementManager.addData(t);
	};

	//接触管理类
	template<>
	inline MechMotionData::mMotionContactManager& mModelData::getManager<MechMotionData::mMotionContactManager>(MxFunctions::PhysicalFieldType type) {
		return _motionContactManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionContact>(MechMotionData::mMotionContact* t)
	{
		_motionContactManager.addData(t);
	};

	//传感器管理类
	template<>
	inline MechMotionData::mMotionSpringManager& mModelData::getManager<MechMotionData::mMotionSpringManager>(MxFunctions::PhysicalFieldType type) {
		return _motionSpringManager;
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionSpringBase>(MechMotionData::mMotionSpringBase* t)
	{
		_motionSpringManager.addData(t);
	}

	template<>
	inline MechMotionData::mMotionSensorManager& mModelData::getManager<MechMotionData::mMotionSensorManager>(MxFunctions::PhysicalFieldType type) {
		return _motionSensorManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionSensorBase>(MechMotionData::mMotionSensorBase* t)
	{
		_motionSensorManager.addData(t);
	};

	//控制器管理类
	template<>
	inline MechMotionData::mMotionControllerManager& mModelData::getManager<MechMotionData::mMotionControllerManager>(MxFunctions::PhysicalFieldType type) {
		return _motionControllerManager;
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionControllerBase>(MechMotionData::mMotionControllerBase* t)
	{
		_motionControllerManager.addData(t);
	};

	//自动柔性体管理类
	template<>
	inline MechMotionData::mMotionAutoFlexManager& mModelData::getManager<MechMotionData::mMotionAutoFlexManager>(MxFunctions::PhysicalFieldType type) {
		return _motionAutoFlexManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionAutoFlex>(MechMotionData::mMotionAutoFlex* t)
	{
		_motionAutoFlexManager.addData(t);
	};

	//控制输入管理类
	template<>
	inline MechMotionData::mMotionControlInputManager& mModelData::getManager<MechMotionData::mMotionControlInputManager>(MxFunctions::PhysicalFieldType type) {
		return _motionControlInputManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionControlInput>(MechMotionData::mMotionControlInput* t)
	{
		_motionControlInputManager.addData(t);
	};

	//机电导入管理类
	template<>
	inline MechMotionData::mMotionElectroImportManager& mModelData::getManager<MechMotionData::mMotionElectroImportManager>(MxFunctions::PhysicalFieldType type) {
		return _motionElectroImportManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionElectroImport>(MechMotionData::mMotionElectroImport* t)
	{
		_motionElectroImportManager.addData(t);
	};

	//机电导出管理类
	template<>
	inline MechMotionData::mMotionElectroExportManager& mModelData::getManager<MechMotionData::mMotionElectroExportManager>(MxFunctions::PhysicalFieldType type) {
		return _motionElectroExportManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionElectroExport>(MechMotionData::mMotionElectroExport* t)
	{
		_motionElectroExportManager.addData(t);
	};

	//液压管理类
	template<>
	inline MechMotionData::mMotionHydraulicManager& mModelData::getManager<MechMotionData::mMotionHydraulicManager>(MxFunctions::PhysicalFieldType type) {
		return _motionHydraulicManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionHydraulic>(MechMotionData::mMotionHydraulic* t)
	{
		_motionHydraulicManager.addData(t);
	};

	//强化学习管理类
	template<>
	inline MechMotionData::mMotionRLManager& mModelData::getManager<MechMotionData::mMotionRLManager>(MxFunctions::PhysicalFieldType type) {
		return _motionRLManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechMotionData::mMotionRL>(MechMotionData::mMotionRL* t)
	{
		_motionRLManager.addData(t);
	};

	//周期性边界管理类
	template<>
	inline MechContactData::mPeriodicBoundaryManager& mModelData::getManager<MechContactData::mPeriodicBoundaryManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_PeriodicBoundary)
			return _periodicBoundaryManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MechContactData::mPeriodicBoundary>(MechContactData::mPeriodicBoundary* t)
	{
		_periodicBoundaryManager.AddPeriodicBoundaryData(t);
	};

	//网格面板管理类
	template<>
	inline MeshPanelData::mMeshPanelManager& mModelData::getManager<MeshPanelData::mMeshPanelManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_MeshPanel)
			return _meshPanelManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MeshPanelData::mMeshPanelBase>(MeshPanelData::mMeshPanelBase* t)
	{
		_meshPanelManager.AddMeshPanelData(t);
	};
	//网格操作面板管理类
	template<>
	inline MeshPanelData::mMeshOptPanelManager& mModelData::getManager<MeshPanelData::mMeshOptPanelManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_MeshOptPanel)
			return _meshOptPanelManager;
		throw std::runtime_error("type error");
	}
	template<>
	inline void mModelData::addPhysicalField<MeshPanelData::mMeshOptPanelBase>(MeshPanelData::mMeshOptPanelBase* t)
	{
		_meshOptPanelManager.AddMeshPanelData(t);
	};

	//后处理管理类
	template<>
	inline PostData::mPostDataManager& mModelData::getManager<PostData::mPostDataManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_Post)
			return _postDataManager;
		throw std::runtime_error("type error");
	}

	//求解任务管理类
	template<>
	inline MechSolveTaskData::mSolveTaskManager& mModelData::getManager<MechSolveTaskData::mSolveTaskManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_SolveTask)
			return _solveTaskManager;
		throw std::runtime_error("type error");
	}

	//复合材料管理类
	template<>
	inline MechFrpData::mFrpManager& mModelData::getManager<MechFrpData::mFrpManager>(MxFunctions::PhysicalFieldType type) {
		if (type == MxFunctions::Physical_FRCMaterial)
			return _mFrpManager;
		throw std::runtime_error("type error");
	}
}