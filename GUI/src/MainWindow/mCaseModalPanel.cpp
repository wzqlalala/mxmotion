
#include "mCaseModalPanel.h"
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
#include "mCaseImplicitLinearStatic.h"
#include "mCaseImplicitModal.h"
#include "mCaseImplicitNonLinearStatic.h"
#include "mCaseImplicitLinearBuckling.h"
#include "mCaseImplicitTransient.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"

#include "mRotateAnalyseManager.h"

#include "mModelData.h"
#include "mMeshData.h"


using namespace MechPreData;
using namespace MechCaseData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseModalPanel::mCaseModalPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "模态分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移"};
        _typeOfoutputCheckBoxs["mxdb"]["能量"] = QStringList{ "单元应变能" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "单元应力" };
        _typeOfoutputCheckBoxs["op2"]["能量"] = QStringList{ "单元应变能", "简单磨平应变能", "应变能密度"};

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];

    }

    mCaseModalPanel::~mCaseModalPanel()
    {

    }

    void mCaseModalPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitModal(caseID, ImplicitModal);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseModalPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitModal>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // =========================================================================
        // 1. 求解器控制
        // =========================================================================
        auto foldSolver = new FoldSection("求解控制", this);
        m_foldWidgets.append(foldSolver);
        _mainLayout->addWidget(foldSolver);

        auto comboType = new QComboBox();
        comboType->addItems({ "模态分析", "复模态分析" });

        auto lblSolver = new QLabel("模态求解器");
        auto comboSolver = new QComboBox();
        comboSolver->addItems({ "自动多重子结构", "兰索斯" });

        auto lblComplexSolver = new QLabel("复模态求解算法");
        auto comboComplexSolver = new QComboBox();
        comboComplexSolver->addItems({ "无", "精确法", "估计法" });

        auto checkNlgeo = new QCheckBox("几何大变形");

        //auto checkPreload = new QCheckBox("预应力状态");
        auto comboPreload = new QComboBox();
        updatePreloadCombobox(comboPreload);

        // 绑定数据
        binder->bind(comboType, linearData, &mCaseImplicitModal::_modalType);
        binder->bind(comboSolver, linearData, &mCaseImplicitModal::_modalSolver);
        binder->bind(comboComplexSolver, linearData, &mCaseImplicitModal::_complexMethod);
        binder->bind(checkNlgeo, linearData, &mCaseImplicitModal::_nlgeo);

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = mapRelation.getMapRelation<int>({ Physical_Case, id }, Physical_Case);
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        //if (id != -1)
        //{
        //    auto name = caseManager.getCaseNameByID(id);
        //    if (name != "")
        //    {
        //        comboPreload->setCurrentText(name);
        //    }
        //    //checkPreload->setChecked(true);
        //    comboPreload->setVisible(true);
        //}
        //else
        //{
        //    checkPreload->setChecked(false);
        //    comboPreload->setVisible(false);
        //}

        // 布局添加
        foldSolver->addRow(new QLabel("模态类型"), comboType);
        //foldSolver->addRow(new QLabel("模态求解器"), comboSolver);
        //foldSolver->addRow(checkNlgeo);
        //foldSolver->addRow(new QLabel("复模态计算方法"), comboComplexSolver);
        foldSolver->addRow(new QLabel("预加载工况"), comboPreload);

        // =========================================================================
        // 2. 输出控制 - 频率范围
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
        binder->bind(minFrequency, linearData, &mCaseImplicitModal::_minFrequency);
        binder->bind(maxFrequency, linearData, &mCaseImplicitModal::_maxFrequency);
        binder->bind(order, linearData, &mCaseImplicitModal::_order);
        binder->bind(tolerance, linearData, &mCaseImplicitModal::_tolerance);
        binder->bind(ampffact, linearData, &mCaseImplicitModal::_ampffact);
        binder->bind(comboNormalization, linearData, &mCaseImplicitModal::_normalization);

        // =========================================================================
        // 3. 复模态值 (Complex Modal Values)
        //    属于“复模态分析”
        // =========================================================================
        auto foldComplexVal = new FoldSection("复模态值", this);
        m_foldWidgets.append(foldComplexVal);
        _mainLayout->addWidget(foldComplexVal);

        auto rootNum = new mCalcLineEdit(this, false, true);

        foldComplexVal->addRow(new QLabel("复模态阶数"), rootNum);

        binder->bind(rootNum, linearData, &mCaseImplicitModal::_rootNum);


        //TODO: 增加转子动力学分析部分
        // =========================================================================
        // 5. 转子动力学分析
        //    属于“复模态分析”
        // =========================================================================
        auto foldRotational = new FoldSection("转子动力学分析", this);
        m_foldWidgets.append(foldRotational);
        _mainLayout->addWidget(foldRotational);

        auto comboStandardCoor = new QComboBox();
        comboStandardCoor->addItems({ "静止坐标系", "旋转坐标系" });

        foldRotational->addRow(new QLabel("参考坐标系"), comboStandardCoor);
        auto checkboxPic = new QCheckBox("坎贝尔图");
        checkboxPic->setChecked(linearData->_picChecked || !linearData->_picValue.isEmpty());
        auto lePicValue = new QLineEdit();
        lePicValue->setText(linearData->_picValue);
        lePicValue->setEnabled(checkboxPic->isChecked());
        foldRotational->addRow(checkboxPic, lePicValue);

        foldRotational->setCheckable(true);
        foldRotational->setChecked(linearData->_rotationalChecked);

        auto clearRotationalMapRelations = [this](int caseID)
        {
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            std::set<int> rotateIDs = mapRelation.getMapRelation<std::set<int>>(
                { Physical_Case, caseID }, Physical_RotateAnalyse);
            if (rotateIDs.empty())
                return;

            auto& rotateManager = getCVM->getModelData()->getManager<MechCaseData::mRotateAnalyseManager>(Physical_RotateAnalyse);
            for (int rotateID : rotateIDs)
            {
                // 1. 删除实际 Physical_RotateAnalyse 数据对象
                rotateManager.deleteRotateAnalyse(rotateID);
                // 2. 删除映射关系（包含部件、分析工况映射）
                mapRelation.deleteMapRelation({ Physical_RotateAnalyse, rotateID });
            }
        };

        connect(foldRotational, &FoldWidget::enabledChanged, [this, linearData, clearRotationalMapRelations](bool checked) {
            if (linearData->_rotationalChecked == checked)
                return;
            linearData->_rotationalChecked = checked;
            if (!checked)
            {
                clearRotationalMapRelations(linearData->getCaseId());
            }
            emit MechCommon::mMechGlobalSignals::getInstance()->caseRotorSettingChangedSig(linearData->getCaseId());
            });

        // 绑定数据
        binder->bind(comboStandardCoor, linearData, &mCaseImplicitModal::_standCoor);
        binder->bind(checkboxPic, linearData, &mCaseImplicitModal::_picChecked);
        binder->bind(lePicValue, linearData, &mCaseImplicitModal::_picValue);

        connect(checkboxPic, &QCheckBox::stateChanged, this, [=](int state) {
            bool checked = (state == Qt::Checked);
            lePicValue->setEnabled(checked);
            if (!checked)
            {
                lePicValue->clear();
            }
            });

        // =========================================================================
        // 4. 阻尼参数 (Damping Parameters)
        //    属于“复模态分析”
        // =========================================================================
        auto foldDamping = new FoldSection("阻尼参数", this);
        m_foldWidgets.append(foldDamping);
        _mainLayout->addWidget(foldDamping);

        auto dampingG = new mCalcLineEdit(this, true, false, true);

        foldDamping->addRow(new QLabel("结构阻尼 G"), dampingG);

        binder->bind(dampingG, linearData, &mCaseImplicitModal::_dampingG);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();

        // =========================================================================
        // 逻辑控制 (Logic & Connections)
        // =========================================================================

        // 定义Lambda函数统一处理显隐逻辑
        auto updateVisibility = [=]() {
            QString currentType = comboType->currentText();
            QString currentSolver = comboSolver->currentText();
            //bool isPreload = checkPreload->isChecked();

            // 2. 根据模态类型控制折叠块显隐
            if (currentType == "模态分析")
            {
                // 显示：频率范围(foldFrequency)
                // 隐藏：复模态值(foldComplexVal)
                foldComplexVal->setVisible(false);
                foldDamping->setVisible(false);

                foldRotational->setVisible(false);

                foldSolver->insertRow(1, lblSolver, comboSolver); lblSolver->show(); comboSolver->show();
                foldSolver->insertRow(2, checkNlgeo); checkNlgeo->show();
                foldSolver->layout()->removeWidget(lblComplexSolver); lblComplexSolver->hide();
                foldSolver->layout()->removeWidget(comboComplexSolver); comboComplexSolver->hide();
            }
            else if (currentType == "复模态分析")
            {
                // 显示：复模态值(foldComplexVal), 阻尼参数(foldDamping)
                foldComplexVal->setVisible(true);
                foldDamping->setVisible(true);
                
                //增加转字动力学分析的显隐
                foldRotational->setVisible(true);

                foldSolver->insertRow(1, lblComplexSolver, comboComplexSolver); lblComplexSolver->show(); comboComplexSolver->show();
                foldSolver->layout()->removeWidget(checkNlgeo); checkNlgeo->hide();
                foldSolver->layout()->removeWidget(lblSolver); lblSolver->hide();
                foldSolver->layout()->removeWidget(comboSolver); comboSolver->hide();
            }

            // 3. 模态分析下的求解器逻辑控制
            if (currentSolver == "自动多重子结构" && currentType == "模态分析")
            {
                // 自动多重子结构：显示容差、放大系数；隐藏特征向量
                //lblTolerance->setVisible(true);       tolerance->setVisible(true);

                
                lblAmpffact->setVisible(true);        ampffact->setVisible(true);
                foldFrequency->addRow(lblAmpffact, ampffact);
                lblNormalization->setVisible(false);  comboNormalization->setVisible(false);
                lblNormalization->setParent(nullptr);
                comboNormalization->setParent(nullptr);
            }
            else
            {
                // 兰索斯：隐藏容差、放大系数；显示特征向量
                //lblTolerance->setVisible(false);      tolerance->setVisible(false);
                lblAmpffact->setVisible(false);       ampffact->setVisible(false);
                lblNormalization->setVisible(true);   comboNormalization->setVisible(true);
                foldFrequency->addRow(lblNormalization, comboNormalization);
                lblAmpffact->setParent(nullptr);
                ampffact->setParent(nullptr);
            }


            // 刷新布局防止留白
            foldSolver->refreshLayout();
            foldFrequency->refreshLayout();
            foldComplexVal->refreshLayout();
            foldDamping->refreshLayout();
            };

        // 信号连接
        // 连接模态类型改变
        connect(comboType, &QComboBox::currentTextChanged, this, [=](const QString&) {
            updateVisibility();
            if (comboType->currentText() == "模态分析" && linearData->_rotationalChecked)
            {
                linearData->_rotationalChecked = false;
                foldRotational->setChecked(false);
                clearRotationalMapRelations(linearData->getCaseId());
            }
            emit MechCommon::mMechGlobalSignals::getInstance()->caseRotorSettingChangedSig(linearData->getCaseId());
            });

        // 连接求解器改变
        connect(comboSolver, &QComboBox::currentTextChanged, this, updateVisibility);

        // 初始化调用一次以设置初始状态
        updateVisibility();
    }
}