#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mGeneralPickUp.h（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：类名/命名空间替换，工厂创建的控件换后处理版本；工厂方法集保持一致
// （createDirectionPickWidget* 创建 mPostDirectPickWidgetView）。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "mPostPickWidgetController.h"
#include "mPostPickWidgetView.h"
#include "mPostDirectPickWidgetView.h"
#include "mPostPickHighlightServiceBase.h"
#include "mPostPickHighlightService.h"

namespace PostWindow
{
    class mPostGeneralPickUp
    {
    public:
        /*
        * @通用拾取
        * @功能：包含多个集合、多个拾取对象
        * @用途：后处理各结果面板的作用对象选择
        */
        static mPostPickWidgetController* createPickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含一个拾取
        */
        static mPostPickWidgetController* createOnePickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含一个拾取或一个集合
        */
        static mPostPickWidgetController* createOneDataWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：只包含多个拾取
        */
        static mPostPickWidgetController* createMorePickWidget(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @通用拾取
        * @功能：包含多个集合、多个拾取对象、无临时高亮
        */
        static mPostPickWidgetController* createPickWidgetNoTempHight(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @极简拾取
        * @功能：包含多个拾取对象、无集合、无临时高亮
        */
        static mPostPickWidgetController* createDirectionPickWidgetPickMore(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

        /*
        * @极简拾取
        * @功能：包含1个拾取对象、无集合、有临时高亮
        */
        static mPostPickWidgetController* createDirectionPickWidgetPickOne(mColorBlock colorBlock = CBNone, QWidget* parent = nullptr);

    private:
        //高亮类
        static std::shared_ptr<mPostPickHighlightServiceBase> _highlightServiceBase;
        static std::shared_ptr<mPostPickHighlightService> _highlightService;
    };
}
