#include "mModelData.h"
#include "LegacyManagerList.h"
#include "mMeshData.h"

using namespace MxFunctions;
namespace MechModelData {
mModelData::mModelData() = default;
mModelData::~mModelData() = default;

QStringList mModelData::getAllNames(PhysicalFieldType type,int) {
    if(type==Physical_Material) return std::get<MechMaterialData::mMaterialManager>(managers_).getAllMatNames();
    if(type==Physical_Part) return mesh_ ? mesh_->getAllPartNames() : QStringList{};
    switch(type) {
#define MX_NAMES(tag,manager) case Physical_Motion##tag: return std::get<MechMotionData::mMotion##manager##Manager>(managers_).getAllNames();
        MX_LEGACY_MOTION_MANAGERS(MX_NAMES)
#undef MX_NAMES
    default: return {};
    }
}
std::vector<int> mModelData::getAllPhyID(PhysicalFieldType type,int) {
    if(type==Physical_Material) return std::get<MechMaterialData::mMaterialManager>(managers_).getAllMatID();
    if(type==Physical_Part) return mesh_ ? mesh_->getAllPartIds() : std::vector<int>{};
    switch(type) {
#define MX_IDS(tag,manager) case Physical_Motion##tag: return std::get<MechMotionData::mMotion##manager##Manager>(managers_).getAllIDs();
        MX_LEGACY_MOTION_MANAGERS(MX_IDS)
#undef MX_IDS
    default: return {};
    }
}
QString mModelData::getNameByPhyID(PhysicalFieldType type,int id) {
    if(type==Physical_Material) return std::get<MechMaterialData::mMaterialManager>(managers_).getMatNameByID(id);
    if(type==Physical_Part) return mesh_ ? mesh_->getPartNameByPartId(id) : QString{};
    switch(type) {
#define MX_NAME(tag,manager) case Physical_Motion##tag: return std::get<MechMotionData::mMotion##manager##Manager>(managers_).getNameByID(id);
        MX_LEGACY_MOTION_MANAGERS(MX_NAME)
#undef MX_NAME
    default: return {};
    }
}
int mModelData::getPhyIDByName(PhysicalFieldType type,QString name,bool) {
    for(int id:getAllPhyID(type)) if(getNameByPhyID(type,id)==name) return id;
    return -1;
}
bool mModelData::isExistName(PhysicalFieldType type,const QString& name,bool) {
    return getAllNames(type).contains(name);
}
QString mModelData::createNameByType(PhysicalFieldType type,const QString& prefix) {
    const auto names=getAllNames(type); int index=1;
    while(names.contains(prefix+QStringLiteral("-%1").arg(index))) ++index;
    return prefix+QStringLiteral("-%1").arg(index);
}
void mModelData::deleteAllByPhysicalType(PhysicalFieldType type,int) {
    for(int id:getAllPhyID(type)) references_.deleteMapRelation({type,id});
    switch(type) {
    case Physical_Material: std::get<MechMaterialData::mMaterialManager>(managers_).deleteAllMat();break;
#define MX_DELETEALL(tag,manager) case Physical_Motion##tag: std::get<MechMotionData::mMotion##manager##Manager>(managers_).deleteAll(); break;
        MX_LEGACY_MOTION_MANAGERS(MX_DELETEALL)
#undef MX_DELETEALL
    default: break;
    }
}
void mModelData::deleteByPhysicalType(PhysicalFieldType type,int id,int) {
    references_.deleteMapRelation({type,id});
    switch(type) {
    case Physical_Material: std::get<MechMaterialData::mMaterialManager>(managers_).deleteMat(id);break;
#define MX_DELETE(tag,manager) case Physical_Motion##tag: std::get<MechMotionData::mMotion##manager##Manager>(managers_).deleteData(id); break;
        MX_LEGACY_MOTION_MANAGERS(MX_DELETE)
#undef MX_DELETE
    default: break;
    }
}
void mModelData::resetName(PhysicalFieldType type,const QString& before,const QString& after) {
    switch(type) {
    case Physical_Material: std::get<MechMaterialData::mMaterialManager>(managers_).resetName(before,after);break;
#define MX_RENAME(tag,manager) case Physical_Motion##tag: std::get<MechMotionData::mMotion##manager##Manager>(managers_).resetName(before,after); break;
        MX_LEGACY_MOTION_MANAGERS(MX_RENAME)
#undef MX_RENAME
    default: break;
    }
}
int mModelData::copyPhysicalField(PhysicalFieldType type,int id,int) {
    int result=-1;
    switch(type) {
    case Physical_Material: result=std::get<MechMaterialData::mMaterialManager>(managers_).copyMaterialData(id);break;
#define MX_COPY(tag,manager) case Physical_Motion##tag: result=std::get<MechMotionData::mMotion##manager##Manager>(managers_).copyData(id); break;
        MX_LEGACY_MOTION_MANAGERS(MX_COPY)
#undef MX_COPY
    default: break;
    }
    if(result>=0) references_.copyMapRelation(type,id,result);
    return result;
}
}
