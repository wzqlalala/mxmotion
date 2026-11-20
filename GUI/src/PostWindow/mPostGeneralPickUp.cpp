#include "mPostGeneralPickUp.h"

// =============================================================================
// 复制自 src/MainWindow/mGeneralPickUp.cpp（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：类名/命名空间替换，其余保持一致（含原版静态服务指针经 std::move
// 移交后置空、下次调用重建的行为）。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

namespace PostWindow
{
    std::shared_ptr<mPostPickHighlightServiceBase> mPostGeneralPickUp::_highlightServiceBase = nullptr;
    std::shared_ptr<mPostPickHighlightService> mPostGeneralPickUp::_highlightService = nullptr;

    mPostPickWidgetController* mPostGeneralPickUp::createPickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPostPickHighlightService>();
        }
        mPostPickWidgetView* view = new mPostPickWidgetView(colorBlock, true, false, false, PickMore, parent);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createOnePickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPostPickHighlightService>();
        }
        mPostPickWidgetView* view = new mPostPickWidgetView(colorBlock, true, true, true, PickOne, parent);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createOneDataWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPostPickHighlightService>();
        }
        mPostPickWidgetView* view = new mPostPickWidgetView(colorBlock, true, false, true, PickOne, parent);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createMorePickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPostPickHighlightService>();
        }
        mPostPickWidgetView* view = new mPostPickWidgetView(colorBlock, false, true, false, PickMore, parent);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createPickWidgetNoTempHight(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightServiceBase)
        {
            _highlightServiceBase = std::make_shared<mPostPickHighlightServiceBase>();
        }
        mPostPickWidgetView* view = new mPostPickWidgetView(colorBlock, true, false, false, PickMore, parent);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightServiceBase), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createDirectionPickWidgetPickMore(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightServiceBase)
        {
            _highlightServiceBase = std::make_shared<mPostPickHighlightServiceBase>();
        }
        mPostDirectPickWidgetView* view = new mPostDirectPickWidgetView(colorBlock, PickMore);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightServiceBase), std::move(view), parent);
        return pickPtr;
    }

    mPostPickWidgetController* mPostGeneralPickUp::createDirectionPickWidgetPickOne(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPostPickHighlightService>();
        }
        mPostDirectPickWidgetView* view = new mPostDirectPickWidgetView(colorBlock, PickOne);
        mPostPickWidgetController* pickPtr = new mPostPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }
}
