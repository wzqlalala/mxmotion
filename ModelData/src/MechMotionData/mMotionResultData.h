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
#include <QMatrix4x4>
#include <QQuaternion>

using namespace MxFunctions;
namespace MechMotionData
{

    /**
     * @brief 标记点单帧结果
     * @details 与 motion::marker_result 对齐，包含运动学参数
     */
    struct MXMOTIONDATA_EXPORT mMotionMarkerFrame {
        int m_markerId;                ///< 标记点 ID
        QVector3D m_worldPosition;     ///< 世界坐标位置
        QQuaternion m_worldRotation;   ///< 世界坐标方向
        QVector3D m_linearVelocity;    ///< 线速度
        QVector3D m_linearAcceleration;///< 线加速度
        QVector3D m_angularVelocity;   ///< 角速度
        QVector3D m_angularAcceleration;///< 角加速度

        mMotionMarkerFrame();
        ~mMotionMarkerFrame();

        friend QDataStream& operator>>(QDataStream& in, mMotionMarkerFrame& frame);
        friend QDataStream& operator<<(QDataStream& out, const mMotionMarkerFrame& frame);
    };

    /**
     * @brief 运动体单帧完整运动学数据
     */
    struct MXMOTIONDATA_EXPORT mMotionBodyFrame {
        QVector3D m_position;
        QVector3D m_velocity;
        QVector3D m_acceleration;
        QQuaternion m_rotation;
        QVector3D m_angularVelocity;
        QVector3D m_angularAcceleration;
        QVector3D m_displacement;
        double m_potentialEnergy;
        double m_kineticEnergy = 0.0;

        mMotionBodyFrame();
        ~mMotionBodyFrame();

        friend QDataStream& operator>>(QDataStream& in, mMotionBodyFrame& frame);
        friend QDataStream& operator<<(QDataStream& out, const mMotionBodyFrame& frame);
    };

    /**
     * @brief 运动副单帧完整结果数据
     */
    struct MXMOTIONDATA_EXPORT mMotionJointFrame {
        mMotionMarkerFrame m_nodeI;         ///< 节点I运动学
        mMotionMarkerFrame m_nodeJ;         ///< 节点J运动学
        QVector3D m_reactionForce;          ///< 约束反力
        QVector3D m_reactionTorque;         ///< 约束反力矩

        mMotionJointFrame();
        ~mMotionJointFrame();

        friend QDataStream& operator>>(QDataStream& in, mMotionJointFrame& frame);
        friend QDataStream& operator<<(QDataStream& out, const mMotionJointFrame& frame);
    };

    /**
     * @brief 驱动单帧完整结果数据
     */
    struct MXMOTIONDATA_EXPORT mMotionDriveFrame {
        mMotionMarkerFrame m_absCoord;      ///< 绝对坐标运动学
        double m_imposedValue;              ///< 驱动位移（转动驱动为度，曲线显示用）
        double m_imposedSpeed;              ///< 驱动速度（转动驱动为°/s）
        double m_imposedAcc;                ///< 驱动加速度（转动驱动为°/s²）
        bool m_isRotation = false;          ///< 转动驱动标志（ imposed 系列单位制判定用）
        QVector3D m_actuationForce;         ///< 驱动力
        QVector3D m_actuationTorque;        ///< 驱动力矩
        QVector3D m_reactionForce;          ///< 约束反力
        QVector3D m_reactionTorque;         ///< 约束反力矩

        mMotionDriveFrame();
        ~mMotionDriveFrame();

        friend QDataStream& operator>>(QDataStream& in, mMotionDriveFrame& frame);
        friend QDataStream& operator<<(QDataStream& out, const mMotionDriveFrame& frame);
    };

    struct MXMOTIONDATA_EXPORT mMotionFrame {
        double m_time; // 当前帧的时间戳 (如 0.01s)
        std::map<int, QMatrix4x4> m_bodyTransforms; // <运动体ID, 4x4变换矩阵>

        // 标记点结果数据
        std::map<int, mMotionMarkerFrame> m_markerFrames;
        // 运动体完整运动学数据（用于曲线后处理）
        std::map<int, mMotionBodyFrame> m_bodyFrames;
        // 运动副完整结果数据（用于曲线后处理）
        std::map<int, mMotionJointFrame> m_jointFrames;
        // 耦合副完整结果数据（用于曲线后处理）
        std::map<int, mMotionJointFrame> m_couplingFrames;
        // 驱动完整结果数据（用于曲线后处理）
        std::map<int, mMotionDriveFrame> m_driveFrames;

        friend QDataStream& operator>>(QDataStream& in, mMotionFrame& mgr);
        friend QDataStream& operator<<(QDataStream& out, const mMotionFrame& mgr);
    };

    /**
     * @brief PID 控制器单步采样数据
     * @details 求解期由求解线程在每个求解步记录，用于控制器曲线调参
     */
    struct MXMOTIONDATA_EXPORT mMotionPidSample {
        double m_time = 0.0;        ///< 采样时刻
        double m_target = 0.0;      ///< 目标值
        double m_feedback = 0.0;    ///< 反馈值（驱动自由度实际值）
        double m_error = 0.0;       ///< 误差 = 目标 - 反馈
        double m_output = 0.0;      ///< PID 输出（注入的速度设定值）
    };

    /**
     * @brief 柔性体单帧数据（刚柔耦合回放）
     * @details 求解期每步由进度回调记录（模态坐标 + 各接口体位姿）。
     * 节点变形恢复公式（渲染/后处理用）：
     *   x(t) = x0 + Psi_bottom * [db_1; ...; db_K; q(t)]
     * 其中 db_k = [p_k(t)-p_k0; log(R_k(t)*R_k0^T)]（接口体 k 相对初始位姿的六维增量，
     * 顺序与 m_interfacePose0 一致），Psi_bottom 为该柔体 m_psi 的全部 N 行
     * （m_psi 已省去 boundary 行，直接 N x m_total，m_total = 6K + n_modes）。
     */
    struct MXMOTIONDATA_EXPORT mMotionFlexFrame {
        double m_time = 0.0;
        QVector<double> m_modalQ;              ///< 模态坐标（n_modes）
        QVector<double> m_interfacePose;       ///< 7*K：[px,py,pz,qx,qy,qz,qw] * K 个接口体位姿
        friend QDataStream& operator>>(QDataStream& in, mMotionFlexFrame& frame);
        friend QDataStream& operator<<(QDataStream& out, const mMotionFlexFrame& frame);
    };

    /**
     * @brief 柔性体结果数据（刚柔耦合）
     * @details 一次性拓扑/振型数据 + 每帧模态坐标时程，供变形动画与后处理展开。
     */
    struct MXMOTIONDATA_EXPORT mMotionFlexBodyData {
        int m_bodyId = -1;              ///< 对应原刚体 ID（树/渲染映射）
        QString m_name;
        QVector<double> m_nodeX0;       ///< 3N 参考节点坐标
        QVector<int> m_tetIds;          ///< 4T 四面体连接（局部索引）
        QVector<int> m_surfaceTris;     ///< 3M 边界面三角（渲染拓扑，提取一次）
        QVector<double> m_psi;          ///< N x m_total 行优先（节点展开矩阵，含静/动模态列）
        int m_nModes = 0;               ///< 模态坐标数
        int m_nInterfaces = 0;          ///< 接口体数 K
        QVector<double> m_interfacePose0; ///< 7*K 初始位姿（增量基准）
        QVector<mMotionFlexFrame> m_frames;  ///< 每帧数据（与 m_frames 同步长）
        friend QDataStream& operator>>(QDataStream& in, mMotionFlexBodyData& d);
        friend QDataStream& operator<<(QDataStream& out, const mMotionFlexBodyData& d);
    };

    /**
      * @brief 驱动基类
      */
    class MXMOTIONDATA_EXPORT mMotionResultData {
    public:
        mMotionResultData() {};
        ~mMotionResultData() {};

        // 这是一个巨大的数组。假设仿真了10秒，步长0.01，这里就有1000个 mMotionFrame。
        // 播放器拖动进度条时，直接用进度条的 Index 去取 m_frames[index] 即可，极度高效！
        std::vector<mMotionFrame> m_frames;

        // 刚柔耦合：柔性体结果（一次性拓扑/振型 + 每帧模态坐标）
        QVector<mMotionFlexBodyData> m_flexBodies;

        // PID 控制器历史（<控制器ID, 采样序列>），内存态不落盘
        std::map<int, std::vector<mMotionPidSample>> m_pidHistories;

        // --- 辅助接口 ---
        int getFrameCount() const {
            return static_cast<int>(m_frames.size());
        }

        double getTotalTime() const {
            if (m_frames.empty()) return 0.0;
            return m_frames.back().m_time;
        }

        friend QDataStream& operator>>(QDataStream& in, mMotionResultData& mgr);
        friend QDataStream& operator<<(QDataStream& out, const mMotionResultData& mgr);
    };

}
