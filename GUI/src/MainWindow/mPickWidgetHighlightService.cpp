#include <memory>
#include "mPickWidgetHighlightService.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/CCSMMeshHighLightRender.h"
#include "mxr/view/mPreMeshFaceHighlightRender.h"
#include "mxr/view/mPreGeoHighLightRender.h"

using namespace MechPreData;
namespace MechMainWindow
{
    mPickWidgetHighlightService::mPickWidgetHighlightService(QObject* parent)
        : mPickWidgetHighlightServiceBase(parent)
    {
    }

    mPickWidgetHighlightService::~mPickWidgetHighlightService() = default;

    void mPickWidgetHighlightService::setColor(const QColor& color)
    {
        ensureRenderWidget();
        if (!_widget) return;
        _color = color;
    }

    void mPickWidgetHighlightService::highlightSet(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        bool on)
    {
        ensureRenderWidget();
        if (!_widget) return;

        auto highlightController = _widget->getHighlightController();
        if (!highlightController) return;

        // 第二参数传false：仅获取已有渲染器，不重置颜色，避免覆盖 setColor 设置的控件色
        auto meshHighLightRender = highlightController->getExtraPreMeshHighLightRender(_colorKey.toStdString());
        meshHighLightRender->setHighLightCover(true, -3.0f);
        meshHighLightRender->setHighLightColor(_color.red(), _color.green(), _color.blue());


        auto geoHighLightRender = highlightController->getExtraPreGeoHighLightRender(_colorKey.toStdString());
        geoHighLightRender->setHighLightCover(true, -3.0f);
        geoHighLightRender->setHighLightColor(_color.red(), _color.green(), _color.blue());        // 几何数据：全类型合并到一个数据对象，避免分次更新导致类型覆盖
        std::unique_ptr<mGeoSelectData> mergedGeoData;
        bool hasGeoData = false;

        // 网格数据：全类型合并到一个数据对象，保留完整 part/face 索引
        std::unique_ptr<MeshSelectMemoryData> mergedMeshData;
        bool hasMeshData = false;

        for (const auto& pair : data)
        {
            // Part 需按 part_id 区分几何 part 与网格 part，分别走对应高亮数据
            if (pair.first == MxFunctions::PickDataType::Part)
            {
                std::vector<int> geoPartIds;
                std::vector<mxr::MeshSelectInfoData> meshPartInfos;
                splitPartPicks(pair.second, geoPartIds, meshPartInfos);

                if (on && !geoPartIds.empty())
                {
                    if (!mergedGeoData)
                        mergedGeoData = std::make_unique<mGeoSelectData>();
                    mergedGeoData->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoPart, geoPartIds);
                    hasGeoData = true;
                }

                if (!meshPartInfos.empty())
                {
                    if (!mergedMeshData)
                        mergedMeshData = std::make_unique<MeshSelectMemoryData>();
                    mergedMeshData->UpdateSelectDataByType(
                        on ? mxr::SelectionOperate::ADD : mxr::SelectionOperate::CLEAR,
                        mxr::SelectionInfoType::MeshPart, meshPartInfos);
                    hasMeshData = true;
                }
                continue;
            }

            mxr::SelectionInfoType infoType = setTypeToSelectionInfoType(pair.first);
            if (infoType == mxr::SelectionInfoType::MeshTypeEnd)
                continue;

            if (infoType >= mxr::SelectionInfoType::GeoPoint && infoType <= mxr::SelectionInfoType::GeoSolid)
            {
                if (on)
                {
                    if (!mergedGeoData)
                        mergedGeoData = std::make_unique<mGeoSelectData>();

                    std::vector<int> ids;
                    ids.reserve(pair.second.size());
                    for (const auto& d : pair.second)
                        ids.push_back(d.result_id);
                    mergedGeoData->UpdatePickData(mxr::SelectionOperate::ADD, infoType, ids);
                    hasGeoData = true;
                }
            }
            else
            {
                if (!mergedMeshData)
                    mergedMeshData = std::make_unique<MeshSelectMemoryData>();

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
                mergedMeshData->UpdateSelectDataByType(
                    on ? mxr::SelectionOperate::ADD : mxr::SelectionOperate::CLEAR,
                    infoType, meshIds);
                hasMeshData = true;
            }
        }

        // 一次性更新几何高亮
        if (geoHighLightRender)
        {
            if (hasGeoData)
                geoHighLightRender->updateHighLightRender(mergedGeoData.get());
            else if (!on)
                clearGeoHighlight(geoHighLightRender);
        }

        // 一次性更新网格高亮
        if (meshHighLightRender)
        {
            if (hasMeshData)
                meshHighLightRender->updateHighLightRender(mergedMeshData.get());
            else if (!on)
                clearMeshHighlight(meshHighLightRender);
        }

        requestUpdateHighlight();
    }

    void mPickWidgetHighlightService::clearOwnTempHighlight()
    {
        ensureRenderWidget();
        if (!_widget) return;

        auto highlightController = _widget->getHighlightController();
        if (!highlightController) return;

        auto meshRender = highlightController->getExtraPreMeshHighLightRender(_colorKey.toStdString());
        if (meshRender)
            clearMeshHighlight(meshRender);

        auto geoRender = highlightController->getExtraPreGeoHighLightRender(_colorKey.toStdString());
        if (geoRender)
            clearGeoHighlight(geoRender);

        requestUpdateHighlight();
    }

    void mPickWidgetHighlightService::clearAllHighlight()
    {
        // 先清空自身临时高亮，再调用基类清空共享选中态
        clearOwnTempHighlight();
        mPickWidgetHighlightServiceBase::clearAllHighlight();
    }
}