
#include "mGenericParamPanel.h"

//Function
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mGenericParamData.h"
#include "mGlobalParam.h"

//Qt
#include <QLabel>
#include "mOtherFunction.h"

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    mGenericParamPanel::mGenericParamPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 超单元参数设置 ==========
        FoldSection* genericFold = new FoldSection("超单元参数设置", this, QColor("#909399"), true);

        // 缩聚方法
        _comboMethod = new QComboBox(this);
        _comboMethod->addItems({ "GUYAN", "CBN", "GM" });
        genericFold->addRow(new QLabel("缩聚方法"), _comboMethod);
        connect(_comboMethod, &QComboBox::currentTextChanged, this, [this]() {
            if (_genericParam) _genericParam->_method = _comboMethod->currentText();
            });

        // 频率上限
        _editFreqLimit = new mCalcLineEdit("", this, true, false);
        genericFold->addRow(new QLabel("频率上限"), _editFreqLimit);
        connect(_editFreqLimit, &QLineEdit::editingFinished, this, [this]() {
            if (_genericParam) _genericParam->_upFrequency = _editFreqLimit->text();
            });

        // 模态阶数
        _editModalOrder = new mCalcLineEdit("", this, true, true);
        genericFold->addRow(new QLabel("模态阶数"), _editModalOrder);
        connect(_editModalOrder, &QLineEdit::editingFinished, this, [this]() {
            if (_genericParam) _genericParam->_order = _editModalOrder->text();
            });

        // 坐标点起始编号
        _editPointStart = new mCalcLineEdit("", this, true, true);
        genericFold->addRow(new QLabel("坐标点起始编号"), _editPointStart);
        connect(_editPointStart, &QLineEdit::editingFinished, this, [this]() {
            if (_genericParam)
            {
                _genericParam->_startID = _editPointStart->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GenericParam, _genericParam->getId());
            }
            });

        m_foldWidgets.append(genericFold);
        _mainLayout->addWidget(genericFold);

        _mainLayout->addStretch();
    }

    mGenericParamPanel::~mGenericParamPanel()
    {
    }
    void mGenericParamPanel::createGenericParam()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        int ID = globalParam->createGenericParamID();
        int cnt = 1;
        QString text = "SE_Param";
        QString name = text + "_" + MxFunctions::mNumberToString(cnt++);
        while (globalParam->existGenericParamName(name)) {
            name = text + "_" + MxFunctions::mNumberToString(cnt++);
        }
        _genericParam = new mGenericParamData(ID, name);
        globalParam->addGenericParamData(_genericParam);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建超单元参数 %1").arg(_genericParam->getName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGenericParamSig(true, _genericParam->getId());
    }
    void mGenericParamPanel::editGenericParamPanel(int id)
    {
        if (!getCVM) return;
        _genericParam = getCVM->getModelData()->getGlobalParam()->getGenericParamData(id);
        if (_genericParam == nullptr) return;

        _comboMethod->blockSignals(true);
        _comboMethod->setCurrentText(_genericParam->_method);
        _comboMethod->blockSignals(false);

        _editFreqLimit->setExpression(_genericParam->_upFrequency);
        _editModalOrder->setExpression(_genericParam->_order);
        _editPointStart->setExpression(_genericParam->_startID);
    }
}
