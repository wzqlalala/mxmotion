#include "mMotionCouplingPanel.h"
#include "mMotionRefMap.h"
#include "mGlobalSignals.h"
#include "mPhysicalFieldEnum.h"
#include "mAppConfiguration.h"
#include "mMotionJointSymbolHelper.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/label/SymbolDescription.h"
#include "mxr/basic/PublicStruct.h"
#include "mxr/view/CCSMRendWidget.h"

//STL
#include <vector>

//BasicData
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointScrew.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionJointManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingCable.h"
#include "mMotionCouplingManager.h"
#include "mModelData.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionCouplingPanel::mMotionCouplingPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_motionCouplingData = nullptr;
        m_lastFirstJointId = -1;
        m_lastSecondJointId = -1;

        // 获取当前视口的数据
        m_mainLayout = new QVBoxLayout(this);

        auto makeSpinBox = [](double min, double max, int decimals, const QString& suffix) {
            QDoubleSpinBox* spb = new QDoubleSpinBox();
            spb->setRange(min, max);
            spb->setDecimals(decimals);
            spb->setButtonSymbols(QAbstractSpinBox::NoButtons);
            if (!suffix.isEmpty()) spb->setSuffix(suffix);
            return spb;
            };

        auto makeIntSpinBox = [](int min, int max, const QString& suffix) {
            QSpinBox* spb = new QSpinBox();
            spb->setRange(min, max);
            spb->setButtonSymbols(QAbstractSpinBox::NoButtons);
            if (!suffix.isEmpty()) spb->setSuffix(suffix);
            return spb;
            };

        // 1. 基础信息层：类型标签
        m_lblCouplingType = new QLabel("耦合副类型:");
        m_mainLayout->addWidget(m_lblCouplingType);

        // 2. 运动副 1 折叠框
        m_foldFirstJoint = new FoldSection("运动副 1", this);
        m_cmbFirstJoint = new QComboBox();
        m_foldFirstJoint->addRow(new QLabel("选择运动副:"), m_cmbFirstJoint);
        m_mainLayout->addWidget(m_foldFirstJoint);
        m_foldWidgets.append(m_foldFirstJoint);

        // 3. 运动副 2 折叠框
        m_foldSecondJoint = new FoldSection("运动副 2", this);
        m_cmbSecondJoint = new QComboBox();
        m_foldSecondJoint->addRow(new QLabel("选择运动副:"), m_cmbSecondJoint);
        m_mainLayout->addWidget(m_foldSecondJoint);
        m_foldWidgets.append(m_foldSecondJoint);

        // 4. 参数折叠框
        m_foldParams = new FoldSection("参数", this);

        // 齿轮齿条副参数：齿轮半径（销半径）
        m_lblGearRadius = new QLabel("齿轮半径 (R):");
        m_spbGearRadius = makeSpinBox(0, 1e12, 4, " mm");

        // 齿轮副参数：模式选择 + 传动比/齿数/分度圆半径
        m_lblGearParamMode = new QLabel("参数模式:");
        m_cmbGearParamMode = new QComboBox();
        m_cmbGearParamMode->addItem("传动比", 0);
        m_cmbGearParamMode->addItem("齿数", 1);
        m_cmbGearParamMode->addItem("分度圆半径", 2);

        m_lblGearRatio = new QLabel("传动比:");
        m_spbGearRatio = makeSpinBox(-1e6, 1e6, 4, "");
        // 传动比符号即啮合方式：正=外啮合（两轮反向），负=内啮合（两轮同向）；
        // 幅值为速比，齿数/分度圆半径始终按幅值同步
        m_lblGearRatio->setToolTip(QStringLiteral("正值 = 外啮合（两轮反向）\n负值 = 内啮合（两轮同向）\n幅值 = 速比"));
        m_spbGearRatio->setToolTip(m_lblGearRatio->toolTip());

        m_lblToothCount1 = new QLabel("齿轮 1 齿数:");
        m_spbToothCount1 = makeIntSpinBox(1, 10000, "");

        m_lblToothCount2 = new QLabel("齿轮 2 齿数:");
        m_spbToothCount2 = makeIntSpinBox(1, 10000, "");

        m_lblPitchRadius1 = new QLabel("齿轮 1 分度圆半径:");
        m_spbPitchRadius1 = makeSpinBox(0.001, 1e12, 4, " mm");

        m_lblPitchRadius2 = new QLabel("齿轮 2 分度圆半径:");
        m_spbPitchRadius2 = makeSpinBox(0.001, 1e12, 4, " mm");

        // 线缆副参数：位移比 + 反向
        m_lblCableRatio = new QLabel("位移比:");
        m_spbCableRatio = makeSpinBox(-1e6, 1e6, 6, "");
        // 位移比符号即传动方向：约束方程 (q2-q20) - ratio*(±1)*(q1-q10) = 0；
        // 幅值为两侧行程之比，反向勾选再额外乘以 -1
        m_lblCableRatio->setToolTip(QStringLiteral("第二侧滑动副行程 / 第一侧滑动副行程\n符号即传动方向；不能为 0\n可再勾选\"反向\"额外取反"));
        m_spbCableRatio->setToolTip(m_lblCableRatio->toolTip());
        m_lblCableReverse = new QLabel("反向:");
        m_chkCableReverse = new QCheckBox();

        // 默认从参数折叠框中移除，后续由 refreshParamsByCouplingType 动态添加
        m_lblGearRadius->setParent(nullptr);
        m_spbGearRadius->setParent(nullptr);

        m_lblGearParamMode->setParent(nullptr);
        m_cmbGearParamMode->setParent(nullptr);
        m_lblGearRatio->setParent(nullptr);
        m_spbGearRatio->setParent(nullptr);
        m_lblToothCount1->setParent(nullptr);
        m_spbToothCount1->setParent(nullptr);
        m_lblToothCount2->setParent(nullptr);
        m_spbToothCount2->setParent(nullptr);
        m_lblPitchRadius1->setParent(nullptr);
        m_spbPitchRadius1->setParent(nullptr);
        m_lblPitchRadius2->setParent(nullptr);
        m_spbPitchRadius2->setParent(nullptr);

        m_lblCableRatio->setParent(nullptr);
        m_spbCableRatio->setParent(nullptr);
        m_lblCableReverse->setParent(nullptr);
        m_chkCableReverse->setParent(nullptr);

        m_mainLayout->addWidget(m_foldParams);
        m_foldWidgets.append(m_foldParams);

        // ==================== Lambda 信号绑定 ====================

        // 运动副 1 切换
        connect(m_cmbFirstJoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!m_motionCouplingData) return;
            int jointId = m_cmbFirstJoint->currentData().toInt();
            mMotionRefMap::set(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint, jointId);
            triggerHighlight();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            });

        // 运动副 2 切换
        connect(m_cmbSecondJoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!m_motionCouplingData) return;
            int jointId = m_cmbSecondJoint->currentData().toInt();
            mMotionRefMap::set(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint, jointId);
            triggerHighlight();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            });

        // 齿轮半径实时同步
        connect(m_spbGearRadius, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (!m_motionCouplingData) return;
            auto* gearRack = dynamic_cast<mMotionCouplingGearRack*>(m_motionCouplingData);
            if (gearRack)
            {
                gearRack->setRadius(m_spbGearRadius->value());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        // 线缆副位移比实时同步
        connect(m_spbCableRatio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (!m_motionCouplingData) return;
            auto* cable = dynamic_cast<mMotionCouplingCable*>(m_motionCouplingData);
            if (cable)
            {
                cable->setRatio(m_spbCableRatio->value());
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        // 线缆副反向实时同步
        connect(m_chkCableReverse, &QCheckBox::toggled, this, [this](bool) {
            if (!m_motionCouplingData) return;
            auto* cable = dynamic_cast<mMotionCouplingCable*>(m_motionCouplingData);
            if (cable)
            {
                cable->setReverse(m_chkCableReverse->isChecked());
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        // 齿轮副参数模式切换
        connect(m_cmbGearParamMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            refreshParamsByCouplingType();
            });

        // 齿轮副传动比实时同步
        connect(m_spbGearRatio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (!m_motionCouplingData) return;
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear) {
                const QSignalBlocker b1(m_spbToothCount1);
                const QSignalBlocker b2(m_spbToothCount2);
                const QSignalBlocker b3(m_spbPitchRadius1);
                const QSignalBlocker b4(m_spbPitchRadius2);
                gear->setGearRatio(m_spbGearRatio->value());
                m_spbToothCount1->setValue(gear->getToothCount1());
                m_spbToothCount2->setValue(gear->getToothCount2());
                m_spbPitchRadius1->setValue(gear->getPitchRadius1());
                m_spbPitchRadius2->setValue(gear->getPitchRadius2());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        // 齿轮副齿数实时同步
        connect(m_spbToothCount1, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
            if (!m_motionCouplingData) return;
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear) {
                gear->setToothCount1(m_spbToothCount1->value());
                const QSignalBlocker b(m_spbGearRatio);
                m_spbGearRatio->setValue(gear->getGearRatio());
                m_spbToothCount2->setValue(gear->getToothCount2());
                m_spbPitchRadius1->setValue(gear->getPitchRadius1());
                m_spbPitchRadius2->setValue(gear->getPitchRadius2());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        connect(m_spbToothCount2, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
            if (!m_motionCouplingData) return;
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear) {
                gear->setToothCount2(m_spbToothCount2->value());
                const QSignalBlocker b(m_spbGearRatio);
                m_spbGearRatio->setValue(gear->getGearRatio());
                m_spbToothCount1->setValue(gear->getToothCount1());
                m_spbPitchRadius1->setValue(gear->getPitchRadius1());
                m_spbPitchRadius2->setValue(gear->getPitchRadius2());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        // 齿轮副分度圆半径实时同步
        connect(m_spbPitchRadius1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (!m_motionCouplingData) return;
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear) {
                gear->setPitchRadius1(m_spbPitchRadius1->value());
                const QSignalBlocker b(m_spbGearRatio);
                m_spbGearRatio->setValue(gear->getGearRatio());
                m_spbToothCount1->setValue(gear->getToothCount1());
                m_spbToothCount2->setValue(gear->getToothCount2());
                m_spbPitchRadius2->setValue(gear->getPitchRadius2());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        connect(m_spbPitchRadius2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (!m_motionCouplingData) return;
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear) {
                gear->setPitchRadius2(m_spbPitchRadius2->value());
                const QSignalBlocker b(m_spbGearRatio);
                m_spbGearRatio->setValue(gear->getGearRatio());
                m_spbToothCount1->setValue(gear->getToothCount1());
                m_spbToothCount2->setValue(gear->getToothCount2());
                m_spbPitchRadius1->setValue(gear->getPitchRadius1());
                triggerHighlight();
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(false, m_motionCouplingData->getId());
            }
            });

        m_mainLayout->addStretch();
    }

    mMotionCouplingPanel::~mMotionCouplingPanel()
    {
        if (m_mainLayout != nullptr)
        {
            delete m_mainLayout;
            m_mainLayout = nullptr;
        }
    }

    void mMotionCouplingPanel::newMotionCouplingPanel(MxFunctions::CouplingType type)
    {
        setEditing(true);
        connectSig();

        int couplingID = getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling).createID();
        if (type == CouplingType::GearRack)
            m_motionCouplingData = new mMotionCouplingGearRack(couplingID, type);
        else if (type == CouplingType::Gear)
            m_motionCouplingData = new mMotionCouplingGear(couplingID, type);
        else if (type == CouplingType::Cable)
            m_motionCouplingData = new mMotionCouplingCable(couplingID, type);
        else
            m_motionCouplingData = new mMotionCouplingGearRack(couplingID, type); // 默认回退到齿轮齿条副

        m_motionCouplingData->setName(getCVM->getModelData()->createNameByType(Physical_MotionCoupling, m_motionCouplingData->getTypeName()), false);
        getCVM->getModelData()->addPhysicalField<mMotionCouplingBase>(m_motionCouplingData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionCouplingSig(true, m_motionCouplingData->getId());
    }

    void mMotionCouplingPanel::editMotionCouplingPanel(int id)
    {
        setEditing(true);
        m_motionCouplingData = dynamic_cast<mMotionCouplingBase*>(getCVM->getModelData()->getManager<mMotionCouplingManager>(Physical_MotionCoupling).getData(id));
        if (!m_motionCouplingData)
            return;

        connectSig();

        refreshJointList();
        updateUIFromData();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionCouplingPanel::onMotionCouplingDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_motionCouplingData == nullptr || id != m_motionCouplingData->getId())
                return;
        }
        m_motionCouplingData = nullptr;
    }

    void mMotionCouplingPanel::uneditMotionCouplingPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        clearHighlight();
        m_lastFirstJointId = -1;
        m_lastSecondJointId = -1;
        disconnectSig();
        m_motionCouplingData = nullptr;
    }

    void mMotionCouplingPanel::refreshJointList()
    {
        if (!getCVM)
            return;

        const QSignalBlocker b1(m_cmbFirstJoint);
        const QSignalBlocker b2(m_cmbSecondJoint);
        m_cmbFirstJoint->clear();
        m_cmbSecondJoint->clear();
        m_cmbFirstJoint->addItem(" ", -1);
        m_cmbSecondJoint->addItem(" ", -1);

        auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);
        CouplingType currentType = m_motionCouplingData ? m_motionCouplingData->getType() : CouplingType::GearRack;

        QVector<JointType> firstAllowedTypes;
        QVector<JointType> secondAllowedTypes;

        if (currentType == CouplingType::GearRack)
        {
            firstAllowedTypes = { JointType::Revolute };
            secondAllowedTypes = { JointType::Translational };
        }
        else if (currentType == CouplingType::Gear)
        {
            // 齿轮副要求两个被耦合运动副均为旋转副
            firstAllowedTypes = { JointType::Revolute };
            secondAllowedTypes = { JointType::Revolute };
        }
        else if (currentType == CouplingType::Cable)
        {
            // 线缆副要求两个被耦合运动副均为移动副（内核只接受滑动副作为传动导引）
            firstAllowedTypes = { JointType::Translational };
            secondAllowedTypes = { JointType::Translational };
        }
        else
        {
            // 默认不做过滤，允许所有基本运动副
            firstAllowedTypes = { JointType::Revolute, JointType::Translational, JointType::Cylindrical,
                                  JointType::Spherical, JointType::Fixed, JointType::Universal,
                                  JointType::PointPlane, JointType::Line, JointType::Parallel,
                                  JointType::Perpendicular, JointType::Direction, JointType::Centripetal,
                                  JointType::ConstantVelocity, JointType::Hooke, JointType::Screw,
                                  JointType::Planar, JointType::Coupling, JointType::Gear, JointType::PointLine };
            secondAllowedTypes = firstAllowedTypes;
        }

        QStringList firstNames = jointManager.getAllNamesByTypes(firstAllowedTypes);
        for (const QString& name : firstNames)
        {
            m_cmbFirstJoint->addItem(name, jointManager.getIDByName(name));
        }

        QStringList secondNames = jointManager.getAllNamesByTypes(secondAllowedTypes);
        for (const QString& name : secondNames)
        {
            m_cmbSecondJoint->addItem(name, jointManager.getIDByName(name));
        }
    }

    void mMotionCouplingPanel::updateUIFromData()
    {
        if (!getCVM || !m_motionCouplingData)
            return;

        // 更新类型标签
        m_lblCouplingType->setText(QString("耦合副类型: %1").arg(m_motionCouplingData->getTypeName()));

        // 刷新运动副下拉框（会阻塞信号）
        refreshJointList();

        // 还原第一个运动副
        int firstJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
        int firstIndex = m_cmbFirstJoint->findData(firstJointId);
        {
            const QSignalBlocker b(m_cmbFirstJoint);
            m_cmbFirstJoint->setCurrentIndex(firstIndex != -1 ? firstIndex : 0);
        }

        // 还原第二个运动副
        int secondJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
        int secondIndex = m_cmbSecondJoint->findData(secondJointId);
        {
            const QSignalBlocker b(m_cmbSecondJoint);
            m_cmbSecondJoint->setCurrentIndex(secondIndex != -1 ? secondIndex : 0);
        }

        // 根据类型刷新参数折叠框内容
        refreshParamsByCouplingType();

        // 还原参数值
        if (m_motionCouplingData->getType() == CouplingType::GearRack)
        {
            auto* gearRack = dynamic_cast<mMotionCouplingGearRack*>(m_motionCouplingData);
            if (gearRack)
            {
                const QSignalBlocker b(m_spbGearRadius);
                m_spbGearRadius->setValue(gearRack->getRadius());
            }
        }
        else if (m_motionCouplingData->getType() == CouplingType::Gear)
        {
            auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
            if (gear)
            {
                const QSignalBlocker b1(m_spbGearRatio);
                const QSignalBlocker b2(m_spbToothCount1);
                const QSignalBlocker b3(m_spbToothCount2);
                const QSignalBlocker b4(m_spbPitchRadius1);
                const QSignalBlocker b5(m_spbPitchRadius2);
                m_spbGearRatio->setValue(gear->getGearRatio());
                m_spbToothCount1->setValue(gear->getToothCount1());
                m_spbToothCount2->setValue(gear->getToothCount2());
                m_spbPitchRadius1->setValue(gear->getPitchRadius1());
                m_spbPitchRadius2->setValue(gear->getPitchRadius2());
            }
        }
        else if (m_motionCouplingData->getType() == CouplingType::Cable)
        {
            auto* cable = dynamic_cast<mMotionCouplingCable*>(m_motionCouplingData);
            if (cable)
            {
                const QSignalBlocker b1(m_spbCableRatio);
                const QSignalBlocker b2(m_chkCableReverse);
                m_spbCableRatio->setValue(cable->getRatio());
                m_chkCableReverse->setChecked(cable->getReverse());
            }
        }

        triggerHighlight();
    }

    void mMotionCouplingPanel::handleTypeChange(MxFunctions::CouplingType newType)
    {
        if (!m_motionCouplingData)
            return;

        // 目前只有齿轮齿条副，直接刷新参数布局即可
        // 后续若支持多类型切换，可在此根据 newType 重建/迁移数据并调用 refreshParamsByCouplingType
        refreshParamsByCouplingType();
    }

    QString mMotionCouplingPanel::getTypeName(MxFunctions::CouplingType type) const
    {
        return mMotionCouplingBase::getTypeName(type);
    }

    void mMotionCouplingPanel::triggerHighlight()
    {
        if (!m_motionCouplingData || !getCVM || !getCVM->getPreRenderWidget())
            return;

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        // 先隐藏旧符号（使用上次记录的 ID），避免运动副选择清空后仍残留旧符号
        clearHighlight();

        auto& jointManager = getCVM->getModelData()->getManager<mMotionJointManager>(Physical_MotionJoint);

        auto showJointSymbol = [&](int jointId) {
            if (jointId == -1) return;
            auto* basicJoint = dynamic_cast<mMotionJointBasic*>(jointManager.getData(jointId));
            auto* screwJoint = dynamic_cast<mMotionJointScrew*>(jointManager.getData(jointId));
            auto* universalJoint = dynamic_cast<mMotionJointUniversal*>(jointManager.getData(jointId));
            auto* perpendicularJoint = dynamic_cast<mMotionJointPerpendicular*>(jointManager.getData(jointId));
            auto* constantVelocityJoint = dynamic_cast<mMotionJointConstantVelocity*>(jointManager.getData(jointId));
            if (!basicJoint && !screwJoint && !universalJoint && !perpendicularJoint && !constantVelocityJoint) return;

            QString symbolName = QString("motionJoint_%1").arg(jointId);
            if (symbolController->isExistSymbol(symbolName)) {
                // 符号已存在，仅恢复显示；由运动副面板创建，耦合副面板只负责借用高亮
                symbolController->setSymbolShowState({ symbolName }, true);
            }
            else if (basicJoint) {
                mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                    jointId,
                    basicJoint->getType(),
                    basicJoint->getName(),
                    basicJoint->getOrigin(),
                    basicJoint->getAxis(),
                    getCVM->getPreRenderWidget());
            }
            else if (screwJoint) {
                mMotionJointSymbolHelper::createOrUpdateJointSymbol(symbolController,
                    jointId,
                    screwJoint->getType(),
                    screwJoint->getName(),
                    screwJoint->getOrigin(),
                    screwJoint->getAxis(),
                    getCVM->getPreRenderWidget());
            }
            else if (universalJoint) {
                mMotionJointSymbolHelper::createOrUpdateUniversalSymbol(symbolController,
                    jointId,
                    universalJoint->getName(),
                    universalJoint->getOrigin(),
                    universalJoint->getActionBodySpinVector(),
                    universalJoint->getActionBodyPinVector(),
                    universalJoint->getBaseBodySpinVector(),
                    universalJoint->getBaseBodyPinVector(),
                    getCVM->getPreRenderWidget());
            }
            else if (perpendicularJoint) {
                mMotionJointSymbolHelper::createOrUpdatePerpendicularSymbol(symbolController,
                    jointId,
                    perpendicularJoint->getName(),
                    perpendicularJoint->getOrigin(),
                    perpendicularJoint->getActionBodyVector(),
                    perpendicularJoint->getBaseBodyVector(),
                    getCVM->getPreRenderWidget());
            }
            else if (constantVelocityJoint) {
                mMotionJointSymbolHelper::createOrUpdateConstantVelocitySymbol(symbolController,
                    jointId,
                    constantVelocityJoint->getName(),
                    constantVelocityJoint->getOrigin(),
                    constantVelocityJoint->getActionBodyVector(),
                    constantVelocityJoint->getBaseBodyVector(),
                    getCVM->getPreRenderWidget());
            }
            };

        showJointSymbol(mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint));
        showJointSymbol(mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint));

        // 显示耦合副自身符号
        int firstJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
        int secondJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
        if (firstJointId >= 0 && secondJointId >= 0) {
            auto* firstJoint = dynamic_cast<mMotionJointBasic*>(jointManager.getData(firstJointId));
            auto* secondJoint = dynamic_cast<mMotionJointBasic*>(jointManager.getData(secondJointId));
            if (firstJoint && secondJoint) {
                double ratio = 1.0;
                if (m_motionCouplingData->getType() == CouplingType::Gear) {
                    auto* gear = dynamic_cast<mMotionCouplingGear*>(m_motionCouplingData);
                    if (gear) ratio = gear->getGearRatio();
                }
                else if (m_motionCouplingData->getType() == CouplingType::GearRack) {
                    auto* gearRack = dynamic_cast<mMotionCouplingGearRack*>(m_motionCouplingData);
                    if (gearRack) ratio = gearRack->getRadius();
                }
                mMotionJointSymbolHelper::createOrUpdateCouplingSymbol(symbolController,
                    m_motionCouplingData->getId(),
                    m_motionCouplingData->getType(),
                    firstJoint->getOrigin(),
                    firstJoint->getAxis(),
                    secondJoint->getOrigin(),
                    secondJoint->getAxis(),
                    ratio,
                    getCVM->getPreRenderWidget());
            }
        }

        symbolController->UpdateAllSymbol();

        // 更新上次高亮记录
        m_lastFirstJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
        m_lastSecondJointId = mMotionRefMap::get(Physical_MotionCoupling, m_motionCouplingData->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);

        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionCouplingPanel::clearHighlight()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        std::vector<QString> symbolsToHide;
        if (m_lastFirstJointId >= 0)
            symbolsToHide.push_back(QString("motionJoint_%1").arg(m_lastFirstJointId));
        if (m_lastSecondJointId >= 0)
            symbolsToHide.push_back(QString("motionJoint_%1").arg(m_lastSecondJointId));
        if (m_motionCouplingData)
            symbolsToHide.push_back(QString("motionCoupling_%1").arg(m_motionCouplingData->getId()));

        if (!symbolsToHide.empty()) {
            symbolController->setSymbolShowState(symbolsToHide, false);
            symbolController->UpdateAllSymbol();
        }

        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mMotionCouplingPanel::refreshParamsByCouplingType()
    {
        if (!m_motionCouplingData)
            return;

        CouplingType type = m_motionCouplingData->getType();

        // 先隐藏所有参数控件（通过 setParent(nullptr) 从参数折叠框中移除）
        m_lblGearRadius->setParent(nullptr);
        m_spbGearRadius->setParent(nullptr);

        m_lblGearParamMode->setParent(nullptr);
        m_cmbGearParamMode->setParent(nullptr);
        m_lblGearRatio->setParent(nullptr);
        m_spbGearRatio->setParent(nullptr);
        m_lblToothCount1->setParent(nullptr);
        m_spbToothCount1->setParent(nullptr);
        m_lblToothCount2->setParent(nullptr);
        m_spbToothCount2->setParent(nullptr);
        m_lblPitchRadius1->setParent(nullptr);
        m_spbPitchRadius1->setParent(nullptr);
        m_lblPitchRadius2->setParent(nullptr);
        m_spbPitchRadius2->setParent(nullptr);

        m_lblCableRatio->setParent(nullptr);
        m_spbCableRatio->setParent(nullptr);
        m_lblCableReverse->setParent(nullptr);
        m_chkCableReverse->setParent(nullptr);

        // 根据当前类型动态添加需要的控件
        if (type == CouplingType::GearRack)
        {
            if (m_lblGearRadius->parent() == nullptr)
                m_foldParams->addRow(m_lblGearRadius, m_spbGearRadius);
        }
        else if (type == CouplingType::Gear)
        {
            // 齿轮副只提供传动比输入，固定按外啮合求解（齿数/分度圆半径
            // 不再暴露，引擎按两轴几何与传动比反算工作半径）。
            if (m_lblGearRatio->parent() == nullptr)
                m_foldParams->addRow(m_lblGearRatio, m_spbGearRatio);
        }
        else if (type == CouplingType::Cable)
        {
            // 线缆副只提供位移比与反向，无几何参数
            if (m_lblCableRatio->parent() == nullptr)
                m_foldParams->addRow(m_lblCableRatio, m_spbCableRatio);
            if (m_lblCableReverse->parent() == nullptr)
                m_foldParams->addRow(m_lblCableReverse, m_chkCableReverse);
        }

        // 刷新参数折叠框布局
        QTimer::singleShot(0, m_foldParams, [this]() {
            m_foldParams->refreshLayout();
            });
    }

    void mMotionCouplingPanel::connectSig()
    {
        // 目前信号在构造函数中已通过 lambda 连接，此处可补充跨视口/全局信号
        // 保留接口以便后续扩展
    }

    void mMotionCouplingPanel::disconnectSig()
    {
        // 目前主要信号在控件生命周期内保持连接，由 lambda 自动判断 m_motionCouplingData 是否有效
        // 保留接口以便后续扩展
    }

    void mMotionCouplingPanel::slot_subWindowActivated(QMdiSubWindow* subWindow)
    {
        // 切换视口时，刷新运动副列表
        if (subWindow)
        {
            refreshJointList();
            updateUIFromData();
        }
    }

    void mMotionCouplingPanel::slot_finishedPick()
    {
        // 耦合副暂不涉及几何拾取，预留接口
    }

}
