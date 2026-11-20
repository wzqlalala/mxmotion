
#include "mCaseStructuralCouplingHeatPanel.h"
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
#include "mCaseImplicitStructuralCouplingHeat.h"
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
    mCaseStructuralCouplingHeatPanel::mCaseStructuralCouplingHeatPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "结构耦合传热分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["mxdb"]["结构力与内力"] = QStringList{ "支反力", "接触力","轴力","梁截面内力","单元节点力","刚性墙" };
        _typeOfoutputCheckBoxs["mxdb"]["能量"] = QStringList{ "节点应变能", "单元应变能", "简单磨平应变能", "应变能密度" };
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "节点应变", "节点等效塑性应变", "节点等效蠕变应变", "单元应变", "单元等效塑性应变", "简单磨平应变", "简单磨平等效塑性应变" };
        _typeOfoutputCheckBoxs["mxdb"]["损伤"] = QStringList{ "损伤","拉伸损伤","压缩损伤" ,"损伤起始准则" ,"剪应力比" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["mxdb"]["复合材料"] = QStringList{ "铺层温度", "Hashin失效系数", "单元损伤状态变量" };
        _typeOfoutputCheckBoxs["mxdb"]["热学"] = QStringList{ "温度", "节点温度梯度", "节点热流密度", "单元温度梯度", "单元热流密度" };

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["op2"]["结构力与内力"] = QStringList{ "支反力", "接触力" };
        _typeOfoutputCheckBoxs["op2"]["能量"] = QStringList{ "节点应变能", "单元应变能", "简单磨平应变能", "应变能密度" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "节点应变", "节点等效塑性应变", "单元应变", "单元等效塑性应变", "简单磨平应变", "简单磨平等效塑性应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["op2"]["热学"] = QStringList{ "温度" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["h5"]["结构力与内力"] = QStringList{ "接触力","梁截面内力" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "节点应变", "节点等效塑性应变", "单元应变", "单元等效塑性应变", "简单磨平应变", "简单磨平等效塑性应变" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["h5"]["热学"] = QStringList{ "温度" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["mxdb"]["热学"] = QStringList{ "温度" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["热学"] = QStringList{ "温度" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

    mCaseStructuralCouplingHeatPanel::~mCaseStructuralCouplingHeatPanel()
    {

    }

    void mCaseStructuralCouplingHeatPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitStructuralCouplingHeat(caseID, ImplicitStructuralCouplingHeat);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseStructuralCouplingHeatPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitStructuralCouplingHeat>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // ========== 求解控制折叠块 ==========
        auto defineFold = new FoldSection("求解控制", this);

        //接续分析步
        auto comboCaseStep = new QComboBox(); comboCaseStep->addItems(QStringList{ "开启","关闭" });

        //时间步
        auto comboTime = new QComboBox(); comboTime->addItems({ "自动", "固定" });

        // 自动时间步
        auto autolabel1 = new QLabel("初始时间步长");
        auto autoline1 = new mCalcLineEdit(this, true, false, true);

        auto autolabel2 = new QLabel("总时间");
        auto autoline2 = new mCalcLineEdit(this, true, false, true);

        auto autolabel3 = new QLabel("最小步长");
        auto autoline3 = new mCalcLineEdit(this, true, false, true);

        auto autolabel4 = new QLabel("最大步长");
        auto autoline4 = new mCalcLineEdit(this, true, false, true);

        QVector<QWidget*> autoWidgets = { autolabel1, autoline1, autolabel2, autoline2,
                                  autolabel3, autoline3, autolabel4, autoline4 };
        QVector<QWidget*> fixedWidgets = { autolabel1, autoline1, autolabel2, autoline2 };

        auto updateVisibility = [=](const QString& text) {
            bool isAuto = (text == "自动");
            if (isAuto)
            {
                for (int i = 0; i < fixedWidgets.size(); i += 2)
                {
                    defineFold->layout()->removeWidget(fixedWidgets[i]);
                    defineFold->layout()->removeWidget(fixedWidgets[i + 1]);
                    fixedWidgets[i]->hide();
                    fixedWidgets[i + 1]->hide();
                }
                for (int i = 0; i < autoWidgets.size(); i += 2)
                {
                    defineFold->insertRow(1 + i, autoWidgets[i], autoWidgets[i + 1]);
                    autoWidgets[i]->show();
                    autoWidgets[i + 1]->show();
                }
            }
            else
            {
                for (int i = 0; i < autoWidgets.size(); i += 2)
                {
                    defineFold->layout()->removeWidget(autoWidgets[i]);
                    defineFold->layout()->removeWidget(autoWidgets[i + 1]);
                    autoWidgets[i]->hide();
                    autoWidgets[i + 1]->hide();
                }
                for (int i = 0; i < fixedWidgets.size(); i += 2)
                {
                    defineFold->insertRow(1 + i, fixedWidgets[i], fixedWidgets[i + 1]);
                    fixedWidgets[i]->show();
                    fixedWidgets[i + 1]->show();
                }
            }
            defineFold->refreshLayout();
            };


        // 添加到折叠块
        defineFold->addRow(new QLabel("时间步"), comboTime);
        // 全部加到 layout（先加自动/固定控件，显隐由 lambda 控制）
        for (int i = 0; i < autoWidgets.size(); i += 2)
        {
            defineFold->addRow(autoWidgets[i], autoWidgets[i + 1]);
        }
        for (int i = 0; i < fixedWidgets.size(); i += 2)
        {
            defineFold->addRow(autoWidgets[i], autoWidgets[i + 1]);
        }
        defineFold->addRow(new QLabel("接续分析步"), comboCaseStep);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();
        // 信号绑定
        connect(comboTime, &QComboBox::currentTextChanged, this, updateVisibility); updateVisibility(comboTime->currentText());

        binder->bind(comboCaseStep, linearData, &mCaseImplicitStructuralCouplingHeat::_cntnlsub);
        binder->bind(comboTime, linearData, &mCaseImplicitStructuralCouplingHeat::_timeStepType);
        binder->bind(autoline1, linearData, &mCaseImplicitStructuralCouplingHeat::_initialStepValue);
        binder->bind(autoline2, linearData, &mCaseImplicitStructuralCouplingHeat::_totolTimeValue);
        binder->bind(autoline3, linearData, &mCaseImplicitStructuralCouplingHeat::_minTimeStep);
        binder->bind(autoline4, linearData, &mCaseImplicitStructuralCouplingHeat::_maxTimeStep);



    }
}