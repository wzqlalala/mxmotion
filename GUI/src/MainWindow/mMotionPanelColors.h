#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QColor>
#include <QVector3D>

namespace MechMainWindow
{
    /**
     * @brief 运动面板统一语义配色
     * @details 体(A/I=红, B/J=蓝)与点(A/单点=橙, B=品红)正交，方向/轴=绿。
     *          体色用于视口整件高亮，点色用于拾取控件色标与视口标记点，
     *          两套色系不得混用；新增面板一律从此处取色，不再自定颜色。
     */
    struct mMotionPanelColors
    {
        static QColor bodyA() { return QColor(255, 0, 0); }      ///< 体·A/作用端(I)
        static QColor bodyB() { return QColor(0, 0, 255); }      ///< 体·B/基准端(J)
        static QColor pointA() { return QColor(255, 128, 0); }   ///< 点·A/单点
        static QColor pointB() { return QColor(255, 0, 255); }   ///< 点·B
        static QColor pointC() { return QColor(0, 180, 255); }   ///< 点·C（第三参考点，如三点力目标点）
        static QColor direction() { return QColor(0, 128, 0); }  ///< 方向/轴

        /// 渲染层用 0~1 浮点颜色向量
        static QVector3D vec(const QColor& c) { return QVector3D(c.redF(), c.greenF(), c.blueF()); }
    };
}
