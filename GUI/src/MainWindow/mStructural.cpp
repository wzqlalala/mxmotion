#include "mStructural.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//label
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMapRelationManager.h"
#include "mDiscreteField.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

#include <QFormLayout>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QStackedWidget>
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace mxr;
using namespace MechPreData;
using namespace MechDiscreteFieldData;

namespace MechMainWindow
{
    mStructual::mStructual(QWidget* parent) : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);
        _mainLayout->setSpacing(0);

        // 标题
        QLabel* titleLabel = new QLabel("结构优化问题向导", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(
            "QLabel {"
            "  color: black;"
            "  font-size: 20px;"         
            "}"
        );
        _mainLayout->addWidget(titleLabel);

        // 自定义 Tab 按钮
        initTabButtons();

        // 堆叠页面
        _stackedWidget = new QStackedWidget(this);
        initDesignSpaceTab();
        initOptimizationProblemTab();
        initOptimizationSolverTab();
        _mainLayout->addWidget(_stackedWidget);
        _mainLayout->addStretch();

        // 只控制全局字体颜色
        this->setStyleSheet("QWidget { color: black; }");

        // 默认选中第一个
        switchTab(0);
    }

    void mStructual::initTabButtons()
    {
        _tabButtonContainer = new QWidget(this);
        _tabButtonContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _tabButtonContainer->setFixedHeight(24);

        _tabButtonContainer->setStyleSheet(
            "background: #D8DFE6;"
            "border-radius: 5px;"
        );

        QHBoxLayout* tabLayout = new QHBoxLayout(_tabButtonContainer);
        tabLayout->setContentsMargins(0, 0, 0, 0);
        tabLayout->setSpacing(0);

        _tabDesignSpaceBtn = new QPushButton("设计空间", this);
        _tabProblemBtn = new QPushButton("优化问题", this);
        _tabSolverBtn = new QPushButton("优化求解", this);

        QList<QPushButton*> buttons = { _tabDesignSpaceBtn, _tabProblemBtn, _tabSolverBtn };
        for (int i = 0; i < buttons.size(); ++i)
        {
            QPushButton* btn = buttons[i];
            btn->setCheckable(true);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            btn->setFixedHeight(24);
            btn->setCursor(Qt::PointingHandCursor);

            connect(btn, &QPushButton::clicked, this, [this, i]() {
                switchTab(i);
                });

            QString leftRadius = (i == 0) ? "5px" : "0px";
            QString rightRadius = (i == 2) ? "5px" : "0px";

            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background: #D8DFE6;"
                "  color: #333333;"
                "  border: none;"
                "  border-top-left-radius: %1;"
                "  border-bottom-left-radius: %1;"
                "  border-top-right-radius: %2;"
                "  border-bottom-right-radius: %2;"
                "  padding: 0px 8px;"
                "  font-size: 13px;"
                "}"
                "QPushButton:checked {"
                "  background: #F0F3F6;"
                "  color: black;"
                "}"
                "QPushButton:hover:!checked {"
                "  background: #E0E5EA;"
                "}"
            ).arg(leftRadius, rightRadius));

            tabLayout->addWidget(btn);
        }

        _mainLayout->addWidget(_tabButtonContainer);
    }

    void mStructual::switchTab(int index)
    {
        if (!_stackedWidget || !_tabDesignSpaceBtn || !_tabProblemBtn || !_tabSolverBtn)
            return;

        _stackedWidget->setCurrentIndex(index);
        _tabDesignSpaceBtn->setChecked(index == 0);
        _tabProblemBtn->setChecked(index == 1);
        _tabSolverBtn->setChecked(index == 2);
    }

    void mStructual::newMyPanel()
    {
        onResetWizard();
        populateStaticCaseCombos();
      
        updateDesignSpaceTableForType(_designVariableTypeCombo->currentIndex());
    }

    void mStructual::uneditMyPanel()
    {
    }

    void mStructual::initDesignSpaceTab()
    {
        QWidget* tab = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(tab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // 优化定义
        FoldSection* optDefSection = new FoldSection("优化定义", this);
        _componentTypeCombo = new QComboBox(this);
        _componentTypeCombo->addItem("壳截面厚度");
        optDefSection->addRow(new PanelLabel("类型"), _componentTypeCombo);
        layout->addWidget(optDefSection);

        // 参数定义
        FoldSection* paramSection = new FoldSection("参数定义", this);
        _designVariableTypeCombo = new QComboBox(this);
        _designVariableTypeCombo->addItems({ "连续", "离散" });
        paramSection->addRow(new PanelLabel("参数类型"), _designVariableTypeCombo);

        _discreteValueBtn = new QPushButton("厚度离散取值表", this);
        _discreteValueBtn->setStyleSheet("QPushButton {  border: 1px solid #D0D5DA; }" "QPushButton:Hover{""background: #F0F3F6;""}");
        _discreteValueBtn->setVisible(false);
        paramSection->addRow(_discreteValueBtn);

        _designSpaceTable = new QTableWidget(this);
        _designSpaceTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _designSpaceTable->setMinimumHeight(120);
        _designSpaceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _designSpaceTable->setSelectionBehavior(QAbstractItemView::SelectItems);
        _designSpaceTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        paramSection->addRow(_designSpaceTable);
        layout->addWidget(paramSection);

        // 优化算法
        FoldSection* algoSection = new FoldSection("优化算法", this);
        _solverTypeCombo = new QComboBox(this);
        _solverTypeCombo->addItems({ "默认","遗传算法" });
        algoSection->addRow(new PanelLabel("优化算法"), _solverTypeCombo);

        _maxSampleCountEdit = new QLineEdit(this);
        _maxSampleCountEdit->setStyleSheet("QLineEdit { border: 1px solid #D0D5DA; }");
        algoSection->addRow(new PanelLabel("最大样本数量"), _maxSampleCountEdit);

        layout->addWidget(algoSection);
        layout->addStretch();

        _stackedWidget->addWidget(tab);

        connect(_designVariableTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mStructual::onDesignVariableTypeChanged);
        connect(_discreteValueBtn, &QPushButton::clicked, this, &mStructual::onShowDiscreteValueTable);

        updateDesignSpaceTableForType(0);
    }

    void mStructual::updateDesignSpaceTableForType(int type)
    {
        bool isDiscrete = (type == 1);

        _discreteValueBtn->setVisible(isDiscrete);

        if (isDiscrete)
        {
            _designSpaceTable->setColumnCount(3);
            _designSpaceTable->setHorizontalHeaderLabels({ "优化截面", "当前", "取值范围" });
            _designSpaceTable->setRowCount(3);
            for (int i = 0; i < 3; ++i)
            {
                QComboBox* sectionCombo = new QComboBox(_designSpaceTable);
                populateShellSectionCombo(sectionCombo);
                sectionCombo->setCurrentIndex(i);
                _designSpaceTable->setCellWidget(i, 0, sectionCombo);

                QLineEdit* curEdit = new QLineEdit(" ", _designSpaceTable);
                curEdit->setAlignment(Qt::AlignCenter);
                curEdit->setStyleSheet("QLineEdit {  border: 1px solid #D0D5DA; }");
                _designSpaceTable->setCellWidget(i, 1, curEdit);

                QComboBox* rangeCombo = new QComboBox(_designSpaceTable);
                rangeCombo->addItems({ "Range1", "Range2", "Range3" });
                rangeCombo->setCurrentIndex(i);
                rangeCombo->setStyleSheet("QComboBox {  border: 1px solid #D0D5DA; }");
                _designSpaceTable->setCellWidget(i, 2, rangeCombo);
            }
        }
        else
        {
            _designSpaceTable->setColumnCount(4);
            _designSpaceTable->setHorizontalHeaderLabels({ "优化截面", "当前", "最小", "最大" });
            _designSpaceTable->setRowCount(3);
            for (int i = 0; i < 3; ++i)
            {
                QComboBox* sectionCombo = new QComboBox(_designSpaceTable);
                populateShellSectionCombo(sectionCombo);
                sectionCombo->setCurrentIndex(i);
                _designSpaceTable->setCellWidget(i, 0, sectionCombo);

                for (int col = 1; col < 4; ++col)
                {
                    QLineEdit* edit = new QLineEdit(" ", _designSpaceTable);
                    edit->setAlignment(Qt::AlignCenter);
                    edit->setStyleSheet("QLineEdit {  border: 1px solid #D0D5DA; }");
                    _designSpaceTable->setCellWidget(i, col, edit);
                }
            }
        }
    }

    void mStructual::initOptimizationProblemTab()
    {
        QWidget* tab = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(tab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // 优化目标
        FoldSection* objSection = new FoldSection("优化目标", this);
        _objectiveResponseCombo = new QComboBox(this);
        _objectiveResponseCombo->addItems({
            "最大化刚度（柔度）",
            "最大化频率",
            "最小化位移",
            "最大化综合刚度（综合柔度）",
            "最大化综合频率",
            "最大化柔度指数",
            "最小化应力",
            "最小化质量"
            });
        objSection->addRow(new PanelLabel("目标响应"), _objectiveResponseCombo);
        _objectiveCaseCombo = new QComboBox(this);
        objSection->addRow(new PanelLabel("目标工况"), _objectiveCaseCombo);
        layout->addWidget(objSection);

        // 优化约束
        FoldSection* constraintSection = new FoldSection("优化约束", this);
        QWidget* constraintWidget = new QWidget(this);
        constraintWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QGridLayout* grid = new QGridLayout(constraintWidget);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(4);
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 0);
        grid->setColumnStretch(2, 1);

        // 质量约束
        _massConstraintCheck = new QCheckBox(this);
        _massConstraintCheck->setChecked(false);
        QLabel* massLabel = new QLabel("质量约束", this);
        massLabel->setCursor(Qt::PointingHandCursor);
        massLabel->installEventFilter(this);
        _labelToCheckMap[massLabel] = _massConstraintCheck;
        _massConstraintEdit = new QLineEdit(this);
        _massConstraintEdit->setEnabled(_massConstraintCheck->isChecked());
        _massConstraintEdit->setStyleSheet("QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(_massConstraintCheck, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(massLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_massConstraintEdit, 0, 2);
        connect(_massConstraintCheck, &QCheckBox::toggled, this, &mStructual::onMassConstraintToggled);

        // 刚度约束
        _stiffnessConstraintCheck = new QCheckBox(this);
        _stiffnessConstraintCheck->setChecked(false);
        QLabel* stiffnessLabel = new QLabel("刚度约束", this);
        stiffnessLabel->setCursor(Qt::PointingHandCursor);
        stiffnessLabel->installEventFilter(this);
        _labelToCheckMap[stiffnessLabel] = _stiffnessConstraintCheck;
        _stiffnessConstraintEdit = new QLineEdit(this);
        _stiffnessConstraintEdit->setEnabled(_stiffnessConstraintCheck->isChecked());
        _stiffnessConstraintEdit->setStyleSheet("QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(_stiffnessConstraintCheck, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(stiffnessLabel, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_stiffnessConstraintEdit, 1, 2);
        connect(_stiffnessConstraintCheck, &QCheckBox::toggled, this, &mStructual::onStiffnessConstraintToggled);

        // 刚度约束 - 工况
        _stiffnessCaseCombo = new QComboBox(this);
        _stiffnessCaseCombo->setEnabled(false);
        _stiffnessCaseCombo->setStyleSheet("QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(new QLabel("工况", this), 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_stiffnessCaseCombo, 2, 2);

        // 应力约束
        _stressConstraintCheck = new QCheckBox(this);
        _stressConstraintCheck->setChecked(false);
        QLabel* stressLabel = new QLabel("应力约束", this);
        stressLabel->setCursor(Qt::PointingHandCursor);
        stressLabel->installEventFilter(this);
        _labelToCheckMap[stressLabel] = _stressConstraintCheck;
        _stressLimitEdit = new QLineEdit(this);
        _stressLimitEdit->setEnabled(_stressConstraintCheck->isChecked());
        _stressLimitEdit->setStyleSheet("QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(_stressConstraintCheck, 3, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(stressLabel, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_stressLimitEdit, 3, 2);
        connect(_stressConstraintCheck, &QCheckBox::toggled, this, &mStructual::onStressConstraintToggled);

        // 应力约束 - 工况
        _stressCaseCombo = new QComboBox(this);
        _stressCaseCombo->setEnabled(false);
        _stressCaseCombo->setStyleSheet("QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(new QLabel("工况", this), 4, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_stressCaseCombo, 4, 2);

        // 频率约束
        _frequencyConstraintCheck = new QCheckBox(this);
        _frequencyConstraintCheck->setChecked(false);
        QLabel* frequencyLabel = new QLabel("频率约束", this);
        frequencyLabel->setCursor(Qt::PointingHandCursor);
        frequencyLabel->installEventFilter(this);
        _labelToCheckMap[frequencyLabel] = _frequencyConstraintCheck;
        _frequencyLimitEdit = new QLineEdit(this);
        _frequencyLimitEdit->setEnabled(_frequencyConstraintCheck->isChecked());
        _frequencyLimitEdit->setStyleSheet("QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(_frequencyConstraintCheck, 5, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(frequencyLabel, 5, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_frequencyLimitEdit, 5, 2);
        connect(_frequencyConstraintCheck, &QCheckBox::toggled, this, &mStructual::onFrequencyConstraintToggled);

        // 频率约束 - 阶数
        _frequencyOrderEdit = new QLineEdit(this);
        _frequencyOrderEdit->setEnabled(_frequencyConstraintCheck->isChecked());
        _frequencyOrderEdit->setStyleSheet("QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(new QLabel("阶数", this), 6, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_frequencyOrderEdit, 6, 2);

        // 频率约束 - 工况
        _frequencyCaseCombo = new QComboBox(this);
        _frequencyCaseCombo->setEnabled(false);
        _frequencyCaseCombo->setStyleSheet("QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        grid->addWidget(new QLabel("工况", this), 7, 1, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(_frequencyCaseCombo, 7, 2);

        constraintSection->addRow(constraintWidget);
        layout->addWidget(constraintSection);
        layout->addStretch();

        _stackedWidget->addWidget(tab);
    }

    bool mStructual::eventFilter(QObject* watched, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            QLabel* label = qobject_cast<QLabel*>(watched);
            if (label && _labelToCheckMap.contains(label))
            {
                QCheckBox* check = _labelToCheckMap[label];
                check->setChecked(!check->isChecked());
                return true;
            }
        }
        return mPanelBase::eventFilter(watched, event);
    }

    void mStructual::initOptimizationSolverTab()
    {
        QWidget* tab = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(tab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // 求解状态
        FoldSection* solverSection = new FoldSection("求解状态", this);
        _currentIterEdit = new QLineEdit(this);
        _currentIterEdit->setStyleSheet("QLineEdit {  border: 1px solid #D0D5DA; }");
        solverSection->addRow(new PanelLabel("当前迭代步"), _currentIterEdit);
        _sampledCountEdit = new QLineEdit(this);
        _sampledCountEdit->setStyleSheet("QLineEdit {  border: 1px solid #D0D5DA; }");
        solverSection->addRow(new PanelLabel("已采样数量"), _sampledCountEdit);
        _progressEdit = new QProgressBar(this);
        _progressEdit->setRange(0, 100);
        _progressEdit->setValue(0);
        _progressEdit->setTextVisible(true);
        _progressEdit->setAlignment(Qt::AlignCenter);
        _progressEdit->setFormat("%p%");
        _progressEdit->setStyleSheet(
            "QProgressBar {"
            "  border: 1px solid #D0D5DA;"
            "  border-radius: 3px;"
            "  background: #FFFFFF;"
            "  text-align: center;"
            "  color: black;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #FFFFFF;"
            "}"
        );
        solverSection->addRow(new PanelLabel("进度百分比"), _progressEdit);
        layout->addWidget(solverSection);

        // 实时监控
        FoldSection* monitorSection = new FoldSection("实时监控", this);
        _monitorTable = new QTableWidget(0, 4, this);
        _monitorTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _monitorTable->setMinimumHeight(120);
        _monitorTable->setHorizontalHeaderLabels({ "序号", "参数组", "优化目标", "优化约束" });
        _monitorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _monitorTable->setSelectionBehavior(QAbstractItemView::SelectItems);
        _monitorTable->verticalHeader()->setVisible(false);

        for (int i = 0; i < 3; ++i)
        {
            _monitorTable->insertRow(i);
            _monitorTable->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(i + 1)));

            QComboBox* groupCombo = new QComboBox(_monitorTable);
            groupCombo->addItems({ "Graup1", "Graup2", "Graup3" });
            groupCombo->setStyleSheet("QComboBox {  border: 1px solid #D0D5DA; }");
            _monitorTable->setCellWidget(i, 1, groupCombo);

            QTableWidgetItem* objItem = new QTableWidgetItem(" ");
            objItem->setBackground(QColor("#F0F3F6"));
            _monitorTable->setItem(i, 2, objItem);

            QTableWidgetItem* conItem = new QTableWidgetItem(" ");
            conItem->setBackground(QColor("#F0F3F6"));
            _monitorTable->setItem(i, 3, conItem);
        }

        monitorSection->addRow(_monitorTable);
        _viewDetailBtn = new QPushButton("查看详细信息", this);
        _viewDetailBtn->setStyleSheet("QPushButton { border: 1px solid #D0D5DA; }" "QPushButton:Hover{""background: #F0F3F6;""}");
        monitorSection->addRow(_viewDetailBtn);
        layout->addWidget(monitorSection);
        layout->addStretch();

        _stackedWidget->addWidget(tab);
    }

    void mStructual::onShowDiscreteValueTable()
    {
        mTableDataWindow::getInstance()->setTableTitle("厚度离散取值表");
        mTableDataWindow::getInstance()->clearAndReset(3, 2);

        mTableWidget* tableWidget = mTableDataWindow::getInstance()->getTableWidget();
        tableWidget->setHorizontalHeaders({ "取值范围", "值" });

        // 仅第 1 列使用无校验 delegate，覆盖全局 NumericItemDelegate
        tableWidget->setItemDelegateForColumn(1, new QStyledItemDelegate(tableWidget));

        for (int r = 0; r < 3; ++r)
        {
            tableWidget->setCellText(r, 0, QString("Range%1").arg(r + 1));
            tableWidget->setCellText(r, 1, " ");
        }

        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mStructual::onDesignVariableTypeChanged(int index)
    {
        updateDesignSpaceTableForType(index);
    }

    void mStructual::onMassConstraintToggled(bool checked)
    {
        _massConstraintEdit->setEnabled(checked);
        _massConstraintEdit->setStyleSheet(checked
            ? "QLineEdit { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
    }

    void mStructual::onStiffnessConstraintToggled(bool checked)
    {
        _stiffnessConstraintEdit->setEnabled(checked);
        _stiffnessCaseCombo->setEnabled(checked);
        _stiffnessConstraintEdit->setStyleSheet(checked
            ? "QLineEdit { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        _stiffnessCaseCombo->setStyleSheet(checked
            ? "QComboBox { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");

        if (checked)
        {
            _stiffnessCaseCombo->clear();
            _stiffnessCaseCombo->addItems(getStaticCaseNames());
        }
        else
        {
            _stiffnessCaseCombo->clear();
        }
    }

    void mStructual::onStressConstraintToggled(bool checked)
    {
        _stressLimitEdit->setEnabled(checked);
        _stressCaseCombo->setEnabled(checked);
        _stressLimitEdit->setStyleSheet(checked
            ? "QLineEdit { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        _stressCaseCombo->setStyleSheet(checked
            ? "QComboBox { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");

        if (checked)
        {
            _stressCaseCombo->clear();
            _stressCaseCombo->addItems(getStaticCaseNames());
        }
        else
        {
            _stressCaseCombo->clear();
        }
    }

    void mStructual::onFrequencyConstraintToggled(bool checked)
    {
        _frequencyLimitEdit->setEnabled(checked);
        _frequencyOrderEdit->setEnabled(checked);
        _frequencyCaseCombo->setEnabled(checked);
        _frequencyLimitEdit->setStyleSheet(checked
            ? "QLineEdit { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        _frequencyOrderEdit->setStyleSheet(checked
            ? "QLineEdit { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QLineEdit { background: #F0F3F6; border: 1px solid #D0D5DA; }");
        _frequencyCaseCombo->setStyleSheet(checked
            ? "QComboBox { background: #FFFFFF; border: 1px solid #D0D5DA; }"
            : "QComboBox { background: #F0F3F6; border: 1px solid #D0D5DA; }");

        if (checked)
        {
            _frequencyCaseCombo->clear();
            _frequencyCaseCombo->addItems(getModalCaseNames());
        }
        else
        {
            _frequencyCaseCombo->clear();
        }
    }

    void mStructual::onRunOptimization()
    {
    }

    QStringList mStructual::getStaticCaseNames()
    {
        QStringList names;
        if (!getCVM || !getCVM->getModelData())
            return names;

        auto& caseMgr = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case);
        for (int id : caseMgr.getAllCaseID())
        {
            MechCaseData::mCaseBase* c = caseMgr.getCaseData(id);
            if (c && c->getCaseType() == ImplicitLinearStatic)
                names.append(c->getCaseName());
        }
        return names;
    }

    QStringList mStructual::getModalCaseNames()
    {
        QStringList names;
        if (!getCVM || !getCVM->getModelData())
            return names;

        auto& caseMgr = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case);
        for (int id : caseMgr.getAllCaseID())
        {
            MechCaseData::mCaseBase* c = caseMgr.getCaseData(id);
            if (c && c->getCaseType() == ImplicitModal)
                names.append(c->getCaseName());
        }
        return names;
    }

    void mStructual::onResetWizard()
    {
        _componentTypeCombo->setCurrentIndex(0);
        _designVariableTypeCombo->setCurrentIndex(0);
        updateDesignSpaceTableForType(0);
        _solverTypeCombo->setCurrentIndex(0);
        _maxSampleCountEdit->clear();

        _objectiveResponseCombo->setCurrentIndex(0);
        _objectiveCaseCombo->setCurrentIndex(0);
        _massConstraintCheck->setChecked(false);
        _massConstraintEdit->clear();

        _stiffnessConstraintCheck->setChecked(false);
        _stiffnessConstraintEdit->clear();
        _stiffnessCaseCombo->clear();
        _stiffnessCaseCombo->setEnabled(false);

        _stressConstraintCheck->setChecked(false);
        _stressLimitEdit->clear();
        _stressCaseCombo->clear();
        _stressCaseCombo->setEnabled(false);

        _frequencyConstraintCheck->setChecked(false);
        _frequencyLimitEdit->clear();
        _frequencyOrderEdit->clear();
        _frequencyCaseCombo->clear();
        _frequencyCaseCombo->setEnabled(false);

        _currentIterEdit->clear();
        _sampledCountEdit->clear();
        _progressEdit->setValue(100);
        if (_monitorTable)
        {
            _monitorTable->setRowCount(0);
            for (int i = 0; i < 3; ++i)
            {
                _monitorTable->insertRow(i);
                _monitorTable->verticalHeader()->setVisible(false);
                _monitorTable->setItem(i, 0, new QTableWidgetItem(MxFunctions::mNumberToString(i + 1)));

                QComboBox* groupCombo = new QComboBox(_monitorTable);
                groupCombo->addItems({ "Group1", "Group2", "Group3" });
                groupCombo->setCurrentIndex(i);
                groupCombo->setStyleSheet("QComboBox {  border: 1px solid #D0D5DA; }");
                _monitorTable->setCellWidget(i, 1, groupCombo);

                QTableWidgetItem* objItem = new QTableWidgetItem(" ");
                _monitorTable->setItem(i, 2, objItem);

                QTableWidgetItem* conItem = new QTableWidgetItem(" ");
                _monitorTable->setItem(i, 3, conItem);
            }
        }

        switchTab(0);
    }

    void mStructual::populateShellSectionCombo(QComboBox* combo)
    {
        if (!combo || !getCVM || !getCVM->getModelData())
            return;

        combo->clear();
        auto& propMgr = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property);
        for (int id : getCVM->getModelData()->getAllPhyID(Physical_Property))
        {
            MechPropertyData::mPropertyBase* prop = propMgr.getPropertyData(id);
            if (prop && prop->getPropertyType() == ShellProperty)
                combo->addItem(prop->getPropertyName());
        }
    }

    void mStructual::populateStaticCaseCombos()
    {
        _objectiveCaseCombo->clear();
        _objectiveCaseCombo->addItems(getStaticCaseNames());
    }

   
}
