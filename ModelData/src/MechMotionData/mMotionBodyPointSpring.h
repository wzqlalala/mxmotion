#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>
#include <QQuaternion>

#include "MXMotionData_global.h"
#include "mMotionSpringBase.h"

namespace MechMotionData {

    /**
     * @brief 体-点弹簧数据类
     * @details 两端分别绑定到两个运动体上的局部点，
     *          安装长度由两个局部点在初始位姿下的世界距离自动计算。
     */
    class MXMOTIONDATA_EXPORT mMotionBodyPointSpring : public mMotionSpringBase {
    public:
        mMotionBodyPointSpring(int id, const QString& name = "");
        mMotionBodyPointSpring() {};
        ~mMotionBodyPointSpring() override;

        mMotionSpringBase* copySpring(int newId, const QString& newName) override;

        // --- 端点 I 绑定信息 ---

        void setLocalI(const QVector3D& pos);
        QVector3D getLocalI() const;

        void setLocalRotI(const QQuaternion& rot);
        QQuaternion getLocalRotI() const;

        // --- 端点 J 绑定信息 ---

        void setLocalJ(const QVector3D& pos);
        QVector3D getLocalJ() const;

        void setLocalRotJ(const QQuaternion& rot);
        QQuaternion getLocalRotJ() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionBodyPointSpring& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionBodyPointSpring& mgr);

    public:
        QVector3D m_localI;     ///< 端点 I 在运动体 I 局部坐标系下的位置
        QVector3D m_localJ;     ///< 端点 J 在运动体 J 局部坐标系下的位置

    private:

        QQuaternion m_localRotI; ///< 端点 I 在运动体 I 局部坐标系下的旋转
        QQuaternion m_localRotJ; ///< 端点 J 在运动体 J 局部坐标系下的旋转
    };

}
