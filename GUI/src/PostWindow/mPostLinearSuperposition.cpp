#include "mPostLinearSuperposition.h"

#include <QStyledItemDelegate>
#include <QCheckBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyle>
#include <QRegularExpression>
#include <QCollator>
#include <QMap>
#include <QBrush>
#include <QFont>
#include <functional>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//MxFunctions
#include "mGlobalSignals.h"
#include <UiPalette.h>
#include <UiMetrics.h>

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mPostDataManager.h"
#include "mPostGUIData.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
// 须在 mIMxdbFile1.h 之后：mIONativeFile.h 的裸 include "mIOEnum.h" 靠 include 栈
// （mIMxdbFile1.h 所在的 PostIOFile 目录）命中 MIOFile 版枚举，先于本行会被 Common 版遮蔽
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "mAppConfiguration.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mModelData.h"
#include "mOtherFunction.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）
#include "DerivedCase/mPostDerivedCaseStore.h"
#include "DerivedCase/mPostDerivedCaseEngine.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace
{
	//按定义 id 在定义集内查找（store 定义 id 全局唯一；找不到返回 nullptr）
	const PostWindow::DerivedCaseDefinition* defById(const QList<PostWindow::DerivedCaseDefinition>& defs, int id)
	{
		for (const PostWindow::DerivedCaseDefinition& def : defs)
			if (def.id == id)
				return &def;
		return nullptr;
	}

	//缩放系数有限数值判定（原型 validNumber：非空且 Number.isFinite）
	bool finiteScaleValue(const QString& text, double& value)
	{
		bool ok = false;
		value = text.trimmed().toDouble(&ok);
		return ok && qIsFinite(value);
	}

	//自然排序（工况/帧名，与同族步序列/包络面板及 mPost3DResult sortQStringList 同语义）
	QStringList naturalSorted(QStringList list)
	{
		QCollator collator;
		collator.setNumericMode(true);
		collator.setCaseSensitivity(Qt::CaseInsensitive);
		std::sort(list.begin(), list.end(), [&collator](const QString& a, const QString& b)
			{ return collator.compare(a, b) < 0; });
		return list;
	}

	//2x 采样绘制小图标（近似原型 inline SVG 语义，同 mPostCurveExtract makeIcon）
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

	//派生行编辑按钮铅笔图标（原型 superposition-edit svg path）
	QIcon pencilIcon()
	{
		return makeIcon(20, QColor(UiPalette::labelHex()), [](QPainter& p)
			{
				p.drawLine(QPointF(14, 5), QPointF(19, 10));
				QPainterPath path;
				path.moveTo(4, 20);
				path.lineTo(8, 19);
				path.lineTo(20, 7);
				path.lineTo(16, 3);
				path.lineTo(4, 15);
				path.closeSubpath();
				p.drawPath(path);
			});
	}

	//管理表行分隔线委托（行底线 + 复选列右线，同 mPostCurveExtract GridLineDelegate）
	class GridLineDelegate : public QStyledItemDelegate
	{
	public:
		using QStyledItemDelegate::QStyledItemDelegate;

		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
		{
			QStyledItemDelegate::paint(painter, option, index);
			painter->save();
			QPen pen(QColor(UiPalette::borderHex()), 1.0);
			painter->setPen(pen);
			const QRect r = option.rect;
			painter->drawLine(QPointF(r.left() + 0.5, r.bottom() - 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			if (index.column() == 0)
				painter->drawLine(QPointF(r.right() - 0.5, r.top() + 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			painter->restore();
		}
	};

	//复选列居中容器（cellWidget：QCheckBox 在 26px 复选列水平垂直居中，原型 margin:auto）
	QCheckBox* centeredCheckBox(QWidget* host)
	{
		QWidget* cell = new QWidget(host);
		QVBoxLayout* lay = new QVBoxLayout(cell);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
		QCheckBox* box = new QCheckBox(cell);
		lay->addWidget(box, 0, Qt::AlignCenter);
		return box;
	}

	//行内缩放系数输入（原型 .superposition-scale：透明底无边框，非法值红框 aria-invalid）
	QLineEdit* createScaleEdit(QWidget* host)
	{
		QLineEdit* edit = new QLineEdit(host);
		edit->setFrame(false);
		edit->setStyleSheet(QString(
			"QLineEdit{background:transparent;border:0;padding:0 %1px;font-size:%2px;color:%3;}"
			"QLineEdit[invalid=\"true\"]{border:1px solid %4;}")
			.arg(UiMetrics::choiceGap())
			.arg(UiMetrics::bodyFontSize())
			.arg(UiPalette::textHex())
			.arg(UiPalette::errorHex()));
		return edit;
	}

	void markScaleValidity(QLineEdit* edit, bool ok)
	{
		if (edit->property("invalid").toBool() == !ok) return;
		edit->setProperty("invalid", ok ? QVariant() : QVariant(true));
		edit->style()->unpolish(edit);
		edit->style()->polish(edit);
	}

	//窄按钮样式（全选/设置共用，令牌同 mPostSetPanel narrowButtonStyle）
	QString narrowButtonStyle()
	{
		return QString(
			"QPushButton{ font-family:'Microsoft YaHei'; color:%1; background:%2; border:1px solid %3; border-radius:%4px; padding:%5px %6px; }"
			"QPushButton:hover{ background:%7; }"
			"QPushButton:pressed{ background:%8; color:%9; }"
			"QPushButton:disabled{ color:%10; background:%11; }")
			.arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex())
			.arg(UiMetrics::zoneRadius())
			.arg(UiMetrics::stackGap()).arg(UiMetrics::pushbuttonInner())
			.arg(UiPalette::hoverHex())
			.arg(UiPalette::selectedHex(), UiPalette::selectedTextHex())
			.arg(UiPalette::disabledHex(), UiPalette::panelHex());
	}

	//主操作蓝按钮（生成；--std-action/-hover）
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
}

namespace PostWindow
{
	mPostLinearSuperpositionPanel::mPostLinearSuperpositionPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;
		_langEn = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1);
		setObjectName("linearSuperpositionParameters");

		setupUI();
		applyLanguageTexts();

		//语言切换全量刷新
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));
	}

	void mPostLinearSuperpositionPanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		_mainLayout->setSpacing(UiMetrics::rowGap());
		setStyleSheet(QString("mPostLinearSuperpositionPanel{background-color:%1;}")
			.arg(UiPalette::surfaceHex()));

		/********************折叠组1：线性叠加定义********************/
		_definitionFold = new FoldSection(QString::fromUtf8("线性叠加定义"), this);

		_labelName = new PanelLabel(QString::fromUtf8("派生工况"));
		_editName = new QLineEdit();
		_editName->setFixedHeight(UiMetrics::baseH());
		_editName->setText(QString::fromUtf8("线性叠加-1"));   //初始文本（语言切换只刷新文案）
		connect(_editName, &QLineEdit::textEdited, this, [this](const QString&)
			{
				if (commitDefinition())
					updateCurrentRowName();
			});
		_definitionFold->addRow(_labelName, _editName);

		_mainLayout->addWidget(_definitionFold);

		/********************折叠组2：工况********************/
		_nativeFold = new FoldSection(QString::fromUtf8("工况"), this);

		//搜索框（按"工况名+帧文本"过滤表格行）
		_editSearch = new QLineEdit();
		_editSearch->setFixedHeight(UiMetrics::baseH());
		_editSearch->setPlaceholderText(QString::fromUtf8("搜索工况或帧"));
		connect(_editSearch, &QLineEdit::textChanged, this, [this](const QString&)
			{
				if (!_rendering) renderTable();
			});
		_nativeFold->addRow(_editSearch);

		//来源工况管理表：复选(26)|原生工况(stretch)|帧(90)|缩放系数(72)
		//列宽取值：复选=M2 复选列 26；帧=原型 col3 公式 5*body+2*groupInset+entryArrow+choiceGap；
		//缩放=原型 --superposition-scale-column 3*body+managerInner+3*choiceGap（managerInner=24）
		_tableLinear = new QTableWidget(this);
		_tableLinear->setObjectName("tableLinearSources");
		_tableLinear->setFocusPolicy(Qt::ClickFocus);
		_tableLinear->setSelectionMode(QAbstractItemView::NoSelection);
		_tableLinear->setEditTriggers(QAbstractItemView::NoEditTriggers);
		_tableLinear->setWordWrap(false);
		_tableLinear->setShowGrid(false);
		_tableLinear->verticalHeader()->setVisible(false);
		_tableLinear->verticalHeader()->setDefaultSectionSize(UiMetrics::controlHeight());
		_tableLinear->setColumnCount(4);
		const int frameCol = 5 * UiMetrics::bodyFontSize() + 2 * UiMetrics::menuItemInset() + 12 + UiMetrics::choiceGap();
		const int scaleCol = 3 * UiMetrics::bodyFontSize() + 24 + 3 * UiMetrics::choiceGap();
		QHeaderView* header = _tableLinear->horizontalHeader();
		header->setFixedHeight(UiMetrics::baseH());
		header->setHighlightSections(false);
		header->setMinimumSectionSize(26);
		header->setSectionResizeMode(0, QHeaderView::Fixed);
		header->setSectionResizeMode(1, QHeaderView::Stretch);
		header->setSectionResizeMode(2, QHeaderView::Fixed);
		header->setSectionResizeMode(3, QHeaderView::Fixed);
		header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_tableLinear->setColumnWidth(0, 26);
		_tableLinear->setColumnWidth(2, frameCol);
		_tableLinear->setColumnWidth(3, scaleCol);
		_tableLinear->setItemDelegate(new GridLineDelegate(_tableLinear));
		_tableLinear->setStyleSheet(QString(
			"QTableWidget{background:%1;border:1px solid %2;font-size:%3px;color:%4;}"
			"QTableWidget::item{padding:0 %5px;}"
			"QHeaderView::section{background:%6;color:%4;border:0;"
			"border-right:1px solid %2;border-bottom:1px solid %2;padding:0 %5px;}"
			"QCheckBox{background:transparent;}")
			.arg(UiPalette::surfaceHex(), UiPalette::borderHex())
			.arg(UiMetrics::bodyFontSize())
			.arg(UiPalette::textHex())
			.arg(UiMetrics::menuItemInset())
			.arg(UiPalette::headerHex()));
		_nativeFold->addRow(_tableLinear);

		//操作条：全选/取消 + 已选计数 + 批量缩放（标签+连体[输入|设置]）
		QWidget* actionsRow = new QWidget(this);
		QHBoxLayout* actionsLayout = new QHBoxLayout(actionsRow);
		actionsLayout->setContentsMargins(0, 0, 0, 0);
		actionsLayout->setSpacing(UiMetrics::rowGap());

		_btnSelectAll = new QPushButton(QString::fromUtf8("全选"), actionsRow);
		_btnSelectAll->setStyleSheet(narrowButtonStyle());
		_btnSelectAll->setFixedSize(UiMetrics::baseWmin(), UiMetrics::baseH());   //两字钮 40×26，钉死防全局 QSS 撑宽（踩坑 38）
		_btnSelectAll->setEnabled(false);
			connect(_btnSelectAll, &QPushButton::clicked, this, [this]()
				{
					//可见可选行全选/取消（原型 lowerAll：全部已选→取消，否则全选；
					//当前定义行与未配置派生行不可选，跳过）
					const QString query = _editSearch->text().trimmed();
					auto visible = [&query](const QString& hay)
						{ return query.isEmpty() || hay.contains(query, Qt::CaseInsensitive); };
					const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
					auto derivedSelectable = [&defs, this](const DerivedRow& dr)
						{
							const PostWindow::DerivedCaseDefinition* def = defById(defs, dr.defId);
							return dr.defId != _editingId && def != nullptr && def->configured;
						};
					auto derivedName = [&defs](const DerivedRow& dr)
						{
							const PostWindow::DerivedCaseDefinition* def = defById(defs, dr.defId);
							return def != nullptr ? def->name : QString();
						};
					bool all = true, any = false;
					for (const NativeRow& r : _rows)
						if (visible(r.caseName + ' ' + r.frameText)) { any = true; if (!r.enabled) all = false; }
					for (const DerivedRow& dr : _derivedRows)
					{
						if (derivedSelectable(dr) && visible(derivedName(dr))) { any = true; if (!dr.enabled) all = false; }
					}
					if (!any) return;
					for (NativeRow& r : _rows)
						if (visible(r.caseName + ' ' + r.frameText)) r.enabled = !all;
					for (DerivedRow& dr : _derivedRows)
					{
						if (derivedSelectable(dr) && visible(derivedName(dr))) dr.enabled = !all;
					}
					commitDefinition();
					renderTable();
				});

		_labelCount = new QLabel(QString::fromUtf8("已选 0 项"), actionsRow);
		_labelCount->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(UiPalette::mutedHex()).arg(UiMetrics::helperFontSize()));

		//批量缩放：标签 + 连体控件（原型 .superposition-scale-setting：grid 2 列 80×26，按钮左分隔线）
		QWidget* batchWrap = new QWidget(actionsRow);
		QHBoxLayout* batchLayout = new QHBoxLayout(batchWrap);
		batchLayout->setContentsMargins(0, 0, 0, 0);
		batchLayout->setSpacing(UiMetrics::choiceGap());
		_labelBatch = new QLabel(QString::fromUtf8("缩放系数"), batchWrap);
		_labelBatch->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(UiPalette::textHex()).arg(UiMetrics::bodyFontSize()));

		QWidget* settingBox = new QWidget(batchWrap);
		settingBox->setObjectName("linearBatchScaleSetting");
		settingBox->setFixedSize(2 * UiMetrics::baseWmin(), UiMetrics::baseH());   //连体框 80×26（两段均分 40）
		//连体容器描边圆角在外层（原型 superposition-scale-setting），内部两段直角无内缝
		settingBox->setStyleSheet(QString(
			"QWidget#linearBatchScaleSetting{background:%1;border:1px solid %2;border-radius:%3px;}")
			.arg(UiPalette::surfaceHex(), UiPalette::borderHex()).arg(UiMetrics::zoneRadius()));
		QHBoxLayout* settingLayout = new QHBoxLayout(settingBox);
		settingLayout->setContentsMargins(0, 0, 0, 0);
		settingLayout->setSpacing(0);
		_editBatchScale = new QLineEdit(settingBox);
		_editBatchScale->setFixedSize(UiMetrics::baseWmin(), UiMetrics::quickButtonSize());   //40×24，钉死防长文本 minHint 撑破连体框（踩坑 38）
		_editBatchScale->setAlignment(Qt::AlignCenter);
		_editBatchScale->setText(QStringLiteral("1"));
		_editBatchScale->setStyleSheet(QString(
			"QLineEdit{background:transparent;border:0;padding:0 %1px;font-size:%2px;color:%3;}"
			"QLineEdit[invalid=\"true\"]{border:1px solid %4;}")
			.arg(UiMetrics::choiceGap())
			.arg(UiMetrics::bodyFontSize())
			.arg(UiPalette::textHex())
			.arg(UiPalette::errorHex()));
		_editBatchScale->setEnabled(false);
		_btnSetScale = new QPushButton(QString::fromUtf8("设置"), settingBox);
		_btnSetScale->setFixedSize(UiMetrics::baseWmin(), UiMetrics::quickButtonSize());   //40×24，钉死防全局 QSS width:80 撑破连体框
		_btnSetScale->setEnabled(false);
		_btnSetScale->setStyleSheet(QString(
			"QPushButton{background:transparent;border:0;border-left:1px solid %1;border-radius:0;"
			"padding:0 %2px;font-family:'Microsoft YaHei';font-size:%3px;color:%4;}"
			"QPushButton:hover{background:%5;}"
			"QPushButton:disabled{color:%6;background:transparent;}")
			.arg(UiPalette::borderHex())
			.arg(UiMetrics::pushbuttonInner())
			.arg(UiMetrics::bodyFontSize())
			.arg(UiPalette::textHex())
			.arg(UiPalette::hoverHex())
			.arg(UiPalette::disabledHex()));
		settingLayout->addWidget(_editBatchScale, 1);
		settingLayout->addWidget(_btnSetScale, 1);
		connect(_btnSetScale, &QPushButton::clicked, this, [this]()
			{
				//批量缩放：应用于全部已选行；非法值红框+反馈（原型 applyScale）
				double value = 0.0;
				if (!finiteScaleValue(_editBatchScale->text(), value))
				{
					markScaleValidity(_editBatchScale, false);
					setFeedback(_langEn ? QString("Scale must be a finite number.") : QString::fromUtf8("缩放系数须为有限数值。"), true);
					return;
				}
				markScaleValidity(_editBatchScale, true);
				const QString text = _editBatchScale->text().trimmed();
				for (NativeRow& r : _rows)
					if (r.enabled) r.scale = text;
				for (DerivedRow& dr : _derivedRows)
					if (dr.enabled && dr.defId != _editingId) dr.scale = text;
				commitDefinition();
				renderTable();
			});

		batchLayout->addWidget(_labelBatch);
		batchLayout->addWidget(settingBox);

		actionsLayout->addWidget(_btnSelectAll);
		actionsLayout->addWidget(_labelCount, 1);
		actionsLayout->addWidget(batchWrap);
		_nativeFold->addRow(actionsRow);

		//反馈行（错误红字/状态灰字，空文案整行收起）
		_labelFeedback = new QLabel(this);
		_labelFeedback->setObjectName("linearSuperpositionFeedback");
		_labelFeedback->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(UiPalette::mutedHex()).arg(UiMetrics::helperFontSize()));
		_labelFeedback->setVisible(false);
		_nativeFold->addRow(_labelFeedback);

		//生成（软件实际能力：叠加勾选来源的云图；原型只保存定义不计算）
		_buttonGenerate = new QPushButton(QString::fromUtf8("生成"), this);
		_buttonGenerate->setFixedHeight(UiMetrics::baseH());
		_buttonGenerate->setStyleSheet(primaryButtonStyle());
		connect(_buttonGenerate, SIGNAL(clicked()), this, SLOT(slot_generateLinear()));
		_nativeFold->addRow(_buttonGenerate);

		_mainLayout->addWidget(_nativeFold);
		_mainLayout->addStretch();
	}

	void mPostLinearSuperpositionPanel::applyLanguageTexts()
	{
		_definitionFold->setTitle(_langEn ? QString("Superposition Definition") : QString::fromUtf8("线性叠加定义"));
		_nativeFold->setTitle(_langEn ? QString("Load Cases") : QString::fromUtf8("工况"));
		_labelName->setText(_langEn ? QString("Derived Case") : QString::fromUtf8("派生工况"));
		_editSearch->setPlaceholderText(_langEn ? QString("Search cases or frames") : QString::fromUtf8("搜索工况或帧"));
		_btnSelectAll->setText(_langEn ? QString("All") : QString::fromUtf8("全选"));
		_labelBatch->setText(_langEn ? QString("Scale") : QString::fromUtf8("缩放系数"));
		_btnSetScale->setText(_langEn ? QString("Set") : QString::fromUtf8("设置"));
		_buttonGenerate->setText(_langEn ? QString("Generate") : QString::fromUtf8("生成"));
		QStringList headers;
		headers << QString()
			<< (_langEn ? QString("Native Cases") : QString::fromUtf8("原生工况"))
			<< (_langEn ? QString("Frame") : QString::fromUtf8("帧"))
			<< (_langEn ? QString("Scale") : QString::fromUtf8("缩放系数"));
		_tableLinear->setHorizontalHeaderLabels(headers);
		setFeedback(QString());
		renderTable();
	}

	void mPostLinearSuperpositionPanel::slot_changeLanguage(int language)
	{
		_langEn = (language == 1);
		applyLanguageTexts();
	}

	void mPostLinearSuperpositionPanel::renderTable()
	{
		if (_tableLinear == nullptr) return;
		_rendering = true;

		const QString query = _editSearch->text().trimmed();
		auto visible = [&query](const QString& hay)
			{ return query.isEmpty() || hay.contains(query, Qt::CaseInsensitive); };

		QList<int> visibleNative;
		for (int i = 0; i < _rows.size(); ++i)
			if (visible(_rows.at(i).caseName + ' ' + _rows.at(i).frameText))
				visibleNative.append(i);
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		QList<int> visibleDerived;
		for (int i = 0; i < _derivedRows.size(); ++i)
		{
			const PostWindow::DerivedCaseDefinition* def = defById(defs, _derivedRows.at(i).defId);
			if (visible(def != nullptr ? def->name : QString()))
				visibleDerived.append(i);
		}

		_tableLinear->setUpdatesEnabled(false);
		_tableLinear->setRowCount(0);      //行移除时旧跨列自动清除
		_tableLinear->clearContents();

		const QString linearLabel = _langEn ? QString("Superposition") : QString::fromUtf8("线性叠加");

		auto appendEmptyRow = [this](const QString& text)
			{
				QTableWidgetItem* empty = new QTableWidgetItem;
				empty->setFlags(Qt::ItemIsEnabled);
				empty->setTextAlignment(Qt::AlignCenter);
				empty->setForeground(QBrush(QColor(UiPalette::mutedHex())));
				QFont f = empty->font();
				f.setPixelSize(UiMetrics::helperFontSize());
				empty->setFont(f);
				empty->setText(text);
				_tableLinear->setItem(0, 0, empty);
				_tableLinear->setSpan(0, 0, 1, 4);
			};

		auto appendNativeRow = [this](int visual, int idx)
			{
				const NativeRow& r = _rows.at(idx);
				QCheckBox* box = centeredCheckBox(_tableLinear);
				box->setChecked(r.enabled);
				box->setToolTip(_langEn
					? QString("Include %1 %2 in superposition").arg(r.caseName, r.frameText)
					: QString::fromUtf8("参与线性叠加 %1 %2").arg(r.caseName, r.frameText));
				connect(box, &QCheckBox::toggled, this, [this, idx](bool on)
					{
						if (_rendering || idx >= _rows.size()) return;
						_rows[idx].enabled = on;
						commitDefinition();
						updateActionsState();
					});
				_tableLinear->setCellWidget(visual, 0, box->parentWidget());

				QTableWidgetItem* caseItem = new QTableWidgetItem(r.caseName);
				caseItem->setFlags(Qt::ItemIsEnabled);
				caseItem->setToolTip(r.caseName);
				_tableLinear->setItem(visual, 1, caseItem);

				QTableWidgetItem* frameItem = new QTableWidgetItem(r.frameText);
				frameItem->setFlags(Qt::ItemIsEnabled);
				frameItem->setToolTip(r.frameText);
				frameItem->setData(Qt::UserRole, r.frameId);
				_tableLinear->setItem(visual, 2, frameItem);

				QLineEdit* edit = createScaleEdit(_tableLinear);
				edit->setText(r.scale);
				double dummy = 0.0;
				markScaleValidity(edit, finiteScaleValue(r.scale, dummy));
				edit->setToolTip(r.caseName + ' ' + r.frameText
					+ (_langEn ? QString(" scale") : QString::fromUtf8(" 缩放系数")));
				connect(edit, &QLineEdit::textEdited, this, [this, idx, edit](const QString& text)
					{
						if (_rendering || idx >= _rows.size()) return;
						_rows[idx].scale = text;
						double v = 0.0;
						markScaleValidity(edit, finiteScaleValue(text, v));
						commitDefinition();
						updateActionsState();
					});
				_tableLinear->setCellWidget(visual, 3, edit);
			};

		auto appendGroupHeader = [this, &linearLabel](int visual)
			{
				const QStringList captions
					= { QString(), linearLabel, (_langEn ? QString("Frame") : QString::fromUtf8("帧")),
						(_langEn ? QString("Scale") : QString::fromUtf8("缩放系数")) };
				for (int c = 0; c < 4; ++c)
				{
					QTableWidgetItem* it = new QTableWidgetItem(captions.at(c));
					it->setFlags(Qt::ItemIsEnabled);
					it->setBackground(QBrush(QColor(UiPalette::headerHex())));
					it->setForeground(QBrush(QColor(UiPalette::labelHex())));
					_tableLinear->setItem(visual, c, it);
				}
			};

		auto appendDerivedRow = [this, &defs, &linearLabel](int visual, int idx)
			{
				const DerivedRow& dr = _derivedRows.at(idx);
				const PostWindow::DerivedCaseDefinition* def = defById(defs, dr.defId);
				const bool isCurrent = (dr.defId == _editingId);
				const bool hasDef = (def != nullptr);
				const QString defName = hasDef ? def->name : QString();
				//未配置定义不可作为来源（原型 disabledReason："该派生工况待配置，请先配置有效来源。"）
				const bool unconfigured = !hasDef || !def->configured;

				QCheckBox* box = centeredCheckBox(_tableLinear);
				if (isCurrent)
				{
					box->setEnabled(false);
					box->setToolTip(_langEn
						? QString("The current case cannot reference itself.")
						: QString::fromUtf8("当前工况不能作为自身来源。"));
				}
				else if (unconfigured)
				{
					box->setEnabled(false);
					box->setToolTip(_langEn
						? QString("This derived case is not configured yet; configure it first.")
						: QString::fromUtf8("该派生工况待配置，请先配置有效来源。"));
				}
				else
				{
					//行级禁用预判（批次C）：环/超两层在勾选前即可见（步序列/包络同款）；
					//已勾选行不强制禁用（原因挂 tooltip，取消勾选后转禁用），提交侧
					//store 校验仍兜底
					const QString denied = derivedRowDeniedReason(dr.defId);
					if (!denied.isEmpty())
					{
						box->setToolTip(denied);
						if (!dr.enabled)
							box->setEnabled(false);
					}
					if (box->isEnabled())
					{
						connect(box, &QCheckBox::toggled, this, [this, idx](bool on)
						{
							if (_rendering || idx >= _derivedRows.size()) return;
							_derivedRows[idx].enabled = on;
							commitDefinition();
							updateActionsState();
						});
					}
				}
				_tableLinear->setCellWidget(visual, 0, box->parentWidget());

				QTableWidgetItem* nameItem = new QTableWidgetItem(defName);
				nameItem->setFlags(Qt::ItemIsEnabled);
				nameItem->setToolTip(defName);
				nameItem->setData(Qt::UserRole, dr.defId);
				_tableLinear->setItem(visual, 1, nameItem);

				QTableWidgetItem* frameItem = new QTableWidgetItem(
					isCurrent ? QString::fromUtf8("—")
							  : QStringLiteral("0：%1").arg(linearLabel));
				frameItem->setFlags(Qt::ItemIsEnabled);
				_tableLinear->setItem(visual, 2, frameItem);

				QWidget* cell = new QWidget(_tableLinear);
				QHBoxLayout* cellLayout = new QHBoxLayout(cell);
				cellLayout->setContentsMargins(UiMetrics::choiceGap(), 0, 0, 0);
				cellLayout->setSpacing(UiMetrics::choiceGap());
				auto statusLabel = [cell](const QString& text)
					{
						QLabel* label = new QLabel(text, cell);
						label->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
							.arg(UiPalette::mutedHex()).arg(UiMetrics::bodyFontSize()));
						return label;
					};
				auto editButton = [this, cell, &defName]()
					{
						QPushButton* editBtn = new QPushButton(cell);
						editBtn->setFixedSize(24, 24);
						editBtn->setIcon(pencilIcon());
						editBtn->setStyleSheet("QPushButton{background:transparent;border:0;}");
						editBtn->setToolTip((_langEn ? QString("Edit ") : QString::fromUtf8("编辑")) + defName);
						return editBtn;
					};
				if (isCurrent)
				{
					QPushButton* editBtn = editButton();
					editBtn->setEnabled(false);
					editBtn->setCursor(Qt::PointingHandCursor);
					editBtn->setToolTip(_langEn ? QString("Currently editing") : QString::fromUtf8("当前正在编辑"));
					cellLayout->addWidget(statusLabel(_langEn ? QString("Current") : QString::fromUtf8("当前")));
					cellLayout->addStretch();
					cellLayout->addWidget(editBtn);
				}
				else if (unconfigured)
				{
					QPushButton* editBtn = editButton();
					editBtn->setCursor(Qt::PointingHandCursor);
					cellLayout->addWidget(statusLabel(_langEn ? QString("Not configured") : QString::fromUtf8("待配置")));
					cellLayout->addStretch();
					cellLayout->addWidget(editBtn);
				}
				else
				{
					QLineEdit* edit = createScaleEdit(cell);
					edit->setText(dr.scale);
					double dummy = 0.0;
					markScaleValidity(edit, finiteScaleValue(dr.scale, dummy));
					connect(edit, &QLineEdit::textEdited, this, [this, idx, edit](const QString& text)
						{
							if (_rendering || idx >= _derivedRows.size()) return;
							_derivedRows[idx].scale = text;
							double v = 0.0;
							markScaleValidity(edit, finiteScaleValue(text, v));
							commitDefinition();
							updateActionsState();
						});
					QPushButton* editBtn = editButton();
					editBtn->setCursor(Qt::PointingHandCursor);
					//切换编辑目标定义（原型 editDefinition：先保存当前草稿再载入目标）
					connect(editBtn, &QPushButton::clicked, this, [this, idx]()
					{
						if (idx >= _derivedRows.size()) return;
						commitDefinition();
						const int target = _derivedRows.at(idx).defId;
						const QList<PostWindow::DerivedCaseDefinition> defs2 = currentDefs();
						const PostWindow::DerivedCaseDefinition* targetDef = defById(defs2, target);
						if (targetDef == nullptr) return;
						_editingId = target;
						_editName->setText(targetDef->name);
						for (DerivedRow& d : _derivedRows)
							if (d.defId == target) { d.enabled = false; d.scale = QStringLiteral("1"); }
						applyEditingDefToRows();
						renderTable();
					});
					cellLayout->addWidget(edit, 1);
					cellLayout->addWidget(editBtn);
				}
				_tableLinear->setCellWidget(visual, 3, cell);
			};

		const bool anyRows = !_rows.isEmpty() || !_derivedRows.isEmpty();
		if (!anyRows || (visibleNative.isEmpty() && visibleDerived.isEmpty()))
		{
			_tableLinear->setRowCount(1);
			appendEmptyRow(anyRows
				? (_langEn ? QString("No matching cases") : QString::fromUtf8("无匹配的工况"))
				: (_langEn ? QString("No cases yet") : QString::fromUtf8("暂无工况")));
		}
		else
		{
			int visual = 0;
			const int nativeCount = visibleNative.size() + visibleDerived.size() + (visibleDerived.isEmpty() ? 0 : 1);
			_tableLinear->setRowCount(nativeCount);
			for (int idx : visibleNative)
				appendNativeRow(visual++, idx);
			if (!visibleDerived.isEmpty())
			{
				appendGroupHeader(visual++);
				for (int idx : visibleDerived)
					appendDerivedRow(visual++, idx);
			}
		}

		//表格高度：min 表头+6 数据行(=M2 184) / max 表头+20 数据行，超出滚动（原型 table-wrap 7~21 行）
		const int rowH = UiMetrics::controlHeight();
		const int headH = UiMetrics::baseH();
		_tableLinear->setFixedHeight(qBound(headH + 6 * rowH + 2, headH + _tableLinear->rowCount() * rowH + 2, headH + 20 * rowH + 2));

		_tableLinear->setUpdatesEnabled(true);
		_rendering = false;
		updateActionsState();
	}

	void mPostLinearSuperpositionPanel::updateActionsState()
	{
		if (_btnSelectAll == nullptr) return;
		const QString query = _editSearch->text().trimmed();
		auto visible = [&query](const QString& hay)
			{ return query.isEmpty() || hay.contains(query, Qt::CaseInsensitive); };

		int selectable = 0, selected = 0, hiddenSelected = 0;
		bool all = true;
		for (const NativeRow& r : _rows)
		{
			const bool vis = visible(r.caseName + ' ' + r.frameText);
			if (r.enabled)
			{
				++selected;
				if (!vis) ++hiddenSelected;
			}
			if (vis) { ++selectable; if (!r.enabled) all = false; }
		}
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		for (const DerivedRow& dr : _derivedRows)
		{
			//当前定义行与未配置派生行不可选，不计入（复选恒禁用）
			const PostWindow::DerivedCaseDefinition* def = defById(defs, dr.defId);
			if (dr.defId == _editingId || def == nullptr || !def->configured)
				continue;
			const QString name = def->name;
			const bool vis = visible(name);
			if (dr.enabled)
			{
				++selected;
				if (!vis) ++hiddenSelected;
			}
			if (vis) { ++selectable; if (!dr.enabled) all = false; }
		}

		_btnSelectAll->setEnabled(selectable > 0);
		_btnSelectAll->setText((selectable > 0 && all)
			? (_langEn ? QString("None") : QString::fromUtf8("取消"))
			: (_langEn ? QString("All") : QString::fromUtf8("全选")));
		_labelCount->setText((_langEn ? QString("Selected %1") : QString::fromUtf8("已选 %1 项")).arg(selected)
			+ (hiddenSelected > 0
				? (_langEn ? QString(" (%1 hidden by filter)").arg(hiddenSelected)
						   : QString::fromUtf8("（筛选外 %1 项）").arg(hiddenSelected))
				: QString()));
		_editBatchScale->setEnabled(selected > 0);
		_btnSetScale->setEnabled(selected > 0);
	}

	void mPostLinearSuperpositionPanel::setFeedback(const QString& text, bool error)
	{
		if (_labelFeedback == nullptr) return;
		_labelFeedback->setText(text);
		_labelFeedback->setStyleSheet(QString("QLabel{color:%1;font-size:%2px;border:0;background:transparent;}")
			.arg(error ? UiPalette::errorHex() : UiPalette::mutedHex())
			.arg(UiMetrics::helperFontSize()));
		//整行摘除/插回（QFormLayout 行隐藏仍摊行距，见 FoldSection::setRowVisible 注释）
		_nativeFold->setRowVisible(_labelFeedback, !text.isEmpty());
	}

	bool mPostLinearSuperpositionPanel::commitDefinition()
	{
		mPostDerivedCaseStore& store = mPostDerivedCaseStore::instance();
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		const PostWindow::DerivedCaseDefinition* editing = defById(defs, _editingId);
		if (editing == nullptr) return false;

		//名称：空/重名拒绝（原型 commit 校验链；名称域为同桶全 kind 共用）
		const QString name = _editName->text().trimmed();
		if (name.isEmpty())
		{
			setFeedback(_langEn ? QString("Enter a derived case name.") : QString::fromUtf8("请输入派生工况名称。"), true);
			return false;
		}
		if (store.nameExists(_storeKey, name, _editingId))
		{
			setFeedback(_langEn ? QString("This derived case name already exists.") : QString::fromUtf8("该派生工况名称已存在。"), true);
			return false;
		}

		//启用行缩放系数必须为有限数值
		for (const NativeRow& r : _rows)
			if (r.enabled)
			{
				double v = 0.0;
				if (!finiteScaleValue(r.scale, v))
				{
					setFeedback(_langEn ? QString("Scale factors of selected cases must be finite numbers.")
										: QString::fromUtf8("所选工况的缩放系数须为有限数值。"), true);
					return false;
				}
			}
		for (const DerivedRow& dr : _derivedRows)
			if (dr.enabled && dr.defId != _editingId)
			{
				double v = 0.0;
				if (!finiteScaleValue(dr.scale, v))
				{
					setFeedback(_langEn ? QString("Scale factors of selected cases must be finite numbers.")
										: QString::fromUtf8("所选工况的缩放系数须为有限数值。"), true);
					return false;
				}
			}

		//快照提交（规范化为 store 定义）：启用的原生来源 + 派生引用
		//（未配置定义不可被引用，UI 已禁选，此处双保险）
		PostWindow::DerivedCaseDefinition def = *editing;
		def.name = name;
		def.rows.clear();
		for (const NativeRow& r : _rows)
		{
			if (!r.enabled) continue;
			PostWindow::DerivedSourceRow row;
			row.id = QString::number(r.frameId);
			row.name = r.caseName;
			row.frame = r.frameText;
			row.scale = r.scale.trimmed();
			row.frameId = r.frameId;
			def.rows.append(row);
		}
		for (const DerivedRow& dr : _derivedRows)
		{
			if (!dr.enabled || dr.defId == _editingId) continue;
			const PostWindow::DerivedCaseDefinition* target = defById(defs, dr.defId);
			if (target == nullptr || !target->configured) continue;
			PostWindow::DerivedSourceRow row;
			row.id = QStringLiteral("derived:%1|0").arg(dr.defId);
			row.name = target->name;
			row.frame = QStringLiteral("0：%1").arg(_langEn ? QString("Superposition") : QString::fromUtf8("线性叠加"));
			row.scale = dr.scale.trimmed();
			row.isDerived = true;
			row.derivedId = dr.defId;
			def.rows.append(row);
		}
		def.configured = !def.rows.isEmpty();

		//整体引用校验（原型 validateDefinition 收敛为 store::checkReferences：
		//编辑草稿以 draftRefs 模拟，无需先写回再回滚）
		QList<int> draftRefs;
		for (const PostWindow::DerivedSourceRow& row : def.rows)
			if (row.isDerived && row.derivedId > 0)
				draftRefs << row.derivedId;
		switch (store.checkReferences(defs, _editingId, draftRefs))
		{
		case mPostDerivedCaseStore::ReferenceError::MissingTarget:
			setFeedback(_langEn ? QString("A referenced derived case is unavailable; reselect it.")
								: QString::fromUtf8("所选派生工况不可用，请重新选择。"), true);
			return false;
		case mPostDerivedCaseStore::ReferenceError::Cycle:
			setFeedback(_langEn ? QString("Derived cases cannot reference themselves or form cycles.")
								: QString::fromUtf8("不能引用自身或形成循环引用。"), true);
			return false;
		case mPostDerivedCaseStore::ReferenceError::DepthExceeded:
			setFeedback(_langEn ? QString("Derived cases can reference at most two levels.")
								: QString::fromUtf8("最多支持两层派生工况引用。"), true);
			return false;
		default:
			break;
		}

		store.updateDefinition(_storeKey, def);
		setFeedback(QString());
		return true;
	}

	void mPostLinearSuperpositionPanel::applyEditingDefToRows()
	{
		for (NativeRow& r : _rows)
		{
			r.enabled = false;
			r.scale = QStringLiteral("1");
		}
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		const PostWindow::DerivedCaseDefinition* def = defById(defs, _editingId);
		if (def == nullptr) return;
		//已保存启用行回填勾选/系数（原生按帧号；派生引用行按定义 id——
		//派生勾选恢复为存量行为缺口，批次C 收口时统一补齐）
		QHash<int, QString> scaleByFrame;
		for (const PostWindow::DerivedSourceRow& row : def->rows)
			if (!row.isDerived)
				scaleByFrame.insert(row.frameId, row.scale);
		for (NativeRow& r : _rows)
			if (scaleByFrame.contains(r.frameId))
			{
				r.enabled = true;
				r.scale = scaleByFrame.value(r.frameId);
			}
	}

	void mPostLinearSuperpositionPanel::updateCurrentRowName()
	{
		if (_tableLinear == nullptr || _editingId < 0) return;
		const QString name = _editName->text().trimmed();
		for (int r = 0; r < _tableLinear->rowCount(); ++r)
		{
			QTableWidgetItem* it = _tableLinear->item(r, 1);
			if (it != nullptr && it->data(Qt::UserRole).isValid() && it->data(Qt::UserRole).toInt() == _editingId)
			{
				it->setText(name);
				it->setToolTip(name);
				return;
			}
		}
	}

	void mPostLinearSuperpositionPanel::syncDerivedRows()
	{
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		//按定义集对齐派生行（defId 匹配保留既有勾选/系数；新定义行默认未选）
		QHash<int, DerivedRow> previousById;
		for (const DerivedRow& dr : _derivedRows)
			previousById.insert(dr.defId, dr);
		_derivedRows.clear();
		for (const PostWindow::DerivedCaseDefinition& def : defs)
		{
			if (previousById.contains(def.id))
				_derivedRows.append(previousById.value(def.id));
			else
				_derivedRows.append({ def.id, QStringLiteral("1"), false });
		}
		//当前编辑行恒不启用（防自引用）
		for (DerivedRow& dr : _derivedRows)
			if (dr.defId == _editingId) { dr.enabled = false; dr.scale = QStringLiteral("1"); }
	}

	QList<PostWindow::DerivedCaseDefinition> mPostLinearSuperpositionPanel::currentDefs() const
	{
		if (_storeKey < 0)
			return {};
		return mPostDerivedCaseStore::instance().definitions(_storeKey, DerivedCaseKind::Superposition);
	}

	bool mPostLinearSuperpositionPanel::collectSelectedFrameScales(QHash<int, double>& frameScales)
	{
		/*
		* 批次C 收敛：勾选来源经 commitDefinition 已落 store（本函数唯一调用点在
		* slot_generateLinear 顶部 commit 之后），直接展开当前编辑定义——
		* mPostDerivedCaseStore::expandDefinition 单一实现（与步序列/包络同源，
		* 私有递归已删）；线性叠加语义=同帧比例相加（frameId 归并）。
		*/
		QList<mPostDerivedCaseStore::ExpandedSource> expanded;
		mPostDerivedCaseStore::ReferenceError error = mPostDerivedCaseStore::ReferenceError::None;
		if (!mPostDerivedCaseStore::expandDefinition(
				mPostDerivedCaseStore::instance().definitions(_storeKey), _editingId,
				&expanded, &error))
		{
			switch (error)
			{
			case mPostDerivedCaseStore::ReferenceError::MissingTarget:
				setFeedback(_langEn ? QString("A referenced derived case is unavailable; reselect it.")
									: QString::fromUtf8("所选派生工况不可用，请重新选择。"), true);
				break;
			case mPostDerivedCaseStore::ReferenceError::Cycle:
				setFeedback(_langEn ? QString("Self-reference or circular reference is not allowed.")
									: QString::fromUtf8("不能引用自身或形成循环引用。"), true);
				break;
			case mPostDerivedCaseStore::ReferenceError::DepthExceeded:
				setFeedback(_langEn ? QString("Derived cases can reference at most two levels.")
									: QString::fromUtf8("最多支持两层派生工况引用。"), true);
				break;
			default:
				setFeedback(_langEn ? QString("Scale factors of selected cases must be finite numbers.")
									: QString::fromUtf8("所选工况的缩放系数须为有限数值。"), true);
				break;
			}
			return false;
		}
		for (const mPostDerivedCaseStore::ExpandedSource& src : expanded)
		{
			//线性叠加来源行直存帧号（面板建行时经 getCaseFrameNamesAndIDs 解析）；
			//失效帧防御（结果文件重导后帧号漂移）走"来源不可用"
			if (src.frameId <= 0)
			{
				setFeedback(_langEn ? QString("A selected source case is unavailable. Re-select sources.")
									: QString::fromUtf8("所选来源工况不可用，请重新选择。"), true);
				return false;
			}
			frameScales[src.frameId] += src.scale;
		}
		return true;
	}

	QString mPostLinearSuperpositionPanel::derivedRowDeniedReason(int candidateDefId) const
	{
		/*
		* 行级禁用预判（批次C）：以"当前勾选的派生引用集 + candidate 行"模拟
		* store::checkReferences（编辑草稿模拟，步序列/包络面板同款机制），
		* 环/两层上限在勾选前即可见。
		*/
		QList<int> draftRefs;
		for (const DerivedRow& dr : _derivedRows)
			if (dr.enabled && dr.defId != _editingId && dr.defId != candidateDefId)
				draftRefs << dr.defId;
		const mPostDerivedCaseStore::ReferenceError error =
			mPostDerivedCaseStore::checkReferences(
				mPostDerivedCaseStore::instance().definitions(_storeKey),
				_editingId, draftRefs, candidateDefId);
		switch (error)
		{
		case mPostDerivedCaseStore::ReferenceError::MissingTarget:
			return _langEn ? QString("The selected derived case is unavailable. Re-select it.")
						   : QString::fromUtf8("所选派生工况不可用，请重新选择。");
		case mPostDerivedCaseStore::ReferenceError::Cycle:
			return _langEn ? QString("Self-reference or circular reference is not allowed.")
						   : QString::fromUtf8("不能引用自身或形成循环引用。");
		case mPostDerivedCaseStore::ReferenceError::DepthExceeded:
			return _langEn ? QString("Derived cases can reference at most two levels.")
						   : QString::fromUtf8("最多支持两层派生工况引用。");
		default:
			return QString();
		}
	}

	void mPostLinearSuperpositionPanel::newLinearPanel(int caseid)
	{
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (postDB == nullptr) return;
		_storeKey = getCVM->getModelData()->getModelDataID();	//store 桶键随入口更新

		//历史链先行（口径同旧版）：emit → 树端 slot_updateLinearSuperpositionTree →
		//editLinear → setCurViewName 绑定源云图视口（_curViewName 就位后行集才取得到
		//正确的云图变量；跨结果库时其内部已重建行集；store 桶为空时其兜底分支会
		//建首个定义——置位 _fallbackCreated 供下方判重，避免本次入口双建）
		_fallbackCreated = false;
		MechCommon::mMechGlobalSignals::getInstance()->createLinearSuperpositionSig();

		//每次入口=新建派生工况定义（原型 createDerivedCase），命名"线性叠加-N"避开已有
		if (!_fallbackCreated)
		{
			PostWindow::DerivedCaseDefinition def;
			def.kind = DerivedCaseKind::Superposition;
			def.name = mPostDerivedCaseStore::instance().uniqueName(_storeKey,
				_langEn ? QString("Superposition") : QString::fromUtf8("线性叠加"));
			const int newId = mPostDerivedCaseStore::instance().appendDefinition(_storeKey, def);
			if (newId >= 0)
			{
				_editingId = newId;
				syncDerivedRows();
			}
		}
		//定义集须用具名局部持有：defById 返回指向列表元素的指针，直接喂临时量
		//（defById(currentDefs(),...)）时临时 QList 在分号处销毁——COW 共享缓冲
		//随存储桶变更/引用归零被释放后即为悬空指针（2026-11-13 崩溃：setText
		//读到 0xDD 调试堆填充，Debug 崩 0xC0000005 读 0xFFFF...F）
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		const PostWindow::DerivedCaseDefinition* editing = defById(defs, _editingId);
		if (editing == nullptr) return;
		_editName->setText(editing->name);
		setFeedback(QString());

		//行集重建（_curViewName 已就位；新定义无启用行=全不勾）并渲染
		setTableData(postDB);
	}

	void mPostLinearSuperpositionPanel::setCurViewName(const QString& viewName)
	{
		_curViewName = viewName;

		//重定位结果库（视口/模型漂移防护；口径同 mPostSetPanel::updateContextByViewName）
		REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(viewName);
		if (rend != nullptr)
		{
			const QString postName = rend->getPostDataName();
			if (!postName.isEmpty())
				_db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		}
		if (_db != nullptr)
		{
			_storeKey = getCVM->getModelData()->getModelDataID();
			if (_db->getFile() != _rowsPostKey)
				setTableData(_db);        //跨结果库：行集随库重建（内部含渲染）
		}

		//定义集恢复/兜底（两路径共用）：空则建首个定义；编辑定义失效则回到最新定义
		QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		if (defs.isEmpty())
		{
			PostWindow::DerivedCaseDefinition def;
			def.kind = DerivedCaseKind::Superposition;
			def.name = mPostDerivedCaseStore::instance().uniqueName(_storeKey,
				_langEn ? QString("Superposition") : QString::fromUtf8("线性叠加"));
			const int newId = mPostDerivedCaseStore::instance().appendDefinition(_storeKey, def);
			if (newId < 0)
			{
				renderTable();
				return;
			}
			_editingId = newId;
			syncDerivedRows();
			_editName->setText(def.name);
			_fallbackCreated = true;   //newLinearPanel 判重用（emit 同步返回后读）
		}
		else if (defById(defs, _editingId) == nullptr)
		{
			_editingId = defs.last().id;
			_editName->setText(defs.last().name);
			applyEditingDefToRows();
		}
		renderTable();
	}

	void mPostLinearSuperpositionPanel::editDefinitionById(int definitionId)
	{
		//树"派生工况"分支单击回编（批次A0）：绑定当前后处理上下文（有后处理
		//视口时重建行集）后装载目标定义进入编辑
		REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
		if (rend != nullptr && rend->getPostRender() != nullptr)
		{
			const QString postName = rend->getPostDataName();
			if (!postName.isEmpty())
			{
				QPointer<mPostDataBase> db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
				if (db != nullptr)
				{
					_storeKey = getCVM->getModelData()->getModelDataID();
					setTableData(db);
				}
			}
		}
		const QList<PostWindow::DerivedCaseDefinition> defs = currentDefs();
		const PostWindow::DerivedCaseDefinition* def = defById(defs, definitionId);
		if (def == nullptr || def->kind != DerivedCaseKind::Superposition)
			return;
		_editingId = def->id;
		_editName->setText(def->name);
		syncDerivedRows();
		applyEditingDefToRows();
		setFeedback(QString());
		renderTable();
	}

	QString mPostLinearSuperpositionPanel::getCloudViewName(QString view)
	{
		//源云图定位：优先管理器关系（LinearInfo.viewName → resultId → CloudItem.viewName）
		//——显示名接管后视口名不再携带 "_Linear<N>" 后缀，名字解析只作兜底
		//（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
		const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
			.getCloudViewNameByView(PostData::ItemType::LinearItem, view);
		if (!byInfo.isEmpty())
			return byInfo;
		QRegularExpression re("^(.*)(_Linear\\d+)$");
		QRegularExpressionMatch match = re.match(view);
		if (match.hasMatch()) {
			return match.captured(1);
		}
		return view;
	}

	void mPostLinearSuperpositionPanel::setTableData(QPointer<mPostDataBase> postDB)
	{
		if (postDB == nullptr) return;
		_db = postDB;
		_rowsPostKey = postDB->getFile();

		//原生来源：当前结果库 工况×帧 笛卡尔积（对齐同族步序列/包络面板与原型 sources()
		//原生段；帧ID 经 getCaseFrameNamesAndIDs 取出供生成链按帧读数，映射缺失的异常行跳过）
		_rows.clear();
		const QStringList caseNames = naturalSorted(postDB->getAllCaseNames());
		for (const QString& caseName : caseNames)
		{
			QHash<QString, QPair<int, int>> frameIds;   //帧label → (帧序号, 帧ID)
			postDB->getCaseFrameNamesAndIDs(caseName, frameIds);
			const QStringList frameNames = naturalSorted(postDB->getCaseFrameNames(caseName));
			for (const QString& frameName : frameNames)
			{
				const int frameId = frameIds.value(frameName, qMakePair(-1, -1)).second;
				if (frameId < 0) continue;
				NativeRow row;
				row.frameId = frameId;
				row.caseName = caseName;
				row.frameText = frameName;
				_rows.append(row);
			}
		}
		syncDerivedRows();
		applyEditingDefToRows();
		renderTable();
	}

	void mPostLinearSuperpositionPanel::setShowMode()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr) return;
		switch (viewportPrivate->_showViewCheckedStatus)
		{
		case 0:postRender->setShowFuntion(ShowFuntion::WireEdge);
			break;
		case 1:postRender->setShowFuntion(ShowFuntion::WireFrame);
			break;
		case 2:postRender->setShowFuntion(ShowFuntion::ElementFace);
			break;
		case 3:postRender->setShowFuntion(ShowFuntion::SmoothShaded);
			break;
		}
		postRend->updatePaint();
	}

	void mPostLinearSuperpositionPanel::slot_generateLinear()
	{
		//提交当前编辑（名称/缩放校验失败即中止，反馈行已提示）
		if (!commitDefinition()) return;

		//收集勾选来源：原生直取 + 派生引用展开（同帧比例相加）
		QHash<int, double> frameScaleHash;
		if (!collectSelectedFrameScales(frameScaleHash)) return;
		if (frameScaleHash.isEmpty())
		{
			setFeedback(_langEn ? QString("Select at least one case first.") : QString::fromUtf8("请先勾选参与叠加的工况。"), true);
			return;
		}
		QList<int> frames = frameScaleHash.keys();
		std::sort(frames.begin(), frames.end());
		QMap<int, double> frameRatios;
		for (auto it = frameScaleHash.constBegin(); it != frameScaleHash.constEnd(); ++it)
			frameRatios.insert(it.key(), qBound(-10000.0, it.value(), 10000.0));

		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		QString postName = rebuildPostRend->getPostDataName();
		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (modeldb == nullptr)	return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata == nullptr) return;

		//读取变量
		QString cloudView = getCloudViewName(_curViewName);
		mPostGUIData* cloudData = getCVM->getGUIDataByViewName(cloudView);
		QString varstr = cloudData->_cloudVarName;
		QString compostr = cloudData->_cloudCompo;
		QString layerstr = cloudData->_cloudLayerName;
		QString complexstr = cloudData->_cloudComplex;
		QString postPathName = modeldb->getFile();

		/*
		* 批次C 读数收敛：四分支读数迁 mPostDerivedCaseEngine::readSourceFrame 统一
		* 读数器（与步序列/包络同一实现）。行为变化：任一分支失败即中止生成并落
		* 输出窗（旧实现仅 native/vtk 失败中止、旧格式失败仅报告继续——坏数据会
		* 静默混入叠加结果，收敛后不再可能）。
		*/
		mPostDerivedCaseEngine::GenerateContext readCtx;
		readCtx.postDB = modeldb;
		readCtx.postPathName = postPathName;
		readCtx.postName = postName;
		readCtx.var = varstr;
		readCtx.compo = compostr;
		readCtx.layer = layerstr;
		readCtx.complexMode = complexstr;
		for (int frame : frames)
		{
			if (!mPostDerivedCaseEngine::readSourceFrame(readCtx, frame))
				return;
		}

		//获取变量并存储
		QHash<int, double> allResults;
		for (int i = 0; i < frames.size(); i++)
		{
			int frame = frames[i];
			double ratio = frameRatios.value(frame, 1.0);
			QHash<int, double> frameResult = modeldb->getResultDataByFrameID(frame);
			for (auto it = frameResult.constBegin(); it != frameResult.constEnd(); ++it)
			{
				allResults[it.key()] += it.value() * ratio;
			}
		}

		modeldb->setLinearSuperpositionData(allResults, frames[0]);

		int fi = frames[0];
		// S5 门面直接替换（B1b(M1) 口径延续）：线性叠加结果已于上方写回 mPostDataBase，
		// 门面装载工厂从库现造代理即含叠加值；旧手造代理三连调随移除集消亡——
		// attachModel（幂等换绑）+requestFrames（渐进装载）+seekFrame（未就绪保持上一帧）
		if (!postRender->attachModel(modeldb.data(), rebuildPostRend->getViewportName()))
			return;
		postRender->requestFrames(fi, fi);
		if (!postRender->seekFrame(fi))
			return;
		mxr::PostFrameDataProxy* frameData = postRender->frameProxy(postRender->currentFrame());   // F8：绑定点代理现取（换帧/重装载后重取）
		if (frameData == nullptr) return;

		QPointer<PostData::mPostModelData> modelData = modeldb->getModelData(fi);
		if (modelData == nullptr)
			return;

		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameData->getRendQueryDataProxy());
		if (query_proxy != nullptr)
		{
			for (auto it : modeldb->getAllRigidWallDatas())
			{
				QString name = it->getRigidWallName();
				QVector3D endPos = modelData->_frameInfo.getRigidWallEndPos(name);
				it->updatePos(endPos);
				query_proxy->appendRigidWallData(it);
			}
		}
		// S5 门面直接替换：绑定已由上方 seekFrame 内部消化，旧 setRendCurrentFrameData 注入步删除。
		// B1b(R18-2①) 口径延续：追加刚体墙数据错过实例构建期 ImportOperate，改走 GUI 可达替代
		// 入口 ReplaceOnePartOperate 逐墙重建 partRender（幂等覆盖；渲染库修复落地后改回单发
		// UpdateAllRigidWallOperate(ImportOperate)）
		{
			std::set<QString> appendedWallNames;
			for (auto it : modeldb->getAllRigidWallDatas())
				appendedWallNames.insert(it->getRigidWallName());
			if (!appendedWallNames.empty())
			PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::ReplaceOnePartOperate,
			appendedWallNames);   // S5 G5（D9 载体）
		}


		//数据读取完成,渲染窗口可接入变量数据
		mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameData->getModelRenderDataProxy());
		if (model_proxy != nullptr) { model_proxy->updateValue(); }

		postRender->updateAllModelOperate(UpdateVariableValue);
		postRender->updateAllModelOperate(UpdateMinMax);
		rebuildPostRend->update();

		QList<PostVariableInfo> varInfos = modelData->_frameInfo._frameVars;
		for (auto it : varInfos)
		{
			if (it._varName == varstr)
			{
				if (it._varType == 0)
				{
					postRender->updateAllModelOperate(ShowNodeColor);
				}
				else if (it._varType == 1)
				{
					postRender->updateAllModelOperate(ShowElementColor);
				}
				break;
			}
		}

		postRender->getColorTableWindow()->setVisiable(true);

		//线性叠加视图继承源云图视口的图例最大最小值显隐状态
		REBUILD_PostRend* cloudRend = getCVM->getPostRenderByViewName(cloudView);
		if (cloudRend != nullptr && cloudRend != rebuildPostRend && cloudRend->getPostRender() != nullptr)
		{
			auto* srcCT = cloudRend->getPostRender()->getColorTableWindow();
			auto* dstCT = postRender->getColorTableWindow();
			dstCT->showOrHidleMaxMinFoot({ 1,  srcCT->isMaxFootVisible() });
			dstCT->showOrHidleMaxMinFoot({ -1, srcCT->isMinFootVisible() });
		}
		std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
		auto id_all = query_proxy->getAllPartID();

		if (auto* stateEditor = query_proxy->partStateEditor())
		{
			stateEditor->setVisibleParts(id_all);
			stateEditor->setInvisibleParts(hideParts);
		}
		getCVM->updateAllPostView();
		for (auto id : hideParts)
		{
			modelData->setMeshPartVisual(rebuildPostRend->getViewportName(), id, false);
		}

		float minval = FLT_MAX;
		float maxval = -FLT_MAX;

		int index = cloudData->_rangeIndex;
		if (index == 0)
		{
			maxval = query_proxy->getOriginalMaxData();
			minval = query_proxy->getOriginalMinData();
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
			    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			//this->slot_setMostValue(false);
			mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameData->getRendQueryDataProxy());   // S5：F8 现取（同帧同步链，frameData 即当前绑定）
			if (query_proxy1 == nullptr)
				return;
			mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameData->getModelRenderDataProxy());
			if (model_query_proxy != nullptr)
			{
				mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();
				std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
				std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
				if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
				{
					postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
					postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
					postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
					postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
				}
				else
				{
					postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
					postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
					postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
					postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
				}
			}
		}
		else if (index == 1)
		{
			auto cur_min_max = model_proxy->getPartMinMax();
			minval = cur_min_max.first;
			maxval = cur_min_max.second;
			query_proxy->setCurrentMinData(minval);
			query_proxy->setCurrentMaxData(maxval);
			{
			    // 色表值域收编（C4 双效点·默认值域段+窗写段一口两掩码，顺序按现状）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
			        | mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
			    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			//this->slot_setMostValue(false);
			mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameData->getRendQueryDataProxy());   // S5：F8 现取（同帧同步链，frameData 即当前绑定）
			if (query_proxy1 == nullptr)
				return;
			mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameData->getModelRenderDataProxy());
			if (model_query_proxy != nullptr)
			{
				mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();
				std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
				std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
				if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
				{
					postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
					postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
					postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
					postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
				}
				else
				{
					postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
					postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
					postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
					postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
				}
			}
		}

		{
		    // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
		    mxr::ColorLegendParams _lg;
		    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
		    _lg.valueMin = minval;
		    _lg.valueMax = maxval;
		    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
		}
		postRender->updateAllModelOperate(ColorAllPartOperate);

		float maxvalue = query_proxy->getOriginalMaxData();
		float minvalue = query_proxy->getOriginalMinData();

		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
		if (cloudData->_cloudLayerName.isEmpty())
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s", cloudData->_cloudVarName.toStdString().c_str(), cloudData->_cloudCompo.toStdString().c_str());
		}
		else
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s:%s", cloudData->_cloudVarName.toStdString().c_str(), cloudData->_cloudCompo.toStdString().c_str(), cloudData->_cloudLayerName.toStdString().c_str());
		}

		//设置显示模式
        setShowMode();

		rebuildPostRend->FitView();
		rebuildPostRend->updatePaint();
	}
}
