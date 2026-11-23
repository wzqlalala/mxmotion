#include "LegacyMotionJsonAdapter.h"
#include "LegacyMaterialCodec.h"
#include "LegacyMotionExtensionCodec.h"
#include "mModelData.h"
#include "mIMotionJsonFile.h"
#include "mOMotionJsonFile.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionScalarTorque.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include <QJsonArray>
#include <stdexcept>
using namespace MxFunctions;
using namespace MechMotionData;
namespace MotionOcc {
void LegacyMotionJsonAdapter::load(const QJsonObject& data,MechModelData::mModelData& model) {
    loadLegacyMotionExtensions(data,model);
    MechIOFile::mIMotionJsonFile codec({},&model);
    auto& materialManager=model.getManager<MechMaterialData::mMaterialManager>(Physical_Material);
    for(const auto& value:data.value("materials").toArray()) {
        const auto object=value.toObject();const int id=object.value("id").toInt(-1);
        if(id<0)throw std::runtime_error("Invalid material ID in original GUI document");
        materialManager.AddMaterialData(readLegacyMaterial(object,id));
    }
    for(const auto& value:data.value("bodies").toArray()) {
        auto* item=codec.jsonToBody(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI bodies record");
        model.getManager<mMotionBodyManager>(Physical_MotionBody).addData(item);
    }
    for(const auto& value:data.value("joints").toArray()) {
        auto* item=codec.jsonToJoint(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI joints record");
        model.getManager<mMotionJointManager>(Physical_MotionJoint).addData(item);
    }
    for(const auto& value:data.value("couplings").toArray()) {
        auto* item=codec.jsonToCoupling(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI couplings record");
        model.getManager<mMotionCouplingManager>(Physical_MotionCoupling).addData(item);
    }
    for(const auto& value:data.value("springs").toArray()) {
        auto* item=codec.jsonToSpring(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI springs record");
        model.getManager<mMotionSpringManager>(Physical_MotionSpring).addData(item);
    }
    for(const auto& value:data.value("drives").toArray()) {
        auto* item=codec.jsonToDrive(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI drives record");
        model.getManager<mMotionDriveManager>(Physical_MotionDrive).addData(item);
    }
    for(const auto& value:data.value("markers").toArray()) {
        auto* item=codec.jsonToMarker(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI markers record");
        model.getManager<mMotionMarkerManager>(Physical_MotionMarker).addData(item);
    }
    for(const auto& value:data.value("contacts").toArray()) {
        auto* item=codec.jsonToContact(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI contacts record");
        model.getManager<mMotionContactManager>(Physical_MotionContact).addData(item);
    }
    for(const auto& value:data.value("sensors").toArray()) {
        auto* item=codec.jsonToSensor(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI sensors record");
        model.getManager<mMotionSensorManager>(Physical_MotionSensor).addData(item);
    }
    for(const auto& value:data.value("controllers").toArray()) {
        auto* item=codec.jsonToController(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI controllers record");
        model.getManager<mMotionControllerManager>(Physical_MotionController).addData(item);
    }
    for(const auto& value:data.value("controlInputs").toArray()) {
        auto* item=codec.jsonToControlInput(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI controlInputs record");
        model.getManager<mMotionControlInputManager>(Physical_MotionControlInput).addData(item);
    }
    for(const auto& value:data.value("autoFlexBodies").toArray()) {
        auto object=value.toObject();const auto embedded=object.value("material").toObject();
        if(!embedded.isEmpty()) {
            int id=object.value("materialId").toInt(-1);
            if(id<0)id=materialManager.createID();
            // Native inline material wins. Preserve conflicting inline records
            // independently rather than altering another body's material.
            if(auto existing=materialManager.getMaterialData(id)) {
                const auto previous=writeLegacyMaterial(*existing);
                for(const auto* key:{"youngModulus","poissonRatio","density"})
                    if(embedded.contains(key)&&previous.value(key)!=embedded.value(key)) { id=materialManager.createID();break; }
            }
            materialManager.AddMaterialData(readLegacyMaterial(embedded,id));object.insert("materialId",id);
        }
        auto* item=codec.jsonToAutoFlex(object);
        if(!item) throw std::runtime_error("Invalid original GUI autoFlexBodies record");
        model.getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex).addData(item);
    }
    for(const auto& value:data.value("scenarios").toArray()) {
        auto* item=codec.jsonToScenario(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI scenarios record");
        model.getManager<mMotionScenarioManager>(Physical_MotionScenario).addData(item);
    }
    for(const auto& value:data.value("vectorForces").toArray()) {
        auto* item=codec.jsonToVectorForce(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI vectorForces record");
        model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(item);
    }
    for(const auto& value:data.value("scalarForces").toArray()) {
        auto* item=codec.jsonToScalarForce(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI scalarForces record");
        model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(item);
    }
    for(const auto& value:data.value("scalarTorques").toArray()) {
        auto* item=codec.jsonToScalarTorque(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI scalarTorques record");
        model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(item);
    }
    for(const auto& value:data.value("threePointForces").toArray()) {
        auto* item=codec.jsonToThreePointForce(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI threePointForces record");
        model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(item);
    }
    for(const auto& value:data.value("threePointTorques").toArray()) {
        auto* item=codec.jsonToThreePointTorque(value.toObject());
        if(!item) throw std::runtime_error("Invalid original GUI threePointTorques record");
        model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(item);
    }
}
QJsonObject LegacyMotionJsonAdapter::save(MechModelData::mModelData& model) {
    MechIOFile::mOMotionJsonFile codec({},&model);
    QJsonObject data;
    {
        QJsonArray rows;auto& manager=model.getManager<MechMaterialData::mMaterialManager>(Physical_Material);
        for(int id:manager.getAllMatID())rows.append(writeLegacyMaterial(*manager.getMaterialData(id)));
        data.insert("materials",rows);
    }
    saveLegacyMotionExtensions(data,model);
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionBodyManager>(Physical_MotionBody);
        for(int id:manager.getAllIDs()) rows.append(codec.bodyToJson(manager.getData(id)));
        data.insert("bodies",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionJointManager>(Physical_MotionJoint);
        for(int id:manager.getAllIDs()) rows.append(codec.jointToJson(manager.getData(id)));
        data.insert("joints",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionCouplingManager>(Physical_MotionCoupling);
        for(int id:manager.getAllIDs()) rows.append(codec.couplingToJson(manager.getData(id)));
        data.insert("couplings",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionSpringManager>(Physical_MotionSpring);
        for(int id:manager.getAllIDs()) rows.append(codec.springToJson(manager.getData(id)));
        data.insert("springs",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionDriveManager>(Physical_MotionDrive);
        for(int id:manager.getAllIDs()) rows.append(codec.driveToJson(manager.getData(id)));
        data.insert("drives",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionMarkerManager>(Physical_MotionMarker);
        for(int id:manager.getAllIDs()) rows.append(codec.markerToJson(manager.getData(id)));
        data.insert("markers",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionContactManager>(Physical_MotionContact);
        for(int id:manager.getAllIDs()) rows.append(codec.contactToJson(manager.getData(id)));
        data.insert("contacts",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionSensorManager>(Physical_MotionSensor);
        for(int id:manager.getAllIDs()) rows.append(codec.sensorToJson(manager.getData(id)));
        data.insert("sensors",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionControllerManager>(Physical_MotionController);
        for(int id:manager.getAllIDs()) rows.append(codec.controllerToJson(manager.getData(id)));
        data.insert("controllers",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionControlInputManager>(Physical_MotionControlInput);
        for(int id:manager.getAllIDs()) rows.append(codec.controlInputToJson(manager.getData(id)));
        data.insert("controlInputs",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex);
        for(int id:manager.getAllIDs()) {
            auto* item=manager.getData(id);auto row=codec.autoFlexToJson(item);
            auto material=model.getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(item->getMaterialId());
            if(material)row.insert("material",writeLegacyMaterial(*material));
            rows.append(row);
        }
        data.insert("autoFlexBodies",rows);
    }
    {
        QJsonArray rows;
        auto& manager=model.getManager<mMotionScenarioManager>(Physical_MotionScenario);
        for(int id:manager.getAllIDs()) rows.append(codec.scenarioToJson(manager.getData(id)));
        data.insert("scenarios",rows);
    }
    QJsonArray vectors,scalars,torques,threeForces,threeTorques;
    auto& forces=model.getManager<mMotionForceElementManager>(Physical_MotionVectorForce);
    for(int id:forces.getAllIDs()) {
        auto* value=forces.getData(id);
        if(auto* item=dynamic_cast<mMotionVectorForce*>(value)) vectors.append(codec.vectorForceToJson(item));
        else if(auto* item=dynamic_cast<mMotionScalarForce*>(value)) scalars.append(codec.scalarForceToJson(item));
        else if(auto* item=dynamic_cast<mMotionScalarTorque*>(value)) torques.append(codec.scalarTorqueToJson(item));
        else if(auto* item=dynamic_cast<mMotionThreePointForce*>(value)) threeForces.append(codec.threePointForceToJson(item));
        else if(auto* item=dynamic_cast<mMotionThreePointTorque*>(value)) threeTorques.append(codec.threePointTorqueToJson(item));
    }
    data.insert("vectorForces",vectors); data.insert("scalarForces",scalars); data.insert("scalarTorques",torques);
    data.insert("threePointForces",threeForces); data.insert("threePointTorques",threeTorques);
    return data;
}
}
