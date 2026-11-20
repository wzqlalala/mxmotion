#include "mMotionControllerPanel.h"
#include "mMotionRefMap.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//BasicData
#include "mModelData.h"
#include "mMotionDriveBase.h"
#include "mMotionDriveManager.h"
#include "mMotionJointDrive.h"
#include "mMotionControllerBase.h"
#include "mMotionControllerManager.h"
#include "mMotionPIDController.h"
#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"
#include "mMotionSensorManager.h"
#include "mMotionSensorBase.h"

//Qt
#include <QLabel>
#include <QMessageBox>

using namespace MechPreData;
using namespace MechMotionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{

    mMotionControllerPanel::mMotionControllerPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_controllerData = nullptr;

        m_mainLayout = new QVBoxLayout(this);

        // ==================== 1. 控制设置 ====================
        FoldSection* foldControl = new FoldSection("控制设置", this);

        m_cmbTargetDrive = new QComboBox();
        foldControl->addRow(new PanelLabel("目标驱动:"), m_cmbTargetDrive);

        m_cmbFeedbackType = new QComboBox();
        m_cmbFeedbackType->addItems({ "位移", "速度" });
        foldControl->addRow(new PanelLabel("反馈量:"), m_cmbFeedbackType);

        m_cmbFeedbackSource = new QComboBox();
        m_cmbFeedbackSource->addItems({ "驱动自由度", "传感器" });
        foldControl->addRow(new PanelLabel("反馈源:"), m_cmbFeedbackSource);

        m_cmbFeedbackSensor = new QComboBox();
        foldControl->addRow(new PanelLabel("反馈传感器:"), m_cmbFeedbackSensor);

        m_chkFeedbackSign = new QCheckBox("反馈取反");
        foldControl->addRow(new PanelLabel("符号:"), m_chkFeedbackSign);

        m_cmbTargetSource = new QComboBox();
        m_cmbTargetSource->addItems({ "常数", "控制输入" });
        foldControl->addRow(new PanelLabel("目标来源:"), m_cmbTargetSource);

        m_cmbTargetInput = new QComboBox();
        foldControl->addRow(new PanelLabel("控制输入:"), m_cmbTargetInput);

        m_edtTargetValue = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        foldControl->addRow(new PanelLabel("目标值:"), m_edtTargetValue);

        m_mainLayout->addWidget(foldControl);

        // ==================== 2. PID 参数 ====================
        FoldSection* foldPid = new FoldSection("PID 参数", this);

        m_edtKp = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        foldPid->addRow(new PanelLabel("比例系数 Kp:"), m_edtKp);

        m_edtKi = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        foldPid->addRow(new PanelLabel("积分系数 Ki:"), m_edtKi);

        m_edtKd = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        foldPid->addRow(new PanelLabel("微分系数 Kd:"), m_edtKd);

        m_edtOutputLimit = new MxFunctions::mCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode::Standard, this);
        m_edtOutputLimit->setToolTip(QStringLiteral("0 表示不限制"));
        foldPid->addRow(new PanelLabel("输出限幅:"), m_edtOutputLimit);

        m_mainLayout->addWidget(foldPid);

        m_mainLayout->addStretch();

        // ==================== 信号绑定 ====================
        connect(m_cmbTargetDrive, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mMotionControllerPanel::slot_targetDriveChanged);
        connect(m_cmbFeedbackType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mMotionControllerPanel::slot_feedbackTypeChanged);
        connect(m_cmbFeedbackSource, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
            if (!m_controllerData)
                return;
            const bool useSensor = (index == 1);
            m_controllerData->setFeedbackSource(useSensor
                ? PidFeedbackSource::Sensor : PidFeedbackSource::DriveDof);
            m_cmbFeedbackType->setEnabled(!useSensor);
            m_cmbFeedbackSensor->setEnabled(useSensor);
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionControllerSig(false, m_controllerData->getId());
        });
        connect(m_cmbFeedbackSensor, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
            if (m_controllerData) {
                mMotionRefMap::set(Physical_MotionController, m_controllerData->getId(), Physical_MotionSensor, MapRelation_None, m_cmbFeedbackSensor->itemData(index).toInt());
                            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionControllerSig(false, m_controllerData->getId());
            }
        });
        connect(m_chkFeedbackSign, &QCheckBox::toggled, this, [this](bool checked) {
            if (m_controllerData)
                m_controllerData->setFeedbackSign(checked ? -1.0 : 1.0);
        });
        connect(m_cmbTargetSource, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &mMotionControllerPanel::slot_targetSourceChanged);
        connect(m_cmbTargetInput, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
            if (!m_controllerData)
                return;
            const int inputId = m_cmbTargetInput->itemData(index).toInt();
            mMotionRefMap::set(Physical_MotionController, m_controllerData->getId(), Physical_MotionControlInput, MapRelation_None, inputId);
            // 未选具体输入时退回常数目标，保持常数框可编辑
            m_edtTargetValue->setEnabled(inputId < 0);
        });

        connect(m_edtTargetValue, &MxFunctions::mCalcLineEdit::editingFinished, this, [this]() {
            if (m_controllerData)
                m_controllerData->setTargetValue(m_edtTargetValue->text().toDouble());
            });
        connect(m_edtKp, &MxFunctions::mCalcLineEdit::editingFinished, this, [this]() {
            if (m_controllerData)
                m_controllerData->setGains(m_edtKp->text().toDouble(),
                    m_controllerData->getKi(), m_controllerData->getKd());
            });
        connect(m_edtKi, &MxFunctions::mCalcLineEdit::editingFinished, this, [this]() {
            if (m_controllerData)
                m_controllerData->setGains(m_controllerData->getKp(),
                    m_edtKi->text().toDouble(), m_controllerData->getKd());
            });
        connect(m_edtKd, &MxFunctions::mCalcLineEdit::editingFinished, this, [this]() {
            if (m_controllerData)
                m_controllerData->setGains(m_controllerData->getKp(),
                    m_controllerData->getKi(), m_edtKd->text().toDouble());
            });
        connect(m_edtOutputLimit, &MxFunctions::mCalcLineEdit::editingFinished, this, [this]() {
            if (m_controllerData)
                m_controllerData->setOutputLimit(m_edtOutputLimit->text().toDouble());
            });
    }

    mMotionControllerPanel::~mMotionControllerPanel()
    {
    }

    void mMotionControllerPanel::newMotionControllerPanel()
    {
        setEditing(true);
        int controllerID = getCVM->getModelData()->getManager<mMotionControllerManager>(Physical_MotionController).createID();

        m_controllerData = new mMotionPIDController(controllerID);
        m_controllerData->setName(getCVM->getModelData()->createNameByType(Physical_MotionController, QStringLiteral("PID控制器")));
        getCVM->getModelData()->addPhysicalField<mMotionControllerBase>(m_controllerData);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionControllerSig(true, m_controllerData->getId());
    }

    void mMotionControllerPanel::editMotionControllerPanel(int id)
    {
        setEditing(true);
        auto* base = getCVM->getModelData()->getManager<mMotionControllerManager>(Physical_MotionController).getData(id);
        m_controllerData = dynamic_cast<mMotionPIDController*>(base);
        if (!m_controllerData)
            return;

        updateUIFromData();
    }

    //数据对象即将被删除（树删除前调用）：置空编辑指针防悬垂（周期边界同款约定）
    void mMotionControllerPanel::onMotionControllerDataAboutToBeDestroyed(int id)
    {
        if (id >= 0)
        {
            if (m_controllerData == nullptr || id != m_controllerData->getId())
                return;
        }
        m_controllerData = nullptr;
    }

    void mMotionControllerPanel::uneditMotionControllerPanel()
    {
        if (!isEditing())
            return;
        setEditing(false);
        m_controllerData = nullptr;
    }

    void mMotionControllerPanel::refreshDriveList()
    {
        const QSignalBlocker b(m_cmbTargetDrive);
        m_cmbTargetDrive->clear();
        m_cmbTargetDrive->addItem(" ", -1);

        auto& driveManager = getCVM->getModelData()->getManager<mMotionDriveManager>(Physical_MotionDrive);
        for (int id : driveManager.getAllIDs()) {
            auto* jointDrive = dynamic_cast<mMotionJointDrive*>(driveManager.getData(id));
            // 闭环注入只支持速度控制模式的驱动
            if (!jointDrive || jointDrive->getControlMode() != DriveControlMode::Velocity)
                continue;
            const QString suffix = (jointDrive->getMotionType() == DriveMotionType::Rotation)
                ? QStringLiteral("（旋转）") : QStringLiteral("（平移）");
            m_cmbTargetDrive->addItem(driveManager.getNameByID(id) + suffix, id);
        }
    }

    void mMotionControllerPanel::updateUIFromData()
    {
        if (!m_controllerData)
            return;

        refreshDriveList();

        // 还原目标驱动
        const int driveId = mMotionRefMap::get(Physical_MotionController, m_controllerData->getId(), Physical_MotionDrive);
        int driveIndex = 0;
        for (int i = 0; i < m_cmbTargetDrive->count(); ++i) {
            if (m_cmbTargetDrive->itemData(i).toInt() == driveId) {
                driveIndex = i;
                break;
            }
        }
        m_cmbTargetDrive->setCurrentIndex(driveIndex);

        // 反馈源/传感器/符号还原
        refreshSensorList();
        const bool useSensor = (m_controllerData->getFeedbackSource() == PidFeedbackSource::Sensor);
        {
            const QSignalBlocker bSource(m_cmbFeedbackSource);
            m_cmbFeedbackSource->setCurrentIndex(useSensor ? 1 : 0);
        }
        if (useSensor) {
            const QSignalBlocker bSensor(m_cmbFeedbackSensor);
            int sensorIdx = m_cmbFeedbackSensor->findData(mMotionRefMap::get(Physical_MotionController, m_controllerData->getId(), Physical_MotionSensor));
            if (sensorIdx >= 0)
                m_cmbFeedbackSensor->setCurrentIndex(sensorIdx);
        }
        {
            const QSignalBlocker bSign(m_chkFeedbackSign);
            m_chkFeedbackSign->setChecked(m_controllerData->getFeedbackSign() < 0.0);
        }
        m_cmbFeedbackType->setEnabled(!useSensor);
        m_cmbFeedbackSensor->setEnabled(useSensor);

        const QSignalBlocker bFeedback(m_cmbFeedbackType);
        m_cmbFeedbackType->setCurrentIndex(
            static_cast<int>(m_controllerData->getFeedbackType()));

        // 目标来源与控制输入
        refreshInputList();
        const QSignalBlocker bSource(m_cmbTargetSource);
        const int inputId = mMotionRefMap::get(Physical_MotionController, m_controllerData->getId(), Physical_MotionControlInput);
        m_cmbTargetSource->setCurrentIndex(inputId >= 0 ? 1 : 0);
        if (inputId >= 0) {
            const QSignalBlocker bInput(m_cmbTargetInput);
            int inputIdx = m_cmbTargetInput->findData(inputId);
            if (inputIdx >= 0)
                m_cmbTargetInput->setCurrentIndex(inputIdx);
        }
        m_edtTargetValue->setEnabled(inputId < 0);
        m_cmbTargetInput->setEnabled(inputId >= 0);

        const QSignalBlocker bTarget(m_edtTargetValue);
        m_edtTargetValue->setExpression(QString::number(m_controllerData->getTargetValue()));

        const QSignalBlocker bKp(m_edtKp);
        m_edtKp->setExpression(QString::number(m_controllerData->getKp()));
        const QSignalBlocker bKi(m_edtKi);
        m_edtKi->setExpression(QString::number(m_controllerData->getKi()));
        const QSignalBlocker bKd(m_edtKd);
        m_edtKd->setExpression(QString::number(m_controllerData->getKd()));
        const QSignalBlocker bLimit(m_edtOutputLimit);
        m_edtOutputLimit->setExpression(QString::number(m_controllerData->getOutputLimit()));

    }

    void mMotionControllerPanel::refreshSensorList()
    {
        const QSignalBlocker b(m_cmbFeedbackSensor);
        m_cmbFeedbackSensor->clear();
        m_cmbFeedbackSensor->addItem(" ", -1);
        auto& sensorMgr = getCVM->getModelData()->getManager<mMotionSensorManager>(Physical_MotionSensor);
        for (int id : sensorMgr.getAllIDs()) {
            auto* sensor = sensorMgr.getData(id);
            if (!sensor || !sensor->isEnabled())
                continue;
            // 求解期可单帧求值的运动学类；距离传感器相对速度/加速度在装配时再校验
            switch (sensor->getType()) {
            case SensorType::BodyDisplacement:
            case SensorType::BodyVelocity:
            case SensorType::BodyAcceleration:
            case SensorType::Distance:
            case SensorType::Angle:
                m_cmbFeedbackSensor->addItem(sensor->getName(), id);
                break;
            default:
                break;
            }
        }
    }

    void mMotionControllerPanel::refreshInputList()
    {
        const QSignalBlocker b(m_cmbTargetInput);
        m_cmbTargetInput->clear();
        m_cmbTargetInput->addItem(" ", -1);
        auto& inputMgr = getCVM->getModelData()->getManager<mMotionControlInputManager>(Physical_MotionControlInput);
        for (int id : inputMgr.getAllIDs()) {
            m_cmbTargetInput->addItem(inputMgr.getNameByID(id), id);
        }
    }

    void mMotionControllerPanel::slot_targetSourceChanged(int index)
    {
        if (!m_controllerData)
            return;
        const bool useInput = (index == 1);
        m_edtTargetValue->setEnabled(!useInput);
        m_cmbTargetInput->setEnabled(useInput);
        if (!useInput) {
            // 切回常数：清除引用并还原常数值
            mMotionRefMap::set(Physical_MotionController, m_controllerData->getId(), Physical_MotionControlInput, MapRelation_None, -1);
            m_controllerData->setTargetValue(m_edtTargetValue->text().toDouble());
        }
        else {
            const int inputId = m_cmbTargetInput->currentData().toInt();
            mMotionRefMap::set(Physical_MotionController, m_controllerData->getId(), Physical_MotionControlInput, MapRelation_None, inputId);
            m_edtTargetValue->setEnabled(inputId < 0);
        }
    }

    void mMotionControllerPanel::slot_targetDriveChanged(int index)
    {
        if (!m_controllerData)
            return;
        mMotionRefMap::set(Physical_MotionController, m_controllerData->getId(), Physical_MotionDrive, MapRelation_None, m_cmbTargetDrive->itemData(index).toInt());
    }

    void mMotionControllerPanel::slot_feedbackTypeChanged(int index)
    {
        if (!m_controllerData)
            return;
        m_controllerData->setFeedbackType(static_cast<PidFeedbackType>(index));
    }

}
