#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include "mMotionControllerType.h"

#include <QString>
#include <QDataStream>

namespace MechMotionData {

    /**
     * @brief 控制器基类
     * @details 所有多体动力学控制器的抽象基类，定义公共属性和接口
     */
    class MXMOTIONDATA_EXPORT mMotionControllerBase {
    public:
        /**
         * @brief 构造函数
         * @param id 控制器全局唯一ID
         * @param type 控制器类型
         * @param name 控制器名称
         */
        mMotionControllerBase(int id, ControllerType type, const QString& name = "", bool isUserRename = false);

        /**
         * @brief 默认构造函数（用于序列化）
         */
        mMotionControllerBase();

        /**
         * @brief 虚析构函数
         */
        virtual ~mMotionControllerBase();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        void setName(const QString& name, bool isUserRename);
        QString getName() const;

        bool getIsUserRename() const;

        void setType(ControllerType type);

        /**
         * @brief 获取控制器类型（运行时类型识别）
         */
        virtual ControllerType getType() const = 0;

        /**
         * @brief 获取控制器类型的中文名称
         */
        QString getTypeName() const;

        // --- 启用状态 ---
        void setEnabled(bool enabled);
        bool isEnabled() const;

        // --- 复制 ---
        /**
         * @brief 复制当前控制器
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的控制器实例
         */
        virtual mMotionControllerBase* copyController(int newId, const QString& newName) const = 0;

        // --- 序列化 ---
        friend QDataStream& operator>>(QDataStream& in, mMotionControllerBase& controller);
        friend QDataStream& operator<<(QDataStream& out, const mMotionControllerBase& controller);

    protected:
        /**
         * @brief 子类实现序列化读取
         */
        virtual void readFromStream(QDataStream& in) = 0;

        /**
         * @brief 子类实现序列化写入
         */
        virtual void writeToStream(QDataStream& out) const = 0;

        int m_id;                           ///< 控制器全局唯一ID
        QString m_name;                     ///< 控制器名称
        ControllerType m_type;              ///< 控制器类型

        bool m_isEnabled;                   ///< 是否启用
        bool m_isUserRename;                ///< 是否用户手动重命名
    };

}
