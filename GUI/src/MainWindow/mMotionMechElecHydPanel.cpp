#include "mMotionMechElecHydPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace MechMainWindow
{
    mMotionMechElecHydPanel::mMotionMechElecHydPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);
        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("机电液系统名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbSystemType = new QComboBox();
        m_cmbSystemType->addItems({ "机电液伺服系统", "电液比例系统", "电液伺服阀控", "电液数字控制", "混合驱动系统" });
        foldBasic->addRow(new QLabel("系统类型:"), m_cmbSystemType);
        m_mainLayout->addWidget(foldBasic);

        // ==================== 电机参数 ====================
        FoldSection* foldMotor = new FoldSection("电机参数", this);
        m_spbMotorPower = new QDoubleSpinBox(); m_spbMotorPower->setRange(0, 1e6); m_spbMotorPower->setDecimals(2); m_spbMotorPower->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbMotorSpeed = new QDoubleSpinBox(); m_spbMotorSpeed->setRange(0, 1e6); m_spbMotorSpeed->setDecimals(0); m_spbMotorSpeed->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbMotorVoltage = new QDoubleSpinBox(); m_spbMotorVoltage->setRange(0, 1e6); m_spbMotorVoltage->setDecimals(1); m_spbMotorVoltage->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbMotorCurrent = new QDoubleSpinBox(); m_spbMotorCurrent->setRange(0, 1e6); m_spbMotorCurrent->setDecimals(2); m_spbMotorCurrent->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbMotorPolePairs = new QSpinBox(); m_spbMotorPolePairs->setRange(1, 100); m_spbMotorPolePairs->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldMotor->addRow(new QLabel("额定功率(kW):"), m_spbMotorPower);
        foldMotor->addRow(new QLabel("额定转速(rpm):"), m_spbMotorSpeed);
        foldMotor->addRow(new QLabel("额定电压(V):"), m_spbMotorVoltage);
        foldMotor->addRow(new QLabel("额定电流(A):"), m_spbMotorCurrent);
        foldMotor->addRow(new QLabel("极对数:"), m_spbMotorPolePairs);
        m_mainLayout->addWidget(foldMotor);

        // ==================== 液压参数 ====================
        FoldSection* foldHyd = new FoldSection("液压参数", this);
        m_spbHydFlow = new QDoubleSpinBox(); m_spbHydFlow->setRange(0, 1e6); m_spbHydFlow->setDecimals(2); m_spbHydFlow->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbHydPressure = new QDoubleSpinBox(); m_spbHydPressure->setRange(0, 1e6); m_spbHydPressure->setDecimals(1); m_spbHydPressure->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbHydTankVolume = new QDoubleSpinBox(); m_spbHydTankVolume->setRange(0, 1e6); m_spbHydTankVolume->setDecimals(1); m_spbHydTankVolume->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbHydPumpDisplacement = new QDoubleSpinBox(); m_spbHydPumpDisplacement->setRange(0, 1e6); m_spbHydPumpDisplacement->setDecimals(2); m_spbHydPumpDisplacement->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldHyd->addRow(new QLabel("流量(L/min):"), m_spbHydFlow);
        foldHyd->addRow(new QLabel("压力(MPa):"), m_spbHydPressure);
        foldHyd->addRow(new QLabel("油箱容积(L):"), m_spbHydTankVolume);
        foldHyd->addRow(new QLabel("泵排量(mL/r):"), m_spbHydPumpDisplacement);
        m_mainLayout->addWidget(foldHyd);

        // ==================== 耦合配置 ====================
        FoldSection* foldCtrl = new FoldSection("耦合配置", this);
        m_cmbControlMode = new QComboBox();
        m_cmbControlMode->addItems({ "位置伺服", "力伺服", "速度伺服", "压力伺服", "复合控制" });
        m_cmbFeedbackMode = new QComboBox();
        m_cmbFeedbackMode->addItems({ "全闭环", "半闭环", "开环" });
        foldCtrl->addRow(new QLabel("控制模式:"), m_cmbControlMode);
        foldCtrl->addRow(new QLabel("反馈方式:"), m_cmbFeedbackMode);
        m_mainLayout->addWidget(foldCtrl);

        m_mainLayout->addStretch();
    }

    mMotionMechElecHydPanel::~mMotionMechElecHydPanel() {}

    void mMotionMechElecHydPanel::newMechElecHydPanel()
    {
        connectSig();
        m_edtName->setText("机电液系统-1");
        m_cmbSystemType->setCurrentIndex(0);
        m_spbMotorPower->setValue(5.5);
        m_spbMotorSpeed->setValue(1500);
        m_spbMotorVoltage->setValue(380);
        m_spbMotorCurrent->setValue(12.0);
        m_spbMotorPolePairs->setValue(2);
        m_spbHydFlow->setValue(40.0);
        m_spbHydPressure->setValue(21.0);
        m_spbHydTankVolume->setValue(100.0);
        m_spbHydPumpDisplacement->setValue(20.0);
        m_cmbControlMode->setCurrentIndex(0);
        m_cmbFeedbackMode->setCurrentIndex(0);
    }

    void mMotionMechElecHydPanel::editMechElecHydPanel(int id)
    {
        connectSig();
        m_edtName->setText("机电液系统-" + QString::number(id));
    }

    void mMotionMechElecHydPanel::uneditMechElecHydPanel()
    {
        disconnectSig();
    }

    void mMotionMechElecHydPanel::connectSig() {}
    void mMotionMechElecHydPanel::disconnectSig() {}
}
