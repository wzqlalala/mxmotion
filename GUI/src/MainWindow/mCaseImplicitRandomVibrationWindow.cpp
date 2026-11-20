#include "mCaseImplicitRandomVibrationWindow.h"
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
#include <QCursor>
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
#include "mCaseImplicitFrequency.h"
#include "mCaseImplicitRandomVibration.h"
#include "mFreqSamplPointBase.h"
#include "mLoadFreqdomain.h"
#include "mLoadForce.h"
using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MxFunctions;

namespace MechMainWindow
{
    // 功率谱矩阵 item 的自定义数据角色
    static const int SpectrumRoleRealFactor = Qt::UserRole + 1;   // 实部因子
    static const int SpectrumRoleRealCurve = Qt::UserRole + 2;    // 实部 PSD 曲线名
    static const int SpectrumRoleImagFactor = Qt::UserRole + 3;   // 虚部因子
    static const int SpectrumRoleImagCurve = Qt::UserRole + 4;    // 虚部 PSD 曲线名
    static const int SpectrumRoleApplied = Qt::UserRole + 5;      // 是否已应用
    static const int SpectrumRoleAppliedRealFactor = Qt::UserRole + 6;   // 已应用实部因子
    static const int SpectrumRoleAppliedRealCurve = Qt::UserRole + 7;    // 已应用实部曲线名
    static const int SpectrumRoleAppliedImagFactor = Qt::UserRole + 8;   // 已应用虚部因子
    static const int SpectrumRoleAppliedImagCurve = Qt::UserRole + 9;    // 已应用虚部曲线名
    // 已应用格子的背景色（绿色）
    static const QColor SpectrumAppliedColor(0xB3, 0xE9, 0xD7);

    mCaseImplicitRandomVibrationWindow::mCaseImplicitRandomVibrationWindow(QWidget* parent)
        : QDialog(parent)
    {
        setAttribute(Qt::WA_DontShowOnScreen, true);

        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        this->setWindowTitle("随机振动分析");
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

    mCaseImplicitRandomVibrationWindow::~mCaseImplicitRandomVibrationWindow()
    {
    }

    bool mCaseImplicitRandomVibrationWindow::eventFilter(QObject* watched, QEvent* event)
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

    bool mCaseImplicitRandomVibrationWindow::isNavigationIndexEnabled(int index) const
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

    void mCaseImplicitRandomVibrationWindow::updateFarthestReachedIndex(int index)
    {
        if (index > m_farthestReachedIndex)
        {
            m_farthestReachedIndex = index;
        }
    }

    void mCaseImplicitRandomVibrationWindow::resetSubsequentSteps(int fromIndex)
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

    void mCaseImplicitRandomVibrationWindow::onPageModified(int pageIndex)
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

    bool mCaseImplicitRandomVibrationWindow::isValidDouble(QLineEdit* lineEdit) const
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

    bool mCaseImplicitRandomVibrationWindow::isValidPositiveInt(QLineEdit* lineEdit) const
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

    void mCaseImplicitRandomVibrationWindow::initLeftNavigation()
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
        m_navIconDelegate->setOutputPageIndex(7);
        m_listWidget_Nav->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget_Nav->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_listWidget_Nav->setFixedHeight(48 * 8);

        QListWidgetItem* itemAnalysis = new QListWidgetItem("分析方法", m_listWidget_Nav);
        QListWidgetItem* itemFrequency = new QListWidgetItem("频率范围设置", m_listWidget_Nav);
        QListWidgetItem* itemDamping = new QListWidgetItem("阻尼设置", m_listWidget_Nav);
        QListWidgetItem* itemIncentive = new QListWidgetItem("激励点设置", m_listWidget_Nav);
        QListWidgetItem* itemConstraint = new QListWidgetItem("约束设置", m_listWidget_Nav);
        QListWidgetItem* itemCase = new QListWidgetItem("工况设置", m_listWidget_Nav);
        QListWidgetItem* itemResponse = new QListWidgetItem("响应设置", m_listWidget_Nav);
        QListWidgetItem* itemOutput = new QListWidgetItem("输出设置", m_listWidget_Nav);
        itemAnalysis->setData(Qt::UserRole, 0);
        itemFrequency->setData(Qt::UserRole, 1);
        itemDamping->setData(Qt::UserRole, 2);
        itemIncentive->setData(Qt::UserRole, 3);
        itemConstraint->setData(Qt::UserRole, 4);
        itemCase->setData(Qt::UserRole, 5);
        itemResponse->setData(Qt::UserRole, 6);
        itemOutput->setData(Qt::UserRole, 7);
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

    void mCaseImplicitRandomVibrationWindow::refreshNavigationIcons()
    {
        if (!m_listWidget_Nav || !m_navIconDelegate || !m_stackWidget)
        {
            return;
        }
        m_navIconDelegate->setCurrentPage(m_stackWidget->currentIndex());
        m_listWidget_Nav->viewport()->update();
    }

    void mCaseImplicitRandomVibrationWindow::initUI()
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
        m_stackWidget->addWidget(createPageCaseSetting());
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

    QWidget* mCaseImplicitRandomVibrationWindow::createPageAnalysisMethod()
    {
        QWidget* page = new QWidget(this);
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(20);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // ----频响方法----
        QFormLayout* topFormLayout = new QFormLayout();
        topFormLayout->setSpacing(10);
        topFormLayout->setLabelAlignment(Qt::AlignLeft);
        topFormLayout->setContentsMargins(0, 0, 0, 0);
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
            this, &mCaseImplicitRandomVibrationWindow::slot_onSolverChanged);
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
        formLayout1->setSpacing(16);
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
        vLayout2->setContentsMargins(0, 0, 0, 0);
        m_checkbox_FluidRangeSetting = new QCheckBox("流体模态提取范围设置：",this);
        m_FluidRangeSettingContainer = new QWidget(page);
        QFormLayout* formLayout2 = new QFormLayout(m_FluidRangeSettingContainer);
        formLayout2->setContentsMargins(0, 0, 0, 0);
        formLayout2->setSpacing(16);
        m_lineEdit_FluidFreqMin = new QLineEdit(this);
        m_lineEdit_FluidFreqMax = new QLineEdit(this);
        m_lineEdit_FluidModeCount = new QLineEdit(this);
        formLayout2->addRow("最小频率（Hz）", m_lineEdit_FluidFreqMin);
        formLayout2->addRow("最大频率（Hz）", m_lineEdit_FluidFreqMax);
        formLayout2->addRow("模态阶数", m_lineEdit_FluidModeCount);
        vLayout2->addWidget(m_checkbox_FluidRangeSetting);
        vLayout2->addWidget(m_FluidRangeSettingContainer);
        m_FluidRangeSettingContainer->setEnabled(false);
        rangeHBox->addLayout(vLayout2);
        pageLayout->addLayout(rangeHBox);

        // 分隔线 2
        QFrame* line2 = new QFrame(this);
        line2->setFrameShape(QFrame::HLine);
        line2->setStyleSheet("QFrame { border: none; background-color: #DCDFE6; max-height: 1px; }");
        pageLayout->addWidget(line2);

        // ---- 声固耦合 ----
        m_ASCoupling_Widget = new QWidget(this);
        QVBoxLayout* ASLayout = new QVBoxLayout(m_ASCoupling_Widget);
        ASLayout->setContentsMargins(0, 0, 0, 0);
        m_checkBox_AcousticSolidCoupling = new QCheckBox("声固耦合", this);
        m_checkBox_AcousticSolidCoupling->setChecked(false);
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

        ASLayout->addWidget(m_checkBox_AcousticSolidCoupling);
        ASLayout->addWidget(ratioWidget);
        ratioWidget->setEnabled(false);
        pageLayout->addWidget(m_ASCoupling_Widget);
        m_ASCoupling_Widget->setEnabled(false);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitRandomVibrationWindow::createPageFrequencyRange()
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

    QWidget* mCaseImplicitRandomVibrationWindow::createPageIncentivePoint()
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
        m_Incentive_PointCount->setFixedWidth(110);
        m_Incentive_PointCount->setText("1");

        QLabel* loadTypeLabel = new QLabel("节点个数：", this);
        m_LoadType = new QComboBox(this);
        m_LoadType->setFixedWidth(110);
        m_LoadType->addItems({ "力","位移","速度","加速度"});
        m_LoadType->setStyleSheet(
            "QComboBox QAbstractItemView { background-color: white; border: 1px solid #d0d0d0; outline: none; }"
            "QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px 8px; }"
        );

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
        topLayout->addWidget(loadTypeLabel);
        topLayout->addWidget(m_LoadType);
        topLayout->addWidget(m_Incentive_AddRow);
        topLayout->addWidget(m_Incentive_RemoveRow);
        topLayout->addStretch();
        topLayout->addWidget(m_Incentive_ImportFile);
        topLayout->addWidget(m_Incentive_ExportFile);

        pageLayout->addLayout(topLayout);

        m_Incentive_NodeTable = new IncentiveNodeTableWidget(this);
        m_Incentive_NodeTable->setShowLabelColumn(false);
        pageLayout->addWidget(m_Incentive_NodeTable, 1);

        return page;
    }

    QWidget* mCaseImplicitRandomVibrationWindow::createPageCaseSetting()
    {
        QWidget* page = new QWidget(this);
        QHBoxLayout* pageLayout = new QHBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout* vLayout1 = new QVBoxLayout(page);
        vLayout1->setSpacing(10);
        vLayout1->setContentsMargins(0, 0, 0, 0);
        QLabel* label1 = new QLabel("频响工况设置：", this);
        m_FrequencyResponse_Table = new QTableWidget(this);
        m_FrequencyResponse_Table->setColumnCount(4);
        m_FrequencyResponse_Table->setHorizontalHeaderLabels(QStringList() << "节点ID" << "载荷设置" << "DOF" << "");
        ButtonHeaderView* header = new ButtonHeaderView(Qt::Horizontal, m_FrequencyResponse_Table);
        m_FrequencyResponse_Table->setHorizontalHeader(header);
        connect(header, &ButtonHeaderView::buttonClicked, this, &mCaseImplicitRandomVibrationWindow::slot_editCaseCombination);
        // 设置第4列的文字和图标
        header->setText("工况组合");
        header->setButtonIcon(QIcon(":/mMainWidgets/Icon/NVH/bianij.png"));   // 替换为您的图标路径
        m_FrequencyResponse_Table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_FrequencyResponse_Table->setColumnWidth(0, 55);
        m_FrequencyResponse_Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        m_FrequencyResponse_Table->setColumnWidth(1, 65);
        m_FrequencyResponse_Table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        m_FrequencyResponse_Table->setColumnWidth(2, 100);
        m_FrequencyResponse_Table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        m_FrequencyResponse_Table->verticalHeader()->setVisible(false);
        m_FrequencyResponse_Table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_FrequencyResponse_Table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_FrequencyResponse_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_FrequencyResponse_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_FrequencyResponse_Table->setShowGrid(false);
        m_FrequencyResponse_Table->setAlternatingRowColors(true);
        m_FrequencyResponse_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_FrequencyResponse_Table->setStyleSheet(
            "QTableWidget {"
            "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
            "   background-color: #FFFFFF;"             /* 偶数行背景 */
            "   selection-background-color: #D0E7FD;"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "   border-radius: 2px;"
            "}"
            "QTableWidget::item {"
            "  border: none;"
            "  border-bottom: 1px solid #ccc;"
            "  padding-left: 8px;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
        );
        vLayout1->addWidget(label1);
        vLayout1->addWidget(m_FrequencyResponse_Table);
        pageLayout->addLayout(vLayout1,1);

        QVBoxLayout* vLayout2 = new QVBoxLayout(page);
        vLayout1->setContentsMargins(0, 0, 0, 0);
        vLayout2->setSpacing(10);
        QLabel* label2 = new QLabel("工况标签列表：", this);
        m_CaseLabel_Table = new QTableWidget(this);
        m_CaseLabel_Table->setFixedHeight(200);
        m_CaseLabel_Table->setColumnCount(2);
        m_CaseLabel_Table->setHorizontalHeaderLabels(QStringList() << "序号" << "标签" );
        m_CaseLabel_Table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        m_CaseLabel_Table->setColumnWidth(0, 70);
        m_CaseLabel_Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_CaseLabel_Table->verticalHeader()->setVisible(false);
        m_CaseLabel_Table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_CaseLabel_Table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_CaseLabel_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_CaseLabel_Table->setSelectionMode(QAbstractItemView::NoSelection);
        m_CaseLabel_Table->setShowGrid(false);
        m_CaseLabel_Table->setAlternatingRowColors(true);
        m_CaseLabel_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_CaseLabel_Table->setStyleSheet(
            "QTableWidget {"
            "   alternate-background-color: #F6F8FA;"   /* 奇数行背景 */
            "   background-color: #FFFFFF;"             /* 偶数行背景 */
            "   selection-background-color: #D0E7FD;"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "   border-radius: 2px;"
            "}"
            "QTableWidget::item {"
            "  border: none;"
            "  border-bottom: 1px solid #ccc;"
            "  padding-left: 8px;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
        );
        vLayout2->addWidget(label2);
        vLayout2->addWidget(m_CaseLabel_Table);

        QLabel* label3 = new QLabel("随机功率谱矩阵设置：", this);
        m_SpectrumMatrix_Table = new QTableWidget(this);
        m_SpectrumMatrix_Table->setFixedHeight(200);
        m_SpectrumMatrix_Table->verticalHeader()->setFixedWidth(48);
        m_SpectrumMatrix_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_SpectrumMatrix_Table->horizontalHeader()->setStyleSheet(
            "QHeaderView::section:horizontal {"
            "   padding-left: 10px;"
            "}"
        );
        m_SpectrumMatrix_Table->setStyleSheet(
            "QTableWidget {"
            "   gridline-color: #DCDFE6;"
            "   border: 1px solid #DCDFE6;"
            "   border-radius: 2px;"
            "}"
            "QTableWidget::item:selected {"
            "  background-color: #FFFFFF;"
            "  color: #606266;"
            "  border: 1px solid #1385F6;"
            "}"
            "QHeaderView::section { background-color: #E8EBF2; border: 1px solid #d0d0d0; padding: 4px; }"
        );
        vLayout2->addWidget(label3);
        vLayout2->addWidget(m_SpectrumMatrix_Table);

        m_RealWidgetContent = new QWidget(this);
        QHBoxLayout* hLayout1 = new QHBoxLayout(m_RealWidgetContent);
        hLayout1->setContentsMargins(0, 0, 0, 0);
        QLabel* label4 = new QLabel("实部因子", this);
        m_RealPartFactor = new QLineEdit(this);
        m_RealPartFactor->setFixedWidth(35);
        QLabel* label5 = new QLabel("PSD曲线", this);
        m_RealPSDCurve = new QComboBox(this);
        m_RealPSDCurve->addItem("无");
        ComboDeleteItemDelegate* realCurveDeleteDelegate = new ComboDeleteItemDelegate(m_RealPSDCurve);
        realCurveDeleteDelegate->setDeleteIcon(QIcon(":/mMainWidgets/Icon/mSectionWindow/shanchu@3x.png"));
        m_RealPSDCurve->view()->setItemDelegate(realCurveDeleteDelegate);
        connect(realCurveDeleteDelegate, &ComboDeleteItemDelegate::deleteRequested, this,
            [this](const QModelIndex& index) { removePsdCurve(m_RealPSDCurve, index.row()); });
        m_RealCurveEdit = new QPushButton(this);
        m_RealCurveEdit->setFixedSize(QSize(24, 24));
        m_RealCurveEdit->setIcon(QIcon(":/mMainWidgets/Icon/NVH/bianji_kedianji.svg"));
        m_RealCurveCreate = new QPushButton("创建", this);
        m_RealCurveCreate->setFixedWidth(45);
        hLayout1->addWidget(label4);
        hLayout1->addWidget(m_RealPartFactor);
        hLayout1->addWidget(label5);
        hLayout1->addWidget(m_RealPSDCurve,1);
        hLayout1->addWidget(m_RealCurveEdit);
        hLayout1->addWidget(m_RealCurveCreate);
        vLayout2->addWidget(m_RealWidgetContent);
        m_RealWidgetContent->setEnabled(false);

        m_ImaginaryWidgetContent = new QWidget(this);
        QHBoxLayout* hLayout2 = new QHBoxLayout(m_ImaginaryWidgetContent);
        hLayout2->setContentsMargins(0, 0, 0, 0);
        QLabel* label6 = new QLabel("虚部因子", this);
        m_ImaginaryPartFactor = new QLineEdit(this);
        m_ImaginaryPartFactor->setFixedWidth(35);
        QLabel* label7 = new QLabel("PSD曲线", this);
        m_ImaginaryPSDCurve = new QComboBox(this);
        m_ImaginaryPSDCurve->addItem("无");
        ComboDeleteItemDelegate* imagCurveDeleteDelegate = new ComboDeleteItemDelegate(m_ImaginaryPSDCurve);
        imagCurveDeleteDelegate->setDeleteIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_ImaginaryPSDCurve->view()->setItemDelegate(imagCurveDeleteDelegate);
        connect(imagCurveDeleteDelegate, &ComboDeleteItemDelegate::deleteRequested, this,
            [this](const QModelIndex& index) { removePsdCurve(m_ImaginaryPSDCurve, index.row()); });
        m_ImaginaryCurveEdit = new QPushButton(this);
        m_ImaginaryCurveEdit->setFixedSize(QSize(24, 24));
        m_ImaginaryCurveEdit->setIcon(QIcon(":/mMainWidgets/Icon/NVH/bianji_kedianji.svg"));
        m_ImaginaryCurveCreate = new QPushButton("创建", this);
        m_ImaginaryCurveCreate->setFixedWidth(45);

        hLayout2->addWidget(label6);
        hLayout2->addWidget(m_ImaginaryPartFactor);
        hLayout2->addWidget(label7);
        hLayout2->addWidget(m_ImaginaryPSDCurve,1);
        hLayout2->addWidget(m_ImaginaryCurveEdit);
        hLayout2->addWidget(m_ImaginaryCurveCreate);
        vLayout2->addWidget(m_ImaginaryWidgetContent);
        m_ImaginaryWidgetContent->setEnabled(false);
        QHBoxLayout* hLayout3 = new QHBoxLayout(this);
        hLayout3->setContentsMargins(0, 0, 0, 0);
        m_CaseClear = new QPushButton("清除", this);
        m_CaseClear->setFixedWidth(45);
        m_CaseApply = new QPushButton("应用", this);
        m_CaseApply->setFixedWidth(45);
        m_CaseApply->setEnabled(false);
        hLayout3->addStretch(1);
        hLayout3->addWidget(m_CaseClear);
        hLayout3->addWidget(m_CaseApply);
        vLayout2->addLayout(hLayout3);
        pageLayout->addSpacing(40);
        pageLayout->addLayout(vLayout2,1);

        return page;
    }

    QScrollArea* mCaseImplicitRandomVibrationWindow::createPageDamping()
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
        m_checkBox_FluidEnableDamping->setEnabled(false);
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

    QWidget* mCaseImplicitRandomVibrationWindow::createPageResponse()
    {
        QWidget* page = new QScrollArea(this);
        page->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollArea > QWidget > QWidget { background: transparent; }"   // 视口背景透明
        );
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setSpacing(10);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setAlignment(Qt::AlignTop);

        // 2. 创建并添加可折叠面板
        m_SoundPressureResponse = new ResponseSettingFoldablePanel("声压响应设置", this);
        m_VibrationResponse = new ResponseSettingFoldablePanel("振动响应设置", this);

        pageLayout->addWidget(m_SoundPressureResponse);
        m_SoundPressureResponse->SetContentEnable(false);
        pageLayout->addWidget(m_VibrationResponse);
        pageLayout->addStretch(1);

        return page;
    }

    QWidget* mCaseImplicitRandomVibrationWindow::createPageConstraint()
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

    QWidget* mCaseImplicitRandomVibrationWindow::createPageOutput()
    {
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

        // 第三列声压结果输出
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
        curveLayout->addLayout(soundLayout);
        curveLayout->addStretch(1);
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
        m_outputCloudContentWidget->setMaximumHeight(250);
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

        QLabel* physicalLabel = new QLabel("物理量设置：");
        m_PhysicalDisplacement = new QCheckBox("位移", this);
        m_PhysicalSpeed = new QCheckBox("速度", this);
        m_Physicalaccelerated = new QCheckBox("加速度", this);
        m_PhysicalStress = new QCheckBox("应力", this);
        cloudMainLayout->addWidget(physicalLabel);
        cloudMainLayout->addWidget(m_PhysicalDisplacement);
        cloudMainLayout->addWidget(m_PhysicalSpeed);
        cloudMainLayout->addWidget(m_Physicalaccelerated);
        cloudMainLayout->addWidget(m_PhysicalStress);
        pageLayout->addWidget(m_outputCloudContentWidget);

        return page;
    }

    void mCaseImplicitRandomVibrationWindow::applySolverDependentState()
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

    void mCaseImplicitRandomVibrationWindow::slot_onSolverChanged()
    {
        applySolverDependentState();
    }

    void mCaseImplicitRandomVibrationWindow::slot_onIncentiveSettingTableChanged()
    {
        if (!m_Incentive_NodeTable) return;
        m_FrequencyResponse_Table->setRowCount(0);

        QList<IncentiveNodeTableWidget::IncentiveNodeRowData> createIncentiveData = m_Incentive_NodeTable->getTableData();

        for (const auto& row : createIncentiveData) {
            QString gridText = row.nodeId.trimmed();
            if (gridText.isEmpty() || gridText == "请输入")
                continue;

            bool ok = false;
            int id = gridText.toInt(&ok);
            if (!ok || id <= 0)
                continue;

            // 为每个选中的自由度创建一行
            auto addItem = [&](const QString& dof) {
                int newRow = m_FrequencyResponse_Table->rowCount();  // 当前行数即新行索引
                m_FrequencyResponse_Table->insertRow(newRow);        // 在末尾插入

                QTableWidgetItem* itemID = new QTableWidgetItem(QString::number(id));
                itemID->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_FrequencyResponse_Table->setItem(newRow, 0, itemID);

                QTableWidgetItem* itemType = new QTableWidgetItem(m_LoadType->currentText());
                itemType->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_FrequencyResponse_Table->setItem(newRow, 1, itemType);

                QTableWidgetItem* itemDOF = new QTableWidgetItem(dof);
                itemDOF->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_FrequencyResponse_Table->setItem(newRow, 2, itemDOF);

                QComboBox* combo = new QComboBox();
                combo->addItems(m_caseCombinationList);
                combo->setFixedWidth(110);
                //combo->setFixedSize(QSize(110, 24));
                combo->setStyleSheet(
                    "QComboBox {"
                    "   background-color: white;"
                    "   border: 1px solid #d0d0d0;"
                    "   padding: 2px 5px;"
                    "}"
                    "QComboBox QAbstractItemView {"
                    "   background-color: white;"
                    "   border: 1px solid #d0d0d0;"
                    "   outline: none;"
                    "}"
                    "QComboBox QAbstractItemView::item {"
                    "   min-height: 24px;"
                    "   padding: 4px 8px;"
                    "}"
                );
                QWidget* container = new QWidget();
                container->setStyleSheet("background: transparent;");
                QHBoxLayout* layout = new QHBoxLayout(container);
                layout->setContentsMargins(0, 3, 0, 3);
                layout->addWidget(combo);
                m_FrequencyResponse_Table->setCellWidget(newRow, 3, container);

                // 工况组合选择变化时同步更新工况标签列表
                connect(combo, &QComboBox::currentTextChanged, this, [this](const QString&)
                {
                    updateCaseLabelTable();
                });
                };

            if (row.x) addItem("X");
            if (row.y) addItem("Y");
            if (row.z) addItem("Z");
            if (row.rx) addItem("RX");
            if (row.ry) addItem("RY");
            if (row.rz) addItem("RZ");
        }

        updateCaseLabelTable();
    }

    void mCaseImplicitRandomVibrationWindow::slot_editCaseCombination()
    {
        // Popup 样式：无标题栏，点击弹框外部自动关闭
        QDialog* dialog = new QDialog(this);
        dialog->setWindowFlags(Qt::Popup);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->resize(240, 200);
        dialog->setStyleSheet(
            "QDialog { background-color: #ffffff; }"
            "QWidget { background-color: #ffffff; }"
            "QPushButton { background-color: white; }"
            "QPushButton:disabled { background-color: #E4E7EE; color: #A8ABB2; }"
        );

        QTableWidget* tableWidget = new QTableWidget(dialog);
        tableWidget->setColumnCount(1);
        tableWidget->horizontalHeader()->setVisible(false);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        tableWidget->setShowGrid(false);
        tableWidget->setStyleSheet(
            "QTableWidget {"
            "   background-color: #FFFFFF;"
            "   selection-background-color: #D0E7FD;"
            "   selection-color: #606266;"
            "   border: 1px solid #DCDFE6;"
            "   border-radius: 2px;"
            "}"
            "QTableWidget::item {"
            "   min-height: 28px;"
            "   color: #606266;"
            "   border: none;"
            "   border-bottom: 1px solid #DCDFE6;"
            "}"
        );

        for (const QString& name : m_caseCombinationList)
        {
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            QTableWidgetItem* item = new QTableWidgetItem(name);
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            if (name == QStringLiteral("独立工况"))
            {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            tableWidget->setItem(row, 0, item);
        }
        tableWidget->setCurrentCell(0, 0);

        QPushButton* addButton = new QPushButton(dialog);
        addButton->setFixedSize(24, 24);
        addButton->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        addButton->setAutoDefault(false);
        QPushButton* removeButton = new QPushButton(dialog);
        removeButton->setFixedSize(24, 24);
        removeButton->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        removeButton->setAutoDefault(false);

        QVBoxLayout* buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(10);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(addButton);
        buttonLayout->addWidget(removeButton);
        buttonLayout->addStretch(1);

        QHBoxLayout* mainLayout = new QHBoxLayout(dialog);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->addWidget(tableWidget, 1);
        mainLayout->addLayout(buttonLayout);

        // 注意：show() 非阻塞，本函数立即返回，applyToList/generateName 是栈上对象，
        // 后续按钮/表格信号触发时栈帧已销毁，因此 connect 中必须按值捕获而不能按引用捕获
        auto applyToList = [this, tableWidget]()
        {
            m_caseCombinationList.clear();
            for (int i = 0; i < tableWidget->rowCount(); ++i)
            {
                if (QTableWidgetItem* item = tableWidget->item(i, 0))
                {
                    m_caseCombinationList.append(item->text().trimmed());
                }
            }
            syncCaseCombinationCombos();
        };

        // 生成不重复的默认名称
        auto generateName = [tableWidget](const QString& base)
        {
            auto nameExists = [tableWidget](const QString& name)
            {
                for (int i = 0; i < tableWidget->rowCount(); ++i)
                {
                    if (QTableWidgetItem* item = tableWidget->item(i, 0))
                    {
                        if (item->text() == name)
                        {
                            return true;
                        }
                    }
                }
                return false;
            };
            int suffix = 1;
            QString name;
            do
            {
                name = base + QString::number(suffix);
                ++suffix;
            } while (nameExists(name));
            return name;
        };

        // “独立工况”为固定项，仅当选中其它项时删除按钮可用
        auto updateRemoveState = [tableWidget, removeButton]()
        {
            removeButton->setEnabled(tableWidget->currentRow() > 0);
        };
        connect(tableWidget, &QTableWidget::currentCellChanged, dialog, [updateRemoveState](int)
        {
            updateRemoveState();
        });
        updateRemoveState();

        connect(addButton, &QPushButton::clicked, dialog, [tableWidget, applyToList, generateName]()
        {
            int row = tableWidget->rowCount();
            tableWidget->insertRow(row);
            QTableWidgetItem* item = new QTableWidgetItem(generateName(QStringLiteral("工况组合")));
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            tableWidget->setItem(row, 0, item);
            tableWidget->setCurrentCell(row, 0);
            tableWidget->editItem(item);
            applyToList();
        });

        connect(removeButton, &QPushButton::clicked, dialog, [tableWidget, applyToList]()
        {
            const int row = tableWidget->currentRow();
            if (row <= 0)
            {
                return;
            }
            tableWidget->removeRow(row);
            tableWidget->setCurrentCell(qMin(row, tableWidget->rowCount() - 1), 0);
            applyToList();
        });

        // 重命名为空时恢复为默认名称
        connect(tableWidget, &QTableWidget::itemChanged, dialog, [tableWidget, applyToList, generateName](QTableWidgetItem* item)
        {
            if (!item)
            {
                return;
            }
            const QString text = item->text().trimmed();
            if (text.isEmpty())
            {
                QSignalBlocker blocker(tableWidget);
                item->setText(generateName(QStringLiteral("工况组合")));
            }
            else if (text != item->text())
            {
                QSignalBlocker blocker(tableWidget);
                item->setText(text);
            }
            applyToList();
        });

        // 在表头“工况组合”按钮下方弹出
        QPoint popupPos;
        if (m_FrequencyResponse_Table && m_FrequencyResponse_Table->horizontalHeader())
        {
            QHeaderView* header = m_FrequencyResponse_Table->horizontalHeader();
            const int sectionX = header->sectionViewportPosition(3);
            const int sectionW = header->sectionSize(3);
            // 按钮绘制于该列右侧（right - 20 - 35），见 ButtonHeaderView::buttonRect
            popupPos = header->viewport()->mapToGlobal(QPoint(sectionX + sectionW - 35, header->height()));
        }
        else
        {
            popupPos = QCursor::pos();
        }
        dialog->move(popupPos.x() + 20 - dialog->width(), popupPos.y() + 4);
        dialog->show();
    }

    void mCaseImplicitRandomVibrationWindow::syncCaseCombinationCombos()
    {
        if (!m_FrequencyResponse_Table)
        {
            return;
        }
        for (int row = 0; row < m_FrequencyResponse_Table->rowCount(); ++row)
        {
            QWidget* cellWidget = m_FrequencyResponse_Table->cellWidget(row, 3);
            if (!cellWidget)
            {
                continue;
            }
            QComboBox* combo = cellWidget->findChild<QComboBox*>();
            if (!combo)
            {
                continue;
            }
            const QString currentText = combo->currentText();
            combo->clear();
            combo->addItems(m_caseCombinationList);
            const int index = combo->findText(currentText);
            combo->setCurrentIndex(index >= 0 ? index : 0);
        }
    }

    void mCaseImplicitRandomVibrationWindow::updateCaseLabelTable()
    {
        if (!m_CaseLabel_Table || !m_FrequencyResponse_Table)
        {
            return;
        }
        const int rowCount = m_FrequencyResponse_Table->rowCount();
        m_CaseLabel_Table->setRowCount(rowCount);
        for (int row = 0; row < rowCount; ++row)
        {
            const QString nodeId = m_FrequencyResponse_Table->item(row, 0) ? m_FrequencyResponse_Table->item(row, 0)->text() : QString();
            const QString dof = m_FrequencyResponse_Table->item(row, 2) ? m_FrequencyResponse_Table->item(row, 2)->text() : QString();

            // 选“独立工况”的行显示“节点ID+自由度”，其余行显示所选工况组合名
            QString label;
            QWidget* cellWidget = m_FrequencyResponse_Table->cellWidget(row, 3);
            QComboBox* combo = cellWidget ? cellWidget->findChild<QComboBox*>() : nullptr;
            if (combo && !combo->currentText().isEmpty() && combo->currentText() != QStringLiteral("独立工况"))
            {
                label = combo->currentText();
            }
            else
            {
                label = nodeId + QStringLiteral("+") + dof;
            }

            QTableWidgetItem* itemIndex = new QTableWidgetItem(QString::number(row + 1));
            itemIndex->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_CaseLabel_Table->setItem(row, 0, itemIndex);

            QTableWidgetItem* itemLabel = new QTableWidgetItem(label);
            itemLabel->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            m_CaseLabel_Table->setItem(row, 1, itemLabel);
        }

        updateSpectrumMatrixTable();
    }

    void mCaseImplicitRandomVibrationWindow::updateSpectrumMatrixTable()
    {
        if (!m_SpectrumMatrix_Table || !m_CaseLabel_Table)
        {
            return;
        }
        const int n = m_CaseLabel_Table->rowCount();

        // 行数未变化时不重建，保留矩阵当前的选中状态
        if (m_SpectrumMatrix_Table->rowCount() == n && m_SpectrumMatrix_Table->columnCount() == n)
        {
            return;
        }

        m_SpectrumMatrix_Table->clearContents();
        m_SpectrumMatrix_Table->setRowCount(n);
        m_SpectrumMatrix_Table->setColumnCount(n+1);

        QStringList headers;
        for (int i = 1; i <= n; ++i)
        {
            headers << QString::number(i);
        }
        m_SpectrumMatrix_Table->setHorizontalHeaderLabels(headers);
        m_SpectrumMatrix_Table->setVerticalHeaderLabels(headers);

        // 表头左对齐
        m_SpectrumMatrix_Table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_SpectrumMatrix_Table->verticalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // 每列固定 48px
        for (int col = 0; col < n; ++col)
        {
            m_SpectrumMatrix_Table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Fixed);
            m_SpectrumMatrix_Table->setColumnWidth(col, 48);
        }
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        m_SpectrumMatrix_Table->setHorizontalHeaderItem(n, headerItem);
        m_SpectrumMatrix_Table->horizontalHeader()->setSectionResizeMode(n, QHeaderView::Stretch);
        for (int row = 0; row < n; ++row)
        {
            for (int col = 0; col < n; ++col)
            {
                QTableWidgetItem* item = new QTableWidgetItem(QString("%1,%2").arg(row + 1).arg(col + 1));
                item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                if (row > col)
                {
                    // 下三角置灰：灰色文字，不可选择
                    item->setForeground(QBrush(QColor(0xC0, 0xC4, 0xCC)));
                    item->setFlags(Qt::NoItemFlags);
                }
                else
                {
                    // 对角线及上三角：不可编辑但可选择
                    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                }
                m_SpectrumMatrix_Table->setItem(row, col, item);
            }
        }
    }

    bool mCaseImplicitRandomVibrationWindow::isValidSpectrumCell(int row, int col) const
    {
        const int n = m_CaseLabel_Table ? m_CaseLabel_Table->rowCount() : 0;
        return row >= 0 && row < n && col >= 0 && col < n && row <= col;
    }

    void mCaseImplicitRandomVibrationWindow::saveSpectrumCellData()
    {
        if (!m_SpectrumMatrix_Table)
        {
            return;
        }
        QTableWidgetItem* item = m_SpectrumMatrix_Table->currentItem();
        if (!item)
        {
            return;
        }
        const int row = m_SpectrumMatrix_Table->currentRow();
        const int col = m_SpectrumMatrix_Table->currentColumn();
        if (!isValidSpectrumCell(row, col))
        {
            return;
        }
        item->setData(SpectrumRoleRealFactor, m_RealPartFactor->text());
        item->setData(SpectrumRoleRealCurve, m_RealPSDCurve->currentText());
        if (row < col)
        {
            // 上三角：实部 + 虚部
            item->setData(SpectrumRoleImagFactor, m_ImaginaryPartFactor->text());
            item->setData(SpectrumRoleImagCurve, m_ImaginaryPSDCurve->currentText());
        }
        else
        {
            // 对角线：仅实部
            item->setData(SpectrumRoleImagFactor, QString());
            item->setData(SpectrumRoleImagCurve, QString());
        }
    }

    void mCaseImplicitRandomVibrationWindow::loadSpectrumCellData(int row, int col)
    {
        // 加载时屏蔽信号，避免触发写回
        QSignalBlocker realFactorBlocker(m_RealPartFactor);
        QSignalBlocker realCurveBlocker(m_RealPSDCurve);
        QSignalBlocker imagFactorBlocker(m_ImaginaryPartFactor);
        QSignalBlocker imagCurveBlocker(m_ImaginaryPSDCurve);

        QTableWidgetItem* item = (m_SpectrumMatrix_Table && isValidSpectrumCell(row, col))
            ? m_SpectrumMatrix_Table->item(row, col) : nullptr;
        if (!item)
        {
            m_RealPartFactor->clear();
            m_RealPSDCurve->setCurrentIndex(0);
            m_ImaginaryPartFactor->clear();
            m_ImaginaryPSDCurve->setCurrentIndex(0);
            return;
        }

        m_RealPartFactor->setText(item->data(SpectrumRoleRealFactor).toString());
        const int realIdx = m_RealPSDCurve->findText(item->data(SpectrumRoleRealCurve).toString());
        m_RealPSDCurve->setCurrentIndex(realIdx >= 0 ? realIdx : 0);

        if (row < col)
        {
            m_ImaginaryPartFactor->setText(item->data(SpectrumRoleImagFactor).toString());
            const int imagIdx = m_ImaginaryPSDCurve->findText(item->data(SpectrumRoleImagCurve).toString());
            m_ImaginaryPSDCurve->setCurrentIndex(imagIdx >= 0 ? imagIdx : 0);
        }
        else
        {
            m_ImaginaryPartFactor->clear();
            m_ImaginaryPSDCurve->setCurrentIndex(0);
        }
        updateCaseApplyButtonState();
    }

    void mCaseImplicitRandomVibrationWindow::updateCaseApplyButtonState()
    {
        if (!m_CaseApply || !m_SpectrumMatrix_Table)
        {
            return;
        }
        const int row = m_SpectrumMatrix_Table->currentRow();
        const int col = m_SpectrumMatrix_Table->currentColumn();
        QTableWidgetItem* item = isValidSpectrumCell(row, col) ? m_SpectrumMatrix_Table->item(row, col) : nullptr;
        if (!item)
        {
            m_CaseApply->setEnabled(false);
            return;
        }

        // 设置完整性：对角线仅需实部，上三角还需虚部
        const QString noneCurve = QStringLiteral("无");
        const bool realComplete = isValidDouble(m_RealPartFactor) && m_RealPSDCurve->currentText() != noneCurve;
        const bool complete = realComplete
            && (row == col
                || (isValidDouble(m_ImaginaryPartFactor) && m_ImaginaryPSDCurve->currentText() != noneCurve));
        if (!complete)
        {
            m_CaseApply->setEnabled(false);
            return;
        }

        // 已应用且设置未变化：保持置灰
        if (item->data(SpectrumRoleApplied).toBool())
        {
            auto sameFactor = [](const QString& a, const QString& b)
            {
                bool okA = false;
                bool okB = false;
                const double da = a.trimmed().toDouble(&okA);
                const double db = b.trimmed().toDouble(&okB);
                if (okA && okB)
                {
                    return qFuzzyCompare(da, db);
                }
                return a.trimmed() == b.trimmed();
            };
            const bool unchanged = sameFactor(item->data(SpectrumRoleAppliedRealFactor).toString(), m_RealPartFactor->text())
                && item->data(SpectrumRoleAppliedRealCurve).toString() == m_RealPSDCurve->currentText()
                && sameFactor(item->data(SpectrumRoleAppliedImagFactor).toString(), m_ImaginaryPartFactor->text())
                && item->data(SpectrumRoleAppliedImagCurve).toString() == m_ImaginaryPSDCurve->currentText();
            m_CaseApply->setEnabled(!unchanged);
            return;
        }
        m_CaseApply->setEnabled(true);
    }

    void mCaseImplicitRandomVibrationWindow::applySpectrumCellSettings()
    {
        if (!m_SpectrumMatrix_Table)
        {
            return;
        }
        // 先确保当前控件值已写入 item
        saveSpectrumCellData();
        const int row = m_SpectrumMatrix_Table->currentRow();
        const int col = m_SpectrumMatrix_Table->currentColumn();
        QTableWidgetItem* item = isValidSpectrumCell(row, col) ? m_SpectrumMatrix_Table->item(row, col) : nullptr;
        if (!item)
        {
            return;
        }

        // 快照已应用的设置
        item->setData(SpectrumRoleApplied, true);
        item->setData(SpectrumRoleAppliedRealFactor, m_RealPartFactor->text());
        item->setData(SpectrumRoleAppliedRealCurve, m_RealPSDCurve->currentText());
        if (row < col)
        {
            item->setData(SpectrumRoleAppliedImagFactor, m_ImaginaryPartFactor->text());
            item->setData(SpectrumRoleAppliedImagCurve, m_ImaginaryPSDCurve->currentText());
        }
        else
        {
            item->setData(SpectrumRoleAppliedImagFactor, QString());
            item->setData(SpectrumRoleAppliedImagCurve, QString());
        }

        // 格子变绿，按钮置灰，刷新步骤完成状态
        item->setBackground(QBrush(SpectrumAppliedColor));
        refreshSpectrumCellColors();
        m_CaseApply->setEnabled(false);
        onPageModified(5);
    }

    void mCaseImplicitRandomVibrationWindow::refreshSpectrumCellColors()
    {
        if (!m_SpectrumMatrix_Table || !m_CaseLabel_Table)
        {
            return;
        }
        const int n = m_CaseLabel_Table->rowCount();
        const QString noneCurve = QStringLiteral("无");
        const auto curveSet = [&noneCurve](QTableWidgetItem* item, int role) -> bool
        {
            const QString curve = item ? item->data(role).toString() : QString();
            return !curve.isEmpty() && curve != noneCurve;
        };
        for (int row = 0; row < n && row < m_SpectrumMatrix_Table->rowCount(); ++row)
        {
            for (int col = row; col < n && col < m_SpectrumMatrix_Table->columnCount(); ++col)
            {
                QTableWidgetItem* item = m_SpectrumMatrix_Table->item(row, col);
                if (!item)
                {
                    continue;
                }
                // 对角线：实部曲线设置好即绿色；上三角：实部+虚部曲线都设置好才绿色
                bool complete = curveSet(item, SpectrumRoleRealCurve);
                if (complete && row < col)
                {
                    complete = curveSet(item, SpectrumRoleImagCurve);
                }
                item->setBackground(complete ? QBrush(SpectrumAppliedColor) : QBrush(Qt::white));
            }
        }
    }

    void mCaseImplicitRandomVibrationWindow::slot_onCaseClear()
    {
        if (!m_SpectrumMatrix_Table)
        {
            return;
        }
        const int row = m_SpectrumMatrix_Table->currentRow();
        const int col = m_SpectrumMatrix_Table->currentColumn();
        QTableWidgetItem* item = isValidSpectrumCell(row, col) ? m_SpectrumMatrix_Table->item(row, col) : nullptr;
        if (!item)
        {
            return;
        }

        // 控件复位：曲线回到“无”，因子清空
        m_RealPSDCurve->setCurrentIndex(0);
        m_RealPartFactor->clear();
        m_ImaginaryPSDCurve->setCurrentIndex(0);
        m_ImaginaryPartFactor->clear();

        // 同步清空 item 中存储的数据（无写回逻辑，需显式修改）
        item->setData(SpectrumRoleRealFactor, QString());
        item->setData(SpectrumRoleRealCurve, QStringLiteral("无"));
        item->setData(SpectrumRoleImagFactor, QString());
        item->setData(SpectrumRoleImagCurve, QStringLiteral("无"));

        // 清除已应用标记与快照
        item->setData(SpectrumRoleApplied, false);
        item->setData(SpectrumRoleAppliedRealFactor, QString());
        item->setData(SpectrumRoleAppliedRealCurve, QString());
        item->setData(SpectrumRoleAppliedImagFactor, QString());
        item->setData(SpectrumRoleAppliedImagCurve, QString());

        // 底色恢复白色，刷新按钮与步骤完成状态
        refreshSpectrumCellColors();
        updateCaseApplyButtonState();
        onPageModified(5);
    }

    void mCaseImplicitRandomVibrationWindow::removePsdCurve(QComboBox* combo, int row)
    {
        if (!combo || row <= 0 || row >= combo->count())
        {
            // 第一项“无”为固定项，不可删除
            return;
        }
        const QString name = combo->itemText(row);
        if (name.isEmpty())
        {
            return;
        }

        // 从曲线库中删除
        m_psdCurveMap.remove(name);

        // 从两个曲线下拉框中移除
        for (QComboBox* c : { m_RealPSDCurve, m_ImaginaryPSDCurve })
        {
            const int idx = c->findText(name);
            if (idx > 0)
            {
                c->removeItem(idx);
            }
        }

        // 复位矩阵中引用了该曲线的格子（含已应用快照），避免残留失效引用
        const int n = m_CaseLabel_Table ? m_CaseLabel_Table->rowCount() : 0;
        if (m_SpectrumMatrix_Table)
        {
            const QString noneCurve = QStringLiteral("无");
            for (int r = 0; r < n && r < m_SpectrumMatrix_Table->rowCount(); ++r)
            {
                for (int col = r; col < n && col < m_SpectrumMatrix_Table->columnCount(); ++col)
                {
                    QTableWidgetItem* item = m_SpectrumMatrix_Table->item(r, col);
                    if (!item)
                    {
                        continue;
                    }
                    if (item->data(SpectrumRoleRealCurve).toString() == name)
                    {
                        item->setData(SpectrumRoleRealCurve, noneCurve);
                    }
                    if (item->data(SpectrumRoleImagCurve).toString() == name)
                    {
                        item->setData(SpectrumRoleImagCurve, noneCurve);
                    }
                    const bool snapReal = item->data(SpectrumRoleAppliedRealCurve).toString() == name;
                    const bool snapImag = item->data(SpectrumRoleAppliedImagCurve).toString() == name;
                    if (snapReal || snapImag)
                    {
                        item->setData(SpectrumRoleApplied, false);
                        if (snapReal)
                        {
                            item->setData(SpectrumRoleAppliedRealCurve, noneCurve);
                        }
                        if (snapImag)
                        {
                            item->setData(SpectrumRoleAppliedImagCurve, noneCurve);
                        }
                    }
                }
            }
        }

        refreshSpectrumCellColors();
        updateCaseApplyButtonState();
        onPageModified(5);
    }

    void mCaseImplicitRandomVibrationWindow::registerPsdCurveName(QComboBox* combo, const QString& name)
    {
        if (!combo || name.trimmed().isEmpty())
        {
            return;
        }
        if (combo->findText(name) < 0)
        {
            combo->addItem(name);
        }
        combo->setCurrentText(name);
    }

    void mCaseImplicitRandomVibrationWindow::openPsdCurveCreateDialog(QComboBox* combo)
    {
        CreatePSDCurveDialog* dialog = CreatePSDCurveDialog::getInstance();
        dialog->initDialog();
        if (dialog->exec() != QDialog::Accepted)
        {
            return;
        }
        const IncentiveCurve data = dialog->getDialogData();
        if (data.name.trimmed().isEmpty())
        {
            return;
        }
        m_psdCurveMap[data.name] = data;
        QComboBox* other = (combo == m_RealPSDCurve) ? m_ImaginaryPSDCurve : m_RealPSDCurve;
        registerPsdCurveName(other, data.name);
        registerPsdCurveName(combo, data.name);
    }

    void mCaseImplicitRandomVibrationWindow::openPsdCurveEditDialog(QComboBox* combo)
    {
        CreatePSDCurveDialog* dialog = CreatePSDCurveDialog::getInstance();
        const QString curveName = combo ? combo->currentText() : QString();
        const bool hasCurve = (!curveName.isEmpty() && curveName != QStringLiteral("无") && m_psdCurveMap.contains(curveName));
        if (hasCurve)
        {
            dialog->setDialogData(m_psdCurveMap.value(curveName));
        }
        else
        {
            dialog->initDialog();
        }
        if (dialog->exec() != QDialog::Accepted)
        {
            return;
        }
        const IncentiveCurve data = dialog->getDialogData();
        if (data.name.trimmed().isEmpty())
        {
            return;
        }
        if (hasCurve && data.name != curveName)
        {
            // 编辑时改了曲线名：旧曲线从库和下拉框中移除
            m_psdCurveMap.remove(curveName);
            for (QComboBox* c : { m_RealPSDCurve, m_ImaginaryPSDCurve })
            {
                const int idx = c->findText(curveName);
                if (idx >= 0)
                {
                    c->removeItem(idx);
                }
            }
        }
        m_psdCurveMap[data.name] = data;
        QComboBox* other = (combo == m_RealPSDCurve) ? m_ImaginaryPSDCurve : m_RealPSDCurve;
        registerPsdCurveName(other, data.name);
        registerPsdCurveName(combo, data.name);
    }

    // 输入框输入限制：数值框仅允许合法数值形态，整数框仅允许数字，非法输入直接置空
    void mCaseImplicitRandomVibrationWindow::installInputValidators()
    {
        QList<QLineEdit*> textEdits;
        QList<QLineEdit*> intEdits = { m_lineEdit_ModeCount, m_lineEdit_FluidModeCount, m_FREQ1_NDF, m_FREQ2_NF,
            m_FREQ3_NEF, m_FREQ4_NFM, m_ModalDamping_PointCount, m_FluidModalDamping_PointCount,
            m_constraint_PointCount, m_Incentive_PointCount };

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

    void mCaseImplicitRandomVibrationWindow::initConnect()
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
        connect(m_checkbox_FluidRangeSetting, &QRadioButton::toggled, this, [=](bool checked) {
            if (m_FluidRangeSettingContainer) {
                m_FluidRangeSettingContainer->setEnabled(checked);
            }
            m_ASCoupling_Widget->setEnabled(checked);
            m_checkBox_FluidEnableDamping->setEnabled(checked);
            m_FluiddampingSettingsContainer->setEnabled(checked);
            m_SoundPressureResponse->SetContentEnable(checked);
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
        connect(m_pushButton_CreateMxCase, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onCreateMxCase);
        connect(m_pushButton_ExportFem, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onExportFem);
        connect(m_pushButton_Previous, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onPreviousStep);
        connect(m_pushButton_Next, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onNextStep);
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
        connect(m_ModalDamping_ExportFile, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onExportDampingFile);
        connect(m_FluidModalDamping_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_onImportFluidDampingFile();
            });
        connect(m_FluidModalDamping_ExportFile, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onExportFluidDampingFile);
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
        connect(m_constraint_ExportFile, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onExportConstraintFile);
        connect(m_Incentive_ImportFile, &QPushButton::clicked, this, [this]() {
            slot_ImportIncentiveFromFile();
            connectTableCheckBoxes(m_Incentive_NodeTable, 4);
            onPageModified(4);
            });
        connect(m_Incentive_ExportFile, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onExportIncentiveFile);
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
                    slot_onIncentiveSettingTableChanged();
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
                slot_onIncentiveSettingTableChanged();
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
        connect(m_ModalDamping_Table, &QTableWidget::itemSelectionChanged, this, &mCaseImplicitRandomVibrationWindow::updateModalDampingDeleteButtonState);
        connect(m_FluidModalDamping_Table, &QTableWidget::itemSelectionChanged, this, &mCaseImplicitRandomVibrationWindow::updateFluidModalDampingDeleteButtonState);
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
        connect(m_OutputDisplacement, &QCheckBox::toggled, this, [this]() { onPageModified(7); });
        connect(m_OutputSpeed, &QCheckBox::toggled, this, [this]() { onPageModified(7); });
        connect(m_Outputaccelerated, &QCheckBox::toggled, this, [this]() { onPageModified(7); });
        connect(m_Outputaccelerated, &QCheckBox::toggled, this, [this]() { onPageModified(7); });
        connect(m_OutputSoundPressure, &QCheckBox::toggled, this, [this]() { onPageModified(7); });

        // 初始时连接约束/激励表格中已有的复选框
        connectTableCheckBoxes(m_Incentive_NodeTable, 3);
        connectTableCheckBoxes(m_constraint_NodeTable, 4);

        // 响应设置四个面板数据变化时刷新完成状态
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
                    onPageModified(6);
                });
        if (m_SoundPressureResponse)
            connect(m_SoundPressureResponse, &ResponseSettingFoldablePanel::dataChanged, this, [this]()
                {
                    if (m_SoundPressureResponse->hasValidRow())
                        m_SoundPressureResponse->setEnabled(true);
                    else
                        m_SoundPressureResponse->setEnabled(false);
                    onPageModified(6);
                });

        // 功率谱矩阵点击：对角线仅实部可设置，上三角实部+虚部均可设置，其余（下三角/空列等）均禁用
        connect(m_SpectrumMatrix_Table, &QTableWidget::cellClicked, this, [this](int row, int col)
        {
            const int n = m_CaseLabel_Table ? m_CaseLabel_Table->rowCount() : 0;
            const bool valid = (row >= 0 && row < n && col >= 0 && col < n);
            if (valid && row == col)
            {
                // 对角线：自功率谱，仅实部
                m_RealWidgetContent->setEnabled(true);
                m_ImaginaryWidgetContent->setEnabled(false);
            }
            else if (valid && row < col)
            {
                // 上三角：互功率谱，实部+虚部
                m_RealWidgetContent->setEnabled(true);
                m_ImaginaryWidgetContent->setEnabled(true);
            }
            else
            {
                // 下三角及空列等其它区域
                m_RealWidgetContent->setEnabled(false);
                m_ImaginaryWidgetContent->setEnabled(false);
            }
            // 将该 item 已存的实部/虚部设置加载到控件（无效区域则复位）
            loadSpectrumCellData(row, col);
        });

        // 实部/虚部因子与曲线选择变化时仅刷新“应用”按钮状态；
        // 控件数据在点击“应用”后才写入 item，切换格子未应用的内容会被丢弃
        connect(m_RealPartFactor, &QLineEdit::editingFinished, this, [this]() { updateCaseApplyButtonState(); });
        connect(m_ImaginaryPartFactor, &QLineEdit::editingFinished, this, [this]() { updateCaseApplyButtonState(); });
        connect(m_RealPSDCurve, &QComboBox::currentTextChanged, this, [this](const QString&) { updateCaseApplyButtonState(); });
        connect(m_ImaginaryPSDCurve, &QComboBox::currentTextChanged, this, [this](const QString&) { updateCaseApplyButtonState(); });

        // PSD 曲线创建/编辑按钮
        connect(m_RealCurveCreate, &QPushButton::clicked, this, [this]() { openPsdCurveCreateDialog(m_RealPSDCurve); });
        connect(m_ImaginaryCurveCreate, &QPushButton::clicked, this, [this]() { openPsdCurveCreateDialog(m_ImaginaryPSDCurve); });
        connect(m_RealCurveEdit, &QPushButton::clicked, this, [this]() { openPsdCurveEditDialog(m_RealPSDCurve); });
        connect(m_ImaginaryCurveEdit, &QPushButton::clicked, this, [this]() { openPsdCurveEditDialog(m_ImaginaryPSDCurve); });

        // “应用”按钮：将当前设置应用到矩阵 item（格子变绿）
        connect(m_CaseApply, &QPushButton::clicked, this, [this]() { applySpectrumCellSettings(); });

        // “清除”按钮：当前格子的曲线恢复“无”，底色变白
        connect(m_CaseClear, &QPushButton::clicked, this, &mCaseImplicitRandomVibrationWindow::slot_onCaseClear);
    }

    /*
    * 构造完成后快照控件默认状态（表格内部单元格控件不参与，随表格整体重置）
    */
    void mCaseImplicitRandomVibrationWindow::captureDefaultState()
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
            // 频响工况表含单元格控件，不走行数恢复，重置时从载荷表整体重建；响应面板表由面板自身重置
            bool inPanel = false;
            for (QWidget* p = t->parentWidget(); p; p = p->parentWidget())
            {
                if (qobject_cast<ResponseSettingFoldablePanel*>(p))
                {
                    inPanel = true;
                    break;
                }
            }
            if (!qobject_cast<IncentiveNodeTableWidget*>(t) && !qobject_cast<IncentiveSettingTableWidget*>(t) && t != m_FrequencyResponse_Table && !inPanel)
            {
                // 表格加入布局后父组件会被重定向，需沿祖先链判断是否属于响应面板
                m_defaultRowCount[t] = t->rowCount();
            }
        }
    }

    /*
    * 恢复窗口默认状态：导航/进度复位 + 表格重置 + 控件默认值恢复
    */
    void mCaseImplicitRandomVibrationWindow::resetToDefault()
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

        // 其余无建行入口的表格：仅恢复默认行数（默认均为空行）
        for (auto it = m_defaultRowCount.begin(); it != m_defaultRowCount.end(); ++it)
        {
            QTableWidget* t = it.key();
            if (t == m_ModalDamping_Table || t == m_FluidModalDamping_Table)
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
        // 频响工况表从重置后的载荷表整体重建
        slot_onIncentiveSettingTableChanged();
        applySolverDependentState();
        updateBottomButtons(0);
        updateValidationState();
    }

    void mCaseImplicitRandomVibrationWindow::closeEvent(QCloseEvent* event)
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

    void mCaseImplicitRandomVibrationWindow::slot_onClose()
    {
        this->close();
    }

    bool mCaseImplicitRandomVibrationWindow::isAnalysisSettingValid() const
    {
        if (!m_comboBox_Solver)
        {
            return false;
        }
        //特征值求解器选择“自动多重子结构“”时，结构模态和流体模态最大频率为必选
        if (m_checkbox_FluidRangeSetting->isChecked() )
        {
            if ( m_comboBox_Solver->currentIndex() == 0)
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
        }
        else 
        {
            if (m_comboBox_Solver->currentIndex() == 0)
            {
                bool strHasMaxFreq = isValidDouble(m_lineEdit_FreqMax);
                if (strHasMaxFreq)
                {
                    return true;
                }
            }
            else
            {
                bool strHasMaxFreq = isValidDouble(m_lineEdit_FreqMax);
                bool strHasModeCount = isValidDouble(m_lineEdit_ModeCount);
                bool structDamping = strHasMaxFreq || strHasModeCount;
                return structDamping;
            }
        }
       
        return false;
    }

    bool mCaseImplicitRandomVibrationWindow::isFrequencyRangeValid() const
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

    bool mCaseImplicitRandomVibrationWindow::isFrequencyGroupValid(int groupIndex) const
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

    bool mCaseImplicitRandomVibrationWindow::isIncentiveValid() const
    {
        if (!m_Incentive_NodeTable)
        {
            return false;
        }

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
                return true;
            }
        }
        return false;
    }

    bool mCaseImplicitRandomVibrationWindow::isDampingValid() const
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

    bool mCaseImplicitRandomVibrationWindow::isFluidDampingValid() const
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

    bool mCaseImplicitRandomVibrationWindow::isConstraintValid() const
    {
        // 约束设置允许为空，始终视为有效
        return true;
    }

    bool mCaseImplicitRandomVibrationWindow::isCaseSettingValid() const
    {
        if (!m_SpectrumMatrix_Table || !m_CaseLabel_Table)
        {
            return false;
        }
        const int n = m_CaseLabel_Table->rowCount();
        if (n <= 0 || m_SpectrumMatrix_Table->rowCount() < n)
        {
            return false;
        }
        // 只有一行：对角线格子设置好 PSD 曲线即可；多行：沿对角线全部设置好才有效
        const QString noneCurve = QStringLiteral("无");
        for (int i = 0; i < n; ++i)
        {
            const QTableWidgetItem* item = m_SpectrumMatrix_Table->item(i, i);
            if (!item)
            {
                return false;
            }
            const QString curve = item->data(SpectrumRoleRealCurve).toString();
            if (curve.isEmpty() || curve == noneCurve)
            {
                return false;
            }
        }
        return true;
    }

    bool mCaseImplicitRandomVibrationWindow::isResponseValid() const
    {
        return (m_VibrationResponse && m_VibrationResponse->hasValidRow())
            || (m_SoundPressureResponse && m_SoundPressureResponse->hasValidRow());
    }

    bool mCaseImplicitRandomVibrationWindow::isOutputValid() const
    {
        if (m_OutputDisplacement->isChecked()) return true;
        if (m_OutputSpeed->isChecked()) return true;
        if (m_Outputaccelerated->isChecked()) return true;
        if (m_OutputSoundPressure->isChecked()) return true;
        return false;
    }

    void mCaseImplicitRandomVibrationWindow::updateValidationState()
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
        // 工况设置：矩阵至少一行且沿对角线的 PSD 曲线均已设置即打勾
        m_navIconDelegate->setPageCompleted(5, m_farthestReachedIndex > 5 && isCaseSettingValid());
        // 响应设置：四个响应表格中至少一个存在有效数据行即打勾
        m_navIconDelegate->setPageCompleted(6, m_farthestReachedIndex > 6 && isResponseValid());
        m_navIconDelegate->setPageCompleted(7, false);

        for (int i = 0; i < m_stackWidget->count(); ++i)
        {
            // 未解锁时仅当前进度页之前的步骤可点击；解锁后允许已到达过的步骤
            m_navIconDelegate->setPageEnabled(i, isNavigationIndexEnabled(i));
        }

        refreshNavigationIcons();
        updateOutputActionButtons();
    }

    void mCaseImplicitRandomVibrationWindow::updateOutputActionButtons()
    {
        bool enabled = isAnalysisSettingValid() && isFrequencyRangeValid() && isDampingValid()
            && isIncentiveValid() && isResponseValid() && isCaseSettingValid() && isOutputValid();
        if(m_pushButton_ExportFem)
            m_pushButton_ExportFem->setEnabled(enabled);
        if(m_pushButton_CreateMxCase)
            m_pushButton_CreateMxCase->setEnabled(enabled);
    }

    void mCaseImplicitRandomVibrationWindow::updateBottomButtons(int currentIndex)
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

    void mCaseImplicitRandomVibrationWindow::updateNextButtonText(int currentIndex)
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

    void mCaseImplicitRandomVibrationWindow::slot_onNextStep()
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

    void mCaseImplicitRandomVibrationWindow::slot_onPreviousStep()
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

    void mCaseImplicitRandomVibrationWindow::slot_onCreateMxCase()
    {
        auto* modelData = getCVM->getModelData();
        mMeshData* meshData = modelData->getMeshData();

        if (!meshData)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("模型网格数据不存在，无法创建激励点"));
            return;
        }

        // 0. 前置校验：约束、激励点必须存在于网格数据中
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
        if (m_checkbox_FluidRangeSetting->isChecked() && m_checkBox_AcousticSolidCoupling->isChecked())
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

        //根据频响工况设置表创建分析工况：工况名与工况标签列表一致
        //（独立工况行=节点ID+自由度；选择了工况组合的行按组合名聚合为一个工况）
        QStringList caseLabels;                       // 工况创建顺序
        QMap<QString, QList<int>> labelForceLoadIDs;  // 标签 -> 力载荷ID列表
        QMap<QString, int> labelFirstRow;             // 标签首次出现的行号（用于频率点表关联）
        QMap<QString, int> labelLoadCount;          // 标签 -> 已创建力载荷个数（载荷按标签顺序编号）
        const QString noneCombination = QStringLiteral("独立工况");
        for (int r = 0; m_FrequencyResponse_Table && r < m_FrequencyResponse_Table->rowCount(); ++r)
        {
            QTableWidgetItem* idItem = m_FrequencyResponse_Table->item(r, 0);
            QTableWidgetItem* dofItem = m_FrequencyResponse_Table->item(r, 2);
            if (!idItem || !dofItem)
            {
                continue;
            }
            bool ok = false;
            const int id = idItem->text().trimmed().toInt(&ok);
            const QString dof = dofItem->text().trimmed();
            if (!ok || id <= 0 || dof.isEmpty())
            {
                continue;
            }

            // 该行的工况组合选择决定工况名（与工况标签列表的生成规则一致）
            QString label;
            QWidget* comboCell = m_FrequencyResponse_Table->cellWidget(r, 3);
            QComboBox* combo = comboCell ? comboCell->findChild<QComboBox*>() : nullptr;
            const QString comboText = combo ? combo->currentText().trimmed() : QString();
            if (!comboText.isEmpty() && comboText != noneCombination)
            {
                label = comboText;
            }
            else
            {
                label = QString::number(id) + "+" + dof;
            }

            // 创建该行的力载荷与节点集合：同一工况（标签）下的载荷按顺序编号 标签_1、标签_2...
            const bool isTorque = (dof == "RX" || dof == "RY" || dof == "RZ");
            const int loadIndex = labelLoadCount[label] + 1;
            labelLoadCount[label] = loadIndex;
            const QString name = label + "_" + QString::number(loadIndex);
            int forceLoadID = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).createID();
            shared_ptr<MechCaseData::mLoadForce> forceLoad = make_shared<MechCaseData::mLoadForce>(
                forceLoadID, isTorque ? LOAD_IM_TORQUE : LOAD_IM_FORCE, name);
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

            // 按工况名聚合载荷
            if (!labelForceLoadIDs.contains(label))
            {
                caseLabels.append(label);
                labelFirstRow.insert(label, r);
            }
            labelForceLoadIDs[label].append(forceLoadID);
        }

        // 每个工况名创建一个子工况，挂接该工况的全部载荷
        for (const QString& label : caseLabels)
        {
            if (m_checkbox_FluidRangeSetting->isChecked())
                createASCouplingAnalysis(label, labelForceLoadIDs.value(label), labelFirstRow.value(label), labelLoadCount.value(label));
            else
                createFrequencyResponseAnalysis(label, labelForceLoadIDs.value(label), labelFirstRow.value(label), labelLoadCount.value(label));
        }
        createRandomVibrationAnalysis();
        mGlobalSignals::getInstance()->updateModelTree();
        this->accept();

        // 工况已创建，关闭时不再提示
        m_mxCaseCreated = true;
    }

    void mCaseImplicitRandomVibrationWindow::createASCouplingAnalysis(const QString& name, const QList<int>& forceLoadIDs, int i, int loadCount)
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplASCoupling* caseData = new MechCaseData::mCaseImplASCoupling(caseID, ASCoupling, name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(caseData);
        //分析设置（结构 + 流体模态提取范围）
        caseData->_modalSolver = m_comboBox_Solver->currentText();
        caseData->_structMinFrequency = m_lineEdit_FreqMin->text();
        caseData->_structMaxFrequency = m_lineEdit_FreqMax->text();
        caseData->_structOrder = m_lineEdit_ModeCount->text();
        caseData->_acousticsMinFrequency = m_lineEdit_FluidFreqMin->text();
        caseData->_acousticsMaxFrequency = m_lineEdit_FluidFreqMax->text();
        caseData->_acousticsOrder = m_lineEdit_FluidModeCount->text();
        if (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked())
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
                for (int r = 0; r < m_ModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / damp);
                }
            }
            else if (m_ModalDamping_Type->currentIndex() == 1)
            {
                for (int r = 0; r < m_ModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / (2.0 * damp));
                }
            }
            else
            {
                for (int r = 0; r < m_ModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp))
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp);
                }
            }
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Case, caseID }, { Physical_Function, funcID });
        }

        //阻尼设置（流体阻尼）
        if (!m_checkBox_FluidEnableDamping->isChecked())
        {
            // 用户跳过了流体阻尼设置，不写入流体阻尼数据
        }
        else if (m_FluidStructuralDamping && m_FluidStructuralDamping->isChecked())
        {
            caseData->_acousticsDampingG = m_FluideditStructuralDamping->text();
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
                for (int r = 0; r < m_FluidModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / damp);
                }
            }
            else if (m_FluidModalDamping_Type->currentIndex() == 1)
            {
                for (int r = 0; r < m_FluidModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp) || damp == 0.0)
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(1.0 / (2.0 * damp));
                }
            }
            else
            {
                for (int r = 0; r < m_FluidModalDamping_Table->rowCount(); ++r)
                {
                    double freq = 0.0;
                    double damp = 0.0;
                    if (!getRowValues(r, freq, damp))
                        continue;
                    fundata.x.push_back(freq);
                    fundata.y.push_back(damp);
                }
            }
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Case, caseID }, { Physical_Function, funcID });
        }

        //频率点设置
        int freqID = createFrequencyPoint(name);
        caseData->checked = true;
        getCVM->getModelData()->getMapRelationManager().appendTableMapRelation({ Physical_Case, caseID },
            { Physical_FreqSamplPoint, freqID }, i);

        //输出设置：先清空云图输出控制与PCH输出控制，再写入NVH中勾选的输出
        caseData->setOutputs({});
        caseData->setPchOutputs({});
        caseData->setSelectedSet({});
        caseData->setPchSelectedSet({});
        //全局云图结果输出取物理量设置，对应随机振动面板的PSD谱分析
        QHash<QString, QStringList> outputs;
        if (m_PhysicalDisplacement->isChecked())
            outputs["PSD谱分析"].append("节点位移功率谱");
        if (m_PhysicalSpeed->isChecked())
            outputs["PSD谱分析"].append("节点速度功率谱");
        if (m_Physicalaccelerated->isChecked())
            outputs["PSD谱分析"].append("节点加速度功率谱");
        if (m_PhysicalStress->isChecked())
            outputs["PSD谱分析"].append("单元应力功率谱");
        caseData->setOutputs(outputs);

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

        //振动结果输出：勾选项写入PCH输出控制，并应用于振动响应节点集合
        if (!vibrationSetName.isEmpty())
        {
            QHash<QString, QStringList> pchOutputs;
            QHash<QString, QHash<QString, QStringList>> pchSetData;
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
            caseData->setPchOutputs(pchOutputs);
            caseData->setPchSelectedSet(pchSetData);
        }
        // 输出文件类型与工况面板的输出文件类型对应
        caseData->setOutputType(m_comboBox_outputForm->currentText());

        //约束设置(位移/转角)
        std::map<RenderPickResultData, QVector<QVariant>> independentTable;
        int boundaryID = -1;
        for (int r = 0; m_constraint_NodeTable && r < m_constraint_NodeTable->rowCount(); ++r)
        {
            auto row = m_constraint_NodeTable->getRowData(r);
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
            if (row.x) values[0] = 0.0;
            if (row.y) values[1] = 0.0;
            if (row.z) values[2] = 0.0;
            if (row.rx) values[3] = 0.0;
            if (row.ry) values[4] = 0.0;
            if (row.rz) values[5] = 0.0;
            independentTable[RenderPickResultData(id, -1, 0)] = values;
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
        shared_ptr<MechCaseData::mLoadFreqdomain> freqLoadLoad = make_shared<MechCaseData::mLoadFreqdomain>(freqLoadID, LOAD_IM_FREQDOMAIN, name + "_" + QString::number(loadCount + 1));
        getCVM->getModelData()->addPhysicalFieldP(freqLoadLoad, caseID);
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Case, caseID },
            { Physical_LoadBoundary, freqLoadID });
        freqLoadLoad->way_ = m_LoadType->currentIndex() + 1;

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
        // 挂接该工况聚合的全部载荷（工况组合时不止一个）
        for (int loadID : forceLoadIDs)
        {
            getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                { Physical_LoadBoundary ,freqLoadID }, { Physical_LoadBoundary ,loadID });
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("创建 Mx 工况：%1").arg(name));
    }

    void mCaseImplicitRandomVibrationWindow::createFrequencyResponseAnalysis(const QString& name, const QList<int>& forceLoadIDs, int i, int loadCount)
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplicitFrequency* caseData = new MechCaseData::mCaseImplicitFrequency(caseID, ImplicitFrequency, name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(caseData);
        //分析设置
        if (m_comboBox_Solver->currentIndex() == 0)
        {
            caseData->_modalSolver = "自动多重子结构";
            caseData->_minFrequency = m_lineEdit_FreqMin->text();
            caseData->_maxFrequency = m_lineEdit_FreqMax->text();
            caseData->_order = m_lineEdit_ModeCount->text();
            caseData->_ampffact = m_lineEdit_ModeScale->text();
        }
        else
        {
            caseData->_modalSolver = "兰索斯";
            caseData->_minFrequency = m_lineEdit_FreqMin->text();
            caseData->_maxFrequency = m_lineEdit_FreqMax->text();
            caseData->_order = m_lineEdit_ModeCount->text();
            caseData->_normalization == "无";
        }
        //阻尼设置
        if (m_checkBox_EnableDamping && !m_checkBox_EnableDamping->isChecked())
        {
            // 用户跳过了阻尼设置，不写入阻尼数据
        }
        else if (m_StructuralDamping && m_StructuralDamping->isChecked())
        {
            caseData->_dampingG = m_editStructuralDamping->text();
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
                    fundata.y.push_back(1.0 / damp);
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
                    fundata.y.push_back(1.0 / (2.0 * damp));
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
                    fundata.y.push_back(damp);
                }
            }
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Case, caseID }, { Physical_Function, funcID });
        }

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
        //全局云图结果输出取物理量设置，对应随机振动面板的PSD谱分析
        QHash<QString, QStringList> outputs;
        if (m_PhysicalDisplacement->isChecked())
            outputs["PSD谱分析"].append("节点位移功率谱");
        if (m_PhysicalSpeed->isChecked())
            outputs["PSD谱分析"].append("节点速度功率谱");
        if (m_Physicalaccelerated->isChecked())
            outputs["PSD谱分析"].append("节点加速度功率谱");
        if (m_PhysicalStress->isChecked())
            outputs["PSD谱分析"].append("单元应力功率谱");
        caseData->setOutputs(outputs);

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

        //振动结果输出：勾选项写入PCH输出控制，并应用于振动响应节点集合
        if (!vibrationSetName.isEmpty())
        {
            QHash<QString, QStringList> pchOutputs;
            QHash<QString, QHash<QString, QStringList>> pchSetData;
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
            caseData->setPchOutputs(pchOutputs);
            caseData->setPchSelectedSet(pchSetData);
        }
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
            if (row.x) values[0] = 0.0;
            if (row.y) values[1] = 0.0;
            if (row.z) values[2] = 0.0;
            if (row.rx) values[3] = 0.0;
            if (row.ry) values[4] = 0.0;
            if (row.rz) values[5] = 0.0;
            independentTable[RenderPickResultData(id, -1, 0)] = values;
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
        shared_ptr<MechCaseData::mLoadFreqdomain> freqLoadLoad = make_shared<MechCaseData::mLoadFreqdomain>(freqLoadID, LOAD_IM_FREQDOMAIN, name + "_" + QString::number(loadCount + 1));
        getCVM->getModelData()->addPhysicalFieldP(freqLoadLoad, caseID);
        getCVM->getModelData()->getMapRelationManager().appendMapRelation(
            { Physical_Case, caseID },
            { Physical_LoadBoundary, freqLoadID });
        freqLoadLoad->way_ = m_LoadType->currentIndex() + 1;

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
        // 挂接该工况聚合的全部载荷（工况组合时不止一个）
        for (int loadID : forceLoadIDs)
        {
            getCVM->getModelData()->getMapRelationManager().appendMapRelation(
                { Physical_LoadBoundary ,freqLoadID }, { Physical_LoadBoundary ,loadID });
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("创建 Mx 工况：%1").arg(name));
    }

    void mCaseImplicitRandomVibrationWindow::createRandomVibrationAnalysis()
    {
        if (!m_SpectrumMatrix_Table || !m_CaseLabel_Table || m_CaseLabel_Table->rowCount() <= 0)
        {
            return;
        }

        // 创建随机振动分析工况
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        MechCaseData::mCaseImplicitRandomVibration* caseData = new MechCaseData::mCaseImplicitRandomVibration(caseID, ImplicitRandomVibration, QStringLiteral("随机振动"));
        getCVM->getModelData()->addPhysicalField<mCaseBase>(caseData);

        // 先把所有 PSD 曲线添加到物理场曲线数据（Physical_Function）
        QMap<QString, int> curveFuncIDs;
        for (auto it = m_psdCurveMap.begin(); it != m_psdCurveMap.end(); ++it)
        {
            const IncentiveCurve& curve = it.value();
            int funcID = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).createID();
            MechFunctionData::mFunctionBase* func = new MechFunctionData::mFunctionBase(funcID, it.key());
            getCVM->getModelData()->addPhysicalField<MechFunctionData::mFunctionBase>(func);
            func->setFunctionType(2);
            MechFunctionData::InterpCol& fundata = func->interp();
            fundata.extrapMode = 1;
            fundata.interpMode = 1;
            for (int i = 0; i < curve.colX.size() && i < curve.colY.size(); ++i)
            {
                fundata.x.push_back(curve.colX[i]);
                fundata.y.push_back(curve.colY[i]);
            }
            curveFuncIDs.insert(it.key(), funcID);
        }

        // 写随机振动功率谱参数表格：第一/二列取工况标签列表中序号对应的标签，
        // 对角线写一行（实部因子，虚部因子0），上三角写实部、虚部两行
        const int n = qMin(m_CaseLabel_Table->rowCount(), m_SpectrumMatrix_Table->rowCount());
        const QString noneCurve = QStringLiteral("无");
        auto labelOfRow = [this](int row) -> QString
        {
            QTableWidgetItem* item = m_CaseLabel_Table->item(row, 1);
            return item ? item->text() : QString();
        };
        auto appendRandPsRow = [this, caseData, caseID](const QString& label1, const QString& label2,
            const QString& realFactor, const QString& imagFactor, const QString& curveName, const QMap<QString, int>& funcIDs)
        {
            MechCaseData::RandomCase_RandPs row;
            row._x = realFactor.trimmed().isEmpty() ? QStringLiteral("0") : realFactor;
            row._y = imagFactor.trimmed().isEmpty() ? QStringLiteral("0") : imagFactor;
            caseData->_tableXYData.append(row);
            const int rowIndex = caseData->_tableXYData.size() - 1;
            // 第一、二列按标签查找对应工况ID，曲线列关联物理场曲线；
            // case1 用 append 创建该行映射条目，case2/曲线用 update 填充同一条目（与 mIMxFile 读入逻辑一致）
            const int subCase1 = getCVM->getModelData()->getPhyIDByName(Physical_Case, label1);
            const int subCase2 = getCVM->getModelData()->getPhyIDByName(Physical_Case, label2);
            getCVM->getModelData()->getMapRelationManager().appendTableMapRelation(
                { Physical_Case, caseID }, { Physical_Case, subCase1 }, rowIndex, MapRelation_TableCase1);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation(
                { Physical_Case, caseID }, { Physical_Case, subCase2 }, rowIndex, MapRelation_TableCase2);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation(
                { Physical_Case, caseID }, { Physical_Case, funcIDs.value(curveName, -1) }, rowIndex, MapRelation_TableFunction);
        };

        for (int row = 0; row < n; ++row)
        {
            const QString label1 = labelOfRow(row);
            for (int col = row; col < n; ++col)
            {
                const QString label2 = labelOfRow(col);
                QTableWidgetItem* item = m_SpectrumMatrix_Table->item(row, col);
                if (!item)
                {
                    continue;
                }
                const QString realCurve = item->data(SpectrumRoleRealCurve).toString();
                const QString imagCurve = item->data(SpectrumRoleImagCurve).toString();
                const QString realFactor = item->data(SpectrumRoleRealFactor).toString();
                const QString imagFactor = item->data(SpectrumRoleImagFactor).toString();

                // 实部行：对角线与上三角均写
                if (!realCurve.isEmpty() && realCurve != noneCurve)
                {
                    appendRandPsRow(label1, label2, realFactor, QStringLiteral("0"), realCurve, curveFuncIDs);
                }
                // 虚部行：仅上三角写
                if (row < col && !imagCurve.isEmpty() && imagCurve != noneCurve)
                {
                    appendRandPsRow(label1, label2, QStringLiteral("0"), imagFactor, imagCurve, curveFuncIDs);
                }
            }
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QStringLiteral("创建随机振动分析工况：随机振动"));
    }

    int mCaseImplicitRandomVibrationWindow::createFrequencyPoint(const QString& name)
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

    void mCaseImplicitRandomVibrationWindow::slot_onExportFem()
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

        const int setID_SoundPressure = 1;
        const int setID_Vibration = 2;
        const int randpsID = 111;
        const int frequencyID = 112;
        const int methodID = 113;
        const int fluidMethodID = 114;
        const int spcID = 115;
        const int sdampingID = 40;
        const int fluidDampingID = 60;
        const int tabled1ID = 1;

        const bool acousticCoupling = (m_checkBox_AcousticSolidCoupling && m_checkBox_AcousticSolidCoupling->isChecked());

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

        // 数值字段（控制在 8 字符内）
        auto numStr = [](double v) { return QString::number(v, 'g', 6); };

        // 响应节点集合：$HMSET 元信息 + SET 卡（每条续行 8 个 ID）
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

        // 频响工况行：节点 + DOF + 载荷类型（力/位移/速度/加速度）
        struct FreqCaseRow {
            int nodeId = 0;
            int dof = 0;
            QString type;      // LOAD / DISP / VELO / ACCE
            QString label;     // 工况标签（组合名或 节点+DOF）
        };
        QList<FreqCaseRow> caseRows;
        if (m_FrequencyResponse_Table)
        {
            const QStringList dofNames = { "X", "Y", "Z", "RX", "RY", "RZ" };
            for (int row = 0; row < m_FrequencyResponse_Table->rowCount(); ++row)
            {
                QTableWidgetItem* idItem = m_FrequencyResponse_Table->item(row, 0);
                QTableWidgetItem* typeItem = m_FrequencyResponse_Table->item(row, 1);
                QTableWidgetItem* dofItem = m_FrequencyResponse_Table->item(row, 2);
                if (!idItem || !typeItem || !dofItem)
                {
                    continue;
                }
                bool ok = false;
                int nodeId = idItem->text().trimmed().toInt(&ok);
                if (!ok || nodeId <= 0)
                {
                    continue;
                }
                const int dof = dofNames.indexOf(dofItem->text().trimmed().toUpper()) + 1;
                if (dof <= 0)
                {
                    continue;
                }
                const QString loadText = typeItem->text().trimmed();
                QString type = QString("DISP");
                if (loadText == QStringLiteral("力"))
                {
                    type = QString("LOAD");
                }
                else if (loadText == QStringLiteral("速度"))
                {
                    type = QString("VELO");
                }
                else if (loadText == QStringLiteral("加速度"))
                {
                    type = QString("ACCE");
                }
                QString label = dofItem->text().trimmed();
                if (m_CaseLabel_Table && m_CaseLabel_Table->item(row, 1))
                {
                    label = m_CaseLabel_Table->item(row, 1)->text().trimmed();
                }
                if (label.isEmpty())
                {
                    label = idItem->text().trimmed() + "+" + dofItem->text().trimmed();
                }
                FreqCaseRow caseRow;
                caseRow.nodeId = nodeId;
                caseRow.dof = dof;
                caseRow.type = type;
                caseRow.label = label;
                caseRows.append(caseRow);
            }
        }

        if (caseRows.isEmpty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                QString("频响工况设置为空，无法导出 fem 文件。"));
            file.close();
            return;
        }

        // 响应集合数据
        QList<QPair<int, QString>> soundPressureIds = m_SoundPressureResponse ? m_SoundPressureResponse->getTableIds() : QList<QPair<int, QString>>();
        QList<QPair<int, QString>> vibrationIds = m_VibrationResponse ? m_VibrationResponse->getTableIds() : QList<QPair<int, QString>>();

        // 约束（决定是否输出 SPC）
        QList<QPair<int, QString>> spcItems;
        if (m_constraint_NodeTable)
        {
            for (int i = 0; i < m_constraint_NodeTable->rowCount(); ++i)
            {
                auto row = m_constraint_NodeTable->getRowData(i);
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
                spcItems.append(qMakePair(nodeId, dofCode));
            }
        }

        // 阻尼判定
        const bool structModalDamping = (m_checkBox_EnableDamping && m_checkBox_EnableDamping->isChecked()
            && m_ModalDamping && m_ModalDamping->isChecked() && m_ModalDamping_Table && m_ModalDamping_Type);
        const bool fluidModalDamping = (m_checkBox_FluidEnableDamping && m_checkBox_FluidEnableDamping->isChecked()
            && m_FluidModalDamping && m_FluidModalDamping->isChecked() && m_FluidModalDamping_Table && m_FluidModalDamping_Type);

        // ---- 工况控制段（全局命令 + 各工况）----
        out << "$ CASE CONTROL COMMANDS\n";
        out << "ANALYSIS MFREQ\n";
        if (!spcItems.isEmpty())
        {
            out << "SPC=" << spcID << "\n";
        }
        out << "FREQUENCY=" << frequencyID << "\n";
        out << "METHOD(STRUCTURE)=" << methodID << "\n";
        if (acousticCoupling)
        {
            out << "METHOD(FLUID)=" << fluidMethodID << "\n";
        }
        if (structModalDamping)
        {
            out << "SDAMPING(STRUCTURE)=" << sdampingID << "\n";
        }
        if (fluidModalDamping)
        {
            out << "SDAMPING(FLUID)=" << fluidDampingID << "\n";
        }
        out << "RANDOM=" << randpsID << "\n";

        if (m_OutputSoundPressure && m_OutputSoundPressure->isChecked() && !soundPressureIds.isEmpty())
        {
            out << "DISPLACEMENT(PUNCH,PHASE,PSDF) = " << setID_SoundPressure << "\n";
        }
        QString vibrationSetStr = QString::number(setID_Vibration);
        if (m_Outputaccelerated && m_Outputaccelerated->isChecked() && !vibrationIds.isEmpty())
        {
            out << "ACCELERATION(PUNCH,PHASE,PSDF) = " << vibrationSetStr << "\n";
        }
        if (m_OutputSpeed && m_OutputSpeed->isChecked() && !vibrationIds.isEmpty())
        {
            out << "VELOCITY(PUNCH,PHASE,PSDF) = " << vibrationSetStr << "\n";
        }
        if (m_OutputDisplacement && m_OutputDisplacement->isChecked() && !vibrationIds.isEmpty())
        {
            out << "DISPLACEMENT(PUNCH,PHASE,PSDF) = " << vibrationSetStr << "\n";
        }

        out << "$ SUBCASE List\n";
        for (int i = 0; i < caseRows.size(); ++i)
        {
            int subcaseID = i + 1;
            int dloadID = 90 + i;
            out << "$HMNAME LOADSTEP" << QString::number(subcaseID).rightJustified(16)
                << "\"" << caseRows.at(i).label << "\"\n";
            out << "SUBCASE " << QString::number(subcaseID).rightJustified(8) << "\n";
            out << "  LABEL " << caseRows.at(i).label << "\n";
            out << "DLOAD = " << QString::number(dloadID).rightJustified(8) << "\n";
        }

        // ---- 批数据段：控制参数用逗号格式，数据卡为 8 字符定宽小场格式 ----
        out << "$ Bulk Data Cards\n";
        out << "BEGIN BULK\n";

        writeResponseSet(setID_SoundPressure, "GRID", "MIC", soundPressureIds);
        writeResponseSet(setID_Vibration, "GRID", "Vel", vibrationIds);

        out << "PARAM,AMSES,YES\n";
        out << "PARAM,AUTOSPC,YES\n";
        out << "PARAM,COUPMASS,YES\n";
        out << "PARAM,FASTFR,YES\n";

        if (m_checkBox_EnableDamping && m_checkBox_EnableDamping->isChecked()
            && m_StructuralDamping && m_StructuralDamping->isChecked() && m_editStructuralDamping)
        {
            QString gText = m_editStructuralDamping->text().trimmed();
            if (!gText.isEmpty())
            {
                out << "PARAM,G," << gText << "\n";
            }
        }
        if (m_checkBox_FluidEnableDamping && m_checkBox_FluidEnableDamping->isChecked()
            && m_FluidStructuralDamping && m_FluidStructuralDamping->isChecked() && m_FluideditStructuralDamping)
        {
            QString gflText = m_FluideditStructuralDamping->text().trimmed();
            if (!gflText.isEmpty())
            {
                out << "PARAM,GFL," << gflText << "\n";
            }
        }

        // 声固耦合（ACMODL）：面单元外法线/横向延伸比例
        if (acousticCoupling)
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

        // ---- 随机功率谱矩阵（RANDPS）：上三角格子，实部/虚部分开出卡，PSD 曲线对应 TABRND1 ----
        QMap<QString, int> tabrndIds;
        auto tabrndIdFor = [&](const QString& curveName) -> int {
            if (curveName.isEmpty() || curveName == QStringLiteral("无") || !m_psdCurveMap.contains(curveName))
            {
                return 0;
            }
            if (!tabrndIds.contains(curveName))
            {
                tabrndIds.insert(curveName, 700 + tabrndIds.size());
            }
            return tabrndIds.value(curveName);
        };

        out << "$ Random Power Spectral Matrix (RANDPS)\n";
        out << "$HMNAME LOADCOL" << QString::number(randpsID).rightJustified(17)
            << "\"RANDPS\"\n";
        const QString noneCurve = QStringLiteral("无");
        if (m_SpectrumMatrix_Table)
        {
            const int n = m_SpectrumMatrix_Table->rowCount();
            for (int j = 0; j < caseRows.size() && j < n; ++j)
            {
                for (int k = j; k < caseRows.size() && k < n; ++k)
                {
                    const QTableWidgetItem* item = m_SpectrumMatrix_Table->item(j, k);
                    if (!item)
                    {
                        continue;
                    }
                    const QString realCurve = item->data(SpectrumRoleRealCurve).toString();
                    const QString imagCurve = item->data(SpectrumRoleImagCurve).toString();
                    const int realTid = tabrndIdFor(realCurve);
                    const int imagTid = tabrndIdFor(imagCurve);
                    if (realTid > 0)
                    {
                        QString xStr = item->data(SpectrumRoleRealFactor).toString().trimmed();
                        writeBulkCard({ "RANDPS", QString::number(randpsID),
                            QString::number(j + 1), QString::number(k + 1),
                            xStr.isEmpty() ? QString("1.0") : xStr, QString("0.0"),
                            QString::number(realTid) });
                    }
                    if (imagTid > 0)
                    {
                        QString yStr = item->data(SpectrumRoleImagFactor).toString().trimmed();
                        writeBulkCard({ "RANDPS", QString::number(randpsID),
                            QString::number(j + 1), QString::number(k + 1),
                            QString("0.0"), yStr.isEmpty() ? QString("1.0") : yStr,
                            QString::number(imagTid) });
                    }
                }
            }
        }

        // ---- 约束 ----
        out << "$ SPC Data (constraints)\n";
        for (const auto& spc : spcItems)
        {
            writeBulkCard({ "SPC", QString::number(spcID), QString::number(spc.first), spc.second, "0.0" });
        }

        // ---- 载荷：每行频响工况一组（力→DAREA，位移/速度/加速度→SPCD）+ RLOAD1 ----
        out << "$ Loads and Boundary Conditions\n";
        for (int i = 0; i < caseRows.size(); ++i)
        {
            const FreqCaseRow& caseRow = caseRows.at(i);
            int exciteID = 500 + i;
            int rload1ID = 600 + i;

            out << "$ Excitation node " << caseRow.nodeId << " DOF " << caseRow.dof
                << " type " << caseRow.type
                << " (SPCD/DAREA " << exciteID << " / RLOAD1 " << rload1ID << ")\n";
            if (caseRow.type == QString("LOAD"))
            {
                writeBulkCard({ "DAREA", QString::number(exciteID), QString::number(caseRow.nodeId),
                    QString::number(caseRow.dof), "1.0" });
            }
            else
            {
                writeBulkCard({ "SPCD", QString::number(exciteID), QString::number(caseRow.nodeId),
                    QString::number(caseRow.dof), "1.0" });
            }
            writeBulkCard({ "RLOAD1", QString::number(rload1ID), QString::number(exciteID),
                "", "", QString::number(tabled1ID), "0", caseRow.type });
        }

        // ---- 特征值 ----
        out << "$ EIGENVALUE EXTRACTION (Structure)\n";
        {
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
            const QString amp = (m_checkBox_ModeScale && m_checkBox_ModeScale->isChecked() && m_lineEdit_ModeScale)
                ? QString::number(m_lineEdit_ModeScale->text().trimmed().toDouble(), 'f', 1) : QString();

            if (solverText == "兰索斯")
            {
                out << "$HMNAME LOADCOL" << QString::number(methodID).rightJustified(17)
                    << "\"EIGRL\"\n";
                writeBulkCard({ "EIGRL", QString::number(methodID), v1, v2, nd });
            }
            else
            {
                out << "$HMNAME LOADCOL" << QString::number(methodID).rightJustified(17)
                    << "\"EIGRA\"\n";
                writeBulkCard({ "EIGRA", QString::number(methodID), v1, v2, nd,
                    amp.isEmpty() ? QString() : amp, "MASS" });
            }

            if (acousticCoupling)
            {
                out << "$ EIGENVALUE EXTRACTION (Acoustic)\n";
                QString fluidFreqMin = m_lineEdit_FluidFreqMin ? m_lineEdit_FluidFreqMin->text().trimmed() : QString();
                QString fluidFreqMax = m_lineEdit_FluidFreqMax ? m_lineEdit_FluidFreqMax->text().trimmed() : QString();
                bool fokMin = false, fokMax = false;
                double ffMin = fluidFreqMin.toDouble(&fokMin);
                double ffMax = fluidFreqMax.toDouble(&fokMax);
                if (!fokMin) ffMin = 0.0;
                if (!fokMax) ffMax = 0.0;
                out << "$HMNAME LOADCOL" << QString::number(fluidMethodID).rightJustified(17)
                    << "\"EIGRA\"\n";
                writeBulkCard({ "EIGRA", QString::number(fluidMethodID),
                    QString::number(ffMin, 'f', 1), QString::number(ffMax, 'f', 1),
                    amp.isEmpty() ? QString() : amp, "MASS" });
            }
        }

        // ---- 频率 ----
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

        // ---- 每工况 DLOAD ----
        out << "$ DLOAD cards\n";
        for (int i = 0; i < caseRows.size(); ++i)
        {
            int dloadID = 90 + i;
            int rload1ID = 600 + i;
            out << "$HMNAME LOADCOL" << QString::number(dloadID).rightJustified(17)
                << "\"DLOAD_" << caseRows.at(i).label << "\"\n";
            writeBulkCard({ "DLOAD", QString::number(dloadID), "1.0", "1.0",
                QString::number(rload1ID) });
        }

        // ---- 激励幅值表（常数 1.0）----
        out << "$HMNAME LOADCOL" << QString::number(tabled1ID).rightJustified(17)
            << "\"TABLED1_" << tabled1ID << "\"\n";
        writeBulkCard({ "TABLED1", QString::number(tabled1ID), "LINEAR", "LINEAR" });
        writeBulkCard({ "+", "0.0", "1.0", "500.0", "1.0", "ENDT" });

        // ---- 阻尼表 ----
        out << "$ TABULAR FUNCTIONS\n";
        if (structModalDamping)
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
        if (fluidModalDamping)
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

        // ---- PSD 功率谱表（TABRND1）：仅输出 RANDPS 引用到的曲线 ----
        for (auto it = tabrndIds.constBegin(); it != tabrndIds.constEnd(); ++it)
        {
            const IncentiveCurve& curve = m_psdCurveMap.value(it.key());
            out << "$HMNAME LOADCOL" << QString::number(it.value()).rightJustified(17)
                << "\"" << it.key() << "\"\n";
            writeBulkCard({ "TABRND1", QString::number(it.value()), "LINEAR", "LINEAR" });
            const int pairCount = curve.colX.size();
            for (int p = 0; p < pairCount; p += 4)
            {
                QStringList fields;
                fields << "+";
                for (int k = p; k < qMin(p + 4, pairCount); ++k)
                {
                    fields << numStr(curve.colX.at(k))
                        << numStr((k < curve.colY.size()) ? curve.colY.at(k) : 0.0);
                }
                if (p + 4 >= pairCount)
                {
                    fields << "ENDT";
                }
                writeBulkCard(fields);
            }
            if (pairCount == 0)
            {
                writeBulkCard({ "+", "0.0", "0.0", "500.0", "0.0", "ENDT" });
            }
        }

        out << "ENDDATA\n";
        file.close();

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("导出 fem 文件：%1").arg(fileName));
    }

    void mCaseImplicitRandomVibrationWindow::slot_onAddConstraintRow()
    {
        if (!m_constraint_NodeTable)
        {
            return;
        }
        m_constraint_NodeTable->addRow();
        m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
    }

    void mCaseImplicitRandomVibrationWindow::slot_onRemoveConstraintRow()
    {
        if (!m_constraint_NodeTable)
        {
            return;
        }
        m_constraint_NodeTable->removeSelectedRows();
        m_constraint_PointCount->setText(QString::number(m_constraint_NodeTable->rowCount()));
    }

    void mCaseImplicitRandomVibrationWindow::slot_onRemoveModalDampingRow()
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

    void mCaseImplicitRandomVibrationWindow::slot_onRemoveFluidModalDampingRow()
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

    void mCaseImplicitRandomVibrationWindow::slot_onAddIncentiveRow()
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

    void mCaseImplicitRandomVibrationWindow::slot_onAddModalDampingRow()
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

    void mCaseImplicitRandomVibrationWindow::slot_onAddFluidModalDampingRow()
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

    void mCaseImplicitRandomVibrationWindow::connectTableCheckBoxes(IncentiveNodeTableWidget* table, int pageIndex)
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
                        if(table == m_Incentive_NodeTable)
                            connect(cb, &QCheckBox::toggled, this, [this]() {
                            slot_onIncentiveSettingTableChanged();
                                });
                    }
                }
            }
        }
    }

    void mCaseImplicitRandomVibrationWindow::slot_ImportIncentiveFromFile()
    {
       
    }

    void mCaseImplicitRandomVibrationWindow::slot_onConstraintPointCountChanged()
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

    void mCaseImplicitRandomVibrationWindow::slot_onIncentivePointCountChanged()
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
        slot_onIncentiveSettingTableChanged();
    }

    void mCaseImplicitRandomVibrationWindow::slot_onModalDampingPointCountChanged()
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
    
    void mCaseImplicitRandomVibrationWindow::slot_onFluidModalDampingPointCountChanged()
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

    void mCaseImplicitRandomVibrationWindow::slot_onImportConstraintFile()
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

    void mCaseImplicitRandomVibrationWindow::slot_onExportConstraintFile()
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

    void mCaseImplicitRandomVibrationWindow::slot_onExportIncentiveFile()
    {
       
    }

    void mCaseImplicitRandomVibrationWindow::slot_onImportDampingFile()
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

    void mCaseImplicitRandomVibrationWindow::slot_onExportDampingFile()
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

    void mCaseImplicitRandomVibrationWindow::slot_onImportFluidDampingFile()
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

    void mCaseImplicitRandomVibrationWindow::slot_onExportFluidDampingFile()
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

    void mCaseImplicitRandomVibrationWindow::updateConstraintDeleteButtonState()
    {
        bool hasSelected = m_constraint_NodeTable && m_constraint_NodeTable->selectedItems().count() > 0;
        if (m_constraint_RemoveRow)
        {
            m_constraint_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitRandomVibrationWindow::updateIncentiveDeleteButtonState()
    {
        bool hasSelected = m_Incentive_NodeTable && m_Incentive_NodeTable->selectedItems().count() > 0;
        if (m_Incentive_RemoveRow)
        {
            m_Incentive_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitRandomVibrationWindow::updateModalDampingDeleteButtonState()
    {
        bool hasSelected = m_ModalDamping_Table && m_ModalDamping_Table->selectedItems().count() > 0;
        if (m_ModalDamping_RemoveRow)
        {
            m_ModalDamping_RemoveRow->setEnabled(hasSelected);
        }
    }

    void mCaseImplicitRandomVibrationWindow::updateFluidModalDampingDeleteButtonState()
    {
        bool hasSelected = m_FluidModalDamping_Table->selectedItems().count() > 0;
        if (m_FluidModalDamping_RemoveRow)
        {
            m_FluidModalDamping_RemoveRow->setEnabled(hasSelected);
        }
    }
}
