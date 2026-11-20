
#include "mCaseNonLinearDynamicPanel.h"
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
#include "mCaseImplicitNonLinearDynamic.h"
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
    mCaseNonLinearDynamicPanel::mCaseNonLinearDynamicPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "非线性瞬态分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移", "速度", "加速度"};
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["mxdb"]["节点贡献量"] = QStringList{ "结构节点贡献量" };

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

	mCaseNonLinearDynamicPanel::~mCaseNonLinearDynamicPanel()
	{

	}

    void mCaseNonLinearDynamicPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitNonLinearDynamic(caseID, ImplicitNonLinearDynamic);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);
        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseNonLinearDynamicPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitNonLinearDynamic>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // ========== 求解控制折叠块 ==========
        auto defineFold = new FoldSection("求解控制", this);

        // ========== 步长控制折叠块 ==========
        auto defineFold2 = new FoldSection("步长控制", this);

        //几何非线性
        auto comboNonlinear = new QComboBox(); comboNonlinear->addItems({ "开", "关" });

        //时间步
        auto comboTime = new QComboBox(); comboTime->addItems({ "自动", "固定" });

        // 自动时间步
        auto autolabel1 = new QLabel("初始时间步长");
        auto autoline1 = new mCalcLineEdit(this, true, false);

        auto autolabel2 = new QLabel("总时间");
        auto autoline2 = new mCalcLineEdit(this, true, false);

        auto autolabel3 = new QLabel("最小步长");
        auto autoline3 = new mCalcLineEdit(this, true, false);

        auto autolabel4 = new QLabel("最大步长");
        auto autoline4 = new mCalcLineEdit(this, true, false);

        //接续分析步
        auto comboCaseStep = new QComboBox(); comboCaseStep->addItems(QStringList{ "关闭","开启" });

        // 惯性释放
        auto comboInertia = new QComboBox(); comboInertia->addItems({ "关闭", "手动开启", "自动开启" });

        auto comboPreload = new QComboBox(); comboPreload->setToolTip(QString("继承选中的工况计算的温度"));
        updatePreloadCombobox(comboPreload, { ImplicitSteadyHeat, ImplicitTransientHeat });

        //准静态分析
        auto comboQuasiStaticAnasys = new QComboBox(); comboQuasiStaticAnasys->addItems({ "关", "开" });

        QVector<QWidget*> autoWidgets = { autolabel1, autoline1, autolabel2, autoline2,
                                  autolabel3, autoline3, autolabel4, autoline4 };
        QVector<QWidget*> fixedWidgets = { autolabel1, autoline1, autolabel2, autoline2 };

        auto updateTimeVisibility = [=](const QString& text) {
            bool isAuto = (text == "自动");
            if (isAuto)
            {
                for (int i = 0; i < fixedWidgets.size(); i += 2)
                {
                    defineFold2->layout()->removeWidget(fixedWidgets[i]);
                    defineFold2->layout()->removeWidget(fixedWidgets[i + 1]);
                    fixedWidgets[i]->hide();
                    fixedWidgets[i + 1]->hide();
                }
                for (int i = 0; i < autoWidgets.size(); i += 2)
                {
                    defineFold2->insertRow(2 + i, autoWidgets[i], autoWidgets[i + 1]);
                    autoWidgets[i]->show();
                    autoWidgets[i + 1]->show();
                }
            }
            else
            {
                for (int i = 0; i < autoWidgets.size(); i += 2)
                {
                    defineFold2->layout()->removeWidget(autoWidgets[i]);
                    defineFold2->layout()->removeWidget(autoWidgets[i + 1]);
                    autoWidgets[i]->hide();
                    autoWidgets[i + 1]->hide();
                }
                for (int i = 0; i < fixedWidgets.size(); i += 2)
                {
                    defineFold2->insertRow(2 + i, fixedWidgets[i], fixedWidgets[i + 1]);
                    fixedWidgets[i]->show();
                    fixedWidgets[i + 1]->show();
                }
            }
            defineFold2->refreshLayout();
            };

        // 添加到折叠块
        defineFold->addRow(new QLabel("几何非线性"), comboNonlinear);
        defineFold2->addRow(new QLabel("时间步"), comboTime);
        // 全部加到 layout（先加自动/固定控件，显隐由 lambda 控制）
        for (int i = 0; i < autoWidgets.size(); i += 2)
        {
            defineFold2->addRow(autoWidgets[i], autoWidgets[i + 1]);
        }
        for (int i = 0; i < fixedWidgets.size(); i += 2)
        {
            defineFold2->addRow(fixedWidgets[i], fixedWidgets[i + 1]);
        }
        defineFold->addRow(new QLabel("接续分析步"), comboCaseStep);
        defineFold->addRow(new QLabel("惯性释放"), comboInertia);
        defineFold->addRow(new QLabel("温度工况"), comboPreload);
        defineFold->addRow(new QLabel("准静态分析"), comboQuasiStaticAnasys);
        defineFold->refreshLayout();
        defineFold2->refreshLayout();

        // 信号绑定
        connect(comboTime, &QComboBox::currentTextChanged, this, updateTimeVisibility); updateTimeVisibility(comboTime->currentText());

        binder->bind(comboNonlinear, linearData, &mCaseImplicitNonLinearDynamic::_nonLinearity);
        binder->bind(comboTime, linearData, &mCaseImplicitNonLinearDynamic::_timeStepType);
        binder->bind(autoline1, linearData, &mCaseImplicitNonLinearDynamic::_initialStepValue);
        binder->bind(autoline2, linearData, &mCaseImplicitNonLinearDynamic::_totolTimeValue);
        binder->bind(autoline3, linearData, &mCaseImplicitNonLinearDynamic::_minTimeStep);
        binder->bind(autoline4, linearData, &mCaseImplicitNonLinearDynamic::_maxTimeStep);
        binder->bind(comboCaseStep, linearData, &mCaseImplicitNonLinearDynamic::_cntnlsub);
        binder->bind(comboInertia, linearData, &mCaseImplicitNonLinearDynamic::_inertiaRelief);
        binder->bind(comboQuasiStaticAnasys, linearData, &mCaseImplicitNonLinearDynamic::_quasiStaticAnasys);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);
        m_foldWidgets.append(defineFold2);
        _mainLayout->addWidget(defineFold2);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();

    }
}