#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 兼容别名：折叠分组控件已下沉至 CommonWidgets 库（mFoldSection.h），
// 前处理/后处理/曲线三方参数栏共用同一实现，样式单一来源（原拷贝实现
// mCurveFoldSection 已删除，其行级显隐 setRowVisible/setRowEnabled、追加块
// addBlock、setHeaderHeight 等能力已合入共享 FoldSection）。
//
// 本头仅保留旧类型名，PostCurve 内既有调用点零改动；新代码可直接包含
// CommonWidgets/mFoldSection.h 使用 FoldSection。
//
// 注意：共享类标题头默认 25px（参数栏规格），弹窗等大尺寸宿主用
// setHeaderHeight 调高；QString 版 addRow 自动创建固定宽 90 标签。
// =============================================================================

#include "../PostCurveGlobal.h"

#include "mFoldSection.h"

namespace PostCurve
{
	using mCurveFoldSection = MechMainWindow::FoldSection;
}
