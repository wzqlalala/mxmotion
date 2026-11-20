#pragma once
#include "MPreData_global.h"
#include "mxr/basic/PublicStruct.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/SelectionCaculator.h"
#include <vector>
#include "mModelStruct.h"
//【镜像六件套摘除】旧 mxr::mGeoModelData1 前置声明（零使用）已删

namespace MechPreData
{
    //处理拾取结果的总函数
    MPREDATA_EXPORT void ProcessPickCaculateResult(mxr::FinishPickInfo, mxr::mBaseRend*, MxFunctions::ViewportType);

    //处理鼠标按下确认时的拾取结果处理
    MPREDATA_EXPORT void ProcessPickCaculateResult_Normal(mxr::FinishPickInfo, mxr::mBaseRend*, MxFunctions::ViewportType);

    //处理鼠标移动预览时的拾取结果处理
    MPREDATA_EXPORT void ProcessPickCaculateResult_PreView(mxr::FinishPickInfo, mxr::mBaseRend*, MxFunctions::ViewportType);

    //用Container的数据刷新一下前处理高亮
    MPREDATA_EXPORT void UpdatePreWidgetHighlight(mxr::CCSMRendWidget*);
    MPREDATA_EXPORT void UpdatePreWidgetHighlightWithoutRTRC(mxr::CCSMRendWidget*);
    //用Container的数据刷新一下后处理高亮
    MPREDATA_EXPORT void UpdatePostWidgetHighlight(mxr::REBUILD_PostRend*);
    //同上但不发 ReadyToReadContainer：拾取控件自身的高亮刷新专用，避免控件回读槽被
    //自己触发的通知二次唤醒（UpdatePreWidgetHighlightWithoutRTRC 同款语义）；
    //无结果帧（frameProxy 为空）时跳过不刷新
    MPREDATA_EXPORT void UpdatePostWidgetHighlightWithoutRTRC(mxr::REBUILD_PostRend*);

    //清空几何的拾取
    MPREDATA_EXPORT void ClearPreWidgetGeoPick(mxr::CCSMRendWidget*);

    //清空网格的拾取
    MPREDATA_EXPORT void ClearPreWidgetMeshPick(mxr::CCSMRendWidget*);
    MPREDATA_EXPORT void ClearPostWidgetMeshPick(mxr::REBUILD_PostRend*);

    //清空符号的拾取
    MPREDATA_EXPORT void ClearPreWidgetSymbolPick(mxr::CCSMRendWidget*);

    //仅清空前处理视口的拾取高亮显示（几何+网格高亮渲染层），不改动选择容器中的拾取数据
    //供云图面板等需要"数据保留、高亮隐藏"的场景使用
    MPREDATA_EXPORT void ClearPreWidgetPickHighlightKeepData(mxr::CCSMRendWidget*);

    //获取拾取部件(几何和网格统一)
    MPREDATA_EXPORT std::vector<int> getPreWidgetSelectPartID(mxr::CCSMRendWidget*);

    //设置部件拾取(几何和网格统一)
    MPREDATA_EXPORT void setPreWidgetSelectPartID(mxr::CCSMRendWidget*, const std::vector<int>&);

    //减少部件拾取(几何和网格统一)
    MPREDATA_EXPORT void reducePreWidgetSelectPartID(mxr::CCSMRendWidget*,const std::vector<int>&);

    MPREDATA_EXPORT void addPreWidgetSelectPartID(mxr::CCSMRendWidget*, const std::vector<int>&);

    //清空部件拾取(几何和网格统一)
    MPREDATA_EXPORT void clearPreWidgetSelectPartID(mxr::CCSMRendWidget*);

    // 获取存储拾取结果到dst
    MPREDATA_EXPORT void SaveSelectMemoryData(mxr::CCSMRendWidget* rend, MxFunctions::PickDataType type, std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& dst);
    MPREDATA_EXPORT void SaveSelectMemoryData(mxr::CCSMRendWidget* rend, std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst);

    // 数据转换：MeshSelectDataInfo ->  RenderPickResult
    MPREDATA_EXPORT RenderPickResultData SelectData2ResultData(mxr::SelectionInfoType type, const MeshSelectInfoData& data);
    MPREDATA_EXPORT RenderPickResultData SelectData2ResultData(MxFunctions::PickDataType type, const MeshSelectInfoData& data);

    // 数据转换：MeshSelectDataInfo ->  RenderPickResult
    MPREDATA_EXPORT MeshSelectInfoData PickData2SelectData(mxr::SelectionInfoType type, const RenderPickResultData& data);
    MPREDATA_EXPORT MeshSelectInfoData PickData2SelectData(MxFunctions::PickDataType type, const RenderPickResultData& data);

}