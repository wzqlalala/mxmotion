#include "mAxis3DPropPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveView/m3DChartView.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //createCurveDoubleSpin/createCurveSpinBox/mCurveCheckBox/mCurveColorButton 行工厂

//QT
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QUndoStack>

namespace PostCurve
{
	namespace
	{
		//「应用到」档位（R43）：会话级 UI 态**不进模型不序列化**——文件局部
		//静态，三实例（参数栏按壳类型各缓存一页，同时存活）共享同一作用范围。
		//0=所选坐标轴（单轴提交）1=全部轴（"范围与刻度类"五键宏提交扇出三壳）
		int g_applyScope = 0;
		//页面公共骨架：纵向堆叠 FoldSection 的外层布局（同 mCurvePropPages.cpp
		//行工厂口径：边距/间距用布局默认值，与前处理参数面板同一骨架规格）
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

		//--- T7b 置灰占位行工厂（20261111 二次拍板"全填入+占位不实现"；对齐
		//"极坐标图置灰占位"先例：构造即禁用固定、不连 commit/信号、不进回填，
		//静态值一次性构造，后续迭代接活 Plot3D API 时再翻活） ---

		//置灰占位下拉（单选项固定文案）
		QComboBox* frozenCombo(std::initializer_list<QString> items, QWidget* parent)
		{
			QComboBox* combo = new QComboBox(parent);
			for (const QString& item : items)
				combo->addItem(item);
			combo->setEnabled(false);
			return combo;
		}

		//置灰占位数字框
		QSpinBox* frozenSpin(int value, int min, int max, QWidget* parent)
		{
			QSpinBox* spin = createCurveSpinBox(min, max, parent);
			spin->setValue(value);
			spin->setEnabled(false);
			return spin;
		}

		//置灰占位浮点数字框（4 位小数，同生效行规格）
		QDoubleSpinBox* frozenDoubleSpin(double value, double min, double max, QWidget* parent)
		{
			QDoubleSpinBox* spin = createCurveDoubleSpin(4, min, max, 1.0, parent);
			spin->setValue(value);
			spin->setEnabled(false);
			return spin;
		}

		//置灰占位勾选框
		QCheckBox* frozenRowCheck(const QString& text, bool checked, QWidget* parent)
		{
			QCheckBox* check = createRowCheck(text, parent);
			check->setChecked(checked);
			check->setEnabled(false);
			return check;
		}

		//置灰占位色块按钮（构造即带展示色，禁用后取色交互不触发+不连信号双保险）
		mCurveColorButton* frozenColorButton(const QColor& color, QWidget* parent)
		{
			mCurveColorButton* btn = new mCurveColorButton(color, parent);
			btn->setEnabled(false);
			return btn;
		}

		//一行双控件容器（参考截图并排形态；边距归零不改 FoldSection 行高，
		//标签列宽与既有行对齐——容器行走 addRow(容器) 单参重载，左标签列为空）
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

	mAxis3DPropPage::mAxis3DPropPage(int axisIndex, QWidget* parent)
		: mCurvePropPageBase(parent)
		, _axisIndex(qBound(0, axisIndex, 2))
	{
		buildUi();

		//同图表内曲线数据直写/增删会改变并集范围：autoFit 勾选时置灰回显的
		//"当前生效范围"随之变化。基类只监听本节点（轴壳）——这里补全局监听
		//（仿 mAxisPropPage 构造先例），命中"本壳所属图表或其后代"时重载回填；
		//本节点自身变化基类已回填，勿重入（loadFromModel 只读，无回环风险）
		auto reloadForUnion = [this](int nodeId)
		{
			if (_nodeId <= 0 || nodeId == _nodeId)   //本节点变化基类已回填
				return;
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const int chartId = doc->chartNodeId(_nodeId);
			if (chartId <= 0)
				return;
			//增删通知到达时被删节点已不可查（chartNodeId 失效）：失败按
			//潜在同图表事件接受（loadFromModel 幂等，多刷一次无副作用）
			const int changedChart = doc->chartNodeId(nodeId);
			if (changedChart == chartId || changedChart <= 0)
			{
				_loading = true;
				loadFromModel();
				_loading = false;
			}
		};
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, reloadForUnion);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
			this, reloadForUnion);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
			this, reloadForUnion);
	}

	void mAxis3DPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//▾ 坐标轴与范围（原"标准"+"缩放设置"并组，组名/行序=截图；生效行=
		//坐标轴/应用到/轴名称/刻度类型/最小值/最大值/自动适配，其余置灰占位）
		_axisRangeFold = new mCurveFoldSection(QStringLiteral("坐标轴与范围"), this);
		//坐标轴（R43 翻活）：三选项 X轴/Y轴/Z轴。用户切换=按本曲线找目标类型
		//兄弟壳命中则 setCurrentNodeId（R37 坐标轴按钮 mPostMenuContributor
		//同款链路——树选中/参数栏换页/工具页联动免费获得）；回填期静默
		//（_loading 守卫），找不到目标壳（异常骨架）静默返回，combo 由提交
		//回环 loadFromModel 回正
		_axisCombo = new QComboBox(this);
		_axisCombo->addItems({ QStringLiteral("X轴"), QStringLiteral("Y轴"),
			QStringLiteral("Z轴") });
		_axisRangeFold->addRow(QStringLiteral("坐标轴"), _axisCombo);
		connect(_axisCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (_loading || _nodeId < 0 || idx < 0 || idx > 2)
				return;
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const int curveId = doc->parentNodeId(_nodeId);
			const CurveNodeType want[3] = { CurveNodeType::AxisX3DData,
				CurveNodeType::AxisY3DData, CurveNodeType::AxisZ3DData };
			if (curveId > 0)
			{
				for (int id : doc->childNodeIds(curveId))
				{
					if (doc->nodeType(id) == want[idx])
					{
						mCurveDocController::getInstance()->setCurrentNodeId(id);
						return;
					}
				}
			}
		});
		//应用到（R43 翻活）：所选坐标轴/全部轴。会话级 UI 态不进模型——切换
		//只改共享静态档，等下次范围/刻度编辑生效；不连 commit
		_applyCombo = new QComboBox(this);
		_applyCombo->addItems({ QStringLiteral("所选坐标轴"), QStringLiteral("全部轴") });
		_axisRangeFold->addRow(QStringLiteral("应用到"), _applyCombo);
		connect(_applyCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (idx >= 0)
				g_applyScope = qBound(0, idx, 1);
		});
		//轴名称（kAxis3DLabel，允许空=渲染 setLabels 空串不显示标题；恒单轴
		//不扇出——用户拍板"轴名称不扇出"）
		_labelEdit = new QLineEdit(this);
		_axisRangeFold->addRow(QStringLiteral("轴名称"), _labelEdit);
		connect(_labelEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kAxis3DLabel, _labelEdit->text());
		});
		//刻度类型（R43 翻活）：四项文案与顺序=二维轴页"缩放方式"同款
		//（mCurvePropPages 同款英文文案，用户拍板）；提交 kAxis3DScaleType
		//0..3 经 commitScoped（全部轴档扇出三壳宏提交）
		_tickTypeCombo = new QComboBox(this);
		_tickTypeCombo->addItems({ QStringLiteral("Linear"), QStringLiteral("Logarithmic"),
			QStringLiteral("dB20"), QStringLiteral("dB10") });
		_axisRangeFold->addRow(QStringLiteral("刻度类型"), _tickTypeCombo);
		connect(_tickTypeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (idx >= 0)
				commitScoped(CurveProp::kAxis3DScaleType, idx);
		});
		//最小值/最大值：一行双控件容器（生效，kAxis3DMin/kAxis3DMax；互钳与
		//autoFit 置灰联动现状保留；行内自带控件文案，左标签列为空）
		{
			QWidget* host = new QWidget(this);
			QHBoxLayout* lay = new QHBoxLayout(host);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(MxFunctions::UiMetrics::rowGap());
			lay->addWidget(new QLabel(QStringLiteral("最小值"), host));
			_minSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, this);
			lay->addWidget(_minSpin, 1);
			lay->addWidget(new QLabel(QStringLiteral("/"), host));
			lay->addWidget(new QLabel(QStringLiteral("最大值"), host));
			_maxSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, this);
			lay->addWidget(_maxSpin, 1);
			_axisRangeFold->addRow(host);
		}
		connect(_minSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			_maxSpin->setMinimum(v);   //最大值不小于最小值（本页控件局部行为——扇出
			                           //值直接提交，各轴页面经提交回环回填自紧）
			commitScoped(CurveProp::kAxis3DMin, v);
		});
		connect(_maxSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			_minSpin->setMaximum(v);   //最小值不大于最大值（同上，仅本页控件局部）
			commitScoped(CurveProp::kAxis3DMax, v);
		});
		//自动适配（生效，勾选=范围按全部曲线数据并集现算，追加曲线自动扩，
		//最小/最大值置灰回显现值；取消=手动范围，回填当前生效范围保证视图
		//无跳变）/ 适配范围（置灰占位，不勾；一行双勾选容器）
		{
			_autoFitCheck = createRowCheck(QStringLiteral("自动适配"), this);
			_fitRangeCheck = frozenRowCheck(QStringLiteral("适配范围"), false, this);
			_axisRangeFold->addRow(pairRow(_autoFitCheck, _fitRangeCheck, this));
		}
		connect(_autoFitCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			if (on)
			{
				//勾选：只提交开关（渲染重建即按并集），置灰回显新并集由提交
				//回环的 loadFromModel 完成
				commitScoped(CurveProp::kAxis3DAutoFit, true);
				return;
			}
			commitManualRange();
		});

		//▾ 刻度与文字（组名/行序=截图；生效行=主刻度数）
		_tickTextFold = new mCurveFoldSection(QStringLiteral("刻度与文字"), this);
		//刻度方式：置灰占位
		_scaleModeCombo = frozenCombo({ QStringLiteral("每轴刻度数") }, this);
		_tickTextFold->addRow(QStringLiteral("刻度方式"), _scaleModeCombo);
		_tickTextFold->setRowEnabled(_scaleModeCombo, false);
		//主刻度数（kAxis3DTickCount，下限 2 与渲染端 qMax(2,值) 同口径）
		_tickCountSpin = createCurveSpinBox(2, 100, this);
		_tickTextFold->addRow(QStringLiteral("主刻度数"), _tickCountSpin);
		connect(_tickCountSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commitScoped(CurveProp::kAxis3DTickCount, v);
		});
		//刻度格式：置灰占位一行双控件（"自动格式"+位数 3）
		{
			_fmtCombo = frozenCombo({ QStringLiteral("自动格式") }, this);
			_fmtSpin = frozenSpin(3, 0, 15, this);
			_tickTextFold->addRow(pairRow(_fmtCombo, _fmtSpin, this));
		}
		//轴颜色：置灰占位（展示色取截图蓝条观感深蓝；Plot3D 无轴颜色 API）
		_axisColorBtn = frozenColorButton(QColor(31, 78, 121), this);
		_tickTextFold->addRow(QStringLiteral("轴颜色"), _axisColorBtn);
		_tickTextFold->setRowEnabled(_axisColorBtn, false);
		//每刻度网格数：置灰占位
		_minorPerTickSpin = frozenSpin(2, 0, 100, this);
		_tickTextFold->addRow(QStringLiteral("每刻度网格数"), _minorPerTickSpin);
		_tickTextFold->setRowEnabled(_minorPerTickSpin, false);
		//字体大小：置灰占位
		_fontSpin = frozenSpin(12, 6, 72, this);
		_tickTextFold->addRow(QStringLiteral("字体大小"), _fontSpin);
		_tickTextFold->setRowEnabled(_fontSpin, false);

		//▾ 网格与外观（组名/行序=截图；生效行=网格样式/网格线颜色/背景颜色
		//（R44 翻活，全图表级经 commitToChart）+显示网格，自原"坐标轴刻度设置"
		//组挪入本组）
		_gridAppearFold = new mCurveFoldSection(QStringLiteral("网格与外观"), this);
		//网格样式（R44 翻活）：三维框/线框两选项（对齐 HyperGraph Grid Style
		//Box/Wireframe）。图表级 kChart3DGridStyle 经 commitToChart（提交目标=
		//所属图表节点，不随「应用到」扇出）；线框=宿主侧把三轴有效旗标置 false
		//复用引擎 setGridVisible"关=该轴面填充与网格都不画"语义（引擎零新 API）
		_gridStyleCombo = new QComboBox(this);
		_gridStyleCombo->addItems({ QStringLiteral("三维框"), QStringLiteral("线框") });
		_gridAppearFold->addRow(QStringLiteral("网格样式"), _gridStyleCombo);
		connect(_gridStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			if (idx >= 0)
				commitToChart(CurveProp::kChart3DGridStyle, idx);
		});
		//网格线颜色（R44 翻活）：bar 通栏色条形态（R34 先例=截图形态）；图表级
		//kChart3DBackdropGridColor。线框期保持可用（休眠状态——值继续提交入模，
		//切回三维框恢复消费，不做置灰）
		_gridColorBtn = new mCurveColorButton(QColor(191, 191, 191), this);
		_gridColorBtn->setBarMode(true);
		_gridAppearFold->addRow(QStringLiteral("网格线颜色"), _gridColorBtn);
		connect(_gridColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commitToChart(CurveProp::kChart3DBackdropGridColor, c);
		});
		//背景颜色（R44 翻活）：口径同网格线颜色，图表级 kChart3DBackdropColor
		//（驱动引擎 Axis3D::setBackdropFillColor）
		_bgColorBtn = new mCurveColorButton(QColor(240, 240, 240), this);
		_bgColorBtn->setBarMode(true);
		_gridAppearFold->addRow(QStringLiteral("背景颜色"), _bgColorBtn);
		connect(_bgColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commitToChart(CurveProp::kChart3DBackdropColor, c);
		});
		//显示网格（kAxis3DGridVisible，本壳对应轴的网格显隐；恒单轴不扇出。
		//线框模式下整行置灰由 loadFromModel 按 gridStyle 联动——防假编辑）
		_gridCheck = createRowCheck(QStringLiteral("显示网格"), this);
		_gridAppearFold->addRow(_gridCheck);
		connect(_gridCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAxis3DGridVisible, on);
		});

		//▾ 坐标轴长度（图表级占位组，三壳页同构恒显三行，不绑定任何键；
		//Plot3D 无坐标轴长度 API）
		_lenFold = new mCurveFoldSection(QStringLiteral("坐标轴长度"), this);
		//手动轴长度：置灰占位（不勾）
		_manualLenCheck = frozenRowCheck(QStringLiteral("手动轴长度"), false, this);
		_lenFold->addRow(_manualLenCheck);
		//X/Y/Z 轴长度：三行置灰占位（无论本页是哪根轴，截图三行恒显）
		_lenXSpin = frozenDoubleSpin(1.0, 0.0, 10.0, this);
		_lenFold->addRow(QStringLiteral("X轴长度"), _lenXSpin);
		_lenFold->setRowEnabled(_lenXSpin, false);
		_lenYSpin = frozenDoubleSpin(1.0, 0.0, 10.0, this);
		_lenFold->addRow(QStringLiteral("Y轴长度"), _lenYSpin);
		_lenFold->setRowEnabled(_lenYSpin, false);
		_lenZSpin = frozenDoubleSpin(1.0, 0.0, 10.0, this);
		_lenFold->addRow(QStringLiteral("Z轴长度"), _lenZSpin);
		_lenFold->setRowEnabled(_lenZSpin, false);

		layout->addWidget(_axisRangeFold);
		layout->addWidget(_tickTextFold);
		layout->addWidget(_gridAppearFold);
		layout->addWidget(_lenFold);
		layout->addStretch(1);
	}

	QList<int> mAxis3DPropPage::siblingShellIds() const
	{
		//本壳所属曲线的三兄弟壳（X/Y/Z 类型序收集；某壳缺失降级为可用壳集合，
		//本壳自身恒在列保底——异常骨架不丢当前编辑）
		QList<int> ids;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int curveId = (_nodeId > 0 && doc) ? doc->parentNodeId(_nodeId) : -1;
		const CurveNodeType want[3] = { CurveNodeType::AxisX3DData,
			CurveNodeType::AxisY3DData, CurveNodeType::AxisZ3DData };
		if (curveId > 0)
		{
			const QList<int> children = doc->childNodeIds(curveId);
			for (int k = 0; k < 3; ++k)
			{
				for (int id : children)
				{
					if (doc->nodeType(id) == want[k])
					{
						ids.append(id);
						break;
					}
				}
			}
		}
		if (!ids.contains(_nodeId))
			ids.append(_nodeId);   //本壳不在集合（查不到父曲线/骨架异常）：保底
		return ids;
	}

	void mAxis3DPropPage::commitScoped(const QString& prop, const QVariant& value)
	{
		//守卫与基类 commit 一致（回填期/未绑定静默丢弃）
		if (_loading || _nodeId < 0)
			return;
		//生效键范围（R43 用户拍板：仅"范围与刻度类"五键扇出）——kAxis3DLabel/
		//kAxis3DGridVisible 恒单轴（轴名称/显示网格不扇出）
		const bool scoped = prop == CurveProp::kAxis3DScaleType
			|| prop == CurveProp::kAxis3DMin
			|| prop == CurveProp::kAxis3DMax
			|| prop == CurveProp::kAxis3DAutoFit
			|| prop == CurveProp::kAxis3DTickCount;
		if (g_applyScope != 1 || !scoped)
		{
			commit(prop, value);   //所选坐标轴（或恒单轴键）：基类原样
			return;
		}
		//全部轴扇出：宏"修改三维轴"包整组，一次 Ctrl+Z 整体还原（beginMacro/
		//endMacro=控制器既有宏 API 创建链先例）
		mCurveDocController* ctrl = mCurveDocController::getInstance();
		QUndoStack* stack = ctrl->undoStack();
		stack->beginMacro(QStringLiteral("修改三维轴"));
		const QList<int> shells = siblingShellIds();
		for (int shellId : shells)
			ctrl->setNodeProperty(shellId, prop, value);
		stack->endMacro();
	}

	void mAxis3DPropPage::commitToChart(const QString& prop, const QVariant& value)
	{
		//守卫与基类 commit 一致（回填期/未绑定静默丢弃）
		if (_loading || _nodeId < 0)
			return;
		//提交目标=本壳所属图表节点（R44 三行全图表级：任意轴壳页编辑全图生效；
		//chartNodeId 对 Chart 节点自身返回自身，图表缺/骨架异常静默返回）
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int chartId = (_nodeId > 0 && doc) ? doc->chartNodeId(_nodeId) : -1;
		if (chartId <= 0)
			return;
		mCurveDocController::getInstance()->setNodeProperty(chartId, prop, value);
	}

	void mAxis3DPropPage::commitManualRange()
	{
		//守卫与基类 commit 一致（仅用户取消勾选触达，防御性兜底）
		if (_loading || _nodeId < 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int chartId = (_nodeId > 0 && doc) ? doc->chartNodeId(_nodeId) : -1;
		double lo[3] = { 0.0, 0.0, 0.0 };
		double hi[3] = { 0.0, 0.0, 0.0 };
		const bool ok = chartId > 0
			&& m3DChartView::queryAxis3DDisplayRange(chartId, lo, hi);
		if (g_applyScope != 1)
		{
			//所选坐标轴：现状逻辑——查询成功回写本轴现值（min/max 先落、
			//autoFit=false 收尾，每次 commit 各成命令与 2D 页同款逐键可撤）；
			//查询失败只退出自动，spin 回填模型休眠值（提交回环 loadFromModel）
			if (ok)
			{
				commit(CurveProp::kAxis3DMin, lo[_axisIndex]);
				commit(CurveProp::kAxis3DMax, hi[_axisIndex]);
			}
			commit(CurveProp::kAxis3DAutoFit, false);
			return;
		}
		//全部轴（R43）：宏内逐轴提交**各自的**当前生效范围（lo/hi 按轴下标
		//写回对应壳）+ autoFit=false，一次撤销整组还原；查询失败=只提交三轴
		//autoFit=false（各轴回退休眠值）
		mCurveDocController* ctrl = mCurveDocController::getInstance();
		QUndoStack* stack = ctrl->undoStack();
		stack->beginMacro(QStringLiteral("修改三维轴"));
		const QList<int> shells = siblingShellIds();
		for (int shellId : shells)
		{
			if (ok)
			{
				const int axis = (doc->nodeType(shellId) == CurveNodeType::AxisX3DData) ? 0
					: (doc->nodeType(shellId) == CurveNodeType::AxisY3DData) ? 1 : 2;
				ctrl->setNodeProperty(shellId, CurveProp::kAxis3DMin, lo[axis]);
				ctrl->setNodeProperty(shellId, CurveProp::kAxis3DMax, hi[axis]);
			}
			ctrl->setNodeProperty(shellId, CurveProp::kAxis3DAutoFit, false);
		}
		stack->endMacro();
	}

	void mAxis3DPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;

		//坐标轴下拉（R43 翻活）：阻塞信号回显壳身份下标（构造期一次填充三
		//选项，不再逐次 clear/addItem；回显永不触发切换——R30 教训）
		{
			const QSignalBlocker blokAxis(_axisCombo);
			_axisCombo->setCurrentIndex(_axisIndex);
		}
		//应用到（R43）：阻塞信号回显会话静态档（UI 态不进模型，跨三实例共享）
		{
			const QSignalBlocker blokApply(_applyCombo);
			_applyCombo->setCurrentIndex(qBound(0, g_applyScope, 1));
		}
		_labelEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kAxis3DLabel).toString());
		//刻度类型（R43）：阻塞信号回显模型值（钳 0..3 防越界档位；qBound 与
		//序列化读侧同口径）
		{
			const QSignalBlocker blokTickType(_tickTypeCombo);
			_tickTypeCombo->setCurrentIndex(qBound(0,
				doc->nodeProperty(_nodeId, CurveProp::kAxis3DScaleType).toInt(), 3));
		}

		//全程阻塞信号回显（R30 教训：回显触发提交=毒化模型）。autoFit 勾选时
		//模型 min/max 是休眠值，回显当前生效范围（=数据并集现算；查询失败
		//回退休眠值）；取消时回显休眠值本身。置灰口径随勾选态。
		//置灰占位控件（适配范围/刻度格式/轴颜色/每刻度网格数/字体大小/手动轴
		//长度/X·Y·Z轴长度）不在回填路径——构造时静态值一次成型，永不回写
		//（R43 翻活的坐标轴/应用到/刻度类型三行与 R44 翻活的网格样式/网格线
		//颜色/背景颜色三行已进回填，见上方各阻塞段）
		const bool autoFit = doc->nodeProperty(_nodeId, CurveProp::kAxis3DAutoFit).toBool();
		double rangeMin = doc->nodeProperty(_nodeId, CurveProp::kAxis3DMin).toDouble();
		double rangeMax = doc->nodeProperty(_nodeId, CurveProp::kAxis3DMax).toDouble();
		if (autoFit)
		{
			const int chartId = doc->chartNodeId(_nodeId);
			double lo[3] = { 0.0, 0.0, 0.0 };
			double hi[3] = { 0.0, 0.0, 0.0 };
			if (chartId > 0
				&& m3DChartView::queryAxis3DDisplayRange(chartId, lo, hi))
			{
				rangeMin = lo[_axisIndex];
				rangeMax = hi[_axisIndex];
			}
		}
		{
			const QSignalBlocker blokAuto(_autoFitCheck);
			const QSignalBlocker blokMin(_minSpin);
			const QSignalBlocker blokMax(_maxSpin);
			_autoFitCheck->setChecked(autoFit);
			//先放开最小≤最大互钳约束再回填（陈旧约束会把新值钳歪），回填后
			//按新值收紧
			_minSpin->setMaximum(1e9);
			_maxSpin->setMinimum(-1e9);
			_minSpin->setValue(rangeMin);
			_maxSpin->setValue(rangeMax);
			_minSpin->setMaximum(rangeMax);
			_maxSpin->setMinimum(rangeMin);
			_minSpin->setEnabled(!autoFit);
			_maxSpin->setEnabled(!autoFit);
		}
		{
			const QSignalBlocker blokTick(_tickCountSpin);
			_tickCountSpin->setValue(qBound(2,
				doc->nodeProperty(_nodeId, CurveProp::kAxis3DTickCount).toInt(), 100));
		}
		//网格样式/两色（R44 翻活）：阻塞信号回显图表级模型值（本壳所属图表；
		//读不到图表=骨架异常，保持控件构造缺省=模型出厂缺省同值）。样式钳 0..1
		//与序列化读侧同口径；两色无效值回退出厂缺省（引擎 setter 兜底同口径）
		int gridStyle = 0;
		QColor bdColor(240, 240, 240);
		QColor bdGridColor(191, 191, 191);
		const int styleChartId = doc->chartNodeId(_nodeId);
		if (styleChartId > 0 && doc->node(styleChartId))
		{
			gridStyle = doc->nodeProperty(styleChartId, CurveProp::kChart3DGridStyle).toInt();
			const QVariant bgVar = doc->nodeProperty(styleChartId, CurveProp::kChart3DBackdropColor);
			const QVariant gridVar = doc->nodeProperty(styleChartId, CurveProp::kChart3DBackdropGridColor);
			if (bgVar.isValid())
				bdColor = bgVar.value<QColor>();
			if (gridVar.isValid())
				bdGridColor = gridVar.value<QColor>();
		}
		if (!bdColor.isValid())
			bdColor = QColor(240, 240, 240);
		if (!bdGridColor.isValid())
			bdGridColor = QColor(191, 191, 191);
		{
			const QSignalBlocker blokStyle(_gridStyleCombo);
			_gridStyleCombo->setCurrentIndex(qBound(0, gridStyle, 1));
		}
		{
			const QSignalBlocker blokGridC(_gridColorBtn);
			_gridColorBtn->setColor(bdGridColor);
		}
		{
			const QSignalBlocker blokBgC(_bgColorBtn);
			_bgColorBtn->setColor(bdColor);
		}
		{
			const QSignalBlocker blokGrid(_gridCheck);
			_gridCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAxis3DGridVisible).toBool());
		}
		//线框联动（R44）：gridStyle==1 置灰「显示网格」整行（R33 防假编辑先例——
		//线框语义压住引擎旗标，编辑无视觉效果）；三维框恢复可用。勾选态上面
		//照常回显模型 kAxis3DGridVisible（线框期休眠值，切回三维框按各轴模型值
		//生效）
		_gridAppearFold->setRowEnabled(_gridCheck, gridStyle != 1);
	}
}
