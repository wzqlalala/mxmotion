
#include "mASCouplingSearchPanel.h"

//Qt
#include <QLabel>
#include <QLineEdit>
#include <QDir>
#include <QFileDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

//Function
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mGenericAssignData.h"
#include "mGlobalParam.h"
#include "mASCouplingSearchData.h"



using namespace MechGlobalParamData;
using namespace MxFunctions;
namespace MechMainWindow
{
    mASCouplingSearchPanel::mASCouplingSearchPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);
        _mainLayout->setSpacing(6);

        FoldSection* searchFold = new FoldSection("声固耦合对搜索参数设置", this, QColor("#909399"), true);

        // ========== 自动搜索开关 ==========
        QLabel* autoSearchLabel = new QLabel("自动搜索开关", this);
        _autoSearchCombo = new QComboBox(this);
        _autoSearchCombo->addItems({ "开", "关" });
        searchFold->addRow(autoSearchLabel, _autoSearchCombo);

        connect(_autoSearchCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
            {
                if (text == "开")
                {
                    _couplingSearchData->setAutosearch(true);
                }
                else if (text == "关")
                {
                    _couplingSearchData->setAutosearch(false);
                }
            }
            });

        // ========== 主面 ==========
        QLabel* mainSurfaceLabel = new QLabel("主面", this);
        _mainSurfaceCombo = new QComboBox(this);
        _mainSurfaceCombo->addItems({ "声腔面" });
        _mainSurfaceCombo->addItems({ "结构面" });
        searchFold->addRow(mainSurfaceLabel, _mainSurfaceCombo);

        connect(_mainSurfaceCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
            {
                if (text == "声腔面")
                {
                    _couplingSearchData->setMainSurface("FluidToStructure");
                }
                else if (text == "结构面")
                {
                    _couplingSearchData->setMainSurface("StructureToFluid");
                }
            }

            });

        // 1) 耦合面搜索夹角
        _searchAngelCheck = new QCheckBox("耦合面搜索夹角", this);
        searchFold->addRow(_searchAngelCheck);

        _searchAngleLineEidt = new QLineEdit("45", this);
        searchFold->addRow(_searchAngleLineEidt);

        connect(_searchAngelCheck, &QCheckBox::stateChanged, this, [this](int state) {
            if (_couplingSearchData)
                _couplingSearchData->setAngelChecked(state == Qt::Checked);
        });
        connect(_searchAngleLineEidt, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
                _couplingSearchData->setSearchAngel(text);
        });
        auto updateAngleEnabled = [this](int state) {
            bool enabled = (state == Qt::Checked);
            _searchAngleLineEidt->setEnabled(enabled);
        };
        connect(_searchAngelCheck, &QCheckBox::stateChanged, this, updateAngleEnabled);
        updateAngleEnabled(_searchAngelCheck->checkState());

        // 2) 面单元外法线方向延伸比例
        _outerNormalCheck = new QCheckBox("面单元外法线方向延伸比例", this);
        searchFold->addRow(_outerNormalCheck);

        _outerNormalLineEdit = new QLineEdit("1.0", this);
        searchFold->addRow(_outerNormalLineEdit);

        connect(_outerNormalCheck, &QCheckBox::stateChanged, this, [this](int state) {
            if (_couplingSearchData)
                _couplingSearchData->setOutChecked(state == Qt::Checked);
        });
        connect(_outerNormalLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
                _couplingSearchData->setOuterScale(text);
        });
        auto updateOuterEnabled = [this](int state) {
            bool enabled = (state == Qt::Checked);
            _outerNormalLineEdit->setEnabled(enabled);
        };
        connect(_outerNormalCheck, &QCheckBox::stateChanged, this, updateOuterEnabled);
        updateOuterEnabled(_outerNormalCheck->checkState());

        // 3) 面单元内法线方向延伸比例
        _innerNormalCheck = new QCheckBox("面单元内法线方向延伸比例", this);
        searchFold->addRow(_innerNormalCheck);

        _innerNormalLineEdit = new QLineEdit("0.5", this);
        searchFold->addRow(_innerNormalLineEdit);

        connect(_innerNormalCheck, &QCheckBox::stateChanged, this, [this](int state) {
            if (_couplingSearchData)
                _couplingSearchData->setInnerChecked(state == Qt::Checked);
        });
        connect(_innerNormalLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
                _couplingSearchData->setInnerScale(text);
        });
        auto updateInnerEnabled = [this](int state) {
            bool enabled = (state == Qt::Checked);
            _innerNormalLineEdit->setEnabled(enabled);
        };
        connect(_innerNormalCheck, &QCheckBox::stateChanged, this, updateInnerEnabled);
        updateInnerEnabled(_innerNormalCheck->checkState());

        // 4) 面单元横向延伸比例
        _transverseCheck = new QCheckBox("面单元横向延伸比例", this);
        searchFold->addRow(_transverseCheck);

        _transverseLineEdit = new QLineEdit("0.5", this);
        searchFold->addRow(_transverseLineEdit);

        connect(_transverseCheck, &QCheckBox::stateChanged, this, [this](int state) {
            if (_couplingSearchData)
                _couplingSearchData->setHorizontalChecked(state == Qt::Checked);
        });
        connect(_transverseLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
                _couplingSearchData->setHorizontalScale(text);
        });
        auto updateTransEnabled = [this](int state) {
            bool enabled = (state == Qt::Checked);
            _transverseLineEdit->setEnabled(enabled);
        };
        connect(_transverseCheck, &QCheckBox::stateChanged, this, updateTransEnabled);
        updateTransEnabled(_transverseCheck->checkState());

        // 5) 包围盒延伸比例
        _boundingBoxCheck = new QCheckBox("包围盒延伸比例", this);
        searchFold->addRow(_boundingBoxCheck);

        _boundingBoxLineEdit = new QLineEdit("1.0", this);
        searchFold->addRow(_boundingBoxLineEdit);

        connect(_boundingBoxCheck, &QCheckBox::stateChanged, this, [this](int state) {
            if (_couplingSearchData)
                _couplingSearchData->setBoundingBoxChecked(state == Qt::Checked);
        });
        connect(_boundingBoxLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (_couplingSearchData)
                _couplingSearchData->setBoundingBoxScale(text);
        });
        auto updateBboxEnabled = [this](int state) {
            bool enabled = (state == Qt::Checked);
            _boundingBoxLineEdit->setEnabled(enabled);
        };
        connect(_boundingBoxCheck, &QCheckBox::stateChanged, this, updateBboxEnabled);
        updateBboxEnabled(_boundingBoxCheck->checkState());

        m_foldWidgets.append(searchFold);
        _mainLayout->addWidget(searchFold);
        _mainLayout->addStretch();
    }

    mASCouplingSearchPanel::~mASCouplingSearchPanel()
    {
    }
    void mASCouplingSearchPanel::createASCouplingSearch()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getASCouplingSearchData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个声固耦合对搜索！"), true);
            return;
        }

        _couplingSearchData = globalParam->createASCouplingSearchData();
        //默认声腔面
        _couplingSearchData->setMainSurface("FluidToStructure");

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建声固耦合对搜索"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateASCouplingSearchSig(true);
    }
    void mASCouplingSearchPanel::editASCouplingSearchPanel()
    {
        if (!getCVM) return;
        _couplingSearchData = getCVM->getModelData()->getGlobalParam()->getASCouplingSearchData();
        if (_couplingSearchData == nullptr) return;

        // 同步控件状态到数据对象
        _searchAngelCheck->setChecked(_couplingSearchData->getAngelChecked());
        _searchAngleLineEidt->setEnabled(_couplingSearchData->getAngelChecked());
        _searchAngleLineEidt->setText(_couplingSearchData->getSearchAngel());

        _outerNormalCheck->setChecked(_couplingSearchData->getOutChecked());
        _outerNormalLineEdit->setEnabled(_couplingSearchData->getOutChecked());
        _outerNormalLineEdit->setText(_couplingSearchData->getOuterScale());

        _innerNormalCheck->setChecked(_couplingSearchData->getInnerChecked());
        _innerNormalLineEdit->setEnabled(_couplingSearchData->getInnerChecked());
        _innerNormalLineEdit->setText(_couplingSearchData->getInnerScale());

        _transverseCheck->setChecked(_couplingSearchData->getHorizontalChecked());
        _transverseLineEdit->setEnabled(_couplingSearchData->getHorizontalChecked());
        _transverseLineEdit->setText(_couplingSearchData->getHorizontalScale());

        _boundingBoxCheck->setChecked(_couplingSearchData->getBoundingBoxChecked());
        _boundingBoxLineEdit->setEnabled(_couplingSearchData->getBoundingBoxChecked());
        _boundingBoxLineEdit->setText(_couplingSearchData->getBoundingBoxScale());
    }

}
