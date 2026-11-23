#pragma once
#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"
#include <map>
#include <set>
#include <tuple>
#include <type_traits>

namespace MechMapRelation {
class mMapRelationManager {
    using Type = MxFunctions::PhysicalFieldType;
    using Role = MxFunctions::MapRelationType;
    using Key = std::tuple<Type,int,Type,Role>;
    std::map<Key,std::set<int>> references_;
public:
    void appendMapRelation(std::pair<Type,int> from, std::pair<Type,int> to,
                           Role role = MxFunctions::MapRelation_None, double = 1.0) {
        if (from.second < 0 || to.second < 0) return;
        references_[{from.first,from.second,to.first,role}].insert(to.second);
    }
    void clearMotionRole(Type from, int id, Type to, Role role) {
        references_.erase({from,id,to,role});
    }
    template<class T> T getMapRelation(std::pair<Type,int> from, Type to,
                                      Role role = MxFunctions::MapRelation_None) const {
        const auto it=references_.find({from.first,from.second,to,role});
        if constexpr (std::is_same_v<T,int>)
            return it==references_.end() || it->second.empty() ? -1 : *it->second.begin();
        else {
            static_assert(std::is_same_v<T,std::set<int>>, "Unsupported relationship result");
            return it==references_.end() ? std::set<int>{} : it->second;
        }
    }
    void deleteMapRelation(std::pair<Type,int> object) {
        for(auto it=references_.begin();it!=references_.end();) {
            if(std::get<0>(it->first)==object.first && std::get<1>(it->first)==object.second)
                it=references_.erase(it);
            else { if(std::get<2>(it->first)==object.first) it->second.erase(object.second); ++it; }
        }
    }
    void deleteMapRelation(std::pair<Type,int> from,std::pair<Type,int> to) {
        for(auto& [key,ids]:references_)
            if(std::get<0>(key)==from.first && std::get<1>(key)==from.second && std::get<2>(key)==to.first)
                ids.erase(to.second);
    }
    void copyMapRelation(Type type,int oldId,int newId,int = -1) {
        const auto old=references_;
        for(const auto& [key,ids]:old) if(std::get<0>(key)==type && std::get<1>(key)==oldId)
            references_[{type,newId,std::get<2>(key),std::get<3>(key)}]=ids;
    }
    void clear() { references_.clear(); }
};
}
