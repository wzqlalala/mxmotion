#include "mPostQueryPanel.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mPostGeneralPickUp.h"
#include "mPostModelData.h"
#include "mPostResultDataStruct.h"
#include "mPostGUIData.h"
#include "mPostInfoHub.h"
#include "mPostValueProvider.h"
#include "mPost3DResult.h"
#include "PickCommonFunction.h"
#include "mElementFunction.h"
#include "mIMxdbFile1.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostRend.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace
{
	//令牌（--std-* 标准档；出处 V1.3 原型 CSS 变量定义块，同 mPostCurveExtract）
	constexpr int kControl = 26;		//--std-control 控件外高
	constexpr int kHeader = 28;			//--std-header 窗口头高（弹窗/停靠窗页签行）
	constexpr int kManagerRow = 26;		//--std-manager-row 管理表格行高
	constexpr int kChoice = 13;			//--std-choice 复选尺寸
	constexpr int kChoiceGap = 6;		//--std-choice-gap 复选行内距
	constexpr int kGroupInset = 7;		//--std-group-inset 文本列左内边距
	constexpr int kRadius = 4;			//--std-radius 控件圆角（V1.3：2→4）
	constexpr int kGap = 6;				//--std-gap

	const char* kLineBorder = "#DCE2E9";	//--std-border
	const char* kHeadBg = "#E9EDF3";		//--std-header-color
	const char* kTextColor = "#303313";		//--std-text / --std-label（V1.3 统一主文本色）
	const char* kLabelColor = "#5C6674";	//变量小标题标签色
	const char* kMuted = "#788391";			//--std-muted
	const char* kHoverBg = "#F0F7FF";		//--std-hover
	const char* kSelectedBg = "#E8F3FF";	//--std-selected
	const char* kPrimary = "#1890FF";		//--std-primary / --std-focus
	const char* kDisabledFg = "#A6AFBB";	//--std-disabled
	const char* kPanelBg = "#F5F7FA";		//--std-panel-color（禁用底）

	//停靠窗总高：页签行 28 + 内边距 2*6 + 表头 26 + 6 数据行 156 + 边框 4
	//（原型 positionResultsDock：header + control + 6*managerRow + 2*gap + 4*border）
	constexpr int kDockHeight = kHeader + kControl + 6 * kManagerRow + 2 * kGap + 4;

	//查询值弹窗宽（原型 placeAnchoredPopup：panel*3/4=225）
	constexpr int kValuesPopupWidth = 225;

	//对象类型（面板/停靠窗/推送接口共用）
	enum QueryObjectType
	{
		QueryNode = 0,
		QueryElement = 1,
		QueryPart = 2
	};

	//类型 → 拾取过滤器（同集合面板口径；拾取条激活收口用）
	mxr::PickFilter queryPickFilter(int objectType)
	{
		static const mxr::PickFilter kFilters[3] =
		{
			mxr::PickFilter::PickMeshNode, mxr::PickFilter::PickMeshElement, mxr::PickFilter::PickMeshPart
		};
		return kFilters[qBound(0, objectType, 2)];
	}

	//查询字段目录（原型 fields 常量；key 为引擎推送与表格列的数据主键）
	struct FieldDef
	{
		const char* key;
		const char* cn;
		const char* en;
	};

	const FieldDef kFieldsNode[] =
	{
		{ "nodeId", "节点编号", "Node ID" },
		{ "referenceSystem", "参考坐标系", "Reference CS" },
		{ "analysisSystem", "分析坐标系", "Analysis CS" },
		{ "nodeCoordinates", "节点坐标", "Coordinates" },
		{ "materialName", "材料", "Material" },
		{ "propertyName", "属性", "Property" },
		{ "componentName", "部件", "Part" },
		{ "loadCase", "工况", "Load Case" },
		{ "simulationStep", "仿真步", "Simulation Step" },
		{ "contourValue", "云图值", "Contour Value" },
		{ "vectorValue", "向量值", "Vector Value" }
	};
	const FieldDef kFieldsElement[] =
	{
		{ "elementId", "单元编号", "Element ID" },
		{ "elementType", "单元类型", "Element Type" },
		{ "connectivity", "节点连接关系", "Connectivity" },
		{ "centroid", "单元形心", "Centroid" },
		{ "normal", "单元法向", "Normal" },
		{ "materialName", "材料", "Material" },
		{ "propertyName", "属性", "Property" },
		{ "componentName", "部件", "Part" },
		{ "loadCase", "工况", "Load Case" },
		{ "simulationStep", "仿真步", "Simulation Step" },
		{ "contourValue", "云图值", "Contour Value" },
		{ "vectorValue", "向量值", "Vector Value" }
	};
	const FieldDef kFieldsPart[] =
	{
		{ "componentId", "部件编号", "Part ID" },
		{ "componentName", "部件名称", "Part Name" },
		{ "materialName", "材料", "Material" },
		{ "propertyName", "属性", "Property" },
		{ "loadCase", "工况", "Load Case" },
		{ "simulationStep", "仿真步", "Simulation Step" },
		{ "contourMax", "云图最大值", "Contour Max" },
		{ "contourMaxId", "云图最大值对象编号", "Contour Max ID" },
		{ "contourMin", "云图最小值", "Contour Min" },
		{ "contourMinId", "云图最小值对象编号", "Contour Min ID" },
		{ "vectorMax", "向量最大值", "Vector Max" },
		{ "vectorMaxId", "向量最大值对象编号", "Vector Max ID" },
		{ "vectorMin", "向量最小值", "Vector Min" },
		{ "vectorMinId", "向量最小值对象编号", "Vector Min ID" }
	};

	//弹窗分组（原型 fieldGroups：节点信息/单元信息/部件信息/模型信息/结果信息）
	struct FieldGroup
	{
		const char* cn;
		const char* en;
		std::initializer_list<const char*> keys;
	};

	const FieldGroup kGroupsNode[] =
	{
		{ "节点信息", "Node Info", { "nodeId", "referenceSystem", "analysisSystem", "nodeCoordinates" } },
		{ "模型信息", "Model Info", { "materialName", "propertyName", "componentName" } },
		{ "结果信息", "Result Info", { "loadCase", "simulationStep", "contourValue", "vectorValue" } }
	};
	const FieldGroup kGroupsElement[] =
	{
		{ "单元信息", "Element Info", { "elementId", "elementType", "connectivity", "centroid", "normal" } },
		{ "模型信息", "Model Info", { "materialName", "propertyName", "componentName" } },
		{ "结果信息", "Result Info", { "loadCase", "simulationStep", "contourValue", "vectorValue" } }
	};
	const FieldGroup kGroupsPart[] =
	{
		{ "部件信息", "Part Info", { "componentId", "componentName" } },
		{ "模型信息", "Model Info", { "materialName", "propertyName" } },
		{ "结果信息", "Result Info", { "loadCase", "simulationStep",
		  "contourMax", "contourMaxId", "contourMin", "contourMinId",
		  "vectorMax", "vectorMaxId", "vectorMin", "vectorMinId" } }
	};

	//默认勾选（原型 selected 三 Set）
	const std::initializer_list<const char*> kDefaultSelected[3] =
	{
		{ "nodeId", "nodeCoordinates" },
		{ "elementId", "centroid" },
		{ "componentId", "componentName" }
	};

	const FieldDef* fieldCatalog(int objectType)
	{
		switch (objectType)
		{
		case QueryNode: return kFieldsNode;
		case QueryElement: return kFieldsElement;
		default: return kFieldsPart;
		}
	}

	int fieldCatalogSize(int objectType)
	{
		switch (objectType)
		{
		case QueryNode: return sizeof(kFieldsNode) / sizeof(FieldDef);
		case QueryElement: return sizeof(kFieldsElement) / sizeof(FieldDef);
		default: return sizeof(kFieldsPart) / sizeof(FieldDef);
		}
	}

	const FieldGroup* fieldGroupList(int objectType)
	{
		switch (objectType)
		{
		case QueryNode: return kGroupsNode;
		case QueryElement: return kGroupsElement;
		default: return kGroupsPart;
		}
	}

	int fieldGroupCount(int objectType)
	{
		return 3;
	}

	//目录内的字段定义（找不到返回 nullptr）
	const FieldDef* findField(int objectType, const QString& key)
	{
		const FieldDef* catalog = fieldCatalog(objectType);
		for (int i = 0; i < fieldCatalogSize(objectType); ++i)
			if (key == QLatin1String(catalog[i].key))
				return &catalog[i];
		return nullptr;
	}

//16×16 关闭叉（描边 1；弹窗头/页签共用，原型 currentColor≈主文本色）
QIcon closeIcon(const QColor& color)
{
	//DPR 2.0 下 32×32 像素即 16×16 逻辑画布，painter 天然按逻辑坐标绘制；
	//不能再 p.scale(2)——双重放大把两条对角线推出画布，只剩中心到右下角一段
	//短斜线（页签/弹窗关闭钮曾因此显示成"\"单斜线）
	QPixmap pm(32, 32);
	pm.setDevicePixelRatio(2.0);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	QPen pen(color, 1.0);
	p.setPen(pen);
	p.drawLine(QPointF(4.5, 4.5), QPointF(11.5, 11.5));
	p.drawLine(QPointF(11.5, 4.5), QPointF(4.5, 11.5));
	return QIcon(pm);
}

	//表格网格线委托：单元格右线+行底线（原型 th/td inset -1px 0 0 border 与行底线）
	class ResultGridDelegate : public QStyledItemDelegate
	{
	public:
		using QStyledItemDelegate::QStyledItemDelegate;
		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override
		{
			QStyledItemDelegate::paint(painter, option, index);
			painter->save();
			QPen pen(QColor(kLineBorder), 1.0);
			painter->setPen(pen);
			const QRect r = option.rect;
			painter->drawLine(QPointF(r.right() - 0.5, r.top() + 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			painter->drawLine(QPointF(r.left() + 0.5, r.bottom() - 0.5), QPointF(r.right() - 0.5, r.bottom() - 0.5));
			painter->restore();
		}
	};
}

namespace PostWindow
{
	/*
	* 查询结果停靠窗（原型 #queryResultsDock）：浮在视口底部的结果表格窗。
	* 结构：页签行（节点/单元/部件数据，仅激活页签带关闭钮）+ 三页内容
	* （左侧 清除数据/导出数据 纵向操作列 + 右侧动态列表格）。
	* 数据由面板经 setColumns/setRows 推送；页签可单独关闭（available=false），
	* 全部关闭即整窗收起；面板入口按钮再次点击重新展开。
	* 注：cpp 内定义不挂 Q_OBJECT（编译门单件编译不跑 moc），操作回调走 std::function。
	*/
	class mPostQueryResultsDock : public QFrame
	{
	public:
		explicit mPostQueryResultsDock(QWidget* parent) : QFrame(parent)
		{
			setObjectName("queryResultsDock");
			setFrameShape(QFrame::NoFrame);
			setStyleSheet(QString(
				"QFrame#queryResultsDock{background:#FFFFFF;border:1px solid %1;}").arg(kLineBorder));

			QVBoxLayout* layout = new QVBoxLayout(this);
			layout->setContentsMargins(0, 0, 0, 0);
			layout->setSpacing(0);

			/******** 页签行（--std-header 28；头部底色+底边线） ********/
			QFrame* header = new QFrame(this);
			header->setFixedHeight(kHeader);
			header->setStyleSheet(QString(
				"QFrame{background:%1;border:0;border-bottom:1px solid %2;}")
				.arg(kHeadBg, kLineBorder));
			QHBoxLayout* headerLayout = new QHBoxLayout(header);
			headerLayout->setContentsMargins(0, 0, 0, 0);
			headerLayout->setSpacing(0);
			for (int i = 0; i < 3; ++i)
			{
				QWidget* item = new QWidget(header);
				QHBoxLayout* itemLayout = new QHBoxLayout(item);
				itemLayout->setContentsMargins(8, 0, 0, 0);
				itemLayout->setSpacing(0);
				QPushButton* tab = new QPushButton(item);
				tab->setCursor(Qt::PointingHandCursor);
				tab->setFlat(true);
				tab->setStyleSheet(
					"QPushButton{background:transparent;border:0;color:#303313;font-size:12px;padding:4px 6px;}"
					"QPushButton:hover{background:#F0F7FF;}");
				QToolButton* dismiss = new QToolButton(item);
				dismiss->setIcon(closeIcon(QColor(kTextColor)));
				dismiss->setFixedSize(24, 24);
				dismiss->setCursor(Qt::PointingHandCursor);
				dismiss->setStyleSheet(
					"QToolButton{background:transparent;border:0;}"
					"QToolButton:hover{background:#F0F7FF;}");
				itemLayout->addWidget(tab);
				itemLayout->addWidget(dismiss);
				headerLayout->addWidget(item, 0, Qt::AlignLeft);

				_surfaces[i].objectType = i;
				_surfaces[i].item = item;
				_surfaces[i].tab = tab;
				_surfaces[i].dismiss = dismiss;

				connect(tab, &QPushButton::clicked, this, [this, i]() { openResults(i); });
				connect(dismiss, &QToolButton::clicked, this, [this, i]() { closeTab(i); });
			}
			headerLayout->addStretch();
			layout->addWidget(header);

			/******** 内容区：三页（操作列+表格） ********/
			_stack = new QStackedWidget(this);
			_stack->setContentsMargins(kGap, kGap, kGap, kGap);
			layout->addWidget(_stack, 1);
			for (int i = 0; i < 3; ++i)
				buildPane(i);

			//Escape 收起整窗（原型 keydown：焦点在停靠窗内时关闭）
			QAction* escapeAction = new QAction(this);
			escapeAction->setShortcut(QKeySequence(Qt::Key_Escape));
			escapeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
			connect(escapeAction, &QAction::triggered, this, &mPostQueryResultsDock::closeResults);
			addAction(escapeAction);

			refreshTabs();
		}

		/** 列定义推送（keys 数据主键序=列序；captions 表头文案；空=未选查询值空态） */
		void setColumns(int objectType, const QStringList& keys, const QStringList& captions)
		{
			Surface& surface = _surfaces[objectType];
			surface.keys = keys;
			surface.captions = captions;
			renderTable(objectType);
		}

		/** 行数据推送（每条为 key→文本；空列表=暂无数据） */
		void setRows(int objectType, const QList<QHash<QString, QString>>& rows)
		{
			_surfaces[objectType].rows = rows;
			renderTable(objectType);
		}

		int rowCount(int objectType) const { return _surfaces[objectType].rows.size(); }
		int activeTab() const { return _active; }

		/** 展开停靠窗并激活对应页签（原型 openResults） */
		void openResults(int objectType)
		{
			_surfaces[objectType].available = true;
			_active = objectType;
			if (!isVisible())
				show();
			_stack->setCurrentIndex(objectType);
			refreshTabs();
			raise();
		}

		/** 关闭单页签（原型 closeResultTab：跳下一个可用页签，全关则收窗） */
		void closeTab(int objectType)
		{
			if (!_surfaces[objectType].available || _active != objectType)
				return;
			_surfaces[objectType].available = false;
			int next = -1;
			for (int i = objectType + 1; i < 3; ++i)
				if (_surfaces[i].available) { next = i; break; }
			if (next < 0)
				for (int i = objectType - 1; i >= 0; --i)
					if (_surfaces[i].available) { next = i; break; }
			if (next >= 0)
			{
				_active = next;
				_stack->setCurrentIndex(next);
				refreshTabs();
			}
			else
				closeResults();
		}

		/** 收起整窗（页签可用态保留，入口可再展开） */
		void closeResults()
		{
			hide();
			refreshTabs();
		}

		/** 页签可用态复位（默认三页全开，原型 resultSurfaces available:true） */
		void resetTabs()
		{
			for (auto& surface : _surfaces)
				surface.available = true;
			refreshTabs();
		}

		//操作回调（面板注入；替代 cpp 内无法 moc 的信号）
		std::function<void(int objectType)> onClearRequested;
		std::function<void(int objectType)> onExportRequested;

	private:
		struct Surface
		{
			int objectType = 0;
			QWidget* item = nullptr;        //页签项（tab+关闭钮）
			QPushButton* tab = nullptr;
			QToolButton* dismiss = nullptr;
			QFrame* pane = nullptr;
			QTableWidget* table = nullptr;
			QPushButton* clearButton = nullptr;
			QPushButton* exportButton = nullptr;
			QStringList keys;
			QStringList captions;
			QList<QHash<QString, QString>> rows;
			bool available = true;
		};

		void buildPane(int objectType)
		{
			Surface& surface = _surfaces[objectType];
			surface.pane = new QFrame(_stack);
			surface.pane->setFrameShape(QFrame::NoFrame);
			QHBoxLayout* paneLayout = new QHBoxLayout(surface.pane);
			paneLayout->setContentsMargins(0, 0, 0, 0);
			paneLayout->setSpacing(kGap);

			//操作列（左，纵向；管理钮 26 高：清除数据/导出数据，4 字宽 64）
			QWidget* actions = new QWidget(surface.pane);
			QVBoxLayout* actionsLayout = new QVBoxLayout(actions);
			actionsLayout->setContentsMargins(0, 0, 0, 0);
			actionsLayout->setSpacing(kGap);
			const char* actionQss =
				"QPushButton{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:4px;color:#303313;font-size:12px;}"
				"QPushButton:hover{background:#F0F7FF;}"
				"QPushButton:disabled{color:#A6AFBB;}";
			surface.clearButton = new QPushButton(actions);
			surface.clearButton->setFixedSize(64, kControl);
			surface.clearButton->setCursor(Qt::PointingHandCursor);
			surface.clearButton->setStyleSheet(actionQss);
			surface.exportButton = new QPushButton(actions);
			surface.exportButton->setFixedSize(64, kControl);
			surface.exportButton->setCursor(Qt::PointingHandCursor);
			surface.exportButton->setStyleSheet(actionQss);
			actionsLayout->addWidget(surface.clearButton);
			actionsLayout->addWidget(surface.exportButton);
			actionsLayout->addStretch();
			paneLayout->addWidget(actions, 0, Qt::AlignTop);

			//结果表（表头 26 头部底色；行 26；列宽随内容+末列拉伸；空态跨列行）
			surface.table = new QTableWidget(surface.pane);
			surface.table->setFocusPolicy(Qt::ClickFocus);
			surface.table->setSelectionMode(QAbstractItemView::NoSelection);
			surface.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
			surface.table->setWordWrap(false);
			surface.table->setShowGrid(false);
			surface.table->verticalHeader()->setVisible(false);
			surface.table->verticalHeader()->setDefaultSectionSize(kManagerRow);
			QHeaderView* header = surface.table->horizontalHeader();
			header->setFixedHeight(kControl);
			header->setSectionResizeMode(QHeaderView::ResizeToContents);
			header->setStretchLastSection(true);
			header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			surface.table->setItemDelegate(new ResultGridDelegate(surface.table));
			surface.table->setStyleSheet(QString(
				"QTableWidget{background:#FFFFFF;border:1px solid %1;font-size:12px;color:%2;}"
				"QTableWidget::item{padding:0 7px;}"
				"QHeaderView::section{background:%3;color:%2;border:0;"
				"border-right:1px solid %1;border-bottom:1px solid %1;padding:0 7px;}"
				"QTableCornerButton::section{background:%3;border:0;border-bottom:1px solid %1;}")
				.arg(kLineBorder, kTextColor, kHeadBg));
			paneLayout->addWidget(surface.table, 1);

			connect(surface.clearButton, &QPushButton::clicked, this, [this, objectType]()
				{ if (onClearRequested) onClearRequested(objectType); });
			connect(surface.exportButton, &QPushButton::clicked, this, [this, objectType]()
				{ if (onExportRequested) onExportRequested(objectType); });

			_stack->addWidget(surface.pane);
			renderTable(objectType);
		}

		//按列定义+行数据重渲染表格（含空态与操作钮可用态）
		void renderTable(int objectType)
		{
			Surface& surface = _surfaces[objectType];
			QTableWidget* table = surface.table;
			table->setUpdatesEnabled(false);
			table->clearContents();
			table->setSpan(0, 0, 1, 1);	//清旧行跨列
			int columns = surface.keys.size();
			QStringList captions = surface.captions;
			if (columns == 0)
			{
				//无列兜底：单列"查询值"（原型 columns.length?columns:[['empty','查询值']]）
				columns = 1;
				captions = QStringList{ _langEn ? QString("Query Values") : QString::fromUtf8("查询值") };
			}
			table->setColumnCount(columns);
			table->setHorizontalHeaderLabels(captions);

			if (!surface.rows.isEmpty() && !surface.keys.isEmpty())
			{
				table->setRowCount(surface.rows.size());
				for (int r = 0; r < surface.rows.size(); ++r)
				{
					const QHash<QString, QString>& record = surface.rows.at(r);
					for (int c = 0; c < surface.keys.size(); ++c)
					{
						QTableWidgetItem* cell = new QTableWidgetItem(record.value(surface.keys.at(c)));
						cell->setFlags(Qt::ItemIsEnabled);
						cell->setToolTip(cell->text());
						table->setItem(r, c, cell);
					}
				}
			}
			else
			{
				//空态：无选列="请先选择查询值"；有列无行="暂无查询结果"（居中置灰）
				table->setRowCount(1);
				const bool noColumns = surface.keys.isEmpty();
				QTableWidgetItem* empty = new QTableWidgetItem(noColumns
					? (_langEn ? QString("Select query values first") : QString::fromUtf8("请先选择查询值"))
					: (_langEn ? QString("No results yet") : QString::fromUtf8("暂无查询结果")));
				empty->setFlags(Qt::ItemIsEnabled);
				empty->setTextAlignment(Qt::AlignCenter);
				empty->setForeground(QBrush(QColor(kMuted)));
				table->setItem(0, 0, empty);
				table->setSpan(0, 0, 1, columns);
			}
			//操作钮可用态（原型：无数据禁清除；无数据或无列禁导出）
			surface.clearButton->setEnabled(surface.rows.size() > 0);
			surface.exportButton->setEnabled(surface.rows.size() > 0 && !surface.keys.isEmpty());
			surface.clearButton->setToolTip(surface.rows.isEmpty()
				? (_langEn ? QString("No data to clear") : QString::fromUtf8("暂无可清除的数据"))
				: (_langEn ? QString("Clear current tab data") : QString::fromUtf8("清除当前页数据")));
			surface.exportButton->setToolTip(surface.exportButton->isEnabled()
				? (_langEn ? QString("Export current tab data") : QString::fromUtf8("导出当前页数据"))
				: (_langEn ? QString("No data to export") : QString::fromUtf8("暂无可导出的数据")));
			table->setUpdatesEnabled(true);
		}

		//页签行刷新：不可用页签整项隐藏；仅激活页签显示关闭钮（原型 refreshResultTabs）
		void refreshTabs()
		{
			for (auto& surface : _surfaces)
			{
				const bool active = surface.available && surface.objectType == _active && isVisible();
				surface.item->setVisible(surface.available);
				surface.item->setStyleSheet(active
					? QString("QWidget{background:#FFFFFF;}")
					: QString("QWidget{background:transparent;}"));
				surface.dismiss->setVisible(active);
			}
		}

	private:
		Surface _surfaces[3];
		QStackedWidget* _stack = nullptr;
		int _active = QueryNode;
		bool _langEn = false;

		friend class mPostQueryPanel;
	};

	/******************************************************************/

	mPostQueryPanel::mPostQueryPanel(QWidget* parent) : mPanelBase(parent)
	{
		_langEn = (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1);
		setObjectName("queryParameters");
		for (int i = 0; i < 3; ++i)
			for (const char* key : kDefaultSelected[i])
				_selectedFields[i].insert(QString::fromLatin1(key));
		setupUI();
		applyLanguageTexts();

		//语言切换全量刷新
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
			this, SLOT(slot_changeLanguage(int)));

		//Escape 逐级退出：查询值弹窗 → 结果停靠窗 → 拾取武装态（原型 keydown 顺序）
		bindShortcut(QKeySequence(Qt::Key_Escape), [this]()
			{
				if (_valuesPopup && _valuesPopup->isVisible())
					closeValuesPopup();
				else if (_resultsDock && _resultsDock->isVisible())
					_resultsDock->closeResults();
				else
					deactivatePick();
			});

		//云图重绘（切帧/切变量）联动重查（拾取条激活/失活收口在 setupUI 内接线）
		if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
			connect(cloudPanel.get(), &mPost3DResultPanel::cloudRedrawnSig,
				this, &mPostQueryPanel::slot_cloudRedrawn, Qt::UniqueConnection);
	}

	mPostQueryPanel::~mPostQueryPanel()
	{
		//停靠窗挂视口、弹窗无父——面板销毁时一并收回
		if (_resultsDock)
		{
			_resultsDock->hide();
			_resultsDock->deleteLater();
		}
		if (_valuesPopup)
		{
			_valuesPopup->hide();
			_valuesPopup->deleteLater();
		}
	}

	void mPostQueryPanel::setupUI()
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);
		_mainLayout->setSpacing(kGap);
		setStyleSheet(
			"mPostQueryPanel{background-color:#FFFFFF;}"
			"QComboBox QAbstractItemView{background-color:#FFFFFF;color:#303313;"
			"selection-background-color:#E8F3FF;selection-color:#303313;}");

		/******** 折叠组：查询对象及坐标系（原型 parameter-section 三行） ********/
		_objectFold = new FoldSection(QString::fromUtf8("查询对象及坐标系"), this);

		//对象选择 = [类型下拉 | mPost 通用拾取条] 单行并排（云图/向量图/阵列/爆炸同款；
		//原共框复合行 mObjectPickComposite 随通用拾取栈接入退役）。本面板拾取为真链路
		//（实时查询）：收割/高亮/重入回显由拾取栈控制器自管，面板经 dataChanged 增量吸收
		_comboObjectType = new QComboBox(this);
		_comboObjectType->setObjectName("queryObjectType");
		_comboObjectType->setFixedHeight(kControl);
		_comboObjectType->addItems(QStringList{ QString::fromUtf8("节点"),
			QString::fromUtf8("单元"), QString::fromUtf8("部件") });	//applyLanguageTexts 重译
		_pickController = mPostGeneralPickUp::createPickWidget(CBBlue, this);
		_pickController->widget()->setObjectName("queryObjectPick");
		_pickController->setSelectSetBtnVisible(false);               //后处理无集合语义
		_pickController->setKeepHighlightOnDeactivate(false);         //解除拾取即清视口高亮
		_pickController->setAllowedTypes([this]() {
			//激活时按当前类型发工具栏白名单（0 节点/1 单元/2 部件）
			static const MxFunctions::PickDataType kTypes[3] = {
				MxFunctions::PickDataType::Node, MxFunctions::PickDataType::Element,
				MxFunctions::PickDataType::Part
			};
			QSet<MxFunctions::PickDataType> types;
			types.insert(kTypes[qBound(0, _comboObjectType->currentIndex(), 2)]);
			return types;
		});
		refreshPickButton();	//空态默认文案（"选择X"随类型/语言）
		_objectFold->addRow(new PanelLabel(QString::fromUtf8("对象选择")),
			_comboObjectType, _pickController->widget());

		//激活/失活收口（云图/向量图/阵列/爆炸同款；勿用 activePickSig——只同步前处理
		//工具栏）：激活=校验后处理上下文+结果库（查询数据源，无效立即失活回退）+按类型
		//设视口拾取过滤器；失活=清过滤器与拾取高亮+恢复工具栏全部拾取类型
		connect(_pickController, &mPostPickWidgetController::pickActiveChanged, this,
			[this](bool active) {
				REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
				const int type = qBound(0, objectType(), 2);
				if (active)
				{
					if (postRend == nullptr || postRend->getPostRender() == nullptr || !_postDB)
					{
						_pickController->setPickActive(false);
						return;
					}
					postRend->setPickFilter(queryPickFilter(type));
				}
				else
				{
					if (postRend != nullptr)
						postRend->setPickFilter(mxr::PickFilter::PickNothing);
					emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
				}
			});
		//拾取数据变化（视口拾取/详情弹窗删项/清空）→ 增量吸收并实时查询
		connect(_pickController, &mPostPickWidgetController::dataChanged,
			this, &mPostQueryPanel::absorbPickedData);

		//坐标系 + 新建坐标系"＋"占位钮（同云图面板口径；分析坐标系默认选中）
		_comboCoordSystem = new QComboBox(this);
		_comboCoordSystem->setFixedHeight(kControl);
		_btnCoordAdd = new QToolButton(this);
		_btnCoordAdd->setText(QString::fromUtf8("＋"));
		_btnCoordAdd->setFixedSize(QSize(kControl, kControl));
		_btnCoordAdd->setEnabled(false);
		QWidget* coordRow = new QWidget(this);
		QHBoxLayout* coordLayout = new QHBoxLayout(coordRow);
		coordLayout->setContentsMargins(0, 0, 0, 0);
		coordLayout->setSpacing(0);
		coordLayout->addWidget(_comboCoordSystem, 1);
		coordLayout->addWidget(_btnCoordAdd);
		_objectFold->addRow(new PanelLabel(QString::fromUtf8("坐标系")), coordRow);

		//查询值 = 高级选项钮（"已选 N 项 ›"；点击开查询值弹窗，再点关闭）
		_btnValuesOpen = new QPushButton(this);
		_btnValuesOpen->setObjectName("queryValuesOpen");
		_btnValuesOpen->setFixedHeight(kControl);
		_btnValuesOpen->setCursor(Qt::PointingHandCursor);
		_btnValuesOpen->setStyleSheet(
			QString("QPushButton{background:#FFFFFF;border:1px solid %1;border-radius:%2px;color:%3;font-size:12px;}"
				"QPushButton:hover{background:%4;border-color:%5;}")
			.arg(kLineBorder).arg(kRadius).arg(kTextColor, kSelectedBg, kPrimary));
		QHBoxLayout* valuesLayout = new QHBoxLayout(_btnValuesOpen);
		valuesLayout->setContentsMargins(kGroupInset, 0, kGroupInset, 0);
		valuesLayout->setSpacing(4);
		_labelValuesSummary = new QLabel(_btnValuesOpen);
		_labelValuesSummary->setStyleSheet("background:transparent;border:0;");
		valuesLayout->addWidget(_labelValuesSummary, 1);
		QLabel* valuesArrow = new QLabel(QString::fromUtf8("›"), _btnValuesOpen);
		valuesArrow->setStyleSheet("background:transparent;border:0;color:#303313;");
		valuesLayout->addWidget(valuesArrow, 0, Qt::AlignRight);
		_objectFold->addRow(new PanelLabel(QString::fromUtf8("查询值")), _btnValuesOpen);

		_objectFold->refreshLayout();
		_mainLayout->addWidget(_objectFold);

		/******** 折叠组：查询结果（三行入口"N项 ›"） ********/
		_resultsFold = new FoldSection(QString::fromUtf8("查询结果"), this);
		static const char* kEntryCn[3] = { "节点数据", "单元数据", "部件数据" };
		for (int i = 0; i < 3; ++i)
		{
			_btnResultEntry[i] = new QPushButton(this);
			_btnResultEntry[i]->setObjectName(QString("query%1ResultsOpen").arg(i));
			_btnResultEntry[i]->setFixedHeight(kControl);
			_btnResultEntry[i]->setCursor(Qt::PointingHandCursor);
			_btnResultEntry[i]->setStyleSheet(_btnValuesOpen->styleSheet());
			QHBoxLayout* entryLayout = new QHBoxLayout(_btnResultEntry[i]);
			entryLayout->setContentsMargins(kGroupInset, 0, kGroupInset, 0);
			entryLayout->setSpacing(4);
			_labelResultSummary[i] = new QLabel(_btnResultEntry[i]);
			_labelResultSummary[i]->setStyleSheet("background:transparent;border:0;");
			entryLayout->addWidget(_labelResultSummary[i], 1);
			QLabel* entryArrow = new QLabel(QString::fromUtf8("›"), _btnResultEntry[i]);
			entryArrow->setStyleSheet("background:transparent;border:0;color:#303313;");
			entryLayout->addWidget(entryArrow, 0, Qt::AlignRight);
			_resultsFold->addRow(new PanelLabel(QString::fromUtf8(kEntryCn[i])), _btnResultEntry[i]);

			connect(_btnResultEntry[i], &QPushButton::clicked, this, [this, i]()
				{ slot_resultEntryClicked(i); });
		}
		_resultsFold->refreshLayout();
		_mainLayout->addWidget(_resultsFold);

		_mainLayout->addStretch();

		connect(_comboObjectType, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mPostQueryPanel::slot_chooseObjectType);
		connect(_btnValuesOpen, &QPushButton::clicked, this, &mPostQueryPanel::slot_valuesOpenClicked);
	}

	void mPostQueryPanel::slot_chooseObjectType(int index)
	{
		Q_UNUSED(index);
		//切换对象类型：先退出拾取武装态（原型 deactivatePick；已拾取对象与查询结果
		//按类型独立保留——与云图/向量图"切类型清数据"口径有意不同，查询面板多类型
		//并行积累），再联动拾取条文案/弹窗/摘要
		deactivatePick();
		refreshPickButton();
		if (_valuesPopup && _valuesPopup->isVisible())
			rebuildFieldOptions();
		refreshFieldSummary();
	}

	void mPostQueryPanel::deactivatePick()
	{
		//退出拾取武装态：失活拾取栈控件即可，视口过滤器/高亮清空/工具栏恢复由
		//pickActiveChanged(false) 链统一收口（mPostVectorPlot::endPick 同款）
		if (_pickController != nullptr && _pickController->isPickActive())
			_pickController->setPickActive(false);
	}

	void mPostQueryPanel::refreshPickButton()
	{
		//拾取条空态默认文案（随对象类型/语言"选择X"；有拾取数据时条体显示栈内建
		//计数文案——语言限制与云图/向量图/阵列/爆炸同款留档）
		if (_pickController == nullptr || _comboObjectType == nullptr)
			return;
		const int type = qBound(0, _comboObjectType->currentIndex(), 2);
		static const char* kTypeCn[3] = { "节点", "单元", "部件" };
		static const char* kTypeEn[3] = { "Nodes", "Elements", "Parts" };
		_pickController->setDefultTip(_langEn
			? QString("Select %1").arg(kTypeEn[type])
			: QString::fromUtf8("选择%1").arg(QString::fromUtf8(kTypeCn[type])));
	}

	namespace
	{
		//当前激活后处理视口的 guidata（查询值后缀与 runQuery 同源；缺失返回 null）。
		//不能按 getPostDataName() 走 getGUIDataByViewName——后处理数据名与视口名是
		//两个命名域（mPost3DResult 两值并列使用），恒查空会让后缀永远"未指定变量"
		mPostGUIData* activeGuidata()
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend && postRend->getPostRender())
				return getCVM->getGUIDataByPostRender(postRend);
			return nullptr;
		}
	}

	QString mPostQueryPanel::cloudVariableName() const
	{
		//云图变量取当前激活后处理视口 guidata（缺失返回空→调用方按"未指定变量"回退）
		if (mPostGUIData* guidata = activeGuidata())
			return guidata->_cloudVarName;
		return QString();
	}

	QString mPostQueryPanel::cloudCompoName() const
	{
		//云图分量名：查询值"向量值"后缀显示当前分量（云图值显示变量名，两者同源）
		if (mPostGUIData* guidata = activeGuidata())
			return guidata->_cloudCompo;
		return QString();
	}

	QString mPostQueryPanel::fieldCaption(const QString& key, const QString& baseText) const
	{
		//列名/选项不带"（变量名）"后缀（用户口径 2026-11-12：变量与分量以数值
		//显示在表格单元格里，列名保持纯文案）；key 保留用于调用方语义分支
		Q_UNUSED(key);
		return baseText;
	}

	QString mPostQueryPanel::optionCaption(const QString& key, const QString& baseText) const
	{
		//弹窗选项文案：部件结果字段缩短为 最大值/最大值编号 等（原型 optionCaption）
		if (_comboObjectType->currentIndex() == QueryPart)
		{
			static const struct { const char* key; const char* cn; const char* en; } kPartShort[] =
			{
				{ "contourMax", "最大值", "Max" }, { "contourMaxId", "最大值编号", "Max ID" },
				{ "contourMin", "最小值", "Min" }, { "contourMinId", "最小值编号", "Min ID" },
				{ "vectorMax", "最大值", "Max" }, { "vectorMaxId", "最大值编号", "Max ID" },
				{ "vectorMin", "最小值", "Min" }, { "vectorMinId", "最小值编号", "Min ID" }
			};
			for (const auto& def : kPartShort)
			{
				if (key == QLatin1String(def.key))
					return _langEn ? QString(def.en) : QString::fromUtf8(def.cn);
			}
		}
		return fieldCaption(key, baseText);
	}

	void mPostQueryPanel::refreshFieldSummary()
	{
		const QStringList keys = selectedFields();
		_labelValuesSummary->setText(_langEn
			? QString("%1 selected").arg(keys.size())
			: QString::fromUtf8("已选 %1 项").arg(keys.size()));
		if (!keys.isEmpty())
		{
			QStringList names;
			for (const QString& key : keys)
			{
				const FieldDef* def = findField(_comboObjectType->currentIndex(), key);
				if (def)
					names << fieldCaption(key, _langEn ? QString(def->en) : QString::fromUtf8(def->cn));
			}
			_btnValuesOpen->setToolTip(_langEn
				? QString("%1 selected: %2").arg(keys.size()).arg(names.join(", "))
				: QString::fromUtf8("已选 %1 项：%2").arg(keys.size()).arg(names.join(QString::fromUtf8("、"))));
		}
		else
		{
			_btnValuesOpen->setToolTip(_langEn
				? QString("No query values selected")
				: QString::fromUtf8("未选择查询值"));
		}
	}

	/******** 查询值弹窗 ********/

	void mPostQueryPanel::slot_valuesOpenClicked()
	{
		if (!_valuesPopup)
		{
			//弹窗壳（原型 fbd-popup：头 28"查询值"+关闭钮；正文滚动分组清单）。
			//挂面板子树（Qt::Popup）：生命周期随面板，且 ui-shot 弹窗序列可定位截图
			_valuesPopup = new QFrame(this, Qt::Popup);
			_valuesPopup->setObjectName("queryValuesPopup");
			_valuesPopup->setFrameShape(QFrame::NoFrame);
			_valuesPopup->setStyleSheet(QString(
				"QFrame#queryValuesPopup{background:#FFFFFF;border:1px solid %1;}").arg(kLineBorder));
			QVBoxLayout* popupLayout = new QVBoxLayout(_valuesPopup);
			popupLayout->setContentsMargins(0, 0, 0, 0);
			popupLayout->setSpacing(0);

			QFrame* header = new QFrame(_valuesPopup);
			header->setFixedHeight(kHeader);
			header->setStyleSheet("QFrame{background:#FFFFFF;border:0;border-bottom:1px solid #DCE2E9;}");
			QHBoxLayout* headerLayout = new QHBoxLayout(header);
			headerLayout->setContentsMargins(kGroupInset, 0, 0, 0);
			headerLayout->setSpacing(0);
			_valuesPopupTitle = new QLabel(header);
			_valuesPopupTitle->setStyleSheet("background:transparent;border:0;color:#303313;font-size:12px;");
			headerLayout->addWidget(_valuesPopupTitle);
			headerLayout->addStretch();
			QToolButton* close = new QToolButton(header);
			close->setIcon(closeIcon(QColor(kTextColor)));
			close->setFixedSize(24, 24);
			close->setCursor(Qt::PointingHandCursor);
			close->setStyleSheet("QToolButton{background:transparent;border:0;}QToolButton:hover{background:#F0F7FF;}");
			headerLayout->addWidget(close);
			popupLayout->addWidget(header);

			_valuesPopupScroll = new QScrollArea(_valuesPopup);
			_valuesPopupScroll->setWidgetResizable(true);
			_valuesPopupScroll->setFrameShape(QFrame::NoFrame);
			_valuesPopupScroll->setStyleSheet("QScrollArea{background:#FFFFFF;border:0;}");
			_valuesPopupContent = new QWidget(_valuesPopupScroll);
			QVBoxLayout* contentLayout = new QVBoxLayout(_valuesPopupContent);
			contentLayout->setContentsMargins(kGap, kGap, kGap, kGap);
			contentLayout->setSpacing(kGap);
			_valuesPopupScroll->setWidget(_valuesPopupContent);
			popupLayout->addWidget(_valuesPopupScroll);

			connect(close, &QToolButton::clicked, this, [this]() { closeValuesPopup(); });

			_valuesPopup->setFixedWidth(kValuesPopupWidth);
		}
		if (_valuesPopup->isVisible())
		{
			closeValuesPopup();
			return;
		}
		//开弹窗前退出拾取武装态（原型互斥模式）
		deactivatePick();
		_valuesPopupTitle->setText(_langEn ? QString("Query Values") : QString::fromUtf8("查询值"));
		rebuildFieldOptions();
		popupUnder(_valuesPopup, _btnValuesOpen);
	}

	void mPostQueryPanel::closeValuesPopup()
	{
		if (_valuesPopup)
			_valuesPopup->hide();
	}

	void mPostQueryPanel::popupUnder(QWidget* popup, QWidget* anchor)
	{
		//弹窗贴锚点控件下方显示（越界收回屏内；同 mPostTagPanel 口径）
		popup->adjustSize();
		QPoint pos = anchor->mapToGlobal(QPoint(0, anchor->height() + 6));
		if (QScreen* screen = QGuiApplication::screenAt(anchor->mapToGlobal(QPoint(0, 0))))
		{
			const QRect avail = screen->availableGeometry();
			if (pos.x() + popup->width() > avail.right())
				pos.setX(avail.right() - popup->width());
			if (pos.y() + popup->height() > avail.bottom())
				pos.setY(anchor->mapToGlobal(QPoint(0, 0)).y() - popup->height() - 6);
		}
		popup->move(pos);
		popup->show();
		popup->raise();
	}

	void mPostQueryPanel::rebuildFieldOptions()
	{
		if (!_valuesPopupContent)
			return;
		QVBoxLayout* contentLayout = static_cast<QVBoxLayout*>(_valuesPopupContent->layout());
		//清空全部旧项（含 stretch，重建后补回）。不能按"保留最后一项=stretch"清：
		//构造期 stretch 先入、addWidget 追加其后，重建一次后尾项已是分组框，
		//再清会把旧分组框误当 stretch 留下（第二次开弹窗首尾各多一个"结果信息"）
		while (contentLayout->count() > 0)
		{
			QLayoutItem* item = contentLayout->takeAt(0);
			if (QWidget* widget = item->widget())
				widget->deleteLater();
			delete item;
		}

		const int objectType = _comboObjectType->currentIndex();
		const FieldGroup* groups = fieldGroupList(objectType);
		const QString resultGroupCn = QString::fromUtf8("结果信息");

		for (int g = 0; g < fieldGroupCount(objectType); ++g)
		{
			const FieldGroup& group = groups[g];
			//分组框（原型 query-field-group：1px 边框圆角 4，标题+复选行纵排）
			QFrame* groupFrame = new QFrame(_valuesPopupContent);
			groupFrame->setStyleSheet(QString(
				"QFrame{background:#FFFFFF;border:1px solid %1;border-radius:%2px;}").arg(kLineBorder).arg(kRadius));
			QVBoxLayout* groupLayout = new QVBoxLayout(groupFrame);
			groupLayout->setContentsMargins(kGap, kGap, kGap, kGap);
			groupLayout->setSpacing(0);
			QLabel* groupTitle = new QLabel(_langEn ? QString(group.en) : QString::fromUtf8(group.cn), groupFrame);
			groupTitle->setStyleSheet("background:transparent;border:0;color:#303313;font-size:12px;");
			groupLayout->addWidget(groupTitle);

			//部件"结果信息"组内云图/向量两族前插变量小标题（原型 query-result-variable-label）
			const bool partResultGroup = (objectType == QueryPart
				&& QString::fromUtf8(group.cn) == resultGroupCn);

			for (const char* keyPtr : group.keys)
			{
				const QString key = QString::fromLatin1(keyPtr);
				const FieldDef* def = findField(objectType, key);
				if (!def)
					continue;
				if (partResultGroup && key == QLatin1String("contourMax"))
					appendVariableHeading(groupLayout, true);
				if (partResultGroup && key == QLatin1String("vectorMax"))
					appendVariableHeading(groupLayout, false);

				const QString baseText = _langEn ? QString(def->en) : QString::fromUtf8(def->cn);
				QCheckBox* box = new QCheckBox(optionCaption(key, baseText), groupFrame);
				box->setObjectName(key);	//ui-shot 点分驱动用（如 contourValue.setChecked(true)）
				box->setStyleSheet(QString(
					"QCheckBox{background:transparent;border:0;color:%1;font-size:12px;"
					"min-height:%2px;spacing:%3px;}"
					"QCheckBox::indicator{width:%4px;height:%4px;}")
					.arg(kTextColor).arg(kControl).arg(kChoiceGap).arg(kChoice));
				box->setChecked(_selectedFields[objectType].contains(key));
				box->setToolTip(fieldCaption(key, baseText));
				groupLayout->addWidget(box);
				connect(box, &QCheckBox::toggled, this, [this, objectType, key](bool on)
					{
						if (on) _selectedFields[objectType].insert(key);
						else _selectedFields[objectType].remove(key);
						refreshFieldSummary();
						refreshDockColumns();
						//字段集变化即重查（该类型已有拾取对象时，列集与值实时刷新）
						if (hasPickedObjects(objectType))
							runQuery(objectType);
					});
			}
			contentLayout->addWidget(groupFrame);
		}
		contentLayout->addStretch();
		_valuesPopup->adjustSize();
	}

	void mPostQueryPanel::appendVariableHeading(QVBoxLayout* groupLayout, bool contour)
	{
		//变量小标题："云图：<变量名>"/"向量：<分量名>"（标签色+上距 6，原型口径；
		//与 fieldCaption 后缀同源：云图族显示当前变量、向量族显示当前分量）
		const QString name = contour ? cloudVariableName() : cloudCompoName();
		QLabel* heading = new QLabel(QString::fromUtf8("%1：%2")
			.arg(QString::fromUtf8(contour ? "云图" : "向量"),
				name.isEmpty() ? QString::fromUtf8("未指定变量") : name));
		heading->setStyleSheet(QString("background:transparent;border:0;color:%1;font-size:12px;")
			.arg(kLabelColor));
		heading->setContentsMargins(0, kGap, 0, 0);
		groupLayout->addWidget(heading);
	}

	/******** 查询结果入口与停靠窗 ********/

	void mPostQueryPanel::slot_resultEntryClicked(int objectType)
	{
		closeValuesPopup();
		deactivatePick();
		ensureResultsDock();
		if (_resultsDock)
		{
			repositionResultsDock();
			_resultsDock->openResults(objectType);
			refreshResultEntries();
		}
	}

	void mPostQueryPanel::slot_openNodeResults()
	{
		//无参包装（ui-shot -Invoke 驱动停靠窗展开；先例 slot_pickNodeClicked 带参化）
		slot_resultEntryClicked(QueryNode);
	}

	void mPostQueryPanel::ensureResultsDock()
	{
		//停靠窗叠加层级：渲染表面是 QOpenGLWidget——Qt5 下其子件会被 GL 纹理盖住，
		//停靠窗须挂其父件作"同级覆盖件"（raise 到 GL 之上），按 osg 几何贴视口底部
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		QWidget* osg = postRend ? postRend->getOsgWidget() : nullptr;
		QWidget* host = osg ? osg->parentWidget() : nullptr;
		if (!host && postRend)
			host = static_cast<QWidget*>(static_cast<mxr::mBaseRend*>(postRend));
		if (!host)
		{
			if (_resultsDock)
			{
				_resultsDock->hide();
				_dockHostViewport = nullptr;
			}
			return;
		}
		if (!_resultsDock)
		{
			_resultsDock = new mPostQueryResultsDock(host);
			_dockHostViewport = osg ? osg : host;
			_dockHostViewport->installEventFilter(this);
			_dockHostViewport->parentWidget()->installEventFilter(this);
			_resultsDock->onClearRequested = [this](int objectType)
				{
					//清除数据=清记录+清该类型已拾取对象（否则下次拾取又并入旧对象）；
					//拾取栈该类型桶一并清（重入回显/条体计数随新数据），其他类型桶不动
					if (objectType == QueryNode)
						_pickedNodes.clear();
					else if (objectType == QueryElement)
						_pickedElements.clear();
					else
						_pickedParts.clear();
					if (_pickController != nullptr)
					{
						static const MxFunctions::PickDataType kTypes[3] = {
							MxFunctions::PickDataType::Node, MxFunctions::PickDataType::Element,
							MxFunctions::PickDataType::Part
						};
						PickWidgetData data = _pickController->getAllData();
						data.pickedData.erase(kTypes[qBound(0, objectType, 2)]);
						_pickController->displayData(data);
					}
					//渲染容器同步清：视口高亮一并消掉，武装态下再拾取也不会并入旧对象
					clearPickedHighlight(objectType);
					_resultsDock->setRows(objectType, QList<QHash<QString, QString>>());
					refreshResultEntries();
				};
			_resultsDock->onExportRequested = [this](int objectType) { exportResults(objectType); };
			_resultsDock->resetTabs();
			applyDockTexts();
			refreshDockColumns();
			return;
		}
		if (_resultsDock->parentWidget() != host)
		{
			//换宿主视口：保持原展开态，重挂+重排
			const bool wasVisible = _resultsDock->isVisible();
			if (_dockHostViewport)
			{
				_dockHostViewport->removeEventFilter(this);
				if (_dockHostViewport->parentWidget())
					_dockHostViewport->parentWidget()->removeEventFilter(this);
			}
			_resultsDock->setParent(host);
			_dockHostViewport = osg ? osg : host;
			_dockHostViewport->installEventFilter(this);
			_dockHostViewport->parentWidget()->installEventFilter(this);
			if (wasVisible)
			{
				repositionResultsDock();
				_resultsDock->show();
				_resultsDock->raise();
			}
		}
	}

	QString mPostQueryPanel::tabTitle(int objectType) const
	{
		static const char* kCn[3] = { "节点数据", "单元数据", "部件数据" };
		static const char* kEn[3] = { "Node Data", "Element Data", "Part Data" };
		return _langEn ? QString(kEn[objectType]) : QString::fromUtf8(kCn[objectType]);
	}

	void mPostQueryPanel::applyDockTexts()
	{
		if (!_resultsDock)
			return;
		for (int i = 0; i < 3; ++i)
		{
			_resultsDock->_surfaces[i].tab->setText(tabTitle(i));
			_resultsDock->_surfaces[i].dismiss->setToolTip(_langEn
				? QString("Close") : QString::fromUtf8("关闭"));
			_resultsDock->_surfaces[i].clearButton->setText(_langEn
				? QString("Clear") : QString::fromUtf8("清除数据"));
			_resultsDock->_surfaces[i].exportButton->setText(_langEn
				? QString("Export") : QString::fromUtf8("导出数据"));
		}
		_resultsDock->_langEn = _langEn;
		refreshDockColumns();
	}

	void mPostQueryPanel::repositionResultsDock()
	{
		if (!_resultsDock || !_dockHostViewport)
			return;
		//贴渲染表面（_dockHostViewport=osg 部件）底部整宽；停靠窗与其同父，
		//坐标为父件空间（叠加 osg 的 x/y 偏移）；高按原型公式（不超过视口高）
		QWidget* osg = _dockHostViewport;
		const int height = qMin(kDockHeight, osg->height());
		_resultsDock->setGeometry(osg->x(), osg->y() + osg->height() - height,
			osg->width(), height);
	}

	bool mPostQueryPanel::eventFilter(QObject* watched, QEvent* event)
	{
		//渲染表面/其父件尺寸变化→停靠窗贴底重排（osg 与停靠窗分属两层，两处都要盯）
		if (_resultsDock && event->type() == QEvent::Resize
			&& (watched == _dockHostViewport
				|| (_dockHostViewport && watched == _dockHostViewport->parentWidget())))
			repositionResultsDock();
		return mPanelBase::eventFilter(watched, event);
	}

	void mPostQueryPanel::refreshResultEntries()
	{
		for (int i = 0; i < 3; ++i)
		{
			const int count = _resultsDock ? _resultsDock->rowCount(i) : 0;
			_labelResultSummary[i]->setText(_langEn
				? QString("%1 items").arg(count)
				: QString::fromUtf8("%1项").arg(count));
		}
	}

	void mPostQueryPanel::refreshDockColumns()
	{
		if (!_resultsDock)
			return;
		for (int objectType = 0; objectType < 3; ++objectType)
		{
			QStringList keys;
			QStringList captions;
			const FieldDef* catalog = fieldCatalog(objectType);
			for (int i = 0; i < fieldCatalogSize(objectType); ++i)
			{
				const QString key = QString::fromLatin1(catalog[i].key);
				if (!_selectedFields[objectType].contains(key))
					continue;
				keys << key;
				captions << fieldCaption(key, _langEn ? QString(catalog[i].en) : QString::fromUtf8(catalog[i].cn));
			}
			_resultsDock->setColumns(objectType, keys, captions);
		}
	}

	void mPostQueryPanel::exportResults(int objectType)
	{
		//CSV 导出（原型 exportResults：BOM+引号转义+CRLF；空数据钮已禁用不可达）
		if (!_resultsDock)
			return;
		const auto& surface = _resultsDock->_surfaces[objectType];
		if (surface.rows.isEmpty() || surface.keys.isEmpty())
			return;
		const QString suggested = tabTitle(objectType) + ".csv";
		const QString path = QFileDialog::getSaveFileName(this,
			_langEn ? QString("Export") : QString::fromUtf8("导出"),
			suggested, "CSV (*.csv)");
		if (path.isEmpty())
			return;
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return;
		QString csv = QChar(0xFEFF);
		auto quote = [](const QString& text)
			{ return QLatin1Char('"') + text + QLatin1Char('"'); };
		QStringList head;
		for (const QString& caption : surface.captions)
			head << quote(caption);
		csv += head.join(QLatin1Char(',')) + "\r\n";
		for (const QHash<QString, QString>& record : surface.rows)
		{
			QStringList row;
			for (const QString& key : surface.keys)
				row << quote(record.value(key));
			csv += row.join(QLatin1Char(',')) + "\r\n";
		}
		file.write(csv.toUtf8());
	}

	/******** 查询引擎接入（拾取 → 取数 → setResults） ********/

	namespace
	{
		//类型 → 拾取过滤器 / 拾取数据类型（同集合面板口径；queryPickFilter 已前移顶部）
		mxr::SelectionInfoType querySelType(int objectType)
		{
			static const mxr::SelectionInfoType kTypes[3] =
			{
				mxr::SelectionInfoType::MeshNode, mxr::SelectionInfoType::MeshElement, mxr::SelectionInfoType::MeshPart
			};
			return kTypes[qBound(0, objectType, 2)];
		}

		//数值显示（6 位有效数字；非法值 "-"；云图值/坐标/形心/法向共用）
		QString fmtValue(double v) { return qIsFinite(v) ? QString::number(v, 'g', 6) : QStringLiteral("-"); }
		QString fmtCoord(const QVector3D& p)
		{
			return QString("(%1, %2, %3)").arg(fmtValue(p.x()), fmtValue(p.y()), fmtValue(p.z()));
		}

		//读取失败统一报输出窗口（口径同高级查询 reportQueryReadFailure）
		void reportContourReadFailure(int frame, const QString& varstr, const QString& compostr,
			const QString& postPathName)
		{
			const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				isChinese ? QStringLiteral("查询读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
								.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
						  : QStringLiteral("Query failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
								.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
				true);
		}
	}

	bool mPostQueryPanel::hasPickedObjects(int objectType) const
	{
		switch (objectType)
		{
		case QueryNode: return !_pickedNodes.isEmpty();
		case QueryElement: return !_pickedElements.isEmpty();
		default: return !_pickedParts.isEmpty();
		}
	}

	void mPostQueryPanel::absorbPickedData()
	{
		//拾取栈数据变化（视口拾取/详情弹窗删项/清空）→ 增量吸收进面板记录（拾取序
		//去重；原 slot_pickDataArrived 的容器直读改为读栈数据模型，收割/重入高亮由
		//控制器自管）；任一类型有新对象即实时查询（原"仅当前类型"口径随多类型桶
		//数据源放宽为按到达类型触发）
		if (_pickController == nullptr || !_postDB)
			return;
		const auto& picked = _pickController->getAllData().pickedData;

		bool changed[3] = { false, false, false };
		auto it = picked.find(MxFunctions::PickDataType::Node);
		if (it != picked.end())
		{
			for (const auto& item : it->second)
			{
				//Node 桶：result_id=节点真实ID
				const int id = item.result_id;
				if (id >= 0 && !_pickedNodes.contains(id))
				{
					_pickedNodes << id;
					changed[QueryNode] = true;
				}
			}
		}
		it = picked.find(MxFunctions::PickDataType::Element);
		if (it != picked.end())
		{
			for (const auto& item : it->second)
			{
				//Element 桶：result_id=单元真实ID，part_id=所属部件（未知记 -1）
				const int eleId = item.result_id;
				if (eleId < 0)
					continue;
				const int partId = item.part_id >= 0 ? item.part_id : -1;
				bool exists = false;
				for (auto& rec : _pickedElements)
				{
					if (rec.first == eleId)
					{
						//重复拾取补全未知部件归属
						if (rec.second < 0 && partId >= 0)
							rec.second = partId;
						exists = true;
						break;
					}
				}
				if (!exists)
				{
					_pickedElements << qMakePair(eleId, partId);
					changed[QueryElement] = true;
				}
			}
		}
		it = picked.find(MxFunctions::PickDataType::Part);
		if (it != picked.end())
		{
			for (const auto& item : it->second)
			{
				if (item.part_id >= 0 && !_pickedParts.contains(item.part_id))
				{
					_pickedParts << item.part_id;
					changed[QueryPart] = true;
				}
			}
		}
		for (int t = QueryNode; t <= QueryPart; ++t)
			if (changed[t])
				runQuery(t);
	}

	void mPostQueryPanel::clearPickedHighlight(int objectType)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (auto* container = postRend ? postRend->getSelectionContainer() : nullptr)
		{
			container->getMeshSelectData()->UpdateSelectDataByType(
				mxr::SelectionOperate::CLEAR, querySelType(qBound(0, objectType, 2)), {});
			MechPreData::UpdatePostWidgetHighlight(postRend);
			postRend->updatePaint();
		}
	}

	void mPostQueryPanel::slot_cloudRedrawn(const QString& viewName)
	{
		//切帧/切变量后云图值族过期：面板在场且有数据的页签按新上下文重查
		//（重绘源即当前激活视口，viewName 仅作来源提示不另判）
		Q_UNUSED(viewName);
		if (!isVisible() || !_postDB)
			return;
		//后缀上下文刷新：云图值（变量名）/向量值（分量名）随当前云图重绘同步，
		//摘要 tooltip 与停靠窗表头一并换新（否则表头留旧变量名）
		refreshFieldSummary();
		refreshDockColumns();
		if (hasPickedObjects(QueryNode))
			runQuery(QueryNode);
		if (hasPickedObjects(QueryElement))
			runQuery(QueryElement);
		if (hasPickedObjects(QueryPart))
			runQuery(QueryPart);
	}

	void mPostQueryPanel::slot_seedPickForShot(int objectType, int id)
	{
		//ui-shot 自动化验证专用：视口拾取无法脚本驱动，经此口注入对象走完整查询链
		//（吸收→runQuery→readContourCompones→setResults 停靠窗/计数全链）；产品交互不经过
		if (objectType < QueryNode || objectType > QueryPart || id < 0)
			return;
		if (objectType == QueryNode && !_pickedNodes.contains(id))
			_pickedNodes << id;
		else if (objectType == QueryElement)
		{
			bool exists = false;
			for (const auto& rec : _pickedElements)
				if (rec.first == id) { exists = true; break; }
			if (!exists)
				_pickedElements << qMakePair(id, -1);
		}
		else if (objectType == QueryPart && !_pickedParts.contains(id))
			_pickedParts << id;
		runQuery(objectType);
	}

	bool mPostQueryPanel::readVarCompones(int frame, const QString& varstr, const QString& compostr,
		const QString& layerstr, const QString& complexStr, PostData::Compones& compone) const
	{
		if (varstr.isEmpty())
			return false;
		const QString postPathName = _postDB->getFile();
		//统一取值 provider（阶段2）：四格式分支收敛为出参式单口（键域补丁/失败报告逐点保持）
		IPostValueProvider* provider = valueProviderFor(_postDB);
		if (provider == nullptr)
		{
			reportContourReadFailure(frame, varstr, compostr, postPathName);
			return false;
		}
		ValueRequest req;
		req.frame = frame;
		req.var = varstr;
		req.compo = compostr;
		req.layer = layerstr;
		req.complexMode = complexStr;
		const bool ok = provider->fetch(req, compone);
		delete provider;
		if (!ok)
		{
			reportContourReadFailure(frame, varstr, compostr, postPathName);
			return false;
		}
		if (_postDB->getDataFlag())
		{
			//mxdb 老链路该重载只填 value1/value2 且不置 isNodeFace：节点位变量
			//（flag==0）的键是连续节点id（云图面板 STL 导出同款坑）——统一换算成
			//真实id 并按变量位形置 isNodeFace，对齐 native 分支布局供消费方单口径
			if (QPointer<mPostModelData> md = _postDB->getModelData(frame))
			{
				if (compone.flag == 0)
				{
					QHash<int, double> real;
					for (auto it = compone.value1.cbegin(); it != compone.value1.cend(); ++it)
					{
						const int k = it.key();
						real.insert(k >= 0 && k < md->_nodedata.nodeamount
							? md->_nodedata.ids[k] : k, it.value());
					}
					compone.value1 = real;
					compone.isNodeFace = true;
				}
				else
					compone.isNodeFace = false;
			}
		}
		return true;
	}

	QHash<int, double> mPostQueryPanel::flattenCompones(const PostData::Compones& compone)
	{
		//实体/壳两层逐 id 层平均（同高级查询 mergeHash 口径；R 先插 Q 覆盖）
		QHash<int, double> flat;
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
		//单层 id→值载荷（mxdb 新链路重载只填 value1/value2；与上面的两层布局
		//按读取分支互斥，后吸收覆盖不冲突。节点位键已在 readVarCompones 换算真实id）
		auto absorbFlat = [&flat](const auto& single)
		{
			for (auto it = single.cbegin(); it != single.cend(); ++it)
				flat.insert(it.key(), double(it.value()));
		};
		if (compone.isD)
		{
			absorb(compone.dRValue);
			absorb(compone.dQValue);
		}
		else
		{
			absorb(compone.fRValue);
			absorb(compone.fQValue);
		}
		absorbFlat(compone.value1);
		absorbFlat(compone.value2);
		absorbFlat(compone.value3);
		absorbFlat(compone.value4);
		return flat;
	}

	void mPostQueryPanel::runQuery(int objectType)
	{
		if (objectType < QueryNode || objectType > QueryPart || !_postDB)
			return;
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		auto postRender = postRend ? postRend->getPostRender() : nullptr;
		mPostGUIData* guidata = postRender ? getCVM->getGUIDataByPostRender(postRend) : nullptr;
		if (!postRender || !guidata)
			return;

		const int fi = guidata->_cloudFrame;
		QPointer<mPostModelData> modelData = _postDB->getModelData(fi);
		if (!modelData)
			return;

		const QStringList keys = selectedFieldsFor(objectType);
		const auto has = [&keys](const char* key) { return keys.contains(QLatin1String(key)); };

		//上下文：工况/帧名（guidata 优先，库按帧号兜底）；坐标系两列
		QString caseName = guidata->_cloudCase;
		if (caseName.isEmpty())
			caseName = _postDB->getCaseNameByFrameNum(fi);
		QString frameName = _postDB->getFrameNameByFrameNum(fi);
		if (frameName.isEmpty())
			frameName = QString::number(fi);
		const QString globalCs = _langEn ? QString("Global CS") : QString::fromUtf8("全局坐标系");
		const QString analysisCs = _comboCoordSystem->currentText().isEmpty()
			? (_langEn ? QString("Analysis CS") : QString::fromUtf8("分析坐标系"))
			: _comboCoordSystem->currentText();

		//材料/截面属性 → 部件反查
		QHash<int, QString> matOfPart;
		for (auto it = _postDB->getMaterialParts().cbegin(); it != _postDB->getMaterialParts().cend(); ++it)
			for (int partId : it.value())
				matOfPart.insert(partId, it.key());
		QHash<int, QString> secOfPart;
		for (auto it = _postDB->getSectionParts().cbegin(); it != _postDB->getSectionParts().cend(); ++it)
			for (int partId : it.value())
				secOfPart.insert(partId, it.key());

		//云图分量（节点/单元行；部件行极值走渲染代理）
		QHash<int, double> flatValues;
		bool contourNodal = true;
		if (objectType != QueryPart && has("contourValue") && !guidata->_cloudVarName.isEmpty())
		{
			PostData::Compones compone;
			if (readVarCompones(fi, guidata->_cloudVarName, guidata->_cloudCompo,
				guidata->_cloudLayerName, guidata->_cloudComplex, compone))
			{
				flatValues = flattenCompones(compone);
				contourNodal = compone.isNodeFace;
			}
		}

		//向量值族（节点/单元行）：当前云图变量的全部分量逐个读取摊平，单元格输出
		//"(v1, v2, …)"；标量/伪变量无分量列表→"-"。向量未另指定变量，与云图值同源
		//（用户口径 2026-11-12：分量值显示在表格里，列名不带后缀）
		QVector<QHash<int, double>> vectorComps;
		bool vectorNodal = true;
		if (objectType != QueryPart && has("vectorValue") && !guidata->_cloudVarName.isEmpty())
		{
			const QStringList compos = _postDB->getVariableCompos(guidata->_cloudVarName);
			for (const QString& cn : compos)
			{
				PostData::Compones vc;
				if (readVarCompones(fi, guidata->_cloudVarName, cn,
					guidata->_cloudLayerName, guidata->_cloudComplex, vc))
				{
					vectorComps << flattenCompones(vc);
					vectorNodal = vc.isNodeFace;
				}
				else
				{
					vectorComps << QHash<int, double>();
				}
			}
			//任一分量空（读取失败）即视为无向量数据，整列回"-"
			for (const auto& vc : vectorComps)
				if (vc.isEmpty())
				{
					vectorComps.clear();
					break;
				}
		}

		//部件极值列（云图最大/最小值+对象编号）：渲染代理反映当前显示数据（同高级查询）
		mxr::IPostModelRenderDataProxy* proxy = nullptr;
		if (objectType == QueryPart && (has("contourMax") || has("contourMin")))
			proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(
				postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());

		//部件归属辅助：节点→所属部件集；单元→所属部件（拾取消息优先，回退按部件单元表扫描）
		QHash<int, QSet<int>> partEleCache;
		auto partIdsOfNode = [&modelData](int nodeId)
		{
			QList<int> partIds;
			for (auto part : modelData->_partdata)
			{
				if (part && part->getNodeIdSet() && part->getNodeIdSet()->count(nodeId) > 0)
					partIds << part->getPartID();
			}
			return partIds;
		};
		auto partOfElement = [&modelData, &partEleCache](int eleId, int hintPartId) -> int
		{
			if (hintPartId >= 0 && modelData->getPartData(hintPartId))
				return hintPartId;
			for (auto part : modelData->_partdata)
			{
				if (!part)
					continue;
				const int partId = part->getPartID();
				if (!partEleCache.contains(partId))
					partEleCache.insert(partId, QSet<int>::fromList(modelData->getElementIdsByPartId(partId)));
				if (partEleCache.value(partId).contains(eleId))
					return partId;
			}
			return -1;
		};
		auto joinNames = [](const QStringList& names)
		{ return names.isEmpty() ? QString::fromUtf8("-") : names.join(QString::fromUtf8("、")); };
		auto modelInfoOf = [&matOfPart, &secOfPart, &joinNames](const QList<int>& partIds,
			bool wantMaterial, bool wantPartName, QPointer<mPostModelData> model)
		{
			QStringList mats, secs, partNames;
			for (int partId : partIds)
			{
				if (const QString& m = matOfPart.value(partId); !m.isEmpty()) mats << m;
				if (const QString& s = secOfPart.value(partId); !s.isEmpty()) secs << s;
				if (wantPartName)
					if (QPointer<mPostPartData> part = model->getPartData(partId))
						partNames << part->getPartName();
			}
			struct Info { QString mat, sec, part; };
			return Info{ wantMaterial ? joinNames(mats) : QString(), wantMaterial ? joinNames(secs) : QString(),
				wantPartName ? joinNames(partNames) : QString() };
		};

		//向量值单元格：逐分量按取值器求值，全部分量命中才输出 "(v1, v2, …)"，缺任一"-"
		auto vectorCellText = [&vectorComps](const std::function<bool(const QHash<int, double>&, double&)>& pick)
		{
			if (vectorComps.isEmpty())
				return QStringLiteral("-");
			QStringList parts;
			for (const auto& comp : vectorComps)
			{
				double v = 0.0;
				if (!pick(comp, v))
					return QStringLiteral("-");
				parts << fmtValue(v);
			}
			return QStringLiteral("(%1)").arg(parts.join(", "));
		};

		QList<QHash<QString, QString>> records;
		if (objectType == QueryNode)
		{
			for (int nodeId : _pickedNodes)
			{
				QHash<QString, QString> rec;
				if (has("nodeId")) rec.insert("nodeId", QString::number(nodeId));
				if (has("referenceSystem")) rec.insert("referenceSystem", globalCs);
				if (has("analysisSystem")) rec.insert("analysisSystem", analysisCs);
				if (has("nodeCoordinates"))
					rec.insert("nodeCoordinates", fmtCoord(modelData->getVectorById(nodeId)));
				if (has("materialName") || has("propertyName") || has("componentName"))
				{
					const auto info = modelInfoOf(partIdsOfNode(nodeId), true, true, modelData);
					if (has("materialName")) rec.insert("materialName", info.mat);
					if (has("propertyName")) rec.insert("propertyName", info.sec);
					if (has("componentName")) rec.insert("componentName", info.part);
				}
				if (has("loadCase")) rec.insert("loadCase", caseName);
				if (has("simulationStep")) rec.insert("simulationStep", frameName);
				if (has("contourValue"))
					rec.insert("contourValue", contourNodal && flatValues.contains(nodeId)
						? fmtValue(flatValues.value(nodeId)) : QStringLiteral("-"));
				if (has("vectorValue"))
					rec.insert("vectorValue", vectorCellText([vectorNodal, nodeId](
						const QHash<int, double>& comp, double& v)
						{
							if (vectorNodal && comp.contains(nodeId))
							{
								v = comp.value(nodeId);
								return true;
							}
							return false;
						}));
				records << rec;
			}
		}
		else if (objectType == QueryElement)
		{
			for (const auto& pick : _pickedElements)
			{
				const int eleId = pick.first;
				const int partId = partOfElement(eleId, pick.second);
				QHash<QString, QString> rec;
				if (has("elementId")) rec.insert("elementId", QString::number(eleId));
				//单元几何族（需部件归属）
				std::vector<int> nodeIds;
				int dimFlag = -1;
				if (partId >= 0)
					nodeIds = modelData->getNodeIdsByEleIdWithoutBuild(partId, eleId, dimFlag);
				if (has("elementType"))
					rec.insert("elementType", partId >= 0
						? MxFunctions::mElementFunction::getElementString(modelData->getEleTypeByEleId(eleId))
						: QStringLiteral("-"));
				if (has("connectivity"))
				{
					QStringList ids;
					for (int nid : nodeIds) ids << QString::number(nid);
					rec.insert("connectivity", ids.isEmpty() ? QStringLiteral("-") : ids.join('-'));
				}
				if (has("centroid") || has("normal") || (has("contourValue") && contourNodal))
				{
					QVector<QVector3D> pts;
					pts.reserve(int(nodeIds.size()));
					for (int nid : nodeIds)
						pts << modelData->getVectorById(nid);
					if (has("centroid") && !pts.isEmpty())
					{
						QVector3D c;
						for (const auto& p : pts) c += p;
						rec.insert("centroid", fmtCoord(c / float(pts.size())));
					}
					if (has("normal") && pts.size() >= 3 && dimFlag == 2)
						rec.insert("normal", fmtCoord(QVector3D::normal(pts[0], pts[1], pts[2])));
					if (has("contourValue") && contourNodal && !flatValues.isEmpty())
					{
						//节点位变量在单元行 = 单元节点值平均
						double sum = 0.0;
						int n = 0;
						for (int nid : nodeIds)
						{
							if (flatValues.contains(nid)) { sum += flatValues.value(nid); ++n; }
						}
						rec.insert("contourValue", n > 0 ? fmtValue(sum / n) : QStringLiteral("-"));
					}
				}
				if (has("materialName") || has("propertyName") || has("componentName"))
				{
					QList<int> partIds;
					if (partId >= 0) partIds << partId;
					const auto info = modelInfoOf(partIds, true, true, modelData);
					if (has("materialName")) rec.insert("materialName", info.mat);
					if (has("propertyName")) rec.insert("propertyName", info.sec);
					if (has("componentName")) rec.insert("componentName", info.part);
				}
				if (has("loadCase")) rec.insert("loadCase", caseName);
				if (has("simulationStep")) rec.insert("simulationStep", frameName);
				if (has("contourValue") && !rec.contains("contourValue"))
					rec.insert("contourValue", !contourNodal && flatValues.contains(eleId)
						? fmtValue(flatValues.value(eleId)) : QStringLiteral("-"));
				if (has("vectorValue"))
					rec.insert("vectorValue", vectorCellText([vectorNodal, &nodeIds, eleId](
						const QHash<int, double>& comp, double& v)
						{
							if (vectorNodal)
							{
								//节点位变量在单元行 = 单元节点值平均（同云图值口径）
								double sum = 0.0;
								int n = 0;
								for (int nid : nodeIds)
									if (comp.contains(nid)) { sum += comp.value(nid); ++n; }
								if (n == 0)
									return false;
								v = sum / n;
								return true;
							}
							if (comp.contains(eleId))
							{
								v = comp.value(eleId);
								return true;
							}
							return false;
						}));
				records << rec;
			}
		}
		else
		{
			for (int partId : _pickedParts)
			{
				QHash<QString, QString> rec;
				if (has("componentId")) rec.insert("componentId", QString::number(partId));
				if (has("componentName")) rec.insert("componentName", modelData->getPartNameByPartId(partId));
				if (has("materialName") || has("propertyName"))
				{
					const auto info = modelInfoOf(QList<int>{ partId }, true, false, modelData);
					if (has("materialName")) rec.insert("materialName", info.mat);
					if (has("propertyName")) rec.insert("propertyName", info.sec);
				}
				if (has("loadCase")) rec.insert("loadCase", caseName);
				if (has("simulationStep")) rec.insert("simulationStep", frameName);
				if (proxy)
				{
					const std::pair<float, float> minMax = proxy->getPartMinMax(partId);
					if (has("contourMax")) rec.insert("contourMax", fmtValue(minMax.second));
					if (has("contourMin")) rec.insert("contourMin", fmtValue(minMax.first));
					auto joinIds = [](const std::set<int>& ids)
					{
						QStringList list;
						for (int id : ids) list << QString::number(id);
						return list.isEmpty() ? QStringLiteral("-") : list.join(',');
					};
					if (has("contourMaxId")) rec.insert("contourMaxId", joinIds(proxy->getPartMaxID(partId)));
					if (has("contourMinId")) rec.insert("contourMinId", joinIds(proxy->getPartMinID(partId)));
				}
				else
				{
					if (has("contourMax")) rec.insert("contourMax", QStringLiteral("-"));
					if (has("contourMaxId")) rec.insert("contourMaxId", QStringLiteral("-"));
					if (has("contourMin")) rec.insert("contourMin", QStringLiteral("-"));
					if (has("contourMinId")) rec.insert("contourMinId", QStringLiteral("-"));
				}
				if (has("vectorMax")) rec.insert("vectorMax", QStringLiteral("-"));
				if (has("vectorMaxId")) rec.insert("vectorMaxId", QStringLiteral("-"));
				if (has("vectorMin")) rec.insert("vectorMin", QStringLiteral("-"));
				if (has("vectorMinId")) rec.insert("vectorMinId", QStringLiteral("-"));
				records << rec;
			}
		}
		setResults(objectType, records);
	}

	/******** 对外接口与上下文刷新 ********/

	void mPostQueryPanel::updateQueryPanel()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		_postDB = nullptr;
		if (postRend && postRend->getPostRender())
		{
			const QString postName = postRend->getPostDataName();
			if (!postName.isEmpty())
				_postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			//拾取数据到达由拾取栈控制器自管（原 ReadyToReadContainer 直连随替换撤除）
		}
		//云图重绘信号（构造期云图面板可能未就绪，此处兜底重连）
		if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
			connect(cloudPanel.get(), &mPost3DResultPanel::cloudRedrawnSig,
				this, &mPostQueryPanel::slot_cloudRedrawn, Qt::UniqueConnection);
		refreshPickButton();
		refreshFieldSummary();
		refreshResultEntries();
		if (_resultsDock)
		{
			ensureResultsDock();
			refreshDockColumns();
			if (_resultsDock->isVisible())
				repositionResultsDock();
		}
	}

	int mPostQueryPanel::objectType() const
	{
		return _comboObjectType->currentIndex();
	}

	QString mPostQueryPanel::coordinateSystem() const
	{
		return _comboCoordSystem->currentText();
	}

	QStringList mPostQueryPanel::selectedFields() const
	{
		return selectedFieldsFor(objectType());
	}

	QStringList mPostQueryPanel::selectedFieldsFor(int objectType) const
	{
		QStringList keys;
		const FieldDef* catalog = fieldCatalog(objectType);
		for (int i = 0; i < fieldCatalogSize(objectType); ++i)
			if (_selectedFields[objectType].contains(QString::fromLatin1(catalog[i].key)))
				keys << QString::fromLatin1(catalog[i].key);
		return keys;
	}

	void mPostQueryPanel::setResults(int objectType, const QList<QHash<QString, QString>>& records)
	{
		if (objectType < 0 || objectType > 2)
			return;
		ensureResultsDock();
		if (!_resultsDock)
			return;
		_resultsDock->setRows(objectType, records);
		repositionResultsDock();
		_resultsDock->openResults(objectType);
		refreshResultEntries();
	}

	/******** 语言切换 / 退出 ********/

	void mPostQueryPanel::applyLanguageTexts()
	{
		_objectFold->setTitle(_langEn ? QString("Object and Coordinate System") : QString::fromUtf8("查询对象及坐标系"));
		_resultsFold->setTitle(_langEn ? QString("Query Results") : QString::fromUtf8("查询结果"));

		//行标签（addRow 建的 PanelLabel 按布局序重设文本）
		const QList<PanelLabel*> objectLabels = _objectFold->findChildren<PanelLabel*>();
		if (objectLabels.size() >= 3)
		{
			objectLabels.at(0)->setText(_langEn ? QString("Object") : QString::fromUtf8("对象选择"));
			objectLabels.at(1)->setText(_langEn ? QString("Coordinate System") : QString::fromUtf8("坐标系"));
			objectLabels.at(2)->setText(_langEn ? QString("Query Values") : QString::fromUtf8("查询值"));
		}
		const QList<PanelLabel*> resultLabels = _resultsFold->findChildren<PanelLabel*>();
		for (int i = 0; i < resultLabels.size() && i < 3; ++i)
			resultLabels.at(i)->setText(tabTitle(i));

		//对象类型下拉（保持当前索引）
		const int keepType = _comboObjectType->currentIndex();
		_comboObjectType->blockSignals(true);
		_comboObjectType->clear();
		_comboObjectType->addItem(_langEn ? QString("Nodes") : QString::fromUtf8("节点"));
		_comboObjectType->addItem(_langEn ? QString("Elements") : QString::fromUtf8("单元"));
		_comboObjectType->addItem(_langEn ? QString("Parts") : QString::fromUtf8("部件"));
		_comboObjectType->setCurrentIndex(keepType < 0 ? QueryNode : keepType);
		_comboObjectType->blockSignals(false);

		//坐标系（分析坐标系默认；同云图面板静态候选）
		_comboCoordSystem->blockSignals(true);
		const QString keepCoord = _comboCoordSystem->currentText();
		_comboCoordSystem->clear();
		_comboCoordSystem->addItem(_langEn ? QString("Analysis CS") : QString::fromUtf8("分析坐标系"));
		_comboCoordSystem->addItem(_langEn ? QString("Global CS") : QString::fromUtf8("全局坐标系"));
		_comboCoordSystem->setCurrentIndex(0);
		if (!keepCoord.isEmpty() && _comboCoordSystem->findText(keepCoord) >= 0)
			_comboCoordSystem->setCurrentText(keepCoord);
		_comboCoordSystem->blockSignals(false);

		_btnCoordAdd->setToolTip(_langEn ? QString("New coordinate system (not integrated)")
			: QString::fromUtf8("新建坐标系（待接入）"));

		if (_valuesPopupTitle)
			_valuesPopupTitle->setText(_langEn ? QString("Query Values") : QString::fromUtf8("查询值"));
		applyDockTexts();

		refreshPickButton();
		refreshFieldSummary();
		refreshResultEntries();
		if (_valuesPopup && _valuesPopup->isVisible())
			rebuildFieldOptions();
	}

	void mPostQueryPanel::slot_changeLanguage(int language)
	{
		_langEn = (language == 1);
		applyLanguageTexts();
	}

	void mPostQueryPanel::hideEvent(QHideEvent* event)
	{
		//面板退出=收弹窗/停靠窗+拾取条收口（原型 panel hidden 联动；激活态解除+
		//清高亮，弹窗为 Qt::Popup 随宿主隐藏——云图/向量图/阵列/爆炸同款）
		closeValuesPopup();
		if (_resultsDock)
			_resultsDock->closeResults();
		if (_pickController != nullptr)
			_pickController->setPickWidgetVisible(false);
		mPanelBase::hideEvent(event);
	}

	void mPostQueryPanel::showEvent(QShowEvent* event)
	{
		mPanelBase::showEvent(event);
		//隐藏期 setPickWidgetVisible(false) 的对称恢复（已拾取数据保留在控件数据模型，
		//条体显示随数据自动恢复）
		if (_pickController != nullptr)
			_pickController->setPickWidgetVisible(true);
	}
}
