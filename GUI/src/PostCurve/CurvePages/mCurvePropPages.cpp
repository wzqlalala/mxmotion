#include "mCurvePropPages.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/mCurveWeighting.h"   //weightGain（极坐标 Y1 计权感知往返，D25 复核补全）
#include "../CurveView/mCurveChartView.h"
#include "../CurveWidgets/mCurveFoldSection.h"
#include "../CurveWidgets/mCurveEditorWidgets.h"
#include "../CurveWidgets/mCurveChannelSelector.h"
#include "../mCurveDataSourceStore.h"
#include "../mCurveImportCoordinator.h"
#include <memory>
#include "../mEditCurveDlg.h"
#include "../mImageGridComboBox.h"
#include "../../PostCommon/mPostAnnotationDialog.h"   //注释文本对话框（属性页"文本"行按钮弹出）

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QSignalBlocker>
#include <algorithm>

namespace PostCurve
{
	namespace
	{
		//极坐标 Y1 计权增益解析（D25 复核补全）：面板往返与渲染域同空间。
		//非极坐标 Y1 返 1.0；freqHz=该图首条可见曲线 xData[0]（与渲染侧同口径）。
		double polarY1WeightGain(int nodeId)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (!doc || nodeId <= 0 || doc->nodeType(nodeId) != CurveNodeType::YAxis)
				return 1.0;
			const int chartId = doc->chartNodeId(nodeId);
			CurveNode* chart = doc->node(chartId);
			if (!chart || chart->type != CurveNodeType::Chart || chart->chart.chartType != 3)
				return 1.0;
			const int wm = qBound(0, doc->nodeProperty(nodeId,
				CurveProp::kWeightMode).toInt(), 4);
			double fHz = 0.0;
			for (int childId : doc->childNodeIds(chartId))
			{
				CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::Curve && c->series.visible
					&& !c->series.xData.isEmpty())
				{
					fHz = c->series.xData.first();
					break;
				}
			}
			const double gain = weightGain(wm, fHz);
			return (qIsFinite(gain) && gain > 0.0) ? gain : 1.0;
		}

		//页面公共骨架：纵向堆叠 FoldSection 的外层布局。
		//边距/间距不显式设置，用布局默认值——与前处理参数面板
		//（mGeoPanelBase 等）同一骨架规格，参数栏切页留白一致
		QVBoxLayout* pageLayout(QWidget* host)
		{
			QVBoxLayout* layout = new QVBoxLayout(host);
			return layout;
		}

		//整行勾选框（勾选框在左、文字在右，参考软件截图的勾选行形态）
		mCurveCheckBox* createRowCheck(const QString& text, QWidget* parent)
		{
			mCurveCheckBox* check = new mCurveCheckBox(parent);
			check->setText(text);
			return check;
		}

		//对齐下拉（0中 1左 2右，与 CurveTextData::align 一致，文案对齐参考软件截图）
		QComboBox* createAlignCombo(QWidget* parent)
		{
			QComboBox* combo = new QComboBox(parent);
			combo->addItem(QStringLiteral("中"));
			combo->addItem(QStringLiteral("左"));
			combo->addItem(QStringLiteral("右"));
			return combo;
		}

		//--- mCurveDataPropPage 来源四级级联下拉 ---

		//通道下拉的选项集合：（显示文本, UserRole 数据）对
		using ChannelOpts = QVector<QPair<QString, QVariant>>;

		//各级选项与"定义曲线"文件页四级列同源同内容（数据对齐）：
		//  工况    subcaseDisplayText（"Subcase {id} (label)"，label 取首个非空
		//          $LABEL 行，与"从文件创建曲线"工况列一致）；
		//  结果类型 object[subcase] 的键（schemaName，即可装配的结果名）排序后仅留
		//          节点矢量（有分量展开者，与"定义曲线"Y 侧联动一致）；
		//  对象    "Point id {id}"；
		//  分量    数值格式（同一对象唯一；异常多格式按"定义曲线"语义跳过）展开，
		//          瞬态标志随索引叶子下发（块级判定），6/12 按此分流。
		//idx/f 任一为空一律返回空 -> 各级退化单项回显
		ChannelOpts subcaseOptions(const ListWgtStruct* idx, const PchReader::PchFile* f)
		{
			ChannelOpts opts;
			if (idx && f)
				for (int subcaseId : idx->operatingcondition)   //std::set 升序
					opts.append({ mCurveDataSourceStore::subcaseDisplayText(*f, subcaseId),
						subcaseId });
			return opts;
		}

		ChannelOpts schemaOptions(const ListWgtStruct* idx, int subcaseId)
		{
			ChannelOpts opts;
			if (idx)
			{
				//横坐标类型置首（解析出的 "Time[Sec]"/"Frequency[Hz]"，随文件数据
				//形态；选中即该侧取记录横坐标），照"从文件创建曲线"X 轴数据源列表
				for (const QString& t : idx->xresulttype)   //std::set 升序
					opts.append({ t, t });
				QStringList schemas = idx->object.value(subcaseId).keys();
				schemas.sort();
				for (const QString& schema : schemas)
					if (mCurveDataSourceStore::isVectorSchema(schema))
						opts.append({ schema, schema });
			}
			return opts;
		}

		//结果类型是否解析出的横坐标类型（"Time[Sec]"/"Frequency[Hz]"；语义同
		//"定义曲线"isAbscissaSchema——该形态下无对象/分量，侧数据取记录横坐标）
		bool isAbscissaType(const ListWgtStruct* idx, const QString& schema)
		{
			return idx && idx->xresulttype.count(schema) > 0;
		}

		ChannelOpts objectOptions(const ListWgtStruct* idx, int subcaseId, const QString& schema)
		{
			ChannelOpts opts;
			if (idx)
				for (int pointId : idx->object.value(subcaseId).value(schema))
					opts.append({ mCurveChannelSelector::objectText(pointId), pointId });
			return opts;
		}

		ChannelOpts componentOptions(const ListWgtStruct* idx, int subcaseId,
			const QString& schema, int pointId)
		{
			ChannelOpts opts;
			if (idx)
			{
				const std::set<NumericFmtInfo> fmts = idx->ycomponent.value(subcaseId)
					.value(schema).value(pointId);
				if (fmts.size() == 1)
					for (const QString& comp : mCurveDataSourceStore::expandedComponents(
						fmts.begin()->fmt, fmts.begin()->transient))
						opts.append({ comp, comp });
			}
			return opts;
		}

		//回填一级下拉（信号阻断）：选项入列并按 curData 定位现值；未命中把模型
		//现值文本补为末项选中（回显兜底：文件不在仓库/通道失配时仍是可用下拉）
		void refillChannelLevel(QComboBox* combo, const ChannelOpts& opts,
			const QVariant& curData, const QString& echoText)
		{
			const QSignalBlocker blocker(combo);
			combo->clear();
			int hit = -1;
			for (int i = 0; i < opts.size(); ++i)
			{
				combo->addItem(opts[i].first, opts[i].second);
				if (hit < 0 && opts[i].second == curData)
					hit = i;
			}
			if (hit < 0)
			{
				combo->addItem(echoText, curData);
				hit = combo->count() - 1;
			}
			combo->setCurrentIndex(hit);
		}

		//用户级联重填一级下拉（信号阻断）：只装选项选首行（无选项则清空）
		void refillChannelLevelFirst(QComboBox* combo, const ChannelOpts& opts)
		{
			const QSignalBlocker blocker(combo);
			combo->clear();
			for (int i = 0; i < opts.size(); ++i)
				combo->addItem(opts[i].first, opts[i].second);
			if (combo->count() > 0)
				combo->setCurrentIndex(0);
		}

		//--- R22 ascii（csv/txt）数据集的四级选项（与本页 pch 分支同构，仅内容不同）---
		//工况级不显示（无 "Subcase 1" 假文本）；结果类型/对象唯一项；分量=列名。
		//UserRole 仍落 id 形态常量（asciiSubcaseId/asciiPointId），落模/装配
		//五元组与 pch 同一 id 语义
		ChannelOpts asciiSchemaOpts()
		{
			ChannelOpts opts;
			const QString schema = mCurveDataSourceStore::asciiSchemaDisplayText();
			opts.append({ schema, schema });
			return opts;
		}

		ChannelOpts asciiObjectOpts()
		{
			ChannelOpts opts;
			opts.append({ mCurveDataSourceStore::asciiObjectDisplayText(),
				mCurveDataSourceStore::asciiPointId() });
			return opts;
		}

		ChannelOpts asciiComponentOpts(const QString& filePath)
		{
			ChannelOpts opts;
			const QStringList cols = mCurveDataSourceStore::getInstance()
				->asciiColumnNames(filePath);
			for (const QString& col : cols)
				opts.append({ col, col });
			return opts;
		}

		//通道装配按数据源类型收口（R22）：csv/txt 走实例版 assembleChannelData
		//（ascii 分支：component=列名首个匹配取列，yData=列值、xData=行号 1..N），
		//pch 走既有静态版（file()+块匹配，行为逐位不变）。未登记/装配失败经 err
		//带原因（pch 不在仓库的文案与原各调用点逐字一致）
		bool assembleChannelByKind(const QString& filePath, int subcaseId, const QString& schema,
			int pointId, const QString& component,
			mCurveDataSourceStore::ChannelDataResult* out, QString* err)
		{
			auto* store = mCurveDataSourceStore::getInstance();
			if (mCurveDataSourceStore::fileKind(filePath)
				== mCurveDataSourceStore::DataSourceKind::AsciiColumn)
				return store->assembleChannelData(filePath, subcaseId, schema, pointId,
					component, out, err);
			const PchReader::PchFile* dataFile = store->file(filePath);
			if (!dataFile)
			{
				if (err)
					*err = QStringLiteral("数据文件不在共享仓库：%1").arg(filePath);
				return false;
			}
			return mCurveDataSourceStore::assembleChannelData(*dataFile, subcaseId,
				schema, pointId, component, out, err);
		}

		//来源子工况显示串按数据源类型分流：pch="Subcase {id}"（既有口径）；
		//ascii=空串（模型不落假工况文本，回显显示占位"-"，标签模板
		//{y.MXSubcase} 解析为空——与 T1 subcaseDisplayText 路径版同口径）
		QString sourceSubcaseText(const QString& filePath, int subcaseId)
		{
			if (mCurveDataSourceStore::fileKind(filePath)
				== mCurveDataSourceStore::DataSourceKind::AsciiColumn)
				return QString();
			return mCurveChannelSelector::subcaseText(subcaseId);
		}

		//来源对象显示串按数据源类型分流：pch="Point id {id}"；ascii="Block 1"
		//（需求口径：来源五元组按 csv/txt 语义落模 文件/Unknown/Block 1/列名）
		QString sourceObjectText(const QString& filePath, int pointId)
		{
			if (mCurveDataSourceStore::fileKind(filePath)
				== mCurveDataSourceStore::DataSourceKind::AsciiColumn)
				return mCurveDataSourceStore::asciiObjectDisplayText();
			return mCurveChannelSelector::objectText(pointId);
		}
	}

	//--- mPagePropPage ---

	mPagePropPage::mPagePropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mPagePropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：标题/页面布局（截图01；页面布局=视口布局图标网格，与 mOpenCurveDlg
		//布局下拉旁的网格选择同款控件，索引入 kLayout——主窗口布局同步据此重排
		//对应视图页（格数多于图表数时余格留空），与视图显示栏布局按钮三方互通）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		//标题 = 页面标题模板文本按钮（Y 轴"文本"行同款：点击弹"编辑字符"弹窗，支持
		//{p1w1c1.y.MX*} 占位符渲染时解析；值入 kPageTitle——与 kName 页名分离，
		//编辑不动树显示名。模板由"打开曲线"建图时按首条曲线自动生成）
		_titleBtn = new mCurveTextButton(this);
		baseFold->addRow(QStringLiteral("标题"), _titleBtn);
		_layoutGrid = new MechMainWindow::mImageGridComboBox(this);
		_layoutGrid->setFixedSize(30, 30);
		_layoutGrid->setIconSize(QSize(20, 20));
		_layoutGrid->setImages(curveViewportIconPaths(), 4);
		baseFold->addRow(QStringLiteral("页面布局"), _layoutGrid);
		connect(_titleBtn, &mCurveTextButton::textCommitted, this, [this](const QString& text)
		{
			commit(CurveProp::kPageTitle, text);
		});
		connect(_layoutGrid, &MechMainWindow::mImageGridComboBox::currentIndexChanged,
			this, [this](int idx)
		{
			commit(CurveProp::kLayout, idx);
		});

		//显示：字体设置（绘图选项同款交互：按钮弹"选择字体"对话框，整只 QFont 提交）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_fontBtn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFont, f);
		});

		//基础：发布页面/发布页面标题
		mCurveFoldSection* pubFold = new mCurveFoldSection(QStringLiteral("基础"), this);
		_publishCheck = createRowCheck(QStringLiteral("发布页面"), this);
        _publishCheck->setEnabled(false);
		pubFold->addRow(_publishCheck);
		_publishTitleCheck = createRowCheck(QStringLiteral("发布页面标题"), this);
        _publishTitleCheck->setEnabled(false);
		pubFold->addRow(_publishTitleCheck);
		connect(_publishCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kPublishLayout, on);
		});
		connect(_publishTitleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kPublishPageTitle, on);
		});

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addWidget(pubFold);
		layout->addStretch(1);
	}

	void mPagePropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_titleBtn->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kPageTitle).toString());
		{
			//setCurrentIndex 同值时本就不发信号，屏蔽兜底（防切换节点时的同值回环）
			QSignalBlocker blocker(_layoutGrid);
			_layoutGrid->setCurrentIndex(doc->nodeProperty(_nodeId, CurveProp::kLayout).toInt());
		}
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFont).value<QFont>());
		_publishCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kPublishLayout).toBool());
		_publishTitleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kPublishPageTitle).toBool());
	}

	//--- mChartPropPage ---

	mChartPropPage::mChartPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mChartPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//基础：表头可见/表头/图表类型（截图02；表头数据挂本图表节点，与页眉属性页、
		//树右键页眉显隐同键联动）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("基础"), this);
		_headerVisibleCheck = createRowCheck(QStringLiteral("表头可见"), this);
		baseFold->addRow(_headerVisibleCheck);
		//表头=按钮弹"编辑字符"弹窗（模板文本长，行内不放输入框）
		_headerTextBtn = new mCurveTextButton(this);
		baseFold->addRow(QStringLiteral("表头"), _headerTextBtn);
		_typeCombo = new QComboBox(this);
		setupChartTypeCombo(_typeCombo);   //5类（图3）；XY/柱状/复数已接渲染，其余置灰占位
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

		//高级：背景颜色/网格线颜色
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

	void mChartPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_headerVisibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kHeaderVisible).toBool());
		_headerTextBtn->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderText).toString());
		//R30：类型回显只回填不触发切换——曾因未阻塞信号，chartType=5（三维图）
		//被 qBound(0,5,4) 钳到 4，combo 首载 0→4 发 currentIndexChanged 走真实
		//切换（5→蛛网图4），首建流程（创建宏中途同步加载本页，hasCurve=false
		//无确认框静默提交）被毒化为"XY Plot+XY 骨架"树+不刷新 3D 空盒。
		//三维图回显第 6 项"三维图"（置灰项，仅显示，修复外观遗留"显示蛛网图"）；
		//其余类型维持既有 qBound(0,t,4) 钳制（0..4 档与 5 类下拉对齐）
		{
			const QSignalBlocker blocker(_typeCombo);
			const int t = doc->nodeProperty(_nodeId, CurveProp::kChartType).toInt();
			_typeCombo->setCurrentIndex(t == 5 ? 5 : qBound(0, t, 4));
		}
		_bgColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBackgroundColor).value<QColor>());
		_gridColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kGridColor).value<QColor>());
	}

	//--- mCurveSeriesPropPage ---

	mCurveSeriesPropPage::mCurveSeriesPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mCurveSeriesPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		baseFold->addRow(_visibleCheck);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});

		//曲线标签：显示标签/标签内容/显示标签前缀·标签前缀/显示标签后缀·标签后缀
		//标签内容/前缀/后缀为模板下拉——点箭头勾选来源变量按"~"拼接，
		//编辑区可直接输入；取消"显示标签"下方设置项置灰；标签渲染待实现）
		_labelFold = new mCurveFoldSection(QStringLiteral("曲线标签"), this);
		_labelVisibleCheck = createRowCheck(QStringLiteral("显示标签"), this);
		_labelFold->addRow(_labelVisibleCheck);
		_labelContentCombo = new mCurveTemplateCombo(this);
		_labelFold->addRow(QStringLiteral("标签内容"), _labelContentCombo);
		_labelPrefixCheck = createRowCheck(QStringLiteral("显示标签前缀"), this);
		_labelFold->addRow(_labelPrefixCheck);
		_labelPrefixEdit = new mCurveTemplateCombo(this);
		_labelFold->addRow(QStringLiteral("标签前缀"), _labelPrefixEdit);
		_labelSuffixCheck = createRowCheck(QStringLiteral("显示标签后缀"), this);
		_labelFold->addRow(_labelSuffixCheck);
		_labelSuffixEdit = new mCurveTemplateCombo(this);
		_labelFold->addRow(QStringLiteral("标签后缀"), _labelSuffixEdit);
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
		//（线型/线宽/符号样式均为绘图选项曲线页同款图标下拉，见 mCurveEditorWidgets）
		mCurveFoldSection* lineFold = new mCurveFoldSection(QStringLiteral("线条属性"), this);
		_lineStyleCombo = new mCurvePenStyleCombo(this);
		lineFold->addRow(QStringLiteral("线型"), _lineStyleCombo);
		_colorBtn = new mCurveColorButton(Qt::blue, this);
		lineFold->addRow(QStringLiteral("线条颜色"), _colorBtn);
		_lineWidthCombo = new mCurveLineWidthCombo(this);
		lineFold->addRow(QStringLiteral("线宽"), _lineWidthCombo);
		_areaFillCheck = createRowCheck(QStringLiteral("区域填充"), this);
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
		connect(_lineWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLineWidth, double(idx + 1));   //图标行0..3 -> 1..4px
		});
		connect(_areaFillCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAreaFill, on);
		});

		//符号属性：符号样式（绘图选项曲线页同款符号图标下拉；渲染层按样式>0
		//绘制符号，见 mCurveChartView 符号派生）。选非空标记后现出下方三行：
		//符号颜色/符号密度（标记间隔，1=逐点）/缩放尺寸（实际大小=基准大小
		//大圆点12px·其余8px × 该系数），选"无"整组隐藏
		_symbolFold = new mCurveFoldSection(QStringLiteral("符号属性"), this);
		_symbolStyleCombo = new mCurveSymbolStyleCombo(this);
		_symbolFold->addRow(QStringLiteral("符号样式"), _symbolStyleCombo);
		_symbolColorBtn = new mCurveColorButton(QColor(204, 0, 0), this);
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
		connect(_symbolColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kSymbolColor, c);
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

		updateLabelStates();
		updateSymbolRows();

		layout->addWidget(baseFold);
		layout->addWidget(_labelFold);
		layout->addWidget(lineFold);
		layout->addWidget(_symbolFold);
		layout->addStretch(1);
	}

	void mCurveSeriesPropPage::updateSymbolRows()
	{
		//参考交互：仅选了非空标记后现出符号设置行（"无"=整组隐藏）
		const bool hasSymbol = _symbolStyleCombo->currentIndex() > 0;
		_symbolFold->setRowVisible(_symbolColorBtn, hasSymbol);
		_symbolFold->setRowVisible(_densitySpin, hasSymbol);
		_symbolFold->setRowVisible(_scaleSpin, hasSymbol);
	}

	void mCurveSeriesPropPage::updateLabelStates()
	{
		//截图交互：取消"显示标签"则下方标签设置项全部置灰（不隐藏，标签随曲线渲染隐藏）；
		//前缀/后缀开关各自控制其模板下拉的可用态。带行标签的三行走 setRowEnabled，
		//行标签一并置灰（只禁编辑器时行标签保持黑字，整行看起来仍可用）
		const bool labelOn = _labelVisibleCheck->isChecked();
		_labelFold->setRowEnabled(_labelContentCombo, labelOn);
		_labelPrefixCheck->setEnabled(labelOn);
		_labelSuffixCheck->setEnabled(labelOn);
		_labelFold->setRowEnabled(_labelPrefixEdit, labelOn && _labelPrefixCheck->isChecked());
		_labelFold->setRowEnabled(_labelSuffixEdit, labelOn && _labelSuffixCheck->isChecked());
	}

	void mCurveSeriesPropPage::loadFromModel()
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
		//线宽图标行 = 线宽px-1（非整px宽度就近取整；越界钳制到 1..4px 档位）
		_lineWidthCombo->setLineWidthIndex(qBound(0,
			int(doc->nodeProperty(_nodeId, CurveProp::kLineWidth).toDouble() + 0.5) - 1, 3));
		_areaFillCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAreaFill).toBool());
		_symbolStyleCombo->setSymbolStyleIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kSymbolStyle).toInt(), 9));
		_symbolColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kSymbolColor).value<QColor>());
		_densitySpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kSymbolDensity).toInt());
		_scaleSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kSymbolScale).toDouble());
		updateLabelStates();
		updateSymbolRows();
	}

	//--- mCurveDataPropPage ---

	mCurveDataPropPage::mCurveDataPropPage(int side, QWidget* parent)
		: mCurvePropPageBase(parent)
		, _side(side)
	{
		buildUi();

		//副轴增删（树右键"添加坐标轴"/"删除坐标轴"/撤销重做）-> 坐标轴下拉重填：
		//选项来自图表实际存在的轴，用户加了 X2/Y2 下拉才出现对应项。删除在
		//aboutToBeRemoved 判定归属（removed 时节点已销毁无从判断类型），重填
		//延迟到事件循环——届时节点已移除，重填结果不再含被删轴；经 setNodeId
		//走 _loading 保护，不触发 commit 回写
		auto reloadIfAxisChanged = [this](int nodeId)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			CurveNode* n = doc ? doc->node(nodeId) : nullptr;
			if (!n || (n->type != CurveNodeType::XAxis2 && n->type != CurveNodeType::YAxis2))
				return;
			if (doc->chartNodeId(_nodeId) != doc->parentNodeId(nodeId))
				return;
			QTimer::singleShot(0, this, [this]()
			{
				setNodeId(_nodeId);
			});
		};
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAdded,
			this, reloadIfAxisChanged);
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeAboutToBeRemoved,
			this, reloadIfAxisChanged);
	}

	void mCurveDataPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//数据：数据源(占位)/文件+更改/子工况/结果类型/对象/分量（来源字段只读回显，"更改"打开数据文件
        //（pch/csv/txt，"从文件创建曲线"同款文件对话框）后按默认通道直接换源，不再弹通道选择器；
        //ascii（csv/txt）数据集按三级口径回显：子工况占位"-"禁用/结果类型 Unknown/对象 Block 1/分量=列名）
		mCurveFoldSection* dataFold = new mCurveFoldSection(QStringLiteral("数据"), this);
		_sourceTypeCombo = new QComboBox(this);
		_sourceTypeCombo->addItem(QStringLiteral("外部文件"));   //占位：表达式等数据源
		dataFold->addRow(QStringLiteral("数据源"), _sourceTypeCombo);
		_sourceFileEdit = new QLineEdit(this);
		_sourceFileEdit->setReadOnly(true);
		_changeBtn = new QPushButton(this);
		_changeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")));
		_changeBtn->setFixedSize(24, 24);   //图标按钮统一规格（同"应用样式"树工具钮）
		_changeBtn->setToolTip(QStringLiteral(
			"打开数据文件（pch/csv/txt）并换源到其默认通道（数据直写不进命令栈，可经\"编辑曲线-撤回\"恢复；通道可在下方四级下拉中调整）"));
		connect(_changeBtn, &QPushButton::clicked, this, &mCurveDataPropPage::changeSource);
		QWidget* fileRow = new QWidget(this);
		QHBoxLayout* fileLay = new QHBoxLayout(fileRow);
		fileLay->setContentsMargins(0, 0, 0, 0);
		fileLay->setSpacing(4);
		_sourceFileEdit->setParent(fileRow);
		_changeBtn->setParent(fileRow);
		fileLay->addWidget(_sourceFileEdit, 1);
		fileLay->addWidget(_changeBtn);
		dataFold->addRow(QStringLiteral("文件"), fileRow);
		_subcaseCombo = new QComboBox(this);
		dataFold->addRow(QStringLiteral("子工况"), _subcaseCombo);
		_schemaCombo = new QComboBox(this);
		dataFold->addRow(QStringLiteral("结果类型"), _schemaCombo);
		_objectCombo = new QComboBox(this);
		dataFold->addRow(QStringLiteral("对象"), _objectCombo);
		_componentCombo = new QComboBox(this);
		dataFold->addRow(QStringLiteral("分量"), _componentCombo);
		//四级级联下拉换源：任一级用户改动 -> onChannelComboChanged 重填下级 +
		//通道链完整即装配落模（"更改..."的内联形态，同一落模链路）
		connect(_subcaseCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mCurveDataPropPage::onChannelComboChanged);
		connect(_schemaCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mCurveDataPropPage::onChannelComboChanged);
		connect(_objectCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mCurveDataPropPage::onChannelComboChanged);
		connect(_componentCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mCurveDataPropPage::onChannelComboChanged);

		//标准：缩放系数/偏移量/起始值/最大值/最小值/用户表达式/坐标轴
		//（起始值=本侧数据首值只读回显；最大/最小值=数据范围置灰回显；
		//用户表达式置灰占位）
		mCurveFoldSection* stdFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_scaleSpin = createCurveDoubleSpin(6, -1e9, 1e9, 0.1, this);
		_scaleSpin->setValue(1.0);
		stdFold->addRow(QStringLiteral("缩放系数"), _scaleSpin);
		_offsetSpin = createCurveDoubleSpin(6, -1e9, 1e9, 1.0, this);
		_offsetSpin->setValue(0.0);
		stdFold->addRow(QStringLiteral("偏移量"), _offsetSpin);
		_startSpin = createCurveDoubleSpin(6, -1e9, 1e9, 1.0, this);
		_startSpin->setReadOnly(true);   //数据首值回显，不可编辑
		stdFold->addRow(QStringLiteral("起始值"), _startSpin);
		_maxSpin = createCurveDoubleSpin(6, -1e9, 1e9, 1.0, this);
		_maxSpin->setEnabled(false);     //数据范围回显，置灰
		stdFold->addRow(QStringLiteral("最大值"), _maxSpin);
		_minSpin = createCurveDoubleSpin(6, -1e9, 1e9, 1.0, this);
		_minSpin->setEnabled(false);
		stdFold->addRow(QStringLiteral("最小值"), _minSpin);
        // 对应的标签文字也禁用（置灰）
        stdFold->setRowEnabled(_maxSpin, false);
        stdFold->setRowEnabled(_minSpin, false);

		_expressionEdit = new QLineEdit(this);
		_expressionEdit->setEnabled(false);   //占位：待表达式数据源实现后接入
		stdFold->addRow(QStringLiteral("用户表达式"), _expressionEdit);
		//坐标轴行仅 X/Y 数据壳有：yp 侧（复数下面板）整行不建；ym 壳（复数曲线
		//Y 侧）在 loadFromModel 按父曲线是否复数动态隐藏。选项在 loadFromModel
		//按图表实际存在的轴生成：主轴恒在，副轴项=用户经"添加坐标轴"创建的
		//副轴节点（加了 X2/Y2 才出现，未加不显示）
		if (_side != 2)
		{
			_axisAssignCombo = new QComboBox(this);
			stdFold->addRow(QStringLiteral("坐标轴"), _axisAssignCombo);
			connect(_axisAssignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
				this, [this](int idx)
			{
				commit(CurveProp::kAxisAssign, idx);
			});
		}
		_stdFold = stdFold;   //成员留存：ym 壳坐标轴行按父曲线复数与否整行显隐
		connect(_scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kScale, v);
		});
		connect(_offsetSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kOffset, v);
		});

		//单位：类型(置灰占位)/量纲
		mCurveFoldSection* unitFold = new mCurveFoldSection(QStringLiteral("单位"), this);
		_unitTypeEdit = new QLineEdit(this);
		_unitTypeEdit->setEnabled(false);   //占位（参考软件置灰形态）
		unitFold->addRow(QStringLiteral("类型"), _unitTypeEdit);
		_unitEdit = new QLineEdit(this);
        _unitEdit->setEnabled(false);
		unitFold->addRow(QStringLiteral("量纲"), _unitEdit);
        // 输入框禁用（变灰、不可点击、不可编辑）
        
        // 对应的标签文字也禁用（置灰）
        unitFold->setRowEnabled(_unitTypeEdit, false);
        unitFold->setRowEnabled(_unitEdit, false);

		connect(_unitEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kUnit, _unitEdit->text());
		});

		layout->addWidget(dataFold);
		layout->addWidget(stdFold);
		layout->addWidget(unitFold);
		layout->addStretch(1);
	}

	void mCurveDataPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_sourceFileEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kSourceFile).toString());
		//子工况/结果类型/对象/分量：四级级联下拉回填——仓库有该文件时列出可选
		//通道并定位到现值（失配层级补模型现值文本兜底），否则单项回显
		refillChannelCombos();
		_expressionEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kExpression).toString());
		_scaleSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kScale).toDouble());
		_offsetSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kOffset).toDouble());

		//起始值/最大值/最小值：本侧数据首值与范围按 缩放系数/偏移量 变换后的
		//显示值回显（与曲线渲染同源，改缩放/偏移即联动刷新；无数据时为 0）
		double first = 0.0, maxV = 0.0, minV = 0.0;
		if (CurveNode* shell = doc->node(_nodeId))
		{
			if (CurveNode* curve = shell->parent)
			{
				//侧别：0=X（xRef/xData）、1=Y（yRef/yData）、2=复数下面板
				//（complexRef/complexYData，"yp" 壳）
				const CurveDataRefData& ref = (_side == 0)
					? curve->series.xRef : (_side == 1)
					? curve->series.yRef : curve->series.complexRef;
				const QVector<double>& data = (_side == 0)
					? curve->series.xData : (_side == 1)
					? curve->series.yData : curve->series.complexYData;
				if (!data.isEmpty())
				{
					first = maxV = minV = ref.applyTo(data.first());
					for (double v : data)
					{
						const double t = ref.applyTo(v);
						maxV = qMax(maxV, t);
						minV = qMin(minV, t);
					}
				}
			}
		}
		_startSpin->setValue(first);
		_maxSpin->setValue(maxV);
		_minSpin->setValue(minV);

		if (_axisAssignCombo)
		{
			//坐标轴下拉按图表实际存在的轴生成（设计图12）：主轴恒在，副轴项=
			//用户经"添加坐标轴"创建的副轴节点（项文本取显示名——复数图 Y2 树上
			//显示 "Phase"，未添加的不出现）；现值越界（副轴被撤销）时钳到现存末项。
			//隐藏行仍照常回填：页面实例按类型缓存复用（y/ym 同类型共用），换绑
			//节点时靠这里恢复
			QSignalBlocker blocker(_axisAssignCombo);
			_axisAssignCombo->clear();
			_axisAssignCombo->addItem(_side == 1 ? QStringLiteral("Y1") : QStringLiteral("X1"));
			const int chartId = doc->chartNodeId(_nodeId);
			const CurveNodeType secondType = _side == 1 ? CurveNodeType::YAxis2 : CurveNodeType::XAxis2;
			for (int childId : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(childId) == secondType)
					_axisAssignCombo->addItem(doc->nodeDisplayName(childId));
			}
			_axisAssignCombo->setCurrentIndex(qBound(0,
				doc->nodeProperty(_nodeId, CurveProp::kAxisAssign).toInt(),
				_axisAssignCombo->count() - 1));

			//ym 壳（复数曲线 Y 侧）整行隐藏：父曲线带 yp 壳（CurveY2Data）即复数
			//曲线，其 Y 侧不显示坐标轴行；普通曲线 y 壳保留（副轴挂轴唯一入口）
			bool onComplex = false;
			if (CurveNode* shell = doc->node(_nodeId))
				if (CurveNode* curve = shell->parent)
					for (int childId : doc->childNodeIds(curve->id))
						if (doc->nodeType(childId) == CurveNodeType::CurveY2Data)
						{
							onComplex = true;
							break;
						}
			_stdFold->setRowVisible(_axisAssignCombo, !onComplex);
		}
		_unitEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kUnit).toString());
	}

	void mCurveDataPropPage::changeSource()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;

		//第一步：打开文件对话框选新数据文件（与"从文件创建曲线"打开文件同款：
		//标题"打开文件"；R22 起过滤含 csv/txt 列数据；目录沿用当前来源文件
		//所在目录，无来源时沿用已导入数据集目录，再无则 C://）
		const QString curFile = doc->nodeProperty(_nodeId, CurveProp::kSourceFile).toString();
		QString dir;
		if (!curFile.isEmpty())
			dir = QFileInfo(curFile).absolutePath();
		else
		{
			const QStringList paths = mCurveDataSourceStore::getInstance()->filePaths();
			dir = paths.isEmpty()
				? QStringLiteral("C://") : QFileInfo(paths.first()).absolutePath();
		}
		const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件"),
			dir, QStringLiteral("数据文件(*.pch *.csv *.txt);;PCH File(*.pch);;"
				"CSV File(*.csv);;TXT File(*.txt)"));
		if (filePath.isEmpty())
			return;

		//共享仓库解析登记（幂等，已导入不重复解析；与"从文件创建曲线"同一入口）。
		//导入异步化（FR-01）：解析移后台线程，成功/失败续接移入
		//sig_importFinished 分支；已登记走原幂等成功链路（C1）。
		//协调器须在 addFileAsync 之前就位（否则首个 sig_importStarted 无订阅方）
		mCurveDocController* controller = mCurveDocController::getInstance();
		mCurveImportCoordinator* coordinator = mCurveImportCoordinator::getInstance(window());
		auto* store = mCurveDataSourceStore::getInstance();
		const auto accepted = store->addFileAsync(filePath);
		if (accepted == mCurveDataSourceStore::AddFileAsyncResult::AlreadyRegistered)
		{
			applyImportedSource(filePath);
			return;
		}

		//Accepted/DuplicateInFlight：解析在后台执行，成败经信号续接。接收上下文
		//=this（C3：解析中可关闭属性页——D5，Qt 随对象销毁自动断连防悬垂）；
		//按键匹配防多导入串扰；命中后自断连
		if (accepted == mCurveDataSourceStore::AddFileAsyncResult::DuplicateInFlight)
			coordinator->notifyDuplicateInFlight(filePath);
		const QString key = mCurveDataSourceStore::normalizePath(filePath);
		const auto conn = std::make_shared<QMetaObject::Connection>();
		*conn = connect(store, &mCurveDataSourceStore::sig_importFinished, this,
			[this, key, filePath, conn](const QString& finishedKey, bool ok, const QString& err)
			{
				if (finishedKey != key)
					return;
				QObject::disconnect(*conn);
				if (!ok)
				{
					mCurveDocController::getInstance()->reportError(
						QStringLiteral("数据文件导入失败：%1").arg(err));
					return;
				}
				applyImportedSource(filePath);
			});
	}

	void mCurveDataPropPage::applyImportedSource(const QString& filePath)
	{
		mCurveDocController* controller = mCurveDocController::getInstance();
		//原成功续接链路（逐字保留）
		controller->reportInfo(QStringLiteral("已导入数据集：%1")
			.arg(QFileInfo(filePath).fileName()));

		//--- R22：csv/txt 列数据——固定默认通道落 [Unknown]/[Block 1]/第 1 列 ---
		//（工况内部 id 恒 1 不落 UI；X 默认第 1 列口径用户拍板 20261109，由
		//commitChannelChange 的 ascii 分支落侧来源；通道细节随后在本页四级
		//下拉中调整——列名清单即分量下拉选项）
		if (mCurveDataSourceStore::fileKind(filePath)
			== mCurveDataSourceStore::DataSourceKind::AsciiColumn)
		{
			const QStringList cols = mCurveDataSourceStore::getInstance()
				->asciiColumnNames(filePath);
			if (cols.isEmpty())
			{
				controller->reportWarning(QStringLiteral("[更改曲线] 文件无可用数据列：%1")
					.arg(QFileInfo(filePath).fileName()));
				return;
			}
			ChannelSelection picked;
			picked.filePath = mCurveDataSourceStore::normalizePath(filePath);
			picked.subcaseId = mCurveDataSourceStore::asciiSubcaseId();
			picked.schema = mCurveDataSourceStore::asciiSchemaDisplayText();
			picked.pointId = mCurveDataSourceStore::asciiPointId();
			picked.component = cols.first();
			picked.complete = true;
			//复数曲线不做成对换源：csv/txt 无复数分量对（siblingComplexComponent
			//恒空），整线换源拒绝（复数曲线换源仍须选 pch 的 MAG/PHA 成对分量）
			CurveDocument* docPick = mCurveDocController::getInstance()->document();
			const int curveIdOfPick = docPick ? docPick->parentNodeId(_nodeId) : 0;
			CurveNode* pickCurve = curveIdOfPick > 0 ? docPick->node(curveIdOfPick) : nullptr;
			if (pickCurve && pickCurve->series.isComplex())
			{
				controller->reportWarning(QStringLiteral(
					"[更改曲线] 复数曲线不支持换源到 csv/txt 列数据（无 MAG/PHA 等复数分量对）"));
				return;
			}
			commitChannelChange(picked);
			return;
		}

		//第二步：不再弹通道选择器——四级各取第一项（子工况升序首个 -> 矢量结果
		//类型排序首个 -> 对象升序首个 -> 分量展开首个）直接换源；通道细节可随后
		//在本页四级下拉中调整（pch 数据集既有口径，逐位保留）
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(filePath);
		int subcaseId = 0;
		QString schema;
		int pointId = 0;
		QString component;
		if (idx && !idx->operatingcondition.empty())
		{
			subcaseId = *idx->operatingcondition.begin();   //std::set 升序首个
			QStringList schemas = idx->object.value(subcaseId).keys();
			schemas.sort();
			for (const QString& s : schemas)
				if (mCurveDataSourceStore::isVectorSchema(s))
				{
					schema = s;   //矢量结果类型排序首个（有分量展开者）
					break;
				}
			const std::set<int>& points = idx->object.value(subcaseId).value(schema);
			if (!points.empty())
				pointId = *points.begin();   //对象升序首个
			const std::set<NumericFmtInfo> fmts = idx->ycomponent.value(subcaseId)
				.value(schema).value(pointId);
			if (fmts.size() == 1)
			{
				const QStringList comps = mCurveDataSourceStore::expandedComponents(
					fmts.begin()->fmt, fmts.begin()->transient);
				if (!comps.isEmpty())
					component = comps.first();   //分量展开首个
			}
		}
		if (subcaseId <= 0 || schema.isEmpty() || pointId <= 0 || component.isEmpty())
		{
			controller->reportWarning(QStringLiteral("[更改曲线] 文件无可用矢量结果通道：%1")
				.arg(QFileInfo(filePath).fileName()));
			return;
		}

		ChannelSelection picked;
		picked.filePath = mCurveDataSourceStore::normalizePath(filePath);
		picked.subcaseId = subcaseId;
		picked.schema = schema;
		picked.pointId = pointId;
		picked.component = component;
		picked.complete = true;
		//复数曲线"更改…"=整线换文件重建（成对换源，X 回取横坐标——文件级
		//操作语义，三侧独立微调走四级下拉）；XY 曲线维持整线换源
		CurveDocument* docPick = mCurveDocController::getInstance()->document();
		const int curveIdOfPick = docPick ? docPick->parentNodeId(_nodeId) : 0;
		CurveNode* pickCurve = curveIdOfPick > 0 ? docPick->node(curveIdOfPick) : nullptr;
		if (pickCurve && pickCurve->series.isComplex())
			commitComplexChannelChange(picked);
		else
			commitChannelChange(picked);
	}

	void mCurveDataPropPage::refillChannelCombos()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const QString filePath = doc->nodeProperty(_nodeId, CurveProp::kSourceFile).toString();
		//侧来源回显（kSubcase/kSchema/kObject/kComponent 经壳节点按侧重定向；
		//"Subcase %1"/"Point id %1" 取尾数字反解）
		const ChannelSelection cur = mCurveChannelSelector::fromModelTexts(filePath,
			doc->nodeProperty(_nodeId, CurveProp::kSubcase).toString(),
			doc->nodeProperty(_nodeId, CurveProp::kSchema).toString(),
			doc->nodeProperty(_nodeId, CurveProp::kObject).toString(),
			doc->nodeProperty(_nodeId, CurveProp::kComponent).toString());
		//仓库无该文件（未导入/数学运算/数值来源）时各级选项为空 -> 单项回显
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(filePath);
		const PchReader::PchFile* f = mCurveDataSourceStore::getInstance()->file(filePath);

		//X 侧旧数据未记录侧来源（schema 空）时按横坐标形态回显：结果类型落
		//解析出的首个横坐标类型
		QString wantSchema = cur.schema;
		if (wantSchema.isEmpty() && idx && !idx->xresulttype.empty())
			wantSchema = *idx->xresulttype.begin();

		//--- R22：ascii（csv/txt）来源——三级口径回显 ---
		//子工况单项占位"-"并禁用（ascii 无工况级语义，不得显示假 "Subcase 1"）；
		//结果类型/对象唯一项（显示 Unknown/Block 1，UserRole=id 形态常量）；
		//分量=列名清单按现值定位，失配补模型现值末项兜底（同 pch 模式）
		if (mCurveDataSourceStore::fileKind(filePath)
			== mCurveDataSourceStore::DataSourceKind::AsciiColumn)
		{
			refillChannelLevel(_subcaseCombo,
				ChannelOpts{ { QStringLiteral("-"),
					QVariant(mCurveDataSourceStore::asciiSubcaseId()) } },
				QVariant(mCurveDataSourceStore::asciiSubcaseId()), QStringLiteral("-"));
			_subcaseCombo->setEnabled(false);
			refillChannelLevel(_schemaCombo, asciiSchemaOpts(),
				QVariant(cur.schema.isEmpty() ? mCurveDataSourceStore::asciiSchemaDisplayText()
					: cur.schema),
				doc->nodeProperty(_nodeId, CurveProp::kSchema).toString());
			refillChannelLevel(_objectCombo, asciiObjectOpts(), QVariant(cur.pointId),
				doc->nodeProperty(_nodeId, CurveProp::kObject).toString());
			refillChannelLevel(_componentCombo, asciiComponentOpts(filePath),
				QVariant(cur.component),
				doc->nodeProperty(_nodeId, CurveProp::kComponent).toString());
			_objectCombo->setEnabled(true);
			_componentCombo->setEnabled(true);
			return;
		}
		//pch 路径（四级联动口径不变）：页面实例按类型缓存复用，从 ascii 来源
		//节点换绑回来时恢复工况级可用
		_subcaseCombo->setEnabled(true);

		//逐级取选项并定位现值；下级选项按上级实际选中（可能是失配回显项）计算
		refillChannelLevel(_subcaseCombo, subcaseOptions(idx, f), cur.subcaseId,
			doc->nodeProperty(_nodeId, CurveProp::kSubcase).toString());
		const int subcaseId = _subcaseCombo->currentData().toInt();
		refillChannelLevel(_schemaCombo, schemaOptions(idx, subcaseId), wantSchema,
			doc->nodeProperty(_nodeId, CurveProp::kSchema).toString());
		const QString schema = _schemaCombo->currentData().toString();
		const bool abscissa = isAbscissaType(idx, schema);
		//横坐标类型形态无对象/分量（侧数据=记录横坐标）：清空回显；子工况
		//保持可用（横坐标形态下仍可切换锚工况）
		refillChannelLevel(_objectCombo,
			abscissa ? ChannelOpts() : objectOptions(idx, subcaseId, schema), cur.pointId,
			doc->nodeProperty(_nodeId, CurveProp::kObject).toString());
		const int pointId = _objectCombo->currentData().toInt();
		refillChannelLevel(_componentCombo,
			abscissa ? ChannelOpts() : componentOptions(idx, subcaseId, schema, pointId),
			cur.component, doc->nodeProperty(_nodeId, CurveProp::kComponent).toString());
		//横坐标类型形态：对象/分量不参与（置灰，照"从文件创建曲线"X 轴数据源
		//选中横坐标类型时的列置灰）；子工况下拉恒可用
		_objectCombo->setEnabled(!abscissa);
		_componentCombo->setEnabled(!abscissa);
	}

	void mCurveDataPropPage::onChannelComboChanged()
	{
		if (_loading)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const QString filePath = doc->nodeProperty(_nodeId, CurveProp::kSourceFile).toString();
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(filePath);
		if (!idx)
			return;   //非仓库来源：各级仅单项回显，无级联无换源

		//本级已由用户选好，逐级重填下级（选首行）；分量是末级无下级。
		//R22：ascii 数据集按三级口径取选项（结果类型/对象唯一项、分量=列名，
		//收口在文件级 asciiSrc 判定一处）
		const bool asciiSrc = mCurveDataSourceStore::fileKind(filePath)
			== mCurveDataSourceStore::DataSourceKind::AsciiColumn;
		const QObject* changed = sender();
		if (changed == _subcaseCombo)
		{
			//换工况尽量保留结果类型（该工况下仍可选时）
			const QString keepSchema = _schemaCombo->currentData().toString();
			refillChannelLevel(_schemaCombo,
				asciiSrc ? asciiSchemaOpts() : schemaOptions(idx, _subcaseCombo->currentData().toInt()),
				keepSchema, keepSchema);
		}

		//本页所属曲线（横坐标形态与矢量形态路由都要按侧/复数判定）
		const int curveIdOfPick = doc->parentNodeId(_nodeId);
		CurveNode* pickCurve = curveIdOfPick > 0 ? doc->node(curveIdOfPick) : nullptr;
		const bool complexPick = pickCurve && pickCurve->series.isComplex();

		//横坐标类型形态：该侧取记录横坐标（对象/分量不参与，置灰清空，照
		//"从文件创建曲线"X 轴数据源选中横坐标类型时的列置灰）；子工况保持
		//可用——切换锚工况后重取该工况下锚通道数据块的记录横坐标
		const QString schema = _schemaCombo->currentData().toString();
		if (isAbscissaType(idx, schema))
		{
			refillChannelLevelFirst(_objectCombo, {});
			refillChannelLevelFirst(_componentCombo, {});
			_objectCombo->setEnabled(false);
			_componentCombo->setEnabled(false);
			//三侧全独立（D7）：复数曲线 ym/yp 页横坐标形态=本侧取记录横坐标
			//（XY 曲线的 commitSideAbscissa 写共享 X/Y 壳，对复数 yp 页不可见
			//——"第 0 个选不中"即此）；X 页/XY 曲线照旧
			if (_side != 0 && complexPick)
				commitComplexSideAbscissa(schema, _subcaseCombo->currentData().toInt());
			else
				commitSideAbscissa(schema, _subcaseCombo->currentData().toInt());
			return;
		}

		//实际结果类型形态：恢复对象/分量参与级联
		_objectCombo->setEnabled(true);
		_componentCombo->setEnabled(true);
		QString schemaRef = schema;
		int pointId = 0;
		if (changed == _subcaseCombo || changed == _schemaCombo)
		{
			refillChannelLevelFirst(_objectCombo, asciiSrc ? asciiObjectOpts()
				: objectOptions(idx, _subcaseCombo->currentData().toInt(), schemaRef));
			pointId = _objectCombo->currentData().toInt();
			refillChannelLevelFirst(_componentCombo, asciiSrc ? asciiComponentOpts(filePath)
				: componentOptions(idx, _subcaseCombo->currentData().toInt(), schemaRef, pointId));
		}
		else if (changed == _objectCombo)
		{
			pointId = _objectCombo->currentData().toInt();
			refillChannelLevelFirst(_componentCombo, asciiSrc ? asciiComponentOpts(filePath)
				: componentOptions(idx, _subcaseCombo->currentData().toInt(), schemaRef, pointId));
		}

		//通道链在仓库中完整才换源：失配回显项（数据不在仓库）装配必无块匹配，
		//静默跳过，避免每次下拉变动弹无关告警
		const ChannelSelection picked = [this, idx, filePath, asciiSrc]()
		{
			ChannelSelection sel;
			sel.filePath = filePath;
			sel.subcaseId = _subcaseCombo->currentData().toInt();
			sel.schema = _schemaCombo->currentData().toString();
			sel.pointId = _objectCombo->currentData().toInt();
			sel.component = _componentCombo->currentData().toString();
			if (asciiSrc)
			{
				//ascii：完整性=列名在已登记数据集中（T1 hasChannelData ascii 分支；
				//工况/结果类型/对象为固定骨架常量，工况下拉禁用占位"-"）
				sel.complete = mCurveDataSourceStore::getInstance()->hasChannelData(
					filePath, sel.subcaseId, sel.schema, sel.pointId, sel.component);
				return sel;
			}
			const std::set<NumericFmtInfo> fmts = idx->ycomponent.value(sel.subcaseId)
				.value(sel.schema).value(sel.pointId);
			sel.complete = idx->operatingcondition.count(sel.subcaseId) > 0
				&& idx->object.value(sel.subcaseId).contains(sel.schema)
				&& idx->object.value(sel.subcaseId).value(sel.schema).count(sel.pointId) > 0
				&& fmts.size() == 1
				&& mCurveDataSourceStore::expandedComponents(fmts.begin()->fmt, fmts.begin()->transient)
					.contains(sel.component);
			return sel;
		}();
		if (!picked.complete)
			return;
		//三侧全独立（D7）：每页四级下拉只动本侧——X 页=X 取通道分量值
		//（XY/复数通用）；复数曲线 ym/yp 页=各自独立换源（见
		//commitComplexSideChannel）；XY 曲线 Y 页=整线换源（同"更改..."）
		if (_side == 0)
			commitSideChannel(picked);
		else if (_side == 2 || complexPick)
			commitComplexSideChannel(picked);
		else
			commitChannelChange(picked);
	}

	void mCurveDataPropPage::commitSideAbscissa(const QString& abscissaType, int subcaseId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve)
			return;
		//锚通道：结果类型/对象/分量取来源五元组（壳节点读不到五元组，直接
		//读父曲线 series——loadFromModel 回显数据同此路径）；子工况取自下拉
		//（横坐标形态下工况保持可用），下拉无选中时回退五元组工况
		ChannelSelection anchor = mCurveChannelSelector::fromModelTexts(
			curve->series.sourceFile, curve->series.subcase, curve->series.schema,
			curve->series.object, curve->series.component);
		if (subcaseId > 0)
			anchor.subcaseId = subcaseId;
		const PchReader::PchFile* dataFile =
			mCurveDataSourceStore::getInstance()->file(anchor.filePath);
		if (!dataFile)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 数据文件不在共享仓库：%1").arg(anchor.filePath));
			return;
		}
		mCurveDataSourceStore::ChannelDataResult r;
		QString err;
		if (!mCurveDataSourceStore::assembleChannelData(*dataFile, anchor.subcaseId,
			anchor.schema, anchor.pointId, anchor.component, &r, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (r.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(anchor.pointId), anchor.component));
		}
		//曲线两轴点数须一致成对：对侧现数据长度不符则不落模
		const QVector<double>& other = _side == 1 ? curve->series.xData : curve->series.yData;
		if (r.xData.size() != other.size())
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] X/Y 数据点数不一致（X:%1 Y:%2），未切换")
					.arg(r.xData.size()).arg(other.size()));
			return;
		}

		//换源前压撤回快照，随后数据直写不进命令栈（与"更改..."同语义）
		mCurveDocController* controller = mCurveDocController::getInstance();
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		if (_side == 1)
			controller->setSeriesData(curveId, curve->series.xData, r.xData);
		else
			controller->setSeriesData(curveId, r.xData, curve->series.yData);
		//旧隐藏区间指向旧数据坐标，换源后失效，清空
		controller->setSeriesHiddenRanges(curveId, {});
		controller->setCurveSideSource(curveId, _side != 1,
			mCurveChannelSelector::subcaseText(anchor.subcaseId), abscissaType,
			QString(), QString());

		//侧来源为静默直写、数据信号发的又是父曲线 id（本页绑定壳节点），
		//基类信号回填不触发——按基类惯例手动回填一次
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	void mCurveDataPropPage::commitSideChannel(const ChannelSelection& picked)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve)
			return;
		//装配 X 侧通道数据（块匹配/缺虚部防线与"从文件创建曲线"同源；R22 起
		//装配按数据源类型收口分流，csv/txt 走实例版 ascii 分支——yData=列值）
		mCurveDataSourceStore::ChannelDataResult r;
		QString err;
		if (!assembleChannelByKind(picked.filePath, picked.subcaseId,
			picked.schema, picked.pointId, picked.component, &r, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (r.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(picked.pointId), picked.component));
		}
		//X/Y 数据点数一致才配对（同"定义曲线"X 侧通道语义）
		if (r.yData.size() != curve->series.yData.size())
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] X/Y 数据点数不一致（X:%1 Y:%2），未切换")
					.arg(r.yData.size()).arg(curve->series.yData.size()));
			return;
		}

		mCurveDocController* controller = mCurveDocController::getInstance();
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		controller->setSeriesData(curveId, r.yData, curve->series.yData);
		controller->setSeriesHiddenRanges(curveId, {});
		controller->setCurveSideSource(curveId, true,
			sourceSubcaseText(picked.filePath, picked.subcaseId), picked.schema,
			sourceObjectText(picked.filePath, picked.pointId), picked.component);

		_loading = true;
		loadFromModel();
		_loading = false;
	}

	void mCurveDataPropPage::commitChannelChange(const ChannelSelection& picked)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		//本页绑定 x/y 壳节点，数据与来源五元组都在父曲线（壳节点读重定向的
		//反向查询）；XY 曲线 x/y 两页共用同一父曲线来源，Y 页换源换整条曲线
		//（复数曲线不进此函数：组合下拉走 commitComplexSideChannel，"更改…"
		//按钮走 commitComplexChannelChange）
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve)
			return;

		//装配新通道数据（块匹配/缺虚部防线与"从文件创建曲线"/"定义曲线"同源；
		//R22 起装配按数据源类型收口分流：csv/txt 走实例版 assembleChannelData
		//ascii 分支，pch 走既有静态版，行为逐位不变）
		mCurveDataSourceStore::ChannelDataResult r;
		QString err;
		if (!assembleChannelByKind(picked.filePath, picked.subcaseId,
			picked.schema, picked.pointId, picked.component, &r, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (r.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(picked.pointId), picked.component));
		}

		//X 侧保持自身形态（Y 侧换源不影响 X 侧来源）：通道形态（X=通道分量值）
		//数据不动——点数配对校验收口在控制器 checkSourcePairing，拒绝路径零副
		//作用；横坐标形态 X = 新锚通道的记录横坐标。R22 ascii 数据集例外：无
		//记录横坐标语义，X 默认落第 1 列通道（用户拍板 20261109，不落行号 1..N；
		//X 已为通道形态时保持原样，经上方配对校验兜底）
		mCurveDocController* controller = mCurveDocController::getInstance();
		if (!controller->checkSourcePairing(curveId, r.yData))
			return;
		const bool xChannelMode = mCurveDataSourceStore::isChannelSchema(curve->series.xSideSchema);
		const bool asciiSrc = mCurveDataSourceStore::fileKind(picked.filePath)
			== mCurveDataSourceStore::DataSourceKind::AsciiColumn;
		QVector<double> xNew;
		QString xColumn;
		if (xChannelMode)
			xNew = curve->series.xData;
		else if (asciiSrc)
		{
			const QStringList cols = mCurveDataSourceStore::getInstance()
				->asciiColumnNames(picked.filePath);
			if (cols.isEmpty())
			{
				controller->reportWarning(QStringLiteral("[更改曲线] 文件无可用数据列：%1")
					.arg(QFileInfo(picked.filePath).fileName()));
				return;
			}
			xColumn = cols.first();
			mCurveDataSourceStore::ChannelDataResult rx;
			QString errX;
			if (!assembleChannelByKind(picked.filePath, picked.subcaseId, picked.schema,
				picked.pointId, xColumn, &rx, &errX) || rx.yData.isEmpty())
			{
				controller->reportWarning(QStringLiteral("[更改曲线] %1").arg(errX));
				return;
			}
			xNew = rx.yData;
		}
		else
			xNew = r.xData;

		//换源前压撤回快照（与编辑曲线"裁剪/删除/隐藏"同一栈，撤回恢复数据
		//与来源五元组；懒创建的编辑曲线窗口保持隐藏，打开后即可撤回）；
		//随后数据直写不进命令栈（与 pch 导入同语义）
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		controller->setSeriesSource(curveId, picked.filePath,
			sourceSubcaseText(picked.filePath, picked.subcaseId), picked.schema,
			sourceObjectText(picked.filePath, picked.pointId), picked.component);
		if (asciiSrc && !xChannelMode)
		{
			//X 侧落第 1 列通道形态（setSeriesSource 对非通道 X 侧已清空侧字段）
			controller->setCurveSideSource(curveId, true, QString(),
				mCurveDataSourceStore::asciiSchemaDisplayText(),
				mCurveDataSourceStore::asciiObjectDisplayText(), xColumn);
		}
		controller->setSeriesData(curveId, xNew, r.yData);
		//隐藏区间按轴清理：X 数据保持时 X 侧（axis==0）区间仍有效，仅清 Y 侧
		//（axis==1）；X 重取横坐标时整体失效，清空
		if (xChannelMode)
		{
			QVector<double> keep;
			const QVector<double>& hid = curve->series.hiddenRanges;
			for (int i = 0; i + 2 < hid.size(); i += 3)
				if (hid.at(i) == 0)   //0=X 轴区间
					keep += hid.mid(i, 3);
			controller->setSeriesHiddenRanges(curveId, keep);
		}
		else
		{
			controller->setSeriesHiddenRanges(curveId, {});
		}

		//来源字段为静默直写、数据信号发的又是父曲线 id（本页绑定壳节点），
		//基类信号回填不触发——按基类惯例手动回填一次
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	void mCurveDataPropPage::commitComplexChannelChange(const ChannelSelection& picked)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve)
			return;
		//所选分量取同方向对侧（MAG↔PHA、REAL↔IMAG）；非配对分量（普通结果
		//类型分量）不适于复数曲线，告警退出
		QString compA = picked.component;
		QString compB = mCurveDataSourceStore::siblingComplexComponent(compA);
		if (compB.isEmpty())
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 分量“%1”无复数对侧，复数曲线须选 MAG/PHA 或 REAL/IMAG 分量")
					.arg(compA));
			return;
		}
		//锚定上面板侧（MAG/REAL）：用户在 yp 页选了下侧分量时互换
		if (!compA.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive)
			&& !compA.startsWith(QStringLiteral("REAL"), Qt::CaseInsensitive))
			std::swap(compA, compB);

		const PchReader::PchFile* dataFile =
			mCurveDataSourceStore::getInstance()->file(picked.filePath);
		if (!dataFile)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 数据文件不在共享仓库：%1").arg(picked.filePath));
			return;
		}
		//两路同块装配（X=同一记录横坐标；缺虚部防线同"从文件创建曲线"）
		mCurveDataSourceStore::ChannelDataResult rA, rB;
		QString err;
		if (!mCurveDataSourceStore::assembleChannelData(*dataFile, picked.subcaseId,
			picked.schema, picked.pointId, compA, &rA, &err)
			|| !mCurveDataSourceStore::assembleChannelData(*dataFile, picked.subcaseId,
				picked.schema, picked.pointId, compB, &rB, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (rA.skippedNoSecondary || rB.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(picked.pointId), compA));
		}
		//X/Y/Y' 三序列等长才落模（同块装配天然等长，防御脏数据）
		const int nPts = qMin(rA.xData.size(), qMin(rA.yData.size(), rB.yData.size()));
		if (nPts <= 0
			|| rA.xData.size() != nPts || rA.yData.size() != nPts || rB.yData.size() != nPts)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 复数两路数据点数不一致，未切换"));
			return;
		}

		mCurveDocController* controller = mCurveDocController::getInstance();
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		controller->setComplexSeriesData(curveId, rA.xData, rA.yData, rB.yData);
		controller->setComplexSeriesSource(curveId, picked.filePath,
			mCurveChannelSelector::subcaseText(picked.subcaseId), picked.schema,
			mCurveChannelSelector::objectText(picked.pointId), compA, compB,
			compA.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive) ? 0 : 1);
		//旧隐藏区间指向旧数据坐标，换源后整体失效
		controller->setSeriesHiddenRanges(curveId, {});

		//来源字段为静默直写、数据信号发的又是父曲线 id（本页绑定壳节点），
		//基类信号回填不触发——按基类惯例手动回填一次
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	//复数曲线 ym/yp 侧独立换源（D7 三侧全独立）：本页四级下拉只动本侧。
	//ym 页（_side==1）：锚四元组=所选，complexSide* 尾参原样传现存（空则
	//控制器镜像回退，旧模板形态不因本页切换升级），复数格式按所选分量族重算
	//（非 MAG/REAL 分量保原格式）；yp 页（_side==2）：只写 complexYData 与
	//complexSide*（锚五元组/复数格式原样保留）。分量不限族——成对路径的
	//"无复数对侧"限制不适用于独立切换；四级下拉选项取自曲线现源文件
	//（onChannelComboChanged 按 kSourceFile 取 idx），故无跨文件侧来源
	void mCurveDataPropPage::commitComplexSideChannel(const ChannelSelection& picked)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve || !curve->series.isComplex())
			return;
		mCurveDataSourceStore::ChannelDataResult r;
		QString err;
		//R22 起装配按数据源类型收口分流（csv/txt 走实例版 ascii 分支——本侧
		//独立换源，不做复数成对：ascii 无复数分量对，普通单侧通道语义天然适用）
		if (!assembleChannelByKind(picked.filePath, picked.subcaseId,
			picked.schema, picked.pointId, picked.component, &r, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (r.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(picked.pointId), picked.component));
		}
		//与现存 X 点数成对才落模（X 为三路共享横坐标，本侧换源不动 X）
		if (r.yData.size() != curve->series.xData.size())
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 数据点数不一致（X:%1 本侧:%2），未切换")
					.arg(curve->series.xData.size()).arg(r.yData.size()));
			return;
		}

		const CurveSeriesData& s = curve->series;
		const bool ymSide = _side == 1;
		mCurveDocController* controller = mCurveDocController::getInstance();
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		if (ymSide)
		{
			controller->setComplexSeriesData(curveId, s.xData, r.yData, s.complexYData);
			const int fmt = picked.component.startsWith(QStringLiteral("MAG"), Qt::CaseInsensitive)
				? 0
				: picked.component.startsWith(QStringLiteral("REAL"), Qt::CaseInsensitive)
				? 1 : s.complexFormat;
			controller->setComplexSeriesSource(curveId, picked.filePath,
				sourceSubcaseText(picked.filePath, picked.subcaseId), picked.schema,
				sourceObjectText(picked.filePath, picked.pointId), picked.component,
				s.complexSideComponent, fmt,
				s.complexSideSubcase, s.complexSideSchema, s.complexSideObject);
			//X 坐标未动：轴 0（X）隐藏区间仍有效，轴 1 区间指向旧 ym 坐标失效
			QVector<double> keep;
			const QVector<double>& hid = s.hiddenRanges;
			for (int i = 0; i + 2 < hid.size(); i += 3)
				if (hid.at(i) == 0)   //0=X 轴区间
					keep += hid.mid(i, 3);
			controller->setSeriesHiddenRanges(curveId, keep);
		}
		else
		{
			controller->setComplexSeriesData(curveId, s.xData, s.yData, r.yData);
			controller->setComplexSeriesSource(curveId, s.sourceFile,
				s.subcase, s.schema, s.object, s.component,
				picked.component, s.complexFormat,
				sourceSubcaseText(picked.filePath, picked.subcaseId),
				picked.schema, sourceObjectText(picked.filePath, picked.pointId));
			//X/ym 均未动，隐藏区间整体保留（yp 下面板不挂轴，无独立区间）
		}

		//来源字段为静默直写、数据信号发的又是父曲线 id（本页绑定壳节点），
		//基类信号回填不触发——按基类惯例手动回填一次
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	//复数曲线 ym/yp 页横坐标形态（D7 三侧全独立）：本侧数据=锚通道数据块在
	//所选工况下的记录横坐标，他侧不动；锚通道取来源五元组（横坐标形态不改
	//动锚通道），子工况取自下拉（横坐标形态下工况保持可用，下拉无选中回退
	//五元组）——锚定规则照 commitSideAbscissa，落模走复数侧写口
	void mCurveDataPropPage::commitComplexSideAbscissa(const QString& abscissaType,
		int subcaseId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		const int curveId = doc->parentNodeId(_nodeId);
		CurveNode* curve = curveId > 0 ? doc->node(curveId) : nullptr;
		if (!curve || curve->type != CurveNodeType::Curve || !curve->series.isComplex())
			return;
		ChannelSelection anchor = mCurveChannelSelector::fromModelTexts(
			curve->series.sourceFile, curve->series.subcase, curve->series.schema,
			curve->series.object, curve->series.component);
		if (subcaseId > 0)
			anchor.subcaseId = subcaseId;
		const PchReader::PchFile* dataFile =
			mCurveDataSourceStore::getInstance()->file(anchor.filePath);
		if (!dataFile)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 数据文件不在共享仓库：%1").arg(anchor.filePath));
			return;
		}
		mCurveDataSourceStore::ChannelDataResult r;
		QString err;
		if (!mCurveDataSourceStore::assembleChannelData(*dataFile, anchor.subcaseId,
			anchor.schema, anchor.pointId, anchor.component, &r, &err))
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] %1").arg(err));
			return;
		}
		if (r.skippedNoSecondary)
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 通道“%1/%2”的数据块无虚部/相位数据（瞬态实数块），已跳过")
					.arg(mCurveChannelSelector::objectText(anchor.pointId), anchor.component));
		}
		//与现存 X 点数成对才落模（X 为三路共享横坐标，本侧取横坐标不动 X）
		if (r.xData.size() != curve->series.xData.size())
		{
			mCurveDocController::getInstance()->reportWarning(
				QStringLiteral("[更改曲线] 数据点数不一致（X:%1 本侧:%2），未切换")
					.arg(curve->series.xData.size()).arg(r.xData.size()));
			return;
		}

		const CurveSeriesData& s = curve->series;
		const bool ymSide = _side == 1;
		mCurveDocController* controller = mCurveDocController::getInstance();
		mEditCurveDlg::getInstance()->snapshotCurve(curveId);
		if (ymSide)
		{
			//ym 取横坐标：锚四元组落所选工况+横坐标类型（分量清空）；b 侧
			//四元组/令牌原样保留，复数格式不动（横坐标无分量族）
			controller->setComplexSeriesData(curveId, s.xData, r.xData, s.complexYData);
			controller->setComplexSeriesSource(curveId, anchor.filePath,
				mCurveChannelSelector::subcaseText(anchor.subcaseId), abscissaType,
				QString(), QString(), s.complexSideComponent, s.complexFormat,
				s.complexSideSubcase, s.complexSideSchema, s.complexSideObject);
			//X 坐标未动：轴 0（X）隐藏区间仍有效，轴 1 区间指向旧 ym 坐标失效
			QVector<double> keep;
			const QVector<double>& hid = s.hiddenRanges;
			for (int i = 0; i + 2 < hid.size(); i += 3)
				if (hid.at(i) == 0)   //0=X 轴区间
					keep += hid.mid(i, 3);
			controller->setSeriesHiddenRanges(curveId, keep);
		}
		else
		{
			//yp 取横坐标：锚五元组/复数格式原样，b 侧四元组=所选工况+横坐标
			//类型（分量空）；X/ym 未动，隐藏区间整体保留
			controller->setComplexSeriesData(curveId, s.xData, s.yData, r.xData);
			controller->setComplexSeriesSource(curveId, s.sourceFile,
				s.subcase, s.schema, s.object, s.component,
				QString(), s.complexFormat,
				mCurveChannelSelector::subcaseText(anchor.subcaseId),
				abscissaType, QString());
		}

		//来源字段为静默直写、数据信号发的又是父曲线 id（本页绑定壳节点），
		//基类信号回填不触发——按基类惯例手动回填一次
		_loading = true;
		loadFromModel();
		_loading = false;
	}

	//--- mAxisPropPage ---

	mAxisPropPage::mAxisPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();

		//类别轴判定还依赖旁系节点（父图表 kChartType、图表下曲线的类别刻度名，
		//"data"直写同样广播），基类只监听本节点——这里补全局监听，命中本轴
		//所属图表或其直接子节点时重载回填（loadFromModel 只读，无回环风险）
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodePropertyChanged,
			this, [this](int nodeId, const QString& prop)
		{
			Q_UNUSED(prop)
			if (_nodeId <= 0 || nodeId == _nodeId)   //本节点变化基类已回填
				return;
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			const int chartId = doc->chartNodeId(_nodeId);
			if (chartId > 0 && (nodeId == chartId || doc->parentNodeId(nodeId) == chartId))
			{
				_loading = true;
				loadFromModel();
				_loading = false;
			}
		});
	}

	void mAxisPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/名称可见性/文本/类型（截图11/12；类型随节点只读回显置灰；
		//类别轴模式下仅"文本"保留，显隐见 updateRowStates）
		_baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		_baseFold->addRow(_visibleCheck);
		_nameVisibleCheck = createRowCheck(QStringLiteral("名称可见性"), this);
		_baseFold->addRow(_nameVisibleCheck);
		//名称 = 树显示名（kName，与注释/基准线页同款行内输入框；区别于
		//下方"文本"行 kTitle 轴标题模板）
		_nameEdit = new QLineEdit(this);
		_baseFold->addRow(QStringLiteral("名称"), _nameEdit);
		//文本=按钮弹"编辑字符"弹窗（同表头行：模板文本长，行内不放输入框；
		//支持 {p1w1c1.y.MX*}/{y.MX*} 模板占位符，渲染时解析）
		_titleEdit = new mCurveTextButton(this);
		_baseFold->addRow(QStringLiteral("文本"), _titleEdit);
		_typeEdit = new QLineEdit(this);
		_typeEdit->setEnabled(false);
		_baseFold->addRow(QStringLiteral("类型"), _typeEdit);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});
		connect(_nameVisibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			//名称不可见时渲染层清空轴标题（CurveAxisData::titleVisible）
			commit(CurveProp::kTitleVisible, on);
		});
		connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kName, _nameEdit->text());
		});
		connect(_titleEdit, &mCurveTextButton::textCommitted, this, [this](const QString& text)
		{
			commit(CurveProp::kTitle, text);
		});

		//显示：标签字体设置/颜色（字体为绘图选项同款交互：按钮弹"选择字体"对话框；
		//基准=绘图选项默认轴标题字体——建图"默认"写入的就是该值，须回显"默认"）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_titleFontBtn = new mCurveFontButton(axisTitleDefaultFont(), this);
		dispFold->addRow(QStringLiteral("标签字体设置"), _titleFontBtn);
		_colorBtn = new mCurveColorButton(Qt::black, this);
		dispFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_titleFontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kTitleFont, f);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kTitleColor, c);
		});

		//缩放设置：缩放方式/参考值/计权/最小值/最大值（设计图11/12：参考值与
		//计权两行仅 dB20/dB10 显示，linear/logarithmic 只有范围两项；参考值
		//dB20/dB10 共用一个值，行标签随缩放方式切换。无"自动范围"行——手动
		//改范围即退出自动，与弹窗轴编辑页同策略）
		_rangeFold = new mCurveFoldSection(QStringLiteral("缩放设置"), this);
		_scaleModeCombo = new QComboBox(this);
		_scaleModeCombo->addItems({ QStringLiteral("Linear"), QStringLiteral("Logarithmic"),
			QStringLiteral("dB20"), QStringLiteral("dB10") });
		_rangeFold->addRow(QStringLiteral("缩放方式"), _scaleModeCombo);
		_dbRefLabel = new QLabel(QStringLiteral("dB20参考值"), this);
		_dbRefLabel->setFixedWidth(90);   //同 addRow(文本) 行标签规格
		_dbRefLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_dbRefSpin = createCurveSciDoubleSpin(4, 1e-12, 1e9, 1.0, this);   //科学计数法（模型默认 2e-11）
		_rangeFold->addRow(_dbRefLabel, _dbRefSpin);
		_weightCombo = new QComboBox(this);
		_weightCombo->addItems({ QStringLiteral("无"), QStringLiteral("A"),
			QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("U") });
		_rangeFold->addRow(QStringLiteral("计权"), _weightCombo);
		_minSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, this);
		_rangeFold->addRow(QStringLiteral("最小值"), _minSpin);
		_maxSpin = createCurveDoubleSpin(4, -1e9, 1e9, 1.0, this);
		_rangeFold->addRow(QStringLiteral("最大值"), _maxSpin);
		//类别轴（柱状图 X 轴带类别刻度名）：起始/终止=可见类别范围下拉，标签
		//对齐方式=类别刻度文字摆放（水平/对角 斜向/垂直）。仅类别轴显示（显隐
		//见 updateRowStates）；提交映射 kMin=序号-0.5/kMax=序号+0.5，见
		//commitCategoryBound
		_catStartCombo = new QComboBox(this);
		_rangeFold->addRow(QStringLiteral("起始"), _catStartCombo);
		_catEndCombo = new QComboBox(this);
		_rangeFold->addRow(QStringLiteral("终止"), _catEndCombo);
		_catAlignCombo = new QComboBox(this);
		_catAlignCombo->addItems({ QStringLiteral("水平"), QStringLiteral("对角 / 斜向"),
			QStringLiteral("垂直"), QStringLiteral("斜排 30°") });   //档位 3=需求稿 02 斜排约 30°
		_rangeFold->addRow(QStringLiteral("标签对齐方式"), _catAlignCombo);
		connect(_catStartCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitCategoryBound(true, idx);
		});
		connect(_catEndCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commitCategoryBound(false, idx);
		});
		connect(_catAlignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kCategoryLabelAlign, idx);   //0水平/1对角 / 斜向/2垂直
		});
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
			//极坐标半径轴（D25/M8 四裁定）：切缩放方式即落该档默认域（原型 05
			//面板——Linear 0/10、Log -3/1、dB20 -60/20、dB10 -30/10；面板所见为
			//变换空间，经 inverseTransformValue 落模型原始空间）并切手动；
			//直角轴维持原行为（不落默认域）
			if (doc && _nodeId > 0
				&& doc->nodeType(_nodeId) == CurveNodeType::YAxis)
			{
				CurveNode* polarChart = doc->node(doc->chartNodeId(_nodeId));
				if (polarChart && polarChart->type == CurveNodeType::Chart
					&& polarChart->chart.chartType == 3)
				{
					double dLo = 0.0, dHi = 10.0;
					if (idx == 1)      { dLo = -3.0;  dHi = 1.0; }
					else if (idx == 2) { dLo = -60.0; dHi = 20.0; }
					else if (idx == 3) { dLo = -30.0; dHi = 10.0; }
					const double dbRef = doc->nodeProperty(_nodeId,
						CurveProp::kDbRef).toDouble();
					//计权感知往返：除回增益落模型原始空间（渲染 scale.transformed 同乘）。
					const double gain = polarY1WeightGain(_nodeId);
					commit(CurveProp::kMin, mCurveChartView::inverseTransformValue(dLo, idx, dbRef) / gain);
					commit(CurveProp::kMax, mCurveChartView::inverseTransformValue(dHi, idx, dbRef) / gain);
					commit(CurveProp::kAutoScale, false);
				}
			}
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
			//极坐标 Y1：提交逆行除回增益（D25 计权感知往返，直角轴恒 1.0）。
			const double gain = polarY1WeightGain(_nodeId);
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
							editingMin ? dispHi : dispLo, mode, dbRef) / gain);
				}
				commit(CurveProp::kAutoScale, false);
			}
			commit(prop, mCurveChartView::inverseTransformValue(v, mode, dbRef) / gain);
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
			this, &mAxisPropPage::onAxisDisplayRangeChanged);

		//坐标轴刻度设置（设计图11/12）：格式/精度/刻度字体设置恒显；刻度生
		//成方式恒显、选项集随缩放方式重建（linear：均匀分步法→刻度总数、固
		//定步长法→固定步长；log/dB：每十倍分步法→每十倍刻度数/每十倍网格线
		//数、均匀分步法/固定步长法同 linear 语义，在刻度变换空间生效；显隐
		//联动见 updateRowStates）
		_tickFold = new mCurveFoldSection(QStringLiteral("坐标轴刻度设置"), this);
		_formatCombo = new QComboBox(this);
		_formatCombo->addItems({ QStringLiteral("自动"), QStringLiteral("科学计数法"),
			QStringLiteral("定点格式") });
		_tickFold->addRow(QStringLiteral("格式"), _formatCombo);
		_tickGenCombo = new QComboBox(this);
		_tickGenCombo->addItems({ QStringLiteral("均匀分步法"), QStringLiteral("固定步长法") });
		_tickFold->addRow(QStringLiteral("刻度生成方式"), _tickGenCombo);
		_tickCountSpin = createCurveDoubleSpin(4, 0.0, 1e9, 1.0, this);
		_tickCountSpin->setValue(6.0);   //默认对齐原型（2026-11-12 D25）
		_tickFold->addRow(QStringLiteral("刻度总数"), _tickCountSpin);
		_fixedStepSpin = createCurveDoubleSpin(4, 0.0, 1e9, 1.0, this);
		_tickFold->addRow(QStringLiteral("固定步长"), _fixedStepSpin);
		_gridPerMajorSpin = createCurveSpinBox(0, 100, this);
		_tickFold->addRow(QStringLiteral("每主刻度网格数"), _gridPerMajorSpin);
		_decadeTickSpin = createCurveSpinBox(1, 100, this);
		_decadeTickSpin->setValue(5);
		_tickFold->addRow(QStringLiteral("每十倍刻度数"), _decadeTickSpin);
		_decadeGridSpin = createCurveSpinBox(0, 100, this);
		_decadeGridSpin->setValue(11);   //默认对齐原型：十等分 10 段线（2026-11-12 D25）
		_tickFold->addRow(QStringLiteral("每十倍网格线数"), _decadeGridSpin);
		_precisionSpin = createCurveSpinBox(0, 15, this);
		_precisionSpin->setValue(5);     //默认对齐原型（2026-11-12 D25）
		_tickFold->addRow(QStringLiteral("精度"), _precisionSpin);
		_labelFontBtn = new mCurveFontButton(this);
		_tickFold->addRow(QStringLiteral("刻度字体设置"), _labelFontBtn);
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
		connect(_labelFontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kLabelFont, f);
		});

		updateRowStates();

		layout->addWidget(_baseFold);
		layout->addWidget(dispFold);
		layout->addWidget(_rangeFold);
		layout->addWidget(_tickFold);
		layout->addStretch(1);
	}

	//极坐标轴模式判定（chartType=3 图表的主轴 X1/Y1）
	static bool polarAxisModeOf(const CurveDocument* doc, int nodeId)
	{
		if (!doc || nodeId < 0 || !doc->node(nodeId))
			return false;
		CurveNode* chartNode = doc->node(doc->chartNodeId(nodeId));
		const CurveNodeType nt = doc->nodeType(nodeId);
		return chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 3
			&& (nt == CurveNodeType::XAxis || nt == CurveNodeType::YAxis);
	}

	void mAxisPropPage::updateRowStates()
	{
		//极坐标半径轴分支用判定（chartType=3 图表的主轴 X1/Y1）
		const bool isPolarAxis = polarAxisModeOf(
			mCurveDocController::getInstance()->document(), _nodeId);
		//类别轴模式（柱状图 X 主轴即切换，设计图）：标准只留"文本"，缩放
		//设置只留 起始/终止/标签对齐方式，坐标轴刻度设置整组隐藏（类别刻度
		//恒步长 1、不走数字刻度引擎，见 applyAxis）；显示组两模式一致
		if (_categoryMode)
		{
			_baseFold->setRowVisible(_visibleCheck, false);
			_baseFold->setRowVisible(_nameVisibleCheck, false);
			_baseFold->setRowVisible(_nameEdit, false);
			_baseFold->setRowVisible(_typeEdit, false);
			_rangeFold->setRowVisible(_scaleModeCombo, false);
			_rangeFold->setRowVisible(_dbRefSpin, false);
			_rangeFold->setRowVisible(_weightCombo, false);
			_rangeFold->setRowVisible(_minSpin, false);
			_rangeFold->setRowVisible(_maxSpin, false);
			_rangeFold->setRowVisible(_catStartCombo, true);
			_rangeFold->setRowVisible(_catEndCombo, true);
			_rangeFold->setRowVisible(_catAlignCombo, true);
			_tickFold->setVisible(false);
			return;
		}
		_tickFold->setVisible(true);
		_baseFold->setRowVisible(_visibleCheck, true);
		_baseFold->setRowVisible(_nameVisibleCheck, true);
		_baseFold->setRowVisible(_nameEdit, true);
		_baseFold->setRowVisible(_typeEdit, true);
		_rangeFold->setRowVisible(_catStartCombo, false);
		_rangeFold->setRowVisible(_catEndCombo, false);
		_rangeFold->setRowVisible(_catAlignCombo, false);

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
			//选项集重建+回显均阻塞信号：打开页面/切缩放方式经此处刷新，不得
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

		//极坐标半径轴（chartType=3 主轴，需求注解）：非 Linear 隐藏刻度生成
		//方式行、显示每十倍刻度数/网格线数；Linear 反之（刻度总数/每主刻度
		//网格数行按上方 uniform 联动照常）；dB 参考值/计权行沿用 db 联动，
		//Log 档自然无
		if (isPolarAxis)
		{
			//极坐标角度轴（X1，D25/L12）：角度恒均匀分步、无缩放档语义——
			//刻度生成方式收窄为仅"均匀分步法"（固定步长法会隐藏唯一生效的
			//刻度总数行，形成整页无可编辑生效行的死区）
			if (doc->nodeType(_nodeId) == CurveNodeType::XAxis)
			{
				{
					const QSignalBlocker blokGen(_tickGenCombo);
					if (_tickGenCombo->property("optionSet").toInt() != 2)
					{
						_tickGenCombo->clear();
						_tickGenCombo->addItem(QStringLiteral("均匀分步法"));
						_tickGenCombo->setProperty("optionSet", 2);
					}
					_tickGenCombo->setCurrentIndex(0);
				}
				_tickFold->setRowVisible(_tickGenCombo, true);
				_tickFold->setRowVisible(_tickCountSpin, true);
				_tickFold->setRowVisible(_fixedStepSpin, false);
				_tickFold->setRowVisible(_gridPerMajorSpin, true);
				_tickFold->setRowVisible(_decadeTickSpin, false);
				_tickFold->setRowVisible(_decadeGridSpin, false);
				return;
			}
			_tickFold->setRowVisible(_tickGenCombo, linear);
			_tickFold->setRowVisible(_decadeTickSpin, !linear);
			_tickFold->setRowVisible(_decadeGridSpin, !linear);
		}
	}

	void mAxisPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		//类别轴判定（须先于 updateRowStates 的模式切换）：柱状图 X 主轴即类别
		//轴（树显示名 Category，与渲染端 applyAxis 的柱状图分支同条件）。起始/
		//终止选项两形态：有类别刻度名（定义曲线"数值"来源）列类别名；文件来
		//源柱状曲线无刻度名时退用曲线 X 数据值（升序去重）
		_categoryLabels.clear();
		_barXValues.clear();
		CurveNode* chartNode = doc->node(doc->chartNodeId(_nodeId));
		_categoryMode = doc->nodeType(_nodeId) == CurveNodeType::XAxis
			&& chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 1;
		if (_categoryMode)
		{
			_categoryLabels = doc->chartCategoryTickLabels(doc->chartNodeId(_nodeId));
			if (_categoryLabels.isEmpty())
				_barXValues = doc->chartBarXValues(doc->chartNodeId(_nodeId));
		}
		//复数图 Y2 副轴模式（下面板分量轴）：幅相展开下相位为角度值，缩放
		//方式仅 Linear（log/dB 对相位无意义）；实虚展开下虚部为实值量，与主
		//轴（Real）同款放开四项——渲染链（transformedComplexPair 的 yModeB
		//变换、applyComplexLowerAxis 的引擎/范围变换）本就按 Y2 节点
		//scaleMode 生效，实虚下直接可用。下拉项按模式重建（页面跨节点复用）；
		//阻塞信号——打开页面不得改写模型（幅相下历史存量非 Linear 值保留，
		//显示按 Linear）
		const bool complexY2Mode = doc->nodeType(_nodeId) == CurveNodeType::YAxis2
			&& chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 2;
		const bool phaseAxisMode = complexY2Mode
			&& chartNode->chart.complexDisplayFormat == 0;   //0=幅相/1=实虚
		//X1 主轴模式（仅复数图）：频率/时间横轴 dB 缩放（dB20/dB10）无意义，
		//缩放方式仅 Linear/Logarithmic（历史存量 dB 值显示按 Logarithmic，模
		//型值不动）。XY 图 X1 维持四项
		const bool x1AxisMode = doc->nodeType(_nodeId) == CurveNodeType::XAxis
			&& chartNode && chartNode->type == CurveNodeType::Chart
			&& chartNode->chart.chartType == 2;
		{
			const QSignalBlocker blokScaleMode(_scaleModeCombo);
			_scaleModeCombo->clear();
			if (phaseAxisMode)
				_scaleModeCombo->addItem(QStringLiteral("Linear"));
			else if (x1AxisMode)
				_scaleModeCombo->addItems({ QStringLiteral("Linear"),
					QStringLiteral("Logarithmic") });
			else
				_scaleModeCombo->addItems({ QStringLiteral("Linear"), QStringLiteral("Logarithmic"),
					QStringLiteral("dB20"), QStringLiteral("dB10") });
		}
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_nameVisibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kTitleVisible).toBool());
		_nameEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kName).toString());
		_titleEdit->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kTitle).toString());
		//极坐标图表（chartType=3）：X1=相位/Y1=恒值（只读文案覆写，design D5
		//属性页语义；直角坐标维持水平/垂直）
		const bool polarAxisEcho = polarAxisModeOf(doc, _nodeId);
		_typeEdit->setText(polarAxisEcho
			? (doc->nodeType(_nodeId) == CurveNodeType::XAxis
				? QStringLiteral("相位") : QStringLiteral("恒值"))
			: ((doc->nodeType(_nodeId) == CurveNodeType::XAxis
				|| doc->nodeType(_nodeId) == CurveNodeType::XAxis2)
				? QStringLiteral("水平") : QStringLiteral("垂直")));
		_titleFontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kTitleFont).value<QFont>());
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kTitleColor).value<QColor>());
		{
			//阻塞信号：打开页面不得改写模型——非阻塞回显会触发缩放方式信号
			//（提交+跨集合重置刻度生成方式），把历史存量值冲掉
			const QSignalBlocker blokScaleIdx(_scaleModeCombo);
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
		//阻塞信号：回显值触发 commitRange 会"提交即退出自动"，打开页面不得
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
			//极坐标 Y1：回显乘增益（与渲染域同空间，D25 计权感知往返）。
			const double echoGain = polarY1WeightGain(_nodeId);
			const double tLo = mCurveChartView::transformValue(rangeLo * echoGain, echoScaleMode, echoDbRef);
			const double tHi = mCurveChartView::transformValue(rangeHi * echoGain, echoScaleMode, echoDbRef);
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
		//现值回显延后一拍：切缩放方式等属性信号里本函数同步执行，此刻引擎轴
		//可能还是旧刻度空间/旧范围（全量重建排在 0ms 队列稍后执行）——延后
		//回查即拿到重建后的真实显示范围（如 dB 轴的 -60~26，而非旧线性 0~20）
		QTimer::singleShot(0, this, [this]()
		{
			echoDisplayRange();
			if (_categoryMode)
				echoCategoryRange();   //起始/终止同理由重建落定后的真实范围回显
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
		_labelFontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kLabelFont).value<QFont>());

		//类别轴回填：选项集两形态（类别名 / 曲线 X 数据值，'g' 格式与刻度数字
		//同风格）、对齐方式回显现值（阻塞信号——重填触发 currentIndexChanged
		//会把空值/旧值提交进模型）；起始/终止序号由 echoCategoryRange 按显示
		//范围回显。两者皆空（图表下无曲线/空数据）时下拉置灰防误触
		if (_categoryMode)
		{
			QStringList options = _categoryLabels;
			if (options.isEmpty())
			{
				for (double v : _barXValues)
					options << QString::number(v, 'g', 10);
			}
			{
				const QSignalBlocker blokStart(_catStartCombo);
				const QSignalBlocker blokEnd(_catEndCombo);
				_catStartCombo->clear();
				_catEndCombo->clear();
				_catStartCombo->addItems(options);
				_catEndCombo->addItems(options);
			}
			_catStartCombo->setEnabled(!options.isEmpty());
			_catEndCombo->setEnabled(!options.isEmpty());
			_catAlignCombo->setCurrentIndex(qBound(0,
				doc->nodeProperty(_nodeId, CurveProp::kCategoryLabelAlign).toInt(), 3));
			echoCategoryRange();
		}
	}

	void mAxisPropPage::echoCategoryRange()
	{
		//起始/终止回显当前显示范围：自动范围=引擎现值（模型 min/max 是休眠的
		//手动值），手动范围=模型 min/max。阻塞信号回填防提交回环。两形态：
		//类别=类别 i 全可见区间 [i-0.5,i+0.5]（fitChartAxes 外扩半格同约定），
		//四舍五入取回序号；数值=显示范围内的首/末 X 数据点（数据升序，二分
		//定位，端点放相对容差防边界值差一个 ulp 漏判）
		if (_nodeId < 0 || _categoryLabels.isEmpty() && _barXValues.isEmpty())
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		double lo = doc->nodeProperty(_nodeId, CurveProp::kMin).toDouble();
		double hi = doc->nodeProperty(_nodeId, CurveProp::kMax).toDouble();
		if (doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
		{
			double dispLo = 0.0, dispHi = 0.0;
			if (mCurveChartView::queryAxisDisplayRange(_nodeId, &dispLo, &dispHi))
			{
				lo = dispLo;
				hi = dispHi;
			}
		}
		const QSignalBlocker blokStart(_catStartCombo);
		const QSignalBlocker blokEnd(_catEndCombo);
		if (!_categoryLabels.isEmpty())
		{
			const int n = _categoryLabels.size();
			const int startIdx = qBound(0, qRound(lo + 0.5), n - 1);
			const int endIdx = qBound(startIdx, qRound(hi - 0.5), n - 1);
			_catStartCombo->setCurrentIndex(startIdx);
			_catEndCombo->setCurrentIndex(endIdx);
			return;
		}
		if (_barXValues.isEmpty())
			return;
		const double eps = 1e-9 * qMax(1.0, qMax(qAbs(lo), qAbs(hi)));
		auto sIt = std::lower_bound(_barXValues.cbegin(), _barXValues.cend(), lo - eps);
		int startIdx = (sIt == _barXValues.cend())
			? _barXValues.size() - 1 : static_cast<int>(sIt - _barXValues.cbegin());
		auto eIt = std::upper_bound(_barXValues.cbegin(), _barXValues.cend(), hi + eps);
		int endIdx = (eIt == _barXValues.cbegin())
			? 0 : static_cast<int>(eIt - _barXValues.cbegin()) - 1;
		if (endIdx < startIdx)
			endIdx = startIdx;
		_catStartCombo->setCurrentIndex(startIdx);
		_catEndCombo->setCurrentIndex(endIdx);
	}

	void mAxisPropPage::commitCategoryBound(bool editingStart, int idx)
	{
		//起始/终止提交（改范围即退出自动，同数值最小/最大策略：自动轴从不落
		//min/max，模型里还是构造默认 0/10，须先按当前显示范围把"另一侧"回写
		//模型，只提交编辑侧会让另一端立即失真；起始>终止时另一端跟随——数值
		//页互相钳制的同款交互。每次 commit 都经信号触发重填，最终态由末次提
		//交后的回填统一回显）：
		//类别形态：序号 idx 落区间 [idx-0.5, idx+0.5]（原始值空间即类别序号
		//空间，与 fitChartAxes 类别范围外扩半格同约定）
		if (_loading || idx < 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!_categoryLabels.isEmpty())
		{
			if (idx >= _categoryLabels.size())
				return;
			double lo = doc->nodeProperty(_nodeId, CurveProp::kMin).toDouble();
			double hi = doc->nodeProperty(_nodeId, CurveProp::kMax).toDouble();
			if (doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
			{
				double dispLo = 0.0, dispHi = 0.0;
				if (mCurveChartView::queryAxisDisplayRange(_nodeId, &dispLo, &dispHi))
				{
					lo = dispLo;
					hi = dispHi;
					//退出自动前先把"未编辑侧"按当前显示范围回写模型（类别轴
					//显示范围即序号空间）：否则只提交编辑侧，未编辑端从显示
					//现值跳回构造默认值 0/10（如改起始后终止端跟着变）
					if (editingStart)
						commit(CurveProp::kMax, dispHi);
					else
						commit(CurveProp::kMin, dispLo);
				}
				commit(CurveProp::kAutoScale, false);
			}
			const int n = _categoryLabels.size();
			const int startIdx = qBound(0, qRound(lo + 0.5), n - 1);
			const int endIdx = qBound(startIdx, qRound(hi - 0.5), n - 1);
			if (editingStart && idx > endIdx)
				commit(CurveProp::kMax, double(idx) + 0.5);   //终止跟随起始
			if (!editingStart && idx < startIdx)
				commit(CurveProp::kMin, double(idx) - 0.5);   //起始跟随终止
			commit(editingStart ? CurveProp::kMin : CurveProp::kMax,
				editingStart ? double(idx) - 0.5 : double(idx) + 0.5);
			return;
		}
		//数值形态：柱状图 X 轴恒 Linear（缩放方式行在类别模式隐藏），按原始值
		//空间直读。提交值外扩"半格"（对齐类别轴语义：选中端点的柱完整框进、
		//相邻未选柱不进范围）——渲染端柱宽是像素宽，数据空间的半格取到相邻
		//数据点距离的一半；首/末点取与内侧相邻点的间距，单点数据退化 ±1
		//（同 fitChartAxes 退化范围兜底）。另一侧跟随按值比较
		if (idx >= _barXValues.size())
			return;
		double lo = doc->nodeProperty(_nodeId, CurveProp::kMin).toDouble();
		double hi = doc->nodeProperty(_nodeId, CurveProp::kMax).toDouble();
		if (doc->nodeProperty(_nodeId, CurveProp::kAutoScale).toBool())
		{
			double dispLo = 0.0, dispHi = 0.0;
			if (mCurveChartView::queryAxisDisplayRange(_nodeId, &dispLo, &dispHi))
			{
				lo = dispLo;
				hi = dispHi;
				//退出自动前先把"未编辑侧"按当前显示范围回写模型（原始值空间）：
				//否则未编辑端从显示现值跳回构造默认值 0/10（改起始终止跟着变）
				if (editingStart)
					commit(CurveProp::kMax, dispHi);
				else
					commit(CurveProp::kMin, dispLo);
			}
			commit(CurveProp::kAutoScale, false);
		}
		const double v = _barXValues.at(idx);
		const double padLo = (idx > 0)
			? (v - _barXValues.at(idx - 1)) / 2.0
			: ((_barXValues.size() > 1) ? (_barXValues.at(1) - v) / 2.0 : 1.0);
		const double padHi = (idx + 1 < _barXValues.size())
			? (_barXValues.at(idx + 1) - v) / 2.0
			: ((_barXValues.size() > 1) ? (v - _barXValues.at(idx - 1)) / 2.0 : 1.0);
		if (editingStart && v > hi)
			commit(CurveProp::kMax, v + padHi);   //终止跟随起始
		if (!editingStart && v < lo)
			commit(CurveProp::kMin, v - padLo);   //起始跟随终止
		commit(editingStart ? CurveProp::kMin : CurveProp::kMax,
			editingStart ? v - padLo : v + padHi);
	}

	void mAxisPropPage::echoDisplayRange()
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

	void mAxisPropPage::onAxisDisplayRangeChanged(int axisNodeId)
	{
		//缩放/平移联动：只联动当前绑定且处于自动范围的轴（手动范围下模型
		//min/max 即生效值，缩放不改变模型值）。阻塞信号回显——不得触发
		//commitRange（提交即退出自动）。类别轴把联动换成起始/终止回显
		if (axisNodeId != _nodeId || _loading)
			return;
		if (_categoryMode)
		{
			echoCategoryRange();
			return;
		}
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

	//--- mLegendPropPage ---

	mLegendPropPage::mLegendPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mLegendPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/自动定位/图例位置（截图08；勾选自动定位隐藏位置行，
		//避让策略见 tooltip）
		_baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		_baseFold->addRow(_visibleCheck);
		_autoPosCheck = createRowCheck(QStringLiteral("自动定位"), this);
		_autoPosCheck->setToolTip(QStringLiteral(
			"勾选自动定位，则图例位置设置隐藏。\n"
			"勾选后图例会根据图表的显示选择不遮挡曲线的位置显示，"
			"在图内左上角、图内左下角、图内右上角、图内右下角这四个方向中变动，"
			"优先图内右上角位置。"));
		_baseFold->addRow(_autoPosCheck);
		//9 位置与弹窗页 mLegendEditorPropPage 同枚举（两族页面共写一组模型键）
		_positionCombo = new QComboBox(this);
		_positionCombo->addItems({
			QStringLiteral("图内-左上角"), QStringLiteral("图内-左下角"),
			QStringLiteral("图内-右上角"), QStringLiteral("图内-右下角"),
			QStringLiteral("图内-用户自定义"),
			QStringLiteral("图外-顶部"), QStringLiteral("图外-底部"),
			QStringLiteral("图外-左侧"), QStringLiteral("图外-右侧") });
		_baseFold->addRow(QStringLiteral("图例位置"), _positionCombo);
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
		});

		//显示：边框线宽/边框颜色/字体设置/引线位置/背景填充色/曲线文本色/文本颜色
		//（截图08；引擎 mPlotLegendItem setter 已扩展，随 rebuild 生效；边框样式
		//改为线宽——线型由样式链路保留，行内图标即线宽样本，与编辑器弹窗页同款）
		_dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_borderWidthCombo = new mCurveBorderWidthCombo(this);
		_dispFold->addRow(QStringLiteral("边框线宽"), _borderWidthCombo);
		_borderColorBtn = new mCurveColorButton(Qt::black, this);
		_dispFold->addRow(QStringLiteral("边框颜色"), _borderColorBtn);
		//模型图例字体默认值显式（legendDefaultFont，渲染对齐原型）——按钮以其
		//为"默认"回显基准，模型值本身不变
		_fontBtn = new mCurveFontButton(legendDefaultFont(), this);
		_dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		_leaderPosCombo = new QComboBox(this);
		_leaderPosCombo->addItems({ QStringLiteral("左侧"), QStringLiteral("下方") });
		_dispFold->addRow(QStringLiteral("引线位置"), _leaderPosCombo);
		_bgColorBtn = new mCurveColorButton(QColor(255, 255, 255, 200), this);
		_dispFold->addRow(QStringLiteral("背景填充色"), _bgColorBtn);
		_textColorModeCombo = new QComboBox(this);
		_textColorModeCombo->addItems({ QStringLiteral("跟随曲线颜色"), QStringLiteral("自定义") });
		_dispFold->addRow(QStringLiteral("曲线文本色"), _textColorModeCombo);
		_textColorBtn = new mCurveColorButton(Qt::black, this);
		_dispFold->addRow(QStringLiteral("文本颜色"), _textColorBtn);
		connect(_borderWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kLegendBorderWidth, idx);   //0=无边框 1..4=线宽px
		});
		connect(_borderColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBorderColor, c);
		});
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kLegendFont, f);
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

		layout->addWidget(_baseFold);
		layout->addWidget(_dispFold);
		layout->addStretch(1);
	}

	void mLegendPropPage::updateRowStates()
	{
		//条件行：勾选自动定位隐藏图例位置行（避让接管）；
		//曲线文本色选"自定义"时显示文本颜色行（setRowVisible 整行收起不占位）
		_baseFold->setRowVisible(_positionCombo, !_autoPosCheck->isChecked());
		_dispFold->setRowVisible(_textColorBtn, _textColorModeCombo->currentIndex() == 1);
	}

	void mLegendPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_autoPosCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAutoPosition).toBool());
		_positionCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kPosition).toInt(), 8));
		_borderWidthCombo->setBorderWidthIndex(qBound(0,
			int(doc->nodeProperty(_nodeId, CurveProp::kLegendBorderWidth).toDouble() + 0.5), 4));
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

	//--- mHeaderPropPage ---

	mHeaderPropPage::mHeaderPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mHeaderPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/文本/类型（类型=节点身份只读回显，置灰；
		//文本=按钮弹"编辑字符"弹窗，模板文本长行内不放输入框）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		baseFold->addRow(_visibleCheck);
		_textBtn = new mCurveTextButton(this);
		baseFold->addRow(QStringLiteral("文本"), _textBtn);
		_typeEdit = new QLineEdit(this);
		_typeEdit->setEnabled(false);
		_typeEdit->setText(QStringLiteral("页眉"));
		baseFold->addRow(QStringLiteral("类型"), _typeEdit);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kHeaderVisible, on);
		});
		connect(_textBtn, &mCurveTextButton::textCommitted, this, [this](const QString& text)
		{
			commit(CurveProp::kHeaderText, text);
		});

		//显示：一级字体设置/二级字体设置/三级字体设置/对齐方式/颜色
		//（截图06；三级各为独立字体按钮，同款"选择字体"弹窗——二级入
		//kHeaderFont2、三级入 kHeaderFont3，未选按一级字体折算渲染）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_fontBtn = new mCurveFontButton(headerDefaultFont(), this);
		dispFold->addRow(QStringLiteral("一级字体设置"), _fontBtn);
		_font2Btn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("二级字体设置"), _font2Btn);
		_font3Btn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("三级字体设置"), _font3Btn);
		_alignCombo = createAlignCombo(this);
		dispFold->addRow(QStringLiteral("对齐方式"), _alignCombo);
		_colorBtn = new mCurveColorButton(Qt::black, this);
		dispFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_alignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kHeaderAlign, idx);
		});
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kHeaderFont, f);
		});
		connect(_font2Btn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kHeaderFont2, f);
		});
		connect(_font3Btn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kHeaderFont3, f);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kHeaderColor, c);
		});

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addStretch(1);
	}

	void mHeaderPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kHeaderVisible).toBool());
		_textBtn->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderText).toString());
		_alignCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kHeaderAlign).toInt(), 2));
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderFont).value<QFont>());
		_font2Btn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderFont2).value<QFont>());
		_font3Btn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kHeaderFont3).value<QFont>());
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kHeaderColor).value<QColor>());
	}

	//--- mFooterPropPage ---

	mFooterPropPage::mFooterPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mFooterPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/文本/类型（截图07；类型=节点身份只读回显，置灰；
		//文本=按钮弹"编辑字符"弹窗，同页眉页）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		baseFold->addRow(_visibleCheck);
		_textBtn = new mCurveTextButton(this);
		baseFold->addRow(QStringLiteral("文本"), _textBtn);
		_typeEdit = new QLineEdit(this);
		_typeEdit->setEnabled(false);
		_typeEdit->setText(QStringLiteral("页脚"));
		baseFold->addRow(QStringLiteral("类型"), _typeEdit);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kFooterVisible, on);
		});
		connect(_textBtn, &mCurveTextButton::textCommitted, this, [this](const QString& text)
		{
			commit(CurveProp::kFooterText, text);
		});

		//显示：一级字体设置/二级字体设置/三级字体设置/对齐方式/颜色
		//（截图07；三级各为独立字体按钮，同款"选择字体"弹窗——二级入
		//kFooterFont2、三级入 kFooterFont3，未选按一级字体折算渲染）
		mCurveFoldSection* dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_fontBtn = new mCurveFontButton(footerDefaultFont(), this);
		dispFold->addRow(QStringLiteral("一级字体设置"), _fontBtn);
		_font2Btn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("二级字体设置"), _font2Btn);
		_font3Btn = new mCurveFontButton(this);
		dispFold->addRow(QStringLiteral("三级字体设置"), _font3Btn);
		_alignCombo = createAlignCombo(this);
		dispFold->addRow(QStringLiteral("对齐方式"), _alignCombo);
		_colorBtn = new mCurveColorButton(Qt::black, this);
		dispFold->addRow(QStringLiteral("颜色"), _colorBtn);
		connect(_alignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kFooterAlign, idx);
		});
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFooterFont, f);
		});
		connect(_font2Btn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFooterFont2, f);
		});
		connect(_font3Btn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFooterFont3, f);
		});
		connect(_colorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kFooterColor, c);
		});

		layout->addWidget(baseFold);
		layout->addWidget(dispFold);
		layout->addStretch(1);
	}

	void mFooterPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kFooterVisible).toBool());
		_textBtn->setTextValue(doc->nodeProperty(_nodeId, CurveProp::kFooterText).toString());
		_alignCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kFooterAlign).toInt(), 2));
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFooterFont).value<QFont>());
		_font2Btn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFooterFont2).value<QFont>());
		_font3Btn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFooterFont3).value<QFont>());
		_colorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kFooterColor).value<QColor>());
	}

	//--- mBaselinePropPage ---

	mBaselinePropPage::mBaselinePropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mBaselinePropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/名称/标签/图层/位置/关联轴/类型（截图09；名称=树显示名，
		//标签=线上文字渲染下一期；图层占位；类型随方向只读回显置灰）
		mCurveFoldSection* baseFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
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

		//显示：字体设置/标签位置/线型/线条颜色/线宽（截图09；
		//字体接 kLabelFont（绘图选项同款弹窗选择，渲染下一期）；线型/线宽为绘图选项同款图标下拉）
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

	void mBaselinePropPage::loadFromModel()
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
		int lineStyle = doc->nodeProperty(_nodeId, CurveProp::kLineStyle).toInt();
		_lineStyleCombo->setPenStyleIndex(qBound(0, lineStyle - 1, 3));
		//线宽图标行 = 线宽px-1（非整px宽度就近取整；越界钳制到 1..4px 档位）
		_lineWidthCombo->setLineWidthIndex(qBound(0,
			int(doc->nodeProperty(_nodeId, CurveProp::kLineWidth).toDouble() + 0.5) - 1, 3));
	}

	//--- mAnnotationPropPage ---

	namespace
	{
		//位置"X Y"单输入框辅助（与弹窗注释编辑页同语义）：固定 5 位小数
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

	mAnnotationPropPage::mAnnotationPropPage(QWidget* parent)
		: mCurvePropPageBase(parent)
	{
		buildUi();
	}

	void mAnnotationPropPage::buildUi()
	{
		QVBoxLayout* layout = pageLayout(this);

		//标准：可见性/名称/文本/标量值格式化[格式/精度]/附着于/附着曲线/附着点/位置
		//（截图10；可见性默认勾选，取消即隐藏注释——渲染层不建渲染项）
		_annoFold = new mCurveFoldSection(QStringLiteral("标准"), this);
		_visibleCheck = createRowCheck(QStringLiteral("可见性"), this);
		_annoFold->addRow(_visibleCheck);
		_nameEdit = new QLineEdit(this);
		_annoFold->addRow(QStringLiteral("名称"), _nameEdit);
		//文本 = 按钮形态（回显当前注释文本），点击弹出注释文本对话框
		//（mPostAnnotationDialog，与图表双击注释同款）；保存经命令栈写回 kText。
		//占位文本"视图注释"回填默认模板，其余回填当前模板文本
		_textBtn = new QPushButton(this);
		_textBtn->setStyleSheet(
			"QPushButton{background:#FFFFFF;border:1px solid #D9DCE0;border-radius:4px;"
			"color:#333333;padding:2px 6px;text-align:left;}"
			"QPushButton:hover{background:#F5F7FA;}");
		_textBtn->setCursor(Qt::PointingHandCursor);
		_textBtn->setToolTip(QStringLiteral("点击编辑注释文本"));
		_annoFold->addRow(QStringLiteral("文本"), _textBtn);
		connect(_textBtn, &QPushButton::clicked, this, [this]()
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (!doc->node(_nodeId))
				return;
			QString tpl = doc->nodeProperty(_nodeId, CurveProp::kText).toString();
			if (tpl == kViewAnnotationPlaceholder)
				tpl = QStringLiteral("Y={Y} @ X={X}");
			PostCommon::mPostAnnotationDialog dlg(this);
			dlg.setAnnotationText(tpl);
			if (dlg.exec() != QDialog::Accepted)
				return;
			commit(CurveProp::kText, dlg.annotationText());
		});
		_scalarCheck = createRowCheck(QStringLiteral("标量值格式化"), this);
		_annoFold->addRow(_scalarCheck);
		//标量值格式化：勾选后 {X}/{Y}/{Slope}/{Curv} 等按格式（0科学计数法 1定点）与精度显示
		_scalarFmtCombo = new QComboBox(this);
		_scalarFmtCombo->addItems({ QStringLiteral("科学计数法"), QStringLiteral("定点格式") });
		_annoFold->addRow(QStringLiteral("格式"), _scalarFmtCombo);
		_scalarPrecisionSpin = createCurveSpinBox(0, 15, this);
		_scalarPrecisionSpin->setValue(5);
		_annoFold->addRow(QStringLiteral("精度"), _scalarPrecisionSpin);
		_attachModeCombo = new QComboBox(this);
		_attachModeCombo->addItems({ QStringLiteral("曲线"), QStringLiteral("视图") });
		_annoFold->addRow(QStringLiteral("附着于"), _attachModeCombo);
		_attachCurveCombo = new QComboBox(this);
		_annoFold->addRow(QStringLiteral("附着曲线"), _attachCurveCombo);
		//附着点：顺序号（1基），改值即附着到该点（视图层吸附锚点、越界回写钳制值）
		_attachIndexSpin = createCurveSpinBox(1, 1000000, this);
		_annoFold->addRow(QStringLiteral("附着点"), _attachIndexSpin);
		//位置：单输入框"X Y"空格分隔（数据坐标，固定 5 位小数回显，不足补 0）
		_posEdit = new QLineEdit(this);
		_annoFold->addRow(QStringLiteral("位置"), _posEdit);
		connect(_visibleCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kVisible, on);
		});
		connect(_nameEdit, &QLineEdit::editingFinished, this, [this]()
		{
			commit(CurveProp::kName, _nameEdit->text());
		});
		connect(_scalarCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kScalarFmtOn, on);
			updateConditionalRows();
		});
		connect(_scalarFmtCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kScalarFmtMode, idx);
		});
		connect(_scalarPrecisionSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kScalarFmtPrecision, v);
		});
		connect(_attachIndexSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kAttachIndex, v);
		});
		connect(_attachModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			//类别切换（与弹窗注释编辑页同语义）：视图->曲线先提交附着曲线再切类别
			//（吸附最近点/按曲线求值）；曲线->视图文本置占位（去引出线、不跟随曲线色）
			if (idx == 0)
			{
				commit(CurveProp::kAttachCurve, _attachCurveCombo->currentText());
				commit(CurveProp::kAttachMode, 0);
			}
			else
			{
				commit(CurveProp::kAttachMode, 1);
				commit(CurveProp::kText, kViewAnnotationPlaceholder);
			}
			updateConditionalRows();
		});
		connect(_attachCurveCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int)
		{
			//附着曲线变化：曲线类注释由图表视图重新吸附最近点并按新曲线求值文本
			commit(CurveProp::kAttachCurve, _attachCurveCombo->currentText());
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

		//选项：最小化[半径/颜色]（截图10；曲线类生效：锚点处实心小方块，悬停还原）
		_optFold = new mCurveFoldSection(QStringLiteral("选项"), this);
		_minimizeCheck = createRowCheck(QStringLiteral("最小化"), this);
		_optFold->addRow(_minimizeCheck);
		_minimizeRadiusSpin = createCurveSpinBox(0, 100, this);
		_minimizeRadiusSpin->setValue(6);
		_optFold->addRow(QStringLiteral("最小化半径"), _minimizeRadiusSpin);
		_minimizeColorBtn = new mCurveColorButton(QColor(0, 200, 0), this);
		_optFold->addRow(QStringLiteral("最小化颜色"), _minimizeColorBtn);
		connect(_minimizeCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kMinimized, on);
			updateConditionalRows();
		});
		connect(_minimizeRadiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, [this](int v)
		{
			commit(CurveProp::kMinimizeRadius, v);
		});
		connect(_minimizeColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kMinimizeColor, c);
		});

		//显示：字体设置(占位)/对齐方式/边框线宽/曲线颜色/自动定位[策略/偏移]/
		//背景颜色[选择背景颜色]/文本颜色[选择文本颜色]（截图10）
		_dispFold = new mCurveFoldSection(QStringLiteral("显示"), this);
		_fontBtn = new mCurveFontButton(this);
		_dispFold->addRow(QStringLiteral("字体设置"), _fontBtn);
		connect(_fontBtn, &mCurveFontButton::fontChanged, this, [this](const QFont& f)
		{
			commit(CurveProp::kFont, f);
		});
		_alignCombo = new QComboBox(this);
		_alignCombo->addItems({ QStringLiteral("左"), QStringLiteral("中"), QStringLiteral("右") });
		_dispFold->addRow(QStringLiteral("对齐方式"), _alignCombo);
		//边框线宽：线宽样本图标（1..4px，绘图选项曲线页同款图标）
		_borderWidthCombo = new mCurveLineWidthCombo(this);
		_dispFold->addRow(QStringLiteral("边框线宽"), _borderWidthCombo);
		//边框颜色：跟随曲线颜色/自定义（样式对齐弹窗编辑器，替换原"曲线颜色"
		//勾选框，同一模型属性 kBorderMode），选"自定义"后加显"选择边框颜色"行
		_borderColorCombo = new QComboBox(this);
		_borderColorCombo->addItems({ QStringLiteral("跟随曲线颜色"), QStringLiteral("自定义") });
		_dispFold->addRow(QStringLiteral("边框颜色"), _borderColorCombo);
		_borderColorBtn = new mCurveColorButton(Qt::black, this);
		_dispFold->addRow(QStringLiteral("选择边框颜色"), _borderColorBtn);
		_autoPosCheck = createRowCheck(QStringLiteral("自动定位"), this);
		_dispFold->addRow(_autoPosCheck);
		//自动定位策略（容差+锚点合一）：首项"不重叠"，其余为锚位
		_autoAnchorCombo = new QComboBox(this);
		_autoAnchorCombo->addItems({ QStringLiteral("不重叠"), QStringLiteral("左上"), QStringLiteral("右上"),
			QStringLiteral("左中"), QStringLiteral("居中"), QStringLiteral("右中"),
			QStringLiteral("左下"), QStringLiteral("正下"), QStringLiteral("右下") });
		_dispFold->addRow(QStringLiteral("自动定位策略"), _autoAnchorCombo);
		_autoOffsetSpin = createCurveDoubleSpin(1, 0.0, 100.0, 0.5, this);
		_autoOffsetSpin->setValue(3.0);
		_dispFold->addRow(QStringLiteral("自动定位偏移"), _autoOffsetSpin);
		_bgColorCombo = new QComboBox(this);
		_bgColorCombo->addItems({ QStringLiteral("默认"), QStringLiteral("自定义") });
		_dispFold->addRow(QStringLiteral("背景颜色"), _bgColorCombo);
		_bgColorBtn = new mCurveColorButton(Qt::white, this);
		_dispFold->addRow(QStringLiteral("选择背景颜色"), _bgColorBtn);
		_textColorCombo = new QComboBox(this);
		_textColorCombo->addItems({ QStringLiteral("默认"), QStringLiteral("自定义") });
		_dispFold->addRow(QStringLiteral("文本颜色"), _textColorCombo);
		_textColorBtn = new mCurveColorButton(QColor(51, 51, 51), this);
		_dispFold->addRow(QStringLiteral("选择文本颜色"), _textColorBtn);
		connect(_alignCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kAlign, idx);
		});
		connect(_borderWidthCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kBorderWidth, double(idx + 1));   //图标行0..3 -> 线宽1..4px
		});
		connect(_borderColorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			//跟随曲线颜色=0（边框取曲线色），自定义=1 并显示取色行
			commit(CurveProp::kBorderMode, idx);
			updateConditionalRows();
		});
		connect(_borderColorBtn, &mCurveColorButton::colorChanged, this, [this](const QColor& c)
		{
			commit(CurveProp::kBorderColor, c);
		});
		connect(_autoPosCheck, &QCheckBox::toggled, this, [this](bool on)
		{
			commit(CurveProp::kAutoPos, on);
			updateConditionalRows();
		});
		connect(_autoAnchorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, [this](int idx)
		{
			commit(CurveProp::kAutoAnchor, idx);
			updateConditionalRows();
		});
		connect(_autoOffsetSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, [this](double v)
		{
			commit(CurveProp::kAutoOffset, v);
		});
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

		//初始条件行显隐（与弹窗编辑页同款）：loadFromModel 的回显信号在索引
		//未变化时不触发（如默认跟随曲线颜色 0->0），buildUi 先建立正确初态
		updateConditionalRows();

		layout->addWidget(_annoFold);
		layout->addWidget(_optFold);
		layout->addWidget(_dispFold);
		layout->addStretch(1);
	}

	void mAnnotationPropPage::updateConditionalRows()
	{
		//标准：勾选"标量值格式化"显示格式/精度；曲线类注释显示附着曲线/附着点
		const bool scalarFmt = _scalarCheck->isChecked();
		_annoFold->setRowVisible(_scalarFmtCombo, scalarFmt);
		_annoFold->setRowVisible(_scalarPrecisionSpin, scalarFmt);
		const bool onCurve = _attachModeCombo->currentIndex() == 0;
		_annoFold->setRowVisible(_attachCurveCombo, onCurve);
		_annoFold->setRowVisible(_attachIndexSpin, onCurve);

		//选项：勾选"最小化"显示半径/颜色
		_optFold->setRowVisible(_minimizeRadiusSpin, _minimizeCheck->isChecked());
		_optFold->setRowVisible(_minimizeColorBtn, _minimizeCheck->isChecked());

		//显示：边框颜色选"自定义"显示选择边框颜色行（跟随曲线颜色=边框取曲线
		//色）；视图类整行隐藏下拉、直显取色（同弹窗编辑页）；自动定位仅曲线类
		//生效（视图类整组隐藏：勾选框/策略/偏移），勾选后显示策略行，策略非
		//"不重叠"再加显偏移；背景/文本颜色选"自定义"显示取色行
		const bool onView = !onCurve;
		_dispFold->setRowVisible(_borderColorCombo, !onView);
		_dispFold->setRowVisible(_borderColorBtn, onView ||
			_borderColorCombo->currentIndex() == 1);
		const bool autoPos = onCurve && _autoPosCheck->isChecked();
		_dispFold->setRowVisible(_autoPosCheck, onCurve);
		_dispFold->setRowVisible(_autoAnchorCombo, autoPos);
		_dispFold->setRowVisible(_autoOffsetSpin, autoPos &&
			_autoAnchorCombo->currentIndex() != 0);
		_dispFold->setRowVisible(_bgColorBtn, _bgColorCombo->currentIndex() == 1);
		_dispFold->setRowVisible(_textColorBtn, _textColorCombo->currentIndex() == 1);
	}

	//位置框回显："X Y"（固定 5 位小数，空格分隔，与弹窗编辑页同语义）
	void mAnnotationPropPage::syncPosEdit()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		_posEdit->setText(formatPosPair(doc->nodeProperty(_nodeId, CurveProp::kPosX).toDouble(),
			doc->nodeProperty(_nodeId, CurveProp::kPosY).toDouble()));
	}

	void mAnnotationPropPage::loadFromModel()
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(_nodeId))
			return;
		_visibleCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kVisible).toBool());
		_nameEdit->setText(doc->nodeProperty(_nodeId, CurveProp::kName).toString());
		_textBtn->setText(doc->nodeProperty(_nodeId, CurveProp::kText).toString());
		_scalarCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kScalarFmtOn).toBool());
		_scalarFmtCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kScalarFmtMode).toInt(), 1));
		_scalarPrecisionSpin->setValue(
			doc->nodeProperty(_nodeId, CurveProp::kScalarFmtPrecision).toInt());
		syncPosEdit();

		//显示组回填：字体/对齐/边框线宽（图标行=线宽-1）/曲线颜色勾选=跟随曲线颜色(0)/
		//背景颜色与文本颜色（无效色=默认态）
		_fontBtn->setFontValue(doc->nodeProperty(_nodeId, CurveProp::kFont).value<QFont>());
		_alignCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kAlign).toInt(), 2));
		_borderWidthCombo->setLineWidthIndex(qBound(0,
			int(doc->nodeProperty(_nodeId, CurveProp::kBorderWidth).toDouble()) - 1, 3));
		_borderColorCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kBorderMode).toInt(), 1));
		_borderColorBtn->setColor(doc->nodeProperty(_nodeId, CurveProp::kBorderColor).value<QColor>());
		const QColor bgColor = doc->nodeProperty(_nodeId, CurveProp::kBgColor).value<QColor>();
		_bgColorBtn->setColor(bgColor.isValid() ? bgColor : QColor(Qt::white));
		_bgColorCombo->setCurrentIndex(bgColor.isValid() ? 1 : 0);
		const QColor textColor = doc->nodeProperty(_nodeId, CurveProp::kColor).value<QColor>();
		_textColorBtn->setColor(textColor.isValid() ? textColor : QColor(51, 51, 51));
		_textColorCombo->setCurrentIndex(textColor.isValid() ? 1 : 0);
		_autoPosCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kAutoPos).toBool());
		_autoOffsetSpin->setValue(doc->nodeProperty(_nodeId, CurveProp::kAutoOffset).toDouble());

		//附着曲线回显：本图表的曲线名列表，选中模型记录的附着曲线；附着于按模型回填
		_attachCurveCombo->clear();
		const int chartId = doc->chartNodeId(_nodeId);
		if (chartId > 0)
		{
			for (int childId : doc->childNodeIds(chartId))
			{
				CurveNode* c = doc->node(childId);
				if (c && c->type == CurveNodeType::Curve)
					_attachCurveCombo->addItem(c->series.name);
			}
		}
		const QString attachCurve = doc->nodeProperty(_nodeId, CurveProp::kAttachCurve).toString();
		if (!attachCurve.isEmpty())
		{
			const int ci = _attachCurveCombo->findText(attachCurve);
			if (ci >= 0)
				_attachCurveCombo->setCurrentIndex(ci);
		}
		_attachModeCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kAttachMode).toInt(), 1));
		//附着点顺序（1基，0=未指定回显为1）/最小化（半径）/自动定位策略
		_attachIndexSpin->setValue(qMax(1,
			doc->nodeProperty(_nodeId, CurveProp::kAttachIndex).toInt()));
		_minimizeCheck->setChecked(doc->nodeProperty(_nodeId, CurveProp::kMinimized).toBool());
		_minimizeRadiusSpin->setValue(
			static_cast<int>(doc->nodeProperty(_nodeId, CurveProp::kMinimizeRadius).toDouble()));
		const QColor minColor = doc->nodeProperty(_nodeId, CurveProp::kMinimizeColor).value<QColor>();
		_minimizeColorBtn->setColor(minColor.isValid() ? minColor : QColor(0, 200, 0));
		_autoAnchorCombo->setCurrentIndex(qBound(0,
			doc->nodeProperty(_nodeId, CurveProp::kAutoAnchor).toInt(), 8));

		updateConditionalRows();
	}
}
