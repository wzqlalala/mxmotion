
#include "mRunnerOptimizationDesignPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationDesignData.h"
#include "mPropertyManager.h"
#include "mMeshData.h"
#include "mCoordinateManager.h"
#include "mCalcLineEdit.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechPropertyData;
using namespace MechStructOptionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mRunnerOptimizationDesignPanel::mRunnerOptimizationDesignPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _optimizationDesignData = nullptr;

        _mainLayout = new QVBoxLayout(this);

        //设计域属性组
        FoldSection* designFold = new FoldSection("设计域属性", this);

        // 设计域类型下拉框（流道优化只有"实体"）
        _comboDesignDomainType = new QComboBox();
        _comboDesignDomainType->addItems({ "实体" });
        designFold->addRow(new PanelLabel("设计域类型"), _comboDesignDomainType);

        // 指定设计域下拉框
        _comboProperty = new QComboBox();
        designFold->addRow(new PanelLabel("指定设计域"), _comboProperty);
        connect(_comboProperty, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_propertyChanged);

        m_foldWidgets.append(designFold);
        _mainLayout->addWidget(designFold);

        // ==========================================
        // 达西惩罚项组
        // ==========================================
        auto darcyFold = new FoldSection("达西惩罚项", this, QColor("#909399"), false);

        // 最大渗透率
        m_checkMaxPermeability = new QCheckBox("最大渗透率", this);
        m_lineMaxPermeability = new mCalcLineEdit(this, false, false, true); m_lineMaxPermeability->setEnabled(false);
        darcyFold->addRow(m_checkMaxPermeability, m_lineMaxPermeability);

        // 最小渗透率
        m_checkMinPermeability = new QCheckBox("最小渗透率", this);
        m_lineMinPermeability = new mCalcLineEdit(this, false, false, true); m_lineMinPermeability->setEnabled(false);
        darcyFold->addRow(m_checkMinPermeability, m_lineMinPermeability);

        m_foldWidgets.append(darcyFold);
        _mainLayout->addWidget(darcyFold);

        // --- 最大渗透率 ---
        connect(m_checkMaxPermeability, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_maxPermeabilityStatus = (state == Qt::Checked);
            m_lineMaxPermeability->setEnabled(_optimizationDesignData->_maxPermeabilityStatus);
        });
        connect(m_lineMaxPermeability, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_maxPermeability = m_lineMaxPermeability->text();
        });

        // --- 最小渗透率 ---
        connect(m_checkMinPermeability, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_minPermeabilityStatus = (state == Qt::Checked);
            m_lineMinPermeability->setEnabled(_optimizationDesignData->_minPermeabilityStatus);
        });
        connect(m_lineMinPermeability, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_minPermeability = m_lineMinPermeability->text();
        });

        // ==========================================
        // 设计域参数组
        // ==========================================
        auto paramFold = new FoldSection("设计域参数", this, QColor("#909399"), false);

        // 最小成员尺寸
        m_checkMinMember = new QCheckBox("最小成员尺寸", this);
        m_lineMinMember = new mCalcLineEdit(this, false, false, true); m_lineMinMember->setEnabled(false);
        paramFold->addRow(m_checkMinMember, m_lineMinMember);

        // 最大成员尺寸
        m_checkMaxMember = new QCheckBox("最大成员尺寸", this);
        m_lineMaxMember = new mCalcLineEdit(this, false, false, true); m_lineMaxMember->setEnabled(false);
        paramFold->addRow(m_checkMaxMember, m_lineMaxMember);

        // 最小间隙
        m_checkMinGap = new QCheckBox("最小间隙", this);
        m_lineMinGap = new mCalcLineEdit(this, false, false, true); m_lineMinGap->setEnabled(false);
        paramFold->addRow(m_checkMinGap, m_lineMinGap);

        m_foldWidgets.append(paramFold);
        _mainLayout->addWidget(paramFold);

        // --- 最小成员尺寸 ---
        connect(m_checkMinMember, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_minMemberStatus = (state == Qt::Checked);
            m_lineMinMember->setEnabled(_optimizationDesignData->_minMemberStatus);
        });
        connect(m_lineMinMember, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_minMemberSize = m_lineMinMember->text();
        });

        // --- 最大成员尺寸 ---
        connect(m_checkMaxMember, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_maxMemberStatus = (state == Qt::Checked);
            m_lineMaxMember->setEnabled(_optimizationDesignData->_maxMemberStatus);
        });
        connect(m_lineMaxMember, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_maxMemberSize = m_lineMaxMember->text();
        });

        // --- 最小间隙 ---
        connect(m_checkMinGap, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_minGapStatus = (state == Qt::Checked);
            m_lineMinGap->setEnabled(_optimizationDesignData->_minGapStatus);
        });
        connect(m_lineMinGap, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_minGapSize = m_lineMinGap->text();
        });

        // ==========================================
        // 对称约束组
        // ==========================================
        FoldSection* patternFold = new FoldSection("对称约束", this, QColor("#909399"), false);
        m_checkPattern = new QCheckBox("对称方式"); m_comboPattern = new QComboBox(); m_comboPattern->addItems({ "单平面", "双平面", "三平面", "周向", "周向+平面" }); m_comboPattern->setEnabled(false);
        connect(m_checkPattern, &QCheckBox::stateChanged, this, &mRunnerOptimizationDesignPanel::slot_checkPatternChanged);
        connect(m_comboPattern, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_comboPatternChanged);
        patternFold->addRow(m_checkPattern, m_comboPattern);

        m_lblPatternNum = new PanelLabel("周向对称数量", this); m_lblPatternNum->setParent(nullptr);
        m_linePatternNum = new mCalcLineEdit(this); m_linePatternNum->setEnabled(false); m_linePatternNum->setParent(nullptr);

        // --- 周向对称数量 ---
        connect(m_linePatternNum, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_patternNum = m_linePatternNum->text();
            });

        m_originWidget = new RunnerPickWidget("原点", _optimizationDesignData, &mRunnerOptimizationDesignData::_origin, getPickPointData, this);
        m_originWidget->setMarkColor(QColor(255, 0, 0)); m_originWidget->setEnabled(false); m_originWidget->setParent(nullptr);
        m_originWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData->_patternStatus) {
                this->updatePointRender("origin", pt, QVector3D(1, 0, 0));
            }
        });

        m_dirPoint1Widget = new RunnerPickWidget("方向点1", _optimizationDesignData, &mRunnerOptimizationDesignData::_dirPoint1, getPickPointData, this);
        m_dirPoint1Widget->setMarkColor(QColor(0, 255, 0)); m_dirPoint1Widget->setEnabled(false); m_dirPoint1Widget->setParent(nullptr);
        m_dirPoint1Widget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData->_patternStatus) {
                this->updatePointRender("direction1", pt, QVector3D(0, 1, 0));
            }
        });

        m_dirPoint2Widget = new RunnerPickWidget("方向点2", _optimizationDesignData, &mRunnerOptimizationDesignData::_dirPoint2, getPickPointData, this);
        m_dirPoint2Widget->setMarkColor(QColor(0, 0, 255)); m_dirPoint2Widget->setEnabled(false); m_dirPoint2Widget->setParent(nullptr);
        m_dirPoint2Widget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData->_patternStatus)
                this->updatePointRender("direction2", pt, QVector3D(0, 0, 1));
        });

        m_foldWidgets.append(patternFold);
        patternFold->setParent(nullptr);

        // ==========================================
        // 拔模约束组
        // ==========================================
        auto draftFold = new FoldSection("拔模约束", this, QColor("#909399"), false);

        // 1. 拔模方式 (复选框 + 下拉框)
        m_checkDraftType = new QCheckBox("拔模方式", this);
        m_comboDraftType = new QComboBox(this); m_comboDraftType->setEnabled(false);
        m_comboDraftType->addItems({ "单向", "双向" });
        draftFold->addRow(m_checkDraftType, m_comboDraftType);
        connect(m_checkDraftType, &QCheckBox::stateChanged, this, &mRunnerOptimizationDesignPanel::slot_checkDraftChanged);
        // --- 拔模方式 ---
        connect(m_comboDraftType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (_optimizationDesignData) _optimizationDesignData->_liftingType = m_comboDraftType->currentText();
            });

        // 2. 无孔约束 (标签 + 下拉框)
        m_lblNoHole = new PanelLabel("无孔约束", this);
        m_comboNoHole = new QComboBox(this); m_comboNoHole->setEnabled(false);
        m_comboNoHole->addItems({ "开启", "关闭" }); m_comboNoHole->setCurrentText("关闭");
        draftFold->addRow(m_lblNoHole, m_comboNoHole);

        // --- 无孔约束 ---
        connect(m_comboNoHole, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (_optimizationDesignData) _optimizationDesignData->_nohole = m_comboNoHole->currentText();
            });

        // 3. 起始点 (使用通用组件)
        m_startPointWidget = new RunnerPickWidget("起始点", _optimizationDesignData, &mRunnerOptimizationDesignData::_startPoint, getPickPointData, this);
        m_startPointWidget->setMarkColor(QColor(255, 165, 0)); m_startPointWidget->setEnabled(false);
        m_startPointWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData->_liftingStatus) {
                this->updatePointRender("draftStart", pt, QVector3D(1.0, 0.65, 0.0));
            }
        });
        draftFold->addRow(m_startPointWidget);

        // 4. 方向点 (使用通用组件)
        m_draftDirWidget = new RunnerPickWidget("方向点", _optimizationDesignData, &mRunnerOptimizationDesignData::_dirPoint, getPickPointData, this);
        m_draftDirWidget->setMarkColor(QColor(128, 0, 128)); m_draftDirWidget->setEnabled(false);
        m_draftDirWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData->_liftingStatus) {
                this->updatePointRender("draftDir", pt, QVector3D(0.5, 0.0, 0.5));
            }
        });
        draftFold->addRow(m_draftDirWidget);

        m_foldWidgets.append(draftFold);
        draftFold->setParent(nullptr);

        // ==========================================
        // 挤出约束组
        // ==========================================
        auto extrusionFold = new FoldSection("挤出约束", this, QColor("#909399"), false);

        // 1. 挤出方式 (下拉框)
        m_checkExtrude = new QCheckBox("挤出方式");
        m_comboExtrudeType = new QComboBox(this); m_comboExtrudeType->setEnabled(false);
        m_comboExtrudeType->addItems({ "平行", "扭转" });
        extrusionFold->addRow(m_checkExtrude, m_comboExtrudeType);

        connect(m_checkExtrude, &QCheckBox::stateChanged, this, &mRunnerOptimizationDesignPanel::slot_checkExtrudeChanged);
        connect(m_comboExtrudeType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_comboExtrudeChanged);

        // 2. 路径1
        m_path1Widget = new RunnerPickMultiIdWidget("路径1", _optimizationDesignData, &mRunnerOptimizationDesignData::_path1, [this](QVector<int>& ids) {
            return this->getPickPathData(ids);
        }, this);
        m_path1Widget->setMarkColor(QColor(0, 150, 136)); m_path1Widget->setEnabled(false);
        m_path1Widget->setValueChangedCallback([this](const QVector<int>& ids) {
            if (_optimizationDesignData->_extrusionStatus) {
                this->updatePointRender("path1", ids, QVector3D(0, 150.0 / 255.0, 136.0 / 255.0));
            }
        });

        // 3. 路径2
        m_path2Widget = new RunnerPickMultiIdWidget("路径2", _optimizationDesignData, &mRunnerOptimizationDesignData::_path2, [this](QVector<int>& ids) {
            return this->getPickPathData(ids);
        }, this);
        m_path2Widget->setMarkColor(QColor(233, 30, 99)); m_path2Widget->setEnabled(false); m_path2Widget->setParent(nullptr);
        m_path2Widget->setValueChangedCallback([this](const QVector<int>& ids) {
            if (_optimizationDesignData->_extrusionStatus) {
                this->updatePointRender("path2", ids, QVector3D(233.0 / 255.0, 30.0 / 255.0, 99.0 / 255.0));
            }
        });

        extrusionFold->addRow(m_path1Widget);
        m_foldWidgets.append(extrusionFold);
        extrusionFold->setParent(nullptr);

        // ==========================================
        // 自支撑约束组
        // ==========================================
        auto supportFold = new FoldSection("自支撑约束", this, QColor("#909399"), false);

        m_checkSupportStatus = new QCheckBox("悬垂角", this);
        m_lineOverhangAngle = new mCalcLineEdit(this, false, false);
        m_lineOverhangAngle->setEnabled(false);
        supportFold->addRow(m_checkSupportStatus, m_lineOverhangAngle);

        connect(m_lineOverhangAngle, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) {
                _optimizationDesignData->_overhangAngle = m_lineOverhangAngle->text();
            }
        });

        m_supportStartWidget = new RunnerPickWidget("起始点", _optimizationDesignData, &mRunnerOptimizationDesignData::_supportStartPoint, getPickPointData, this);
        m_supportStartWidget->setMarkColor(QColor(255, 140, 0)); m_supportStartWidget->setEnabled(false);
        m_supportStartWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_supportStatus) {
                this->updatePointRender("supportStart", pt, QVector3D(1.0, 140.0 / 255.0, 0.0));
            }
        });
        supportFold->addRow(m_supportStartWidget);

        m_supportDirWidget = new RunnerPickWidget("方向点", _optimizationDesignData, &mRunnerOptimizationDesignData::_supportDirPoint, getPickPointData, this);
        m_supportDirWidget->setMarkColor(QColor(0, 191, 255)); m_supportDirWidget->setEnabled(false);
        m_supportDirWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_supportStatus) {
                this->updatePointRender("supportDir", pt, QVector3D(0.0, 191.0 / 255.0, 1.0));
            }
        });
        supportFold->addRow(m_supportDirWidget);

        connect(m_checkSupportStatus, &QCheckBox::stateChanged, this, &mRunnerOptimizationDesignPanel::slot_checkSupportChanged);
        m_foldWidgets.append(supportFold);
        supportFold->setParent(nullptr);

        // ==========================================
        // 模式重复组
        // ==========================================
        auto patternRepFold = new FoldSection("模式重复", this, QColor("#909399"), false);

        m_checkPatternRepStatus = new QCheckBox("类型", this);
        m_comboDomainType = new QComboBox(this); m_comboDomainType->setEnabled(false);
        m_comboDomainType->addItems({ "主设计域", "从设计域" });
        patternRepFold->addRow(m_checkPatternRepStatus, m_comboDomainType);

        m_lblDomain = new PanelLabel("参考设计域", this);
        m_comboDomain = new QComboBox(this); m_comboDomain->setEnabled(false);
        m_lblDomain->setParent(nullptr);
        m_comboDomain->setParent(nullptr);

        m_comboCoordType = new QComboBox(this);
        m_comboCoordType->addItems({ "全局坐标系", "指定点" }); m_comboCoordType->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("坐标系", this), m_comboCoordType);

        m_lineScaleX = new mCalcLineEdit(this, false, false, true); m_lineScaleX->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 X", this), m_lineScaleX);
        m_lineScaleY = new mCalcLineEdit(this, false, false, true); m_lineScaleY->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 Y", this), m_lineScaleY);
        m_lineScaleZ = new mCalcLineEdit(this, false, false, true); m_lineScaleZ->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 Z", this), m_lineScaleZ);

        m_repBasePointWidget = new RunnerPickWidget("基准点", _optimizationDesignData, &mRunnerOptimizationDesignData::_repBasePoint, getPickPointData, this);
        m_repBasePointWidget->setMarkColor(QColor(0, 255, 255)); m_repBasePointWidget->setEnabled(false);
        m_repBasePointWidget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_patternRepStatus) this->updatePointRender("patternRep_base", pt, QVector3D(0.0, 1.0, 1.0));
        });
        patternRepFold->addRow(m_repBasePointWidget);

        m_repDirPoint1Widget = new RunnerPickWidget("方向点1", _optimizationDesignData, &mRunnerOptimizationDesignData::_repDirPoint1, getPickPointData, this);
        m_repDirPoint1Widget->setMarkColor(QColor(255, 0, 255)); m_repDirPoint1Widget->setEnabled(false);
        m_repDirPoint1Widget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_patternRepStatus) this->updatePointRender("patternRep_dir1", pt, QVector3D(1.0, 0.0, 1.0));
        });
        patternRepFold->addRow(m_repDirPoint1Widget);

        m_repDirPoint2Widget = new RunnerPickWidget("方向点2", _optimizationDesignData, &mRunnerOptimizationDesignData::_repDirPoint2, getPickPointData, this);
        m_repDirPoint2Widget->setMarkColor(QColor(210, 105, 30)); m_repDirPoint2Widget->setEnabled(false);
        m_repDirPoint2Widget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_patternRepStatus) this->updatePointRender("patternRep_dir2", pt, QVector3D(210.0 / 255.0, 105.0 / 255.0, 30.0 / 255.0));
        });
        patternRepFold->addRow(m_repDirPoint2Widget);

        m_repDirPoint3Widget = new RunnerPickWidget("方向点3", _optimizationDesignData, &mRunnerOptimizationDesignData::_repDirPoint3, getPickPointData, this);
        m_repDirPoint3Widget->setMarkColor(QColor(127, 255, 0)); m_repDirPoint3Widget->setEnabled(false);
        m_repDirPoint3Widget->setValueChangedCallback([this](const QVector3D& pt) {
            if (_optimizationDesignData && _optimizationDesignData->_patternRepStatus) this->updatePointRender("patternRep_dir3", pt, QVector3D(127.0 / 255.0, 1.0, 0.0));
        });
        patternRepFold->addRow(m_repDirPoint3Widget);

        connect(m_checkPatternRepStatus, &QCheckBox::stateChanged, this, &mRunnerOptimizationDesignPanel::slot_checkPatternRepChanged);
        connect(m_comboDomain, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_comboPatternRepDomainChanged);
        connect(m_comboDomainType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_comboPatternRepTypeChanged);
        connect(m_comboCoordType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationDesignPanel::slot_comboPatternRepCoordChanged);
        connect(m_lineScaleX, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_scaleX = m_lineScaleX->text(); });
        connect(m_lineScaleY, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_scaleY = m_lineScaleY->text(); });
        connect(m_lineScaleZ, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_scaleZ = m_lineScaleZ->text(); });

        m_foldWidgets.append(patternRepFold);
        patternRepFold->setParent(nullptr);

        // 将所有约束组添加到主布局
        _mainLayout->addWidget(m_foldWidgets[2]);  // 对称约束
        _mainLayout->addWidget(m_foldWidgets[3]);  // 拔模约束
        _mainLayout->addWidget(m_foldWidgets[4]);  // 挤出约束
        _mainLayout->addWidget(m_foldWidgets[5]);  // 自支撑约束
        _mainLayout->addWidget(m_foldWidgets[6]);  // 模式重复

        _mainLayout->addStretch();
    }

    mRunnerOptimizationDesignPanel::~mRunnerOptimizationDesignPanel()
    {
        if (_mainLayout != nullptr) {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mRunnerOptimizationDesignPanel::createRunnerOptimizationDesign(QString type)
    {
        if (!getCVM) return;
        auto runnerOption = getCVM->getModelData()->getRunnerStructOption();
        int ID = runnerOption->createOptimizationDesignID();
        int cnt = 1;
        QString text = "设计域";
        QString name = text + "_" + MxFunctions::mNumberToString(cnt++);
        while (runnerOption->existOptimizationDesignName(name)) {
            name = text + "_" + MxFunctions::mNumberToString(cnt++);
        }
        _optimizationDesignData = new mRunnerOptimizationDesignData(ID, name);
        _optimizationDesignData->_designDomainType = type;
        runnerOption->addOptimizationDesignData(_optimizationDesignData);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建流道设计域 %1").arg(_optimizationDesignData->getName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(true, _optimizationDesignData->getId());
    }

    void mRunnerOptimizationDesignPanel::editRunnerOptimizationDesignPanel(int id)
    {
        if (!getCVM) return;
        _optimizationDesignData = getCVM->getModelData()->getRunnerStructOption()->getOptimizationDesignData(id);
        if (_optimizationDesignData == nullptr) return;
        setEditing(true);

        // 更新设计域属性组
        _comboDesignDomainType->blockSignals(true);
        _comboDesignDomainType->setCurrentText(_optimizationDesignData->_designDomainType);
        _comboDesignDomainType->blockSignals(false);

        updatePropertyCombobox(_comboProperty);

        // 回显达西惩罚项
        const QSignalBlocker blockerMaxPermCheck(m_checkMaxPermeability);
        const QSignalBlocker blockerMaxPermLine(m_lineMaxPermeability);
        const QSignalBlocker blockerMinPermCheck(m_checkMinPermeability);
        const QSignalBlocker blockerMinPermLine(m_lineMinPermeability);

        m_checkMaxPermeability->setChecked(_optimizationDesignData->_maxPermeabilityStatus);
        m_lineMaxPermeability->setExpression(_optimizationDesignData->_maxPermeability);
        m_lineMaxPermeability->setEnabled(_optimizationDesignData->_maxPermeabilityStatus);

        m_checkMinPermeability->setChecked(_optimizationDesignData->_minPermeabilityStatus);
        m_lineMinPermeability->setExpression(_optimizationDesignData->_minPermeability);
        m_lineMinPermeability->setEnabled(_optimizationDesignData->_minPermeabilityStatus);

        // 回显设计域参数
        const QSignalBlocker blockerMinCheck(m_checkMinMember);
        const QSignalBlocker blockerMinLine(m_lineMinMember);
        const QSignalBlocker blockerMaxCheck(m_checkMaxMember);
        const QSignalBlocker blockerMaxLine(m_lineMaxMember);
        const QSignalBlocker blockerGapCheck(m_checkMinGap);
        const QSignalBlocker blockerGapLine(m_lineMinGap);

        m_checkMinMember->setChecked(_optimizationDesignData->_minMemberStatus);
        m_lineMinMember->setExpression(_optimizationDesignData->_minMemberSize);
        m_lineMinMember->setEnabled(_optimizationDesignData->_minMemberStatus);

        m_checkMaxMember->setChecked(_optimizationDesignData->_maxMemberStatus);
        m_lineMaxMember->setExpression(_optimizationDesignData->_maxMemberSize);
        m_lineMaxMember->setEnabled(_optimizationDesignData->_maxMemberStatus);

        m_checkMinGap->setChecked(_optimizationDesignData->_minGapStatus);
        m_lineMinGap->setExpression(_optimizationDesignData->_minGapSize);
        m_lineMinGap->setEnabled(_optimizationDesignData->_minGapStatus);

        // 对称约束组
        m_checkPattern->setChecked(_optimizationDesignData->_patternStatus);
        m_comboPattern->blockSignals(true);
        m_comboPattern->setCurrentText(_optimizationDesignData->_patternType);
        m_comboPattern->blockSignals(false);
        slot_comboPatternChanged(_optimizationDesignData->_patternType);
        const QSignalBlocker blockerPatternNum(m_linePatternNum);
        m_linePatternNum->setExpression(_optimizationDesignData->_patternNum);

        // 拔模约束组
        m_checkDraftType->setChecked(_optimizationDesignData->_liftingStatus);
        m_comboDraftType->blockSignals(true);
        m_comboDraftType->setCurrentText(_optimizationDesignData->_liftingType);
        m_comboDraftType->blockSignals(false);
        m_comboNoHole->blockSignals(true);
        m_comboNoHole->setCurrentText(_optimizationDesignData->_nohole);
        m_comboNoHole->blockSignals(false);
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftDir");
        }
        if (m_startPointWidget) m_startPointWidget->setObject(_optimizationDesignData);
        if (m_draftDirWidget) m_draftDirWidget->setObject(_optimizationDesignData);

        // 挤出约束组
        m_checkExtrude->setChecked(_optimizationDesignData->_extrusionStatus);
        m_comboExtrudeType->blockSignals(true);
        m_comboExtrudeType->setCurrentText(_optimizationDesignData->_extrusionType);
        m_comboExtrudeType->blockSignals(false);
        slot_comboExtrudeChanged(_optimizationDesignData->_extrusionType);

        // 自支撑约束组
        const QSignalBlocker blockerLine(m_lineOverhangAngle);
        m_checkSupportStatus->setChecked(_optimizationDesignData->_supportStatus);
        m_lineOverhangAngle->setExpression(_optimizationDesignData->_overhangAngle);
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
        }
        if (m_supportStartWidget) m_supportStartWidget->setObject(_optimizationDesignData);
        if (m_supportDirWidget) m_supportDirWidget->setObject(_optimizationDesignData);

        // 模式重复组
        updateDomainCombobox(m_comboDomain);

        const QSignalBlocker blockerCombo(m_comboDomainType);
        const QSignalBlocker blockerX(m_lineScaleX);
        const QSignalBlocker blockerY(m_lineScaleY);
        const QSignalBlocker blockerZ(m_lineScaleZ);

        bool status = _optimizationDesignData->_patternRepStatus;
        m_checkPatternRepStatus->setChecked(status);
        m_comboDomainType->setCurrentText(_optimizationDesignData->_domainType);
        m_comboCoordType->blockSignals(true);
        m_comboCoordType->setCurrentText(_optimizationDesignData->_coordType);
        m_comboCoordType->blockSignals(false);
        slot_comboPatternRepTypeChanged(_optimizationDesignData->_domainType);
        slot_comboPatternRepCoordChanged(_optimizationDesignData->_coordType);
        m_lineScaleX->setExpression(_optimizationDesignData->_scaleX);
        m_lineScaleY->setExpression(_optimizationDesignData->_scaleY);
        m_lineScaleZ->setExpression(_optimizationDesignData->_scaleZ);

        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_base");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir2");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir3");
        }
        if (m_repBasePointWidget) m_repBasePointWidget->setObject(_optimizationDesignData);
        if (m_repDirPoint1Widget) m_repDirPoint1Widget->setObject(_optimizationDesignData);
        if (m_repDirPoint2Widget) m_repDirPoint2Widget->setObject(_optimizationDesignData);
        if (m_repDirPoint3Widget) m_repDirPoint3Widget->setObject(_optimizationDesignData);
    }

    void mRunnerOptimizationDesignPanel::uneditRunnerOptimizationDesignPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _optimizationDesignData = nullptr;
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("origin");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftStart");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftDir");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("path1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("path2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_base");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir3");
    }

    void mRunnerOptimizationDesignPanel::updatePointRender(QString str, QVector3D point, QVector3D color)
    {
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint(str, { point }, color);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mRunnerOptimizationDesignPanel::updatePointRender(QString str, QVector<int> nodes, QVector3D color)
    {
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        auto* meshData = getCVM->getModelData()->getMeshData();
        QVector<QVector3D> points;
        for (const auto& id : nodes) {
            double* v = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;
            points.append(QVector3D(v[0], v[1], v[2]));
        }
        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint(str, points, color);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    bool mRunnerOptimizationDesignPanel::getPickPathData(QVector<int>& ids)
    {
        auto* preRender = getCVM->getPreRenderWidget();
        auto* selContainer = preRender->getSelectionContainer();
        auto nodes = selContainer->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
        if (!nodes.empty()) {
            int num = nodes.size();
            ids.resize(num);
            for (int i = 0; i < num; i++) {
                ids[i] = nodes.at(i).data_id.node_id;
            }
            return true;
        }
        return false;
    }

    void mRunnerOptimizationDesignPanel::updatePropertyCombobox(QComboBox* combobox)
    {
        if (!_optimizationDesignData || !getCVM) return;
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_Property);
        QString currentText = propertyManager.getPropertyNameByID(propertyId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        QStringList names = propertyManager.getAllPropertyNamesByTypes(SolidProperty);
        combobox->addItems(names);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0);
        }
        combobox->blockSignals(false);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mRunnerOptimizationDesignPanel::updateDomainCombobox(QComboBox* combobox)
    {
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int designId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_OptimationDesign);
        QString currentText = getCVM->getModelData()->getRunnerStructOption()->getOptimizationDesignNameByID(designId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        QStringList names = getCVM->getModelData()->getRunnerStructOption()->getAllOptimizationMasterDesignNames();
        names.removeOne(_optimizationDesignData->getName());
        combobox->addItems(names);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0);
        }
        combobox->blockSignals(false);
    }

    void mRunnerOptimizationDesignPanel::slot_checkPatternChanged(int state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_patternStatus = (state == Qt::Checked);
        if (_optimizationDesignData->_patternStatus) {
            QString type = _optimizationDesignData->_patternType;
            if (m_originWidget) m_originWidget->setObject(_optimizationDesignData);
            if (type != "周向") { if (m_dirPoint1Widget) m_dirPoint1Widget->setObject(_optimizationDesignData); }
            if (type != "单平面") { if (m_dirPoint2Widget) m_dirPoint2Widget->setObject(_optimizationDesignData); }
        } else {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("origin");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction2");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_comboPattern->setEnabled(_optimizationDesignData->_patternStatus);
        m_linePatternNum->setEnabled(_optimizationDesignData->_patternStatus);
        m_originWidget->setEnabled(_optimizationDesignData->_patternStatus);
        m_dirPoint1Widget->setEnabled(_optimizationDesignData->_patternStatus);
        m_dirPoint2Widget->setEnabled(_optimizationDesignData->_patternStatus);
    }

    void mRunnerOptimizationDesignPanel::slot_comboPatternChanged(const QString& text)
    {
        m_lblPatternNum->setParent(nullptr);
        m_linePatternNum->setParent(nullptr);
        m_originWidget->setParent(nullptr);
        m_dirPoint1Widget->setParent(nullptr);
        m_dirPoint2Widget->setParent(nullptr);
        auto fold = m_foldWidgets[2];
        _optimizationDesignData->_patternType = text;
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("origin");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction2");
        }
        QString type = _optimizationDesignData->_patternType;
        if (m_originWidget) m_originWidget->setObject(_optimizationDesignData);
        if (type == "周向" || type == "周向+平面") fold->addRow(m_lblPatternNum, m_linePatternNum);
        fold->addRow(m_originWidget);
        fold->addRow(m_dirPoint1Widget); if (m_dirPoint1Widget) m_dirPoint1Widget->setObject(_optimizationDesignData);
        if (type != "单平面" && type != "周向") { fold->addRow(m_dirPoint2Widget); if (m_dirPoint2Widget) m_dirPoint2Widget->setObject(_optimizationDesignData); }
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mRunnerOptimizationDesignPanel::slot_checkDraftChanged(int state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_liftingStatus = (state == Qt::Checked);
        if (_optimizationDesignData->_liftingStatus) {
            if (m_startPointWidget) m_startPointWidget->setObject(_optimizationDesignData);
            if (m_draftDirWidget) m_draftDirWidget->setObject(_optimizationDesignData);
        } else {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftDir");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_comboDraftType->setEnabled(_optimizationDesignData->_liftingStatus);
        m_comboNoHole->setEnabled(_optimizationDesignData->_liftingStatus);
        m_startPointWidget->setEnabled(_optimizationDesignData->_liftingStatus);
        m_draftDirWidget->setEnabled(_optimizationDesignData->_liftingStatus);
    }

    void mRunnerOptimizationDesignPanel::slot_checkExtrudeChanged(int state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_extrusionStatus = (state == Qt::Checked);
        if (_optimizationDesignData->_extrusionStatus) {
            QString type = _optimizationDesignData->_extrusionType;
            if (m_path1Widget) m_path1Widget->setObject(_optimizationDesignData);
            QString extrudeType = m_comboExtrudeType->currentText();
            if (extrudeType == "扭转") {
                if (m_path2Widget) m_path2Widget->setObject(_optimizationDesignData);
            } else {
                _optimizationDesignData->_path2.clear();
            }
        } else {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("path1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("path2");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_comboExtrudeType->setEnabled(_optimizationDesignData->_extrusionStatus);
        m_path1Widget->setEnabled(_optimizationDesignData->_extrusionStatus);
        m_path2Widget->setEnabled(_optimizationDesignData->_extrusionStatus);
    }

    void mRunnerOptimizationDesignPanel::slot_comboExtrudeChanged(const QString& text)
    {
        if (m_path2Widget) m_path2Widget->setParent(nullptr);
        auto fold = m_foldWidgets[4];
        _optimizationDesignData->_extrusionType = text;
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("path2");
        }
        if (m_path1Widget) m_path1Widget->setObject(_optimizationDesignData);
        QString extrudeType = m_comboExtrudeType->currentText();
        if (extrudeType == "扭转") {
            if (fold && m_path2Widget) { fold->addRow(m_path2Widget); m_path2Widget->setObject(_optimizationDesignData); }
        } else {
            _optimizationDesignData->_path2.clear();
        }
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mRunnerOptimizationDesignPanel::slot_checkSupportChanged(int state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_supportStatus = (state == Qt::Checked);
        if (_optimizationDesignData->_supportStatus) {
            if (m_supportStartWidget) m_supportStartWidget->setObject(_optimizationDesignData);
            if (m_supportDirWidget) m_supportDirWidget->setObject(_optimizationDesignData);
        } else {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_lineOverhangAngle->setEnabled(_optimizationDesignData->_supportStatus);
        m_supportStartWidget->setEnabled(_optimizationDesignData->_supportStatus);
        m_supportDirWidget->setEnabled(_optimizationDesignData->_supportStatus);
    }

    void mRunnerOptimizationDesignPanel::slot_checkPatternRepChanged(int state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_patternRepStatus = (state == Qt::Checked);
        if (_optimizationDesignData->_patternRepStatus) {
            m_repBasePointWidget->setObject(_optimizationDesignData);
            m_repDirPoint1Widget->setObject(_optimizationDesignData);
            m_repDirPoint2Widget->setObject(_optimizationDesignData);
            m_repDirPoint3Widget->setObject(_optimizationDesignData);
        } else {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_base");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir2");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir3");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_comboDomainType->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_comboDomain->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_comboCoordType->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_lineScaleX->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_lineScaleY->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_lineScaleZ->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_repBasePointWidget->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_repDirPoint1Widget->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_repDirPoint2Widget->setEnabled(_optimizationDesignData->_patternRepStatus);
        m_repDirPoint3Widget->setEnabled(_optimizationDesignData->_patternRepStatus);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mRunnerOptimizationDesignPanel::slot_comboPatternRepTypeChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_domainType = text;
        auto fold = m_foldWidgets[6];
        int id = _optimizationDesignData->getId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        if (_optimizationDesignData->_domainType == "从设计域") {
            mapRelation.deleteMapRelation({ PhysicalFieldType::Physical_OptimationDesign, id }, { PhysicalFieldType::Physical_OptimationDesign, -1 });
            fold->insertRow(1, m_lblDomain, m_comboDomain);
        } else {
            mapRelation.appendMapRelation({ PhysicalFieldType::Physical_OptimationDesign, id }, { PhysicalFieldType::Physical_OptimationDesign, -1 });
            m_lblDomain->setParent(nullptr); m_comboDomain->setParent(nullptr);
        }
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mRunnerOptimizationDesignPanel::slot_comboPatternRepDomainChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int designId = getCVM->getModelData()->getRunnerStructOption()->getOptimizationDesignIDByName(text);
        int id = _optimizationDesignData->getId();
        mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_OptimationDesign, designId });
        m_comboDomain->setCurrentIndex(designId == -1 ? 0 : m_comboDomain->findText(text));
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mRunnerOptimizationDesignPanel::slot_comboPatternRepCoordChanged(const QString& text)
    {
        m_repDirPoint1Widget->setParent(nullptr); m_repDirPoint2Widget->setParent(nullptr); m_repDirPoint3Widget->setParent(nullptr);
        auto fold = m_foldWidgets[6];
        _optimizationDesignData->_coordType = text;
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_base");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir2");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir3");
        }
        m_repBasePointWidget->setObject(_optimizationDesignData);
        if (text == "指定点") {
            fold->addRow(m_repDirPoint1Widget); fold->addRow(m_repDirPoint2Widget); fold->addRow(m_repDirPoint3Widget);
            m_repDirPoint1Widget->setObject(_optimizationDesignData); m_repDirPoint2Widget->setObject(_optimizationDesignData); m_repDirPoint3Widget->setObject(_optimizationDesignData);
        }
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mRunnerOptimizationDesignPanel::slot_propertyChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = propertyManager.getIDByPropertyName(text);
        int id = _optimizationDesignData->getId();
        mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_Property, propertyId });
        _comboProperty->setCurrentIndex(propertyId == -1 ? 0 : _comboProperty->findText(text));
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationDesignSig(false, _optimizationDesignData->getId());
    }
}
