#include "mBrowserWindow.h"
#include "mMechGlobalSignals.h"	//树信号挂起/恢复接线（mViewTreeFunction视口激活包裹用）
#include "mCenterWindowProvider.h"
#include "mTreeModel.h"	//mTreeView.h传递提供mTreeModel相关类型（browserTreeModelProvider转交已废止）
#include "mTreeView.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include <QGridLayout>
#include "mTitleBar.h"
#include <QFocusEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include "mTreeView.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"	//refreshToolBarTheme 经共享入口统一下发（与后处理树窗口同一实现）


using namespace MxFunctions;

namespace MechMainWindow
{
	mBrowserWindow* mBrowserWindow::_instance = nullptr;

	mBrowserWindow* mBrowserWindow::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mBrowserWindow;
			//_instance->setTitleBarWidget(new TitleBarWidget(_instance, "模型树"));
			_instance->setObjectName("BrowserDock");
			_instance->setMinimumWidth(265);
		}
		return _instance;
	}

	mBrowserWindow::mBrowserWindow(QWidget* parent) :QWidget(parent)
	{
		//设置Dock名称
		//设置允许的位置
		//this->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
		// 窗口底色 = surface（规则 0.2），随主题切换（原固定白色）
		QPalette pal;
		pal.setColor(QPalette::Background, UiPalette::surface());
		this->setAutoFillBackground(true);
		this->setPalette(pal);
		//初始化前处理模型树
		//_browserTree = new mBrowserTree(this);
        _treeView = new mTreeView(this);
        //后处理树已改用独立模型（PostWindow::mPostTreeModel），browserTreeModelProvider 转交通道废止
        //mViewTreeFunction(居GUIFoundation)视口激活包裹用的树信号挂起/恢复经总线直连
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::suspendBrowserTreeSig, _treeView, &mTreeView::disconnectSig);
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::resumeBrowserTreeSig, _treeView, &mTreeView::connectSig);

		QLabel* _filter = new QLabel("过滤：",this);
        _filter->setFixedWidth(40);
        _filterLabel = _filter;
        // 标签/下拉/输入框样式由构造末尾的 refreshToolBarTheme 统一下发，不在此自设

		_nameComboBox = new QComboBox(this);
		_nameComboBox->addItems({ "名称", "状态" });
		_findLineEdit = new QLineEdit(this);
		_findLineEdit->setPlaceholderText("搜索概要");

		_statusComboBox = new QComboBox(this);
		_statusComboBox->addItems({ "全部", "正确", "警告", "错误"});

		_stackedWidget = new QStackedWidget(this);
		_stackedWidget->addWidget(_findLineEdit);
		_stackedWidget->addWidget(_statusComboBox);

		QLabel* _sortLabel = new QLabel("排序：", this);
        _sortLabel->setFixedWidth(40);
        this->_sortLabel = _sortLabel;
		_sortTypeComboBox = new QComboBox(this);
		_sortTypeComboBox->addItems({ "拼音/字母", "后缀序号" });

		_sortTool = new QToolBar(this);
        _sortTool->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		QAction* action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/moren.png"),"默认");
		connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(0,Qt::AscendingOrder);
			});
		action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/shengxu.png"), "升序");
		connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(_sortTypeComboBox->currentIndex() + 1, Qt::AscendingOrder);
			});
		action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/jiangxu.png"), "降序");
		connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(_sortTypeComboBox->currentIndex() + 1, Qt::DescendingOrder);
			});
        
        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(5);
        gridLayout->setContentsMargins(15, 0, 0, 0);

        QHBoxLayout* searchLayout = new QHBoxLayout();
        searchLayout->addWidget(_stackedWidget);
        searchLayout->addStretch(1);

        QHBoxLayout* toolLayout = new QHBoxLayout();
        toolLayout->addWidget(_sortTool);
        toolLayout->addStretch(1);

        gridLayout->addWidget(_filter, 0, 0, Qt::AlignVCenter);
        gridLayout->addWidget(_nameComboBox, 0, 1, Qt::AlignVCenter);
        gridLayout->addLayout(searchLayout, 0, 2, Qt::AlignVCenter);
        gridLayout->addWidget(_sortLabel, 1, 0, Qt::AlignVCenter);
        gridLayout->addWidget(_sortTypeComboBox, 1, 1, Qt::AlignVCenter);
        gridLayout->addLayout(toolLayout, 1, 2, Qt::AlignVCenter);

        gridLayout->setColumnStretch(0, 0);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 1);

        _nameComboBox->setMinimumWidth(70);
        _sortTypeComboBox->setMinimumWidth(70);
        _findLineEdit->setMinimumWidth(90);
        _sortTool->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        _sortTool->setFixedHeight(UiMetrics::controlHeight());

        // 主布局
        QHBoxLayout* h = new QHBoxLayout;
        h->addSpacing(10);
        h->addWidget(_treeView);
        QVBoxLayout* _mainLayout = new QVBoxLayout(this);
        _mainLayout->addLayout(gridLayout, 0);  // 用 gridLayout 替换 _layout1/_layout2
        _mainLayout->setContentsMargins(0, 5, 0, 0);
        _mainLayout->addLayout(h, 1);

        // 首次按当前档位/主题下发工具条样式（此后由 ThemeManager::applyAll 统一重发）
        refreshToolBarTheme();

		connect(_nameComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](int index) {
				_findLineEdit->clear();
				_statusComboBox->blockSignals(true);
				_statusComboBox->setCurrentIndex(0);
				_statusComboBox->blockSignals(false);

				if (index == 0)
					_stackedWidget->setCurrentIndex(0);
				else
					_stackedWidget->setCurrentIndex(1);

                _treeView->nameFilterTree(_findLineEdit->text());
			});

		connect(_findLineEdit, &QLineEdit::editingFinished, this,
			[this]() {
				_treeView->nameFilterTree(_findLineEdit->text());
			});

		connect(_statusComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](int index) {
				if (index == 0)
                    _treeView->nameFilterTree("");
				else if(index == 1)
                    _treeView->statusFilterTree(Tree_Yes);
				else if (index == 2)
                    _treeView->statusFilterTree(Tree_Failure);
				else if (index == 3)
                    _treeView->statusFilterTree(Tree_Update);
			});
        _treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        this->setMinimumSize(320, 400);
	}

	void mBrowserWindow::refreshToolBarTheme()
	{
		// 过滤/排序控件的主题/档位统一下发（规则 0.1/0.2/2.1）：与后处理树窗口
		// mPostBrowserWindow 同构，共用 ThemeManager::applyTreeFilterBarTheme 一份实现。
		// 幂等：构造期调用一次，ThemeManager::applyAll 遍历树时再调。
		ThemeManager::TreeFilterBar bar;
		bar.nameCombo = _nameComboBox;
		bar.statusCombo = _statusComboBox;
		bar.sortTypeCombo = _sortTypeComboBox;
		bar.findEdit = _findLineEdit;
		bar.filterLabel = _filterLabel;
		bar.sortLabel = _sortLabel;
		bar.sortTool = _sortTool;
		bar.paletteHost = this;
		ThemeManager::applyTreeFilterBarTheme(bar);
	}

	mBrowserWindow::~mBrowserWindow()
	{
		if (_findLineEdit)
		{
			delete _findLineEdit;
			_findLineEdit = nullptr;
		}
		if (_nameComboBox)
		{
			delete _nameComboBox;
			_nameComboBox = nullptr;
		}
		if (_statusComboBox)
		{
			delete _statusComboBox;
			_statusComboBox = nullptr;
		}

		if (_sortTypeComboBox)
		{
			delete _sortTypeComboBox;
			_sortTypeComboBox = nullptr;
		}
		if (_treeView)
		{
			delete _treeView;
            _treeView = nullptr;
		}
		if (_stackedWidget)
		{
			delete _stackedWidget;
			_stackedWidget = nullptr;
		}
	}

    mTreeView* mBrowserWindow::getBrowserTree()
	{
		return _treeView;
	}

	void mBrowserWindow::slot_changeLanguage(int language)
	{
		//if (language == 0)
		//{
		//	this->setDockWindowTitle(QString("模型树"));

		//}
		//else if (language == 1)
		//{
		//	this->setDockWindowTitle(QString("Model tree"));
		//}
	}
}