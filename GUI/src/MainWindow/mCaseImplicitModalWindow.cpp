#include "mCaseImplicitModalWindow.h"
#include "mNVHWindowComponent.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollArea>
#include <QFrame>
#include <QFile>
#include <QTextStream>
#include <QVector3D>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QSignalBlocker>
#include <QTimer>
#include <QMouseEvent>
#include <QHoverEvent>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mModelData.h"
#include "mCaseImplicitModal.h"
#include "mBoundaryDisplacement.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mMixSet.h"
#include "mMapRelationManager.h"
#include "mLoadBoundaryManager.h"
#include "mMeshData.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseImplicitModalWindow::mCaseImplicitModalWindow(QWidget* parent)
        : QDialog(parent)
    {
        // 初始化完成前不要让窗口内容提前显示到屏幕上，避免“先出白框再填控件”的闪烁感
        setAttribute(Qt::WA_DontShowOnScreen, true);

        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        this->setWindowTitle("模态分析");
        this->resize(1000, 668);
        this->setStyleSheet(
            "QDialog { background-color: #ffffff; }"
            "QWidget { background-color: #ffffff; }"
            "QComboBox:enabled { background-color: white; }"
            "QComboBox:disabled { background-color: #F5F7FA; color: #808080; }"
            "QLineEdit { background-color: white; }"
            "QLineEdit:enabled { background-color: white; }"
            "QLineEdit:disabled { background-color: #F5F7FA; color: #808080; }"
            "QCheckBox { background-color: transparent; }"
            "QLabel { background-color: transparent; }"
            "QPushButton { background-color: white; }"
            "QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
        );
        initUI();
        initConnect();
        installInputValidators();
        applySolverDependentState();
        updateFarthestReachedIndex(0);
        updateValidationState();

        // 允许窗口在屏幕上显示后再调用 show()/raise()
        captureDefaultState();
        setAttribute(Qt::WA_DontShowOnScreen, false);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
        this->show();
        this->raise();
    }

    mCaseImplicitModalWindow::~mCaseImplicitModalWindow()
    {
    }

    bool mCaseImplicitModalWindow::eventFilter(QObject* watched, QEvent* event)
    {
        bool isNav = (watched == m_listWidget_Nav);
        bool isNavViewport = (m_listWidget_Nav && watched == m_listWidget_Nav->viewport());
        if (isNav || isNavViewport)
        {
            QEvent::Type type = event->type();
            // 只处理鼠标/悬停事件，焦点事件交给默认处理
            if (type == QEvent::MouseButtonPress ||
                type == QEvent::MouseButtonRelease ||
                type == QEvent::MouseButtonDblClick ||
                type == QEvent::MouseMove ||
                type == QEvent::HoverMove ||
                type == QEvent::HoverEnter ||
                type == QEvent::HoverLeave)
            {
                // 逐项判断可点击性：未解锁时仅进度页之前可点，解锁后为已到达过的步骤
                if (m_listWidget_Nav)
                {
                    QPoint pos;
                    if (type == QEvent::MouseButtonPress ||
                        type == QEvent::MouseButtonRelease ||
                        type == QEvent::MouseButtonDblClick ||
                        type == QEvent::MouseMove)
                    {
                        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                        pos = mouseEvent->pos();
                    }
                    else if (type == QEvent::HoverMove ||
                             type == QEvent::HoverEnter ||
                             type == QEvent::HoverLeave)
                    {
                        QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
                        pos = hoverEvent->pos();
                    }

                    if (!pos.isNull())
                    {
                        QPoint viewportPos = pos;
                        if (!isNavViewport && m_listWidget_Nav->viewport())
                        {
                            viewportPos = m_listWidget_Nav->viewport()->mapFromParent(pos);
                        }
                        QModelIndex index = m_listWidget_Nav->indexAt(viewportPos);
                        if (index.isValid())
                        {
                            int pageIndex = index.data(Qt::UserRole).toInt();
                            if (!isNavigationIndexEnabled(pageIndex))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return QDialog::eventFilter(watched, event);
    }

    bool mCaseImplicitModalWindow::isNavigationIndexEnabled(int index) const
    {
        if (index < 0 || !m_stackWidget)
        {
            return false;
        }
        if (!m_navUnlocked)
        {
            // 未解锁时，仅当前进度页（最远到达页）之前的步骤可点击
            return index < m_farthestReachedIndex;
        }
        // 解锁后，可以返回已经到达过或当前所在的步骤
        return index <= m_farthestReachedIndex;
    }

    void mCaseImplicitModalWindow::updateFarthestReachedIndex(int index)
    {
        if (index > m_farthestReachedIndex)
        {
            m_farthestReachedIndex = index;
        }
    }

    void mCaseImplicitModalWindow::resetSubsequentSteps(int fromIndex)
    {
        if (!m_navIconDelegate || !m_stackWidget)
        {
            return;
        }
        for (int i = fromIndex + 1; i < m_stackWidget->count(); ++i)
        {
            m_navIconDelegate->setPageCompleted(i, false);
        }
    }

    void mCaseImplicitModalWindow::onPageModified(int pageIndex)
    {
        // 修改已到达过的页：立即置其后的页为不可点击并清除完成标记（打勾仍统一在“下一步”时重新判断）
        if (pageIndex < m_farthestReachedIndex)
        {
            m_farthestReachedIndex = pageIndex;
            resetSubsequentSteps(pageIndex);
            if (m_navIconDelegate && m_stackWidget)
            {
                for (int i = 0; i < m_stackWidget->count(); ++i)
                {
                    m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
                }
                refreshNavigationIcons();
            }
        }
    }

    QString mCaseImplicitModalWindow::getCaseName() const
    {
        if (m_lineEdit_CaseName)
        {
            return m_lineEdit_CaseName->text();
        }
        return QString();
    }

    void mCaseImplicitModalWindow::initUI()
    {
        // 构造期间禁止重绘，避免“先出白框再填控件”的闪烁
        setUpdatesEnabled(false);

        // 主垂直布局：上方内容区 + 底部按钮栏
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 20, 20, 20);

        // 上方水平布局：左侧导航 + 分隔线 + 右侧内容
        QHBoxLayout* contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(0);
        contentLayout->setContentsMargins(0, 0, 0, 20);
        contentLayout->setAlignment(Qt::AlignTop);

        // 左侧竖向导航
        initLeftNavigation();
        if (m_listWidget_Nav)
        {
            contentLayout->addWidget(m_listWidget_Nav, 0, Qt::AlignTop);
        }

        // 分隔线
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::StyledPanel);
        line->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        line->setFixedWidth(1);
        line->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        contentLayout->addWidget(line);

        // 右侧堆叠内容区
        QVBoxLayout* rightLayout = new QVBoxLayout();
        rightLayout->setSpacing(0);
        rightLayout->setContentsMargins(20, 10, 0, 0);

        m_stackWidget = new QStackedWidget(this);
        m_stackWidget->addWidget(createPageAnalysisSetting());
        m_stackWidget->addWidget(createPageConstraint());
        m_stackWidget->addWidget(createPageOutput());
        m_stackWidget->setCurrentIndex(0);
        rightLayout->addWidget(m_stackWidget, 1);

        contentLayout->addLayout(rightLayout, 1);
        mainLayout->addLayout(contentLayout, 1);

        // 底部水平按钮布局
        m_bottomLayout = new QHBoxLayout();
        m_bottomLayout->setSpacing(10);
        m_bottomLayout->setContentsMargins(0, 0, 0, 0);
        m_bottomLayout->addStretch(1);

        m_pushButton_Previous = new QPushButton("上一步", this);
        m_pushButton_Previous->setFixedSize(110, 30);
        m_pushButton_Next = new QPushButton("下一步", this);
        m_pushButton_Next->setFixedSize(110, 30);
        m_pushButton_ExportFem = new QPushButton("导出工况", this);
        m_pushButton_ExportFem->setFixedSize(110, 30);
        m_pushButton_CreateMxCase = new QPushButton("创建Mx工况", this);
        m_pushButton_CreateMxCase->setFixedSize(110, 30);

        m_pushButton_Previous->setAutoDefault(false);
        m_pushButton_Next->setAutoDefault(false);
        m_pushButton_ExportFem->setAutoDefault(false);
        m_pushButton_CreateMxCase->setAutoDefault(false);

        // 蓝色主按钮样式
        QString primaryButtonStyle =
            "QPushButton { background-color: #1385F6; color: white; border: none; border-radius: 4px; padding: 6px 18px; }"
            "QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
            "QPushButton:hover:!disabled { background-color: #409eff; }";
        // 白色次按钮样式
        QString secondaryButtonStyle =
            "QPushButton { background-color: #ffffff; color: #606266; border: 1px solid #dcdfe6; border-radius: 4px; padding: 6px 18px; }"
            "QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
            "QPushButton:hover { border-color: #c0c4cc; }";

        m_pushButton_Previous->setStyleSheet(secondaryButtonStyle);
        m_pushButton_Next->setStyleSheet(primaryButtonStyle);
        m_pushButton_ExportFem->setStyleSheet(secondaryButtonStyle);
        m_pushButton_CreateMxCase->setStyleSheet(primaryButtonStyle);

        m_bottomLayout->addWidget(m_pushButton_Previous);
        m_bottomLayout->addWidget(m_pushButton_Next);
        m_bottomLayout->addWidget(m_pushButton_ExportFem);
        m_bottomLayout->addWidget(m_pushButton_CreateMxCase);

        mainLayout->addLayout(m_bottomLayout);

        updateBottomButtons(0);

        // 全部构造完成后再允许重绘
        setUpdatesEnabled(true);
    }

    void mCaseImplicitModalWindow::initLeftNavigation()
    {
        m_listWidget_Nav = new QListWidget(this);
        QFont font;
        font.setPointSize(10);
        m_listWidget_Nav->setFont(font);
        m_listWidget_Nav->setContentsMargins(0, 0, 0, 0);
        m_listWidget_Nav->setFixedWidth(200);
        m_listWidget_Nav->setFocusPolicy(Qt::NoFocus);
        m_listWidget_Nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setSpacing(0);
        m_listWidget_Nav->setFrameShape(QFrame::NoFrame);
        m_listWidget_Nav->setViewMode(QListView::ListMode);
        m_listWidget_Nav->setUniformItemSizes(true);
        m_listWidget_Nav->setItemDelegate(m_navIconDelegate = new NavIconDelegate(this));
        m_navIconDelegate->setOutputPageIndex(2);
        m_listWidget_Nav->setSelectionMode(QAbstractItemView::SingleSelection);

        QListWidgetItem* itemAnalysis = new QListWidgetItem("分析设置", m_listWidget_Nav);
        QListWidgetItem* itemConstraint = new QListWidgetItem("约束设置", m_listWidget_Nav);
        QListWidgetItem* itemOutput = new QListWidgetItem("输出设置", m_listWidget_Nav);
        itemAnalysis->setData(Qt::UserRole, 0);
        itemConstraint->setData(Qt::UserRole, 1);
        itemOutput->setData(Qt::UserRole, 2);
        itemAnalysis->setSizeHint(QSize(120, 48));
        itemConstraint->setSizeHint(QSize(120, 48));
        itemOutput->setSizeHint(QSize(120, 48));

        // 自定义导航栏样式：无边框、无背景、文字颜色
        m_listWidget_Nav->setStyleSheet(
            "QListWidget::item { background-color: transparent; color: #606266; "
            "  border: none; padding: 0px; padding-left: 16px; "
            "  min-height: 48px; max-height: 48px; }"
            "QListWidget { background-color: #ffffff; border: none; outline: none; }"
            "QListWidget::item:selected { background-color: transparent; color: #1385F6;}"
            "QListWidget::item:hover:!selected { background-color: transparent; }"
        );

        m_listWidget_Nav->setCurrentItem(itemAnalysis);

        // 安装事件过滤器以控制导航项的可点击范围
        if (m_listWidget_Nav)
        {
            m_listWidget_Nav->installEventFilter(this);
            if (m_listWidget_Nav->viewport())
            {
                m_listWidget_Nav->viewport()->installEventFilter(this);
            }
        }
    }

    void mCaseImplicitModalWindow::refreshNavigationIcons()
    {
        if (!m_listWidget_Nav || !m_navIconDelegate || !m_stackWidget)
        {
            return;
        }
        m_navIconDelegate->setCurrentPage(m_stackWidget->currentIndex());
        m_listWidget_Nav->viewport()->update();
    }

    QWidget* mCaseImplicitModalWindow::createPageAnalysisSetting()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(16);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // ---- 顶部：工况名称 + 预应力工况 ----
        QFormLayout* topFormLayout = new QFormLayout();
        topFormLayout->setSpacing(10);
        topFormLayout->setLabelAlignment(Qt::AlignLeft);
        topFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelCaseName = new QLabel("分析工况名称", this);
        labelCaseName->setFixedWidth(87);
        m_lineEdit_CaseName = new QLineEdit(this);
        m_lineEdit_CaseName->setFixedWidth(251);
        topFormLayout->addRow(labelCaseName, m_lineEdit_CaseName);

        m_checkBox_EnablePreload = new QCheckBox("预应力工况", this);
        m_checkBox_EnablePreload->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_EnablePreload->setFixedWidth(87);

        m_comboBox_PreloadCase = new QComboBox(this);
        m_comboBox_PreloadCase->setEditable(false);
        m_comboBox_PreloadCase->setEnabled(false);
        m_comboBox_PreloadCase->setFixedWidth(251);
        m_comboBox_PreloadCase->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        auto* preloadModelData = getCVM->getModelData();
        if (preloadModelData)
        {
            m_comboBox_PreloadCase->addItems(preloadModelData->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(ImplicitLinearStatic));
        }
        topFormLayout->addRow(m_checkBox_EnablePreload, m_comboBox_PreloadCase);

        pageLayout->addLayout(topFormLayout);

        // 分隔线 1
        QFrame* line1 = new QFrame(this);
        line1->setFrameShape(QFrame::HLine);
        line1->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line1);

        // ---- 中部：特征值求解方法 + 放大系数 ----
        QFormLayout* middleFormLayout = new QFormLayout();
        middleFormLayout->setSpacing(10);
        middleFormLayout->setLabelAlignment(Qt::AlignLeft);
        middleFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        // 特征值求解方法
        QLabel* labelSolver = new QLabel("特征值求解方法", this);
        labelSolver->setFixedWidth(87);
        m_comboBox_Solver = new QComboBox(this);
        m_comboBox_Solver->addItems({ "自动多重子结构", "兰索斯" });
        m_comboBox_Solver->setFixedWidth(251);
        m_comboBox_Solver->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        connect(m_comboBox_Solver, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mCaseImplicitModalWindow::slot_onSolverChanged);
        middleFormLayout->addRow(labelSolver, m_comboBox_Solver);

        // 放大系数
        m_checkBox_ModeScale = new QCheckBox("放大系数", this);
        m_checkBox_ModeScale->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_ModeScale->setChecked(true);
        m_checkBox_ModeScale->setFixedWidth(87);
        m_lineEdit_ModeScale = new QLineEdit(this);
        m_lineEdit_ModeScale->setText("5");
        m_lineEdit_ModeScale->setFixedWidth(251);
        middleFormLayout->addRow(m_checkBox_ModeScale, m_lineEdit_ModeScale);

        pageLayout->addLayout(middleFormLayout);

        // 分隔线 2
        QFrame* line2 = new QFrame(this);
        line2->setFrameShape(QFrame::HLine);
        line2->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line2);

        // ---- 底部：模态提取范围 ----
        m_rangeFormLayout = new QFormLayout();
        m_rangeFormLayout->setSpacing(10);
        m_rangeFormLayout->setLabelAlignment(Qt::AlignLeft);
        m_rangeFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelFreqMin = new QLabel("最小频率（Hz）", this);
        labelFreqMin->setFixedWidth(87);
        m_lineEdit_FreqMin = new QLineEdit(this);
        m_lineEdit_FreqMin->setFixedWidth(251);
        m_rangeFormLayout->addRow(labelFreqMin, m_lineEdit_FreqMin);

        QLabel* labelFreqMax = new QLabel("最大频率（Hz）", this);
        labelFreqMax->setFixedWidth(87);
        m_lineEdit_FreqMax = new QLineEdit(this);
        m_lineEdit_FreqMax->setFixedWidth(251);
        m_rangeFormLayout->addRow(labelFreqMax, m_lineEdit_FreqMax);

        QLabel* labelModeCount = new QLabel("模态阶数", this);
        labelModeCount->setFixedWidth(87);
        m_lineEdit_ModeCount = new QLineEdit(this);
        m_lineEdit_ModeCount->setFixedWidth(251);
        m_rangeFormLayout->addRow(labelModeCount, m_lineEdit_ModeCount);

        pageLayout->addLayout(m_rangeFormLayout);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitModalWindow::createPageConstraint()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);
        // 第一行：节点个数 + 添加一行 + 删除
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setSpacing(10);
        topLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* countLabel = new QLabel("节点个数：", this);
        m_lineEdit_PointCount = new QLineEdit(this);
        m_lineEdit_PointCount->setMinimumWidth(290);
        m_lineEdit_PointCount->setText("0");
        m_pushButton_AddRow = new QPushButton(this);
        m_pushButton_AddRow->setMaximumWidth(30);
        m_pushButton_RemoveRow = new QPushButton(this);
        m_pushButton_RemoveRow->setMaximumWidth(30);
        m_pushButton_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_pushButton_AddRow->setIconSize(QSize(14,14));
        m_pushButton_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_pushButton_RemoveRow->setIconSize(QSize(14, 14));
        m_pushButton_RemoveRow->setEnabled(false);
        m_pushButton_AddRow->setAutoDefault(false);
        m_pushButton_RemoveRow->setAutoDefault(false);
        topLayout->addWidget(countLabel);
        topLayout->addWidget(m_lineEdit_PointCount);
        topLayout->addWidget(m_pushButton_AddRow);
        topLayout->addWidget(m_pushButton_RemoveRow);
        topLayout->addStretch();
        m_pushButton_ImportFile = new QPushButton("导入文件", this);
        m_pushButton_ExportFile = new QPushButton("导出文件", this);
        topLayout->addWidget(m_pushButton_ImportFile);
        topLayout->addWidget(m_pushButton_ExportFile);

        pageLayout->addLayout(topLayout);
        m_nodetable = new IncentiveNodeTableWidget(this);
        m_nodetable->setEmptyDefault();   // 约束表初始不保留默认行，节点个数为0
        pageLayout->addWidget(m_nodetable, 1);
        return page;
    }

    static QIcon drawOutputFoldIcon(bool open)
    {
        qreal dpr = qApp->devicePixelRatio();
        const int px = 64;
        QPixmap pm(px * dpr, px * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);

        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const qreal w = px, h = px;
        QPainterPath path;
        if (open) {
            // ↓ 三角
            path.moveTo(0.20 * w, 0.35 * h);
            path.lineTo(0.80 * w, 0.35 * h);
            path.lineTo(0.50 * w, 0.75 * h);
        }
        else {
            // → 三角
            path.moveTo(0.35 * w, 0.20 * h);
            path.lineTo(0.75 * w, 0.50 * h);
            path.lineTo(0.35 * w, 0.80 * h);
        }
        path.closeSubpath();

        QColor fill("#909399");
        QColor outline(0, 0, 0, 70);

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);

        if (outline.alpha() > 0) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(outline, 1.2 * dpr));
            p.drawPath(path);
        }

        return QIcon(pm);
    }

    QWidget* mCaseImplicitModalWindow::createPageOutput()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        m_checkBox_OutputStress = new QCheckBox("应力", this);
        m_checkBox_OutputStress->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_OutputStrain = new QCheckBox("应变", this);
        m_checkBox_OutputStrain->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_OutputStrainEnergy = new QCheckBox("应变能", this);
        m_checkBox_OutputStrainEnergy->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_OutputDisplacement = new QCheckBox("位移", this);
        m_checkBox_OutputDisplacement->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");

        // 可折叠面板：响应点云图结果输出
        m_outputOp2FoldButton = new QPushButton("全局云图结果输出", this);
        m_outputOp2FoldButton->setIcon(drawOutputFoldIcon(true));
        m_outputOp2FoldButton->setLayoutDirection(Qt::LeftToRight);
        m_outputOp2FoldButton->setMinimumWidth(200);
        m_outputOp2FoldButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; background:#ffffff; color:#000000; font-weight:bold;}"
            "QPushButton:hover{background:#ffffff;}"
            "QPushButton:checked{background:#ffffff;}");
        connect(m_outputOp2FoldButton, &QPushButton::clicked, this, [this]() {
            bool visible = !m_outputOp2ContentWidget->isVisible();
            m_outputOp2ContentWidget->setVisible(visible);
            m_outputOp2FoldButton->setIcon(drawOutputFoldIcon(visible));
            });

        m_outputOp2ContentWidget = new QWidget(this);
        QVBoxLayout* outputOp2ContentLayout = new QVBoxLayout(m_outputOp2ContentWidget);
        outputOp2ContentLayout->setContentsMargins(0, 0, 0, 0);
        outputOp2ContentLayout->setSpacing(10);
        outputOp2ContentLayout->setAlignment(Qt::AlignTop);

        // 输出格式行
        QHBoxLayout* formatLayout = new QHBoxLayout();
        formatLayout->setSpacing(10);
        formatLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* formatLabel = new QLabel("输出格式", this);
        m_comboBox_OutputFormat = new QComboBox(this);
        m_comboBox_OutputFormat->addItems({ "mxdb", "op2", "h5" });   // 与工况面板的输出文件类型一致，默认 mxdb
        m_comboBox_OutputFormat->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_comboBox_OutputFormat->setMinimumWidth(200);
        formatLayout->addWidget(formatLabel);
        formatLayout->addWidget(m_comboBox_OutputFormat);
        formatLayout->addStretch();

        outputOp2ContentLayout->addLayout(formatLayout);

        // 物理量设置标签
        QLabel* physicalQuantityLabel = new QLabel("物理量设置：", this);

        QHBoxLayout* foldButtonLayout = new QHBoxLayout();
        foldButtonLayout->setContentsMargins(0, 0, 0, 0);
        foldButtonLayout->setSpacing(0);
        foldButtonLayout->addWidget(m_outputOp2FoldButton);
        foldButtonLayout->addStretch();
        pageLayout->addLayout(foldButtonLayout);
        pageLayout->addWidget(m_outputOp2ContentWidget);
        pageLayout->addWidget(physicalQuantityLabel);
        pageLayout->addWidget(m_checkBox_OutputDisplacement);
        pageLayout->addWidget(m_checkBox_OutputStress);
        pageLayout->addWidget(m_checkBox_OutputStrain);
        pageLayout->addWidget(m_checkBox_OutputStrainEnergy);
        pageLayout->addStretch(1);

        return page;
    }

    void mCaseImplicitModalWindow::updateBottomButtons(int currentIndex)
    {
        if (!m_pushButton_Previous || !m_pushButton_Next ||
            !m_pushButton_ExportFem || !m_pushButton_CreateMxCase || !m_stackWidget)
        {
            return;
        }

        const int lastIndex = m_stackWidget->count() - 1;

        // 第一页：只有“下一步”
        if (currentIndex == 0)
        {
            m_pushButton_Previous->setVisible(false);
            m_pushButton_Next->setVisible(true);
            m_pushButton_ExportFem->setVisible(false);
            m_pushButton_CreateMxCase->setVisible(false);
        }
        // 最后一页（输出设置）：隐藏“上一步/下一步”，显示“导出工况 | 创建Mx工况”
        else if (currentIndex == lastIndex)
        {
            m_pushButton_Previous->setVisible(false);
            m_pushButton_Next->setVisible(false);
            m_pushButton_ExportFem->setVisible(true);
            m_pushButton_CreateMxCase->setVisible(true);
            updateOutputActionButtons();
        }
        // 中间页：显示“上一步 | 下一步”
        else
        {
            m_pushButton_Previous->setVisible(true);
            m_pushButton_Next->setVisible(true);
            m_pushButton_ExportFem->setVisible(false);
            m_pushButton_CreateMxCase->setVisible(false);
        }

        updateNextButtonText(currentIndex);
    }

    /*
    * 构造完成后快照控件默认状态（表格内部单元格控件不参与，随表格整体重置）
    */
    void mCaseImplicitModalWindow::captureDefaultState()
    {
        auto insideTable = [](QWidget* w) {
            for (QWidget* p = w->parentWidget(); p; p = p->parentWidget())
            {
                if (qobject_cast<QTableWidget*>(p))
                    return true;
            }
            return false;
        };
        const QList<QLineEdit*> edits = findChildren<QLineEdit*>();
        for (QLineEdit* w : edits)
        {
            if (!insideTable(w))
                m_defaultEditTexts[w] = w->text();
        }
        const QList<QCheckBox*> checks = findChildren<QCheckBox*>();
        for (QCheckBox* w : checks)
        {
            if (!insideTable(w))
                m_defaultChecks[w] = w->isChecked();
        }
        const QList<QComboBox*> combos = findChildren<QComboBox*>();
        for (QComboBox* w : combos)
        {
            if (!insideTable(w))
                m_defaultComboIndex[w] = w->currentIndex();
        }
        const QList<QTableWidget*> tables = findChildren<QTableWidget*>();
        for (QTableWidget* t : tables)
        {
            bool inPanel = false;
            for (QWidget* p = t->parentWidget(); p; p = p->parentWidget())
            {
                if (qobject_cast<ResponseSettingFoldablePanel*>(p))
                {
                    inPanel = true;
                    break;
                }
            }
            if (!qobject_cast<IncentiveNodeTableWidget*>(t) && !qobject_cast<IncentiveSettingTableWidget*>(t) && !inPanel)
            {
                // 表格加入布局后父组件会被重定向，需沿祖先链判断是否属于响应面板
                m_defaultRowCount[t] = t->rowCount();
            }
        }
    }

    /*
    * 恢复窗口默认状态：导航/进度复位 + 表格重置 + 控件默认值恢复
    */
    void mCaseImplicitModalWindow::resetToDefault()
    {
        m_navUnlocked = false;
        m_farthestReachedIndex = 0;
        m_mxCaseCreated = false;
        if (m_stackWidget)
        {
            m_stackWidget->setCurrentIndex(0);
        }
        if (m_listWidget_Nav)
        {
            QSignalBlocker blocker(m_listWidget_Nav);
            m_listWidget_Nav->setCurrentRow(0);
        }
        // 激励/约束等共享表格整体重置（单元格控件随行销毁重建）
        const QList<IncentiveNodeTableWidget*> nodeTables = findChildren<IncentiveNodeTableWidget*>();
        for (IncentiveNodeTableWidget* t : nodeTables)
        {
            t->resetToDefault();
        }
        const QList<IncentiveSettingTableWidget*> settingTables = findChildren<IncentiveSettingTableWidget*>();
        for (IncentiveSettingTableWidget* t : settingTables)
        {
            t->resetToDefault();
        }
        // 响应设置面板：清空后按各自添加行逻辑重建
        const QList<ResponseSettingFoldablePanel*> panels = findChildren<ResponseSettingFoldablePanel*>();
        for (ResponseSettingFoldablePanel* p : panels)
        {
            p->resetToDefault();
        }

        // 其余无建行入口的表格：仅恢复默认行数（默认均为空行）
        for (auto it = m_defaultRowCount.begin(); it != m_defaultRowCount.end(); ++it)
        {
            QTableWidget* t = it.key();
            t->clearContents();
            t->setRowCount(it.value());
        }
        // 控件默认值恢复（信号正常触发以联动依赖控件的启用状态）
        for (auto it = m_defaultEditTexts.begin(); it != m_defaultEditTexts.end(); ++it)
        {
            it.key()->setText(it.value());
        }
        for (auto it = m_defaultChecks.begin(); it != m_defaultChecks.end(); ++it)
        {
            it.key()->setChecked(it.value());
        }
        for (auto it = m_defaultComboIndex.begin(); it != m_defaultComboIndex.end(); ++it)
        {
            it.key()->setCurrentIndex(it.value());
        }
        applySolverDependentState();
        updateBottomButtons(0);
        updateValidationState();
    }

    void mCaseImplicitModalWindow::closeEvent(QCloseEvent* event)
    {
        // 工况未创建时关闭需二次确认
        if (!m_mxCaseCreated)
        {
            if (QMessageBox::question(this, QString("提示"), QString("工况未创建，确定要关闭吗？"),
                QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok)
            {
                event->ignore();
                return;
            }
        }
        QDialog::closeEvent(event);
    }

    void mCaseImplicitModalWindow::slot_onClose()
    {
        this->close();
    }

    bool mCaseImplicitModalWindow::isValidDouble(QLineEdit* lineEdit) const
    {
        if (!lineEdit)
        {
            return false;
        }
        QString text = lineEdit->text().trimmed();
        bool ok = false;
        text.toDouble(&ok);
        return ok;
    }

    bool mCaseImplicitModalWindow::isValidPositiveInt(QLineEdit* lineEdit) const
    {
        if (!lineEdit)
        {
            return false;
        }
        QString text = lineEdit->text().trimmed();
        bool ok = false;
        int value = text.toInt(&ok);
        return ok && value > 0;
    }

    void mCaseImplicitModalWindow::applySolverDependentState()
    {
        if (!m_comboBox_Solver)
        {
            return;
        }
        const QString solver = m_comboBox_Solver->currentText();
        const bool isAMS = (solver == "自动多重子结构");
        const bool isLanczos = (solver == "兰索斯");

        // 最大频率
        if (m_lineEdit_FreqMax)
        {
            m_lineEdit_FreqMax->setEnabled(true);
        }

        // 模态阶数
        if (m_lineEdit_ModeCount)
        {
            m_lineEdit_ModeCount->setEnabled(true);
        }

        // 放大系数：AMS 默认勾选且值为5；兰索斯禁用且取消勾选
        if (m_checkBox_ModeScale)
        {
            if (isAMS)
            {
                m_checkBox_ModeScale->setEnabled(true);
                m_checkBox_ModeScale->setChecked(true);
                if (m_lineEdit_ModeScale)
                {
                    if (m_lineEdit_ModeScale->text().trimmed().isEmpty())
                    {
                        m_lineEdit_ModeScale->setText("5");
                    }
                    m_lineEdit_ModeScale->setEnabled(true);
                }
            }
            else if (isLanczos)
            {
                m_checkBox_ModeScale->setChecked(false);
                m_checkBox_ModeScale->setEnabled(false);
                if (m_lineEdit_ModeScale)
                {
                    m_lineEdit_ModeScale->setEnabled(false);
                }
            }
        }
    }

    void mCaseImplicitModalWindow::slot_onSolverChanged()
    {
        applySolverDependentState();
    }

    // 输入框输入限制：数值框仅允许合法数值形态，整数框仅允许数字，非法输入直接置空
    void mCaseImplicitModalWindow::installInputValidators()
    {
        QList<QLineEdit*> textEdits = { m_lineEdit_CaseName };
        QList<QLineEdit*> intEdits = { m_lineEdit_ModeCount, m_lineEdit_PointCount };

        const QList<QLineEdit*> allEdits = findChildren<QLineEdit*>();
        for (QLineEdit* edit : allEdits)
        {
            if (!edit || textEdits.contains(edit))
                continue;
            if (intEdits.contains(edit))
                installIntInputFilter(edit);
            else
                installDoubleInputFilter(edit);
        }
    }

    void mCaseImplicitModalWindow::initConnect()
    {
        // 放大系数勾选控制输入框是否可编辑
        connect(m_checkBox_ModeScale, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_lineEdit_ModeScale)
            {
                m_lineEdit_ModeScale->setEnabled(checked && m_checkBox_ModeScale->isEnabled());
                if (m_lineEdit_ModeScale->text().trimmed().isEmpty())
                {
                    m_lineEdit_ModeScale->setText("5");
                }
            }
            onPageModified(0);
            });
        // 左侧导航切换（未解锁时仅可点击进度页之前的步骤，解锁后可点击已到达过的步骤）
        connect(m_listWidget_Nav, &QListWidget::currentRowChanged, this, [this](int row) {
            int currentRow = m_stackWidget->currentIndex();
            if (row >= 0 && row < m_stackWidget->count() && isNavigationIndexEnabled(row))
            {
                if (QWidget* fw = QApplication::focusWidget())
                {
                    fw->clearFocus();
                }
                m_stackWidget->setCurrentIndex(row);
                if (m_stackWidget->currentWidget())
                {
                    m_stackWidget->currentWidget()->setFocus();
                }
                refreshNavigationIcons();
                updateBottomButtons(row);
            }
            else if (m_listWidget_Nav && m_listWidget_Nav->currentRow() != currentRow)
            {
                // 非法目标，恢复到当前 stack 页
                QSignalBlocker blocker(m_listWidget_Nav);
                m_listWidget_Nav->setCurrentRow(currentRow);
            }
            });
        connect(m_pushButton_Next, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onNextStep);
        connect(m_pushButton_ExportFem, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onExportFem);
        connect(m_checkBox_EnablePreload, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_comboBox_PreloadCase)
            {
                m_comboBox_PreloadCase->setEnabled(checked);
                if (!checked)
                {
                    m_comboBox_PreloadCase->setCurrentIndex(0);
                }
            }
            onPageModified(0);
            });
        connect(m_pushButton_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddConstraintRow();
            onPageModified(1);
            });
        connect(m_lineEdit_PointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onPointCountChanged();
            onPageModified(1);
            });
        connect(m_pushButton_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveConstraintRow();
            onPageModified(1);
            });
        connect(m_pushButton_ImportFile, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onImportConstraintFile);
        connect(m_pushButton_ExportFile, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onExportConstraintFile);
        connect(m_pushButton_Previous, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onPreviousStep);
        connect(m_pushButton_CreateMxCase, &QPushButton::clicked, this, &mCaseImplicitModalWindow::slot_onCreateMxCase);
        connect(m_nodetable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                updateDeleteButtonState();
            });
        connect(m_nodetable, &IncentiveNodeTableWidget::itemChanged, this, [this]() {
            onPageModified(1);
            });

        // 分析设置输入变化时刷新完成状态并自动勾选输出
        connect(m_lineEdit_FreqMax, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_ModeCount, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_FreqMin, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_ModeScale, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_CaseName, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_comboBox_Solver, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            onPageModified(0);
            });
        connect(m_comboBox_PreloadCase, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            onPageModified(0);
            });

        // 输出设置复选框变化时刷新按钮状态
        connect(m_checkBox_OutputStress, &QCheckBox::toggled, this, [this]() {
            onPageModified(2);
            updateOutputActionButtons();
            });
        connect(m_checkBox_OutputStrain, &QCheckBox::toggled, this, [this]() {
            onPageModified(2);
            updateOutputActionButtons();
            });
        connect(m_checkBox_OutputStrainEnergy, &QCheckBox::toggled, this, [this]() {
            onPageModified(2);
            updateOutputActionButtons();
            });
        connect(m_checkBox_OutputDisplacement, &QCheckBox::toggled, this, [this]() {
            onPageModified(2);
            updateOutputActionButtons();
            });
        connect(m_comboBox_OutputFormat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            onPageModified(2);
            });
    }

    void mCaseImplicitModalWindow::updateDeleteButtonState()
    {
        if (!m_pushButton_RemoveRow || !m_nodetable)
        {
            return;
        }
        m_pushButton_RemoveRow->setEnabled(!m_nodetable->selectionModel()->selectedRows().isEmpty());
    }

    bool mCaseImplicitModalWindow::isAnalysisSettingValid() const
    {
        if (!m_comboBox_Solver)
        {
            return false;
        }
        const QString solver = m_comboBox_Solver->currentText();
        bool hasValidModeCount = isValidPositiveInt(m_lineEdit_ModeCount);
        bool hasValidFreqMax = isValidDouble(m_lineEdit_FreqMax);

        if (solver == "自动多重子结构")
        {
            // 自动多重子结构：最大频率必填
            return hasValidFreqMax;
        }
        else if (solver == "兰索斯")
        {
            // 兰索斯：最大频率和模态阶数二者任选其一
            return hasValidModeCount || hasValidFreqMax;
        }
        return hasValidModeCount || hasValidFreqMax;
    }

    bool mCaseImplicitModalWindow::isConstraintValid() const
    {
        // 约束设置允许为空，始终视为有效
        return true;
    }

    bool mCaseImplicitModalWindow::isOutputValid() const
    {
        int checkedCount = 0;
        if (m_checkBox_OutputStress && m_checkBox_OutputStress->isChecked()) ++checkedCount;
        if (m_checkBox_OutputStrain && m_checkBox_OutputStrain->isChecked()) ++checkedCount;
        if (m_checkBox_OutputStrainEnergy && m_checkBox_OutputStrainEnergy->isChecked()) ++checkedCount;
        if (m_checkBox_OutputDisplacement && m_checkBox_OutputDisplacement->isChecked()) ++checkedCount;
        return checkedCount >= 1;
    }

    void mCaseImplicitModalWindow::updateValidationState()
    {
        if (!m_navIconDelegate || !m_stackWidget)
        {
            return;
        }

        int currentIndex = m_stackWidget->currentIndex();

        // 分析设置：有效时打勾
        m_navIconDelegate->setPageCompleted(0, m_farthestReachedIndex > 0 && isAnalysisSettingValid());

        // 约束设置：通过“下一步”离开该页后打勾
        m_navIconDelegate->setPageCompleted(1, m_farthestReachedIndex > 1 && isConstraintValid());

        // 输出设置：不用打勾图标，所以永远不打勾
        m_navIconDelegate->setPageCompleted(2, false);

        // 未解锁时仅当前进度页之前的步骤可点击；解锁后允许已到达过的步骤
        for (int i = 0; i < m_stackWidget->count(); ++i)
        {
            m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
        }

        refreshNavigationIcons();
        updateOutputActionButtons();
    }

    void mCaseImplicitModalWindow::updateOutputActionButtons()
    {
        bool analysisCompleted = m_navIconDelegate && m_navIconDelegate->isPageCompleted(0);
        bool outputValid = isOutputValid();
        bool enabled = analysisCompleted && outputValid;
        if (m_pushButton_ExportFem)
        {
            m_pushButton_ExportFem->setEnabled(enabled);
        }
        if (m_pushButton_CreateMxCase)
        {
            m_pushButton_CreateMxCase->setEnabled(enabled);
        }
    }

    void mCaseImplicitModalWindow::slot_onCreateMxCase()
    {
        QString caseName = m_lineEdit_CaseName ? m_lineEdit_CaseName->text().trimmed() : QString();
        auto* modelData = getCVM->getModelData();
        if (!modelData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("模型网格数据不存在，无法创建工况"));
            return;
        }

        // 0. 前置校验：约束节点ID必须存在于网格数据中
        mMeshData* meshData = modelData->getMeshData();
        if (meshData)
        {
            QStringList missingIds;
            for (int i = 0; i < m_nodetable->rowCount(); ++i)
            {
                auto row = m_nodetable->getRowData(i);
                QString gridText = row.nodeId;
                if (gridText.isEmpty())
                {
                    continue;
                }
                bool ok = false;
                int id = gridText.trimmed().toInt(&ok);
                if (!ok || id <= 0)
                {
                    continue;
                }
                if (!meshData->_nodedata.nodeidmap.contains(id))
                {
                    missingIds.append(gridText);
                }
            }
            if (!missingIds.isEmpty())
            {
                QString msg = QString("以下节点id在界面网格数据中不存在：%1").arg(missingIds.join(","));
                QMessageBox::warning(this, QString("提示"), msg);
                return;
            }
        }

        // 1. 创建模态分析工况
        int caseID = modelData->getManager<mCaseManager>(Physical_Case).createID();
        int boundaryID = -1;
        mCaseImplicitModal* caseData = new mCaseImplicitModal(caseID, ImplicitModal, caseName);
        caseData->setCaseName(caseName);
        modelData->addPhysicalField<mCaseBase>(caseData);

        // 2. 预应力设置
        if (m_checkBox_EnablePreload && m_checkBox_EnablePreload->isChecked())
        {
            QString preloadName = m_comboBox_PreloadCase->currentText();
            if (!preloadName.isEmpty() && preloadName != "请选择")
            {
                int preloadID = modelData->getManager<mCaseManager>(Physical_Case).getIDByCaseName(preloadName);
                if (preloadID != -1)
                {
                    modelData->getMapRelationManager().appendMapRelation(
                        { Physical_Case, caseID },
                        { Physical_Case, preloadID },
                        MapRelation_None);
                }
            }
        }

        // 3. 分析设置
        if (m_comboBox_Solver)
        {
            caseData->_modalSolver = m_comboBox_Solver->currentText();
        }
        caseData->_minFrequency = m_lineEdit_FreqMin ? m_lineEdit_FreqMin->text() : QString();
        caseData->_maxFrequency = m_lineEdit_FreqMax ? m_lineEdit_FreqMax->text() : QString();
        caseData->_order = m_lineEdit_ModeCount ? m_lineEdit_ModeCount->text() : QString();
        if (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked())
        {
            caseData->_ampffact = m_lineEdit_ModeScale ? m_lineEdit_ModeScale->text() : QString();
        }
        else
        {
            caseData->_ampffact.clear();
        }

        // 4. 约束设置：收集所有行节点数据并创建单个内部节点集和位移/转角边界
        QString baseName = "NVH_DispRot";
        std::map<RenderPickResultData, QVector<QVariant>> independentTable;
        if (!meshData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("模型网格数据不存在，无法创建约束"));
        }
        else if (!m_nodetable)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("约束表格不存在，无法创建约束"));
        }
        else
        {
            for (int i = 0; i < m_nodetable->rowCount(); ++i)
            {
                auto row = m_nodetable->getRowData(i);
                QString gridText = row.nodeId.trimmed();
                if (gridText.isEmpty())
                {
                    continue;
                }

                bool ok = false;
                int id = gridText.trimmed().toInt(&ok);
                if (!ok || id <= 0)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        QString("约束设置中节点ID%1格式无效。").arg(gridText));
                    continue;
                }

                if (!meshData->_nodedata.nodeidmap.contains(id))
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        QString("约束设置中节点ID%1在模型中不存在。").arg(gridText));
                    continue;
                }

                QVector<QVariant> values(6);
                if (row.x) values[0] = 0.0;
                if (row.y) values[1] = 0.0;
                if (row.z) values[2] = 0.0;
                if (row.rx) values[3] = 0.0;
                if (row.ry) values[4] = 0.0;
                if (row.rz) values[5] = 0.0;

                independentTable[RenderPickResultData(id, -1, -1)] = std::move(values);
            }
            if (!independentTable.empty())
            {
                boundaryID = modelData->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
                std::shared_ptr<mBoundaryDisplacement> boundary = std::make_shared<mBoundaryDisplacement>(boundaryID, BOUNDARY_IM_DISPLACEMENT, baseName);
                boundary->_definitionMode = 1; // 独立赋值模式
                boundary->_independentTable = independentTable;
                modelData->addPhysicalFieldP(boundary, caseID);
                modelData->getMapRelationManager().appendMapRelation(
                    { Physical_Case, caseID },
                    { Physical_LoadBoundary, boundaryID });
            }
        }

        // 5. 输出设置：先清空云图输出控制与PCH输出控制，再写入勾选的输出
        caseData->setOutputs({});
        caseData->setPchOutputs({});
        caseData->setSelectedSet({});
        caseData->setPchSelectedSet({});
        //    物理量按输出格式与工况面板（mCaseModalPanel）的可选项对应
        //    mxdb：位移、单元应变能；op2：位移、单元应力、单元应变能；h5：仅位移；应变暂不处理
        QHash<QString, QStringList> outputs = caseData->getOutputs();
        const QString outputFormat = m_comboBox_OutputFormat ? m_comboBox_OutputFormat->currentText() : QString("mxdb");
        if (m_checkBox_OutputDisplacement && m_checkBox_OutputDisplacement->isChecked())
        {
            QStringList& list = outputs["基本结果"];
            if (!list.contains("位移"))
            {
                list.append("位移");
            }
        }
        if (outputFormat == "op2")
        {
            if (m_checkBox_OutputStress && m_checkBox_OutputStress->isChecked())
            {
                QStringList& list = outputs["应力"];
                if (!list.contains("单元应力"))
                {
                    list.append("单元应力");
                }
            }
        }
        if (outputFormat == "op2" || outputFormat == "mxdb")
        {
            if (m_checkBox_OutputStrainEnergy && m_checkBox_OutputStrainEnergy->isChecked())
            {
                QStringList& list = outputs["能量"];
                if (!list.contains("单元应变能"))
                {
                    list.append("单元应变能");
                }
            }
        }
        caseData->setOutputs(outputs);
        // 输出文件类型与工况面板（mCaseModalPanel）对应
        caseData->setOutputType(outputFormat);

        // 6. 触发模型树更新
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, caseID);
        emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(caseID, boundaryID, 0);
        emit MechCommon::mMechGlobalSignals::getInstance()->updataLoadBoundaryTreeView(-1, boundaryID, 0);
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("创建 Mx 工况：%1").arg(caseName));
        this->accept();

        // 工况已创建，关闭时不再提示
        m_mxCaseCreated = true;
    }

    void mCaseImplicitModalWindow::slot_onExportFem()
    {
        QString fileName = QFileDialog::getSaveFileName(this, "导出 fem 文件", QString(), "fem 文件 (*.fem);;所有文件 (*.*)");
        if (fileName.isEmpty())
        {
            return;
        }
        QString caseName = m_lineEdit_CaseName ? m_lineEdit_CaseName->text().trimmed() : QString("ModalAnalysis");
        if (caseName.isEmpty())
        {
            caseName = "ModalAnalysis";
        }

        QString solverText = m_comboBox_Solver->currentText();
        QString freqMin = m_lineEdit_FreqMin->text().trimmed();
        QString freqMax = m_lineEdit_FreqMax->text().trimmed();
        QString modeCount = m_lineEdit_ModeCount->text().trimmed();
        bool modeScaleEnabled = m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked();
        QString modeScale = modeScaleEnabled ? m_lineEdit_ModeScale->text().trimmed() : QString();

        QVector<int> spcNodeIds;
        QMap<int, QString> spcDofMap;
        for (int i = 0; i < m_nodetable->rowCount(); ++i)
        {
            auto row = m_nodetable->getRowData(i);
            QString gridText = row.nodeId;
            if (gridText.isEmpty())
            {
                continue;
            }
            std::vector<int> rowNodeIds;
            bool ok = false;
            int id = gridText.trimmed().toInt(&ok);
            if (!ok || id <= 0)
            {
                continue;
            }
            spcNodeIds.push_back(id);
            QString dofCode;
            if (row.x) dofCode += "1";
            if (row.y) dofCode += "2";
            if (row.z) dofCode += "3";
            if (row.rx) dofCode += "4";
            if (row.ry) dofCode += "5";
            if (row.rz) dofCode += "6";
            if (dofCode.isEmpty())
            {
                continue;
            }
            spcDofMap[id] = dofCode;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("无法打开文件：%1").arg(fileName));
            return;
        }
        QTextStream out(&file);
        out.setCodec("UTF-8");
        const int caseID = 1;
        const int boundartID = 1;
        const int methodID = 1;

        // 小场格式辅助：卡名左对齐、数据字段右对齐，每字段固定 8 字符宽，超出部分截断
        auto writeBulkCard = [&out](const QStringList& fields)
        {
            QString line;
            for (int i = 0; i < fields.size(); ++i)
            {
                QString f = fields.at(i);
                if (f.length() > 8)
                {
                    f = f.left(8);
                }
                line += (i == 0) ? f.leftJustified(8) : f.rightJustified(8);
            }
            out << line << "\n";
        };

        //// ---- 执行控制段：法向模态分析 ----
        //out << "$ Executive control: normal modes analysis (SOL 103)\n";
        //out << "SOL 103\n";
        //out << "CEND\n";

        // ---- 工况控制段：选择约束集、特征值方法与结果输出 ----
        out << "$ Case control: constraint set, eigenvalue method and output requests\n";
        out << "SUBCASE " << caseID << "\n";
        out << "LABEL " << caseName << "\n";
        out << "  ANALYSIS MODES\n";
        out << "  METHOD = " << methodID << "\n";
        out << "  SPC = " << boundartID << "\n";
        if (m_checkBox_OutputDisplacement && m_checkBox_OutputDisplacement->isChecked())
        {
            out << "$ Output request: displacement\n";
            out << "  DISPLACEMENT = ALL\n";
        }
        if (m_checkBox_OutputStrainEnergy && m_checkBox_OutputStrainEnergy->isChecked())
        {
            out << "$ Output request: strain energy\n";
            out << "  ESE = ALL\n";
        }
        if (m_checkBox_OutputStrain && m_checkBox_OutputStrain->isChecked())
        {
            out << "$ Output request: strain (von Mises)\n";
            out << "  STRAIN(VON) = ALL\n";
        }
        if (m_checkBox_OutputStress && m_checkBox_OutputStress->isChecked())
        {
            out << "$ Output request: stress (von Mises)\n";
            out << "  STRESS(VON) = YES\n";
        }
        if (m_checkBox_EnablePreload && m_checkBox_EnablePreload->isChecked())
        {
            out << "$ Preload subcase: static preload\n";
            out << "  STATSUB = PRELOAD\n";
        }

        // ---- 批数据段：以下卡片均为 8 字符定宽小场格式 ----
        out << "$ Bulk data section: small field format (8-char fixed width)\n";
        out << "BEGIN BULK\n";
        out << "$ PARAM card: enable AMSES automated multi-stage substructure eigensolver\n";
        writeBulkCard({ "PARAM", "AMSES", "YES" });
        out << "$ PARAM card: automatically constrain unconnected DOF\n";
        writeBulkCard({ "PARAM", "AUTOSPC", "YES" });

        bool okMin = false, okMax = false;
        double fMin = freqMin.toDouble(&okMin);
        double fMax = freqMax.toDouble(&okMax);
        const QString v1 = QString::number(okMin ? fMin : 0.0, 'f', 1);
        const QString v2 = QString::number(okMax ? fMax : 0.0, 'f', 1);
        const QString nd = modeCount.isEmpty() ? QString("0") : QString::number(modeCount.toInt());
        if (solverText == "兰索斯")
        {
            out << "$ EIGRL card (Lanczos): SID=" << methodID
                << ", freq range V1-V2 (Hz), number of modes ND\n";
            writeBulkCard({ "EIGRL", QString::number(methodID), v1, v2, nd });
        }
        else
        {
            const QString norm = modeScale.isEmpty() ? QString("0.0") : QString::number(modeScale.toDouble());
            out << "$ EIGRA card (AMSES): SID=" << methodID
                << ", freq range V1-V2 (Hz), number of modes ND, amplification factor\n";
            writeBulkCard({ "EIGRA", QString::number(methodID), v1, v2, nd, norm });
        }

        for (int nid : spcNodeIds)
        {
            out << "$ SPC card: SID=" << boundartID << ", node G=" << nid
                << ", constrained DOF C=" << spcDofMap[nid] << ", enforced displacement D=0\n";
            writeBulkCard({ "SPC", QString::number(boundartID), QString::number(nid),
                spcDofMap[nid], QString::number(0.0, 'f', 1) });
        }

        out << "$ End of bulk data\n";
        out << "ENDDATA\n";

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("导出 fem 文件：%1").arg(fileName));
    }

    void mCaseImplicitModalWindow::slot_onAddConstraintRow()
    {
        m_nodetable->addRow();
        m_lineEdit_PointCount->setText(MxFunctions::mNumberToString(m_nodetable->rowCount()));
    }

    void mCaseImplicitModalWindow::slot_onRemoveConstraintRow()
    {
        m_nodetable->removeSelectedRows();
        m_lineEdit_PointCount->setText(MxFunctions::mNumberToString(m_nodetable->rowCount()));
    }

    void mCaseImplicitModalWindow::slot_onPointCountChanged()
    {
        m_nodetable->setRowCount(m_lineEdit_PointCount->text().toInt());
    }

    void mCaseImplicitModalWindow::slot_onImportConstraintFile()
    {
        if (!m_nodetable)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入约束文件",
            QString(),
            "文本文件 (*.txt *.csv)");
        if (fileName.isEmpty())
        {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("无法打开文件：%1").arg(fileName));
            return;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> importedData;
        int lineIndex = 0;
        bool hasHeader = false;

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            ++lineIndex;
            if (line.isEmpty())
            {
                continue;
            }

            // 以英文逗号作为唯一分隔符
            QStringList parts = line.split(',');
            if (parts.size() != 7)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 7。").arg(lineIndex));
                file.close();
                return;
            }

            // 判断是否为表头行（首列非纯数字即视为表头）
            QString firstCol = parts[0].trimmed();
            if (firstCol.compare("节点ID", Qt::CaseInsensitive) == 0 ||
                firstCol.compare("nodeid", Qt::CaseInsensitive) == 0 ||
                firstCol.compare("id", Qt::CaseInsensitive) == 0)
            {
                hasHeader = true;
                continue;
            }

            bool idOk = false;
            int nodeId = firstCol.toInt(&idOk);
            if (!idOk || nodeId <= 0)
            {
                // 首行若未命中表头关键字但非数字，也视为表头，避免误拦截
                if (lineIndex == 1)
                {
                    hasHeader = true;
                    continue;
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行节点ID格式无效。").arg(lineIndex));
                file.close();
                return;
            }

            IncentiveNodeTableWidget::IncentiveNodeRowData row;
            row.nodeId = MxFunctions::mNumberToString(nodeId);
            row.x = parts[1].trimmed().toInt() != 0;
            row.y = parts[2].trimmed().toInt() != 0;
            row.z = parts[3].trimmed().toInt() != 0;
            row.rx = parts[4].trimmed().toInt() != 0;
            row.ry = parts[5].trimmed().toInt() != 0;
            row.rz = parts[6].trimmed().toInt() != 0;
            importedData.append(row);
        }

        file.close();

        if (importedData.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("导入失败：文件中没有有效数据。"));
            return;
        }

        m_nodetable->setTableData(importedData);
        m_lineEdit_PointCount->setText(MxFunctions::mNumberToString(m_nodetable->rowCount()));
        onPageModified(1);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条约束数据。").arg(importedData.size()));
    }

    void mCaseImplicitModalWindow::slot_onExportConstraintFile()
    {
        if (!m_nodetable)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出约束文件",
            QString(),
            "CSV 文件 (*.csv);;文本文件 (*.txt)");
        if (fileName.isEmpty())
        {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("无法打开文件：%1").arg(fileName));
            return;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");

        // 表头
        out << "NodeID, X, Y, Z, RX, RY, RZ\n";

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> data = m_nodetable->getTableData();
        for (const IncentiveNodeTableWidget::IncentiveNodeRowData& row : data)
        {
            // 只导出节点ID有效的行
            QString gridText = row.nodeId.trimmed();
            if (gridText.isEmpty())
            {
                continue;
            }
            bool ok = false;
            int id = gridText.toInt(&ok);
            if (!ok || id <= 0)
            {
                continue;
            }

            out << MxFunctions::mNumberToString(id) << ", "
                << (row.x ? "1" : "0") << ", "
                << (row.y ? "1" : "0") << ", "
                << (row.z ? "1" : "0") << ", "
                << (row.rx ? "1" : "0") << ", "
                << (row.ry ? "1" : "0") << ", "
                << (row.rz ? "1" : "0") << "\n";
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出约束文件：%1").arg(fileName));
    }

    void mCaseImplicitModalWindow::updateNextButtonText(int currentIndex)
    {
        if (!m_pushButton_Next)
        {
            return;
        }
        if (currentIndex < m_stackWidget->count() - 1)
        {
            m_pushButton_Next->setText("下一步");
        }
        else
        {
            m_pushButton_Next->setText("创建Mx工况");
        }
    }

    void mCaseImplicitModalWindow::slot_onNextStep()
    {
        int currentIndex = m_stackWidget->currentIndex();
        if (currentIndex < m_stackWidget->count() - 1)
        {
            int nextIndex = currentIndex + 1;
            if (nextIndex == m_stackWidget->count() - 1)
            {
                m_navUnlocked = true;
                if (m_listWidget_Nav)
                {
                    m_listWidget_Nav->setAttribute(Qt::WA_TransparentForMouseEvents, false);
                }
            }

            if (QWidget* fw = QApplication::focusWidget())
            {
                fw->clearFocus();
            }
            m_stackWidget->setCurrentIndex(nextIndex);
            if (m_stackWidget->currentWidget())
            {
                m_stackWidget->currentWidget()->setFocus();
            }
            updateFarthestReachedIndex(nextIndex);
            {
                QSignalBlocker blocker(m_listWidget_Nav);
                m_listWidget_Nav->setCurrentRow(nextIndex);
            }
            refreshNavigationIcons();
            updateBottomButtons(nextIndex);
            updateValidationState();
        }
        else
        {
            slot_onCreateMxCase();
        }
    }

    void mCaseImplicitModalWindow::slot_onPreviousStep()
    {
        int currentIndex = m_stackWidget->currentIndex();
        if (currentIndex > 0)
        {
            int preIndex = currentIndex - 1;
            if (QWidget* fw = QApplication::focusWidget())
            {
                fw->clearFocus();
            }
            m_stackWidget->setCurrentIndex(preIndex);
            if (m_stackWidget->currentWidget())
            {
                m_stackWidget->currentWidget()->setFocus();
            }
            {
                QSignalBlocker blocker(m_listWidget_Nav);
                m_listWidget_Nav->setCurrentRow(preIndex);
            }
            refreshNavigationIcons();
            updateBottomButtons(preIndex);

            // 回退到分析设置后，重新根据求解器类型刷新控件与输出项的关联状态
            if (preIndex == 0)
            {
                applySolverDependentState();
            }

        }
    }
}