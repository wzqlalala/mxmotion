#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include "mMotionSensorType.h"

#include <QString>
#include <QVector3D>
#include <QDataStream>

namespace MechMotionData {

    /**
     * @brief 传感器基类
     * @details 所有多体动力学传感器的抽象基类，定义公共属性和接口
     */
    class MXMOTIONDATA_EXPORT mMotionSensorBase {
    public:
        /**
         * @brief 构造函数
         * @param id 传感器全局唯一ID
         * @param type 传感器类型
         * @param name 传感器名称
         */
        mMotionSensorBase(int id, SensorType type, const QString& name = "", bool isUserRename = false);

        /**
         * @brief 默认构造函数（用于序列化）
         */
        mMotionSensorBase();

        /**
         * @brief 虚析构函数
         */
        virtual ~mMotionSensorBase();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        void setName(const QString& name, bool isUserRename);
        QString getName() const;

        bool getIsUserRename() const;

        void setType(SensorType type);

        /**
         * @brief 获取传感器类型（运行时类型识别）
         */
        virtual SensorType getType() const = 0;

        /**
         * @brief 获取传感器类型的中文名称
         */
        QString getTypeName() const;

        // --- 坐标系设置 ---
        void setCoordSystem(CoordSystem coord);
        CoordSystem getCoordSystem() const;


        // --- 分量模式 ---
        void setComponentMode(ComponentMode mode);
        ComponentMode getComponentMode() const;

        // --- 启用状态 ---
        void setEnabled(bool enabled);
        bool isEnabled() const;

        // --- 复制 ---
        /**
         * @brief 复制当前传感器
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的传感器实例
         */
        virtual mMotionSensorBase* copySensor(int newId, const QString& newName) const = 0;

        // --- 序列化 ---
        friend QDataStream& operator>>(QDataStream& in, mMotionSensorBase& sensor);
        friend QDataStream& operator<<(QDataStream& out, const mMotionSensorBase& sensor);

    protected:
        /**
         * @brief 子类实现序列化读取
         */
        virtual void readFromStream(QDataStream& in) = 0;

        /**
         * @brief 子类实现序列化写入
         */
        virtual void writeToStream(QDataStream& out) const = 0;

        int m_id;                           ///< 传感器全局唯一ID
        QString m_name;                     ///< 传感器名称
        SensorType m_type;                  ///< 传感器类型

        CoordSystem m_coordSystem;          ///< 坐标系类型

        ComponentMode m_componentMode;      ///< 分量输出模式
        bool m_isEnabled;                   ///< 是否启用
        bool m_isUserRename;                ///< 是否用户手动重命名
    };

}
