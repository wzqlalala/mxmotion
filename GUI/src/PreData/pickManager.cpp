// PickManager.cpp
#include"pickManager.h"

#include <QtConcurrent/qtconcurrentiteratekernel.h>
#include <QtConcurrent/qtconcurrentreducekernel.h>
#include<vector>
#include <vector>
#include <memory>
#include <QObject>
#include <set>
#include <QHash>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include"mModelData.h"
#include"mGeoManager.h"
#include"mMeshData.h"
#include"mViewportModelManager.h"
#include"mViewportModel.h"
#include"mxr/view/CCSMPreRender.h"
#include"mxr/view/CCSMRendWidget.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】死 include（零使用；该头经 mOCCDocument.h 拉 OCC 链）
#include "PickCommonFunction.h"
#include"mMeshData.h"
//#include"mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1】死 include（零使用；S9 已退役的 OCC 重度件）
//获取两条边
bool PickManager::pickTwoEdgesForAxis(MechCommon::DefaultLine& outLine)
{
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return false;
    auto* geoSelectData = preRender->getSelectionContainer()->getGeoSelectData();
    auto geoManager = getCVM->getModelData()->getGeoManager();

    vector<int> edgeIds = geoSelectData->getSelectDataByType(SelectionInfoType::GeoEdge);
    if (edgeIds.size() < 2) return false;

    // 取最新两条边
    vector<int> lastTwo = { edgeIds[edgeIds.size() - 2], edgeIds[edgeIds.size() - 1] };
    auto edgesAttr = getGEOServer.geoCalculate().CalculateEdgeAttr(lastTwo);
    if (edgesAttr.size() != 2) return false;

    QVector3D mid1 = edgesAttr[0].m_coor;
    QVector3D mid2 = edgesAttr[1].m_coor;
    QVector3D dir1 = edgesAttr[0].m_dir;
    QVector3D dir2 = edgesAttr[1].m_dir;

    QVector3D axisDir = (dir1.normalized() + dir2.normalized());
    if (axisDir.length() < 1e-6) axisDir = dir1;
    axisDir.normalize();

    QVector3D axisCenter = (mid1 + mid2) * 0.5f;
    float halfLen = (mid2 - mid1).length() * 0.5f;
    if (halfLen < 0.1f) halfLen = 3.0f;

    outLine.start = axisCenter - axisDir * halfLen;
    outLine.end = axisCenter + axisDir * halfLen;
    return true;
}





//计算平均点
bool PickManager::pickPointMultiAverage(QVector3D& result)
{
    auto points = getSelectedPoints();
    if (points.size() < 2) return false;
    QVector3D sum;   
    for (const auto& p : points) sum += p;
    result = sum / points.size();
    return true;
}



//方向切换（待定）
bool PickManager::pickDirectionAuto(QVector3D& direction)
{
    auto* selContainer = getCVM->getPreRenderWidget()->getSelectionContainer();
    if (!selContainer) return false;
    auto* geoData = selContainer->getGeoSelectData();
    auto* geoManager = getCVM->getModelData()->getGeoManager();

    // 优先两个点
    auto points = getSelectedPoints();
    if (points.size() == 2) {
        direction = points[1] - points[0];
        return true;
    }
    // 一条边 -> 边的方向（切线）
    std::vector<int> edgeIds = geoData->getSelectDataByType(SelectionInfoType::GeoEdge);
    if (edgeIds.size() == 1) {
        auto edgesAttr = getGEOServer.geoCalculate().CalculateEdgeAttr(edgeIds);
        if (!edgesAttr.empty()) {
            // 可以通过两点计算
            direction = edgesAttr[0].m_dir;
            return true;
        }
    }
    // 一个面 -> 法向
    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (faceIds.size() == 1) {
        auto facesAttr = getGEOServer.geoCalculate().CalculateFaceAttr(faceIds);
        if (!facesAttr.empty()) {
            direction = facesAttr[0].m_dir;
            return true;
        }
    }
    return false;
}

//单点选择
MPREDATA_EXPORT bool PickManager::pickSinglePoint(QVector3D& result)
{
    auto points = getSelectedPoints();
    if (points.empty()) return false;
    //result = points[0]; // 只取第一个选中的点
    result = points.back();
    return true;
}

MPREDATA_EXPORT int PickManager::getSelectedEdgeCount()
{
    if (!getCVM) return 0;
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return 0;
    auto* container = preRender->getSelectionContainer();
    if (!container) return 0;
    auto* geoData = container->getGeoSelectData();
    if (!geoData) return 0;
    return (int)geoData->getSelectDataByType(SelectionInfoType::GeoEdge).size();
}
//获取所有拾取数据
std::vector<QVector3D> PickManager::getSelectedPoints()
{
    
    
    //return points;
    std::vector<QVector3D> points;
    auto* selContainer = getCVM->getPreRenderWidget()->getSelectionContainer();
    if (!selContainer) return points;
    auto* geoData = selContainer->getGeoSelectData();
    auto* meshData = selContainer->getMeshSelectData();
    auto* geoManager = getCVM->getModelData()->getGeoManager();
    auto* meshDataStruct = getCVM->getModelData()->getMeshData();

    // 1. 几何点
    std::vector<int> pointIds = geoData->getSelectDataByType(SelectionInfoType::GeoPoint);
    if (!pointIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateVertexAttr(pointIds);
        for (const auto& attr : attrs)
            points.push_back(attr.m_coor);
    }

    
    std::vector<int> edgeIds = geoData->getSelectDataByType(SelectionInfoType::GeoEdge);
    for (int edgeId : edgeIds) {
        auto attrs = getGEOServer.geoCalculate().CalculateEdgeAttr({ edgeId });
        if (!attrs.empty()) {
            // 打印调试信息，确认 attrs[0].m_coor 是否为中点的坐标
            qDebug() << "Edge" << edgeId << "mid point:" << attrs[0].m_coor;
            points.push_back(attrs[0].m_coor);
        }
        else {
            // 如果获取失败，可以尝试其他方式（例如通过端点计算）
            qDebug() << "Failed to get edge attr for" << edgeId;
        }
    }
    // 3. 几何面 → 取面心
    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (!faceIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr(faceIds);
        for (const auto& attr : attrs)
            points.push_back(attr.m_coor);
    }

    // 4. 网格节点
    const auto& nodeInfos = meshData->getSelectDataByType(SelectionInfoType::MeshNode);
    for (const auto& info : nodeInfos) {
        int nodeId = info.data_id.node_id;
        double* vertex = meshDataStruct->_nodedata.xyz + meshDataStruct->_nodedata.nodeidmap[nodeId] * 3;
        points.push_back(QVector3D(vertex[0], vertex[1], vertex[2]));
    }
    // 5. 网格线 → 中点（直接下标，不检查存在性）
    auto meshLines = meshData->getSelectDataByType(SelectionInfoType::MeshLine);
    for (const auto& info : meshLines) {
        int node1 = info.data_id.node_id;
        int node2 = info.data_id.node_id;
        int idx1 = meshDataStruct->_nodedata.nodeidmap[node1];
        int idx2 = meshDataStruct->_nodedata.nodeidmap[node2];
        double* v1 = meshDataStruct->_nodedata.xyz + idx1 * 3;
        double* v2 = meshDataStruct->_nodedata.xyz + idx2 * 3;
        QVector3D p1(v1[0], v1[1], v1[2]);
        QVector3D p2(v2[0], v2[1], v2[2]);
        points.push_back((p1 + p2) * 0.5f);
    }

    // 6. 网格面 → 面心
    auto meshFaces = meshData->getSelectDataByType(SelectionInfoType::MeshFace);
    for (const auto& info : meshFaces) {
        RenderPickResultData faceData;
        faceData.part_id = info.part_id;
        faceData.result_id = info.data_id.element_id;
        faceData.result_id2 = info.data_index.face_index;

        QVector3D outVec, centerPos;
        meshDataStruct->GetOutVectorByMeshFaceID(faceData, outVec, centerPos);
        points.push_back(centerPos);
    }

    // 7. 几何体 → 取体心（包围盒中心或质心）
    std::vector<int> partIds = geoData->getSelectDataByType(SelectionInfoType::GeoSolid);
    if (!partIds.empty()) {
        for (int partId : partIds) {
            QVector3D centroid = getGEOServer.geoCalculate().getCentroid(partId);
            if (centroid.isNull()) {
                // 降级：使用包围盒中心
                auto bbox = getGEOServer.geoCalculate().getBoundingBox(partId);
                centroid = (bbox.first + bbox.second) * 0.5f;
            }
            points.push_back(centroid);
        }
    }

    return points;
}
//新增
MPREDATA_EXPORT bool PickManager::getDirectionFromLastPick(QVector3D& direction)
{
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return false;
    auto* container = preRender->getSelectionContainer();
    if (!container) return false;
    auto* geoData = container->getGeoSelectData();
    auto* meshData = container->getMeshSelectData();
    auto* modelData = getCVM->getModelData();
    if (!modelData) return false;
    auto* geoManager = modelData->getGeoManager();
    auto* meshDataStruct = modelData->getMeshData();

    // 1. 几何面（取最后选中的面）
    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (!faceIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr({ faceIds.back() });
        if (!attrs.empty()) {
            direction = attrs[0].m_dir.normalized();
            return true;
        }
    }

    // 2. 几何边（取最后选中的边）
    std::vector<int> edgeIds = geoData->getSelectDataByType(SelectionInfoType::GeoEdge);
    if (!edgeIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateEdgeAttr({ edgeIds.back() });
        if (!attrs.empty()) {
            direction = attrs[0].m_dir.normalized();
            return true;
        }
    }

    // 3. 网格面（取最后选中的网格面）
    auto meshFaces = meshData->getSelectDataByType(SelectionInfoType::MeshFace);
    if (!meshFaces.empty()) {
        const auto& info = meshFaces.back();
        RenderPickResultData faceData;
        faceData.part_id = info.part_id;
        faceData.result_id = info.data_id.element_id;
        faceData.result_id2 = info.data_index.face_index;
        QVector3D normal, center;
        meshDataStruct->GetOutVectorByMeshFaceID(faceData, normal, center);
        direction = normal.normalized();
        return true;
    }

    // 4. 网格边（取最后选中的网格边，通过两端点计算方向）
    auto meshEdges = meshData->getSelectDataByType(SelectionInfoType::MeshLine);
    if (!meshEdges.empty()) {
        const auto& info = meshEdges.back();
        // 根据实际 SDK 获取节点ID（假设 data_id 包含 node_id1, node_id2）
        int node1 = info.data_id.node_id;
        int node2 = info.data_id.node_id;
        auto getCoord = [&](int nid) -> QVector3D {
            auto it = meshDataStruct->_nodedata.nodeidmap.find(nid);
            if (it == meshDataStruct->_nodedata.nodeidmap.end()) return QVector3D();
            int idx = meshDataStruct->_nodedata.nodeidmap[node1];
            double* v = meshDataStruct->_nodedata.xyz + idx * 3;
            return QVector3D(v[0], v[1], v[2]);
            };
        QVector3D p1 = getCoord(node1);
        QVector3D p2 = getCoord(node2);
        if (!p1.isNull() && !p2.isNull()) {
            direction = (p2 - p1).normalized();
            return true;
        }
    }

    // 5. 两个点（几何点或网格节点）-> 方向矢量（从第一个点到最后一个点）
    std::vector<QVector3D> points = getSelectedPoints();
    if (points.size() >= 2) {
        direction = (points.back() - points[points.size() - 2]).normalized();
        return true;
    }

    return false;
}

MPREDATA_EXPORT QVector3D PickManager::getDirectionFromLastPick()
{
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return QVector3D();
    auto* container = preRender->getSelectionContainer();
    if (!container) return QVector3D();
    auto* geoData = container->getGeoSelectData();
    auto* meshData = container->getMeshSelectData();
    auto* modelData = getCVM->getModelData();
    if (!modelData) return QVector3D();
    auto* geoManager = modelData->getGeoManager();
    auto* meshDataStruct = modelData->getMeshData();

    // 1. 几何面 -> 法向
    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (!faceIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr({ faceIds.back() });
        if (!attrs.empty()) return attrs[0].m_dir.normalized();
    }

    // 2. 几何边 -> 方向
    std::vector<int> edgeIds = geoData->getSelectDataByType(SelectionInfoType::GeoEdge);
    if (!edgeIds.empty()) {
        auto attrs = getGEOServer.geoCalculate().CalculateEdgeAttr({ edgeIds.back() });
        if (!attrs.empty()) return attrs[0].m_dir.normalized();
    }

    // 3. 网格面 -> 法向
    auto meshFaces = meshData->getSelectDataByType(SelectionInfoType::MeshFace);
    if (!meshFaces.empty()) {
        const auto& info = meshFaces.back();
        RenderPickResultData faceData;
        faceData.part_id = info.part_id;
        faceData.result_id = info.data_id.element_id;
        faceData.result_id2 = info.data_index.face_index;
        QVector3D normal, center;
        meshDataStruct->GetOutVectorByMeshFaceID(faceData, normal, center);
        return normal.normalized();
    }

    // 4. 网格边 -> 方向（通过两个节点坐标差）
    auto meshEdges = meshData->getSelectDataByType(SelectionInfoType::MeshLine);
    if (!meshEdges.empty()) {
        const auto& info = meshEdges.back();
        // 根据实际 SDK 获取节点ID
        int node1 = info.data_id.node_id;
        int node2 = info.data_id.node_id;
        auto getCoord = [&](int nid) -> QVector3D {
            auto it = meshDataStruct->_nodedata.nodeidmap.find(nid);
            if (it == meshDataStruct->_nodedata.nodeidmap.end()) return QVector3D();
            int idx = meshDataStruct->_nodedata.nodeidmap[node1];
            double* v = meshDataStruct->_nodedata.xyz + idx * 3;
            return QVector3D(v[0], v[1], v[2]);
            };
        QVector3D p1 = getCoord(node1);
        QVector3D p2 = getCoord(node2);
        if (!p1.isNull() && !p2.isNull()) return (p2 - p1).normalized();
    }

    // 5. 几何点或网格节点 -> 两点方向（需要知道最新拾取的点与第一个点的差值）
    // 这里简化：如果找不到边/面，则放弃
    return QVector3D();
}

//新增针对多面的情况
MPREDATA_EXPORT bool PickManager::getSelectedFaceCenterAndNormal(QVector3D& center, QVector3D& normal)
{
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return false;
    auto* container = preRender->getSelectionContainer();
    if (!container) return false;
    auto* geoData = container->getGeoSelectData();
    if (!geoData) return false;
    auto* geoManager = getCVM->getModelData()->getGeoManager();
    if (!geoManager) return false;

    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (faceIds.size() != 1) return false;

    auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr(faceIds);
    if (attrs.empty()) return false;
    center = attrs[0].m_coor;   // 面心
    normal = attrs[0].m_dir;    // 法向（假设存在）
    return true;
}

MPREDATA_EXPORT std::vector<QVector3D> PickManager::getSelectedFaceCenters()
{

    std::vector<QVector3D> centers;
    auto* preRender = getCVM->getPreRenderWidget();
    if (!preRender) return centers;
    auto* container = preRender->getSelectionContainer();
    if (!container) return centers;
    auto* geoData = container->getGeoSelectData();
    if (!geoData) return centers;
    auto* geoManager = getCVM->getModelData()->getGeoManager();
    if (!geoManager) return centers;

    std::vector<int> faceIds = geoData->getSelectDataByType(SelectionInfoType::GeoFace);
    if (faceIds.empty()) return centers;
    auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr(faceIds);
    for (const auto& attr : attrs) {
        centers.push_back(attr.m_coor);
    }
    return centers;
}

//新增
MPREDATA_EXPORT float PickManager::getModelDiagonalLength(float defaultLen)
{
    if (!getCVM) return defaultLen;
    auto* modelData = getCVM->getModelData();
    if (!modelData) return defaultLen;
    auto* geoManager = modelData->getGeoManager();
    if (!geoManager) return defaultLen;
    auto* geoInfoCalc = geoManager->getGeoInfoCalc();
    if (!geoInfoCalc) return defaultLen;
  
    return defaultLen;   // 取对角线长度的 1/10 作为线的长度，可调
}



