

#include "mMotionJointPanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mGeneralPickUp.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionJointSymbolHelper.h"
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
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointManager.h"
#include "mModelData.h"

//Geo
#include "mGeometricFeatureAttribute.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QtMath>
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

    mMotionJointPanel::mMotionJointPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionJointData = nullptr;

        //获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);


        auto makePickBtn = [](const QString& iconPath, const QString& tip) {
            QToolButton* b = new QToolButton();
            b->setIcon(QIcon(iconPath));
            b->setToolTip(tip);
            b->setFixedWidth(30);
            return b;
            };

        // 1. 类型
        FoldSection* foldType = new FoldSection("类型", this);
        m_foldAction = new FoldSection("动作", this);
        m_foldBase = new FoldSection("基本", this);
        m_cmbJointType = new QComboBox();
        // 使用 addItem(text, userData) 绑定枚举值，不再依赖索引顺序
        auto addJointTypeItem = [this](const QString& text, JointType type) {
            m_cmbJointType->addItem(text, static_cast<int>(type));
            };
        addJointTypeItem("旋转副", JointType::Revolute);
        addJointTypeItem("移动副", JointType::Translational);
        addJointTypeItem("柱面副", JointType::Cylindrical);
        addJointTypeItem("球面副", JointType::Spherical);
        addJointTypeItem("固定副", JointType::Fixed);
        addJointTypeItem("万向节", JointType::Universal);
        addJointTypeItem("点面副", JointType::PointPlane);
        addJointTypeItem("平行约束", JointType::Parallel);
        addJointTypeItem("垂直约束", JointType::Perpendicular);
        addJointTypeItem("方向约束", JointType::Direction);
        addJointTypeItem("恒速副", JointType::ConstantVelocity);
        //addJointTypeItem("胡克副", JointType::Hooke);
        addJointTypeItem("螺纹副", JointType::Screw);
        addJointTypeItem("平面副", JointType::Planar);
        addJointTypeItem("点线副", JointType::PointLine);
        addJointTypeItem("点在线上副", JointType::PointOnCurve);
        addJointTypeItem("点在面上副", JointType::PointOnSurface);
        addJointTypeItem("线在线上副", JointType::CurveOnCurve);
        foldType->addRow(new QLabel("类型:"), m_cmbJointType);
        m_mainLayout->addWidget(foldType);

        // 2. 动作实体及坐标系定义 (唯一真相来源)
        // 【修改布局】：ComboBox 和 Button 放同一行
        QHBoxLayout* layActionBody = new QHBoxLayout();
        layActionBody->setContentsMargins(0, 0, 0, 0);
        m_cmbActionBody = new QComboBox();
        //m_btnPickAction = makePickBtn(":/mMainWidgets/Icon/tihuan3x.png", "在视图中拾取");
        layActionBody->addWidget(m_cmbActionBody);
        //layActionBody->addWidget(m_btnPickAction);
        m_foldAction->addRow(new QLabel("选择运动体(1):"), layActionBody);

        // 方向控件统一工厂：矢量类输入（指定矢量/万向节四矢量/垂直约束双矢量）
        auto makeDirSelector = [this](const QString& label) {
            mDirectionSelectorWidget* selector = new mDirectionSelectorWidget(label, false, QString(), false, QString(), false, this);
            selector->setParent(nullptr);
            connect(selector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
                if (m_foldAction) m_foldAction->refreshLayout();
                if (m_foldBase) m_foldBase->refreshLayout();
                });
            return selector;
            };

        m_originPicker = new mPointPickerWidget(QStringLiteral("指定原点"), this);
        m_originPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        m_foldAction->addRow(m_originPicker);

        m_axisDirSelector = makeDirSelector(QStringLiteral("指定矢量"));
        m_foldAction->addRow(m_axisDirSelector);

        // 万向节两个矢量（先创建但不加入布局，按类型动态插入）
        m_actionSpinDirSelector = makeDirSelector(QStringLiteral("自旋矢量"));
        m_actionPinDirSelector = makeDirSelector(QStringLiteral("十字销矢量"));

        // 垂直约束运动体矢量（先创建但不加入布局，按类型动态插入）
        m_actionVecDirSelector = makeDirSelector(QStringLiteral("运动体矢量"));

        m_mainLayout->addWidget(m_foldAction);

        // 3. 基本实体 (砍掉冗余坐标，仅保留拾取)
        // 【修改布局】：ComboBox 和 Button 放同一行
        QHBoxLayout* layBaseBody = new QHBoxLayout();
        layBaseBody->setContentsMargins(0, 0, 0, 0);
        m_cmbBaseBody = new QComboBox();
        //m_btnPickBase = makePickBtn(":/mMainWidgets/Icon/tihuan3x.png", "在视图中拾取");
        layBaseBody->addWidget(m_cmbBaseBody);
        //layBaseBody->addWidget(m_btnPickBase);
        m_foldBase->addRow(new QLabel("选择运动体(2):"), m_cmbBaseBody);

        // 万向节基本体两个矢量（先创建但不加入布局，按类型动态插入）
        m_baseSpinDirSelector = makeDirSelector(QStringLiteral("自旋矢量"));
        m_basePinDirSelector = makeDirSelector(QStringLiteral("十字销矢量"));

        // 垂直约束基本体矢量（先创建但不加入布局，按类型动态插入）
        m_baseVecDirSelector = makeDirSelector(QStringLiteral("基本体矢量"));

        m_mainLayout->addWidget(m_foldBase);

        // 3.5 点在面上副：约束曲面（几何面多选拾取控件，激活后在视口点选/框选面即可）
        m_foldSurface = new FoldSection("约束曲面", this);
        m_surfaceFaceController = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, this);
        m_surfaceFaceController->setDefultTip(QStringLiteral("请选择"), QStringLiteral("约束曲面"));
        m_surfaceFaceController->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType> {
            return { MxFunctions::PickDataType::GeoFace };
            });
        m_surfaceFaceController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_surfaceFaceController->widget()->setFixedHeight(30);
        m_foldSurface->addRow(m_surfaceFaceController->widget());
        m_lblFaceCount = new QLabel("未选择面（用基本体整体网格）");
        m_foldSurface->addRow(new QLabel("已选约束面:"), m_lblFaceCount);
        m_mainLayout->addWidget(m_foldSurface);
        m_foldSurface->setVisible(false);

        // 3.6 点在线上副：约束曲线（几何边多选拾取控件，激活后在视口点选边即可；
        // 多段共端点的边求解装配时自动串接成一条折线）
        m_foldCurve = new FoldSection("约束曲线", this);
        m_curveEdgeController = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, this);
        m_curveEdgeController->setDefultTip(QStringLiteral("请选择"), QStringLiteral("约束曲线"));
        m_curveEdgeController->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType> {
            return { MxFunctions::PickDataType::GeoEdge };
            });
        m_curveEdgeController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_curveEdgeController->widget()->setFixedHeight(30);
        m_foldCurve->addRow(m_curveEdgeController->widget());
        m_lblEdgeCount = new QLabel("未选择边");
        m_foldCurve->addRow(new QLabel("已选约束边:"), m_lblEdgeCount);
        m_mainLayout->addWidget(m_foldCurve);
        m_foldCurve->setVisible(false);

        // 3.7 线在线上副：约束曲线I（动作体侧，几何边多选拾取控件）
        m_foldCurveI = new FoldSection("约束曲线（动作体）", this);
        m_curveIEdgeController = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, this);
        m_curveIEdgeController->setDefultTip(QStringLiteral("请选择"), QStringLiteral("约束曲线（动作体）"));
        m_curveIEdgeController->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType> {
            return { MxFunctions::PickDataType::GeoEdge };
            });
        m_curveIEdgeController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_curveIEdgeController->widget()->setFixedHeight(30);
        m_foldCurveI->addRow(m_curveIEdgeController->widget());
        m_lblEdgeICount = new QLabel("未选择边");
        m_foldCurveI->addRow(new QLabel("已选约束边:"), m_lblEdgeICount);
        m_mainLayout->addWidget(m_foldCurveI);
        m_foldCurveI->setVisible(false);

        // 3.8 线在线上副：约束曲线J（基本体侧，几何边多选拾取控件）
        m_foldCurveJ = new FoldSection("约束曲线（基本体）", this);
        m_curveJEdgeController = mGeneralPickUp::createDirectionPickWidgetPickMore(CBNone, this);
        m_curveJEdgeController->setDefultTip(QStringLiteral("请选择"), QStringLiteral("约束曲线（基本体）"));
        m_curveJEdgeController->setAllowedTypes([]() -> QSet<MxFunctions::PickDataType> {
            return { MxFunctions::PickDataType::GeoEdge };
            });
        m_curveJEdgeController->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_curveJEdgeController->widget()->setFixedHeight(30);
        m_foldCurveJ->addRow(m_curveJEdgeController->widget());
        m_lblEdgeJCount = new QLabel("未选择边");
        m_foldCurveJ->addRow(new QLabel("已选约束边:"), m_lblEdgeJCount);
        // 允许滑动（UG Allow Sliding）：勾选=相切+切向滑移；不勾=无滑滚动（默认）
        m_chkAllowSliding = new QCheckBox(QStringLiteral("允许滑动"));
        m_chkAllowSliding->setToolTip(QStringLiteral("勾选：两曲线相切接触、允许沿切向滑动；不勾（默认）：无滑滚动——接触点切向速度锁为零。"));
        m_foldCurveJ->addRow(m_chkAllowSliding);
        m_mainLayout->addWidget(m_foldCurveJ);
        m_foldCurveJ->setVisible(false);
        connect(m_chkAllowSliding, &QCheckBox::toggled, this, [this](bool checked) {
            if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
                cvcv->setAllowSliding(checked);
            }
        });

        // 4. 运动副额外参数（螺纹副导程等）
        m_foldJointParams = new FoldSection("运动副参数", this);
        m_lblLead = new QLabel("螺纹导程 (mm):");
        // 默认 1mm/转（mm·kg·s 口径；旧默认 0.01 为米制残留，螺纹几乎不进给）
        m_spbLead = new MxFunctions::mCalcLineEdit(
            "1.0", MxFunctions::mCalcLineEdit::CalcMode::NoEmptyAndPositive, this);
        m_lblLead->setParent(nullptr);
        m_spbLead->setParent(nullptr);
        m_mainLayout->addWidget(m_foldJointParams);

        auto setupCalc = [](MxFunctions::mCalcLineEdit* edit,
            double value,
            MxFunctions::mCalcLineEdit::CalcMode mode) {
                edit->setExpression(QString::number(value));
                if (mode == MxFunctions::mCalcLineEdit::CalcMode::NonNegative) {
                    edit->setNonNegative(true);
                }
            };

        // 5. 摩擦
        m_foldFriction = new FoldSection("摩擦", this);
        m_chkEnableFriction = new QCheckBox("启用摩擦");
        m_foldFriction->addRow(m_chkEnableFriction);

        m_spbStaticMu = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbDynamicMu = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbFrictionRadius = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_lblFrictionRadius = new QLabel("等效摩擦半径 (mm)");

        setupCalc(m_spbStaticMu, 0.3, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbDynamicMu, 0.1, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbFrictionRadius, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);

        m_foldFriction->addRow(new QLabel("静摩擦系数"), m_spbStaticMu);
        m_foldFriction->addRow(new QLabel("动摩擦系数"), m_spbDynamicMu);
        m_foldFriction->addRow(m_lblFrictionRadius, m_spbFrictionRadius);
        m_mainLayout->addWidget(m_foldFriction);

        // 6. 限位（仅旋转副、移动副）
        m_foldLimit = new FoldSection("限位", this);
        m_chkEnableLimit = new QCheckBox("启用限位");
        m_chkEnableLowerLimit = new QCheckBox("启用下限");
        m_chkEnableUpperLimit = new QCheckBox("启用上限");
        m_chkPenaltyOnly = new QCheckBox("仅使用软限位");

        m_spbLowerLimit = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbUpperLimit = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_spbLowerCushion = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbUpperCushion = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbLowerLimitStiffness = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbUpperLimitStiffness = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbLowerLimitDamping = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);
        m_spbUpperLimitDamping = new MxFunctions::mCalcLineEdit(
            MxFunctions::mCalcLineEdit::CalcMode::NonNegative, this);

        setupCalc(m_spbLowerLimit, 0.0, MxFunctions::mCalcLineEdit::CalcMode::Standard);
        setupCalc(m_spbUpperLimit, 0.0, MxFunctions::mCalcLineEdit::CalcMode::Standard);
        setupCalc(m_spbLowerCushion, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbUpperCushion, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbLowerLimitStiffness, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbUpperLimitStiffness, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbLowerLimitDamping, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        setupCalc(m_spbUpperLimitDamping, 0.0, MxFunctions::mCalcLineEdit::CalcMode::NonNegative);

        m_lblLowerLimit = new QLabel("下限:");
        m_lblUpperLimit = new QLabel("上限:");
        m_lblLowerCushion = new QLabel("下限缓冲区:");
        m_lblUpperCushion = new QLabel("上限缓冲区:");
        m_lblLowerLimitStiffness = new QLabel("下限刚度:");
        m_lblUpperLimitStiffness = new QLabel("上限刚度:");
        m_lblLowerLimitDamping = new QLabel("下限阻尼:");
        m_lblUpperLimitDamping = new QLabel("上限阻尼:");

        m_foldLimit->addRow(m_chkEnableLimit);
        m_foldLimit->addRow(m_chkEnableLowerLimit);
        m_foldLimit->addRow(m_lblLowerLimit, m_spbLowerLimit);
        m_foldLimit->addRow(m_chkEnableUpperLimit);
        m_foldLimit->addRow(m_lblUpperLimit, m_spbUpperLimit);
        m_foldLimit->addRow(m_chkPenaltyOnly);
        m_foldLimit->addRow(m_lblLowerCushion, m_spbLowerCushion);
        m_foldLimit->addRow(m_lblUpperCushion, m_spbUpperCushion);
        m_foldLimit->addRow(m_lblLowerLimitStiffness, m_spbLowerLimitStiffness);
        m_foldLimit->addRow(m_lblUpperLimitStiffness, m_spbUpperLimitStiffness);
        m_foldLimit->addRow(m_lblLowerLimitDamping, m_spbLowerLimitDamping);
        m_foldLimit->addRow(m_lblUpperLimitDamping, m_spbUpperLimitDamping);
        m_mainLayout->addWidget(m_foldLimit);

        // 2. 构造函数中直接绑定 Lambda 信号 (纯即时生效逻辑)
        // ==================== 交互控件的 Lambda 实时同步 ====================

        // 类型同步
        connect(m_cmbJointType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (m_motionJointData)
                handleTypeChange(static_cast<MxFunctions::JointType>(m_cmbJointType->currentData().toInt()));
            });

        // 原点坐标实时同步
        connect(m_originPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!m_motionJointData) return;
            const QVector3D pt = m_originPicker->coordinate();
            auto* basicJoint = dynamic_cast<mMotionJointBasic*>(m_motionJointData);
            auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
            auto* screwJoint = dynamic_cast<mMotionJointScrew*>(m_motionJointData);
            auto* perpendicularJoint = dynamic_cast<mMotionJointPerpendicular*>(m_motionJointData);
            auto* constantVelocityJoint = dynamic_cast<mMotionJointConstantVelocity*>(m_motionJointData);
            if (basicJoint)
                basicJoint->setOrigin(pt);
            else if (universalJoint)
                universalJoint->setOrigin(pt);
            else if (screwJoint)
                screwJoint->setOrigin(pt);
            else if (perpendicularJoint)
                perpendicularJoint->setOrigin(pt);
            else if (constantVelocityJoint)
                constantVelocityJoint->setOrigin(pt);
            updateUIFromData();
            });

        // 方向控件取值辅助（默认方向返回无效 QVariant，跳过）
        auto dirSelectorValue = [](mDirectionSelectorWidget* selector) -> QVector3D {
            QVariant dv = selector->getDirection();
            if (!dv.isValid() || dv.isNull())
                return QVector3D();
            return dv.value<QVector3D>();
            };

        // 矢量坐标实时同步
        connect(m_axisDirSelector, &mDirectionSelectorWidget::directionChanged, this, [this, dirSelectorValue]() {
            if (!m_motionJointData) return;
            const QVector3D vec = dirSelectorValue(m_axisDirSelector);
            if (vec.isNull()) return;
            auto* basicJoint = dynamic_cast<mMotionJointBasic*>(m_motionJointData);
            auto* screwJoint = dynamic_cast<mMotionJointScrew*>(m_motionJointData);
            if (basicJoint) {
                basicJoint->setAxis(vec);
                saveDirPickState(m_axisDirSelector, basicJoint->axisPickState_);
            }
            else if (screwJoint) {
                screwJoint->setAxis(vec);
                saveDirPickState(m_axisDirSelector, screwJoint->axisPickState_);
            }
            updateUIFromData();
            });

        // 万向节四个矢量实时同步（同步矢量与拾取会话状态到数据）
        auto syncUniversalVector = [this](mDirectionSelectorWidget* selector, DirectionPickState& state,
            void (mMotionJointUniversal::* setter)(const QVector3D&), const QVector3D& vec) {
            auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
            if (universalJoint) {
                (universalJoint->*setter)(normalizeVector(vec));
                saveDirPickState(selector, state);
                updateUIFromData();
            }
            };

        connect(m_actionSpinDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncUniversalVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                auto* u = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
                if (!u) return;
                syncUniversalVector(m_actionSpinDirSelector, u->actionSpinPickState_,
                    &mMotionJointUniversal::setActionBodySpinVector, dirSelectorValue(m_actionSpinDirSelector));
            });
        connect(m_actionPinDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncUniversalVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                auto* u = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
                if (!u) return;
                syncUniversalVector(m_actionPinDirSelector, u->actionPinPickState_,
                    &mMotionJointUniversal::setActionBodyPinVector, dirSelectorValue(m_actionPinDirSelector));
            });
        connect(m_baseSpinDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncUniversalVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                auto* u = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
                if (!u) return;
                syncUniversalVector(m_baseSpinDirSelector, u->baseSpinPickState_,
                    &mMotionJointUniversal::setBaseBodySpinVector, dirSelectorValue(m_baseSpinDirSelector));
            });
        connect(m_basePinDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncUniversalVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                auto* u = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
                if (!u) return;
                syncUniversalVector(m_basePinDirSelector, u->basePinPickState_,
                    &mMotionJointUniversal::setBaseBodyPinVector, dirSelectorValue(m_basePinDirSelector));
            });

        // 双矢量（垂直约束/恒速副共用）实时同步：isAction 区分运动体/基本体矢量。
        // 拾取会话状态必须以"数据类成员引用"保存：此前经三元表达式传参
        //（p ? p->member : DirectionPickState()）产生临时副本，saveDirPickState
        // 写入副本即丢，updateUIFromData 随后按成员里的旧状态回放，把"定义方式"
        // 下拉弹回旧项——表现为恒速副/垂直约束矢量拾取框的选择类型无法切换
        auto syncDualVector = [this](mDirectionSelectorWidget* selector, bool isAction, const QVector3D& vec) {
            if (vec.isNull()) return;
            const QVector3D nvec = normalizeVector(vec);
            if (auto* perpendicularJoint = dynamic_cast<mMotionJointPerpendicular*>(m_motionJointData)) {
                DirectionPickState& state = isAction ? perpendicularJoint->actionVecPickState_
                                                     : perpendicularJoint->baseVecPickState_;
                if (isAction) perpendicularJoint->setActionBodyVector(nvec);
                else        perpendicularJoint->setBaseBodyVector(nvec);
                saveDirPickState(selector, state);
                updateUIFromData();
            }
            else if (auto* constantVelocityJoint = dynamic_cast<mMotionJointConstantVelocity*>(m_motionJointData)) {
                DirectionPickState& state = isAction ? constantVelocityJoint->actionVecPickState_
                                                     : constantVelocityJoint->baseVecPickState_;
                if (isAction) constantVelocityJoint->setActionBodyVector(nvec);
                else        constantVelocityJoint->setBaseBodyVector(nvec);
                saveDirPickState(selector, state);
                updateUIFromData();
            }
            };

        connect(m_actionVecDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncDualVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                syncDualVector(m_actionVecDirSelector, true, dirSelectorValue(m_actionVecDirSelector));
            });
        connect(m_baseVecDirSelector, &mDirectionSelectorWidget::directionChanged, this,
            [this, syncDualVector, dirSelectorValue]() {
                if (!m_motionJointData) return;
                syncDualVector(m_baseVecDirSelector, false, dirSelectorValue(m_baseVecDirSelector));
            });

        // ==================== 拾取按钮的 Lambda 逻辑 ====================

        connect(m_cmbActionBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionJointData) return;
            mMotionRefMap::set(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionActionBody, m_cmbActionBody->itemData(index).toInt());

            updateUIFromData();

            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionJointSig(false, m_motionJointData->getId());
            });
        // 约束曲面拾取变化：面ID集合整体替换写入（清空=退回基本体整体网格）
        connect(m_surfaceFaceController, &mPickWidgetController::dataChanged, this, [this]() {
            auto* pos = dynamic_cast<MechMotionData::mMotionJointPointOnSurface*>(m_motionJointData);
            if (!pos)
                return;
            QVector<int> faceIds;
            const auto& picked = m_surfaceFaceController->getAllData().pickedData;
            const auto it = picked.find(MxFunctions::PickDataType::GeoFace);
            if (it != picked.end())
            {
                for (const auto& d : it->second)
                    faceIds.append(d.result_id);
            }
            pos->setSurfaceFaceIds(faceIds);
            refreshSurfaceFaceLabel();
            });
        // 约束曲线拾取变化：边ID集合整体替换写入（曲线必须显式拾取，清空即非法）
        connect(m_curveEdgeController, &mPickWidgetController::dataChanged, this, [this]() {
            auto* poc = dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(m_motionJointData);
            if (!poc)
                return;
            QVector<int> edgeIds;
            const auto& picked = m_curveEdgeController->getAllData().pickedData;
            const auto it = picked.find(MxFunctions::PickDataType::GeoEdge);
            if (it != picked.end())
            {
                for (const auto& d : it->second) {
                    // 去重：同一条边可能被重复提交
                    if (!edgeIds.contains(d.result_id))
                        edgeIds.append(d.result_id);
                }
            }
            poc->setCurveEdgeIds(edgeIds);
            refreshCurveEdgeLabel();
            });
        // 线在线上副：约束曲线I（动作体侧）拾取变化
        connect(m_curveIEdgeController, &mPickWidgetController::dataChanged, this, [this]() {
            auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData);
            if (!cvcv)
                return;
            QVector<int> edgeIds;
            const auto& picked = m_curveIEdgeController->getAllData().pickedData;
            const auto it = picked.find(MxFunctions::PickDataType::GeoEdge);
            if (it != picked.end())
            {
                for (const auto& d : it->second) {
                    if (!edgeIds.contains(d.result_id))
                        edgeIds.append(d.result_id);
                }
            }
            cvcv->setCurveIEdgeIds(edgeIds);
            refreshCurveIEdgeLabel();
            });
        // 线在线上副：约束曲线J（基本体侧）拾取变化
        connect(m_curveJEdgeController, &mPickWidgetController::dataChanged, this, [this]() {
            auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData);
            if (!cvcv)
                return;
            QVector<int> edgeIds;
            const auto& picked = m_curveJEdgeController->getAllData().pickedData;
            const auto it = picked.find(MxFunctions::PickDataType::GeoEdge);
            if (it != picked.end())
            {
                for (const auto& d : it->second) {
                    if (!edgeIds.contains(d.result_id))
                        edgeIds.append(d.result_id);
                }
            }
            cvcv->setCurveJEdgeIds(edgeIds);
            refreshCurveJEdgeLabel();
            });
        connect(m_cmbBaseBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_motionJointData) return;
            mMotionRefMap::set(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, m_cmbBaseBody->itemData(index).toInt());

            updateUIFromData();

            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionJointSig(false, m_motionJointData->getId());
            });

        // 摩擦数据实时同步
        auto onFrictionEdited = [this]() {
            syncFrictionDataFromUI();
            updateFrictionLimitVisibility();
            };
        connect(m_chkEnableFriction, &QCheckBox::toggled, this, [onFrictionEdited](bool) { onFrictionEdited(); });
        connect(m_spbStaticMu, &QLineEdit::editingFinished, this, [onFrictionEdited]() { onFrictionEdited(); });
        connect(m_spbDynamicMu, &QLineEdit::editingFinished, this, [onFrictionEdited]() { onFrictionEdited(); });
        connect(m_spbFrictionRadius, &QLineEdit::editingFinished, this, [onFrictionEdited]() { onFrictionEdited(); });

        // 限位数据实时同步
        auto onLimitEdited = [this]() {
            syncLimitDataFromUI();
            updateFrictionLimitVisibility();
            };
        connect(m_chkEnableLimit, &QCheckBox::toggled, this, [onLimitEdited](bool) { onLimitEdited(); });
        connect(m_chkEnableLowerLimit, &QCheckBox::toggled, this, [onLimitEdited](bool) { onLimitEdited(); });
        connect(m_chkEnableUpperLimit, &QCheckBox::toggled, this, [onLimitEdited](bool) { onLimitEdited(); });
        connect(m_chkPenaltyOnly, &QCheckBox::toggled, this, [onLimitEdited](bool) { onLimitEdited(); });
        connect(m_spbLowerLimit, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbUpperLimit, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbLowerCushion, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbUpperCushion, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbLowerLimitStiffness, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbUpperLimitStiffness, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbLowerLimitDamping, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });
        connect(m_spbUpperLimitDamping, &QLineEdit::editingFinished, this, [onLimitEdited]() { onLimitEdited(); });

        // 螺纹导程实时同步
        connect(m_spbLead, &QLineEdit::editingFinished, this, [this]() {
            if (m_motionJointData) {
                auto* screwJoint = dynamic_cast<mMotionJointScrew*>(m_motionJointData);
                if (screwJoint) {
                    screwJoint->setLead(m_spbLead->text().toDouble());
                }
            }
            });

        updateFrictionLimitVisibility();
        m_mainLayout->addStretch();
    }

    mMotionJointPanel::~mMotionJointPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }

    }

    void mMotionJointPanel::refreshSurfaceFaceLabel()
    {
        if (!m_lblFaceCount) return;
        if (auto* pos = dynamic_cast<MechMotionData::mMotionJointPointOnSurface*>(m_motionJointData)) {
            const int n = pos->getSurfaceFaceIds().size();
            m_lblFaceCount->setText(n > 0 ? QString("已选 %1 个面").arg(n)
                                          : QStringLiteral("未选择面（用基本体整体网格）"));
        }
    }

    void mMotionJointPanel::refreshCurveEdgeLabel()
    {
        if (!m_lblEdgeCount) return;
        if (auto* poc = dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(m_motionJointData)) {
            const int n = poc->getCurveEdgeIds().size();
            m_lblEdgeCount->setText(n > 0 ? QString("已选 %1 段边（将串接为一条折线）").arg(n)
                                          : QStringLiteral("未选择边"));
        }
    }

    void mMotionJointPanel::refreshCurveIEdgeLabel()
    {
        if (!m_lblEdgeICount) return;
        if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
            const int n = cvcv->getCurveIEdgeIds().size();
            m_lblEdgeICount->setText(n > 0 ? QString("已选 %1 段边（将串接为一条折线）").arg(n)
                                           : QStringLiteral("未选择边"));
        }
    }

    void mMotionJointPanel::refreshCurveJEdgeLabel()
    {
        if (!m_lblEdgeJCount) return;
        if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
            const int n = cvcv->getCurveJEdgeIds().size();
            m_lblEdgeJCount->setText(n > 0 ? QString("已选 %1 段边（将串接为一条折线）").arg(n)
                                           : QStringLiteral("未选择边"));
        }
    }

    void mMotionJointPanel::newMotionJointPanel(MxFunctions::JointType type)
    {
        setEditing(true);
        connectSig();

        int motionJointID = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).createID();
        if (type == JointType::Universal) {
            m_motionJointData = new mMotionJointUniversal(motionJointID, type);
        }
        else if (type == JointType::Screw) {
            m_motionJointData = new mMotionJointScrew(motionJointID, type);
        }
        else if (type == JointType::Perpendicular) {
            m_motionJointData = new mMotionJointPerpendicular(motionJointID, type);
        }
        else if (type == JointType::ConstantVelocity) {
            m_motionJointData = new mMotionJointConstantVelocity(motionJointID, type);
        }
        else if (type == JointType::PointOnSurface) {
            m_motionJointData = new MechMotionData::mMotionJointPointOnSurface(motionJointID, type);
        }
        else if (type == JointType::PointOnCurve) {
            m_motionJointData = new MechMotionData::mMotionJointPointOnCurve(motionJointID, type);
        }
        else if (type == JointType::CurveOnCurve) {
            m_motionJointData = new MechMotionData::mMotionJointCurveOnCurve(motionJointID, type);
        }
        else {
            m_motionJointData = new mMotionJointBasic(motionJointID, type);
        }

        m_motionJointData->setName(getCVM->getModelData()->createNameByType(Physical_MotionJoint, m_motionJointData->getTypeName()), false);
        getCVM->getModelData()->addPhysicalField<mMotionJointBase>(m_motionJointData);

        //新建对象：清掉方向控件里上一个对象残留的拾取会话（数据本身为空，无状态可恢复）
        for (mDirectionSelectorWidget* selector : {
            m_axisDirSelector, m_actionSpinDirSelector, m_actionPinDirSelector,
            m_baseSpinDirSelector, m_basePinDirSelector, m_actionVecDirSelector, m_baseVecDirSelector })
        {
            restoreDirPickState(selector, DirectionPickState(), QVector3D());
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionJointSig(true, m_motionJointData->getId());
    }

    void mMotionJointPanel::editMotionJointPanel(int id)
    {
        setEditing(true);
        m_motionJointData = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).getData(id);
        if (!m_motionJointData)
            return;

        //旧工程兼容：早期版本点在线上副底层落 mMotionJointBasic 组（无曲线引用），
        //编辑入口归一化为派生类，保留原点/轴等已有数据（曲线引用需用户重拾取）
        if (m_motionJointData->getType() == JointType::PointOnCurve
            && !dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(m_motionJointData))
        {
            auto* legacy = dynamic_cast<MechMotionData::mMotionJointBasic*>(m_motionJointData);
            if (legacy)
            {
                auto* migrated = new MechMotionData::mMotionJointPointOnCurve(legacy->getId(), JointType::PointOnCurve, legacy->getName());
                migrated->setName(legacy->getName(), legacy->getIsUserRename());
                migrated->setFrictionData(legacy->getFrictionData());
                migrated->setLimitData(legacy->getLimitData());
                migrated->setOrigin(legacy->getOrigin());
                migrated->setAxis(legacy->getAxis());
                migrated->axisPickState_ = legacy->axisPickState_;
                getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).deleteData(legacy->getId());
                getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).addData(migrated);
                m_motionJointData = migrated;
            }
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);

        connectSig();

        //进入编辑为新会话：反显数据值（拾取会话模式仍保留，见 restoreDirectionSelector）
        updateUIFromData();

        //进入编辑激活第一个可见点拾取控件（原点），方向控件按模式自管理
        activateFirstVisiblePointPicker({ m_originPicker },
            { m_axisDirSelector, m_actionSpinDirSelector, m_actionPinDirSelector,
              m_baseSpinDirSelector, m_basePinDirSelector, m_actionVecDirSelector, m_baseVecDirSelector });

        //恢复拾取会话：延迟到事件循环下一拍执行——树点击链路（unEdit清理、
        // setSymbolShow、浏览器树刷新等）在面板编辑返回后还有一串操作，
        // 同步恢复会被其中动共享高亮层的调用清掉；延迟后恢复即最终状态。
        // 选择对象模式有数据的方向控件重新激活（进面板即可继续拾取），
        // 未被激活但持数据的点控件重绘临时高亮。
        QTimer::singleShot(0, this, [this]() {
            if (!m_motionJointData)
                return;
            restorePickSessionsOnEntry({ m_originPicker },
                { m_axisDirSelector, m_actionSpinDirSelector, m_actionPinDirSelector,
                  m_baseSpinDirSelector, m_basePinDirSelector, m_actionVecDirSelector, m_baseVecDirSelector });
            });
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionJointPanel::onMotionJointDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionJointData == nullptr || id != m_motionJointData->getId())
                return;
        }
        m_motionJointData = nullptr;
    }

    void mMotionJointPanel::uneditMotionJointPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_motionJointData = nullptr;
        //退出编辑时失活原点拾取控件；拾取数据是临时的（确定值已入数据类），一并清空，
        //防止下次进入面板时残留拾取被激活态渲染成高亮
        if (m_originPicker && m_originPicker->pickController())
        {
            m_originPicker->pickController()->setPickActive(false);
            m_originPicker->clearAllHighlight();
            m_originPicker->pickController()->displayData(PickWidgetData());
        }
        //退出编辑时失活约束曲面拾取控件
        if (m_surfaceFaceController)
        {
            m_surfaceFaceController->setPickActive(false);
            m_surfaceFaceController->clearAllHighlight();
        }
        //退出编辑时失活约束曲线拾取控件
        if (m_curveEdgeController)
        {
            m_curveEdgeController->setPickActive(false);
            m_curveEdgeController->clearAllHighlight();
        }
        //退出编辑时失活约束曲线I/J拾取控件
        if (m_curveIEdgeController)
        {
            m_curveIEdgeController->setPickActive(false);
            m_curveIEdgeController->clearAllHighlight();
        }
        if (m_curveJEdgeController)
        {
            m_curveJEdgeController->setPickActive(false);
            m_curveJEdgeController->clearAllHighlight();
        }
        //退出编辑时失活全部方向控件内部拾取
        for (mDirectionSelectorWidget* selector : {
            m_axisDirSelector, m_actionSpinDirSelector, m_actionPinDirSelector,
            m_baseSpinDirSelector, m_basePinDirSelector, m_actionVecDirSelector, m_baseVecDirSelector })
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
        disconnectSig();
        getCVM->getPreRenderWidget()->getCommonShapeRender()->deletePoint("jointPoint");
    }

    void mMotionJointPanel::refreshBodyComboBoxes()
    {
        // 阻塞信号，防止在清空和添加项目时触发 Lambda
        const QSignalBlocker b1(m_cmbActionBody);
        const QSignalBlocker b2(m_cmbBaseBody);

        m_cmbActionBody->clear();
        m_cmbBaseBody->clear();

        // 1. 添加默认的“空白选项”，内部 ID 设为 -1 代表未选择
        m_cmbActionBody->addItem(" ", -1);
        m_cmbBaseBody->addItem(" ", -1);

        // 2. 获取所有的运动体数据 (伪代码：请换成你实际获取 Manager 的代码)
        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        std::vector<int> allIds = bodyManager.getAllIDs();
        for (int id : allIds) {
            QString name = bodyManager.getNameByID(id);
            // 将名称显示在界面，将 ID 作为隐藏数据 (userData) 存入
            m_cmbActionBody->addItem(name, id);
            m_cmbBaseBody->addItem(name, id);
        }
    }

    void mMotionJointPanel::updateUIFromData()
    {
        if (!getCVM)
        {
            return;
        }

        if (!m_motionJointData)
            return;

        refreshBodyComboBoxes();

        // 阻塞信号防死循环
        const QSignalBlocker bType(m_cmbJointType);
        int typeIndex = m_cmbJointType->findData(static_cast<int>(m_motionJointData->getType()));
        m_cmbJointType->setCurrentIndex(typeIndex != -1 ? typeIndex : 0);

        // 摩擦数据 -> UI
        {
            const auto& friction = m_motionJointData->getFrictionData();
            const QSignalBlocker b0(m_chkEnableFriction);
            const QSignalBlocker b1(m_spbStaticMu);
            const QSignalBlocker b2(m_spbDynamicMu);
            const QSignalBlocker b4(m_spbFrictionRadius);

            m_chkEnableFriction->setChecked(friction.enabled);
            m_spbStaticMu->setExpression(QString::number(friction.staticCoefficient));
            m_spbDynamicMu->setExpression(QString::number(friction.kineticCoefficient));
            m_spbFrictionRadius->setExpression(QString::number(friction.effectiveRadius));
        }

        // 限位数据 -> UI。角度量（旋转副下限/上限/缓冲区）统一以"度"存储与显示，
        // 求解导出处统一换算 rad（引擎保持纯弧度）。
        {
            const auto& limit = m_motionJointData->getLimitData();

            const QSignalBlocker b0(m_chkEnableLimit);
            const QSignalBlocker b1(m_chkEnableLowerLimit);
            const QSignalBlocker b2(m_chkEnableUpperLimit);
            const QSignalBlocker b3(m_chkPenaltyOnly);
            const QSignalBlocker b4(m_spbLowerLimit);
            const QSignalBlocker b5(m_spbUpperLimit);
            const QSignalBlocker b6(m_spbLowerCushion);
            const QSignalBlocker b7(m_spbUpperCushion);
            const QSignalBlocker b8(m_spbLowerLimitStiffness);
            const QSignalBlocker b9(m_spbUpperLimitStiffness);
            const QSignalBlocker b10(m_spbLowerLimitDamping);
            const QSignalBlocker b11(m_spbUpperLimitDamping);

            m_chkEnableLimit->setChecked(limit.enabled);
            m_chkEnableLowerLimit->setChecked(limit.lowerEnabled);
            m_chkEnableUpperLimit->setChecked(limit.upperEnabled);
            m_chkPenaltyOnly->setChecked(limit.penaltyOnly);
            m_spbLowerLimit->setExpression(QString::number(limit.lowerValue));
            m_spbUpperLimit->setExpression(QString::number(limit.upperValue));
            m_spbLowerCushion->setExpression(QString::number(limit.lowerCushion));
            m_spbUpperCushion->setExpression(QString::number(limit.upperCushion));
            m_spbLowerLimitStiffness->setExpression(QString::number(limit.lowerStiffness));
            m_spbUpperLimitStiffness->setExpression(QString::number(limit.upperStiffness));
            m_spbLowerLimitDamping->setExpression(QString::number(limit.lowerDamping));
            m_spbUpperLimitDamping->setExpression(QString::number(limit.upperDamping));

            const bool isRotation = (m_motionJointData->getType() == JointType::Revolute);
            if (isRotation) {
                m_lblLowerLimit->setText(QStringLiteral("下限 (°):"));
                m_lblUpperLimit->setText(QStringLiteral("上限 (°):"));
                m_lblLowerCushion->setText(QStringLiteral("下限缓冲区 (°):"));
                m_lblUpperCushion->setText(QStringLiteral("上限缓冲区 (°):"));
                m_lblLowerLimitStiffness->setText(QStringLiteral("下限刚度 (N·mm/°):"));
                m_lblUpperLimitStiffness->setText(QStringLiteral("上限刚度 (N·mm/°):"));
                m_lblLowerLimitDamping->setText(QStringLiteral("下限阻尼 (N·mm/(°/s)):"));
                m_lblUpperLimitDamping->setText(QStringLiteral("上限阻尼 (N·mm/(°/s)):"));
            }
            else {
                m_lblLowerLimit->setText(QStringLiteral("下限 (mm):"));
                m_lblUpperLimit->setText(QStringLiteral("上限 (mm):"));
                m_lblLowerCushion->setText(QStringLiteral("下限缓冲区 (mm):"));
                m_lblUpperCushion->setText(QStringLiteral("上限缓冲区 (mm):"));
                m_lblLowerLimitStiffness->setText(QStringLiteral("下限刚度 (N/mm):"));
                m_lblUpperLimitStiffness->setText(QStringLiteral("上限刚度 (N/mm):"));
                m_lblLowerLimitDamping->setText(QStringLiteral("下限阻尼 (N·s/mm):"));
                m_lblUpperLimitDamping->setText(QStringLiteral("上限阻尼 (N·s/mm):"));
            }
        }

        updateFrictionLimitVisibility();

        // --- 核心：根据数据类里的 ID，反向寻找下拉框中的索引并选中 ---
        int actionId = mMotionRefMap::get(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
        int actionIndex = m_cmbActionBody->findData(actionId); // 拿着 ID 去找
        const QSignalBlocker bAction(m_cmbActionBody);
        m_cmbActionBody->setCurrentIndex(actionIndex != -1 ? actionIndex : 0); // 找不到就选第0项(空白)

        //渲染运动体1
        auto motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(actionId);

        if (motionBodyData)
        {
            auto widget_ptr = getCVM->getPreRenderWidget();
            auto highlight_controller = widget_ptr->getHighlightController();

            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            geoHighLightRender->setHighLightColor(mMotionPanelColors::bodyA().red(),
                mMotionPanelColors::bodyA().green(), mMotionPanelColors::bodyA().blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            const std::set<int> highlightPartIds = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(highlightPartIds.begin(), highlightPartIds.end()));
            geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        else if (auto* hc = getCVM->getPreRenderWidget()->getHighlightController())
        {
            //运动体为空时清除该槽位高亮，避免残留上一个体的高亮
            if (auto* render = hc->getExtraPreGeoHighLightRender(std::string("red")))
            {
                mGeoSelectData* geoData = new mGeoSelectData();
                geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
                render->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
            }
        }

        int baseId = mMotionRefMap::get(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
        int baseIndex = m_cmbBaseBody->findData(baseId);
        const QSignalBlocker bBase(m_cmbBaseBody);
        m_cmbBaseBody->setCurrentIndex(baseIndex != -1 ? baseIndex : 0);

        motionBodyData = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody).getData(baseId);

        if (motionBodyData)
        {
            auto widget_ptr = getCVM->getPreRenderWidget();
            auto highlight_controller = widget_ptr->getHighlightController();

            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"));
            geoHighLightRender->setHighLightColor(mMotionPanelColors::bodyB().red(),
                mMotionPanelColors::bodyB().green(), mMotionPanelColors::bodyB().blue());
            mGeoSelectData* geoData = new mGeoSelectData();
            const std::set<int> highlightPartIds = mMotionRefMap::getSet(Physical_MotionBody, motionBodyData->getId(), Physical_Part);
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>(highlightPartIds.begin(), highlightPartIds.end()));
            geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        else if (auto* hc = getCVM->getPreRenderWidget()->getHighlightController())
        {
            //运动体为空时清除该槽位高亮，避免残留上一个体的高亮
            if (auto* render = hc->getExtraPreGeoHighLightRender(std::string("blue")))
            {
                mGeoSelectData* geoData = new mGeoSelectData();
                geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart, std::vector<int>());
                render->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
            }
        }

        auto* basicJoint = dynamic_cast<mMotionJointBasic*>(m_motionJointData);
        auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
        auto* screwJoint = dynamic_cast<mMotionJointScrew*>(m_motionJointData);
        auto* perpendicularJoint = dynamic_cast<mMotionJointPerpendicular*>(m_motionJointData);
        auto* constantVelocityJoint = dynamic_cast<mMotionJointConstantVelocity*>(m_motionJointData);

        // 统一显示/隐藏：通过 setParent(nullptr) 从布局中移除/插入实现
        auto ensureInLayout = [](QWidget* widget, bool shouldShow, FoldSection* section) {
            if (shouldShow) {
                if (widget->parent() == nullptr) {
                    section->addRow(widget);
                }
            }
            else {
                if (widget->parent() != nullptr) {
                    widget->setParent(nullptr);
                }
            }
            };

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

        ensureInLayout(m_originPicker, basicJoint || universalJoint || screwJoint || perpendicularJoint || constantVelocityJoint, m_foldAction);
        // 点在面上副的法向由曲面网格决定、点在线上副的切向由曲线决定，轴矢量无意义不显示
        const bool hideAxisType = m_motionJointData
            && (m_motionJointData->getType() == JointType::PointOnSurface
             || m_motionJointData->getType() == JointType::PointOnCurve);
        const bool showAxisRow = (basicJoint || screwJoint) && !hideAxisType;
        ensureInLayout(m_axisDirSelector, showAxisRow, m_foldAction);
        ensureInLayout(m_actionSpinDirSelector, universalJoint, m_foldAction);
        ensureInLayout(m_actionPinDirSelector, universalJoint, m_foldAction);
        ensureInLayout(m_actionVecDirSelector, perpendicularJoint || constantVelocityJoint, m_foldAction);

        ensureInLayout(m_baseSpinDirSelector, universalJoint, m_foldBase);
        ensureInLayout(m_basePinDirSelector, universalJoint, m_foldBase);
        ensureInLayout(m_baseVecDirSelector, perpendicularJoint || constantVelocityJoint, m_foldBase);

        // 点在面上副：约束曲面折叠区显隐 + 拾取控件反显/失活
        if (m_foldSurface) {
            const bool posJoint = (m_motionJointData && m_motionJointData->getType() == JointType::PointOnSurface);
            m_foldSurface->setVisible(posJoint);
            if (posJoint) {
                refreshSurfaceFaceLabel();
                // 已选面ID反显到拾取控件（不置脏、不高亮，避免遮挡运动体高亮）
                PickWidgetData faceData;
                const auto faceIds = dynamic_cast<MechMotionData::mMotionJointPointOnSurface*>(m_motionJointData)->getSurfaceFaceIds();
                if (!faceIds.isEmpty()) {
                    std::vector<RenderPickResultData> items;
                    items.reserve(faceIds.size());
                    for (int fid : faceIds) {
                        RenderPickResultData item;
                        item.result_id = fid;
                        items.push_back(item);
                    }
                    faceData.pickedData[MxFunctions::PickDataType::GeoFace] = std::move(items);
                }
                m_surfaceFaceController->displayData(faceData);
            }
            else if (m_surfaceFaceController) {
                // 折叠区隐藏时失活，避免隐藏控件占用拾取状态
                m_surfaceFaceController->setPickActive(false);
            }
        }

        // 点在线上副：约束曲线折叠区显隐 + 拾取控件反显/失活
        if (m_foldCurve) {
            const bool pocJoint = (m_motionJointData && m_motionJointData->getType() == JointType::PointOnCurve);
            m_foldCurve->setVisible(pocJoint);
            if (pocJoint) {
                refreshCurveEdgeLabel();
                // 已选边ID反显到拾取控件（不置脏、不高亮，避免遮挡运动体高亮）
                PickWidgetData edgeData;
                if (auto* poc = dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(m_motionJointData)) {
                    const auto edgeIds = poc->getCurveEdgeIds();
                    if (!edgeIds.isEmpty()) {
                        std::vector<RenderPickResultData> items;
                        items.reserve(edgeIds.size());
                        for (int eid : edgeIds) {
                            RenderPickResultData item;
                            item.result_id = eid;
                            items.push_back(item);
                        }
                        edgeData.pickedData[MxFunctions::PickDataType::GeoEdge] = std::move(items);
                    }
                }
                m_curveEdgeController->displayData(edgeData);
            }
            else if (m_curveEdgeController) {
                // 折叠区隐藏时失活，避免隐藏控件占用拾取状态
                m_curveEdgeController->setPickActive(false);
            }
        }

        // 线在线上副：约束曲线I（动作体侧）折叠区显隐 + 拾取控件反显/失活
        if (m_foldCurveI) {
            const bool cvcvJoint = (m_motionJointData && m_motionJointData->getType() == JointType::CurveOnCurve);
            m_foldCurveI->setVisible(cvcvJoint);
            if (cvcvJoint) {
                refreshCurveIEdgeLabel();
                PickWidgetData edgeData;
                if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
                    const auto edgeIds = cvcv->getCurveIEdgeIds();
                    if (!edgeIds.isEmpty()) {
                        std::vector<RenderPickResultData> items;
                        items.reserve(edgeIds.size());
                        for (int eid : edgeIds) {
                            RenderPickResultData item;
                            item.result_id = eid;
                            items.push_back(item);
                        }
                        edgeData.pickedData[MxFunctions::PickDataType::GeoEdge] = std::move(items);
                    }
                }
                m_curveIEdgeController->displayData(edgeData);
            }
            else if (m_curveIEdgeController) {
                m_curveIEdgeController->setPickActive(false);
            }
        }

        // 线在线上副：约束曲线J（基本体侧）折叠区显隐 + 拾取控件反显/失活
        if (m_foldCurveJ) {
            const bool cvcvJoint = (m_motionJointData && m_motionJointData->getType() == JointType::CurveOnCurve);
            m_foldCurveJ->setVisible(cvcvJoint);
            if (cvcvJoint) {
                // "允许滑动"勾选反显
                if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
                    m_chkAllowSliding->blockSignals(true);
                    m_chkAllowSliding->setChecked(cvcv->getAllowSliding());
                    m_chkAllowSliding->blockSignals(false);
                }
                refreshCurveJEdgeLabel();
                PickWidgetData edgeData;
                if (auto* cvcv = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(m_motionJointData)) {
                    const auto edgeIds = cvcv->getCurveJEdgeIds();
                    if (!edgeIds.isEmpty()) {
                        std::vector<RenderPickResultData> items;
                        items.reserve(edgeIds.size());
                        for (int eid : edgeIds) {
                            RenderPickResultData item;
                            item.result_id = eid;
                            items.push_back(item);
                        }
                        edgeData.pickedData[MxFunctions::PickDataType::GeoEdge] = std::move(items);
                    }
                }
                m_curveJEdgeController->displayData(edgeData);
            }
            else if (m_curveJEdgeController) {
                m_curveJEdgeController->setPickActive(false);
            }
        }

        if (basicJoint)
        {
            ensurePairInLayout(m_lblLead, m_spbLead, false, m_foldJointParams);
            m_foldJointParams->setVisible(false);
            // 还原原点
            QVector3D origin = basicJoint->getOrigin();
            m_originPicker->setCoordinate(origin);

            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("jointPoint", { origin }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

            // 还原矢量
            QVector3D axis = basicJoint->getAxis();
            restoreDirectionSelector(m_axisDirSelector, basicJoint->axisPickState_, axis);

            //创建或更新符号
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString newsymbol = QString("motionJoint_%1").arg(m_motionJointData->getId());
            mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                m_motionJointData->getId(),
                m_motionJointData->getType(),
                m_motionJointData->getName(),
                origin,
                axis,
                getCVM->getPreRenderWidget());
            //symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
            getCVM->getPreRenderWidget()->updatePaint();

        }
        else if (universalJoint)
        {
            ensurePairInLayout(m_lblLead, m_spbLead, false, m_foldJointParams);
            m_foldJointParams->setVisible(false);
            QVector3D origin = universalJoint->getOrigin();
            m_originPicker->setCoordinate(origin);

            restoreDirectionSelector(m_actionSpinDirSelector, universalJoint->actionSpinPickState_, universalJoint->getActionBodySpinVector());
            restoreDirectionSelector(m_actionPinDirSelector, universalJoint->actionPinPickState_, universalJoint->getActionBodyPinVector());
            restoreDirectionSelector(m_baseSpinDirSelector, universalJoint->baseSpinPickState_, universalJoint->getBaseBodySpinVector());
            restoreDirectionSelector(m_basePinDirSelector, universalJoint->basePinPickState_, universalJoint->getBaseBodyPinVector());

            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("jointPoint", { origin }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString newsymbol = QString("motionJoint_%1").arg(m_motionJointData->getId());
            mMotionJointSymbolHelper::createOrUpdateUniversalSymbol(symbolController,
                m_motionJointData->getId(),
                m_motionJointData->getName(),
                origin,
                universalJoint->getActionBodySpinVector(),
                universalJoint->getActionBodyPinVector(),
                universalJoint->getBaseBodySpinVector(),
                universalJoint->getBaseBodyPinVector(),
                getCVM->getPreRenderWidget());
            //symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { newsymbol }, 0.05);
            getCVM->getPreRenderWidget()->updatePaint();
        }
        else if (screwJoint)
        {
            m_foldJointParams->setVisible(true);
            ensurePairInLayout(m_lblLead, m_spbLead, true, m_foldJointParams);
            QVector3D origin = screwJoint->getOrigin();
            m_originPicker->setCoordinate(origin);

            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("jointPoint", { origin }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

            QVector3D axis = screwJoint->getAxis();
            restoreDirectionSelector(m_axisDirSelector, screwJoint->axisPickState_, axis);

            // 同步螺纹导程
            const QSignalBlocker bLead(m_spbLead);
            m_spbLead->setExpression(QString::number(screwJoint->getLead()));

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString newsymbol = QString("motionJoint_%1").arg(m_motionJointData->getId());
            mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                m_motionJointData->getId(),
                m_motionJointData->getType(),
                m_motionJointData->getName(),
                origin,
                axis,
                getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
        }
        else if (perpendicularJoint)
        {
            ensurePairInLayout(m_lblLead, m_spbLead, false, m_foldJointParams);
            m_foldJointParams->setVisible(false);
            QVector3D origin = perpendicularJoint->getOrigin();
            m_originPicker->setCoordinate(origin);

            restoreDirectionSelector(m_actionVecDirSelector, perpendicularJoint->actionVecPickState_, perpendicularJoint->getActionBodyVector());
            restoreDirectionSelector(m_baseVecDirSelector, perpendicularJoint->baseVecPickState_, perpendicularJoint->getBaseBodyVector());

            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("jointPoint", { origin }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString newsymbol = QString("motionJoint_%1").arg(m_motionJointData->getId());
            mMotionJointSymbolHelper::createOrUpdatePerpendicularSymbol(symbolController,
                m_motionJointData->getId(),
                m_motionJointData->getName(),
                origin,
                perpendicularJoint->getActionBodyVector(),
                perpendicularJoint->getBaseBodyVector(),
                getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
        }
        else if (constantVelocityJoint)
        {
            ensurePairInLayout(m_lblLead, m_spbLead, false, m_foldJointParams);
            m_foldJointParams->setVisible(false);
            QVector3D origin = constantVelocityJoint->getOrigin();
            m_originPicker->setCoordinate(origin);

            restoreDirectionSelector(m_actionVecDirSelector, constantVelocityJoint->actionVecPickState_, constantVelocityJoint->getActionBodyVector());
            restoreDirectionSelector(m_baseVecDirSelector, constantVelocityJoint->baseVecPickState_, constantVelocityJoint->getBaseBodyVector());

            getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint("jointPoint", { origin }, mMotionPanelColors::vec(mMotionPanelColors::pointA()));

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            QString newsymbol = QString("motionJoint_%1").arg(m_motionJointData->getId());
            mMotionJointSymbolHelper::createOrUpdateConstantVelocitySymbol(symbolController,
                m_motionJointData->getId(),
                m_motionJointData->getName(),
                origin,
                constantVelocityJoint->getActionBodyVector(),
                constantVelocityJoint->getBaseBodyVector(),
                getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
        }
        else
        {
            ensurePairInLayout(m_lblLead, m_spbLead, false, m_foldJointParams);
            m_foldJointParams->setVisible(false);
        }
        QTimer::singleShot(0, m_foldAction, [=]() {
            m_foldBase->refreshLayout();
            m_foldAction->refreshLayout();
            m_foldJointParams->refreshLayout();
            });


        // 万向节矢量正交校验（仅针对万向节，默认值正交不会触发提示）
        validateUniversalVectors();
    }

    void mMotionJointPanel::handleTypeChange(JointType newType)
    {
        if (!m_motionJointData || m_motionJointData->getType() == newType) return;

        JointType oldType = m_motionJointData->getType();
        const auto oldFrictionData = m_motionJointData->getFrictionData();
        const auto oldLimitData = m_motionJointData->getLimitData();
        QString name = "";
        if (m_motionJointData->getIsUserRename())//用户重命名过
        {
            name = m_motionJointData->getName();
        }
        else//默认名称，类型更改后自动修改名称
        {
            QStringList list = m_motionJointData->getName().split("_");
            if (list.size() >= 1)
            {
                name = m_motionJointData->getTypeName(newType);
                if (list.size() == 2)
                {
                    name = QString("%1_%2").arg(name).arg(list.at(1));
                }
            }
            else
            {
                name = QString("%1_%2").arg(m_motionJointData->getTypeName(newType)).arg(m_motionJointData->getId());
            }
            if (getCVM->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint).existName(name))
            {
                name = getCVM->getModelData()->createNameByType(Physical_MotionJoint, m_motionJointData->getTypeName(newType));
            }
        }

        //判断类的同构性
        auto getClassGroup = [](JointType t) {
            // 万向节需要更多坐标系，底层使用特殊的派生类
            if (t == JointType::Universal) return 2;
            // 螺纹副需要额外的导程参数，底层使用 mMotionJointScrew
            if (t == JointType::Screw) return 3;
            // 垂直约束需要双矢量，底层使用 mMotionJointPerpendicular
            if (t == JointType::Perpendicular) return 4;
            // 恒速副需要双矢量，底层使用 mMotionJointConstantVelocity
            if (t == JointType::ConstantVelocity) return 5;
            // 点在面上副需要约束面引用，底层使用 mMotionJointPointOnSurface
            if (t == JointType::PointOnSurface) return 6;
            // 点在线上副需要约束边引用，底层使用 mMotionJointPointOnCurve
            if (t == JointType::PointOnCurve) return 7;
            // 线在线上副需要双曲线边引用，底层使用 mMotionJointCurveOnCurve
            if (t == JointType::CurveOnCurve) return 8;
            return 1; // 其他的基础副都共用 mMotionJointBasic
            };

        if (getClassGroup(oldType) == getClassGroup(newType))
        {
            // 旋转副与移动副虽然共用 mMotionJointBasic，但限位单位分别是 rad 与长度单位。
            // 类型互换时清空限位，避免把“10度”误解释成“10mm”或反过来。
            const bool switchingLimitCoordinate =
                (oldType == JointType::Revolute && newType == JointType::Translational) ||
                (oldType == JointType::Translational && newType == JointType::Revolute);
            if (switchingLimitCoordinate) {
                m_motionJointData->clearLimit();
            }

            m_motionJointData->setType(newType);
            m_motionJointData->setName(name, m_motionJointData->getIsUserRename());
        }
        else
        {
            // 【情形 B：类不一致】触发底层重构与数据迁移
            int id = m_motionJointData->getId();
            int actionId = mMotionRefMap::get(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
            int baseId = mMotionRefMap::get(Physical_MotionJoint, m_motionJointData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);

            mMotionJointBase* newJointObj = nullptr;

            // 实例化新派生类
            if (getClassGroup(newType) == 1) {
                newJointObj = new mMotionJointBasic(id, newType, name);
            }
            else if (getClassGroup(newType) == 2) {
                newJointObj = new mMotionJointUniversal(id, newType, name);
            }
            else if (getClassGroup(newType) == 3) {
                newJointObj = new mMotionJointScrew(id, newType, name);
            }
            else if (getClassGroup(newType) == 4) {
                newJointObj = new mMotionJointPerpendicular(id, newType, name);
            }
            else if (getClassGroup(newType) == 5) {
                newJointObj = new mMotionJointConstantVelocity(id, newType, name);
            }
            else if (getClassGroup(newType) == 6) {
                newJointObj = new MechMotionData::mMotionJointPointOnSurface(id, newType, name);
            }
            else if (getClassGroup(newType) == 7) {
                newJointObj = new MechMotionData::mMotionJointPointOnCurve(id, newType, name);
            }
            else if (getClassGroup(newType) == 8) {
                newJointObj = new MechMotionData::mMotionJointCurveOnCurve(id, newType, name);
            }

            if (newJointObj) {
                // 【2. 数据迁移】通用基类数据迁移
                mMotionRefMap::set(Physical_MotionJoint, newJointObj->getId(), Physical_MotionBody, MapRelation_MotionActionBody, actionId);
                mMotionRefMap::set(Physical_MotionJoint, newJointObj->getId(), Physical_MotionBody, MapRelation_MotionBaseBody, baseId);
                newJointObj->setFrictionData(oldFrictionData);
                newJointObj->setLimitData(oldLimitData);

                // 【2.1 派生类特殊数据迁移】(尽可能保留用户的输入心血)
                // 比如从复杂副切回简单副，尝试把原点抢救过来
                auto oldBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(m_motionJointData);
                auto newBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(newJointObj);
                if (oldBasic && newBasic) {
                    newBasic->setOrigin(oldBasic->getOrigin());
                    newBasic->setAxis(oldBasic->getAxis());
                    newBasic->axisPickState_ = oldBasic->axisPickState_;
                }

                auto oldUniversal = dynamic_cast<MechMotionData::mMotionJointUniversal*>(m_motionJointData);
                auto newUniversal = dynamic_cast<MechMotionData::mMotionJointUniversal*>(newJointObj);
                if (oldUniversal && newUniversal) {
                    newUniversal->setActionBodySpinVector(oldUniversal->getActionBodySpinVector());
                    newUniversal->setActionBodyPinVector(oldUniversal->getActionBodyPinVector());
                    newUniversal->setBaseBodySpinVector(oldUniversal->getBaseBodySpinVector());
                    newUniversal->setBaseBodyPinVector(oldUniversal->getBaseBodyPinVector());
                    newUniversal->actionSpinPickState_ = oldUniversal->actionSpinPickState_;
                    newUniversal->actionPinPickState_ = oldUniversal->actionPinPickState_;
                    newUniversal->baseSpinPickState_ = oldUniversal->baseSpinPickState_;
                    newUniversal->basePinPickState_ = oldUniversal->basePinPickState_;
                }

                auto oldScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(m_motionJointData);
                auto newScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(newJointObj);
                if (oldScrew && newScrew) {
                    newScrew->setOrigin(oldScrew->getOrigin());
                    newScrew->setAxis(oldScrew->getAxis());
                    newScrew->setLead(oldScrew->getLead());
                    newScrew->axisPickState_ = oldScrew->axisPickState_;
                }
                // 从其他类型切换到螺纹副时，尝试继承原点和轴
                else if (newScrew && oldBasic) {
                    newScrew->setOrigin(oldBasic->getOrigin());
                    newScrew->setAxis(oldBasic->getAxis());
                    newScrew->axisPickState_ = oldBasic->axisPickState_;
                }
                else if (newScrew && oldUniversal) {
                    newScrew->setOrigin(oldUniversal->getOrigin());
                    newScrew->setAxis(oldUniversal->getBaseBodySpinVector());
                }

                auto oldPerpendicular = dynamic_cast<MechMotionData::mMotionJointPerpendicular*>(m_motionJointData);
                auto newPerpendicular = dynamic_cast<MechMotionData::mMotionJointPerpendicular*>(newJointObj);
                if (oldPerpendicular && newPerpendicular) {
                    newPerpendicular->setOrigin(oldPerpendicular->getOrigin());
                    newPerpendicular->setActionBodyVector(oldPerpendicular->getActionBodyVector());
                    newPerpendicular->setBaseBodyVector(oldPerpendicular->getBaseBodyVector());
                    newPerpendicular->actionVecPickState_ = oldPerpendicular->actionVecPickState_;
                    newPerpendicular->baseVecPickState_ = oldPerpendicular->baseVecPickState_;
                }
                // 从其他类型切换到垂直约束时，尝试继承原点和轴
                else if (newPerpendicular && oldBasic) {
                    newPerpendicular->setOrigin(oldBasic->getOrigin());
                    newPerpendicular->setActionBodyVector(oldBasic->getAxis());
                }
                else if (newPerpendicular && oldUniversal) {
                    newPerpendicular->setOrigin(oldUniversal->getOrigin());
                    newPerpendicular->setActionBodyVector(oldUniversal->getActionBodySpinVector());
                    newPerpendicular->setBaseBodyVector(oldUniversal->getBaseBodySpinVector());
                }
                else if (newPerpendicular && oldScrew) {
                    newPerpendicular->setOrigin(oldScrew->getOrigin());
                    newPerpendicular->setActionBodyVector(oldScrew->getAxis());
                }

                auto oldConstantVelocity = dynamic_cast<MechMotionData::mMotionJointConstantVelocity*>(m_motionJointData);
                auto newConstantVelocity = dynamic_cast<MechMotionData::mMotionJointConstantVelocity*>(newJointObj);
                if (oldConstantVelocity && newConstantVelocity) {
                    newConstantVelocity->setOrigin(oldConstantVelocity->getOrigin());
                    newConstantVelocity->setActionBodyVector(oldConstantVelocity->getActionBodyVector());
                    newConstantVelocity->setBaseBodyVector(oldConstantVelocity->getBaseBodyVector());
                    newConstantVelocity->actionVecPickState_ = oldConstantVelocity->actionVecPickState_;
                    newConstantVelocity->baseVecPickState_ = oldConstantVelocity->baseVecPickState_;
                }
                // 从其他类型切换到恒速副时，尝试继承原点和轴
                else if (newConstantVelocity && oldBasic) {
                    newConstantVelocity->setOrigin(oldBasic->getOrigin());
                    newConstantVelocity->setActionBodyVector(oldBasic->getAxis());
                }
                else if (newConstantVelocity && oldUniversal) {
                    newConstantVelocity->setOrigin(oldUniversal->getOrigin());
                    newConstantVelocity->setActionBodyVector(oldUniversal->getActionBodySpinVector());
                    newConstantVelocity->setBaseBodyVector(oldUniversal->getBaseBodySpinVector());
                }
                else if (newConstantVelocity && oldScrew) {
                    newConstantVelocity->setOrigin(oldScrew->getOrigin());
                    newConstantVelocity->setActionBodyVector(oldScrew->getAxis());
                }
                else if (newConstantVelocity && oldPerpendicular) {
                    newConstantVelocity->setOrigin(oldPerpendicular->getOrigin());
                    newConstantVelocity->setActionBodyVector(oldPerpendicular->getActionBodyVector());
                    newConstantVelocity->setBaseBodyVector(oldPerpendicular->getBaseBodyVector());
                }
                // 从恒速副切换到垂直约束时，继承双矢量
                else if (newPerpendicular && oldConstantVelocity) {
                    newPerpendicular->setOrigin(oldConstantVelocity->getOrigin());
                    newPerpendicular->setActionBodyVector(oldConstantVelocity->getActionBodyVector());
                    newPerpendicular->setBaseBodyVector(oldConstantVelocity->getBaseBodyVector());
                }

                //替换数据，防止野指针
                getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).deleteData(id);
                getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint).addData(newJointObj);

                // 【4. 更新面板自身的指针】
                m_motionJointData = newJointObj;
            }
        }

        //类型切换替换数据对象后全量反显（拾取会话模式保留）
        updateUIFromData();
        activateFirstVisiblePointPicker({ m_originPicker },
            { m_axisDirSelector, m_actionSpinDirSelector, m_actionPinDirSelector,
              m_baseSpinDirSelector, m_basePinDirSelector, m_actionVecDirSelector, m_baseVecDirSelector });

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionJointSig(false, m_motionJointData->getId());
    }

    bool mMotionJointPanel::supportsFriction(JointType type) const
    {
        switch (type)
        {
        case JointType::Revolute:
        case JointType::Translational:
        case JointType::Cylindrical:
        case JointType::Universal:
        case JointType::Spherical:
            return true;
        default:
            return false;
        }
    }

    bool mMotionJointPanel::supportsLimit(JointType type) const
    {
        return type == JointType::Revolute ||
            type == JointType::Translational;
    }

    void mMotionJointPanel::updateFrictionLimitVisibility()
    {
        JointType type = JointType::Revolute;
        if (m_motionJointData) {
            type = m_motionJointData->getType();
        }
        else if (m_cmbJointType && m_cmbJointType->currentIndex() >= 0) {
            type = static_cast<JointType>(m_cmbJointType->currentData().toInt());
        }

        const bool frictionSupported = supportsFriction(type);
        const bool limitSupported = supportsLimit(type);

        m_foldFriction->setVisible(frictionSupported);
        m_foldLimit->setVisible(limitSupported);

        const bool frictionEnabled = frictionSupported && m_chkEnableFriction->isChecked();
        m_spbStaticMu->setEnabled(frictionEnabled);
        m_spbDynamicMu->setEnabled(frictionEnabled);

        // 平移副不需要旋转摩擦等效半径。
        const bool radiusVisible = frictionSupported && type != JointType::Translational;
        m_lblFrictionRadius->setVisible(radiusVisible);
        m_spbFrictionRadius->setVisible(radiusVisible);
        m_spbFrictionRadius->setEnabled(frictionEnabled && radiusVisible);

        const bool limitEnabled = limitSupported && m_chkEnableLimit->isChecked();
        m_chkEnableLowerLimit->setEnabled(limitEnabled);
        m_chkEnableUpperLimit->setEnabled(limitEnabled);
        m_chkPenaltyOnly->setEnabled(limitEnabled);

        const bool lowerEnabled = limitEnabled && m_chkEnableLowerLimit->isChecked();
        const bool upperEnabled = limitEnabled && m_chkEnableUpperLimit->isChecked();

        m_spbLowerLimit->setEnabled(lowerEnabled);
        m_spbUpperLimit->setEnabled(upperEnabled);
        m_spbLowerCushion->setEnabled(lowerEnabled);
        m_spbUpperCushion->setEnabled(upperEnabled);
        m_spbLowerLimitStiffness->setEnabled(lowerEnabled);
        m_spbUpperLimitStiffness->setEnabled(upperEnabled);
        m_spbLowerLimitDamping->setEnabled(lowerEnabled);
        m_spbUpperLimitDamping->setEnabled(upperEnabled);

        m_foldFriction->refreshLayout();
        m_foldLimit->refreshLayout();
    }

    void mMotionJointPanel::syncFrictionDataFromUI()
    {
        if (!m_motionJointData || !supportsFriction(m_motionJointData->getType())) {
            return;
        }

        mMotionJointFrictionData data = m_motionJointData->getFrictionData();
        data.enabled = m_chkEnableFriction->isChecked();
        data.staticCoefficient = m_spbStaticMu->text().toDouble();
        data.kineticCoefficient = m_spbDynamicMu->text().toDouble();
        // 保留扩展区或工程文件设定的黏性摩擦系数，不在同步静/动摩擦时清零
        data.effectiveRadius =
            m_motionJointData->getType() == JointType::Translational
            ? 0.0
            : m_spbFrictionRadius->text().toDouble();

        m_motionJointData->setFrictionData(data);

        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateMotionJointSig(false, m_motionJointData->getId());
    }

    void mMotionJointPanel::syncLimitDataFromUI()
    {
        if (!m_motionJointData || !supportsLimit(m_motionJointData->getType())) {
            return;
        }

        // 角度量以"度"直接存储（换算在求解导出侧做）
        mMotionJointLimitData data = m_motionJointData->getLimitData();
        data.enabled = m_chkEnableLimit->isChecked();
        data.lowerEnabled = m_chkEnableLowerLimit->isChecked();
        data.upperEnabled = m_chkEnableUpperLimit->isChecked();
        data.penaltyOnly = m_chkPenaltyOnly->isChecked();

        data.lowerValue = m_spbLowerLimit->text().toDouble();
        data.upperValue = m_spbUpperLimit->text().toDouble();
        data.lowerCushion = m_spbLowerCushion->text().toDouble();
        data.upperCushion = m_spbUpperCushion->text().toDouble();

        data.lowerStiffness = m_spbLowerLimitStiffness->text().toDouble();
        data.upperStiffness = m_spbUpperLimitStiffness->text().toDouble();
        data.lowerDamping = m_spbLowerLimitDamping->text().toDouble();
        data.upperDamping = m_spbUpperLimitDamping->text().toDouble();

        m_motionJointData->setLimitData(data);

        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateMotionJointSig(false, m_motionJointData->getId());
    }

    QVector3D mMotionJointPanel::normalizeVector(const QVector3D& v) const
    {
        float len2 = v.lengthSquared();
        if (len2 < 1e-12f) return v;
        return v / std::sqrt(len2);
    }

    bool mMotionJointPanel::isOrthogonal(const QVector3D& a, const QVector3D& b) const
    {
        return std::abs(QVector3D::dotProduct(a, b)) < 1e-3f;
    }

    bool mMotionJointPanel::validateUniversalVectors() const
    {
        if (!m_motionJointData) return true;
        auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(m_motionJointData);
        if (!universalJoint) return true;

        QVector3D sA = normalizeVector(universalJoint->getActionBodySpinVector());
        QVector3D cA = normalizeVector(universalJoint->getActionBodyPinVector());
        QVector3D sB = normalizeVector(universalJoint->getBaseBodySpinVector());
        QVector3D cB = normalizeVector(universalJoint->getBaseBodyPinVector());

        bool ok = isOrthogonal(sA, cA) && isOrthogonal(sB, cB) && isOrthogonal(cA, cB);
        if (!ok) {
            mGlobalSignals::getInstance()->outputMessageSig(1, QStringLiteral("万向节轴线应正交"));
        }
        return ok;
    }

    void mMotionJointPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionJointPanel::slot_finishedPick);
    }

    void mMotionJointPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionJointPanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        //重置面板，类型根据拾取模式来确定
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionJointPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
        //if (getCVM == _currentViewportModel)
        //{
        //    return;
        //}

        ////获取当前视口的数据
        //_currentViewportModel = getCVM;
    }
}
