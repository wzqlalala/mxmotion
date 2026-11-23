#include "LegacyFlexAttachmentCodec.h"
#include <algorithm>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace MotionOcc {
namespace {
QJsonObject byId(const QJsonArray& rows,int id) {
    for(const auto& value:rows)if(value.toObject().value("id").toInt(-1)==id)return value.toObject();return {};
}
bool hasAttachments(const QJsonObject& row) {
    return !row.value("attachPoints").toArray().isEmpty() || !row.value("attachGroups").toArray().isEmpty()
        || !row.value("attachPointPositions").toArray().isEmpty();
}
bool sameMesh(const QJsonObject& left,const QJsonObject& right) {
    return !left.isEmpty()&&!right.isEmpty()&&left.value("nodes")==right.value("nodes")
        &&left.value("tetrahedra")==right.value("tetrahedra");
}
QVector<int> oldLocalToCurrentGlobal(const QJsonObject& object,const mMeshData& mesh,const QVector<int>& parts) {
    FemMesh old;QString error;if(!readFemMeshJson(object,old,error))return {};
    auto ranges=object.value("partRanges").toArray();
    if(ranges.isEmpty()) {
        if(parts.size()!=1)return {};
        ranges.append(QJsonObject{{"partId",parts.front()},{"firstNode",0},{"nodeCount",int(old.nodes.size())},
            {"firstTet",0},{"tetCount",int(old.tetrahedra.size())}});
    }
    QVector<int> result(int(old.nodes.size()),0);QSet<int> covered;
    for(const auto& entry:ranges) {
        const auto range=entry.toObject();const int part=range.value("partId").toInt(-1);
        if(!parts.contains(part)||covered.contains(part))return {};covered.insert(part);
        const auto found=mesh.meshes().find(part);const auto ids=mesh.nodeIds().find(part);
        if(found==mesh.meshes().end()||ids==mesh.nodeIds().end())return {};
        const auto& source=found->second;
        const int first=range.value("firstNode").toInt(-1),count=range.value("nodeCount").toInt(-1);
        const int firstTet=range.value("firstTet").toInt(-1),tetCount=range.value("tetCount").toInt(-1);
        if(first<0||count!=int(source.nodes.size())||first+std::int64_t(count)>std::int64_t(old.nodes.size())
           ||count!=ids->second.size()||firstTet<0||tetCount!=int(source.tetrahedra.size())
           ||firstTet+std::int64_t(tetCount)>std::int64_t(old.tetrahedra.size()))return {};
        for(int node=0;node<count;++node) {
            if(result[first+node] || old.nodes[std::size_t(first+node)]!=source.nodes[std::size_t(node)])return {};
            result[first+node]=ids->second[node];
        }
        for(int tet=0;tet<tetCount;++tet)for(int corner=0;corner<4;++corner)
            if(old.tetrahedra[std::size_t(firstTet+tet)][corner]!=source.tetrahedra[std::size_t(tet)][corner]+std::uint32_t(first))return {};
    }
    if(covered.size()!=parts.size()||std::any_of(result.begin(),result.end(),[](int id){return id<=0;}))return {};
    return result;
}
}
QJsonObject legacyFlexMeshSnapshot(const mMeshData& mesh,const QVector<int>& parts) {
    if(parts.isEmpty())return {};
    FemMesh merged;QJsonArray ranges,globalIds;QSet<int> seen;
    for(int part:parts) {
        if(seen.contains(part))return {};seen.insert(part);
        const auto found=mesh.meshes().find(part);const auto ids=mesh.nodeIds().find(part);
        if(found==mesh.meshes().end()||found->second.tetrahedra.empty()||ids==mesh.nodeIds().end()
           ||std::size_t(ids->second.size())!=found->second.nodes.size())return {};
        const auto& source=found->second;const auto offset=std::uint32_t(merged.nodes.size());
        ranges.append(QJsonObject{{"partId",part},{"firstNode",int(offset)},{"nodeCount",int(source.nodes.size())},
            {"firstTet",int(merged.tetrahedra.size())},{"tetCount",int(source.tetrahedra.size())},
            {"firstTriangle",int(merged.surface.size())},{"triangleCount",int(source.surface.size())}});
        merged.nodes.insert(merged.nodes.end(),source.nodes.begin(),source.nodes.end());
        for(int id:ids->second)globalIds.append(id);
        for(auto tet:source.tetrahedra) { for(auto& node:tet)node+=offset;merged.tetrahedra.push_back(tet); }
        for(auto triangle:source.surface) { for(auto& node:triangle)node+=offset;merged.surface.push_back(triangle); }
    }
    auto object=femMeshJson(merged);object.insert("partRanges",ranges);object.insert("nodeIds",globalIds);return object;
}
QJsonArray legacyFlexGuiAttachmentIds(const QJsonObject& flex,const mMeshData& mesh,const QVector<int>& parts) {
    const auto original=flex.value("attachPoints").toArray();
    if(flex.value("attachmentMeshChanged").toBool())return original;
    const auto localToGlobal=oldLocalToCurrentGlobal(flex.value("mesh").toObject(),mesh,parts);
    if(localToGlobal.isEmpty())return original;
    std::set<int> selected;
    const auto groups=flex.value("attachGroups").toArray();
    if(!groups.isEmpty()) {
        for(const auto& group:groups)for(const auto& value:group.toArray()) {
            const int local=value.toInt(-1);if(local<0||local>=localToGlobal.size())return original;
            selected.insert(localToGlobal[local]);
        }
    } else {
        const auto ids=flex.value("mesh").toObject().value("nodeIds").toArray();
        if(ids.size()!=localToGlobal.size())return original;
        std::unordered_map<int,int> lookup;
        for(int node=0;node<ids.size();++node)if(!lookup.emplace(ids[node].toInt(-1),node).second)return original;
        for(const auto& value:original) {
            const auto found=lookup.find(value.toInt(-1));if(found==lookup.end())return original;
            selected.insert(localToGlobal[found->second]);
        }
    }
    QJsonArray result;for(int id:selected)result.append(id);return result;
}
void synchronizeLegacyFlexAttachments(const NativeMotionDocument& previous,const NativeMotionDocument& current,
    QJsonObject& data,const QJsonObject& before,const QJsonObject& after,const mMeshData& mesh,
    const QSet<int>& committed,const QSet<int>& confirmed) {
    auto rows=data.value("autoFlexBodies").toArray();
    for(int index=0;index<rows.size();++index) {
        auto row=rows[index].toObject();const int id=row.value("id").toInt(-1),body=row.value("targetBodyId").toInt(-1);
        const auto original=previous.entity("autoFlexBodies",id);
        const auto oldTyped=byId(before.value("autoFlexBodies").toArray(),id),newTyped=byId(after.value("autoFlexBodies").toArray(),id);
        // The original JSON may contain only materialId. Its typed baseline
        // has resolved E/nu/rho for editing, so a recursive delta into a missing
        // inline object would otherwise write just the one edited property.
        // Native preparation gives inline material precedence: publish the
        // complete actual material whenever that resolved object was edited.
        if(oldTyped.value("material")!=newTyped.value("material")&&newTyped.value("material").isObject())
            row.insert("material",newTyped.value("material"));
        const bool targetChanged=!original.isEmpty()&&(original.value("targetBodyId").toInt(-1)!=body
            || previous.bodyPartBindings.value(body)!=current.bodyPartBindings.value(body));
        const bool changed=oldTyped.value("attachPoints")!=newTyped.value("attachPoints");
        const bool explicitSelection=confirmed.contains(id);
        const bool preserveExtended=!explicitSelection&&(!original.value("attachGroups").toArray().isEmpty()
            ||!original.value("attachPointPositions").toArray().isEmpty());
        if(targetChanged) {
            row.remove("mesh");row.remove("meshSourceFile");row.remove("meshFile");
            if(hasAttachments(row))row.insert("attachmentMeshChanged",true);
        }
        const auto snapshot=legacyFlexMeshSnapshot(mesh,current.bodyPartBindings.value(body));
        if((changed||explicitSelection)&&!preserveExtended) {
            const auto selected=newTyped.value("attachPoints").toArray();
            if(!selected.isEmpty()) {
                const auto ids=snapshot.value("nodeIds").toArray();QSet<int> available;
                for(const auto& value:ids)available.insert(value.toInt(-1));
                for(const auto& value:selected)if(!available.contains(value.toInt(-1)))
                    throw std::runtime_error(QStringLiteral("自动柔性体 %1 的附着节点 %2 不属于当前目标体网格，请重新拾取。")
                        .arg(id).arg(value.toInt()).toStdString());
            }
            row.insert("attachPoints",selected);row.remove("attachGroups");row.remove("attachPointPositions");
            row.remove("attachmentMeshChanged");row.remove("meshFile");row.remove("meshSourceFile");
            if(snapshot.isEmpty())row.remove("mesh");else row.insert("mesh",snapshot);
        } else if(preserveExtended&&!targetChanged) {
            // A legacy panel save may auto-populate its flat list. Existing
            // explicit native groups/positions remain authoritative until the
            // user actually edits the original attachment picker.
            if(original.contains("attachPoints"))row.insert("attachPoints",original.value("attachPoints"));else row.remove("attachPoints");
        } else if(row.value("mesh").toObject().isEmpty()&&!snapshot.isEmpty()) {
            row.insert("mesh",snapshot);
            if(!original.value("attachPoints").toArray().isEmpty())row.insert("attachmentMeshChanged",true);
        } else if(committed.contains(id)&&!snapshot.isEmpty()&&!hasAttachments(row)) {
            row.insert("mesh",snapshot);
        }
        // Mesh regeneration through other UI paths invalidates the old global
        // numbering as well. Do not repair old numeric IDs by coincidence.
        const auto embedded=row.value("mesh").toObject();
        if(!explicitSelection&&!snapshot.isEmpty()&&!embedded.isEmpty()&&!sameMesh(embedded,snapshot)&&hasAttachments(row))
            row.insert("attachmentMeshChanged",true);
        rows[index]=row;
    }
    if(!rows.isEmpty()||data.contains("autoFlexBodies"))data.insert("autoFlexBodies",rows);
}
}
