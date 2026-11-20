
#include "mOptimizationDesignPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mOtherFunction.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mGeneralPickUp.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mStructOption.h"
#include "mOptimizationDesignData.h"
#include "mPropertyManager.h"
#include "mCoordinateManager.h"
#include "mCalcLineEdit.h"
#include "mSetManager.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
using namespace MechPreData;
using namespace MechPropertyData;
using namespace MechStructOptionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mOptimizationDesignPanel::mOptimizationDesignPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _optimizationDesignData = nullptr;

        _mainLayout = new QVBoxLayout(this);

        //设计域属性组
        FoldSection* designFold = new FoldSection("设计域属性", this);

        // 设计域类型下拉框
        _comboDesignDomainType = new QComboBox();
        _comboDesignDomainType->addItems({ "实体", "壳"/*, "形状"*/ });
        designFold->addRow(new PanelLabel("设计域类型"), _comboDesignDomainType);
        connect(_comboDesignDomainType, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_designDomainTypeChanged);

        // 形状专用复选框（默认隐藏）
        m_checkShapeSplitDomain = new QCheckBox("按域分割", this);
        m_checkShapeSplitPart = new QCheckBox("按部件分割", this);
        m_checkShapeKeepHandle = new QCheckBox("保留控制柄", this);
        m_checkShape2DSplit = new QCheckBox("2D域分割", this);
        m_checkShapeSplitDomain->setParent(nullptr);
        m_checkShapeSplitPart->setParent(nullptr);
        m_checkShapeKeepHandle->setParent(nullptr);
        m_checkShape2DSplit->setParent(nullptr);

        connect(m_checkShapeSplitDomain, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_shapeSplitDomain = (state == Qt::Checked);
        });
        connect(m_checkShapeSplitPart, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_shapeSplitPart = (state == Qt::Checked);
        });
        connect(m_checkShapeKeepHandle, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_shapeKeepHandle = (state == Qt::Checked);
        });
        connect(m_checkShape2DSplit, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_shape2DSplit = (state == Qt::Checked);
        });

        // 指定设计域下拉框
        _comboProperty = new QComboBox();
        designFold->addRow(new PanelLabel("指定设计域"), _comboProperty);
        connect(_comboProperty, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_propertyChanged);

        // 优化方式下拉框（默认先添加，后面实体时隐藏）
        m_lblOptimizationMethod = new PanelLabel("优化方式", this);
        m_comboOptimizationMethod = new QComboBox();
        m_comboOptimizationMethod->addItems({ "厚度", "加筋"/*, "形貌"*/ });
        m_lblOptimizationMethod->setParent(nullptr);
        m_comboOptimizationMethod->setParent(nullptr);
        //designFold->addRow(m_lblOptimizationMethod, m_comboOptimizationMethod);
        connect(m_comboOptimizationMethod, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_optimizationMethodChanged);

        // 基准厚度输入框
        m_lblBaseThickness = new PanelLabel("基准厚度", this);
        m_lineBaseThickness = new mCalcLineEdit(this, false, false, true);
        m_lblBaseThickness->setParent(nullptr);
        m_lineBaseThickness->setParent(nullptr);
        //designFold->addRow(m_lblBaseThickness, m_lineBaseThickness);

        // 冻结区域（实体/壳时显示）
        m_lblFreezeRegion = new PanelLabel("冻结区域", this);
        m_comboFreezeRegion = new QComboBox(this);
        m_lblFreezeRegion->setParent(nullptr);
        m_comboFreezeRegion->setParent(nullptr);

        connect(m_comboFreezeRegion, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (!_optimizationDesignData || !getCVM) return;
            auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            int setId = setManager.getIDBySetName(text);
            int id = _optimizationDesignData->getId();
            mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_Set, setId });
            m_comboFreezeRegion->setCurrentIndex(setId == -1 ? -1 : m_comboFreezeRegion->findText(text));
            //emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
        });

        m_foldWidgets.append(designFold);
        _mainLayout->addWidget(designFold);

        // ==========================================
        // 设计域参数组/加强筋参数组
        // ==========================================
        auto paramFold = new FoldSection("设计域参数", this, QColor("#909399"), false);

        // 最小成员尺寸
        m_checkMinMember = new QCheckBox("最小成员尺寸", this);
        m_lineMinMember = new mCalcLineEdit(mCalcLineEdit::CalcMode::NoEmptyAndPositive, this); m_lineMinMember->setEnabled(false);
        paramFold->addRow(m_checkMinMember, m_lineMinMember);

        // 最大成员尺寸
        m_checkMaxMember = new QCheckBox("最大成员尺寸", this);
        m_lineMaxMember = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineMaxMember->setEnabled(false);
        paramFold->addRow(m_checkMaxMember, m_lineMaxMember);

        // 最小间隙
        m_checkMinGap = new QCheckBox("最小间隙", this);
        m_lineMinGap = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineMinGap->setEnabled(false);
        paramFold->addRow(m_checkMinGap, m_lineMinGap);

        // 应力约束
        m_checkStressConstraint = new QCheckBox("应力约束", this);
        m_lineStressConstraint = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineStressConstraint->setEnabled(false);
        paramFold->addRow(m_checkStressConstraint, m_lineStressConstraint);

        // 加强筋高度
        m_checkRibHeight = new QCheckBox("加强筋高度", this);
        m_lineRibHeight = new mCalcLineEdit(this, false, false, true); m_lineRibHeight->setEnabled(false);
        m_checkRibHeight->setParent(nullptr);
        m_lineRibHeight->setParent(nullptr);

        // 加强筋宽度
        m_checkRibWidth = new QCheckBox("加强筋宽度", this);
        m_lineRibWidth = new mCalcLineEdit(this, false, false, true); m_lineRibWidth->setEnabled(false);
        m_checkRibWidth->setParent(nullptr);
        m_lineRibWidth->setParent(nullptr);

        m_foldWidgets.append(paramFold);
        _mainLayout->addWidget(paramFold);

        // ==========================================
        // 信号槽绑定
        // ==========================================

        // --- 最小成员尺寸 ---
        connect(m_checkMinMember, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_minMemberStatus = (state == Qt::Checked);
                m_lineMinMember->setEnabled(_optimizationDesignData->_minMemberStatus);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });
        connect(m_lineMinMember, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_minMemberSize = m_lineMinMember->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });

        // --- 最大成员尺寸 ---
        connect(m_checkMaxMember, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_maxMemberStatus = (state == Qt::Checked);
                m_lineMaxMember->setEnabled(_optimizationDesignData->_maxMemberStatus);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });
        connect(m_lineMaxMember, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_maxMemberSize = m_lineMaxMember->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });

        // --- 最小间隙 ---
        connect(m_checkMinGap, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_minGapStatus = (state == Qt::Checked);
                m_lineMinGap->setEnabled(_optimizationDesignData->_minGapStatus);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });
        connect(m_lineMinGap, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_minGapSize = m_lineMinGap->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });

        // --- 应力约束 ---
        connect(m_checkStressConstraint, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_stressConstraintStatus = (state == Qt::Checked);
                m_lineStressConstraint->setEnabled(_optimizationDesignData->_stressConstraintStatus);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });
        connect(m_lineStressConstraint, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_stressConstraintValue = m_lineStressConstraint->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });

        // --- 基准厚度 ---
        connect(m_lineBaseThickness, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_baseThickness = m_lineBaseThickness->text();
        });

        // --- 加强筋高度 ---
        connect(m_checkRibHeight, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_ribHeightStatus = (state == Qt::Checked);
            m_lineRibHeight->setEnabled(_optimizationDesignData->_ribHeightStatus);
        });
        connect(m_lineRibHeight, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_ribHeight = m_lineRibHeight->text();
        });

        // --- 加强筋宽度 ---
        connect(m_checkRibWidth, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationDesignData) _optimizationDesignData->_ribWidthStatus = (state == Qt::Checked);
            m_lineRibWidth->setEnabled(_optimizationDesignData->_ribWidthStatus);
        });
        connect(m_lineRibWidth, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_ribWidth = m_lineRibWidth->text();
        });


        //对称约束组
        // ==========================================
        // 对称约束组
        // ==========================================
        FoldSection* patternFold = new FoldSection("对称约束", this, QColor("#909399"), false);patternFold->setCheckable(true);
        m_lblPattern = new PanelLabel("对称方式"); m_comboPattern = new QComboBox(); m_comboPattern->addItems({ "单平面", "双平面", "三平面", "周向", "周向+平面" }); m_comboPattern->setEnabled(false);
        connect(patternFold, &FoldWidget::enabledChanged, this, &mOptimizationDesignPanel::slot_checkPatternChanged);
        connect(m_comboPattern, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_comboPatternChanged);
        patternFold->addRow(m_lblPattern, m_comboPattern);

        m_lblPatternNum = new PanelLabel("周向对称数量", this); m_lblPatternNum->setParent(nullptr);
        m_linePatternNum = new mCalcLineEdit(this); m_linePatternNum->setEnabled(false); m_linePatternNum->setParent(nullptr);

        // --- 周向对称数量 ---
        connect(m_linePatternNum, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_patternNum = m_linePatternNum->text();
            });

        m_originWidget = createPointPicker(QStringLiteral("原点"), &mOptimizationDesignData::_origin, QStringLiteral("origin"), QVector3D(1, 0, 0));
        m_originWidget->setParent(nullptr);

        m_dirPoint1Widget = createPointPicker(QStringLiteral("方向点1"), &mOptimizationDesignData::_dirPoint1, QStringLiteral("direction1"), QVector3D(0, 1, 0));
        m_dirPoint1Widget->setParent(nullptr);

        m_dirPoint2Widget = createPointPicker(QStringLiteral("方向点2"), &mOptimizationDesignData::_dirPoint2, QStringLiteral("direction2"), QVector3D(0, 0, 1));
        m_dirPoint2Widget->setParent(nullptr);

        m_foldWidgets.append(patternFold);
        patternFold->setParent(nullptr);

        // ==========================================
        // 拔模约束组
        // ==========================================
        auto draftFold = new FoldSection("拔模约束", this, QColor("#909399"), false);draftFold->setCheckable(true);

        // 1. 拔模方式 (复选框 + 下拉框)
        m_lblDraftType = new PanelLabel("拔模方式", this);
        m_comboDraftType = new QComboBox(this); m_comboDraftType->setEnabled(false);
        m_comboDraftType->addItems({ "单向", "双向" });
        draftFold->addRow(m_lblDraftType, m_comboDraftType);
        connect(draftFold, &FoldWidget::enabledChanged, this, &mOptimizationDesignPanel::slot_checkDraftChanged);
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

        // 3. 起始点 (使用计算点控件)
        m_startPointWidget = createPointPicker(QStringLiteral("起始点"), &mOptimizationDesignData::_startPoint, QStringLiteral("draftStart"), QVector3D(1.0, 0.65, 0.0));
        draftFold->addRow(m_startPointWidget);

        // 4. 方向点 (使用计算点控件)
        m_draftDirWidget = createPointPicker(QStringLiteral("方向点"), &mOptimizationDesignData::_dirPoint, QStringLiteral("draftDir"), QVector3D(0.5, 0.0, 0.5));
        draftFold->addRow(m_draftDirWidget);

        m_foldWidgets.append(draftFold);
        draftFold->setParent(nullptr);

        // ==========================================
        // 挤出约束组
        // ==========================================
        auto extrusionFold = new FoldSection("挤出约束", this, QColor("#909399"), false);extrusionFold->setCheckable(true);

        // 1. 挤出方式 (下拉框)
        m_lblExtrude = new PanelLabel("挤出方式");
        m_comboExtrudeType = new QComboBox(this); m_comboExtrudeType->setEnabled(false);
        m_comboExtrudeType->addItems({ "平行", "扭转" });
        extrusionFold->addRow(m_lblExtrude, m_comboExtrudeType);

        connect(extrusionFold, &FoldWidget::enabledChanged, this, &mOptimizationDesignPanel::slot_checkExtrudeChanged);
        connect(m_comboExtrudeType, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_comboExtrudeChanged);

        // 2. 路径1（通用拾取控件，仅节点拾取，路径节点按拾取先后顺序存储）
        m_path1Controller = createPathPickWidget(QStringLiteral("路径1"), &mOptimizationDesignData::_path1);
        m_path1Controller->widget()->setEnabled(false);

        // 3. 路径2（扭转时显示）
        m_path2Controller = createPathPickWidget(QStringLiteral("路径2"), &mOptimizationDesignData::_path2);
        m_path2Controller->widget()->setEnabled(false);
        m_path2Controller->widget()->setParent(nullptr);

        extrusionFold->addRow(m_path1Controller->widget());
        m_foldWidgets.append(extrusionFold);
        extrusionFold->setParent(nullptr);

        // ==========================================
        // 自支撑约束组
        // ==========================================
        auto supportFold = new FoldSection("自支撑约束", this, QColor("#909399"), false);supportFold->setCheckable(true);

        m_lblSupportStatus = new PanelLabel("悬垂角", this);
        m_lineOverhangAngle = new mCalcLineEdit(mCalcLineEdit::CalcMode::NoEmpty, this);
        m_lineOverhangAngle->setEnabled(false);
        supportFold->addRow(m_lblSupportStatus, m_lineOverhangAngle);

        connect(m_lineOverhangAngle, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) {
                _optimizationDesignData->_overhangAngle = m_lineOverhangAngle->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
        });

        m_supportStartWidget = createPointPicker(QStringLiteral("起始点"), &mOptimizationDesignData::_supportStartPoint, QStringLiteral("supportStart"), QVector3D(1.0, 140.0 / 255.0, 0.0));
        supportFold->addRow(m_supportStartWidget);

        m_supportDirWidget = createPointPicker(QStringLiteral("方向点"), &mOptimizationDesignData::_supportDirPoint, QStringLiteral("supportDir"), QVector3D(0.0, 191.0 / 255.0, 1.0));
        supportFold->addRow(m_supportDirWidget);

        connect(supportFold, &FoldWidget::enabledChanged, this, &mOptimizationDesignPanel::slot_checkSupportChanged);
        m_foldWidgets.append(supportFold);
        supportFold->setParent(nullptr);

        // ==========================================
        // 模式重复组
        // ==========================================
        auto patternRepFold = new FoldSection("模式重复", this, QColor("#909399"), false);patternRepFold->setCheckable(true);

        m_lblPatternRepStatus = new PanelLabel("类型", this);
        m_comboDomainType = new QComboBox(this); m_comboDomainType->setEnabled(false);
        m_comboDomainType->addItems({ "主设计域", "从设计域" });
        patternRepFold->addRow(m_lblPatternRepStatus, m_comboDomainType);

        m_lblDomain = new PanelLabel("参考设计域", this);
        m_comboDomain = new QComboBox(this); m_comboDomain->setEnabled(false);
        m_lblDomain->setParent(nullptr);
        m_comboDomain->setParent(nullptr);

        m_comboCoordType = new QComboBox(this);
        m_comboCoordType->addItems({ "全局坐标系", "指定点" }); m_comboCoordType->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("坐标系", this), m_comboCoordType);

        m_lineScaleX = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineScaleX->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 X", this), m_lineScaleX);
        m_lineScaleY = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineScaleY->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 Y", this), m_lineScaleY);
        m_lineScaleZ = new mCalcLineEdit(mCalcLineEdit::CalcMode::Positive, this); m_lineScaleZ->setEnabled(false);
        patternRepFold->addRow(new PanelLabel("缩放系数 Z", this), m_lineScaleZ);

        m_repBasePointWidget = createPointPicker(QStringLiteral("基准点"), &mOptimizationDesignData::_repBasePoint, QStringLiteral("patternRep_base"), QVector3D(0.0, 1.0, 1.0));
        patternRepFold->addRow(m_repBasePointWidget);

        m_repDirPoint1Widget = createPointPicker(QStringLiteral("方向点1"), &mOptimizationDesignData::_repDirPoint1, QStringLiteral("patternRep_dir1"), QVector3D(1.0, 0.0, 1.0));
        patternRepFold->addRow(m_repDirPoint1Widget);

        m_repDirPoint2Widget = createPointPicker(QStringLiteral("方向点2"), &mOptimizationDesignData::_repDirPoint2, QStringLiteral("patternRep_dir2"), QVector3D(210.0 / 255.0, 105.0 / 255.0, 30.0 / 255.0));
        patternRepFold->addRow(m_repDirPoint2Widget);

        m_repDirPoint3Widget = createPointPicker(QStringLiteral("方向点3"), &mOptimizationDesignData::_repDirPoint3, QStringLiteral("patternRep_dir3"), QVector3D(127.0 / 255.0, 1.0, 0.0));
        patternRepFold->addRow(m_repDirPoint3Widget);

        connect(patternRepFold, &FoldWidget::enabledChanged, this, &mOptimizationDesignPanel::slot_checkPatternRepChanged);
        connect(m_comboDomain, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_comboPatternRepDomainChanged);
        connect(m_comboDomainType, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_comboPatternRepTypeChanged);
        connect(m_comboCoordType, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_comboPatternRepCoordChanged);
        connect(m_lineScaleX, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_scaleX = m_lineScaleX->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }});
        connect(m_lineScaleY, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_scaleY = m_lineScaleY->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }});
        connect(m_lineScaleZ, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData)
            {
                _optimizationDesignData->_scaleZ = m_lineScaleZ->text();
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
            }
            });

        m_foldWidgets.append(patternRepFold);
        patternRepFold->setParent(nullptr);

        // ==========================================
        // 形貌-起筋参数
        // ==========================================
        auto morphParamFold = new FoldSection("起筋参数", this, QColor("#909399"), false);
        m_lblMorphUpLimit = new PanelLabel("起筋高度", this);
        m_lineMorphUpLimit = new mCalcLineEdit(this, false, false, true);
        morphParamFold->addRow(m_lblMorphUpLimit, m_lineMorphUpLimit);

        m_lblMorphDownLimit = new PanelLabel("起筋宽度", this);
        m_lineMorphDownLimit = new mCalcLineEdit(this, false, false, true);
        morphParamFold->addRow(m_lblMorphDownLimit, m_lineMorphDownLimit);

        m_lblMorphInitRatio = new PanelLabel("初始起筋比", this);
        m_comboMorphInitRatio = new QComboBox(this);
        m_comboMorphInitRatio->addItems({ "默认" });
        morphParamFold->addRow(m_lblMorphInitRatio, m_comboMorphInitRatio);

        connect(m_lineMorphUpLimit, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_morphUpLimit = m_lineMorphUpLimit->text();
        });
        connect(m_lineMorphDownLimit, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_morphDownLimit = m_lineMorphDownLimit->text();
        });
        connect(m_comboMorphInitRatio, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (_optimizationDesignData) _optimizationDesignData->_morphInitRatio = text;
        });

        m_foldWidgets.append(morphParamFold);
        _mainLayout->addWidget(morphParamFold);
        m_morphParamFold = morphParamFold;
        m_morphParamFold->setParent(nullptr);

        // ==========================================
        // 形貌-起筋模式
        // ==========================================
        auto morphModeFold = new FoldSection("起筋模式", this, QColor("#909399"), false);
        m_morphModeFold = morphModeFold;

        m_comboMorphMode = new QComboBox(this);
        m_comboMorphMode->addItems({ "无", "线性", "周向", "对称" });
        morphModeFold->addRow(new PanelLabel("起筋模式", this), m_comboMorphMode);

        m_lineMorphMinWidth = new mCalcLineEdit(this, false, false, true);
        morphModeFold->addRow(new PanelLabel("最小宽度", this), m_lineMorphMinWidth);

        m_lineMorphAngle = new mCalcLineEdit(this, false, false, true);
        morphModeFold->addRow(new PanelLabel("起筋角度", this), m_lineMorphAngle);

        m_lineMorphHeight = new mCalcLineEdit(this, false, false, true);
        morphModeFold->addRow(new PanelLabel("起筋高度", this), m_lineMorphHeight);

        m_comboMorphDirection = new QComboBox(this);
        m_comboMorphDirection->addItems({ "单元法向" });
        morphModeFold->addRow(new PanelLabel("起筋方向", this), m_comboMorphDirection);

        connect(m_comboMorphMode, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_morphModeChanged);
        connect(m_lineMorphMinWidth, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_morphMinWidth = m_lineMorphMinWidth->text();
        });
        connect(m_lineMorphAngle, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_morphAngle = m_lineMorphAngle->text();
        });
        connect(m_lineMorphHeight, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationDesignData) _optimizationDesignData->_morphHeight = m_lineMorphHeight->text();
        });
        connect(m_comboMorphDirection, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (_optimizationDesignData) _optimizationDesignData->_morphDirection = text;
        });

        m_foldWidgets.append(morphModeFold);
        _mainLayout->addWidget(morphModeFold);
        m_morphModeFold->setParent(nullptr);

        // ==========================================
        // 形貌-起筋界限
        // ==========================================
        auto morphBoundFold = new FoldSection("起筋界限", this, QColor("#909399"), false);
        m_morphBoundFold = morphBoundFold;

        m_morphBoundCheck = new QCheckBox("起筋界限", this);
        morphBoundFold->addRow(m_morphBoundCheck);

        m_morphOriginWidget = createPointPicker(QStringLiteral("原点"), &mOptimizationDesignData::_morphOrigin, QStringLiteral("morphOrigin"), QVector3D(1, 0, 0));
        morphBoundFold->addRow(m_morphOriginWidget);

        m_morphDirPoint1Widget = createPointPicker(QStringLiteral("方向点1"), &mOptimizationDesignData::_morphDirPoint1, QStringLiteral("morphDir1"), QVector3D(0, 1, 0));
        morphBoundFold->addRow(m_morphDirPoint1Widget);

        m_morphDirPoint2Widget = createPointPicker(QStringLiteral("方向点2"), &mOptimizationDesignData::_morphDirPoint2, QStringLiteral("morphDir2"), QVector3D(0, 0, 1));
        morphBoundFold->addRow(m_morphDirPoint2Widget);

        connect(m_morphBoundCheck, &QCheckBox::stateChanged, this, &mOptimizationDesignPanel::slot_checkMorphBoundChanged);

        m_foldWidgets.append(morphBoundFold);
        _mainLayout->addWidget(morphBoundFold);
        m_morphBoundFold->setParent(nullptr);

        // ==========================================
        // 形状-变形定义
        // ==========================================
        auto shapeDeformFold = new FoldSection("变形定义", this, QColor("#909399"), false);
        m_shapeDeformFold = shapeDeformFold;

        m_comboShapeDeformMode = new QComboBox(this);
        m_comboShapeDeformMode->addItems({ "交互", "平移", "旋转", "缩放", "移动至节点" });
        shapeDeformFold->addRow(new PanelLabel("变形方式", this), m_comboShapeDeformMode);

        // 平移/缩放/旋转公共：x, y, z
        m_lblShapeTx = new PanelLabel("X", this); m_lineShapeTx = new mCalcLineEdit(this, false, false, true);
        m_lblShapeTy = new PanelLabel("Y", this); m_lineShapeTy = new mCalcLineEdit(this, false, false, true);
        m_lblShapeTz = new PanelLabel("Z", this); m_lineShapeTz = new mCalcLineEdit(this, false, false, true);
        m_lblShapeTx->setParent(nullptr); m_lineShapeTx->setParent(nullptr);
        m_lblShapeTy->setParent(nullptr); m_lineShapeTy->setParent(nullptr);
        m_lblShapeTz->setParent(nullptr); m_lineShapeTz->setParent(nullptr);

        // 坐标系（平移/缩放时显示）
        m_lblShapeCoordSystem = new PanelLabel("坐标系", this);
        m_comboShapeCoordSystem = new QComboBox(this);
        // TODO: 通过 getCVM->getModelData()->getCoordSystemManager() 或类似接口获取所有坐标系名称
        m_lblShapeCoordSystem->setParent(nullptr); m_comboShapeCoordSystem->setParent(nullptr);

        // 旋转专用
        m_lblShapeRotAngle = new PanelLabel("旋转角度", this);
        m_lineShapeRotAngle = new mCalcLineEdit(this, false, false, true);
        m_lblShapeRotAngle->setParent(nullptr); m_lineShapeRotAngle->setParent(nullptr);

        m_shapeRotPickWidget = createPointPicker(QStringLiteral("旋转中心点"), &mOptimizationDesignData::_shapeRotPick, QStringLiteral("shapeRotPick"), QVector3D(1.0, 0.5, 0.0));
        m_shapeRotPickWidget->setParent(nullptr);

        m_lblShapeRotAxis = new PanelLabel("旋转轴", this);
        m_comboShapeRotAxis = new QComboBox(this);
        m_comboShapeRotAxis->addItems({ "X轴", "Y轴", "Z轴", "自定义" });
        m_lblShapeRotAxis->setParent(nullptr); m_comboShapeRotAxis->setParent(nullptr);

        m_lblShapeCustomAxis = new PanelLabel("自定义中心轴", this); m_lineShapeCustomAxis = new mCalcLineEdit(this, false, false, true);
        m_lblShapeCustomAxis->setParent(nullptr); m_lineShapeCustomAxis->setParent(nullptr);

        // 移动至节点专用
        m_lblShapeTargetNode = new PanelLabel("节点", this); m_lineShapeTargetNode = new mCalcLineEdit(this, false, false, true);
        m_lblShapeTargetNode->setParent(nullptr); m_lineShapeTargetNode->setParent(nullptr);

        connect(m_comboShapeDeformMode, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_shapeDeformModeChanged);
        connect(m_lineShapeTx, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeTx = m_lineShapeTx->text(); });
        connect(m_lineShapeTy, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeTy = m_lineShapeTy->text(); });
        connect(m_lineShapeTz, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeTz = m_lineShapeTz->text(); });
        connect(m_comboShapeCoordSystem, &QComboBox::currentTextChanged, this, [this](const QString& text) {
            if (!_optimizationDesignData || !getCVM) return;
            auto& coordManager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            int coordId = coordManager.getIDByCoordinateName(text);
            int id = _optimizationDesignData->getId();
            mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_Coordinate, coordId });
            m_comboShapeCoordSystem->setCurrentIndex(coordId == -1 ? 0 : m_comboShapeCoordSystem->findText(text));
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
        });
        connect(m_comboShapeRotAxis, &QComboBox::currentTextChanged, this, &mOptimizationDesignPanel::slot_shapeRotAxisChanged);
        connect(m_lineShapeRotAngle, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeRotAngle = m_lineShapeRotAngle->text(); });
        connect(m_lineShapeCustomAxis, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeCustomAxis = m_lineShapeCustomAxis->text(); });
        connect(m_lineShapeTargetNode, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeTargetNode = m_lineShapeTargetNode->text(); });

        m_foldWidgets.append(shapeDeformFold);
        _mainLayout->addWidget(shapeDeformFold);
        m_shapeDeformFold->setParent(nullptr);

        // ==========================================
        // 形状-变形参数
        // ==========================================
        auto shapeParamFold = new FoldSection("变形参数", this, QColor("#909399"), false);
        m_shapeParamFold = shapeParamFold;

        m_lblShapeInitValue = new PanelLabel("初始值", this); m_lineShapeInitValue = new mCalcLineEdit(this, false, false, true);
        shapeParamFold->addRow(m_lblShapeInitValue, m_lineShapeInitValue);

        m_lblShapeMinValue = new PanelLabel("下限值", this); m_lineShapeMinValue = new mCalcLineEdit(this, false, false, true);
        shapeParamFold->addRow(m_lblShapeMinValue, m_lineShapeMinValue);

        m_lblShapeMaxValue = new PanelLabel("上限值", this); m_lineShapeMaxValue = new mCalcLineEdit(this, false, false, true);
        shapeParamFold->addRow(m_lblShapeMaxValue, m_lineShapeMaxValue);

        m_lblShapeMoveLimit = new PanelLabel("移动限制", this); m_lineShapeMoveLimit = new mCalcLineEdit(this, false, false, true);
        shapeParamFold->addRow(m_lblShapeMoveLimit, m_lineShapeMoveLimit);

        connect(m_lineShapeInitValue, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeInitValue = m_lineShapeInitValue->text(); });
        connect(m_lineShapeMinValue, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeMinValue = m_lineShapeMinValue->text(); });
        connect(m_lineShapeMaxValue, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeMaxValue = m_lineShapeMaxValue->text(); });
        connect(m_lineShapeMoveLimit, &QLineEdit::editingFinished, this, [this]() { if (_optimizationDesignData) _optimizationDesignData->_shapeMoveLimit = m_lineShapeMoveLimit->text(); });

        m_foldWidgets.append(shapeParamFold);
        _mainLayout->addWidget(shapeParamFold);
        m_shapeParamFold->setParent(nullptr);

        _mainLayout->addStretch();

        // 含拾取控件（计算点控件/通用拾取控件）的折叠框：收起后控件不可见，需失活避免占用全局拾取激活状态
        const QList<FoldWidget*> pickerFolds = { patternFold, draftFold, extrusionFold, supportFold, patternRepFold, m_morphBoundFold, m_shapeDeformFold };
        for (FoldWidget* fold : pickerFolds)
        {
            connect(fold, &FoldWidget::foldExpandChanged, this, [this](bool) {
                // 收起时内容隐藏发生在折叠动画结束后，延迟失活不可用控件
                QTimer::singleShot(100, this, [this]() { deactivateUnavailablePickers(); });
            });
        }
    }

    mOptimizationDesignPanel::~mOptimizationDesignPanel()
    {
        if (_mainLayout != nullptr) {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mOptimizationDesignPanel::createOptimizationDesign(QString type)
    {
        if (!getCVM) return;
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (structOption == nullptr)
        {
            structOption = getCVM->getModelData()->createStructOption();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationSig(true);
        }
        int ID = structOption->createOptimizationDesignID();
        int cnt = 1;
        QString text = "DesignDomain";
        QString name = text + "_" + MxFunctions::mNumberToString(cnt++);
        while (structOption->existOptimizationDesignName(name)) {
            name = text + "_" + MxFunctions::mNumberToString(cnt++);
        }
        _optimizationDesignData = new mOptimizationDesignData(ID, name);
        _optimizationDesignData->_designDomainType = type;
        structOption->addOptimizationDesignData(_optimizationDesignData);
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建设计域 %1").arg(_optimizationDesignData->getName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(true, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::editOptimizationDesignPanel(int id)
    {
        if (!getCVM) return;
        _optimizationDesignData = getCVM->getModelData()->getStructOptionBase()->getOptimizationDesignData(id);
        if (_optimizationDesignData == nullptr) return;
        setEditing(true);

        // 更新设计域属性组
        _comboDesignDomainType->blockSignals(true);
        _comboDesignDomainType->setCurrentText(_optimizationDesignData->_designDomainType);
        _comboDesignDomainType->blockSignals(false);

        updatePropertyComboboxByType(_comboProperty, _optimizationDesignData->_designDomainType);

        const QSignalBlocker blockerBaseThickness(m_lineBaseThickness);
        m_lineBaseThickness->setExpression(_optimizationDesignData->_baseThickness);

        // 回显冻结区域
        if (m_comboFreezeRegion) {
            updateFreezeRegionSetCombobox(m_comboFreezeRegion);
        }

        // 根据设计域类型和优化方式更新UI
        m_comboOptimizationMethod->blockSignals(true);
        slot_designDomainTypeChanged(_optimizationDesignData->_designDomainType);

        // slot_designDomainTypeChanged 中的 clear 会重置优化方式，这里恢复并手动触发更新
        if (_optimizationDesignData->_designDomainType != "实体") {
            m_comboOptimizationMethod->setCurrentText(_optimizationDesignData->_optimizationMethod);
            slot_optimizationMethodChanged(_optimizationDesignData->_optimizationMethod);
        }
        m_comboOptimizationMethod->blockSignals(false);

        // 回显设计域参数
        const QSignalBlocker blockerMinCheck(m_checkMinMember);
        const QSignalBlocker blockerMinLine(m_lineMinMember);
        const QSignalBlocker blockerMaxCheck(m_checkMaxMember);
        const QSignalBlocker blockerMaxLine(m_lineMaxMember);
        const QSignalBlocker blockerGapCheck(m_checkMinGap);
        const QSignalBlocker blockerGapLine(m_lineMinGap);
        const QSignalBlocker blockerStressCheck(m_checkStressConstraint);
        const QSignalBlocker blockerStressLine(m_lineStressConstraint);

        m_checkMinMember->setChecked(_optimizationDesignData->_minMemberStatus);
        m_lineMinMember->setExpression(_optimizationDesignData->_minMemberSize);
        m_lineMinMember->setEnabled(_optimizationDesignData->_minMemberStatus);

        m_checkMaxMember->setChecked(_optimizationDesignData->_maxMemberStatus);
        m_lineMaxMember->setExpression(_optimizationDesignData->_maxMemberSize);
        m_lineMaxMember->setEnabled(_optimizationDesignData->_maxMemberStatus);

        m_checkMinGap->setChecked(_optimizationDesignData->_minGapStatus);
        m_lineMinGap->setExpression(_optimizationDesignData->_minGapSize);
        m_lineMinGap->setEnabled(_optimizationDesignData->_minGapStatus);

        m_checkStressConstraint->setChecked(_optimizationDesignData->_stressConstraintStatus);
        m_lineStressConstraint->setExpression(_optimizationDesignData->_stressConstraintValue);
        m_lineStressConstraint->setEnabled(_optimizationDesignData->_stressConstraintStatus);

        // 回显加强筋参数
        const QSignalBlocker blockerRibH(m_checkRibHeight);
        const QSignalBlocker blockerRibHLine(m_lineRibHeight);
        const QSignalBlocker blockerRibW(m_checkRibWidth);
        const QSignalBlocker blockerRibWLine(m_lineRibWidth);

        m_checkRibHeight->setChecked(_optimizationDesignData->_ribHeightStatus);
        m_lineRibHeight->setExpression(_optimizationDesignData->_ribHeight);
        m_lineRibHeight->setEnabled(_optimizationDesignData->_ribHeightStatus);

        m_checkRibWidth->setChecked(_optimizationDesignData->_ribWidthStatus);
        m_lineRibWidth->setExpression(_optimizationDesignData->_ribWidth);
        m_lineRibWidth->setEnabled(_optimizationDesignData->_ribWidthStatus);

        // 对称约束组
        m_foldWidgets[2]->setChecked(_optimizationDesignData->_patternStatus);
        m_comboPattern->blockSignals(true);
        m_comboPattern->setCurrentText(_optimizationDesignData->_patternType);
        m_comboPattern->blockSignals(false);
        slot_comboPatternChanged(_optimizationDesignData->_patternType);
        const QSignalBlocker blockerPatternNum(m_linePatternNum);
        m_linePatternNum->setExpression(_optimizationDesignData->_patternNum);

        // 拔模约束组
        m_foldWidgets[3]->setChecked(_optimizationDesignData->_liftingStatus);
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

        // 挤出约束组：先恢复 combo 值，再恢复 checkbox，确保槽函数读取到正确值
        m_comboExtrudeType->blockSignals(true);
        m_comboExtrudeType->setCurrentText(_optimizationDesignData->_extrusionType);
        m_comboExtrudeType->blockSignals(false);

        m_foldWidgets[4]->setChecked(_optimizationDesignData->_extrusionStatus);
        slot_comboExtrudeChanged(_optimizationDesignData->_extrusionType);

        // 自支撑约束组
        const QSignalBlocker blockerLine(m_lineOverhangAngle);
        m_foldWidgets[5]->setChecked(_optimizationDesignData->_supportStatus);
        m_lineOverhangAngle->setExpression(_optimizationDesignData->_overhangAngle);
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
        }

        // 模式重复组
        updateDomainCombobox(m_comboDomain);

        const QSignalBlocker blockerCombo(m_comboDomainType);
        const QSignalBlocker blockerX(m_lineScaleX);
        const QSignalBlocker blockerY(m_lineScaleY);
        const QSignalBlocker blockerZ(m_lineScaleZ);

        bool status = _optimizationDesignData->_patternRepStatus;
        m_foldWidgets[6]->setChecked(status);
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

        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphOrigin");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir1");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir2");
        }

        // 形貌参数回显
        const QSignalBlocker blockerMorphUp(m_lineMorphUpLimit);
        const QSignalBlocker blockerMorphDown(m_lineMorphDownLimit);
        const QSignalBlocker blockerMorphInit(m_comboMorphInitRatio);
        const QSignalBlocker blockerMorphMode(m_comboMorphMode);
        const QSignalBlocker blockerMorphMinW(m_lineMorphMinWidth);
        const QSignalBlocker blockerMorphAng(m_lineMorphAngle);
        const QSignalBlocker blockerMorphH(m_lineMorphHeight);
        const QSignalBlocker blockerMorphDir(m_comboMorphDirection);
        const QSignalBlocker blockerMorphBound(m_morphBoundCheck);

        m_lineMorphUpLimit->setExpression(_optimizationDesignData->_morphUpLimit);
        m_lineMorphDownLimit->setExpression(_optimizationDesignData->_morphDownLimit);
        m_comboMorphInitRatio->setCurrentText(_optimizationDesignData->_morphInitRatio.isEmpty() ? "默认" : _optimizationDesignData->_morphInitRatio);
        m_comboMorphMode->setCurrentText(_optimizationDesignData->_morphMode.isEmpty() ? "无" : _optimizationDesignData->_morphMode);
        m_lineMorphMinWidth->setExpression(_optimizationDesignData->_morphMinWidth);
        m_lineMorphAngle->setExpression(_optimizationDesignData->_morphAngle);
        m_lineMorphHeight->setExpression(_optimizationDesignData->_morphHeight);
        m_comboMorphDirection->setCurrentText(_optimizationDesignData->_morphDirection.isEmpty() ? "单元法向" : _optimizationDesignData->_morphDirection);

        m_morphBoundCheck->setChecked(_optimizationDesignData->_morphBoundStatus);
        m_morphOriginWidget->setEnabled(_optimizationDesignData->_morphBoundStatus);
        m_morphDirPoint1Widget->setEnabled(_optimizationDesignData->_morphBoundStatus);
        m_morphDirPoint2Widget->setEnabled(_optimizationDesignData->_morphBoundStatus);

        // 删除形状渲染点
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("shapeRotPick");
        }

        // 形状参数回显
        const QSignalBlocker blockerShapeSplitD(m_checkShapeSplitDomain);
        const QSignalBlocker blockerShapeSplitP(m_checkShapeSplitPart);
        const QSignalBlocker blockerShapeKeep(m_checkShapeKeepHandle);
        const QSignalBlocker blockerShape2D(m_checkShape2DSplit);
        m_checkShapeSplitDomain->setChecked(_optimizationDesignData->_shapeSplitDomain);
        m_checkShapeSplitPart->setChecked(_optimizationDesignData->_shapeSplitPart);
        m_checkShapeKeepHandle->setChecked(_optimizationDesignData->_shapeKeepHandle);
        m_checkShape2DSplit->setChecked(_optimizationDesignData->_shape2DSplit);

        const QSignalBlocker blockerShapeDeformMode(m_comboShapeDeformMode);
        m_comboShapeDeformMode->setCurrentText(_optimizationDesignData->_shapeDeformMode.isEmpty() ? "交互" : _optimizationDesignData->_shapeDeformMode);

        const QSignalBlocker blockerShapeTx(m_lineShapeTx);
        const QSignalBlocker blockerShapeTy(m_lineShapeTy);
        const QSignalBlocker blockerShapeTz(m_lineShapeTz);
        m_lineShapeTx->setExpression(_optimizationDesignData->_shapeTx);
        m_lineShapeTy->setExpression(_optimizationDesignData->_shapeTy);
        m_lineShapeTz->setExpression(_optimizationDesignData->_shapeTz);

        updateShapeCoordCombobox(m_comboShapeCoordSystem);

        const QSignalBlocker blockerShapeRotAxis(m_comboShapeRotAxis);
        m_comboShapeRotAxis->setCurrentText(_optimizationDesignData->_shapeRotAxis.isEmpty() ? "X轴" : _optimizationDesignData->_shapeRotAxis);

        m_lineShapeRotAngle->setExpression(_optimizationDesignData->_shapeRotAngle);
        m_lineShapeCustomAxis->setExpression(_optimizationDesignData->_shapeCustomAxis);
        m_lineShapeTargetNode->setExpression(_optimizationDesignData->_shapeTargetNode);

        m_lineShapeInitValue->setExpression(_optimizationDesignData->_shapeInitValue);
        m_lineShapeMinValue->setExpression(_optimizationDesignData->_shapeMinValue);
        m_lineShapeMaxValue->setExpression(_optimizationDesignData->_shapeMaxValue);
        m_lineShapeMoveLimit->setExpression(_optimizationDesignData->_shapeMoveLimit);

        // 触发槽函数以正确显示动态控件
        slot_shapeDeformModeChanged(m_comboShapeDeformMode->currentText());
        slot_shapeRotAxisChanged(m_comboShapeRotAxis->currentText());

        // 回显计算点控件坐标
        m_originWidget->setCoordinate(_optimizationDesignData->_origin);
        m_dirPoint1Widget->setCoordinate(_optimizationDesignData->_dirPoint1);
        m_dirPoint2Widget->setCoordinate(_optimizationDesignData->_dirPoint2);
        m_startPointWidget->setCoordinate(_optimizationDesignData->_startPoint);
        m_draftDirWidget->setCoordinate(_optimizationDesignData->_dirPoint);
        m_supportStartWidget->setCoordinate(_optimizationDesignData->_supportStartPoint);
        m_supportDirWidget->setCoordinate(_optimizationDesignData->_supportDirPoint);
        m_repBasePointWidget->setCoordinate(_optimizationDesignData->_repBasePoint);
        m_repDirPoint1Widget->setCoordinate(_optimizationDesignData->_repDirPoint1);
        m_repDirPoint2Widget->setCoordinate(_optimizationDesignData->_repDirPoint2);
        m_repDirPoint3Widget->setCoordinate(_optimizationDesignData->_repDirPoint3);
        m_morphOriginWidget->setCoordinate(_optimizationDesignData->_morphOrigin);
        m_morphDirPoint1Widget->setCoordinate(_optimizationDesignData->_morphDirPoint1);
        m_morphDirPoint2Widget->setCoordinate(_optimizationDesignData->_morphDirPoint2);
        m_shapeRotPickWidget->setCoordinate(_optimizationDesignData->_shapeRotPick);
    }

    void mOptimizationDesignPanel::uneditOptimizationDesignPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        // 退出编辑时失活所有拾取控件（计算点控件/通用拾取控件）
        for (const auto& entry : _pickerEntries)
        {
            if (!entry.controller) continue;
            entry.controller->setPickActive(false);
            if (entry.pointPicker)
                entry.pointPicker->clearAllHighlight();
            else
                entry.controller->clearAllHighlight();
        }

        _optimizationDesignData = nullptr;
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("origin");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("direction3");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftStart");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftDir");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_base");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("patternRep_dir3");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphOrigin");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir1");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir2");
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("shapeRotPick");
    }

    void mOptimizationDesignPanel::updatePointRender(QString str, QVector3D point, QVector3D color)
    {
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint(str, { point }, color);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    mPickWidgetController* mOptimizationDesignPanel::createPathPickWidget(const QString& tip,
        QVector<int> MechStructOptionData::mOptimizationDesignData::* member)
    {
        // 通用拾取控件：仅节点、多选、无集合选择（多选模式下控件内部按拾取先后顺序追加去重）
        auto* controller = mGeneralPickUp::createSetPickWidget(CBNone, this);
        controller->setDefultTip(tip);
        controller->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType> {
            return { MxFunctions::PickDataType::Node };
        });

        // 路径存储有顺序（节点先后次序即路径走向），数据变化时按控件内部顺序
        // （拾取先后顺序）原样同步到绑定的 QVector<int> 成员，不得重排或去重破坏顺序
        connect(controller, &mPickWidgetController::dataChanged, this, [this, controller, member]() {
            if (!_optimizationDesignData) return;
            QVector<int> newPath;
            const auto& pickedData = controller->getAllData().pickedData;
            auto it = pickedData.find(MxFunctions::PickDataType::Node);
            if (it != pickedData.end())
            {
                newPath.reserve(static_cast<int>(it->second.size()));
                for (const auto& d : it->second)
                    newPath.append(d.result_id);
            }
            if (newPath != (_optimizationDesignData->*member))
                _optimizationDesignData->*member = newPath;
        });

        // 按面板自上而下顺序追加，供拾取控件激活/失活调度使用
        PickerEntry entry;
        entry.view = controller->widget();
        entry.controller = controller;
        _pickerEntries.append(entry);
        return controller;
    }

    void mOptimizationDesignPanel::displayPathData(mPickWidgetController* controller, const QVector<int>& path)
    {
        if (!controller) return;
        PickWidgetData data;
        if (!path.isEmpty())
        {
            std::vector<RenderPickResultData> nodes;
            nodes.reserve(path.size());
            for (int id : path)
                nodes.emplace_back(id);
            data.pickedData[MxFunctions::PickDataType::Node] = std::move(nodes);
        }
        controller->displayData(data);
    }

    mPointPickerWidget* mOptimizationDesignPanel::createPointPicker(const QString& label,
        QVector3D MechStructOptionData::mOptimizationDesignData::* member,
        const QString& renderKey,
        const QVector3D& color)
    {
        auto* picker = new mPointPickerWidget(label, this);
        picker->setEnabled(false);
        connect(picker, &mPointPickerWidget::coordinateChanged, this, [this, picker, member, renderKey, color]() {
            if (!_optimizationDesignData) return;
            QVector3D coord = picker->coordinate();
            // 拾取累积过程中也会发 coordinateChanged，此时输入框值未变；跳过避免清空正在累积的拾取选择
            if (coord == _optimizationDesignData->*member) return;
            _optimizationDesignData->*member = coord;
            this->updatePointRender(renderKey, coord, color);
        });
        // 按面板自上而下顺序追加，供拾取控件激活/失活调度使用
        PickerEntry entry;
        entry.view = picker;
        entry.controller = picker->pickController();
        entry.pointPicker = picker;
        _pickerEntries.append(entry);
        return picker;
    }

    void mOptimizationDesignPanel::deactivateUnavailablePickers()
    {
        // 仅失活所有不可用（隐藏/禁用）的拾取控件（计算点控件/通用拾取控件），
        // 避免不可用控件占用全局拾取激活状态；默认激活见 activateFirstCheckedFoldPicker
        for (const auto& entry : _pickerEntries)
        {
            if (!entry.view || !entry.controller) continue;
            if (!entry.view->isVisible() || !entry.view->isEnabled())
            {
                entry.controller->setPickActive(false);
                if (entry.pointPicker)
                    entry.pointPicker->clearAllHighlight();
                else
                    entry.controller->clearAllHighlight();
            }
        }
    }

    void mOptimizationDesignPanel::activateFirstPickerInFold(FoldWidget* fold)
    {
        if (!fold) return;
        // 激活当前折叠框内第一个可见可用的拾取控件（计算点控件或通用拾取控件）；
        // 收起/移出本折叠框或禁用的控件不参与激活；控件内部全局互斥会自动失活其他控件
        for (const auto& entry : _pickerEntries)
        {
            if (!entry.view || !entry.controller) continue;
            if (!fold->isAncestorOf(entry.view)) continue;
            if (!entry.view->isVisible() || !entry.view->isEnabled()) continue;
            entry.controller->setPickActive(true);
            break;
        }
    }

    void mOptimizationDesignPanel::activateFirstCheckedFoldPicker()
    {
        // _pickerEntries 按面板自上而下顺序记录，首个满足条件的控件即位于最上方的被勾选折叠框内；
        // 再次校验可见/可用以兼容收起、移出布局或禁用状态；不可勾选的折叠框（如起筋界限、变形定义）不参与默认激活
        for (const auto& entry : _pickerEntries)
        {
            if (!entry.view || !entry.controller) continue;
            if (!entry.view->isVisible() || !entry.view->isEnabled()) continue;
            FoldWidget* fold = nullptr;
            for (FoldSection* section : m_foldWidgets)
            {
                if (section && section->isAncestorOf(entry.view)) { fold = section; break; }
            }
            if (!fold || !fold->isCheckable() || !fold->isChecked()) continue;
            entry.controller->setPickActive(true);
            return;
        }
    }

    void mOptimizationDesignPanel::updateFreezeRegionSetCombobox(QComboBox* combobox)
    {
        if (!combobox || !getCVM || !_optimizationDesignData) return;
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = _optimizationDesignData->getId();
        int setId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, id }, Physical_Set);
        QString currentText = setManager.getSetNameByID(setId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        combobox->addItems(setManager.getAllSetNamesByTypes(Element));
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(-1);
        }
        combobox->blockSignals(false);
    }

    void mOptimizationDesignPanel::updatePropertyCombobox(QComboBox* combobox)
    {
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_Property);
        QString currentText = propertyManager.getPropertyNameByID(propertyId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        combobox->addItems(propertyManager.getAllPropertyNames());
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(-1);
        }
        combobox->blockSignals(false);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::updateDomainCombobox(QComboBox* combobox)
    {
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int designId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_OptimationDesign);
        QString currentText = getCVM->getModelData()->getStructOptionBase()->getOptimizationDesignNameByID(designId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        QStringList names = getCVM->getModelData()->getStructOptionBase()->getAllOptimizationMasterDesignNames();
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

    void mOptimizationDesignPanel::slot_checkPatternChanged(bool state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_patternStatus = state;
        if (!_optimizationDesignData->_patternStatus) {
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
        // 勾选时激活本组第一个拾取控件；取消勾选仅失活不可用控件，不做默认激活
        if (_optimizationDesignData->_patternStatus)
            activateFirstPickerInFold(m_foldWidgets[2]);
        else
            deactivateUnavailablePickers();
    }

    void mOptimizationDesignPanel::slot_comboPatternChanged(const QString& text)
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
        if (type == "周向" || type == "周向+平面") fold->addRow(m_lblPatternNum, m_linePatternNum);
        fold->addRow(m_originWidget);
        fold->addRow(m_dirPoint1Widget);
        //if (type != "周向") { fold->addRow(m_dirPoint1Widget); }
        if (type != "单平面" && type != "周向") { fold->addRow(m_dirPoint2Widget); }
        deactivateUnavailablePickers();
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_checkDraftChanged(bool state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_liftingStatus = state;
        if (!_optimizationDesignData->_liftingStatus) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("draftDir");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_comboDraftType->setEnabled(_optimizationDesignData->_liftingStatus);
        m_comboNoHole->setEnabled(_optimizationDesignData->_liftingStatus);
        m_startPointWidget->setEnabled(_optimizationDesignData->_liftingStatus);
        m_draftDirWidget->setEnabled(_optimizationDesignData->_liftingStatus);
        // 勾选时激活本组第一个拾取控件；取消勾选仅失活不可用控件，不做默认激活
        if (_optimizationDesignData->_liftingStatus)
            activateFirstPickerInFold(m_foldWidgets[3]);
        else
            deactivateUnavailablePickers();
    }

    void mOptimizationDesignPanel::slot_checkExtrudeChanged(bool state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_extrusionStatus = state;
        if (_optimizationDesignData->_extrusionStatus) {
            // 勾选时回显已存路径（按存储顺序）
            displayPathData(m_path1Controller, _optimizationDesignData->_path1);
            QString extrudeType = m_comboExtrudeType->currentText();
            if (extrudeType == "扭转") {
                displayPathData(m_path2Controller, _optimizationDesignData->_path2);
            } else {
                _optimizationDesignData->_path2.clear();
            }
        } else {
            // 取消勾选：失活并清除路径拾取高亮（失活触发提交，数据已实时同步到存储）
            if (m_path1Controller) { m_path1Controller->setPickActive(false); m_path1Controller->clearAllHighlight(); }
            if (m_path2Controller) { m_path2Controller->setPickActive(false); m_path2Controller->clearAllHighlight(); }
        }
        m_comboExtrudeType->setEnabled(_optimizationDesignData->_extrusionStatus);
        if (m_path1Controller) m_path1Controller->widget()->setEnabled(_optimizationDesignData->_extrusionStatus);
        if (m_path2Controller) m_path2Controller->widget()->setEnabled(_optimizationDesignData->_extrusionStatus);
        // 勾选时激活本组第一个拾取控件（路径1）；取消勾选时上方已失活本组路径拾取
        if (_optimizationDesignData->_extrusionStatus)
            activateFirstPickerInFold(m_foldWidgets[4]);
    }

    void mOptimizationDesignPanel::slot_comboExtrudeChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        // 路径2随挤出方式显隐：先失活并移出，扭转时再添加回来
        if (m_path2Controller) {
            m_path2Controller->setPickActive(false);
            m_path2Controller->widget()->setParent(nullptr);
            m_path2Controller->clearAllHighlight();
        }
        auto fold = m_foldWidgets[4];
        _optimizationDesignData->_extrusionType = text;
        displayPathData(m_path1Controller, _optimizationDesignData->_path1);
        QString extrudeType = m_comboExtrudeType->currentText();
        if (extrudeType == "扭转") {
            if (fold && m_path2Controller) {
                fold->addRow(m_path2Controller->widget());
                m_path2Controller->widget()->setEnabled(_optimizationDesignData->_extrusionStatus);
                displayPathData(m_path2Controller, _optimizationDesignData->_path2);
            }
        } else {
            _optimizationDesignData->_path2.clear();
            if (m_path2Controller) m_path2Controller->replacePickedDataAndClearSets({});
        }
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_checkSupportChanged(bool state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_supportStatus = state;
        if (!_optimizationDesignData->_supportStatus) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportStart");
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("supportDir");
            getCVM->getPreRenderWidget()->updatePaint();
        }
        m_lineOverhangAngle->setEnabled(_optimizationDesignData->_supportStatus);
        m_supportStartWidget->setEnabled(_optimizationDesignData->_supportStatus);
        m_supportDirWidget->setEnabled(_optimizationDesignData->_supportStatus);
        // 勾选时激活本组第一个拾取控件；取消勾选仅失活不可用控件，不做默认激活
        if (_optimizationDesignData->_supportStatus)
            activateFirstPickerInFold(m_foldWidgets[5]);
        else
            deactivateUnavailablePickers();

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationDesign, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::slot_checkPatternRepChanged(bool state)
    {
        if (_optimizationDesignData) _optimizationDesignData->_patternRepStatus = state;
        if (!_optimizationDesignData->_patternRepStatus) {
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
        // 勾选时激活本组第一个拾取控件；取消勾选仅失活不可用控件，不做默认激活
        if (_optimizationDesignData->_patternRepStatus)
            activateFirstPickerInFold(m_foldWidgets[6]);
        else
            deactivateUnavailablePickers();
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::slot_comboPatternRepTypeChanged(const QString& text)
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
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::slot_comboPatternRepDomainChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int designId = getCVM->getModelData()->getStructOptionBase()->getOptimizationDesignIDByName(text);
        int id = _optimizationDesignData->getId();
        mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_OptimationDesign, designId });
        m_comboDomain->setCurrentIndex(designId == -1 ? 0 : m_comboDomain->findText(text));
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::slot_comboPatternRepCoordChanged(const QString& text)
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
        if (text == "指定点") {
            fold->addRow(m_repDirPoint1Widget); fold->addRow(m_repDirPoint2Widget); fold->addRow(m_repDirPoint3Widget);
        }
        deactivateUnavailablePickers();
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_propertyChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = propertyManager.getIDByPropertyName(text);
        int id = _optimizationDesignData->getId();
        mapRelation.appendMapRelation({ Physical_OptimationDesign, id }, { Physical_Property, propertyId });
        _comboProperty->setCurrentIndex(propertyId == -1 ? 0 : _comboProperty->findText(text));
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::slot_designDomainTypeChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_designDomainType = text;
        updatePropertyComboboxByType(_comboProperty, text);

        auto designFold = m_foldWidgets[0];
        auto paramFold = m_foldWidgets[1];

        // 先隐藏所有形状专用控件
        m_checkShapeSplitDomain->setParent(nullptr);
        m_checkShapeSplitPart->setParent(nullptr);
        m_checkShapeKeepHandle->setParent(nullptr);
        m_checkShape2DSplit->setParent(nullptr);

        // 先隐藏冻结区域，再根据类型决定是否显示
        m_lblFreezeRegion->setParent(nullptr);
        m_comboFreezeRegion->setParent(nullptr);

        if (text == "实体") {
            // 实体模式：隐藏优化方式和基准厚度
            m_lblOptimizationMethod->setParent(nullptr);
            m_comboOptimizationMethod->setParent(nullptr);
            m_lblBaseThickness->setParent(nullptr);
            m_lineBaseThickness->setParent(nullptr);
            // 显示冻结区域
            updateFreezeRegionSetCombobox(m_comboFreezeRegion);
            designFold->addRow(m_lblFreezeRegion, m_comboFreezeRegion);
        } else if (text == "壳") {
            // 恢复优化方式选项为壳专用
            m_comboOptimizationMethod->clear();
            m_comboOptimizationMethod->addItems({ "厚度", "加筋"/*, "形貌"*/ });
            designFold->addRow(m_lblOptimizationMethod, m_comboOptimizationMethod);
            // 显示冻结区域
            updateFreezeRegionSetCombobox(m_comboFreezeRegion);
            designFold->addRow(m_lblFreezeRegion, m_comboFreezeRegion);
        } else if (text == "形状") {
            // 形状模式：隐藏基准厚度，优化方式变为变形域选项
            m_lblBaseThickness->setParent(nullptr);
            m_lineBaseThickness->setParent(nullptr);
            m_comboOptimizationMethod->clear();
            m_comboOptimizationMethod->addItems({ "1D变形域", "2D变形域", "3D变形域" });
            designFold->addRow(m_lblOptimizationMethod, m_comboOptimizationMethod);
            // 添加四行复选框到设计域属性末尾
            designFold->addRow(m_checkShapeSplitDomain);
            designFold->addRow(m_checkShapeSplitPart);
            designFold->addRow(m_checkShapeKeepHandle);
            designFold->addRow(m_checkShape2DSplit);
        }

        updateDesignParamUI();

        if (text == "壳") {
            slot_optimizationMethodChanged(_optimizationDesignData->_optimizationMethod);
        }

        QTimer::singleShot(0, designFold, [=]() { designFold->refreshLayout(); });
        QTimer::singleShot(0, paramFold, [=]() { paramFold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_optimizationMethodChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_optimizationMethod = text;

        if (_optimizationDesignData->_designDomainType == "实体" || _optimizationDesignData->_designDomainType == "形状") {
            updateDesignParamUI();
            return;
        }

        auto designFold = m_foldWidgets[0];
        auto paramFold = m_foldWidgets[1];
        if (text == "厚度") {
            designFold->addRow(m_lblBaseThickness, m_lineBaseThickness);
        } else if (text == "加筋" || text == "形貌") {
            m_lblBaseThickness->setParent(nullptr);
            m_lineBaseThickness->setParent(nullptr);
        }

        updateDesignParamUI();

        QTimer::singleShot(0, paramFold, [=]() { paramFold->refreshLayout(); });
        QTimer::singleShot(0, designFold, [=]() { designFold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_morphModeChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_morphMode = text;
    }

    void mOptimizationDesignPanel::slot_checkMorphBoundChanged(int state)
    {
        bool checked = (state == Qt::Checked);
        if (_optimizationDesignData) _optimizationDesignData->_morphBoundStatus = checked;
        if (!checked) {
            if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphOrigin");
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir1");
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir2");
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }
        m_morphOriginWidget->setEnabled(checked);
        m_morphDirPoint1Widget->setEnabled(checked);
        m_morphDirPoint2Widget->setEnabled(checked);
        // 取消勾选后控件禁用，仅失活不可用控件，不做默认激活
        deactivateUnavailablePickers();
    }

    void mOptimizationDesignPanel::slot_shapeDeformModeChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_shapeDeformMode = text;

        auto fold = m_shapeDeformFold;
        // 先隐藏所有动态行
        m_lblShapeTx->setParent(nullptr); m_lineShapeTx->setParent(nullptr);
        m_lblShapeTy->setParent(nullptr); m_lineShapeTy->setParent(nullptr);
        m_lblShapeTz->setParent(nullptr); m_lineShapeTz->setParent(nullptr);
        m_lblShapeCoordSystem->setParent(nullptr); m_comboShapeCoordSystem->setParent(nullptr);
        m_lblShapeRotAngle->setParent(nullptr); m_lineShapeRotAngle->setParent(nullptr);
        m_shapeRotPickWidget->setParent(nullptr);
        m_lblShapeRotAxis->setParent(nullptr); m_comboShapeRotAxis->setParent(nullptr);
        m_lblShapeCustomAxis->setParent(nullptr); m_lineShapeCustomAxis->setParent(nullptr);
        m_lblShapeTargetNode->setParent(nullptr); m_lineShapeTargetNode->setParent(nullptr);
        if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
            getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("shapeRotPick");
        }
        if (text == "交互") {
            // 无其他参数
        } else if (text == "平移" || text == "缩放") {
            fold->addRow(m_lblShapeTx, m_lineShapeTx);
            fold->addRow(m_lblShapeTy, m_lineShapeTy);
            fold->addRow(m_lblShapeTz, m_lineShapeTz);
            fold->addRow(m_lblShapeCoordSystem, m_comboShapeCoordSystem);
        } else if (text == "旋转") {
            fold->addRow(m_lblShapeRotAngle, m_lineShapeRotAngle);
            fold->addRow(m_shapeRotPickWidget);
            fold->addRow(m_lblShapeRotAxis, m_comboShapeRotAxis);
            QString axis = m_comboShapeRotAxis->currentText();
            if (axis == "自定义") {
                fold->addRow(m_lblShapeCustomAxis, m_lineShapeCustomAxis);
            }
        } else if (text == "移动至节点") {
            fold->addRow(m_lblShapeTargetNode, m_lineShapeTargetNode);
        }
        deactivateUnavailablePickers();
        QTimer::singleShot(0, fold, [=]() { fold->refreshLayout(); });
    }

    void mOptimizationDesignPanel::slot_shapeRotAxisChanged(const QString& text)
    {
        if (!_optimizationDesignData) return;
        _optimizationDesignData->_shapeRotAxis = text;
        if (m_shapeDeformFold) {
            if (text == "自定义") {
                m_shapeDeformFold->addRow(m_lblShapeCustomAxis, m_lineShapeCustomAxis);
            } else {
                m_lblShapeCustomAxis->setParent(nullptr);
                m_lineShapeCustomAxis->setParent(nullptr);
            }
            QTimer::singleShot(0, m_shapeDeformFold, [=]() { m_shapeDeformFold->refreshLayout(); });
        }
    }

    void mOptimizationDesignPanel::updatePropertyComboboxByType(QComboBox* combobox, const QString& type)
    {
        if (!_optimizationDesignData || !getCVM) return;
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_Property);
        QString currentText = propertyManager.getPropertyNameByID(propertyId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("");
        QStringList names;
        if (type == "实体") {
            names = propertyManager.getAllPropertyNamesByTypes(SolidProperty);
        } else if (type == "壳") {
            names = propertyManager.getAllPropertyNamesByTypes(ShellProperty);
        } else {
            names = propertyManager.getAllPropertyNames();
        }
        combobox->addItems(names);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0);
        }
        combobox->blockSignals(false);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::updateShapeCoordCombobox(QComboBox* combobox)
    {
        if (!_optimizationDesignData || !getCVM) return;
        auto& coordManager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int coordId = mapRelation.getMapRelation<int>({ Physical_OptimationDesign, _optimizationDesignData->getId() }, Physical_Coordinate);
        QString currentText = coordManager.getCoordinateNameByID(coordId);
        combobox->blockSignals(true);
        combobox->clear();
        QStringList names = coordManager.getAllCoordinateNames();
        combobox->addItems(names);
        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        } else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0);
        }
        combobox->blockSignals(false);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationDesignSig(false, _optimizationDesignData->getId());
    }

    void mOptimizationDesignPanel::updateDesignParamUI()
    {
        if (!_optimizationDesignData) return;

        auto designFold = m_foldWidgets[0];
        auto paramFold = m_foldWidgets[1];

        QString domainType = _optimizationDesignData->_designDomainType;
        QString method = _optimizationDesignData->_optimizationMethod;

        m_checkMinMember->setParent(nullptr); m_lineMinMember->setParent(nullptr);
        m_checkMaxMember->setParent(nullptr); m_lineMaxMember->setParent(nullptr);
        m_checkMinGap->setParent(nullptr); m_lineMinGap->setParent(nullptr);
        m_checkStressConstraint->setParent(nullptr); m_lineStressConstraint->setParent(nullptr);
        m_checkRibHeight->setParent(nullptr); m_lineRibHeight->setParent(nullptr);
        m_checkRibWidth->setParent(nullptr); m_lineRibWidth->setParent(nullptr);

        // 隐藏形貌相关折叠框
        if (m_morphParamFold) m_morphParamFold->setParent(nullptr);
        if (m_morphModeFold) m_morphModeFold->setParent(nullptr);
        if (m_morphBoundFold) m_morphBoundFold->setParent(nullptr);

        // 隐藏形状相关折叠框
        if (m_shapeDeformFold) m_shapeDeformFold->setParent(nullptr);
        if (m_shapeParamFold) m_shapeParamFold->setParent(nullptr);

        // 隐藏约束组折叠框（对称、拔模、挤出、自支撑、模式重复）
        if (m_foldWidgets.size() > 2) m_foldWidgets[2]->setParent(nullptr);
        if (m_foldWidgets.size() > 3) m_foldWidgets[3]->setParent(nullptr);
        if (m_foldWidgets.size() > 4) m_foldWidgets[4]->setParent(nullptr);
        if (m_foldWidgets.size() > 5) m_foldWidgets[5]->setParent(nullptr);
        if (m_foldWidgets.size() > 6) m_foldWidgets[6]->setParent(nullptr);

        // 确保非形貌/非形状时 paramFold 重新显示在设计域属性后面
        bool isMorph = (domainType == "壳" && method == "形貌");
        bool isShape = (domainType == "形状");
        if (!isMorph && !isShape && _mainLayout->indexOf(paramFold) < 0) {
            int insertIdx = _mainLayout->indexOf(designFold) + 1;
            _mainLayout->insertWidget(insertIdx, paramFold);
        }

        // 从形貌切换走时，删除起筋界限渲染点
        if (!(domainType == "壳" && method == "形貌")) {
            if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphOrigin");
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir1");
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("morphDir2");
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }

        // 从形状切换走时，删除旋转拾取渲染点
        if (domainType != "形状") {
            if (getCVM && getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getCommonShapeRender()) {
                getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("shapeRotPick");
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }

        if (domainType == "实体") {
            paramFold->setTitle("设计域参数");
            paramFold->addRow(m_checkMinMember, m_lineMinMember);
            paramFold->addRow(m_checkMaxMember, m_lineMaxMember);
            paramFold->addRow(m_checkMinGap, m_lineMinGap);
            paramFold->addRow(m_checkStressConstraint, m_lineStressConstraint);

            // 实体：显示所有约束组
            int insertIdx = _mainLayout->indexOf(paramFold) + 1;
            _mainLayout->insertWidget(insertIdx++, m_foldWidgets[2]);
            _mainLayout->insertWidget(insertIdx++, m_foldWidgets[3]);
            _mainLayout->insertWidget(insertIdx++, m_foldWidgets[4]);
            _mainLayout->insertWidget(insertIdx++, m_foldWidgets[5]);
            _mainLayout->insertWidget(insertIdx++, m_foldWidgets[6]);
        } else if (domainType == "壳") {
            if (method == "厚度") {
                paramFold->setTitle("设计域参数");
                paramFold->addRow(m_checkMinMember, m_lineMinMember);
                paramFold->addRow(m_checkMaxMember, m_lineMaxMember);
                paramFold->addRow(m_checkMinGap, m_lineMinGap);
                paramFold->addRow(m_checkStressConstraint, m_lineStressConstraint);

                // 壳-厚度：不显示自支撑
                int insertIdx = _mainLayout->indexOf(paramFold) + 1;
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[2]);
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[3]);
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[4]);
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[6]);
            } else if (method == "加筋") {
                paramFold->setTitle("加强筋参数");
                paramFold->addRow(m_checkRibHeight, m_lineRibHeight);
                paramFold->addRow(m_checkRibWidth, m_lineRibWidth);

                // 壳-加筋：不显示拔模、自支撑
                int insertIdx = _mainLayout->indexOf(paramFold) + 1;
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[2]);
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[4]);
                _mainLayout->insertWidget(insertIdx++, m_foldWidgets[6]);
            } else if (method == "形貌") {
                // 隐藏原来的设计域参数/加强筋参数折叠框
                paramFold->setParent(nullptr);
                // 在设计域属性折叠框后面插入三个形貌折叠框
                int insertIdx = _mainLayout->indexOf(designFold) + 1;
                if (m_morphParamFold) _mainLayout->insertWidget(insertIdx, m_morphParamFold);
                if (m_morphModeFold) _mainLayout->insertWidget(insertIdx + 1, m_morphModeFold);
                if (m_morphBoundFold) _mainLayout->insertWidget(insertIdx + 2, m_morphBoundFold);
            }
        } else if (domainType == "形状") {
            // 形状模式：隐藏 paramFold，插入变形定义和变形参数折叠框
            paramFold->setParent(nullptr);
            int insertIdx = _mainLayout->indexOf(designFold) + 1;
            if (m_shapeDeformFold) _mainLayout->insertWidget(insertIdx, m_shapeDeformFold);
            if (m_shapeParamFold) _mainLayout->insertWidget(insertIdx + 1, m_shapeParamFold);
        }

        // 设计域类型/优化方式切换会显隐含计算点控件的折叠框，先失活被隐藏/禁用的拾取控件；
        // 本函数同时覆盖进入面板（editOptimizationDesignPanel 回显）与模式切换两条路径，
        // 布局重建、折叠展开及面板显示均在事件循环中完成，延迟到其后再默认激活第一个被勾选折叠框中的拾取控件
        deactivateUnavailablePickers();
        QTimer::singleShot(100, this, [this]() { activateFirstCheckedFoldPicker(); });
    }
}
