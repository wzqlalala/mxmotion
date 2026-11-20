#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>

using namespace MxFunctions;
namespace MechMotionData {

    /**
     * @brief 运动副摩擦参数
     */
    class MXMOTIONDATA_EXPORT mMotionJointFrictionData
    {
    public:
        mMotionJointFrictionData() {};

        ~mMotionJointFrictionData() {};

        bool enabled = false;

        double staticCoefficient = 0.0;   ///< 静摩擦系数
        double kineticCoefficient = 0.0;  ///< 动摩擦系数
        double viscousCoefficient = 0.0;  ///< 黏性摩擦系数

        /**
         * 旋转副摩擦力矩的等效半径。
         * 移动副暂时忽略该参数。
         */
        double effectiveRadius = 0.0;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointFrictionData& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointFrictionData& mgr);
    };

    /**
     * @brief 运动副限位参数
     *
     * 旋转副：
     *   lowerValue、upperValue 的内部单位为度（°），求解导出时统一换算 rad。
     *
     * 移动副：
     *   lowerValue、upperValue 使用软件统一长度单位。
     */
    class MXMOTIONDATA_EXPORT mMotionJointLimitData
    {
    public:
        mMotionJointLimitData() {};

        ~mMotionJointLimitData() {};

        bool enabled = false;

        bool lowerEnabled = false;
        bool upperEnabled = false;

        double lowerValue = 0.0;
        double upperValue = 0.0;

        /**
         * false：使用硬限位单边约束；
         * true：使用纯弹簧阻尼软限位。
         */
        bool penaltyOnly = false;

        // 接近上下限时的缓冲区宽度
        double lowerCushion = 0.0;
        double upperCushion = 0.0;

        // 缓冲区刚度
        double lowerStiffness = 0.0;
        double upperStiffness = 0.0;

        // 缓冲区阻尼
        double lowerDamping = 0.0;
        double upperDamping = 0.0;
        
        friend QDataStream& operator>>(QDataStream& in, mMotionJointLimitData& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointLimitData& mgr);
    };

    /**
     * @brief 运动副基类
     */
    class MXMOTIONDATA_EXPORT mMotionJointBase {
    public:
        mMotionJointBase(int id, JointType type, const QString& name = "", bool isUserRename = false);

        mMotionJointBase() {};
        virtual ~mMotionJointBase();

        /*
        * 获取是否为用户重命名
        */
        bool getIsUserRename();

        int getId() const;
        QString getName() const;
        void setName(const QString& name, bool isUserRename = true);
        void setType(JointType type);
        JointType getType() const;

        /**
        * @brief 获取当前运动副类型的中文名称
        * @return QString 中文名称 (如 "旋转副")
        */
        QString getTypeName() const;

        /**
        * @brief 静态辅助函数：将枚举类型转换为中文名称
        * @param type 运动副枚举类型
        * @return QString 中文名称
        */
        static QString getTypeName(JointType type);


        // 摩擦：整组读写，避免为每个参数暴露大量接口。
        const mMotionJointFrictionData& getFrictionData() const noexcept;
        void setFrictionData(const mMotionJointFrictionData& data);
        void clearFriction();
        bool supportsFriction() const noexcept;

        // 限位：旋转副和平移副共用一组“唯一自由坐标”限位数据。
        const mMotionJointLimitData& getLimitData() const noexcept;
        void setLimitData(const mMotionJointLimitData& data);
        void clearLimit();
        bool supportsLimit() const noexcept;

        virtual mMotionJointBase* copyJoint(int newId, const QString& newName) = 0;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointBase& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointBase& mgr);

    protected:
        /**
         * @brief 派生类复制时复制公共属性
         */
        void copyCommonDataTo(mMotionJointBase& target) const;

    protected:
        //是否为用户重命名
        bool m_isUserRename;

        int m_id;                       ///< 运动副ID
        QString m_name;                 ///< 运动副名称
        JointType m_type;               ///< 运动副类型

        mMotionJointFrictionData m_frictionData;
        mMotionJointLimitData m_limitData;
    };

}
