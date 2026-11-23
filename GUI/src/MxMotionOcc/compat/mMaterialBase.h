#pragma once

#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <memory>

namespace MxFunctions {
// Source-level material tags used by the original AutoFlex panel. Persistent
// material values use named JSON fields, never an unavailable SDK enum layout.
enum MaterialType { NONEMATERIALTYPE, LinearElastic_Isotropic };
enum MaterialClass { MAT_Isotropy, MAT_Density };
enum MaterialPropType { MatProp_YoungsModulus, MatProp_Botson, MatProp_Density };
}

namespace MechMaterialData {
class mMaterialBase {
public:
    using Properties=QHash<MxFunctions::MaterialClass,QHash<MxFunctions::MaterialPropType,QVariant>>;
    mMaterialBase(int id,MxFunctions::MaterialType type,const QString& name={})
        :mMaterialBase(id,QColor::fromHsv((qMax(id,1)-1)*47%360,110,210),type,name) {}
    mMaterialBase(int id,QColor color,MxFunctions::MaterialType type=MxFunctions::NONEMATERIALTYPE,const QString& name={})
        :id_(id),name_(name.isEmpty() ? QStringLiteral("Material_%1").arg(id):name),type_(type),color_(color) {}
    int getMaterialId() const { return id_; }
    void setMaterialId(int value) { id_=value; }
    QString getMaterialName() const { return name_; }
    void setMaterialName(const QString& value) { name_=value; }
    MxFunctions::MaterialType getMaterialType() const { return type_; }
    void setMaterialType(MxFunctions::MaterialType value) { type_=value; }
    QColor getMatColor() const { return color_; }
    void setMatColor(const QColor& value) { color_=value; }
    void addProperty(const MxFunctions::MaterialClass& category,const MxFunctions::MaterialPropType& property,const QVariant& value) {
        props_[category][property]=value;
    }
    Properties getAllProps() const { return props_; }
    void setProperty(const Properties& values) { props_=values; }
    std::shared_ptr<mMaterialBase> copyMaterial(const int& id,const QString& name) const {
        auto copy=std::make_shared<mMaterialBase>(*this);copy->id_=id;copy->name_=name;return copy;
    }
    // Preserve extension fields from imported material documents when the
    // original panel edits only its three isotropic values.
    QJsonObject sourceJson;
private:
    int id_;
    QString name_;
    MxFunctions::MaterialType type_;
    QColor color_;
    Properties props_;
};
}
