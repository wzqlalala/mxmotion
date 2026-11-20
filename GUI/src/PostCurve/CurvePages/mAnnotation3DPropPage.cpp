#include "mAnnotation3DPropPage.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"   //mCurveColorButton/mCurveFontButton/mCurveLineWidthCombo/mCurveCheckBox 行工厂

//QT
#include <QVBoxLayout>
#include <QSignalBlocker>

namespace PostCurve
{
	namespace
	{
		//页面公共骨架：纵向堆叠 FoldSection 的外层布局（同 mCurve3DPropPage/
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

		//按钮可收缩化（B02 根治）：显式最小宽≈1（覆盖 minimumSizeHint，QLayout
		//即允许压缩到任意宽度）+ 紧凑横向内边距（行高不缩）——按钮随面板宽度
		//实时收缩，文字极端窄时截断（tooltip 兜底），符合"控件随大小变化而变化"
		//用户拍板；不改全局 QSS，仅按钮自身样式表（优先级最高）
		void makeShrinkable(QPushButton* btn)
		{
			btn->setMinimumWidth(1);
			btn->setStyleSheet(QStringLiteral("QPushButton { padding-left: 2px; padding-right: 2px; }"));
		}

		//置灰占位按钮（构造即禁用，不连 commit/信号、不进回填；R31/R33 同款
		//纪律+makeShrinkable 可收缩）。FoldSection 的 :disabled 规则只覆盖
		//QComboBox/QLineEdit/QSpinBox/QDoubleSpinBox，QPushButton 以自身样式表
		//兜底置灰观感（自身样式表优先级最高，宿主 QWidget 级规则压不过）。
		//R46 后仅 {U}{V}{W} 三模板钮使用（显示注释/剪切/复制/粘贴/新增/显示边框/
		//其余四模板钮均已翻活——其中前四者随注释管理组 R46 整组退役）
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

		//一行多按钮容器（截图按钮排形态；边距归零不改 FoldSection 行高，
		//容器行走 addRow(容器) 单参重载，左标签列为空——mAxis3DPropPage::
		//pairRow 同款，等宽拉伸）
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
	}

	mAnnotation3DPropPage::mAnnotation3DPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mAnnotation3DPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//▾ 文字（3 行：生效=文本编辑区/对齐，占位=模板插入七按钮；B02 后模板钮
		//回归一行七钮——推翻 B01 两行方案，用户拍板"一行七钮最贴设计稿"）
		_textFold = new mCurveFoldSection(QStringLiteral("文字"), this);
		//文本编辑区（kText，生效）：提交时机=失焦且文本有变化才 commit 一次
		//（防逐键压撤回栈；FocusOut 经事件过滤器抓，见 eventFilter）
		_textEdit = new QPlainTextEdit(this);
		_textEdit->setMinimumHeight(110);
		_textEdit->installEventFilter(this);   //失焦提交
		_textFold->addRow(_textEdit);
		//(X)(Y)(Z)(U)(V)(W)(Label)：七钮一行（B02 回归一行七钮——用户拍板"一行
		//七钮最贴设计稿"）。R42 二期翻活口径：{X}{Y}{Z}{Label} 四钮生效——点击=
		//向 _textEdit 光标处插入 token 文本（QPlainTextEdit::insertPlainText，
		//不直接 commit，维持"失焦且有变化才提交"口径）；{U}{V}{W} 维持置灰占位
		//（3D 模板不提供 U/V/W 求值，字面保留无编辑价值）。生效钮 makeShrinkable
		//可收缩，占位钮 frozenButton 自带最小宽；tooltip=将插入的完整 token
		//（窄面板截断兜底；占位钮 tooltip=自身 token 文本与一期口径一致）
		const struct TmplBtnSpec { const char* label; const char* insert; bool active; }
		tmplTokens[7] = {
			{ "(X)",     "{X}",     true },
			{ "(Y)",     "{Y}",     true },
			{ "(Z)",     "{Z}",     true },
			{ "(U)",     nullptr,   false },
			{ "(V)",     nullptr,   false },
			{ "(W)",     nullptr,   false },
			{ "(Label)", "{Label}", true },
		};
		for (const TmplBtnSpec& spec : tmplTokens)
		{
			QPushButton* btn = spec.active ? new QPushButton(spec.label, this)
				: frozenButton(spec.label, this);
			if (spec.active)
			{
				makeShrinkable(btn);
				const QString token = QString::fromUtf8(spec.insert);
				btn->setToolTip(token);
				connect(btn, &QPushButton::clicked, this, [this, token]()
				{
					//光标处插入 token 文本（不直接 commit——维持失焦提交口径，
					//失焦经 eventFilter 与模型现值比对"无变化不发"）
					_textEdit->insertPlainText(token);
				});
			}
			else
			{
				btn->setToolTip(QString::fromUtf8(spec.label));   //截断态可读完整 token
			}
			_tmplBtns.append(btn);
		}
		_textFold->addRow(buttonRow(_tmplBtns, this));
		//对齐（kAlign，生效：0左 1中 2右，注释侧语义——与 2D 注释页提交口径一致）
		_alignCombo = new QComboBox(this);
		_alignCombo->addItems({ QStringLiteral("左对齐"), QStringLiteral("居中"), QStringLiteral("右对齐") });
		_textFold->addRow(QStringLiteral("对齐"), _alignCombo);
		connect(_alignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kAlign, idx);
		});

		//▾ 属性（5 行：全部生效=文字颜色/字体设置/背景颜色/边框线宽度/显示边框；
		//组名/行序=截图（字体/字号/粗体/斜体按用户拍板合并为"字体设置"一行））
		_propFold = new mCurveFoldSection(QStringLiteral("属性"), this);
		//显示边框（R42 二期翻活，kBorderShow 仅 Annotation 节点放行）：回显=模型
		//现值（loadFromModel QSignalBlocker），勾选变更 commit 走命令栈可撤销；
		//画布消费见 m3DChartAnnotationItem（kBorderShow 且 kBorderWidth>0 画框）
		_borderCheck = createRowCheck(QStringLiteral("显示边框"), this);
		_propFold->addRow(_borderCheck);
		connect(_borderCheck, &QCheckBox::toggled, this, [this](bool checked)
		{
			commit(CurveProp::kBorderShow, checked);
		});
		//文字颜色（kColor，生效）：B02 改通栏色条（mCurveColorButton bar 模式，
		//宽度随表单拉伸整块填充，点击弹同款取色对话框）——不设"默认/自定义"
		//组合（无效色恢复入口随渲染下一期）
		_textColorBtn = new mCurveColorButton(QColor(51, 51, 51), this);
		_textColorBtn->setBarMode(true);
		_propFold->addRow(QStringLiteral("文字颜色"), _textColorBtn);
		connect(_textColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kColor, c);
		});
		//字体设置（kFont，生效）：mCurveFontButton 点击弹 2D 同款 QFontDialog
		//"选择字体"（族/字号/粗体/斜体一并设置，用户拍板不单列四行；B02 二次
		//拍板维持字体弹窗一行不改）
		_fontBtn = new mCurveFontButton(this);
		_propFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFont, f);
		});
		//背景颜色（kBgColor，生效）：同文字颜色口径（B02 通栏色条）
		_bgColorBtn = new mCurveColorButton(Qt::white, this);
		_bgColorBtn->setBarMode(true);
		_propFold->addRow(QStringLiteral("背景颜色"), _bgColorBtn);
		connect(_bgColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBgColor, c);
		});
		//边框线宽度（kBorderWidth，生效）：提交=档位px（图标行 0..3 -> 1..4px），
		//回显就近取整钳 1..4，与 2D 注释页同映射（CurveReadMe §5.3）
		_borderWidthCombo = new mCurveLineWidthCombo(this);
		_propFold->addRow(QStringLiteral("边框线宽度"), _borderWidthCombo);
		connect(_borderWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kBorderWidth, double(idx + 1));
		});

		//▾ 附着对象（1 行：附着方式置灰回显"三维"——R42 修订起 3D 注释语义=三维
		//数据空间锚点（锚点随相机投影移动）；R46 起模型档位 0=内容锚定（注释
		//模式画布打点创建，画布渲染=牵引线+框）/1=窗口（旧直接创建入口存量，
		//框左上角即锚点）——两档位均锚定三维、无第二附着语义，可交互切换无
		//意义，按占位行纪律不连 commit/不进回填）
		_attachFold = new mCurveFoldSection(QStringLiteral("附着对象"), this);
		_attachCombo = new QComboBox(this);
		_attachCombo->addItem(QStringLiteral("三维"));
		_attachCombo->setEnabled(false);
		_attachFold->addRow(QStringLiteral("附着方式"), _attachCombo);
		_attachFold->setRowEnabled(_attachCombo, false);   //行标签一并置灰

		layout->addWidget(_textFold);
		layout->addWidget(_propFold);
		layout->addWidget(_attachFold);
		layout->addStretch(1);
	}

	bool mAnnotation3DPropPage::eventFilter(QObject* watched, QEvent* event)
	{
		//文本编辑区提交时机=失焦且文本有变化才 commit 一次（防逐键压撤回栈）。
		//"无变化不发"对齐 mCurveTextButton 同值不压栈口径——直接与模型现值比对，
		//回填（阻塞信号不改脏状态）/同值失焦/切节点零提交；commit 内部另有
		//_loading 与未绑定守卫双保险
		if (watched == _textEdit && event->type() == QEvent::FocusOut)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (doc->node(_nodeId))
			{
				const QString text = _textEdit->toPlainText();
				if (text != doc->nodeProperty(_nodeId, CurveProp::kText).toString())
					commit(CurveProp::kText, text);
			}
		}
		return mCurvePropPageBase::eventFilter(watched, event);
	}

	void mAnnotation3DPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(_nodeId);
		if (!n)
			return;

		//生效行回显全程阻塞信号（R30 教训：回显触发提交=毒化模型）。置灰占位
		//控件（{U}{V}{W} 三模板钮/附着方式）不在回填路径——构造时静态值一次
		//成型，永不回写。R46 起注释管理组退役，回填只覆盖 文字/属性 两组生效行
		{
			const QSignalBlocker blockText(_textEdit);
			const QSignalBlocker blockAlign(_alignCombo);
			const QSignalBlocker blockTextColor(_textColorBtn);
			const QSignalBlocker blockFont(_fontBtn);
			const QSignalBlocker blockBg(_bgColorBtn);
			const QSignalBlocker blockWidth(_borderWidthCombo);
			const QSignalBlocker blockBorder(_borderCheck);
			_textEdit->setPlainText(doc->nodeProperty(_nodeId, CurveProp::kText).toString());
			_alignCombo->setCurrentIndex(qBound(0,
				doc->nodeProperty(_nodeId, CurveProp::kAlign).toInt(), 2));
			//文字/背景颜色：无效色按 2D 文本色口径回显兜底色（模型值不动）
			const QColor textColor = doc->nodeProperty(_nodeId, CurveProp::kColor).value<QColor>();
			_textColorBtn->setColor(textColor.isValid() ? textColor : QColor(51, 51, 51));
			_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFont).value<QFont>());
			const QColor bgColor = doc->nodeProperty(_nodeId, CurveProp::kBgColor).value<QColor>();
			_bgColorBtn->setColor(bgColor.isValid() ? bgColor : QColor(Qt::white));
			_borderWidthCombo->setLineWidthIndex(qBound(0,
				int(doc->nodeProperty(_nodeId, CurveProp::kBorderWidth).toDouble() + 0.5) - 1, 3));
			//显示边框（kBorderShow，R42 二期翻活；V5 及更早快照读入缺省 true）
			_borderCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kBorderShow).toBool());
		}
	}
}
