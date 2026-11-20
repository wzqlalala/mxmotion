#include "mCaseImplicitNTFWindow.h"
#include "mNVHWindowComponent.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QSet>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QItemSelectionModel>
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
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QVector3D>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QMouseEvent>
#include <QHoverEvent>
//全局参数
#include "mGlobalParam.h"
#include "mGenericParamData.h"

//Render
#include "PickCommonFunction.h"
#include "mElementSet.h"
using namespace MechSetData;

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mModelData.h"
#include "mBoundaryDisplacement.h"
#include "mSetManager.h"
#include "mSetBase.h"
#include "mNode.h"
#include "mMixSet.h"
#include "mMapRelationManager.h"
#include "mLoadBoundaryManager.h"
#include "mMeshData.h"
#include "mCaseImplASCoupling.h"
#include "mFreqSamplPointBase.h"
#include "mLoadFreqdomain.h"
#include "mLoadForce.h"
using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseImplicitNTFWindow::mCaseImplicitNTFWindow(QWidget* parent)
        : QDialog(parent)
    {
        setAttribute(Qt::WA_DontShowOnScreen, true);

        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        this->setWindowTitle("NTF分析");
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
            "QTableWidget:disabled { background-color: #F5F7FA; alternate-background-color: #F5F7FA; color: #808080; }"
            "QTableWidget:disabled QHeaderView::section { background-color: #E8EBF2; color: #A8ABB2; }"
        );
        initUI();
        initConnect();
        installInputValidators();
        applySolverDependentState();
        updateFarthestReachedIndex(0);
        updateValidationState();
        captureDefaultState();
        setAttribute(Qt::WA_DontShowOnScreen, false);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
        this->show();
        this->raise();
    }

    mCaseImplicitNTFWindow::~mCaseImplicitNTFWindow()
    {
    }

    bool mCaseImplicitNTFWindow::eventFilter(QObject* watched, QEvent* event)
    {
        bool isNav = (watched == m_listWidget_Nav);
        bool isNavViewport = (m_listWidget_Nav && watched == m_listWidget_Nav->viewport());
        if (isNav || isNavViewport)
        {
            QEvent::Type type = event->type();
            if (type == QEvent::MouseButtonPress ||
                type == QEvent::MouseButtonRelease ||
                type == QEvent::MouseButtonDblClick ||
                type == QEvent::MouseMove ||
                type == QEvent::HoverMove ||
                type == QEvent::HoverEnter ||
                type == QEvent::HoverLeave)
            {
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

    bool mCaseImplicitNTFWindow::isNavigationIndexEnabled(int index) const
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
        return index <= m_farthestReachedIndex;
    }

    void mCaseImplicitNTFWindow::updateFarthestReachedIndex(int index)
    {
        if (index > m_farthestReachedIndex)
        {
            m_farthestReachedIndex = index;
        }
    }

    void mCaseImplicitNTFWindow::resetSubsequentSteps(int fromIndex)
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

    void mCaseImplicitNTFWindow::onPageModified(int pageIndex)
    {
        // 输出页没有“下一步”可触发判勾，控件变化时直接刷新底部操作按钮
        if (m_stackWidget && pageIndex == m_stackWidget->count() - 1)
        {
            updateOutputActionButtons();
            return;
        }
        // 修改已到达过的页：立即置其后的页为不可点击并清除完成标记（打勾仍统一在“下一步”时重新判断）
        if (pageIndex <= m_farthestReachedIndex)
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

    bool mCaseImplicitNTFWindow::isValidDouble(QLineEdit* lineEdit) const
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

    bool mCaseImplicitNTFWindow::isValidPositiveInt(QLineEdit* lineEdit) const
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

    void mCaseImplicitNTFWindow::initLeftNavigation()
    {
        m_listWidget_Nav = new QListWidget(this);
        QFont font;
        font.setPointSize(10);
        m_listWidget_Nav->setFont(font);
        m_listWidget_Nav->setFixedWidth(220);
        m_listWidget_Nav->setFocusPolicy(Qt::NoFocus);
        m_listWidget_Nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget_Nav->setSpacing(0);
        m_listWidget_Nav->setFrameShape(QFrame::NoFrame);
        m_listWidget_Nav->setViewMode(QListView::ListMode);
        m_listWidget_Nav->setUniformItemSizes(true);
        m_listWidget_Nav->setItemDelegate(m_navIconDelegate = new NavIconDelegate(this));
        m_navIconDelegate->setOutputPageIndex(6);
        m_listWidget_Nav->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget_Nav->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_listWidget_Nav->setFixedHeight(48 * 7);

        QListWidgetItem* itemAnalysis = new QListWidgetItem("分析方法", m_listWidget_Nav);
        QListWidgetItem* itemFrequency = new QListWidgetItem("频率范围设置", m_listWidget_Nav);
        QListWidgetItem* itemDamping = new QListWidgetItem("阻尼设置", m_listWidget_Nav);
        QListWidgetItem* itemIncentive = new QListWidgetItem("激励点设置", m_listWidget_Nav);
        QListWidgetItem* itemConstraint = new QListWidgetItem("约束设置", m_listWidget_Nav);
        QListWidgetItem* itemResponse = new QListWidgetItem("响应设置", m_listWidget_Nav);
        QListWidgetItem* itemOutput = new QListWidgetItem("输出设置", m_listWidget_Nav);
        itemAnalysis->setData(Qt::UserRole, 0);
        itemFrequency->setData(Qt::UserRole, 1);
        itemDamping->setData(Qt::UserRole, 2);
        itemIncentive->setData(Qt::UserRole, 3);
        itemConstraint->setData(Qt::UserRole, 4);
        itemResponse->setData(Qt::UserRole, 5);
        itemOutput->setData(Qt::UserRole, 6);
        itemAnalysis->setSizeHint(QSize(120, 48));
        itemFrequency->setSizeHint(QSize(120, 48));
        itemDamping->setSizeHint(QSize(120, 48));
        itemConstraint->setSizeHint(QSize(120, 48));
        itemIncentive->setSizeHint(QSize(120, 48));
        itemResponse->setSizeHint(QSize(120, 48));
        itemOutput->setSizeHint(QSize(120, 48));

        m_listWidget_Nav->setStyleSheet(
            "QListWidget::item { background-color: transparent; color: #606266; "
            "  border: none; padding: 0px; padding-left: 16px; "
            "  min-height: 48px; max-height: 48px; }"
            "QListWidget { background-color: #ffffff; border: none; outline: none; }"
            "QListWidget::item:selected { background-color: transparent; color: #1385F6; }"
            "QListWidget::item:hover:!selected { background-color: transparent; }"
        );

        m_listWidget_Nav->setCurrentItem(itemAnalysis);

        if (m_listWidget_Nav)
        {
            m_listWidget_Nav->installEventFilter(this);
            if (m_listWidget_Nav->viewport())
            {
                m_listWidget_Nav->viewport()->installEventFilter(this);
            }
        }
    }

    void mCaseImplicitNTFWindow::refreshNavigationIcons()
    {
        if (!m_listWidget_Nav || !m_navIconDelegate || !m_stackWidget)
        {
            return;
        }
        m_navIconDelegate->setCurrentPage(m_stackWidget->currentIndex());
        m_listWidget_Nav->viewport()->update();
    }

    void mCaseImplicitNTFWindow::initUI()
    {
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
        m_stackWidget->addWidget(createPageAnalysisMethod());
        m_stackWidget->addWidget(createPageFrequencyRange());
        m_stackWidget->addWidget(createPageDamping());
        m_stackWidget->addWidget(createPageIncentivePoint());
        m_stackWidget->addWidget(createPageConstraint());
        m_stackWidget->addWidget(createPageResponse());
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

        QString primaryButtonStyle =
            "QPushButton { background-color: #1385F6; color: white; border: none; border-radius: 4px; padding: 6px 18px; }"
            "QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
            "QPushButton:hover:!disabled { background-color: #409eff; }";
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
    }

    QWidget* mCaseImplicitNTFWindow::createPageAnalysisMethod()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(20);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // ---- 特征值求解方法 + 放大系数 ----
        QFormLayout* topFormLayout = new QFormLayout();
        topFormLayout->setSpacing(10);
        topFormLayout->setLabelAlignment(Qt::AlignLeft);
        topFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        // 特征值求解方法
        QLabel* labelSolver = new QLabel("特征值求解方法", this);
        m_comboBox_Solver = new QComboBox(this);
        m_comboBox_Solver->addItems({ "自动多重子结构", "兰索斯" });
        m_comboBox_Solver->setFixedWidth(258);
        m_comboBox_Solver->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        connect(m_comboBox_Solver, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mCaseImplicitNTFWindow::slot_onSolverChanged);
        topFormLayout->addRow(labelSolver, m_comboBox_Solver);

        // 放大系数
        m_checkBox_ModeScale = new QCheckBox("放大系数", this);
        m_checkBox_ModeScale->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_checkBox_ModeScale->setChecked(true);
        m_lineEdit_ModeScale = new QLineEdit(this);
        m_lineEdit_ModeScale->setText("5");
        m_lineEdit_ModeScale->setFixedWidth(258);
        topFormLayout->addRow(m_checkBox_ModeScale, m_lineEdit_ModeScale);
        pageLayout->addLayout(topFormLayout);

        // 分隔线 1
        QFrame* line1 = new QFrame(this);
        line1->setFrameShape(QFrame::HLine);
        line1->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line1);
        
        // ---- 结构/流体模态提取范围（左右两列） ----
        QHBoxLayout* rangeHBox = new QHBoxLayout();
        rangeHBox->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout* vLayout1 = new QVBoxLayout();
        QLabel* label1 = new QLabel("结构模态提取范围设置：");
        QFormLayout* formLayout1 = new QFormLayout(page);
        m_lineEdit_FreqMin = new QLineEdit(this);
        m_lineEdit_FreqMax = new QLineEdit(this);
        m_lineEdit_ModeCount = new QLineEdit(this);
        formLayout1->addRow("最小频率（Hz）", m_lineEdit_FreqMin);
        formLayout1->addRow("最大频率（Hz）", m_lineEdit_FreqMax);
        formLayout1->addRow("模态阶数", m_lineEdit_ModeCount);
        vLayout1->addWidget(label1);
        vLayout1->addLayout(formLayout1);
        rangeHBox->addLayout(vLayout1);

        rangeHBox->addSpacing(20);

        QVBoxLayout* vLayout2 = new QVBoxLayout();
        QLabel* label2 = new QLabel("流体模态提取范围设置：");
        QFormLayout* formLayout2 = new QFormLayout(page);
        m_lineEdit_FluidFreqMin = new QLineEdit(this);
        m_lineEdit_FluidFreqMax = new QLineEdit(this);
        m_lineEdit_FluidModeCount = new QLineEdit(this);
        formLayout2->addRow("最小频率（Hz）", m_lineEdit_FluidFreqMin);
        formLayout2->addRow("最大频率（Hz）", m_lineEdit_FluidFreqMax);
        formLayout2->addRow("模态阶数", m_lineEdit_FluidModeCount);
        vLayout2->addWidget(label2);
        vLayout2->addLayout(formLayout2);
        rangeHBox->addLayout(vLayout2);
        pageLayout->addLayout(rangeHBox);

        // 分隔线 2
        QFrame* line2 = new QFrame(this);
        line2->setFrameShape(QFrame::HLine);
        line2->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line2);

        // ---- 声固耦合 ----
        m_checkBox_AcousticSolidCoupling = new QCheckBox("声固耦合", this);
        m_checkBox_AcousticSolidCoupling->setChecked(true);
        pageLayout->addWidget(m_checkBox_AcousticSolidCoupling);

        // ---- 延伸比例 ----
        ratioWidget = new QWidget(this);
        QFormLayout* ratioFormLayout = new QFormLayout(ratioWidget);
        ratioFormLayout->setContentsMargins(0, 0, 0, 0);
        ratioFormLayout->setSpacing(10);
        ratioFormLayout->setLabelAlignment(Qt::AlignLeft);
        ratioFormLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

        QLabel* labelNormal = new QLabel("面单元外法线延伸比例(NORMAL)", this);
        m_lineEdit_NormalExtRatio = new QLineEdit(this);
        m_lineEdit_NormalExtRatio->setText("3");
        m_lineEdit_NormalExtRatio->setFixedWidth(160);
        ratioFormLayout->addRow(labelNormal, m_lineEdit_NormalExtRatio);

        QLabel* labelSkneps = new QLabel("面单元横向延伸比例(SKNEPS)", this);
        m_lineEdit_SknepsExtRatio = new QLineEdit(this);
        m_lineEdit_SknepsExtRatio->setText("6");
        m_lineEdit_SknepsExtRatio->setFixedWidth(160);
        ratioFormLayout->addRow(labelSkneps, m_lineEdit_SknepsExtRatio);

        pageLayout->addWidget(ratioWidget);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitNTFWindow::createPageFrequencyRange()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* outerLayout = new QVBoxLayout(page);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        QScrollArea* scrollArea = new QScrollArea(page);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outerLayout->addWidget(scrollArea);
        QWidget* container = new QWidget(scrollArea);
        QVBoxLayout* pageLayout = new QVBoxLayout(container);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(5, 0, 5, 5);
        pageLayout->setAlignment(Qt::AlignTop);
        scrollArea->setWidget(container);

        m_FREQ = new QCheckBox("FREQ", this);
        pageLayout->addWidget(m_FREQ);
        m_widget_FREQ = new QWidget(this);
        QHBoxLayout* layout_FREQ = new QHBoxLayout(m_widget_FREQ);
        layout_FREQ->setContentsMargins(0, 0, 0, 0);
        QLabel* label1 = new QLabel("频率点F");
        label1->setMinimumWidth(178);
        layout_FREQ->addWidget(label1);
        m_FrepPoint1 = new QLineEdit(this);
        layout_FREQ->addWidget(m_FrepPoint1);
        m_widget_FREQ->setEnabled(false);
        pageLayout->addWidget(m_widget_FREQ);

        m_FREQ1 = new QCheckBox("FREQ1", this);
        m_FREQ1->setChecked(true);
        pageLayout->addWidget(m_FREQ1);
        m_widget_FREQ1 = new QWidget(this);
        QGridLayout* layout_FREQ1 = new QGridLayout(m_widget_FREQ1);
        layout_FREQ1->setSpacing(12);
        layout_FREQ1->setContentsMargins(0, 0, 0, 0);
        layout_FREQ1->setColumnStretch(0, 1);
        layout_FREQ1->setColumnStretch(1, 1);
        layout_FREQ1->setColumnStretch(2, 1);
        layout_FREQ1->setColumnStretch(3, 1);
        QLabel* label2 = new QLabel("起始频率F1");
        QLabel* label3 = new QLabel("起始步长DF");
        QLabel* label4 = new QLabel("线性分布的频率总点数NDF");
        m_FREQ1_F1 = new QLineEdit(this);
        m_FREQ1_DF = new QLineEdit(this);
        m_FREQ1_NDF = new QLineEdit(this);
        layout_FREQ1->addWidget(label2, 0, 0);
        layout_FREQ1->addWidget(m_FREQ1_F1, 0, 1);
        layout_FREQ1->addWidget(label3, 0, 2);
        layout_FREQ1->addWidget(m_FREQ1_DF, 0, 3);
        layout_FREQ1->addWidget(label4, 1, 0);
        layout_FREQ1->addWidget(m_FREQ1_NDF, 1, 1);
        pageLayout->addWidget(m_widget_FREQ1);

        m_FREQ2 = new QCheckBox("FREQ2", this);
        pageLayout->addWidget(m_FREQ2);
        m_widget_FREQ2 = new QWidget(this);
        QGridLayout* layout_FREQ2 = new QGridLayout(m_widget_FREQ2);
        layout_FREQ2->setSpacing(12);
        layout_FREQ2->setContentsMargins(0, 0, 0, 0);
        layout_FREQ2->setColumnStretch(0, 1);
        layout_FREQ2->setColumnStretch(1, 1);
        layout_FREQ2->setColumnStretch(2, 1);
        layout_FREQ2->setColumnStretch(3, 1);
        QLabel* label5 = new QLabel("起始频率F1");
        QLabel* label6 = new QLabel("终止频率F2");
        QLabel* label7 = new QLabel("对数分布的频率点总数NF");
        m_FREQ2_F1 = new QLineEdit(this);
        m_FREQ2_DF = new QLineEdit(this);
        m_FREQ2_NF = new QLineEdit(this);
        layout_FREQ2->addWidget(label5, 0, 0);
        layout_FREQ2->addWidget(m_FREQ2_F1, 0, 1);
        layout_FREQ2->addWidget(label6, 0, 2);
        layout_FREQ2->addWidget(m_FREQ2_DF, 0, 3);
        layout_FREQ2->addWidget(label7, 1, 0);
        layout_FREQ2->addWidget(m_FREQ2_NF, 1, 1);
        m_widget_FREQ2->setEnabled(false);
        pageLayout->addWidget(m_widget_FREQ2);

        m_FREQ3 = new QCheckBox("FREQ3", this);
        pageLayout->addWidget(m_FREQ3);
        m_widget_FREQ3 = new QWidget(this);
        QGridLayout* layout_FREQ3 = new QGridLayout(m_widget_FREQ3);
        layout_FREQ3->setSpacing(12);
        layout_FREQ3->setContentsMargins(0, 0, 0, 0);
        layout_FREQ3->setColumnStretch(0, 1);
        layout_FREQ3->setColumnStretch(1, 1);
        layout_FREQ3->setColumnStretch(2, 1);
        layout_FREQ3->setColumnStretch(3, 1);
        QLabel* label8 = new QLabel("起始频率F1");
        QLabel* label9 = new QLabel("终止频率F2");
        QLabel* label10 = new QLabel("计算形式");
        QLabel* label11 = new QLabel("每个子区间的频率等分数NEF");
        QLabel* label12 = new QLabel("聚集系数CLUSTER");
        m_FREQ3_F1 = new QLineEdit(this);
        m_FREQ3_F2 = new QLineEdit(this);
        m_FREQ3_Form = new QComboBox(this);
        m_FREQ3_Form->addItem("线性");
        m_FREQ3_NEF = new QLineEdit(this);
        m_FREQ3_CLUSTER = new QLineEdit(this);
        layout_FREQ3->addWidget(label8, 0, 0);
        layout_FREQ3->addWidget(m_FREQ3_F1, 0, 1);
        layout_FREQ3->addWidget(label9, 0, 2);
        layout_FREQ3->addWidget(m_FREQ3_F2, 0, 3);
        layout_FREQ3->addWidget(label10, 1, 0);
        layout_FREQ3->addWidget(m_FREQ3_Form, 1, 1);
        layout_FREQ3->addWidget(label11, 1, 2);
        layout_FREQ3->addWidget(m_FREQ3_NEF, 1, 3);
        layout_FREQ3->addWidget(label12, 2, 0);
        layout_FREQ3->addWidget(m_FREQ3_CLUSTER, 2, 1);
        m_widget_FREQ3->setEnabled(false);
        pageLayout->addWidget(m_widget_FREQ3);

        m_FREQ4 = new QCheckBox("FREQ4", this);
        pageLayout->addWidget(m_FREQ4);
        m_widget_FREQ4 = new QWidget(this);
        QGridLayout* layout_FREQ4 = new QGridLayout(m_widget_FREQ4);
        layout_FREQ4->setSpacing(12);
        layout_FREQ4->setContentsMargins(0, 0, 0, 0);
        layout_FREQ4->setColumnStretch(0, 1);
        layout_FREQ4->setColumnStretch(1, 1);
        layout_FREQ4->setColumnStretch(2, 1);
        layout_FREQ4->setColumnStretch(3, 1);
        QLabel* label13 = new QLabel("起始频率F1");
        QLabel* label14 = new QLabel("终止频率F2");
        QLabel* label15 = new QLabel("模态频率子区间的百分比FSPD");
        QLabel* label16 = new QLabel("子区间频率的均匀分布个数NFM");
        m_FREQ4_F1 = new QLineEdit(this);
        m_FREQ4_F2 = new QLineEdit(this);
        m_FREQ4_FSPD = new QLineEdit(this);
        m_FREQ4_NFM = new QLineEdit(this);
        layout_FREQ4->addWidget(label13, 0, 0);
        layout_FREQ4->addWidget(m_FREQ4_F1, 0, 1);
        layout_FREQ4->addWidget(label14, 0, 2);
        layout_FREQ4->addWidget(m_FREQ4_F2, 0, 3);
        layout_FREQ4->addWidget(label15, 1, 0);
        layout_FREQ4->addWidget(m_FREQ4_FSPD, 1, 1);
        layout_FREQ4->addWidget(label16, 1, 2);
        layout_FREQ4->addWidget(m_FREQ4_NFM, 1, 3);
        m_widget_FREQ4->setEnabled(false);
        pageLayout->addWidget(m_widget_FREQ4);

        return page;
    }

    QWidget* mCaseImplicitNTFWindow::createPageIncentivePoint()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // 第一行：节点个数 + 添加一行 + 删除 + 从文件导入
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setSpacing(10);
        topLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* countLabel = new QLabel("节点个数：", this);
        m_Incentive_PointCount = new QLineEdit(this);
        m_Incentive_PointCount->setMinimumWidth(290);
        m_Incentive_PointCount->setText("1");

        m_Incentive_AddRow = new QPushButton(this);
        m_Incentive_AddRow->setMaximumWidth(30);
        m_Incentive_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_Incentive_AddRow->setIconSize(QSize(14, 14));
        m_Incentive_RemoveRow = new QPushButton(this);
        m_Incentive_RemoveRow->setMaximumWidth(30);
        m_Incentive_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_Incentive_RemoveRow->setIconSize(QSize(14, 14));
        m_Incentive_RemoveRow->setEnabled(false);
        m_Incentive_ImportFile = new QPushButton("导入文件", this);
        m_Incentive_ExportFile = new QPushButton("导出文件", this);
        m_Incentive_AddRow->setAutoDefault(false);
        m_Incentive_RemoveRow->setAutoDefault(false);
        m_Incentive_ImportFile->setAutoDefault(false);
        m_Incentive_ExportFile->setAutoDefault(false);

        topLayout->addWidget(countLabel);
        topLayout->addWidget(m_Incentive_PointCount);
        topLayout->addWidget(m_Incentive_AddRow);
        topLayout->addWidget(m_Incentive_RemoveRow);
        topLayout->addStretch();
        topLayout->addWidget(m_Incentive_ImportFile);
        topLayout->addWidget(m_Incentive_ExportFile);

        pageLayout->addLayout(topLayout);

        m_Incentive_NodeTable = new IncentiveNodeTableWidget(this);
        m_Incentive_NodeTable->setShowLabelColumn(true);
        pageLayout->addWidget(m_Incentive_NodeTable, 1);

        return page;
    }

    QScrollArea* mCaseImplicitNTFWindow::createPageDamping()
    {
        QScrollArea* page = new QScrollArea(this);
        page->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"   // 视口背景透明
        );

        QWidget* container = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(container);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        QHBoxLayout* hLayout = new QHBoxLayout(this);
        hLayout->setContentsMargins(3, 0, 0, 0);
        m_checkBox_EnableDamping = new QCheckBox("结构阻尼设置：", this);
        m_checkBox_EnableDamping->setChecked(true);
        hLayout->addWidget(m_checkBox_EnableDamping);
        pageLayout->addLayout(hLayout);

        m_dampingSettingsContainer = new QWidget(container);
        m_dampingSettingsContainer->setMinimumHeight(300);
        QVBoxLayout* dampingSettingsLayout = new QVBoxLayout(m_dampingSettingsContainer);
        dampingSettingsLayout->setSpacing(10);
        dampingSettingsLayout->setContentsMargins(0, 0, 0, 0);
        dampingSettingsLayout->setAlignment(Qt::AlignTop);

        QHBoxLayout* structuralLayout = new QHBoxLayout();
        m_StructuralDamping = new QRadioButton("全局结构阻尼：", this);
        m_StructuralDamping->setChecked(true);
        m_editStructuralDamping = new QLineEdit(this);
        m_editStructuralDamping->setFixedWidth(245);
        structuralLayout->addWidget(m_StructuralDamping);
        structuralLayout->addWidget(m_editStructuralDamping);
        structuralLayout->addStretch(1);
        dampingSettingsLayout->addLayout(structuralLayout);
        m_ModalDamping = new QRadioButton("全局模态阻尼", this);
        dampingSettingsLayout->addWidget(m_ModalDamping);

        m_modelDampingWidget = new QWidget(m_dampingSettingsContainer);
        QVBoxLayout* modelDampingLayout = new QVBoxLayout(m_modelDampingWidget);
        QHBoxLayout* topLayout = new QHBoxLayout(m_modelDampingWidget);
        topLayout->setSpacing(10);
        topLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* countLabel = new QLabel("阻尼个数：", this);
        m_ModalDamping_PointCount = new QLineEdit(this);
        m_ModalDamping_PointCount->setFixedWidth(113);
        topLayout->addWidget(countLabel);
        topLayout->addWidget(m_ModalDamping_PointCount);
        QLabel* typeLabel = new QLabel("类型：", this);
        m_ModalDamping_Type = new QComboBox(this);
        m_ModalDamping_Type->setFixedWidth(113);
        m_ModalDamping_Type->addItems({ "G","CRIT","Q" });
        m_ModalDamping_Type->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        topLayout->addWidget(typeLabel);
        topLayout->addWidget(m_ModalDamping_Type);

        m_ModalDamping_AddRow = new QPushButton(this);
        m_ModalDamping_AddRow->setMaximumWidth(30);
        m_ModalDamping_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_ModalDamping_AddRow->setIconSize(QSize(14, 14));
        m_ModalDamping_RemoveRow = new QPushButton(this);
        m_ModalDamping_RemoveRow->setMaximumWidth(30);
        m_ModalDamping_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_ModalDamping_RemoveRow->setIconSize(QSize(14, 14));
        m_ModalDamping_RemoveRow->setEnabled(false);
        m_ModalDamping_RemoveRow->setAutoDefault(false);
        m_ModalDamping_ImportFile = new QPushButton("导入文件", this);
        m_ModalDamping_ExportFile = new QPushButton("导出文件", this);
        topLayout->addWidget(m_ModalDamping_AddRow);
        topLayout->addWidget(m_ModalDamping_RemoveRow);
        topLayout->addStretch();
        topLayout->addWidget(m_ModalDamping_ImportFile);
        topLayout->addWidget(m_ModalDamping_ExportFile);
        modelDampingLayout->addLayout(topLayout);

        m_ModalDamping_Table = new QTableWidget(this);
        m_ModalDamping_Table->setColumnCount(2);
        m_ModalDamping_Table->setHorizontalHeaderLabels(QStringList() << "频率" << "阻尼");
        m_ModalDamping_Table->verticalHeader()->setVisible(false);
        m_ModalDamping_Table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_ModalDamping_Table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_ModalDamping_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ModalDamping_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_ModalDamping_Table->setShowGrid(false);
        m_ModalDamping_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_ModalDamping_Table->setStyleSheet(
            "QTableWidget {"
            "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
            "   background-color: #FFFFFF;"             /* 偶数行背景 */
            "   selection-background-color: #D0E7FD;"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "    border-radius: 2px;"
            "}"
            "QTableWidget::item {"
            "  border: none;"
            "  border-bottom: 1px solid #ccc;"
            "   padding-left: 8px;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
            "QTableWidget:disabled {"
            "   background-color: #F5F7FA;"
            "   alternate-background-color: #F5F7FA;"
            "   color: #808080;"
            "}"
            "QTableWidget:disabled QHeaderView::section { background-color: #E8EBF2; color: #A8ABB2; }"
        );
        modelDampingLayout->addWidget(m_ModalDamping_Table, 1);

        // 默认保留2行
        slot_onAddModalDampingRow();
        slot_onAddModalDampingRow();
        dampingSettingsLayout->addWidget(m_modelDampingWidget);
        m_modelDampingWidget->setEnabled(false);

        pageLayout->addWidget(m_dampingSettingsContainer);

        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line);

        QHBoxLayout* hLayout1 = new QHBoxLayout(this);
        hLayout1->setContentsMargins(3, 0, 0, 0);
        m_checkBox_FluidEnableDamping = new QCheckBox("流体阻尼设置：", this);
        m_checkBox_FluidEnableDamping->setChecked(false);
        hLayout1->addWidget(m_checkBox_FluidEnableDamping);
        pageLayout->addLayout(hLayout1);

        m_FluiddampingSettingsContainer = new QWidget(page);
        m_FluiddampingSettingsContainer->setMinimumHeight(300);
        QVBoxLayout* dampingSettingsLayout1 = new QVBoxLayout(m_FluiddampingSettingsContainer);
        dampingSettingsLayout1->setSpacing(10);
        dampingSettingsLayout1->setContentsMargins(0, 0, 0, 0);
        dampingSettingsLayout1->setAlignment(Qt::AlignTop);

        QHBoxLayout* structuralLayout1 = new QHBoxLayout();
        m_FluidStructuralDamping = new QRadioButton("全局流体阻尼：", this);
        m_FluidStructuralDamping->setChecked(true);
        m_FluideditStructuralDamping = new QLineEdit(this);
        m_FluideditStructuralDamping->setFixedWidth(245);
        structuralLayout1->addWidget(m_FluidStructuralDamping);
        structuralLayout1->addWidget(m_FluideditStructuralDamping);
        structuralLayout1->addStretch(1);
        dampingSettingsLayout1->addLayout(structuralLayout1);
        m_FluidModalDamping = new QRadioButton("全局流体阻尼", this);
        dampingSettingsLayout1->addWidget(m_FluidModalDamping);

        m_FluidmodelDampingWidget = new QWidget(m_FluiddampingSettingsContainer);
        QVBoxLayout* modelDampingLayout1 = new QVBoxLayout(m_FluidmodelDampingWidget);
        QHBoxLayout* topLayout1 = new QHBoxLayout(m_FluidmodelDampingWidget);
        topLayout1->setSpacing(10);
        topLayout1->setContentsMargins(0, 0, 0, 0);
        QLabel* countLabel1 = new QLabel("阻尼个数：", this);
        m_FluidModalDamping_PointCount = new QLineEdit(this);
        m_FluidModalDamping_PointCount->setFixedWidth(113);
        topLayout1->addWidget(countLabel1);
        topLayout1->addWidget(m_FluidModalDamping_PointCount);
        QLabel* typeLabel1 = new QLabel("类型：", this);
        m_FluidModalDamping_Type = new QComboBox(this);
        m_FluidModalDamping_Type->setFixedWidth(113);
        m_FluidModalDamping_Type->addItems({ "G","CRIT","Q" });
        m_FluidModalDamping_Type->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        topLayout1->addWidget(typeLabel1);
        topLayout1->addWidget(m_FluidModalDamping_Type);

        m_FluidModalDamping_AddRow = new QPushButton(this);
        m_FluidModalDamping_AddRow->setMaximumWidth(30);
        m_FluidModalDamping_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_FluidModalDamping_AddRow->setIconSize(QSize(14, 14));
        m_FluidModalDamping_RemoveRow = new QPushButton(this);
        m_FluidModalDamping_RemoveRow->setMaximumWidth(30);
        m_FluidModalDamping_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_FluidModalDamping_RemoveRow->setIconSize(QSize(14, 14));
        m_FluidModalDamping_RemoveRow->setEnabled(false);
        m_FluidModalDamping_RemoveRow->setAutoDefault(false);
        m_FluidModalDamping_ImportFile = new QPushButton("导入文件", this);
        m_FluidModalDamping_ExportFile = new QPushButton("导出文件", this);
        topLayout1->addWidget(m_FluidModalDamping_AddRow);
        topLayout1->addWidget(m_FluidModalDamping_RemoveRow);
        topLayout1->addStretch();
        topLayout1->addWidget(m_FluidModalDamping_ImportFile);
        topLayout1->addWidget(m_FluidModalDamping_ExportFile);
        modelDampingLayout1->addLayout(topLayout1);

        m_FluidModalDamping_Table = new QTableWidget(this);
        m_FluidModalDamping_Table->setColumnCount(2);
        m_FluidModalDamping_Table->setHorizontalHeaderLabels(QStringList() << "频率" << "阻尼");
        m_FluidModalDamping_Table->verticalHeader()->setVisible(false);
        m_FluidModalDamping_Table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_FluidModalDamping_Table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_FluidModalDamping_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_FluidModalDamping_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_FluidModalDamping_Table->setShowGrid(false);
        m_FluidModalDamping_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_FluidModalDamping_Table->setStyleSheet(
            "QTableWidget {"
            "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
            "   background-color: #FFFFFF;"             /* 偶数行背景 */
            "   selection-background-color: #D0E7FD;"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "    border-radius: 2px;"
            "}"
            "QTableWidget::item {"
            "  border: none;"
            "  border-bottom: 1px solid #ccc;"
            "   padding-left: 8px;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
            "QTableWidget:disabled {"
            "   background-color: #F5F7FA;"
            "   alternate-background-color: #F5F7FA;"
            "   color: #808080;"
            "}"
            "QTableWidget:disabled QHeaderView::section { background-color: #E8EBF2; color: #A8ABB2; }"
        );
        modelDampingLayout1->addWidget(m_FluidModalDamping_Table, 1);

        // 默认保留2行
        slot_onAddFluidModalDampingRow();
        slot_onAddFluidModalDampingRow();
        dampingSettingsLayout1->addWidget(m_FluidmodelDampingWidget);
        m_FluidmodelDampingWidget->setEnabled(false);

        pageLayout->addWidget(m_FluiddampingSettingsContainer);
        m_FluiddampingSettingsContainer->setEnabled(false);
        page->setWidget(container);
        page->setWidgetResizable(true);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        page->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return page;
    }

    QScrollArea* mCaseImplicitNTFWindow::createPageResponse()
    {
        QScrollArea* page = new QScrollArea(this);
        page->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"   // 视口背景透明
        );
        // 1. 创建内部容器
        QWidget* container = new QWidget();
        QVBoxLayout* pageLayout = new QVBoxLayout(container);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // 2. 创建并添加可折叠面板
        m_SoundPressureResponse = new ResponseSettingFoldablePanel("声压响应设置", this);
        m_VibrationResponse = new ResponseSettingFoldablePanel("振动响应设置", this);
        m_NodalForces = new ResponseSettingFoldablePanel("节点力设置", this);
        m_ElementForces = new ResponseSettingFoldablePanel("单元力设置", this);
        m_ConstraintReaction = new ResponseSettingFoldablePanel("约束反力设置", this);

        pageLayout->addWidget(m_SoundPressureResponse);
        pageLayout->addWidget(m_VibrationResponse);
        pageLayout->addWidget(m_NodalForces);
        pageLayout->addWidget(m_ElementForces);
        pageLayout->addWidget(m_ConstraintReaction);
        pageLayout->addStretch(1);

        m_VibrationResponse->collapsedOnInitialization();
        m_NodalForces->collapsedOnInitialization();
        m_ElementForces->collapsedOnInitialization();
        m_ConstraintReaction->collapsedOnInitialization();
        page->setWidget(container);
        page->setWidgetResizable(true);
        container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        page->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return page;
    }

    QWidget* mCaseImplicitNTFWindow::createPageConstraint()
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
        m_constraint_PointCount = new QLineEdit(this);
        m_constraint_PointCount->setMinimumWidth(290);
        m_constraint_PointCount->setText("0");
        m_constraint_AddRow = new QPushButton(this);
        m_constraint_AddRow->setMaximumWidth(30);
        m_constraint_RemoveRow = new QPushButton(this);
        m_constraint_RemoveRow->setMaximumWidth(30);
        m_constraint_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_constraint_AddRow->setIconSize(QSize(14, 14));
        m_constraint_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_constraint_RemoveRow->setIconSize(QSize(14, 14));
        m_constraint_RemoveRow->setEnabled(false);
        m_constraint_AddRow->setAutoDefault(false);
        m_constraint_RemoveRow->setAutoDefault(false);
        topLayout->addWidget(countLabel);
        topLayout->addWidget(m_constraint_PointCount);
        topLayout->addWidget(m_constraint_AddRow);
        topLayout->addWidget(m_constraint_RemoveRow);
        topLayout->addStretch();

        m_constraint_ImportFile = new QPushButton("导入文件", this);
        m_constraint_ExportFile = new QPushButton("导出文件", this);
        topLayout->addWidget(m_constraint_ImportFile);
        topLayout->addWidget(m_constraint_ExportFile);

        pageLayout->addLayout(topLayout);
        m_constraint_NodeTable = new IncentiveNodeTableWidget(this);
        m_constraint_NodeTable->setEmptyDefault();   // 约束表初始不保留默认行，节点个数为0
        pageLayout->addWidget(m_constraint_NodeTable, 1);

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

    QScrollArea* mCaseImplicitNTFWindow::createPageOutput()
    {
        QScrollArea* scrollPage = new QScrollArea(this);
        scrollPage->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"   // 视口背景透明
        );

        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // ---------- 折叠框 1：响应点曲线结果输出 ----------
        m_outputCurveButton = new QPushButton("响应点曲线结果输出", this);
        m_outputCurveButton->setIcon(drawOutputFoldIcon(true));
        m_outputCurveButton->setLayoutDirection(Qt::LeftToRight);
        m_outputCurveButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; padding-left:8px;"
            "  background:#ffffff; color:#000000; font-weight:bold; min-height:28px; }"
            "QPushButton:hover{background:#ffffff;}");
        connect(m_outputCurveButton, &QPushButton::clicked, this, [this]() {
            bool visible = m_outputCurveContentWidget->isVisible();
            m_outputCurveContentWidget->setVisible(!visible);
            m_outputCurveButton->setIcon(drawOutputFoldIcon(!visible));
            });
        pageLayout->addWidget(m_outputCurveButton);

        m_outputCurveContentWidget = new QWidget(this);
        m_outputCurveContentWidget->setMaximumHeight(120);
        QHBoxLayout* curveLayout = new QHBoxLayout(m_outputCurveContentWidget);
        curveLayout->setSpacing(15);
        curveLayout->setContentsMargins(10, 5, 10, 5);
        curveLayout->setAlignment(Qt::AlignTop);

        // 第一列振动结果输出
        QVBoxLayout* vibrationLayout = new QVBoxLayout();
        vibrationLayout->setSpacing(10);
        vibrationLayout->setContentsMargins(0, 0, 0, 0);
        vibrationLayout->setAlignment(Qt::AlignTop);
        QLabel* labelVibration = new QLabel("振动结果输出：", this);
        m_OutputDisplacement = new QCheckBox("位移", this);
        m_OutputDisplacement->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputDisplacement->setEnabled(false);
        m_OutputSpeed = new QCheckBox("速度", this);
        m_OutputSpeed->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputSpeed->setEnabled(false);
        m_Outputaccelerated = new QCheckBox("加速度", this);
        m_Outputaccelerated->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_Outputaccelerated->setEnabled(false);
        vibrationLayout->addWidget(labelVibration);
        vibrationLayout->addWidget(m_OutputDisplacement);
        vibrationLayout->addWidget(m_OutputSpeed);
        vibrationLayout->addWidget(m_Outputaccelerated);
        vibrationLayout->addStretch(1);

        // 竖线分割1
        QFrame* vLine1 = new QFrame(this);
        vLine1->setFrameShape(QFrame::VLine);
        vLine1->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        vLine1->setFixedWidth(1);
        vLine1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // 第二列激励点动刚度结果输出
        QVBoxLayout* incentiveLayout = new QVBoxLayout();
        incentiveLayout->setSpacing(10);
        incentiveLayout->setContentsMargins(0, 0, 0, 0);
        incentiveLayout->setAlignment(Qt::AlignTop);
        QLabel* labelIncentive = new QLabel("激励点动刚度结果输出：", this);
        m_IncentiveDisplacement = new QCheckBox("位移", this);
        m_IncentiveDisplacement->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_IncentiveDisplacement->setEnabled(false);
        m_IncentiveSpeed = new QCheckBox("速度", this);
        m_IncentiveSpeed->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_IncentiveSpeed->setEnabled(false);
        m_Incentiveaccelerated = new QCheckBox("加速度", this);
        m_Incentiveaccelerated->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_Incentiveaccelerated->setEnabled(false);
        incentiveLayout->addWidget(labelIncentive);
        incentiveLayout->addWidget(m_IncentiveDisplacement);
        incentiveLayout->addWidget(m_IncentiveSpeed);
        incentiveLayout->addWidget(m_Incentiveaccelerated);
        incentiveLayout->addStretch(1);

        // 竖线分割2
        QFrame* vLine2 = new QFrame(this);
        vLine2->setFrameShape(QFrame::VLine);
        vLine2->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        vLine2->setFixedWidth(1);
        vLine2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // 第三列力结果输出
        QVBoxLayout* forceLayout = new QVBoxLayout();
        forceLayout->setSpacing(10);
        forceLayout->setContentsMargins(0, 0, 0, 0);
        forceLayout->setAlignment(Qt::AlignTop);
        QLabel* labelForce = new QLabel("力结果输出：", this);
        m_OutputNodesForce = new QCheckBox("节点力", this);
        m_OutputNodesForce->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputNodesForce->setEnabled(false);
        m_OutputElementForce = new QCheckBox("单元力", this);
        m_OutputElementForce->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputElementForce->setEnabled(false);
        m_OutputConstraintReaction = new QCheckBox("约束反力", this);
        m_OutputConstraintReaction->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputConstraintReaction->setEnabled(false);
        forceLayout->addWidget(labelForce);
        forceLayout->addWidget(m_OutputNodesForce);
        forceLayout->addWidget(m_OutputElementForce);
        forceLayout->addWidget(m_OutputConstraintReaction);
        forceLayout->addStretch(1);

        // 竖线分割3
        QFrame* vLine3 = new QFrame(this);
        vLine3->setFrameShape(QFrame::VLine);
        vLine3->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        vLine3->setFixedWidth(1);
        vLine3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // 第四列声压结果输出
        QVBoxLayout* soundLayout = new QVBoxLayout();
        soundLayout->setSpacing(10);
        soundLayout->setContentsMargins(0, 0, 0, 0);
        soundLayout->setAlignment(Qt::AlignTop);
        QLabel* labelSound = new QLabel("声压结果输出：", this);
        m_OutputSoundPressure = new QCheckBox("声压", this);
        m_OutputSoundPressure->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_OutputSoundPressure->setEnabled(false);
        soundLayout->addWidget(labelSound);
        soundLayout->addWidget(m_OutputSoundPressure);
        soundLayout->addStretch(1);

        curveLayout->addLayout(vibrationLayout);
        curveLayout->addSpacing(50);
        curveLayout->addWidget(vLine1);
        curveLayout->addLayout(incentiveLayout);
        curveLayout->addSpacing(50);
        curveLayout->addWidget(vLine2);
        curveLayout->addLayout(forceLayout);
        curveLayout->addSpacing(50);
        curveLayout->addWidget(vLine3);
        curveLayout->addLayout(soundLayout);
        pageLayout->addWidget(m_outputCurveContentWidget);

        // ---------- 折叠框 2：全局云图结果输出 ----------
        m_outputCloudButton = new QPushButton("全局云图结果输出", this);
        m_outputCloudButton->setIcon(drawOutputFoldIcon(false));
        m_outputCloudButton->setLayoutDirection(Qt::LeftToRight);
        m_outputCloudButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; padding-left:8px;"
            "  background:#ffffff; color:#000000; font-weight:bold; min-height:28px; }"
            "QPushButton:hover{background:#ffffff;}");
        connect(m_outputCloudButton, &QPushButton::clicked, this, [this]() {
            bool visible = m_outputCloudContentWidget->isVisible();
            m_outputCloudContentWidget->setVisible(!visible);
            m_outputCloudButton->setIcon(drawOutputFoldIcon(!visible));
            });
        pageLayout->addWidget(m_outputCloudButton);

        m_outputCloudContentWidget = new QWidget(this);
        m_outputCloudContentWidget->setMaximumHeight(150);
        QVBoxLayout* cloudMainLayout = new QVBoxLayout(m_outputCloudContentWidget);
        cloudMainLayout->setSpacing(10);
        cloudMainLayout->setContentsMargins(10, 5, 10, 5);
        cloudMainLayout->setAlignment(Qt::AlignTop);

        QHBoxLayout* formatLayout = new QHBoxLayout();
        formatLayout->setSpacing(10);
        formatLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* formatLabel = new QLabel("输出格式", this);
        m_comboBox_outputForm = new QComboBox(this);
        m_comboBox_outputForm->addItems({ "mxdb", "op2", "h5" });   // 与工况面板的输出文件类型一致，默认 mxdb
        m_comboBox_outputForm->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_comboBox_outputForm->setMinimumWidth(200);
        formatLayout->addWidget(formatLabel);
        formatLayout->addWidget(m_comboBox_outputForm);
        formatLayout->addStretch();
        cloudMainLayout->addLayout(formatLayout);
        QLabel* obslabel = new QLabel("ODS设置");
        m_ODSDisplacement = new QCheckBox("位移", this);
        m_ODSStrain = new QCheckBox("应变期", this);
        m_ODSPeckout = new QCheckBox("Peckout", this);
        m_ODSPeckout->setEnabled(false);
        cloudMainLayout->addWidget(obslabel);
        cloudMainLayout->addWidget(m_ODSDisplacement);
        cloudMainLayout->addWidget(m_ODSStrain);
        cloudMainLayout->addWidget(m_ODSPeckout);
        pageLayout->addWidget(m_outputCloudContentWidget);
        m_outputCloudContentWidget->setVisible(false);

        // ---------- 折叠框 3：峰值筛选 ----------
        m_PeakSelectionButton = new QPushButton("峰值筛选", this);
        m_PeakSelectionButton->setIcon(drawOutputFoldIcon(false));
        m_PeakSelectionButton->setLayoutDirection(Qt::LeftToRight);
        m_PeakSelectionButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; padding-left:8px;"
            "  background:#ffffff; color:#000000; font-weight:bold; min-height:28px; }"
            "QPushButton:hover{background:#ffffff;}");
        connect(m_PeakSelectionButton, &QPushButton::clicked, this, [this]() {
            bool visible = m_PeakSelectionContentWidget->isVisible();
            m_PeakSelectionContentWidget->setVisible(!visible);
            m_PeakSelectionButton->setIcon(drawOutputFoldIcon(!visible));
            });
        pageLayout->addWidget(m_PeakSelectionButton);

        m_PeakSelectionContentWidget = new QWidget(this);
        QVBoxLayout* PeakSelectionLayout = new QVBoxLayout(m_PeakSelectionContentWidget);
        PeakSelectionLayout->setSpacing(10);
        PeakSelectionLayout->setContentsMargins(0, 0, 0, 0);
        PeakSelectionLayout->setAlignment(Qt::AlignTop);
        m_PeakoutSetting = new QCheckBox("Peakout设置：",this);
        PeakSelectionLayout->addWidget(m_PeakoutSetting);
        m_PeakoutSettingContentWidget = new QWidget(m_PeakSelectionContentWidget);
        QVBoxLayout* PeakSettingLayout = new QVBoxLayout(m_PeakoutSettingContentWidget);
        PeakSettingLayout->setContentsMargins(0, 0, 0, 0);
        QHBoxLayout* hLayout = new QHBoxLayout(m_PeakoutSettingContentWidget);
        QLabel* labelSID = new QLabel("SID");
        QLabel* labelNPEAK = new QLabel("NPEAK");
        QLabel* labelNEAR = new QLabel("NEAR");
        QLabel* labelFAR= new QLabel("FAR");
        QLabel* labelLFREQ = new QLabel("LFREQ");
        QLabel* labelHFREQ = new QLabel("HFREQ");
        m_edit_SID = new QLineEdit(this);
        m_edit_SID->setText("1");
        m_edit_NPEAK = new QLineEdit(this);
        m_edit_NPEAK->setText("5");
        m_edit_NEAR = new QLineEdit(this);
        m_edit_NEAR->setText("0.000");
        m_edit_FAR = new QLineEdit(this);
        m_edit_FAR->setText("0.000");
        m_edit_LFREQ = new QLineEdit(this);
        m_edit_LFREQ->setText("20");
        m_edit_HFREQ = new QLineEdit(this);
        m_edit_HFREQ->setText("0.000");
        hLayout->addWidget(labelSID);
        hLayout->addWidget(m_edit_SID);
        hLayout->addWidget(labelNPEAK);
        hLayout->addWidget(m_edit_NPEAK);
        hLayout->addWidget(labelNEAR);
        hLayout->addWidget(m_edit_NEAR);
        hLayout->addWidget(labelFAR);
        hLayout->addWidget(m_edit_FAR);
        hLayout->addWidget(labelLFREQ);
        hLayout->addWidget(m_edit_LFREQ);
        hLayout->addWidget(labelHFREQ);
        hLayout->addWidget(m_edit_HFREQ);
        PeakSettingLayout->addLayout(hLayout);

        QHBoxLayout* hLayout1 = new QHBoxLayout(m_PeakoutSettingContentWidget);
        QLabel* labelTYPE = new QLabel("TYPE");
        QLabel* labelSCAlE = new QLabel("SCAlE");
        m_comboBox_TYPE = new QComboBox(this);
        m_comboBox_TYPE->addItems({ "DISP","VELO","ACCE" });
        m_comboBox_TYPE->setMinimumWidth(120);
        m_comboBox_TYPE->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        m_comboBox_SCALE = new QComboBox(this);
        m_comboBox_SCALE->addItems({ "DB","DBA","NONE" });
        m_comboBox_SCALE->setMinimumWidth(120);
        m_comboBox_SCALE->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        m_PeakSelection_AddRow = new QPushButton(this);
        m_PeakSelection_AddRow->setMaximumWidth(30);
        m_PeakSelection_AddRow->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_PeakSelection_AddRow->setIconSize(QSize(14, 14));
        m_PeakSelection_RemoveRow = new QPushButton(this);
        m_PeakSelection_RemoveRow->setMaximumWidth(30);
        m_PeakSelection_RemoveRow->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_PeakSelection_RemoveRow->setIconSize(QSize(14, 14));
        m_PeakSelection_RemoveRow->setEnabled(false);
        m_PeakSelection_ImportFile = new QPushButton("导入文件", this);
        m_PeakSelection_ExportFile = new QPushButton("导出文件", this);
        hLayout1->addWidget(labelTYPE);
        hLayout1->addWidget(m_comboBox_TYPE);
        hLayout1->addWidget(labelSCAlE);
        hLayout1->addWidget(m_comboBox_SCALE);
        hLayout1->addStretch();
        hLayout1->addWidget(m_PeakSelection_AddRow);
        hLayout1->addWidget(m_PeakSelection_RemoveRow);
        hLayout1->addWidget(m_PeakSelection_ImportFile);
        hLayout1->addWidget(m_PeakSelection_ExportFile);
        PeakSettingLayout->addLayout(hLayout1);

        m_PeakSelection_Table = new QTableWidget(this);
        m_PeakSelection_Table->setMinimumHeight(200);
        m_PeakSelection_Table->setColumnCount(3);
        m_PeakSelection_Table->setHorizontalHeaderLabels(QStringList() << "节点ID" << "CID" <<"CUTOFF");
        m_PeakSelection_Table->verticalHeader()->setVisible(false);
        m_PeakSelection_Table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_PeakSelection_Table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_PeakSelection_Table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_PeakSelection_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_PeakSelection_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_PeakSelection_Table->setShowGrid(false);
        m_PeakSelection_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_PeakSelection_Table->setStyleSheet(
            "QTableWidget {"
            "   selection-background-color: #D0E7FD;"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "   background - color: #ffffff;"
            "   border-radius: 2px;"          // 圆角半径，数值越大越圆滑
            "}"
            "QTableWidget::item {"
            "  border: none;"
            "  border-bottom: 1px solid #ccc;"
            "   padding-left: 8px;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
            "QTableWidget:disabled {"
            "   background-color: #F5F7FA;"
            "   alternate-background-color: #F5F7FA;"
            "   color: #808080;"
            "}"
            "QTableWidget:disabled QHeaderView::section { background-color: #E8EBF2; color: #A8ABB2; }"
        );
        PeakSettingLayout->addWidget(m_PeakSelection_Table, 1);
        PeakSelectionLayout->addWidget(m_PeakoutSettingContentWidget);
        m_PeakoutSettingContentWidget->setEnabled(false);
        pageLayout->addWidget(m_PeakSelectionContentWidget);
        m_PeakSelectionContentWidget->setVisible(false);

        // ---------- 折叠框 4：等效辐射功率（ERP）结果输出 ----------
        m_erpOutputButton = new QPushButton("等效辐射功率（ERP）结果输出", this);
        m_erpOutputButton->setIcon(drawOutputFoldIcon(false));
        m_erpOutputButton->setLayoutDirection(Qt::LeftToRight);
        m_erpOutputButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; padding-left:8px;"
            "  background:#ffffff; color:#000000; font-weight:bold; min-height:28px; }"
            "QPushButton:hover{background:#ffffff;}");
        connect(m_erpOutputButton, &QPushButton::clicked, this, [this]() {
            bool visible = m_erpOutputContentWidget->isVisible();
            m_erpOutputContentWidget->setVisible(!visible);
            m_erpOutputButton->setIcon(drawOutputFoldIcon(!visible));
            });
        pageLayout->addWidget(m_erpOutputButton);

        m_erpOutputContentWidget = new QWidget(this);
        QVBoxLayout* erpMainLayout = new QVBoxLayout(m_erpOutputContentWidget);
        erpMainLayout->setSpacing(10);
        erpMainLayout->setContentsMargins(10, 5, 10, 5);
        erpMainLayout->setAlignment(Qt::AlignTop);

        // ERP 参数网格
        QWidget* erpParamWidget = new QWidget(m_erpOutputContentWidget);
        m_erpOutputContentWidget->setMaximumHeight(160);
        QGridLayout* erpGrid = new QGridLayout(erpParamWidget);
        erpGrid->setSpacing(16);
        erpGrid->setContentsMargins(0, 0, 0, 0);
        erpGrid->setColumnStretch(1, 1);
        erpGrid->setColumnStretch(3, 1);

        m_OutputERP = new QCheckBox("ERP", this);
        m_OutputERP->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_comboBox_ERPCurveType = new QComboBox(this);
        m_comboBox_ERPCurveType->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );
        m_comboBox_ERPCurveType->addItems({ "ERP曲线输出","ERP云图输出","ERP曲线输出+ERP云图输出" });
        m_comboBox_ERPCurveType->setEnabled(false);
        m_edit_ERPC = new QLineEdit(this);
        m_edit_ERPC->setText("340000.0");
        m_edit_ERPRLF = new QLineEdit(this);
        m_edit_ERPRLF->setText("1");
        m_edit_ERPRHO = new QLineEdit(this);
        m_edit_ERPRHO->setText("1.225e-12");
        m_edit_RHOCP = new QLineEdit(this);
        m_edit_RHOCP->setText("1");
        m_edit_ERPREFDB = new QLineEdit(this);
        m_edit_ERPREFDB->setText("1e-9");

        erpGrid->addWidget(m_OutputERP, 0, 0);
        erpGrid->addWidget(m_comboBox_ERPCurveType, 0, 1);

        erpGrid->addWidget(new QLabel("ERPC", this), 1, 0);
        erpGrid->addWidget(m_edit_ERPC, 1, 1);
        erpGrid->addWidget(new QLabel("ERPRLF", this), 1, 2);
        erpGrid->addWidget(m_edit_ERPRLF, 1, 3);

        erpGrid->addWidget(new QLabel("ERPRHO", this), 2, 0);
        erpGrid->addWidget(m_edit_ERPRHO, 2, 1);
        erpGrid->addWidget(new QLabel("RHOCP", this), 2, 2);
        erpGrid->addWidget(m_edit_RHOCP, 2, 3);

        erpGrid->addWidget(new QLabel("ERPREFDB", this), 3, 0);
        erpGrid->addWidget(m_edit_ERPREFDB, 3, 1);

        for (int i = 1; i < erpGrid->count(); ++i) {
            QLayoutItem* item = erpGrid->itemAt(i);
            if (!item) continue;
            QWidget* widget = item->widget();
            if (widget) {
                widget->setEnabled(false);
            }
        }
        erpMainLayout->addWidget(erpParamWidget);
        pageLayout->addWidget(m_erpOutputContentWidget);
        m_erpOutputContentWidget->setVisible(false);

        // 勾选 ERP 时启用下方参数和下拉框
        connect(m_OutputERP, &QCheckBox::toggled, this, [this, erpGrid](bool checked) {
            m_comboBox_ERPCurveType->setEnabled(checked);
            for (int i = 1; i < erpGrid->count(); ++i) {
                QLayoutItem* item = erpGrid->itemAt(i);
                if (!item) continue;
                QWidget* widget = item->widget();
                if (widget) {
                    widget->setEnabled(checked);
                }
                onPageModified(6);
            }});

        // ---------- 折叠框 5：贡献量结果输出 ----------
        m_ContributionButton = new QPushButton("贡献量结果输出", this);
        m_ContributionButton->setIcon(drawOutputFoldIcon(false));
        m_ContributionButton->setLayoutDirection(Qt::LeftToRight);
        m_ContributionButton->setStyleSheet("QPushButton{"
            "  text-align:left; border:none; margin:0; padding-left:8px;"
            "  background:#ffffff; color:#000000; font-weight:bold; min-height:28px; }"
            "QPushButton:hover{background:#ffffff;}");
        connect(m_ContributionButton, &QPushButton::clicked, this, [this]() {
            bool visible = m_ContributionContentWidget->isVisible();
            m_ContributionContentWidget->setVisible(!visible);
            m_ContributionButton->setIcon(drawOutputFoldIcon(!visible));
            });
        pageLayout->addWidget(m_ContributionButton);

        m_ContributionContentWidget = new QWidget(this);
        m_ContributionContentWidget->setMaximumHeight(120);
        QHBoxLayout* contributionLayout = new QHBoxLayout(m_ContributionContentWidget);
        contributionLayout->setSpacing(15);
        contributionLayout->setContentsMargins(10, 5, 10, 5);
        contributionLayout->setAlignment(Qt::AlignTop);

        // 第一列模态贡献量设置
        QVBoxLayout* modalLayout = new QVBoxLayout();
        modalLayout->setSpacing(10);
        modalLayout->setContentsMargins(0, 0, 0, 0);
        modalLayout->setAlignment(Qt::AlignTop);
        QLabel* labelModal = new QLabel("模态贡献量设置：", this);
        m_ModalContribution1 = new QCheckBox("结构模态贡献量", this);
        m_ModalContribution1->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_ModalContribution2 = new QCheckBox("流体模态贡献量", this);
        m_ModalContribution2->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_ModalContribution3 = new QCheckBox("Peakout", this);
        m_ModalContribution3->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_ModalContribution3->setEnabled(false);
        modalLayout->addWidget(labelModal);
        modalLayout->addWidget(m_ModalContribution1);
        modalLayout->addWidget(m_ModalContribution2);
        modalLayout->addWidget(m_ModalContribution3);
        modalLayout->addStretch(1);

        // 竖线分割1
        QFrame* contributionvLine1 = new QFrame(this);
        contributionvLine1->setFrameShape(QFrame::VLine);
        contributionvLine1->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        contributionvLine1->setFixedWidth(1);
        contributionvLine1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // 第二列节点贡献量设置：
        QVBoxLayout* nodeLayout = new QVBoxLayout();
        nodeLayout->setSpacing(10);
        nodeLayout->setContentsMargins(0, 0, 0, 0);
        nodeLayout->setAlignment(Qt::AlignTop);
        QLabel* labelNode = new QLabel("节点贡献量设置：", this);
        m_NodeContribution1 = new QCheckBox("结构节点贡献量", this);
        m_NodeContribution1->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_NodeContribution2 = new QCheckBox("流体节点贡献量", this);
        m_NodeContribution2->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_NodeContribution3 = new QCheckBox("Peakout", this);
        m_NodeContribution3->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_NodeContribution3->setEnabled(false);
        nodeLayout->addWidget(labelNode);
        nodeLayout->addWidget(m_NodeContribution1);
        nodeLayout->addWidget(m_NodeContribution2);
        nodeLayout->addWidget(m_NodeContribution3);
        nodeLayout->addStretch(1);

        // 竖线分割2
        QFrame* contributionvLine2 = new QFrame(this);
        contributionvLine2->setFrameShape(QFrame::VLine);
        contributionvLine2->setStyleSheet("QFrame { background-color: #DCDFE6; border: none; }");
        contributionvLine2->setFixedWidth(1);
        contributionvLine2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        // 第三列面板贡献量设置
        QVBoxLayout* panelLayout = new QVBoxLayout();
        panelLayout->setSpacing(10);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setAlignment(Qt::AlignTop);
        QLabel* labelPanel = new QLabel("面板贡献量设置：", this);
        m_PanelContribution1 = new QCheckBox("面板贡献量", this);
        m_PanelContribution1->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_PanelContribution2 = new QCheckBox("Peakout", this);
        m_PanelContribution2->setStyleSheet("QCheckBox::indicator:unchecked{background: white;}");
        m_PanelContribution2->setEnabled(false);
        panelLayout->addWidget(labelPanel);
        panelLayout->addWidget(m_PanelContribution1);
        panelLayout->addWidget(m_PanelContribution2);
        panelLayout->addStretch(1);
        contributionLayout->addLayout(modalLayout);
        contributionLayout->addSpacing(50);
        contributionLayout->addWidget(contributionvLine1);
        contributionLayout->addLayout(nodeLayout);
        contributionLayout->addSpacing(50);
        contributionLayout->addWidget(contributionvLine2);
        contributionLayout->addLayout(panelLayout);
        contributionLayout->addStretch(1);
        pageLayout->addWidget(m_ContributionContentWidget);
        m_ContributionContentWidget->setVisible(false);

        scrollPage->setWidget(page);
        scrollPage->setWidgetResizable(true);
        page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        scrollPage->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        slot_onAddPeakSelectionRow();
        return scrollPage;
    }

    void mCaseImplicitNTFWindow::applySolverDependentState()
    {
        if (!m_comboBox_Solver)
        {
            return;
        }
        const QString solver = m_comboBox_Solver->currentText();
        const bool isAMS = (solver == "自动多重子结构");
        const bool isLanczos = (solver == "兰索斯");

        // 结构模态：最大频率、模态阶数
        if (m_lineEdit_FreqMax)
        {
            m_lineEdit_FreqMax->setEnabled(true);
        }
        if (m_lineEdit_ModeCount)
        {
            m_lineEdit_ModeCount->setEnabled(true);
        }

        // 流体模态：最大频率、模态阶数
        if (m_lineEdit_FluidFreqMax)
        {
            m_lineEdit_FluidFreqMax->setEnabled(true);
        }
        if (m_lineEdit_FluidModeCount)
        {
            m_lineEdit_FluidModeCount->setEnabled(true);
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

    void mCaseImplicitNTFWindow::slot_onSolverChanged()
    {
        applySolverDependentState();
    }

    // 输入框输入限制：数值框仅允许合法数值形态，整数框仅允许数字，非法输入直接置空
    void mCaseImplicitNTFWindow::installInputValidators()
    {
        QList<QLineEdit*> textEdits;
        QList<QLineEdit*> intEdits = { m_lineEdit_ModeCount, m_lineEdit_FluidModeCount, m_FREQ1_NDF, m_FREQ2_NF,
            m_FREQ3_NEF, m_FREQ4_NFM, m_ModalDamping_PointCount, m_FluidModalDamping_PointCount,
            m_constraint_PointCount, m_Incentive_PointCount, m_edit_SID, m_edit_NPEAK };

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

    void mCaseImplicitNTFWindow::initConnect()
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
        connect(m_lineEdit_FreqMax, &QLineEdit::editingFinished, this, [this]() {
            m_lineEdit_FluidFreqMax->setText(QString::number(m_lineEdit_FreqMax->text().toDouble() * 2));
            onPageModified(0);
            });
        connect(m_lineEdit_ModeCount, &QLineEdit::editingFinished, this, [this]() {
            onPageModified(0);
            });
        connect(m_lineEdit_FluidFreqMax, &QLineEdit::editingFinished, this, [this]() {
            m_lineEdit_FreqMax->setText(QString::number(m_lineEdit_FluidFreqMax->text().toDouble() / 2));
            onPageModified(0);
            });
        connect(m_lineEdit_FluidModeCount, &QLineEdit::editingFinished, this, [this]() {
            onPageModified(0);
            });
        //声固耦合
        connect(m_checkBox_AcousticSolidCoupling, &QCheckBox::toggled, this, [=](bool checked) {
            if (ratioWidget)
            {
                ratioWidget->setEnabled(checked);
            }
            onPageModified(0);
            });
        connect(m_FREQ, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_widget_FREQ)
            {
                m_widget_FREQ->setEnabled(checked);
            }
            });
        connect(m_FREQ1, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_widget_FREQ1)
            {
                m_widget_FREQ1->setEnabled(checked);
            }
            });
        connect(m_FREQ2, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_widget_FREQ2)
            {
                m_widget_FREQ2->setEnabled(checked);
            }
            });
        connect(m_FREQ3, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_widget_FREQ3)
            {
                m_widget_FREQ3->setEnabled(checked);
            }
            });
        connect(m_FREQ4, &QCheckBox::toggled, this, [=](bool checked) {
            if (m_widget_FREQ4)
            {
                m_widget_FREQ4->setEnabled(checked);
            }
            });
        connect(m_pushButton_CreateMxCase, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onCreateMxCase);
        connect(m_pushButton_ExportFem, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onExportFem);
        connect(m_pushButton_Previous, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onPreviousStep);
        connect(m_pushButton_Next, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onNextStep);
        connect(m_Incentive_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddIncentiveRow();
            connectTableCheckBoxes(m_Incentive_NodeTable, 3);
            onPageModified(3);
            });
        if (m_Incentive_NodeTable && m_Incentive_NodeTable->selectionModel())
        {
            connect(m_Incentive_NodeTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this]() {
                    updateIncentiveDeleteButtonState();
                });
        }
        // 全局结构阻尼输入实时刷新阻尼页完成状态
        connect(m_editStructuralDamping, &QLineEdit::textChanged, this, [this]() { onPageModified(2); });
        // 全局流体阻尼输入实时刷新阻尼页完成状态
        connect(m_FluideditStructuralDamping, &QLineEdit::textChanged, this, [this]() { onPageModified(2); });
        connect(m_ModalDamping_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddModalDampingRow();
            onPageModified(2);
            });
        connect(m_FluidModalDamping_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddFluidModalDampingRow();
            onPageModified(2);
            });
        connect(m_ModalDamping_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_onImportDampingFile();
            });
        connect(m_ModalDamping_ExportFile, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onExportDampingFile);
        connect(m_FluidModalDamping_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_onImportFluidDampingFile();
            });
        connect(m_FluidModalDamping_ExportFile, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onExportFluidDampingFile);
        connect(m_constraint_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddConstraintRow();
            connectTableCheckBoxes(m_constraint_NodeTable, 4);
            onPageModified(4);
            });
        connect(m_constraint_PointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onConstraintPointCountChanged();
            connectTableCheckBoxes(m_constraint_NodeTable, 4);
            onPageModified(4);
            });
        connect(m_constraint_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveConstraintRow();
            connectTableCheckBoxes(m_constraint_NodeTable, 4);
            onPageModified(4);
            });
        connect(m_constraint_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_onImportConstraintFile();
            connectTableCheckBoxes(m_constraint_NodeTable, 4);
            onPageModified(4);
            });
        connect(m_constraint_ExportFile, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onExportConstraintFile);
        connect(m_Incentive_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_ImportIncentiveFromFile();
            connectTableCheckBoxes(m_Incentive_NodeTable, 3);
            onPageModified(3);
            });
        connect(m_Incentive_ExportFile, &QPushButton::clicked, this, &mCaseImplicitNTFWindow::slot_onExportIncentiveFile);
        if (m_constraint_NodeTable && m_constraint_NodeTable->selectionModel())
        {
            connect(m_constraint_NodeTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, [this]() {
                    updateConstraintDeleteButtonState();
                });
        }
        if (m_Incentive_NodeTable)
        {
            connect(m_Incentive_NodeTable, &IncentiveNodeTableWidget::itemChanged,
                this, [this](QTableWidgetItem*){
                    onPageModified(3);
                });
        }
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
                QSignalBlocker blocker(m_listWidget_Nav);
                m_listWidget_Nav->setCurrentRow(currentRow);
            }
            });
        connect(m_StructuralDamping, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_modelDampingWidget->setEnabled(false);
                m_editStructuralDamping->setEnabled(true);
            }
            onPageModified(2);
            });
        connect(m_ModalDamping, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_modelDampingWidget->setEnabled(true);
                m_editStructuralDamping->setEnabled(false);
            }
            onPageModified(2);
            });
        connect(m_FluidStructuralDamping, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_FluidmodelDampingWidget->setEnabled(false);
                m_FluideditStructuralDamping->setEnabled(true);
            }
            onPageModified(2);
            });
        connect(m_FluidModalDamping, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_FluidmodelDampingWidget->setEnabled(true);
                m_FluideditStructuralDamping->setEnabled(false);
            }
            onPageModified(2);
            });
        connect(m_checkBox_EnableDamping, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_dampingSettingsContainer)
            {
                m_dampingSettingsContainer->setEnabled(checked);
            }
            onPageModified(2);
            });
        connect(m_checkBox_FluidEnableDamping, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_FluiddampingSettingsContainer)
            {
                m_FluiddampingSettingsContainer->setEnabled(checked);
            }
            onPageModified(2);
            });
        connect(m_Incentive_RemoveRow, &QPushButton::clicked, this, [this]() {
            if (m_Incentive_NodeTable)
            {
                m_Incentive_NodeTable->removeSelectedRows();
                if (m_Incentive_PointCount)
                {
                    m_Incentive_PointCount->setText(QString::number(m_Incentive_NodeTable->rowCount()));
                }
            }
            connectTableCheckBoxes(m_Incentive_NodeTable, 3);
            onPageModified(3);
            });
        connect(m_ModalDamping_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveModalDampingRow();
            onPageModified(2);
            });
        connect(m_FluidModalDamping_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemoveFluidModalDampingRow();
            onPageModified(2);
            });
        connect(m_Incentive_PointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onIncentivePointCountChanged();
            connectTableCheckBoxes(m_Incentive_NodeTable, 3);
            onPageModified(3);
            });
        connect(m_ModalDamping_PointCount, &QLineEdit::editingFinished, this, [this]() {
            slot_onModalDampingPointCountChanged();
            });
        connect(m_ModalDamping_Table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
            if (!item)
            {
                return;
            }
            QString text = item->text().trimmed();
            if (text.isEmpty())
            {
                QSignalBlocker blocker(m_ModalDamping_Table);
                item->setText("请输入");
                item->setForeground(QBrush(QColor(169, 169, 169)));
            }
            else if (text != "请输入")
            {
                item->setForeground(QBrush(QColor(0, 0, 0)));
            }
            onPageModified(2);
            });
        connect(m_FluidModalDamping_Table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
            if (!item)
            {
                return;
            }
            QString text = item->text().trimmed();
            if (text.isEmpty())
            {
                QSignalBlocker blocker(m_FluidModalDamping_Table);
                item->setText("请输入");
                item->setForeground(QBrush(QColor(169, 169, 169)));
            }
            else if (text != "请输入")
            {
                item->setForeground(QBrush(QColor(0, 0, 0)));
            }
            onPageModified(2);
            });
        connect(m_ModalDamping_Table, &QTableWidget::itemSelectionChanged, this, &mCaseImplicitNTFWindow::updateModalDampingDeleteButtonState);
        connect(m_FluidModalDamping_Table, &QTableWidget::itemSelectionChanged, this, &mCaseImplicitNTFWindow::updateFluidModalDampingDeleteButtonState);
        connect(m_PeakSelection_Table, &QTableWidget::itemSelectionChanged, this, &mCaseImplicitNTFWindow::updatePeakSelectionDeleteButtonState);
        // 分析设置输入变化时刷新完成状态
        connect(m_lineEdit_FreqMax, &QLineEdit::textChanged, this, [this]() { onPageModified(0); });
        connect(m_lineEdit_ModeCount, &QLineEdit::textChanged, this, [this]() { onPageModified(0); });
        connect(m_lineEdit_FreqMin, &QLineEdit::textChanged, this, [this]() { onPageModified(0); });
        connect(m_lineEdit_ModeScale, &QLineEdit::textChanged, this, [this]() { onPageModified(0); });

        // 频率范围页面变化时刷新完成状态
        auto freqChanged = [this]() { onPageModified(1); };
        connect(m_FREQ, &QCheckBox::toggled, this, freqChanged);
        connect(m_FREQ1, &QCheckBox::toggled, this, freqChanged);
        connect(m_FREQ2, &QCheckBox::toggled, this, freqChanged);
        connect(m_FREQ3, &QCheckBox::toggled, this, freqChanged);
        connect(m_FREQ4, &QCheckBox::toggled, this, freqChanged);
        connect(m_FrepPoint1, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ1_F1, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ1_DF, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ1_NDF, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ2_F1, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ2_DF, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ2_NF, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ3_F1, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ3_F2, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ3_NEF, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ3_CLUSTER, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ4_F1, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ4_F2, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ4_FSPD, &QLineEdit::textChanged, this, freqChanged);
        connect(m_FREQ4_NFM, &QLineEdit::textChanged, this, freqChanged);

        // 输出设置复选框变化时刷新按钮状态
        connect(m_OutputDisplacement, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_OutputSpeed, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_Outputaccelerated, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_OutputNodesForce, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_OutputElementForce, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_OutputConstraintReaction, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_IncentiveDisplacement, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_IncentiveSpeed, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_Outputaccelerated, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_Incentiveaccelerated, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_OutputSoundPressure, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ODSDisplacement, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ODSStrain, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ODSPeckout, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ModalContribution1, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ModalContribution2, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_ModalContribution3, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_NodeContribution1, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_NodeContribution2, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_NodeContribution3, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_PanelContribution1, &QCheckBox::toggled, this, [this]() { onPageModified(6); });
        connect(m_PanelContribution2, &QCheckBox::toggled, this, [this]() { onPageModified(6); });

        // 初始时连接约束/激励表格中已有的复选框
        connectTableCheckBoxes(m_constraint_NodeTable, 4);
        connectTableCheckBoxes(m_Incentive_NodeTable, 3);

        // 响应设置五个面板数据变化时刷新完成状态
        if (m_SoundPressureResponse)
            connect(m_SoundPressureResponse, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_SoundPressureResponse->hasValidRow())
                    {
                        m_OutputSoundPressure->setEnabled(true);
                    }
                    else
                    {
                        m_OutputSoundPressure->setEnabled(false);
                    }
                    onPageModified(5);
                });
        if (m_VibrationResponse)
            connect(m_VibrationResponse, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_VibrationResponse->hasValidRow())
                    {
                        m_OutputDisplacement->setEnabled(true);
                        m_OutputSpeed->setEnabled(true);
                        m_Outputaccelerated->setEnabled(true);
                    }
                    else
                    {
                        m_OutputDisplacement->setEnabled(false);
                        m_OutputSpeed->setEnabled(false);
                        m_Outputaccelerated->setEnabled(false);
                    }
                    onPageModified(5);
                });
        if (m_NodalForces)
            connect(m_NodalForces, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_NodalForces->hasValidRow())
                        m_OutputNodesForce->setEnabled(true);
                    else
                        m_OutputNodesForce->setEnabled(false);
                    onPageModified(5);
                });
        if (m_ElementForces)
            connect(m_ElementForces, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_ElementForces->hasValidRow())
                        m_OutputElementForce->setEnabled(true);
                    else
                        m_OutputElementForce->setEnabled(false);
                    onPageModified(5);
                });
        if (m_ConstraintReaction)
            connect(m_ConstraintReaction, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_ConstraintReaction->hasValidRow())
                        m_OutputConstraintReaction->setEnabled(true);
                    else
                        m_OutputConstraintReaction->setEnabled(false);
                    onPageModified(5);
                });
        connect(m_PeakoutSetting, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) 
                m_PeakoutSettingContentWidget->setEnabled(true);
            else
                m_PeakoutSettingContentWidget->setEnabled(false);
            onPageModified(6);
            });
        connect(m_PeakSelection_AddRow, &QPushButton::clicked, this, [this]() {
            slot_onAddPeakSelectionRow();
            onPageModified(6);
            });
        connect(m_PeakSelection_RemoveRow, &QPushButton::clicked, this, [this]() {
            slot_onRemovePeakSelectionRow();
            onPageModified(6);
            });
        connect(m_PeakSelection_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_onImportPeakSelectionFile();
            });
        connect(m_PeakSelection_ExportFile, &QPushButton::clicked, this, [this]() {
            slot_onExportPeakSelectionFile();
            });
    }

    /*
    * 构造完成后快照控件默认状态（表格内部单元格控件不参与，随表格整体重置）
    */
    void mCaseImplicitNTFWindow::captureDefaultState()
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
    void mCaseImplicitNTFWindow::resetToDefault()
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

        // 阻尼表：清空后按添加行逻辑重建（与“添加”按钮产生的行一致）
        if (m_ModalDamping_Table)
        {
            m_ModalDamping_Table->clearContents();
            m_ModalDamping_Table->setRowCount(0);
            const int n = m_defaultRowCount.value(m_ModalDamping_Table, 1);
            for (int i = 0; i < n; ++i)
            {
                slot_onAddModalDampingRow();
            }
        }

        // 流体阻尼表：清空后按添加行逻辑重建（与“添加”按钮产生的行一致）
        if (m_FluidModalDamping_Table)
        {
            m_FluidModalDamping_Table->clearContents();
            m_FluidModalDamping_Table->setRowCount(0);
            const int n = m_defaultRowCount.value(m_FluidModalDamping_Table, 1);
            for (int i = 0; i < n; ++i)
            {
                slot_onAddFluidModalDampingRow();
            }
        }

        // 峰值选择表：清空后按添加行逻辑重建（与“添加”按钮产生的行一致）
        if (m_PeakSelection_Table)
        {
            m_PeakSelection_Table->clearContents();
            m_PeakSelection_Table->setRowCount(0);
            const int n = m_defaultRowCount.value(m_PeakSelection_Table, 1);
            for (int i = 0; i < n; ++i)
            {
                slot_onAddPeakSelectionRow();
            }
        }

        // 其余无建行入口的表格：仅恢复默认行数（默认均为空行）
        for (auto it = m_defaultRowCount.begin(); it != m_defaultRowCount.end(); ++it)
        {
            QTableWidget* t = it.key();
            if (t == m_ModalDamping_Table || t == m_FluidModalDamping_Table || t == m_PeakSelection_Table)
            {
                continue;   // 已按添加行逻辑重建
            }
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

    void mCaseImplicitNTFWindow::closeEvent(QCloseEvent* event)
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

    void mCaseImplicitNTFWindow::slot_onClose()
    {
        this->close();
    }

    bool mCaseImplicitNTFWindow::isAnalysisSettingValid() const
    {
        if (!m_comboBox_Solver)
        {
            return false;
        }
        //特征值求解器选择“自动多重子结构“”时，结构模态和流体模态最大频率为必选
        if (m_comboBox_Solver->currentIndex() == 0)
        {
            bool strHasMaxFreq = isValidDouble(m_lineEdit_FreqMax);
            bool fluidHasMaxFreq = isValidDouble(m_lineEdit_FluidFreqMax);
            if (strHasMaxFreq && fluidHasMaxFreq)
            {
                return true;
            }
        }
        else
        {
            bool strHasMaxFreq = isValidDouble(m_lineEdit_FreqMax);
            bool strHasModeCount = isValidDouble(m_lineEdit_ModeCount);
            bool strFluidHasMaxFreq = isValidDouble(m_lineEdit_FluidFreqMax);
            bool strFluidHasModeCount = isValidDouble(m_lineEdit_FluidModeCount);
            bool structDamping = strHasMaxFreq || strHasModeCount;
            bool fluidDamping = strFluidHasMaxFreq || strFluidHasModeCount;
            return structDamping && fluidDamping;
        }
        return false;
    }

    bool mCaseImplicitNTFWindow::isFrequencyRangeValid() const
    {
        bool hasCheckedGroup = false;
        bool allCheckedValid = true;
        for (int i = 0; i < 5; ++i)
        {
            bool checked = false;
            if (i == 0) checked = m_FREQ && m_FREQ->isChecked();
            else if (i == 1) checked = m_FREQ1 && m_FREQ1->isChecked();
            else if (i == 2) checked = m_FREQ2 && m_FREQ2->isChecked();
            else if (i == 3) checked = m_FREQ3 && m_FREQ3->isChecked();
            else if (i == 4) checked = m_FREQ4 && m_FREQ4->isChecked();

            if (checked)
            {
                hasCheckedGroup = true;
                if (!isFrequencyGroupValid(i))
                {
                    allCheckedValid = false;
                }
            }
        }
        return hasCheckedGroup && allCheckedValid;
    }

    bool mCaseImplicitNTFWindow::isFrequencyGroupValid(int groupIndex) const
    {
        switch (groupIndex)
        {
        case 0:
            return isValidDouble(m_FrepPoint1);
        case 1:
            return isValidDouble(m_FREQ1_F1)
                && isValidDouble(m_FREQ1_DF)
                && isValidPositiveInt(m_FREQ1_NDF);
        case 2:
            return isValidDouble(m_FREQ2_F1)
                && isValidDouble(m_FREQ2_DF)
                && isValidPositiveInt(m_FREQ2_NF);
        case 3:
            return isValidDouble(m_FREQ3_F1)
                && isValidDouble(m_FREQ3_F2)
                && isValidPositiveInt(m_FREQ3_NEF)
                && isValidDouble(m_FREQ3_CLUSTER);
        case 4:
            return isValidDouble(m_FREQ4_F1)
                && isValidDouble(m_FREQ4_F2)
                && isValidDouble(m_FREQ4_FSPD)
                && isValidPositiveInt(m_FREQ4_NFM);
        }
        return false;
    }

    bool mCaseImplicitNTFWindow::isIncentiveValid() const
    {
        if (!m_Incentive_NodeTable)
        {
            return false;
        }

        bool hasValidRow = false;
        bool hasEmptyLabel = false;
        for (int i = 0; i < m_Incentive_NodeTable->rowCount(); ++i)
        {
            auto row = m_Incentive_NodeTable->getRowData(i);
            QString gridText = row.nodeId.trimmed();
            if (gridText.isEmpty() || gridText == "请输入")
            {
                continue;
            }
            bool ok = false;
            int id = gridText.toInt(&ok);
            if (ok && id > 0)
            {
                // 标签列必填：有效行的标签为空或占位符则本页校验不通过
                QString labelText = row.label.trimmed();
                if (labelText.isEmpty() || labelText == "请输入")
                {
                    hasEmptyLabel = true;
                }
                else
                {
                    hasValidRow = true;
                }
            }
        }
        m_IncentiveSpeed->setEnabled(hasValidRow && !hasEmptyLabel);
        m_Incentiveaccelerated->setEnabled(hasValidRow && !hasEmptyLabel);
        m_IncentiveDisplacement->setEnabled(hasValidRow && !hasEmptyLabel);
        return hasValidRow && !hasEmptyLabel;
    }

    bool mCaseImplicitNTFWindow::isDampingValid() const
    {
        if (!m_checkBox_EnableDamping->isChecked())
        {
            return true;
        }
        if (m_StructuralDamping && m_StructuralDamping->isChecked())
        {
            return isValidDouble(m_editStructuralDamping);
        }
        int count = 0;
        for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
        {
            QTableWidgetItem* freqItem = m_ModalDamping_Table->item(i, 0);
            QTableWidgetItem* dampItem = m_ModalDamping_Table->item(i, 1);
            if (!freqItem || !dampItem)
            {
                continue;
            }
            bool freqOk = false;
            bool dampOk = false;
            double freq = freqItem->text().trimmed().toDouble(&freqOk);
            double damp = dampItem->text().trimmed().toDouble(&dampOk);
            if (freqOk && dampOk)
            {
                count++;
            }
            if (count >= 2)
                return true;
        }
        return false;
    }

    bool mCaseImplicitNTFWindow::isFluidDampingValid() const
    {
        if (!m_checkBox_FluidEnableDamping->isChecked())
        {
            return true;
        }
        if (m_FluidStructuralDamping->isChecked())
        {
            return isValidDouble(m_FluideditStructuralDamping);
        }
        int count = 0;
        for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
        {
            QTableWidgetItem* freqItem = m_FluidModalDamping_Table->item(i, 0);
            QTableWidgetItem* dampItem = m_FluidModalDamping_Table->item(i, 1);
            if (!freqItem || !dampItem)
            {
                continue;
            }
            bool freqOk = false;
            bool dampOk = false;
            double freq = freqItem->text().trimmed().toDouble(&freqOk);
            double damp = dampItem->text().trimmed().toDouble(&dampOk);
            if (freqOk && dampOk)
            {
                count++;
            }
            if (count >= 2)
                return true;
        }
        return false;
    }

    bool mCaseImplicitNTFWindow::isConstraintValid() const
    {
        // 约束设置允许为空，始终视为有效
        return true;
    }

    bool mCaseImplicitNTFWindow::isResponseValid() const
    {
        return (m_SoundPressureResponse && m_SoundPressureResponse->hasValidRow())
            || (m_VibrationResponse && m_VibrationResponse->hasValidRow())
            || (m_NodalForces && m_NodalForces->hasValidRow())
            || (m_ElementForces && m_ElementForces->hasValidRow())
            || (m_ConstraintReaction && m_ConstraintReaction->hasValidRow());
    }

    bool mCaseImplicitNTFWindow::isOutputValid() const
    {
        if(m_OutputDisplacement->isChecked()) return true;
        if (m_OutputSpeed->isChecked()) return true;
        if (m_Outputaccelerated->isChecked()) return true;
        if (m_OutputNodesForce->isChecked()) return true;
        if (m_OutputElementForce->isChecked()) return true;
        if (m_OutputConstraintReaction->isChecked()) return true;
        if (m_IncentiveDisplacement->isChecked()) return true;
        if (m_IncentiveSpeed->isChecked()) return true;
        if (m_Incentiveaccelerated->isChecked()) return true;
        if (m_OutputSoundPressure->isChecked()) return true;
        if (m_ODSDisplacement->isChecked()) return true;
        if (m_ODSStrain->isChecked()) return true;
        if (m_ODSPeckout->isChecked()) return true;
        if (m_PeakoutSetting->isChecked() && !m_edit_SID->text().isEmpty() && !m_edit_NPEAK->text().isEmpty()
            && !m_edit_NEAR->text().isEmpty() && !m_edit_FAR->text().isEmpty() && !m_edit_LFREQ->text().isEmpty()
            && !m_edit_HFREQ->text().isEmpty() && m_PeakSelection_Table->rowCount() != 0)
            return true;
        if (m_OutputERP->isChecked() && !m_edit_ERPC->text().isEmpty() && !m_edit_ERPRLF->text().isEmpty()
            && !m_edit_ERPRHO->text().isEmpty() && !m_edit_RHOCP->text().isEmpty() && !m_edit_ERPREFDB->text().isEmpty())
            return true;
        if (m_ModalContribution1->isChecked()) return true;
        if (m_ModalContribution2->isChecked()) return true;
        if (m_ModalContribution3->isChecked()) return true;
        if (m_NodeContribution1->isChecked()) return true;
        if (m_NodeContribution2->isChecked()) return true;
        if (m_NodeContribution3->isChecked()) return true;
        if (m_PanelContribution1->isChecked()) return true;
        if (m_PanelContribution2->isChecked()) return true;
        return false;
    }

    void mCaseImplicitNTFWindow::updateValidationState()
    {
        if (!m_navIconDelegate || !m_stackWidget)
        {
            return;
        }

        m_navIconDelegate->setPageCompleted(0, m_farthestReachedIndex > 0 && isAnalysisSettingValid());
        m_navIconDelegate->setPageCompleted(1, m_farthestReachedIndex > 1 && isFrequencyRangeValid());
        m_navIconDelegate->setPageCompleted(2, m_farthestReachedIndex > 2 && isDampingValid() && isFluidDampingValid());
        m_navIconDelegate->setPageCompleted(3, m_farthestReachedIndex > 3 && isIncentiveValid());
        // 约束设置：通过“下一步”离开该页后打勾
        m_navIconDelegate->setPageCompleted(4, m_farthestReachedIndex > 4 && isConstraintValid());
        // 响应设置：四个响应表格中至少一个存在有效数据行即打勾
        m_navIconDelegate->setPageCompleted(5, m_farthestReachedIndex > 5 && isResponseValid());
        m_navIconDelegate->setPageCompleted(6, false);

        for (int i = 0; i < m_stackWidget->count(); ++i)
        {
            // 未解锁时仅当前进度页之前的步骤可点击；解锁后允许已到达过的步骤
            m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
        }

        refreshNavigationIcons();
        updateOutputActionButtons();
    }

    void mCaseImplicitNTFWindow::updateOutputActionButtons()
    {
        bool enabled = isAnalysisSettingValid() && isFrequencyRangeValid() && isDampingValid()
            && isIncentiveValid() && isResponseValid() && isOutputValid();
        if(m_pushButton_ExportFem)
            m_pushButton_ExportFem->setEnabled(enabled);
        if(m_pushButton_CreateMxCase)
            m_pushButton_CreateMxCase->setEnabled(enabled);
    }

    void mCaseImplicitNTFWindow::updateBottomButtons(int currentIndex)
    {
        if (!m_pushButton_Previous || !m_pushButton_Next || !m_stackWidget)
        {
            return;
        }

        const int lastIndex = m_stackWidget->count() - 1;

        if (currentIndex == 0)
        {
            m_pushButton_Previous->setVisible(false);
            m_pushButton_Next->setVisible(true);
            m_pushButton_ExportFem->setVisible(false);
            m_pushButton_CreateMxCase->setVisible(false);
        }
        else if (currentIndex == lastIndex)
        {
            m_pushButton_Previous->setVisible(false);
            m_pushButton_Next->setVisible(false);
            m_pushButton_ExportFem->setVisible(true);
            m_pushButton_CreateMxCase->setVisible(true);
            updateOutputActionButtons();
        }
        else
        {
            m_pushButton_Previous->setVisible(true);
            m_pushButton_Next->setVisible(true);
            m_pushButton_ExportFem->setVisible(false);
            m_pushButton_CreateMxCase->setVisible(false);
        }

        updateNextButtonText(currentIndex);
    }

    void mCaseImplicitNTFWindow::updateNextButtonText(int currentIndex)
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

    void mCaseImplicitNTFWindow::slot_onNextStep()
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

    void mCaseImplicitNTFWindow::slot_onPreviousStep()
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

            if (preIndex == 0)
            {
                applySolverDependentState();
            }

            // 回退查看保留此前数据，不重置最远到达索引与完成状态
        }
    }

    void mCaseImplicitNTFWindow::slot_onCreateMxCase()
    {
        auto* modelData = getCVM->getModelData();
        mMeshData* meshData = modelData->getMeshData();

        if (!meshData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("模型网格数据不存在，无法创建激励点"));
            return;
        }

        // 0. 前置校验：约束、激励点、响应设置中的节点/单元ID必须存在于网格数据中
        QSet<int> missingNodeIdSet;
        QSet<int> missingElementIdSet;

        auto checkNodeId = [&](int id) {
            if (!meshData->_nodedata.nodeidmap.contains(id))
                missingNodeIdSet.insert(id);
        };
        auto checkElementId = [&](int id) {
            if (!meshData->isExistElement(id))
                missingElementIdSet.insert(id);
        };

        // 激励点
        if (m_Incentive_NodeTable)
        {
            const auto& incentiveData = m_Incentive_NodeTable->getTableData();
            for (const auto& row : incentiveData)
            {
                QString gridText = row.nodeId.trimmed();
                if (gridText.isEmpty() || gridText == "请输入")
                    continue;
                bool ok = false;
                int id = gridText.toInt(&ok);
                if (!ok || id <= 0)
                    continue;
                checkNodeId(id);
            }
        }

        // 约束设置
        if (m_constraint_NodeTable)
        {
            for (int i = 0; i < m_constraint_NodeTable->rowCount(); ++i)
            {
                auto row = m_constraint_NodeTable->getRowData(i);
                QString gridText = row.nodeId.trimmed();
                if (gridText.isEmpty() || gridText == "请输入")
                    continue;
                bool ok = false;
                int id = gridText.toInt(&ok);
                if (!ok || id <= 0)
                    continue;
                checkNodeId(id);
            }
        }

        // 响应设置：节点力和单元力
        if (m_NodalForces)
        {
            for (const auto& pair : m_NodalForces->getTableIds())
                checkNodeId(pair.first);
        }
        if (m_ElementForces)
        {
            for (const auto& pair : m_ElementForces->getTableIds())
                checkElementId(pair.first);
        }

        // 峰值筛选
        if (m_PeakSelection_Table)
        {
            for (int i = 0; i < m_PeakSelection_Table->rowCount(); ++i)
            {
                auto item = m_PeakSelection_Table->itemAt(i, 0);
                QString gridText = item->text();
                if (gridText.isEmpty() || gridText == "请输入")
                    continue;
                bool ok = false;
                int id = gridText.toInt(&ok);
                if (!ok || id <= 0)
                    continue;
                checkNodeId(id);
            }
        }

        if (!missingNodeIdSet.isEmpty() || !missingElementIdSet.isEmpty())
        {
            QStringList missingItems;
            for (int id : missingNodeIdSet)
                missingItems.append(QString::number(id));
            for (int id : missingElementIdSet)
                missingItems.append(QString::number(id));
            QString msg = QString("以下节点id在界面网格数据中不存在：%1").arg(missingItems.join(","));
            QMessageBox::warning(this, QString("提示"), msg);
            return;
        }

        //创建声固耦合对搜索设置
        if (m_checkBox_AcousticSolidCoupling->isChecked())
        {
            auto searchData = getCVM->getModelData()->getGlobalParam()->getASCouplingSearchData();
            if (!searchData) searchData = getCVM->getModelData()->getGlobalParam()->createASCouplingSearchData();
            if (!m_lineEdit_NormalExtRatio->text().isEmpty())
            {
                searchData->setOutChecked(true);
                searchData->setOuterScale(m_lineEdit_NormalExtRatio->text());
            }
            if (!m_lineEdit_SknepsExtRatio->text().isEmpty())
            {
                searchData->setHorizontalChecked(true);
                searchData->setHorizontalScale(m_lineEdit_NormalExtRatio->text());
            }
        }

        //根据激励设置创建分析工况
        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> createIncentiveData =
            m_Incentive_NodeTable ? m_Incentive_NodeTable->getTableData() : QList<IncentiveNodeTableWidget::IncentiveNodeRowData>();
        for (int i = 0; i < createIncentiveData.size(); ++i)
        {
            const auto& row = createIncentiveData.at(i);
            QString gridText = row.nodeId.trimmed();
            if (gridText.isEmpty() || gridText == "请输入")
            {
                continue;
            }
            bool ok = false;
            int id = gridText.toInt(&ok);
            if (!ok || id <= 0)
            {
                continue;
            }
            QString labelText = row.label.trimmed();
            QString baseName = labelText + "_";
            auto addForce = [&](const QString& dof, bool isTorque) {
                QString name = baseName + dof;
                int forceLoadID = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
                shared_ptr<MechCaseData::mLoadForce> forceLoad = make_shared<MechCaseData::mLoadForce>(
                    forceLoadID, isTorque ? LOAD_IM_TORQUE : LOAD_IM_FORCE, name + "_1");
                getCVM->getModelData()->addPhysicalFieldP(forceLoad);
                forceLoad->way_ = 0;
                std::map<RenderPickResultData, QVector<QVariant>> nodeForceMap;
                QVector<QVariant> values(3);
                if (dof == "X" || dof == "RX")
                {
                    forceLoad->force_x = 1;
                    values[0] = 1;
                }
                else if (dof == "Y" || dof == "RY")
                {
                    forceLoad->force_y = 1;
                    values[1] = 1;
                }
                else
                {
                    forceLoad->force_z = 1;
                    values[2] = 1;
                }
                nodeForceMap[RenderPickResultData(id, -1, 0)] = std::move(values);
                forceLoad->nodeForceMap_ = nodeForceMap;

                int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                auto nodeSet = new mNode(setID, Node, name);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeSet);
                std::vector<RenderPickResultData> nodes;
                RenderPickResultData data;
                data.result_id = id;
                int partID = meshData->getPartIdsByNodeid(id)[0];
                data.part_id = partID;
                nodes.push_back(data);
                nodeSet->setNodeData(nodes);
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary, forceLoadID }, { Physical_Set, setID });
                createASCouplingAnalysis(name, forceLoadID, i);
                };

            if (row.x) addForce("X", false);
            if (row.y) addForce("Y", false);
            if (row.z) addForce("Z", false);
            if (row.rx) addForce("RX", true);
            if (row.ry) addForce("RY", true);
            if (row.rz) addForce("RZ", true);
        }
        if (m_OutputERP->isChecked())
            readERPData();
        mGlobalSignals::getInstance()->updateModelTree();
        this->accept();

        // 工况已创建，关闭时不再提示
        m_mxCaseCreated = true;
    }

    void mCaseImplicitNTFWindow::createASCouplingAnalysis(const QString& name, int forceLoadID, int i)
    {
        mMeshData* meshData = getCVM->getModelData()->getMeshData();

        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplASCoupling* caseData = new MechCaseData::mCaseImplASCoupling(caseID, ASCoupling, name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(caseData);
        //分析设置
        caseData->_modalSolver = m_comboBox_Solver->currentText();
        caseData->_structMinFrequency = m_lineEdit_FreqMin->text();
        caseData->_structMaxFrequency = m_lineEdit_FreqMax->text();
        caseData->_structOrder = m_lineEdit_ModeCount->text();
        caseData->_acousticsMinFrequency = m_lineEdit_FluidFreqMin->text();
        caseData->_acousticsMaxFrequency = m_lineEdit_FluidFreqMax->text();
        caseData->_acousticsOrder = m_lineEdit_FluidModeCount->text();
        if (m_checkBox_ModeScale->isChecked())
        {
            caseData->_structAmpffact = m_lineEdit_ModeScale->text();
        }

        //阻尼设置（结构阻尼）
        if (!m_checkBox_EnableDamping->isChecked())
        {
            // 用户跳过了结构阻尼设置，不写入结构阻尼数据
        }
        else if (m_StructuralDamping && m_StructuralDamping->isChecked())
        {
            caseData->_structDampingG = m_editStructuralDamping->text();
        }
        else if (m_ModalDamping_Table)
        {
            int funcID = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).createID();
            MechFunctionData::mFunctionBase* func = new MechFunctionData::mFunctionBase(funcID, name + "_1");
            getCVM->getModelData()->addPhysicalField<MechFunctionData::mFunctionBase>(func);
            func->setFunctionType(5);
            MechFunctionData::TableCol& fundata = func->table();
            fundata.extrapMode = 1;
            auto getRowValues = [this](int row, double& freq, double& damp)
                {
                    QTableWidgetItem* freqItem = m_ModalDamping_Table->item(row, 0);
                    QTableWidgetItem* dampItem = m_ModalDamping_Table->item(row, 1);
                    if (!freqItem || !dampItem)
                    {
                        return false;
                    }
                    QString freqText = freqItem->text().trimmed();
                    QString dampText = dampItem->text().trimmed();
                    if (freqText.isEmpty() || freqText == "请输入" || dampText.isEmpty() || dampText == "请输入")
                    {
                        return false;
                    }
                    bool freqOk = false;
                    bool dampOk = false;
                    freq = freqText.toDouble(&freqOk);
                    damp = dampText.toDouble(&dampOk);
                    return freqOk && dampOk;
                };
            if (m_ModalDamping_Type->currentIndex() == 0)
            {
                for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp / 2.0);   // 存CRIT：CRIT=G/2=阻尼/2
                }
            }
            else if (m_ModalDamping_Type->currentIndex() == 1)
            {
                for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp);   // 存CRIT：填入值即临界阻尼比
                }
            }
            else
            {
                for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp))
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / (2.0 * damp));   // 存CRIT：CRIT=1/(2Q)
                }
            }
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Case, caseID }, { Physical_Function, funcID });
        }

        //阻尼设置（流体阻尼）
        if (!m_checkBox_FluidEnableDamping->isChecked())
        {
            // 用户跳过了结构阻尼设置，不写入结构阻尼数据
        }
        else if (m_FluidStructuralDamping && m_FluidStructuralDamping->isChecked())
        {
            caseData->_acousticsDampingG = m_editStructuralDamping->text();
        }
        else if (m_FluidModalDamping_Table)
        {
            int funcID = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).createID();
            MechFunctionData::mFunctionBase* func = new MechFunctionData::mFunctionBase(funcID, name + "_3"); 
            getCVM->getModelData()->addPhysicalField<MechFunctionData::mFunctionBase>(func);
            func->setFunctionType(5);
            MechFunctionData::TableCol& fundata = func->table();
            fundata.extrapMode = 1;
            auto getRowValues = [this](int row, double& freq, double& damp)
                {
                    QTableWidgetItem* freqItem = m_FluidModalDamping_Table->item(row, 0);
                    QTableWidgetItem* dampItem = m_FluidModalDamping_Table->item(row, 1);
                    if (!freqItem || !dampItem)
                    {
                        return false;
                    }
                    QString freqText = freqItem->text().trimmed();
                    QString dampText = dampItem->text().trimmed();
                    if (freqText.isEmpty() || freqText == "请输入" || dampText.isEmpty() || dampText == "请输入")
                    {
                        return false;
                    }
                    bool freqOk = false;
                    bool dampOk = false;
                    freq = freqText.toDouble(&freqOk);
                    damp = dampText.toDouble(&dampOk);
                    return freqOk && dampOk;
                };
            if (m_FluidModalDamping_Type->currentIndex() == 0)
            {
                for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp / 2.0);   // 存CRIT：CRIT=G/2=阻尼/2
                }
            }
            else if (m_FluidModalDamping_Type->currentIndex() == 1)
            {
                for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp);   // 存CRIT：填入值即临界阻尼比
                }
            }
            else
            {
                for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(i, freq, damp))
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / (2.0 * damp));   // 存CRIT：CRIT=1/(2Q)
                }
            }
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Case, caseID }, { Physical_Function, funcID });
        }

        //响应设置：创建节点力/单元力集合并映射到当前频响分析
        auto createResponseSet = [&](ResponseSettingFoldablePanel* panel, bool isElement, const QString& setName) {
            if (!panel || !panel->hasValidRow())
                return;

            auto ids = panel->getTableIds();
            if (ids.isEmpty())
                return;

            int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
            if (isElement)
            {
                MechSetData::mElementSet* eleSet = new MechSetData::mElementSet(setID, Element, setName, false, true);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleSet);
                std::vector<RenderPickResultData> elements;
                for (const auto& pair : ids)
                {
                    int eleId = pair.first;
                    if (!meshData || !meshData->isExistElement(eleId))
                        continue;
                    RenderPickResultData data;
                    data.result_id = eleId;
                    data.part_id = meshData->getPartIdByEleId(eleId);
                    elements.push_back(data);
                }
                eleSet->setElementSetData(elements);
            }
            else
            {
                MechSetData::mNode* nodeSet = new MechSetData::mNode(setID, Node, setName, false, true);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(nodeSet);
                std::vector<RenderPickResultData> nodes;
                for (const auto& pair : ids)
                {
                    int nodeId = pair.first;
                    if (!meshData || !meshData->_nodedata.nodeidmap.contains(nodeId))
                        continue;
                    RenderPickResultData data;
                    data.result_id = nodeId;
                    data.part_id = meshData->getPartIdsByNodeid(nodeId)[0];
                    nodes.push_back(data);
                }
                nodeSet->setNodeData(nodes);
            }

            getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                { Physical_Case, caseID },
                { Physical_Set, setID },
                isElement ? MapRelation_ElforceSet : MapRelation_GpforceSet);
        };

        createResponseSet(m_NodalForces, false, "NodeForce_" + name);
        createResponseSet(m_ElementForces, true, "ElementForce_" + name);

        //频率点设置
        int freqID = createFrequencyPoint(name);
        caseData->checked = true;
        vector<QString> freqNames;
        getCVM->getModelData()->getMapRelationManager().appendTableMapRelation({ Physical_Case, caseID },
            { Physical_FreqSamplPoint, freqID }, i);

        //输出设置：先清空云图输出控制与PCH输出控制，再写入NVH中勾选的输出
        caseData->setOutputs({});
        caseData->setPchOutputs({});
        caseData->setSelectedSet({});
        caseData->setPchSelectedSet({});
        //响应点曲线结果输出：激励点动刚度结果输出的位移/速度/加速度，对应频响面板的PCH输出控制
        QHash<QString, QStringList> pchOutputs;
        if (m_IncentiveDisplacement->isChecked())
            pchOutputs["基本结果"].append("位移");
        if (m_IncentiveSpeed->isChecked())
            pchOutputs["基本结果"].append("速度");
        if (m_Incentiveaccelerated->isChecked())
            pchOutputs["基本结果"].append("加速度");

        //振动响应设置：有有效行时创建节点集合（与节点力/单元力集合同逻辑），供PCH输出应用
        QString vibrationSetName;
        if (m_VibrationResponse && m_VibrationResponse->hasValidRow())
        {
            std::vector<RenderPickResultData> nodes;
            mMeshData* vibMesh = getCVM->getModelData()->getMeshData();
            for (const auto& pair : m_VibrationResponse->getTableIds())
            {
                if (!vibMesh || !vibMesh->_nodedata.nodeidmap.contains(pair.first))
                    continue;
                RenderPickResultData data;
                data.result_id = pair.first;
                data.part_id = vibMesh->getPartIdsByNodeid(pair.first)[0];
                nodes.push_back(data);
            }
            if (!nodes.empty())
            {
                vibrationSetName = "Vibration_" + name;
                int vibSetID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
                MechSetData::mNode* vibSet = new MechSetData::mNode(vibSetID, Node, vibrationSetName, false, true);
                getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(vibSet);
                vibSet->setNodeData(nodes);
            }
        }

        //振动结果输出：勾选项并入PCH输出控制，并应用于振动响应节点集合
        QHash<QString, QHash<QString, QStringList>> pchSetData;
        if (!vibrationSetName.isEmpty())
        {
            if (m_OutputDisplacement && m_OutputDisplacement->isEnabled() && m_OutputDisplacement->isChecked())
            {
                pchOutputs["基本结果"].append("位移");
                pchSetData["基本结果"]["位移"].append(vibrationSetName);
            }
            if (m_OutputSpeed && m_OutputSpeed->isEnabled() && m_OutputSpeed->isChecked())
            {
                pchOutputs["基本结果"].append("速度");
                pchSetData["基本结果"]["速度"].append(vibrationSetName);
            }
            if (m_Outputaccelerated && m_Outputaccelerated->isEnabled() && m_Outputaccelerated->isChecked())
            {
                pchOutputs["基本结果"].append("加速度");
                pchSetData["基本结果"]["加速度"].append(vibrationSetName);
            }
        }
        caseData->setPchOutputs(pchOutputs);
        caseData->setPchSelectedSet(pchSetData);

        //全局云图结果输出：ODS设置目前仅位移有对应项（应变期/Peckout暂无，不写入）
        QHash<QString, QStringList> outputs;
        if (m_ODSDisplacement->isChecked())
            outputs["基本结果"].append("位移");
        caseData->setOutputs(outputs);
        // 输出文件类型与工况面板的输出文件类型对应
        caseData->setOutputType(m_comboBox_outputForm->currentText());

        //约束设置(位移/转角)
        std::map<RenderPickResultData, QVector<QVariant>> independentTable;
        int boundaryID = -1;
        for (int i = 0; i < m_constraint_NodeTable->rowCount(); ++i)
        {
            auto row = m_constraint_NodeTable->getRowData(i);
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
            boundaryID = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            std::shared_ptr<mBoundaryDisplacement> boundary = std::make_shared<mBoundaryDisplacement>(boundaryID, BOUNDARY_IM_DISPLACEMENT, name);
            boundary->_definitionMode = 1; // 独立赋值模式
            boundary->_independentTable = independentTable;
            getCVM->getModelData()->addPhysicalFieldP(boundary, caseID);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                { Physical_Case, caseID },
                { Physical_LoadBoundary, boundaryID });
        }

        //频响载荷
        int freqLoadID = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
        shared_ptr<MechCaseData::mLoadFreqdomain> freqLoadLoad = make_shared<MechCaseData::mLoadFreqdomain>(freqLoadID, LOAD_IM_FREQDOMAIN, name + "_2");
        getCVM->getModelData()->addPhysicalFieldP(freqLoadLoad, caseID);
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Case, caseID },
            { Physical_LoadBoundary, freqLoadID });
        freqLoadLoad->way_ = 0;

        //赋值曲线
        int funcID = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).createID();
        MechFunctionData::mFunctionBase* func = new MechFunctionData::mFunctionBase(funcID, name + "_2");
        getCVM->getModelData()->addPhysicalField<MechFunctionData::mFunctionBase>(func);
        func->setFunctionType(2);
        MechFunctionData::InterpCol& fundata = func->interp();
        fundata.extrapMode = 1;
        fundata.interpMode = 1;
        fundata.x.push_back(m_FREQ1_F1->text().toDouble());
        fundata.x.push_back(m_FREQ1_F1->text().toDouble() + m_FREQ1_NDF->text().toDouble());
        fundata.y.push_back(m_FREQ1_DF->text().toDouble());
        fundata.y.push_back(m_FREQ1_DF->text().toDouble());
        getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary ,freqLoadID }, { Physical_Function ,funcID });
        getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary ,freqLoadID }, { Physical_LoadBoundary ,forceLoadID });
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("创建 Mx 工况：%1").arg(name));
    }

    int mCaseImplicitNTFWindow::createFrequencyPoint(const QString& name)
    {
        int id = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).createID();
        MechCaseData::mFreqSamplPointBase* data = new MechCaseData::mFreqSamplPointBase(id, name);
        getCVM->getModelData()->addPhysicalField<MechCaseData::mFreqSamplPointBase>(data);

        // 同步5个频率分组的勾选状态
        data->_checked.clear();
        data->_checked.append(m_FREQ && m_FREQ->isChecked());
        data->_checked.append(m_FREQ1 && m_FREQ1->isChecked());
        data->_checked.append(m_FREQ2 && m_FREQ2->isChecked());
        data->_checked.append(m_FREQ3 && m_FREQ3->isChecked());
        data->_checked.append(m_FREQ4 && m_FREQ4->isChecked());

        // FREQ：单个频率点
        if (m_FREQ && m_FREQ->isChecked() && isValidDouble(m_FrepPoint1))
        {
            data->_freqSamplPoint.append(m_FrepPoint1->text().toDouble());
        }

        // FREQ1：线性分布 F1, DF, NDF
        if (m_FREQ1 && m_FREQ1->isChecked() &&
            isValidDouble(m_FREQ1_F1) && isValidDouble(m_FREQ1_DF) && isValidPositiveInt(m_FREQ1_NDF))
        {
            data->_freqSamplPoint1.startFreq = m_FREQ1_F1->text().toDouble();
            data->_freqSamplPoint1.stepFreq = m_FREQ1_DF->text().toDouble();
            data->_freqSamplPoint1.intervalNum = m_FREQ1_NDF->text().toInt();
        }

        // FREQ2：对数分布 F1, F2, NF
        if (m_FREQ2 && m_FREQ2->isChecked() &&
            isValidDouble(m_FREQ2_F1) && isValidDouble(m_FREQ2_DF) && isValidPositiveInt(m_FREQ2_NF))
        {
            data->_freqSamplPoint2.startFreq = m_FREQ2_F1->text().toDouble();
            data->_freqSamplPoint2.stepFreq = m_FREQ2_DF->text().toDouble();
            data->_freqSamplPoint2.totalPoints = m_FREQ2_NF->text().toInt();
        }

        // FREQ3：聚集分布 F1, F2, NEF, CLUSTER, 计算形式
        if (m_FREQ3 && m_FREQ3->isChecked() &&
            isValidDouble(m_FREQ3_F1) && isValidDouble(m_FREQ3_F2) &&
            isValidPositiveInt(m_FREQ3_NEF) && isValidDouble(m_FREQ3_CLUSTER))
        {
            data->_freqSamplPoint3.startFreq = m_FREQ3_F1->text().toDouble();
            data->_freqSamplPoint3.endFreq = m_FREQ3_F2->text().toDouble();
            data->_freqSamplPoint3.refineNum = m_FREQ3_NEF->text().toInt();
            data->_freqSamplPoint3.expandValue = m_FREQ3_CLUSTER->text().toDouble();
            data->_freqSamplPoint3.diffType = m_FREQ3_Form ? m_FREQ3_Form->currentIndex() : 0;
        }

        // FREQ4：模态频率子区间 F1, F2, FSPD, NFM
        if (m_FREQ4 && m_FREQ4->isChecked() &&
            isValidDouble(m_FREQ4_F1) && isValidDouble(m_FREQ4_F2) &&
            isValidDouble(m_FREQ4_FSPD) && isValidPositiveInt(m_FREQ4_NFM))
        {
            data->_freqSamplPoint4.lowerFreq = m_FREQ4_F1->text().toDouble();
            data->_freqSamplPoint4.stepFreq = m_FREQ4_F2->text().toDouble();
            data->_freqSamplPoint4.shiftValue = m_FREQ4_FSPD->text().toDouble();
            data->_freqSamplPoint4.sampleNum = m_FREQ4_NFM->text().toInt();
        }

        return id;
    }

    void mCaseImplicitNTFWindow::readERPData()
    {
        auto* modelData = getCVM->getModelData();

        if (modelData->getGlobalParam()->getERPData())
        {
            return;
        }

        mERPData* erpData = modelData->getGlobalParam()->createERPData();
        erpData->_erprho = m_edit_ERPRHO->text();
        erpData->_erpc = m_edit_ERPC->text();
        erpData->_rhocp = m_edit_RHOCP->text();
        erpData->_erprlf = m_edit_ERPRLF->text();
        erpData->_erprefdb = m_edit_ERPREFDB->text();
    }

    void mCaseImplicitNTFWindow::slot_onExportFem()
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

        const int setID_Incentive = 1001;

        const int setID_Vibration = 2;
        const int setID_NodalForce = 3;
        const int setID_ElementForce = 4;
        const int setID_ConstraintReaction = 5;
        const int setID_SoundPressure = 1;
        const int methodID = 10;
        const int fluidMethodID = 20;
        const int frequencyID = 30;
        const int sdampingID = 40;
        const int fluidDampingID = 60;
        const int tabled1ID = 50;

        struct IncentiveItem {
            int nodeId = 0;
            QString label;
            int dof = 0;
            QString dofName;
            int setId = 0;
        };
        QList<IncentiveItem> incentives;
        QList<QPair<int, int>> rowSets;
        if (m_Incentive_NodeTable)
        {
            for (int i = 0; i < m_Incentive_NodeTable->rowCount(); ++i)
            {
                auto row = m_Incentive_NodeTable->getRowData(i);
                QString gridText = row.nodeId.trimmed();
                if (gridText.isEmpty() || gridText == "请输入")
                {
                    continue;
                }
                bool ok = false;
                int nodeId = gridText.toInt(&ok);
                if (!ok || nodeId <= 0)
                {
                    continue;
                }
                QString label = row.label.trimmed();
                if (label.isEmpty() || label == "请输入")
                {
                    label = "Incentive";
                }
                // 一个激励点（表格行）一个节点集合，集合 ID 依次递增
                const int rowSetId = setID_Incentive + rowSets.size();

                int added = 0;
                auto add = [&](bool checked, int dof, const QString& name) {
                    if (checked)
                    {
                        IncentiveItem item;
                        item.nodeId = nodeId;
                        item.label = label;
                        item.dof = dof;
                        item.dofName = name;
                        item.setId = rowSetId;
                        incentives.append(item);
                        ++added;
                    }
                };
                add(row.x, 1, "X");
                add(row.y, 2, "Y");
                add(row.z, 3, "Z");
                add(row.rx, 4, "RX");
                add(row.ry, 5, "RY");
                add(row.rz, 6, "RZ");

                if (added > 0)
                {
                    rowSets.append(qMakePair(rowSetId, nodeId));
                }
            }
        }

        if (incentives.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("激励点设置为空，无法导出 fem 文件。"));
            file.close();
            return;
        }

        QList<QPair<int, QString>> vibrationIds = m_VibrationResponse ? m_VibrationResponse->getTableIds() : QList<QPair<int, QString>>();
        QList<QPair<int, QString>> nodalForceIds = m_NodalForces ? m_NodalForces->getTableIds() : QList<QPair<int, QString>>();
        QList<QPair<int, QString>> elementForceIds = m_ElementForces ? m_ElementForces->getTableIds() : QList<QPair<int, QString>>();
        QList<QPair<int, QString>> constraintReactionIds = m_ConstraintReaction ? m_ConstraintReaction->getTableIds() : QList<QPair<int, QString>>();
        QList<QPair<int, QString>> soundPressureIds = m_SoundPressureResponse ? m_SoundPressureResponse->getTableIds() : QList<QPair<int, QString>>();

        bool outputNodalForce = m_OutputNodesForce && m_OutputNodesForce->isChecked() && !nodalForceIds.isEmpty();
        bool outputElementForce = m_OutputElementForce && m_OutputElementForce->isChecked() && !elementForceIds.isEmpty();
        bool outputConstraintReaction = m_OutputConstraintReaction && m_OutputConstraintReaction->isChecked() && !constraintReactionIds.isEmpty();

        out << "$ Excitation NODES\n";
        for (const QPair<int, int>& rowSet : rowSets)
        {
            out << "SET " << rowSet.first << " = " << rowSet.second << "\n";
        }

        out << "$ CASE CONTROL COMMANDS\n";
        out << "  ANALYSIS MFREQ\n";
        out << "  METHOD(FLUID) = " << QString::number(fluidMethodID).rightJustified(8) << "\n";
        out << "  METHOD(STRUCTURE) = " << QString::number(methodID).rightJustified(8) << "\n";
        out << "  FREQUENCY = " << QString::number(frequencyID).rightJustified(8) << "\n";

        if (m_checkBox_EnableDamping && m_checkBox_EnableDamping->isChecked()
            && m_ModalDamping && m_ModalDamping->isChecked()
            && m_ModalDamping_Table && m_ModalDamping_Type)
        {
            out << "  SDAMPING(STRUCTURE) = " << QString::number(sdampingID).rightJustified(8) << "\n";
        }

        if (m_checkBox_FluidEnableDamping && m_checkBox_FluidEnableDamping->isChecked()
            && m_FluidModalDamping && m_FluidModalDamping->isChecked()
            && m_FluidModalDamping_Table && m_FluidModalDamping_Type)
        {
            out << "  SDAMPING(FLUID) = " << QString::number(fluidDampingID).rightJustified(8) << "\n";
        }

        if (m_PeakoutSetting && m_PeakoutSetting->isChecked())
        {
            out << "  PEAKOUT = " << (m_edit_SID ? m_edit_SID->text().trimmed() : QString("1")) << "\n";
        }

        if (m_OutputSoundPressure && m_OutputSoundPressure->isChecked() && !soundPressureIds.isEmpty())
        {
            out << "  DISPLACEMENT(SORT2,PUNCH,PHASE) = " << setID_SoundPressure << "\n";
        }

        QString vibrationSetStr = QString::number(setID_Vibration);
        if (m_Outputaccelerated && m_Outputaccelerated->isChecked() && !vibrationIds.isEmpty())
        {
            out << "  ACCELERATION(SORT2,PHASE,PUNCH)= " << vibrationSetStr << "\n";
        }
        if (m_OutputSpeed && m_OutputSpeed->isChecked() && !vibrationIds.isEmpty())
        {
            out << "  VELOCITY(SORT2,PHASE,PUNCH)= " << vibrationSetStr << "\n";
        }
        if (m_OutputDisplacement && m_OutputDisplacement->isChecked() && !vibrationIds.isEmpty())
        {
            out << "  DISPLACEMENT(SORT2,PHASE,PUNCH)= " << vibrationSetStr << "\n";
        }

        QString odsOption = (m_PeakoutSetting && m_PeakoutSetting->isChecked()) ? QString(",PEAKOUT") : QString();
        if (m_ODSDisplacement && m_ODSDisplacement->isChecked())
        {
            out << "  DISPLACEMENT(SORT1,H3D,PHASE" << odsOption << ") = ALL\n";
        }
        if (m_ODSStrain && m_ODSStrain->isChecked())
        {
            out << "  ESE(SORT1,H3D,PHASE" << odsOption << ") = ALL\n";
        }

        if (outputNodalForce)
        {
            out << "  GPFORCE(PUNCH,PHASE) = " << setID_NodalForce << "\n";
        }
        if (outputElementForce)
        {
            out << "  ELFORCE(PUNCH,PHASE) = " << setID_ElementForce << "\n";
        }
        if (outputConstraintReaction)
        {
            out << "  SPCFORCE(PUNCH,PHASE) = " << setID_ConstraintReaction << "\n";
        }
        if (m_OutputERP && m_OutputERP->isChecked())
        {
            QString erpType = m_comboBox_ERPCurveType ? m_comboBox_ERPCurveType->currentText() : QString();
            if (erpType.contains("曲线"))
            {
                out << "  ERP(PUNCH) = ALL\n";
            }
            if (erpType.contains("云图"))
            {
                out << "  ERP(H3D,GRID) = ALL\n";
            }
        }

        out << "$ SUBCASE List\n";
        for (int i = 0; i < incentives.size(); ++i)
        {
            const IncentiveItem& item = incentives.at(i);
            int subcaseID = 101 + i;
            int dloadID = 1001 + i;
            QString subcaseName = item.label + ":+" + item.dofName;
            out << "$HMNAME LOADSTEP" << QString::number(subcaseID).rightJustified(16)
                << "\"" << subcaseName << "\"\n";
            out << "SUBCASE " << QString::number(subcaseID).rightJustified(8) << "\n";
            out << "  LABEL " << subcaseName << "\n";
            out << "  DLOAD = " << QString::number(dloadID).rightJustified(8) << "\n";
            if (m_IncentiveDisplacement && m_IncentiveDisplacement->isChecked())
            {
                out << "  DISPLACEMENT(SORT2,PUNCH,PHASE) = " << item.setId << "\n";
            }
            if (m_IncentiveSpeed && m_IncentiveSpeed->isChecked())
            {
                out << "  VELOCITY(SORT2,PHASE,PUNCH) = " << item.setId << "\n";
            }
            if (m_Incentiveaccelerated && m_Incentiveaccelerated->isChecked())
            {
                out << "  ACCELERATION(SORT2,PUNCH,PHASE) = " << item.setId << "\n";
            }
        }

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

        // 响应节点/单元集合：$HMSET 元信息 + SET 卡（每条续行 8 个 ID）
        auto writeResponseSet = [&](int setId, const QString& type, const QString& name,
            const QList<QPair<int, QString>>& ids) {
            if (ids.isEmpty())
            {
                return;
            }
            QString idStr = QString::number(setId);
            out << "$HMSET" << idStr.rightJustified(9) << idStr.rightJustified(9)
                << " \"" << name << "\" 18\n";
            out << "$HMSETTYPE" << idStr.rightJustified(9) << " \"regular\" 18\n";
            writeBulkCard({ "SET", idStr, type, "LIST" });
            QStringList fields;
            fields << "+";
            for (const auto& p : ids)
            {
                fields << QString::number(p.first);
                if (fields.size() >= 9)
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
        };

        out << "$ Bulk Data Cards\n";
        out << "BEGIN BULK\n";
        writeResponseSet(setID_SoundPressure, "GRID", "MIC", soundPressureIds);
        writeResponseSet(setID_Vibration, "GRID", "Vibration", vibrationIds);
        writeResponseSet(setID_NodalForce, "GRID", "Gpforce", nodalForceIds);
        writeResponseSet(setID_ElementForce, "ELEM", "Elforce", elementForceIds);
        writeResponseSet(setID_ConstraintReaction, "GRID", "Spcforce", constraintReactionIds);

        out << "PARAM,AMSES,YES\n";
        out << "PARAM,AUTOSPC,YES\n";
        out << "PARAM,COUPMASS,YES\n";
        out << "PARAM,FASTFR,YES\n";
        if ((m_ODSDisplacement && m_ODSDisplacement->isChecked()) || (m_ODSStrain && m_ODSStrain->isChecked()))
        {
            out << "PARAM,ODS,YES\n";
        }

        if (m_checkBox_EnableDamping && m_checkBox_EnableDamping->isChecked()
            && m_StructuralDamping && m_StructuralDamping->isChecked()
            && m_editStructuralDamping)
        {
            QString gText = m_editStructuralDamping->text().trimmed();
            if (!gText.isEmpty())
            {
                out << "PARAM,G," << gText << "\n";
            }
        }

        // 声固耦合（ACMODL）：面单元外法线/横向延伸比例
        if (m_checkBox_AcousticSolidCoupling && m_checkBox_AcousticSolidCoupling->isChecked())
        {
            QString normalRatio = m_lineEdit_NormalExtRatio ? m_lineEdit_NormalExtRatio->text().trimmed() : QString();
            QString sknEpsRatio = m_lineEdit_SknepsExtRatio ? m_lineEdit_SknepsExtRatio->text().trimmed() : QString();
            if (!normalRatio.isEmpty() && !sknEpsRatio.isEmpty())
            {
                out << "$ Acoustic-structure coupling (ACMODL): NORMAL=" << normalRatio
                    << ", SKNEPS=" << sknEpsRatio << "\n";
                writeBulkCard({ "ACMODL", "", "", "", "", normalRatio, "", sknEpsRatio });
            }
        }

        // 峰值筛选（PEAKOUT）：主卡 SID/NPEAK/NEAR/FAR/LFREQ/HFREQ/TYPE/SCALE，续行 GRIDC + 节点表（每行 2 组）
        if (m_PeakoutSetting && m_PeakoutSetting->isChecked())
        {
            out << "$ Peak Selection (PEAKOUT)\n";
            writeBulkCard({ "PEAKOUT",
                m_edit_SID ? m_edit_SID->text().trimmed() : QString("1"),
                m_edit_NPEAK ? m_edit_NPEAK->text().trimmed() : QString("5"),
                m_edit_NEAR ? m_edit_NEAR->text().trimmed() : QString(),
                m_edit_FAR ? m_edit_FAR->text().trimmed() : QString(),
                m_edit_LFREQ ? m_edit_LFREQ->text().trimmed() : QString(),
                m_edit_HFREQ ? m_edit_HFREQ->text().trimmed() : QString(),
                m_comboBox_TYPE ? m_comboBox_TYPE->currentText() : QString("DISP"),
                m_comboBox_SCALE ? m_comboBox_SCALE->currentText() : QString("DB") });
            if (m_PeakSelection_Table)
            {
                QStringList fields;
                fields << "+" << "GRIDC";
                int gridCount = 0;
                for (int r = 0; r < m_PeakSelection_Table->rowCount(); ++r)
                {
                    QTableWidgetItem* nodeItem = m_PeakSelection_Table->item(r, 0);
                    QTableWidgetItem* cidItem = m_PeakSelection_Table->item(r, 1);
                    QTableWidgetItem* cutoffItem = m_PeakSelection_Table->item(r, 2);
                    if (!nodeItem || !cidItem || !cutoffItem)
                    {
                        continue;
                    }
                    QString nodeText = nodeItem->text().trimmed();
                    if (nodeText.isEmpty() || nodeText == "请输入")
                    {
                        continue;
                    }
                    fields << nodeText << cidItem->text().trimmed() << cutoffItem->text().trimmed();
                    ++gridCount;
                    if (gridCount % 2 == 0)
                    {
                        writeBulkCard(fields);
                        fields.clear();
                        fields << "+" << "GRIDC";
                    }
                }
                if (gridCount % 2 != 0)
                {
                    writeBulkCard(fields);
                }
            }
        }

        if (m_OutputERP && m_OutputERP->isChecked())
        {
            out << "PARAM,ERPC," << (m_edit_ERPC ? m_edit_ERPC->text().trimmed() : QString("340000.0")) << "\n";
            out << "PARAM,ERPRLF," << (m_edit_ERPRLF ? m_edit_ERPRLF->text().trimmed() : QString("1.0")) << "\n";
            out << "PARAM,ERPRHO," << (m_edit_ERPRHO ? m_edit_ERPRHO->text().trimmed() : QString("1.225e-12")) << "\n";
            out << "PARAM,RHOCP," << (m_edit_RHOCP ? m_edit_RHOCP->text().trimmed() : QString("1.0")) << "\n";
            out << "PARAM,ERPREFDB," << (m_edit_ERPREFDB ? m_edit_ERPREFDB->text().trimmed() : QString("1e-9")) << "\n";
        }

        out << "$ Loads and Boundary Conditions\n";
        for (int i = 0; i < incentives.size(); ++i)
        {
            const IncentiveItem& item = incentives.at(i);
            int dloadID = 1001 + i;
            int rload1ID = 2001 + i;
            int dareaID = 3001 + i;

            out << "$ Excitation " << item.label << ":+" << item.dofName
                << " (DLOAD " << dloadID << " / RLOAD1 " << rload1ID << " / DAREA " << dareaID << ")\n";
            writeBulkCard({ "DLOAD", QString::number(dloadID), "1.0", "1.0",
                QString::number(rload1ID) });
            writeBulkCard({ "RLOAD1", QString::number(rload1ID), QString::number(dareaID),
                "", "", QString::number(tabled1ID), "0" });
            writeBulkCard({ "DAREA", QString::number(dareaID), QString::number(item.nodeId),
                QString::number(item.dof), "1.0" });
        }

        out << "$ FREQUENCY LIST\n";
        if (m_FREQ1 && m_FREQ1->isChecked()
            && isValidDouble(m_FREQ1_F1) && isValidDouble(m_FREQ1_DF) && isValidPositiveInt(m_FREQ1_NDF))
        {
            out << "$HMNAME LOADCOL" << QString::number(frequencyID).rightJustified(17)
                << "\"FREQi_" << frequencyID << "\"\n";
            writeBulkCard({ "FREQ1", QString::number(frequencyID),
                m_FREQ1_F1->text().trimmed(), m_FREQ1_DF->text().trimmed(), m_FREQ1_NDF->text().trimmed() });
        }
        else if (m_FREQ && m_FREQ->isChecked() && isValidDouble(m_FrepPoint1))
        {
            out << "$HMNAME LOADCOL" << QString::number(frequencyID).rightJustified(17)
                << "\"FREQi_" << frequencyID << "\"\n";
            writeBulkCard({ "FREQ", QString::number(frequencyID), m_FrepPoint1->text().trimmed() });
        }

        out << "$ EIGENVALUE EXTRACTION (Structure)\n";
        QString solverText = m_comboBox_Solver ? m_comboBox_Solver->currentText() : QString("兰索斯");
        QString freqMin = m_lineEdit_FreqMin ? m_lineEdit_FreqMin->text().trimmed() : QString();
        QString freqMax = m_lineEdit_FreqMax ? m_lineEdit_FreqMax->text().trimmed() : QString();
        QString modeCount = m_lineEdit_ModeCount ? m_lineEdit_ModeCount->text().trimmed() : QString();
        bool okMin = false, okMax = false;
        double fMin = freqMin.toDouble(&okMin);
        double fMax = freqMax.toDouble(&okMax);
        if (!okMin) fMin = 0.0;
        if (!okMax) fMax = 0.0;
        const QString v1 = QString::number(fMin, 'f', 1);
        const QString v2 = QString::number(fMax, 'f', 1);
        const QString nd = modeCount.isEmpty() ? QString() : QString::number(modeCount.toInt());

        if (solverText == "兰索斯")
        {
            out << "$HMNAME LOADCOL" << QString::number(methodID).rightJustified(17)
                << "\"EIGRL\"\n";
            writeBulkCard({ "EIGRL", QString::number(methodID), v1, v2, nd });
        }
        else
        {
            QString modeScale = (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked() && m_lineEdit_ModeScale)
                ? m_lineEdit_ModeScale->text().trimmed() : QString();
            out << "$HMNAME LOADCOL" << QString::number(methodID).rightJustified(17)
                << "\"EIGRA\"\n";
            writeBulkCard({ "EIGRA", QString::number(methodID), v1, v2, nd,
                modeScale.isEmpty() ? QString() : modeScale, "MASS" });
        }

        // 流体模态提取（自动多重子结构 EIGRA）
        out << "$ EIGENVALUE EXTRACTION (Acoustic)\n";
        QString fluidFreqMin = m_lineEdit_FluidFreqMin ? m_lineEdit_FluidFreqMin->text().trimmed() : QString();
        QString fluidFreqMax = m_lineEdit_FluidFreqMax ? m_lineEdit_FluidFreqMax->text().trimmed() : QString();
        QString fluidModeCount = m_lineEdit_FluidModeCount ? m_lineEdit_FluidModeCount->text().trimmed() : QString();
        bool fokMin = false, fokMax = false;
        double ffMin = fluidFreqMin.toDouble(&fokMin);
        double ffMax = fluidFreqMax.toDouble(&fokMax);
        if (!fokMin) ffMin = 0.0;
        if (!fokMax) ffMax = 0.0;
        const QString fv1 = QString::number(ffMin, 'f', 1);
        const QString fv2 = QString::number(ffMax, 'f', 1);
        const QString fnd = fluidModeCount.isEmpty() ? QString() : QString::number(fluidModeCount.toInt());
        QString fluidAmp = (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked() && m_lineEdit_ModeScale)
            ? m_lineEdit_ModeScale->text().trimmed() : QString();
        out << "$HMNAME LOADCOL" << QString::number(fluidMethodID).rightJustified(17)
            << "\"EIGRA\"\n";
        writeBulkCard({ "EIGRA", QString::number(fluidMethodID), fv1, fv2, fnd,
            fluidAmp.isEmpty() ? QString() : fluidAmp, "MASS" });

        out << "$ TABULAR FUNCTIONS\n";
        if (m_checkBox_EnableDamping && m_checkBox_EnableDamping->isChecked()
            && m_ModalDamping && m_ModalDamping->isChecked()
            && m_ModalDamping_Table && m_ModalDamping_Type)
        {
            QString typeText = m_ModalDamping_Type->currentText();
            out << "$HMNAME LOADCOL" << QString::number(sdampingID).rightJustified(17)
                << "\"TABDMP1_" << sdampingID << "\"\n";
            writeBulkCard({ "TABDMP1", QString::number(sdampingID), typeText });
            QList<QPair<QString, QString>> dampPairs;
            for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
            {
                QTableWidgetItem* freqItem = m_ModalDamping_Table->item(i, 0);
                QTableWidgetItem* dampItem = m_ModalDamping_Table->item(i, 1);
                if (!freqItem || !dampItem)
                {
                    continue;
                }
                QString freqText = freqItem->text().trimmed();
                QString dampText = dampItem->text().trimmed();
                if (freqText.isEmpty() || freqText == "请输入" || dampText.isEmpty() || dampText == "请输入")
                {
                    continue;
                }
                bool freqOk = false, dampOk = false;
                freqText.toDouble(&freqOk);
                dampText.toDouble(&dampOk);
                if (!freqOk || !dampOk)
                {
                    continue;
                }
                dampPairs.append(qMakePair(freqText, dampText));
            }
            // 每条续行最多 4 对频率-阻尼（8 个字段），ENDT 紧跟最后一对
            for (int p = 0; p < dampPairs.size(); p += 4)
            {
                QStringList fields;
                fields << "+";
                for (int k = p; k < qMin(p + 4, dampPairs.size()); ++k)
                {
                    fields << dampPairs[k].first << dampPairs[k].second;
                }
                if (p + 4 >= dampPairs.size())
                {
                    fields << "ENDT";
                }
                writeBulkCard(fields);
            }
        }

        // 流体阻尼曲线（TABDMP1）
        if (m_checkBox_FluidEnableDamping && m_checkBox_FluidEnableDamping->isChecked()
            && m_FluidModalDamping && m_FluidModalDamping->isChecked()
            && m_FluidModalDamping_Table && m_FluidModalDamping_Type)
        {
            QString fluidTypeText = m_FluidModalDamping_Type->currentText();
            out << "$HMNAME LOADCOL" << QString::number(fluidDampingID).rightJustified(17)
                << "\"TABDMP1_" << fluidDampingID << "\"\n";
            writeBulkCard({ "TABDMP1", QString::number(fluidDampingID), fluidTypeText });
            QList<QPair<QString, QString>> fluidPairs;
            for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
            {
                QTableWidgetItem* freqItem = m_FluidModalDamping_Table->item(i, 0);
                QTableWidgetItem* dampItem = m_FluidModalDamping_Table->item(i, 1);
                if (!freqItem || !dampItem)
                {
                    continue;
                }
                QString freqText = freqItem->text().trimmed();
                QString dampText = dampItem->text().trimmed();
                if (freqText.isEmpty() || freqText == "请输入" || dampText.isEmpty() || dampText == "请输入")
                {
                    continue;
                }
                bool freqOk = false, dampOk = false;
                freqText.toDouble(&freqOk);
                dampText.toDouble(&dampOk);
                if (!freqOk || !dampOk)
                {
                    continue;
                }
                fluidPairs.append(qMakePair(freqText, dampText));
            }
            for (int p = 0; p < fluidPairs.size(); p += 4)
            {
                QStringList fields;
                fields << "+";
                for (int k = p; k < qMin(p + 4, fluidPairs.size()); ++k)
                {
                    fields << fluidPairs[k].first << fluidPairs[k].second;
                }
                if (p + 4 >= fluidPairs.size())
                {
                    fields << "ENDT";
                }
                writeBulkCard(fields);
            }
        }

        out << "$HMNAME LOADCOL" << QString::number(tabled1ID).rightJustified(17)
            << "\"TABLED1_" << tabled1ID << "\"\n";
        writeBulkCard({ "TABLED1", QString::number(tabled1ID), "LINEAR", "LINEAR" });
        writeBulkCard({ "+", "0.0", "1.0", "1000.0", "1.0", "ENDT" });

        if (m_constraint_NodeTable)
        {
            for (int i = 0; i < m_constraint_NodeTable->rowCount(); ++i)
            {
                auto row = m_constraint_NodeTable->getRowData(i);
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
                writeBulkCard({ "SPC", "1", QString::number(nodeId), dofCode, "0.0" });
            }
        }

        out << "ENDDATA\n";
        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出 fem 文件：%1").arg(fileName));
    }

    void mCaseImplicitNTFWindow::slot_onAddConstraintRow()
    {
        if (!m_constraint_NodeTable)
        {
            return;
        }
        m_constraint_NodeTable->addRow();
        m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onRemoveConstraintRow()
    {
        if (!m_constraint_NodeTable)
        {
            return;
        }
        m_constraint_NodeTable->removeSelectedRows();
        m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onRemoveModalDampingRow()
    {
        QModelIndexList selected = m_ModalDamping_Table->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;

        QList<int> rows;
        for (const QModelIndex& index : selected) {
            rows.append(index.row());
        }
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        for (int row : rows) {
            m_ModalDamping_Table->removeRow(row);
        }
        m_ModalDamping_PointCount->setText(QString::number(m_ModalDamping_Table->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onRemoveFluidModalDampingRow()
    {
        QModelIndexList selected = m_FluidModalDamping_Table->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;

        QList<int> rows;
        for (const QModelIndex& index : selected) {
            rows.append(index.row());
        }
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        for (int row : rows) {
            m_FluidModalDamping_Table->removeRow(row);
        }
        m_FluidModalDamping_PointCount->setText(QString::number(m_FluidModalDamping_Table->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onRemovePeakSelectionRow()
    {
        if (m_PeakSelection_Table->rowCount() == 1)
            return;
        QModelIndexList selected = m_PeakSelection_Table->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;

        QList<int> rows;
        for (const QModelIndex& index : selected) {
            rows.append(index.row());
        }
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        for (int row : rows) {
            m_PeakSelection_Table->removeRow(row);
        }
    }

    void mCaseImplicitNTFWindow::slot_onAddIncentiveRow()
    {
        if (!m_Incentive_NodeTable)
        {
            return;
        }
        m_Incentive_NodeTable->addRow();
        if (m_Incentive_PointCount)
        {
            m_Incentive_PointCount->setText(QString::number(m_Incentive_NodeTable->rowCount()));
        }
    }

    void mCaseImplicitNTFWindow::slot_onAddModalDampingRow()
    {
        int row = m_ModalDamping_Table->rowCount();
        m_ModalDamping_Table->insertRow(row);
        QTableWidgetItem* freqItem = new QTableWidgetItem("请输入");
        freqItem->setForeground(QBrush(QColor(169, 169, 169)));
        freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QTableWidgetItem* dampItem = new QTableWidgetItem("请输入");
        dampItem->setForeground(QBrush(QColor(169, 169, 169)));
        dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_ModalDamping_Table->setItem(row, 0, freqItem);
        m_ModalDamping_Table->setItem(row, 1, dampItem);
        m_ModalDamping_PointCount->setText(QString::number(m_ModalDamping_Table->rowCount()));
        m_ModalDamping_RemoveRow->setEnabled(true);
    }

    void mCaseImplicitNTFWindow::slot_onAddFluidModalDampingRow()
    {
        int row = m_FluidModalDamping_Table->rowCount();
        m_FluidModalDamping_Table->insertRow(row);
        QTableWidgetItem* freqItem = new QTableWidgetItem("请输入");
        freqItem->setForeground(QBrush(QColor(169, 169, 169)));
        freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QTableWidgetItem* dampItem = new QTableWidgetItem("请输入");
        dampItem->setForeground(QBrush(QColor(169, 169, 169)));
        dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_FluidModalDamping_Table->setItem(row, 0, freqItem);
        m_FluidModalDamping_Table->setItem(row, 1, dampItem);
        m_FluidModalDamping_PointCount->setText(QString::number(m_FluidModalDamping_Table->rowCount()));
        m_FluidModalDamping_RemoveRow->setEnabled(true);
    }

    void mCaseImplicitNTFWindow::slot_onAddPeakSelectionRow()
    {
        int row = m_PeakSelection_Table->rowCount();
        m_PeakSelection_Table->insertRow(row);
        QTableWidgetItem* nodeIDItem = new QTableWidgetItem("请输入");
        nodeIDItem->setForeground(QBrush(QColor(169, 169, 169)));
        nodeIDItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QTableWidgetItem* CIDItem = new QTableWidgetItem("请输入");
        CIDItem->setForeground(QBrush(QColor(169, 169, 169)));
        CIDItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QTableWidgetItem* CUTOFFItem = new QTableWidgetItem("请输入");
        CUTOFFItem->setForeground(QBrush(QColor(169, 169, 169)));
        CUTOFFItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_PeakSelection_Table->setItem(row, 0, nodeIDItem);
        m_PeakSelection_Table->setItem(row, 1, CIDItem);
        m_PeakSelection_Table->setItem(row, 2, CUTOFFItem);
        m_PeakSelection_RemoveRow->setEnabled(true);
        onPageModified(6);
    }

    void mCaseImplicitNTFWindow::connectTableCheckBoxes(IncentiveNodeTableWidget* table, int pageIndex)
    {
        if (!table)
        {
            return;
        }
        for (int row = 0; row < table->rowCount(); ++row)
        {
            for (int col = 1; col < 7; ++col)
            {
                if (QWidget* w = table->cellWidget(row, col))
                {
                    if (QCheckBox* cb = w->findChild<QCheckBox*>())
                    {
                        connect(cb, &QCheckBox::toggled, this, [this, pageIndex]() {
                            onPageModified(pageIndex);
                            });
                    }
                }
            }
        }
    }

    void mCaseImplicitNTFWindow::slot_ImportIncentiveFromFile()
    {
        if (!m_Incentive_NodeTable)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入激励点文件",
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

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            ++lineIndex;
            if (line.isEmpty())
            {
                continue;
            }

            QStringList parts = line.split(',');
            if (parts.size() != 8)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 8。").arg(lineIndex));
                file.close();
                return;
            }

            QString firstCol = parts[0].trimmed();
            if (firstCol.compare("节点ID", Qt::CaseInsensitive) == 0 ||
                firstCol.compare("nodeid", Qt::CaseInsensitive) == 0 ||
                firstCol.compare("id", Qt::CaseInsensitive) == 0)
            {
                continue;
            }

            bool idOk = false;
            int nodeId = firstCol.toInt(&idOk);
            if (!idOk || nodeId <= 0)
            {
                if (lineIndex == 1)
                {
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
            row.label = parts[7].trimmed();
            importedData.append(row);
        }

        file.close();

        if (importedData.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("导入失败：文件中没有有效数据。"));
            return;
        }

        m_Incentive_NodeTable->setTableData(importedData);
        if (m_Incentive_PointCount)
        {
            m_Incentive_PointCount->setText(QString::number(m_Incentive_NodeTable->rowCount()));
        }
        onPageModified(3);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条激励点数据。").arg(importedData.size()));
    }

    void mCaseImplicitNTFWindow::slot_onConstraintPointCountChanged()
    {
        if (!m_constraint_PointCount || !m_constraint_NodeTable)
        {
            return;
        }

        bool ok = false;
        int targetCount = m_constraint_PointCount->text().trimmed().toInt(&ok);
        if (!ok || targetCount < 0)
        {
            m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
            return;
        }

        m_constraint_NodeTable->setRowCount(targetCount);
        m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onIncentivePointCountChanged()
    {
        if (!m_Incentive_PointCount || !m_Incentive_NodeTable)
        {
            return;
        }

        bool ok = false;
        int targetCount = m_Incentive_PointCount->text().trimmed().toInt(&ok);
        if (!ok || targetCount < 0)
        {
            m_Incentive_PointCount->setText(QString::number(m_Incentive_NodeTable->rowCount()));
            return;
        }

        m_Incentive_NodeTable->setRowCount(targetCount);
        m_Incentive_PointCount->setText(QString::number(m_Incentive_NodeTable->rowCount()));
    }

    void mCaseImplicitNTFWindow::slot_onModalDampingPointCountChanged()
    {
        if (!m_ModalDamping_PointCount || !m_ModalDamping_Table)
        {
            return;
        }

        bool ok = false;
        int targetCount = m_ModalDamping_PointCount->text().trimmed().toInt(&ok);
        if (!ok || targetCount < 0)
        {
            m_ModalDamping_PointCount->setText(QString::number(m_ModalDamping_Table->rowCount()));
            return;
        }

        int currentCount = m_ModalDamping_Table->rowCount();
        if (targetCount == currentCount)
        {
            return;
        }

        // 批量调整行数时阻塞 lineEdit 信号，防止 slot_onAdd/RemoveModalDampingRow
        // 内部 setText 触发 textChanged/editingFinished 导致重入或多算一次。
        QSignalBlocker blocker(m_ModalDamping_PointCount);
        QSignalBlocker tableBlocker(m_ModalDamping_Table);
        int diff = targetCount - currentCount;
        if (diff > 0)
        {
            for (int i = 0; i < diff; ++i)
            {
                int row = m_ModalDamping_Table->rowCount();
                m_ModalDamping_Table->insertRow(row);
                QTableWidgetItem* freqItem = new QTableWidgetItem("请输入");
                freqItem->setForeground(QBrush(QColor(169, 169, 169)));
                freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                QTableWidgetItem* dampItem = new QTableWidgetItem("请输入");
                dampItem->setForeground(QBrush(QColor(169, 169, 169)));
                dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_ModalDamping_Table->setItem(row, 0, freqItem);
                m_ModalDamping_Table->setItem(row, 1, dampItem);
            }
        }
        else if (diff < 0)
        {
            for (int i = 0; i < -diff; ++i)
            {
                int lastRow = m_ModalDamping_Table->rowCount() - 1;
                if (lastRow >= 0)
                {
                    m_ModalDamping_Table->removeRow(lastRow);
                }
            }
        }
        m_ModalDamping_PointCount->setText(QString::number(m_ModalDamping_Table->rowCount()));
        m_ModalDamping_RemoveRow->setEnabled(m_ModalDamping_Table->rowCount() > 0);
        onPageModified(2);
    }
    
    void mCaseImplicitNTFWindow::slot_onFluidModalDampingPointCountChanged()
    {
        if (!m_FluidModalDamping_PointCount || !m_FluidModalDamping_Table)
        {
            return;
        }

        bool ok = false;
        int targetCount = m_FluidModalDamping_PointCount->text().trimmed().toInt(&ok);
        if (!ok || targetCount < 0)
        {
            m_FluidModalDamping_PointCount->setText(QString::number(m_FluidModalDamping_Table->rowCount()));
            return;
        }

        int currentCount = m_FluidModalDamping_Table->rowCount();
        if (targetCount == currentCount)
        {
            return;
        }

        // 批量调整行数时阻塞 lineEdit 信号，防止 slot_onAdd/RemoveModalDampingRow
        // 内部 setText 触发 textChanged/editingFinished 导致重入或多算一次。
        QSignalBlocker blocker(m_FluidModalDamping_PointCount);
        QSignalBlocker tableBlocker(m_FluidModalDamping_Table);
        int diff = targetCount - currentCount;
        if (diff > 0)
        {
            for (int i = 0; i < diff; ++i)
            {
                int row = m_FluidModalDamping_Table->rowCount();
                m_FluidModalDamping_Table->insertRow(row);
                QTableWidgetItem* freqItem = new QTableWidgetItem("请输入");
                freqItem->setForeground(QBrush(QColor(169, 169, 169)));
                freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                QTableWidgetItem* dampItem = new QTableWidgetItem("请输入");
                dampItem->setForeground(QBrush(QColor(169, 169, 169)));
                dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_FluidModalDamping_Table->setItem(row, 0, freqItem);
                m_FluidModalDamping_Table->setItem(row, 1, dampItem);
            }
        }
        else if (diff < 0)
        {
            for (int i = 0; i < -diff; ++i)
            {
                int lastRow = m_FluidModalDamping_Table->rowCount() - 1;
                if (lastRow >= 0)
                {
                    m_FluidModalDamping_Table->removeRow(lastRow);
                }
            }
        }
        m_FluidModalDamping_PointCount->setText(QString::number(m_FluidModalDamping_Table->rowCount()));
        m_FluidModalDamping_RemoveRow->setEnabled(m_FluidModalDamping_Table->rowCount() > 0);
        onPageModified(2);
    }

    void mCaseImplicitNTFWindow::slot_onImportConstraintFile()
    {
        if (!m_constraint_NodeTable)
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
                continue;
            }

            bool idOk = false;
            int nodeId = firstCol.toInt(&idOk);
            if (!idOk || nodeId <= 0)
            {
                if (lineIndex == 1)
                {
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

        m_constraint_NodeTable->setTableData(importedData);
        if (m_constraint_PointCount)
        {
            m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
        }
        onPageModified(4);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条约束数据。").arg(importedData.size()));
    }

    void mCaseImplicitNTFWindow::slot_onExportConstraintFile()
    {
        if (!m_constraint_NodeTable)
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

        out << "NodeID, X, Y, Z, RX, RY, RZ\n";

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> data = m_constraint_NodeTable->getTableData();
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
            QString("导出约束文件：%1").arg(fileName));
    }

    void mCaseImplicitNTFWindow::slot_onExportIncentiveFile()
    {
        if (!m_Incentive_NodeTable)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出激励点文件",
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

        out << "NodeID, X, Y, Z, RX, RY, RZ, Label\n";

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> data = m_Incentive_NodeTable->getTableData();
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

            QString labelText = row.label.trimmed();
            if (labelText == "请输入")
            {
                labelText.clear();
            }
            out << QString::number(id) << ", "
                << (row.x ? "1" : "0") << ", "
                << (row.y ? "1" : "0") << ", "
                << (row.z ? "1" : "0") << ", "
                << (row.rx ? "1" : "0") << ", "
                << (row.ry ? "1" : "0") << ", "
                << (row.rz ? "1" : "0") << ", "
                << labelText << "\n";
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出激励点文件：%1").arg(fileName));
    }

    void mCaseImplicitNTFWindow::slot_onImportDampingFile()
    {
        if (!m_ModalDamping_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入阻尼文件",
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

        QList<QPair<double, double>> importedData;
        int lineIndex = 0;

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            ++lineIndex;
            if (line.isEmpty())
            {
                continue;
            }

            QStringList parts = line.split(',');
            if (parts.size() != 2)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 2。").arg(lineIndex));
                file.close();
                return;
            }

            QString firstCol = parts[0].trimmed();
            if (firstCol.compare("frequency", Qt::CaseInsensitive) == 0)
            {
                continue;
            }

            bool freqOk = false;
            bool dampOk = false;
            double freq = firstCol.toDouble(&freqOk);
            double damp = parts[1].trimmed().toDouble(&dampOk);
            if (!freqOk || !dampOk)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行数据格式无效。").arg(lineIndex));
                file.close();
                return;
            }

            importedData.append(qMakePair(freq, damp));
        }

        file.close();

        if (importedData.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("导入失败：文件中没有有效数据。"));
            return;
        }

        // 切换到模态阻尼模式，确保表格可见
        if (m_ModalDamping && !m_ModalDamping->isChecked())
        {
            m_ModalDamping->setChecked(true);
        }

        // 清空并重新填充表格
        m_ModalDamping_Table->clearContents();
        m_ModalDamping_Table->setRowCount(0);
        {
            QSignalBlocker blocker(m_ModalDamping_Table);
            for (const QPair<double, double>& pair : importedData)
            {
                int row = m_ModalDamping_Table->rowCount();
                m_ModalDamping_Table->insertRow(row);
                QTableWidgetItem* freqItem = new QTableWidgetItem(QString::number(pair.first));
                freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                QTableWidgetItem* dampItem = new QTableWidgetItem(QString::number(pair.second));
                dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_ModalDamping_Table->setItem(row, 0, freqItem);
                m_ModalDamping_Table->setItem(row, 1, dampItem);
            }
        }
        if (m_ModalDamping_PointCount)
        {
            m_ModalDamping_PointCount->setText(QString::number(importedData.size()));
        }
        if (m_ModalDamping_RemoveRow)
        {
            m_ModalDamping_RemoveRow->setEnabled(importedData.size() > 0);
        }
        onPageModified(2);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条阻尼数据。").arg(importedData.size()));
    }

    void mCaseImplicitNTFWindow::slot_onExportDampingFile()
    {
        if (!m_ModalDamping_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出阻尼文件",
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

        out << "frequency, damping\n";

        int exportedCount = 0;
        for (int i = 0; i < m_ModalDamping_Table->rowCount(); ++i)
        {
            QTableWidgetItem* freqItem = m_ModalDamping_Table->item(i, 0);
            QTableWidgetItem* dampItem = m_ModalDamping_Table->item(i, 1);
            if (!freqItem || !dampItem)
            {
                continue;
            }
            QString freqText = freqItem->text().trimmed();
            QString dampText = dampItem->text().trimmed();
            if (freqText.isEmpty() || freqText == "请输入" ||
                dampText.isEmpty() || dampText == "请输入")
            {
                continue;
            }
            bool freqOk = false;
            bool dampOk = false;
            double freq = freqText.toDouble(&freqOk);
            double damp = dampText.toDouble(&dampOk);
            if (!freqOk || !dampOk)
            {
                continue;
            }

            out << QString::number(freq) << " ," << QString::number(damp) << "\n";
            ++exportedCount;
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出阻尼文件：%1，共 %2 条数据。").arg(fileName).arg(exportedCount));
    }

    void mCaseImplicitNTFWindow::slot_onImportFluidDampingFile()
    {
        if (!m_FluidModalDamping_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入流体阻尼文件",
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

        QList<QPair<double, double>> importedData;
        int lineIndex = 0;

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            ++lineIndex;
            if (line.isEmpty())
            {
                continue;
            }

            QStringList parts = line.split(',');
            if (parts.size() != 2)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 2。").arg(lineIndex));
                file.close();
                return;
            }

            QString firstCol = parts[0].trimmed();
            if (firstCol.compare("frequency", Qt::CaseInsensitive) == 0)
            {
                continue;
            }

            bool freqOk = false;
            bool dampOk = false;
            double freq = firstCol.toDouble(&freqOk);
            double damp = parts[1].trimmed().toDouble(&dampOk);
            if (!freqOk || !dampOk)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行数据格式无效。").arg(lineIndex));
                file.close();
                return;
            }

            importedData.append(qMakePair(freq, damp));
        }

        file.close();

        if (importedData.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("导入失败：文件中没有有效数据。"));
            return;
        }

        // 切换到模态阻尼模式，确保表格可见
        if (m_ModalDamping && !m_ModalDamping->isChecked())
        {
            m_ModalDamping->setChecked(true);
        }

        // 清空并重新填充表格
        m_FluidModalDamping_Table->clearContents();
        m_FluidModalDamping_Table->setRowCount(0);
        {
            QSignalBlocker blocker(m_FluidModalDamping_Table);
            for (const QPair<double, double>& pair : importedData)
            {
                int row = m_FluidModalDamping_Table->rowCount();
                m_FluidModalDamping_Table->insertRow(row);
                QTableWidgetItem* freqItem = new QTableWidgetItem(QString::number(pair.first));
                freqItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                QTableWidgetItem* dampItem = new QTableWidgetItem(QString::number(pair.second));
                dampItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_FluidModalDamping_Table->setItem(row, 0, freqItem);
                m_FluidModalDamping_Table->setItem(row, 1, dampItem);
            }
        }
        if (m_FluidModalDamping_PointCount)
        {
            m_FluidModalDamping_PointCount->setText(QString::number(importedData.size()));
        }
        if (m_FluidModalDamping_RemoveRow)
        {
            m_FluidModalDamping_RemoveRow->setEnabled(importedData.size() > 0);
        }
        onPageModified(2);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条阻尼数据。").arg(importedData.size()));
    }

    void mCaseImplicitNTFWindow::slot_onExportFluidDampingFile()
    {
        if (!m_FluidModalDamping_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出流体阻尼文件",
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

        out << "frequency, damping\n";

        int exportedCount = 0;
        for (int i = 0; i < m_FluidModalDamping_Table->rowCount(); ++i)
        {
            QTableWidgetItem* freqItem = m_FluidModalDamping_Table->item(i, 0);
            QTableWidgetItem* dampItem = m_FluidModalDamping_Table->item(i, 1);
            if (!freqItem || !dampItem)
            {
                continue;
            }
            QString freqText = freqItem->text().trimmed();
            QString dampText = dampItem->text().trimmed();
            if (freqText.isEmpty() || freqText == "请输入" ||
                dampText.isEmpty() || dampText == "请输入")
            {
                continue;
            }
            bool freqOk = false;
            bool dampOk = false;
            double freq = freqText.toDouble(&freqOk);
            double damp = dampText.toDouble(&dampOk);
            if (!freqOk || !dampOk)
            {
                continue;
            }

            out << QString::number(freq) << " ," << QString::number(damp) << "\n";
            ++exportedCount;
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出流体阻尼文件：%1，共 %2 条数据。").arg(fileName).arg(exportedCount));
    }

    void mCaseImplicitNTFWindow::slot_onImportPeakSelectionFile()
    {
        if (!m_PeakSelection_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入峰值筛选文件",
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

        QVector<QVector<int>> importedData;
        int lineIndex = 0;

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            ++lineIndex;
            if (line.isEmpty())
            {
                continue;
            }

            QStringList parts = line.split(',');
            if (parts.size() != 3)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行列数不等于 3。").arg(lineIndex));
                file.close();
                return;
            }

            QString firstCol = parts[0].trimmed();
            if (firstCol.compare("NodeID", Qt::CaseInsensitive) == 0)
            {
                continue;
            }

            bool nodeIDOk = false;
            bool CIDOk = false;
            bool CUTOFFOk = false;
            int nodeID = firstCol.toInt(&nodeIDOk);
            int CID = parts[1].trimmed().toInt(&CIDOk);
            int CUTOFF = parts[2].trimmed().toInt(&CUTOFFOk);
            if (!nodeIDOk || !CIDOk ||!CUTOFFOk)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    QString("导入失败：第 %1 行数据格式无效。").arg(lineIndex));
                file.close();
                return;
            }

            importedData.append({ nodeID, CID, CUTOFF });
        }

        file.close();

        if (importedData.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("导入失败：文件中没有有效数据。"));
            return;
        }

        // 清空并重新填充表格
        m_PeakSelection_Table->clearContents();
        m_PeakSelection_Table->setRowCount(0);
        {
            QSignalBlocker blocker(m_FluidModalDamping_Table);
            for (auto list : importedData)
            {
                int row = m_PeakSelection_Table->rowCount();
                m_PeakSelection_Table->insertRow(row);
                QTableWidgetItem* nodeIDItem = new QTableWidgetItem(QString::number(list[1]));
                QTableWidgetItem* CIDItem = new QTableWidgetItem(QString::number(list[2]));
                QTableWidgetItem* CUTOFFItem = new QTableWidgetItem(QString::number(list[3]));
                m_PeakSelection_Table->setItem(row, 0, nodeIDItem);
                m_PeakSelection_Table->setItem(row, 1, CIDItem);
                m_PeakSelection_Table->setItem(row, 2, CUTOFFItem);
            }
        }
        onPageModified(6);

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("成功导入 %1 条峰值筛选数据。").arg(importedData.size()));
    }

    void mCaseImplicitNTFWindow::slot_onExportPeakSelectionFile()
    {
        if (!m_PeakSelection_Table)
        {
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出峰值筛选文件",
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

        out << "NodeID, CID, CUTOFF\n";

        int exportedCount = 0;
        for (int i = 0; i < m_PeakSelection_Table->rowCount(); ++i)
        {
            QTableWidgetItem* nodeIDItem = m_PeakSelection_Table->item(i, 0);
            QTableWidgetItem* CIDItem = m_PeakSelection_Table->item(i, 1);
            QTableWidgetItem* CUTOFFItem = m_PeakSelection_Table->item(i, 2);
            if (!nodeIDItem || !CIDItem || !CIDItem)
            {
                continue;
            }
            QString nodeIDText = nodeIDItem->text().trimmed();
            QString CIDText = CIDItem->text().trimmed();
            QString CUTOFFText = CUTOFFItem->text().trimmed();
            if (nodeIDText.isEmpty() || nodeIDText == "请输入" ||
                CIDText.isEmpty() || CIDText == "请输入" ||
                CUTOFFText.isEmpty() || CUTOFFText == "请输入")
            {
                continue;
            }
            bool nodeIDOk = false;
            bool CIDOk = false;
            bool CUTOFFOk = false;
            int nodeID = nodeIDText.toInt(&nodeIDOk);
            int CID = CIDText.toInt(&CIDOk);
            int CUTOFF = CUTOFFText.toInt(&CUTOFFOk);
            if (!nodeIDOk || !CIDOk || !CUTOFFOk)
            {
                continue;
            }

            out << QString::number(nodeID) << " ," << QString::number(CID) << " ," << QString::number(CUTOFF) << "\n";
            ++exportedCount;
        }

        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出峰值筛选：%1，共 %2 条数据。").arg(fileName).arg(exportedCount));
    }

    void mCaseImplicitNTFWindow::updateConstraintDeleteButtonState()
    {
        bool hasSelected = m_constraint_NodeTable && m_constraint_NodeTable->selectedItems().count() > 0;
        if (m_constraint_RemoveRow)
        {
            m_constraint_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitNTFWindow::updateIncentiveDeleteButtonState()
    {
        bool hasSelected = m_Incentive_NodeTable && m_Incentive_NodeTable->selectedItems().count() > 0;
        if (m_Incentive_RemoveRow)
        {
            m_Incentive_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitNTFWindow::updateModalDampingDeleteButtonState()
    {
        bool hasSelected = m_ModalDamping_Table && m_ModalDamping_Table->selectedItems().count() > 0;
        if (m_ModalDamping_RemoveRow)
        {
            m_ModalDamping_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitNTFWindow::updateFluidModalDampingDeleteButtonState()
    {
        bool hasSelected = m_FluidModalDamping_Table->selectedItems().count() > 0;
        if (m_FluidModalDamping_RemoveRow)
        {
            m_FluidModalDamping_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitNTFWindow::updatePeakSelectionDeleteButtonState()
    {
        bool hasSelected = m_PeakSelection_Table->selectedItems().count() > 0;
        if (m_PeakSelection_RemoveRow)
        {
            m_PeakSelection_RemoveRow->setEnabled(hasSelected);
        }
    }
}
