#include "mMotionSensorPanel.h"
#include "mMotionRefMap.h"
#include "mMotionPickerActivate.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mMotionPanelColors.h"
#include "mParameterPageLayout.h"

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
#include "mMotionJointBase.h"
#include "mMotionJointManager.h"
#include "mMotionDriveBase.h"
#include "mMotionDriveManager.h"
#include "mMotionSensorBase.h"
#include "mMotionSensorManager.h"
#include "mMotionBodySensor.h"
#include "mMotionJointSensor.h"
#include "mMotionDriveSensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionSensorPanel::mMotionSensorPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_sensorData = nullptr;

        m_mainLayout = new QVBoxLayout(this);

        // ==================== 1. 基础信息层 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);

        m_cmbSensorType = new QComboBox();
        m_cmbSensorType->addItems({
            "体位移", "体速度", "体加速度",
            "关节反力",
            "驱动输出",
            "距离",
            "角度"
            });
        foldBasic->addRow(new PanelLabel("传感器类型:"), m_cmbSensorType);

        m_mainLayout->addWidget(foldBasic);

        // ==================== 2. 通用参数层 ====================
        FoldSection* foldCommon = new FoldSection("通用参数", this);

        m_cmbCoordSystem = new QComboBox();
        m_cmbCoordSystem->addItems({ "全局坐标系", "局部坐标系" });
        foldCommon->addRow(new PanelLabel("坐标系:"), m_cmbCoordSystem);

        m_cmbComponentMode = new QComboBox();
        m_cmbComponentMode->addItems({ "X分量", "Y分量", "Z分量", "模值", "全部" });
        foldCommon->addRow(new PanelLabel("分量模式:"), m_cmbComponentMode);

        m_mainLayout->addWidget(foldCommon);

        // ==================== 3. 类型特定参数层 ====================
        auto* specificParameters = new QWidget(this);
        m_stkSpecific = new ParameterPageLayout(specificParameters);

        // --- 页面 0: 体状态传感器 ---
        m_pageBody = new QWidget();
        QVBoxLayout* layBody = new QVBoxLayout(m_pageBody);
        layBody->setContentsMargins(0, 0, 0, 0);
        layBody->setSpacing(0);

        FoldSection* foldBodyTarget = new FoldSection("目标体", m_pageBody);
        m_cmbTargetBody = new QComboBox();
        foldBodyTarget->addRow(new PanelLabel("目标体:"), m_cmbTargetBody);
        // 拾取/输入为世界坐标，入库存为体局部坐标（= 世界 - 质心），与求解器装配约定一致
        m_refPointPicker = new mPointPickerWidget(QStringLiteral("参考点"), this);
        m_refPointPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        foldBodyTarget->addRow(m_refPointPicker);
        m_cmbBodyVariable = new QComboBox();
        // 项按传感器类型动态填充（见 populateBodyVariableCombo）
        foldBodyTarget->addRow(new PanelLabel("变量:"), m_cmbBodyVariable);
        layBody->addWidget(foldBodyTarget);
        layBody->addStretch();
        m_stkSpecific->addWidget(m_pageBody);

        // --- 页面 1: 关节反力传感器 ---
        m_pageJoint = new QWidget();
        QVBoxLayout* layJoint = new QVBoxLayout(m_pageJoint);
        layJoint->setContentsMargins(0, 0, 0, 0);
        layJoint->setSpacing(0);

        FoldSection* foldJointTarget = new FoldSection("目标关节", m_pageJoint);
        m_cmbTargetJoint = new QComboBox();
        foldJointTarget->addRow(new PanelLabel("目标关节:"), m_cmbTargetJoint);
        m_cmbJointVariable = new QComboBox();
        m_cmbJointVariable->addItems({ "约束反力", "约束反力矩" });
        foldJointTarget->addRow(new PanelLabel("变量:"), m_cmbJointVariable);
        layJoint->addWidget(foldJointTarget);
        layJoint->addStretch();
        m_stkSpecific->addWidget(m_pageJoint);

        // --- 页面 2: 驱动传感器 ---
        m_pageDrive = new QWidget();
        QVBoxLayout* layDrive = new QVBoxLayout(m_pageDrive);
        layDrive->setContentsMargins(0, 0, 0, 0);
        layDrive->setSpacing(0);

        FoldSection* foldDriveTarget = new FoldSection("目标驱动", m_pageDrive);
        m_cmbTargetDrive = new QComboBox();
        foldDriveTarget->addRow(new PanelLabel("目标驱动:"), m_cmbTargetDrive);
        m_cmbDriveVariable = new QComboBox();
        m_cmbDriveVariable->addItems({ "驱动力", "驱动力矩", "驱动位移", "驱动速度", "驱动加速度", "约束反力", "约束反力矩" });
        foldDriveTarget->addRow(new PanelLabel("变量:"), m_cmbDriveVariable);
        layDrive->addWidget(foldDriveTarget);
        layDrive->addStretch();
        m_stkSpecific->addWidget(m_pageDrive);

        // --- 页面 3: 距离传感器 ---
        m_pageDistance = new QWidget();
        QVBoxLayout* layDistance = new QVBoxLayout(m_pageDistance);
        layDistance->setContentsMargins(0, 0, 0, 0);
        layDistance->setSpacing(0);

        FoldSection* foldDistPoints = new FoldSection("测量点", m_pageDistance);
        m_cmbPointABody = new QComboBox();
        foldDistPoints->addRow(new PanelLabel("点A所属体:"), m_cmbPointABody);
        m_pointAPicker = new mPointPickerWidget(QStringLiteral("点A坐标"), this);
        m_pointAPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        foldDistPoints->addRow(m_pointAPicker);
        m_cmbPointBBody = new QComboBox();
        foldDistPoints->addRow(new PanelLabel("点B所属体:"), m_cmbPointBBody);
        m_pointBPicker = new mPointPickerWidget(QStringLiteral("点B坐标"), this);
        m_pointBPicker->pickController()->setAllowedTypes(&MechCommon::pickGeoPointEdgeFaceSolidNodePart);
        foldDistPoints->addRow(m_pointBPicker);
        m_cmbDistanceVariable = new QComboBox();
        m_cmbDistanceVariable->addItems({ "距离", "相对速度", "相对加速度" });
        foldDistPoints->addRow(new PanelLabel("变量:"), m_cmbDistanceVariable);
        layDistance->addWidget(foldDistPoints);
        layDistance->addStretch();
        m_stkSpecific->addWidget(m_pageDistance);

        // --- 页面 4: 角度传感器 ---
        m_pageAngle = new QWidget();
        QVBoxLayout* layAngle = new QVBoxLayout(m_pageAngle);
        layAngle->setContentsMargins(0, 0, 0, 0);
        layAngle->setSpacing(0);

        FoldSection* foldAngleRef = new FoldSection("参考轴", m_pageAngle);
        m_cmbAxisBody = new QComboBox();
        foldAngleRef->addRow(new PanelLabel("轴参考体:"), m_cmbAxisBody);
        m_axisDirSelector = new mDirectionSelectorWidget(
            QStringLiteral("轴方向"), false, QString(), false, QString(), false, this);
        foldAngleRef->addRow(m_axisDirSelector);
        layAngle->addWidget(foldAngleRef);

        FoldSection* foldAngleBodies = new FoldSection("测量体", m_pageAngle);
        m_cmbBodyA = new QComboBox();
        foldAngleBodies->addRow(new PanelLabel("体A:"), m_cmbBodyA);
        m_cmbBodyB = new QComboBox();
        foldAngleBodies->addRow(new PanelLabel("体B:"), m_cmbBodyB);
        layAngle->addWidget(foldAngleBodies);
        layAngle->addStretch();
        m_stkSpecific->addWidget(m_pageAngle);

        m_mainLayout->addWidget(specificParameters);

        // ==================== Lambda 信号绑定 ====================

        // 传感器类型切换（真正切换数据对象类型并自动命名）
        connect(m_cmbSensorType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            SensorType newType = static_cast<SensorType>(index);
            handleTypeChange(newType);
            });

        // 坐标系
        connect(m_cmbCoordSystem, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_sensorData) m_sensorData->setCoordSystem(static_cast<CoordSystem>(index));
            });

        // 分量模式
        connect(m_cmbComponentMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_sensorData) m_sensorData->setComponentMode(static_cast<ComponentMode>(index));
            });

        // 体状态传感器 - 目标体
        connect(m_cmbTargetBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbTargetBody->itemData(index).toInt();
            auto bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData);
            if (bodySensor) mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_None, bodyId);
            refreshBodyHighlight();
            refreshPointRender();
                        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
            });

        // 体状态传感器 - 参考点（控件值为世界坐标，入库转为体局部坐标）
        connect(m_refPointPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!m_sensorData) return;
            auto bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData);
            if (bodySensor) {
                bodySensor->setReferencePoint(
                    worldToBodyLocal(m_cmbTargetBody->currentData().toInt(), m_refPointPicker->coordinate()));
            }
            refreshPointRender();
            });

        // 体状态传感器 - 变量
        connect(m_cmbBodyVariable, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            auto bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData);
            if (bodySensor)
                bodySensor->setVariable(static_cast<BodySensorVariable>(
                    m_cmbBodyVariable->itemData(index).toInt()));
            });

        // 关节传感器 - 目标关节
        connect(m_cmbTargetJoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int jointId = m_cmbTargetJoint->itemData(index).toInt();
            auto jointSensor = dynamic_cast<mMotionJointSensor*>(m_sensorData);
            mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionJoint, MapRelation_None, jointId);
            });

        // 关节传感器 - 变量
        connect(m_cmbJointVariable, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            auto jointSensor = dynamic_cast<mMotionJointSensor*>(m_sensorData);
            if (jointSensor) jointSensor->setVariable(static_cast<JointSensorVariable>(index));
            });

        // 驱动传感器 - 目标驱动
        connect(m_cmbTargetDrive, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int driveId = m_cmbTargetDrive->itemData(index).toInt();
            auto driveSensor = dynamic_cast<mMotionDriveSensor*>(m_sensorData);
            if (driveSensor) mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionDrive, MapRelation_None, driveId);
            });

        // 驱动传感器 - 变量
        connect(m_cmbDriveVariable, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            auto driveSensor = dynamic_cast<mMotionDriveSensor*>(m_sensorData);
            if (driveSensor) driveSensor->setVariable(static_cast<DriveSensorVariable>(index));
            });

        // 距离传感器 - 点A
        connect(m_cmbPointABody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbPointABody->itemData(index).toInt();
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (distSensor) {
                auto pt = distSensor->getPointA();
                pt.bodyId = bodyId;
                distSensor->setPointA(pt);
            }
            refreshBodyHighlight();
                        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
            });
        // 距离传感器 - 点A坐标（控件值为世界坐标，入库转为体局部坐标）
        connect(m_pointAPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!m_sensorData) return;
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (distSensor) {
                auto ref = distSensor->getPointA();
                ref.localPos = worldToBodyLocal(m_cmbPointABody->currentData().toInt(), m_pointAPicker->coordinate());
                distSensor->setPointA(ref);
            }
            refreshPointRender();
            });

        // 距离传感器 - 点B
        connect(m_cmbPointBBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbPointBBody->itemData(index).toInt();
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (distSensor) {
                auto pt = distSensor->getPointB();
                pt.bodyId = bodyId;
                distSensor->setPointB(pt);
            }
            refreshBodyHighlight();
                        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
            });
        // 距离传感器 - 点B坐标（控件值为世界坐标，入库转为体局部坐标）
        connect(m_pointBPicker, &mPointPickerWidget::coordinateChanged, this, [this]() {
            if (!m_sensorData) return;
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (distSensor) {
                auto ref = distSensor->getPointB();
                ref.localPos = worldToBodyLocal(m_cmbPointBBody->currentData().toInt(), m_pointBPicker->coordinate());
                distSensor->setPointB(ref);
            }
            refreshPointRender();
            });

        // 距离传感器 - 变量
        connect(m_cmbDistanceVariable, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (distSensor) distSensor->setVariable(static_cast<DistanceVariable>(index));
            });

        // 角度传感器 - 轴参考体
        connect(m_cmbAxisBody, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbAxisBody->itemData(index).toInt();
            auto angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData);
            if (angleSensor) mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionAxisBody, bodyId);
            });
        // 角度传感器 - 轴方向（方向控件）
        connect(m_axisDirSelector, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            if (!m_sensorData) return;
            auto angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData);
            if (angleSensor) {
                QVariant dv = m_axisDirSelector->getDirection();
                if (dv.isValid() && !dv.isNull()) {
                    angleSensor->setAxisDirection(dv.value<QVector3D>());
                    saveDirPickState(m_axisDirSelector, angleSensor->axisDirPickState_);
                }
            }
            });
        connect(m_axisDirSelector, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (m_mainLayout)
                m_mainLayout->invalidate();
            });

        // 角度传感器 - 体A
        connect(m_cmbBodyA, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbBodyA->itemData(index).toInt();
            auto angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData);
            if (angleSensor) mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyA, bodyId);
            refreshBodyHighlight();
                        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
            });

        // 角度传感器 - 体B
        connect(m_cmbBodyB, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_sensorData) return;
            int bodyId = m_cmbBodyB->itemData(index).toInt();
            auto angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData);
            if (angleSensor) mMotionRefMap::set(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyB, bodyId);
            refreshBodyHighlight();
                        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
            });

        m_mainLayout->addStretch();
    }

    mMotionSensorPanel::~mMotionSensorPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    QString mMotionSensorPanel::getTypeName(SensorType type) const
    {
        switch (type) {
        case SensorType::BodyDisplacement: return "体位移传感器";
        case SensorType::BodyVelocity:     return "体速度传感器";
        case SensorType::BodyAcceleration: return "体加速度传感器";
        case SensorType::JointReaction:    return "关节反力传感器";
        case SensorType::DriveActuation:   return "驱动输出传感器";
        case SensorType::Distance:         return "距离传感器";
        case SensorType::Angle:            return "角度传感器";
        default:                           return "传感器";
        }
    }

    void mMotionSensorPanel::handleTypeChange(SensorType newType)
    {
        if (!m_sensorData || m_sensorData->getType() == newType) return;

        SensorType oldType = m_sensorData->getType();
        QString name = "";
        if (m_sensorData->getIsUserRename())
        {
            name = m_sensorData->getName();
        }
        else
        {
            QStringList list = m_sensorData->getName().split("_");
            if (list.size() >= 1)
            {
                name = getTypeName(newType);
                if (list.size() == 2)
                {
                    name = QString("%1_%2").arg(name).arg(list.at(1));
                }
            }
            else
            {
                name = QString("%1_%2").arg(getTypeName(newType)).arg(m_sensorData->getId());
            }
            if (getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).existName(name))
            {
                name = getCVM->getModelData()->createNameByType(Physical_MotionSensor, getTypeName(newType));
            }
        }

        // 判断是否同构（体位移/速度/加速度共用 mMotionBodySensor）
        auto getClassGroup = [](SensorType t) {
            if (t == SensorType::BodyDisplacement || t == SensorType::BodyVelocity || t == SensorType::BodyAcceleration)
                return 0;
            if (t == SensorType::JointReaction) return 1;
            if (t == SensorType::DriveActuation) return 2;
            if (t == SensorType::Distance) return 3;
            if (t == SensorType::Angle) return 4;
            return -1;
            };

        if (getClassGroup(oldType) == getClassGroup(newType))
        {
            m_sensorData->setType(newType);
            m_sensorData->setName(name, m_sensorData->getIsUserRename());

            // 体三类互切：变量重置为新类型的首个合法变量并重建变量下拉
            if (auto* bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData)) {
                const auto allowed = allowedBodyVariables(newType);
                if (!allowed.isEmpty())
                    bodySensor->setVariable(allowed.first());
                populateBodyVariableCombo(newType);
            }
        }
        else
        {
            int id = m_sensorData->getId();
            bool isUserRename = m_sensorData->getIsUserRename();
            getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).deleteData(id);

            switch (newType) {
            case SensorType::BodyDisplacement:
            case SensorType::BodyVelocity:
            case SensorType::BodyAcceleration:
                m_sensorData = new mMotionBodySensor(id, newType, name, isUserRename);
                break;
            case SensorType::JointReaction:
                m_sensorData = new mMotionJointSensor(id, newType, name, isUserRename);
                break;
            case SensorType::DriveActuation:
                m_sensorData = new mMotionDriveSensor(id, newType, name, isUserRename);
                break;
            case SensorType::Distance:
                m_sensorData = new mMotionDistanceSensor(id, newType, name, isUserRename);
                break;
            case SensorType::Angle:
                m_sensorData = new mMotionAngleSensor(id, newType, name, isUserRename);
                break;
            default:
                m_sensorData = new mMotionBodySensor(id, SensorType::BodyDisplacement, name, isUserRename);
                break;
            }
            getCVM->getModelData()->addPhysicalField<mMotionSensorBase>(m_sensorData);
        }

        // 完整恢复控件状态并刷新视口高亮/标记点（清理旧类型的残留渲染）
        updateUIFromData();

        //页面切换后刷新拾取激活状态
        activateFirstVisiblePointPicker({ m_refPointPicker, m_pointAPicker, m_pointBPicker },
            { m_axisDirSelector });

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(false, m_sensorData->getId());
    }

    void mMotionSensorPanel::populateBodyVariableCombo(MechMotionData::SensorType type)
    {
        const QSignalBlocker blocker(m_cmbBodyVariable);
        m_cmbBodyVariable->clear();
        const auto allowed = allowedBodyVariables(type);
        for (const auto var : allowed) {
            m_cmbBodyVariable->addItem(getBodySensorVariableName(var), static_cast<int>(var));
        }
    }

    void mMotionSensorPanel::refreshBodyHighlight()
    {
        if (!m_sensorData || !getCVM || !getCVM->getModelData())
            return;

        // 按类型取关联体：体传感器目标体 → 红；距离/角度的 A/B 体 → 红/蓝（大地 -1 不高亮）
        int idA = -1, idB = -1;
        switch (m_sensorData->getType()) {
        case SensorType::BodyDisplacement:
        case SensorType::BodyVelocity:
        case SensorType::BodyAcceleration:
            if (auto* bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData))
                idA = mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody);
            break;
        case SensorType::Distance:
            if (auto* distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData)) {
                idA = distSensor->getPointA().bodyId;
                idB = distSensor->getPointB().bodyId;
            }
            break;
        case SensorType::Angle:
            if (auto* angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData)) {
                idA = mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyA);
                idB = mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyB);
            }
            break;
        default:
            break;
        }

        // 两个槽位都无条件刷新：无体的一侧清除，避免类型切换后残留旧高亮
        const QColor colorA = mMotionPanelColors::bodyA();
        const QColor colorB = mMotionPanelColors::bodyB();
        highlightBodyById(idA, "red", colorA.red(), colorA.green(), colorA.blue());
        highlightBodyById(idB, "blue", colorB.red(), colorB.green(), colorB.blue());

        // 高亮渲染更新后需要主动触发视口重绘
        if (getCVM->getPreRenderWidget())
            getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionSensorPanel::highlightBodyById(int bodyId, const std::string& key, int r, int g, int b)
    {
        if (!getCVM || !getCVM->getModelData() || !getCVM->getPreRenderWidget())
            return;

        auto highlight_controller = getCVM->getPreRenderWidget()->getHighlightController();
        if (!highlight_controller)
            return;
        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(key);
        if (!geoHighLightRender)
            return;

        auto* bodyData = getCVM->getModelData()->getManager<MechMotionData::mMotionBodyManager>(
            Physical_MotionBody).getData(bodyId);
        if (bodyData)
        {
            geoHighLightRender->setHighLightColor(r, g, b);
            mGeoSelectData* geoData = new mGeoSelectData();
            auto hlParts = mMotionRefMap::getSet(Physical_MotionBody, bodyData->getId(), Physical_Part);
            geoData->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPart,
                std::vector<int>(hlParts.begin(), hlParts.end()));
            geoHighLightRender->updateHighLightRender(geoData, HighStyle::PartALLFeatures);
        }
        else
        {
            // 无有效体时清除该槽位高亮
            mxr::mGeoSelectData geoData;
            geoData.UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(&geoData);
        }
    }

    // 体局部 <-> 世界坐标换算：本系统体局部系原点=质心、姿态与世界系平行（与求解器装配约定一致），
    // 故换算只做质心平移。大地（bodyId<0）时该点本身就是世界坐标，原样返回。
    QVector3D mMotionSensorPanel::worldToBodyLocal(int bodyId, const QVector3D& world) const
    {
        if (bodyId < 0)
            return world;
        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        auto* body = bodyManager.getData(bodyId);
        if (!body)
            return world;
        return world - body->getCenterOfMass();
    }

    QVector3D mMotionSensorPanel::bodyLocalToWorld(int bodyId, const QVector3D& local) const
    {
        if (bodyId < 0)
            return local;
        auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
        auto* body = bodyManager.getData(bodyId);
        if (!body)
            return local;
        return local + body->getCenterOfMass();
    }

    void mMotionSensorPanel::refreshPointRender()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
        if (!render)
            return;

        // 先清掉本面板的旧点
        render->deletePoint("sensorRefPoint");
        render->deletePoint("sensorPointA");
        render->deletePoint("sensorPointB");

        if (m_sensorData) {
            // 体上的点以局部坐标存储，渲染时转回世界（+质心）；大地点本身就是世界坐标
            switch (m_sensorData->getType()) {
            case SensorType::BodyDisplacement:
            case SensorType::BodyVelocity:
            case SensorType::BodyAcceleration:
                if (auto* bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData)) {
                    if (mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody) >= 0)
                        render->appendPoint("sensorRefPoint",
                            { bodyLocalToWorld(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody), bodySensor->getReferencePoint()) },
                            mMotionPanelColors::vec(mMotionPanelColors::pointA()));
                }
                break;
            case SensorType::Distance:
                if (auto* distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData)) {
                    render->appendPoint("sensorPointA",
                        { bodyLocalToWorld(distSensor->getPointA().bodyId, distSensor->getPointA().localPos) },
                        mMotionPanelColors::vec(mMotionPanelColors::pointA()));
                    render->appendPoint("sensorPointB",
                        { bodyLocalToWorld(distSensor->getPointB().bodyId, distSensor->getPointB().localPos) },
                        mMotionPanelColors::vec(mMotionPanelColors::pointB()));
                }
                break;
            default:
                break;
            }
        }

        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionSensorPanel::newMotionSensorPanel(SensorType type)
    {
        setEditing(true);
        connectSig();

        int sensorID = getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).createID();

        QString defaultName = getTypeName(type);

        switch (type) {
        case SensorType::BodyDisplacement:
        case SensorType::BodyVelocity:
        case SensorType::BodyAcceleration:
            m_sensorData = new mMotionBodySensor(sensorID, type);
            break;
        case SensorType::JointReaction:
            m_sensorData = new mMotionJointSensor(sensorID, type);
            break;
        case SensorType::DriveActuation:
            m_sensorData = new mMotionDriveSensor(sensorID, type);
            break;
        case SensorType::Distance:
            m_sensorData = new mMotionDistanceSensor(sensorID, type);
            break;
        case SensorType::Angle:
            m_sensorData = new mMotionAngleSensor(sensorID, type);
            break;
        default:
            m_sensorData = new mMotionBodySensor(sensorID, SensorType::BodyDisplacement);
            break;
        }

        m_sensorData->setName(getCVM->getModelData()->createNameByType(Physical_MotionSensor, defaultName));
        getCVM->getModelData()->addPhysicalField<mMotionSensorBase>(m_sensorData);

        //新建对象：清掉方向控件里上一个对象残留的拾取会话
        restoreDirPickState(m_axisDirSelector, DirectionPickState(), QVector3D());

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionSensorSig(true, m_sensorData->getId());
    }

    void mMotionSensorPanel::editMotionSensorPanel(int id)
    {
        setEditing(true);
        m_sensorData = getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor).getData(id);
        if (!m_sensorData)
            return;

        connectSig();
        //进入编辑反显数据值（方向控件拾取会话模式保留）
        updateUIFromData();

        //进入编辑激活当前页第一个可见点拾取控件（距离页为点A，体状态页为参考点）
        activateFirstVisiblePointPicker({ m_refPointPicker, m_pointAPicker, m_pointBPicker },
            { m_axisDirSelector });

        //恢复拾取会话：延迟到事件循环下一拍（避开树点击链路后续清理），详见Joint面板注释
        QTimer::singleShot(0, this, [this]() {
            if (!m_sensorData)
                return;
            restorePickSessionsOnEntry({ m_refPointPicker, m_pointAPicker, m_pointBPicker },
                { m_axisDirSelector });
            });
    }

    void mMotionSensorPanel::clearViewportRender()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
        if (render) {
            render->deletePoint("sensorRefPoint");
            render->deletePoint("sensorPointA");
            render->deletePoint("sensorPointB");
        }
        highlightBodyById(-1, "red", 0, 0, 0);
        highlightBodyById(-1, "blue", 0, 0, 0);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionSensorPanel::onMotionSensorDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_sensorData == nullptr || id != m_sensorData->getId())
                return;
        }
        m_sensorData = nullptr;
    }

    void mMotionSensorPanel::uneditMotionSensorPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_sensorData = nullptr;
        //退出编辑时失活所有拾取控件并清理高亮
        for (mPointPickerWidget* picker : { m_refPointPicker, m_pointAPicker, m_pointBPicker })
        {
            if (picker && picker->pickController())
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
                //拾取数据是临时的（确定值已入数据类），退出编辑清空防残留高亮
                picker->pickController()->displayData(PickWidgetData());
            }
        }
        if (m_axisDirSelector)
        {
            const QList<mPickWidgetController*> controllers = {
                m_axisDirSelector->startPickController(),
                m_axisDirSelector->endPickController(),
                m_axisDirSelector->objectPickController() };
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
        clearViewportRender();
    }

    void mMotionSensorPanel::refreshBodyList()
    {
        auto fillBodyCombo = [this](QComboBox* cmb) {
            const QSignalBlocker b(cmb);
            cmb->clear();
            cmb->addItem(" ", -1);
            auto& bodyManager = getCVM->getModelData()->getManager<mMotionBodyManager>(Physical_MotionBody);
            std::vector<int> allIds = bodyManager.getAllIDs();
            for (int id : allIds) {
                QString name = bodyManager.getNameByID(id);
                cmb->addItem(name, id);
            }
            };
        fillBodyCombo(m_cmbTargetBody);
        fillBodyCombo(m_cmbPointABody);
        fillBodyCombo(m_cmbPointBBody);
        fillBodyCombo(m_cmbAxisBody);
        fillBodyCombo(m_cmbBodyA);
        fillBodyCombo(m_cmbBodyB);
    }

    void mMotionSensorPanel::refreshJointList()
    {
        const QSignalBlocker b(m_cmbTargetJoint);
        m_cmbTargetJoint->clear();
        m_cmbTargetJoint->addItem(" ", -1);
        auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
        std::vector<int> allIds = jointManager.getAllIDs();
        for (int id : allIds) {
            QString name = jointManager.getNameByID(id);
            m_cmbTargetJoint->addItem(name, id);
        }
    }

    void mMotionSensorPanel::refreshDriveList()
    {
        const QSignalBlocker b(m_cmbTargetDrive);
        m_cmbTargetDrive->clear();
        m_cmbTargetDrive->addItem(" ", -1);
        auto& driveManager = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive);
        std::vector<int> allIds = driveManager.getAllIDs();
        for (int id : allIds) {
            QString name = driveManager.getNameByID(id);
            m_cmbTargetDrive->addItem(name, id);
        }
    }

    void mMotionSensorPanel::refreshUIBySensorType()
    {
        if (!m_sensorData) return;
        SensorType type = m_sensorData->getType();
        int pageIndex = 0;
        switch (type) {
        case SensorType::BodyDisplacement:
        case SensorType::BodyVelocity:
        case SensorType::BodyAcceleration:
            pageIndex = 0; break;
        case SensorType::JointReaction:
            pageIndex = 1; break;
        case SensorType::DriveActuation:
            pageIndex = 2; break;
        case SensorType::Distance:
            pageIndex = 3; break;
        case SensorType::Angle:
            pageIndex = 4; break;
        }
        m_stkSpecific->setCurrentIndex(pageIndex);

        refreshBodyList();
        refreshJointList();
        refreshDriveList();
    }

    void mMotionSensorPanel::updateUIFromData()
    {
        if (!getCVM || !m_sensorData)
            return;

        refreshUIBySensorType();

        // 基础信息
        SensorType type = m_sensorData->getType();
        const QSignalBlocker bType(m_cmbSensorType);
        m_cmbSensorType->setCurrentIndex(static_cast<int>(type));

        // 通用参数
        const QSignalBlocker bCoord(m_cmbCoordSystem);
        m_cmbCoordSystem->setCurrentIndex(static_cast<int>(m_sensorData->getCoordSystem()));

        const QSignalBlocker bComp(m_cmbComponentMode);
        m_cmbComponentMode->setCurrentIndex(static_cast<int>(m_sensorData->getComponentMode()));

        // 类型特定参数
        switch (type) {
        case SensorType::BodyDisplacement:
        case SensorType::BodyVelocity:
        case SensorType::BodyAcceleration:
        {
            auto bodySensor = dynamic_cast<mMotionBodySensor*>(m_sensorData);
            if (!bodySensor) break;

            int bodyIndex = m_cmbTargetBody->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody));
            const QSignalBlocker b(m_cmbTargetBody);
            m_cmbTargetBody->setCurrentIndex(bodyIndex != -1 ? bodyIndex : 0);

            // 体局部坐标转世界坐标反显（与拾取入库路径互逆）
            m_refPointPicker->setCoordinate(
                bodyLocalToWorld(m_cmbTargetBody->currentData().toInt(), bodySensor->getReferencePoint()));

            const QSignalBlocker bVar(m_cmbBodyVariable);
            populateBodyVariableCombo(type);
            const int varIdx = m_cmbBodyVariable->findData(static_cast<int>(bodySensor->getVariable()));
            m_cmbBodyVariable->setCurrentIndex(varIdx != -1 ? varIdx : 0);
            break;
        }
        case SensorType::JointReaction:
        {
            auto jointSensor = dynamic_cast<mMotionJointSensor*>(m_sensorData);
            if (!jointSensor) break;

            int jointIndex = m_cmbTargetJoint->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionJoint));
            const QSignalBlocker b(m_cmbTargetJoint);
            m_cmbTargetJoint->setCurrentIndex(jointIndex != -1 ? jointIndex : 0);

            const QSignalBlocker bVar(m_cmbJointVariable);
            m_cmbJointVariable->setCurrentIndex(static_cast<int>(jointSensor->getVariable()));
            break;
        }
        case SensorType::DriveActuation:
        {
            auto driveSensor = dynamic_cast<mMotionDriveSensor*>(m_sensorData);
            if (!driveSensor) break;

            int driveIndex = m_cmbTargetDrive->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionDrive));
            const QSignalBlocker b(m_cmbTargetDrive);
            m_cmbTargetDrive->setCurrentIndex(driveIndex != -1 ? driveIndex : 0);

            const QSignalBlocker bVar(m_cmbDriveVariable);
            m_cmbDriveVariable->setCurrentIndex(static_cast<int>(driveSensor->getVariable()));
            break;
        }
        case SensorType::Distance:
        {
            auto distSensor = dynamic_cast<mMotionDistanceSensor*>(m_sensorData);
            if (!distSensor) break;

            auto ptA = distSensor->getPointA();
            int aIndex = m_cmbPointABody->findData(ptA.bodyId);
            const QSignalBlocker ba(m_cmbPointABody);
            m_cmbPointABody->setCurrentIndex(aIndex != -1 ? aIndex : 0);
            m_pointAPicker->setCoordinate(bodyLocalToWorld(ptA.bodyId, ptA.localPos));

            auto ptB = distSensor->getPointB();
            int bIndex = m_cmbPointBBody->findData(ptB.bodyId);
            const QSignalBlocker bb(m_cmbPointBBody);
            m_cmbPointBBody->setCurrentIndex(bIndex != -1 ? bIndex : 0);
            m_pointBPicker->setCoordinate(bodyLocalToWorld(ptB.bodyId, ptB.localPos));

            const QSignalBlocker bVar(m_cmbDistanceVariable);
            m_cmbDistanceVariable->setCurrentIndex(static_cast<int>(distSensor->getVariable()));
            break;
        }
        case SensorType::Angle:
        {
            auto angleSensor = dynamic_cast<mMotionAngleSensor*>(m_sensorData);
            if (!angleSensor) break;

            int axisIndex = m_cmbAxisBody->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
            const QSignalBlocker b(m_cmbAxisBody);
            m_cmbAxisBody->setCurrentIndex(axisIndex != -1 ? axisIndex : 0);

            // 轴方向反显：走数据类里的拾取会话状态（照集中力 dirPickState_ 模式），
            // 无记录（新建/历史数据）回"向量"并写入数据值
            restoreDirPickState(m_axisDirSelector, angleSensor->axisDirPickState_, angleSensor->getAxisDirection());

            int aIndex = m_cmbBodyA->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyA));
            const QSignalBlocker ba(m_cmbBodyA);
            m_cmbBodyA->setCurrentIndex(aIndex != -1 ? aIndex : 0);

            int bIndex = m_cmbBodyB->findData(mMotionRefMap::get(Physical_MotionSensor, m_sensorData->getId(), Physical_MotionBody, MapRelation_MotionBodyB));
            const QSignalBlocker bb(m_cmbBodyB);
            m_cmbBodyB->setCurrentIndex(bIndex != -1 ? bIndex : 0);
            break;
        }
        }

        refreshBodyHighlight();
        refreshPointRender();

    }

    void mMotionSensorPanel::connectSig()
    {
        disconnectSig();
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }

        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mMotionSensorPanel::slot_finishedPick);
    }

    void mMotionSensorPanel::disconnectSig()
    {
        if (conn_geo)
            disconnect(conn_geo);
    }

    void mMotionSensorPanel::slot_finishedPick()
    {
        if (!getCVM)
        {
            return;
        }
        int pickNum = 0;
        auto pickFilter = getCVM->getCurrentPickDataType(pickNum);
        if (pickFilter == PickDataType::None)
        {
            return;
        }
    }

    void mMotionSensorPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        if (!getCVM)
        {
            return;
        }
    }

}
