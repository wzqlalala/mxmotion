#pragma once

#include "mMapRelationManager.h"
#include <QJsonObject>
#include <QStringList>
#include <tuple>
#include <type_traits>
#include <functional>
#include <stdexcept>
#include "mMotionBodyManager.h"
#include "mMotionBody.h"
#include "mMotionJointManager.h"
#include "mMotionJointBase.h"
#include "mMotionCouplingManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionSpringManager.h"
#include "mMotionSpringBase.h"
#include "mMotionForceElementManager.h"
#include "mMotionForceElement.h"
#include "mMotionContactManager.h"
#include "mMotionContact.h"
#include "mMotionMarkerManager.h"
#include "mMotionMarker.h"
#include "mMotionSensorManager.h"
#include "mMotionSensorBase.h"
#include "mMotionControllerManager.h"
#include "mMotionControllerBase.h"
#include "mMotionControlInputManager.h"
#include "mMotionControlInput.h"
#include "mMotionScenarioManager.h"
#include "mMotionScenario.h"
#include "mMotionAutoFlexManager.h"
#include "mMotionAutoFlex.h"
#include "mMotionElectroImportManager.h"
#include "mMotionElectroImport.h"
#include "mMotionElectroExportManager.h"
#include "mMotionElectroExport.h"
#include "mMotionHydraulicManager.h"
#include "mMotionHydraulic.h"
#include "mMotionRLManager.h"
#include "mMotionRL.h"
#include "mMaterialManager.h"

class mMeshData;
namespace GeoNeutralData { class GeoEngineData; }
namespace CommonData { class mUnitSystemManager; }
namespace MotionOcc { class LegacyMotionContext; }
namespace MechModelData {
class mModelData {
public:
    mModelData();
    ~mModelData();
    mModelData(const mModelData&)=delete;
    mModelData& operator=(const mModelData&)=delete;
    template<class T> T& getManager(MxFunctions::PhysicalFieldType) { return std::get<T>(managers_); }
    template<class T> const T& getManager(MxFunctions::PhysicalFieldType) const { return std::get<T>(managers_); }
    void addPhysicalFieldP(std::shared_ptr<MechMaterialData::mMaterialBase> value) {
        std::get<MechMaterialData::mMaterialManager>(managers_).AddMaterialData(std::move(value));
    }
    template<class T> void addPhysicalField(T* object) {
        if constexpr (std::is_base_of_v<MechMotionData::mMotionBody,T>) std::get<MechMotionData::mMotionBodyManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionJointBase,T>) std::get<MechMotionData::mMotionJointManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionCouplingBase,T>) std::get<MechMotionData::mMotionCouplingManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionDriveBase,T>) std::get<MechMotionData::mMotionDriveManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionSpringBase,T>) std::get<MechMotionData::mMotionSpringManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionForceElement,T>) std::get<MechMotionData::mMotionForceElementManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionContact,T>) std::get<MechMotionData::mMotionContactManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionMarker,T>) std::get<MechMotionData::mMotionMarkerManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionSensorBase,T>) std::get<MechMotionData::mMotionSensorManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionControllerBase,T>) std::get<MechMotionData::mMotionControllerManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionControlInput,T>) std::get<MechMotionData::mMotionControlInputManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionScenario,T>) std::get<MechMotionData::mMotionScenarioManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionAutoFlex,T>) std::get<MechMotionData::mMotionAutoFlexManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionElectroImport,T>) std::get<MechMotionData::mMotionElectroImportManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionElectroExport,T>) std::get<MechMotionData::mMotionElectroExportManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionHydraulic,T>) std::get<MechMotionData::mMotionHydraulicManager>(managers_).addData(object);
        else if constexpr (std::is_base_of_v<MechMotionData::mMotionRL,T>) std::get<MechMotionData::mMotionRLManager>(managers_).addData(object);
        else static_assert(!sizeof(T), "Unsupported model data type");
    }
    MechMapRelation::mMapRelationManager& getMapRelationManager() { return references_; }
    const MechMapRelation::mMapRelationManager& getMapRelationManager() const { return references_; }
    QString createNameByType(MxFunctions::PhysicalFieldType type,const QString& prefix);
    QString getNameByPhyID(MxFunctions::PhysicalFieldType type,int id);
    int getPhyIDByName(MxFunctions::PhysicalFieldType type,QString name,bool = false);
    QStringList getAllNames(MxFunctions::PhysicalFieldType type,int = -1);
    std::vector<int> getAllPhyID(MxFunctions::PhysicalFieldType type,int = -1);
    bool isExistName(MxFunctions::PhysicalFieldType type,const QString& name,bool = false);
    void deleteAllByPhysicalType(MxFunctions::PhysicalFieldType type,int = -1);
    void deleteByPhysicalType(MxFunctions::PhysicalFieldType type,int id,int = -1);
    int copyPhysicalField(MxFunctions::PhysicalFieldType type,int id,int = -1);
    void resetName(MxFunctions::PhysicalFieldType type,const QString& before,const QString& after);
    mMeshData* getMeshData() { return mesh_; }
    GeoNeutralData::GeoEngineData* getGeoEngineDataMgr() { return geometry_; }
    CommonData::mUnitSystemManager* getUnitSystemManager() { return units_; }
    int getModelDataID() const { return 1; }
    QString getModelDataName() const { return name_; }
    void setModelDataName(const QString& name) { name_=name; }
    void bindBackends(mMeshData* mesh,GeoNeutralData::GeoEngineData* geometry,CommonData::mUnitSystemManager* units) { mesh_=mesh;geometry_=geometry;units_=units; }
    MotionOcc::LegacyMotionContext* context = nullptr;
private:
    std::tuple<MechMotionData::mMotionBodyManager,
        MechMotionData::mMotionJointManager,
        MechMotionData::mMotionCouplingManager,
        MechMotionData::mMotionDriveManager,
        MechMotionData::mMotionSpringManager,
        MechMotionData::mMotionForceElementManager,
        MechMotionData::mMotionContactManager,
        MechMotionData::mMotionMarkerManager,
        MechMotionData::mMotionSensorManager,
        MechMotionData::mMotionControllerManager,
        MechMotionData::mMotionControlInputManager,
        MechMotionData::mMotionScenarioManager,
        MechMotionData::mMotionAutoFlexManager,
        MechMotionData::mMotionElectroImportManager,
        MechMotionData::mMotionElectroExportManager,
        MechMotionData::mMotionHydraulicManager,
        MechMotionData::mMotionRLManager,
        MechMaterialData::mMaterialManager> managers_;
    MechMapRelation::mMapRelationManager references_;
    mMeshData* mesh_ = nullptr;
    GeoNeutralData::GeoEngineData* geometry_ = nullptr;
    CommonData::mUnitSystemManager* units_ = nullptr;
    QString name_ = QStringLiteral("多体动力学-1");
};
}
