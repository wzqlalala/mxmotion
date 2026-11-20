#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图曲线节点参数栏属性页（chartType=5 的 Curve 子节点，R33）
//
// 现状问题：三维图下选中曲线节点参数栏回落二维曲线通用页 mCurveSeriesPropPage
//（标准/曲线标签/线条属性/符号属性四组），与三维设计图不符。本页对齐设计图
// design/3d-curve/ref-3dcurve-prop-page.png 三组行集（组名/行名/行序逐项一致），
// 控件样式沿用模块参数栏既有控件（用户拍板"样式使用原本的"）；注册分流见
// mCurvePropPageManager::takeParamBarPage（Curve 节点按父图表 chartType==5
// 路由到本页，独立缓存槽；弹窗族不注册——三维画布无双击/右键编辑入口，
// showEditorPopup 路径不可达，行为不变）。
//
// 行集与生效/占位口径（20261112 用户拍板；R36 起外观七行翻活）：
//   ▾标准（1 行）：可见性（生效，kVisible，整行勾选——四类 plot3dType 均经
//     collectRenderCurves 通用过滤，恒可用）
//   ▾曲线标签（6 行，全占位）：显示标签(置灰不勾)/标签内容(置灰回显曲线本名——
//     唯一进 loadFromModel 的占位行，只读展示不连 commit；本节点 kName 变化经
//     基类 onNodePropertyChanged→loadFromModel 自动刷新)/显示标签前缀(置灰
//     不勾)/标签前缀(置灰空)/显示标签后缀(置灰不勾)/标签后缀(置灰空)
//   ▾外观与着色（9 行，R36 起全生效——仅 plot3dType==1 可用，其余类型整组
//     置灰防假编辑）：颜色(kColor)/线宽(kLineWidth，模块既有 1..4px 图标档，
//     提交=档位px、回显就近取整钳 1..4 与二维页同口径)/线型(kLineStyle，档位
//     直传 0实/1虚/2点/3点划)/符号(kSymbolStyle 样式+kSymbolColor 色块双控件
//     容器；样式 0=无 的语义由渲染端消费，页面不额外写 showSymbol)/符号间隔
//     (kSymbolDensity，1..50)/符号大小(kSymbolSize，1..100，模型默认 6——设计
//     图示 5 属截图态备案)/线下区域填充(kAreaFill，模型默认 false——设计图勾选
//     态属截图态备案)/填充透明度(kAreaFillOpacity，0..100，语义=不透明度，
//     默认 70)/云图([kContour 勾选框+kContourVar 变量下拉 x/y/z+"编辑图例..."
//     按钮]三控件容器——按钮保持置灰占位，色带图例编辑下一期)
//
// 占位纪律（R31 mAxis3DPropPage 同款先例，逐条）：占位控件构造即禁用固定、
// 不连 commit/信号、不进 loadFromModel（标签内容回显本名例外）；生效行回显
// 全程 QSignalBlocker（R30 教训：chartType=5 曾被信号钳制毒化）；页面骨架
// =pageLayout+addStretch，不设边距。
//
// 动态启停：loadFromModel 按本曲线 series.plot3dType 启停 外观与着色全部九行
//（==1 三维曲线可用；0 瀑布/2 曲面/3 柱状走色带渲染不消费外观样式，置灰防
// "提交无视觉效果"的假编辑；setRowEnabled 连行标签一并置灰）。
//
// 行集按类型二分（R38，20261113 设计稿拍板）：色带类（plot3dType!=1）外观与
// 着色不再显示九行整组置灰，改为新四行（同一折叠组内两套行集按类型切可见，
// FoldSection 行摘除/插回机制）：
//   1.显示模式（QComboBox 三选项 着色曲面/等高线/着色曲面+等高线，下标=
//     kDrawStyle 值）——仅 plot3dType==2 生效（connect 带类型守卫提交
//     kDrawStyle，m3DChartView 增量消费）；瀑布/柱状引擎无样式 API，置灰
//     固定单选项"着色曲面"（占位纪律，程序化触发零 commit）
//   2.颜色（mCurveColorButton 通栏色条形态，R34 B02 bar 模式先例）——置灰
//     静态色值照设计稿截图（瀑布=蓝/曲面=橙/柱状=紫，截图取色备案），不连
//     commit；同一控件三类型显示不同静态色=loadFromModel 按类型 setColor
//   3.不透明度（QSlider）——置灰静态，滑位照截图（瀑布 65/曲面/柱状 50 备案），
//     不连 commit
//   4.云图（[勾选框+变量下拉+"编辑图例..."按钮]三控件容器）——全置灰占位
//     （编辑图例钮 tooltip 同 R36"色带图例编辑下一期"）
// 两套行集的可见性/启停切换集中在 updateAppearanceRows 单一函数（loadFromModel
// 末尾调用，仿 updateStyleRowStates 模式不散落）。
//
// 生效路径：外观九行/曲面显示模式提交走命令栈后，m3DChartView 增量样式刷新
//（保视角，不整场景重建——见 m3DChartView.cpp 样式分流分支，R36 键集扩至
// 线型/符号四件/填充两件/云图两件；R38 增 kDrawStyle 曲面专享分支）。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "mCurvePropPageBase.h"
#include "../CurveWidgets/mCurveFoldSection.h"   //mCurveFoldSection = 共享 FoldSection 别名（成员指针用）

//QT
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QSpinBox>

namespace PostCurve
{
	class mCurveColorButton;       //实心色块按钮（CurveWidgets，成员指针用）
	class mCurveLineWidthCombo;    //线宽图标下拉（1..4px 档）
	class mCurvePenStyleCombo;     //线型图标下拉
	class mCurveSymbolStyleCombo;  //符号图标下拉

	class POSTCURVE_EXPORT mCurve3DPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mCurve3DPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//外观与着色全部九行动态启停（plot3dType==1 可用，其余色带类整组置灰；
		//setRowEnabled 连行标签一并置灰）——loadFromModel 末尾按模型现值调用
		void updateStyleRowStates(bool enabled);
		//外观与着色两套行集按类型二分+各行动态启停（R38，loadFromModel 末尾
		//统一调用不散落——仿 updateStyleRowStates 模式）：曲线类（==1）九行/
		//色带类（0 瀑布/2 曲面/3 柱状）四行互斥可见（FoldSection 行摘除/插回
		//机制，行序按 addRow 逻辑序插回、内容区高度同步由该机制自带）
		void updateAppearanceRows(int plot3dType);
		mCurveFoldSection* _baseFold = nullptr;      //标准分组
		QCheckBox* _visibleCheck = nullptr;          //可见性（kVisible，生效，恒可用）
		mCurveFoldSection* _labelFold = nullptr;     //曲线标签分组（全占位）
		QCheckBox* _labelVisibleCheck = nullptr;     //显示标签（置灰占位，不勾）
		QLineEdit* _labelContentEdit = nullptr;      //标签内容（置灰回显曲线本名，唯一进回填路径的占位行）
		QCheckBox* _labelPrefixCheck = nullptr;      //显示标签前缀（置灰占位，不勾）
		QLineEdit* _labelPrefixEdit = nullptr;       //标签前缀（置灰占位，空）
		QCheckBox* _labelSuffixCheck = nullptr;      //显示标签后缀（置灰占位，不勾）
		QLineEdit* _labelSuffixEdit = nullptr;       //标签后缀（置灰占位，空）
		mCurveFoldSection* _appearFold = nullptr;    //外观与着色分组（九行，R36 起全生效）
		mCurveColorButton* _colorBtn = nullptr;      //颜色（kColor，生效）
		mCurveLineWidthCombo* _lineWidthCombo = nullptr; //线宽（kLineWidth，生效，1..4px 图标档）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr;  //线型（kLineStyle，档位直传 0..3）
		QWidget* _symbolRow = nullptr;               //符号行容器[样式下拉+色块]（启停按容器整行）
		mCurveSymbolStyleCombo* _symbolStyleCombo = nullptr; //符号·样式（kSymbolStyle 0无..9点）
		mCurveColorButton* _symbolColorBtn = nullptr;    //符号·色块（kSymbolColor）
		QSpinBox* _symbolGapSpin = nullptr;          //符号间隔（kSymbolDensity，1..50）
		QSpinBox* _symbolSizeSpin = nullptr;         //符号大小（kSymbolSize，1..100，模型默认 6）
		QCheckBox* _areaFillCheck = nullptr;         //线下区域填充（kAreaFill，模型默认 false）
		QSlider* _fillOpacitySlider = nullptr;       //填充透明度（kAreaFillOpacity 0..100，默认 70）
		QWidget* _contourRow = nullptr;              //云图行容器[勾选框+变量下拉+编辑图例按钮]
		QCheckBox* _contourCheck = nullptr;          //云图·勾选框（kContour）
		QComboBox* _contourCombo = nullptr;          //云图·变量下拉（kContourVar 0=x/1=y/2=z）
		QPushButton* _contourEditBtn = nullptr;      //云图·编辑图例按钮（恒置灰占位，下一期）
		//色带类四行（R38，plot3dType!=1 可见；除曲面显示模式外全部占位纪律——
		//构造即禁用、不连 commit/信号、不进回填）
		QComboBox* _drawStyleCombo = nullptr;        //显示模式（kDrawStyle，仅 plot3dType==2 生效提交）
		mCurveColorButton* _bandColorBtn = nullptr;  //颜色（置灰静态色条照设计稿截图，按类型换色）
		QSlider* _bandOpacitySlider = nullptr;       //不透明度（置灰静态滑条照截图，按类型就位）
		QWidget* _bandContourRow = nullptr;          //云图行容器[勾选框+变量下拉+编辑图例按钮]（全置灰占位）
		QCheckBox* _bandContourCheck = nullptr;      //云图·勾选框（置灰占位）
		QComboBox* _bandContourCombo = nullptr;      //云图·变量下拉（置灰占位）
		QPushButton* _bandContourEditBtn = nullptr;  //云图·编辑图例按钮（恒置灰占位，下一期）
	};
}
