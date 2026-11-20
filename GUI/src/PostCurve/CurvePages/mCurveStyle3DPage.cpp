#include "mCurveStyle3DPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveColorButton/mCurveLineWidthCombo/mCurvePenStyleCombo/mCurveSymbolStyleCombo/mCurveCheckBox 行工厂

//QT
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QUndoStack>
#include <QVBoxLayout>

namespace PostCurve
{
	namespace
	{
		//类型下拉序 -> plot3dType（对齐 set3DSeriesData 的序列类型：0 瀑布/1 曲线/
		//2 曲面/3 柱状；下拉默认"曲线"；与 mDefine3DCurveDlg 匿名区同表）
		const int kTypePlot3d[] = { 1, 2, 0, 3 };

		QVBoxLayout* pageLayout(QWidget* host)
		{
			QVBoxLayout* layout = new QVBoxLayout(host);
			return layout;
		}

		QLabel* titleLabel(const QString& text, QWidget* parent)
		{
			QLabel* label = new QLabel(text, parent);
			QFont f = label->font();
			f.setBold(true);
			label->setFont(f);
			return label;
		}

		//按钮可收缩化（B02 根治，mAnnotation3DPropPage::makeShrinkable 同款）
		void makeShrinkable(QPushButton* btn)
		{
			btn->setMinimumWidth(1);
			btn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 2px; padding-right: 2px; }"));
		}

		//置灰占位按钮（构造即禁用，不连 commit/信号；mAnnotation3DPropPage
		//::frozenButton 同款——自身样式表兜底置灰观感）
		QPushButton* frozenButton(const QString& text, QWidget* parent)
		{
			QPushButton* btn = new QPushButton(text, parent);
			btn->setEnabled(false);
			btn->setStyleSheet(QStringLiteral(
				"QPushButton { padding-left: 2px; padding-right: 2px; }"
				"QPushButton:disabled { background-color:#F5F7FA; color:#A8ABB2; border-color:#E4E7ED; }"));
			btn->setMinimumWidth(1);
			return btn;
		}

		//一行多按钮容器（mAnnotation3DPropPage::buttonRow 同款：等宽拉伸）
		QWidget* buttonRow(const QVector<QPushButton*>& btns, QWidget* parent)
		{
			QWidget* host = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			for (QPushButton* btn : btns)
				lay->addWidget(btn, 1);
			return host;
		}

		//一行双控件容器（符号行：样式下拉+色块，mCurve3DPropPage::pairRow 同款）
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

	mCurveStyle3DPage::mCurveStyle3DPage(QWidget* parent)
		: QWidget(parent)
	{
		buildUi();

		//上下文维护（全局监听，mAnnotation3DPropPage 先例）：本图表节点及其后代
		//增删/属性变化即重载列表与回显。本页自身提交触发的通知同样进入（回显
		//全程阻塞信号，同值赋值不回环，幂等收敛）；增删通知到达时被删节点已不可
		//查（chartNodeId 失效）：失败按潜在本图表事件接受（refreshAll 幂等，多刷
		//一次无副作用）
		auto onDocChanged = [this](int nodeId)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (_chartId > 0 && !doc->node(_chartId))
			{
				_chartId = -1;   //绑定图表已被删除：清空上下文（列表清空整组禁用）
				refreshAll();
				return;
			}
			if (_chartId <= 0)
				return;
			if (nodeId == _chartId || doc->chartNodeId(nodeId) == _chartId
				|| doc->chartNodeId(nodeId) <= 0)
				refreshAll();
		};
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, onDocChanged);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
			this, onDocChanged);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
			this, onDocChanged);
	}

	void mCurveStyle3DPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);
		layout->addWidget(titleLabel(QStringLiteral("曲线样式"), this));

		//▾ 图形选择与显示（图形类型=类型视图过滤器，同 mDefine3DCurveDlg；
		//列表/按钮行生效）
		_selectFold = new mCurveFoldSection(QStringLiteral("图形选择与显示"), this);
		_plotTypeCombo = new QComboBox(this);
		{
			//序对齐 kTypePlot3d：曲线/曲面/瀑布图/柱状图。阻塞 -1→0 首项信号
			//（handler 触及 _curveList，建序同 mDefine3DCurveDlg：connect 放
			//addItems 之后）
			const QSignalBlocker blocker(_plotTypeCombo);
			_plotTypeCombo->addItems({ QStringLiteral("曲线"), QStringLiteral("曲面"),
				QStringLiteral("瀑布图"), QStringLiteral("柱状图") });
		}
		_selectFold->addRow(QStringLiteral("图形类型"), _plotTypeCombo);
		//类型视图下拉恒可用（纯视图态过滤器，不随选中态/图表启停——
		//mDefine3DCurveDlg 同口径）
		connect(_plotTypeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int) { onTypeChanged(); });

		//曲线列表（生效）：条目=当前图表当前类型视图 Curve 子节点树序，勾选态=
		//series.visible；用户勾选/取消条目=立即提交该条目 kVisible（单命令可
		//撤销，mAnnotation3DPropPage 注释列表同交互）
		_curveList = new QListWidget(this);
		_curveList->setMinimumHeight(110);
		_curveList->setSelectionMode(QAbstractItemView::ExtendedSelection);   //多选同调样式
		_selectFold->addRow(_curveList);
		connect(_curveList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item)
		{
			//条目文本不可编辑，itemChanged 仅勾选态变化可达；回填期间信号已阻塞，
			//_syncing 双保险
			if (!item || _syncing)
				return;
			const int curveId = item->data(Qt::UserRole).toInt();
			if (curveId <= 0)
				return;
			mCurveDocController::getInstance()->setNodeProperty(curveId,
				CurveProp::kVisible, item->checkState() == Qt::Checked);
		});

		//按钮行1（列表选中集操作，纯 UI 态不提交——20261117 勘正：原勾选态
		//操作改选中集，"全部"=一键多选）：全部=全选/取消=清空选中/反选=翻转/
		//已显示=仅选当前 visible 者（模型 visible 回查）
		_allBtn = new QPushButton(QStringLiteral("全部"), this);
		_noneBtn = new QPushButton(QStringLiteral("取消"), this);
		_invertBtn = new QPushButton(QStringLiteral("反选"), this);
		_shownOnlyBtn = new QPushButton(QStringLiteral("已显示"), this);
		for (QPushButton* b : { _allBtn, _noneBtn, _invertBtn, _shownOnlyBtn })
			makeShrinkable(b);
		_selectFold->addRow(buttonRow({ _allBtn, _noneBtn, _invertBtn, _shownOnlyBtn }, this));
		//全部/取消走 selectAll/clearSelection（批量一次 itemSelectionChanged，
		//外观组即时重回显）；反选/已显示逐行翻选中，期间阻塞选中信号、收口
		//一次重回显（纯 UI 态，零提交）
		connect(_allBtn, &QPushButton::clicked, this, [this]() { _curveList->selectAll(); });
		connect(_noneBtn, &QPushButton::clicked, this, [this]() { _curveList->clearSelection(); });
		connect(_invertBtn, &QPushButton::clicked, this, [this]()
		{
			const QSignalBlocker blocker(_curveList);
			for (int i = 0; i < _curveList->count(); ++i)
			{
				QListWidgetItem* item = _curveList->item(i);
				item->setSelected(!item->isSelected());
			}
			refreshAppearance();
		});
		connect(_shownOnlyBtn, &QPushButton::clicked, this, [this]()
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const QSignalBlocker blocker(_curveList);
			for (int i = 0; i < _curveList->count() && i < _curveIds.size(); ++i)
			{
				const CurveNode* c = doc->node(_curveIds.at(i));
				_curveList->item(i)->setSelected(c && c->series.visible);
			}
			refreshAppearance();
		});

		//按钮行2（生效提交钮）：显示/隐藏=把列表勾选集批量提交 kVisible，宏包裹
		//一次 Ctrl+Z 整组撤销；图例显示/图例隐藏=查本图表 Legend 节点提交 kVisible
		_showBtn = new QPushButton(QStringLiteral("显示"), this);
		_hideBtn = new QPushButton(QStringLiteral("隐藏"), this);
		_legendShowBtn = new QPushButton(QStringLiteral("图例显示"), this);
		_legendHideBtn = new QPushButton(QStringLiteral("图例隐藏"), this);
		for (QPushButton* b : { _showBtn, _hideBtn, _legendShowBtn, _legendHideBtn })
			makeShrinkable(b);
		_selectFold->addRow(buttonRow({ _showBtn, _hideBtn, _legendShowBtn, _legendHideBtn }, this));
		connect(_showBtn, &QPushButton::clicked, this, [this]() { commitSelectedVisible(true); });
		connect(_hideBtn, &QPushButton::clicked, this, [this]() { commitSelectedVisible(false); });
		connect(_legendShowBtn, &QPushButton::clicked, this, [this]() { commitLegendVisible(true); });
		connect(_legendHideBtn, &QPushButton::clicked, this, [this]() { commitLegendVisible(false); });

		//外观与着色（9 行：行集/键/提交口径=mCurve3DPropPage R36 翻活后现状，
		//作用于列表选中集全部曲线，回显锚=当前行优先；空选中整组禁用）
		//用户改选（点击/Ctrl·Shift 多选/行1钮）→外观组按回显锚重回显+显示/
		//隐藏钮随选中集启停（refreshAll 路径已覆盖重建，此处补交互路径）
		connect(_curveList, &QListWidget::itemSelectionChanged,
			this, [this]() { refreshAppearance(); });
		_appearFold = new mCurveFoldSection(QStringLiteral("外观与着色"), this);
		//颜色（kColor）：提交后 m3DChartView 增量刷新保视角（既有渲染路径）
		_colorBtn = new mCurveColorButton(Qt::blue, this);
		_appearFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commitSelected(CurveProp::kColor, c);
		});
		//线宽（kLineWidth）：模块既有 1..4px 图标档，提交=档位px（与三维曲线页同口径）
		_lineWidthCombo = new mCurveLineWidthCombo(this);
		_appearFold->addRow(QStringLiteral("线宽"), _lineWidthCombo);
		connect(_lineWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitSelected(CurveProp::kLineWidth, double(idx + 1));
		});
		//线型（kLineStyle）：档位直传 0实/1虚/2点/3点划
		_lineStyleCombo = new mCurvePenStyleCombo(this);
		_appearFold->addRow(QStringLiteral("线型"), _lineStyleCombo);
		connect(_lineStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitSelected(CurveProp::kLineStyle, idx);
		});
		//符号：[样式下拉+色块]双控件容器（kSymbolStyle/kSymbolColor，与三维曲线
		//页同件同键；启停走 setRowEnabled(容器)）
		_symbolStyleCombo = new mCurveSymbolStyleCombo(this);
		_symbolColorBtn = new mCurveColorButton(QColor(204, 0, 0), this);
		_symbolRow = pairRow(_symbolStyleCombo, _symbolColorBtn, this);
		_appearFold->addRow(QStringLiteral("符号"), _symbolRow);
		connect(_symbolStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitSelected(CurveProp::kSymbolStyle, idx);
		});
		connect(_symbolColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commitSelected(CurveProp::kSymbolColor, c);
		});
		//符号间隔（kSymbolDensity 1..50）/符号大小（kSymbolSize 1..100）
		_symbolGapSpin = createCurveSpinBox(1, 50, this);
		_symbolGapSpin->setValue(1);
		_appearFold->addRow(QStringLiteral("符号间隔"), _symbolGapSpin);
		connect(_symbolGapSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commitSelected(CurveProp::kSymbolDensity, v);
		});
		_symbolSizeSpin = createCurveSpinBox(1, 100, this);
		_symbolSizeSpin->setValue(6);
		_appearFold->addRow(QStringLiteral("符号大小"), _symbolSizeSpin);
		connect(_symbolSizeSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commitSelected(CurveProp::kSymbolSize, v);
		});
		//线下区域填充（kAreaFill）+ 填充透明度（kAreaFillOpacity 0..100 滑条）
		_areaFillCheck = new mCurveCheckBox(this);
		_areaFillCheck->setText(QStringLiteral("线下区域填充"));
		_appearFold->addRow(_areaFillCheck);
		connect(_areaFillCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commitSelected(CurveProp::kAreaFill, on);
		});
		_fillOpacitySlider = new QSlider(Qt::Horizontal, this);
		_fillOpacitySlider->setRange(0, 100);
		_fillOpacitySlider->setValue(70);
		_appearFold->addRow(QStringLiteral("填充透明度"), _fillOpacitySlider);
		connect(_fillOpacitySlider, &QSlider::valueChanged, this, [this](int v)
		{
			commitSelected(CurveProp::kAreaFillOpacity, v);
		});
		//云图：[勾选框(kContour)+变量下拉(kContourVar)+"编辑图例..."恒置灰钮]
		//三控件容器（与三维曲线页同款）
		_contourCheck = new mCurveCheckBox(this);
		_contourCheck->setText(QStringLiteral("云图"));
		_contourCombo = new QComboBox(this);
		_contourCombo->addItem(QStringLiteral("x"));
		_contourCombo->addItem(QStringLiteral("y"));
		_contourCombo->addItem(QStringLiteral("z"));
		_contourEditBtn = frozenButton(QStringLiteral("编辑图例..."), this);
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
			commitSelected(CurveProp::kContour, on);
		});
		connect(_contourCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitSelected(CurveProp::kContourVar, idx);
		});

		//色带类四行（R38 第二套行集，同一折叠组内与曲线九行按视图类型互斥
		//可见——切换见 updateAppearanceRows。行序=设计稿：显示模式/颜色/
		//不透明度/云图）
		//1.显示模式：三选项下标=kDrawStyle 值（0 着色曲面/1 等高线/2 着色曲面+
		//等高线，语义=引擎 DrawStyle 同值）。仅曲面视图（==2）生效——connect
		//带类型守卫（mCurve3DPropPage 同款）：占位类型（0 瀑布/3 柱状引擎无
		//样式 API）程序化触发零 commit（置灰不可达是双保险）
		_drawStyleCombo = new QComboBox(this);
		_drawStyleCombo->addItem(QStringLiteral("着色曲面"));
		_drawStyleCombo->addItem(QStringLiteral("等高线"));
		_drawStyleCombo->addItem(QStringLiteral("着色曲面+等高线"));
		_appearFold->addRow(QStringLiteral("显示模式"), _drawStyleCombo);
		connect(_drawStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (viewPlot3dType() == 2)
				commitSelected(CurveProp::kDrawStyle, qBound(0, idx, 2));
		});
		//2.颜色：通栏色条形态（bar 模式），构造即禁用占位；静态色照截图取色
		//备案（瀑布=蓝/曲面=橙/柱状=紫），refreshAppearance 按当前类型 setColor
		//（阻塞信号；置灰态 Qt 照常显色）
		_bandColorBtn = new mCurveColorButton(QColor(85, 116, 189), this);
		_bandColorBtn->setBarMode(true);
		_bandColorBtn->setEnabled(false);
		_appearFold->addRow(QStringLiteral("颜色"), _bandColorBtn);
		//3.不透明度：置灰静态滑条（滑位备案：瀑布 65/曲面 50/柱状 50），不连 commit
		_bandOpacitySlider = new QSlider(Qt::Horizontal, this);
		_bandOpacitySlider->setRange(0, 100);
		_bandOpacitySlider->setValue(50);
		_bandOpacitySlider->setEnabled(false);
		_appearFold->addRow(QStringLiteral("不透明度"), _bandOpacitySlider);
		//4.云图：[勾选框+变量下拉+"编辑图例..."钮]三控件容器全置灰占位
		//（引擎对色带类本就自带色带，云图键下一期再议；钮 tooltip 同 R36）
		_bandContourCheck = new mCurveCheckBox(this);
		_bandContourCheck->setText(QStringLiteral("云图"));
		_bandContourCheck->setEnabled(false);
		_bandContourCombo = new QComboBox(this);
		_bandContourCombo->addItem(QStringLiteral("x"));
		_bandContourCombo->addItem(QStringLiteral("y"));
		_bandContourCombo->addItem(QStringLiteral("z"));
		_bandContourCombo->setEnabled(false);
		_bandContourEditBtn = frozenButton(QStringLiteral("编辑图例..."), this);
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

		//构造默认=曲线视图九行（类型视图下拉首项"曲线"）；色带类四行待
		//refreshAppearance 按视图类型切换（updateAppearanceRows）
		_appearFold->setRowVisible(_drawStyleCombo, false);
		_appearFold->setRowVisible(_bandColorBtn, false);
		_appearFold->setRowVisible(_bandOpacitySlider, false);
		_appearFold->setRowVisible(_bandContourRow, false);

		layout->addWidget(_selectFold);
		layout->addWidget(_appearFold);
		layout->addStretch(1);
	}

	void mCurveStyle3DPage::setChart(int chartId)
	{
		_chartId = chartId;
		refreshAll();
	}

	QList<int> mCurveStyle3DPage::selectedCurveIds() const
	{
		//行序选中集（多选同调/批量显隐的共同目标集）
		QList<int> ids;
		for (int i = 0; i < _curveList->count() && i < _curveIds.size(); ++i)
		{
			if (_curveList->item(i)->isSelected())
				ids.append(_curveIds.at(i));
		}
		return ids;
	}

	int mCurveStyle3DPage::echoCurveId() const
	{
		//外观回显锚：当前行优先（须在选中集内），否则选中集首行；空选中 -1
		const int row = _curveList->currentRow();
		if (row >= 0 && row < _curveIds.size() && _curveList->item(row)->isSelected())
			return _curveIds.at(row);
		for (int i = 0; i < _curveList->count() && i < _curveIds.size(); ++i)
		{
			if (_curveList->item(i)->isSelected())
				return _curveIds.at(i);
		}
		return -1;
	}

	void mCurveStyle3DPage::commitSelected(const QString& prop, const QVariant& value)
	{
		if (_syncing)
			return;
		//多选同调：选中集每条各推一命令，多条宏包裹一次 Ctrl+Z（单条保持原
		//单命令口径零宏包裹）；ids 预取——循环内每次提交触发的通知会同步重建
		//列表（选中集保持），取快照免受中途重建影响
		const QList<int> ids = selectedCurveIds();
		if (ids.isEmpty())
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		if (ids.size() == 1)
		{
			controller->setNodeProperty(ids.front(), prop, value);
			return;
		}
		controller->undoStack()->beginMacro(QStringLiteral("曲线样式-批量修改"));
		for (int id : ids)
			controller->setNodeProperty(id, prop, value);
		controller->undoStack()->endMacro();
	}

	void mCurveStyle3DPage::commitSelectedVisible(bool on)
	{
		//选中集批量提交：宏包裹一次整组撤销（mApplyStyleDlg 先例）；空选中集
		//零操作（不产空宏）
		const QList<int> ids = selectedCurveIds();
		if (ids.isEmpty())
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		controller->undoStack()->beginMacro(on ? QStringLiteral("曲线样式-显示")
			: QStringLiteral("曲线样式-隐藏"));
		for (int id : ids)
			controller->setNodeProperty(id, CurveProp::kVisible, on);
		controller->undoStack()->endMacro();
	}

	void mCurveStyle3DPage::commitLegendVisible(bool on)
	{
		if (_chartId <= 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		for (int childId : doc->childNodeIds(_chartId))
		{
			const CurveNode* c = doc->node(childId);
			if (c && c->type == CurveNodeType::Legend)
			{
				//单命令自带撤销（R32 覆盖层消费 kVisible——图例整框显隐）
				mCurveDocController::getInstance()->setNodeProperty(childId,
					CurveProp::kVisible, on);
				return;
			}
		}
		//本图表无 Legend 节点（异常骨架）：静默返回（防御）
	}

	int mCurveStyle3DPage::viewPlot3dType() const
	{
		//类型视图下拉序 -> plot3dType（kTypePlot3d 固定映射；越界 -1=无视图）
		const int idx = _plotTypeCombo->currentIndex();
		if (idx < 0 || idx > 3)
			return -1;
		return kTypePlot3d[idx];
	}

	void mCurveStyle3DPage::onTypeChanged()
	{
		if (_syncing)
			return;
		//类型下拉=类型视图过滤器（mDefine3DCurveDlg::onTypeChanged 同款）：切换
		//只重建过滤列表，不回写任何模型属性。旧选中集/当前行属旧视图类型，
		//refreshList 恢复必落空→自动选中新视图首条（无条目空选中态，外观组
		//禁用，下拉自身保持可用）
		refreshList();
		refreshAppearance();
	}

	void mCurveStyle3DPage::refreshAll()
	{
		_syncing = true;
		refreshList();
		refreshAppearance();
		_syncing = false;
	}

	void mCurveStyle3DPage::refreshList()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//重建前捕获：选中集（跨刷新保持多选）+ 当前行曲线（回显锚优先恢复）
		QList<int> keepSel;
		const QList<QListWidgetItem*> selItems = _curveList->selectedItems();
		for (QListWidgetItem* it : selItems)
			keepSel.append(it->data(Qt::UserRole).toInt());
		int keepId = -1;
		{
			const int row = _curveList->currentRow();
			if (row >= 0 && row < _curveIds.size())
				keepId = _curveIds.at(row);
		}

		//当前类型视图 Curve 子节点重建（过滤口径=mDefine3DCurveDlg 类型视图：
		//只显 plot3dType==视图类型者，其他类型隐藏不删、切回完整恢复；全程
		//阻塞信号：itemChanged/selectionChanged 在回填期间不得触发提交或外观
		//重载）
		_curveIds.clear();
		const int view = viewPlot3dType();
		if (_chartId > 0)
		{
			for (int childId : doc->childNodeIds(_chartId))
			{
				const CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::Curve && c->series.plot3dType == view)
					_curveIds.append(childId);
			}
		}
		{
			const QSignalBlocker blocker(_curveList);
			_curveList->clear();
			for (int id : _curveIds)
			{
				const CurveNode* c = doc->node(id);
				if (!c)
					continue;
				QListWidgetItem* item = new QListWidgetItem(c->name, _curveList);
				item->setData(Qt::UserRole, id);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				item->setCheckState(c->series.visible ? Qt::Checked : Qt::Unchecked);
			}
			//恢复选中集（重绑/提交回显/撤销等重建后多选不丢）
			for (int i = 0; i < _curveIds.size(); ++i)
			{
				if (keepSel.contains(_curveIds.at(i)))
					_curveList->item(i)->setSelected(true);
			}
			//恢复当前行：原当前行 > 选中集首行 > 首行（无历史保外观组有目标；
			//类型视图切换时新旧集必不交，落空即自动选中新视图首条）
			int restore = (keepId > 0) ? _curveIds.indexOf(keepId) : -1;
			if (restore < 0 && !keepSel.isEmpty())
			{
				for (int i = 0; i < _curveIds.size(); ++i)
				{
					if (keepSel.contains(_curveIds.at(i)))
					{
						restore = i;
						break;
					}
				}
			}
			if (restore < 0 && !_curveIds.isEmpty())
				restore = 0;
			if (restore >= 0)
			{
				_curveList->setCurrentRow(restore);
				if (!_curveList->item(restore)->isSelected())
					_curveList->item(restore)->setSelected(true);
			}
		}

		//按钮启停：选中集操作钮有曲线才可用（显示/隐藏随选中集在
		//refreshAppearance 收口）；图例钮有图表即可用（Legend 查不到由点击
		//路径静默兜底）
		const bool hasChart = _chartId > 0 && doc->node(_chartId) != nullptr;
		const bool hasCurve = !_curveIds.isEmpty();
		_allBtn->setEnabled(hasCurve);
		_noneBtn->setEnabled(hasCurve);
		_invertBtn->setEnabled(hasCurve);
		_shownOnlyBtn->setEnabled(hasCurve);
		_legendShowBtn->setEnabled(hasChart);
		_legendHideBtn->setEnabled(hasChart);
	}

	void mCurveStyle3DPage::refreshAppearance()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int curveId = echoCurveId();
		const CurveNode* n = (curveId > 0) ? doc->node(curveId) : nullptr;
		const bool styleOk = n != nullptr && n->series.plot3dType == 1;
		const bool hasSel = !selectedCurveIds().isEmpty();

		//两套行集按视图类型二分（20261117 勘正三期；类型过滤保证选中集类型
		//恒等于视图类型，行集归属只随视图切换，与选中态无关）
		updateAppearanceRows(viewPlot3dType());

		//回显全程阻塞信号（R30 教训：回显触发提交=毒化模型）。键集/口径=
		//mCurve3DPropPage R36（线宽就近取整钳 1..4、线型/符号钳档、透明度钳
		//0..100、云图变量钳 0..2）
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
			if (n)
			{
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
		}

		//色带类四行静态占位值回显（R38）：显示模式仅曲面回显锚曲线模型值
		//（钳 0..2），占位类型固定单选项"着色曲面"（下标 0）；颜色/滑位照截图
		//备案按类型就位（取色备案：瀑布=蓝 85,116,189/曲面=橙 221,147,96/
		//柱状=紫 145,112,188；滑位备案：瀑布 65/曲面 50/柱状 50）。置灰控件
		//不连 commit，QSignalBlocker 回填防毒化双保险（R30 教训）
		{
			const QSignalBlocker blockerDraw(_drawStyleCombo);
			const QSignalBlocker blockerColor(_bandColorBtn);
			const QSignalBlocker blockerOpacity(_bandOpacitySlider);
			const int t = n ? n->series.plot3dType : viewPlot3dType();
			if (t == 2)
				_drawStyleCombo->setCurrentIndex(qBound(0, n->series.drawStyle, 2));
			else
				_drawStyleCombo->setCurrentIndex(0);
			switch (t)
			{
			case 2: _bandColorBtn->setColor(QColor(221, 147, 96)); break;   //曲面=橙
			case 3: _bandColorBtn->setColor(QColor(145, 112, 188)); break;  //柱状=紫
			default: _bandColorBtn->setColor(QColor(85, 116, 189)); break;  //瀑布=蓝
			}
			_bandOpacitySlider->setValue(t == 0 ? 65 : 50);
		}

		//启停（mCurve3DPropPage::updateStyleRowStates 口径）：未选中整组禁用；
		//选中但 plot3dType!=1（色带类）同样整组置灰防"提交无视觉效果"的假编辑；
		//"编辑图例..."恒置灰占位不随容器恢复翻活（子控件自身 enabled 独立）
		_appearFold->setRowEnabled(_colorBtn, styleOk);
		_appearFold->setRowEnabled(_lineWidthCombo, styleOk);
		_appearFold->setRowEnabled(_lineStyleCombo, styleOk);
		_appearFold->setRowEnabled(_symbolRow, styleOk);
		_appearFold->setRowEnabled(_symbolGapSpin, styleOk);
		_appearFold->setRowEnabled(_symbolSizeSpin, styleOk);
		_appearFold->setRowEnabled(_areaFillCheck, styleOk);
		_appearFold->setRowEnabled(_fillOpacitySlider, styleOk);
		_appearFold->setRowEnabled(_contourRow, styleOk);
		//显示/隐藏随选中集启停（改选即时生效；空选中禁用防"点了没反应"）
		_showBtn->setEnabled(hasSel);
		_hideBtn->setEnabled(hasSel);
	}

	void mCurveStyle3DPage::updateAppearanceRows(int plot3dType)
	{
		//外观与着色两套行集按类型二分（R38 口径镜像 mCurve3DPropPage 同名
		//函数）：曲线类（==1）九行原样（R36 口径零回归），色带类互斥可见——
		//曲面/瀑布四行（显示模式/颜色/不透明度/云图）、柱状仅颜色一行
		//（20261113 勘正口径）——可见性切换走 FoldSection 行摘除/插回
		//（setRowVisible 幂等，改选链路每次重回显重入零副作用）
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
		const bool isBand = !isCurve;
		const bool bandWithModes = (plot3dType == 0 || plot3dType == 2);   //瀑布/曲面四行；柱状仅颜色一行
		_appearFold->setRowVisible(_drawStyleCombo, isBand && bandWithModes);
		_appearFold->setRowVisible(_bandColorBtn, isBand);
		_appearFold->setRowVisible(_bandOpacitySlider, isBand && bandWithModes);
		_appearFold->setRowVisible(_bandContourRow, isBand && bandWithModes);
		//显示模式仅曲面可用——瀑布/柱状引擎无样式 API 置灰防假编辑（曲线视图
		//本行隐藏同步置灰）；曲线九行启停在 refreshAppearance 既有 setRowEnabled
		//块按 styleOk 收口（色带视图行已隐藏，重发幂等收敛）
		_appearFold->setRowEnabled(_drawStyleCombo, plot3dType == 2);
	}
}
