#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QQuaternion>
#include <QColor>

namespace MechMotionData {

    /**
     * @brief 标记点数据模型
     * @details 用于定义运动体上的局部坐标参考点，对应 ADAMS Marker 语义
     */
    class MXMOTIONDATA_EXPORT mMotionMarker {
    public:
        mMotionMarker(int id, const QString& name = "");
        mMotionMarker();
        ~mMotionMarker();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        // --- 附着对象 ---

        // --- 局部坐标（相对于附着体）---
        void setLocalPosition(const QVector3D& pos);
        QVector3D getLocalPosition() const;

        void setLocalRotation(const QQuaternion& rot);
        QQuaternion getLocalRotation() const;

        // --- 显示属性 ---
        void setColor(const QColor& color);
        QColor getColor() const;

        void setSize(double size);
        double getSize() const;

        void setVisible(bool visible);
        bool getVisible() const;

        mMotionMarker* copyMarker(int newId, const QString& newName);

        friend QDataStream& operator>>(QDataStream& in, mMotionMarker& marker);
        friend QDataStream& operator<<(QDataStream& out, mMotionMarker& marker);

    private:
        int m_id;                       ///< 标记点全局唯一ID
        QString m_name;                 ///< 标记点名称

        QVector3D m_localPosition;      ///< 在运动体局部坐标系下的位置
        QQuaternion m_localRotation;    ///< 在运动体局部坐标系下的方向（四元数）

        QColor m_color;                 ///< 渲染颜色
        double m_size;                  ///< 显示大小
        bool m_visible;                 ///< 是否可见
    };

}
