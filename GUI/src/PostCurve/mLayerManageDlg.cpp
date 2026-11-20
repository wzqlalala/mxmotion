#pragma execution_character_set("utf-8")

// =============================================================================
// mLayerManageDlg —— 曲线图层管理对话框（实现），语义见 mLayerManageDlg.h
// 设计依据 openspec add-template-curve D5 + 任务 5.2
// =============================================================================

#include "mLayerManageDlg.h"
#include "mCurveLayerManager.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveCore/CurveDefine.h"
#include "mTreeDelegate.h"   //曲线树同款虚线分支绘制（PaintDashBranches）
#include "ThemeManager.h"   //自样式弹窗豁免标记（kSelfStyledProp）

//QT
#include <QApplication>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include "mUiMenu.h"
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <set>

namespace PostCurve
{
	namespace
	{
		//树节点角色分类（Qt::UserRole+1）：页面分组/曲线层/基准线层（D10 独立层）
		enum NodeRole { NGroup = 0, NLayer = 1, NDatum = 2 };

		//曲线树同款分支样式：drawBranches 画父子虚线连接 + 展开箭头（PaintDashBranches，
		//原生箭头经离屏渲染后贴近节点）——层级有虚线导引、分组可收缩，与曲线结构树一致
		class DashBranchTreeWidget : public QTreeWidget
		{
		public:
			using QTreeWidget::QTreeWidget;
			//缩进与 PaintDashBranches 箭头口径配套（箭头对齐父行符号列且贴近自身内容）
			explicit DashBranchTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) { setIndentation(14); }
		protected:
			void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override
			{
				MechMainWindow::PaintDashBranches(this, painter, rect, index,
					[this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeWidget::drawBranches(p, r, i); });
			}
		};
	}

	mLayerManageDlg* mLayerManageDlg::_instance = nullptr;

	void mLayerManageDlg::showInstance(QWidget* parent)
	{
		//父窗口沿链走到顶层主窗口（对齐 mTemplateCurveDlg 同款处理）
		QWidget* host = parent;
		while (host && host->parentWidget())
			host = host->parentWidget();
		if (!host)
			host = parent;

		if (!_instance)
			_instance = new mLayerManageDlg(host);
		_instance->rebuild();   //重开/已开都现刷（层是会话内存活数据）
		_instance->show();
		_instance->raise();
		_instance->activateWindow();
	}

	mLayerManageDlg::mLayerManageDlg(QWidget* parent)
		: QDialog(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
	{
		//自样式弹窗：豁免 ThemeManager 整窗重发，保留自带设计稿 QSS
		setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
		setWindowTitle(QStringLiteral("图层"));
		resize(500, 410);   //需求图：标题栏 + 显示说明 + 两级树 + 右下按钮（高对齐原型内框 ≈496×404）
		buildUi();
		rebuild();
	}

	void mLayerManageDlg::buildUi()
	{
		//白卡片 + M 徽标标题栏（对齐 mTemplateCurveDlg；QSS 须用 objectName 选择器）
		setObjectName("layerManageDlg");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet("QWidget#layerManageDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(1, 1, 1, 1);
		layout->setSpacing(0);

		_titleBar = new QWidget(this);
		//"mWindowTitleBar" 为 ui-spec §2 约定名：本框是 QDialog，显示时 ThemeManager
		//会整树补发（applyDialogTheme），只有挂此名的标题栏及其直接子件被跳过——
		//标题 13px/600、✗ 无边框 QSS 靠它存活
		_titleBar->setObjectName("mWindowTitleBar");
		_titleBar->setFixedHeight(52);
		_titleBar->setStyleSheet("background:#FFFFFF;");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(20, 20, 20, 10);
		titleLayout->setSpacing(3);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("图层"), _titleBar);
		titleLabel->setStyleSheet(
			"QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
		QToolButton* closeBtn = new QToolButton(_titleBar);
		closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
		closeBtn->setIconSize(QSize(24, 24));
		closeBtn->setToolTip(QStringLiteral("关闭"));
		closeBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
		)");
		connect(closeBtn, &QToolButton::clicked, this, &QDialog::close);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(closeBtn);

		//主体：显示说明 + 两级树 + 右下 关闭/刷新
		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:#FFFFFF;}");
		QVBoxLayout* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(20, 6, 20, 16);
		bodyLayout->setSpacing(8);

		QLabel* caption = new QLabel(QStringLiteral("显示:"), body);
		caption->setStyleSheet(
			"QLabel{color:#333333; font-size:12px; background:transparent;}");
		bodyLayout->addWidget(caption);

		//两级树：Page(s) 组 → "Layer N: 层名"；蓝色勾选指示器（check1x.png，
		//对齐模板曲线对话框）、选中行浅蓝；分支绘制用曲线树同款（虚线导引+可收缩）
		_tree = new DashBranchTreeWidget(body);
		_tree->setHeaderHidden(true);
		_tree->setColumnCount(1);
		_tree->setContextMenuPolicy(Qt::CustomContextMenu);
		//ui-spec §2：本框显示时会被 ThemeManager 整树补发，QTreeWidget 分支会把
		//控件自带 QSS 整体替换（行高 24→26、勾选指示器 check1x 与选中浅蓝全被抹）。
		//需求明确树形态（原型蓝勾指示器/浅蓝选中/24 行高），按约定属性豁免
		//（ThemeManager 对应分支 mx*NoTheme 短路，勿在调用点对抗主题）
		_tree->setProperty("mxTreeNoTheme", true);
		_tree->setStyleSheet(R"(
			QTreeWidget{
				background:#FFFFFF; border:1px solid #D9DCE0; border-radius:4px;
				font-size:12px; color:#333333;
			}
			QTreeWidget::item{ height:24px; }
			QTreeWidget::item:selected{ background:#CCE8FF; color:#333333; }
			QTreeWidget::indicator{ width:14px; height:14px;
				border:1px solid #C9CDD4; border-radius:3px; background:#FFFFFF; }
			QTreeWidget::indicator:hover{ border-color:#0066CC; }
			QTreeWidget::indicator:checked{ border:none; background:transparent;
				image:url(:/mMainWidgets/Icon/check1x.png); }
		)");
		connect(_tree, &QTreeWidget::itemChanged,
			this, &mLayerManageDlg::onItemChanged);
		connect(_tree, &QTreeWidget::customContextMenuRequested,
			this, &mLayerManageDlg::onContextMenu);
		bodyLayout->addWidget(_tree, 1);

		QHBoxLayout* buttons = new QHBoxLayout();
		buttons->setSpacing(12);
		const QString btnStyle = R"(
			QPushButton{
				background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
				color:#333333; padding:2px 14px;
			}
			QPushButton:hover{ background:#EFF1F2; }
		)";
		QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"), body);
		refreshBtn->setFixedSize(60, 26);
		refreshBtn->setStyleSheet(btnStyle);
		connect(refreshBtn, &QPushButton::clicked, this, [this]() { rebuild(); });
		QPushButton* closeDlgBtn = new QPushButton(QStringLiteral("关闭"), body);
		closeDlgBtn->setFixedSize(60, 26);
		closeDlgBtn->setStyleSheet(btnStyle);
		connect(closeDlgBtn, &QPushButton::clicked, this, &QDialog::close);
		buttons->addStretch(1);
		buttons->addWidget(closeDlgBtn);   //需求图：关闭 在左
		buttons->addWidget(refreshBtn);    //刷新 在右
		//原型实测（内框 480×388 → 对话框 500×410，×1.0567）：树卡底边框 y334 →
		//按钮行顶 y355，白隙 20 行 ≈21px；布局 spacing 8 只给 8px，补 13 凑足气口
		bodyLayout->addSpacing(13);   //树卡底→按钮行气口 21（8+13）
		bodyLayout->addLayout(buttons);

		layout->addWidget(_titleBar);
		layout->addWidget(body, 1);
	}

	void mLayerManageDlg::rebuild()
	{
		//展开态记忆：清树前按节点 id 收集，重建后恢复
		_expandedIds.clear();
		for (QTreeWidgetItemIterator it(_tree); *it; ++it)
		{
			if ((*it)->isExpanded())
				_expandedIds.insert((*it)->data(0, Qt::UserRole).toInt());
		}

		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//页 id -> 位置序（1 基，按根下 Page 顺序；分组行页号与视图页序同源）
		QHash<int, int> pageNo;
		const QList<int> rootPages = doc->childNodeIds(doc->rootNodeId());
		for (int i = 0; i < rootPages.size(); ++i)
			pageNo.insert(rootPages.at(i), i + 1);

		_building = true;
		_tree->clear();

		//按起始页归组（层 pageIds 首个存活页；页全删归入"（页面已删）"兜底组）。
		//两级树（需求图）：组节点="Page(s): 页号列表"（可勾选），子节点基准线层
		//置顶（按生成序，D10）、曲线层在后（按应用序）——两遍扫 layers()
		QHash<int, QTreeWidgetItem*> groupNodes;
		//跨组合并基准线层的独立页面分组节点（D10 v3：键=层 id，与起始页键
		//分图存储防正整数碰撞；标签/勾选/展开通道与起始页组通用）
		QHash<int, QTreeWidgetItem*> datumGroupNodes;
		QTreeWidgetItem* orphan = nullptr;
		const QList<mCurveLayerManager::LayerInfo> layers =
			mCurveLayerManager::getInstance()->layers();

		//组条目获取（惰性建）
		auto groupFor = [this, &groupNodes, &datumGroupNodes, &orphan](
			const mCurveLayerManager::LayerInfo& info) -> QTreeWidgetItem*
		{
			if (info.ownGroup)
			{
				//跨组合并的基准线层（D10 v3）：升独立页面分组节点单独挂载
				//（脱离原组与当前组），标签走下方页号并集构建
				if (!datumGroupNodes.contains(info.id))
				{
					QTreeWidgetItem* group = new QTreeWidgetItem(_tree);
					group->setData(0, Qt::UserRole, info.id);
					group->setData(0, Qt::UserRole + 1, NGroup);
					group->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
					datumGroupNodes.insert(info.id, group);
				}
				return datumGroupNodes.value(info.id);
			}
			const int startPage = info.pageIds.isEmpty() ? -1 : info.pageIds.first();
			if (startPage >= 0)
			{
				if (!groupNodes.contains(startPage))
				{
					QTreeWidgetItem* group = new QTreeWidgetItem(_tree);
					group->setData(0, Qt::UserRole, startPage);
					group->setData(0, Qt::UserRole + 1, NGroup);
					group->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
					groupNodes.insert(startPage, group);
				}
				return groupNodes.value(startPage);
			}
			if (!orphan)
			{
				orphan = new QTreeWidgetItem(_tree);
				orphan->setText(0, QStringLiteral("（页面已删）"));
				orphan->setData(0, Qt::UserRole + 1, NGroup);
				orphan->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			}
			return orphan;
		};

		//层条目构建：文本/角色/覆盖页/勾选态（任一成员可见=勾，成员=曲线+基准线，
		//spec 层勾选态现算口径；覆盖页号挂 UserRole+2 供组节点标签汇总）
		auto makeLayerItem = [this, &doc, &groupFor](
			const mCurveLayerManager::LayerInfo& info, int role, const QString& text)
		{
			bool anyVisible = false;
			for (int curveId : info.curveIds)
				anyVisible = anyVisible
					|| doc->nodeProperty(curveId, CurveProp::kVisible).toBool();
			for (int baselineId : info.baselineIds)
				anyVisible = anyVisible
					|| doc->nodeProperty(baselineId, CurveProp::kVisible).toBool();

			QTreeWidgetItem* layerItem = new QTreeWidgetItem(groupFor(info));
			layerItem->setText(0, text);
			layerItem->setData(0, Qt::UserRole, info.id);
			layerItem->setData(0, Qt::UserRole + 1, role);
			layerItem->setData(0, Qt::UserRole + 2, QVariant::fromValue(info.pageIds));
			layerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			layerItem->setCheckState(0, anyVisible ? Qt::Checked : Qt::Unchecked);
		};

		//第一遍：基准线层置顶（layers() 应用序即 datumNo 升序=组内按生成序，D10）；
		//行文本沿用 "Layer N: 名" 格式但编号取负（Layer -1/-2…，与曲线层正编号
		//视觉分序；层名固定裸名，负号仅树行显示，datumNo 本身仍为正）
		for (const mCurveLayerManager::LayerInfo& info : layers)
		{
			if (info.datumNo <= 0)
				continue;
			makeLayerItem(info, NDatum, QStringLiteral("Layer -%1: %2")
				.arg(info.datumNo).arg(info.name));
		}
		//第二遍：曲线层（显示序=应用序全局递增，与基准线层编号分序互不相干）
		int curveLayerNo = 0;
		for (const mCurveLayerManager::LayerInfo& info : layers)
		{
			if (info.datumNo > 0)
				continue;
			makeLayerItem(info, NLayer, QStringLiteral("Layer %1: %2")
				.arg(++curveLayerNo).arg(info.name));
		}

		//组节点标签="Page(s): 组内层覆盖页号并集（升序）"（需求图形态），
		//勾选态=任一子层可见
		for (int i = 0; i < _tree->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* group = _tree->topLevelItem(i);
			std::set<int> nums;
			for (int j = 0; j < group->childCount(); ++j)
			{
				//存取必须同型：写入是 QVector<int>（LayerInfo::pageIds），Qt5 下
				//QVector<int> 与 QList<int> 是不同元类型且无自动转换，异型 value<>
				//恒返回空表——组标签页号并集恒空，即验收 P2"裸 Page(s):"的根因
				const auto pages =
					group->child(j)->data(0, Qt::UserRole + 2).value<QVector<int>>();
				for (int pid : pages)
				{
					if (pageNo.contains(pid))
						nums.insert(pageNo.value(pid));
				}
			}
			if (group->data(0, Qt::UserRole).toInt() > 0)
			{
				QStringList numTexts;
				for (int n : nums)
					numTexts << QString::number(n);
				group->setText(0, QStringLiteral("Page(s): %1")
					.arg(numTexts.join(QStringLiteral(", "))));
			}
			bool anyVisible = false;
			for (int j = 0; j < group->childCount(); ++j)
				anyVisible = anyVisible
					|| group->child(j)->checkState(0) == Qt::Checked;
			group->setCheckState(0, anyVisible ? Qt::Checked : Qt::Unchecked);
		}

		//恢复展开态（收集在挂树前展开的分组）；无记忆时组节点默认全展开（需求图）
		for (int i = 0; i < _tree->topLevelItemCount(); ++i)
		{
			QTreeWidgetItem* group = _tree->topLevelItem(i);
			if (_expandedIds.contains(group->data(0, Qt::UserRole).toInt()))
				group->setExpanded(true);
		}
		if (_expandedIds.isEmpty())
		{
			for (int i = 0; i < _tree->topLevelItemCount(); ++i)
				_tree->topLevelItem(i)->setExpanded(true);
		}
		_building = false;
	}

	void mLayerManageDlg::onItemChanged(QTreeWidgetItem* item, int column)
	{
		if (_building || column != 0 || !item)
			return;
		const int nodeId = item->data(0, Qt::UserRole).toInt();
		const int role = item->data(0, Qt::UserRole + 1).toInt();
		const bool checked = item->checkState(0) == Qt::Checked;
		if (role == NGroup)
		{
			//组勾：组内全部层整体显/隐；子层勾选态就地刷新（护栏内防回声）
			_building = true;
			for (int i = 0; i < item->childCount(); ++i)
			{
				QTreeWidgetItem* layerItem = item->child(i);
				mCurveLayerManager::getInstance()->setCurvesVisible(
					layerItem->data(0, Qt::UserRole).toInt(), checked);
				layerItem->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
			}
			_building = false;
		}
		else if ((role == NLayer || role == NDatum) && nodeId > 0)
		{
			//层勾（曲线层/基准线层）：成员整体显/隐（写 kVisible 走命令栈，
			//基准线层由 5.3 扩展覆盖到基准线）；父组勾选态（任一子层可见）就地跟随
			mCurveLayerManager::getInstance()->setCurvesVisible(nodeId, checked);
			if (QTreeWidgetItem* p = item->parent())
			{
				bool anyVisible = false;
				for (int i = 0; i < p->childCount(); ++i)
					anyVisible = anyVisible || p->child(i)->checkState(0) == Qt::Checked;
				if ((p->checkState(0) == Qt::Checked) != anyVisible)
					p->setCheckState(0, anyVisible ? Qt::Checked : Qt::Unchecked);
			}
		}
	}

	void mLayerManageDlg::onContextMenu(const QPoint& pos)
	{
		QTreeWidgetItem* item = _tree->itemAt(pos);
		const int role = item ? item->data(0, Qt::UserRole + 1).toInt() : NGroup;
		if (role != NLayer && role != NDatum)
			return;   //层节点（曲线层/基准线层）带管理菜单；分组行不带
		const int layerId = item->data(0, Qt::UserRole).toInt();
		if (layerId <= 0)
			return;

		MechMainWindow::mUiMenu menu(this);
		QAction* renameAct = menu.addAction(QStringLiteral("重命名"));
		QAction* deleteAct = menu.addAction(QStringLiteral("删除"));
		QAction* act = menu.exec(_tree->viewport()->mapToGlobal(pos));
		if (act == renameAct)
		{
			bool ok = false;
			const QString name = QInputDialog::getText(this,
				QStringLiteral("重命名图层"), QStringLiteral("图层名称："),
				QLineEdit::Normal, item->text(0).section(QLatin1Char(':'), 1).trimmed(),
				&ok);
			if (ok && !name.trimmed().isEmpty())
			{
				mCurveLayerManager::getInstance()->renameLayer(layerId, name.trimmed());
				rebuild();
			}
		}
		else if (act == deleteAct)
		{
			if (QMessageBox::question(this, QStringLiteral("图层"),
				QStringLiteral("删除图层将同时删除其全部曲线/基准线（一次 Ctrl+Z 可整体撤销），确定删除？"))
				!= QMessageBox::Yes)
				return;
			QString err;
			if (!mCurveLayerManager::getInstance()->removeLayer(layerId, &err))
				QMessageBox::warning(this, QStringLiteral("图层"), err);
			rebuild();
		}
	}

	/*************************** 标题栏拖动（对齐 mTemplateCurveDlg） ***************************/

	void mLayerManageDlg::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton && _titleBar->underMouse())
			_pressPos = event->globalPos();
		QDialog::mousePressEvent(event);
	}

	void mLayerManageDlg::mouseMoveEvent(QMouseEvent* event)
	{
		if ((event->buttons() & Qt::LeftButton) && !_dragging
			&& _titleBar->underMouse()
			&& (event->globalPos() - _pressPos).manhattanLength()
				>= QApplication::startDragDistance())
		{
			_dragging = true;
			_dragOffset = event->globalPos() - frameGeometry().topLeft();
		}
		if (_dragging && (event->buttons() & Qt::LeftButton))
			move(event->globalPos() - _dragOffset);
		QDialog::mouseMoveEvent(event);
	}

	void mLayerManageDlg::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragging = false;
		QDialog::mouseReleaseEvent(event);
	}
}
