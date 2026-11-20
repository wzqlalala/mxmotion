#include "mMotionTransmissionPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace MechMainWindow
{
    mMotionTransmissionPanel::mMotionTransmissionPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);
        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("传动链名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbTransType = new QComboBox();
        m_cmbTransType->addItems({ "齿轮传动", "链传动", "带传动", "蜗杆传动", "行星齿轮" });
        foldBasic->addRow(new QLabel("传动类型:"), m_cmbTransType);
        m_mainLayout->addWidget(foldBasic);

        // ==================== 齿轮/链轮参数 ====================
        FoldSection* foldGear = new FoldSection("齿轮/链轮参数", this);
        m_spbDrivingTeeth = new QSpinBox(); m_spbDrivingTeeth->setRange(1, 1000); m_spbDrivingTeeth->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbDrivenTeeth = new QSpinBox(); m_spbDrivenTeeth->setRange(1, 1000); m_spbDrivenTeeth->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbModule = new QDoubleSpinBox(); m_spbModule->setRange(0.1, 100); m_spbModule->setDecimals(2); m_spbModule->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbPressureAngle = new QDoubleSpinBox(); m_spbPressureAngle->setRange(0, 45); m_spbPressureAngle->setDecimals(2); m_spbPressureAngle->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldGear->addRow(new QLabel("主动轮齿数:"), m_spbDrivingTeeth);
        foldGear->addRow(new QLabel("从动轮齿数:"), m_spbDrivenTeeth);
        foldGear->addRow(new QLabel("模数(mm):"), m_spbModule);
        foldGear->addRow(new QLabel("压力角(°):"), m_spbPressureAngle);
        m_mainLayout->addWidget(foldGear);

        // ==================== 传动布局 ====================
        FoldSection* foldLayout = new FoldSection("传动布局", this);
        m_spbAxisDistance = new QDoubleSpinBox(); m_spbAxisDistance->setRange(0, 1e6); m_spbAxisDistance->setDecimals(2); m_spbAxisDistance->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbRatio = new QDoubleSpinBox(); m_spbRatio->setRange(0.001, 1000); m_spbRatio->setDecimals(3); m_spbRatio->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_cmbDirection = new QComboBox();
        m_cmbDirection->addItems({ "同向", "反向" });
        foldLayout->addRow(new QLabel("轴间距(mm):"), m_spbAxisDistance);
        foldLayout->addRow(new QLabel("传动比:"), m_spbRatio);
        foldLayout->addRow(new QLabel("转向:"), m_cmbDirection);
        m_mainLayout->addWidget(foldLayout);

        m_mainLayout->addStretch();
    }

    mMotionTransmissionPanel::~mMotionTransmissionPanel() {}

    void mMotionTransmissionPanel::newTransmissionPanel()
    {
        connectSig();
        m_edtName->setText("传动链-1");
        m_cmbTransType->setCurrentIndex(0);
        m_spbDrivingTeeth->setValue(20);
        m_spbDrivenTeeth->setValue(40);
        m_spbModule->setValue(2.0);
        m_spbPressureAngle->setValue(20.0);
        m_spbAxisDistance->setValue(60.0);
        m_spbRatio->setValue(2.0);
        m_cmbDirection->setCurrentIndex(0);
    }

    void mMotionTransmissionPanel::editTransmissionPanel(int id)
    {
        connectSig();
        m_edtName->setText("传动链-" + QString::number(id));
    }

    void mMotionTransmissionPanel::uneditTransmissionPanel()
    {
        disconnectSig();
    }

    void mMotionTransmissionPanel::connectSig() {}
    void mMotionTransmissionPanel::disconnectSig() {}
}
