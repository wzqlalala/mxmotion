#include "mMeshData.h"
#include <QCryptographicHash>
#include <algorithm>
#include <type_traits>
#include <limits>
#include <stdexcept>
void mMeshData::rebuildNodeIndex() {
    xyz_.clear();_nodedata.nodeidmap.clear();
    size_t total=0;for(const auto& item:meshes_)total+=item.second.nodes.size();
    if(total>size_t(std::numeric_limits<int>::max()-1))throw std::length_error("Mesh has too many global nodes");
    xyz_.reserve(total*3);
    for(auto it=nodeIds_.begin();it!=nodeIds_.end();) {
        if(!meshes_.count(it->first)) { meshFingerprints_.erase(it->first);it=nodeIds_.erase(it); }
        else ++it;
    }
    for(const auto& [part,mesh]:meshes_) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        const auto append=[&](const auto& values) {
            const auto count=std::uint64_t(values.size());hash.addData(reinterpret_cast<const char*>(&count),sizeof(count));
            const auto* bytes=reinterpret_cast<const char*>(values.data());
            std::size_t remaining=values.size()*sizeof(typename std::decay_t<decltype(values)>::value_type);
            while(remaining) { const int length=int(std::min<std::size_t>(remaining,1024*1024));hash.addData(bytes,length);bytes+=length;remaining-=length; }
        };
        append(mesh.nodes);append(mesh.tetrahedra);append(mesh.surface);const auto fingerprint=hash.result();
        auto& ids=nodeIds_[part];
        if(meshFingerprints_[part]!=fingerprint || std::size_t(ids.size())!=mesh.nodes.size()) {
            if(nextNodeId_+mesh.nodes.size()>std::uint64_t(std::numeric_limits<int>::max()))throw std::length_error("Global mesh node ID range exhausted");
            ids.clear();ids.reserve(int(mesh.nodes.size()));
            for(std::size_t node=0;node<mesh.nodes.size();++node)ids.append(int(nextNodeId_++));
            meshFingerprints_[part]=fingerprint;
        }
        for(std::size_t node=0;node<mesh.nodes.size();++node) {
            const auto& p=mesh.nodes[node];_nodedata.nodeidmap.emplace(ids[int(node)],int(xyz_.size()/3));
            xyz_.insert(xyz_.end(),p.begin(),p.end());
        }
    }
    _nodedata.xyz=xyz_.empty() ? nullptr : xyz_.data();
}
