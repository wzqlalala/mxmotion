
#include "mCaseGenericPanel.h"
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
#include <QCompleter>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitGeneric.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"
#include "mGlobalParam.h"
#include "mGenericParamData.h"

#include "mModelData.h"
#include "mMeshData.h"


using namespace MechPreData;
using namespace MechCaseData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mCaseGenericPanel::mCaseGenericPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "超单元分析")
    {
 
    }

    mCaseGenericPanel::~mCaseGenericPanel()
    {

    }

    void mCaseGenericPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitGeneric(caseID, ImplicitGeneric);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());

        if (!getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).isExistGenericBoundary())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, "缺失超单元边界！", true);
        }
    }

    void mCaseGenericPanel::updatePanel()
    {
        auto genericData = toType<mCaseImplicitGeneric>(_caseData);
        if (!genericData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // ========== 超单元参数设置折叠块 ==========
        auto defineFold = new FoldSection("超单元参数设置", this);

        auto comboPreload = new QComboBox();
        updateGenericParamCombobox(comboPreload);

        connect(comboPreload, &QComboBox::currentTextChanged, this, [=](const QString& text) {
            if (getCVM == nullptr)
            {
                return;
            }
            if (_caseData == nullptr)
            {
                return;
            }
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            int caseID = _caseData->getCaseId();
            int id = getCVM->getModelData()->getPhyIDByName(Physical_GenericParam, text);
            mapRelation.appendMapRelation({ Physical_Case, caseID }, { Physical_GenericParam, id });
            });

        defineFold->addRow(new QLabel("超单元参数："), comboPreload);
        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        // ========== 分析部件折叠块 ==========
        auto casePartFold = new FoldSection("分析部件", this);
        QPushButton* pb = new QPushButton("设置", this);

        connect(pb, &QPushButton::clicked, this, [this]() {
            if (getCVM->getModelData()->getGlobalParam()->getGenericPartData())
            {
                mInfoWindow::getInstance()->editGenericPart();
            }
            else
            {
                mInfoWindow::getInstance()->createGenericPart();
            }

            });

        casePartFold->addRow(new QLabel("超单元部件："), pb);

        m_foldWidgets.append(casePartFold);
        _mainLayout->addWidget(casePartFold);

        _mainLayout->addStretch();
    }
}