#include "mMotionVectorForcePanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mMotionExprBuilderWidget.h"
#include "mMotionScalarTorque.h"
#include "mMotionVectorSymbolHelper.h"
#include "mAppConfiguration.h"
#include <QStandardItemModel>
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mGetSetTypeTemplate.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mModelData.h"
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionVectorForce.h"
#include "mMotionBushing.h"
#include "mMotionScalarForce.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include "mMotionForceElementManager.h"

//Geo
#include "mGeometricFeatureAttribute.h"

//Qt
#include <QComboBox>
#include <QTimer>
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QStackedWidget>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    namespace
    {
        // 方向控件值反显：统一走数据类里的拾取会话状态（照集中力 dirPickState_ 模式），
        // 见 mMotionPickerActivate.h 的 restoreDirPickState。
        void restoreDirectionSelector(mDirectionSelectorWidget* selector, const DirectionPickState& state, const QVector3D& dir)
        {
            restoreDirPickState(selector, state, dir);
        }
    }

    // =====================================================================
    // 构造 / 析构
    // =====================================================================

    mMotionVectorForcePanel::mMotionVectorForcePanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_forceElementData = nullptr;
        m_mainLayout = new QVBoxLayout(this);

        buildUI();
        connectSignals();

        m_mainLayout->addStretch();
    }

    mMotionVectorForcePanel::~mMotionVectorForcePanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }
    // 界面构建
    // =====================================================================

    MxFunctions::mCalcLineEdit* mMotionVectorForcePanel::makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode mode)
    {
        return new MxFunctions::mCalcLineEdit(mode, this);
    }

    void mMotionVectorForcePanel::buildUI()
    {
        buildBasicSection();
        buildTargetSection();
        buildDirectionSection();
        buildBaseBodySection();
        buildMagnitudeSection();
        buildStiffnessSection();
        buildDampingSection();
        buildPreloadSection();
        buildScalarForceSection();
        buildScalarTorqueSection();
        buildThreePointForceSection();
    }

    void mMotionVectorForcePanel::buildBasicSection()
    {
        FoldSection* foldBasic = new FoldSection("基础信息", this);

        m_cmbLoadType = new QComboBox();
        foldBasic->addRow(new QLabel("载荷类型:"), m_cmbLoadType);

        //力元类型下拉：仅列求解装配支持的力元类型。衬套属柔性连接族，不在力元类型之列
        //（编辑衬套对象时由 updateUIFromData 在末尾临时追加只读项）。索引与枚举值不对齐，
        //经 comboTypeTable 映射
        m_cmbLoadType->addItems({
            "矢量力", "矢量力矩",
            "三点力", "三点力矩",
            "标量力", "标量力矩",
            "衬套"
            });

        m_mainLayout->addWidget(foldBasic);
    }

    void mMotionVectorForcePanel::buildTargetSection()
    {
        m_foldTarget = new FoldSection("作用对象", this);

        // 作用体（marker_I 所属体）
        m_cmbBody = new QComboBox();
        m_foldTarget->addRow(new QLabel("作用体"), m_cmbBody);

        // 作用点（marker_I 原点）通用拾取控件
        m_actionPointPicker = new mPointPickerWidget(QStringLiteral("作用点"), this);
        m_actionPointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldTarget->addRow(m_actionPointPicker);

        m_mainLayout->addWidget(m_foldTarget);
    }

    void mMotionVectorForcePanel::buildDirectionSection()
    {
        m_foldDirection = new FoldSection("方向定义", this);

        // 方向向量（marker_I 定向矢量）方向选择控件
        m_directionSelector = new mDirectionSelectorWidget(
            QStringLiteral("方向定义"), false, QString(), false, QString(), false, this);
        connect(m_directionSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (m_foldDirection)
                m_foldDirection->refreshLayout();
            });
        m_foldDirection->addRow(m_directionSelector);

        m_cmbDirectionMode = new QComboBox();
        m_cmbDirectionMode->addItems({ "全局固定", "体固定" });
        m_lblDirectionMode = new QLabel("方向模式:");
        m_foldDirection->addRow(m_lblDirectionMode, m_cmbDirectionMode);

        m_mainLayout->addWidget(m_foldDirection);
    }

    void mMotionVectorForcePanel::buildBaseBodySection()
    {
        m_foldBaseBody = new FoldSection("基本体", this);

        m_cmbBaseBody = new QComboBox();
        m_foldBaseBody->addRow(new QLabel("基本体"), m_cmbBaseBody);

        m_cmbBaseOffset = new QComboBox();
        m_cmbBaseOffset->addItems({ "否", "是" });
        m_foldBaseBody->addRow(new QLabel("应用偏置:"), m_cmbBaseOffset);

        auto makeOffsetSpin = [this]() {
            MxFunctions::mCalcLineEdit* spb = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
            spb->setEnabled(false);
            return spb;
        };
        m_spbOffsetX = makeOffsetSpin();
        m_spbOffsetY = makeOffsetSpin();
        m_spbOffsetZ = makeOffsetSpin();
        m_foldBaseBody->addRow(new QLabel("偏置 X (mm):"), m_spbOffsetX);
        m_foldBaseBody->addRow(new QLabel("偏置 Y (mm):"), m_spbOffsetY);
        m_foldBaseBody->addRow(new QLabel("偏置 Z (mm):"), m_spbOffsetZ);

        m_mainLayout->addWidget(m_foldBaseBody);
        m_foldBaseBody->setVisible(false);
    }

    void mMotionVectorForcePanel::buildMagnitudeSection()
    {
        m_foldMagnitude = new FoldSection("载荷大小", this);
        m_cmbMagnitudeMode = new QComboBox();
        m_cmbMagnitudeMode->addItems({ "常量", "表达式" });
        m_foldMagnitude->addRow(new QLabel("大小模式:"), m_cmbMagnitudeMode);
        // 常量 / 表达式输入框：与三点力/标量力一致，按模式动态插拔（不用 QStackedWidget，
        // 堆叠容器高度取所有页最大值，常量框会被表达式构造器撑高）
        m_lblMagnitude = new QLabel("大小数值 (N)", this);
        m_lblMagnitude->setParent(nullptr);

        m_spbMagnitude = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_spbMagnitude->setParent(nullptr);

        m_exprBuilder = new mMotionExprBuilderWidget(this);
        m_exprBuilder->setParent(nullptr);

        m_mainLayout->addWidget(m_foldMagnitude);
    }

    void mMotionVectorForcePanel::buildStiffnessSection()
    {
        m_foldStiffness = new FoldSection("刚度", this);

        auto makeRow = [&](const QString& label, MxFunctions::mCalcLineEdit*& spb) {
            spb = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
            m_foldStiffness->addRow(new QLabel(label), spb);
            };
        makeRow("Kxx (平动刚度 N/mm):", m_spbKxx);
        makeRow("Kyy (平动刚度 N/mm):", m_spbKyy);
        makeRow("Kzz (平动刚度 N/mm):", m_spbKzz);
        makeRow("Krx (转动刚度 N·mm/rad):", m_spbKrx);
        makeRow("Kry (转动刚度 N·mm/rad):", m_spbKry);
        makeRow("Krz (转动刚度 N·mm/rad):", m_spbKrz);

        m_mainLayout->addWidget(m_foldStiffness);
        m_foldStiffness->setVisible(false); // 仅轴套类型显示
    }

    void mMotionVectorForcePanel::buildDampingSection()
    {
        m_foldDamping = new FoldSection("阻尼", this);

        auto makeRow = [&](const QString& label, MxFunctions::mCalcLineEdit*& spb) {
            spb = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
            m_foldDamping->addRow(new QLabel(label), spb);
            };
        makeRow("Cxx (平动阻尼 N·s/mm):", m_spbCxx);
        makeRow("Cyy (平动阻尼 N·s/mm):", m_spbCyy);
        makeRow("Czz (平动阻尼 N·s/mm):", m_spbCzz);
        makeRow("Crx (转动阻尼 N·mm·s/rad):", m_spbCrx);
        makeRow("Cry (转动阻尼 N·mm·s/rad):", m_spbCry);
        makeRow("Crz (转动阻尼 N·mm·s/rad):", m_spbCrz);

        m_mainLayout->addWidget(m_foldDamping);
        m_foldDamping->setVisible(false); // 仅轴套类型显示
    }

    void mMotionVectorForcePanel::buildPreloadSection()
    {
        m_foldPreload = new FoldSection("预载", this);

        auto makeRow = [&](const QString& label, MxFunctions::mCalcLineEdit*& spb) {
            spb = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
            m_foldPreload->addRow(new QLabel(label), spb);
            };
        makeRow("F0x (预载力 N):", m_spbF0x);
        makeRow("F0y (预载力 N):", m_spbF0y);
        makeRow("F0z (预载力 N):", m_spbF0z);
        makeRow("T0x (预载扭矩 N·mm):", m_spbT0x);
        makeRow("T0y (预载扭矩 N·mm):", m_spbT0y);
        makeRow("T0z (预载扭矩 N·mm):", m_spbT0z);

        m_mainLayout->addWidget(m_foldPreload);
        m_foldPreload->setVisible(false); // 仅轴套类型显示
    }
    void mMotionVectorForcePanel::buildScalarForceSection()
    {
        m_foldScalarForce = new FoldSection("标量力", this);

        m_cmbBodyJ = new QComboBox();
        // 基本体标签（一次性创建；combo 和 pick 控件由 refreshLayoutByType 动态搬运）
        m_lblBaseBodyJ = new QLabel("基本体", this);
        m_lblBaseBodyJ->setParent(nullptr);

        m_basePointPicker = new mPointPickerWidget(QStringLiteral("基本点"), this);
        m_basePointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        // 基本点同上，动态放置

        m_cmbScalarDirectionMode = new QComboBox();
        m_cmbScalarDirectionMode->addItems({ "两点连线", "全局固定", "作用体局部固定", "基本体局部固定" });
        m_foldScalarForce->addRow(new QLabel("方向模式"), m_cmbScalarDirectionMode);

        // 输入模式下拉（常量/函数），控制载荷大小和力函数的显隐
        m_cmbScalarInputMode = new QComboBox();
        m_cmbScalarInputMode->addItems({ "常量", "函数" });
        m_foldScalarForce->addRow(new QLabel("输入模式:"), m_cmbScalarInputMode);

        m_lblScalarMagnitude = new QLabel("载荷大小 (N)", this);
        m_spbScalarMagnitude = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_lblScalarMagnitude->setParent(nullptr);
        m_spbScalarMagnitude->setParent(nullptr);

        m_forceFuncBuilder = new mMotionExprBuilderWidget(this);
        m_forceFuncBuilder->setParent(nullptr);

        m_chkActionOnly = new QCheckBox("仅作用在基本体");
        m_foldScalarForce->addRow(m_chkActionOnly);

        m_mainLayout->addWidget(m_foldScalarForce);
        m_foldScalarForce->setVisible(false);
    }

    void mMotionVectorForcePanel::buildScalarTorqueSection()
    {
        m_foldScalarTorque = new FoldSection("标量力矩", this);

        // 作用体 / 作用点复用 m_foldTarget 的控件（与标量力共用）
        m_cmbTorqueAxisMode = new QComboBox();
        m_cmbTorqueAxisMode->addItems({ "两点连线", "全局固定轴", "作用体固定轴", "基本体固定轴" });
        m_foldScalarTorque->addRow(new QLabel("轴模式"), m_cmbTorqueAxisMode);

        // 输入模式下拉（常量/函数）
        m_cmbTorqueInputMode = new QComboBox();
        m_cmbTorqueInputMode->addItems({ "常量", "函数" });
        m_foldScalarTorque->addRow(new QLabel("输入模式:"), m_cmbTorqueInputMode);

        // 固定轴方向输入（轴模式≠0时显示，两点连线时隐藏）
        m_axisDirSelector = new mDirectionSelectorWidget(
            QStringLiteral("固定轴方向"), false, QString(), false, QString(), false, this);
        connect(m_axisDirSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (m_foldScalarTorque)
                m_foldScalarTorque->refreshLayout();
            });
        // 固定轴方向由轴模式切换动态 addRow / setParent(nullptr)
        m_axisDirSelector->setParent(nullptr);  // 不放固定位置，由轴模式切换动态 addRow/setParent

        m_spbTorqueMagnitude = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_lblTorqueMagnitude = new QLabel("力矩大小 (N·mm)", this);
        m_lblTorqueMagnitude->setParent(nullptr);
        m_spbTorqueMagnitude->setParent(nullptr);

        m_torqueFuncBuilder = new mMotionExprBuilderWidget(this);
        m_torqueFuncBuilder->setParent(nullptr);

        m_chkTorqueActionOnly = new QCheckBox("仅作用在基本体");
        m_foldScalarTorque->addRow(m_chkTorqueActionOnly);

        m_mainLayout->addWidget(m_foldScalarTorque);
        m_foldScalarTorque->setVisible(false);
    }

    void mMotionVectorForcePanel::buildThreePointForceSection()
    {
        m_foldThreePointForce = new FoldSection("三点力", this);

        // 应用体 / 应用点（复用 m_foldTarget 的 m_cmbBody / m_actionPointPicker，动态搬运）

        // 出发点（方向连线起点）
        m_cmbOriginBody = new QComboBox();
        m_foldThreePointForce->addRow(new QLabel("出发体"), m_cmbOriginBody);

        m_originPointPicker = new mPointPickerWidget(QStringLiteral("出发点"), this);
        m_originPointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldThreePointForce->addRow(m_originPointPicker);

        // 目标点（方向连线终点）
        m_cmbTargetBody = new QComboBox();
        m_foldThreePointForce->addRow(new QLabel("目标体"), m_cmbTargetBody);

        m_targetPointPicker = new mPointPickerWidget(QStringLiteral("目标点"), this);
        m_targetPointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldThreePointForce->addRow(m_targetPointPicker);

        // 输入模式（常量/函数）
        m_cmbThreePointInputMode = new QComboBox();
        m_cmbThreePointInputMode->addItems({ "常量", "函数" });
        m_foldThreePointForce->addRow(new QLabel("输入模式:"), m_cmbThreePointInputMode);

        m_lblThreePointMagnitude = new QLabel("载荷大小 (N)", this);
        m_spbThreePointMagnitude = makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard);
        m_lblThreePointMagnitude->setParent(nullptr);
        m_spbThreePointMagnitude->setParent(nullptr);

        m_threePointFuncBuilder = new mMotionExprBuilderWidget(this);
        m_threePointFuncBuilder->setParent(nullptr);

        m_mainLayout->addWidget(m_foldThreePointForce);
        m_foldThreePointForce->setVisible(false);
    }

    // =====================================================================
    // 信号绑定
    // =====================================================================

    namespace
    {
        //力元类型下拉索引 -> VectorLoadType（下拉不含衬套/不支持项，索引与枚举值不对齐）
        const QVector<MechMotionData::VectorLoadType>& comboTypeTable()
        {
            static const QVector<MechMotionData::VectorLoadType> table = {
                MechMotionData::VectorLoadType::Force,
                MechMotionData::VectorLoadType::Torque,
                MechMotionData::VectorLoadType::TriForce,
                MechMotionData::VectorLoadType::TriTorque,
                MechMotionData::VectorLoadType::ScalarForce,
                MechMotionData::VectorLoadType::ScalarTorque,
                MechMotionData::VectorLoadType::Bushing,
            };
            return table;
        }
    }
    void mMotionVectorForcePanel::connectSignals()
    {
        // ---- 载荷类型切换：切换时替换统一力元对象 ----
        connect(m_cmbLoadType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_forceElementData) return;
            //末尾只读"衬套"项（仅编辑衬套会话存在）不可经此切换
            if (index < 0 || index >= comboTypeTable().size()) return;
            handleLoadTypeChange(comboTypeTable().at(index));
            });

        // ---- 作用体（marker_I 侧）切换 ----
        connect(m_cmbBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            int bodyId = m_cmbBody->itemData(index).toInt();
            int feId = -1;
            if (auto* data = vectorData()) { feId = data->getId(); mMotionRefMap::set(Physical_MotionVectorForce, feId, Physical_MotionBody, MapRelation_MotionActionBody, bodyId); }
            if (auto* data = scalarData()) { feId = data->getId(); mMotionRefMap::set(Physical_MotionVectorForce, feId, Physical_MotionBody, MapRelation_MotionBodyI, bodyId); }
            if (auto* data = scalarTorqueData()) { feId = data->getId(); mMotionRefMap::set(Physical_MotionVectorForce, feId, Physical_MotionBody, MapRelation_MotionBodyI, bodyId); }
            if (auto* data = threePointData()) { feId = data->getId(); mMotionRefMap::set(Physical_MotionVectorForce, feId, Physical_MotionBody, MapRelation_MotionBodyI, bodyId); }
            if (auto* data = threePointTorqueData()) { feId = data->getId(); mMotionRefMap::set(Physical_MotionVectorForce, feId, Physical_MotionBody, MapRelation_MotionBodyI, bodyId); }
            updateRenderFromData();
            notifyDataChanged();
            });

        // ---- 作用点（marker_I 原点） ----
        connect(m_actionPointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_actionPointPicker->coordinate();
            if (auto* data = vectorData()) data->setActionPoint(pt);
            if (auto* data = scalarData()) data->setLocalI(pt);
            if (auto* data = scalarTorqueData()) data->setLocalI(pt);
            if (auto* data = threePointData()) data->setLocalI(pt);
            if (auto* data = threePointTorqueData()) data->setLocalI(pt);
            updateRenderFromData();
            });

        // ---- 方向向量（marker_I 定向，方向控件） ----
        connect(m_directionSelector, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            QVariant dv = m_directionSelector->getDirection();
            if (!dv.isValid() || dv.isNull())
                return;
            const QVector3D vec = dv.value<QVector3D>();
            if (auto* data = vectorData()) { data->setDirection(vec); saveDirPickState(m_directionSelector, data->dirPickState_); }
            if (auto* data = scalarData()) { data->setFixedDirection(vec); saveDirPickState(m_directionSelector, data->dirPickState_); }
            updateRenderFromData();
            });

        // ---- 基本体（marker_J 侧）切换 ----
        connect(m_cmbBaseBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (auto* b = bushingData()) {
                mMotionRefMap::set(Physical_MotionVectorForce, b->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, m_cmbBaseBody->itemData(index).toInt());
                updateRenderFromData();
                notifyDataChanged();
            }
            });

        // ---- 应用偏置切换：联动偏置输入框启用状态 ----
        connect(m_cmbBaseOffset, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            const bool apply = (index == 1);
            m_spbOffsetX->setEnabled(apply);
            m_spbOffsetY->setEnabled(apply);
            m_spbOffsetZ->setEnabled(apply);
            if (auto* b = bushingData()) b->setApplyOffset(apply);
            });

        // ---- 偏置位移实时同步 ----
        auto syncOffset = [this]() {
            if (auto* b = bushingData())
                b->setOffset(QVector3D(m_spbOffsetX->text().toDouble(), m_spbOffsetY->text().toDouble(), m_spbOffsetZ->text().toDouble()));
            };
        connect(m_spbOffsetX, &QLineEdit::editingFinished, this, syncOffset);
        connect(m_spbOffsetY, &QLineEdit::editingFinished, this, syncOffset);
        connect(m_spbOffsetZ, &QLineEdit::editingFinished, this, syncOffset);

        // ---- 预载实时同步 ----
        auto syncPreloadForce = [this]() {
            if (auto* b = bushingData()) {
                QVector<double> f0;
                f0 << m_spbF0x->text().toDouble() << m_spbF0y->text().toDouble() << m_spbF0z->text().toDouble();
                b->setPreloadForce(f0);
            }
            };
        auto syncPreloadTorque = [this]() {
            if (auto* b = bushingData()) {
                QVector<double> t0;
                t0 << m_spbT0x->text().toDouble() << m_spbT0y->text().toDouble() << m_spbT0z->text().toDouble();
                b->setPreloadTorque(t0);
            }
            };
        connect(m_spbF0x, &QLineEdit::editingFinished, this, syncPreloadForce);
        connect(m_spbF0y, &QLineEdit::editingFinished, this, syncPreloadForce);
        connect(m_spbF0z, &QLineEdit::editingFinished, this, syncPreloadForce);
        connect(m_spbT0x, &QLineEdit::editingFinished, this, syncPreloadTorque);
        connect(m_spbT0y, &QLineEdit::editingFinished, this, syncPreloadTorque);
        connect(m_spbT0z, &QLineEdit::editingFinished, this, syncPreloadTorque);

        // ---- 刚度实时同步 ----
        auto syncStiffness = [this]() {
            if (auto* b = bushingData()) {
                QVector<double> k;
                k << m_spbKxx->text().toDouble() << m_spbKyy->text().toDouble() << m_spbKzz->text().toDouble()
                  << m_spbKrx->text().toDouble() << m_spbKry->text().toDouble() << m_spbKrz->text().toDouble();
                b->setStiffness(k);
            }
            };
        connect(m_spbKxx, &QLineEdit::editingFinished, this, syncStiffness);
        connect(m_spbKyy, &QLineEdit::editingFinished, this, syncStiffness);
        connect(m_spbKzz, &QLineEdit::editingFinished, this, syncStiffness);
        connect(m_spbKrx, &QLineEdit::editingFinished, this, syncStiffness);
        connect(m_spbKry, &QLineEdit::editingFinished, this, syncStiffness);
        connect(m_spbKrz, &QLineEdit::editingFinished, this, syncStiffness);

        // ---- 阻尼实时同步 ----
        auto syncDamping = [this]() {
            if (auto* b = bushingData()) {
                QVector<double> c;
                c << m_spbCxx->text().toDouble() << m_spbCyy->text().toDouble() << m_spbCzz->text().toDouble()
                  << m_spbCrx->text().toDouble() << m_spbCry->text().toDouble() << m_spbCrz->text().toDouble();
                b->setDamping(c);
            }
            };
        connect(m_spbCxx, &QLineEdit::editingFinished, this, syncDamping);
        connect(m_spbCyy, &QLineEdit::editingFinished, this, syncDamping);
        connect(m_spbCzz, &QLineEdit::editingFinished, this, syncDamping);
        connect(m_spbCrx, &QLineEdit::editingFinished, this, syncDamping);
        connect(m_spbCry, &QLineEdit::editingFinished, this, syncDamping);
        connect(m_spbCrz, &QLineEdit::editingFinished, this, syncDamping);

        // ---- 方向模式切换 ----
        connect(m_cmbDirectionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (auto* data = vectorData()) data->setDirectionMode(static_cast<DirectionMode>(index));
            });

        // ---- 大小模式切换：常量/表达式输入框动态插拔（插到大小模式下拉正下方） ----
        connect(m_cmbMagnitudeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            m_lblMagnitude->setParent(nullptr);
            m_spbMagnitude->setParent(nullptr);
            m_exprBuilder->setParent(nullptr);
            {
                int row = -1;
                QFormLayout::ItemRole role;
                m_foldMagnitude->layout()->getWidgetPosition(m_cmbMagnitudeMode, &row, &role);
                if (row >= 0) {
                    if (idx == 0)
                        m_foldMagnitude->insertRow(row + 1, m_lblMagnitude, m_spbMagnitude);
                    else
                        m_foldMagnitude->insertRow(row + 1, m_exprBuilder);
                }
            }
            QTimer::singleShot(0, m_foldMagnitude, [this]() { m_foldMagnitude->refreshLayout(); });
            if (!m_forceElementData) return;
            if (idx == 0) {
                if (auto* data = vectorData()) {
                    data->setMagnitudeMode(MagnitudeMode::Constant);
                    data->setMagnitude(m_spbMagnitude->text().toDouble());
                }
            }
            else {
                if (auto* data = vectorData()) {
                    data->setMagnitudeMode(MagnitudeMode::Expression);
                    if (m_exprBuilder->isValid())
                        data->setExpression(m_exprBuilder->expression());
                }
            }
            });

        // ---- 常量大小实时同步 ----
        connect(m_spbMagnitude, &QLineEdit::editingFinished, this, [this]() {
            double val = m_spbMagnitude->text().toDouble();
            if (auto* data = vectorData()) data->setMagnitude(val);
            if (auto* data = scalarData()) data->setMagnitude(val);
            });

        // ---- 表达式实时同步 ----
        connect(m_exprBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [this](const QString& text) {
            if (auto* data = vectorData()) data->setExpression(text);
            if (auto* data = scalarData()) data->setForceFunction(text);
            });

        connect(m_cmbBodyJ, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (auto* data = scalarData()) {
                mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionBodyJ, m_cmbBodyJ->itemData(index).toInt());
                updateRenderFromData();
                notifyDataChanged();
            }
            if (auto* data = scalarTorqueData()) {
                mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionBodyJ, m_cmbBodyJ->itemData(index).toInt());
                updateRenderFromData();
                notifyDataChanged();
            }
            });

        connect(m_basePointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_basePointPicker->coordinate();
            if (auto* data = scalarData()) {
                data->setLocalJ(pt);
                updateRenderFromData();
            }
            if (auto* data = scalarTorqueData()) {
                data->setLocalJ(pt);
                updateRenderFromData();
            }
            });

        connect(m_cmbScalarDirectionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (auto* data = scalarData())
                data->setDirectionMode(static_cast<ScalarForceDirectionMode>(index));
            // 两点连线(0)时方向由两点决定，矢量方向定义区隐藏；固定方向(1/2/3)恢复显示
            if (m_forceElementData && m_forceElementData->getLoadType() == VectorLoadType::ScalarForce)
                m_foldDirection->setVisible(index != 0);
            });

        connect(m_cmbScalarInputMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            // 常量(0)：显示载荷大小，移除力函数；函数(1)：显示力函数，移除载荷大小
            // 用 insertRow 插入到输入模式下拉的正下方，保证关联控件相邻
            m_lblScalarMagnitude->setParent(nullptr);
            m_spbScalarMagnitude->setParent(nullptr);
            m_forceFuncBuilder->setParent(nullptr);
            {
                int row = -1;
                QFormLayout::ItemRole role;
                m_foldScalarForce->layout()->getWidgetPosition(m_cmbScalarInputMode, &row, &role);
                if (row >= 0) {
                    if (index == 0) {
                        m_foldScalarForce->insertRow(row + 1, m_lblScalarMagnitude, m_spbScalarMagnitude);
                        if (auto* data = scalarData()) data->setForceFunction("");
                    }
                    else {
                        m_foldScalarForce->insertRow(row + 1, m_forceFuncBuilder);
                    }
                }
            }
            QTimer::singleShot(0, m_foldScalarForce, [this]() { m_foldScalarForce->refreshLayout(); });
            });

        connect(m_spbScalarMagnitude, &QLineEdit::editingFinished, this, [this]() {
            if (auto* data = scalarData()) data->setMagnitude(m_spbScalarMagnitude->text().toDouble());
            });

        connect(m_forceFuncBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [this](const QString& text) {
            if (auto* data = scalarData()) data->setForceFunction(text);
            });

        // ---- 标量力矩同步 ----
        connect(m_cmbTorqueAxisMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (auto* data = scalarTorqueData()) data->setAxisMode(static_cast<MechMotionData::ScalarTorqueAxisMode>(index));
            // 两点连线(0)时从折叠区移除，固定轴(1/2/3)时 insertRow 到轴模式下拉正下方
            if (index != 0) {
                if (m_axisDirSelector->parent() == nullptr) {
                    int row = -1;
                    QFormLayout::ItemRole role;
                    m_foldScalarTorque->layout()->getWidgetPosition(m_cmbTorqueAxisMode, &row, &role);
                    if (row >= 0) {
                        m_foldScalarTorque->insertRow(row + 1, m_axisDirSelector);
                    }
                }
            }
            else {
                if (m_axisDirSelector->parent() != nullptr) {
                    m_axisDirSelector->setParent(nullptr);
                }
            }
            QTimer::singleShot(0, m_foldScalarTorque, [this]() { m_foldScalarTorque->refreshLayout(); });
            });
        // 固定轴方向写回（方向控件）
        connect(m_axisDirSelector, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            QVariant dv = m_axisDirSelector->getDirection();
            if (!dv.isValid() || dv.isNull())
                return;
            if (auto* data = scalarTorqueData()) {
                data->setFixedAxis(dv.value<QVector3D>());
                saveDirPickState(m_axisDirSelector, data->fixedAxisPickState_);
            }
            });
        connect(m_spbTorqueMagnitude, &QLineEdit::editingFinished, this, [this]() {
            if (auto* data = scalarTorqueData()) data->setMagnitude(m_spbTorqueMagnitude->text().toDouble());
            });
        connect(m_cmbTorqueInputMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            // 常量(0)：显示力矩大小，移除力矩函数；函数(1)：显示力矩函数，移除力矩大小
            // 用 insertRow 插入到输入模式下拉的正下方
            m_lblTorqueMagnitude->setParent(nullptr);
            m_spbTorqueMagnitude->setParent(nullptr);
            m_torqueFuncBuilder->setParent(nullptr);
            {
                int row = -1;
                QFormLayout::ItemRole role;
                m_foldScalarTorque->layout()->getWidgetPosition(m_cmbTorqueInputMode, &row, &role);
                if (row >= 0) {
                    if (index == 0) {
                        m_foldScalarTorque->insertRow(row + 1, m_lblTorqueMagnitude, m_spbTorqueMagnitude);
                        if (auto* data = scalarTorqueData()) data->setTorqueFunction("");
                    }
                    else {
                        m_foldScalarTorque->insertRow(row + 1, m_torqueFuncBuilder);
                    }
                }
            }
            QTimer::singleShot(0, m_foldScalarTorque, [this]() { m_foldScalarTorque->refreshLayout(); });
            });

        connect(m_torqueFuncBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [this](const QString& text) {
            if (auto* data = scalarTorqueData()) data->setTorqueFunction(text);
            });
        connect(m_chkTorqueActionOnly, &QCheckBox::stateChanged, this, [this](int state) {
            if (auto* data = scalarTorqueData()) data->setActionOnly(state == Qt::Checked);
            });

        connect(m_chkActionOnly, &QCheckBox::stateChanged, this, [this](int state) {
            if (auto* data = scalarData()) data->setActionOnly(state == Qt::Checked);
            });

        // ---- 三点力同步 ----
        // 应用体/应用点复用 m_cmbBody / m_actionPointPicker 的既有绑定，
        // 此处只补出发点/目标点、输入模式与幅值
        connect(m_cmbOriginBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            const int bodyId = m_cmbOriginBody->itemData(index).toInt();
            if (auto* data = threePointData()) mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionActionBody, bodyId);
            if (auto* data = threePointTorqueData()) mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionActionBody, bodyId);
            updateRenderFromData();
            notifyDataChanged();
            });
        connect(m_originPointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_originPointPicker->coordinate();
            if (auto* data = threePointData()) data->setLocalOrigin(pt);
            if (auto* data = threePointTorqueData()) data->setLocalOrigin(pt);
            updateRenderFromData();
            });
        connect(m_cmbTargetBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            const int bodyId = m_cmbTargetBody->itemData(index).toInt();
            if (auto* data = threePointData()) mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, bodyId);
            if (auto* data = threePointTorqueData()) mMotionRefMap::set(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, bodyId);
            updateRenderFromData();
            notifyDataChanged();
            });
        connect(m_targetPointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            const QVector3D pt = m_targetPointPicker->coordinate();
            if (auto* data = threePointData()) data->setLocalTarget(pt);
            if (auto* data = threePointTorqueData()) data->setLocalTarget(pt);
            updateRenderFromData();
            });
        connect(m_cmbThreePointInputMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index == 1) {
                if (auto* data = threePointData()) data->setForceFunction("");
                if (auto* data = threePointTorqueData()) data->setTorqueFunction("");
            }
            refreshLayoutByType(m_forceElementData && m_forceElementData->getLoadType() == VectorLoadType::Bushing);
            });
        connect(m_spbThreePointMagnitude, &QLineEdit::editingFinished, this, [this]() {
            const double val = m_spbThreePointMagnitude->text().toDouble();
            if (auto* data = threePointData()) data->setMagnitude(val);
            if (auto* data = threePointTorqueData()) data->setMagnitude(val);
            });
        connect(m_threePointFuncBuilder, &mMotionExprBuilderWidget::expressionAccepted, this, [this](const QString& text) {
            if (auto* data = threePointData()) data->setForceFunction(text);
            if (auto* data = threePointTorqueData()) data->setTorqueFunction(text);
            });
    }

    // =====================================================================
    // 辅助
    // =====================================================================

    MechMotionData::mMotionBushing* mMotionVectorForcePanel::bushingData()
    {
        return dynamic_cast<MechMotionData::mMotionBushing*>(m_forceElementData);
    }

    MechMotionData::mMotionVectorForce* mMotionVectorForcePanel::vectorData()
    {
        return dynamic_cast<MechMotionData::mMotionVectorForce*>(m_forceElementData);
    }

    MechMotionData::mMotionScalarForce* mMotionVectorForcePanel::scalarData()
    {
        return dynamic_cast<MechMotionData::mMotionScalarForce*>(m_forceElementData);
    }

    MechMotionData::mMotionScalarTorque* mMotionVectorForcePanel::scalarTorqueData()
    {
        return dynamic_cast<MechMotionData::mMotionScalarTorque*>(m_forceElementData);
    }

    MechMotionData::mMotionThreePointForce* mMotionVectorForcePanel::threePointData()
    {
        return dynamic_cast<MechMotionData::mMotionThreePointForce*>(m_forceElementData);
    }

    MechMotionData::mMotionThreePointTorque* mMotionVectorForcePanel::threePointTorqueData()
    {
        return dynamic_cast<MechMotionData::mMotionThreePointTorque*>(m_forceElementData);
    }

    void mMotionVectorForcePanel::notifyDataChanged()
    {
        if (m_forceElementData)
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionVectorForceSig(false, m_forceElementData->getId());
    }

    // 力元默认名前缀（与各数据类构造函数的命名规则一致），用于类型切换时名称跟随
    static QString loadTypeDefaultPrefix(MechMotionData::VectorLoadType type)
    {
        using namespace MechMotionData;
        switch (type) {
        case VectorLoadType::Force:           return "VectorForce";
        case VectorLoadType::Torque:          return "VectorTorque";
        case VectorLoadType::UniForce:        return "UniForce";
        case VectorLoadType::UniTorque:       return "UniTorque";
        case VectorLoadType::TriForce:        return "TriForce";
        case VectorLoadType::TriTorque:       return "TriTorque";
        case VectorLoadType::GeneralForce:    return "GeneralForce";
        case VectorLoadType::Gravity:         return "Gravity";
        case VectorLoadType::ForceField:      return "ForceField";
        case VectorLoadType::LinearSpringDamper: return "SpringDamper";
        case VectorLoadType::TireForce:       return "TireForce";
        case VectorLoadType::Bushing:         return "Bushing";
        case VectorLoadType::ScalarForce:     return "ScalarForce";
        case VectorLoadType::ScalarTorque:    return "ScalarTorque";
        default:                              return "VectorForce";
        }
    }

    void mMotionVectorForcePanel::handleLoadTypeChange(VectorLoadType newType)
    {
        if (!m_forceElementData || m_forceElementData->getLoadType() == newType)
            return;

        auto& mgr = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce);
        const int id = m_forceElementData->getId();
        // 名称策略（与运动副 handleTypeChange 一致，标志存于数据类 m_isUserRename）：
        // 用户重命名过则保留原名；默认名 "前缀_编号" 跟随新类型
        const bool wasUserRename = m_forceElementData->getIsUserRename();
        QString name = m_forceElementData->getName();
        if (!wasUserRename) {
            const QStringList parts = name.split("_");
            bool suffixIsNumber = false;
            if (parts.size() == 2)
                parts.at(1).toInt(&suffixIsNumber);
            if (suffixIsNumber) {
                const QString newName = QString("%1_%2").arg(loadTypeDefaultPrefix(newType)).arg(parts.at(1));
                // 新名与其他力元冲突时保留原名（同一 id 后缀正常不会冲突）
                const int owner = mgr.getIDByName(newName);
                if (owner == -1 || owner == id)
                    name = newName;
            }
        }

        auto* oldVector = vectorData();
        auto* oldScalar = scalarData();
        auto* oldScalarTorque = scalarTorqueData();
        // 类型互转时尽量搬运端点/幅值等公共字段，避免用户已填数据全部丢失
        const int srcFeId = oldVector ? oldVector->getId() : (oldScalar ? oldScalar->getId() : (oldScalarTorque ? oldScalarTorque->getId() : -1));
        const int commonBodyI = srcFeId >= 0 ? mMotionRefMap::get(Physical_MotionVectorForce, srcFeId, Physical_MotionBody,
            oldVector ? MapRelation_MotionActionBody : MapRelation_MotionBodyI) : -1;
        const QVector3D commonLocalI = oldVector ? oldVector->getActionPoint()
            : oldScalar ? oldScalar->getLocalI()
            : oldScalarTorque ? oldScalarTorque->getLocalI() : QVector3D();
        const int commonBodyJ = (oldScalar || oldScalarTorque) && srcFeId >= 0 ? mMotionRefMap::get(Physical_MotionVectorForce, srcFeId, Physical_MotionBody, MapRelation_MotionBodyJ) : -1;
        const QVector3D commonLocalJ = oldScalar ? oldScalar->getLocalJ()
            : oldScalarTorque ? oldScalarTorque->getLocalJ() : QVector3D();
        const QVector3D commonDir = oldVector ? oldVector->getDirection()
            : oldScalar ? oldScalar->getFixedDirection()
            : oldScalarTorque ? oldScalarTorque->getFixedAxis() : QVector3D(1, 0, 0);
        const double commonMagnitude = oldVector ? oldVector->getMagnitude()
            : oldScalar ? oldScalar->getMagnitude()
            : oldScalarTorque ? oldScalarTorque->getMagnitude() : 0.0;
        const QString commonFunction = oldVector ? oldVector->getExpression()
            : oldScalar ? oldScalar->getForceFunction()
            : oldScalarTorque ? oldScalarTorque->getTorqueFunction() : QString();

        mMotionForceElement* newData = nullptr;
        if (newType == VectorLoadType::TriForce) {
            // 三点力：应用点继承原作用体/作用点；出发点/目标点默认大地（原点重合，方向退化为 X）
            auto* data = new MechMotionData::mMotionThreePointForce(id, name);
            data->setLocalI(commonLocalI);
            
            data->setLocalOrigin(QVector3D(0, 0, 0));
            data->setLocalTarget(QVector3D(1, 0, 0));
            data->setMagnitude(commonMagnitude);
            data->setForceFunction(commonFunction);
            newData = data;
        }
        else if (newType == VectorLoadType::TriTorque) {
            // 三点力矩：应用点继承原作用体/作用点；出发点/目标点默认大地（轴为全局 X）
            auto* data = new MechMotionData::mMotionThreePointTorque(id, name);
            data->setLocalI(commonLocalI);
            
            data->setLocalOrigin(QVector3D(0, 0, 0));
            data->setLocalTarget(QVector3D(1, 0, 0));
            data->setMagnitude(commonMagnitude);
            data->setTorqueFunction(commonFunction);
            newData = data;
        }
        else if (newType == VectorLoadType::ScalarForce) {
            auto* data = new mMotionScalarForce(id, name);
            data->setLocalI(commonLocalI);
            data->setLocalJ(commonLocalJ);
            data->setFixedDirection(commonDir);
            data->setMagnitude(commonMagnitude);
            data->setForceFunction(commonFunction);
            newData = data;
        }
        else if (newType == VectorLoadType::ScalarTorque) {
            auto* data = new mMotionScalarTorque(id, name);
            data->setLocalI(commonLocalI);
            data->setLocalJ(commonLocalJ);
            data->setFixedAxis(commonDir);
            data->setMagnitude(commonMagnitude);
            data->setTorqueFunction(commonFunction);
            newData = data;
        }
        else {
            mMotionVectorForce* data = nullptr;
            if (newType == VectorLoadType::Bushing)
                data = new mMotionBushing(id, name);
            else
                data = new mMotionVectorForce(id, newType, name);

            if (oldVector) {
                data->setActionPoint(oldVector->getActionPoint());
                data->setDirection(oldVector->getDirection());
                data->setMagnitude(oldVector->getMagnitude());
                data->setExpression(oldVector->getExpression());
                data->setMagnitudeMode(oldVector->getMagnitudeMode());
                data->setDirectionMode(oldVector->getDirectionMode());
                auto* oldBushing = dynamic_cast<mMotionBushing*>(oldVector);
                auto* newBushing = dynamic_cast<mMotionBushing*>(data);
                if (oldBushing && newBushing) {
                    newBushing->setStiffness(oldBushing->getStiffness());
                    newBushing->setDamping(oldBushing->getDamping());
                }
            }
            else {
                // 来自标量力/标量力矩：搬运端点与幅值/函数
                data->setActionPoint(commonLocalI);
                data->setDirection(commonDir);
                data->setMagnitude(commonMagnitude);
                data->setExpression(commonFunction);
            }
            newData = data;
        }

        // 重命名标志随新对象传递，保证后续类型切换仍能正确判断
        newData->setName(name, wasUserRename);

        mgr.deleteData(id);
        mgr.addData(newData);
        m_forceElementData = newData;
        //类型切换替换数据对象后全量反显（拾取会话模式保留）
        updateUIFromData();

        //类型切换后可见性变化，刷新拾取激活状态
        activateFirstVisiblePointPicker({ m_actionPointPicker, m_basePointPicker, m_originPointPicker, m_targetPointPicker },
            { m_directionSelector, m_axisDirSelector });

        notifyDataChanged();
    }

    // =====================================================================
    // 对外接口（新建 / 编辑 / 取消编辑）
    // =====================================================================

    void mMotionVectorForcePanel::newMotionVectorForcePanel(VectorLoadType type)
    {
        setEditing(true);
        connectSig();

        int forceID = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).createID();
        // 按载荷类型创建具体力元对象
        if (type == VectorLoadType::ScalarForce)
            m_forceElementData = new mMotionScalarForce(forceID);
        else if (type == VectorLoadType::ScalarTorque)
            m_forceElementData = new mMotionScalarTorque(forceID);
        else if (type == VectorLoadType::Bushing)
            m_forceElementData = new mMotionBushing(forceID);
        else if (type == VectorLoadType::TriForce) {
            // 三点力：出发/目标点默认大地（连线为全局 X 方向），用户再按需挂体
            auto* tp = new MechMotionData::mMotionThreePointForce(forceID);
            tp->setLocalOrigin(QVector3D(0, 0, 0));
            tp->setLocalTarget(QVector3D(1, 0, 0));
            m_forceElementData = tp;
        }
        else if (type == VectorLoadType::TriTorque) {
            // 三点力矩：出发/目标点默认大地（轴为全局 X 方向），用户再按需挂体
            auto* tt = new MechMotionData::mMotionThreePointTorque(forceID);
            tt->setLocalOrigin(QVector3D(0, 0, 0));
            tt->setLocalTarget(QVector3D(1, 0, 0));
            m_forceElementData = tt;
        }
        else
            m_forceElementData = new mMotionVectorForce(forceID, type);
        QString defaultName;
        switch (type) {
        case VectorLoadType::Force: defaultName = "矢量力"; break;
        case VectorLoadType::Torque: defaultName = "矢量力矩"; break;
        case VectorLoadType::UniForce: defaultName = "单向力"; break;
        case VectorLoadType::UniTorque: defaultName = "单向力矩"; break;
        case VectorLoadType::TriForce: defaultName = "三点力"; break;
        case VectorLoadType::TriTorque: defaultName = "三点力矩"; break;
        case VectorLoadType::GeneralForce: defaultName = "一般作用力"; break;
        case VectorLoadType::Gravity: defaultName = "重力"; break;
        case VectorLoadType::ForceField: defaultName = "力场"; break;
        case VectorLoadType::LinearSpringDamper: defaultName = "线性弹簧阻尼"; break;
        case VectorLoadType::TireForce: defaultName = "轮胎力"; break;
        case VectorLoadType::Bushing: defaultName = "轴套"; break;
        case VectorLoadType::ScalarForce: defaultName = "标量力"; break;
        case VectorLoadType::ScalarTorque: defaultName = "标量力矩"; break;
        default: defaultName = "矢量力"; break;
        }
        getCVM->getModelData()->addPhysicalField<MechMotionData::mMotionForceElement>(m_forceElementData);

        //新建对象：清掉方向控件里上一个对象残留的拾取会话
        for (mDirectionSelectorWidget* selector : { m_directionSelector, m_axisDirSelector })
        {
            restoreDirPickState(selector, DirectionPickState(), QVector3D());
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionVectorForceSig(true, m_forceElementData->getId());
    }

    void mMotionVectorForcePanel::editMotionVectorForcePanel(int id)
    {
        setEditing(true);
        m_forceElementData = getCVM->getModelData()->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).getData(id);
        if (!m_forceElementData)
            return;

        emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);

        connectSig();

        //进入编辑为新会话：反显数据值（拾取会话模式仍保留）
        updateUIFromData();

        //进入编辑激活第一个可见点拾取控件（作用点），方向控件按模式自管理
        activateFirstVisiblePointPicker({ m_actionPointPicker, m_basePointPicker, m_originPointPicker, m_targetPointPicker },
            { m_directionSelector, m_axisDirSelector });

        //恢复拾取会话：延迟到事件循环下一拍（避开树点击链路后续清理），详见Joint面板注释
        QTimer::singleShot(0, this, [this]() {
            if (!m_forceElementData)
                return;
            restorePickSessionsOnEntry({ m_actionPointPicker, m_basePointPicker, m_originPointPicker, m_targetPointPicker },
                { m_directionSelector, m_axisDirSelector });
            });
    }

    //数据对象即将被删除（树删除力元时在 delete 数据前调用）：置空编辑指针防悬垂。
    //周期边界面板 onPeriodicBoundaryDataAboutToBeDestroyed 同款约定
    void mMotionVectorForcePanel::onForceElementDataAboutToBeDestroyed(int feID)
    {
        if (feID >= 0)
        {
            if (m_forceElementData == nullptr || feID != m_forceElementData->getId())
                return;
        }
        m_forceElementData = nullptr;
    }

    void mMotionVectorForcePanel::uneditMotionVectorForcePanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        //退出编辑时失活所有点拾取控件并清理高亮
        for (mPointPickerWidget* picker : { m_actionPointPicker, m_basePointPicker, m_originPointPicker, m_targetPointPicker })
        {
            if (picker && picker->pickController())
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
                //拾取数据是临时的（确定值已入数据类），退出编辑清空防残留高亮
                picker->pickController()->displayData(PickWidgetData());
            }
        }
        //退出编辑时失活方向控件内部拾取
        for (mDirectionSelectorWidget* selector : { m_directionSelector, m_axisDirSelector })
        {
            if (!selector)
                continue;
            const QList<mPickWidgetController*> controllers = {
                selector->startPickController(), selector->endPickController(), selector->objectPickController() };
            for (auto* controller : controllers)
            {
                if (controller)
                {
                    controller->setPickActive(false);
                    controller->clearAllHighlight();
                }
            }
        }
        // 清理作用点显示和高亮；不删除符号，避免符号控制器状态重建失败
        // 保持与运动副面板一致
        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            commonShapeRender->deletePoint("vectorForcePoint");
            commonShapeRender->deletePoint("scalarForcePointI");
            commonShapeRender->deletePoint("scalarForcePointJ");
            commonShapeRender->deletePoint("scalarTorquePointI");
            commonShapeRender->deletePoint("scalarTorquePointJ");
            commonShapeRender->deletePoint("threePointForcePointI");
            commonShapeRender->deletePoint("threePointForcePointO");
            commonShapeRender->deletePoint("threePointForcePointT");
            commonShapeRender->deleteCylinder("threePointForceDir");
            highlightBody(-1, "scalarTorqueBodyI", QColor());
            highlightBody(-1, "scalarTorqueBodyJ", QColor());
            highlightBody(-1, "threePointForceBodyI", QColor());
            highlightBody(-1, "threePointForceBodyO", QColor());
            highlightBody(-1, "threePointForceBodyT", QColor());

            if (m_forceElementData) {
                if (auto* scalar = scalarData()) {
                    mMotionVectorSymbolHelper::clearVectorSymbol(
                        symbolController, QString("scalarForce_%1").arg(scalar->getId()));
                }
                if (auto* stq = scalarTorqueData()) {
                    mMotionVectorSymbolHelper::clearVectorSymbol(
                        symbolController, QString("scalarTorque_%1").arg(stq->getId()));
                }
                else if (auto* tpf = threePointData()) {
                    mMotionVectorSymbolHelper::clearVectorSymbol(
                        symbolController, QString("threePointForce_%1").arg(tpf->getId()));
                }
                else if (auto* tpt = threePointTorqueData()) {
                    mMotionVectorSymbolHelper::clearVectorSymbol(
                        symbolController, QString("threePointTorque_%1").arg(tpt->getId()));
                }
                else {
                    mMotionVectorSymbolHelper::clearVectorSymbol(
                        symbolController, QString("vectorForce_%1").arg(m_forceElementData->getId()));
                }
            }

            // 清理动作体 / 基本体高亮
            clearBodyHighlights();

            getCVM->getPreRenderWidget()->updatePaint();
        }

        m_forceElementData = nullptr;
        disconnectSig();
    }

    // =====================================================================
    // 界面刷新
    // =====================================================================

    void mMotionVectorForcePanel::refreshBodyList()
    {
        const QSignalBlocker b(m_cmbBody);
        const QSignalBlocker bBase(m_cmbBaseBody);
        const QSignalBlocker bScalarBody(m_cmbBodyJ);
        const QSignalBlocker bOrigin(m_cmbOriginBody);
        const QSignalBlocker bTarget(m_cmbTargetBody);
        m_cmbBody->clear();
        m_cmbBody->addItem(" ", -1); // 默认空白项
        m_cmbBaseBody->clear();
        m_cmbBaseBody->addItem(" ", -1);
        m_cmbBodyJ->clear();
        m_cmbBaseBody->addItem(" ", -1); // 默认空白项（大地）
        m_cmbBodyJ->addItem(" ", -1);
        m_cmbOriginBody->clear();
        m_cmbOriginBody->addItem(" ", -1); // 空白项=大地，与其他下拉一致
        m_cmbTargetBody->clear();
        m_cmbTargetBody->addItem(" ", -1);

        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        std::vector<int> allIds = bodyManager.getAllIDs();
        for (int id : allIds) {
            QString name = bodyManager.getNameByID(id);
            m_cmbBody->addItem(name, id);
            m_cmbBaseBody->addItem(name, id);
            m_cmbBodyJ->addItem(name, id);
            m_cmbOriginBody->addItem(name, id);
            m_cmbTargetBody->addItem(name, id);
        }
    }

    void mMotionVectorForcePanel::refreshLayoutByType(bool)
    {
        const auto loadType = m_forceElementData ? m_forceElementData->getLoadType() : VectorLoadType::Force;
        const bool isScalarForce = loadType == VectorLoadType::ScalarForce;
        const bool isScalarTorque = loadType == VectorLoadType::ScalarTorque;
        const bool isScalar = isScalarForce;
        const bool isBushing = !isScalarForce && !isScalarTorque && loadType == VectorLoadType::Bushing;
        // 标量力"两点连线"(方向模式 0)时方向由基点→作用点连线决定，
        // 矢量方向定义区（固定方向输入）不参与，应隐藏
        const bool scalarForceTwoPoint = isScalarForce && scalarData()
            && static_cast<int>(scalarData()->getDirectionMode()) == 0;
        // 轴套专属折叠框：仅轴套显示
        m_foldBaseBody->setVisible(isBushing);
        m_foldStiffness->setVisible(isBushing);
        m_foldDamping->setVisible(isBushing);
        m_foldPreload->setVisible(isBushing);

        // 非轴套控件：标量力时隐藏载荷大小，轴套时也隐藏载荷大小
        m_foldMagnitude->setVisible(!isScalarForce && !isScalarTorque && !isBushing);
        m_foldScalarForce->setVisible(isScalarForce);
        m_foldScalarTorque->setVisible(isScalarTorque);
        // 与运动副面板保持一致：移除时脱离父窗口，插回时重新 addRow
        auto ensurePairInLayout = [](QLabel* label, QWidget* field, bool shouldShow, FoldSection* section) {
            if (shouldShow) {
                if (label->parent() == nullptr || field->parent() == nullptr) {
                    section->addRow(label, field);
                }
            }
            else {
                if (label->parent() != nullptr) {
                    label->setParent(nullptr);
                }
                if (field->parent() != nullptr) {
                    field->setParent(nullptr);
                }
            }
            };

        ensurePairInLayout(m_lblDirectionMode, m_cmbDirectionMode, !isScalarForce, m_foldDirection);
        // 标量力矩不用矢量方向定义区（轴由两点连线或固定轴决定），其余类型保持可见
        m_foldDirection->setVisible(!isScalarTorque && !scalarForceTwoPoint);

        // 基本体/基本点（标量力/标量力矩共用，与运动副面板的移除/插回模式一致）
        // 先脱离旧 parent，再插回到折叠区顶部（端点组保持在参数组之前）
        m_lblBaseBodyJ->setParent(nullptr);
        m_cmbBodyJ->setParent(nullptr);
        m_basePointPicker->setParent(nullptr);
        if (isScalarForce) {
            m_foldScalarForce->insertRow(0, m_lblBaseBodyJ, m_cmbBodyJ);
            m_foldScalarForce->insertRow(1, m_basePointPicker);
            QTimer::singleShot(0, m_foldScalarForce, [this]() { m_foldScalarForce->refreshLayout(); });
        }
        else if (isScalarTorque) {
            m_foldScalarTorque->insertRow(0, m_lblBaseBodyJ, m_cmbBodyJ);
            m_foldScalarTorque->insertRow(1, m_basePointPicker);
            QTimer::singleShot(0, m_foldScalarTorque, [this]() { m_foldScalarTorque->refreshLayout(); });
        }

        // 三点力/三点力矩：应用体/应用点搬进三点折叠区顶部，方向定义区隐藏（方向/轴由出发点→目标点连线决定）
        const bool isThreePoint = (loadType == VectorLoadType::TriForce || loadType == VectorLoadType::TriTorque);
        m_lblThreePointMagnitude->setText(loadType == VectorLoadType::TriTorque ? "力矩大小 (N·mm)" : "载荷大小 (N)");
        {
            // 作用体/作用点平时挂在 m_foldTarget；三点力时移入专属区保持就近展示
            if (isThreePoint) {
                if (m_cmbBody->parent() == m_foldTarget || m_actionPointPicker->parent() == m_foldTarget) {
                    m_cmbBody->setParent(nullptr);
                    m_actionPointPicker->setParent(nullptr);
                }
                if (m_cmbBody->parent() == nullptr)
                    m_foldThreePointForce->insertRow(0, new QLabel("应用体"), m_cmbBody);
                if (m_actionPointPicker->parent() == nullptr)
                    m_foldThreePointForce->insertRow(1, m_actionPointPicker);
            }
            else if (m_cmbBody->parent() == nullptr || m_actionPointPicker->parent() == nullptr) {
                m_cmbBody->setParent(nullptr);
                m_actionPointPicker->setParent(nullptr);
                m_foldTarget->addRow(new QLabel("作用体"), m_cmbBody);
                m_foldTarget->addRow(m_actionPointPicker);
            }
            m_foldThreePointForce->setVisible(isThreePoint);
            m_foldDirection->setVisible(!isScalarTorque && !isThreePoint && !scalarForceTwoPoint);
            m_foldMagnitude->setVisible(!isScalarForce && !isScalarTorque && !isBushing && !isThreePoint);
            // 输入模式联动：常量显示载荷大小，函数显示表达式构造器
            const bool useFunc = m_cmbThreePointInputMode->currentIndex() == 1;
            if (isThreePoint) {
                if (useFunc) {
                    if (m_threePointFuncBuilder->parent() == nullptr)
                        m_foldThreePointForce->addRow(m_threePointFuncBuilder);
                    m_lblThreePointMagnitude->setParent(nullptr);
                    m_spbThreePointMagnitude->setParent(nullptr);
                }
                else {
                    if (m_lblThreePointMagnitude->parent() == nullptr || m_spbThreePointMagnitude->parent() == nullptr) {
                        m_foldThreePointForce->addRow(m_lblThreePointMagnitude, m_spbThreePointMagnitude);
                    }
                    m_threePointFuncBuilder->setParent(nullptr);
                }
            }
            else {
                m_lblThreePointMagnitude->setParent(nullptr);
                m_spbThreePointMagnitude->setParent(nullptr);
                m_threePointFuncBuilder->setParent(nullptr);
            }
            if (isThreePoint)
                QTimer::singleShot(0, m_foldThreePointForce, [this]() { m_foldThreePointForce->refreshLayout(); });
        }

        // 与运动副面板一致：类型变化会影响多个折叠区的高度，统一刷新
        m_foldTarget->refreshLayout();
        m_foldDirection->refreshLayout();
        m_foldMagnitude->refreshLayout();
        m_foldScalarForce->refreshLayout();
        m_foldScalarTorque->refreshLayout();
        m_foldThreePointForce->refreshLayout();
        m_foldBaseBody->refreshLayout();
        m_foldStiffness->refreshLayout();
        m_foldDamping->refreshLayout();
        m_foldPreload->refreshLayout();
    }

    void mMotionVectorForcePanel::updateUIFromData()
    {
        if (!getCVM || !m_forceElementData)
            return;

        refreshBodyList();

        // 1. 还原载荷类型并按类型刷新布局（下拉索引经 comboTypeTable 映射，衬套为常规可选项）
        const QSignalBlocker bType(m_cmbLoadType);
        const auto loadType = m_forceElementData->getLoadType();
        m_cmbLoadType->setCurrentIndex(comboTypeTable().indexOf(loadType));
        refreshLayoutByType(loadType == VectorLoadType::Bushing);

        if (auto* scalar = scalarData()) {
            int bodyIndex = m_cmbBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
            const QSignalBlocker bBody(m_cmbBody);
            m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);
            m_actionPointPicker->setCoordinate(scalar->getLocalI());
            restoreDirectionSelector(m_directionSelector, scalar->dirPickState_, scalar->getFixedDirection());

            int bodyJIndex = m_cmbBodyJ->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ));
            const QSignalBlocker bBodyJ(m_cmbBodyJ);
            m_cmbBodyJ->setCurrentIndex(bodyJIndex != -1 ? bodyJIndex : 0);
            m_basePointPicker->setCoordinate(scalar->getLocalJ());

            const QSignalBlocker bDir(m_cmbScalarDirectionMode);
            m_cmbScalarDirectionMode->setCurrentIndex(static_cast<int>(scalar->getDirectionMode()));
            const QSignalBlocker bMag(m_spbScalarMagnitude);
            m_spbScalarMagnitude->setExpression(QString::number(scalar->getMagnitude()));
            m_forceFuncBuilder->setExpression(scalar->getForceFunction());
            // 按函数有无还原输入模式
            {
                const QSignalBlocker bMode(m_cmbScalarInputMode);
                const bool hasFn = !scalar->getForceFunction().isEmpty();
                m_cmbScalarInputMode->setCurrentIndex(hasFn ? 1 : 0);
                m_lblScalarMagnitude->setParent(nullptr);
                m_spbScalarMagnitude->setParent(nullptr);
                m_forceFuncBuilder->setParent(nullptr);
                int row = -1;
                QFormLayout::ItemRole role;
                m_foldScalarForce->layout()->getWidgetPosition(m_cmbScalarInputMode, &row, &role);
                if (row >= 0) {
                    if (hasFn) {
                        m_foldScalarForce->insertRow(row + 1, m_forceFuncBuilder);
                    }
                    else {
                        m_foldScalarForce->insertRow(row + 1, m_lblScalarMagnitude, m_spbScalarMagnitude);
                    }
                }
            }
            QTimer::singleShot(0, m_foldScalarForce, [this]() { m_foldScalarForce->refreshLayout(); });
            const QSignalBlocker bOnly(m_chkActionOnly);
            m_chkActionOnly->setChecked(scalar->getActionOnly());
            updateRenderFromData();
            return;
        }
        else if (auto* st = scalarTorqueData()) {
            int bodyIndex = m_cmbBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
            const QSignalBlocker bBody(m_cmbBody);
            m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);
            m_actionPointPicker->setCoordinate(st->getLocalI());
            int bodyJIndex = m_cmbBodyJ->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyJ));
            const QSignalBlocker bBodyJ(m_cmbBodyJ);
            m_cmbBodyJ->setCurrentIndex(bodyJIndex != -1 ? bodyJIndex : 0);
            m_basePointPicker->setCoordinate(st->getLocalJ());

            const QSignalBlocker bAxis(m_cmbTorqueAxisMode);
            m_cmbTorqueAxisMode->setCurrentIndex(static_cast<int>(st->getAxisMode()));
            if (static_cast<int>(st->getAxisMode()) != 0) {
                if (m_axisDirSelector->parent() == nullptr) {
                    int row = -1;
                    QFormLayout::ItemRole role;
                    m_foldScalarTorque->layout()->getWidgetPosition(m_cmbTorqueAxisMode, &row, &role);
                    if (row >= 0) {
                        m_foldScalarTorque->insertRow(row + 1, m_axisDirSelector);
                    }
                }
            }
            else {
                if (m_axisDirSelector->parent() != nullptr) {
                    m_axisDirSelector->setParent(nullptr);
                }
            }
            QTimer::singleShot(0, m_foldScalarTorque, [this]() { m_foldScalarTorque->refreshLayout(); });
            restoreDirectionSelector(m_axisDirSelector, st->fixedAxisPickState_, st->getFixedAxis());
            m_spbTorqueMagnitude->setExpression(QString::number(st->getMagnitude()));
            m_torqueFuncBuilder->setExpression(st->getTorqueFunction());
            // 按函数有无还原输入模式
            {
                const QSignalBlocker bMode(m_cmbTorqueInputMode);
                const bool hasFn = !st->getTorqueFunction().isEmpty();
                m_cmbTorqueInputMode->setCurrentIndex(hasFn ? 1 : 0);
                m_lblTorqueMagnitude->setParent(nullptr);
                m_spbTorqueMagnitude->setParent(nullptr);
                m_torqueFuncBuilder->setParent(nullptr);
                int row = -1;
                QFormLayout::ItemRole role;
                m_foldScalarTorque->layout()->getWidgetPosition(m_cmbTorqueInputMode, &row, &role);
                if (row >= 0) {
                    if (hasFn) {
                        m_foldScalarTorque->insertRow(row + 1, m_torqueFuncBuilder);
                    }
                    else {
                        m_foldScalarTorque->insertRow(row + 1, m_lblTorqueMagnitude, m_spbTorqueMagnitude);
                    }
                }
            }
            QTimer::singleShot(0, m_foldScalarTorque, [this]() { m_foldScalarTorque->refreshLayout(); });
            const QSignalBlocker bOnly(m_chkTorqueActionOnly);
            m_chkTorqueActionOnly->setChecked(st->getActionOnly());
            updateRenderFromData();
            return;
        }

        if (auto* tt = threePointTorqueData()) {
            // 应用体/应用点（复用 m_cmbBody / m_actionPointPicker）
            int bodyIndex = m_cmbBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
            const QSignalBlocker bBody(m_cmbBody);
            m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);
            m_actionPointPicker->setCoordinate(tt->getLocalI());

            // 出发体/出发点
            int originIndex = m_cmbOriginBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionActionBody));
            const QSignalBlocker bOrigin(m_cmbOriginBody);
            m_cmbOriginBody->setCurrentIndex(originIndex != -1 ? originIndex : 0);
            m_originPointPicker->setCoordinate(tt->getLocalOrigin());

            // 目标体/目标点
            int targetIndex = m_cmbTargetBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody));
            const QSignalBlocker bTarget(m_cmbTargetBody);
            m_cmbTargetBody->setCurrentIndex(targetIndex != -1 ? targetIndex : 0);
            m_targetPointPicker->setCoordinate(tt->getLocalTarget());

            // 幅值/函数与输入模式
            m_spbThreePointMagnitude->setExpression(QString::number(tt->getMagnitude()));
            m_threePointFuncBuilder->setExpression(tt->getTorqueFunction());
            {
                const QSignalBlocker bMode(m_cmbThreePointInputMode);
                m_cmbThreePointInputMode->setCurrentIndex(tt->getTorqueFunction().isEmpty() ? 0 : 1);
            }
            updateRenderFromData();
            return;
        }

        if (auto* tp = threePointData()) {
            // 应用体/应用点（复用 m_cmbBody / m_actionPointPicker）
            int bodyIndex = m_cmbBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBodyI));
            const QSignalBlocker bBody(m_cmbBody);
            m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);
            m_actionPointPicker->setCoordinate(tp->getLocalI());

            // 出发体/出发点
            int originIndex = m_cmbOriginBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionActionBody));
            const QSignalBlocker bOrigin(m_cmbOriginBody);
            m_cmbOriginBody->setCurrentIndex(originIndex != -1 ? originIndex : 0);
            m_originPointPicker->setCoordinate(tp->getLocalOrigin());

            // 目标体/目标点
            int targetIndex = m_cmbTargetBody->findData(mMotionRefMap::get(Physical_MotionVectorForce, m_forceElementData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody));
            const QSignalBlocker bTarget(m_cmbTargetBody);
            m_cmbTargetBody->setCurrentIndex(targetIndex != -1 ? targetIndex : 0);
            m_targetPointPicker->setCoordinate(tp->getLocalTarget());

            // 幅值/函数与输入模式
            m_spbThreePointMagnitude->setExpression(QString::number(tp->getMagnitude()));
            m_threePointFuncBuilder->setExpression(tp->getForceFunction());
            {
                const QSignalBlocker bMode(m_cmbThreePointInputMode);
                m_cmbThreePointInputMode->setCurrentIndex(tp->getForceFunction().isEmpty() ? 0 : 1);
            }
            updateRenderFromData();
            return;
        }

        auto* data = vectorData();
        if (!data) return;

        // 2. 还原作用体（marker_I 侧）
        int bodyId = mMotionRefMap::get(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
        int bodyIndex = m_cmbBody->findData(bodyId);
        const QSignalBlocker bBody(m_cmbBody);
        m_cmbBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);

        // 3. 还原作用点（marker_I 原点）
        m_actionPointPicker->setCoordinate(data->getActionPoint());

        // 4. 还原方向（marker_I 定向）
        restoreDirectionSelector(m_directionSelector, data->dirPickState_, data->getDirection());

        // 5. 还原方向模式
        const QSignalBlocker bDirMode(m_cmbDirectionMode);
        m_cmbDirectionMode->setCurrentIndex(static_cast<int>(data->getDirectionMode()));

        // 6. 还原大小模式与数值（常量/表达式输入框动态插拔，不进布局的先脱离）
        MagnitudeMode magMode = data->getMagnitudeMode();
        const QSignalBlocker bMagMode(m_cmbMagnitudeMode);
        m_lblMagnitude->setParent(nullptr);
        m_spbMagnitude->setParent(nullptr);
        m_exprBuilder->setParent(nullptr);
        {
            int row = -1;
            QFormLayout::ItemRole role;
            m_foldMagnitude->layout()->getWidgetPosition(m_cmbMagnitudeMode, &row, &role);
            if (row >= 0) {
                if (magMode == MagnitudeMode::Constant) {
                    m_cmbMagnitudeMode->setCurrentIndex(0);
                    m_foldMagnitude->insertRow(row + 1, m_lblMagnitude, m_spbMagnitude);
                    const QSignalBlocker bMag(m_spbMagnitude);
                    m_spbMagnitude->setExpression(QString::number(data->getMagnitude()));
                }
                else {
                    m_cmbMagnitudeMode->setCurrentIndex(1);
                    m_foldMagnitude->insertRow(row + 1, m_exprBuilder);
                    m_exprBuilder->setExpression(data->getExpression());
                }
            }
        }
        QTimer::singleShot(0, m_foldMagnitude, [this]() { m_foldMagnitude->refreshLayout(); });

        // 7. 还原轴套专属字段
        if (auto* bushing = bushingData()) {
            // 刚度 / 阻尼
            QVector<double> k = bushing->getStiffness();
            if (k.size() == 6) {
                const QSignalBlocker bkxx(m_spbKxx); m_spbKxx->setExpression(QString::number(k[0]));
                const QSignalBlocker bkyy(m_spbKyy); m_spbKyy->setExpression(QString::number(k[1]));
                const QSignalBlocker bkzz(m_spbKzz); m_spbKzz->setExpression(QString::number(k[2]));
                const QSignalBlocker bkrx(m_spbKrx); m_spbKrx->setExpression(QString::number(k[3]));
                const QSignalBlocker bkry(m_spbKry); m_spbKry->setExpression(QString::number(k[4]));
                const QSignalBlocker bkrz(m_spbKrz); m_spbKrz->setExpression(QString::number(k[5]));
            }
            QVector<double> c = bushing->getDamping();
            if (c.size() == 6) {
                const QSignalBlocker bcxx(m_spbCxx); m_spbCxx->setExpression(QString::number(c[0]));
                const QSignalBlocker bcyy(m_spbCyy); m_spbCyy->setExpression(QString::number(c[1]));
                const QSignalBlocker bczz(m_spbCzz); m_spbCzz->setExpression(QString::number(c[2]));
                const QSignalBlocker bcrx(m_spbCrx); m_spbCrx->setExpression(QString::number(c[3]));
                const QSignalBlocker bcry(m_spbCry); m_spbCry->setExpression(QString::number(c[4]));
                const QSignalBlocker bcrz(m_spbCrz); m_spbCrz->setExpression(QString::number(c[5]));
            }

            // 基本体（marker_J）
            int baseBodyId = mMotionRefMap::get(Physical_MotionVectorForce, bushing->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
            int baseBodyIndex = m_cmbBaseBody->findData(baseBodyId);
            const QSignalBlocker bBaseBody(m_cmbBaseBody);
            m_cmbBaseBody->setCurrentIndex(baseBodyIndex != -1 ? baseBodyIndex : 0);

            // 偏置（应用偏置 + x/y/z）
            const bool applyOffset = bushing->getApplyOffset();
            {
                const QSignalBlocker bOff(m_cmbBaseOffset);
                m_cmbBaseOffset->setCurrentIndex(applyOffset ? 1 : 0);
            }
            m_spbOffsetX->setEnabled(applyOffset);
            m_spbOffsetY->setEnabled(applyOffset);
            m_spbOffsetZ->setEnabled(applyOffset);
            QVector3D offset = bushing->getOffset();
            {
                const QSignalBlocker bx(m_spbOffsetX); m_spbOffsetX->setExpression(QString::number(offset.x()));
                const QSignalBlocker by(m_spbOffsetY); m_spbOffsetY->setExpression(QString::number(offset.y()));
                const QSignalBlocker bz(m_spbOffsetZ); m_spbOffsetZ->setExpression(QString::number(offset.z()));
            }

            // 预载
            QVector<double> f0 = bushing->getPreloadForce();
            if (f0.size() == 3) {
                const QSignalBlocker bf0x(m_spbF0x); m_spbF0x->setExpression(QString::number(f0[0]));
                const QSignalBlocker bf0y(m_spbF0y); m_spbF0y->setExpression(QString::number(f0[1]));
                const QSignalBlocker bf0z(m_spbF0z); m_spbF0z->setExpression(QString::number(f0[2]));
            }
            QVector<double> t0 = bushing->getPreloadTorque();
            if (t0.size() == 3) {
                const QSignalBlocker bt0x(m_spbT0x); m_spbT0x->setExpression(QString::number(t0[0]));
                const QSignalBlocker bt0y(m_spbT0y); m_spbT0y->setExpression(QString::number(t0[1]));
                const QSignalBlocker bt0z(m_spbT0z); m_spbT0z->setExpression(QString::number(t0[2]));
            }
        }

        // 同步渲染
        updateRenderFromData();
    }

    // =====================================================================
    // 渲染
    // =====================================================================

    void mMotionVectorForcePanel::updateRenderFromData()
    {
        if (!getCVM || !getCVM->getPreRenderWidget() || !m_forceElementData)
            return;

        // 跨类型切换时统一清除所有可能的高亮、点和旧符号（防止残留导致渲染重叠）
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();

            highlightBody(-1, "scalarForceBodyI", QColor());
            highlightBody(-1, "scalarForceBodyJ", QColor());
            highlightBody(-1, "scalarTorqueBodyI", QColor());
            highlightBody(-1, "scalarTorqueBodyJ", QColor());
            highlightBody(-1, "vectorForceBodyI", QColor());
            highlightBody(-1, "vectorForceBodyJ", QColor());
            highlightBody(-1, "threePointForceBodyI", QColor());
            highlightBody(-1, "threePointForceBodyO", QColor());
            highlightBody(-1, "threePointForceBodyT", QColor());

            if (commonShapeRender) {
                commonShapeRender->deletePoint("scalarForcePointI");
                commonShapeRender->deletePoint("scalarForcePointJ");
                commonShapeRender->deletePoint("scalarTorquePointI");
                commonShapeRender->deletePoint("scalarTorquePointJ");
                commonShapeRender->deletePoint("vectorForcePoint");
                commonShapeRender->deletePoint("threePointForcePointI");
                commonShapeRender->deletePoint("threePointForcePointO");
                commonShapeRender->deletePoint("threePointForcePointT");
                commonShapeRender->deleteCylinder("threePointForceDir");
            }

            if (symbolController && !m_lastSymbolName.isEmpty()) {
                symbolController->deleteSymbol({ m_lastSymbolName });
                m_lastSymbolName.clear();
            }
        }

        // 三点力矩：应用体高亮 + 应用点橙球 + 出发点粉球 + 目标点青球 + 两点间绿色轴线 + 应用点力矩符号
        if (auto* tt = threePointTorqueData()) {
            const QString symbol = QString("threePointTorque_%1").arg(tt->getId());
            m_lastSymbolName = symbol;
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tt->getId(), Physical_MotionBody, MapRelation_MotionBodyI), "threePointForceBodyI", mMotionPanelColors::bodyA());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tt->getId(), Physical_MotionBody, MapRelation_MotionActionBody), "threePointForceBodyO", mMotionPanelColors::bodyB());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tt->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), "threePointForceBodyT", mMotionPanelColors::bodyB());
            commonShapeRender->deletePoint("threePointForcePointI");
            commonShapeRender->deletePoint("threePointForcePointO");
            commonShapeRender->deletePoint("threePointForcePointT");
            commonShapeRender->deleteCylinder("threePointForceDir");
            commonShapeRender->appendPoint("threePointForcePointI", { tt->getLocalI() },
                mMotionPanelColors::vec(mMotionPanelColors::pointA()));
            commonShapeRender->appendPoint("threePointForcePointO", { tt->getLocalOrigin() },
                mMotionPanelColors::vec(mMotionPanelColors::pointB()));
            commonShapeRender->appendPoint("threePointForcePointT", { tt->getLocalTarget() },
                mMotionPanelColors::vec(mMotionPanelColors::pointC()));

            QVector3D axis = tt->getLocalTarget() - tt->getLocalOrigin();
            if (axis.length() < 1e-6) axis = QVector3D(1, 0, 0);
            const QVector3D mid = tt->getLocalOrigin() + axis / 2.0;
            const QVector3D green = mMotionPanelColors::vec(mMotionPanelColors::direction());
            const float cylRadius = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget()) * 0.004f;
            commonShapeRender->appendCylinder("threePointForceDir", mid, axis / 2.0, cylRadius,
                QVector4D(green.x(), green.y(), green.z(), 1.0f));

            symbolController->deleteSymbol({ symbol });
            const float sceneScale = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget());
            mMotionVectorSymbolHelper::buildVectorTorqueSymbol(
                symbolController, symbol, tt->getLocalI(), axis, sceneScale);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        // 三点力：应用体高亮 + 应用点橙球 + 出发点粉球 + 目标点青球 + 两点间绿色方向柱
        if (auto* tp = threePointData()) {
            const QString symbol = QString("threePointForce_%1").arg(tp->getId());
            m_lastSymbolName = symbol;
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tp->getId(), Physical_MotionBody, MapRelation_MotionBodyI), "threePointForceBodyI", mMotionPanelColors::bodyA());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tp->getId(), Physical_MotionBody, MapRelation_MotionActionBody), "threePointForceBodyO", mMotionPanelColors::bodyB());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, tp->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), "threePointForceBodyT", mMotionPanelColors::bodyB());
            commonShapeRender->deletePoint("threePointForcePointI");
            commonShapeRender->deletePoint("threePointForcePointO");
            commonShapeRender->deletePoint("threePointForcePointT");
            commonShapeRender->deleteCylinder("threePointForceDir");
            commonShapeRender->appendPoint("threePointForcePointI", { tp->getLocalI() },
                mMotionPanelColors::vec(mMotionPanelColors::pointA()));
            commonShapeRender->appendPoint("threePointForcePointO", { tp->getLocalOrigin() },
                mMotionPanelColors::vec(mMotionPanelColors::pointB()));
            commonShapeRender->appendPoint("threePointForcePointT", { tp->getLocalTarget() },
                mMotionPanelColors::vec(mMotionPanelColors::pointC()));

            QVector3D dir = tp->getLocalTarget() - tp->getLocalOrigin();
            if (dir.length() < 1e-6) dir = QVector3D(1, 0, 0);
            const QVector3D mid = tp->getLocalOrigin() + dir / 2.0;
            const QVector3D green = mMotionPanelColors::vec(mMotionPanelColors::direction());
            const float cylRadius = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget()) * 0.004f;
            commonShapeRender->appendCylinder("threePointForceDir", mid, dir / 2.0, cylRadius,
                QVector4D(green.x(), green.y(), green.z(), 1.0f));

            symbolController->deleteSymbol({ symbol });
            const float sceneScale = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget());
            mMotionVectorSymbolHelper::buildScalarForceSymbol(
                symbolController, symbol, tp->getLocalI(), dir, sceneScale);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        if (auto* scalar = scalarData()) {
            const QString symbol = QString("scalarForce_%1").arg(scalar->getId());
            m_lastSymbolName = symbol;
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, scalar->getId(), Physical_MotionBody, MapRelation_MotionBodyI), "scalarForceBodyI", mMotionPanelColors::bodyA());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, scalar->getId(), Physical_MotionBody, MapRelation_MotionBodyJ), "scalarForceBodyJ", mMotionPanelColors::bodyB());
            commonShapeRender->deletePoint("scalarForcePointI");
            commonShapeRender->deletePoint("scalarForcePointJ");
            commonShapeRender->appendPoint("scalarForcePointI", { scalar->getLocalI() },
                mMotionPanelColors::vec(mMotionPanelColors::pointA()));
            commonShapeRender->appendPoint("scalarForcePointJ", { scalar->getLocalJ() },
                mMotionPanelColors::vec(mMotionPanelColors::pointB()));

            if (mMotionRefMap::get(Physical_MotionVectorForce, scalar->getId(), Physical_MotionBody, MapRelation_MotionBodyI) == -1 || mMotionRefMap::get(Physical_MotionVectorForce, scalar->getId(), Physical_MotionBody, MapRelation_MotionBodyJ) == -1) {
                mMotionVectorSymbolHelper::clearVectorSymbol(symbolController, symbol);
                getCVM->getPreRenderWidget()->updatePaint();
                return;
            }

            QVector3D dir = scalar->getLocalJ() - scalar->getLocalI();
            if (dir.length() < 1e-12) dir = QVector3D(1, 0, 0);
            symbolController->deleteSymbol({ symbol });
            const float sceneScale = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget());
            mMotionVectorSymbolHelper::buildScalarForceSymbol(
                symbolController, symbol, scalar->getLocalI(), dir, sceneScale);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        // 标量力矩：体高亮 + 两点渲染 + 力矩符号
        if (auto* st = scalarTorqueData()) {
            const QString symbol = QString("scalarTorque_%1").arg(st->getId());
            m_lastSymbolName = symbol;
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyI), "scalarTorqueBodyI", mMotionPanelColors::bodyA());
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyJ), "scalarTorqueBodyJ", mMotionPanelColors::bodyB());
            commonShapeRender->deletePoint("scalarTorquePointI");
            commonShapeRender->deletePoint("scalarTorquePointJ");
            commonShapeRender->appendPoint("scalarTorquePointI", { st->getLocalI() },
                mMotionPanelColors::vec(mMotionPanelColors::pointA()));
            commonShapeRender->appendPoint("scalarTorquePointJ", { st->getLocalJ() },
                mMotionPanelColors::vec(mMotionPanelColors::pointB()));

            if (mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyI) == -1 || mMotionRefMap::get(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyJ) == -1) {
                mMotionVectorSymbolHelper::clearVectorSymbol(symbolController, symbol);
                getCVM->getPreRenderWidget()->updatePaint();
                return;
            }

            // 符号方向：两点连线取 J-I，固定轴模式取固定轴（与求解器轴语义一致）
            QVector3D axis = (st->getAxisMode() == MechMotionData::ScalarTorqueAxisMode::Line)
                ? (st->getLocalJ() - st->getLocalI())
                : st->getFixedAxis();
            if (axis.length() < 1e-12) axis = QVector3D(0, 0, 1);
            symbolController->deleteSymbol({ symbol });
            const float sceneScale = mMotionVectorSymbolHelper::computeSceneScale(getCVM->getPreRenderWidget());
            mMotionVectorSymbolHelper::buildVectorTorqueSymbol(
                symbolController, symbol, st->getLocalI(), axis, sceneScale);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        auto* data = vectorData();
        if (!data) return;
        int bodyId = mMotionRefMap::get(Physical_MotionVectorForce, data->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
        QString newsymbol = QString("vectorForce_%1").arg(data->getId());
            m_lastSymbolName = newsymbol;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        auto commonShapeRender = getCVM->getPreRenderWidget()->getCommonShapeRender();

        // 1. 高亮运动体：作用体为红色；轴套时再高亮基本体为蓝色
        highlightBody(bodyId, std::string("vectorForceBodyI"), mMotionPanelColors::bodyA());
        if (auto* bushing = bushingData())
            highlightBody(mMotionRefMap::get(Physical_MotionVectorForce, bushing->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), std::string("vectorForceBodyJ"), mMotionPanelColors::bodyB());
        else
            highlightBody(-1, std::string("vectorForceBodyJ"), QColor()); // 无基本体时清除

        // 2. 显示作用点（marker_I 原点）
        QVector3D actionPt = data->getActionPoint();
        commonShapeRender->appendPoint("vectorForcePoint", { actionPt },
            mMotionPanelColors::vec(mMotionPanelColors::pointA()));

        // 3. 更新力 / 力矩符号
        if (bodyId == -1) {
            mMotionVectorSymbolHelper::clearVectorSymbol(symbolController, newsymbol);
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        QVector3D origin = data->getActionPoint();
        QVector3D dir = data->getDirection();
        if (dir.length() < 1e-12) dir = QVector3D(1, 0, 0);

        // 先删除旧符号，避免同名符号数据类型不一致导致重建失败
        symbolController->deleteSymbol({ newsymbol });

        const float sceneScale = mMotionVectorSymbolHelper::computeSceneScale(
            getCVM->getPreRenderWidget());

        VectorLoadType loadType = data->getLoadType();

        if (loadType == VectorLoadType::Force) {
            mMotionVectorSymbolHelper::buildVectorForceSymbol(
                symbolController, newsymbol, origin, dir, sceneScale);
        }
        else if (loadType == VectorLoadType::Torque) {
            mMotionVectorSymbolHelper::buildVectorTorqueSymbol(
                symbolController, newsymbol, origin, dir, sceneScale);
        }
        else if (loadType == VectorLoadType::Bushing) {
            mMotionVectorSymbolHelper::buildBushingSymbol(
                symbolController, newsymbol, origin, dir, sceneScale);
        }
        symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();

        // 清空拾取状态
    }

    void mMotionVectorForcePanel::highlightBody(int bodyId, const std::string& key, const QColor& color)
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto highlight_controller = getCVM->getPreRenderWidget()->getHighlightController();
        if (!highlight_controller)
            return;
        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(key);
        if (!geoHighLightRender)
            return;

        auto* motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(bodyId);
        if (motionBodyData)
        {
            // 高亮该运动体
            geoHighLightRender->setHighLightColor(color.red(), color.green(), color.blue());
            mxr::mGeoSelectData geoData;
            auto vfHl = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
        geoData.UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(vfHl.begin(), vfHl.end()));
            geoHighLightRender->updateHighLightRender(&geoData, HighStyle::PartALLFeatures);
        }
        else
        {
            // 无有效体时清除该 key 的高亮
            mxr::mGeoSelectData geoData;
            geoData.UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(&geoData);
        }
    }

    void mMotionVectorForcePanel::clearBodyHighlights()
    {
        highlightBody(-1, std::string("vectorForceBodyI"), QColor());
        highlightBody(-1, std::string("vectorForceBodyJ"), QColor());
        highlightBody(-1, std::string("scalarForceBodyI"), QColor());
        highlightBody(-1, std::string("scalarForceBodyJ"), QColor());
    }

    // =====================================================================
    // 拾取信号连接 / 断开
    // =====================================================================

    void mMotionVectorForcePanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionVectorForcePanel::slot_finishedPick);
    }

    void mMotionVectorForcePanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionVectorForcePanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        // 重置面板，类型根据拾取模式来确定
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionVectorForcePanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
    }

}
