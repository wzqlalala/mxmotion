#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>

#include "MXMotionData_global.h"

namespace MechMotionData {

    /**
     * @brief 接触类型枚举
     */
    enum class MXMOTIONDATA_EXPORT MotionContactType {
        CADContact,      ///< CAD 接触（小面 / Nurbs 曲面）
        FacetContact     ///< 小面接触
    };

    /**
     * @brief 3D 接触数据模型
     * @details 对应 UG Motion 中的 3D Contact
     */
    class MXMOTIONDATA_EXPORT mMotionContact {
    public:
        mMotionContact(int id, const QString& name = "");
        mMotionContact();
        ~mMotionContact();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        // --- 接触类型 ---
        void setContactType(MotionContactType type);
        MotionContactType getContactType() const;

        // --- 动作体 (Action Body) ---

        // --- 基本体 (Base Body) ---

        // --- 基本参数 (Basic) ---
        void setStiffness(double k);
        double getStiffness() const;

        void setForceExponent(double e);
        double getForceExponent() const;

        void setMaterialDamping(double c);
        double getMaterialDamping() const;

        void setPenetrationDepth(double d);
        double getPenetrationDepth() const;

        // --- 库仑摩擦 ---
        void setFrictionEnabled(bool enabled);
        bool getFrictionEnabled() const;

        void setStaticFriction(double mu);
        double getStaticFriction() const;

        void setDynamicFriction(double mu);
        double getDynamicFriction() const;

        // --- 高级参数 (Advanced) ---
        void setForceModel(const QString& model);
        QString getForceModel() const;

        void setForceModelTolerance(double tol);
        double getForceModelTolerance() const;

        // 接触几何处理：true=凸包碰撞（默认，稳定高效），false=精确三角网格
        // （平面/薄片等退化几何的凸包不稳定，建议精确网格；引擎对退化点集也会自动降级）
        void setUseConvexHullAction(bool use);
        bool getUseConvexHullAction() const;

        void setUseConvexHullBase(bool use);
        bool getUseConvexHullBase() const;

        // 动作体小平面参数
        void setActionUseFacetFile(bool use);
        bool getActionUseFacetFile() const;

        void setActionDistanceTolerance(double tol);
        double getActionDistanceTolerance() const;

        void setActionAngleTolerance(double tol);
        double getActionAngleTolerance() const;  // 共用角度公差

        void setActionUseMaxFacetSize(bool use);
        bool getActionUseMaxFacetSize() const;

        void setActionMaxFacetSize(double size);
        double getActionMaxFacetSize() const;

        // 基本体小平面参数
        void setBaseUseFacetFile(bool use);
        bool getBaseUseFacetFile() const;

        void setBaseDistanceTolerance(double tol);
        double getBaseDistanceTolerance() const;

        void setBaseUseMaxFacetSize(bool use);
        bool getBaseUseMaxFacetSize() const;

        void setBaseMaxFacetSize(double size);
        double getBaseMaxFacetSize() const;

        // --- 辅助方法 ---
        QString getContactTypeName() const;
        static QString getContactTypeName(MotionContactType type);

        mMotionContact* copyContact(int newId, const QString& newName);

        friend QDataStream& operator>>(QDataStream& in, mMotionContact& contact);
        friend QDataStream& operator<<(QDataStream& out, mMotionContact& contact);

    private:
        int m_id;                       ///< 全局唯一ID
        QString m_name;                 ///< 名称
        MotionContactType m_contactType;      ///< 接触类型


        // 基本参数
        double m_stiffness;             ///< 刚度
        double m_forceExponent;         ///< 力指数
        double m_materialDamping;       ///< 材料阻尼
        double m_penetrationDepth;      ///< 最大穿透深度

        // 摩擦
        bool m_frictionEnabled;         ///< 库仑摩擦开关
        double m_staticFriction;        ///< 静摩擦系数
        double m_dynamicFriction;       ///< 动摩擦系数

        // 高级参数
        QString m_forceModel;           ///< 力模型
        double m_forceModelTolerance;   ///< 力模型公差

        // 接触几何处理
        bool m_useConvexHullAction;     ///< 作用体使用凸包碰撞
        bool m_useConvexHullBase;       ///< 基本体使用凸包碰撞

        // 动作体小平面
        bool m_actionUseFacetFile;      ///< 使用细分文件
        double m_actionDistanceTolerance; ///< 距离公差
        double m_angleTolerance;        ///< 角度公差（共用）
        bool m_actionUseMaxFacetSize;   ///< 启用最大小平面大小
        double m_actionMaxFacetSize;    ///< 最大小平面大小

        // 基本体小平面
        bool m_baseUseFacetFile;        ///< 使用细分文件
        double m_baseDistanceTolerance; ///< 距离公差
        bool m_baseUseMaxFacetSize;     ///< 启用最大小平面大小
        double m_baseMaxFacetSize;      ///< 最大小平面大小
    };

}
