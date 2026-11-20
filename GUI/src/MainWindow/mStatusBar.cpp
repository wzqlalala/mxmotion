#include "mStatusBar.h"
#include "mFramelessMessageBox.h"
#include "mUiMenu.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mSelectInfoWindow.h"
#include "mOutputInfoWindow.h"
#include "mDockLayoutManager.h"
#include "mViewportManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mInfoWindow.h"
#include "UiPalette.h"   //状态栏底色/文字随主题（规则 0.2）
#include "ThemeManager.h"//单位菜单 QSS 统一入口
//Qt
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QShowEvent>

//单位系统
#include "mUnitSystemManager.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace MechPreData;

namespace
{
	// 单位菜单在状态栏底边展开，Qt 默认会让选中行对齐鼠标，导致菜单向下遮住按钮。
	// QToolButton::showMenu 会在 aboutToShow 之后重新计算默认位置，因此须在 Show
	// 事件中按完整菜单高度上移，保证每次打开菜单的底边都贴着按钮顶边。
	class UnitMenu final : public MechMainWindow::mUiMenu
	{
	public:
		explicit UnitMenu(QToolButton* anchor, QWidget* parent = nullptr)
			: MechMainWindow::mUiMenu(parent), _anchor(anchor)
		{
		}

	protected:
		void showEvent(QShowEvent* event) override
		{
			MechMainWindow::mUiMenu::showEvent(event);
			if (_anchor)
				move(_anchor->mapToGlobal(QPoint(0, -sizeHint().height())));
		}

	private:
		QPointer<QToolButton> _anchor;
	};

	// 重写样式表后强制卸载/重挂样式并刷新——Qt 对 setStyleSheet 的样式缓存
	// 在控件已抛光（polished）时可能不重排背景，表现为"点击后颜色才变"
	void repolish(QWidget* w)
	{
		if (!w) return;
		if (QStyle* s = w->style())
		{
			s->unpolish(w);
			s->polish(w);
		}
		w->update();
	}
    int unitButtonMinWidth(const QToolButton* btn)
    {
        QFont f(UiMetrics::uiFontFamily());
        f.setPixelSize(UiMetrics::bodyFontSize());
        const QFontMetrics fm(f);
        return btn->iconSize().width() + 4
            + fm.horizontalAdvance(btn->text())
            + 2 * fm.horizontalAdvance(QLatin1Char(' '))
            + 5; // QSS padding-right（applyTheme 单位按钮规则）
    }
}

namespace MechMainWindow
{
    mStatusBar::mStatusBar(QWidget* parent):QStatusBar(parent)
    {
        QWidget* container = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->setContentsMargins(5, 0, 5, 0);
        layout->setSpacing(10);

		_timer = new QTimer(this);
		connect(_timer, SIGNAL(timeout()), this, SLOT(slot_clearMessage()));

		//进度条
		_progressBar = new QProgressBar();
		_progressBar->setRange(0, 100);
		_progressBar->setAlignment(Qt::AlignCenter);
		_progressBar->setFixedWidth(200);
		_progressBar->hide();
		connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::setProgressBarValue, this, &mStatusBar::setProgressBarValue);

		layout->addWidget(_progressBar);
		layout->addStretch();

		// --- 左侧信息 Label ---
		infoLabel = new QLabel(this);
		infoLabel->setText(""); // 初始为空
		infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		layout->addWidget(infoLabel, 1);  // 添加到左侧（非 permanent）

        // 消息 Label
        msgLabel = createClickableLabel("消息: 无");
        msgLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        layout->addWidget(msgLabel);
        connect(msgLabel, &QLabel::linkActivated, this, &mStatusBar::showMessageDialog);

        // 选择 Label
        selLabel = createClickableLabel("选择: 无");
        selLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        layout->addWidget(selLabel);
        connect(selLabel, &QLabel::linkActivated, this, &mStatusBar::showSelectionDialog);

		//单位制
	auto* _menu = getUnitsMenu();
	if(unitButton)
        	unitButton->setMenu(_menu);
	if(layout)
        	layout->addWidget(unitButton);

        // 容器策略：最小宽度，自动右对齐
        container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

        // 直接加到状态栏右边
        addPermanentWidget(container, 1);

        // 状态栏钉死 24px：Qt 内部结构为 3(上spacing)+内容行高+2(下spacing)，
        // 内容行高由单位按钮 QSS 19px 撑起（3+19+2=24）。不固定时 strut 会被
        // 全局 QToolButton 的 24px(controlIn) 撑到 29px；setFixedHeight 使
        // 切主题/切档等后续变化均不再改变栏高
        setFixedHeight(24);

		_timer = new QTimer(this);
		connect(_timer, SIGNAL(timeout()), this, SLOT(slot_clearMessage()));

		connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::outputStatusBarSig, this, &mStatusBar::slot_outputMessage);
		connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::outPutInfoShowSig, this, &mStatusBar::outPutInfoShow);
		connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::outputPermanentInfoSig, this, &mStatusBar::slot_outputPermanentInfo, Qt::DirectConnection);

		// 全部子控件就绪后做初始主题下发（底色/文字/进度条全部取 UiPalette 语义色，
		// 规则 0.2；切主题后由 ThemeManager::applyAll 经元方法重发）。
		// 不能在构造开头调：msgLabel/selLabel/unitButton 尚未创建，applyTheme 会解空指针。
		applyTheme();

		// 全局 QSS（mApplicationStart.qss）在主窗构造之后才加载，会盖掉这里的初始样式；
		// 监听 qApp 的样式变化事件补发（见 eventFilter）
		if (qApp)
			qApp->installEventFilter(this);
    }

	bool mStatusBar::eventFilter(QObject* watched, QEvent* event)
	{
		if (watched == qApp && event
			&& (event->type() == QEvent::StyleChange
				|| event->type() == QEvent::ApplicationPaletteChange))
		{
			applyTheme();
		}
		return QStatusBar::eventFilter(watched, event);
	}

	mStatusBar::~mStatusBar()
	{
		if (_timer)
		{
			delete _timer;
			_timer = nullptr;
		}
		if (_timerProgressBar)
		{
			delete _timerProgressBar;
			_timerProgressBar = nullptr;
		}
	}

	void mStatusBar::setMessage(const QString& msg)
	{
		const QString linkStyle = QStringLiteral("color:%1").arg(MxFunctions::UiPalette::label().name());
		if (msg.isEmpty())
		{
			pair<int, int> count = mOutputInfoWindow::getInstance()->getErrorWarningCount();
			if (count.first + count.second > 0)
			{
				msgLabel->setText(QString("<a style='%3' href=' '>%1 警告，%2 错误</a >").arg(count.first).arg(count.second).arg(linkStyle));
			}
			else
			{
				msgLabel->setText(QString("<a style='%1' href=' '>消息: 空</a >").arg(linkStyle));
			}
		}
		else
		{
			// 1. 获取包含此状态栏的顶层窗口
			QWidget* mainWindow = this->window();
			if (!mainWindow) {
				// 如果无法找到父窗口，则直接设置文本
				msgLabel->setText(QString("<a style='%2' href=' '>%1</a >").arg(msg, linkStyle));
				return;
			}

			// 2. 计算允许的最大像素宽度 (主窗口宽度的50%)
			const int maxWidth = mainWindow->width() * 0.5;

			// 3. 使用 QLabel 的字体创建一个 QFontMetrics 实例
			QFontMetrics fm(msgLabel->font());

			// 4. 使用 elidedText 方法获取处理后的字符串
			//    - 如果未超长，返回原字符串。
			//    - 如果超长，返回末尾带省略号的截断字符串。
			QString elidedMsg = fm.elidedText(msg, Qt::ElideRight, maxWidth);

			// 5. 设置处理后的文本到 QLabel
			msgLabel->setText(QString("<a style='%2' href=' '>%1</a >").arg(elidedMsg, linkStyle));

			// 6. 如果文本被截断了，设置一个工具提示
			if (elidedMsg != msg) {
				msgLabel->setToolTip(msg);
			}
			else {
				msgLabel->setToolTip(""); // 如果文本未被截断，则清除工具提示
			}
		}
	}

	QMenu* mStatusBar::getUnitsMenu()
	{
		// 分组标题：禁用 QAction + mxMenuSectionTitle 标记，由 mUiMenu::paintItem 自绘
		//（与 ribbon 单位菜单同方案）。旧 QLabel+QWidgetAction 宿主方案中，宿主控件
		// 行矩形由 QMenu 原生排布，与自绘行矩形错位时标题/内容互相挤压——即"内容跑到
		// 标题项上"的根因
		auto addGrayTitle = [](QMenu* menu, const QString& text)
		{
			QAction* title = new QAction(text, menu);
			title->setEnabled(false);
			title->setProperty("mxMenuSectionTitle", true);
			menu->addAction(title);
		};
		QStringList unitslist = {
			"度量标准(m、kg、N、s、V、A)",
			"度量标准(cm、g、dyne、s、V、A)",
			"度量标准(mm、kg、N、s、mV、mA)",
			"度量标准(mm、t、N、s、mV、mA)",
			"度量标准(mm、dat、N、s、mV、mA)",
			"度量标准(μm、kg、μN、s、V、mA)",
			"美国惯用单位(ft、lbm、lbf、°F、s、V、A)",
			"美国惯用单位(in、lbm、lbf、°F、s、V、A)",
		};
		QStringList anglelist = {
			"度",
			"弧度",
		};
		QStringList speedlist = {
			"rad",
			"RPM",
		};
		QStringList templist = {
			"摄氏度",
			"开尔文",
		};
		// 单位按钮
		unitButton = new QToolButton();
		// objectName 供高优先级 QSS 选择器使用：全局 mApplicationStart.qss 的裸
		// "QToolButton"（白底 #FFFFFF+边框）在应用级样式表加载后会盖掉构造期写入的
		// 透明底样式（同特异性后定义者胜），必须用具名选择器拿到更高优先级
		unitButton->setObjectName(QStringLiteral("statusUnitButton"));
		// 先设置箭头再设置其他属性
		unitButton->setArrowType(Qt::UpArrow);  // 必须在ToolButtonStyle之前设置
		unitButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		QVector<int> curunitsidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
		unitButton->setText("单位: "
			+ unitslist[curunitsidx[0]] + " "
			+ anglelist[curunitsidx[1]] + " "
			+ speedlist[curunitsidx[2]] + " " +
			(curunitsidx[0] > 5 ? "华氏度" : templist[curunitsidx[3]]));
		unitButton->setPopupMode(QToolButton::InstantPopup);

		// 样式表统一在 applyTheme() 下发（构造末尾会自调一次），此处不再另写一份
		//（两份相同 QSS 手写漂移风险 + 重复抛光）；选择器说明见 applyTheme

		// 确保有足够的空间显示箭头
        // 高度交给 QSS（statusUnitButton 高 19px）；宽度按自然内容宽精确约束，
        // 文字紧贴栏右缘（多留固定余量会出现"启动不贴右、切换后才贴右"）
        unitButton->setMinimumWidth(unitButtonMinWidth(unitButton));
		QMenu* unitsMenu = new UnitMenu(unitButton, this);
		// 菜单容器/行样式与全局命令菜单统一（规则 1.0，ThemeManager 一份收口 + mUiStyle 自绘行内容）
		ThemeManager::applyMenuStyle(unitsMenu);
		// 2. 下面 4 处替换掉原来的 addAction("...") 标题行
		addGrayTitle(unitsMenu, "单位系统");
		QActionGroup* unitsGroup = new QActionGroup(unitsMenu);
		unitsGroup->setExclusive(true);
		for (int i = 0; i < unitslist.size(); ++i) {
			QAction* act = unitsMenu->addAction(unitslist[i]);
			act->setData(i);
			act->setCheckable(true);
			act->setChecked(i == 2);
			unitsGroup->addAction(act);
		}


		/* 下面保持你原来的 for 循环不变 */
		addGrayTitle(unitsMenu, "角度");
		QActionGroup* angleGroup = new QActionGroup(unitsMenu);
		angleGroup->setExclusive(true);
		for (int i = 0; i < anglelist.size(); ++i) {
			QAction* act = unitsMenu->addAction(anglelist[i]);
			act->setData(i);
			act->setCheckable(true);
			act->setChecked(i == 0);
			angleGroup->addAction(act);
		}

		addGrayTitle(unitsMenu, "转速");
		/* for ... */
		QActionGroup* speedGroup = new QActionGroup(unitsMenu);
		speedGroup->setExclusive(true);
		for (int i = 0; i < speedlist.size(); ++i) {
			QAction* act = unitsMenu->addAction(speedlist[i]);
			act->setData(i);
			act->setCheckable(true);
			act->setChecked(i == 0);
			speedGroup->addAction(act);
		}

		addGrayTitle(unitsMenu, "温度");
		/* for ... */
		QActionGroup* tempGroup = new QActionGroup(unitsMenu);
		tempGroup->setExclusive(true);
		for (int i = 0; i < templist.size(); ++i) {
			QAction* act = unitsMenu->addAction(templist[i]);
			act->setData(i);
			act->setCheckable(true);
			act->setChecked(i == 0);
			tempGroup->addAction(act);
		}

		connect(unitsGroup, &QActionGroup::triggered, this, [this, tempGroup](QAction* act) {
			int idx = act->data().toInt();
			bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[0];
			getCVM->getModelData()->getUnitSystemManager()->switchUnits(idx);
			if (idx > 5)
			{
				getCVM->getModelData()->getUnitSystemManager()->swicthTemp(2);
				tempGroup->setDisabled(true);
			}
			else
			{
				//切回原来的温度单位
				getCVM->getModelData()->getUnitSystemManager()->swicthTemp(tempGroup->checkedAction()->data().toInt());
				tempGroup->setDisabled(false);
			}
			if (changedata)
				getCVM->getModelData()->updateDataFromUnitSystem(0);
			mInfoWindow::getInstance()->updateAllPanelUnits();
			emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);
			});
		connect(angleGroup, &QActionGroup::triggered, this, [this](QAction* act) {
			int idx = act->data().toInt();
			bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[1];
			getCVM->getModelData()->getUnitSystemManager()->switchAngle(idx);
			if (changedata)
				getCVM->getModelData()->updateDataFromUnitSystem(1);
			mInfoWindow::getInstance()->updateAllPanelUnits();
			emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

			});
		connect(speedGroup, &QActionGroup::triggered, this, [this](QAction* act) {
			int idx = act->data().toInt();
			bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[2];
			getCVM->getModelData()->getUnitSystemManager()->switchSpeed(idx);
			if (changedata)
				getCVM->getModelData()->updateDataFromUnitSystem(2);
			mInfoWindow::getInstance()->updateAllPanelUnits();
			emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

			});
		connect(tempGroup, &QActionGroup::triggered, this, [this](QAction* act) {
			int idx = act->data().toInt();
			bool changedata = idx != getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex()[3];
			getCVM->getModelData()->getUnitSystemManager()->swicthTemp(idx);
			if (changedata)
				getCVM->getModelData()->updateDataFromUnitSystem(3);
			mInfoWindow::getInstance()->updateAllPanelUnits();
			emit mMechGlobalSignals::getInstance()->clikNodesOnTree(false);

			});
		connect(unitsMenu, &QMenu::triggered, this,
			[this, unitsGroup, angleGroup, speedGroup, tempGroup, unitslist, anglelist, speedlist, templist](QAction*) {
				int unitIdx = unitsGroup->checkedAction()
					? unitsGroup->checkedAction()->data().toInt() : 0;
				int angleIdx = angleGroup->checkedAction()
					? angleGroup->checkedAction()->data().toInt() : 0;
				int speedIdx = speedGroup->checkedAction()
					? speedGroup->checkedAction()->data().toInt() : 0;
				int tempIdx = tempGroup->checkedAction()
					? tempGroup->checkedAction()->data().toInt() : 0;
				unitButton->setText("单位: " + unitslist[unitIdx] + " " + anglelist[angleIdx] + " " + speedlist[speedIdx] + " " +
					(unitIdx > 5 ? "华氏度" : templist[tempIdx]));
                unitButton->setMinimumWidth(unitButtonMinWidth(unitButton));
			});
		//UI同步
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::unitSystemChanged, this, [this, unitsGroup, unitsMenu](int index) {
			if (auto* act = unitsGroup->actions().value(index))
			{
				if (act->isChecked())
				{
					return;
				}
				else
				{
					act->setChecked(true);
					unitsMenu->triggered(act);
				}
			}
			});
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::angleUnitChanged, this, [this, angleGroup, unitsMenu](int index) {
			if (auto* act = angleGroup->actions().value(index))
			{
				if (act->isChecked())
				{
					return;
				}
				else
				{
					act->setChecked(true);
					unitsMenu->triggered(act);
				}
			}
			});
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::speedUnitChanged, this, [this, speedGroup, unitsMenu](int index) {
			if (auto* act = speedGroup->actions().value(index))
			{
				if (act->isChecked())
				{
					return;
				}
				else
				{
					act->setChecked(true);
					unitsMenu->triggered(act);
				}
			}
			});
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::tempUnitChanged, this, [this, tempGroup, unitsMenu](int index) {
			tempGroup->setDisabled(index == 2);
			if (auto* act = tempGroup->actions().value(index))
			{
				if (act->isChecked())
				{
					return;
				}
				else
				{
					act->setChecked(true);
					unitsMenu->triggered(act);
				}
			}
			});
		return unitsMenu;
	}

	void mStatusBar::applyTheme()
	{
		// 状态栏按 UiPalette 语义色整体重发（规则 0.2，附 A：状态栏条款）：
		// header 底 + text 字 + 顶边 1px border 线；单位按钮/进度条同取色板。
		// 样式表必须限定 QStatusBar 选择器——裸写 background
		// 会传播到子控件（QLabel/QToolButton 都继承），出现一堆黑框
		this->setStyleSheet(QStringLiteral(
			"QStatusBar { background:%1; color:%2; }"
			"QStatusBar QLabel { color:%2; background:transparent; }"
			"QStatusBar::item { border:none; }"
		).arg(MxFunctions::UiPalette::header().name(), MxFunctions::UiPalette::text().name()));

		// 消息/选择可点击标签的颜色内联在富文本 HTML 里，需重生成文本刷新（切主题后旧色残留）；
		// 消息是 3 秒自动清除的瞬态提示，主题切换时按"空"态重生成即可（原 setMessage("") 语义）
		setMessage("");
		setSelection(m_lastSelection);

		if (unitButton)
		{
			// 单位按钮字体直接取平台 UI 字体族。
			const QString unitQss = QStringLiteral(
				"QToolButton#statusUnitButton::menu-indicator { image:none; }"
                "QToolButton#statusUnitButton { color:%1; background:transparent; border:none; height:19px;"
				" font-family:'%4'; text-align:left; padding-right:5px; }"
				"QToolButton#statusUnitButton:hover { background:%2; border-radius:3px; }"
				"QToolButton#statusUnitButton:pressed { background:%3; border-radius:3px; }"
			).arg(MxFunctions::UiPalette::label().name(), MxFunctions::UiPalette::hover().name(),
				  MxFunctions::UiPalette::selected().name(), UiMetrics::uiFontFamily());
			unitButton->setStyleSheet(unitQss);
			repolish(unitButton); // 否则样式缓存不刷新，表现为"点击后颜色才变"
		}
		if (_progressBar)
		{
			_progressBar->setStyleSheet(QStringLiteral(
				"QProgressBar { height:8px; background:%1; border:none; }"
				"QProgressBar::chunk { background-color:%2; }"
			).arg(MxFunctions::UiPalette::panel().name(), MxFunctions::UiPalette::primary().name()));
			repolish(_progressBar);
		}
		repolish(this);
		update(); // 顶边 1px 线（paintEvent 取 border 色）

		// 单位系统弹窗的分组标题行（单位系统/角度/转速/温度）已改为 mxMenuSectionTitle
		// 禁用 QAction，由 mUiMenu::paintItem 自绘并实时读 UiPalette，主题切换无需补发
	}

	void mStatusBar::progressBarHideOrShow(int value)
	{
		if (value != 100 && !_progressBar->isVisible())
		{
			_progressBar->show();
		}
		else if (value == 100 && _progressBar->isVisible())
		{
			if (!_timerProgressBar) {
				_timerProgressBar = new QTimer(this);
				connect(_timerProgressBar, &QTimer::timeout, this, [=]() {
					_timerProgressBar->stop();
					_progressBar->hide();
					});
			}
			_timerProgressBar->start(3000);
		}
	}

    void mStatusBar::paintEvent(QPaintEvent* event)
    {
        QStatusBar::paintEvent(event);
        QPainter painter(this);
        //1px高，顶边线取 border 语义色（原写死 #DCDFE6 不随主题）
        painter.fillRect(0, 0, width(), 1, MxFunctions::UiPalette::border());

    }

    void mStatusBar::showMessageDialog() {
        //MechMainWindow::mFramelessMessageBox::information(this, "消息", "这里显示详细消息内容。");
        //MxFunctions::mGlobalSignals::getInstance()->activateDockWidgetSig(mOutputInfoWindow::getInstance());
        mDockLayoutManager::getInstance()->activateDock("Output");
    }

    void mStatusBar::showSelectionDialog() {
        //MxFunctions::mGlobalSignals::getInstance()->activateDockWidgetSig(mSelectInfoWindow::getInstance());
        mDockLayoutManager::getInstance()->activateDock("Select");
    }
	void mStatusBar::slot_outputPermanentInfo(const QString& msg)
	{
		if (msg.isEmpty()) {
			infoLabel->clear();
		}
		else {
			infoLabel->setText(msg);
		}
	}
	void mStatusBar::slot_updateSelectInfo(QString text)
    {
        setSelection(text);
    }

    void mStatusBar::slot_clearMessage()
    {
		setMessage("");
		_timer->stop();
    }

	void mStatusBar::setProgressBarValue(int value)
	{
		_progressBar->setValue(value); 
		progressBarHideOrShow(value);
	}

	void mStatusBar::outPutInfoShow()
	{
		if (!_timer->isActive())
			setMessage("");
	}

	QProgressBar* mStatusBar::getStatusBar()
	{
		return _progressBar;
	}

    void mStatusBar::slot_outputMessage(const QString& msg)
    {
        int timeout = 3000;//3秒
        _timer->start(timeout);
        if (msgLabel->text() == msg)
            return;
        setMessage(msg);
    }
}
