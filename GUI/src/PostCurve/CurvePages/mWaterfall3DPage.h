#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图工具页：瀑布图（R37，T37-1）
//
// Ribbon"3D曲线"页"瀑布图"按钮挂载的参数栏工具页（非节点绑定页：不继承
// mCurvePropPageBase，无 setNodeId/commit 语义，直接 QWidget 组合 FoldSection）。
// 功能不存在，纯展示页——行集（组名/行名/行序/控件值）严格对齐设计稿
// design/3d-curve/ref-3dcurve-waterfall-page.png 三组+底部按钮：
//   ▾绘图类型：绘图类型(频率(Frequency))/切片坐标(时间)/响应曲线(3D曲线 1)/
//     输入曲线(无)/☐输入矢量为每分钟转数
//   ▾瀑布图切片：☑切片数量=12/☐切片步长=0.01/☐等值着色
//   ▾参数：采样长度(64)/窗函数(汉窗(Hanning))/幅值类型(幅值谱)/
//     频率计权(无(None))/☐展示全部功能
//   底部：[应用][撤销] 两钮
//
// 占位纪律（R31/R33/R34 同款，逐条）：全部控件构造即 setEnabled(false) 固定、
// 不连 commit/信号、不进任何回填路径；控件值=设计稿截图值静态写死；置灰观感
// 用控件自身样式表兜底（FoldSection 的 :disabled 规则只覆盖 Combo/Line/Spin，
// 按钮由全局 QSS QPushButton:disabled 兜底、勾选框由 mCurveCheckBox 自身
// 样式表兜底）。窄宽自适配（B01/B02 红线）：行按钮显式 setMinimumWidth(1)
// （覆盖全局 QSS width:80px 度量），整页 minimumSizeHint 宽 < 340px 级别，
// 窄 dock（~316px）无横向裁切。
//
// 页顶部一行加粗标题"瀑布图"（设计稿每页顶部形态）；页面骨架=标题+分组堆叠
// +addStretch，不设边距。页面由 mPostMenuContributor 懒创建缓存复用（挂载序列
// 照 mPostInfoHub::createCurve 先例）。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QWidget>

namespace PostCurve
{
	class POSTCURVE_EXPORT mWaterfall3DPage : public QWidget
	{
		Q_OBJECT
	public:
		explicit mWaterfall3DPage(QWidget* parent = nullptr);
	};
}
