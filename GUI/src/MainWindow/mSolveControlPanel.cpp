#include "mSolveControlPanel.h"

//Qt
#include <QLabel>
#include <QIntValidator>

//Function
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mSolveControlData.h"
#include "mGlobalParam.h"

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    mSolveControlPanel::mSolveControlPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 求解控制设置 ==========
        FoldSection* solveFold = new FoldSection("求解控制设置", this, QColor("#909399"), true);

        _cpuCoreCountEdit = new QLineEdit(this);
        _cpuCoreCountEdit->setValidator(new QIntValidator(1, 999999, this));
        _cpuCoreCountEdit->setPlaceholderText("请输入正整数");
        solveFold->addRow(new PanelLabel("CPU并行核数"), _cpuCoreCountEdit);
        connect(_cpuCoreCountEdit, &QLineEdit::editingFinished, this, [this]() {
            auto data = getCVM->getModelData()->getGlobalParam()->getSolveControlData();
            if (data)
            {
                data->setCpuCoreCount(_cpuCoreCountEdit->text());
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_SolveControl, 0);
            }
            });

        m_foldWidgets.append(solveFold);
        _mainLayout->addWidget(solveFold);

        _mainLayout->addStretch();
    }

    mSolveControlPanel::~mSolveControlPanel()
    {
    }

    void mSolveControlPanel::createSolveControl()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getSolveControlData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个求解控制"), true);
            return;
        }

        globalParam->createSolveControlData();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建求解控制"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateSolveControlSig(true);
    }

    void mSolveControlPanel::editSolveControlPanel()
    {
        if (!getCVM) return;
        auto data = getCVM->getModelData()->getGlobalParam()->getSolveControlData();
        if (data == nullptr) return;

        _cpuCoreCountEdit->setText(data->getCpuCoreCountString());
    }
}
