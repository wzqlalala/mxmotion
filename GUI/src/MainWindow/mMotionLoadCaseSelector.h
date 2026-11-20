#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QString>
#include <QVector>
#include <QVector3D>
#include <QSet>

#include "mMotionResultQueryService.h"

namespace MechMainWindow
{
    /**
     * @brief 特征工况载荷筛选器
     * @details
     * 从多个运动副的全时程反力曲线中，筛出"特征时刻"并按时刻组织成静力工况。
     *
     * 规则（与《荷载转换设计方案》一致）：
     *  - 每个运动副分别处理：对其 6 条分量曲线（FX/FY/FZ/TX/TY/TZ）各取严格局部
     *    极大值、极小值时刻；另对合力 FM、合力矩 TM 曲线只取极大值时刻。
     *  - 每条曲线最多保留 peaksPerSignal 个时刻（按 |值| 从大到小取前 m 个），
     *    起始时刻(t[0])不取。
     *  - 汇总所有选中运动副的候选时刻，按时间值去重合并（相差 <= mergeTolerance
     *    的时刻视为同一时刻），得到 f 个特征时刻。
     *  - 每个特征时刻生成一个工况，包含"在该时刻取到极值"的所有运动副的六分量
     *    载荷（有的工况 1 个载荷，有的多个）。
     *
     * 结果通过 buildLoadCaseJson() 输出为 JSON（见 docs/loadcase_format_spec.json）。
     */
    class mMotionLoadCaseSelector
    {
    public:
        struct JointInput {
            int jointId = -1;
            QString jointName;
            QString jointType;              ///< 中文类型名（如 "旋转副"）
            int actionBodyId = -1;
            int baseBodyId = -1;
            QVector3D actionPoint;          ///< 导出模型中的世界坐标作用点（参考）
            mMotionResultQueryService::LoadSeries series;  ///< 全时程反力
        };

        struct Params {
            int peaksPerSignal = 4;         ///< 每条分量曲线最多取的极值点数 m
            double mergeTolerance = 1e-9;   ///< 跨副时刻合并容差（秒）
            bool synchronizedAllJoints = false; ///< 连杆同步模式：每个特征时刻包含全部关联副（而非仅取到极值的副）
        };

        /// 一个特征时刻 = 一个工况；loads 是该时刻所有取到极值的运动副载荷
        struct CaseLoad {
            int jointId = -1;
            QString jointName;
            QVector3D force;                ///< FX/FY/FZ
            QVector3D torque;               ///< TX/TY/TZ
        };
        struct LoadCase {
            QString name;                   ///< LC_<i>
            double timeSeconds = 0.0;
            QVector<CaseLoad> loads;
        };

        /**
         * @brief 执行筛选并生成工况
         * @param joints  选中运动副的全时程反力（每个含完整 times/forces/torques）
         * @param params  筛选参数
         * @param cases   输出：f 个工况
         * @param error   失败原因
         * @return 是否成功（至少产生 1 个含载荷的工况）
         */
        static bool select(const QVector<JointInput>& joints,
            const Params& params,
            QVector<LoadCase>& cases,
            QString& error);

    private:
        /// 将若干候选时刻按容差合并：返回合并后的代表时刻 -> 来源(joint索引)集合
        static void mergeTimes(const QVector<double>& candTimes,
            const QVector<int>& candJointIdx,
            double tolerance,
            QVector<double>& mergedTimes,
            QVector<QSet<int>>& timeToJoints);
    };
}
