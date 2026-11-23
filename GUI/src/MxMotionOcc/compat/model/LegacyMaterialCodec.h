#pragma once
#include "mMaterialManager.h"
#include <QJsonArray>
#include <algorithm>
#include <stdexcept>

namespace MotionOcc {
inline std::shared_ptr<MechMaterialData::mMaterialBase> readLegacyMaterial(const QJsonObject& object,int id) {
    using namespace MxFunctions;
    auto material=std::make_shared<MechMaterialData::mMaterialBase>(id,LinearElastic_Isotropic,object.value("name").toString());
    material->sourceJson=object;
    const auto color=QColor(object.value("color").toString());
    if(color.isValid())material->setMatColor(color);
    for(const auto& value:object.value("legacyProperties").toArray()) {
        const auto property=value.toObject();
        material->addProperty(static_cast<MaterialClass>(property.value("category").toInt()),
            static_cast<MaterialPropType>(property.value("property").toInt()),property.value("value").toVariant());
    }
    const std::pair<const char*,MaterialPropType> fields[]={{"youngModulus",MatProp_YoungsModulus},{"poissonRatio",MatProp_Botson},{"density",MatProp_Density}};
    for(const auto& field:fields)if(object.contains(field.first))
        material->addProperty(MAT_Isotropy,field.second,object.value(field.first).toVariant());
    return material;
}
inline QJsonObject writeLegacyMaterial(const MechMaterialData::mMaterialBase& material) {
    using namespace MxFunctions;
    QJsonObject result=material.sourceJson;
    result.insert("id",material.getMaterialId());result.insert("name",material.getMaterialName());
    result.insert("color",material.getMatColor().name(QColor::HexArgb));
    QJsonArray properties;
    const auto all=material.getAllProps();
    QList<MaterialClass> categories=all.keys();std::sort(categories.begin(),categories.end());
    for(auto category:categories) {
        const auto values=all.value(category);auto keys=values.keys();std::sort(keys.begin(),keys.end());
        for(auto property:keys)properties.append(QJsonObject{{"category",int(category)},{"property",int(property)},
            {"value",QJsonValue::fromVariant(values.value(property))}});
    }
    result.insert("legacyProperties",properties);
    const std::pair<const char*,MaterialPropType> fields[]={{"youngModulus",MatProp_YoungsModulus},{"poissonRatio",MatProp_Botson},{"density",MatProp_Density}};
    for(const auto& field:fields) {
        // AutoFlex writes the isotropic group; it has precedence over a legacy
        // duplicate category when the user changes an isotropic value.
        QVariant value=all.value(MAT_Isotropy).value(field.second);
        if(!value.isValid())for(auto category:categories)if(all.value(category).contains(field.second)) { value=all.value(category).value(field.second);break; }
        if(value.isValid())result.insert(field.first,QJsonValue::fromVariant(value));
    }
    return result;
}
}
