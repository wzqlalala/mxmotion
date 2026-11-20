
#include "mGenericMatrixPanel.h"

//Function
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mGenericMatrixData.h"
#include "mGlobalParam.h"

//Qt
#include <QLabel>

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    mGenericMatrixPanel::mGenericMatrixPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 矩阵名称 ==========
        FoldSection* genericFold = new FoldSection("矩阵名称", this, QColor("#909399"), true);

        // 频率上限
        _matrixName = new QLineEdit(this);
        genericFold->addRow(new QLabel("矩阵名称"), _matrixName);
        connect(_matrixName, &QLineEdit::editingFinished, this, [this]() {
            auto _matrix = getCVM->getModelData()->getGlobalParam()->getGenericMatrixData();
            if (_matrix)
            {
                _matrix->_matrixName = _matrixName->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericMatrix, 0);
            }
            });

        m_foldWidgets.append(genericFold);
        _mainLayout->addWidget(genericFold);

        _mainLayout->addStretch();
    }

    mGenericMatrixPanel::~mGenericMatrixPanel()
    {
    }
    void mGenericMatrixPanel::createGenericMatrix()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getGenericMatrixData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个超单元矩阵！"), true);
            return;
        }

        globalParam->createGenericMatrixData();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建超单元矩阵"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGenericMatrixSig(true);
    }
    void mGenericMatrixPanel::editGenericMatrixPanel()
    {
        if (!getCVM) return;
        auto _GenericMatrix = getCVM->getModelData()->getGlobalParam()->getGenericMatrixData();
        if (_GenericMatrix == nullptr) return;

        _matrixName->setText(_GenericMatrix->_matrixName);
    }
}
