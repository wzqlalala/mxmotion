#include "mSectionWindow.h"
#include "mAppConfiguration.h"
#include "mTitleBar.h"
#include "mMechGlobalSignals.h"
#include "mCustomTooltip.h"
#include "mRibbonButton.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "PreViewportWidget.h"

#include "InteractionCore/Controller/InteractionToolManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMMeshCutFaceRender.h"
#include "mxr/basic/CameraController.h"
#include "mxr/gizmo/GizmoManager.h"
#include "mxr/basic/GizmoBase.h"


//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QScrollBar>
#include <QActionGroup>
#include <QToolTip>
#include <QTimer>
#include <QItemSelectionModel>
#include <algorithm>
#include <functional>
#include <QSignalBlocker>
#include <InteractionCore/Tool/CreateSectionTool.h>

/*
1.新建剖面时，界面调用渲染设置模式（输入int mode），用户在界面中拾取（坐标系平面或几何面）或者划线后就会新建成功剖面，渲染内部存储线条参数。
界面在收到渲染拾取或者划线完成后会在界面中显示剖面的列表（输出创建或者拾取完成的信号）
2.编辑剖面时，界面调用渲染编辑剖面（输入int index），渲染会让剖面的线条显示出来，支持用户拖动（持续）
3.取消编辑剖面时，界面调用取消编辑剖面（输入 无），渲染会隐藏线条
4.启用和禁用剖面时，界面调用渲染是否启用剖面（输入 int index, bool isEnable），渲染会根据是否启用剖面来渲染最终的状态
5.删除剖面时，界面调用渲染删除剖面(输入int index)，剖面会从渲染内部删除
*/

namespace MechMainWindow
{
    namespace
    {
        /*
        * 剖面树标准样式表（单一来源，规则 0.2：颜色取 UiPalette 语义色）
        *
        * 行内名称由 itemWidget 的 QLineEdit 显示，item 自身文字在所有状态下透明
        *（悬停/选中的伪态规则必须显式重声明 color:transparent，伪态选择器特异度
        * 高于基础规则，漏写会让条目文字在伪态下重新可见）；悬停/选中底色取
        * hover/selected 语义色，容器底/文字色与 ThemeManager::applyTreeTheme 的
        * 标准树主题同口径（surface/text）。
        *
        * 注意：applyGlobalRound 的树遍历（applyTreeTheme）会把本树当标准树整表
        * 替换成 "QTreeView {...}"，透明条目文字规则被冲掉后，选中/悬浮时条目
        * 文字与行内 QLineEdit 的名称叠加成"剖面剖面N"重影。每轮全局主题下发
        * 的收尾阶段（applyDockTheme 的 extra appliers，晚于树遍历）经构造函数
        * 注册的回调重新断言本表。
        */
        QString sectionTreeStyleSheet()
        {
            return QStringLiteral(
                "QTreeWidget { outline: none; background-color: %1; color: %2; border: none; }"
                "QTreeWidget::item { border: none; padding: 0px; color: transparent; }"
                "QTreeWidget::item:hover { background: %3; color: transparent; }"
                "QTreeWidget::item:selected { background: %4; color: transparent; }"
                "QTreeWidget::item:selected:!active { background: %4; color: transparent; }"
            ).arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::textHex(),
                MxFunctions::UiPalette::hoverHex(), MxFunctions::UiPalette::selectedHex());
        }
    }

    mSectionWindow* mSectionWindow::_instance = nullptr;
    mSectionWindow* mSectionWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mSectionWindow;
            //_instance->setTitleBarWidget(new TitleBarWidget(_instance, "剖面"));
            //_instance->setDockWindowTitle(QString("剖面"));
            _instance->setObjectName("SectionDock");
            _instance->setMinimumWidth(150);
            // 面板不声明任何控件级 QSS：按钮的悬浮/选中态由 createToolBar 里的
            // 全自绘 mRibbonButton 负责（规则 1.3，状态色取 UiPalette），树样式
            // 收口到 sectionTreeStyleSheet()。面板局部 QToolButton 规则与全局 QSS
            // 叠加下，Qt 5.13 对 QToolBar 内按钮的 :hover/:checked 绘制不可靠
            //（悬浮无背景、选中底近乎不可见），不再走 QSS 通路
        }
        return _instance;
    }

    mSectionWindow::mSectionWindow(QWidget* parent) :QWidget(parent)
    {
        //设置允许的位置

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        createToolBar();
        createTreeWidget();
        //m_toolBar->setBaseSize(300, 100);
        //m_toolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        //m_treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_toolBar);
        layout->addWidget(m_treeWidget);

        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mSectionWindow::slot_subWindowActivated);

        // applyGlobalRound 的树遍历（applyTreeTheme）会把本面板树当标准树整表
        // 替换 QSS，条目透明文字规则被冲掉（重影根因，见 sectionTreeStyleSheet
        // 注释）。dock extra applier 在每轮全局主题下发的收尾阶段运行（启动与
        // 切档/切主题都会走），晚于树遍历，此处重新断言标准样式表。
        // 与 mCenterMainWindow::applyToolbarTheme 同一解耦手法（Common 不反向
        // 链接业务类型，业务侧注册回调）；lambda 无捕获，_instance 为空时不动作
        ThemeManager::addDockThemeExtraApplier([]() {
            if (_instance == nullptr)
            {
                return;
            }
            _instance->m_treeWidget->setStyleSheet(sectionTreeStyleSheet());
            // 工具栏栏高与按钮 sizeHint 随档位重发（UiMetrics 派生，见 createToolBar；
            // mRibbonButton 自身不监听档位，需外部失效几何缓存）
            _instance->m_toolBar->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
            const auto btns = _instance->m_toolBar->findChildren<QToolButton*>();
            for (QToolButton* btn : btns)
            {
                btn->updateGeometry();
            }
            });
    }

    void mSectionWindow::startRenameEdit(QLineEdit* edit)
    {
        if (!edit)
        {
            return;
        }

        edit->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        edit->setReadOnly(false);
        edit->setFocusPolicy(Qt::StrongFocus);
        edit->setCursor(Qt::IBeamCursor);
        edit->setStyleSheet(
            "background: #FFFFFF;"
            "color: #303133;"
            "border: 1px solid #409EFF;");

        // clicked 信号仍处于按钮鼠标事件处理中，延迟到下一轮事件循环再抢焦点，
        // 可以确保输入框真正进入文本输入状态。
        QTimer::singleShot(0, edit, [edit]()
            {
                edit->setFocus(Qt::MouseFocusReason);
                edit->selectAll();
            });
    }

    void mSectionWindow::createToolBar()
    {
        m_toolBar = new QToolBar();
        // 栏高 = 控件外高 H（标准 26，与视口工具栏/树窗口工具条同一口径），
        // 容纳 quickControl（H−2）方形图标钮
        m_toolBar->setFixedHeight(MxFunctions::UiMetrics::controlHeight());
        m_actionGroup = new ToggleActionGroup(m_toolBar);

        // 面板按钮统一用全自绘 mRibbonButton，纯图标形态（与顶部快捷图标钮同款：
        // quickControl = H−2 标准 24×24 方形、图标居中，名称经 defaultAction 以
        // tooltip 呈现）。不能用 QToolBar::addAction 的默认 QToolButton：
        // 全局 QSS 命中下 Qt 5.13 对工具栏内按钮的 :hover/:checked 盒模型绘制不可靠
        //（悬浮无背景、选中底近乎不可见，实测复现）；mRibbonButton 在 paintEvent 里
        // 按 UiPalette 自绘全部状态（悬停 hover/选中按下 selected/禁用 disabled），完全
        // 绕开样式系统，生产功能区按钮即以此方式规避
        // inGroup：是否加入互斥组。只有三个"新建"动作参与互斥（组信号驱动
        // createSection/unEditSection）；剖面线显隐/主体颜色/透明是独立开关，
        // 入组会误触发建剖面/退出编辑逻辑
        auto addSectionAction = [this](const QIcon& icon, const QString& text,
            bool inGroup) -> QAction*
        {
            QAction* action = new QAction(icon, text, m_toolBar);
            action->setCheckable(true);
            if (inGroup)
            {
                m_actionGroup->addAction(action);
            }

            mRibbonButton* btn = new mRibbonButton(mRibbonButton::Mode::Small, m_toolBar);
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
            btn->setDefaultAction(action);   // 图标/勾选态自动同步；text 转为 tooltip
            m_toolBar->addWidget(btn);
            return action;
        };

        QAction* actionNew = addSectionAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/xinjianpouqiemian3x.png"), "新建剖切面", true);
        actionNew->setData(1);
        actionNew = addSectionAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/xinjianzuobiaoxi3x.png"), "新建坐标系平面剖切面", true);
        actionNew->setData(2);
        actionNew = addSectionAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/xinjianjihe3x.png"), "新建几何平面剖切面", true);
        actionNew->setData(3);

        connect(m_actionGroup, &ToggleActionGroup::actionChecked, this, [this](QAction* act) {
            //qDebug() << "选中:" << act->text();
            createSection(act->data().toInt());
            });

        connect(m_actionGroup, &ToggleActionGroup::actionUnchecked, this, [this](QAction* act) {
            //qDebug() << "取消:" << act->text();
            unEditSection();
            });

        /*
        connect(m_actionGroup, &QActionGroup::triggered, this, [=, &lastAction](QAction*action) {
            if (lastAction == action)
            {
                action->setChecked(false);
                lastAction = nullptr;
                unEditSection();
                return;
            }
            else
            {
                lastAction = action;
            }
            createSection(action->data().toInt());
            });
            */
            //m_actionEdit = m_toolBar->addAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/bianji3x.png"), "编辑剖面");
            //connect(m_actionEdit, &QAction::triggered, this, [this]() {editSection(); });

            //m_actionDelete = m_toolBar->addAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/shanchu3x.png"), "删除剖面");
            //connect(m_actionDelete, &QAction::triggered, this, [this]() {
            //	if (!VMManager->getCurrentViewportPrivate())
            //	{
            //		return;
            //	}
            //	QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
            //	for (QTreeWidgetItem* item : selectedItems)
            //	{
            //		QList<int> indexs = getItemPath(item);
            //		int index = indexs.first();
            //		if (index <= -1)
            //		{
            //			continue;
            //		}
            //		if (VMManager->getCurrentViewportPrivate()->_sections.size() <= index)
            //			return;
            //		//5.删除剖面，调用渲染删除剖面(使用index)
            //		getCVM->getPreRenderWidget()->getSectionManager()->deleteSection(index);
            //		getCVM->getPreRenderWidget()->updatePaint();
            //		VMManager->getCurrentViewportPrivate()->_sections.removeAt(index);
            //		delete item;  // 直接从 tree 中删除并释放内存

            //	}
            //	});

            //m_actionRename = m_toolBar->addAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/chongmingming3x.png"), "重命名剖面");
            //connect(m_actionRename, &QAction::triggered, this, [this]() {
            //	auto index = m_treeWidget->currentIndex();
            //	if (index.isValid()) {
            //		m_treeWidget->edit(index);  // 启用内联编辑
            //	}
            //	});

        m_actionType = addSectionAction(QIcon(":/mMainWidgets/Icon/homepage/poumian3x.png"), "剖面线显隐", false);
        connect(m_actionType, &QAction::triggered, this, [this]() {
            if (!getCVM)
            {
                return;
            }

            auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
            newPrivate->_isShowSectionLine = m_actionType->isChecked();
            getCVM->getPreRenderWidget()->getSectionManager()->setSectionType(m_actionType->isChecked() ? 1 : 0);
            getCVM->getPreRenderWidget()->updatePaint();
            });

        m_actionColor = addSectionAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/zhutiyanse.png"), "主体颜色", false);
        connect(m_actionColor, &QAction::triggered, this, [this]() {
            if (!getCVM)
            {
                return;
            }
            auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
            newPrivate->_isSectionColor = m_actionColor->isChecked();
            getCVM->getPreRenderWidget()->getSectionManager()->setSectionColorType(m_actionColor->isChecked() ? 1 : 0);
            getCVM->getPreRenderWidget()->updatePaint();
            });

        m_actionTransparency = addSectionAction(QIcon(":/mMainWidgets/Icon/mSectionWindow/touming.png"), "透明", false);
        connect(m_actionTransparency, &QAction::triggered, this, [this]() {
            if (!getCVM)
            {
                return;
            }
            auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
            newPrivate->_isSectionTransparency = m_actionTransparency->isChecked();
            //getCVM->getPreRenderWidget()->getSectionManager()->setSectionType(m_actionType->isChecked() ? 1 : 0);
            getCVM->getPreRenderWidget()->getSectionManager()->setSectionTransparent(m_actionTransparency->isChecked() ? true : false);
            getCVM->getPreRenderWidget()->updatePaint();
            });

        // 默认禁用
    /*	m_actionEdit->setEnabled(false);
        m_actionRename->setEnabled(false);
        m_actionDelete->setEnabled(false);*/

        //默认显示剖切线
        m_actionColor->setChecked(true);
        //默认显示主体颜色
        m_actionType->setChecked(true);
        //默认透明
        m_actionTransparency->setChecked(true);
    }

    void mSectionWindow::createTreeWidget()
    {
        m_treeWidget = new QTreeWidget();

        // 行内名称由 itemWidget 的 QLineEdit 显示，item 自身文字必须所有状态透明：
        // 悬停/选中的伪态规则要显式带上 color:transparent，否则伪态选择器特异度
        // 高于基础规则，条目文字会在伪态下重新可见。颜色取 UiPalette 语义色板
        //（规则 0.2），与 ThemeManager 树主题同一来源
        m_treeWidget->setStyleSheet(sectionTreeStyleSheet());

        m_treeWidget->setHeaderHidden(true);         // 隐藏标题头
        m_treeWidget->setRootIsDecorated(false);     // 不显示展开箭头（可选）
        m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //auto delegate = new CheckBoxFriendlyDelegate(m_treeWidget);
        //m_treeWidget->setItemDelegate(delegate);

        m_treeWidget->setColumnCount(1);
        m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);



        // 监听选择变化
        connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, [this]() {
            // 重置所有按钮容器背景为透明
            for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i)
            {
                QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
                QWidget* widget = m_treeWidget->itemWidget(item, 0);
                if (widget)
                {
                    QPalette pal = widget->palette();
                    pal.setColor(QPalette::Window, QColor(255, 255, 255, 0));
                    widget->setPalette(pal);
                }
            }

            // 选中项按钮容器背景变蓝
            QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();
            for (QTreeWidgetItem* item : selectedItems)
            {
                QWidget* widget = m_treeWidget->itemWidget(item, 0);
                if (widget)
                {
                    QPalette pal = widget->palette();
                    pal.setColor(QPalette::Window, QColor("#D0E9FF"));
                    widget->setPalette(pal);
                }
            }

            // 选中时显示剖面编辑 gizmo
            int count = selectedItems.count();
            if (count == 1)
            {
                m_treeWidget->setCurrentItem(selectedItems.first());
                editSection();
            }
            else
            {
                MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("");
                getCVM->getPreRenderWidget()->getGizmoManager()->setActiveGizmo();
                getCVM->getPreRenderWidget()->updatePaint();
            }
            });

        //监听是否勾选
        //connect(delegate, &CheckBoxFriendlyDelegate::checkBoxToggled,
        //	this, [this](const QModelIndex& idx, Qt::CheckState state) {
        //		if (!VMManager->getCurrentViewportPrivate())
        //			return;
        //		int index = idx.row();//第几个剖面
        //		if (VMManager->getCurrentViewportPrivate()->_sections.size() <= index)
        //			return;

        //		VMManager->getCurrentViewportPrivate()->_sections[index]->_isEnable = state == Qt::Checked ? true : false;
        //		//4.启用和禁用剖面调用渲染发送启用或禁用第几个剖面（使用index）
  //              getCVM->getPreRenderWidget()->getSectionManager()->setSectionVisible(index, VMManager->getCurrentViewportPrivate()->_sections[index]->_isEnable);
  //              getCVM->getPreRenderWidget()->updatePaint();
        //	});

        connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int column) {
            if (column == 0 && item)
            {
                QWidget* widget = m_treeWidget->itemWidget(item, 0);
                if (widget)
                {
                    QLineEdit* edit = widget->findChild<QLineEdit*>();
                    if (edit)
                    {
                        startRenameEdit(edit);
                    }
                }
            }
            });
    }

    QTreeWidgetItem* mSectionWindow::createTreeWidgetItem(MechPreData::mSection* section)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget);
        item->setText(0, section->_name);
        item->setForeground(0, QBrush(Qt::transparent));
        // 使用自定义 QLineEdit 完成重命名，不再启用 QTreeWidget 的原生编辑器。
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        // 创建整行容器，包含名字和按钮
        QWidget* rowWidget = new QWidget(m_treeWidget);
        rowWidget->setStyleSheet("background: transparent;");
        QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(14, 2, 14, 2);  // 左右都是 14，可调
        rowLayout->setSpacing(0);
        QLineEdit* nameEdit = new QLineEdit(section->_name);

        nameEdit->setStyleSheet("background: transparent; color: #303133; border: none;");
        nameEdit->setFrame(false);
        nameEdit->setReadOnly(true);
        nameEdit->setFocusPolicy(Qt::NoFocus);
        nameEdit->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        rowLayout->addWidget(nameEdit);

        // 用事件过滤器捕获双击
        nameEdit->installEventFilter(this);
        rowLayout->addStretch();
        // 显隐按钮
        QPushButton* visibilityBtn = new QPushButton(rowWidget);
        visibilityBtn->setFixedSize(20, 20);
        visibilityBtn->setObjectName("eyeBtn");
        visibilityBtn->setIcon(QIcon(section->_isEnable
            ? ":/mMainWidgets/Icon/mSectionWindow/kaiyan@3x.png"
            : ":/mMainWidgets/Icon/mSectionWindow/biyan@3x.png"));
        //visibilityBtn->setToolTip(section->_isEnable ? "隐藏" : "显示");
        visibilityBtn->setFlat(true);
        visibilityBtn->setCheckable(true);
        visibilityBtn->setChecked(section->_isEnable);
        visibilityBtn->setFocusPolicy(Qt::NoFocus);
        visibilityBtn->setAttribute(Qt::WA_Hover, false);
        visibilityBtn->installEventFilter(new TooltipInstaller(section->_isEnable ? tr("显示") : tr("隐藏"),
            QPixmap(section->_isEnable ? ":/mMainWidgets/Icon/mSectionWindow/kaiyan@3x.png": ":/mMainWidgets/Icon/mSectionWindow/biyan@3x.png"),
            section->_isEnable ? tr("显示") : tr("隐藏"),
            200, visibilityBtn));
        visibilityBtn->setStyleSheet(R"(
    QPushButton {
        border: none;
        background-color: transparent;
        padding: 0px;
        outline: none;
    }
    QPushButton:hover,
    QPushButton:pressed,
    QPushButton:focus,
    QPushButton:checked {
        border: none;
        background-color: transparent;
        outline: none;
    }
)");
        rowLayout->addWidget(visibilityBtn);

        // 编辑按钮
        QPushButton* editBtn = new QPushButton(rowWidget);
        editBtn->setFixedSize(20, 20);
        editBtn->setIcon(QIcon(":/mMainWidgets/Icon/mSectionWindow/bianji.png"));
        editBtn->setIconSize(QSize(16, 16));
        //editBtn->setToolTip("编辑");
        editBtn->setFlat(true);
        editBtn->setFocusPolicy(Qt::NoFocus);
        editBtn->setAttribute(Qt::WA_Hover, false);
        editBtn->installEventFilter(new TooltipInstaller("重命名",
            QPixmap(":/mMainWidgets/Icon/mSectionWindow/bianji.png"),
            "重命名",
            200, editBtn));
        editBtn->setStyleSheet(R"(
    QPushButton {
        border: none;
        background-color: transparent;
        padding: 0px;
        outline: none;
    }
    QPushButton:hover,
    QPushButton:pressed,
    QPushButton:focus,
    QPushButton:checked {
        border: none;
        background-color: transparent;
        outline: none;
    }
)");
        rowLayout->addWidget(editBtn);

        // 删除按钮
        QPushButton* deleteBtn = new QPushButton(rowWidget);
        deleteBtn->setFixedSize(20, 20);
        deleteBtn->setIcon(QIcon(":/mMainWidgets/Icon/mSectionWindow/shanchu@3x.png"));
        deleteBtn->setIconSize(QSize(16, 16));
        deleteBtn->setFlat(true);
        deleteBtn->setFocusPolicy(Qt::NoFocus);
        deleteBtn->setAttribute(Qt::WA_Hover, false);
        deleteBtn->installEventFilter(new TooltipInstaller("删除",
            QPixmap(":/mMainWidgets/Icon/mSectionWindow/shanchu@3x.png"),
            "删除",
            200, deleteBtn));
        deleteBtn->setStyleSheet(R"(
    QPushButton {
        border: none;
        background-color: transparent;
        padding: 0px;
        outline: none;
    }
    QPushButton:hover,
    QPushButton:pressed,
    QPushButton:focus,
    QPushButton:checked {
        border: none;
        background-color: transparent;
        outline: none;
    }
)");
        rowLayout->addWidget(deleteBtn);

        // 5. 把按钮容器放到第 1 列
        m_treeWidget->setItemWidget(item, 0, rowWidget);

        // 6. 绑定按钮信号（必须放在按钮创建之后）
        connect(visibilityBtn, &QPushButton::clicked, this,
            [this, item, visibilityBtn]()
            {
                const bool visible = visibilityBtn->isChecked();
                setSectionsVisible(itemsForRowAction(item), visible);
            });

        connect(editBtn, &QPushButton::clicked, this, [this, item, nameEdit]()
            {
                // 重命名始终只针对点击的这一项。
                m_treeWidget->setCurrentItem(
                    item,
                    0,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

                startRenameEdit(nameEdit);
            });
        // 双击编辑结束
        connect(nameEdit, &QLineEdit::editingFinished, this,
            [this, nameEdit, item, section]()
            {
                nameEdit->setReadOnly(true);
                nameEdit->setFocusPolicy(Qt::NoFocus);
                nameEdit->setStyleSheet(
                    "background: transparent;"
                    "color: #303133;"
                    "border: none;");
                nameEdit->unsetCursor();
                nameEdit->setAttribute(Qt::WA_TransparentForMouseEvents, true);

                if (!VMManager->getCurrentViewportPrivate() || !section)
                {
                    return;
                }

                const QString oldName = section->_name;
                const QString newName = nameEdit->text().trimmed();

                // 名称没有变化
                if (newName == oldName)
                {
                    QSignalBlocker blocker(nameEdit);
                    nameEdit->setText(oldName);
                    return;
                }

                // 不允许空名称
                if (newName.isEmpty())
                {
                    {
                        QSignalBlocker blocker(nameEdit);
                        nameEdit->setText(oldName);
                    }

                    {
                        QSignalBlocker blocker(m_treeWidget->model());
                        item->setText(0, oldName);
                    }

                    mGlobalSignals::getInstance()->outputMessageSig(1, "名称为空！");

                    return;
                }

                auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
                
                // 判断是否与其他剖面重名，排除当前 section
                if (MechPreData::mSection::isSectionNameExists(
                    newName,
                    newPrivate->_sections,
                    section))
                {
                    // 注意：真正显示的是 nameEdit，必须恢复 nameEdit
                    {
                        QSignalBlocker blocker(nameEdit);
                        nameEdit->setText(oldName);
                    }

                    // 同步恢复 QTreeWidgetItem 内部的数据
                    {
                        QSignalBlocker blocker(m_treeWidget->model());
                        item->setText(0, oldName);
                    }

                    mGlobalSignals::getInstance()->outputMessageSig(1, "该名称已存在！");

                    return;
                }

                // 校验成功后再更新业务数据
                section->_name = newName;
                // 同步输入框中的去空格结果
                {
                    QSignalBlocker blocker(nameEdit);
                    nameEdit->setText(newName);
                } // 同步 TreeWidgetItem 的内部文本
                {
                    QSignalBlocker blocker(m_treeWidget->model());
                    item->setText(0, newName);
                }
            });


        connect(deleteBtn, &QPushButton::clicked, this, [this, item]()
            {
                deleteSections(itemsForRowAction(item));
            });


        m_treeWidget->addTopLevelItem(item);

        return item;
    }

    QList<int> mSectionWindow::getItemPath(QTreeWidgetItem* target)
    {
        QList<int> path;

        std::function<bool(QTreeWidgetItem*, QList<int>)> dfs =
            [&](QTreeWidgetItem* item, QList<int> currentPath) -> bool
            {
                if (item == target) {
                    path = currentPath;
                    return true;
                }

                for (int i = 0; i < item->childCount(); ++i) {
                    QList<int> childPath = currentPath;
                    childPath.append(i);
                    if (dfs(item->child(i), childPath))
                        return true;
                }
                return false;
            };

        for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
            QList<int> rootPath;
            rootPath.append(i);
            if (dfs(m_treeWidget->topLevelItem(i), rootPath))
                break;
        }

        return path;
    }

    QList<QTreeWidgetItem*> mSectionWindow::itemsForRowAction(QTreeWidgetItem* clickedItem)
    {
        if (!m_treeWidget || !clickedItem)
        {
            return {};
        }

        const QList<QTreeWidgetItem*> selectedItems = m_treeWidget->selectedItems();

        // 点击的是当前多选集合中的一项：操作作用于整个多选集合。
        if (selectedItems.contains(clickedItem))
        {
            return selectedItems;
        }

        // 点击的是未选中项：清除旧选择，只操作当前点击项。
        m_treeWidget->setCurrentItem(
            clickedItem,
            0,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        return { clickedItem };
    }

    void mSectionWindow::setSectionsVisible(
        const QList<QTreeWidgetItem*>& items,
        bool visible)
    {
        if (!VMManager->getCurrentViewportPrivate() || !getCVM || items.isEmpty())
        {
            return;
        }

        auto* renderWidget = getCVM->getPreRenderWidget();
        if (!renderWidget || !renderWidget->getSectionManager())
        {
            return;
        }

        auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
        for (QTreeWidgetItem* targetItem : items)
        {
            const int index = m_treeWidget->indexOfTopLevelItem(targetItem);
            if (index < 0 || index >= newPrivate->_sections.size())
            {
                continue;
            }

            auto* section = newPrivate->_sections[index];
            if (!section)
            {
                continue;
            }

            section->_isEnable = visible;
            renderWidget->getSectionManager()->setSectionVisible(index, visible);

            QWidget* rowWidget = m_treeWidget->itemWidget(targetItem, 0);
            QPushButton* targetButton = rowWidget
                ? rowWidget->findChild<QPushButton*>(QStringLiteral("eyeBtn"))
                : nullptr;

            if (targetButton)
            {
                QSignalBlocker blocker(targetButton);
                targetButton->setChecked(visible);
                targetButton->installEventFilter(new TooltipInstaller(section->_isEnable ? tr("显示") : tr("隐藏"),
                    QPixmap(section->_isEnable ? ":/mMainWidgets/Icon/mSectionWindow/kaiyan@3x.png" : ":/mMainWidgets/Icon/mSectionWindow/biyan@3x.png"),
                    section->_isEnable ? tr("显示") : tr("隐藏"),
                    200, targetButton));
                targetButton->setIcon(QIcon(visible
                    ? ":/mMainWidgets/Icon/mSectionWindow/kaiyan@3x.png"
                    : ":/mMainWidgets/Icon/mSectionWindow/biyan@3x.png"));
            }
        }

        renderWidget->updatePaint();
    }

    void mSectionWindow::deleteSections(const QList<QTreeWidgetItem*>& items)
    {
        if (!VMManager->getCurrentViewportPrivate() || !getCVM || items.isEmpty())
        {
            return;
        }

        auto* renderWidget = getCVM->getPreRenderWidget();
        if (!renderWidget || !renderWidget->getSectionManager())
        {
            return;
        }

        QList<int> indexes;
        indexes.reserve(items.size());

        auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
        for (QTreeWidgetItem* targetItem : items)
        {
            const int index = m_treeWidget->indexOfTopLevelItem(targetItem);
            if (index >= 0 && index < newPrivate->_sections.size())
            {
                indexes.append(index);
            }
        }

        // 必须倒序删除。否则前面的元素删除后，后面的索引会整体前移。
        std::sort(indexes.begin(), indexes.end(), std::greater<int>());
        indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());

        if (indexes.isEmpty())
        {
            return;
        }


        {
            QSignalBlocker blocker(m_treeWidget);
            m_treeWidget->clearSelection();

            for (const int index : indexes)
            {
                renderWidget->getSectionManager()->deleteSection(index);
                newPrivate->_sections.removeAt(index);

                QTreeWidgetItem* removedItem = m_treeWidget->takeTopLevelItem(index);
                delete removedItem;
            }
        }

        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("");
        renderWidget->getGizmoManager()->setActiveGizmo();
        renderWidget->updatePaint();
    }

    void mSectionWindow::stopBoundViewportInteraction()
    {
        // 断开旧视口剖面创建信号
        if (m_conn)
        {
            disconnect(m_conn);
            m_conn = {};
        }

        // 取消旧视口的创建工具
        if (m_boundPreViewportWidget)
        {
            auto* interaction = m_boundPreViewportWidget->interaction();
            if (interaction && interaction->getToolManager())
            {
                interaction->getToolManager()->setActive(
                    mxr::InteractionToolType::None);
            }
        }

        // 取消旧视口的剖面编辑 Gizmo
        if (m_boundRenderWidget)
        {
            auto gizmoManager = m_boundRenderWidget->getGizmoManager();

            if (gizmoManager)
            {
                if (gizmoManager->activeGizmo())
                {
                    gizmoManager->activeGizmo()->active(false);
                }

                gizmoManager->setActiveGizmo();
            }

            m_boundRenderWidget->updatePaint();
        }
    }

    bool mSectionWindow::eventFilter(QObject* obj, QEvent* event)
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>(obj);
        if (!edit) return QObject::eventFilter(obj, event);

        if (event->type() == QEvent::MouseButtonDblClick)
        {
            startRenameEdit(edit);
            return true;
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            // 单击不拦截，让 QTreeWidget 选中 item
            return false;
        }
        return QObject::eventFilter(obj, event);
    }

    mSectionWindow::~mSectionWindow()
    {

    }

    void mSectionWindow::createSection(int type)
    {
        mDockLayoutManager::getInstance()->activateDock("Section");
        if (!VMManager->getCurrentViewportPrivate() || !getCVM)
        {
            return;
        }

        if (VMManager->getCurrentViewportPrivate()->getViewportType() != ViewportType::Viewport3D)
        {
            this->setEnabled(false);
            return;
        }

        m_boundRenderWidget = getCVM->getPreRenderWidget();
        m_boundPreViewportWidget = getCVM->getPreViewportWidget();

        auto actions = m_actionGroup->actions();
        if (actions.size() != 3)
        {
            return;
        }
        m_actionGroup->setCurrentAction(type - 1);
        m_treeWidget->clearSelection();
        //m_actionGroup->blockSignals(false);
        //1.新建剖面，调用渲染设置拾取模式
        switch (type)
        {
        case 1:
        {
            MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("请在模型上拖动鼠标选择两点来创建剖面");
            //getCVM->getPreRenderWidget()->getStateMachineManager()->setCreateMode(CreationMode::ScreenLine); // 设置创建模式为剖面创建
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreate(true); // 开启创建模式，剖面创建

            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(InteractionToolType::CreateSection);
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->active<mxr::InteractionCreateSectionTool>();
            tool->setCreateMode(CreationMode::ScreenLine);

            if (m_conn) disconnect(m_conn);
            // 连接剖面管理的创建成功信号
            m_conn = connect(getCVM->getPreRenderWidget()->getSectionManager(), &mxr::SectionManager::create_section, this, &mSectionWindow::slot_finishedCreatePick, Qt::UniqueConnection);//调用连接渲染接口
            break;//设置模式为划线
        }
        case 2:
        {
            MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("请选择坐标系平面来创建剖面");
            //getCVM->getPreRenderWidget()->getStateMachineManager()->setCreateMode(CreationMode::Axis); // 设置创建模式为剖面创建
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreate(true); // 开启创建模式，剖面创建

            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(InteractionToolType::CreateSection);
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->active<mxr::InteractionCreateSectionTool>();
            tool->setCreateMode(CreationMode::Axis);

            if (m_conn) disconnect(m_conn);
            // 连接剖面管理的创建成功信号
            m_conn = connect(getCVM->getPreRenderWidget()->getSectionManager(), &mxr::SectionManager::create_section, this, &mSectionWindow::slot_finishedCreatePick, Qt::UniqueConnection);//调用连接渲染接口
            break;//设置模式为拾取坐标系平面
        }
        case 3:
        {
            MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("请选择几何平面来创建剖面");
            //getCVM->getPreRenderWidget()->getStateMachineManager()->setCreateMode(CreationMode::PickFace); // 设置创建模式为剖面创建
            //getCVM->getPreRenderWidget()->getStateMachineManager()->enableCreate(true); // 开启创建模式，剖面创建

            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(InteractionToolType::CreateSection);
            auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->active<mxr::InteractionCreateSectionTool>();
            tool->setCreateMode(CreationMode::PickFace);

            if (m_conn) disconnect(m_conn);
            // 连接剖面管理的创建成功信号
            m_conn = connect(getCVM->getPreRenderWidget()->getSectionManager(), &mxr::SectionManager::create_section, this, &mSectionWindow::slot_finishedCreatePick, Qt::UniqueConnection);//调用连接渲染接口
            //getCVM->getPreRenderWidget()->setPickFilter(PickFilter::PickGeoFace); 
            //getCVM->getPreRenderWidget()->startPick();
            break;
        }
        default:
            break;
        }

    }

    void mSectionWindow::editSection()
    {
        if (!VMManager->getCurrentViewportPrivate() || !getCVM)
        {
            return;
        }
        int count = m_treeWidget->selectedItems().count();
        if (count != 1)
        {
            return;
        }
        auto selectitems = m_treeWidget->selectedItems();
        //selectitems[0]->text(1);
        QList<int> indexs = getItemPath(selectitems.first());
        int index = indexs.first();
        if (index <= -1)
        {
            return;
        }


        m_boundRenderWidget = getCVM->getPreRenderWidget();
        m_boundPreViewportWidget = getCVM->getPreViewportWidget();

        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("请编辑剖面");
        qDebug() << "编辑:" << index;
        //2.编辑剖面，调用渲染然后显示可拖动的线条，无论创建方式为何种，都是拖拽线条编辑剖面（编辑剖面，界面不做任何更新）（使用index）

        // 获得对应剖面
        auto section_manager = getCVM->getPreRenderWidget()->getSectionManager();
        // 将其转化为选中对象
        std::vector<SelectionItem> selections;
        SelectionItem item;
        item.handle = AnyHandle::from(section_manager); // 封装之后的void*

        item.id = index;
        selections.push_back(item);

        getCVM->getPreRenderWidget()->getGizmoManager()->setActiveGizmo(mxr::GizmoType::Section); // 设置控件模式
        getCVM->getPreRenderWidget()->getGizmoManager()->setSelection(selections); // 设置选中的编辑对象
        if (getCVM->getPreRenderWidget()->getGizmoManager()->activeGizmo())
            getCVM->getPreRenderWidget()->getGizmoManager()->activeGizmo()->active(true);
        //getCVM->getPreRenderWidget()->getStateMachineManager()->enableGizmo(true); // 开启控件编辑
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mSectionWindow::unEditSection()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        const auto actions = m_actionGroup->actions();

        for (auto* action : actions)
        {
            if (action)
            {
                action->setChecked(false);
            }
        }

        m_treeWidget->clearSelection();

        MechCommon::mMechGlobalSignals::getInstance()
            ->outputPermanentInfoSig("");

        stopBoundViewportInteraction();

    }

    void mSectionWindow::hideEvent(QHideEvent* event)
    {
        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("");
        if (!VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        unEditSection();
    }

    void mSectionWindow::slot_finishedCreatePick()
    {
        MechCommon::mMechGlobalSignals::getInstance()->outputPermanentInfoSig("");
        if (!VMManager->getCurrentViewportPrivate())
        {
            return;
        }
        auto action = m_actionGroup->getCurrentAction();
        if (action)
        {
            //m_actionGroup->blockSignals(true);
            action->setChecked(false);
            //m_actionGroup->blockSignals(false);
        }
        int sectionId = 0;
        QString sectionName;
        auto* newPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
        ++newPrivate->_sectionMaxID;
        // ID 唯一不代表名称唯一，因为已有剖面可能被手动重命名为“剖面N”
        do
        {
            ++sectionId;
            sectionName = QStringLiteral("剖面%1").arg(sectionId);
        } while (MechPreData::mSection::isSectionNameExists(sectionName, newPrivate->_sections, nullptr));
        auto section = new MechPreData::mSection(
            newPrivate->_sectionMaxID,
            sectionName,
            true);
        newPrivate->_sections.append(section);
        //getCVM->getPreRenderWidget()->getCreateToolManager()->active(); // 设置控件模式，空则是无，退出控件模式

        getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(InteractionToolType::None);

        auto item = createTreeWidgetItem(section);
        m_treeWidget->clearSelection();
        m_treeWidget->setItemSelected(item, true);
        m_treeWidget->update();
        editSection();
        //创建完成后断拾取信号
        if (m_conn)
            disconnect(m_conn);
    }

    void mSectionWindow::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);

        auto* newViewportPrivate =
            VMManager->getCurrentViewportPrivate();

        // 这里清理的是保存的旧视口，不是 getCVM 所代表的新视口
        stopBoundViewportInteraction();

        m_boundRenderWidget.clear();
        m_boundPreViewportWidget.clear();

        // 清理旧列表时阻止 itemSelectionChanged，
        // 否则它可能通过 getCVM 操作到新视口
        {
            QSignalBlocker blocker(m_treeWidget);
            m_treeWidget->clear();
        }

        // 删除最后一个视口时，这里可能为空
        if (!newViewportPrivate || !getCVM)
        {
            setEnabled(false);
            return;
        }

        if (newViewportPrivate->getViewportType() !=
            ViewportType::Viewport3D)
        {
            setEnabled(false);
            return;
        }

        auto* newPrivate =
            dynamic_cast<MechPreData::mPreViewportPrivate*>(
                newViewportPrivate);

        if (!newPrivate)
        {
            setEnabled(false);
            return;
        }

        setEnabled(true);

        // 记录新视口，供下一次切换时清理
        m_boundRenderWidget = getCVM->getPreRenderWidget();
        m_boundPreViewportWidget = getCVM->getPreViewportWidget();

        // 重建列表时也不要触发选择编辑逻辑
        {
            QSignalBlocker blocker(m_treeWidget);

            for (auto* section : newPrivate->_sections)
            {
                createTreeWidgetItem(section);
            }
        }

        m_actionType->setChecked(
            newPrivate->_isShowSectionLine);

        m_actionColor->setChecked(
            newPrivate->_isSectionColor);

        m_actionTransparency->setChecked(
            newPrivate->_isSectionTransparency);
    }
}