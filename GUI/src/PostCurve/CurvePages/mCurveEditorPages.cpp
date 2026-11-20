#include "mCurveEditorPages.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveView/mCurveChartView.h"
#include "../CurveWidgets/mCurveFoldSection.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QSpinBox>
#include <QPixmap>
#include <QPainter>

namespace PostCurve
{
	//--- mCurveEditorPropPage ---

	mCurveEditorPropPage::mCurveEditorPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mCurveEditorPropPage::buildUi()
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(6);

		//标准：可见性（取消勾选曲线连同标签一并隐藏）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = new mCurveCheckBox(this);
		_visibleCheck->setText(QStringLiteral("可见性"));
		baseFold->addRow(_visibleCheck);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});

		//曲线标签：显示标签/标签内容/显示标签前缀·标签前缀/显示标签后缀·标签后缀
		//（截图03交互，与参数栏曲线页一致：标签内容/前缀/后缀为模板下拉——点箭头
		//勾选来源变量按"~"拼接，编辑区可直接输入；取消"显示标签"下方设置项置灰；
		//标签渲染待实现，模型与编辑UI先行）
		mCurveFoldSection* labelFold = new mCurveFoldSection(QStringLiteral("曲线标签"), this);
		_labelFold = labelFold;
		_labelVisibleCheck = new mCurveCheckBox(this);
		_labelVisibleCheck->setText(QStringLiteral("显示标签"));
		labelFold->addRow(_labelVisibleCheck);
		_labelContentCombo = new mCurveTemplateCombo(this);
		labelFold->addRow(QStringLiteral("标签内容"), _labelContentCombo);
		_labelPrefixCheck = new mCurveCheckBox(this);
		_labelPrefixCheck->setText(QStringLiteral("显示标签前缀"));
		labelFold->addRow(_labelPrefixCheck);
		_labelPrefixEdit = new mCurveTemplateCombo(this);
		labelFold->addRow(QStringLiteral("标签前缀"), _labelPrefixEdit);
		_labelSuffixCheck = new mCurveCheckBox(this);
		_labelSuffixCheck->setText(QStringLiteral("显示标签后缀"));
		labelFold->addRow(_labelSuffixCheck);
		_labelSuffixEdit = new mCurveTemplateCombo(this);
		labelFold->addRow(QStringLiteral("标签后缀"), _labelSuffixEdit);
		connect(_labelVisibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kLabelVisible, on);
			updateLabelStates();
		});
		connect(_labelContentCombo, &mCurveTemplateCombo::templateTextChanged,
			this, [this](const QString& text)
		{
			commit(CurveProp::kLabelText, text);
		});
		connect(_labelPrefixCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kLabelPrefixVisible, on);
			updateLabelStates();
		});
		connect(_labelPrefixEdit, &mCurveTemplateCombo::templateTextChanged,
			this, [this](const QString& text)
		{
			commit(CurveProp::kLabelPrefix, text);
		});
		connect(_labelSuffixCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kLabelSuffixVisible, on);
			updateLabelStates();
		});
		connect(_labelSuffixEdit, &mCurveTemplateCombo::templateTextChanged,
			this, [this](const QString& text)
		{
			commit(CurveProp::kLabelSuffix, text);
		});

		//线条属性：线型/线条颜色/线宽/区域填充
		mCurveFoldSection* lineFold = new mCurveFoldSection(QStringLiteral("线条属性"), this);
		_lineStyleCombo = new mCurvePenStyleCombo(this);
		lineFold->addRow(QStringLiteral("线型"), _lineStyleCombo);
		_colorBtn = new mCurveColorButton(Qt::blue, this);
		lineFold->addRow(QStringLiteral("线条颜色"), _colorBtn);
		_lineWidthSpin = createCurveDoubleSpin(1, 0.5, 10.0, 0.5, this);
		lineFold->addRow(QStringLiteral("线宽"), _lineWidthSpin);
		_areaFillCheck = new mCurveCheckBox(this);
		_areaFillCheck->setText(QStringLiteral("区域填充"));
		lineFold->addRow(_areaFillCheck);
		connect(_lineStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLineStyle, idx);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kColor, c);
		});
		connect(_lineWidthSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kLineWidth, v);
		});
		connect(_areaFillCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAreaFill, on);
		});

		//符号属性：标识样式（绘图选项曲线页同款 10 种符号图标，索引=kSymbolStyle 0..9）
		_symbolFold = new mCurveFoldSection(QStringLiteral("符号属性"), this);
		_symbolStyleCombo = new mCurveSymbolStyleCombo(this);
	_symbolFold->addRow(QStringLiteral("标识样式"), _symbolStyleCombo);
	//参考交互：选非空标记后现出符号颜色/符号密度（标记间隔，1=逐点）/
	//缩放尺寸（实际大小=基准大小 大圆点12px·其余8px × 该系数），选"无"隐藏
	_symbolColorBtn = new mCurveColorButton(Qt::red, this);
	_symbolFold->addRow(QStringLiteral("符号颜色"), _symbolColorBtn);
	_densitySpin = createCurveSpinBox(1, 50, this);
	_densitySpin->setValue(1);
	_symbolFold->addRow(QStringLiteral("符号密度"), _densitySpin);
	_scaleSpin = createCurveDoubleSpin(6, 0.001, 100.0, 0.1, this);
	_scaleSpin->setValue(1.0);
	_symbolFold->addRow(QStringLiteral("缩放尺寸"), _scaleSpin);
	connect(_symbolStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		commit(CurveProp::kSymbolStyle, idx);
		updateSymbolRows();
	});
	connect(_densitySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this, [this](int v)
	{
		commit(CurveProp::kSymbolDensity, v);
	});
	connect(_scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, [this](double v)
	{
		commit(CurveProp::kSymbolScale, v);
	});
	connect(_symbolColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
	{
		commit(CurveProp::kSymbolColor, c);
	});

		updateLabelStates();
		updateSymbolRows();

		layout->addWidget(baseFold);
		layout->addWidget(labelFold);
		layout->addWidget(lineFold);
		layout->addWidget(_symbolFold);
		layout->addStretch(1);
	}

	void mCurveEditorPropPage::updateLabelStates()
	{
		//截图交互：取消"显示标签"则下方标签设置项全部置灰（不隐藏）；
		//前缀/后缀开关各自控制其模板下拉的可用态。带行标签的三行走 setRowEnabled，
		//行标签一并置灰（只禁编辑器时行标签保持黑字，整行看起来仍可用）
		const bool labelOn = _labelVisibleCheck->isChecked();
		_labelFold->setRowEnabled(_labelContentCombo, labelOn);
		_labelPrefixCheck->setEnabled(labelOn);
		_labelSuffixCheck->setEnabled(labelOn);
		_labelFold->setRowEnabled(_labelPrefixEdit, labelOn && _labelPrefixCheck->isChecked());
		_labelFold->setRowEnabled(_labelSuffixEdit, labelOn && _labelSuffixCheck->isChecked());
	}

	void mCurveEditorPropPage::updateSymbolRows()
	{
		//参考交互：仅选了非空标记后现出符号设置行（"无"=整组隐藏）
		const bool hasSymbol = _symbolStyleCombo->currentIndex() > 0;
		_symbolFold->setRowVisible(_symbolColorBtn, hasSymbol);
		_symbolFold->setRowVisible(_densitySpin, hasSymbol);
		_symbolFold->setRowVisible(_scaleSpin, hasSymbol);
	}

	void mCurveEditorPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_labelVisibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kLabelVisible).toBool());
		//模板下拉回填（setTemplateText 不发信号），原样显示模型值——默认模板来自
		//CurveSeriesData 出厂初值；此处不再做"空值换默认"替换，否则清空/互换提交的
		//空文本回填时又被默认值写回并重新勾选，清空立即失效
		_labelContentCombo->setTemplateText(doc->nodeProperty(_nodeId, CurveProp::kLabelText).toString());
		_labelPrefixCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kLabelPrefixVisible).toBool());
		_labelPrefixEdit->setTemplateText(doc->nodeProperty(_nodeId, CurveProp::kLabelPrefix).toString());
		_labelSuffixCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kLabelSuffixVisible).toBool());
		_labelSuffixEdit->setTemplateText(doc->nodeProperty(_nodeId, CurveProp::kLabelSuffix).toString());
		_lineStyleCombo->setPenStyleIndex(doc->nodeProperty(_nodeId, CurveProp::kLineStyle).toInt());
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kColor).value<QColor>());
		_lineWidthSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kLineWidth).toDouble());
		_areaFillCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAreaFill).toBool());
	_symbolStyleCombo->setSymbolStyleIndex(qBound(0,
		doc->nodeProperty(_nodeId, CurveProp::kSymbolStyle).toInt(), 9));
	_densitySpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kSymbolDensity).toInt());
	_scaleSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kSymbolScale).toDouble());
	_symbolColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kSymbolColor).value<QColor>());
	updateLabelStates();
	updateSymbolRows();
}

	//--- mChartEditorPropPage ---

	mChartEditorPropPage::mChartEditorPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mChartEditorPropPage::buildUi()
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(6);

		//基础：表头可见性/表头/图表类型（页眉数据挂图表节点，渲染已接线即时生效；
		//表头=按钮弹"编辑字符"弹窗，与参数栏图表页同款）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("基础"), this);
		_headerVisibleCheck = new mCurveCheckBox(this);
		baseFold->addRow(QStringLiteral("表头可见性"), _headerVisibleCheck);
		_headerTextBtn = new mCurveTextButton(this);
		baseFold->addRow(QStringLiteral("表头"), _headerTextBtn);
		_typeCombo = new QComboBox(this);
		setupChartTypeCombo(_typeCombo);   //XY/柱状/复数已接渲染；极坐标/蛛网置灰占位
		baseFold->addRow(QStringLiteral("图表类型"), _typeCombo);
		connect(_headerVisibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kHeaderVisible, on);
		});
		connect(_headerTextBtn, &mCurveTextButton::textCommitted, this, [this](const QString& text)
		{
			commit(CurveProp::kHeaderText, text);
		});
		connect(_typeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			//切换全流程（截图）：确认弹窗 + 继续则类型切换并清空窗口曲线，
			//取消则下拉已回退不提交
			applyChartTypeSwitch(_typeCombo, _nodeId, idx);
		});

		//高级：背景颜色/网格线颜色（渲染随 rebuild 下发：背景色 PlotSettings，
		//网格色各轴 AxisSettings）
		mCurveFoldSection* advFold = new mCurveFoldSection(QStringLiteral("高级"), this);
		_bgColorBtn = new mCurveColorButton(Qt::white, this);
		advFold->addRow(QStringLiteral("背景颜色"), _bgColorBtn);
		_gridColorBtn = new mCurveColorButton(QColor(200, 200, 200), this);
		advFold->addRow(QStringLiteral("网格线颜色"), _gridColorBtn);
		connect(_bgColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBackgroundColor, c);
		});
		connect(_gridColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kGridColor, c);
		});

		layout->addWidget(baseFold);
		layout->addWidget(advFold);
		layout->addStretch(1);
	}

	void mChartEditorPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_headerVisibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kHeaderVisible).toBool());
		_headerTextBtn->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderText).toString());
		//R30：类型回显只回填不触发切换（毒化根因同参数栏 mChartPropPage——未阻塞
		//信号时 chartType=5 被 qBound(0,5,4) 钳到 4，首载触发真实切换 5→蛛网图4）。
		//三维图回显第 6 项"三维图"（置灰项，仅显示）；其余类型维持既有
		//qBound(0,t,4) 钳制
		{
			const QSignalBlocker blocker(_typeCombo);
			const int t = doc->nodeProperty(_nodeId, CurveProp::kChartType).toInt();
			_typeCombo->setCurrentIndex(t == 5 ? 5 : qBound(0, t, 4));
		}
		_bgColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBackgroundColor).value<QColor>());
		_gridColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kGridColor).value<QColor>());
	}
}

namespace PostCurve
{
	//--- mAnnotationEditorPropPage ---

	namespace
	{
		//位置"X Y"单输入框辅助（与注释属性页同语义）：固定 5 位小数
		//（不足补 0）、空格分隔解析；simplified() 折叠空白后按空格切分，天然无空串项
		QString formatPosPair(double x, double y)
		{
			return QStringLiteral("%1 %2").arg(QString::number(x, 'f', 5),
				QString::number(y, 'f', 5));
		}
		bool parsePosPair(const QString& text, double& x, double& y)
		{
			const QStringList parts = text.simplified().split(' ');
			if (parts.size() != 2)
				return false;
			bool okX = false, okY = false;
			x = parts[0].toDouble(&okX);
			y = parts[1].toDouble(&okY);
			return okX && okY;
		}
	}

mAnnotationEditorPropPage::mAnnotationEditorPropPage(QWidget* parent)
	: mCurvePropPageBase(parent)
{
	buildUi();
}

void mAnnotationEditorPropPage::buildUi()
{
	//条目多于弹窗默认高度（280x400），整页放滚动区可滚动查看
	QVBoxLayout* outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	QScrollArea* scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	QWidget* content = new QWidget;
	content->setAutoFillBackground(true);
	QVBoxLayout* layout = new QVBoxLayout(content);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(6);
	scroll->setWidget(content);
	outer->addWidget(scroll);

	//标准：可见性/名称/文本/标量值格式化[格式/精度]/附着于/附录曲线/附录点/位置，
	//设计图15（位置行本期保持原样不动）
	_annoFold = new mCurveFoldSection(QStringLiteral("标准"), content);
	_visibleCheck = new mCurveCheckBox(content);
	_visibleCheck->setText(QStringLiteral("可见性"));
	_annoFold->addRow(_visibleCheck);
	//可见性：默认勾选，取消勾选即隐藏注释（渲染层不建渲染项），勾选恢复显示
	connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
	{
		commit(CurveProp::kVisible, on);
	});
	_nameEdit = new QLineEdit(content);
	_annoFold->addRow(QStringLiteral("名称"), _nameEdit);
	_textBtn = new QPushButton(content);
    _textBtn->setStyleSheet("QPushButton { text-align: left; }");
	_annoFold->addRow(QStringLiteral("文本"), _textBtn);
	_scalarCBox = new mCurveCheckBox(content);
	_scalarCBox->setText(QStringLiteral("标量值格式化"));
	_annoFold->addRow(_scalarCBox);
	//标量值格式化：勾选后注释文本中 {X}/{Y}/{Slope}/{Curv} 等求值数值按
	//下方格式（0科学计数法 1定点格式）与精度显示
	connect(_scalarCBox, &QCheckBox::toggled, this, [this](bool on)
	{
		commit(CurveProp::kScalarFmtOn, on);
		updateConditionalRows();
	});
	_scalarFmtCombo = new QComboBox(content);
	_scalarFmtCombo->addItems({ QStringLiteral("科学计数法"), QStringLiteral("定点格式") });
	_annoFold->addRow(QStringLiteral("格式"), _scalarFmtCombo);
	connect(_scalarFmtCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		commit(CurveProp::kScalarFmtMode, idx);
	});
	_scalarPrecisionSpin = createCurveSpinBox(0, 15, content);
	_scalarPrecisionSpin->setValue(5);
	_annoFold->addRow(QStringLiteral("精度"), _scalarPrecisionSpin);
	connect(_scalarPrecisionSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this, [this](int v)
	{
		commit(CurveProp::kScalarFmtPrecision, v);
	});
	_dependOnCombo = new QComboBox(content);
	_dependOnCombo->addItems({QStringLiteral("曲线"), QStringLiteral("视图") });
	_annoFold->addRow(QStringLiteral("附着于"), _dependOnCombo);
	_dependCurveCombo = new QComboBox(content);
	_annoFold->addRow(QStringLiteral("附着曲线"), _dependCurveCombo);
	//附着点：顺序号（1基），改值即附着到该点（视图层吸附锚点、越界回写钳制值）
	_attachIndexSpin = createCurveSpinBox(1, 1000000, content);
	_annoFold->addRow(QStringLiteral("附着点"), _attachIndexSpin);
	connect(_attachIndexSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this, [this](int v)
	{
		commit(CurveProp::kAttachIndex, v);
	});

	//位置：单输入框"X Y"空格分隔（数据坐标，固定 5 位小数回显，不足补 0，与属性页同语义）
	_posEdit = new QLineEdit(content);
	_annoFold->addRow(QStringLiteral("位置"), _posEdit);

	connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
	{
		commit(CurveProp::kName, _nameEdit->text());
	});
	connect(_textBtn, &QPushButton::clicked, this, [this]()
	{
		//与视图中双击注释一致（mCurveChartView::showAnnotationEditor）：
		//占位"视图注释"回填默认模板，弹 mPostAnnotationDialog，保存写回 kText
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(_nodeId);
		if (!n)
			return;
		QString tpl = n->annotation.text;
		if (tpl == kViewAnnotationPlaceholder)
			tpl = QStringLiteral("Y={Y} @ X={X}");
		PostCommon::mPostAnnotationDialog dlg(this);
		dlg.setAnnotationText(tpl);
		if (dlg.exec() != QDialog::Accepted)
			return;
		commit(CurveProp::kText, dlg.annotationText());
	});
	connect(_dependOnCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		//类别切换（曲线类/视图类，创建位置决定的分类）：
		//视图->曲线：先提交附着曲线再切类别（图表视图据此吸附附着曲线最近点、
		//占位文本回填曲线模板，样式对齐曲线上创建）；
		//曲线->视图：样式对齐曲线外新建的视图注释（文本置占位，渲染去引出线、
		//不再跟随曲线色）
		if (idx == 0)
		{
			commit(CurveProp::kAttachCurve, _dependCurveCombo->currentText());
			commit(CurveProp::kAttachMode, 0);
		}
		else
		{
			commit(CurveProp::kAttachMode, 1);
			commit(CurveProp::kText, kViewAnnotationPlaceholder);
		}
		//曲线附着显示附着曲线/附着点；视图附着隐藏"边框颜色"行直显取色
		updateConditionalRows();
	});
	connect(_dependCurveCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int)
	{
		//附着曲线变化：曲线类注释由图表视图重新吸附最近点并按新曲线求值文本
		commit(CurveProp::kAttachCurve, _dependCurveCombo->currentText());
	});
	connect(_posEdit, &QLineEdit::editingFinished, this, [this]()
	{
		//"X Y"空格分隔解析：两项均合法才提交；缺项/非数字按模型回显，
		//不留半提交状态
		double x = 0.0, y = 0.0;
		if (!parsePosPair(_posEdit->text(), x, y))
		{
			syncPosEdit();
			return;
		}
		commit(CurveProp::kPosX, x);
		commit(CurveProp::kPosY, y);
	});

	//选项：最小化[半径/颜色] + 显示子分组，设计图15
	_optFold = new mCurveFoldSection(QStringLiteral("选项"), content);
	_minimizeCheck = new mCurveCheckBox(content);
	_minimizeCheck->setText(QStringLiteral("最小化"));
	_optFold->addRow(_minimizeCheck);
	_minimizeRadiusSpin = createCurveSpinBox(0, 100, content);
	_minimizeRadiusSpin->setValue(6);
	_optFold->addRow(QStringLiteral("最小化半径"), _minimizeRadiusSpin);
	_minimizeColorBtn = new mCurveColorButton(QColor(0, 200, 0), content);
	_optFold->addRow(QStringLiteral("最小化颜色"), _minimizeColorBtn);
	//最小化（曲线类生效）：锚点处实心小方块（边长=半径），悬停还原完整样式
	connect(_minimizeCheck, &QCheckBox::toggled, this, [this](bool on)
	{
		commit(CurveProp::kMinimized, on);
		updateConditionalRows();
	});
	connect(_minimizeRadiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		this, [this](int v)
	{
		commit(CurveProp::kMinimizeRadius, v);   //方块边长
	});
	connect(_minimizeColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
	{
		commit(CurveProp::kMinimizeColor, c);
	});

	//显示：字体设置/对齐方式/边框线宽/边框颜色[选择边框颜色]/自动定位[策略/偏移]/背景颜色[选择背景颜色]/文本颜色[选择文本颜色]
	_dispFold = new mCurveFoldSection(QStringLiteral("显示"), content);
	//字体设置：绘图选项同款交互（按钮弹"选择字体"对话框），接 kFont（渲染下一期）
	_fontBtn = new mCurveFontButton(content);
	_dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
	connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
	{
		commit(CurveProp::kFont, f);
	});
	_alignCombo = new QComboBox(content);
	_alignCombo->addItems({ QStringLiteral("左"), QStringLiteral("中"), QStringLiteral("右") });
	_dispFold->addRow(QStringLiteral("对齐方式"), _alignCombo);

    //边框线宽：线宽样本图标（1..4px 横线），与设计图"——"形态一致
	_borderWidthCombo = new QComboBox(content);
    _borderWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/1px.svg"), "");
    _borderWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/2px.svg"), "");
    _borderWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/3px.svg"), "");
    _borderWidthCombo->addItem(QIcon(":/mMainWidgets/Icon/NVH/4px.svg"), "");
	_dispFold->addRow(QStringLiteral("边框线宽"), _borderWidthCombo);
	connect(_borderWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		commit(CurveProp::kBorderWidth, double(idx + 1));   //图标行0..3 -> 线宽1..4px
	});
	//边框颜色：附着曲线时显示，默认"跟随曲线颜色"，选"自定义"后加显
	//"选择边框颜色"行；附着于=视图时本行整行隐藏，直接显示"选择边框颜色"，
	//切回曲线后复现（设计图15）
	_borderColorCombo = new QComboBox(content);
	_borderColorCombo->addItems({ QStringLiteral("跟随曲线颜色"), QStringLiteral("自定义") });
	_dispFold->addRow(QStringLiteral("边框颜色"), _borderColorCombo);
	_borderColorBtn = new mCurveColorButton(Qt::black, content);
	_dispFold->addRow(QStringLiteral("选择边框颜色"), _borderColorBtn);
	connect(_borderColorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		commit(CurveProp::kBorderMode, idx);   //0跟随曲线颜色 1自定义
		updateConditionalRows();
	});
	connect(_borderColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
	{
		commit(CurveProp::kBorderColor, c);   //自定义色（视图注释下按钮直显，同样生效）
	});
	_autoPosCheck = new mCurveCheckBox(content);
	_autoPosCheck->setText(QStringLiteral("自动定位"));
	_dispFold->addRow(_autoPosCheck);
	//自动定位策略（容差+锚点合一）：首项"不重叠"，其余为锚位
	_autoPosToleranceCombo = new QComboBox(content);
    _autoPosToleranceCombo->addItems({ QStringLiteral("不重叠"), QStringLiteral("左上"), QStringLiteral("右上"),
        QStringLiteral("左中"), QStringLiteral("居中"), QStringLiteral("右中"),
        QStringLiteral("左下"), QStringLiteral("正下"), QStringLiteral("右下") });
	_dispFold->addRow(QStringLiteral("自动定位策略"), _autoPosToleranceCombo);
	connect(_autoPosToleranceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		commit(CurveProp::kAutoAnchor, idx);   //策略0..8与模型语义一致
		updateConditionalRows();
	});

	//策略选了具体锚位才需要偏移，"不重叠"时隐藏；偏移=注释到点的距离
	_autoPosOffsetSpin = createCurveDoubleSpin(1, 0.0, 100.0, 0.5, content);
	_autoPosOffsetSpin->setValue(3.0);
	_dispFold->addRow(QStringLiteral("自动定位偏移"), _autoPosOffsetSpin);
	connect(_autoPosOffsetSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, [this](double v)
	{
		commit(CurveProp::kAutoOffset, v);
	});
	connect(_autoPosCheck, &QCheckBox::toggled, this, [this](bool on)
	{
		commit(CurveProp::kAutoPos, on);   //文本框按策略绕点排布
		updateConditionalRows();
	});
	_bgColorCombo = new QComboBox(content);
	_bgColorCombo->addItems({ QStringLiteral("默认"), QStringLiteral("自定义") });
	_dispFold->addRow(QStringLiteral("背景颜色"), _bgColorCombo);
	_bgColorBtn = new mCurveColorButton(Qt::white, content);
	_dispFold->addRow(QStringLiteral("选择背景颜色"), _bgColorBtn);

	connect(_bgColorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		//默认=无效色（渲染回退半透明白），自定义=当前取色行颜色
		commit(CurveProp::kBgColor, idx == 1 ? _bgColorBtn->color() : QColor());
		updateConditionalRows();
	});
	connect(_bgColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
	{
		commit(CurveProp::kBgColor, c);
	});
	_textColorCombo = new QComboBox(content);
	_textColorCombo->addItems({ QStringLiteral("默认"), QStringLiteral("自定义") });
	_dispFold->addRow(QStringLiteral("文本颜色"), _textColorCombo);
	_textColorBtn = new mCurveColorButton(QColor(51, 51, 51), content);
	_dispFold->addRow(QStringLiteral("选择文本颜色"), _textColorBtn);
	connect(_textColorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		this, [this](int idx)
	{
		//默认=无效色（渲染随边框色），自定义=当前取色行颜色
		commit(CurveProp::kColor, idx == 1 ? _textColorBtn->color() : QColor());
		updateConditionalRows();
	});
	connect(_textColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
	{
		commit(CurveProp::kColor, c);
	});
	//占位项（对齐方式）：渲染接线未完善，不接命令栈（字体设置已接 kFont，
	//经命令栈提交），待功能后续完善（可见性/标量值格式化/附着于/最小化/自动定位已接命令栈）

	layout->addWidget(_annoFold);
	layout->addWidget(_optFold);
    layout->addWidget(_dispFold);
	layout->addStretch(1);

	//默认附着曲线，随后统一刷新一次条件行显隐
	_dependOnCombo->setCurrentIndex(0);
	updateConditionalRows();
}

void mAnnotationEditorPropPage::updateConditionalRows()
{
	//标准：勾选"标量值格式化"显示格式/精度；附着曲线时显示附录曲线/附录点
	const bool scalarFmt = _scalarCBox->isChecked();
	_annoFold->setRowVisible(_scalarFmtCombo, scalarFmt);
	_annoFold->setRowVisible(_scalarPrecisionSpin, scalarFmt);

	const bool onCurve = _dependOnCombo->currentIndex() == 0;
	_annoFold->setRowVisible(_dependCurveCombo, onCurve);
	_annoFold->setRowVisible(_attachIndexSpin, onCurve);

	//选项：勾选"最小化"显示半径/颜色
	_optFold->setRowVisible(_minimizeRadiusSpin, _minimizeCheck->isChecked());
	_optFold->setRowVisible(_minimizeColorBtn, _minimizeCheck->isChecked());

	//显示：附着视图时"边框颜色"行整行隐藏，直接显示"选择边框颜色"，
	//切回曲线复现下拉（该行再随"自定义"显隐）；自动定位仅曲线类生效（视图类
	//整组隐藏：勾选框/策略/偏移），勾选后显示策略行，策略非"不重叠"再加显
	//偏移；背景/文本颜色自定义后显示取色行
	const bool onView = _dependOnCombo->currentIndex() == 1;
	_dispFold->setRowVisible(_borderColorCombo, !onView);
	_dispFold->setRowVisible(_borderColorBtn, onView ||
		_borderColorCombo->currentIndex() == 1);
	const bool autoPos = !onView && _autoPosCheck->isChecked();
	_dispFold->setRowVisible(_autoPosCheck, !onView);
	_dispFold->setRowVisible(_autoPosToleranceCombo, autoPos);
	_dispFold->setRowVisible(_autoPosOffsetSpin, autoPos &&
		_autoPosToleranceCombo->currentIndex() != 0);
	_dispFold->setRowVisible(_bgColorBtn, _bgColorCombo->currentIndex() == 1);
	_dispFold->setRowVisible(_textColorBtn, _textColorCombo->currentIndex() == 1);
}

	//位置框回显："X Y"（固定 5 位小数，空格分隔，与属性页同语义）
void mAnnotationEditorPropPage::syncPosEdit()
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	_posEdit->setText(formatPosPair(doc->nodeProperty(_nodeId, CurveProp::kPosX).toDouble(),
		doc->nodeProperty(_nodeId, CurveProp::kPosY).toDouble()));
}

void mAnnotationEditorPropPage::loadFromModel()
{
	CurveDocument* doc = mCurveDocController::getInstance()->document();
	if (!doc->node(_nodeId))
		return;
	//可见性（默认勾选）与标量值格式化（格式/精度）回填
	_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
	_scalarCBox->setChecked(doc->nodeProperty(_nodeId, CurveProp::kScalarFmtOn).toBool());
	_scalarFmtCombo->setCurrentIndex(qBound(0,
		doc->nodeProperty(_nodeId, CurveProp::kScalarFmtMode).toInt(), 1));
	_scalarPrecisionSpin->setValue(
		doc->nodeProperty(_nodeId, CurveProp::kScalarFmtPrecision).toInt());
	_nameEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kName).toString());
    _textBtn->setText(doc->nodeProperty(_nodeId, CurveProp::kText).toString());
	//_textEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kText).toString());
	syncPosEdit();
	//显示组回填：文本颜色（无效=默认态，按钮回退深灰）/字体/对齐/边框颜色模式与
	//自定义值/边框线宽（图标行=线宽-1）/背景颜色（无效=默认态，按钮回退白色）
	_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFont).value<QFont>());
	const QColor textColor = doc->nodeProperty(_nodeId, CurveProp::kColor).value<QColor>();
	_textColorBtn->setColor(textColor.isValid() ? textColor : QColor(51, 51, 51));
	_textColorCombo->setCurrentIndex(textColor.isValid() ? 1 : 0);
	_alignCombo->setCurrentIndex(qBound(0, doc->nodeProperty(_nodeId, CurveProp::kAlign).toInt(), 2));
	_borderColorCombo->setCurrentIndex(qBound(0,
		doc->nodeProperty(_nodeId, CurveProp::kBorderMode).toInt(), 1));
	_borderColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBorderColor).value<QColor>());
	_borderWidthCombo->setCurrentIndex(qBound(0,
		int(doc->nodeProperty(_nodeId, CurveProp::kBorderWidth).toDouble()) - 1, 3));
	const QColor bgColor = doc->nodeProperty(_nodeId, CurveProp::kBgColor).value<QColor>();
	_bgColorBtn->setColor(bgColor.isValid() ? bgColor : QColor(Qt::white));
	_bgColorCombo->setCurrentIndex(bgColor.isValid() ? 1 : 0);
	_autoPosCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAutoPos).toBool());
	_autoPosOffsetSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kAutoOffset).toDouble());

	//附着曲线回显：本图表的曲线名列表，并选中模型记录的附着曲线；
	//附着于（注释类别）按模型回填
	_dependCurveCombo->clear();
	const int chartId = doc->chartNodeId(_nodeId);
	if (chartId > 0)
	{
		for (int childId : doc->childNodeIds(chartId))
		{
			CurveNode* c = doc->node(childId);
			if (c && c->type == CurveNodeType::Curve)
				_dependCurveCombo->addItem(c->series.name);
		}
	}
	const QString attachCurve = doc->nodeProperty(_nodeId, CurveProp::kAttachCurve).toString();
	if (!attachCurve.isEmpty())
	{
		const int ci = _dependCurveCombo->findText(attachCurve);
		if (ci >= 0)
			_dependCurveCombo->setCurrentIndex(ci);
	}
	_dependOnCombo->setCurrentIndex(qBound(0,
		doc->nodeProperty(_nodeId, CurveProp::kAttachMode).toInt(), 1));
	//附着点顺序（1基，0=未指定回显为1）/最小化（曲线类小方块）/自动定位策略
	_attachIndexSpin->setValue(qMax(1,
		doc->nodeProperty(_nodeId, CurveProp::kAttachIndex).toInt()));
	_minimizeCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kMinimized).toBool());
	_minimizeRadiusSpin->setValue(
		static_cast<int>(doc->nodeProperty(_nodeId, CurveProp::kMinimizeRadius).toDouble()));
	const QColor minColor = doc->nodeProperty(_nodeId, CurveProp::kMinimizeColor).value<QColor>();
	_minimizeColorBtn->setColor(minColor.isValid() ? minColor : QColor(0, 200, 0));
	_autoPosToleranceCombo->setCurrentIndex(qBound(0,
		doc->nodeProperty(_nodeId, CurveProp::kAutoAnchor).toInt(), 8));
}
}

namespace PostCurve
{
	//--- mAxisEditorPropPage ---

	mAxisEditorPropPage::mAxisEditorPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mAxisEditorPropPage::buildUi()
	{
		//条目多于弹窗默认高度（280x400），整页放滚动区可滚动查看；
		//内容对齐参考软件的坐标轴属性编辑器（与参数栏轴页是两套内容，设计图05/06）
		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(0, 0, 0, 0);
		QScrollArea* scroll = new QScrollArea(this);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		QWidget* content = new QWidget;
		content->setAutoFillBackground(true);
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(6);
		scroll->setWidget(content);
		outer->addWidget(scroll);

		//标准：可见性/名称可见性/文本/类型
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), content);
		_visibleCheck = new mCurveCheckBox(content);
		baseFold->addRow(QStringLiteral("可见性"), _visibleCheck);
		_nameVisibleCheck = new mCurveCheckBox(content);
		baseFold->addRow(QStringLiteral("名称可见性"), _nameVisibleCheck);
		//名称 = 树显示名（kName，与参数栏轴页/注释页同款行内输入框；区别于
		//下方"文本"行 kTitle 轴标题）
		_nameEdit = new QLineEdit(content);
		baseFold->addRow(QStringLiteral("名称"), _nameEdit);
		_textEdit = new QLineEdit(content);
		baseFold->addRow(QStringLiteral("文本"), _textEdit);
		_typeCombo = new QComboBox(content);
		_typeCombo->setEnabled(false);   //类型随节点只读回显（X轴=水平，Y/Y2轴=垂直）
		baseFold->addRow(QStringLiteral("类型"), _typeCombo);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});
		connect(_nameVisibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kTitleVisible, on);
		});
		connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kName, _nameEdit->text());
		});
		connect(_textEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kTitle, _textEdit->text());
		});

		//显示：标签字体设置/颜色（字体为绘图选项同款交互：按钮弹"选择字体"对话框；
		//基准=绘图选项默认轴标题字体——建图"默认"写入的就是该值，须回显"默认"）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), content);
		_labelFontBtn = new mCurveFontButton(axisTitleDefaultFont(), content);
		dispFold->addRow(QStringLiteral("标签字体设置"), _labelFontBtn);
		_colorBtn = new mCurveColorButton(Qt::black, content);
		dispFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_labelFontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kTitleFont, f);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kTitleColor, c);
		});

		//缩放设置：缩放方式/参考值/计权/最小值/最大值（设计图11/12：参考值与
		//计权两行仅 dB20/dB10 显示，linear/logarithmic 只有范围两项；参考值
		//dB20/dB10 共用一个值，行标签随缩放方式切换）
		_rangeFold = new mCurveFoldSection(QStringLiteral("缩放设置"), content);
		_scaleModeCombo = new QComboBox(content);
		_scaleModeCombo->addItem(QStringLiteral("Linear"));
		_scaleModeCombo->addItem(QStringLiteral("Logarithmic"));
		_scaleModeCombo->addItem(QStringLiteral("dB20"));
		_scaleModeCombo->addItem(QStringLiteral("dB10"));
		_rangeFold->addRow(QStringLiteral("缩放方式"), _scaleModeCombo);
		_dbRefLabel = new QLabel(QStringLiteral("dB20参考值"), content);
		_dbRefLabel->setFixedWidth(90);   //同 addRow(文本) 行标签规格
		_dbRefLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_dbRefSpin = createCurveSciDoubleSpin(4, 1e-12, 1e9, 1.0, content);   //科学计数法（模型默认 2e-11）
		_rangeFold->addRow(_dbRefLabel, _dbRefSpin);
		_weightCombo = new QComboBox(content);
		_weightCombo->addItems({ QStringLiteral("无"), QStringLiteral("A"),
			QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("U") });
		_rangeFold->addRow(QStringLiteral("计权"), _weightCombo);
		_minSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, content);
		_rangeFold->addRow(QStringLiteral("最小值"), _minSpin);
		_maxSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, content);
		_rangeFold->addRow(QStringLiteral("最大值"), _maxSpin);
		connect(_weightCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kWeightMode, idx);   //0无/1A/2B/3C/4U（U=unweighted 与无等效）
		});
		connect(_dbRefSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kDbRef, v);
		});
		connect(_scaleModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			//跨 linear↔log/dB 切换时重置刻度生成方式为新模式默认：两模式共
			//用 kTickGenMode 但枚举集不同（linear 0均匀/1固定步长；log/dB
			//0每十倍/1均匀/2固定步长），跨集保留旧值会与渲染端分派错位；
			//同集合内切换（dB20↔dB10）保留现值
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const int prevMode = (doc && _nodeId >= 0)
				? doc->nodeProperty(_nodeId, CurveProp::kScaleMode).toInt() : 0;
			commit(CurveProp::kScaleMode, idx);
			if ((prevMode == 0) != (idx == 0))
				commit(CurveProp::kTickGenMode, 0);
			updateRowStates();
		});
		//面板最小值/最大值按"轴上所见"（刻度变换空间）显示与编辑，模型
		//kMin/kMax 存原始值空间：提交前逆变换（linear 恒等）
		auto commitRange = [this](const QString& prop, double v)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			int mode = (doc && _nodeId > 0)
				? doc->nodeProperty(_nodeId, CurveProp::kScaleMode).toInt() : 0;
			//复数图词表收窄轴按有效模式逆变换（与显示/渲染钳制同源，模型值
			//不动）：X1 仅 Linear/Logarithmic（存量 dB 按 Logarithmic），幅相
			//展开 Phase 仅 Linear
			if (doc && _nodeId > 0 && mode > 0)
			{
				const CurveNodeType nt = doc->nodeType(_nodeId);
				CurveNode* chart = doc->node(doc->chartNodeId(_nodeId));
				if (chart && chart->type == CurveNodeType::Chart
					&& chart->chart.chartType == 2)
				{
					if (nt == CurveNodeType::XAxis && mode > 1)
						mode = 1;
					else if (nt == CurveNodeType::YAxis2
						&& chart->chart.complexDisplayFormat == 0)
						mode = 0;
				}
			}
			const double dbRef = (doc && _nodeId > 0)
				? doc->nodeProperty(_nodeId, CurveProp::kDbRef).toDouble() : 1.0;
			//退出自动范围时先把引擎当前显示范围的"另一侧"回写模型：自动轴
			//从不落 min/max（模型里还是构造默认值 0/10），只提交编辑侧会让
			//手动范围立即失真——如改最小值后最大值跳回默认 10
			if (_nodeId > 0 && doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
			{
				double dispLo = 0.0, dispHi = 0.0;
				if (mCurveChartView::queryAxisDisplayRange(_nodeId, &dispLo, &dispHi))
				{
					const bool editingMin = (prop == CurveProp::kMin);
					commit(editingMin ? CurveProp::kMax : CurveProp::kMin,
						mCurveChartView::inverseTransformValue(
							editingMin ? dispHi : dispLo, mode, dbRef));
				}
				commit(CurveProp::kAutoScale, false);
			}
			commit(prop, mCurveChartView::inverseTransformValue(v, mode, dbRef));
		};
		connect(_minSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this, commitRange](double v)
		{
			_maxSpin->setMinimum(v);   //最大值不小于最小值（越界自动抬升并提交）
			commitRange(CurveProp::kMin, v);
		});
		connect(_maxSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this, commitRange](double v)
		{
			_minSpin->setMaximum(v);   //最小值不大于最大值（越界自动压低并提交）
			commitRange(CurveProp::kMax, v);
		});
		//缩放/平移联动：自动范围轴的最小值/最大值回显引擎现值并随缩放刷新
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_axisDisplayRangeChanged,
			this, &mAxisEditorPropPage::onAxisDisplayRangeChanged);

		//坐标轴刻度设置（设计图11/12）：格式/精度/刻度字体设置恒显；刻度生
		//成方式恒显、选项集随缩放方式重建（linear：均匀分步法→刻度总数、固
		//定步长法→固定步长；log/dB：每十倍分步法→每十倍刻度数/每十倍网格线
		//数、均匀分步法/固定步长法同 linear 语义，在刻度变换空间生效；显隐
		//联动见 updateRowStates）
		_tickFold = new mCurveFoldSection(QStringLiteral("坐标轴刻度设置"), content);
		_formatCombo = new QComboBox(content);
		_formatCombo->addItems({ QStringLiteral("自动"), QStringLiteral("科学计数法"),
			QStringLiteral("定点格式") });
		_tickFold->addRow(QStringLiteral("格式"), _formatCombo);
		_tickGenCombo = new QComboBox(content);
		_tickGenCombo->addItems({ QStringLiteral("均匀分步法"), QStringLiteral("固定步长法") });
		_tickFold->addRow(QStringLiteral("刻度生成方式"), _tickGenCombo);
		_tickCountSpin = createCurveDoubleSpin(4, 0.0, 1e9, 1.0, content);
		_tickCountSpin->setValue(10.0);
		_tickFold->addRow(QStringLiteral("刻度总数"), _tickCountSpin);
		_fixedStepSpin = createCurveDoubleSpin(4, 0.0, 1e9, 1.0, content);
		_tickFold->addRow(QStringLiteral("固定步长"), _fixedStepSpin);
		_gridPerMajorSpin = createCurveSpinBox(0, 100, content);
		_tickFold->addRow(QStringLiteral("每主刻度网格数"), _gridPerMajorSpin);
		_decadeTickSpin = createCurveSpinBox(1, 100, content);
		_decadeTickSpin->setValue(5);
		_tickFold->addRow(QStringLiteral("每十倍刻度数"), _decadeTickSpin);
		_decadeGridSpin = createCurveSpinBox(0, 100, content);
		_decadeGridSpin->setValue(5);
		_tickFold->addRow(QStringLiteral("每十倍网格线数"), _decadeGridSpin);
		_precisionSpin = createCurveSpinBox(0, 15, content);
		_precisionSpin->setValue(3);
		_tickFold->addRow(QStringLiteral("精度"), _precisionSpin);
		_tickFontBtn = new mCurveFontButton(content);
		_tickFold->addRow(QStringLiteral("刻度字体设置"), _tickFontBtn);
		connect(_tickFontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kLabelFont, f);
		});
		connect(_formatCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLabelFormat, idx);   //0自动/1科学计数法/2定点格式
		});
		connect(_tickGenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kTickGenMode, idx);   //值=下拉索引：linear 0均匀/1固定步长；log/dB 0每十倍/1均匀/2固定步长
			updateRowStates();
		});
		connect(_tickCountSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kTickCount, v);
		});
		connect(_fixedStepSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kMajorStep, v);
		});
		connect(_gridPerMajorSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kMinorCount, v);
		});
		connect(_decadeTickSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kDecadeTickCount, v);
		});
		connect(_decadeGridSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kDecadeMinorCount, v);
		});
		connect(_precisionSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kPrecision, v);
		});

		updateRowStates();

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addWidget(_rangeFold);
		layout->addWidget(_tickFold);
		layout->addStretch(1);
	}

	void mAxisEditorPropPage::updateRowStates()
	{
		//缩放设置：参考值/计权两行仅 dB20/dB10 显示；参考值行标签随模式切换
		//（两模式共用一个参考值）。坐标轴刻度设置：刻度生成方式恒显，选项集
		//随缩放方式重建并回显模型现值（值=下拉索引；linear 0均匀分步法/1固
		//定步长法；log/dB 0每十倍分步法=遗留默认/1均匀分步法/2固定步长法）；
		//行显隐：均匀→刻度总数+每主刻度网格数，固定步长→固定步长+每主刻度
		//网格数，每十倍（log/dB 默认）→每十倍刻度数+每十倍网格线数
		const int mode = _scaleModeCombo->currentIndex();
		const bool db = mode >= 2;   //2=dB20/3=dB10
		_dbRefLabel->setText((mode == 3)
			? QStringLiteral("dB10参考值") : QStringLiteral("dB20参考值"));
		_rangeFold->setRowVisible(_dbRefSpin, db);
		_rangeFold->setRowVisible(_weightCombo, db);

		const bool linear = mode == 0;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int gen = (doc && _nodeId >= 0 && doc->node(_nodeId))
			? qBound(0, doc->nodeProperty(_nodeId, CurveProp::kTickGenMode).toInt(),
				linear ? 1 : 2)
			: 0;
		{
			//选项集重建+回显均阻塞信号：打开弹窗/切缩放方式经此处刷新，不得
			//回写模型（回写只来自刻度生成方式下拉自身的用户操作）
			const QSignalBlocker blokGen(_tickGenCombo);
			const int optionSet = linear ? 0 : 1;
			if (_tickGenCombo->property("optionSet").toInt() != optionSet)
			{
				_tickGenCombo->clear();
				if (linear)
					_tickGenCombo->addItems({ QStringLiteral("均匀分步法"),
						QStringLiteral("固定步长法") });
				else
					_tickGenCombo->addItems({ QStringLiteral("每十倍分步法"),
						QStringLiteral("均匀分步法"), QStringLiteral("固定步长法") });
				_tickGenCombo->setProperty("optionSet", optionSet);
			}
			_tickGenCombo->setCurrentIndex(gen);
		}
		const bool uniform = gen == (linear ? 0 : 1);   //均匀分步法→刻度总数组
		const bool fixedStep = gen == (linear ? 1 : 2);   //固定步长法→固定步长组
		const bool decade = !linear && gen == 0;   //每十倍分步法→每十倍组（log/dB 默认）
		_tickFold->setRowVisible(_tickCountSpin, uniform);
		_tickFold->setRowVisible(_fixedStepSpin, fixedStep);
		_tickFold->setRowVisible(_gridPerMajorSpin, uniform || fixedStep);
		_tickFold->setRowVisible(_decadeTickSpin, decade);
		_tickFold->setRowVisible(_decadeGridSpin, decade);
	}

	void mAxisEditorPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		//复数图 Y2 幅相展开（Phase）：相位为角度值，缩放方式仅 Linear 显示
		//（log/dB 对相位无意义；实虚展开下虚部为实值量，与参数栏轴页同款放
		//开四项）。幅相下历史存量非 Linear 值保留，显示按 Linear
		CurveNode* chartNode = doc->node(doc->chartNodeId(_nodeId));
		const bool phaseAxisMode = doc->nodeType(_nodeId) == CurveNodeType::YAxis2
			&& chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 2
			&& chartNode->chart.complexDisplayFormat == 0;   //0=幅相/1=实虚
		//复数图 X1 主轴：频率/时间横轴 dB 缩放无意义，词表仅 Linear/Logarithmic
		//（与参数栏轴页同款；历史存量 dB 值显示按 Logarithmic，模型值不动）
		const bool x1AxisMode = doc->nodeType(_nodeId) == CurveNodeType::XAxis
			&& chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 2;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_nameVisibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kTitleVisible).toBool());
		_nameEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kName).toString());
		_textEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kTitle).toString());
		_typeCombo->clear();
		_typeCombo->addItem((doc->nodeType(_nodeId) == CurveNodeType::XAxis
			|| doc->nodeType(_nodeId) == CurveNodeType::XAxis2)
			? QStringLiteral("水平") : QStringLiteral("垂直"));
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kTitleColor).value<QColor>());
		_labelFontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kTitleFont).value<QFont>());
		{
			//阻塞信号：打开弹窗不得改写模型——非阻塞回显会触发缩放方式信号
			//（提交+跨集合重置刻度生成方式），把历史存量值冲掉。下拉项按模
			//式重建（弹窗页跨节点复用，同参数栏轴页词表收窄）
			const QSignalBlocker blokScaleIdx(_scaleModeCombo);
			_scaleModeCombo->clear();
			if (phaseAxisMode)
				_scaleModeCombo->addItem(QStringLiteral("Linear"));
			else if (x1AxisMode)
				_scaleModeCombo->addItems({ QStringLiteral("Linear"),
					QStringLiteral("Logarithmic") });
			else
				_scaleModeCombo->addItems({ QStringLiteral("Linear"),
					QStringLiteral("Logarithmic"), QStringLiteral("dB20"),
					QStringLiteral("dB10") });
			_scaleModeCombo->setCurrentIndex(phaseAxisMode
				? 0
				: qBound(0, doc->nodeProperty(_nodeId, CurveProp::kScaleMode).toInt(),
					x1AxisMode ? 1 : 3));
		}
		_weightCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kWeightMode).toInt(), 4));
		_dbRefSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kDbRef).toDouble());
		_formatCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kLabelFormat).toInt(), 2));
		updateRowStates();   //刻度生成方式选项集重建与回显在 updateRowStates 统一做
		//范围回显：面板按"轴上所见"（刻度变换空间）显示。自动范围下模型
		//min/max 是休眠的手动值，回显引擎现值（queryAxisDisplayRange 已是
		//变换空间）；手动范围下模型 min/max 为原始值空间，正变换后回显。
		//阻塞信号：回显值触发 commitRange 会"提交即退出自动"，打开弹窗不得
		//改写模型。log/dB 下原始值<=0 变换为 NaN，回退显示原值
		double rangeLo = doc->nodeProperty(_nodeId, CurveProp::kMin).toDouble();
		double rangeHi = doc->nodeProperty(_nodeId, CurveProp::kMax).toDouble();
		//回显缩放按有效模式：复数 X1 存量 dB 按 Logarithmic、幅相 Phase 按
		//Linear（词表收窄同源，模型值不动；与渲染端钳制一致，面板与视口所见
		//相同）
		const int rawEchoMode = doc->nodeProperty(_nodeId, CurveProp::kScaleMode).toInt();
		const int echoScaleMode = x1AxisMode ? qMin(rawEchoMode, 1)
			: (phaseAxisMode ? 0 : rawEchoMode);
		const double echoDbRef = doc->nodeProperty(_nodeId, CurveProp::kDbRef).toDouble();
		if (doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
			mCurveChartView::queryAxisDisplayRange(_nodeId, &rangeLo, &rangeHi);
		else
		{
			const double tLo = mCurveChartView::transformValue(rangeLo, echoScaleMode, echoDbRef);
			const double tHi = mCurveChartView::transformValue(rangeHi, echoScaleMode, echoDbRef);
			if (qIsFinite(tLo))
				rangeLo = tLo;
			if (qIsFinite(tHi))
				rangeHi = tHi;
		}
		{
			const QSignalBlocker blokMin(_minSpin);
			const QSignalBlocker blokMax(_maxSpin);
			_minSpin->setValue(rangeLo);
			_maxSpin->setValue(rangeHi);
			//最小/最大互相钳制的边界随回显值同步
			_minSpin->setMaximum(rangeHi);
			_maxSpin->setMinimum(rangeLo);
		}
		//现值回显延后一拍（重建落定后再回查，理由同参数栏轴页）
		QTimer::singleShot(0, this, [this]()
		{
			echoDisplayRange();
		});

		_gridPerMajorSpin->setValue(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kMinorCount).toInt(), 100));
		_tickCountSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kTickCount).toDouble());
		_fixedStepSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kMajorStep).toDouble());
		_decadeTickSpin->setValue(qBound(1,
			doc->nodeProperty(_nodeId, CurveProp::kDecadeTickCount).toInt(), 100));
		_decadeGridSpin->setValue(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kDecadeMinorCount).toInt(), 100));
		_precisionSpin->setValue(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kPrecision).toInt(), 15));
		_tickFontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kLabelFont).value<QFont>());
	}

	void mAxisEditorPropPage::echoDisplayRange()
	{
		//自动范围轴把最小值/最大值回显为引擎当前显示范围（阻塞信号，不得
		//触发 commitRange 退出自动；手动范围轴模型值即生效值，不回查）
		if (_loading || _nodeId < 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId) || !doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
			return;
		double rangeLo = 0.0, rangeHi = 0.0;
		if (!mCurveChartView::queryAxisDisplayRange(_nodeId, &rangeLo, &rangeHi))
			return;
		const QSignalBlocker blokMin(_minSpin);
		const QSignalBlocker blokMax(_maxSpin);
		_minSpin->setValue(rangeLo);
		_maxSpin->setValue(rangeHi);
		_minSpin->setMaximum(rangeHi);
		_maxSpin->setMinimum(rangeLo);
	}

	void mAxisEditorPropPage::onAxisDisplayRangeChanged(int axisNodeId)
	{
		//缩放/平移联动：只联动当前绑定且处于自动范围的轴（手动范围下模型
		//min/max 即生效值，缩放不改变模型值）。阻塞信号回显——不得触发
		//commitRange（提交即退出自动）
		if (axisNodeId != _nodeId || _loading)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId) || !doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
			return;
		double rangeLo = 0.0, rangeHi = 0.0;
		if (!mCurveChartView::queryAxisDisplayRange(_nodeId, &rangeLo, &rangeHi))
			return;
		const QSignalBlocker blokMin(_minSpin);
		const QSignalBlocker blokMax(_maxSpin);
		_minSpin->setValue(rangeLo);
		_maxSpin->setValue(rangeHi);
		//最小/最大互相钳制的边界随回显值同步
		_minSpin->setMaximum(rangeHi);
		_maxSpin->setMinimum(rangeLo);
	}
}

namespace PostCurve
{
	//--- mLegendEditorPropPage ---

	namespace
	{
		//条件显隐行：label(与 FoldSection 行标签同宽同对齐) + 编辑器。
		//行本身不单独显隐——挂进块的 QVBoxLayout，由块整体隐藏时完全收起
		QWidget* makeLegendFormRow(const QString& label, QWidget* editor, QWidget* parent)
		{
			QWidget* row = new QWidget(parent);
			QHBoxLayout* lay = new QHBoxLayout(row);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(8);
			QLabel* lab = new QLabel(label, row);
			lab->setMinimumWidth(72);
			lab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			editor->setParent(row);
			lay->addWidget(lab);
			lay->addWidget(editor, 1);
			return row;
		}
	}

	mLegendEditorPropPage::mLegendEditorPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mLegendEditorPropPage::buildUi()
	{
		//条目多于弹窗默认高度（280x400），整页放滚动区可滚动查看（同轴编辑页）
		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(0, 0, 0, 0);
		QScrollArea* scroll = new QScrollArea(this);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		QWidget* content = new QWidget;
		content->setAutoFillBackground(true);
		QVBoxLayout* layout = new QVBoxLayout(content);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(6);
		scroll->setWidget(content);
		outer->addWidget(scroll);

		//标准：可见性/自动定位/图例位置/X坐标/Y坐标，设计图08
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), content);
		_visibleCheck = new mCurveCheckBox(content);
		baseFold->addRow(QStringLiteral("可见性"), _visibleCheck);
		_autoPosCheck = new mCurveCheckBox(content);
		//设计图08提示文案：勾选隐藏图例位置行；自动避让曲线（图内四角变动，优先右上）
		_autoPosCheck->setToolTip(QStringLiteral(
			"勾选自动定位，则下方图例位置设置隐藏。\n"
			"勾选后图例会根据图表的显示选择不遮挡曲线的位置显示，"
			"在图内左上角、图内左下角、图内右上角、图内右下角这四个方向中变动，"
			"优先图内右上角位置。"));
		baseFold->addRow(QStringLiteral("自动定位"), _autoPosCheck);
		_positionCombo = new QComboBox(content);
		_positionCombo->addItems({
			QStringLiteral("图内-左上角"), QStringLiteral("图内-左下角"),
			QStringLiteral("图内-右上角"), QStringLiteral("图内-右下角"),
			QStringLiteral("图内-用户自定义"),
			QStringLiteral("图外-顶部"), QStringLiteral("图外-底部"),
			QStringLiteral("图外-左侧"), QStringLiteral("图外-右侧") });
		//X/Y 坐标：画布百分比，仅"图内-用户自定义"显示（设计图提示语义）。
		//条件行全部挂块（addBlock -> 内容区 QVBoxLayout）：QFormLayout 行隐藏
		//仍占位，块隐藏才完全收起（否则勾选自动定位后留空白）
		_xSpin = createCurveDoubleSpin(1, 0.0, 100.0, 1.0, content);
		_xSpin->setToolTip(QStringLiteral("图例左上角在画布内的水平位置（百分比）"));
		_ySpin = createCurveDoubleSpin(1, 0.0, 100.0, 1.0, content);
		_ySpin->setToolTip(QStringLiteral("图例左上角在画布内的垂直位置（百分比）"));
		_positionBlock = new QWidget(content);
		QVBoxLayout* posBlockLay = new QVBoxLayout(_positionBlock);
		posBlockLay->setContentsMargins(10, 0, 10, 0);
		posBlockLay->setSpacing(6);
		posBlockLay->addWidget(makeLegendFormRow(QStringLiteral("图例位置"), _positionCombo, _positionBlock));
		_xyBlock = new QWidget(_positionBlock);
		QVBoxLayout* xyLay = new QVBoxLayout(_xyBlock);
		xyLay->setContentsMargins(0, 0, 0, 0);
		xyLay->setSpacing(6);
		xyLay->addWidget(makeLegendFormRow(QStringLiteral("X坐标"), _xSpin, _xyBlock));
		xyLay->addWidget(makeLegendFormRow(QStringLiteral("Y坐标"), _ySpin, _xyBlock));
		posBlockLay->addWidget(_xyBlock);
		baseFold->addBlock(_positionBlock);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});
		connect(_autoPosCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAutoPosition, on);
			updateRowStates();
		});
		connect(_positionCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kPosition, idx);
			updateRowStates();
		});
		connect(_xSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kPosX, v);
		});
		connect(_ySpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kPosY, v);
		});

		//显示：边框样式/边框颜色/字体设置/引线位置/背景填充色/曲线文本色/文本颜色，设计图08
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), content);
		_borderStyleCombo = new mCurveBorderStyleCombo(content);
		dispFold->addRow(QStringLiteral("边框样式"), _borderStyleCombo);
		_borderColorBtn = new mCurveColorButton(Qt::black, content);
		dispFold->addRow(QStringLiteral("边框颜色"), _borderColorBtn);
		//字体设置：绘图选项同款交互（按钮弹"选择字体"对话框），接 kLegendFont；
		//基准=模型图例默认字体（legendDefaultFont，渲染对齐原型），未显式选择时回显"默认"
		_fontBtn = new mCurveFontButton(legendDefaultFont(), content);
		dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kLegendFont, f);
		});
		_leaderPosCombo = new QComboBox(content);
		_leaderPosCombo->addItems({ QStringLiteral("左侧"), QStringLiteral("下方") });
		dispFold->addRow(QStringLiteral("引线位置"), _leaderPosCombo);
		_bgColorBtn = new mCurveColorButton(QColor(255, 255, 255, 200), content);
		dispFold->addRow(QStringLiteral("背景填充色"), _bgColorBtn);
		_textColorModeCombo = new QComboBox(content);
		_textColorModeCombo->addItems({ QStringLiteral("跟随曲线颜色"), QStringLiteral("自定义") });
		dispFold->addRow(QStringLiteral("曲线文本色"), _textColorModeCombo);
		_textColorBtn = new mCurveColorButton(Qt::black, content);
		_textColorBlock = new QWidget(content);
		QVBoxLayout* tcLay = new QVBoxLayout(_textColorBlock);
		tcLay->setContentsMargins(10, 0, 10, 0);
		tcLay->addWidget(makeLegendFormRow(QStringLiteral("文本颜色"), _textColorBtn, _textColorBlock));
		dispFold->addBlock(_textColorBlock);
		connect(_borderStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kBorderStyle, idx);
		});
		connect(_borderColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBorderColor, c);
		});
		connect(_leaderPosCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLeaderPos, idx);
		});
		connect(_bgColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBgColor, c);
		});
		connect(_textColorModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kTextColorMode, idx);
			updateRowStates();
		});
		connect(_textColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kTextColor, c);
		});

		updateRowStates();

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addStretch(1);
	}

	void mLegendEditorPropPage::updateRowStates()
	{
		//图4：勾选自动定位隐藏图例位置块；图5：选"图内-用户自定义"显示X/Y坐标块；
		//图3：曲线文本色选"自定义"显示文本颜色块。显隐只发生在块（VBox）层
		const bool autoPos = _autoPosCheck->isChecked();
		_positionBlock->setVisible(!autoPos);
		_xyBlock->setVisible(!autoPos && _positionCombo->currentIndex() == 4);
		_textColorBlock->setVisible(_textColorModeCombo->currentIndex() == 1);
	}

	void mLegendEditorPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_autoPosCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAutoPosition).toBool());
		_positionCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kPosition).toInt(), 8));
		_xSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kPosX).toDouble());
		_ySpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kPosY).toDouble());
		_borderStyleCombo->setBorderStyleIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kBorderStyle).toInt(), 4));
		_borderColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBorderColor).value<QColor>());
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kLegendFont).value<QFont>());
		_leaderPosCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kLeaderPos).toInt(), 1));
		_bgColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBgColor).value<QColor>());
		_textColorModeCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kTextColorMode).toInt(), 1));
		_textColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kTextColor).value<QColor>());
		updateRowStates();
	}

	//--- mBaselineEditorPropPage ---

	mBaselineEditorPropPage::mBaselineEditorPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mBaselineEditorPropPage::buildUi()
	{
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(6, 6, 6, 6);
		layout->setSpacing(6);

		//标准：可见性/名称/标签/图层/位置/关联轴/类型（图4；与参数栏基准线页
		//同行集。名称=树显示名，标签=线上文字渲染下一期；图层占位；类型随
		//方向只读回显置灰）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = new mCurveCheckBox(this);
		_visibleCheck->setText(QStringLiteral("可见性"));
		baseFold->addRow(_visibleCheck);
		_nameEdit = new QLineEdit(this);
		baseFold->addRow(QStringLiteral("名称"), _nameEdit);
		_labelEdit = new QLineEdit(this);
		baseFold->addRow(QStringLiteral("标签"), _labelEdit);
		_layerEdit = new QLineEdit(this);
		baseFold->addRow(QStringLiteral("图层"), _layerEdit);
		_valueSpin = createCurveDoubleSpin(6, -1e9, 1e9, 1.0, this);
		baseFold->addRow(QStringLiteral("位置"), _valueSpin);
		_linkCombo = new QComboBox(this);   //关联轴：X1/Y1 + 既有副轴（loadFromModel 动态回填）
		baseFold->addRow(QStringLiteral("关联轴"), _linkCombo);
		_typeEdit = new QLineEdit(this);
		_typeEdit->setEnabled(false);
		baseFold->addRow(QStringLiteral("类型"), _typeEdit);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});
		connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kName, _nameEdit->text());
		});
		connect(_labelEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kLabel, _labelEdit->text());
		});
		connect(_layerEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kLayer, _layerEdit->text());
		});
		connect(_valueSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kValue, v);
		});
		connect(_linkCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int)
		{
			//关联轴切换方向：X侧=纵向(固定X值)，Y侧=横向(固定Y值)；轴名入 kLink
			//（渲染暂只取方向，按具体轴定位下一期）
			const QString axis = _linkCombo->currentText();
			const bool xSide = axis.startsWith(QLatin1Char('X'));
			commit(CurveProp::kLink, axis);
			commit(CurveProp::kOrientation, xSide ? 1 : 0);
			_typeEdit->setText(xSide ? QStringLiteral("垂直") : QStringLiteral("水平"));
		});

		//显示：字体设置/标签位置/线型/线条颜色/线宽（图4；字体接 kLabelFont
		//（绘图选项同款弹窗选择，渲染下一期）；线型/线宽为绘图选项同款图标下拉）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_fontBtn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kLabelFont, f);
		});
		_labelPosCombo = new QComboBox(this);
		_labelPosCombo->addItems({
			QStringLiteral("左上"), QStringLiteral("中上"), QStringLiteral("右上"),
			QStringLiteral("左中"), QStringLiteral("居中"), QStringLiteral("右中"),
			QStringLiteral("左下"), QStringLiteral("中下"), QStringLiteral("右下"),
		});
		dispFold->addRow(QStringLiteral("标签位置"), _labelPosCombo);
		_lineStyleCombo = new mCurvePenStyleCombo(this);
		dispFold->addRow(QStringLiteral("线型"), _lineStyleCombo);
		_colorBtn = new mCurveColorButton(Qt::blue, this);
		dispFold->addRow(QStringLiteral("线条颜色"), _colorBtn);
		_lineWidthCombo = new mCurveLineWidthCombo(this);
		dispFold->addRow(QStringLiteral("线宽"), _lineWidthCombo);
		connect(_labelPosCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLabelPos, idx);
		});
		connect(_lineStyleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			//页面index 0..3 -> Qt::PenStyle 1..4（1实线 2虚线 3点线 4点划线，基准线侧）
			commit(CurveProp::kLineStyle, idx + 1);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kColor, c);
		});
		connect(_lineWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLineWidth, double(idx + 1));   //图标行0..3 -> 1..4px
		});

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addStretch(1);
	}

	void mBaselineEditorPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_nameEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kName).toString());
		_labelEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kLabel).toString());
		_layerEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kLayer).toString());
		_valueSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kValue).toDouble());
		const int orient = doc->nodeProperty(_nodeId, CurveProp::kOrientation).toInt();
		_typeEdit->setText(orient == 0 ? QStringLiteral("水平") : QStringLiteral("垂直"));
		//关联轴下拉回填：X1/Y1 + 本图表既有副轴（X2/Y2...，默认名即节点名）；
		//link 为空或失效（旧曲线名语义）时按方向回退 X1/Y1
		_linkCombo->clear();
		_linkCombo->addItem(QStringLiteral("X1"));
		_linkCombo->addItem(QStringLiteral("Y1"));
		const int chartId = doc->chartNodeId(_nodeId);
		if (chartId > 0)
		{
			for (int childId : doc->childNodeIds(chartId))
			{
				CurveNode* c = doc->node(childId);
				if (c && (c->type == CurveNodeType::XAxis2 || c->type == CurveNodeType::YAxis2))
					_linkCombo->addItem(c->name);
			}
		}
		const QString link = doc->nodeProperty(_nodeId, CurveProp::kLink).toString();
		int linkIdx = _linkCombo->findText(link);
		if (linkIdx < 0)
			linkIdx = (orient == 0) ? 1 : 0;
		_linkCombo->setCurrentIndex(linkIdx);
		_labelPosCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kLabelPos).toInt(), 8));
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kLabelFont).value<QFont>());
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kColor).value<QColor>());
		const int lineStyle = doc->nodeProperty(_nodeId, CurveProp::kLineStyle).toInt();
		_lineStyleCombo->setPenStyleIndex(qBound(0, lineStyle - 1, 3));
		//线宽图标行 = 线宽px-1（非整px宽度就近取整；越界钳制到 1..4px 档位）
		_lineWidthCombo->setLineWidthIndex(qBound(0,
			int(doc->nodeProperty(_nodeId, CurveProp::kLineWidth).toDouble() + 0.5) - 1, 3));
	}
}
