
#include "mStructuralOptimizationPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"


//BasicData
#include "mModelData.h"
#include "mStructOption.h"
//Qt
#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>

using namespace MechPreData;
using namespace MechSetData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechStructOptionData;

namespace MechMainWindow
{
    mStructuralOptimizationPanel::mStructuralOptimizationPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        FoldSection* dirshowFold = new FoldSection("结构优化", this, QColor("#909399"), true);

        QStringList checkText = {
            "最大迭代步","目标函数容差","收敛离散指数",
            "优化步长","最小尺寸","惩罚参数","初始材料分数",
            "最小密度值","过滤"
        };

        vector<mCalcLineEdit::CalcMode> cale_mode = {
           mCalcLineEdit::CalcMode::StrictPositive,
           mCalcLineEdit::CalcMode::NoEmptyAndPositive,
           mCalcLineEdit::CalcMode::NoEmptyAndPositive,
           mCalcLineEdit::CalcMode::NoEmptyAndNonNegative,
           mCalcLineEdit::CalcMode::NoEmptyAndPositive,
           mCalcLineEdit::CalcMode::NoEmptyAndPositive,
           mCalcLineEdit::CalcMode::NoEmptyAndNonNegative,
           mCalcLineEdit::CalcMode::NoEmptyAndNonNegative,
           mCalcLineEdit::CalcMode::NoEmptyAndNonNegative,
        };

        for (int i = 0; i < checkText.size(); i++)
        {
            auto cb = new QCheckBox(checkText[i], this);
            _cBoxs.push_back(cb);
            connect(cb, &QCheckBox::toggled, this, &mStructuralOptimizationPanel::slot_checkChanged);
        }

        for (int i = 0; i < checkText.size(); i++)
        {
            auto ce = new mCalcLineEdit(cale_mode[i],this);
            ce->setEnabled(false);
            dirshowFold->addRow(_cBoxs[i], ce);
            _cLineEdits.push_back(ce);
            connect(ce, &QLineEdit::textChanged, this, &mStructuralOptimizationPanel::slot_textChanged);
        }

        //【0,1】范围参数的输入限制与悬浮提示
        _cLineEdits[3]->setRange(0, 1);
        _cLineEdits[3]->setToolTip("数值范围【0-1】");
        _cLineEdits[6]->setRange(0, 1);
        _cLineEdits[6]->setToolTip("数值范围【0-1】");
        _cLineEdits[7]->setRange(0, 1);
        _cLineEdits[7]->setToolTip("数值范围【0-1】");
        _cLineEdits[8]->setRange(0, 1);
        _cLineEdits[8]->setToolTip("数值范围【0-1】");

        //优化算法
        _matinterCb = new QCheckBox("优化算法", this);
        connect(_matinterCb, &QCheckBox::toggled, this, &mStructuralOptimizationPanel::slot_checkChanged);
        _matinterCombox = new QComboBox(this);
        _matinterCombox->addItems({ "SIMP","RAMP","SIMPEmin", "POLYNOMAL" });
        _matinterCombox->setEnabled(false);
        dirshowFold->addRow(_matinterCb, _matinterCombox);
        connect(_matinterCombox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            _structOptionBase->_mStructOptionData._matintertype = text;
            });

        //模型重构
        _rebuildCb = new QCheckBox("模型重构", this);
        connect(_rebuildCb, &QCheckBox::toggled, this, &mStructuralOptimizationPanel::slot_checkChanged);
        _rebuildCLine = new mCalcLineEdit(mCalcLineEdit::CalcMode::NoEmptyAndNonNegative, this);
        _rebuildCLine->setEnabled(false);
        _rebuildCLine->setRange(0, 1);
        _rebuildCLine->setToolTip("数值范围【0-1】");
        dirshowFold->addRow(_rebuildCb, _rebuildCLine);
        connect(_rebuildCLine, &QLineEdit::textChanged, this, &mStructuralOptimizationPanel::slot_textChanged);

        //重启动
        _resCb = new QCheckBox("重启动", this);
        connect(_resCb, &QCheckBox::toggled, this, &mStructuralOptimizationPanel::slot_checkChanged);
        _resCLine = new mCalcLineEdit(mCalcLineEdit::CalcMode::StrictPositive, this);
        _resCLine->setEnabled(false);
        dirshowFold->addRow(_resCb, _resCLine);
        connect(_resCLine, &QLineEdit::textChanged, this, &mStructuralOptimizationPanel::slot_textChanged);

        QStyleOptionButton opt;
        opt.initFrom(_resCb);
        opt.text = _resCb->text();
        const QRect textRect = _resCb->style()->subElementRect(QStyle::SE_CheckBoxContents, &opt, _resCb);
        _resLabel = new QLabel("重启动文件", this);
        _resLabel->setEnabled(false);
        _resLabel->setContentsMargins(textRect.x(), 0, 0, 0);
        _resLine = new QLineEdit(this);
        _resLine->setEnabled(false);
        _resLine->setReadOnly(true);
        _resTButton = new QToolButton(this);
        _resTButton->setIcon(QIcon(":/mMainWidgets/Icon/homepage/dakai3x.png"));
        _resTButton->setEnabled(false);
        dirshowFold->addRow(_resLabel, _resLine, _resTButton);

        connect(_resTButton, &QToolButton::clicked, this, [this]() {
            QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
            QFileInfo info(defaultDir);
            if (defaultDir.isEmpty() || !info.exists())
            {
                defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
            }
            QString openFile = QFileDialog::getOpenFileName(this
                , "选择文件"
                , defaultDir
                , QString("File(*.rst)"));

            if (openFile.isEmpty())
            {
                return;
            }

            QFileInfo fi(openFile);
            QString fileName = fi.fileName();

            _resLine->setText(fileName);
            _resLine->setToolTip(openFile);
            _structOptionBase->_mStructOptionData._resStartPath = openFile;

            });

        m_foldWidgets.append(dirshowFold);
        _mainLayout->addWidget(dirshowFold);
        _mainLayout->addStretch();
    }

    mStructuralOptimizationPanel::~mStructuralOptimizationPanel()
    {
    }

    void mStructuralOptimizationPanel::newStructuralOptimization()
    {
        if (!getCVM)
        {
            return;
        }

        if (getCVM->getModelData()->getStructOptionBase() == nullptr)
        {
            getCVM->getModelData()->createStructOption();
        }
        else
        {
            getCVM->getModelData()->getStructOptionBase();
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationSig(true);
    }

    void mStructuralOptimizationPanel::editStructuralOptimization()
    {
        _structOptionBase = getCVM->getModelData()->getStructOptionBase();
        _cLineEdits[0]->setExpression(_structOptionBase->_mStructOptionData._maxiter);
        _cLineEdits[1]->setExpression(_structOptionBase->_mStructOptionData._objectivetolrance);
        _cLineEdits[2]->setExpression(_structOptionBase->_mStructOptionData._discretetarget);
        _cLineEdits[3]->setExpression(_structOptionBase->_mStructOptionData._topomovelimit);
        _cLineEdits[4]->setExpression(_structOptionBase->_mStructOptionData._rmin);
        _cLineEdits[5]->setExpression(_structOptionBase->_mStructOptionData._penal);
        _cLineEdits[6]->setExpression(_structOptionBase->_mStructOptionData._initdensity);
        _cLineEdits[7]->setExpression(_structOptionBase->_mStructOptionData._minDesity);
        _cLineEdits[8]->setExpression(_structOptionBase->_mStructOptionData._checker);
        _matinterCombox->setCurrentText(_structOptionBase->_mStructOptionData._matintertype);
        _rebuildCLine->setExpression(_structOptionBase->_mStructOptionData._rebuild);
        _resCLine->setExpression(_structOptionBase->_mStructOptionData._iteration);

        QFileInfo fi(_structOptionBase->_mStructOptionData._resStartPath);
        QString fileName = fi.fileName();
        _resLine->setToolTip(_structOptionBase->_mStructOptionData._resStartPath);
        _resLine->setText(fileName);

        _cBoxs[0]->setChecked(_structOptionBase->_mStructOptionData._isMaxiter);
        _cBoxs[1]->setChecked(_structOptionBase->_mStructOptionData._isObjectivetolrance);
        _cBoxs[2]->setChecked(_structOptionBase->_mStructOptionData._isDiscretetarget);
        _cBoxs[3]->setChecked(_structOptionBase->_mStructOptionData._isTopomovelimit);
        _cBoxs[4]->setChecked(_structOptionBase->_mStructOptionData._isRmin);
        _cBoxs[5]->setChecked(_structOptionBase->_mStructOptionData._isPenal);
        _cBoxs[6]->setChecked(_structOptionBase->_mStructOptionData._isInitdensity);
        _cBoxs[7]->setChecked(_structOptionBase->_mStructOptionData._isMinDesity);
        _cBoxs[8]->setChecked(_structOptionBase->_mStructOptionData._isChecker);
        _matinterCb->setChecked(_structOptionBase->_mStructOptionData._isMatintertype);
        _rebuildCb->setChecked(_structOptionBase->_mStructOptionData._isRebuild);
        _resCb->setChecked(_structOptionBase->_mStructOptionData._isRestart);
    }

    void mStructuralOptimizationPanel::slot_textChanged()
    {
        auto ce = qobject_cast<mCalcLineEdit*>(sender());
        if (ce == _cLineEdits[0])
        {
            _structOptionBase->_mStructOptionData._maxiter = ce->text();
        }
        else if (ce == _cLineEdits[1])
        {
            _structOptionBase->_mStructOptionData._objectivetolrance = ce->text();
        }
        else if (ce == _cLineEdits[2])
        {
            _structOptionBase->_mStructOptionData._discretetarget = ce->text();
        }
        else if (ce == _cLineEdits[3])
        {
            _structOptionBase->_mStructOptionData._topomovelimit = ce->text();
        }
        else if (ce == _cLineEdits[4])
        {
            _structOptionBase->_mStructOptionData._rmin = ce->text();
        }
        else if (ce == _cLineEdits[5])
        {
            _structOptionBase->_mStructOptionData._penal = ce->text();
        }
        else if (ce == _cLineEdits[6])
        {
            _structOptionBase->_mStructOptionData._initdensity = ce->text();
        }
        else if (ce == _cLineEdits[7])
        {
            _structOptionBase->_mStructOptionData._minDesity = ce->text();
        }
        else if (ce == _cLineEdits[8])
        {
            _structOptionBase->_mStructOptionData._checker = ce->text();
        }
        else if (ce == _rebuildCLine)
        {
            _structOptionBase->_mStructOptionData._rebuild = ce->text();
        }
        else if (ce == _resCLine)
        {
            _structOptionBase->_mStructOptionData._iteration = ce->text();
        }
    }


    void mStructuralOptimizationPanel::slot_checkChanged(bool isChecked)
    {
        auto cb = qobject_cast<QCheckBox*>(sender());

        if (cb == _cBoxs[0])
        {
            _structOptionBase->_mStructOptionData._isMaxiter = isChecked;
            _cLineEdits[0]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._maxiter = _cLineEdits[0]->text();
        }
        else if (cb == _cBoxs[1])
        {
            _structOptionBase->_mStructOptionData._isObjectivetolrance = isChecked;
            _cLineEdits[1]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._objectivetolrance = _cLineEdits[1]->text();
        }
        else if (cb == _cBoxs[2])
        {
            _structOptionBase->_mStructOptionData._isDiscretetarget = isChecked;
            _cLineEdits[2]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._discretetarget = _cLineEdits[2]->text();
        }
        else if (cb == _cBoxs[3])
        {
            _structOptionBase->_mStructOptionData._isTopomovelimit = isChecked;
            _cLineEdits[3]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._topomovelimit = _cLineEdits[3]->text();
        }
        else if (cb == _cBoxs[4])
        {
            _structOptionBase->_mStructOptionData._isRmin = isChecked;
            _cLineEdits[4]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._rmin = _cLineEdits[4]->text();
        }
        else if (cb == _cBoxs[5])
        {
            _structOptionBase->_mStructOptionData._isPenal = isChecked;
            _cLineEdits[5]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._penal = _cLineEdits[5]->text();
        }
        else if (cb == _cBoxs[6])
        {
            _structOptionBase->_mStructOptionData._isInitdensity = isChecked;
            _cLineEdits[6]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._initdensity = _cLineEdits[6]->text();
        }
        else if (cb == _cBoxs[7])
        {
            _structOptionBase->_mStructOptionData._isMinDesity = isChecked;
            _cLineEdits[7]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._minDesity = _cLineEdits[7]->text();
        }
        else if (cb == _cBoxs[8])
        {
            _structOptionBase->_mStructOptionData._isChecker = isChecked;
            _cLineEdits[8]->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._checker = _cLineEdits[8]->text();
        }
        else if (cb == _matinterCb)
        {
            _structOptionBase->_mStructOptionData._isMatintertype = isChecked;
            _matinterCombox->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._matintertype = _matinterCombox->currentText();
        }
        else if (cb == _rebuildCb)
        {
            _structOptionBase->_mStructOptionData._isRebuild = isChecked;
            _rebuildCLine->setEnabled(isChecked);
            if (isChecked) _structOptionBase->_mStructOptionData._rebuild = _rebuildCLine->text();
        }
        else if (cb == _resCb)
        {
            _structOptionBase->_mStructOptionData._isRestart = isChecked;
            _resCLine->setEnabled(isChecked);
            _resLabel->setEnabled(isChecked);
            _resLine->setEnabled(isChecked);
            _resTButton->setEnabled(isChecked);
            if (isChecked)
            {
                _structOptionBase->_mStructOptionData._iteration = _resCLine->text();
                _structOptionBase->_mStructOptionData._resStartPath = _resLine->toolTip();
            }
        }
    }
}








