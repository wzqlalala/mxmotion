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
    class MMAINWINDOW_EXPORT mMotionTransmissionPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        mMotionTransmissionPanel(QWidget* parent = nullptr);
        ~mMotionTransmissionPanel();

        void newTransmissionPanel();
        void editTransmissionPanel(int id = 0);
        void uneditTransmissionPanel();

    private:
        void connectSig();
        void disconnectSig();

    private:
        QVBoxLayout* m_mainLayout;

        // 基础信息
        QLineEdit* m_edtName;
        QComboBox* m_cmbTransType;

        // 齿轮/链轮参数
        QSpinBox*      m_spbDrivingTeeth;
        QSpinBox*      m_spbDrivenTeeth;
        QDoubleSpinBox* m_spbModule;
        QDoubleSpinBox* m_spbPressureAngle;

        // 传动布局
        QDoubleSpinBox* m_spbAxisDistance;
        QDoubleSpinBox* m_spbRatio;
        QComboBox* m_cmbDirection;
    };
}
