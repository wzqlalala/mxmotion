#pragma once
#include "CadDocument.h"
#include "FemMesh.h"
#include <QStringList>
#include <QByteArray>
#include <map>
#include <vector>
#include <unordered_map>

// Original GUI mesh/part queries backed by the real OCC document and FEM mesh.
// CAD parts exist before meshing, as they did in the original geometry facade.
class mMeshData {
public:
    struct NodeData {
        double* xyz = nullptr;
        std::unordered_map<int,int> nodeidmap;
    } _nodedata;
    mMeshData(const MotionOcc::CadDocument& cad,const std::map<int,MotionOcc::FemMesh>& meshes)
        :cad_(cad),meshes_(meshes) { rebuildNodeIndex(); }
    QStringList getAllPartNames() const {
        QStringList names; for(const auto& part:cad_.parts()) names.append(part.name); return names;
    }
    std::vector<int> getAllPartIds() const {
        std::vector<int> ids; for(const auto& part:cad_.parts()) ids.push_back(part.id); return ids;
    }
    QString getPartNameByPartId(int id) const { const auto* part=cad_.part(id); return part ? part->name : QString{}; }
    int getPartIdByPartName(const QString& name) const {
        for(const auto& part:cad_.parts()) if(part.name==name) return part.id;
        return -1;
    }
    void rebuildNodeIndex();
    bool havePart(const QString& name) const { return getPartIdByPartName(name)>0; }
    const std::map<int,QVector<int>>& nodeIds() const { return nodeIds_; }
    const MotionOcc::CadDocument& cad() const { return cad_; }
    const std::map<int,MotionOcc::FemMesh>& meshes() const { return meshes_; }
private:
    const MotionOcc::CadDocument& cad_;
    const std::map<int,MotionOcc::FemMesh>& meshes_;
    std::vector<double> xyz_;
    std::map<int,QVector<int>> nodeIds_;
    std::map<int,QByteArray> meshFingerprints_;
    std::uint64_t nextNodeId_=1;
};
