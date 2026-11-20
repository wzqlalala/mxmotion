#include "mPostCurveExtract.h"

#include <QComboBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFrame>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPen>
#include <QSet>
#include <QHash>

#include <functional>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "PickCommonFunction.h"
#include "mPostGeneralPickUp.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveView/mCurveViewportFactory.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "PostCurveStruct.h"
#include "mPostInfoHub.h"
#include "mPost3DResult.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"

#include <algorithm>

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace
{
	//令牌（--std-* 标准档；出处 V1.3 原型 CSS 变量定义块）
	constexpr int kControl = 26;		//--std-control 控件外高
	constexpr int kManagerRow = 26;		//--std-manager-row 管理表格行高
	constexpr int kChoice = 13;			//--std-choice 复选尺寸
	constexpr int kGroupInset = 7;		//--std-group-inset 文本列左内边距
	constexpr int kBlockInset = 10;		//块左右内边距：原型 fold+field inset 12 − 面板边距 2
	constexpr int kRadius = 4;			//--std-radius 控件圆角（V1.3：2→4）
	constexpr int kGap = 6;				//--std-gap

	const char* kLineBorder = "#DCE2E9";	//--std-border
	const char* kHeadBg = "#E9EDF3";		//--std-header-color
	const char* kTextColor = "#303313";		//--std-text / --std-label（V1.3 统一主文本色，原 #424B57/#5C6674）
	const char* kMuted = "#788391";			//--std-muted
	const char* kHoverBg = "#F0F7FF";		//--std-hover
	const char* kSelectedBg = "#E8F3FF";	//--std-selected
	const char* kPrimary = "#1890FF";		//--std-primary / --std-focus

	//主操作蓝按钮（绘制）：蓝底白字，悬停深蓝，禁用灰（--std-action/-hover）
	const char* kPrimaryBtnQss =
		"QPushButton{background:#1677C8;border:1px solid #1677C8;color:#FFFFFF;border-radius:4px;}"
		"QPushButton:hover{background:#1267B0;border:1px solid #1267B0;}"
		"QPushButton:disabled{background:#F5F7FA;border:1px solid #DCE2E9;color:#A6AFBB;}";

	//绘制选项（原型 grid minmax(0,1fr)+auto 的下拉+主按钮行；枚举序即原型选项序）
	enum LayoutOption
	{
		LayoutOnePerRequest = 0,	//每个请求一张图
		LayoutOnePerComponent,		//每个分量一张图
		LayoutOnePerPlot,			//每条曲线一张图（默认）
		LayoutNewWindow,			//新视图页
		LayoutExistingXy			//现有曲线图（显示绘制目标树）
	};

	//2x 采样绘制线条图标（近似原型 inline SVG 语义）
	QIcon makeIcon(int px, const QColor& color, const std::function<void(QPainter&)>& paint)
	{
		QPixmap pm(px * 2, px * 2);
		pm.setDevicePixelRatio(2.0);
		pm.fill(Qt::transparent);
		QPainter p(&pm);
		p.setRenderHint(QPainter::Antialiasing);
		QPen pen(color, 1.4);
		pen.setWidthF(1.4);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		p.setPen(pen);
		paint(p);
		return QIcon(pm);
	}

	//全选：两框各带勾；取消：两框无勾；反选：两框+上下交换箭头（原型 actionIcons）
	QIcon tableActionIcon(int kind)
	{
		return makeIcon(20, QColor(kTextColor), [kind](QPainter& p)
			{
				p.drawRoundedRect(QRectF(1.5, 2.5, 11, 7), 1.0, 1.0);
				p.drawRoundedRect(QRectF(1.5, 12.5, 11, 7), 1.0, 1.0);
				if (kind == 0)
				{
					p.drawLine(QPointF(4, 6), QPointF(6, 8));
					p.drawLine(QPointF(6, 8), QPointF(10, 4));
					p.drawLine(QPointF(4, 16), QPointF(6, 18));
					p.drawLine(QPointF(6, 18), QPointF(10, 14));
				}
				else if (kind == 2)
				{
					p.drawLine(QPointF(16, 4), QPointF(16, 9));
					p.drawLine(QPointF(14.5, 7.5), QPointF(16, 9));
					p.drawLine(QPointF(16, 9), QPointF(17.5, 7.5));
					p.drawLine(QPointF(16, 16), QPointF(16, 11));
					p.drawLine(QPointF(14.5, 12.5), QPointF(16, 11));
					p.drawLine(QPointF(16, 11), QPointF(17.5, 12.5));
				}
			});
	}

	//管理表格网格线委托：行底线 + 复选列右线（原型 td border-right/bottom）
	class GridLineDelegate : public QStyledItemDelegate
	{
	public:
		using QStyledItemDelegate::QStyledItemDelegate;
		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override
		{
			//文本列内容整体右移 7px（原型 td padding:0 7px 的左内边距）
			QStyleOptionViewItem opt = option;
			if (index.column() == 1)
			{
				opt.rect.adjust(kGroupInset, 0, 0, 0);
				QStyledItemDelegate::paint(painter, opt, index);
			}
			else
			{
				QStyledItemDelegate::paint(painter, option, index);
			}
			painter->save();
			QPen pen(QColor(kLineBorder), 1.0);
			pen.setWidthF(1.0);
			painter->setPen(pen);
			const QRect r = option.rect;
			painter->drawLine(QPointF(r.left() + 0.5, r.bottom() - 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			if (index.column() == 0)
				painter->drawLine(QPointF(r.right() - 0.5, r.top() + 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			painter->restore();
		}
	};

	//复选列居中容器（cellWidget：QCheckBox 在 26px 复选列水平垂直居中，原型 margin:auto）
	QCheckBox* centeredCheckBox(QTableWidget* parent)
	{
		QWidget* cell = new QWidget(parent);
		QVBoxLayout* lay = new QVBoxLayout(cell);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
		QCheckBox* box = new QCheckBox(cell);
		lay->addWidget(box, 0, Qt::AlignCenter);
		return box;
	}

	//变量对象类型（1=单元）→ 拾取过滤器（拾取栈 pickActiveChanged 收口用；
	//activePickSig 只同步工具栏拾取组，对后处理渲染视口不生效，须直设过滤器）
	mxr::PickFilter pickFilterOf(bool isElement)
	{
		return isElement ? mxr::PickFilter::PickMeshElement : mxr::PickFilter::PickMeshNode;
	}
}

namespace PostWindow
{
	mPostCurveExtractPanel::mPostCurveExtractPanel(QWidget* parent) : mPanelBase(parent)
	{
		_langEn = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1);
		setObjectName("curveExtractParameters");
		setupUI();
		applyLanguageTexts();

		//语言切换全量刷新
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));

		//Escape 退出拾取态（原型 keydown Escape deactivate）
		bindShortcut(QKeySequence(Qt::Key_Escape), [this]() { _pickController->setPickActive(false); });
	}

	void mPostCurveExtractPanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		_mainLayout->setSpacing(kGap);
		setStyleSheet(
			"mPostCurveExtractPanel{background-color:#FFFFFF;}"
			"QComboBox QAbstractItemView{background-color:#FFFFFF;color:#303313;"
			"selection-background-color:#E8F3FF;selection-color:#303313;}");

		/******** 折叠组：工况/变量/对象选择（原型 parameter-fields 三行） ********/
		_mainFold = new FoldSection(QString::fromUtf8("曲线提取"), this);

		_comboCase = new QComboBox(this);
		_comboCase->setObjectName("curveExtractCase");
		_comboCase->setFixedHeight(kControl);
		_mainFold->addRow(new PanelLabel(QString::fromUtf8("工况")), _comboCase);

		_comboVariable = new QComboBox(this);
		_comboVariable->setObjectName("curveExtractVariable");
		_comboVariable->setFixedHeight(kControl);
		_mainFold->addRow(new PanelLabel(QString::fromUtf8("变量")), _comboVariable);

		//对象选择：mPost 拾取栈拾取条（2026-11-18 由手搓 PickEntryRow 替换，云图/向量图/
		//爆炸/阵列同款）——点击条体激活拾取/再点停止（开关语义）、详情弹窗（分组+逐项删/
		//按类型整删）、清空钮、计数文案内建；CBBlue 颜色块=云图同款视觉标识。对象类型=
		//当前变量固有属性（节点/单元），不出类型下拉（云图下拉因其类型用户可选）
		_pickController = mPostGeneralPickUp::createPickWidget(CBBlue, this);
		_pickController->widget()->setObjectName("curveExtractPick");   //ui-shot 沿用旧名
		_pickController->setSelectSetBtnVisible(false);                 //后处理无集合语义
		_pickController->setKeepHighlightOnDeactivate(false);           //解除拾取即清视口高亮
		_pickController->setAllowedTypes([this]() {
			//激活时按当前变量对象类型发白名单（1=单元，否则节点）
			QSet<MxFunctions::PickDataType> types;
			types.insert(_curVarElement
				? MxFunctions::PickDataType::Element : MxFunctions::PickDataType::Node);
			return types;
			});
		_mainFold->addRow(new PanelLabel(QString::fromUtf8("对象选择")), _pickController->widget());

		_mainFold->refreshLayout();
		_mainLayout->addWidget(_mainFold);

		/******** 分量/层 勾选表（组外沿字段边界平铺，标题及批量动作置于表头） ********/
		//外层边距 10：表格块左缘对齐原型 padding-inline 12（面板 2+10），与折叠组行左缘同基线
		_tableComponent = buildCheckTable("curveExtractComponentTable");
		_tableLayer = buildCheckTable("curveExtractLayerTable");
		for (CheckTable* ct : { &_tableComponent, &_tableLayer })
		{
			QWidget* wrap = new QWidget(this);
			QVBoxLayout* wrapLayout = new QVBoxLayout(wrap);
			wrapLayout->setContentsMargins(10, 0, 10, 0);
			wrapLayout->setSpacing(0);
			wrapLayout->addWidget(ct->root);
			_mainLayout->addWidget(wrap);
		}

		/******** 绘制选项行：下拉（1fr）+ 绘制主按钮（auto） ********/
		QWidget* drawRow = new QWidget(this);
		QHBoxLayout* drawLayout = new QHBoxLayout(drawRow);
		drawLayout->setContentsMargins(kBlockInset, 0, kBlockInset, 0);
		drawLayout->setSpacing(8);	//标签→控件列距 8（--std-column-gap，对齐原型 field x 偏移）
		_labelDraw = new PanelLabel(QString::fromUtf8("绘制选项"), drawRow);
		drawLayout->addWidget(_labelDraw);
		_comboLayout = new QComboBox(drawRow);
		_comboLayout->setObjectName("curveExtractLayout");
		_comboLayout->setFixedHeight(kControl);
		drawLayout->addWidget(_comboLayout, 1);
		_btnDraw = new QPushButton(drawRow);
		_btnDraw->setObjectName("curveExtractDraw");
		_btnDraw->setStyleSheet(kPrimaryBtnQss);
		//两字按钮 40×26（M2 口径；全局 QSS width:80px 需显式覆写，原型实测 34 为窄版）
		_btnDraw->setFixedSize(40, kControl);
		drawLayout->addWidget(_btnDraw);
		_mainLayout->addWidget(drawRow);

		/******** 绘制目标树（仅"现有曲线图"显示；高 7 行管理表格） ********/
		QWidget* targetRegion = new QWidget(this);
		targetRegion->setObjectName("curveExtractTargetRegion");
		QVBoxLayout* targetLayout = new QVBoxLayout(targetRegion);
		targetLayout->setContentsMargins(kBlockInset, 0, kBlockInset, kGap);
		targetLayout->setSpacing(0);
		_targetTree = new QTreeWidget(targetRegion);
		_targetTree->setObjectName("curveExtractTargetTree");
		_targetTree->setHeaderHidden(true);
		_targetTree->setIndentation(12);
		_targetTree->setSelectionMode(QAbstractItemView::NoSelection);
		_targetTree->setFixedHeight(7 * kManagerRow + 2);
		_targetTree->setStyleSheet(
			QString("QTreeWidget{background:#FFFFFF;border:1px solid %1;font-size:12px;}"
				"QTreeWidget::item{height:%2px;}").arg(kLineBorder).arg(kManagerRow));
		targetLayout->addWidget(_targetTree);
		targetRegion->setVisible(false);
		_mainLayout->addWidget(targetRegion);

		/******** 状态行（空文案整行收起） ********/
		_statusLabel = new QLabel(this);
		_statusLabel->setObjectName("curveExtractDrawStatus");
		_statusLabel->setContentsMargins(10, 0, 10, kGap);
		_statusLabel->setStyleSheet(
			QString("QLabel{color:%1;font-size:11px;border:0;background:transparent;}").arg(kMuted));
		_statusLabel->setVisible(false);
		_mainLayout->addWidget(_statusLabel);

		_mainLayout->addStretch();

		//级联与交互（原型：工况→变量→分量/层+拾取；任意变更清状态行）
		connect(_comboCase, &QComboBox::currentTextChanged, this, &mPostCurveExtractPanel::slot_chooseCase);
		connect(_comboVariable, &QComboBox::currentTextChanged, this, &mPostCurveExtractPanel::slot_chooseVariable);
		connect(_btnDraw, &QPushButton::clicked, this, &mPostCurveExtractPanel::slot_drawClicked);
		//拾取条激活/失活收口（云图 slot_objectPickActiveChanged 同口径；勿用
		//activePickSig——该信号只同步前处理工具栏按钮组，对后处理视口不生效）：
		//激活=校验后处理上下文（无效立即失活回退）+视口按变量对象类型设节点/单元
		//拾取过滤器；失活=清过滤器与拾取高亮+恢复工具栏全部拾取类型。收割
		//（ReadyToReadContainer 连接）、高亮与重入回显（激活时历史数据写回选择容器）
		//由拾取栈控制器自管
		connect(_pickController, &mPostPickWidgetController::pickActiveChanged, this,
			[this](bool active) {
				REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
				if (active)
				{
					if (postRend == nullptr || postRend->getPostRender() == nullptr || !_postDB)
					{
						_pickController->setPickActive(false);
						return;
					}
					postRend->setPickFilter(pickFilterOf(_curVarElement));
				}
				else
				{
					if (postRend != nullptr)
						postRend->setPickFilter(mxr::PickFilter::PickNothing);
					emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
				}
			});
		//拾取数据变化（视口拾取/详情弹窗删项/清空）→ 清状态行（旧收割即时反馈的等价收口；
		//计数与清单由条内文案/详情弹窗承载）
		connect(_pickController, &mPostPickWidgetController::dataChanged, this, [this]()
			{
				setStatus(QString());
			});
		connect(_comboLayout, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index)
			{
				setStatus(QString());
				//目标树仅"现有曲线图"显示；切换先退出拾取态（原型互斥模式），
				//进入该选项时刷新曲线模块图表列表（卡片随时增删）
				_pickController->setPickActive(false);
				if (index == LayoutExistingXy)
					rebuildTargetTree();
				_targetTree->parentWidget()->setVisible(index == LayoutExistingXy);
			});
		connect(_targetTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int)
			{
				const QString viewportId = item->data(0, Qt::UserRole + 1).toString();
				if (viewportId.isEmpty())
					return;
				_selectedTargetViewport = viewportId;
				for (QTreeWidgetItemIterator it(_targetTree); *it; ++it)
				{
					QTreeWidgetItem* node = *it;
					const bool selected = (node->data(0, Qt::UserRole + 1).toString() == viewportId);
					//V1.3 选中节点：selected 底，文字保持主文本色（--std-selectedText 已废除）
					node->setBackground(0, selected ? QBrush(QColor(kSelectedBg)) : QBrush());
					node->setForeground(0, QBrush(QColor(kTextColor)));
				}
				setStatus(QString());
			});
	}

	mPostCurveExtractPanel::CheckTable mPostCurveExtractPanel::buildCheckTable(const char* objectName)
	{
		CheckTable ct;
		ct.root = new QFrame(this);
		ct.root->setObjectName(objectName);
		ct.root->setFrameShape(QFrame::NoFrame);
		ct.root->setStyleSheet(
			QString("QFrame#%1{background:#FFFFFF;border:1px solid %2;}").arg(objectName, kLineBorder));
		QVBoxLayout* layout = new QVBoxLayout(ct.root);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		//表头：标题（左）+ 全选/取消/反选（右，24×24 无边框悬停浅蓝）
		QFrame* header = new QFrame(ct.root);
		header->setFixedHeight(kManagerRow);
		header->setFrameShape(QFrame::NoFrame);
		header->setStyleSheet(
			QString("QFrame{background:%1;border:0;border-bottom:1px solid %2;}"
				"QLabel{background:transparent;border:0;color:%3;font-size:12px;}"
				"QPushButton{background:transparent;border:0;border-radius:2px;}"
				"QPushButton:hover{background:%4;}")
				.arg(kHeadBg, kLineBorder, kTextColor, kHoverBg));
		QHBoxLayout* headerLayout = new QHBoxLayout(header);
		headerLayout->setContentsMargins(4, 0, 4, 0);
		headerLayout->setSpacing(0);
		ct.caption = new QLabel(header);
		headerLayout->addWidget(ct.caption);
		headerLayout->addStretch();
		ct.btnAll = new QPushButton(header);
		ct.btnAll->setIcon(tableActionIcon(0));
		ct.btnAll->setFixedSize(24, 24);
		ct.btnAll->setCursor(Qt::PointingHandCursor);
		headerLayout->addWidget(ct.btnAll);
		ct.btnNone = new QPushButton(header);
		ct.btnNone->setIcon(tableActionIcon(1));
		ct.btnNone->setFixedSize(24, 24);
		ct.btnNone->setCursor(Qt::PointingHandCursor);
		headerLayout->addWidget(ct.btnNone);
		ct.btnInvert = new QPushButton(header);
		ct.btnInvert->setIcon(tableActionIcon(2));
		ct.btnInvert->setFixedSize(24, 24);
		ct.btnInvert->setCursor(Qt::PointingHandCursor);
		headerLayout->addWidget(ct.btnInvert);
		layout->addWidget(header);

		//正文：复选(26)/文本 两列；行高 26；高度随内容行数 3~6 行（超出滚动），
		//Fixed 策略+按行数钉高（refreshCheckTable）防布局按默认 sizeHint(~192) 撑满上限
		ct.table = new QTableWidget(0, 2, ct.root);
		ct.table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		ct.table->setFocusPolicy(Qt::NoFocus);
		ct.table->setSelectionMode(QAbstractItemView::NoSelection);
		ct.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		ct.table->setWordWrap(false);
		ct.table->setShowGrid(false);
		ct.table->horizontalHeader()->setVisible(false);
		ct.table->verticalHeader()->setVisible(false);
		ct.table->verticalHeader()->setDefaultSectionSize(kManagerRow);
		ct.table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
		ct.table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		ct.table->setColumnWidth(0, kManagerRow);
		ct.table->setMinimumHeight(3 * kManagerRow);
		ct.table->setMaximumHeight(6 * kManagerRow);
		ct.table->setItemDelegate(new GridLineDelegate(ct.table));
		ct.table->setStyleSheet(
			QString("QTableWidget{background:#FFFFFF;border:0;font-size:12px;color:%1;}"
				"QTableWidget::item{padding:0;}"
				"QCheckBox{background:transparent;}").arg(kTextColor));
		layout->addWidget(ct.table);

		//批量动作（0 全选/1 取消/2 反选，写回见 applyTableAction）
		connect(ct.btnAll, &QPushButton::clicked, this, [this, ct]() { applyTableAction(const_cast<CheckTable&>(ct), 0); });
		connect(ct.btnNone, &QPushButton::clicked, this, [this, ct]() { applyTableAction(const_cast<CheckTable&>(ct), 1); });
		connect(ct.btnInvert, &QPushButton::clicked, this, [this, ct]() { applyTableAction(const_cast<CheckTable&>(ct), 2); });
		return ct;
	}

	QCheckBox* mPostCurveExtractPanel::rowCheckBox(QTableWidget* table, int row)
	{
		QWidget* cell = table->cellWidget(row, 0);
		return cell ? cell->findChild<QCheckBox*>() : nullptr;
	}

	void mPostCurveExtractPanel::applyTableAction(CheckTable& ct, int action)
	{
		QHash<QString, QSet<QString>>& state =
			(ct.table == _tableComponent.table) ? _checkedCompoByVar : _checkedLayerByVar;
		QSet<QString>& checked = state[_curVariable];
		for (int row = 0; row < ct.table->rowCount(); ++row)
		{
			QCheckBox* box = rowCheckBox(ct.table, row);
			QTableWidgetItem* text = ct.table->item(row, 1);
			if (!box || !text || !(text->flags() & Qt::ItemIsEnabled))
				continue;
			const bool on = (action == 0) || (action == 2 && !box->isChecked());
			box->setChecked(on);
			if (on) checked.insert(text->text()); else checked.remove(text->text());
		}
		setStatus(QString());
	}

	void mPostCurveExtractPanel::refreshCheckTable(const CheckTable& ct, const QStringList& items,
		QHash<QString, QSet<QString>>& stateByVar)
	{
		QTableWidget* table = ct.table;
		table->setUpdatesEnabled(false);
		table->setRowCount(0);	//行移除时旧跨列自动清除
		QSet<QString>& checked = stateByVar[_curVariable];
		if (items.isEmpty())
		{
			table->setRowCount(1);
			QTableWidgetItem* empty = new QTableWidgetItem;
			empty->setFlags(Qt::ItemIsEnabled);
			empty->setTextAlignment(Qt::AlignCenter);
			empty->setForeground(QBrush(QColor(kMuted)));
			QFont f = empty->font();
			f.setPixelSize(11);
			empty->setFont(f);
			empty->setText(_langEn
				? (ct.table == _tableComponent.table ? QString("No component data") : QString("No layer data"))
				: (ct.table == _tableComponent.table ? QString::fromUtf8("暂无分量数据") : QString::fromUtf8("暂无层数据")));
			table->setItem(0, 0, empty);
			table->setSpan(0, 0, 1, 2);
		}
		else
		{
			table->setRowCount(items.size());
			for (int row = 0; row < items.size(); ++row)
			{
				QCheckBox* box = centeredCheckBox(table);
				box->setChecked(checked.contains(items.at(row)));
				//cellWidget 必须承载居中容器而非复选框自身：rowCheckBox 经
				//findChild 取框（子孙不含自身），直设复选框会使勾选状态与
				//批量动作（全选/取消/反选）全部读不到
				table->setCellWidget(row, 0, box->parentWidget());
				//勾选状态写回按变量的独立集合；任意变更清状态行
				connect(box, &QCheckBox::toggled, this, [this, table, text = items.at(row)](bool on)
					{
						QHash<QString, QSet<QString>>& state = (table == _tableComponent.table)
							? _checkedCompoByVar : _checkedLayerByVar;
						if (on) state[_curVariable].insert(text); else state[_curVariable].remove(text);
						setStatus(QString());
					});
				QTableWidgetItem* textItem = new QTableWidgetItem(items.at(row));
				textItem->setFlags(Qt::ItemIsEnabled);
				table->setItem(row, 1, textItem);
			}
		}
		//高度随内容行数钉到 3~6 行（QSizePolicy::Fixed 下布局不再按 sizeHint 撑高）
		table->setFixedHeight(qBound(3 * kManagerRow, table->rowCount() * kManagerRow, 6 * kManagerRow));
		table->setUpdatesEnabled(true);
	}

	QStringList mPostCurveExtractPanel::checkedValues(const CheckTable& ct) const
	{
		QStringList values;
		for (int row = 0; row < ct.table->rowCount(); ++row)
		{
			QCheckBox* box = rowCheckBox(ct.table, row);
			QTableWidgetItem* text = ct.table->item(row, 1);
			if (box && text && (text->flags() & Qt::ItemIsEnabled) && box->isChecked())
				values.append(text->text());
		}
		return values;
	}

	int mPostCurveExtractPanel::checkableRowCount(const CheckTable& ct) const
	{
		int count = 0;
		for (int row = 0; row < ct.table->rowCount(); ++row)
		{
			if (rowCheckBox(ct.table, row) != nullptr)
				++count;
		}
		return count;
	}

	void mPostCurveExtractPanel::refreshPickBar()
	{
		//拾取条可用态/空态文案（云图 slot_chooseObjectType 的失活+displayData+setDefultTip
		//收口；旧 refreshPickButton/refreshPickCaption 两函数收敛于此）：无变量=退出拾取态
		//+置灰+清拾取数据（对象类型未定义不可拾取）；有变量=恢复可用+按对象类型设默认
		//文案（有数据时条内自显计数，tip 仅空态呈现）
		if (_pickController == nullptr) return;
		const bool hasVariable = _postDB && !_curVariable.isEmpty();
		if (!hasVariable)
		{
			if (_pickController->isPickActive())
				_pickController->setPickActive(false);
			_pickController->displayData(PickWidgetData{});
		}
		_pickController->widget()->setPickWidgetEnabled(hasVariable);
		QString tip;
		if (!hasVariable)
			tip = _langEn ? QString("No pickable objects") : QString::fromUtf8("暂无可拾取对象");
		else
		{
			const QString noun = _langEn
				? (_curVarElement ? QString("Elements") : QString("Nodes"))
				: (_curVarElement ? QString::fromUtf8("单元") : QString::fromUtf8("节点"));
			tip = (_langEn ? QString("Pick %1") : QString::fromUtf8("拾取%1")).arg(noun);
		}
		_pickController->setDefultTip(tip);
	}

	void mPostCurveExtractPanel::harvestPickedIds()
	{
		//拾取条数据收割为提取对象清单（绘制前一次性收割；激活期间每次拾取已由控制器
		//逐次入库，getAllData 即全集）。节点/单元 id=result_id（SelectData2ResultData 口径）
		_pickIds.clear();
		if (_pickController == nullptr) return;
		const MxFunctions::PickDataType type = _curVarElement
			? MxFunctions::PickDataType::Element : MxFunctions::PickDataType::Node;
		const auto& picked = _pickController->getAllData().pickedData;
		const auto it = picked.find(type);
		if (it != picked.end())
			for (const auto& item : it->second)
				if (item.result_id >= 0 && !_pickIds.contains(item.result_id))
					_pickIds.push_back(item.result_id);
	}

	void mPostCurveExtractPanel::rebuildTargetTree()
	{
		_targetTree->clear();
		_selectedTargetViewport.clear();
		//绘制目标=曲线模块已登记卡片的全量表（显示名同视口卡片标题"p1w1 : XY Plot"；
		//UserRole+1 存图表节点id，点选高亮沿用既有 itemClicked 处理）
		PostCurve::mCurveViewportFactory* factory = PostCurve::mCurveViewportFactory::getInstance();
		const QList<int> chartIds = factory->allCardChartIds();
		for (int chartId : chartIds)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem(_targetTree);
			item->setFlags(Qt::ItemIsEnabled);
			item->setText(0, factory->chartDisplayName(chartId));
			item->setData(0, Qt::UserRole + 1, QString::number(chartId));
		}
		if (chartIds.isEmpty())
		{
			QTreeWidgetItem* empty = new QTreeWidgetItem(_targetTree);
			empty->setFlags(Qt::ItemIsEnabled);
			empty->setText(0, _langEn ? QString("No view pages") : QString::fromUtf8("暂无视图页"));
			empty->setForeground(0, QBrush(QColor(kMuted)));
		}
	}

	int mPostCurveExtractPanel::ensureCurrentViewCurvePage()
	{
		PostCurve::mCurveDocController* controller = PostCurve::mCurveDocController::getInstance();
		PostCurve::mCurveViewportFactory* factory = PostCurve::mCurveViewportFactory::getInstance();
		const int viewPageId = factory->currentViewPageId();
		if (viewPageId < 0)
			return -1;
		int pageId = controller->curvePageIdForViewPage(viewPageId);
		if (pageId >= 0)
			return pageId;
		//新建并绑定当前视图页（NVH resolveCurrentViewPageTarget 同口径）：绑定后
		//本批图表卡片登记挂到当前视图页（openChart 按曲线页绑定解析 viewPageId），
		//页名对齐视图页名（无页名回退"曲线提取"），曲线树/视图树两页同名对应
		const QString viewName = factory->viewPageName(viewPageId);
		pageId = controller->addPage(viewName.isEmpty()
			? (_langEn ? QString("Curve Extraction") : QString::fromUtf8("曲线提取"))
			: viewName);
		controller->setPageViewPageId(pageId, viewPageId);
		return pageId;
	}

	void mPostCurveExtractPanel::setStatus(const QString& text)
	{
		_statusLabel->setText(text);
		_statusLabel->setVisible(!text.isEmpty());
	}

	PostData::mPostDataBase* mPostCurveExtractPanel::currentPostDB() const
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
		if (rebuildPostRend == nullptr || rebuildPostRend->getPostRender() == nullptr)
			return nullptr;
		const QString postname = rebuildPostRend->getPostDataName();
		if (postname.isEmpty())
			return nullptr;
		return getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
	}

	void mPostCurveExtractPanel::updateCurveExtractPanel()
	{
		_postDB = currentPostDB();
		//视口拾取数据收割/高亮/重入回显由拾取栈控制器自管（激活时连接视口信号）
		refreshFromResultData();
	}

	void mPostCurveExtractPanel::refreshFromResultData()
	{
		//目标树取自曲线模块卡片状态（与结果库无关），开面板即刷新一次
		rebuildTargetTree();
		if (!_postDB)
		{
			_comboCase->blockSignals(true);
			_comboCase->clear();
			_comboCase->blockSignals(false);
			_comboVariable->blockSignals(true);
			_comboVariable->clear();
			_comboVariable->blockSignals(false);
			_curVariable.clear();
			refreshCheckTable(_tableComponent, QStringList(), _checkedCompoByVar);
			refreshCheckTable(_tableLayer, QStringList(), _checkedLayerByVar);
			refreshPickBar();
			setStatus(QString());
			return;
		}

		//工况（保持当前选择若仍存在）
		const QString keepCase = _comboCase->currentText();
		const QStringList cases = _postDB->getAllCaseNames();
		_comboCase->blockSignals(true);
		_comboCase->clear();
		_comboCase->addItems(cases);
		if (cases.contains(keepCase))
			_comboCase->setCurrentText(keepCase);
		else if (!cases.isEmpty())
			_comboCase->setCurrentIndex(0);
		_comboCase->blockSignals(false);
		slot_chooseCase(_comboCase->currentText());
	}

	void mPostCurveExtractPanel::slot_chooseCase(const QString& text)
	{
		setStatus(QString());
		if (!_postDB || text.isEmpty())
		{
			_comboVariable->blockSignals(true);
			_comboVariable->clear();
			_comboVariable->blockSignals(false);
			_curVariable.clear();
			refreshCheckTable(_tableComponent, QStringList(), _checkedCompoByVar);
			refreshCheckTable(_tableLayer, QStringList(), _checkedLayerByVar);
			refreshPickBar();
			return;
		}

		//该工况首帧的变量表（曲线提取无帧选择，取首帧代表）
		QHash<QString, QPair<int, int>> frames;
		_postDB->getCaseFrameNamesAndIDs(text, frames);
		int frameId = -1;
		if (!frames.isEmpty())
			frameId = frames.constBegin().value().second;
		const QStringList vars = frameId > 0 ? _postDB->getVariableByFrameID(frameId) : QStringList();

		const QString keepVar = _comboVariable->currentText();
		_comboVariable->blockSignals(true);
		_comboVariable->clear();
		_comboVariable->addItems(vars);
		if (vars.contains(keepVar))
			_comboVariable->setCurrentText(keepVar);
		else if (!vars.isEmpty())
			_comboVariable->setCurrentIndex(0);
		_comboVariable->blockSignals(false);
		slot_chooseVariable(_comboVariable->currentText());
	}

	QStringList mPostCurveExtractPanel::composOfVariable(const QString& var)
	{
		QStringList compos = _postDB ? _postDB->getVariableCompos(var) : QStringList();
		if (!compos.isEmpty() || var.isEmpty() || !_postDB)
			return compos;

		//兜底：遍历库内帧信息，同名变量 _compoNames 空且 _nCompo>0 的合成索引名
		//写回（新链 convertFrameVariables 同款 C0 起；云图/查询等按名读取随之恢复）
		const QHash<int, QPointer<PostData::mPostModelData>> frames = _postDB->getAllFrameData();
		for (auto it = frames.constBegin(); it != frames.constEnd(); ++it)
		{
			PostData::mPostModelData* modelData = it.value();
			if (modelData == nullptr)
				continue;
			QList<PostData::PostVariableInfo>& vars = modelData->_frameInfo._frameVars;
			for (PostData::PostVariableInfo& vinfo : vars)
			{
				if (vinfo._varName != var || !vinfo._compoNames.isEmpty() || vinfo._nCompo <= 0)
					continue;
				for (int c = 0; c < vinfo._nCompo; ++c)
					vinfo._compoNames.append(QString("C%1").arg(c));
			}
		}
		QStringList patched = _postDB->getVariableCompos(var);
		if (!patched.isEmpty())
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,
				QString("[曲线提取] 变量 %1 无分量名记录，已按分量数合成 C0..C%2 兜底")
					.arg(var).arg(patched.size() - 1), false);
		}
		return patched;
	}

	void mPostCurveExtractPanel::slot_chooseVariable(const QString& text)
	{
		setStatus(QString());
		//换变量=换提取上下文（对象类型/分量表都变），已拾对象失效：退出拾取态并清
		//拾取条数据（云图 slot_chooseObjectType 同款；类型可能翻转节点↔单元，旧数据不带入）
		if (_curVariable != text)
		{
			if (_pickController != nullptr)
			{
				if (_pickController->isPickActive())
					_pickController->setPickActive(false);
				_pickController->displayData(PickWidgetData{});
			}
			_pickIds.clear();
		}
		_curVariable = text;
		QStringList components;
		QStringList layers;
		if (_postDB && !text.isEmpty())
		{
			components = composOfVariable(text);
			layers = _postDB->getVariableLayer(text);
			layers.sort();	//云图面板同口径（lnames.sort）
			_curVarElement = (_postDB->getVariableType(text) == 1);
		}
		refreshCheckTable(_tableComponent, components, _checkedCompoByVar);
		refreshCheckTable(_tableLayer, layers, _checkedLayerByVar);
		refreshPickBar();
	}

	bool mPostCurveExtractPanel::extractCurvesByFrame(const QString& var,
		const QList<MIOFile::CurveKey>& keys,
		MIOFile::mIONativeFile* nativeReader, MIOFile::mIOVtkFile* vtkReader,
		QStringList& cnames, QHash<QString, QPair<QVector<double>, QVector<double>>>& curves)
	{
		//帧序遍历（getAllFrameData 全帧口径同 readTimeVarCurve；X=帧时间）
		const QHash<int, QPointer<PostData::mPostModelData>> frames = _postDB->getAllFrameData();
		QList<int> frameNos = frames.keys();
		std::sort(frameNos.begin(), frameNos.end());
		if (frameNos.isEmpty())
			return false;

		//单帧单 key 读取：readVarDatas（层命中取层切片/空跨层平均）→ getComponeValues
		//→ 实体/壳两层逐 id 平均摊平（查询面板 flattenCompones 同口径）
		auto flatValuesOf = [&](int frameNo, const MIOFile::CurveKey& key)
		{
			QHash<int, double> flat;
			const bool ok = nativeReader != nullptr
				? nativeReader->readVarDatas(frameNo, var, key.component, QString(), key.layer)
				: vtkReader->readVarDatas(frameNo, var, key.component, QString(), key.layer);
			if (!ok)
				return flat;
			PostData::Compones& comp = _postDB->getComponeValues();
			auto absorb = [&flat](const auto& twoLevel)
			{
				for (auto it = twoLevel.cbegin(); it != twoLevel.cend(); ++it)
				{
					const auto vals = it.value().values();
					if (vals.isEmpty())
						continue;
					double sum = 0.0;
					for (double v : vals)
						sum += v;
					flat.insert(it.key(), sum / vals.size());
				}
			};
			if (comp.isD)
			{
				absorb(comp.dRValue);
				absorb(comp.dQValue);
			}
			else
			{
				absorb(comp.fRValue);
				absorb(comp.fQValue);
			}
			return flat;
		};

		//帧×key 循环收集（cname 同构命名 "Time-{var}_{分量[-层]}-ID{id}" 与 mxdb
		//分支一致；某帧某 id 缺值不占位，各曲线点数独立）
		for (int frameNo : frameNos)
		{
			PostData::mPostModelData* modelData = frames.value(frameNo);
			if (modelData == nullptr)
				continue;
			const double frameTime = modelData->_frameInfo._frameTime;
			for (const MIOFile::CurveKey& key : keys)
			{
				const QHash<int, double> flat = flatValuesOf(frameNo, key);
				if (flat.isEmpty())
					continue;
				for (int id : _pickIds)
				{
					if (!flat.contains(id))
						continue;
					const QString cname = QString::fromUtf8("Time-%1_%2-ID%3")
						.arg(var).arg(key.toString()).arg(id);
					QPair<QVector<double>, QVector<double>>& c = curves[cname];
					c.first.append(frameTime);
					c.second.append(flat.value(id));
				}
			}
		}

		//生成序（id 外层 key 内层，同 mxdb 分支）
		for (int id : _pickIds)
			for (const MIOFile::CurveKey& key : keys)
			{
				const QString cname = QString::fromUtf8("Time-%1_%2-ID%3")
					.arg(var).arg(key.toString()).arg(id);
				if (curves.contains(cname))
					cnames.append(cname);
			}
		return !curves.isEmpty();
	}

	void mPostCurveExtractPanel::slot_drawClicked()
	{
		//校验序与文案按原型 draw click 分支。判定依据取数据侧（变量是否有分量/
		//层）而非表侧行数：面板打开早于导入收尾时表可能仍空态；此况现场补表
		//供勾选；变量确无分量（标量）放行——空分量占位生成一条曲线（旧面板口径）
		const QStringList components = checkedValues(_tableComponent);
		const QStringList layers = checkedValues(_tableLayer);
		const QStringList varCompos = _postDB ? composOfVariable(_curVariable) : QStringList();
		const QStringList varLayers = _postDB ? _postDB->getVariableLayer(_curVariable) : QStringList();
		if (checkableRowCount(_tableComponent) == 0 && !varCompos.isEmpty())
			refreshCheckTable(_tableComponent, varCompos, _checkedCompoByVar);
		if (checkableRowCount(_tableLayer) == 0 && !varLayers.isEmpty())
			refreshCheckTable(_tableLayer, varLayers, _checkedLayerByVar);
		if (!varCompos.isEmpty() && components.isEmpty())
		{
			setStatus(_langEn ? QString("Select at least one component.")
				: QString::fromUtf8("请选择提取分量。"));
			return;
		}
		if (!varLayers.isEmpty() && layers.isEmpty())
		{
			setStatus(_langEn ? QString("Select at least one layer.")
				: QString::fromUtf8("请选择提取层。"));
			return;
		}
		const PostCurve::CurveBatchLayout layout =
			static_cast<PostCurve::CurveBatchLayout>(_comboLayout->currentIndex());
		if (layout == PostCurve::CurveBatchLayout::ExistingChart && _selectedTargetViewport.isEmpty())
		{
			setStatus(_langEn ? QString("Select an existing curve plot viewport.")
				: QString::fromUtf8("请选择已有曲线图视口。"));
			return;
		}
		if (!_postDB)
		{
			setStatus(_langEn ? QString("No result data in the current post viewport.")
				: QString::fromUtf8("当前后处理视口无结果数据。"));
			return;
		}
		harvestPickedIds();
		if (_pickIds.isEmpty())
		{
			setStatus(_langEn ? QString("Pick nodes/elements first.")
				: QString::fromUtf8("请先拾取节点/单元。"));
			return;
		}

		//提取时间历程数据（全部帧，X=帧时间；无复数选项，cplxForms 传空；
		//标量变量补空分量占位）。读取分支照查询面板四分支协议（native→vtk→
		//mxdb 旧→mxdb 新）：native/vtk 数据无 mxdb 变量块（_varByte），readTimeVarCurve
		//读不出，须逐帧走常驻 reader 的 readVarDatas
		QStringList compos = components;
		if (compos.isEmpty())
			compos << QString();
		const QString var = _curVariable;
		const QString pname = _postDB->getFile();

		MIOFile::CurveKey curveKey;
		const QList<MIOFile::CurveKey> keys = curveKey.generateCurveKeys(compos, layers, QStringList());

		//曲线名→(X 序列, Y 序列)（native/vtk 分支按曲线独立收集，帧缺失不占位）
		QStringList cnames;
		QHash<QString, QPair<QVector<double>, QVector<double>>> curves;

		auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
		MIOFile::mIONativeFile* nativeReader = cloudPanel ? cloudPanel->nativeReaderFor(pname) : nullptr;
		MIOFile::mIOVtkFile* vtkReader = (nativeReader == nullptr && cloudPanel)
			? cloudPanel->vtkReaderFor(pname) : nullptr;
		if (nativeReader != nullptr || vtkReader != nullptr)
		{
			if (!extractCurvesByFrame(var, keys, nativeReader, vtkReader, cnames, curves))
			{
				setStatus(_langEn ? QString("Curve extraction failed.")
					: QString::fromUtf8("曲线生成失败。"));
				return;
			}
		}
		else
		{
			//mxdb 链路（旧"时间-变量曲线"双分流，一次读全部帧）
			QStringList names;
			QVector<double> xvalues;
			QHash<QString, QVector<double>> yvaluesD;
			QHash<QString, QVector<float>> yvaluesF;
			if (_postDB->getDataFlag())
			{
				MIOFile::mIMxdbFile1 reader;
				reader.readTimeVarCurve(pname, var, keys, _pickIds, names, xvalues, yvaluesD);
			}
			else
			{
				MIOFile::mMxdbThread reader(pname, _postDB, this);
				reader.readTimeVarCurve(pname, var, keys, _pickIds, names, xvalues, yvaluesD, yvaluesF);
			}

			//float→double 归一（getCurveDatas 口径）
			QHash<QString, QVector<double>> yval;
			if (!yvaluesD.isEmpty())
			{
				yval = yvaluesD;
			}
			else
			{
				for (auto it = yvaluesF.constBegin(); it != yvaluesF.constEnd(); ++it)
				{
					QVector<double> dv;
					dv.reserve(it.value().size());
					for (float v : it.value())
						dv.append(static_cast<double>(v));
					yval.insert(it.key(), dv);
				}
			}

			//mxdb 两条实现 cname 同格式，按同构构造映射（X 为全帧共享序列）
			for (int id : _pickIds)
			{
				for (const MIOFile::CurveKey& key : keys)
				{
					const QString cname = QString::fromUtf8("Time-%1_%2-ID%3")
						.arg(var).arg(key.toString()).arg(id);
					if (!yval.contains(cname))
						continue;
					cnames.append(cname);
					curves.insert(cname, qMakePair(xvalues, yval.value(cname)));
				}
			}
		}

		//组装（两分支共用；分组键=分量、对象=节点/单元号、来源五元组）
		QList<PostCurve::CurveBatchItem> items;
		const QString objWord = _curVarElement ? QString::fromUtf8("单元") : QString::fromUtf8("节点");
		for (int id : _pickIds)
		{
			for (const MIOFile::CurveKey& key : keys)
			{
				const QString cname = QString::fromUtf8("Time-%1_%2-ID%3")
					.arg(var).arg(key.toString()).arg(id);
				if (!curves.contains(cname))
					continue;
				PostCurve::CurveBatchItem item;
				item.name = cname;
				item.xData = curves.value(cname).first;
				item.yData = curves.value(cname).second;
				item.group = key.component;
				item.sourceFile = pname;
				item.subcase = _comboCase->currentText();
				item.schema = var;
				item.object = objWord + QString(" %1").arg(id);
				item.component = key.toString();
				items.append(item);
			}
		}

		if (items.isEmpty())
		{
			setStatus(_langEn ? QString("Curve extraction failed.")
				: QString::fromUtf8("曲线生成失败。"));
			return;
		}

		//落图与呈现。布局 0/1/2（每个请求/每个分量/每条曲线一张图）：曲线落到
		//当前视图页绑定的曲线页（无则新建并绑定，ensureCurrentViewCurvePage——
		//绑定使卡片登记挂到当前视图页），绘制后经工厂 registerChartsOnPage 静默
		//登记（用户终态裁定：不自动显示——不占空格、不浮出、不动页布局，呈现集
		//冻结不含新卡），显示时机完全由用户自行调整页面布局决定；"新视图页"
		//强制新建页、"现有曲线图"并入所选图表，两者保持静默落图（选项显式语义）
		const int chartTarget = (layout == PostCurve::CurveBatchLayout::ExistingChart)
			? _selectedTargetViewport.toInt() : -1;
		const bool presentOnCurrentPage = (layout != PostCurve::CurveBatchLayout::NewPage
			&& layout != PostCurve::CurveBatchLayout::ExistingChart);
		PostCurve::mCurveDocController* controller = PostCurve::mCurveDocController::getInstance();
		QList<int> createdCharts;
		const int pageId = controller->addCurveBatchToPage(items, layout,
			_langEn ? QString("Curve Extraction") : QString::fromUtf8("曲线提取"),
			chartTarget, QStringLiteral("Time"), var,
			presentOnCurrentPage ? ensureCurrentViewCurvePage() : -1,
			presentOnCurrentPage ? &createdCharts : nullptr);
		if (pageId < 0)
		{
			setStatus(_langEn ? QString("Curve plotting failed.")
				: QString::fromUtf8("曲线绘制失败。"));
			return;
		}
		if (presentOnCurrentPage)
		{
			//仅登记落点页上的图表（超页上限自动分页的溢出页保持静默，其卡片
			//挂在各自新建视图页，不并入当前页）
			QList<int> pageCharts;
			for (int chartId : createdCharts)
			{
				if (controller->document()->parentNodeId(chartId) == pageId)
					pageCharts.append(chartId);
			}
			PostCurve::mCurveViewportFactory::getInstance()->registerChartsOnPage(pageId, pageCharts);
			controller->reportInfo(QString(_langEn
				? "[Curve] %1 curve(s) added to the current view page."
				: QString::fromUtf8("[曲线] 曲线提取已绘制%1条曲线（已加入当前页）。")).arg(items.size()));
			setStatus(_langEn ? QString("Plotted %1 curve(s) to the current view page; adjust the page layout to show them.")
				: QString::fromUtf8("已绘制%1条曲线，已加入当前页，调整页面布局后显示。").arg(items.size()));
			return;
		}
		const QString pageName = controller->document()->nodeProperty(pageId,
			PostCurve::CurveProp::kName).toString();
		controller->reportInfo(QString(_langEn ? "[Curve] %1 curve(s) plotted from curve extraction."
			: QString::fromUtf8("[曲线] 曲线提取已绘制%1条曲线。")).arg(items.size()));
		setStatus((_langEn ? QString("Plotted %1 curve(s) to page \"%2\".")
			: QString::fromUtf8("已绘制%1条曲线，落入页“%2”。")).arg(items.size()).arg(pageName));
	}

	void mPostCurveExtractPanel::applyLanguageTexts()
	{
		_mainFold->setTitle(_langEn ? QString("Curve Extraction") : QString::fromUtf8("曲线提取"));
		//行标签（addRow 建的 PanelLabel 按布局序重设文本）
		const QList<PanelLabel*> labels = _mainFold->findChildren<PanelLabel*>();
		if (labels.size() >= 3)
		{
			labels.at(0)->setText(_langEn ? QString("Case") : QString::fromUtf8("工况"));
			labels.at(1)->setText(_langEn ? QString("Variable") : QString::fromUtf8("变量"));
			labels.at(2)->setText(_langEn ? QString("Objects") : QString::fromUtf8("对象选择"));
		}

		_tableComponent.caption->setText(_langEn ? QString("Components") : QString::fromUtf8("分量"));
		_tableLayer.caption->setText(_langEn ? QString("Layers") : QString::fromUtf8("层"));
		if (_labelDraw)
			_labelDraw->setText(_langEn ? QString("Plot") : QString::fromUtf8("绘制选项"));
		for (CheckTable* ct : { &_tableComponent, &_tableLayer })
		{
			ct->btnAll->setToolTip(_langEn ? QString("Select all") : QString::fromUtf8("全选"));
			ct->btnNone->setToolTip(_langEn ? QString("Deselect all") : QString::fromUtf8("取消"));
			ct->btnInvert->setToolTip(_langEn ? QString("Invert selection") : QString::fromUtf8("反选"));
		}

		//绘制选项（保持当前索引；默认"每条曲线一张图"）
		const int keepLayout = _comboLayout->currentIndex();
		_comboLayout->blockSignals(true);
		_comboLayout->clear();
		const struct { const char* cn; const char* en; } layoutDefs[] =
		{
			{ "每个请求一张图", "One plot per request" },
			{ "每个分量一张图", "One plot per component" },
			{ "每条曲线一张图", "One curve per plot" },
			{ "新视图页", "New view page" },
			{ "现有曲线图", "Existing curve plot" }
		};
		for (const auto& def : layoutDefs)
		{
			_comboLayout->addItem(QString::fromUtf8(def.cn));
			_comboLayout->setItemData(_comboLayout->count() - 1, QString::fromLatin1(def.en), Qt::ToolTipRole);
		}
		_comboLayout->setCurrentIndex(keepLayout < 0 ? LayoutOnePerPlot : keepLayout);
		_comboLayout->blockSignals(false);

		_btnDraw->setText(_langEn ? QString("Draw") : QString::fromUtf8("绘制"));

		//表格/拾取/目标树按当前上下文重刷（空态与对象类型文案随语言）
		QStringList components;
		QStringList layers;
		if (_postDB && !_curVariable.isEmpty())
		{
			components = composOfVariable(_curVariable);
			layers = _postDB->getVariableLayer(_curVariable);
			layers.sort();
		}
		refreshCheckTable(_tableComponent, components, _checkedCompoByVar);
		refreshCheckTable(_tableLayer, layers, _checkedLayerByVar);
		refreshPickBar();
		rebuildTargetTree();
		setStatus(QString());
	}

	void mPostCurveExtractPanel::slot_changeLanguage(int language)
	{
		_langEn = (language == 1);
		applyLanguageTexts();
	}

	void mPostCurveExtractPanel::hideEvent(QHideEvent* event)
	{
		//面板退出（宿主 setMainWidget 切换/复位时触发）：拾取条收口——失活（清视口
		//过滤器/高亮/工具栏白名单由 pickActiveChanged 链收口）并置隐藏态拒绝再激活；
		//详情弹窗为 Qt::Popup 随宿主隐藏自动关闭（云图/爆炸/阵列同款）
		if (_pickController != nullptr)
			_pickController->setPickWidgetVisible(false);
		mPanelBase::hideEvent(event);
	}

	void mPostCurveExtractPanel::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);
		//隐藏期 setPickWidgetVisible(false) 的对称恢复（控件可再激活；已拾取数据保留
		//在控件数据模型，条体显示随数据自动恢复——云图/爆炸/阵列同款）
		if (_pickController != nullptr)
			_pickController->setPickWidgetVisible(true);
	}
}
