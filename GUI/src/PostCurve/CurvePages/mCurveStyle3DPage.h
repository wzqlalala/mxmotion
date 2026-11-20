#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图工具页：曲线样式（R37，T37-1）
//
// Ribbon"3D曲线"页"曲线样式"按钮挂载的参数栏工具页（非节点绑定页：不继承
// mCurvePropPageBase，图表级新页——构造后经 setChart(factory->currentChartId())
// 绑定当前激活三维图，重复挂载/换图表由 Ribbon 侧刷新上下文）。
//
// 行集对齐设计稿 design/3d-curve/ref-3dcurve-curve-style-page.png 两组
// （20261117 用户勘正：图形类型改类型视图过滤器，不再置灰占位）：
//   ▾图形选择与显示：图形类型(类型视图下拉=过滤器，同 mDefine3DCurveDlg——
//     曲线/曲面/瀑布图/柱状图恒可用，切换=按 plot3dType 重建过滤列表并自动
//     选中首条，纯视图态不回写模型)/曲线列表(QListWidget=当前图表当前类型
//     视图 Curve 子节点树序，ExtendedSelection 多选——20261117 用户勘正二期：
//     多选同调样式，选中集跨刷新保持；条目勾选态=series.visible 回显，
//     QSignalBlocker；用户勾选/取消条目=立即提交该曲线 kVisible 单命令，
//     可撤销)/按钮行1 [全部][取消][反选][已显示]=列表**选中集**操作（纯 UI
//     态不提交——全部=一键全选/取消=清空选中/反选=翻转/已显示=仅选当前
//     visible 者；20261117 勘正：原勾选态操作改选中集）/按钮行2 [显示][隐藏]
//     =把列表**选中集**批量提交 kVisible(true/false)，beginMacro("曲线样式-
//     显示"/"曲线样式-隐藏") 包裹一次 Ctrl+Z 整组撤销（mApplyStyleDlg 先例；
//     20261117 勘正：原勾选集口径下"显示"恒空集无作用——被隐藏曲线必未勾选
//     进不了勾选集，改选中集后选中之即可再显示）；[图例显示][图例隐藏]=查
//     本图表 Legend 节点提交 kVisible（R32 覆盖层消费，单命令自带撤销；查
//     不到静默返回）
//   ▾外观与着色（作用于列表选中集全部曲线——回显锚=当前行（须在选中集内）
//     否则选中集首行；空选中整组禁用；行集与
//     提交口径=mCurve3DPropPage R36 翻活后现状）：颜色(kColor)/线宽(kLineWidth
//     1..4px 档)/线型(kLineStyle)/符号(kSymbolStyle+kSymbolColor 双控件)/
//     符号间隔(kSymbolDensity)/符号大小(kSymbolSize)/线下区域填充(kAreaFill)/
//     填充透明度(kAreaFillOpacity 滑条)/云图([kContour 勾选框+kContourVar 变量
//     下拉+"编辑图例..."恒置灰钮]三控件容器)。全部经
//     mCurveDocController::setNodeProperty 提交到选中集每条曲线（单条=单
//     命令，多条=beginMacro"曲线样式-批量修改"一次 Ctrl+Z；命令栈可撤销；
//     颜色/线宽走 R33 保视角增量刷新既有渲染路径，本页零新增渲染代码）。
//     两套行集按图形类型视图二分（20261117 用户勘正三期：切类型外观组随变
//     ——R38 口径自 mCurve3DPropPage 镜像）：曲线（==1）九行如上；色带类=
//     曲面/瀑布（2/0）四行[显示模式(kDrawStyle 三选项下标 0 着色曲面/1 等
//     高线/2 着色曲面+等高线——仅曲面视图生效，瀑布置灰固定"着色曲面"；颜色
//     =bar 形态置灰静态色照截图取色备案：瀑布=蓝 85,116,189/曲面=橙
//     221,147,96/柱状=紫 145,112,188；不透明度=置灰静态滑条：瀑布 65/曲面
//     50；云图=三控件容器全置灰)]、柱状（3）仅颜色一行（20261113 勘正口径）；
//     可见性切换=FoldSection setRowVisible 行摘除/插回，曲线视图九行启停
//     口径不变（==1 可用），占位纪律同 mCurve3DPropPage R38。
//
// 上下文维护：构造函数全局监听控制器 sig_nodePropertyChanged/nodeAdded/
// nodeRemoved（mAnnotation3DPropPage 先例）——本图表节点及其后代变化即重载
// 列表与外观回显（回显全程阻塞信号，无回环；本页自身提交触发的通知同样走
// 重载，幂等收敛）；图表被删除→列表清空、全部按钮与外观组禁用，不崩溃。
//
// 占位纪律/窄宽自适配（B01/B02）同 mWaterfall3DPage；页顶部加粗标题"曲线样式"。
// 页面由 mPostMenuContributor 懒创建缓存复用（QPointer 保活）。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "../CurveWidgets/mCurveFoldSection.h"

//QT
#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QList>
#include <QListWidget>
#include <QPushButton>

class QSpinBox;
class QSlider;

namespace PostCurve
{
	class mCurveColorButton;       //实心色块按钮（CurveWidgets）
	class mCurveLineWidthCombo;    //线宽图标下拉（1..4px 档）
	class mCurvePenStyleCombo;     //线型图标下拉
	class mCurveSymbolStyleCombo;  //符号图标下拉

	class POSTCURVE_EXPORT mCurveStyle3DPage : public QWidget
	{
		Q_OBJECT
	public:
		explicit mCurveStyle3DPage(QWidget* parent = nullptr);

		//绑定当前图表（挂载前由 Ribbon 传 factory->currentChartId()）：
		//换图表/图表删除后重绑均走这里刷新列表与回显
		void setChart(int chartId);
		int chartId() const { return _chartId; }

	private:
		void buildUi();
		//按 _chartId 现状整体刷新（列表+按钮启停+外观回显；幂等，可重入）
		void refreshAll();
		//曲线列表重建（当前类型视图 Curve 子节点树序，勾选态=series.visible；
		//全程阻塞信号；尽量保持当前选中曲线不变）
		void refreshList();
		//类型视图下拉序 -> plot3dType（kTypePlot3d 固定映射；越界 -1=无视图）
		int viewPlot3dType() const;
		//类型视图切换（mDefine3DCurveDlg::onTypeChanged 同款）：按新类型重建
		//过滤列表，有条目自动选中首条、无条目无选中态（外观组随之禁用）
		void onTypeChanged();
		//外观与着色回显+启停（按回显锚曲线回显；空选中/图表无效整组禁用，
		//色带类占位行静态备案值回显；显示/隐藏钮随选中集启停在此收口）
		void refreshAppearance();
		//外观两套行集按图形类型视图二分（R38 口径镜像 mCurve3DPropPage
		//::updateAppearanceRows）：曲线九行/色带类曲面瀑布四行、柱状仅颜色
		//一行；幂等可重入（改选链路每次重回显都带视图类型）
		void updateAppearanceRows(int plot3dType);
		//列表选中集曲线 id（行序；多选同调/显示隐藏批量提交的目标集）
		QList<int> selectedCurveIds() const;
		//外观回显锚曲线节点 id：当前行（须在选中集内）优先，否则选中集首行；
		//空选中返回 -1
		int echoCurveId() const;
		//对选中集全部曲线提交（空选中/_syncing 期间静默忽略；单条=单命令，
		//多条=宏"曲线样式-批量修改"一次撤销）
		void commitSelected(const QString& prop, const QVariant& value);
		//选中集批量提交 kVisible（宏包裹一次整组撤销；空选中集零操作）
		void commitSelectedVisible(bool on);
		//图例显隐（查本图表 Legend 节点提交 kVisible；查不到静默返回）
		void commitLegendVisible(bool on);

		int _chartId = -1;           //绑定的图表节点 id（-1=未绑定/图表已删）
		bool _syncing = false;       //回填标志：refreshAll 期间屏蔽提交（防回环）

		mCurveFoldSection* _selectFold = nullptr;    //图形选择与显示分组
		QComboBox* _plotTypeCombo = nullptr;         //图形类型（类型视图过滤器，同 mDefine3DCurveDlg）
		QListWidget* _curveList = nullptr;           //曲线列表（多选，勾选=kVisible/条目，选中=样式目标集）
		QList<int> _curveIds;                        //列表条目曲线 id（树序，与条目一一对应）
		QPushButton* _allBtn = nullptr;              //全部（一键全选）
		QPushButton* _noneBtn = nullptr;             //取消（清空选中）
		QPushButton* _invertBtn = nullptr;           //反选（翻转选中集）
		QPushButton* _shownOnlyBtn = nullptr;        //已显示（仅选当前 visible 者）
		QPushButton* _showBtn = nullptr;             //显示（选中集批量 kVisible=true，宏）
		QPushButton* _hideBtn = nullptr;             //隐藏（选中集批量 kVisible=false，宏）
		QPushButton* _legendShowBtn = nullptr;       //图例显示（Legend kVisible）
		QPushButton* _legendHideBtn = nullptr;       //图例隐藏

		mCurveFoldSection* _appearFold = nullptr;    //外观与着色分组（九行，键=mCurve3DPropPage R36 口径）
		mCurveColorButton* _colorBtn = nullptr;      //颜色（kColor）
		mCurveLineWidthCombo* _lineWidthCombo = nullptr; //线宽（kLineWidth）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr;  //线型（kLineStyle）
		QWidget* _symbolRow = nullptr;               //符号行容器[样式下拉+色块]
		mCurveSymbolStyleCombo* _symbolStyleCombo = nullptr; //符号·样式（kSymbolStyle）
		mCurveColorButton* _symbolColorBtn = nullptr;    //符号·色块（kSymbolColor）
		QSpinBox* _symbolGapSpin = nullptr;          //符号间隔（kSymbolDensity）
		QSpinBox* _symbolSizeSpin = nullptr;         //符号大小（kSymbolSize）
		QCheckBox* _areaFillCheck = nullptr;         //线下区域填充（kAreaFill，mCurveCheckBox 实体）
		QSlider* _fillOpacitySlider = nullptr;       //填充透明度（kAreaFillOpacity）
		QWidget* _contourRow = nullptr;              //云图行容器[勾选框+变量下拉+编辑图例钮]
		QCheckBox* _contourCheck = nullptr;          //云图·勾选框（kContour，mCurveCheckBox 实体）
		QComboBox* _contourCombo = nullptr;          //云图·变量下拉（kContourVar）
		QPushButton* _contourEditBtn = nullptr;      //编辑图例...（恒置灰占位）

		//色带类四行（R38 第二套行集，与曲线九行按视图类型互斥可见；曲面/瀑布
		//四行、柱状仅颜色行）
		QComboBox* _drawStyleCombo = nullptr;        //显示模式（kDrawStyle，仅曲面视图生效）
		mCurveColorButton* _bandColorBtn = nullptr;  //颜色（bar 形态置灰静态色，按类型备案取色）
		QSlider* _bandOpacitySlider = nullptr;       //不透明度（置灰静态滑条，按类型备案滑位）
		QCheckBox* _bandContourCheck = nullptr;      //云图·勾选框（置灰占位）
		QComboBox* _bandContourCombo = nullptr;      //云图·变量下拉（置灰占位）
		QPushButton* _bandContourEditBtn = nullptr;  //编辑图例...（恒置灰占位）
		QWidget* _bandContourRow = nullptr;          //云图行容器
	};
}
