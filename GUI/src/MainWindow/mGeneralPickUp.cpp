#include "mGeneralPickUp.h"


using namespace MechPreData;
namespace MechMainWindow
{
    std::shared_ptr<mPickWidgetHighlightServiceBase> mGeneralPickUp::_highlightServiceBase = nullptr;
    std::shared_ptr<mPickWidgetHighlightService> mGeneralPickUp::_highlightService = nullptr;

    mPickWidgetController* mGeneralPickUp::createPickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, false, false, PickMore, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mSetPickWidgetConntroller* mGeneralPickUp::createNoCommitPickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, false, false, PickMore, parent);
        mSetPickWidgetConntroller* pickPtr = new mSetPickWidgetConntroller(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createOnePickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, true, true, PickOne, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createOneDataWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, false, true, PickOne, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createMorePickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, false, true, false, PickMore, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createPickWidgetNoTempHight(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightServiceBase)
        {
            _highlightServiceBase = std::make_shared<mPickWidgetHighlightServiceBase>();
        }
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, false, false, PickMore, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightServiceBase), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createSetPickWidget(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightServiceBase)
        {
            _highlightServiceBase = std::make_shared<mPickWidgetHighlightServiceBase>();
        }
        // 无临时高亮（基础高亮服务，仅存储高亮）+ 隐藏集合选择按钮
        mPickWidgetView* view = new mPickWidgetView(colorBlock, true, true, false, PickMore, parent);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightServiceBase), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createDirectionPickWidgetPickMore(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightServiceBase)
        {
            _highlightServiceBase = std::make_shared<mPickWidgetHighlightServiceBase>();
        }
        mDirectPickWidgetView* view = new mDirectPickWidgetView(colorBlock, PickMore);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightServiceBase), std::move(view), parent);
        return pickPtr;
    }

    mPickWidgetController* mGeneralPickUp::createDirectionPickWidgetPickOne(mColorBlock colorBlock, QWidget* parent)
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        mDirectPickWidgetView* view = new mDirectPickWidgetView(colorBlock, PickOne);
        mPickWidgetController* pickPtr = new mPickWidgetController(std::move(_highlightService), std::move(view), parent);
        return pickPtr;
    }

    std::shared_ptr<mPickWidgetHighlightService> mGeneralPickUp::getPickWidgetHighlightService()
    {
        if (!_highlightService)
        {
            _highlightService = std::make_shared<mPickWidgetHighlightService>();
        }
        return _highlightService;
    }
}