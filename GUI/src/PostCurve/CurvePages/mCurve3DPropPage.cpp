#include "mCurve3DPropPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveColorButton/mCurveLineWidthCombo/mCurvePenStyleCombo/mCurveSymbolStyleCombo/mCurveCheckBox 行工厂

//QT
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>

namespace PostCurve
{
	namespace
	{
		//页面公共骨架：纵向堆叠 FoldSection 的外层布局（同 mAxis3DPropPage/
		//mCurvePropPages.cpp 行工厂口径：边距/间距用布局默认值）
		QVBoxLayout* pageLayout(QWidget* host)
		{
			QVBoxLayout* layout = new QVBoxLayout(host);
			return layout;
		}

		//整行勾选框（勾选框在左、文字在右，§5.2 起勾选行统一形态）
		QCheckBox* createRowCheck(const QString& text, QWidget* parent)
		{
			mCurveCheckBox* check = new mCurveCheckBox(parent);
			check->setText(text);
			return check;
		}

		//置灰占位勾选框（构造即禁用固定勾选态，不连 commit/信号、不进回填；
		//R31 mAxis3DPropPage 同款工厂）
		QCheckBox* frozenRowCheck(const QString& text, bool checked, QWidget* parent)
		{
			QCheckBox* check = createRowCheck(text, parent);
			check->setChecked(checked);
			check->setEnabled(false);
			return check;
		}

		//一行双控件容器（参考截图并排形态；边距归零不改 FoldSection 行高，
		//容器行走 addRow(容器) 单参重载，左标签列为空——mAxis3DPropPage::pairRow
		//同款）
		QWidget* pairRow(QWidget* first, QWidget* second, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			lay->addWidget(first);
			lay->addWidget(second);
			return host;
		}
	}

	mCurve3DPropPage::mCurve3DPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mCurve3DPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//▾ 标准（1 行：可见性，生效；四类 plot3dType 均经 collectRenderCurves
		//通用过滤，恒可用）
		_baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		_baseFold->addRow(_visibleCheck);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});

		//▾ 曲线标签（6 行全占位，R31 先例：构造即禁用固定、不连 commit/信号、
		//不进 loadFromModel——唯一例外"标签内容"置灰回显曲线本名，只读展示）
		_labelFold = new mCurveFoldSection(QStringLiteral("曲线标签"), this);
		_labelVisibleCheck = frozenRowCheck(QStringLiteral("显示标签"), false, this);
		_labelFold->addRow(_labelVisibleCheck);
		_labelContentEdit = new QLineEdit(this);
		_labelContentEdit->setEnabled(false);   //置灰回显本名（loadFromModel 刷新），只读不连 commit
		_labelFold->addRow(QStringLiteral("标签内容"), _labelContentEdit);
		_labelFold->setRowEnabled(_labelContentEdit, false);   //行标签一并置灰
		_labelPrefixCheck = frozenRowCheck(QStringLiteral("显示标签前缀"), false, this);
		_labelFold->addRow(_labelPrefixCheck);
		_labelPrefixEdit = new QLineEdit(this);
		_labelPrefixEdit->setEnabled(false);   //置灰占位空
		_labelFold->addRow(QStringLiteral("标签前缀"), _labelPrefixEdit);
		_labelFold->setRowEnabled(_labelPrefixEdit, false);
		_labelSuffixCheck = frozenRowCheck(QStringLiteral("显示标签后缀"), false, this);
		_labelFold->addRow(_labelSuffixCheck);
		_labelSuffixEdit = new QLineEdit(this);
		_labelSuffixEdit->setEnabled(false);   //置灰占位空
		_labelFold->addRow(QStringLiteral("标签后缀"), _labelSuffixEdit);
		_labelFold->setRowEnabled(_labelSuffixEdit, false);

		//▾ 外观与着色（9 行，R36 起全生效——提交走命令栈，渲染消费见
		//m3DChartView 增量样式分支；仅 plot3dType==1 可用，启停见
		//updateStyleRowStates）
		_appearFold = new mCurveFoldSection(QStringLiteral("外观与着色"), this);
		//颜色（kColor，生效；提交后 m3DChartView 增量刷新保视角）
		_colorBtn = new mCurveColorButton(Qt::blue, this);
		_appearFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kColor, c);
		});
		//线宽（kLineWidth，生效；模块既有 1..4px 图标档，提交=档位px，与二维
		//曲线页同口径）
		_lineWidthCombo = new mCurveLineWidthCombo(this);
		_appearFold->addRow(QStringLiteral("线宽"), _lineWidthCombo);
		connect(_lineWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLineWidth, double(idx + 1));   //图标行0..3 -> 1..4px
		});
		//线型（kLineStyle，档位直传 0实/1虚/2点/3点划，与二维页同件同口径；
		//渲染端=引擎线型着色器虚线，R36 接线）
		_lineStyleCombo = new mCurvePenStyleCombo(this);
		_appearFold->addRow(QStringLiteral("线型"), _lineStyleCombo);
		connect(_lineStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLineStyle, idx);   //档位=模型值直传（TecLinePattern 同源）
		});
		//符号：行标签"符号"+行内[样式下拉+色块]双控件容器（对齐设计图行首标签+
		//行内双控件形态）。样式提交 kSymbolStyle（0=无 的语义由渲染端消费，页面
		//不额外写 showSymbol，与二维页"样式>0 派生"口径同源）；色块提交
		//kSymbolColor（符号色独立于线色）。启停走 setRowEnabled(容器)——行标签
		//一并置灰（与 mAxis3DPropPage 用法一致）
		_symbolStyleCombo = new mCurveSymbolStyleCombo(this);
		_symbolColorBtn = new mCurveColorButton(QColor(204, 0, 0), this);   //初值=模块符号色默认同源，回填覆盖
		_symbolRow = pairRow(_symbolStyleCombo, _symbolColorBtn, this);
		_appearFold->addRow(QStringLiteral("符号"), _symbolRow);
		connect(_symbolStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kSymbolStyle, idx);
		});
		connect(_symbolColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kSymbolColor, c);
		});
		//符号间隔（kSymbolDensity 1..50，1=逐点）/符号大小（kSymbolSize 1..100，
		//模型默认 6——设计图示 5 属截图态，不为截图改默认，文档备案）
		_symbolGapSpin = createCurveSpinBox(1, 50, this);
		_symbolGapSpin->setValue(1);
		_appearFold->addRow(QStringLiteral("符号间隔"), _symbolGapSpin);
		connect(_symbolGapSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kSymbolDensity, v);
		});
		_symbolSizeSpin = createCurveSpinBox(1, 100, this);
		_symbolSizeSpin->setValue(6);
		_appearFold->addRow(QStringLiteral("符号大小"), _symbolSizeSpin);
		connect(_symbolSizeSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kSymbolSize, v);
		});
		//线下区域填充（kAreaFill，模型默认 false——设计图勾选态属截图态备案；
		//渲染=幕帘填充自曲线拉至 Z 最小值地板，R36 接线）
		_areaFillCheck = createRowCheck(QStringLiteral("线下区域填充"), this);
		_appearFold->addRow(_areaFillCheck);
		connect(_areaFillCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAreaFill, on);
		});
		//填充透明度（kAreaFillOpacity 0..100，语义=不透明度，默认 70 对齐设计图
		//滑块位置；渲染 alpha=值/100）
		_fillOpacitySlider = new QSlider(Qt::Horizontal, this);
		_fillOpacitySlider->setRange(0, 100);
		_fillOpacitySlider->setValue(70);
		_appearFold->addRow(QStringLiteral("填充透明度"), _fillOpacitySlider);
		connect(_fillOpacitySlider, &QSlider::valueChanged, this, [this](int v)
		{
			commit(CurveProp::kAreaFillOpacity, v);
		});
		//云图：行内[勾选框"云图"+变量组合 x/y/z+"编辑图例..."按钮]三控件容器
		//（对齐设计图勾选框+下拉+按钮形态；addRow 容器单参，行首无标签）。
		//勾选框提交 kContour、变量下拉提交 kContourVar（0=x/1=y/2=z，默认 x）；
		//"编辑图例..."按钮保持置灰占位（色带图例编辑器=独立下一期需求）
		_contourCheck = createRowCheck(QStringLiteral("云图"), this);
		_contourCombo = new QComboBox(this);
		_contourCombo->addItem(QStringLiteral("x"));
		_contourCombo->addItem(QStringLiteral("y"));
		_contourCombo->addItem(QStringLiteral("z"));
		_contourEditBtn = new QPushButton(QStringLiteral("编辑图例..."), this);
		_contourEditBtn->setEnabled(false);   //恒置灰占位（下一期），tooltip 注明
		_contourEditBtn->setToolTip(QStringLiteral("色带图例编辑下一期"));
		_contourRow = new QWidget(this);
		{
			QHBoxLayout* lay = new QHBoxLayout(_contourRow);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			lay->addWidget(_contourCheck);
			lay->addWidget(_contourCombo);
			lay->addWidget(_contourEditBtn);
			_appearFold->addRow(_contourRow);
		}
		connect(_contourCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kContour, on);
		});
		connect(_contourCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kContourVar, idx);
		});

		//色带类四行（R38，plot3dType!=1；同一折叠组内第二套行集，与九行按类型
		//互斥可见——切换见 updateAppearanceRows。行序=设计稿：显示模式/颜色/
		//不透明度/云图）
		//1.显示模式：三选项下标=kDrawStyle 值（0 着色曲面/1 等高线/2 着色曲面+
		//等高线，语义=引擎 DrawStyle 同值）。仅 plot3dType==2 生效——connect
		//带类型守卫：占位类型（0 瀑布/3 柱状引擎无样式 API）程序化触发零 commit
		//（占位纪律"不连 commit"的等价实现；用户侧置灰不可达是双保险）
		_drawStyleCombo = new QComboBox(this);
		_drawStyleCombo->addItem(QStringLiteral("着色曲面"));
		_drawStyleCombo->addItem(QStringLiteral("等高线"));
		_drawStyleCombo->addItem(QStringLiteral("着色曲面+等高线"));
		_appearFold->addRow(QStringLiteral("显示模式"), _drawStyleCombo);
		connect(_drawStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			CurveNode* n = doc ? doc->node(_nodeId) : nullptr;
			if (n && n->series.plot3dType == 2)
				commit(CurveProp::kDrawStyle, qBound(0, idx, 2));
		});
		//2.颜色：通栏色条形态（R34 B02 bar 模式先例），构造即禁用占位不连
		//commit；静态色值照设计稿截图（瀑布=蓝/曲面=橙/柱状=紫，截图取色备案），
		//同一控件三类型显示不同静态色=loadFromModel 按当前类型 setColor
		//（阻塞信号；置灰态 Qt 照常显色）
		_bandColorBtn = new mCurveColorButton(QColor(85, 116, 189), this);
		_bandColorBtn->setBarMode(true);
		_bandColorBtn->setEnabled(false);
		_appearFold->addRow(QStringLiteral("颜色"), _bandColorBtn);
		//3.不透明度：置灰静态滑条（滑位照截图就近取整备案：瀑布 65/曲面 50/
		//柱状 50，loadFromModel 按类型就位），不连 commit
		_bandOpacitySlider = new QSlider(Qt::Horizontal, this);
		_bandOpacitySlider->setRange(0, 100);
		_bandOpacitySlider->setValue(50);
		_bandOpacitySlider->setEnabled(false);
		_appearFold->addRow(QStringLiteral("不透明度"), _bandOpacitySlider);
		//4.云图：行内[勾选框"云图"+变量组合 x/y/z+"编辑图例..."按钮]三控件
		//容器，全置灰占位（引擎对色带类本就自带色带，云图键下一期再议；
		//编辑图例钮 tooltip 同 R36"色带图例编辑下一期"）
		_bandContourCheck = createRowCheck(QStringLiteral("云图"), this);
		_bandContourCheck->setEnabled(false);
		_bandContourCombo = new QComboBox(this);
		_bandContourCombo->addItem(QStringLiteral("x"));
		_bandContourCombo->addItem(QStringLiteral("y"));
		_bandContourCombo->addItem(QStringLiteral("z"));
		_bandContourCombo->setEnabled(false);
		_bandContourEditBtn = new QPushButton(QStringLiteral("编辑图例..."), this);
		_bandContourEditBtn->setEnabled(false);   //恒置灰占位（下一期），tooltip 注明
		_bandContourEditBtn->setToolTip(QStringLiteral("色带图例编辑下一期"));
		_bandContourRow = new QWidget(this);
		{
			QHBoxLayout* lay = new QHBoxLayout(_bandContourRow);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			lay->addWidget(_bandContourCheck);
			lay->addWidget(_bandContourCombo);
			lay->addWidget(_bandContourEditBtn);
			_appearFold->addRow(_bandContourRow);
		}

		//构造默认=曲线类九行（plot3dType 缺省 1，页面骨架与 R36 现状一致）；
		//色带类四行待 loadFromModel 按本曲线类型切换（未绑定节点不显四行）
		_appearFold->setRowVisible(_drawStyleCombo, false);
		_appearFold->setRowVisible(_bandColorBtn, false);
		_appearFold->setRowVisible(_bandOpacitySlider, false);
		_appearFold->setRowVisible(_bandContourRow, false);

		layout->addWidget(_baseFold);
		layout->addWidget(_labelFold);
		layout->addWidget(_appearFold);
		layout->addStretch(1);
	}

	void mCurve3DPropPage::updateStyleRowStates(bool enabled)
	{
		//外观与着色全部九行动态启停：plot3dType==1（三维曲线）可用；0 瀑布/
		//2 曲面/3 柱状走色带渲染不消费外观样式，整组置灰防"提交无视觉效果"的
		//假编辑（R36 从颜色/线宽两行扩展）。setRowEnabled 连行标签一并置灰
		//（只禁编辑器时行标签保持黑字）；容器行走容器整体启停——子控件自身
		//enabled 标志独立，恒置灰的"编辑图例..."按钮不会因容器恢复而翻活
		_appearFold->setRowEnabled(_colorBtn, enabled);
		_appearFold->setRowEnabled(_lineWidthCombo, enabled);
		_appearFold->setRowEnabled(_lineStyleCombo, enabled);
		_appearFold->setRowEnabled(_symbolRow, enabled);
		_appearFold->setRowEnabled(_symbolGapSpin, enabled);
		_appearFold->setRowEnabled(_symbolSizeSpin, enabled);
		_appearFold->setRowEnabled(_areaFillCheck, enabled);
		_appearFold->setRowEnabled(_fillOpacitySlider, enabled);
		_appearFold->setRowEnabled(_contourRow, enabled);
	}

	void mCurve3DPropPage::updateAppearanceRows(int plot3dType)
	{
		//外观与着色两套行集按类型二分（R38）：曲线类（plot3dType==1）九行原样
		//（R36 口径零回归），色带类互斥可见——曲面/瀑布四行（显示模式/颜色/
		//不透明度/云图）、柱状仅颜色一行（20261113 用户二次拍板勘正：初版
		//误按四行实现，柱状设计稿本就只有颜色一行）——可见性切换走
		//FoldSection 行摘除/插回机制（setRowVisible 内同步内容区高度，行序
		//按 addRow 逻辑序插回）
		const bool isCurve = (plot3dType == 1);
		_appearFold->setRowVisible(_colorBtn, isCurve);
		_appearFold->setRowVisible(_lineWidthCombo, isCurve);
		_appearFold->setRowVisible(_lineStyleCombo, isCurve);
		_appearFold->setRowVisible(_symbolRow, isCurve);
		_appearFold->setRowVisible(_symbolGapSpin, isCurve);
		_appearFold->setRowVisible(_symbolSizeSpin, isCurve);
		_appearFold->setRowVisible(_areaFillCheck, isCurve);
		_appearFold->setRowVisible(_fillOpacitySlider, isCurve);
		_appearFold->setRowVisible(_contourRow, isCurve);
		//曲线类九行启停口径不变（R36；色带类行已隐藏，重发一次幂等收敛，
		//切回曲线类时状态正确）
		updateStyleRowStates(isCurve);
		//色带类四行可见性；显示模式仅曲面（plot3dType==2）可用——瀑布/柱状
		//引擎无样式 API 置灰防假编辑，曲线类行隐藏同步置灰（占位控件不参与
		//生效控件普查，与"可用下拉恰四"既有口径一致）；颜色/不透明度/云图
		//构造即禁用恒占位，无动态启停
		const bool isBand = !isCurve;
		const bool bandWithModes = (plot3dType == 0 || plot3dType == 2);   //瀑布/曲面四行；柱状仅颜色一行
		_appearFold->setRowVisible(_drawStyleCombo, isBand && bandWithModes);
		_appearFold->setRowVisible(_bandColorBtn, isBand);
		_appearFold->setRowVisible(_bandOpacitySlider, isBand && bandWithModes);
		_appearFold->setRowVisible(_bandContourRow, isBand && bandWithModes);
		_appearFold->setRowEnabled(_drawStyleCombo, plot3dType == 2);
	}

	void mCurve3DPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(_nodeId);
		if (!n)
			return;

		//可见性（生效行，恒可用）：全程阻塞信号回显（R30 教训：回显触发提交=
		//毒化模型；占位控件不连 commit 是双保险，生效行必须 QSignalBlocker）
		{
			const QSignalBlocker blocker(_visibleCheck);
			_visibleCheck->setChecked(n->series.visible);
		}

		//曲线标签组（全占位）：仅"标签内容"回显节点本名（对齐设计图"3D曲线 1"
		//形态；QLineEdit 置灰只读、不连 commit；本节点 kName 变化经基类
		//onNodePropertyChanged→loadFromModel 自动刷新，无需额外接线）。其余
		//占位控件构造时静态值一次成型，永不回写
		{
			const QSignalBlocker blocker(_labelContentEdit);
			_labelContentEdit->setText(n->name);
		}

		//外观与着色（生效行）：全程阻塞信号回显（R30 教训：回显触发提交=毒化
		//模型；同值赋值多数控件天然不发信号，QSignalBlocker 是双保险）。线宽
		//回显=二维曲线页同口径（非整px就近取整、钳制 1..4 档）；线型档位直传
		//（setPenStyleIndex 越界忽略）；符号样式钳 0..9。动态启停按本曲线
		//plot3dType（色带类九行整组置灰；可见性行恒可用不动）
		{
			const QSignalBlocker blockerColor(_colorBtn);
			const QSignalBlocker blockerWidth(_lineWidthCombo);
			const QSignalBlocker blockerStyle(_lineStyleCombo);
			const QSignalBlocker blockerSymStyle(_symbolStyleCombo);
			const QSignalBlocker blockerSymColor(_symbolColorBtn);
			const QSignalBlocker blockerGap(_symbolGapSpin);
			const QSignalBlocker blockerSize(_symbolSizeSpin);
			const QSignalBlocker blockerFill(_areaFillCheck);
			const QSignalBlocker blockerOpacity(_fillOpacitySlider);
			const QSignalBlocker blockerContour(_contourCheck);
			const QSignalBlocker blockerContourVar(_contourCombo);
			_colorBtn->setColor(n->series.color);
			_lineWidthCombo->setLineWidthIndex(qBound(0,
				int(n->series.lineWidth + 0.5) - 1, 3));
			_lineStyleCombo->setPenStyleIndex(qBound(0, n->series.lineStyle, 3));
			_symbolStyleCombo->setSymbolStyleIndex(qBound(0, n->series.symbolStyle, 9));
			_symbolColorBtn->setColor(n->series.symbolColor);
			_symbolGapSpin->setValue(qBound(1, n->series.symbolDensity, 50));
			_symbolSizeSpin->setValue(qBound(1, n->series.symbolSize, 100));
			_areaFillCheck->setChecked(n->series.areaFill);
			_fillOpacitySlider->setValue(qBound(0, n->series.areaFillOpacity, 100));
			_contourCheck->setChecked(n->series.contour);
			_contourCombo->setCurrentIndex(qBound(0, n->series.contourVar, 2));
		}

		//色带类四行静态占位值（R38）：显示模式仅曲面回显模型值（钳 0..2），
		//占位类型固定单选项"着色曲面"（下标 0，照截图）；颜色/滑位照设计稿
		//截图按类型就位（取色备案：瀑布=蓝 85,116,189/曲面=橙 221,147,96/
		//柱状=紫 145,112,188；滑位备案：瀑布 65/曲面 50/柱状 50）。置灰控件
		//不连 commit，QSignalBlocker 是回填防毒化双保险（R30 教训）
		{
			const QSignalBlocker blockerDraw(_drawStyleCombo);
			const QSignalBlocker blockerColor(_bandColorBtn);
			const QSignalBlocker blockerOpacity(_bandOpacitySlider);
			if (n->series.plot3dType == 2)
				_drawStyleCombo->setCurrentIndex(qBound(0, n->series.drawStyle, 2));
			else
				_drawStyleCombo->setCurrentIndex(0);
			switch (n->series.plot3dType)
			{
			case 2: _bandColorBtn->setColor(QColor(221, 147, 96)); break;   //曲面=橙
			case 3: _bandColorBtn->setColor(QColor(145, 112, 188)); break;  //柱状=紫
			default: _bandColorBtn->setColor(QColor(85, 116, 189)); break;  //瀑布=蓝
			}
			_bandOpacitySlider->setValue(n->series.plot3dType == 0 ? 65 : 50);
		}

		//两套行集按类型二分+动态启停（R38 统一出口，仿 updateStyleRowStates
		//末尾调用模式）
		updateAppearanceRows(n->series.plot3dType);
	}
}
