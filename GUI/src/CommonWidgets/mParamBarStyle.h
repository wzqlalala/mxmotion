#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 参数栏控件状态样式（单一来源）
//
// 内容：参数栏内按钮/工具钮/输入框/下拉的常态与悬停、按压、选中、禁用态规格。
// 取值同源全局 mApplicationStart.qss 的令牌体系（调用时从 UiPalette/UiMetrics
// 取值，与 ThemeManager::applyGlobalQss 同一口径），切主题/切档随
// mPanelBase::applyTheme 重发，不与全局 QSS 漂移。
//
// 为什么不在全局 QSS 一份了事：参数栏宿主（mInfoWindow 等）自身带
// QWidget{background:white} 级联样式表，Qt 中越近的祖先样式表优先级越高，
// 会盖掉应用级 QSS 的 :disabled 规则——参数面板需在自身层级重新落一份
// 同规格样式，禁用态灰底才生效（分散在各面板各写一份必然漂移，故收口于此）。
//
// 消费方：前处理面板基座 mPanelBase、曲线属性页基座 mCurvePropPageBase
// 构造时调用；调整参数栏状态色只改本文件。
// =============================================================================

#include "CommonWidgets_global.h"

class QWidget;

namespace MechMainWindow
{
    //在参数面板（或任意容器）上应用参数栏控件状态样式；host 为空时安全返回
    COMMONWIDGETS_EXPORT void applyParamBarStyle(QWidget* host);
}
