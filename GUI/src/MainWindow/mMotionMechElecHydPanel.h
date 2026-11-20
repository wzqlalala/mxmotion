#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mMotionMechElecHydPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        mMotionMechElecHydPanel(QWidget* parent = nullptr);
        ~mMotionMechElecHydPanel();

        void newMechElecHydPanel();
        void editMechElecHydPanel(int id = 0);
        void uneditMechElecHydPanel();

    private:
        void connectSig();
        void disconnectSig();

    private:
        QVBoxLayout* m_mainLayout;

        // 基础信息
        QLineEdit* m_edtName;
        QComboBox* m_cmbSystemType;

        // 电机参数
        QDoubleSpinBox* m_spbMotorPower;
        QDoubleSpinBox* m_spbMotorSpeed;
        QDoubleSpinBox* m_spbMotorVoltage;
        QDoubleSpinBox* m_spbMotorCurrent;
        QSpinBox*      m_spbMotorPolePairs;

        // 液压参数
        QDoubleSpinBox* m_spbHydFlow;
        QDoubleSpinBox* m_spbHydPressure;
        QDoubleSpinBox* m_spbHydTankVolume;
        QDoubleSpinBox* m_spbHydPumpDisplacement;

        // 耦合配置
        QComboBox* m_cmbControlMode;
        QComboBox* m_cmbFeedbackMode;
    };
}
