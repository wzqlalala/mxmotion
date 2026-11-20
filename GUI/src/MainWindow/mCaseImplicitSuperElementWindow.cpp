#include "mCaseImplicitSuperElementWindow.h"
#include "mNVHWindowComponent.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
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
#include "mCaseImplicitGeneric.h"
#include "mBoundaryDisplacement.h"
#include "mBoundarySuperelement.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mMixSet.h"
#include "mMapRelationManager.h"
#include "mLoadBoundaryManager.h"
#include "mMeshData.h"
#include "mGlobalParam.h"
#include "mGenericParamData.h"
#include "mGenericMatrixData.h"

using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MechGlobalParamData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseImplicitSuperElementWindow::mCaseImplicitSuperElementWindow(QWidget* parent)
        : QDialog(parent)
    {
        // 初始化完成前不要让窗口内容提前显示到屏幕上，避免“先出白框再填控件”的闪烁感
        setAttribute(Qt::WA_DontShowOnScreen, true);

        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        this->setWindowTitle("超单元分析");
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
        applyAcousticCouplingDependentState();
        updateFarthestReachedIndex(0);
        updateValidationState();

        // 允许窗口在屏幕上显示后再调用 show()/raise()
        captureDefaultState();
        setAttribute(Qt::WA_DontShowOnScreen, false);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
        this->show();
        this->raise();
    }

    mCaseImplicitSuperElementWindow::~mCaseImplicitSuperElementWindow()
    {
    }

    bool mCaseImplicitSuperElementWindow::eventFilter(QObject* watched, QEvent* event)
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

    bool mCaseImplicitSuperElementWindow::isNavigationIndexEnabled(int index) const
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

    void mCaseImplicitSuperElementWindow::updateFarthestReachedIndex(int index)
    {
        if (index > m_farthestReachedIndex)
        {
            m_farthestReachedIndex = index;
        }
    }

    void mCaseImplicitSuperElementWindow::resetSubsequentSteps(int fromIndex)
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

    void mCaseImplicitSuperElementWindow::onPageModified(int pageIndex)
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

    QString mCaseImplicitSuperElementWindow::getCaseName() const
    {
        if (m_lineEdit_CaseName)
        {
            return m_lineEdit_CaseName->text();
        }
        return QString();
    }

    void mCaseImplicitSuperElementWindow::initUI()
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
        m_stackWidget->addWidget(createPageParametersSetting());
        m_stackWidget->addWidget(createPageDampingSetting());
        m_stackWidget->addWidget(createPageBoundaryPointSetting());
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

    void mCaseImplicitSuperElementWindow::initLeftNavigation()
    {
        m_listWidget_Nav = new QListWidget(this);
        QFont font;
        font.setPointSize(10);
        m_listWidget_Nav->setFont(font);
        m_listWidget_Nav->setContentsMargins(0, 0, 0, 0);
        m_listWidget_Nav->setFixedWidth(200);
        // 5 个导航项，每项 48 像素，确保输出设置项可见
        m_listWidget_Nav->setMinimumHeight(5 * 48);
        m_listWidget_Nav->setFocusPolicy(Qt::NoFocus);
        m_listWidget_Nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setSpacing(0);
        m_listWidget_Nav->setFrameShape(QFrame::NoFrame);
        m_listWidget_Nav->setViewMode(QListView::ListMode);
        m_listWidget_Nav->setUniformItemSizes(true);
        m_listWidget_Nav->setItemDelegate(m_navIconDelegate = new NavIconDelegate(this));
        m_navIconDelegate->setOutputPageIndex(4);
        m_listWidget_Nav->setSelectionMode(QAbstractItemView::SingleSelection);

        QListWidgetItem* itemAnalysis = new QListWidgetItem("超单元参数设置", m_listWidget_Nav);
        QListWidgetItem* itemDamping = new QListWidgetItem("阻尼设置", m_listWidget_Nav);
        QListWidgetItem* itemBoundary = new QListWidgetItem("边界点设置", m_listWidget_Nav);
        QListWidgetItem* itemConstraint = new QListWidgetItem("约束设置", m_listWidget_Nav);
        QListWidgetItem* itemOutput = new QListWidgetItem("输出设置", m_listWidget_Nav);
        itemAnalysis->setData(Qt::UserRole, 0);
        itemDamping->setData(Qt::UserRole, 1);
        itemBoundary->setData(Qt::UserRole, 2);
        itemConstraint->setData(Qt::UserRole, 3);
        itemOutput->setData(Qt::UserRole, 4);
        itemAnalysis->setSizeHint(QSize(120, 48));
        itemDamping->setSizeHint(QSize(120, 48));
        itemBoundary->setSizeHint(QSize(120, 48));
        itemConstraint->setSizeHint(QSize(120, 48));
        itemOutput->setSizeHint(QSize(120, 48));

        // 自定义导航栏样式：无边框、无背景、文字颜色
        m_listWidget_Nav->setStyleSheet(
            "QListWidget::item { background-color: transparent; color: #606266; "
            "  border: none; padding: 0px; padding-left: 16px; "
            "  min-height: 48px; max-height: 48px; }"
            "QListWidget { background-color: #ffffff; border: none; outline: none; }"
            "QListWidget::item:selected { background-color: transparent; color: #1385F6; }"
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

    void mCaseImplicitSuperElementWindow::refreshNavigationIcons()
    {
        if (!m_listWidget_Nav || !m_navIconDelegate || !m_stackWidget)
        {
            return;
        }
        m_navIconDelegate->setCurrentPage(m_stackWidget->currentIndex());
        m_listWidget_Nav->viewport()->update();
    }

    QWidget* mCaseImplicitSuperElementWindow::createPageParametersSetting()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(16);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // ---- 第一层：工况名称 + CMS方法 ----
        QFormLayout* topFormLayout = new QFormLayout();
        topFormLayout->setSpacing(10);
        topFormLayout->setLabelAlignment(Qt::AlignLeft);
        topFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelCaseName = new QLabel("工况名称", this);
        labelCaseName->setFixedWidth(87);
        m_lineEdit_CaseName = new QLineEdit(this);
        m_lineEdit_CaseName->setFixedWidth(251);
        topFormLayout->addRow(labelCaseName, m_lineEdit_CaseName);

        QLabel* labelCMSMethod = new QLabel("CMS方法", this);
        labelCMSMethod->setFixedWidth(87);
        m_comboBox_CMSMethod = new QComboBox(this);
        m_comboBox_CMSMethod->addItems({ "CBN", "GM", "GUYAN" });
        m_comboBox_CMSMethod->setFixedWidth(251);
        m_comboBox_CMSMethod->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        topFormLayout->addRow(labelCMSMethod, m_comboBox_CMSMethod);

        pageLayout->addLayout(topFormLayout);

        // 分隔线 1
        QFrame* line1 = new QFrame(this);
        line1->setFrameShape(QFrame::HLine);
        line1->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line1);

        // ---- 第二层：特征值求解方法 + 放大系数（保持现有逻辑） ----
        QFormLayout* middleFormLayout = new QFormLayout();
        middleFormLayout->setSpacing(10);
        middleFormLayout->setLabelAlignment(Qt::AlignLeft);
        middleFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelSolver = new QLabel("特征值求解方法", this);
        labelSolver->setFixedWidth(87);
        m_comboBox_Solver = new QComboBox(this);
        m_comboBox_Solver->addItems({ "自动多重子结构", "兰索斯" });
        // 默认使用兰索斯，并禁用暂不支持的“自动多重子结构”选项
        m_comboBox_Solver->setCurrentIndex(1);
        m_comboBox_Solver->setItemData(0, QVariant(0), Qt::UserRole - 1);
        m_comboBox_Solver->setFixedWidth(251);
        m_comboBox_Solver->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        connect(m_comboBox_Solver, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &mCaseImplicitSuperElementWindow::slot_onSolverChanged);
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

        // ---- 第三层：模态提取范围 + 声固耦合超单元分析 ----
        QWidget* thirdLayerWidget = new QWidget(this);
        QHBoxLayout* thirdLayerLayout = new QHBoxLayout(thirdLayerWidget);
        thirdLayerLayout->setSpacing(20);
        thirdLayerLayout->setContentsMargins(0, 0, 0, 0);
        thirdLayerLayout->setAlignment(Qt::AlignTop);

        // 左侧：模态提取范围
        QFormLayout* leftFormLayout = new QFormLayout();
        leftFormLayout->setContentsMargins(0, 35, 0, 0);
        leftFormLayout->setSpacing(10);
        leftFormLayout->setLabelAlignment(Qt::AlignLeft);
        leftFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
        QLabel* labelFreqMax = new QLabel("频率上限（Hz）", this);
        labelFreqMax->setFixedWidth(87);
        m_lineEdit_FreqMax = new QLineEdit(this);
        m_lineEdit_FreqMax->setFixedWidth(251);
        leftFormLayout->addRow(labelFreqMax, m_lineEdit_FreqMax);

        QLabel* labelModeCount = new QLabel("模态阶数上限", this);
        labelModeCount->setFixedWidth(87);
        m_lineEdit_ModeCount = new QLineEdit(this);
        m_lineEdit_ModeCount->setFixedWidth(251);
        leftFormLayout->addRow(labelModeCount, m_lineEdit_ModeCount);

        QLabel* labelNodeStartID = new QLabel("节点起始编号", this);
        labelNodeStartID->setFixedWidth(87);
        m_lineEdit_NodeStartID = new QLineEdit(this);
        m_lineEdit_NodeStartID->setFixedWidth(251);
        leftFormLayout->addRow(labelNodeStartID, m_lineEdit_NodeStartID);
        thirdLayerLayout->addLayout(leftFormLayout);

        // 右侧：声固耦合超单元分析
        QVBoxLayout* rightLayout = new QVBoxLayout();
        rightLayout->setSpacing(10);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setAlignment(Qt::AlignTop);

        m_AcousticSuperElement = new QCheckBox("声固耦合超单元分析：", this);
        m_AcousticSuperElement->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_AcousticSuperElement->setEnabled(false);
        rightLayout->addWidget(m_AcousticSuperElement);

        QWidget* m_SuperElementWidget = new QWidget(this);
        QFormLayout* rightFormLayout = new QFormLayout(m_SuperElementWidget);
        rightFormLayout->setContentsMargins(0, 0, 0, 0);
        rightFormLayout->setSpacing(10);
        rightFormLayout->setLabelAlignment(Qt::AlignLeft);
        rightFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelAcousticFreqMax = new QLabel("频率上限（Hz）", this);
        labelAcousticFreqMax->setFixedWidth(87);
        m_lineEdit_AcousticFreqMax = new QLineEdit(this);
        m_lineEdit_AcousticFreqMax->setFixedWidth(251);
        rightFormLayout->addRow(labelAcousticFreqMax, m_lineEdit_AcousticFreqMax);

        QLabel* labelAcousticModeCount = new QLabel("模态阶数上限", this);
        labelAcousticModeCount->setFixedWidth(87);
        m_lineEdit_AcousticModeCount = new QLineEdit(this);
        m_lineEdit_AcousticModeCount->setFixedWidth(251);
        rightFormLayout->addRow(labelAcousticModeCount, m_lineEdit_AcousticModeCount);

        QLabel* labelAcousticNodeStartID = new QLabel("节点起始编号", this);
        labelAcousticNodeStartID->setFixedWidth(87);
        m_lineEdit_AcousticNodeStartID = new QLineEdit(this);
        m_lineEdit_AcousticNodeStartID->setFixedWidth(251);
        rightFormLayout->addRow(labelAcousticNodeStartID, m_lineEdit_AcousticNodeStartID);

        rightLayout->addWidget(m_SuperElementWidget);
        m_SuperElementWidget->setEnabled(false);
        thirdLayerLayout->addLayout(rightLayout);

        pageLayout->addWidget(thirdLayerWidget);

        // 分隔线 3
        QFrame* line3 = new QFrame(this);
        line3->setFrameShape(QFrame::HLine);
        line3->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line3);

        // ---- 第四层：声固耦合 ----
        QVBoxLayout* bottomLayerLayout = new QVBoxLayout();
        bottomLayerLayout->setSpacing(10);
        bottomLayerLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayerLayout->setAlignment(Qt::AlignTop);

        m_AcousticStructureCoupling = new QCheckBox("声固耦合：", this);
        m_AcousticStructureCoupling->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_AcousticStructureCoupling->setEnabled(false);
        bottomLayerLayout->addWidget(m_AcousticStructureCoupling);

        QHBoxLayout* bottomColumnsLayout = new QHBoxLayout(page);
        bottomColumnsLayout->setSpacing(20);
        bottomColumnsLayout->setContentsMargins(0, 0, 0, 0);
        bottomColumnsLayout->setAlignment(Qt::AlignTop);

        QWidget* m_AcousticWidget = new QWidget(this);
        QFormLayout* normalFormLayout = new QFormLayout(m_AcousticWidget);
        normalFormLayout->setContentsMargins(0, 0, 0, 0);
        normalFormLayout->setSpacing(10);
        normalFormLayout->setLabelAlignment(Qt::AlignLeft);
        normalFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelNormalRatio = new QLabel("面单元外法线延伸比例（NORMAL）", this);
        labelNormalRatio->setFixedWidth(201);
        m_lineEdit_NormalRatio = new QLineEdit(this);
        m_lineEdit_NormalRatio->setText("3");
        m_lineEdit_NormalRatio->setFixedWidth(137);
        normalFormLayout->addRow(labelNormalRatio, m_lineEdit_NormalRatio);

        QLabel* labelSknEpsRatio = new QLabel("面单元横向延伸比例（SKNEPS）", this);
        labelSknEpsRatio->setFixedWidth(201);
        m_lineEdit_SknEpsRatio = new QLineEdit(this);
        m_lineEdit_SknEpsRatio->setText("6");
        m_lineEdit_SknEpsRatio->setFixedWidth(137);
        normalFormLayout->addRow(labelSknEpsRatio, m_lineEdit_SknEpsRatio);
        bottomColumnsLayout->addWidget(m_AcousticWidget);
        m_AcousticWidget->setEnabled(false);
        bottomColumnsLayout->addStretch(1);
        bottomLayerLayout->addLayout(bottomColumnsLayout);
        pageLayout->addLayout(bottomLayerLayout);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitSuperElementWindow::createPageDampingSetting()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(16);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // 第一行：启用结构阻尼（仅展示，暂不可编辑）
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setContentsMargins(3, 0, 0, 0);
        m_checkBox_EnableDamping = new QCheckBox("结构阻尼设置：", this);
        m_checkBox_EnableDamping->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_EnableDamping->setEnabled(false);
        topLayout->addWidget(m_checkBox_EnableDamping);
        topLayout->addStretch(1);
        pageLayout->addLayout(topLayout);

        // 第二行：全局结构阻尼（默认禁用，作为展示）
        QHBoxLayout* contentLayout = new QHBoxLayout();
        contentLayout->setContentsMargins(0, 0, 0, 0);

        m_radioButton_GlobalDamping = new QRadioButton("全局结构阻尼", this);
        m_radioButton_GlobalDamping->setEnabled(false);
        m_radioButton_GlobalDamping->setChecked(false);
        contentLayout->addWidget(m_radioButton_GlobalDamping);

        m_lineEdit_GlobalDamping = new QLineEdit(this);
        m_lineEdit_GlobalDamping->setFixedWidth(200);
        m_lineEdit_GlobalDamping->setEnabled(false);
        contentLayout->addWidget(m_lineEdit_GlobalDamping);
        contentLayout->addStretch(1);

        pageLayout->addLayout(contentLayout);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitSuperElementWindow::createPageBoundaryPointSetting()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        QHBoxLayout* typeLayout = new QHBoxLayout(page);
        QLabel* typeLabel = new QLabel("边界类型：", this);
        m_comboBox_BoundaryType = new QComboBox(this);
        m_comboBox_BoundaryType->addItems({ "固定", "自由" });
        m_comboBox_BoundaryType->setFixedWidth(290);
        typeLayout->addWidget(typeLabel);
        typeLayout->addWidget(m_comboBox_BoundaryType);
        typeLayout->addStretch(1);
        pageLayout->addLayout(typeLayout);
        // 第一行：节点个数 + 添加一行 + 删除
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setSpacing(10);
        topLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* countLabel = new QLabel("节点个数：", this);
        m_lineEdit_BoundaryPointCount = new QLineEdit(this);
        m_lineEdit_BoundaryPointCount->setFixedWidth(290);
        m_lineEdit_BoundaryPointCount->setText("1");
        m_pushButton_BoundaryAddRow = new QPushButton(this);
        m_pushButton_BoundaryAddRow->setMaximumWidth(30);
        m_pushButton_BoundaryRemoveRow = new QPushButton(this);
        m_pushButton_BoundaryRemoveRow->setMaximumWidth(30);
        m_pushButton_BoundaryAddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_pushButton_BoundaryAddRow->setIconSize(QSize(14, 14));
        m_pushButton_BoundaryRemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_pushButton_BoundaryRemoveRow->setIconSize(QSize(14, 14));
        m_pushButton_BoundaryRemoveRow->setEnabled(false);
        m_pushButton_BoundaryAddRow->setAutoDefault(false);
        m_pushButton_BoundaryRemoveRow->setAutoDefault(false);
        topLayout->addWidget(countLabel);
        topLayout->addWidget(m_lineEdit_BoundaryPointCount);
        topLayout->addWidget(m_pushButton_BoundaryAddRow);
        topLayout->addWidget(m_pushButton_BoundaryRemoveRow);
        topLayout->addStretch();
        m_pushButton_BoundaryImportFile = new QPushButton("导入文件", this);
        m_pushButton_BoundaryExportFile = new QPushButton("导出文件", this);
        topLayout->addWidget(m_pushButton_BoundaryImportFile);
        topLayout->addWidget(m_pushButton_BoundaryExportFile);

        pageLayout->addLayout(topLayout);
        m_boundaryNodeTable = new IncentiveNodeTableWidget(this);
        pageLayout->addWidget(m_boundaryNodeTable, 1);

        // 初始根据 CMS 方法设置边界类型可选项
        updateBoundaryTypeByCMSMethod();

        return page;
    }

    QWidget* mCaseImplicitSuperElementWindow::createPageConstraint()
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

    QWidget* mCaseImplicitSuperElementWindow::createPageOutput()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        QLabel* para = new QLabel("参数:");
        pageLayout->addWidget(para);
        QHBoxLayout* hLayout = new QHBoxLayout(page);
        m_checkBox_MatrixName = new QCheckBox("矩阵名称",page);
        m_lineEdit_MatrixName = new QLineEdit(page);
        m_lineEdit_MatrixName->setMinimumWidth(200);
        m_lineEdit_MatrixName->setEnabled(false);
        hLayout->addWidget(m_checkBox_MatrixName);
        hLayout->addWidget(m_lineEdit_MatrixName);
        hLayout->addStretch(1);
        pageLayout->addLayout(hLayout);
        pageLayout->addSpacing(10);

        QLabel* punch = new QLabel("输出PUNCH", this);
        pageLayout->addWidget(punch);
        m_checkBox_MxSim = new QCheckBox("MxSim", page);
        m_checkBox_Optistruct = new QCheckBox("Optistruct", page);
        m_checkBox_Nastran = new QCheckBox("Nastran", page);
        pageLayout->addWidget(m_checkBox_MxSim);
        pageLayout->addWidget(m_checkBox_Optistruct);
        pageLayout->addWidget(m_checkBox_Nastran);

        return page;
    }

    void mCaseImplicitSuperElementWindow::updateBottomButtons(int currentIndex)
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
    void mCaseImplicitSuperElementWindow::captureDefaultState()
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
        const QList<QRadioButton*> radios = findChildren<QRadioButton*>();
        for (QRadioButton* w : radios)
        {
            if (!insideTable(w))
                m_defaultRadioChecks[w] = w->isChecked();
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
    void mCaseImplicitSuperElementWindow::resetToDefault()
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
        for (auto it = m_defaultRadioChecks.begin(); it != m_defaultRadioChecks.end(); ++it)
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

    void mCaseImplicitSuperElementWindow::closeEvent(QCloseEvent* event)
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

    void mCaseImplicitSuperElementWindow::slot_onClose()
    {
        this->close();
    }

    bool mCaseImplicitSuperElementWindow::isValidDouble(QLineEdit* lineEdit) const
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

    bool mCaseImplicitSuperElementWindow::isValidPositiveInt(QLineEdit* lineEdit) const
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

    void mCaseImplicitSuperElementWindow::applySolverDependentState()
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

        // 节点起始编号
        if (m_lineEdit_NodeStartID)
        {
            m_lineEdit_NodeStartID->setEnabled(true);
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

    void mCaseImplicitSuperElementWindow::slot_onSolverChanged()
    {
        applySolverDependentState();
    }

    // 输入框输入限制：数值框仅允许合法数值形态，整数框仅允许数字，非法输入直接置空
    void mCaseImplicitSuperElementWindow::installInputValidators()
    {
        QList<QLineEdit*> textEdits = { m_lineEdit_MatrixName };
        QList<QLineEdit*> intEdits = { m_lineEdit_ModeCount, m_lineEdit_NodeStartID, m_lineEdit_AcousticModeCount, m_lineEdit_AcousticNodeStartID,
            m_lineEdit_BoundaryPointCount, m_lineEdit_PointCount };

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

    void mCaseImplicitSuperElementWindow::initConnect()
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

        // 声固耦合超单元分析：勾选时启用/禁用下方三个输入框
        connect(m_AcousticSuperElement, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_lineEdit_AcousticFreqMax)
            {
                m_lineEdit_AcousticFreqMax->setEnabled(checked);
            }
            if (m_lineEdit_AcousticModeCount)
            {
                m_lineEdit_AcousticModeCount->setEnabled(checked);
            }
            if (m_lineEdit_AcousticNodeStartID)
            {
                m_lineEdit_AcousticNodeStartID->setEnabled(checked);
            }
            onPageModified(0);
            });

        // 声固耦合：勾选时启用/禁用下方两个比例输入框
        connect(m_AcousticStructureCoupling, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_lineEdit_NormalRatio)
            {
                m_lineEdit_NormalRatio->setEnabled(checked);
            }
            if (m_lineEdit_SknEpsRatio)
            {
                m_lineEdit_SknEpsRatio->setEnabled(checked);
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
        connect(m_pushButton_Next, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onNextStep);
        connect(m_pushButton_ExportFem, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onExportFem);
        connect(m_pushButton_BoundaryAddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddBoundaryPointRow();
            onPageModified(2);
            });
        connect(m_lineEdit_BoundaryPointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onBoundaryPointCountChanged();
            onPageModified(2);
            });
        connect(m_pushButton_BoundaryRemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveBoundaryPointRow();
            onPageModified(2);
            });
        connect(m_pushButton_BoundaryImportFile, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onImportBoundaryPointFile);
        connect(m_pushButton_BoundaryExportFile, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onExportBoundaryPointFile);
        connect(m_boundaryNodeTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                updateBoundaryPointDeleteButtonState();
            });
        connect(m_boundaryNodeTable, &IncentiveNodeTableWidget::itemChanged, this, [this]() {
            onPageModified(2);
            });
        connect(m_pushButton_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddConstraintRow();
            onPageModified(3);
            });
        connect(m_lineEdit_PointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onPointCountChanged();
            onPageModified(3);
            });
        connect(m_pushButton_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveConstraintRow();
            onPageModified(3);
            });
        connect(m_pushButton_ImportFile, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onImportConstraintFile);
        connect(m_pushButton_ExportFile, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onExportConstraintFile);
        connect(m_pushButton_Previous, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onPreviousStep);
        connect(m_pushButton_CreateMxCase, &QPushButton::clicked, this, &mCaseImplicitSuperElementWindow::slot_onCreateMxCase);
        connect(m_nodetable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                updateDeleteButtonState();
            });
        connect(m_nodetable, &IncentiveNodeTableWidget::itemChanged, this, [this]() {
            onPageModified(3);
            });

        // 分析设置输入变化时刷新完成状态并自动勾选输出
        connect(m_lineEdit_CaseName, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_comboBox_CMSMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            onPageModified(0);
            updateBoundaryTypeByCMSMethod();
            });
        connect(m_comboBox_Solver, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            onPageModified(0);
            });
        connect(m_checkBox_ModeScale, &QCheckBox::toggled, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_ModeScale, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_FreqMax, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_ModeCount, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_NodeStartID, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_AcousticFreqMax, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_AcousticModeCount, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_AcousticNodeStartID, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_NormalRatio, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_SknEpsRatio, &QLineEdit::textChanged, this, [this]() {
            onPageModified(0);
            });

        // 输出设置复选框变化时刷新按钮状态
        connect(m_checkBox_MxSim, &QCheckBox::toggled, this, [this]() {
            onPageModified(4);
            updateOutputActionButtons();
            });
        connect(m_checkBox_Optistruct, &QCheckBox::toggled, this, [this]() {
            onPageModified(4);
            updateOutputActionButtons();
            });
        connect(m_checkBox_Nastran, &QCheckBox::toggled, this, [this]() {
            onPageModified(4);
            updateOutputActionButtons();
            });
        connect(m_checkBox_MatrixName, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_lineEdit_MatrixName)
            {
                m_lineEdit_MatrixName->setEnabled(checked);
            }
            onPageModified(4);
            });
    }

    void mCaseImplicitSuperElementWindow::updateDeleteButtonState()
    {
        if (!m_pushButton_RemoveRow || !m_nodetable)
        {
            return;
        }
        m_pushButton_RemoveRow->setEnabled(!m_nodetable->selectionModel()->selectedRows().isEmpty());
    }

    void mCaseImplicitSuperElementWindow::updateBoundaryTypeByCMSMethod()
    {
        if (!m_comboBox_BoundaryType || !m_comboBox_CMSMethod)
        {
            return;
        }
        QString cmsMethod = m_comboBox_CMSMethod->currentText();
        QString currentText = m_comboBox_BoundaryType->currentText();
        m_comboBox_BoundaryType->clear();
        if (cmsMethod == "GM")
        {
            m_comboBox_BoundaryType->addItems({ "固定", "自由" });
        }
        else
        {
            m_comboBox_BoundaryType->addItem("固定");
        }
        int index = m_comboBox_BoundaryType->findText(currentText);
        if (index >= 0)
        {
            m_comboBox_BoundaryType->setCurrentIndex(index);
        }
        else
        {
            m_comboBox_BoundaryType->setCurrentIndex(0);
        }
    }

    bool mCaseImplicitSuperElementWindow::isDampingValid() const
    {
        // 阻尼设置功能未实现，暂直接返回有效
        return true;
    }

    bool mCaseImplicitSuperElementWindow::isAnalysisSettingValid() const
    {
        // 工况名称必须有值
        if (!m_lineEdit_CaseName || m_lineEdit_CaseName->text().trimmed().isEmpty())
        {
            return false;
        }

        // 节点起始编号必须有值
        if (!isValidPositiveInt(m_lineEdit_NodeStartID))
        {
            return false;
        }

        // 频率上限和模态阶数上限其一有值即可
        bool hasValidModeCount = isValidPositiveInt(m_lineEdit_ModeCount);
        bool hasValidFreqMax = isValidDouble(m_lineEdit_FreqMax);
        return hasValidModeCount || hasValidFreqMax;
    }

    void mCaseImplicitSuperElementWindow::applyAcousticCouplingDependentState()
    {
        const bool superEnabled = m_AcousticSuperElement &&
            m_AcousticSuperElement->isEnabled() &&
            m_AcousticSuperElement->isChecked();
        if (m_lineEdit_AcousticFreqMax)
        {
            m_lineEdit_AcousticFreqMax->setEnabled(superEnabled);
        }
        if (m_lineEdit_AcousticModeCount)
        {
            m_lineEdit_AcousticModeCount->setEnabled(superEnabled);
        }
        if (m_lineEdit_AcousticNodeStartID)
        {
            m_lineEdit_AcousticNodeStartID->setEnabled(superEnabled);
        }

        const bool acousticEnabled = m_AcousticStructureCoupling &&
            m_AcousticStructureCoupling->isEnabled() &&
            m_AcousticStructureCoupling->isChecked();
        if (m_lineEdit_NormalRatio)
        {
            m_lineEdit_NormalRatio->setEnabled(acousticEnabled);
        }
        if (m_lineEdit_SknEpsRatio)
        {
            m_lineEdit_SknEpsRatio->setEnabled(acousticEnabled);
        }
    }

    bool mCaseImplicitSuperElementWindow::isBoundaryPointValid() const
    {
        if (!m_boundaryNodeTable)
        {
            return false;
        }
        for (int i = 0; i < m_boundaryNodeTable->rowCount(); ++i)
        {
            auto row = m_boundaryNodeTable->getRowData(i);
            QString gridText = row.nodeId.trimmed();
            if (gridText.isEmpty())
            {
                continue;
            }
            bool ok = false;
            int id = gridText.toInt(&ok);
            if (ok && id > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool mCaseImplicitSuperElementWindow::isConstraintValid() const
    {
        // 约束设置允许为空，始终视为有效
        return true;
    }

    bool mCaseImplicitSuperElementWindow::isOutputValid() const
    {
        int checkedCount = 0;
        if (m_checkBox_MxSim && m_checkBox_MxSim->isChecked()) ++checkedCount;
        if (m_checkBox_Optistruct && m_checkBox_Optistruct->isChecked()) ++checkedCount;
        if (m_checkBox_Nastran && m_checkBox_Nastran->isChecked()) ++checkedCount;
        return checkedCount >= 1;
    }

    void mCaseImplicitSuperElementWindow::updateValidationState()
    {
        if (!m_navIconDelegate || !m_stackWidget)
        {
            return;
        }

        // 分析设置：有效时打勾
        m_navIconDelegate->setPageCompleted(0, m_farthestReachedIndex > 0 && isAnalysisSettingValid());

        // 阻尼设置
        m_navIconDelegate->setPageCompleted(1, m_farthestReachedIndex > 1 && isDampingValid());

        // 边界点设置：至少有一行有效数据才打勾
        m_navIconDelegate->setPageCompleted(2, m_farthestReachedIndex > 2 && isBoundaryPointValid());

        // 约束设置：通过“下一步”离开该页后打勾
        m_navIconDelegate->setPageCompleted(3, m_farthestReachedIndex > 3 && isConstraintValid());

        // 输出设置：不用打勾图标，所以永远不打勾
        m_navIconDelegate->setPageCompleted(4, false);

        // 未解锁时仅当前进度页之前的步骤可点击；解锁后允许已到达过的步骤
        for (int i = 0; i < m_stackWidget->count(); ++i)
        {
            m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
        }

        refreshNavigationIcons();
        updateOutputActionButtons();
    }

    void mCaseImplicitSuperElementWindow::updateOutputActionButtons()
    {
        bool analysisCompleted = m_navIconDelegate && m_navIconDelegate->isPageCompleted(0);
        bool boundaryPointCompleted = m_navIconDelegate && m_navIconDelegate->isPageCompleted(2);
        bool outputValid = isOutputValid();
        bool enabled = analysisCompleted && boundaryPointCompleted && outputValid;
        if (m_pushButton_ExportFem)
        {
            m_pushButton_ExportFem->setEnabled(enabled);
        }
        if (m_pushButton_CreateMxCase)
        {
            m_pushButton_CreateMxCase->setEnabled(enabled);
        }
    }

    void mCaseImplicitSuperElementWindow::slot_onCreateMxCase()
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
            for (int i = 0; i < m_boundaryNodeTable->rowCount(); ++i)
            {
                auto row = m_boundaryNodeTable->getRowData(i);
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

        // 1. 创建全局设置中的超单元参数（GenericParam）
        auto* globalParam = modelData->getGlobalParam();
        int paramID = globalParam->createGenericParamID();
        mGenericParamData* paramData = new mGenericParamData(paramID, caseName);
        paramData->_method = m_comboBox_CMSMethod->currentText();
        paramData->_upFrequency = m_lineEdit_FreqMax->text() ;
        paramData->_order = m_lineEdit_ModeCount->text();
        paramData->_startID = m_lineEdit_NodeStartID->text();
        globalParam->addGenericParamData(paramData);

        // 2. 创建超单元分析工况（Generic/ImplicitGeneric），并关联超单元参数
        int caseID = modelData->getManager<mCaseManager>(Physical_Case).createID();
        mCaseImplicitGeneric* caseData = new mCaseImplicitGeneric(caseID, ImplicitGeneric);
        caseData->setCaseName(caseName);
        modelData->addPhysicalField<mCaseBase>(caseData);

        modelData->getMapRelationManager().appendMapRelation(
            { Physical_Case, caseID },
            { Physical_GenericParam, paramID });

        // 3. 根据边界点表格创建超单元边界（mBoundarySuperelement）
        QString boundaryTypeText = m_comboBox_BoundaryType ? m_comboBox_BoundaryType->currentText() : QString("固定");
        QString constraintType = (boundaryTypeText == "自由") ? "BNDFREE1" : "BNDFIX1";

        auto& setManager = modelData->getManager<MechSetData::mSetManager>(Physical_Set);
        auto& boundaryManager = modelData->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
        auto& mapRelation = modelData->getMapRelationManager();

        for (int i = 0; i < m_boundaryNodeTable->rowCount(); ++i)
        {
            auto row = m_boundaryNodeTable->getRowData(i);
            QString gridText = row.nodeId;
            if (gridText.isEmpty())
            {
                continue;
            }
            bool ok = false;
            int nodeId = gridText.trimmed().toInt(&ok);
            if (!ok || nodeId <= 0)
            {
                continue;
            }

            int boundaryID = boundaryManager.createID();
            std::shared_ptr<mBoundarySuperelement> boundary = std::make_shared<mBoundarySuperelement>(
                boundaryID, BOUNDARY_IM_SUPERELEMENT,
                QString("%1_%2").arg(caseName).arg(i + 1));
            boundary->_ConstraintType = constraintType;
            boundary->_DOF[0] = row.x;
            boundary->_DOF[1] = row.y;
            boundary->_DOF[2] = row.z;
            boundary->_DOF[3] = row.rx;
            boundary->_DOF[4] = row.ry;
            boundary->_DOF[5] = row.rz;

            // 创建内部集合（SetType::Node），只包含当前节点
            int setID = setManager.createID();
            mMixSet* set = new mMixSet(setID, SetType::Mix, "contain", true);
            std::vector<RenderPickResultData> nodeData;
            RenderPickResultData pick;
            pick.result_id = nodeId;
            nodeData.push_back(pick);
            set->setMixSetData({ { PickDataType::Node, nodeData } });
            modelData->addPhysicalField<MechSetData::mSetBase>(set);

            modelData->addPhysicalFieldP(boundary, caseID);

            mapRelation.appendMapRelation(
                { Physical_Case, caseID },
                { Physical_LoadBoundary, boundaryID });
            mapRelation.appendMapRelation(
                { Physical_LoadBoundary, boundaryID },
                { Physical_Set, setID });
        }

        // 4. 创建位移/转角边界（mBoundaryDisplacement），复用 mCaseImplicitModalWindow 的逻辑
        QString baseName = "NVH_DispRot";
        std::map<RenderPickResultData, QVector<QVariant>> independentTable;
        if (!meshData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("模型网格数据不存在，无法创建约束"));
        }
        else
        {
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
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                        QString("约束设置中节点ID%1格式无效。").arg(gridText));
                    continue;
                }
                QVector<QVariant> values(6);
                if (row.x) values[1] = 0.0;
                if (row.y) values[2] = 0.0;
                if (row.z) values[3] = 0.0;
                if (row.rx) values[4] = 0.0;
                if (row.ry) values[5] = 0.0;
                if (row.rz) values[6] = 0.0;
                independentTable[RenderPickResultData(id, -1, 0)] = std::move(values);
            }
            if (!independentTable.empty())
            {
                int boundaryID = boundaryManager.createID();
                std::shared_ptr<mBoundaryDisplacement> boundary = std::make_shared<mBoundaryDisplacement>(
                    boundaryID, BOUNDARY_IM_DISPLACEMENT, baseName);
                boundary->_definitionMode = 1;
                boundary->_independentTable = independentTable;
                modelData->addPhysicalFieldP(boundary, caseID);
                mapRelation.appendMapRelation(
                    { Physical_Case, caseID },
                    { Physical_LoadBoundary, boundaryID });
            }
        }

        // 5. 如果输出页的矩阵名称有值，则创建超单元矩阵（GenericMatrix）
        if (m_checkBox_MatrixName && m_checkBox_MatrixName->isChecked())
        {
            QString matrixName = m_lineEdit_MatrixName ? m_lineEdit_MatrixName->text().trimmed() : QString();
            if (!matrixName.isEmpty() && !globalParam->getGenericMatrixData())
            {
                mGenericMatrixData* matrix = globalParam->createGenericMatrixData();
                if (matrix)
                {
                    matrix->_matrixName = matrixName;
                }
            }
        }

        // 6. 触发模型树更新
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, caseID);
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("创建 Mx 工况：%1").arg(caseName));
        mGlobalSignals::getInstance()->updateModelTree();
        this->accept();

        // 工况已创建，关闭时不再提示
        m_mxCaseCreated = true;
    }

    void mCaseImplicitSuperElementWindow::slot_onExportFem()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出 fem 文件",
            QString(),
            "FEM 文件 (*.fem);;所有文件 (*.*)");
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

        const int cmsID = 1;
        const int boundaryID = 2;
        const int spcID = 3;

        // 超单元参数
        QString freqMax = m_lineEdit_FreqMax->text().trimmed();
        QString modeCount = m_lineEdit_ModeCount->text().trimmed();

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

        // 1. 输出设置中的矩阵名称
        if (m_checkBox_MatrixName && m_checkBox_MatrixName->isChecked())
        {
            QString matrixName = m_lineEdit_MatrixName->text();
            if (!matrixName.isEmpty())
            {
                out << "DMIGNAME = " << matrixName << "\n";
            }
        }

        // 2. 工况控制：CMS 方法与约束集
        out << "$ CASE CONTROL COMMANDS\n";
        out << "  CMSMETH = " << QString::number(cmsID).rightJustified(8) << "\n";
        out << "  SPC = " << QString::number(spcID).rightJustified(8) << "\n";

        out << "$ Bulk Data Cards\n";
        out << "BEGIN BULK\n";

        // 3. 声固耦合（ACMODL）：面单元外法线/横向延伸比例
        if (m_AcousticStructureCoupling && m_AcousticStructureCoupling->isChecked())
        {
            QString normalRatio = m_lineEdit_NormalRatio ? m_lineEdit_NormalRatio->text().trimmed() : QString();
            QString sknEpsRatio = m_lineEdit_SknEpsRatio ? m_lineEdit_SknEpsRatio->text().trimmed() : QString();
            if (!normalRatio.isEmpty() && !sknEpsRatio.isEmpty())
            {
                out << "$ Acoustic-structure coupling (ACMODL): NORMAL=" << normalRatio
                    << ", SKNEPS=" << sknEpsRatio << "\n";
                writeBulkCard({ "ACMODL", "", "", "", "", normalRatio, "", sknEpsRatio });
            }
        }

        out << "PARAM,AUTOMSET,YES\n";
        out << "PARAM,EXTOUT,DMIGPCH\n";

        // 4. 边界点：BNDFIX1 / BNDFREE1，按自由度代码分组，每组一张卡（首行 7 个节点、续行 8 个）
        QString boundaryTypeText = m_comboBox_BoundaryType ? m_comboBox_BoundaryType->currentText() : QString("固定");
        QString boundaryCard = (boundaryTypeText == "自由") ? "BNDFREE1" : "BNDFIX1";

        QMap<QString, QList<int>> boundaryGroups;
        if (m_boundaryNodeTable)
        {
            for (int i = 0; i < m_boundaryNodeTable->rowCount(); ++i)
            {
                auto row = m_boundaryNodeTable->getRowData(i);
                QString gridText = row.nodeId.trimmed();
                if (gridText.isEmpty())
                {
                    continue;
                }

                bool ok = false;
                int nodeId = gridText.toInt(&ok);
                if (!ok || nodeId <= 0)
                {
                    continue;
                }

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

                boundaryGroups[dofCode].append(nodeId);
            }
        }

        if (!boundaryGroups.isEmpty())
        {
            out << "$ Boundary Points (interface nodes)\n";
            out << "$HMNAME LOADCOL" << QString::number(boundaryID).rightJustified(17)
                << "\"FRACMS_" << (boundaryCard == "BNDFREE1" ? "BNDFREE" : "BNDFIX") << "\"\n";
        }
        for (auto it = boundaryGroups.constBegin(); it != boundaryGroups.constEnd(); ++it)
        {
            QStringList fields;
            fields << boundaryCard << it.key();
            for (int nodeId : it.value())
            {
                fields << QString::number(nodeId);
                if (fields.size() == 9)
                {
                    writeBulkCard(fields);
                    fields.clear();
                    fields << "+";
                }
            }
            if (fields.size() > 1)
            {
                writeBulkCard(fields);
            }
        }

        // 5. CMSMETH 参数卡：主行为结构模态提取，续行为声固耦合超单元（勾选时输出）
        out << "$ Superelement CMS Method\n";
        out << "$HMNAME LOADCOL" << QString::number(cmsID).rightJustified(17)
            << "\"FRACMS_CmsMeth\"\n";
        QString methodText = m_comboBox_CMSMethod ? m_comboBox_CMSMethod->currentText() : QString("CBN");
        QString solverText = (m_comboBox_Solver && m_comboBox_Solver->currentText() == "兰索斯")
            ? QString("LANCZOS") : QString("AMSES");
        QString ampFactor = (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked() && m_lineEdit_ModeScale)
            ? m_lineEdit_ModeScale->text().trimmed() : QString();
        writeBulkCard({ "CMSMETH", QString::number(cmsID), methodText,
            freqMax, "", modeCount, solverText, ampFactor });
        if (m_AcousticSuperElement && m_AcousticSuperElement->isChecked())
        {
            QString acousticFreqMax = m_lineEdit_AcousticFreqMax ? m_lineEdit_AcousticFreqMax->text().trimmed() : QString();
            QString acousticModeCount = m_lineEdit_AcousticModeCount ? m_lineEdit_AcousticModeCount->text().trimmed() : QString();
            out << "$ Acoustic superelement: freq limit=" << acousticFreqMax
                << ", mode count=" << acousticModeCount << "\n";
            writeBulkCard({ "+", "", "", acousticFreqMax, "", acousticModeCount });
        }

        // 6. 约束 SPC
        bool hasSpc = false;
        if (m_nodetable)
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
                int nodeId = gridText.toInt(&ok);
                if (!ok || nodeId <= 0)
                {
                    continue;
                }

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

                if (!hasSpc)
                {
                    out << "$ SPC Data (constraints)\n";
                    hasSpc = true;
                }
                writeBulkCard({ "SPC", QString::number(spcID),
                    QString::number(nodeId), dofCode, "0.0" });
            }
        }

        out << "ENDDATA\n";
        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出 fem 文件：%1").arg(fileName));
    }

    void mCaseImplicitSuperElementWindow::slot_onAddBoundaryPointRow()
    {
        m_boundaryNodeTable->addRow();
        m_lineEdit_BoundaryPointCount->setText(QString::number(m_boundaryNodeTable->rowCount()));
    }

    void mCaseImplicitSuperElementWindow::slot_onRemoveBoundaryPointRow()
    {
        m_boundaryNodeTable->removeSelectedRows();
        m_lineEdit_BoundaryPointCount->setText(QString::number(m_boundaryNodeTable->rowCount()));
    }

    void mCaseImplicitSuperElementWindow::slot_onBoundaryPointCountChanged()
    {
        m_boundaryNodeTable->setRowCount(m_lineEdit_BoundaryPointCount->text().toInt());
    }

    void mCaseImplicitSuperElementWindow::updateBoundaryPointDeleteButtonState()
    {
        if (!m_pushButton_BoundaryRemoveRow || !m_boundaryNodeTable)
        {
            return;
        }
        m_pushButton_BoundaryRemoveRow->setEnabled(!m_boundaryNodeTable->selectionModel()->selectedRows().isEmpty());
    }

    void mCaseImplicitSuperElementWindow::slot_onImportBoundaryPointFile()
    {
        if (!m_boundaryNodeTable)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入边界点文件",
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

            QStringList parts = line.split(',');
            if (parts.size() != 7)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 7。").arg(lineIndex));
                file.close();
                return;
            }

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
            row.nodeId = QString::number(nodeId);
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

        m_boundaryNodeTable->setTableData(importedData);
        m_lineEdit_BoundaryPointCount->setText(QString::number(m_boundaryNodeTable->rowCount()));
        onPageModified(2);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条边界点数据。").arg(importedData.size()));
    }

    void mCaseImplicitSuperElementWindow::slot_onExportBoundaryPointFile()
    {
        if (!m_boundaryNodeTable)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出边界点文件",
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

        out << "NodeID, X, Y, Z, RX, RY, RZ\n";

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> data = m_boundaryNodeTable->getTableData();
        for (const IncentiveNodeTableWidget::IncentiveNodeRowData& row : data)
        {
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

            out << QString::number(id) << ", "
                << (row.x ? "1" : "0") << ", "
                << (row.y ? "1" : "0") << ", "
                << (row.z ? "1" : "0") << ", "
                << (row.rx ? "1" : "0") << ", "
                << (row.ry ? "1" : "0") << ", "
                << (row.rz ? "1" : "0") << "\n";
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出边界点文件：%1").arg(fileName));
    }

    void mCaseImplicitSuperElementWindow::slot_onAddConstraintRow()
    {
        m_nodetable->addRow();
        m_lineEdit_PointCount->setText(QString::number(m_nodetable->rowCount()));
    }

    void mCaseImplicitSuperElementWindow::slot_onRemoveConstraintRow()
    {
        m_nodetable->removeSelectedRows();
        m_lineEdit_PointCount->setText(QString::number(m_nodetable->rowCount()));
    }

    void mCaseImplicitSuperElementWindow::slot_onPointCountChanged()
    {
        m_nodetable->setRowCount(m_lineEdit_PointCount->text().toInt());
    }

    void mCaseImplicitSuperElementWindow::slot_onImportConstraintFile()
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
            row.nodeId = QString::number(nodeId);
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
        m_lineEdit_PointCount->setText(QString::number(m_nodetable->rowCount()));
        onPageModified(3);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条约束数据。").arg(importedData.size()));
    }

    void mCaseImplicitSuperElementWindow::slot_onExportConstraintFile()
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

            out << QString::number(id) << ", "
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

    void mCaseImplicitSuperElementWindow::updateNextButtonText(int currentIndex)
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

    void mCaseImplicitSuperElementWindow::slot_onNextStep()
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

    void mCaseImplicitSuperElementWindow::slot_onPreviousStep()
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