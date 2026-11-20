
#include "mCaseTransientPanel.h"
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
#include "mCaseImplicitTransient.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"
#include "mSetManager.h"
#include "mSetBase.h"

#include "mModelData.h"
#include "mMeshData.h"


using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseTransientPanel::mCaseTransientPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "线性瞬态分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "单元应力",  "节点应力", "简单磨平应力" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };
        _typeOfoutputCheckBoxs["mxdb"]["节点贡献量"] = QStringList{ "结构节点贡献量" };

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "单元应力",  "节点应力", "简单磨平应力" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "单元应力",  "节点应力", "简单磨平应力" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];

        _historyOutputCheckboxs["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _historyOutputCheckboxs["应变"] = QStringList{ "节点应变", "单元应变" };
        _historyOutputCheckboxs["应力"] = QStringList{ "节点应力", "单元应力" };
        _historyKdCheckboxs["动刚度结果"] = QStringList{ "平均法", "指数法", "面积法" };
    }

    mCaseTransientPanel::~mCaseTransientPanel()
    {

    }

    void mCaseTransientPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitTransient(caseID, ImplicitTransient);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseTransientPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitTransient>(_caseData);
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

        //直接求解方法
        auto lblDirectSolver = new QLabel("求解方法");
        auto comboDirectSolver = new QComboBox(); comboDirectSolver->addItems({ "瞬态newmark法", "瞬态hht法", "瞬态wilson法" });   

        // 绑定数据
        binder->bind(comboType, linearData, &mCaseImplicitTransient::_method);
        binder->bind(comboSolver, linearData, &mCaseImplicitTransient::_modalSolver);
        binder->bind(checkRemnant, linearData, &mCaseImplicitTransient::_isRemnantOpen);
        binder->bind(comboDirectSolver, linearData, &mCaseImplicitTransient::_solveMethod);

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);

        // 布局添加
        foldSolver->addRow(new QLabel("类型"), comboType);
        foldSolver->addRow(new QLabel("预加载工况"), comboPreload);
        //foldSolver->addRow(lblSolver, comboSolver);
        //foldSolver->addRow(checkRemnant);
        //foldSolver->addRow(lblDirectSolver, comboDirectSolver);


        // ========== 时间步折叠块 ==========
        auto timeStepFold = new FoldSection("时间步", this);

        auto timeStepCount = new mCalcLineEdit(linearData->_timestepCount, nullptr, false, true);//整数 不允许为空
        auto timeStepValue = new mCalcLineEdit(linearData->_timestepVal, nullptr, false, false);//浮点数 不允许为空

        timeStepFold->addRow(new QLabel("步数"), timeStepCount);
        timeStepFold->addRow(new QLabel("步长"), timeStepValue);

        binder->bind(timeStepCount, linearData, &mCaseImplicitTransient::_timestepCount);
        binder->bind(timeStepValue, linearData, &mCaseImplicitTransient::_timestepVal);

        m_foldWidgets.append(timeStepFold);
        _mainLayout->addWidget(timeStepFold);

        // =========================================================================
        // 2. 频率范围
        // 属于模态方法
        // =========================================================================
        auto foldFrequency = new FoldSection("频率范围", this);
        m_foldWidgets.append(foldFrequency);
        _mainLayout->addWidget(foldFrequency);

        // 定义控件
        auto minFrequency = new mCalcLineEdit(this, true, false);
        auto maxFrequency = new mCalcLineEdit(this, true, false);
        auto order = new mCalcLineEdit(this, true, true, true);
        auto tolerance = new mCalcLineEdit(this, true, false);
        auto ampffact = new mCalcLineEdit(this, true, false, true);
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
        binder->bind(minFrequency, linearData, &mCaseImplicitTransient::_minFrequency);
        binder->bind(maxFrequency, linearData, &mCaseImplicitTransient::_maxFrequency);
        binder->bind(order, linearData, &mCaseImplicitTransient::_order);
        binder->bind(tolerance, linearData, &mCaseImplicitTransient::_tolerance);
        binder->bind(ampffact, linearData, &mCaseImplicitTransient::_ampffact);
        binder->bind(comboNormalization, linearData, &mCaseImplicitTransient::_normalization);

        // =========================================================================
        // 阻尼参数 (Damping Parameters)
        // =========================================================================
        auto foldDamping = new FoldSection("阻尼参数", this);
        m_foldWidgets.append(foldDamping);
        _mainLayout->addWidget(foldDamping);

        auto dampingG = new mCalcLineEdit(this, true, false, true);
        auto dampingOmg3 = new mCalcLineEdit(this, true, false);
        auto dampingAlpha1 = new mCalcLineEdit(this, true, false);
        auto dampingAlpha2 = new mCalcLineEdit(this, true, false);
        auto dampingCurve = new QLabel("模态阻尼曲线");

        auto comboFunction = new QComboBox(this);
        updateFunctionCombobox(comboFunction); 

        auto btnFunction = new QToolButton(this);
        btnFunction->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        foldDamping->addRow(new QLabel("整体结构阻尼"), dampingG);
        foldDamping->addRow(new QLabel("全局结构阻尼"), dampingOmg3);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha1"), dampingAlpha1);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha2"), dampingAlpha2);
        auto dampingFunction = new QWidget(this);
        auto layout = new QHBoxLayout(dampingFunction); layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(comboFunction);
        layout->addWidget(btnFunction);
        dampingFunction->setParent(nullptr);

        binder->bind(dampingG, linearData, &mCaseImplicitTransient::_dampingG);
        binder->bind(dampingOmg3, linearData, &mCaseImplicitTransient::_dampingOmg3);
        binder->bind(dampingAlpha1, linearData, &mCaseImplicitTransient::_dampingAlpha1);
        binder->bind(dampingAlpha2, linearData, &mCaseImplicitTransient::_dampingAlpha2);

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

            // 2. 根据模态类型控制折叠块显隐
            if (currentType == "模态法")
            {
                // 显示：频率范围(foldOutput)
                // 隐藏：复模态值(foldComplexVal), 阻尼参数(foldDamping)
                foldFrequency->setVisible(true);

                // 3. 模态分析下的求解器逻辑控制
                if (currentSolver == "自动多重子结构")
                {
                    // 自动多重子结构：放大系数；隐藏特征向量
                    foldFrequency->addRow(lblAmpffact, ampffact);
                    lblNormalization->setParent(nullptr);
                    comboNormalization->setParent(nullptr);
                }
                else if (currentSolver == "兰索斯")
                {
                    // 兰索斯：放大系数；显示特征向量
                    foldFrequency->addRow(lblNormalization, comboNormalization);
                    lblAmpffact->setParent(nullptr);
                    ampffact->setParent(nullptr);
                }

                {
                    lblDirectSolver->setParent(nullptr);
                    comboDirectSolver->setParent(nullptr);
                }

                {
                    foldSolver->insertRow(2, lblSolver, comboSolver);
                    foldSolver->insertRow(3, checkRemnant);
                }

                {
                    foldDamping->insertRow(4, dampingCurve, dampingFunction);
                }
            }
            else if (currentType == "直接法")
            {
                foldFrequency->setVisible(false);
                {
                    foldSolver->insertRow(2, lblDirectSolver, comboDirectSolver);
                }

                {
                    lblSolver->setParent(nullptr);
                    comboSolver->setParent(nullptr);
                    checkRemnant->setParent(nullptr);
                }

                {
                    foldFrequency->addRow(lblAmpffact, ampffact);
                    lblNormalization->setParent(nullptr);
                    comboNormalization->setParent(nullptr);
                }

                {
                    dampingCurve->setParent(nullptr);
                    dampingFunction->setParent(nullptr);
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
            int id = _caseData->getCaseId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Function, functionData->getFunctionId() });
            updateFunctionCombobox(comboFunction);
            comboFunction->setCurrentText(functionData->getFunctionName());
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
}