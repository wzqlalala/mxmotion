#include "mPostStepSequence.h"

#include <QCollator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPushButton>
#include <QSet>
#include <QStack>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

#include "UiMetrics.h"
#include <UiPalette.h>

#include "DerivedCase/mPostDerivedCaseStore.h"
#include "DerivedCase/mPostDerivedCaseEngine.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostGUIData.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "mxr/view/REBUILD_PostRend.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace PostWindow
{
	namespace
	{
		//颜色令牌（V1.3 原型 #derivedLoadParameters/superposition-* 样式块 --std-* 浅色标准档）；
		//尺寸令牌经 MxFunctions::UiMetrics 取档位值（RebuildFunctions UI 体系，禁就地写死像素）
		const char* kText = "#303313";		//--std-text / --std-label（V1.3 统一主文本）
		const char* kMuted = "#788391";		//--std-muted
		const char* kDisabled = "#A6AFBB";	//--std-disabled
		const char* kBorder = "#DCE2E9";	//--std-border
		const char* kHeadBg = "#E9EDF3";	//--std-header-color（superposition th 底）
		const char* kError = "#BF3434";		//--std-error
		const char* kHover = "#F0F7FF";		//--std-hover

		//操作行文字钮（全选）：透明底、悬停浅蓝（原型 label-defaults-button 族）
		const char* kFooterBtnQss =
			"QPushButton{border:0;background:transparent;color:#303313;border-radius:4px;padding:0 6px;}"
			"QPushButton:hover{background:#F0F7FF;}"
			"QPushButton:disabled{color:#A6AFBB;background:transparent;}";

		//行内缩放系数输入（原型 .superposition-scale：无边框透明底、padding 0 4px、高 24）
		const char* kScaleEditQss =
			"QLineEdit{border:0;background:transparent;padding:0 4px;color:#303313;}";
		//无效数值（原型 aria-invalid 视觉：红字）
		const char* kScaleEditInvalidQss =
			"QLineEdit{border:0;background:transparent;padding:0 4px;color:#BF3434;}";

		//2x 采样绘制线条图标（原型 inline SVG 语义）
		QIcon makeIcon(int px, const QColor& color, const std::function<void(QPainter&)>& paint)
		{
			QPixmap pm(px * 2, px * 2);
			pm.setDevicePixelRatio(2.0);
			pm.fill(Qt::transparent);
			QPainter p(&pm);
			p.setRenderHint(QPainter::Antialiasing);
			QPen pen(color, 1.4);
			pen.setCapStyle(Qt::RoundCap);
			pen.setJoinStyle(Qt::RoundJoin);
			p.setPen(pen);
			paint(p);
			return QIcon(pm);
		}

		//编辑铅笔（原型 ui-edit-icon path "m14 5 5 5M4 20l4-1L20 7a2 2 0 0 0-4-4L4 15z"，16 viewBox：
		//笔身斜线 + 笔尖短边，弧段以两条短线近似，16px 内视觉等效）
		QIcon pencilIcon(const QColor& color)
		{
			return makeIcon(16, color, [](QPainter& p)
				{
					p.drawLine(QPointF(13.5, 5.5), QPointF(18.5, 10.5));
					p.drawLine(QPointF(15.8, 3.2), QPointF(20.8, 8.2));
					p.drawLine(QPointF(15.8, 3.2), QPointF(13.5, 5.5));
					p.drawLine(QPointF(18.5, 10.5), QPointF(20.8, 8.2));
					p.drawLine(QPointF(4, 20), QPointF(8, 19));
					p.drawLine(QPointF(8, 19), QPointF(18.5, 8.5));
				});
		}

		//自然排序（工况/帧名与 mPost3DResult 的 sortQStringList 同语义）
		QStringList naturalSorted(QStringList list)
		{
			QCollator collator;
			collator.setNumericMode(true);
			collator.setCaseSensitivity(Qt::CaseInsensitive);
			std::sort(list.begin(), list.end(), [&collator](const QString& a, const QString& b)
				{ return collator.compare(a, b) < 0; });
			return list;
		}

		//主操作蓝按钮（生成；--std-action/-hover。与 mPostLinearSuperposition 同款，
		//UiPalette 令牌取值——生成钮为软件超原型能力，样式对齐线性叠加范式）
		QString primaryButtonStyle()
		{
			return QString(
				"QPushButton{ font-family:'Microsoft YaHei'; background:%1; border:1px solid %1; color:#FFFFFF; border-radius:%2px; font-size:%3px; }"
				"QPushButton:hover{ background:%4; border:1px solid %4; }"
				"QPushButton:pressed{ background:%4; border:1px solid %4; }"
				"QPushButton:disabled{ background:%5; border:1px solid %6; color:%7; }")
				.arg(UiPalette::actionHex())
				.arg(UiMetrics::zoneRadius())
				.arg(UiMetrics::bodyFontSize())
				.arg(UiPalette::actionHoverHex())
				.arg(UiPalette::panelHex(), UiPalette::borderHex(), UiPalette::disabledHex());
		}

		//表格线委托（原型 M2/M3：列间竖线 + 行底线含末行；跨列行不画竖线）
		class GridLineDelegate : public QStyledItemDelegate
		{
		public:
			using QStyledItemDelegate::QStyledItemDelegate;
			void paint(QPainter* painter, const QStyleOptionViewItem& option,
				const QModelIndex& index) const override
			{
				QStyledItemDelegate::paint(painter, option, index);
				painter->save();
				QPen pen(QColor(kBorder), 1.0);
				painter->setPen(pen);
				const QRect r = option.rect;
				//行底线：末行也画（"最后一条数据行保留1px细线"），跨列行（组头/空态）同样画底
				painter->drawLine(QPointF(r.left() + 0.5, r.bottom() - 0.5),
					QPointF(r.right() + 0.5, r.bottom() - 0.5));
				//列间竖线：非最后列、且该行非跨列行（跨列标记 UserRole+2）
				const QAbstractItemModel* model = index.model();
				const bool spanRow = model->data(index, Qt::UserRole + 2).toBool();
				if (!spanRow && index.column() < model->columnCount(index.parent()) - 1)
					painter->drawLine(QPointF(r.right() + 0.5, r.top() + 0.5),
						QPointF(r.right() + 0.5, r.bottom() - 0.5));
				painter->restore();
			}
		};
	}

	mPostStepSequencePanel::mPostStepSequencePanel(QWidget* parent) : mPanelBase(parent)
	{
		setupUI();

		//切换界面语言（随全局语言信号刷新）
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));
		slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

		this->setMinimumWidth(319);	//原型侧栏宽（derivedLoadParameters 面板宽，与标签/测量面板同族）
	}

	mPostStepSequencePanel::~mPostStepSequencePanel() = default;

	void mPostStepSequencePanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		//原型 parameter-panel：内容左右 4、顶部 6（与标签/测量面板同款）
		_mainLayout->setContentsMargins(4, 6, 4, 0);

		const int control = UiMetrics::controlHeight();	//标准档 26

		/********************步序列定义********************/
		_definitionFold = new FoldSection(QString::fromUtf8("步序列定义"), this);

		_nameLabel = new PanelLabel;
		_nameLabel->setText(QString::fromUtf8("派生工况"));
		_editName = new QLineEdit(_definitionFold);
		_editName->setObjectName("stepSeqName");
		_editName->setFixedHeight(control);
		applyNameEditQss(false);
		connect(_editName, &QLineEdit::textChanged, this, &mPostStepSequencePanel::commit);
		_definitionFold->addRow(_nameLabel, _editName);
		_mainLayout->addWidget(_definitionFold);

		/********************工况（来源管理表）********************/
		_nativeFold = new FoldSection(QString::fromUtf8("工况"), this);
		//跨列行几何=表单边距（同标签面板管理组口径）：边距保持 0，底边距 4 补组间距
		if (auto* form = _nativeFold->layout())
		{
			form->setContentsMargins(0, 10, 0, 4);
		}

		//搜索框（原型 superposition-search：全宽 26 高）
		_searchEdit = new QLineEdit(_nativeFold);
		_searchEdit->setObjectName("stepSeqSearch");
		_searchEdit->setClearButtonEnabled(true);
		_searchEdit->setFixedHeight(control);
		_searchEdit->setStyleSheet(
			QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
				"padding:0 %3px;color:%4;}")
			.arg(kBorder).arg(UiMetrics::zoneRadius()).arg(UiMetrics::menuItemInset()).arg(kText));
		connect(_searchEdit, &QLineEdit::textChanged, this, &mPostStepSequencePanel::slot_searchChanged);
		_nativeFold->addRow(_searchEdit);

		//来源工况表（原型 superposition-table：复选 26 / 原生工况 stretch / 帧 90 / 缩放系数 72）
		_table = new QTableWidget(0, 4, this);
		_table->setObjectName("stepSeqTable");
		//表头 26 高 / 12px / #E9EDF3 底 / #303313 字（V1.3 superposition th 专值；
		//实参与占位符严格一一对应——踩坑 35）
		_table->setStyleSheet(
			QString("QTableWidget{background:#FFFFFF;border:1px solid %1;border-radius:0;"
				"selection-background-color:#E8F3FF;selection-color:%2;}"
				"QHeaderView::section{background:%3;color:%4;font-size:%5px;font-weight:400;height:%6px;"
				"border:0;border-bottom:1px solid %1;border-right:1px solid %1;padding:0 %7px;}")
			.arg(QString(kBorder), QString(kText), QString(kHeadBg), QString(kText))
			.arg(QString::number(UiMetrics::bodyFontSize()))
			.arg(QString::number(control))
			.arg(QString::number(UiMetrics::menuItemInset())));
		_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
		_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
		_table->setColumnWidth(0, control);												//复选列 26（--std-manager-row）
		_table->setColumnWidth(2, 5 * 12 + 2 * 7 + 12 + 4);								//帧列 90（原型 col3 公式）
		_table->setColumnWidth(3, 3 * 12 + UiMetrics::quickButtonSize() + 3 * 4);		//缩放列 72（原型 col4 公式）
		_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		_table->setSelectionMode(QAbstractItemView::SingleSelection);
		_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_table->verticalHeader()->setVisible(false);
		_table->verticalHeader()->setDefaultSectionSize(control);	//M2：数据行高=表头=26
		_table->setWordWrap(false);
		_table->setTextElideMode(Qt::ElideRight);
		_table->setShowGrid(false);
		_table->setItemDelegate(new GridLineDelegate(_table));
		_table->horizontalHeader()->setSortIndicatorShown(false);
		connect(_table, &QTableWidget::itemChanged, this, &mPostStepSequencePanel::slot_itemChanged);

		//表格高度：min 7 行 184 / max 21 行 548（原型 superposition-table-wrap）
		_table->setMinimumHeight(7 * control + 2);
		_table->setMaximumHeight(21 * control + 2);
		QWidget* tableHost = new QWidget(this);
		QVBoxLayout* tableLay = new QVBoxLayout(tableHost);
		tableLay->setContentsMargins(0, 0, 0, 0);
		tableLay->addWidget(_table);
		_nativeFold->addRow(tableHost);

		//操作行：[全选][已选 N 项]…[缩放系数 label][批量连体框 80]（原型 superposition-actions）
		QWidget* actionsRow = new QWidget(this);
		QHBoxLayout* actionsLay = new QHBoxLayout(actionsRow);
		actionsLay->setContentsMargins(0, 0, 0, 0);
		actionsLay->setSpacing(UiMetrics::choiceGap());
		_btnSelectAll = new QPushButton(actionsRow);
		_btnSelectAll->setObjectName("stepSeqSelectAll");
		_btnSelectAll->setStyleSheet(kFooterBtnQss);
		_btnSelectAll->setFixedSize(UiMetrics::baseWmin(), control);	//两字钮 40×26，钉死防全局 QSS 撑宽
		_btnSelectAll->setEnabled(false);
		connect(_btnSelectAll, &QPushButton::clicked, this, &mPostStepSequencePanel::slot_toggleAll);
		actionsLay->addWidget(_btnSelectAll);
		//已选计数（原型 measurement-selected-count）
		_countLabel = new QLabel(actionsRow);
		_countLabel->setStyleSheet(
			QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(kMuted).arg(UiMetrics::helperFontSize()));
		actionsLay->addWidget(_countLabel);
		actionsLay->addSpacing(2);
		actionsLay->addStretch(1);
		//批量缩放块（原型 superposition-batch：右对齐 label + 连体框 80×26）
		_scaleLabel = new QLabel(actionsRow);
		_scaleLabel->setStyleSheet(
			QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(kText).arg(UiMetrics::bodyFontSize()));
		actionsLay->addWidget(_scaleLabel);
		QWidget* scaleSetting = new QWidget(actionsRow);
		scaleSetting->setObjectName("stepSeqScaleSetting");
		scaleSetting->setFixedSize(2 * UiMetrics::baseWmin(), control);	//连体框 80×26（两段均分 40）
		//连体容器描边圆角 4 在外层（原型 superposition-scale-setting），内部两段直角无内缝
		scaleSetting->setStyleSheet(
			QString("#stepSeqScaleSetting{background:#FFFFFF;border:1px solid %1;border-radius:%2px;}")
			.arg(kBorder).arg(UiMetrics::zoneRadius()));
		QHBoxLayout* scaleLay = new QHBoxLayout(scaleSetting);
		scaleLay->setContentsMargins(0, 0, 0, 0);
		scaleLay->setSpacing(0);
		_batchScaleEdit = new QLineEdit(scaleSetting);
		_batchScaleEdit->setObjectName("stepSeqBatchScale");
		_batchScaleEdit->setFixedSize(UiMetrics::baseWmin(), UiMetrics::quickButtonSize());	//40×24，钉死防长文本 minHint 撑破连体框（踩坑 38）
		_batchScaleEdit->setText(QStringLiteral("1"));
		_batchScaleEdit->setAlignment(Qt::AlignCenter);
		_batchScaleEdit->setStyleSheet(
			QString("QLineEdit{border:0;background:transparent;padding:0 %1px;color:%2;}")
			.arg(UiMetrics::choiceGap()).arg(kText));
		_batchScaleEdit->setDisabled(true);
		scaleLay->addWidget(_batchScaleEdit, 1);
		_btnApplyScale = new QPushButton(scaleSetting);
		_btnApplyScale->setObjectName("stepSeqApplyScale");
		_btnApplyScale->setFixedSize(UiMetrics::baseWmin(), UiMetrics::quickButtonSize());	//40×24，钉死防全局 QSS width:80 撑破连体框
		_btnApplyScale->setDisabled(true);
		_btnApplyScale->setStyleSheet(
			QString("QPushButton{border:0;border-left:1px solid %1;background:transparent;"
				"color:%2;border-radius:0;padding:0;}")
			.arg(kBorder, kText)
			+ QString("QPushButton:hover{background:%1;}").arg(kHover)
			+ QString("QPushButton:disabled{color:%1;background:transparent;}").arg(kDisabled));
		connect(_btnApplyScale, &QPushButton::clicked, this, &mPostStepSequencePanel::slot_applyBatchScale);
		scaleLay->addWidget(_btnApplyScale, 1);
		actionsLay->addWidget(scaleSetting);
		_nativeFold->addRow(actionsRow);

		//状态反馈行（原型 superposition-feedback：11px，错误红字，空则隐藏）
		_feedback = new QLabel(this);
		_feedback->setObjectName("stepSeqFeedback");
		_feedback->setWordWrap(true);
		_feedback->setStyleSheet(
			QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;padding:0 2px;}")
			.arg(kError).arg(UiMetrics::helperFontSize()));
		_feedback->hide();
		_nativeFold->addRow(_feedback);

		//生成（软件实际能力，超原型：物化派生工况帧进当前云图视口；原型只保存定义
		//不计算，范式对齐线性叠加生成钮）
		_buttonGenerate = new QPushButton(QString::fromUtf8("生成"), this);
		_buttonGenerate->setObjectName("stepSeqGenerate");
		_buttonGenerate->setFixedHeight(UiMetrics::baseH());
		_buttonGenerate->setStyleSheet(primaryButtonStyle());
		connect(_buttonGenerate, &QPushButton::clicked, this, &mPostStepSequencePanel::slot_generateSteps);
		_nativeFold->addRow(_buttonGenerate);

		_mainLayout->addWidget(_nativeFold);
		_mainLayout->addStretch(1);

		renderTable();
	}

	void mPostStepSequencePanel::applyNameEditQss(bool invalid)
	{
		_editName->setStyleSheet(
			QString("QLineEdit{background:#FFFFFF;border:1px solid %1;border-radius:%2px;"
				"padding:0 %3px;color:%4;}")
			.arg(invalid ? kError : kBorder)
			.arg(UiMetrics::zoneRadius())
			.arg(UiMetrics::menuItemInset())
			.arg(kText));
	}

	void mPostStepSequencePanel::activateFromMenu()
	{
		resolveContext();
		newDefinition();
	}

	void mPostStepSequencePanel::updateStepSequencePanel()
	{
		resolveContext();
		if (_editingId < 0)
			return;
		//编辑定义不属当前模型桶（切模型/模型删除）：复位编辑态防陈旧草稿渲染
		if (!mPostDerivedCaseStore::instance().definition(_storeKey, _editingId))
		{
			_editingId = -1;
			_rows.clear();
			_editName->blockSignals(true);
			_editName->clear();
			_editName->blockSignals(false);
			applyNameEditQss(false);
			showMessage(QString());
			renderTable();
			return;
		}
		renderTable();
	}

	void mPostStepSequencePanel::resolveContext()
	{
		_postDB = nullptr;
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend && postRend->getPostRender())
		{
			const QString postName = postRend->getPostDataName();
			if (!postName.isEmpty())
			{
				_postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
				if (_postDB)
					_storeKey = getCVM->getModelData()->getModelDataID();	//store 桶键随有效上下文更新
			}
		}
	}

	QList<DerivedCaseDefinition> mPostStepSequencePanel::kindDefinitions() const
	{
		if (_storeKey < 0)
			return {};
		return mPostDerivedCaseStore::instance().definitions(_storeKey, DerivedCaseKind::Steps);
	}

	void mPostStepSequencePanel::newDefinition()
	{
		//名称"步序列-N"查重递增（原型 createDerivedCase；名称域为同桶全 kind 共用）
		const QString label = _langEn ? QStringLiteral("Steps") : QString::fromUtf8("步序列");

		DerivedCaseDefinition def;
		def.kind = DerivedCaseKind::Steps;
		def.name = mPostDerivedCaseStore::instance().uniqueName(_storeKey, label);
		const int newId = mPostDerivedCaseStore::instance().appendDefinition(_storeKey, def);
		if (newId < 0)
			return;	//防御：uniqueName 保证不重名，正常不可达
		_editingId = newId;
		_rows.clear();	//全新草稿，不继承上一编辑的勾选（原型 draft.rows=sources()）
		rebuildSources();
		_editName->blockSignals(true);
		_editName->setText(def.name);
		_editName->blockSignals(false);
		applyNameEditQss(false);
		showMessage(QString());
		renderTable();
	}

	void mPostStepSequencePanel::editDefinition(int definitionId)
	{
		resolveContext();	//树回编入口可能未经菜单激活：先绑定当前后处理上下文
		DerivedCaseDefinition def;
		if (!mPostDerivedCaseStore::instance().definition(_storeKey, definitionId, &def)
			|| def.kind != DerivedCaseKind::Steps)
			return;
		_editingId = def.id;
		_rows.clear();	//丢弃上一编辑的未保存草稿（原型 editDefinition 直接覆盖 draft）
		rebuildSources();
		//目标定义已保存行全部勾选并保留系数（原型 editDefinition 的 saved 合并）
		for (const DerivedSourceRow& saved : def.rows)
		{
			bool found = false;
			for (SourceRow& row : _rows)
			{
				if (row.id == saved.id)
				{
					row.enabled = !row.isCurrent;
					row.scale = saved.scale;
					found = true;
					break;
				}
			}
			if (!found)
			{
				//已保存行在当前来源中消失：保留为缺失行（仍勾选，提交时被校验拦截）
				SourceRow row;
				row.id = saved.id;
				row.name = saved.name;
				row.frame = saved.frame;
				row.scale = saved.scale;
				row.enabled = true;
				row.isDerived = saved.isDerived;
				row.derivedId = saved.derivedId;
				row.missing = true;
				_rows.append(row);
			}
		}
		_editName->blockSignals(true);
		_editName->setText(def.name);
		_editName->blockSignals(false);
		applyNameEditQss(false);
		showMessage(QString());
		renderTable();
	}

	void mPostStepSequencePanel::rebuildSources()
	{
		//当前编辑草稿的勾选/系数状态（跨来源刷新保留，原型 nativeRows 的 byId 合并）
		QHash<QString, SourceRow> savedById;
		for (const SourceRow& row : _rows)
			savedById.insert(row.id, row);

		QList<SourceRow> rows;
		//原生来源：当前结果库 工况×帧 笛卡尔积（原型 sources() 原生段）
		if (_postDB)
		{
			const QStringList caseNames = naturalSorted(_postDB->getAllCaseNames());
			for (const QString& caseName : caseNames)
			{
				const QStringList frameNames = naturalSorted(_postDB->getCaseFrameNames(caseName));
				for (const QString& frameName : frameNames)
				{
					SourceRow row;
					row.id = caseName + QLatin1Char('|') + frameName;
					row.name = caseName;
					row.frame = frameName;
					rows.append(row);
				}
			}
		}

		//派生来源：既有步序列定义展开（原型 sources() 派生段）
		const QList<DerivedCaseDefinition> defs = kindDefinitions();
		for (const DerivedCaseDefinition& def : defs)
		{
			if (def.id == _editingId)
			{
				//当前编辑定义自身：单行"当前"占位，禁止勾选
				SourceRow row;
				row.id = QStringLiteral("derived:%1|current").arg(def.id);
				row.name = def.name;
				row.frame = QStringLiteral("—");
				row.isDerived = true;
				row.derivedId = def.id;
				row.isCurrent = true;
				row.disabledReason = trStepSelfSourceDenied();
				rows.append(row);
				continue;
			}
			if (!def.configured || def.rows.isEmpty())
			{
				//未配置定义：单行"待配置"占位，禁止勾选
				SourceRow row;
				row.id = QStringLiteral("derived:%1|0").arg(def.id);
				row.name = def.name;
				row.frame = QStringLiteral("—");
				row.isDerived = true;
				row.derivedId = def.id;
				row.unconfigured = true;
				row.disabledReason = trStepUnconfiguredDenied();
				rows.append(row);
				continue;
			}
			//已配置定义：每步一行"第 N 步"，逐行做引用校验
			int step = 0;
			for (const DerivedSourceRow& saved : def.rows)
			{
				++step;
				SourceRow row;
				row.id = QStringLiteral("derived:%1|%2").arg(def.id).arg(saved.id);
				row.name = def.name;
				row.frame = trStepN().arg(step);
				row.isDerived = true;
				row.derivedId = def.id;
				row.scale = saved.scale;
				row.disabledReason = referenceDeniedReason(row.id, def.id);
				rows.append(row);
			}
		}

		//勾选/系数合并：以旧草稿状态为准（当前行恒不可选；原型 enabled=!current&&saved.enabled）
		QSet<QString> freshIds;
		for (SourceRow& row : rows)
		{
			freshIds.insert(row.id);
			const auto savedIt = savedById.constFind(row.id);
			if (savedIt != savedById.constEnd())
			{
				row.enabled = !row.isCurrent && savedIt->enabled;
				row.scale = savedIt->scale;
			}
		}
		//草稿中已启用但来源消失的行：保留为缺失（原型 nativeRows 尾段）
		for (const SourceRow& saved : _rows)
		{
			if (saved.enabled && !saved.isCurrent && !freshIds.contains(saved.id))
			{
				SourceRow row = saved;
				row.missing = true;
				rows.append(row);
			}
		}
		_rows = rows;
	}

	QString mPostStepSequencePanel::trStepSelfSourceDenied() const
	{
		return _langEn ? QStringLiteral("The current case cannot be its own source.")
			: QString::fromUtf8("当前工况不能作为自身来源。");
	}

	QString mPostStepSequencePanel::trStepUnconfiguredDenied() const
	{
		return _langEn ? QStringLiteral("This derived case is unconfigured. Configure it first.")
			: QString::fromUtf8("该派生工况待配置，请先配置有效来源。");
	}

	QString mPostStepSequencePanel::trStepN() const
	{
		return _langEn ? QStringLiteral("Step %1") : QString::fromUtf8("第 %1 步");
	}

	QString mPostStepSequencePanel::referenceDeniedReason(const QString& candidateRowId, int candidateDerivedId)
	{
		/*
		* 原型 validateDefinition 的单一实现收敛在 mPostDerivedCaseStore::
		* checkReferences（环/两层上限/引用目标缺失；编辑草稿以 draftRefs 模拟，
		* candidate 模拟"再勾选一行"）。文案归面板（中英随语言）。
		*/
		QList<int> draftRefs;
		for (const SourceRow& row : _rows)
			if (row.enabled && row.isDerived && row.derivedId > 0)
				draftRefs << row.derivedId;
		const mPostDerivedCaseStore::ReferenceError error =
			mPostDerivedCaseStore::checkReferences(
				mPostDerivedCaseStore::instance().definitions(_storeKey),
				_editingId, draftRefs,
				(candidateDerivedId > 0 && !candidateRowId.isEmpty()) ? candidateDerivedId : -1);
		switch (error)
		{
		case mPostDerivedCaseStore::ReferenceError::MissingTarget:
			return _langEn ? QStringLiteral("The selected derived case is unavailable. Re-select it.")
				: QString::fromUtf8("所选派生工况不可用，请重新选择。");
		case mPostDerivedCaseStore::ReferenceError::Cycle:
			return _langEn ? QStringLiteral("Self-reference or circular reference is not allowed.")
				: QString::fromUtf8("不能引用自身或形成循环引用。");
		case mPostDerivedCaseStore::ReferenceError::DepthExceeded:
			return _langEn ? QStringLiteral("At most two levels of derived case reference are supported.")
				: QString::fromUtf8("最多支持两层派生工况引用。");
		default:
			return QString();
		}
	}

	bool mPostStepSequencePanel::validNumber(const QString& value)
	{
		bool ok = false;
		const double number = value.trimmed().toDouble(&ok);
		return !value.trimmed().isEmpty() && ok && qIsFinite(number);
	}

	bool mPostStepSequencePanel::commit()
	{
		if (_editingId < 0)
			return false;
		mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();
		DerivedCaseDefinition def;
		if (!store.definition(_storeKey, _editingId, &def))
		{
			showMessage(_langEn ? QStringLiteral("The derived case being edited no longer exists.")
				: QString::fromUtf8("当前派生工况已不存在。"), true);
			return false;
		}

		const QString name = _editName->text().trimmed();
		if (name.isEmpty())
		{
			applyNameEditQss(true);	//原型 aria-invalid 红框
			showMessage(_langEn ? QStringLiteral("Enter a derived case name.")
				: QString::fromUtf8("请输入派生工况名称。"), true);
			return false;
		}
		if (store.nameExists(_storeKey, name, _editingId))
		{
			applyNameEditQss(true);	//原型重名同样标 aria-invalid
			showMessage(_langEn ? QStringLiteral("This derived case name already exists.")
				: QString::fromUtf8("该派生工况名称已存在。"), true);
			return false;
		}
		//批次A1：派生工况以定义名为工况名物化（appendDerivedFrame），不得与分析工况
		//重名（重名工况在下拉/树两侧歧义、getFrameNumByCaseAndFrameName 解析错位）
		if (_postDB && _postDB->getAllCaseNames().contains(name))
		{
			applyNameEditQss(true);
			showMessage(_langEn ? QStringLiteral("This name conflicts with a native case name.")
				: QString::fromUtf8("该名称与分析工况重名，请修改。"), true);
			return false;
		}
		for (const SourceRow& row : _rows)
		{
			if (!row.enabled || row.isCurrent)
				continue;
			if (row.missing)
			{
				showMessage(_langEn ? QStringLiteral("A selected source case is unavailable. Re-select sources.")
					: QString::fromUtf8("所选来源工况不可用，请重新选择。"), true);
				return false;
			}
			if (!row.disabledReason.isEmpty())
			{
				showMessage(row.disabledReason, true);
				return false;
			}
		}
		for (const SourceRow& row : _rows)
		{
			if (row.enabled && !row.isCurrent && !validNumber(row.scale))
			{
				showMessage(_langEn ? QStringLiteral("Scale factors of the selected cases must be finite numbers.")
					: QString::fromUtf8("所选工况的缩放系数须为有限数值。"), true);
				return false;
			}
		}
		//整体引用校验兜底（原型 validateDefinition(definition)：以当前启用行为模拟集，
		//拦截"勾选后其他派生行禁用态尚未刷新"窗口期写入的循环/超两层引用）
		const QString referenceError = referenceDeniedReason(QString(), -1);
		if (!referenceError.isEmpty())
		{
			showMessage(referenceError, true);
			return false;
		}

		//写回定义（rows 顺序即步序）
		const QString oldName = def.name;	//改名联动派生工况名（批次A1：已物化定义改名同步）
		def.name = name;
		def.rows.clear();
		for (const SourceRow& row : _rows)
		{
			if (!row.enabled || row.isCurrent)
				continue;
			DerivedSourceRow saved;
			saved.id = row.id;
			saved.name = row.name;
			saved.frame = row.frame;
			saved.scale = row.scale;
			saved.isDerived = row.isDerived;
			saved.derivedId = row.derivedId;
			def.rows.append(saved);
		}
		def.configured = !def.rows.isEmpty();
		store.updateDefinition(_storeKey, def);
		//改名联动：旧名已物化（已生成派生工况）时同步结果库帧上的工况名，
		//未生成时 renameDerivedCase 不命中、零动作
		if (oldName != name && _postDB)
			_postDB->renameDerivedCase(oldName, name);
		applyNameEditQss(false);
		showMessage(QString());
		//重命名即时同步"当前"行的表格显示（原型 commit 直接改 DOM；不整表重建保输入焦点）
		const QString currentRowId = QStringLiteral("derived:%1|current").arg(_editingId);
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			QTableWidgetItem* checkItem = _table->item(r, 0);
			if (checkItem && checkItem->data(Qt::UserRole).toString() == currentRowId)
			{
				if (QTableWidgetItem* nameItem = _table->item(r, 1))
				{
					nameItem->setText(name);
					nameItem->setToolTip(name);
				}
				break;
			}
		}
		return true;
	}

	void mPostStepSequencePanel::renderTable()
	{
		if (!_table)
			return;
		_rebuilding = true;

		//搜索过滤（原型 visibleNative：工况名+帧文本包含匹配，大小写不敏感）
		const QString query = _nativeQuery.trimmed().toLower();
		QList<int> visible;
		for (int i = 0; i < _rows.size(); ++i)
		{
			const SourceRow& row = _rows.at(i);
			if ((row.name + QLatin1Char(' ') + row.frame).toLower().contains(query))
				visible.append(i);
		}

		_table->clearSpans();	//空态跨列 span 残留清理（clearContents 不清 span）
		_table->clearContents();
		const bool hasDerivedVisible = std::any_of(visible.cbegin(), visible.cend(),
			[this](int i) { return _rows.at(i).isDerived; });
		const int rowCount = visible.size() + (hasDerivedVisible ? 1 : 0) + (visible.isEmpty() ? 1 : 0);
		_table->setRowCount(rowCount);

		QFont cellFont = _table->font();
		cellFont.setPixelSize(UiMetrics::bodyFontSize());	//12px
		cellFont.setWeight(QFont::Normal);
		QFont helperFont = _table->font();
		helperFont.setPixelSize(UiMetrics::helperFontSize());	//11px

		int r = 0;
		bool derivedHeaderPlaced = false;
		for (int rowIdx : visible)
		{
			const SourceRow& row = _rows.at(rowIdx);
			//首条可见派生行前插组头（原生组无组头——表头"原生工况"列已表明）
			if (row.isDerived && !derivedHeaderPlaced)
			{
				const QStringList headText = QStringList
				{
					QString(),
					_langEn ? QStringLiteral("Derived Case") : QString::fromUtf8("派生工况"),
					_langEn ? QStringLiteral("Frame") : QString::fromUtf8("帧"),
					_langEn ? QStringLiteral("Scale") : QString::fromUtf8("缩放系数")
				};
				for (int c = 0; c < 4; ++c)
				{
					QTableWidgetItem* item = new QTableWidgetItem(headText.at(c));
					item->setFlags(Qt::ItemIsEnabled);
					item->setData(Qt::UserRole + 2, true);	//跨列行：委托不画竖线
					item->setBackground(QBrush(QColor(kHeadBg)));
					item->setForeground(QBrush(QColor(kText)));
					item->setFont(cellFont);
					item->setTextAlignment(c == 0 ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignVCenter));
					_table->setItem(r, c, item);
				}
				++r;
				derivedHeaderPlaced = true;
			}

			//复选列（可勾选判定同原型 check.disabled：当前行恒禁；禁用行已勾选可取消）
			QTableWidgetItem* checkItem = new QTableWidgetItem;
			checkItem->setData(Qt::UserRole, row.id);
			checkItem->setFont(cellFont);
			Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
			if (row.isCurrent || (!row.disabledReason.isEmpty() && !row.enabled))
				flags &= ~(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			checkItem->setFlags(flags);
			checkItem->setCheckState(row.enabled ? Qt::Checked : Qt::Unchecked);
			if (!row.disabledReason.isEmpty())
				checkItem->setToolTip(row.disabledReason);
			_table->setItem(r, 0, checkItem);

			//工况名（缺失来源带"（来源不可用）"后缀；缺失/当前行 muted 色）
			QString nameText = row.name;
			if (row.missing)
				nameText += _langEn ? QStringLiteral(" (source unavailable)")
					: QString::fromUtf8("（来源不可用）");
			QTableWidgetItem* nameItem = new QTableWidgetItem(nameText);
			nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			nameItem->setToolTip(nameText);
			nameItem->setFont(cellFont);
			if (row.missing || row.isCurrent)
				nameItem->setForeground(QBrush(QColor(kMuted)));
			_table->setItem(r, 1, nameItem);

			//帧文本
			QTableWidgetItem* frameItem = new QTableWidgetItem(row.frame);
			frameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			frameItem->setToolTip(row.frame);
			frameItem->setFont(cellFont);
			_table->setItem(r, 2, frameItem);

			//缩放系数列：当前/待配置=状态标签；其余=行内输入；派生行尾加编辑钮
			QWidget* cell = new QWidget;
			cell->setFixedHeight(UiMetrics::quickButtonSize());	//行内控件高 24（--std-manager-inner）
			QHBoxLayout* cellLay = new QHBoxLayout(cell);
			cellLay->setContentsMargins(0, 0, 0, 0);
			cellLay->setSpacing(UiMetrics::choiceGap());
			if (row.isCurrent || row.unconfigured)
			{
				QLabel* status = new QLabel(row.isCurrent
					? (_langEn ? QStringLiteral("Current") : QString::fromUtf8("当前"))
					: (_langEn ? QStringLiteral("Unconfigured") : QString::fromUtf8("待配置")), cell);
				status->setFont(helperFont);
				status->setStyleSheet(
					QString("QLabel{color:%1;background:transparent;border:0;}").arg(kMuted));
				cellLay->addWidget(status, 1);
			}
			else
			{
				QLineEdit* scaleEdit = new QLineEdit(cell);
				scaleEdit->setFixedHeight(UiMetrics::quickButtonSize());
				scaleEdit->setText(row.scale);
				scaleEdit->setStyleSheet(validNumber(row.scale) ? kScaleEditQss : kScaleEditInvalidQss);
				scaleEdit->setEnabled(row.disabledReason.isEmpty() || row.enabled);
				scaleEdit->setFont(cellFont);
				const QString rowId = row.id;
				connect(scaleEdit, &QLineEdit::textChanged, this,
					[this, rowId, scaleEdit](const QString& text)
					{
						for (SourceRow& item : _rows)
						{
							if (item.id != rowId)
								continue;
							item.scale = text;
							scaleEdit->setStyleSheet(validNumber(text) ? kScaleEditQss : kScaleEditInvalidQss);
							break;
						}
						commit();
					});
				cellLay->addWidget(scaleEdit, 1);
			}
			if (row.isDerived)
			{
				//编辑钮（原型 ui-edit-icon：跳转编辑该派生定义；当前行禁用）
				QToolButton* editBtn = new QToolButton(cell);
				editBtn->setIcon(pencilIcon(QColor(row.isCurrent ? kDisabled : kMuted)));
				editBtn->setIconSize(QSize(14, 14));
				editBtn->setFixedSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());
				editBtn->setCursor(Qt::PointingHandCursor);
				editBtn->setStyleSheet(
					"QToolButton{border:0;background:transparent;border-radius:2px;}"
					"QToolButton:hover{background:#F0F7FF;}");
				editBtn->setEnabled(!row.isCurrent);
				const QString tip = row.isCurrent
					? (_langEn ? QStringLiteral("Currently editing") : QString::fromUtf8("当前正在编辑"))
					: (_langEn ? QStringLiteral("Edit %1").arg(row.name)
						: QString::fromUtf8("编辑%1").arg(row.name));
				editBtn->setToolTip(tip);
				const int derivedId = row.derivedId;
				connect(editBtn, &QToolButton::clicked, this, [this, derivedId]()
					{ editDefinition(derivedId); });
				cellLay->addWidget(editBtn, 0, Qt::AlignRight);
			}
			_table->setCellWidget(r, 3, cell);
			++r;
		}

		//空态行（原型 superposition-empty：跨 4 列、11px、muted、居中）
		if (visible.isEmpty())
		{
			QTableWidgetItem* emptyItem = new QTableWidgetItem(
				_rows.isEmpty()
				? (_langEn ? QStringLiteral("No cases") : QString::fromUtf8("暂无工况"))
				: (_langEn ? QStringLiteral("No matching cases") : QString::fromUtf8("无匹配的工况")));
			emptyItem->setFlags(Qt::ItemIsEnabled);
			emptyItem->setData(Qt::UserRole + 2, true);
			emptyItem->setForeground(QBrush(QColor(kMuted)));
			emptyItem->setFont(helperFont);
			emptyItem->setTextAlignment(Qt::AlignCenter);
			_table->setItem(r, 0, emptyItem);
			_table->setSpan(r, 0, 1, 4);
		}

		_rebuilding = false;
		refreshActions();
	}

	void mPostStepSequencePanel::refreshActions()
	{
		//口径与原型 updateNativeActions/selectableNative 一致
		const QString query = _nativeQuery.trimmed().toLower();
		int selectedCount = 0;
		int hiddenSelected = 0;
		int selectableCount = 0;
		bool allSelected = true;
		for (const SourceRow& row : _rows)
		{
			if (row.enabled && !row.isCurrent)
				++selectedCount;
			const bool matches = (row.name + QLatin1Char(' ') + row.frame).toLower().contains(query);
			if (row.enabled && !matches)
				++hiddenSelected;
			const bool selectable = matches && !row.isCurrent
				&& (row.enabled || (row.disabledReason.isEmpty() && !row.missing));
			if (selectable)
			{
				++selectableCount;
				if (!row.enabled)
					allSelected = false;
			}
		}

		const bool hasSelectable = selectableCount > 0;
		_btnSelectAll->setEnabled(hasSelectable);
		_btnSelectAll->setText(hasSelectable && allSelected
			? (_langEn ? QStringLiteral("Clear") : QString::fromUtf8("取消"))
			: (_langEn ? QStringLiteral("All") : QString::fromUtf8("全选")));
		QString countText = (_langEn ? QStringLiteral("Selected %1") : QString::fromUtf8("已选 %1 项")).arg(selectedCount);
		if (hiddenSelected > 0)
			countText += (_langEn ? QStringLiteral(" (%1 outside filter)")
				: QString::fromUtf8("（筛选外 %1 项）")).arg(hiddenSelected);
		_countLabel->setText(countText);
		_batchScaleEdit->setEnabled(selectedCount > 0);
		_btnApplyScale->setEnabled(selectedCount > 0);
	}

	void mPostStepSequencePanel::slot_searchChanged(const QString& text)
	{
		_nativeQuery = text;
		renderTable();
	}

	void mPostStepSequencePanel::slot_toggleAll()
	{
		const QString query = _nativeQuery.trimmed().toLower();
		//目标行集合与 refreshActions 的 selectable 口径一致；全部已选则反转取消
		QList<int> targets;
		bool all = true;
		for (int i = 0; i < _rows.size(); ++i)
		{
			const SourceRow& row = _rows.at(i);
			const bool matches = (row.name + QLatin1Char(' ') + row.frame).toLower().contains(query);
			const bool selectable = matches && !row.isCurrent
				&& (row.enabled || (row.disabledReason.isEmpty() && !row.missing));
			if (selectable)
			{
				targets.append(i);
				if (!row.enabled)
					all = false;
			}
		}
		if (targets.isEmpty())
			return;
		for (int i : targets)
			_rows[i].enabled = !all;
		//同步表格勾选显示（blockSignals 防 itemChanged 重入）
		_table->blockSignals(true);
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			QTableWidgetItem* item = _table->item(r, 0);
			if (!item)
				continue;
			const QString id = item->data(Qt::UserRole).toString();
			for (const SourceRow& row : _rows)
			{
				if (row.id == id)
				{
					item->setCheckState(row.enabled ? Qt::Checked : Qt::Unchecked);
					break;
				}
			}
		}
		_table->blockSignals(false);
		commit();
		//全选批量变化与单选同理：重算派生行禁用态后重绘（原型 lowerAll→renderNative）
		rebuildSources();
		renderTable();
	}

	void mPostStepSequencePanel::slot_applyBatchScale()
	{
		const QString value = _batchScaleEdit->text();
		if (!validNumber(value))
		{
			showMessage(_langEn ? QStringLiteral("The scale factor must be a finite number.")
				: QString::fromUtf8("缩放系数须为有限数值。"), true);
			return;
		}
		//启用行系数统一覆盖（原型 applyScale），行内输入框同步显示
		QList<QString> updatedIds;
		for (SourceRow& row : _rows)
		{
			if (row.enabled && !row.isCurrent)
			{
				row.scale = value;
				updatedIds.append(row.id);
			}
		}
		for (int r = 0; r < _table->rowCount(); ++r)
		{
			QTableWidgetItem* checkItem = _table->item(r, 0);
			QWidget* cell = checkItem ? _table->cellWidget(r, 3) : nullptr;
			if (!cell || !updatedIds.contains(checkItem->data(Qt::UserRole).toString()))
				continue;
			if (QLineEdit* scaleEdit = cell->findChild<QLineEdit*>())
			{
				scaleEdit->blockSignals(true);
				scaleEdit->setText(value);
				scaleEdit->setStyleSheet(validNumber(value) ? kScaleEditQss : kScaleEditInvalidQss);
				scaleEdit->blockSignals(false);
			}
		}
		showMessage(QString());
		commit();
		refreshActions();
	}

	void mPostStepSequencePanel::slot_itemChanged(QTableWidgetItem* item)
	{
		if (_rebuilding || !_table || !item || item->column() != 0)
			return;
		const QString id = item->data(Qt::UserRole).toString();
		if (id.isEmpty())
			return;
		for (SourceRow& row : _rows)
		{
			if (row.id == id)
			{
				row.enabled = item->checkState() == Qt::Checked;
				break;
			}
		}
		commit();
		//勾选变化重算全部派生行禁用态（原型 check→renderNative：模拟集变化会使其他
		//派生行在超两层/循环时转禁；rebuildSources 合并保留勾选与系数）
		rebuildSources();
		renderTable();
	}

	void mPostStepSequencePanel::slot_generateSteps()
	{
		//先落盘当前草稿（失败原因已在反馈行提示，生成中止）
		if (!commit())
			return;

		resolveContext();
		if (_postDB == nullptr || _editingId < 0)
		{
			showMessage(_langEn ? QStringLiteral("Open a post result before generating.")
				: QString::fromUtf8("请先打开后处理结果再生成。"), true);
			return;
		}
		DerivedCaseDefinition def;
		if (!mPostDerivedCaseStore::instance().definition(_storeKey, _editingId, &def)
			|| def.kind != DerivedCaseKind::Steps)
			return;
		if (!def.configured || def.rows.isEmpty())
		{
			showMessage(_langEn ? QStringLiteral("This derived case is unconfigured. Configure it first.")
				: QString::fromUtf8("该派生工况待配置，请先配置有效来源。"), true);
			return;
		}

		//变量身份取当前云图视口 guidata（SolidColor 为伪变量，引擎预检同款拦截）
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		mPostGUIData* guidata = (postRend != nullptr) ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
		if (guidata == nullptr || guidata->_cloudVarName.trimmed().isEmpty()
			|| guidata->_cloudVarName == QStringLiteral("SolidColor"))
		{
			showMessage(_langEn ? QStringLiteral("Select a result variable in the cloud map first.")
				: QString::fromUtf8("请先在云图中选择结果变量。"), true);
			return;
		}

		mPostDerivedCaseEngine::GenerateContext ctx;
		ctx.postDB = _postDB;
		ctx.postPathName = _postDB->getFile();
		ctx.postName = postRend->getPostDataName();
		ctx.var = guidata->_cloudVarName;
		ctx.compo = guidata->_cloudCompo;
		ctx.layer = guidata->_cloudLayerName;
		ctx.complexMode = guidata->_cloudComplex;
		ctx.progressParent = this;

		QString errMsg;
		if (!mPostDerivedCaseEngine::generateSteps(def,
			mPostDerivedCaseStore::instance().definitions(_storeKey), ctx, &errMsg))
		{
			showMessage(errMsg, true);
			return;
		}

		//生成成功：当前云图视口工况下拉切到派生工况（blockSignals 重灌+手动级联，
		//帧表/变量/绘制/图例/实时动画浮层走标准链；失败/取消路径上面已反馈）
		mPostInfoHub::getInstance()->cloudResultPanel()->selectCaseByName(
			postRend->getViewportName(), def.name);
		showMessage(QString());
	}

	void mPostStepSequencePanel::showMessage(const QString& text, bool error)
	{
		if (text.isEmpty())
		{
			_feedback->clear();
			//行级显隐（QFormLayout 对 hide 的 widget 仍保留行占位——踩坑 27 口径）
			_nativeFold->setRowVisible(_feedback, false);
			_feedback->hide();
			return;
		}
		_feedback->setText(text);
		_feedback->setStyleSheet(
			QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;padding:0 2px;}")
			.arg(error ? kError : kMuted).arg(UiMetrics::helperFontSize()));
		_feedback->show();
		_nativeFold->setRowVisible(_feedback, true);
	}

	void mPostStepSequencePanel::hideEvent(QHideEvent* event)
	{
		//本面板无拾取/武装瞬时态；仅收错误反馈避免重开残留（定义与草稿为进程态保留）
		showMessage(QString());
		mPanelBase::hideEvent(event);
	}

	void mPostStepSequencePanel::slot_changeLanguage(int language)
	{
		_langEn = (language != 0);
		applyLanguageTexts();
	}

	void mPostStepSequencePanel::applyLanguageTexts()
	{
		_definitionFold->setTitle(_langEn ? QStringLiteral("Step Sequence Definition")
			: QString::fromUtf8("步序列定义"));
		_nativeFold->setTitle(_langEn ? QStringLiteral("Load Cases") : QString::fromUtf8("工况"));
		_nameLabel->setText(_langEn ? QStringLiteral("Derived Case") : QString::fromUtf8("派生工况"));
		_searchEdit->setPlaceholderText(_langEn ? QStringLiteral("Search cases or frames")
			: QString::fromUtf8("搜索工况或帧"));
		_table->setHorizontalHeaderLabels(QStringList
			{
				QString(),
				_langEn ? QStringLiteral("Native Case") : QString::fromUtf8("原生工况"),
				_langEn ? QStringLiteral("Frame") : QString::fromUtf8("帧"),
				_langEn ? QStringLiteral("Scale") : QString::fromUtf8("缩放系数")
			});
		_scaleLabel->setText(_langEn ? QStringLiteral("Scale") : QString::fromUtf8("缩放系数"));
		_btnApplyScale->setText(_langEn ? QStringLiteral("Set") : QString::fromUtf8("设置"));
		_buttonGenerate->setText(_langEn ? QStringLiteral("Generate") : QString::fromUtf8("生成"));
		//来源行文案（禁用原因/第N步）随语言重建后再重绘（组头/空态/状态标签）；
		//rebuildSources 以当前草稿合并，勾选与系数不丢
		if (_editingId > 0)
			rebuildSources();
		renderTable();
	}
}
