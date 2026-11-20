#include "mRotateAnalysePanelBase.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSurface.h>
#include<mElementSet.h>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mRotateAnalysePanelBase::mRotateAnalysePanelBase(QWidget* parent)
        : mPanelBase(parent)
    {}

    mRotateAnalysePanelBase::~mRotateAnalysePanelBase()
    {}

    void mRotateAnalysePanelBase::slot_finishedPick()
    {
        int objNum = 0;
        MxFunctions::PickDataType pickDataType = getCVM->getCurrentPickDataType(objNum);
        // 过滤非当前拾取类型的历史选择数据，避免切换拾取模式后旧数据被追加
        if (pickDataType != MxFunctions::PickDataType::None)
        {
            SaveSelectMemoryData(getCVM->getPreRenderWidget(), _pickeddata);
        }
        else
        {
            _pickeddata.clear();
        }
        if (_pickeddata.empty())
        {
            auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
            for (int i = 0; i < partIds.size(); ++i)
            {
                RenderPickResultData pickdata;
                pickdata.result_id = partIds[i];
                pickdata.part_id = partIds[i];
                _pickeddata[PickDataType::Part] = { pickdata };
            }
        }

    }

    void mRotateAnalysePanelBase::updateTarObject(QTreeWidget* treewidget, unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> _picks)
    {
        treewidget->clear();
        for (const auto& pair : _picks)
        {
            QTreeWidgetItem* parentItem = new QTreeWidgetItem(treewidget);
            QString parentname = "";
            switch (pair.first)
            {
            case PickDataType::GeoPoint:
                parentname = "\xe5\x87\xa0\xe4\xbd\x95\xe7\x82\xb9";
                break;
            case PickDataType::GeoEdge:
                parentname = "\xe5\x87\xa0\xe4\xbd\x95\xe8\xbe\xb9";
                break;
            case PickDataType::GeoFace:
                parentname = "\xe5\x87\xa0\xe4\xbd\x95\xe9\x9d\xa2";
                break;
            case PickDataType::GeoSolid:
                parentname = "\xe5\x87\xa0\xe4\xbd\x95\xe5\xae\x9e\xe4\xbd\x93";
                break;
            case PickDataType::Element:
                //      parentname = "\xe5\x8d\x95\xe5\x85\x83";
                continue;
            case PickDataType::Node:
                parentname = "\xe8\x8a\x82\xe7\x82\xb9";
                break;
            case PickDataType::Part:
                //      parentname = "\xe9\x83\xa8\xe4\xbb\xb6";
                continue;
            case PickDataType::ElementFace:
                //      parentname = "\xe5\x8d\x95\xe5\x85\x83\xe9\x9d\xa2";
                continue;
            case PickDataType::ElementEdge:
                //      parentname = "\xe5\x8d\x95\xe5\x85\x83\xe8\xbe\xb9";
                continue;
            default:
                break;
            }
            parentItem->setText(0, parentname);
            parentItem->setFlags(parentItem->flags() & ~Qt::ItemIsSelectable);   //让父节点不能被选中
            for (const auto& elem : pair.second)
            {
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, MxFunctions::mNumberToString(elem.result_id));
                item->setText(1, MxFunctions::mNumberToString(static_cast<int>(pair.first)));
            }
        }
        treewidget->expandAll();

    }

    void mRotateAnalysePanelBase::clearPick()
    {
        if (!getCVM)
        {
            return;
        }
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mRotateAnalysePanelBase::setHighlight(const MxFunctions::PickDataType& settype, const std::vector<int>& ids, bool on)
    {
        if (ids.size() == 0)
        {
            return;
        }
        mxr::SelectionOperate highlight;
        highlight = on ? mxr::SelectionOperate::ADD : mxr::SelectionOperate::REDUCE;
        std::vector<mxr::MeshSelectInfoData> meshids;
        for (auto id : ids)
        {
            mxr::MeshSelectInfoData n;
            n.data_id.element_id = id;
            meshids.push_back(n);
        }
        switch (settype)
        {
        case PickDataType::GeoPoint:
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, mxr::SelectionInfoType::GeoPoint, ids);
            break;
        case PickDataType::GeoEdge:
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoEdge, vector<int>{});
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, mxr::SelectionInfoType::GeoEdge, ids);
            break;
        case PickDataType::GeoFace:
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, mxr::SelectionInfoType::GeoFace, ids);
            break;
        case PickDataType::GeoSolid:
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, mxr::SelectionInfoType::GeoSolid, ids);
            break;
        case PickDataType::Node:
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, meshids);
            break;
        default:
            break;
        }
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    bool mRotateAnalysePanelBase::getPickVecData(QVector3D& vec)
    {
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
            double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;

            vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
            };
        auto lines = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
        if (lines.size() == 1)
        {
            auto res = geoManager->CalculateEdgeAttr(lines);
            vec = res.at(0).m_dir;
            return true;
        }
        auto faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
        if (faces.size() == 1)
        {
            auto res = geoManager->CalculateFaceAttr(faces);
            vec = res.at(0).m_dir;
            return true;
        }

        return false;
    }

    bool mRotateAnalysePanelBase::getPickPointData(QVector3D& point)
    {
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
            double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;

            vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
            };
        auto nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
        if (nodes.size() == 1)
        {
            getNodeVertexByID(meshData, nodes.at(0).data_id.node_id, point);
            return true;
        }
        auto points = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPoint);
        if (points.size() == 1)
        {
            auto res = geoManager->CalculateVertexAttr(points);
            point = res.at(0).m_coor;
            return true;
        }
        auto lines = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
        if (lines.size() == 1)
        {
            auto res = geoManager->CalculateEdgeAttr(lines);
            point = res.at(0).m_coor;
            return true;
        }
        auto faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
        if (faces.size() == 1)
        {
            auto res = geoManager->CalculateFaceAttr(faces);
            point = res.at(0).m_coor;
            return true;
        }
        auto solids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoSolid);
        if (solids.size() == 1)
        {
            auto res = geoManager->CalculateSolidAttr(solids);
            point = res.at(0).m_coor;
            return true;
        }

        return false;
    }

    void mRotateAnalysePanelBase::setRotateAnalyseData(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateData, const int& caseId)
    {
        _rotateData = rotateData;
        _caseId = caseId;
    }

}
