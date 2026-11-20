#include <mGeoDataBase.h>
#include <mMeshData.h>
#include "mPickWidgetHighlightServiceBase.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"

using namespace MechPreData;
namespace MechMainWindow
{
    mPickWidgetHighlightServiceBase::mPickWidgetHighlightServiceBase(QObject* parent)
        : QObject(parent)
    {
        // 0ms 单次定时器：合并同一事件循环内多次高亮请求，避免批量操作卡顿
        _updateTimer = new QTimer(this);
        _updateTimer->setSingleShot(true);
        _updateTimer->setInterval(0);
        connect(_updateTimer, &QTimer::timeout, this, &mPickWidgetHighlightServiceBase::doUpdateHighlight);
    }

    void mPickWidgetHighlightServiceBase::setRenderWidget(mxr::CCSMRendWidget* widget)
    {
        _widget = widget;
    }

    void mPickWidgetHighlightServiceBase::setColorKey(const QString& colorKey)
    {
        _colorKey = colorKey;
    }

    void mPickWidgetHighlightServiceBase::setColor(const QColor& color)
    {
        // 基类无临时高亮渲染器，空实现
        Q_UNUSED(color);
    }

    void mPickWidgetHighlightServiceBase::ensureRenderWidget()
    {
        //模型全删后 getCVM 为空：置空 _widget，后续接口按空渲染窗口兜底
        _widget = getCVM ? getCVM->getPreRenderWidget() : nullptr;
    }

    mxr::SelectionInfoType mPickWidgetHighlightServiceBase::setTypeToSelectionInfoType(MxFunctions::PickDataType type) const
    {
        switch (type)
        {
        case PickDataType::GeoPoint:        return mxr::SelectionInfoType::GeoPoint;
        case PickDataType::GeoEdge:         return mxr::SelectionInfoType::GeoEdge;
        case PickDataType::GeoFace:         return mxr::SelectionInfoType::GeoFace;
        case PickDataType::GeoSolid:        return mxr::SelectionInfoType::GeoSolid;
        case PickDataType::Node:            return mxr::SelectionInfoType::MeshNode;
        case PickDataType::Element:         return mxr::SelectionInfoType::MeshElement;
        case PickDataType::ElementFace:     return mxr::SelectionInfoType::MeshFace;
        case PickDataType::ElementEdge:     return mxr::SelectionInfoType::MeshLine;
        // Part 类型无法仅凭类型区分几何/网格，调用处需通过 splitPartPicks 按 part_id 分流
        default:                           return mxr::SelectionInfoType::MeshTypeEnd;
        }
    }

    void mPickWidgetHighlightServiceBase::splitPartPicks(
        const std::vector<RenderPickResultData>& parts,
        std::vector<int>& geoPartIds,
        std::vector<mxr::MeshSelectInfoData>& meshPartInfos) const
    {
        if (!_widget) return;

        // 判断逻辑同 PickCommonFunction::addPreWidgetSelectPartID：
        // returnPartType > 1 为网格 part；getSetByPartId 非空为几何 part
        const auto& modelData = _widget->getModelData();
        for (const auto& d : parts)
        {
            const int partId = (d.part_id != -1) ? d.part_id : d.result_id;

            if (modelData._meshData &&
                mxr::refData<mMeshData>(modelData._meshData)->returnPartType(
                    mxr::refData<mMeshData>(modelData._meshData)->getPartNameByPartId(partId)) > 1)
            {
                mxr::MeshSelectInfoData n;
                n.part_id = partId;
                meshPartInfos.push_back(n);
            }

            if (modelData._geoDataBase && mxr::refData<MGeoData::GeoDataBase>(modelData._geoDataBase)->getSetByPartId(partId) != nullptr)
            {
                geoPartIds.push_back(partId);
            }
        }
    }

    void mPickWidgetHighlightServiceBase::highlightSet(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        bool on)
    {
        // 基类无临时高亮能力，空实现
        Q_UNUSED(data);
        Q_UNUSED(on);
    }

    void mPickWidgetHighlightServiceBase::highlightPicked(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks,
        bool on)
    {
        ensureRenderWidget();
        if (!_widget) return;

        // 拾取数据写入共享选中态容器，与视口交互拾取表现完全一致
        auto highlightOp = on ? mxr::SelectionOperate::ADD : mxr::SelectionOperate::REDUCE;
        for (const auto& pair : picks)
        {
            // Part 需按 part_id 区分几何 part 与网格 part，分别写入对应选中数据
            if (pair.first == MxFunctions::PickDataType::Part)
            {
                std::vector<int> geoPartIds;
                std::vector<mxr::MeshSelectInfoData> meshPartInfos;
                splitPartPicks(pair.second, geoPartIds, meshPartInfos);

                if (!geoPartIds.empty())
                {
                    _widget->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
                        highlightOp, mxr::SelectionInfoType::GeoPart, geoPartIds);
                }
                if (!meshPartInfos.empty())
                {
                    _widget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
                        highlightOp, mxr::SelectionInfoType::MeshPart, meshPartInfos);
                }
                continue;
            }

            mxr::SelectionInfoType infoType = setTypeToSelectionInfoType(pair.first);
            if (infoType == mxr::SelectionInfoType::MeshTypeEnd)
                continue;

            if (infoType >= mxr::SelectionInfoType::GeoPoint && infoType <= mxr::SelectionInfoType::GeoSolid)
            {
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                {
                    ids.push_back(d.result_id);
                }
                _widget->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlightOp, infoType, ids);
            }
            else
            {
                std::vector<mxr::MeshSelectInfoData> meshIds;
                meshIds.reserve(pair.second.size());
                for (const auto& d : pair.second)
                {
                    mxr::MeshSelectInfoData n;
                    n.data_id.element_id = d.result_id;
                    n.data_index.line_index = d.result_id2;
                    n.part_id = d.part_id;
                    meshIds.push_back(n);
                }
                _widget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
                    highlightOp, infoType, meshIds);
            }
        }

        requestUpdateHighlight();
    }

    void mPickWidgetHighlightServiceBase::clearGeoHighlight(mxr::mPreGeoHighLightRender* geoRender)
    {
        if (!geoRender) return;
        mGeoSelectData* emptyGeo = new mGeoSelectData();
        emptyGeo->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoPoint, {});
        emptyGeo->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoEdge, {});
        emptyGeo->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoFace, {});
        emptyGeo->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoSolid, {});
        geoRender->updateHighLightRender(emptyGeo);
        delete emptyGeo;
    }

    void mPickWidgetHighlightServiceBase::clearMeshHighlight(mxr::CCSMMeshHighLightRender* meshRender)
    {
        if (!meshRender) return;
        MeshSelectMemoryData* emptyMesh = new MeshSelectMemoryData();
        emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshNode, {});
        emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshElement, {});
        emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshFace, {});
        emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshLine, {});
        emptyMesh->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::MeshPart, {});
        meshRender->updateHighLightRender(emptyMesh);
        delete emptyMesh;
    }

    void mPickWidgetHighlightServiceBase::clearOwnTempHighlight()
    {
        // 基类无独立临时高亮渲染器，空实现
    }

    void mPickWidgetHighlightServiceBase::clearAllHighlight()
    {
        ensureRenderWidget();
        if (!_widget) return;

        // 清空共享选中态容器
        ClearPreWidgetGeoPick(_widget);
        ClearPreWidgetMeshPick(_widget);
        requestUpdateHighlight();
    }

    void mPickWidgetHighlightServiceBase::requestUpdateHighlight()
    {
        // 定时器已激活则跳过，合并为一次刷新
        if (_updateTimer && !_updateTimer->isActive())
        {
            _updateTimer->start();
        }
    }

    void mPickWidgetHighlightServiceBase::flushHighlightNow()
    {
        if (_updateTimer && _updateTimer->isActive())
        {
            _updateTimer->stop();
        }
        doUpdateHighlight();
    }

    void mPickWidgetHighlightServiceBase::doUpdateHighlight()
    {
        if (!_widget) return;
        // _widget 均来源于 getPreRenderWidget()（见 ensureRenderWidget/setRenderWidget）。
        // 定时器触发时前处理视口可能已被删除，_widget 悬空；与当前视口不一致则跳过。
        if (_widget != getCVM->getPreRenderWidget()) return;
        // 云图模式等高亮抑制场景：不从共享选中容器重画高亮（保留拾取数据，仅抑制显示）
        if (_highlightSuppressed)
        {
            _widget->updatePaint();
            return;
        }
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(_widget);
        _widget->updatePaint();
    }
}