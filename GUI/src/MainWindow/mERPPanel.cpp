#include "mERPPanel.h"

#include "mCalcLineEdit.h"
#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mMaterialManager.h"
#include "mSetManager.h"
#include "mMapRelationManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetBase.h"
#include "mCaseBase.h"
#include "mCaseImplASCoupling.h"

#include <QCompleter>
#include <QTimer>
#include <QPointer>
#include <functional>
#include <algorithm>
#include <QStringListModel>

#include "mGlobalParam.h"

using namespace MechCommon;
using namespace MechSetData;
using namespace MechCaseData;

namespace MechMainWindow
{
    mERPPanel::mERPPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        data = nullptr;
        /**********************************************等效辐射功率**********************************************************/

        FoldSection* erpFold = new FoldSection("等效辐射功率参数", this);

        _editRho = new mCalcLineEdit("", this, false, false);
        erpFold->addRow(new PanelLabel("介质密度"), _editRho);
        connect(_editRho, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_erprho = _editRho->text();
            });

        _editC = new mCalcLineEdit("", this, false, false);
        erpFold->addRow(new PanelLabel("声速"), _editC);
        connect(_editC, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_erpc = _editC->text();
            });

        _editRhocp = new mCalcLineEdit("", this, false, false);
        erpFold->addRow(new PanelLabel("缩放系数"), _editRhocp);
        connect(_editRhocp, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_rhocp = _editRhocp->text();
            });

        _editRlf = new mCalcLineEdit("", this, false, false);
        erpFold->addRow(new PanelLabel("辐射损耗密度"), _editRlf);
        connect(_editRlf, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_erprlf = _editRlf->text();
            });

        _editRefDb = new mCalcLineEdit("", this, false, false);
        erpFold->addRow(new PanelLabel("参考值"), _editRefDb);
        connect(_editRefDb, &QLineEdit::editingFinished, this, [this]() {
            if (data)
                data->_erprefdb = _editRefDb->text();
            });

        m_foldWidgets.append(erpFold);
        _mainLayout->addWidget(erpFold);
        _mainLayout->addStretch();
    }

    mERPPanel::~mERPPanel()
    {
    }

    void mERPPanel::createERP(int id)
    {
        if (!getCVM) return;

        if (id == 0) //id等于0，从模型树创建
        {
            auto globalParam = getCVM->getModelData()->getGlobalParam();
            if (globalParam->getERPData())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个等效辐射功率！"), true);
                return;
            }
            data = globalParam->createERPData();
        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建等效辐射功率"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContriSig(true, id, "等效辐射功率");
    }

    void mERPPanel::editERP(int erpID)
    {
        //获取正确的data
        if (erpID == 0) //0为模型树上的
        {
            if (!getCVM) return;
            data = getCVM->getModelData()->getGlobalParam()->getERPData();
            if (data == nullptr) return;
        }

        //填充面板
        _editRho->setExpression(data->_erprho);
        _editC->setExpression(data->_erpc);
        _editRhocp->setExpression(data->_rhocp);
        _editRlf->setExpression(data->_erprlf);
        _editRefDb->setExpression(data->_erprefdb);
    }
}
