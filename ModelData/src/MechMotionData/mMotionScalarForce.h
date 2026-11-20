#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>

#include "MXMotionData_global.h"
#include "mModelStruct.h"
#include "mMotionForceElement.h"

namespace MechMotionData {

    /**
     * @brief 标量力方向模式枚举
     * @details 对应求解器 ADMScalarForce 的 direction_mode
     *          0 = 两点连线方向（line）
     *          1 = 全局固定方向（fixed abs）
     *          2 = 作用体局部固定方向（fixed local A）
     *          3 = 基本体局部固定方向（fixed local B）
     */
    enum class MXMOTIONDATA_EXPORT ScalarForceDirectionMode {
        Line = 0,        ///< 两点连线方向
        FixedGlobal = 1, ///< 全局固定方向
        FixedLocalA = 2, ///< 作用体（marker_I 所在体）局部固定方向
        FixedLocalB = 3  ///< 基本体（marker_J 所在体）局部固定方向
    };

    /**
     * @brief 标量力数据类
     * @details 对应求解器 ADMScalarForce，两端分别作用在两个运动体上的局部点，
     *          力的大小由刚度 k、阻尼 r、预载 f0、常量力 magnitude 或函数表达式决定。
     */
    class MXMOTIONDATA_EXPORT mMotionScalarForce : public mMotionForceElement {
    public:
        DirectionPickState dirPickState_;                   ///< 固定方向输入的拾取会话状态
        mMotionScalarForce(int id, const QString& name = "");
        mMotionScalarForce();
        ~mMotionScalarForce() override;

        // --- 基础标识（来自基类） ---
        using mMotionForceElement::setId;
        using mMotionForceElement::getId;
        using mMotionForceElement::setName;
        using mMotionForceElement::getName;

        VectorLoadType getLoadType() const override { return VectorLoadType::ScalarForce; }

        // --- 作用体 I（动作侧） ---

        void setLocalI(const QVector3D& pos);
        QVector3D getLocalI() const;

        // --- 作用体 J（基本侧，-1 表示大地） ---

        void setLocalJ(const QVector3D& pos);
        QVector3D getLocalJ() const;

        // --- 方向模式与固定方向 ---
        void setDirectionMode(ScalarForceDirectionMode mode);
        ScalarForceDirectionMode getDirectionMode() const;

        void setFixedDirection(const QVector3D& dir);
        QVector3D getFixedDirection() const;

        // --- 常量力幅值 ---
        void setMagnitude(double magnitude);
        double getMagnitude() const;

        // --- 力函数表达式（可选） ---
        void setForceFunction(const QString& expr);
        QString getForceFunction() const;

        // --- 仅作用在基本体 ---
        void setActionOnly(bool actionOnly);
        bool getActionOnly() const;

        /**
         * @brief 复制当前标量力对象
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的 mMotionScalarForce 实例
         */
        virtual mMotionScalarForce* copyScalarForce(int newId, const QString& newName);
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        using mMotionForceElement::m_id;
        using mMotionForceElement::m_name;

        friend QDataStream& operator>>(QDataStream& in, mMotionScalarForce& force);
        friend QDataStream& operator<<(QDataStream& out, mMotionScalarForce& force);

    public:

        QVector3D m_localI;                 ///< 作用点 I 在动作体局部坐标系下的位置

        QVector3D m_localJ;                 ///< 作用点 J 在基本体局部坐标系下的位置

        ScalarForceDirectionMode m_directionMode; ///< 方向模式
        QVector3D m_fixedDirection;         ///< 固定方向向量（未归一化存储）

        double m_magnitude;                 ///< 常量力幅值

        QString m_forceFunction;            ///< 力函数表达式（可选）
        bool m_actionOnly;                  ///< 是否仅作用在基本体
    };

}
