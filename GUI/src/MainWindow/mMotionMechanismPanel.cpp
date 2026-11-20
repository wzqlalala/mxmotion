#include "mMotionMechanismPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace MechMainWindow
{
    mMotionMechanismPanel::mMotionMechanismPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(4);

        // ==================== 基础信息 ====================
        FoldSection* foldBasic = new FoldSection("基础信息", this);
        m_edtName = new QLineEdit();
        m_edtName->setPlaceholderText("机构名称");
        foldBasic->addRow(new QLabel("名称:"), m_edtName);

        m_cmbMechanismType = new QComboBox();
        m_cmbMechanismType->addItems({ "曲柄摇杆机构", "双曲柄机构", "双摇杆机构", "曲柄滑块机构", "导杆机构", "凸轮机构", "齿轮机构" });
        foldBasic->addRow(new QLabel("机构类型:"), m_cmbMechanismType);
        m_mainLayout->addWidget(foldBasic);

        // ==================== 几何参数 ====================
        FoldSection* foldGeo = new FoldSection("几何参数", this);
        m_spbLength = new QDoubleSpinBox(); m_spbLength->setRange(0, 1e9); m_spbLength->setDecimals(4); m_spbLength->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbAngle = new QDoubleSpinBox(); m_spbAngle->setRange(0, 360); m_spbAngle->setDecimals(2); m_spbAngle->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbEccentricity = new QDoubleSpinBox(); m_spbEccentricity->setRange(0, 1e6); m_spbEccentricity->setDecimals(4); m_spbEccentricity->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbRodLength = new QDoubleSpinBox(); m_spbRodLength->setRange(0, 1e9); m_spbRodLength->setDecimals(4); m_spbRodLength->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldGeo->addRow(new QLabel("基准长度(mm):"), m_spbLength);
        foldGeo->addRow(new QLabel("安装角度(°):"), m_spbAngle);
        foldGeo->addRow(new QLabel("偏心距(mm):"), m_spbEccentricity);
        foldGeo->addRow(new QLabel("连杆长度(mm):"), m_spbRodLength);
        m_mainLayout->addWidget(foldGeo);

        // ==================== 运动参数 ====================
        FoldSection* foldMotion = new FoldSection("运动参数", this);
        m_spbDriveSpeed = new QDoubleSpinBox(); m_spbDriveSpeed->setRange(-1e6, 1e6); m_spbDriveSpeed->setDecimals(2); m_spbDriveSpeed->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbInitAngle = new QDoubleSpinBox(); m_spbInitAngle->setRange(0, 360); m_spbInitAngle->setDecimals(2); m_spbInitAngle->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_spbDof = new QSpinBox(); m_spbDof->setRange(0, 10); m_spbDof->setButtonSymbols(QAbstractSpinBox::NoButtons);
        foldMotion->addRow(new QLabel("驱动转速(rpm):"), m_spbDriveSpeed);
        foldMotion->addRow(new QLabel("初始角度(°):"), m_spbInitAngle);
        foldMotion->addRow(new QLabel("自由度:"), m_spbDof);
        m_mainLayout->addWidget(foldMotion);

        m_mainLayout->addStretch();
    }

    mMotionMechanismPanel::~mMotionMechanismPanel() {}

    void mMotionMechanismPanel::newMechanismPanel()
    {
        connectSig();
        m_edtName->setText("机构-1");
        m_cmbMechanismType->setCurrentIndex(0);
        m_spbLength->setValue(100.0);
        m_spbAngle->setValue(0.0);
        m_spbEccentricity->setValue(0.0);
        m_spbRodLength->setValue(200.0);
        m_spbDriveSpeed->setValue(60.0);
        m_spbInitAngle->setValue(0.0);
        m_spbDof->setValue(1);
    }

    void mMotionMechanismPanel::editMechanismPanel(int id)
    {
        connectSig();
        m_edtName->setText("机构-" + QString::number(id));
    }

    void mMotionMechanismPanel::uneditMechanismPanel()
    {
        disconnectSig();
    }

    void mMotionMechanismPanel::connectSig() {}
    void mMotionMechanismPanel::disconnectSig() {}
}
