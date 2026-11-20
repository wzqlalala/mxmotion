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
    class MMAINWINDOW_EXPORT mMotionMechanismPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        mMotionMechanismPanel(QWidget* parent = nullptr);
        ~mMotionMechanismPanel();

        void newMechanismPanel();
        void editMechanismPanel(int id = 0);
        void uneditMechanismPanel();

    private:
        void connectSig();
        void disconnectSig();

    private:
        QVBoxLayout* m_mainLayout;

        // 基础信息
        QLineEdit* m_edtName;
        QComboBox* m_cmbMechanismType;

        // 几何参数
        QDoubleSpinBox* m_spbLength;
        QDoubleSpinBox* m_spbAngle;
        QDoubleSpinBox* m_spbEccentricity;
        QDoubleSpinBox* m_spbRodLength;

        // 运动参数
        QDoubleSpinBox* m_spbDriveSpeed;
        QDoubleSpinBox* m_spbInitAngle;
        QSpinBox*      m_spbDof;
    };
}
