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
     * @brief 弹簧类型枚举
     * @details 对应 ADAMS 中 SPRING 的 dof_type
     */
    enum class MXMOTIONDATA_EXPORT SpringType {
        Translational,  ///< 轴向弹簧（拉压）
        Torsional,      ///< 扭转弹簧（旋转）
        BodyPoint       ///< 体-点弹簧（两端分别绑定到运动体+局部点）
    };

    /**
     * @brief 弹簧方向模式
     * @details 用于限定力或阻尼仅作用于拉伸、压缩或双向
     */
    enum class MXMOTIONDATA_EXPORT SpringDirectionMode {
        Bidirectional,  ///< 双向
        PositiveOnly,   ///< 仅正方向（拉压：拉伸；扭转：正转角）
        NegativeOnly    ///< 仅反方向（拉压：压缩；扭转：负转角）
    };

    /**
     * @brief 弹簧基类
     * @details 对应 ADAMS 中的 ADMSpring，关联到某个运动副
     *          通过运动副可推导出两个 marker 点
     *          拉压弹簧和扭转弹簧公用同一套字段，仅通过类型区分物理含义
     */
    class MXMOTIONDATA_EXPORT mMotionSpringBase {
    public:
        mMotionSpringBase(int id, SpringType type, const QString& name = "", bool isUserRename = false);
        mMotionSpringBase() {};
        virtual ~mMotionSpringBase();

        /*
        * 获取是否为用户重命名
        */
        bool getIsUserRename();

        int getId() const;
        void setName(const QString& name, bool isUserRename = true);
        QString getName() const;

        void setType(SpringType type);
        SpringType getType() const;

        /**
         * @brief 获取当前弹簧类型的中文名称
         */
        QString getTypeName() const;

        /**
         * @brief 静态辅助函数：将枚举类型转换为中文名称
         */
        static QString getTypeName(SpringType type);

        // --- 关联运动副 ---

        // --- 刚度 / 阻尼 ---
        void setStiffness(double k);
        double getStiffness() const;

        void setDamping(double c);
        double getDamping() const;

        // --- 预紧力（最终传给求解器的预载） ---
        void setPreload(double preload);
        double getPreload() const;

        // --- 弹簧方向模式（用于 UI 限制） ---
        void setSpringDirectionMode(SpringDirectionMode mode);
        SpringDirectionMode getSpringDirectionMode() const;

        void setDampingDirectionMode(SpringDirectionMode mode);
        SpringDirectionMode getDampingDirectionMode() const;

        // --- 自由值 / 安装值（拉压：长度；扭转：角度） ---
        void setFreeValue(double value);
        double getFreeValue() const;

        void setInstallValue(double value);
        double getInstallValue() const;

        // --- 曲线数据（可选，用于非线性） ---
        void setTable(const QVector<double>& x, const QVector<double>& y);
        QVector<double> getTableX() const;
        QVector<double> getTableY() const;

        bool hasTable() const;

        // --- 函数表达式（可选） ---
        void setForceFunction(const QString& expr);
        QString getForceFunction() const;

        void setDampingFunction(const QString& expr);
        QString getDampingFunction() const;

        void setDampingEnabled(bool enabled);
        bool isDampingEnabled() const;

        /**
         * @brief 复制当前弹簧对象
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的 mMotionSpringBase 实例
         */
        virtual mMotionSpringBase* copySpring(int newId, const QString& newName) = 0;

        friend QDataStream& operator>>(QDataStream& in, mMotionSpringBase& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionSpringBase& mgr);

    protected:
        /**
         * @brief 切换类型时清空类型相关参数
         */
        void clearTypeSpecificData();

    protected:
        //是否为用户重命名
        bool m_isUserRename;

        int m_id;                       ///< 弹簧ID
        QString m_name;                 ///< 弹簧名称
        SpringType m_type;              ///< 弹簧类型（轴向/扭转）


        double m_stiffness;             ///< 刚度（轴向：N/mm，扭转：N·mm/°——面板口径，求解导出统一换算）
        double m_damping;               ///< 阻尼（轴向：N·s/mm，扭转：N·mm/(°/s)——面板口径）
        double m_preload;               ///< 预紧力 N / 预载扭矩 N·mm（由安装值和自由值计算得到，叠加用户输入）

        SpringDirectionMode m_springDirectionMode;  ///< 弹簧作用方向模式
        SpringDirectionMode m_dampingDirectionMode; ///< 阻尼作用方向模式

        double m_freeValue;             ///< 自由值（轴向：长度，扭转：角度）
        double m_installValue;          ///< 安装值（轴向：长度，扭转：角度）

        QVector<double> m_tableX;       ///< 曲线横坐标（轴向：位移，扭转：转角）
        QVector<double> m_tableY;       ///< 曲线纵坐标（轴向：力，扭转：扭矩）
        bool m_hasTable;                ///< 是否使用曲线

        QString m_forceFunction;      ///< 力/扭矩函数表达式
        QString m_dampingFunction;      ///< 阻尼函数表达式
        bool m_dampingEnabled;          ///< 是否启用阻尼
    };

}
