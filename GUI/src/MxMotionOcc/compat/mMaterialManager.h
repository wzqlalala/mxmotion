#pragma once
#include "mMaterialBase.h"
#include <QStringList>
#include <map>
#include <vector>

namespace MechMaterialData {
class mMaterialManager {
public:
    std::shared_ptr<mMaterialBase> getMaterialData(const int& id) const {
        auto found=values_.find(id);return found==values_.end() ? nullptr:found->second;
    }
    std::shared_ptr<mMaterialBase> getMaterialData(const QString& name) const {
        for(const auto& entry:values_)if(entry.second->getMaterialName()==name)return entry.second;
        return {};
    }
    void AddMaterialData(std::shared_ptr<mMaterialBase> value) {
        if(value)values_[value->getMaterialId()]=std::move(value);
    }
    void AddMaterialData(mMaterialBase* value) { AddMaterialData(std::shared_ptr<mMaterialBase>(value)); }
    QString getMatNameByID(int id) const { auto value=getMaterialData(id);return value?value->getMaterialName():QString{}; }
    int getIDByMatName(const QString& name) const { auto value=getMaterialData(name);return value?value->getMaterialId():-1; }
    bool ExistMaterialName(const QString& name) const { return bool(getMaterialData(name)); }
    void resetName(const QString& before,const QString& after) { if(auto value=getMaterialData(before))value->setMaterialName(after); }
    std::vector<int> getAllMatID() const { std::vector<int> result;for(const auto& entry:values_)result.push_back(entry.first);return result; }
    QStringList getAllMatNames() const { QStringList result;for(const auto& entry:values_)result.append(entry.second->getMaterialName());return result; }
    QStringList getAllMatNamesByTypes(MxFunctions::MaterialType type) const {
        QStringList result;for(const auto& entry:values_)if(entry.second->getMaterialType()==type)result.append(entry.second->getMaterialName());return result;
    }
    int createID() const { return values_.empty()?1:values_.rbegin()->first+1; }
    void deleteAllMat() { values_.clear(); }
    void deleteMat(const int& id) { values_.erase(id); }
    int copyMaterialData(const int& id) {
        auto value=getMaterialData(id);if(!value)return -1;
        int next=createID();QString name=value->getMaterialName()+QStringLiteral("-%1").arg(next);
        while(ExistMaterialName(name))name+=QLatin1Char('_');
        AddMaterialData(value->copyMaterial(next,name));return next;
    }
private: std::map<int,std::shared_ptr<mMaterialBase>> values_;
};
}
