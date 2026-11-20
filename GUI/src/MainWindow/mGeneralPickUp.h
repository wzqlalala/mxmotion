#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "mPickWidgetController.h"
#include "mPickWidgetView.h"
#include "mDirectPickWidgetView.h"
#include "mPickWidgetHighlightServiceBase.h"
#include "mPickWidgetHighlightService.h"

namespace MechMainWindow
{
    class mGeneralPickUp
    {
    public:
        /*
        * @通用拾取
        * @功能：包含多个集合、多个拾取对象
        * @用途：载荷、边界
        */
        static mPickWidgetController* createPickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：包含多个集合对象，无提交
        * @用途：载荷、边界
        */
        static mSetPickWidgetConntroller* createNoCommitPickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含一个拾取
        * @用途：连接
        */
        static mPickWidgetController* createOnePickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含一个拾取或一个集合
        * @用途：载荷
        */
        static mPickWidgetController* createOneDataWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含多个拾取
        * @用途：连接
        */
        static mPickWidgetController* createMorePickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：包含多个集合、多个拾取对象、无临时高亮
        * @用途：接触
        */
        static mPickWidgetController* createPickWidgetNoTempHight(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含多个拾取对象、无集合选择、无临时高亮（仅拾取高亮）
        * @用途：集合面板
        */
        static mPickWidgetController* createSetPickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @方向选择拾取
        * @功能：包含多个拾取对象、无集合、无临时高亮
        * @用途：载荷，边界
        */
        static mPickWidgetController* createDirectionPickWidgetPickMore(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @方向选择拾取
        * @功能：包含1个拾取对象、无集合、有临时高亮
        * @用途：载荷，边界
        */
        static mPickWidgetController* createDirectionPickWidgetPickOne(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * 获取拾取高亮服务（懒加载，供需要直接组装 mPickWidgetController 的面板复用）
        */
        static std::shared_ptr<mPickWidgetHighlightService> getPickWidgetHighlightService();

    private:
        //高亮类
        static std::shared_ptr<mPickWidgetHighlightServiceBase> _highlightServiceBase;
        static std::shared_ptr<mPickWidgetHighlightService> _highlightService;
    };
}