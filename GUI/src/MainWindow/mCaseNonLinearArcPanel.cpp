
#include "mCaseNonLinearArcPanel.h"
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
#include <QToolButton>
#include <QHBoxLayout>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitNonLinearArc.h"
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
    mCaseNonLinearArcPanel::mCaseNonLinearArcPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "后屈曲分析")
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
        _typeOfoutputCheckBoxs["op2"]["结构力与内力"] = QStringList{ "支反力", "接触力"};
        _typeOfoutputCheckBoxs["op2"]["能量"] = QStringList{ "节点应变能", "单元应变能", "简单磨平应变能", "应变能密度" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "节点应变", "节点等效塑性应变", "单元应变", "单元等效塑性应变", "简单磨平应变", "简单磨平等效塑性应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["op2"]["热学"] = QStringList{ "温度"};

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["h5"]["结构力与内力"] = QStringList{ "支反力", "接触力","轴力","梁截面内力","单元节点力","刚性墙" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "节点应变", "节点等效塑性应变", "单元应变", "单元等效塑性应变", "简单磨平应变", "简单磨平等效塑性应变" };
        _typeOfoutputCheckBoxs["h5"]["损伤"] = QStringList{ "损伤","拉伸损伤","压缩损伤" ,"损伤起始准则" ,"剪应力比" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["h5"]["复合材料"] = QStringList{ "铺层温度", "Hashin失效系数", "单元损伤状态变量" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

    mCaseNonLinearArcPanel::~mCaseNonLinearArcPanel()
    {

    }

    void mCaseNonLinearArcPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitNonLinearArc(caseID, ImplicitPostBuckling);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);
        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseNonLinearArcPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitNonLinearArc>(_caseData);
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
        auto autolabel1 = new QLabel("初始载荷因子");
        auto autoline1 = new mCalcLineEdit(this, true, false);

        auto autolabel2 = new QLabel("总载荷");
        auto autoline2 = new mCalcLineEdit(this, true, false);

        auto autolabel3 = new QLabel("最小弧长增量");
        auto autoline3 = new mCalcLineEdit(this, true, false);

        auto autolabel4 = new QLabel("最大弧长增量");
        auto autoline4 = new mCalcLineEdit(this, true, false);

        //最大载荷因子
        auto lineLmax = new mCalcLineEdit(this, true, false, false);

        //最大载荷步数
        auto lineNmax = new mCalcLineEdit(this, true, true, true);

        //接续分析步
        auto comboCaseStep = new QComboBox(); comboCaseStep->addItems(QStringList{ "关闭","开启" });

        // 惯性释放
        auto comboInertia = new QComboBox(); comboInertia->addItems({ "关闭", "手动开启", "自动开启" });

        auto comboPreload = new QComboBox(); comboPreload->setToolTip(QString("继承选中的工况计算的温度"));
        updatePreloadCombobox(comboPreload, { ImplicitSteadyHeat, ImplicitTransientHeat });




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
        defineFold2->addRow(new QLabel("最大载荷因子"), lineLmax);
        defineFold2->addRow(new QLabel("最大载荷步数"), lineNmax);
        defineFold->addRow(new QLabel("接续分析步"), comboCaseStep);
        defineFold->addRow(new QLabel("惯性释放"), comboInertia);
        defineFold->addRow(new QLabel("温度工况"), comboPreload);
        defineFold->refreshLayout();
        defineFold2->refreshLayout();

        // 信号绑定
        connect(comboTime, &QComboBox::currentTextChanged, this, updateTimeVisibility); updateTimeVisibility(comboTime->currentText());

        binder->bind(comboNonlinear, linearData, &mCaseImplicitNonLinearArc::_nonLinearity);
        binder->bind(comboTime, linearData, &mCaseImplicitNonLinearArc::_timeStepType);
        binder->bind(autoline1, linearData, &mCaseImplicitNonLinearArc::_initialStepValue);
        binder->bind(autoline2, linearData, &mCaseImplicitNonLinearArc::_totolTimeValue);
        binder->bind(autoline3, linearData, &mCaseImplicitNonLinearArc::_minTimeStep);
        binder->bind(autoline4, linearData, &mCaseImplicitNonLinearArc::_maxTimeStep);
        binder->bind(lineLmax, linearData, &mCaseImplicitNonLinearArc::_lMax);
        binder->bind(lineNmax, linearData, &mCaseImplicitNonLinearArc::_nMax);
        binder->bind(comboCaseStep, linearData, &mCaseImplicitNonLinearArc::_cntnlsub);
        binder->bind(comboInertia, linearData, &mCaseImplicitNonLinearArc::_inertiaRelief);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);
        m_foldWidgets.append(defineFold2);
        _mainLayout->addWidget(defineFold2);

        //============================初始缺陷折叠块=====================================
        auto initBugFold = new FoldSection("初始缺陷", this);

        auto comboInitBugType = new QComboBox();
        comboInitBugType->addItems({ "线性屈曲模态", "静力工况位移" });

        auto lineInitBugFile = new QLineEdit();
        auto btnInitBugFile = new QToolButton();
        btnInitBugFile->setIcon(QIcon(":/mMainWidgets/Icon/Case/openFile@3x.png"));
        btnInitBugFile->setToolTip("选择初始缺陷文件");

        auto lineInitBugScale = new mCalcLineEdit(this, true, false);

        // 文件选择按钮
        connect(btnInitBugFile, &QToolButton::clicked, this, [=]() {
            QString filePath = QFileDialog::getOpenFileName(this, "选择初始缺陷文件", QString(), "All Files (*)");
            if (!filePath.isEmpty())
            {
                QFileInfo fileInfo(filePath);
                lineInitBugFile->setText(fileInfo.fileName());
            }
            });

        // 添加到折叠块
        initBugFold->addRow(new QLabel("缺陷类型"), comboInitBugType);

        auto fileWidget = new QWidget();
        auto fileLayout = new QHBoxLayout(fileWidget);
        fileLayout->setContentsMargins(0, 0, 0, 0);
        fileLayout->addWidget(lineInitBugFile);
        fileLayout->addWidget(btnInitBugFile);
        initBugFold->addRow(new QLabel("初始缺陷文件"), fileWidget);

        initBugFold->addRow(new QLabel("缩放系数"), lineInitBugScale);
        initBugFold->refreshLayout();

        // 绑定数据
        initBugFold->setCheckable(true);
        initBugFold->setChecked(linearData->_initBug);
        binder->bind(lineInitBugFile, linearData, &mCaseImplicitNonLinearArc::_initBugFileName);
        binder->bind(lineInitBugScale, linearData, &mCaseImplicitNonLinearArc::_scaleValue);

        // 类型combobox手动绑定
        if (linearData->_initBugType == MxFunctions::InitBugType::Static)
            comboInitBugType->setCurrentIndex(1);
        else
            comboInitBugType->setCurrentIndex(0);

        connect(comboInitBugType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            linearData->_initBugType = (index == 1) ? MxFunctions::InitBugType::Static : MxFunctions::InitBugType::Buckling;
            });

        connect(initBugFold, &FoldWidget::enabledChanged, [=](bool checked) {
            if (!linearData)
                return;
            linearData->_initBug = checked;
            });

        m_foldWidgets.append(initBugFold);
        _mainLayout->addWidget(initBugFold);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();

    }
}