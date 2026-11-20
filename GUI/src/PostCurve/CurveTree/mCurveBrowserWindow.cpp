#include "mCurveBrowserWindow.h"
#include "mCurveTreeView.h"
#include "../CurveCore/CurveDefine.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QToolButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>
#include <QVector>

namespace PostCurve
{
	namespace
	{
		//筛选按钮定义（UI设计图01.2，面板00~06）：图标 + tooltip + 允许显示的节点类型
		//集合（空=完整树不过滤）。createFilterBar 出按钮、slot_filterChanged 查同一张表。
		//经函数内惰性静态取表：命名空间级 QVector 静态在 DLL 加载/卸载时机构造析构，
		//存在打开/关闭时机的初始化次序隐患（函数内 static 首调构造，C++11 保证线程安全）
		//R41 三维基准面处置备案：本表不新增"基准面"分组（本期最小改动）——
		//DatumPlane3D 节点仅"全部"（空集合=完整树）视图可见；按曲线/注释/基准线/
		//坐标轴等分类筛选时基准节点被隐藏（不在任何允许集内，rowAcceptedByType
		//拒绝且非骨架类型），选中/交互不受影响（clearSelectionIfFilteredOut 兜底）。
		//后续需要独立分组时在表尾追加即可（按钮行布局自适应）
		struct CurveFilterDef
		{
			const char* icon;               //图片路径
			const char* toolTip;            //没有文字后，tooltip 必须保留
			QVector<CurveNodeType> types;   //空=显示全部节点
		};
		const QVector<CurveFilterDef>& curveFilterDefs()
		{
			static const QVector<CurveFilterDef> defs = {
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/绘图.svg",     "曲线完整结构树（显示全部节点）", {} },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/曲线.svg",   "按曲线筛选（含曲线 x/y 数据节点）",
				  { CurveNodeType::Curve, CurveNodeType::CurveXData, CurveNodeType::CurveYData, CurveNodeType::CurveY2Data } },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/图例.svg",  "按图例筛选", { CurveNodeType::Legend } },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/注释.svg",    "按注释筛选", { CurveNodeType::Annotation } },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/基准.svg", "按基准线筛选", { CurveNodeType::Baseline } },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/坐标轴.svg",    "按坐标轴筛选",
				  { CurveNodeType::XAxis, CurveNodeType::YAxis, CurveNodeType::YAxis2, CurveNodeType::XAxis2,
				    CurveNodeType::AxisX3DData, CurveNodeType::AxisY3DData, CurveNodeType::AxisZ3DData } },
				{ ":/mMainWidgets/Icon/Curve/结构树筛选/页眉页脚.svg",  "按页眉页脚筛选",
				  { CurveNodeType::Header, CurveNodeType::Footer } },
			};
			return defs;
		}
	}

	mCurveBrowserWindow* mCurveBrowserWindow::_instance = nullptr;

	mCurveBrowserWindow* mCurveBrowserWindow::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mCurveBrowserWindow;
			_instance->setObjectName("CurveTreeDock");
			_instance->setMinimumWidth(265);
		}
		return _instance;
	}

	mCurveBrowserWindow::mCurveBrowserWindow(QWidget* parent) : QWidget(parent)
	{
		QPalette pal;
		pal.setColor(QPalette::Background, QColor(255, 255, 255));
		this->setAutoFillBackground(true);
		this->setPalette(pal);

		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(4, 4, 4, 4);
		mainLayout->setSpacing(4);

		//页下拉框（视图显示栏页下拉同款样式，由全局 QSS 统一出图）：
		//列出曲线页快速切换，选中即树上选中该页节点（联动切换对应视图页）
		_pageCombo = new QComboBox(this);
		_pageCombo->setMinimumWidth(200);
		mainLayout->addWidget(_pageCombo);
		connect(_pageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mCurveBrowserWindow::slot_pageComboChanged);

		//节点类型筛选按钮行（UI设计图01.2：单选图标按钮；图标资源暂缺，以文字替代）
		mainLayout->addWidget(createFilterBar());

		//名称搜索框（UI设计图01.2：占位"搜索"，与类型筛选叠加过滤）
		_searchEdit = new QLineEdit(this);
		_searchEdit->setPlaceholderText(QStringLiteral("搜索"));
		_searchEdit->setClearButtonEnabled(true);
		connect(_searchEdit, &QLineEdit::textChanged, this, &mCurveBrowserWindow::slot_searchChanged);
		mainLayout->addWidget(_searchEdit);

		//曲线树
		_treeView = new mCurveTreeView(this);

		mainLayout->addWidget(_treeView, 1);
		setLayout(mainLayout);

		//页下拉框与文档联动：页增删/改名重列选项，选中节点变化回显所属页
		//（仅页节点增删与页改名需要重列，图表/曲线等节点事件不扰动下拉框）
		auto* controller = mCurveDocController::getInstance();
		connect(controller, &mCurveDocController::sig_nodeAdded, this, [this](int nodeId)
		{
			if (mCurveDocController::getInstance()->document()->nodeType(nodeId) == CurveNodeType::Page)
				refreshPageCombo();
		});
		connect(controller, &mCurveDocController::sig_nodeAboutToBeRemoved, this, [this](int nodeId)
		{
			if (mCurveDocController::getInstance()->document()->nodeType(nodeId) == CurveNodeType::Page)
				refreshPageCombo();
		});
		connect(controller, &mCurveDocController::sig_nodePropertyChanged, this, [this](int nodeId, const QString& prop)
		{
			Q_UNUSED(nodeId);
			if (prop == CurveProp::kName)
				refreshPageCombo();
		});
		connect(controller, &mCurveDocController::sig_currentNodeChanged, this, &mCurveBrowserWindow::refreshPageCombo);
		refreshPageCombo();
	}

	mCurveBrowserWindow::~mCurveBrowserWindow()
	{
	}

    QWidget* mCurveBrowserWindow::createFilterBar()
    {
        //按钮顺序与筛选类型同 UI设计图01.2（面板00~06）：
        //完整树/曲线/图例/注释/基准线/坐标轴/页眉页脚（定义见 curveFilterDefs）
        const QVector<CurveFilterDef>& defs = curveFilterDefs();
        const int count = defs.size();

        QWidget* bar = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(bar);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        //样式表基本不用动：hover/checked 的背景色继续负责状态反馈；
        //只删掉了 color 和 font-weight（没有文字了，这两条失效）
        bar->setStyleSheet(QStringLiteral(R"(
		QToolButton {
			background: transparent;
			border: 1px solid transparent;
			border-radius: 2px;
			padding: 2px 4px;
		}
		QToolButton:hover {
			background-color: #d0e6ff;
			border: 1px solid #a0c8ff;
		}
		QToolButton:checked {
			background-color: #a0c8ff;
			border: 1px solid #6da8ff;
		}
	)"));

        _filterGroup = new QButtonGroup(this);
        _filterGroup->setExclusive(true);
        for (int i = 0; i < count; ++i)
        {
            QToolButton* btn = new QToolButton(bar);
            btn->setIcon(QIcon(QString::fromUtf8(defs.at(i).icon)));
            btn->setIconSize(QSize(20, 20));                 //按切图实际尺寸调整
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly); //只显示图片，不显示文字
            btn->setToolTip(QString::fromUtf8(defs.at(i).toolTip));
            btn->setCheckable(true);
            _filterGroup->addButton(btn, i);
            layout->addWidget(btn);
        }
        layout->addStretch(1);

        _filterGroup->button(0)->setChecked(true);   //默认完整树（设计稿面板00）
        connect(_filterGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &mCurveBrowserWindow::slot_filterChanged);
        return bar;
    }

	void mCurveBrowserWindow::slot_pageComboChanged(int index)
	{
		if (index < 0 || !_pageCombo || !_treeView)
			return;
		const int pageId = _pageCombo->itemData(index).toInt();
		//树上选中该页节点：经选择中枢联动（工厂据此切换对应视图页，链路同树上点选页）
		if (pageId > 0)
			_treeView->selectNode(pageId);
	}

	void mCurveBrowserWindow::refreshPageCombo()
	{
		if (!_pageCombo)
			return;
		//重列曲线页（文档序），并回显当前选中节点所属页（无页归属保持现状）
		_pageCombo->blockSignals(true);
		_pageCombo->clear();
		auto* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();
		for (int pageId : doc->childNodeIds(doc->rootNodeId()))
		{
			if (doc->nodeType(pageId) != CurveNodeType::Page)
				continue;
			_pageCombo->addItem(doc->nodeDisplayName(pageId), pageId);
		}
		int cur = controller->currentNodeId();
		while (cur > 0 && doc->node(cur) && doc->nodeType(cur) != CurveNodeType::Page)
			cur = doc->parentNodeId(cur);
		if (cur > 0 && doc->node(cur))
		{
			const int idx = _pageCombo->findData(cur);
			if (idx >= 0)
				_pageCombo->setCurrentIndex(idx);
		}
		_pageCombo->blockSignals(false);
	}

	mCurveTreeView* mCurveBrowserWindow::getCurveTreeView() const
	{
		return _treeView;
	}

	void mCurveBrowserWindow::slot_filterChanged(int buttonId)
	{
		if (!_treeView)
			return;
		mCurveTreeFilterProxy* proxy = _treeView->proxyModel();
		if (!proxy)
			return;

		//筛选类型集合查表（curveFilterDefs；类型集合空 = 完整树，clearFilter）
		QSet<int> types;
		const QVector<CurveFilterDef>& defs = curveFilterDefs();
		if (buttonId >= 0 && buttonId < defs.size())
		{
			for (CurveNodeType t : defs.at(buttonId).types)
				types.insert(static_cast<int>(t));
		}
		if (types.isEmpty())
			proxy->clearFilter();
		else
			proxy->setAllowedTypes(types);

		//筛选只影响可见性；切换后原选中节点若已被筛掉（不可见），
		//清空树选中并写空选择中枢，参数栏/图上高亮随之复位；
		//仍可见的节点（含骨架）选中保持
		_treeView->clearSelectionIfFilteredOut();
	}

	void mCurveBrowserWindow::slot_searchChanged(const QString& text)
	{
		if (!_treeView)
			return;
		mCurveTreeFilterProxy* proxy = _treeView->proxyModel();
		if (proxy)
			proxy->setFilterText(text.trimmed());
	}

	void mCurveBrowserWindow::slot_changeLanguage(int language)
	{
		Q_UNUSED(language)
		//TODO：多语言支持（骨架阶段预留）
	}
}
