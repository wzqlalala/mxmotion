#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图工具页：数据查询（截面查询，R37，T37-1）
//
// Ribbon"3D曲线"页"数据查询"按钮挂载的参数栏工具页（非节点绑定页，纯展示）。
// 行集严格对齐设计稿 design/3d-curve/ 数据查询页截图（仓库文件
// ref-3dcurve-data-query-page.png；三页 PNG 名与内容的交错已于 20261112
// 复核修正——文件名现与页名一致）三组：
//   ▾截面管理：☑显示截面/空列表/按钮行 [新增][删除][全选 / 清空][反选]
//   ▾截取定义：截取类型(下拉"阶次截取")/阶次=1/容差=0.15
//   ▾创建截面曲线：曲线横坐标(下拉"X")/曲线纵坐标(下拉"Z")/
//     创建窗口(下拉"当前窗口")/[创建截面曲线]钮
//
// 占位纪律与窄宽自适配同 mWaterfall3DPage（构造即禁用固定、不连 commit/信号、
// 不进回填；列表留空不造假数据；行按钮 setMinimumWidth(1) 可收缩）。
// 页面由 mPostMenuContributor 懒创建缓存复用。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QWidget>

namespace PostCurve
{
	class POSTCURVE_EXPORT mSectionQuery3DPage : public QWidget
	{
		Q_OBJECT
	public:
		explicit mSectionQuery3DPage(QWidget* parent = nullptr);
	};
}
