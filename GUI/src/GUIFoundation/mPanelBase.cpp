#include <mMeshData.h>
#include "mPanelBase.h"
#include <QPalette>
#include <QStyle>
#include <QDebug>
#include <QPushButton>

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·集成】退役头（经 mOCCDocument.h 拉 OCC 链）；Calculate 族已切 engine().geoCalculate()

#include "mViewportModelManager.h"
#include "mViewportModel.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

namespace MechMainWindow
{

    bool getPickVecData(QVector3D& vec)
    {
        if (!getCVM)
        {
            return false;
        }

        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto selGeo = getCVM->getPreRenderWidget()
            ->getSelectionContainer()
            ->getGeoSelectData();

        QVector3D sum(0, 0, 0);
        int count = 0;

        // ---------- 几何边 ----------
        auto edges = selGeo->getSelectDataByType(SelectionInfoType::GeoEdge);
        if (!edges.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateEdgeAttr(edges);
            for (const auto& r : res)
            {
                sum += r.m_dir;
                ++count;
            }

            if (sum.length() < 1e-6)
                return false;

            vec = sum.normalized();
            return true;
        }

        // ---------- 几何面 ----------
        auto faces = selGeo->getSelectDataByType(SelectionInfoType::GeoFace);
        if (!faces.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateFaceAttr(faces);
            for (const auto& r : res)
            {
                sum += r.m_dir;
                ++count;
            }

            if (sum.length() < 1e-6)
                return false;

            vec = sum.normalized();
            return true;
        }

        return false;
    }

    bool getPickPointData(QVector3D& point)
    {
        auto* preRender = getCVM->getPreRenderWidget();
        auto* selContainer = preRender->getSelectionContainer();
        auto* geoManager = getCVM->getModelData()->getGeoManager();
        auto* meshData = getCVM->getModelData()->getMeshData();

        QVector3D sum(0, 0, 0);
        int count = 0;

        // ---------- 节点 ----------
        auto nodes = selContainer->getMeshSelectData()
            ->getSelectDataByType(SelectionInfoType::MeshNode);
        if (!nodes.empty())
        {
            for (const auto& n : nodes)
            {
                int id = n.data_id.node_id;
                double* v = meshData->_nodedata.xyz +
                    meshData->_nodedata.nodeidmap[id] * 3;
                sum += QVector3D(v[0], v[1], v[2]);
                ++count;
            }
            point = sum / count;
            return true;
        }

        // ---------- 几何点 ----------
        auto points = selContainer->getGeoSelectData()
            ->getSelectDataByType(SelectionInfoType::GeoPoint);
        if (!points.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateVertexAttr(points);
            for (const auto& r : res)
            {
                sum += r.m_coor;
                ++count;
            }
            point = sum / count;
            return true;
        }

        // ---------- 几何边 ----------
        auto edges = selContainer->getGeoSelectData()
            ->getSelectDataByType(SelectionInfoType::GeoEdge);
        if (!edges.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateEdgeAttr(edges);
            for (const auto& r : res)
            {
                sum += r.m_coor;
                ++count;
            }
            point = sum / count;
            return true;
        }

        // ---------- 几何面 ----------
        auto faces = selContainer->getGeoSelectData()
            ->getSelectDataByType(SelectionInfoType::GeoFace);
        if (!faces.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateFaceAttr(faces);
            for (const auto& r : res)
            {
                sum += r.m_coor;
                ++count;
            }
            point = sum / count;
            return true;
        }

        // ---------- 几何实体 ----------
        auto solids = selContainer->getGeoSelectData()
            ->getSelectDataByType(SelectionInfoType::GeoSolid);
        if (!solids.empty())
        {
            auto res = getCVM->getModelData()->getGeoEngineDataMgr()->engine().geoCalculate().CalculateSolidAttr(solids);
            for (const auto& r : res)
            {
                sum += r.m_coor;
                ++count;
            }
            point = sum / count;
            return true;
        }

        return false;
    }


}
