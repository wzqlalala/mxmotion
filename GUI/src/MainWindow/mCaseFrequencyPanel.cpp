
#include "mCaseFrequencyPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitFrequency.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"
#include "mSetManager.h"
#include "mSetBase.h"

#include "mModelData.h"
#include "mMeshData.h"

#include "mDockLayoutManager.h"

using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseFrequencyPanel::mCaseFrequencyPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "频响分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };
        _typeOfoutputCheckBoxs["mxdb"]["节点贡献量"] = QStringList{ "结构节点贡献量"};

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力",  "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];

        _historyOutputCheckboxs["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _historyOutputCheckboxs["应变"] = QStringList{ "节点应变", "单元应变" };
        _historyOutputCheckboxs["应力"] = QStringList{ "节点应力", "单元应力" };
        _historyKdCheckboxs["动刚度结果"] = QStringList{ "平均法", "指数法", "面积法" };
    }

    mCaseFrequencyPanel::~mCaseFrequencyPanel()
    {

    }

    void mCaseFrequencyPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitFrequency(caseID, ImplicitFrequency);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseFrequencyPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitFrequency>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // =========================================================================
        // 1. 求解器控制 (Solver Control)
        // =========================================================================
        auto foldSolver = new FoldSection("求解控制", this);
        m_foldWidgets.append(foldSolver);
        _mainLayout->addWidget(foldSolver);

        auto comboType = new QComboBox();
        comboType->addItems({ "直接法", "模态法" });

        //auto checkPreload = new QCheckBox("预应力状态");
        auto comboPreload = new QComboBox();
        updatePreloadCombobox(comboPreload);

        //模态方法
        auto lblSolver = new QLabel("模态求解器");
        auto comboSolver = new QComboBox();
        comboSolver->addItems({ "自动多重子结构", "兰索斯" });

        // 残余模态
        auto checkRemnant = new QCheckBox("残余模态");

        // 绑定数据
        binder->bind(comboType, linearData, &mCaseImplicitFrequency::_method);
        binder->bind(comboSolver, linearData, &mCaseImplicitFrequency::_modalSolver);
        binder->bind(checkRemnant, linearData, &mCaseImplicitFrequency::_isRemnantOpen);

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        // 布局添加
        foldSolver->addRow(new QLabel("类型"), comboType);
        foldSolver->addRow(new QLabel("预加载工况"), comboPreload);
        //foldSolver->addRow(lblSolver, comboSolver);
        //foldSolver->addRow(checkRemnant);

        // =========================================================================
        // 频率范围
        // =========================================================================
        auto foldFrequency = new FoldSection("频率范围", this);
        m_foldWidgets.append(foldFrequency);
        _mainLayout->addWidget(foldFrequency);

        // 定义控件
        auto minFrequency = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);
        auto maxFrequency = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);
        auto order = new mCalcLineEdit(mCalcLineEdit::CalcMode::RequireInt, this);
        auto tolerance = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);
        auto ampffact = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this);
        auto comboNormalization = new QComboBox();
        comboNormalization->addItems({ "无", "广义质量", "最大位移" });

        // 定义标签（为了显隐控制，需要指针）
        auto lblMinFreq = new QLabel("最小频率");
        auto lblMaxFreq = new QLabel("最大频率");
        auto lblOrder = new QLabel("阶数");
        auto lblTolerance = new QLabel("容差");
        auto lblAmpffact = new QLabel("放大系数");
        auto lblNormalization = new QLabel("特征向量");

        // 添加到折叠块
        foldFrequency->addRow(lblMinFreq, minFrequency);
        foldFrequency->addRow(lblMaxFreq, maxFrequency);
        foldFrequency->addRow(lblOrder, order);
        foldFrequency->addRow(lblTolerance, tolerance);
        foldFrequency->addRow(lblAmpffact, ampffact);
        foldFrequency->addRow(lblNormalization, comboNormalization);

        // 绑定数据
        binder->bind(minFrequency, linearData, &mCaseImplicitFrequency::_minFrequency);
        binder->bind(maxFrequency, linearData, &mCaseImplicitFrequency::_maxFrequency);
        binder->bind(order, linearData, &mCaseImplicitFrequency::_order);
        binder->bind(tolerance, linearData, &mCaseImplicitFrequency::_tolerance);
        binder->bind(ampffact, linearData, &mCaseImplicitFrequency::_ampffact);
        binder->bind(comboNormalization, linearData, &mCaseImplicitFrequency::_normalization);

        // =========================================================================
        // 阻尼参数 (Damping Parameters)
        // =========================================================================
        auto foldDamping = new FoldSection("阻尼参数", this);
        m_foldWidgets.append(foldDamping);
        _mainLayout->addWidget(foldDamping);

        auto dampingG = new mCalcLineEdit(this, true, false, true);
        auto dampingAlpha1 = new mCalcLineEdit(this, true, false);
        auto dampingAlpha2 = new mCalcLineEdit(this, true, false);

        auto comboFunction = new QComboBox();
        updateFunctionCombobox(comboFunction);

        auto btnFunction = new QToolButton();
        btnFunction->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        foldDamping->addRow(new QLabel("整体结构阻尼"), dampingG);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha1"), dampingAlpha1);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha2"), dampingAlpha2);
        foldDamping->addRow(new QLabel("模态阻尼曲线"), comboFunction, btnFunction);

        binder->bind(dampingG, linearData, &mCaseImplicitFrequency::_dampingG);
        binder->bind(dampingAlpha1, linearData, &mCaseImplicitFrequency::_dampingAlpha1);
        binder->bind(dampingAlpha2, linearData, &mCaseImplicitFrequency::_dampingAlpha2);

        // =========================================================================
        // 力输出集合
        // =========================================================================
        auto foldForce = new FoldSection("力输出集合", this);
        m_foldWidgets.append(foldForce);
        _mainLayout->addWidget(foldForce);

        auto lblelForce = new QLabel("单元力集合");
        auto lblgpForce = new QLabel("节点力集合");

        auto comboElForce = new QComboBox(); updateSetCombobox(comboElForce, {SetType::Element}, MapRelation_ElforceSet);
        auto comboGpForce = new QComboBox(); updateSetCombobox(comboGpForce, { SetType::Node }, MapRelation_GpforceSet);

        foldForce->addRow(lblelForce, comboElForce);
        foldForce->addRow(lblgpForce, comboGpForce);

        auto freqSampPointFold = new FoldSection("采样频率点", this);

        tableBtn = new singleCheckBox();
        tableBtn->initWidget("名称");
        tableBtn->tablebtn->setEnabled(false);
        connect(tableBtn->cb, &QCheckBox::toggled, this, &mCaseFrequencyPanel::slot_btnUse);
        connect(tableBtn->tablebtn, &QPushButton::clicked, this, &mCaseFrequencyPanel::slot_freqSampPointTable);
        freqSampPointFold->addRow(tableBtn->propwid);
        tableBtn->cb->setChecked(linearData->checked);

        m_foldWidgets.append(freqSampPointFold);
        _mainLayout->addWidget(freqSampPointFold);

        table = new mFreqTableSelectorWidget();
        // 连接 新增行
        connect(table, &mFreqTableSelectorWidget::rowAdded,
            this, &mCaseFrequencyPanel::onRowAdded);
        // 连接 删除行
        connect(table, &mFreqTableSelectorWidget::rowRemoved,
            this, &mCaseFrequencyPanel::onRowRemoved);
        // 连接 下拉切换
        connect(table, &mFreqTableSelectorWidget::comboChanged,
            this, &mCaseFrequencyPanel::onComboChanged);

        initOutputSection(linearData->getOutputs());
        _outputFoldWidget->setCheckable(true);

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);
        initPchOutputSection(linearData->getPchOutputs());
        m_foldWidgets.append(_outputPchFoldWidget);
        _mainLayout->addWidget(_outputPchFoldWidget);
        /* 0630版本屏蔽
        initHistoryOutputSection(linearData->getHistoryOutputs(), QHash<QString, QStringList>());
        m_foldWidgets.append(_historyOutputFoldWidget);
        _mainLayout->addWidget(_historyOutputFoldWidget);

        // 绑定历史输出设置下拉框到数据字段（使用基类字段，手动连接）
        _historyOutputControlWidget->m_comboFileType->setCurrentText(linearData->getHstFileType());
        connect(_historyOutputControlWidget->m_comboFileType, &QComboBox::currentTextChanged, this, [=](const QString& val) {
            linearData->setHstFileType(val);
        });
        _historyOutputControlWidget->m_comboDataType->setCurrentText(linearData->getHstDataType());
        connect(_historyOutputControlWidget->m_comboDataType, &QComboBox::currentTextChanged, this, [=](const QString& val) {
            linearData->setHstDataType(val);
        });
        _historyOutputControlWidget->m_dynamicStiffLineEdit->setText(linearData->getHstDynamicStiff());
        connect(_historyOutputControlWidget->m_dynamicStiffLineEdit, &QLineEdit::textChanged, this, [=](const QString& val) {
            linearData->setHstDynamicStiff(val);
        });

        // 更新集合下拉框
        updateSetCombobox(_historyOutputControlWidget->m_comboSet, {}, MapRelation_HistorySet);
        */
        _mainLayout->addStretch();

        // =========================================================================
        // 逻辑控制 (Logic & Connections)
        // =========================================================================

        // 定义Lambda函数统一处理显隐逻辑
        auto updateVisibility = [=]() {
            QString currentType = comboType->currentText();
            QString currentSolver = comboSolver->currentText();
            //bool isPreload = checkPreload->isChecked();

            // 1. 预应力显隐
            //comboPreload->setVisible(isPreload);

            // 2. 根据模态类型控制折叠块显隐
            if (currentType == "模态法")
            {
                // 显示：频率范围(foldOutput)
                // 隐藏：复模态值(foldComplexVal), 阻尼参数(foldDamping)
                foldFrequency->setVisible(true);
                foldForce->setVisible(true);
                // 3. 模态分析下的求解器逻辑控制
                if (currentSolver == "自动多重子结构")
                {
                    // 自动多重子结构：放大系数；隐藏特征向量
                    lblTolerance->setVisible(true);       tolerance->setVisible(true);

                    lblAmpffact->setVisible(true);        ampffact->setVisible(true);
                    foldFrequency->addRow(lblAmpffact, ampffact);
                    lblNormalization->setVisible(false);  comboNormalization->setVisible(false);
                    lblNormalization->setParent(nullptr);
                    comboNormalization->setParent(nullptr);
                }
                else if (currentSolver == "兰索斯")
                {
                    // 兰索斯：放大系数；显示特征向量
                    lblAmpffact->setVisible(false);       ampffact->setVisible(false);
                    lblNormalization->setVisible(true);   comboNormalization->setVisible(true);
                    foldFrequency->addRow(lblNormalization, comboNormalization);
                    lblAmpffact->setParent(nullptr);
                    ampffact->setParent(nullptr);
                }

                {
                    foldSolver->insertRow(2, lblSolver, comboSolver);
                    foldSolver->insertRow(3, checkRemnant);
                    checkRemnant->show();
                    lblSolver->show();
                    comboSolver->show();
                }
            }
            else if (currentType == "直接法")
            {
                foldFrequency->setVisible(false);
                foldForce->setVisible(false);
                {
                    foldSolver->layout()->removeWidget(lblSolver);
                    foldSolver->layout()->removeWidget(comboSolver);
                    foldSolver->layout()->removeWidget(checkRemnant);
                    checkRemnant->hide();
                    lblSolver->hide();
                    comboSolver->hide();
                }
            }

            // 刷新布局防止留白
            foldSolver->refreshLayout();
            foldFrequency->refreshLayout();
            foldDamping->refreshLayout();
            };

        // 信号连接
        // 连接模态类型改变
        connect(comboType, &QComboBox::currentTextChanged, this, updateVisibility);

        // 连接求解器改变
        connect(comboSolver, &QComboBox::currentTextChanged, this, updateVisibility);

        //点击函数新建
        connect(btnFunction, &QAbstractButton::clicked, this, [=]() {
            if (getCVM == nullptr)
            {
                return;
            }
            if (_caseData == nullptr)
            {
                return;
            }
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            updateFunctionCombobox(comboFunction);
            auto& functionManager = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function);

            auto functionIDs = functionManager.getAllFunctionID();
            std::sort(functionIDs.begin(), functionIDs.end());
            if (functionIDs.size() == 0)
            {
                return;
            }
            auto functionData = functionManager.getFunctionData(functionIDs.at(functionIDs.size() - 1));
            if (functionData == nullptr)
            {
                return;
            }
            comboFunction->setCurrentText(functionData->getFunctionName());
            //设置id
            int id = _caseData->getCaseId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Function, functionData->getFunctionId() });
            //emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, id);
            //updateVisibility();
            });

        //函数选择变化
        connect(comboFunction, &QComboBox::currentTextChanged, this, [=](const QString& text) {
            if (getCVM == nullptr)
            {
                return;
            }
            if (_caseData == nullptr)
            {
                return;
            }
            auto& functionManager = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function);
            int functionId = functionManager.getIDByFunctionName(text);
            if (functionId == -1)
            {
                return;
            }
            comboFunction->setCurrentText(text);
            //设置id
            int id = _caseData->getCaseId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Function, functionId });

            });

        // 初始化调用一次以设置初始状态
        updateVisibility();
    }

    void mCaseFrequencyPanel::slot_btnUse(bool checked)
    {
        auto linearData = toType<mCaseImplicitFrequency>(_caseData);
        linearData->checked = checked;
        tableBtn->tablebtn->setEnabled(checked);
        if (!checked)
            mTableDataWindow::getInstance()->resetInitialTable();
    }

    void mCaseFrequencyPanel::slot_freqSampPointTable()
    {
        table->clearTableData();
        //获取所有freq名称
        QStringList names = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getAllFreqSamplPointNames();
        //填充数据
        table->setComboBoxItems(names);

        //获取映射
        int id = _caseData->getCaseId();
        QList<int> freqIDs = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, id }, Physical_FreqSamplPoint);
        //添加行数
        table->addRows(freqIDs.size());
        //设置当前下拉框
        for (int i = 0; i < freqIDs.size(); ++i)
        {
            QString name = getCVM->getModelData()->getManager<mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointNameByID(freqIDs[i]);
            table->setCurrentText(i, name);
        }

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    // 1. 添加行 → 新增映射
    void mCaseFrequencyPanel::onRowAdded(int row)
    {
        int caseid = _caseData->getCaseId();

        //先获取已存在的映射
        QList<int> freqIDs = getCVM->getModelData()->getMapRelationManager()
            .getMapRelation<QList<int>>({ Physical_Case, caseid }, Physical_FreqSamplPoint);

        //已经有映射了，直接返回，不添加！
        if (row < freqIDs.size())
        {
            // 行号已经在映射里 → 不重复添加
            return;
        }

        // 获取当前表格所有选中项
        QStringList selNames = table->getAllSelectedNames();

        // 检查行号是否有效
        if (row < 0 || row >= selNames.size())
            return;

        // 获取当前行的名称
        QString name = selNames[row];
        if (name.isEmpty())
            return;

        // 通过名称获取 freq ID
        int freqId = getCVM->getModelData()
            ->getManager<mFreqSamplPointManager>(Physical_FreqSamplPoint)
            .getIDByFreqSamplPointName(name);

        if (freqId < 0)
            return;

        // 添加映射
        getCVM->getModelData()->getMapRelationManager()
            .appendTableMapRelation(
                { Physical_Case, caseid },
                { Physical_FreqSamplPoint, freqId },
                row
            );
    }

    // 2. 删除行 → 删除该行映射
    void mCaseFrequencyPanel::onRowRemoved(int row)
    {
        int caseid = _caseData->getCaseId();

        // 删除指定行的映射
        getCVM->getModelData()->getMapRelationManager()
            .deleteTabelMapRelation(
                { Physical_Case, caseid },
                { Physical_FreqSamplPoint,-1 },
                row
            );
    }

    // 3. 下拉框切换 → 更新或新增该行映射
    void mCaseFrequencyPanel::onComboChanged(int row, const QString& text)
    {
        if (text.isEmpty()) return;

        int caseid = _caseData->getCaseId();

        // 通过名称获取 ID
        int freqId = getCVM->getModelData()
            ->getManager<mFreqSamplPointManager>(Physical_FreqSamplPoint)
            .getIDByFreqSamplPointName(text);

        if (freqId < 0) return;

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        QList<int> freqIDs = mapRelation.getMapRelation<QList<int>>(
            { Physical_Case, caseid }, Physical_FreqSamplPoint);

        if (row >= 0 && row < freqIDs.size())
        {
            // 已存在映射，执行更新
            mapRelation.updateTabelMapRelation(
                { Physical_Case, caseid },
                { Physical_FreqSamplPoint, freqId },
                row
            );
        }
        else
        {
            // 新行或空白行首次赋值，追加映射
            mapRelation.appendTableMapRelation(
                { Physical_Case, caseid },
                { Physical_FreqSamplPoint, freqId },
                row
            );
        }
    }
}