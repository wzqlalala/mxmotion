#include "mPredefinedFieldPanelBase.h"

#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "PickCommonFunction.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>

#include "mSetManager.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mPredefinedFieldPanelBase::mPredefinedFieldPanelBase(QWidget* parent)
        : mPanelBase(parent)
    {
    }

    mPredefinedFieldPanelBase::~mPredefinedFieldPanelBase()
    {
    }

    void mPredefinedFieldPanelBase::updateTarObject(QTreeWidget* treewidget, unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> picks)
    {
        if (treewidget == nullptr)
        {
            return;
        }
        treewidget->clear();
        for (auto& pick : picks)
        {
            QString settypestr = "";
            switch (pick.first)
            {
            case PickDataType::GeoPoint:
                settypestr = "几何点";
                break;
            case PickDataType::GeoEdge:
                settypestr = "几何边";
                break;
            case PickDataType::GeoFace:
                settypestr = "几何面";
                break;
            case PickDataType::GeoSolid:
                settypestr = "几何体";
                break;
            case PickDataType::Node:
                settypestr = "节点";
                break;
            default:
                break;
            }
            if (!settypestr.isEmpty())
            {
                QTreeWidgetItem* top = new QTreeWidgetItem();
                top->setText(0, settypestr);
                top->setData(0, Qt::UserRole, static_cast<int>(pick.first));
                for (auto& d : pick.second)
                {
                    QTreeWidgetItem* child = new QTreeWidgetItem();
                    child->setText(0, MxFunctions::mNumberToString(d.result_id));
                    child->setText(1, MxFunctions::mNumberToString(static_cast<int>(pick.first)));
                    child->setData(0, Qt::UserRole, static_cast<int>(pick.first));
                    top->addChild(child);
                }
                treewidget->addTopLevelItem(top);
                treewidget->expandItem(top);
            }
        }
    }

    void mPredefinedFieldPanelBase::clearPick()
    {
        if (!getCVM ||
            getCVM->getPreRenderWidget() == nullptr)
        {
            return;
        }
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mPredefinedFieldPanelBase::setHighlight(const MxFunctions::PickDataType& settype, const std::vector<int>& ids, bool on)
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
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mPredefinedFieldPanelBase::setPredefinedFieldData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData, const int& caseId)
    {
        _fieldData = fieldData;
        _caseId = caseId;
    }
}
