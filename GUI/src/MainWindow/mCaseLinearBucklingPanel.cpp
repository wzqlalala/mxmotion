
#include "mCaseLinearBucklingPanel.h"
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
    mCaseLinearBucklingPanel::mCaseLinearBucklingPanel(QWidget* parent):mCaseSettingPanel(parent, "线性屈曲分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };
        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

    mCaseLinearBucklingPanel::~mCaseLinearBucklingPanel()
    {

    }

    void mCaseLinearBucklingPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitLinearBuckling(caseID, ImplicitLinearBuckling);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseLinearBucklingPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitLinearBuckling>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // ========== 求解控制折叠块 ==========
        auto defineFold = new FoldSection("求解控制", this);

        auto minFrequency = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);
        auto maxFrequency = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);
        auto order = new mCalcLineEdit(mCalcLineEdit::CalcMode::RequireInt, this);
        auto tolerance = new mCalcLineEdit(mCalcLineEdit::CalcMode::Standard, this);

        // 预应力状态
        //auto checkPreload = new QCheckBox("预应力状态");
        auto comboPreload = new QComboBox(); updatePreloadCombobox(comboPreload);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);

        binder->bind(minFrequency, linearData, &mCaseImplicitLinearBuckling::_minFrequency);
        binder->bind(maxFrequency, linearData, &mCaseImplicitLinearBuckling::_maxFrequency);
        binder->bind(order, linearData, &mCaseImplicitLinearBuckling::_order);
        binder->bind(tolerance, linearData, &mCaseImplicitLinearBuckling::_tolerance);

        // 添加到折叠块
        defineFold->addRow(new QLabel("最小频率"), minFrequency);
        defineFold->addRow(new QLabel("最大频率"), maxFrequency);
        defineFold->addRow(new QLabel("阶数"), order);
        defineFold->addRow(new QLabel("容差"), tolerance);
        defineFold->addRow(new QLabel("预加载工况"), comboPreload);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();
    }
}