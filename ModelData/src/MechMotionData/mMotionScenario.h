#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mModelStruct.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>
#include "mMotionResultData.h"
#include "mMotionCurveData.h"
#include "mMotionCurveRequest.h"

namespace MechMotionData {

    /**
     * @brief 求解器算法类型枚举
     */
    enum class MXMOTIONDATA_EXPORT IntegratorType {
        EULER_IMPLICIT_LINEARIZED = 0,
        EULER_IMPLICIT_PROJECTED = 1,
        EULER_IMPLICIT = 2,
        TRAPEZOIDAL = 3,
        TRAPEZOIDAL_LINEARIZED = 4,
        HHT = 5,
        HEUN = 6,
        RUNGEKUTTA45 = 7,
        EULER_EXPLICIT = 8,
        LEAPFROG = 9,
        NEWMARK = 10,
        CUSTOM_INTEGRATOR = 11
    };

    /**
     * @brief 方程组求解器类型枚举
     */
    enum class MXMOTIONDATA_EXPORT SolverType {
        PSOR = 0,
        PSSOR = 1,
        PJACOBI = 2,
        PMINRES = 3,
        BARZILAIBORWEIN = 4,
        APGD = 5,
        ADDM = 6,
        SPARSE_LU = 7,
        SPARSE_QR = 8,
        PARDISO_MKL = 9,
        PARDISO_PROJECT = 10,
        MUMPS = 11,
        GMRES = 12,
        MINRES = 13,
        BICGSTAB = 14,
        CUSTOM_SOLVER = 15
    };

    /**
     * @brief 预设求解模式枚举
     */
    enum class MXMOTIONDATA_EXPORT PresetMode {
        FAST = 0,
        CONTACT = 1,
        ROBUST = 2,
        HIGH_PRECISION = 3
    };

    //class mMotionResultData;
    //class mMotionCurveData;

    /**
     * @brief 求解设置数据模型
     * @details 仅存储前处理的数值计算参数，绝对不包含计算结果
     */
    class MXMOTIONDATA_EXPORT mMotionScenario {
    public:
        DirectionPickState gravityPickState_;           ///< 重力方向输入的拾取会话状态
        mMotionScenario(int id, const QString& name = "");
        mMotionScenario()
            : m_id(0), m_endTime(1.0), m_stepSize(0.01),
            m_gravity(0.0, -9810.0, 0.0),
            m_usePreset(true), m_presetMode(PresetMode::FAST),
            m_integratorType(IntegratorType::HHT), m_integratorAlpha(-0.2), m_integratorBeta(0.25), m_integratorMaxIters(50),
            m_integratorTol(1e-8), m_solverType(SolverType::PSOR), m_solverMaxIters(100),
            m_solverTolerance(1e-10), m_solverVerbose(false), m_result(nullptr)
        {};

        ~mMotionScenario();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;
        void setName(const QString& name);
        QString getName() const;

        // --- 仿真时间控制 ---
        void setEndTime(double endTime);
        double getEndTime() const;

        void setStepSize(double stepSize);
        double getStepSize() const;

        // --- 物理环境设置 ---
        void setGravity(const QVector3D& gravity);
        QVector3D getGravity() const;

        // --- 求解模式（预设） ---
        void setUsePreset(bool use);
        bool getUsePreset() const;

        void setPresetMode(PresetMode mode);
        PresetMode getPresetMode() const;

        // --- 积分器高级设置 ---
		void setIntegratorType(IntegratorType type);
		IntegratorType getIntegratorType() const;

        void setIntegratorAlpha(double alpha);
        double getIntegratorAlpha() const;

        void setIntegratorBeta(double beta);
        double getIntegratorBeta() const;

        void setIntegratorMaxIters(int iters);
        int getIntegratorMaxIters() const;

        void setIntegratorTol(double tol);
        double getIntegratorTol() const;

        // --- 求解器高级设置 ---
        void setSolverType(SolverType type);
        SolverType getSolverType() const;

        void setSolverMaxIters(int iters);
        int getSolverMaxIters() const;

        void setSolverTolerance(double tol);
        double getSolverTolerance() const;

        void setSolverVerbose(bool verbose);
        bool getSolverVerbose() const;

        //拷贝
        mMotionScenario* copyScenario(int newId, const QString& newName);

        friend QDataStream& operator>>(QDataStream& in, mMotionScenario& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionScenario& mgr);

        void attachResult(mMotionResultData* res) {
            if (m_result) delete m_result;
            m_result = res;
        }
        mMotionResultData* getResult() const { return m_result; }
        bool hasResult() const { return m_result != nullptr; }

        // ==========================================
        // --- 2.5 标记点追踪配置 ---
        // ==========================================
        void addTrackedMarkerId(int markerId);
        void removeTrackedMarkerId(int markerId);
        bool isTrackingMarker(int markerId) const;
        const std::vector<int>& getTrackedMarkerIds() const { return m_trackedMarkerIds; }
        void clearTrackedMarkers() { m_trackedMarkerIds.clear(); }

        // ==========================================
        // --- 3. 曲线节点 (一个方案一个曲线节点) ---
        // ==========================================
        void setCurveNode(mMotionCurveData* curve);
        mMotionCurveData* getCurveNode() const { return m_curveNode; }
        void clearCurveNode();
        bool hasCurveNode() const { return m_curveNode != nullptr; }

        // ==========================================
        // --- 4. 曲线请求列表 (多曲线查看状态) ---
        // ==========================================
        void addCurveRequest(const mMotionCurveRequest& req);
        void updateCurveRequest(const mMotionCurveRequest& req);
        void removeCurveRequest(int requestId);
        bool hasCurveRequest(int requestId) const;
        const std::vector<mMotionCurveRequest>& getCurveRequests() const { return m_curveRequests; }
        void clearCurveRequests() { m_curveRequests.clear(); }

        // --- 当前查看的曲线选择状态（切换方案时自动恢复） ---
        void setCurrentCurveRequest(const mMotionCurveRequest& req) { m_currentCurveRequest = req; }
        const mMotionCurveRequest& getCurrentCurveRequest() const { return m_currentCurveRequest; }
        bool hasCurrentCurveRequest() const { return m_currentCurveRequest.m_sourceId >= 0; }
        void clearCurrentCurveRequest() { m_currentCurveRequest = mMotionCurveRequest(); }

    private:
        int m_id;                           ///< 求解方案ID
        QString m_name;                     ///< 方案名称

        double m_endTime;                   ///< 仿真总时间 (s)
        double m_stepSize;                  ///< 积分步长 (s)

        QVector3D m_gravity;                ///< 重力加速度向量（mm/s² 制，如 0, -9810, 0；0 向量=失重）


        // 求解模式（预设）
        bool m_usePreset;                   ///< 是否使用预设配置
        PresetMode m_presetMode;            ///< 预设模式

        // 积分器高级参数
        IntegratorType m_integratorType;    ///< 积分器/求解算法类型
        double m_integratorAlpha;           ///< HHT alpha 参数
        double m_integratorBeta;            ///< Newmark beta 参数
        int m_integratorMaxIters;           ///< 积分器最大迭代次数
        double m_integratorTol;             ///< 积分器收敛容差

        // 求解器高级参数
        SolverType m_solverType;            ///< 求解器类型
        int m_solverMaxIters;               ///< 求解器最大迭代次数
        double m_solverTolerance;           ///< 求解器收敛容差
        bool m_solverVerbose;               ///< 求解器输出日志

        // 动画节点 (唯一)
        mMotionResultData* m_result = nullptr;

        // 曲线节点 (唯一)
        mMotionCurveData* m_curveNode = nullptr;

        // 需要追踪输出时序数据的标记点ID列表
        std::vector<int> m_trackedMarkerIds;

        // 当前面板选中的曲线查看状态（与方案绑定）
        mMotionCurveRequest m_currentCurveRequest;

        // 曲线查看请求列表（多曲线状态，与方案绑定）
        std::vector<mMotionCurveRequest> m_curveRequests;

    };

}
