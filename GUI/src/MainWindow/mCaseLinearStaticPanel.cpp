
#include "mCaseLinearStaticPanel.h"
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

#include "mModelData.h"
#include "mMeshData.h"


using namespace MechPreData;
using namespace MechCaseData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseLinearStaticPanel::mCaseLinearStaticPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "线性静力学分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力", "单元高斯积分点应力" };
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "单元应变", "节点应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["mxdb"]["能量"] = QStringList{ "单元应变能", "简单磨平应变能" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };
        _typeOfoutputCheckBoxs["mxdb"]["复合材料"] = QStringList{ "单元复合应变", "单元复合应力", "单元失效指数", "Hashin失效系数", "铺层温度", "单元失效强度比", "单元铺层应力", "实体铺层节点应力", "铺层划分" };
        _typeOfoutputCheckBoxs["mxdb"]["结构力与内力"] = QStringList{ "支反力", "多点约束力", "杆梁节点内力", "梁截面内力", "602载荷", "单元节点力" };
        
        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["op2"]["能量"] = QStringList{ "单元应变能", "简单磨平应变能", "应变能密度" };
        _typeOfoutputCheckBoxs["op2"]["结构力与内力"] = QStringList{ "支反力" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["op2"]["复合材料"] = QStringList{ "单元复合应变", "单元复合应力","单元失效指数" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["h5"]["结构力与内力"] = QStringList{ "梁截面内力" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

	mCaseLinearStaticPanel::~mCaseLinearStaticPanel()
	{

	}
    void mCaseLinearStaticPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitLinearStatic(caseID, ImplicitLinearStatic);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }
    void mCaseLinearStaticPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitLinearStatic>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // ========== 求解控制折叠块 ==========
        auto defineFold = new FoldSection("求解控制", this);

        //求解器
        auto comboSolver = new QComboBox(); comboSolver->addItems({ "直接法求解器DSV", "迭代法求解器Pariters" });

        // 惯性释放
        auto comboInertia = new QComboBox();
        comboInertia->addItems({ "关闭", "手动开启", "自动开启" });

        // 核外求解
        //auto labelOOC = new QLabel("核外求解");
        //auto comboOOC = new QComboBox(); comboOOC->addItems({ "关闭", "开启" });

        // 预应力状态
        //auto checkPreload = new QCheckBox("预应力状态");

        // 预应力具体工况     
        //auto comboboxPreload = new QComboBox(); updatePreloadCombobox(comboboxPreload);

        // --- 保持之前选项 ---
        //connect(mGlobalSignals::getInstance(), &mGlobalSignals::updateModelData, this, [this, comboboxPreload](PhysicalFieldType physicalFieldType, int modelId)
        //    {
        //        if (physicalFieldType == Physical_Case)
        //        {
        //            updatePreloadCombobox(comboboxPreload);
        //        }
        //    });

        // 单点约束
        auto comboAutoSpc = new QComboBox();
        comboAutoSpc->addItems({ "开启", "关闭" });

        //核外求解显隐
        //connect(comboSolver, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        //    linearData->_equationSolver = text;
        //    if (text == "DSV")
        //    {
        //        comboOOC->setVisible(true); labelOOC->setVisible(true);
        //    }
        //    else
        //    {
        //        comboOOC->setVisible(false); labelOOC->setVisible(false);
        //    }
        //    defineFold->refreshLayout();
        //    });

        //预应力显隐
        //connect(checkPreload, &QCheckBox::stateChanged, this, [=](int state) {
        //    //linearData->_statusPreload = bool(state);
        //    if (state)
        //    {
        //        comboboxPreload->setVisible(true);
        //    }
        //    else
        //    {
        //        comboboxPreload->setVisible(false);
        //    }
        //    defineFold->refreshLayout();
        //    });

        binder->bind(comboSolver, linearData, &mCaseImplicitLinearStatic::_equationSolver);
        binder->bind(comboInertia, linearData, &mCaseImplicitLinearStatic::_inertiaRelief);
        //binder->bind(comboOOC, linearData, &mCaseImplicitLinearStatic::_ooc);
        //binder->bind(checkPreload, linearData, &mCaseImplicitLinearStatic::_statusPreload);
        //binder->bind(comboboxPreload, linearData, &mCaseImplicitLinearStatic::_preloadName); comboboxPreload->setVisible(checkPreload->isChecked());
        binder->bind(comboAutoSpc, linearData, &mCaseImplicitLinearStatic::_autospc);

        // 添加到折叠块
        defineFold->addRow(new QLabel("方程求解器"), comboSolver);
        defineFold->addRow(new QLabel("惯性释放"), comboInertia);
        defineFold->addRow(new QLabel("自动单点约束"), comboAutoSpc);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();

    }
}