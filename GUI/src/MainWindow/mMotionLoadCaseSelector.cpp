
#include "mMotionLoadCaseSelector.h"

#include <algorithm>
#include <cmath>

namespace MechMainWindow
{
    // 从一条标量曲线找严格局部极值（剔除端点）：返回 (time, |value|)
    static QVector<QPair<double,double>> findExtremaImpl(
        const QVector<double>& times,
        const QVector<double>& values,
        bool peakAndValley)
    {
        QVector<QPair<double,double>> out;
        const int n = times.size();
        if (n < 3)
            return out;
        for (int i = 1; i < n - 1; ++i) {
            const double v = values[i];
            const bool isPeak = v > values[i - 1] && v > values[i + 1];
            const bool isValley = v < values[i - 1] && v < values[i + 1];
            if (isPeak || (peakAndValley && isValley))
                out.push_back(qMakePair(times[i], std::fabs(v)));
        }
        return out;
    }

    // 从一条曲线里筛出保留的时刻（按 |值| 取前 m 个，剔除 t[0]）
    static QVector<double> pickTopExtremaTimes(
        const QVector<double>& times,
        const QVector<double>& values,
        bool peakAndValley,
        int peaksPerSignal)
    {
        QVector<QPair<double,double>> ext =
            findExtremaImpl(times, values, peakAndValley);

        // 按幅值从大到小排序
        std::sort(ext.begin(), ext.end(),
            [](const QPair<double,double>& a, const QPair<double,double>& b) {
                return a.second > b.second;
            });

        QVector<double> out;
        for (int i = 0; i < ext.size() && i < peaksPerSignal; ++i)
            out.push_back(ext[i].first);
        return out;
    }

    bool mMotionLoadCaseSelector::select(const QVector<JointInput>& joints,
        const Params& params,
        QVector<LoadCase>& cases,
        QString& error)
    {
        cases.clear();
        if (joints.isEmpty()) {
            error = QStringLiteral("未选择任何运动副。");
            return false;
        }
        if (params.peaksPerSignal <= 0) {
            error = QStringLiteral("每条曲线极值点数必须为正。");
            return false;
        }

        // 1) 每个运动副独立筛极值时刻（候选时刻 + 该副索引）
        QVector<double> candTimes;
        QVector<int> candJointIdx;
        QVector<int> validJointIdx;   ///< 有有效时程的副索引（同步模式整组导出用）
        int validJoints = 0;

        for (int ji = 0; ji < joints.size(); ++ji) {
            const auto& series = joints[ji].series;
            if (!series.isValid() || series.times.size() < 3)
                continue;
            ++validJoints;
            validJointIdx.push_back(ji);

            const int n = series.times.size();
            // 构造 8 条标量曲线：FX,FY,FZ,FM, TX,TY,TZ,TM
            QVector<QVector<double>> comp(8);
            for (int c = 0; c < 8; ++c) comp[c].reserve(n);
            for (int i = 0; i < n; ++i) {
                const QVector3D& f = series.forces[i];
                const QVector3D& t = series.torques[i];
                comp[0].push_back(f.x());
                comp[1].push_back(f.y());
                comp[2].push_back(f.z());
                comp[3].push_back(f.length());      // FM
                comp[4].push_back(t.x());
                comp[5].push_back(t.y());
                comp[6].push_back(t.z());
                comp[7].push_back(t.length());      // TM
            }

            // 各曲线取极值时刻并汇总（FM/TM 只取峰，其余取峰+谷）
            for (int c = 0; c < 8; ++c) {
                const bool pv = (c != 3 && c != 7); // FM/TM 只取极大
                const QVector<double> ts = pickTopExtremaTimes(
                    series.times, comp[c], pv, params.peaksPerSignal);
                for (double t : ts) {
                    candTimes.push_back(t);
                    candJointIdx.push_back(ji);
                }
            }
        }

        if (validJoints == 0) {
            error = QStringLiteral("选中的运动副没有可用的求解结果数据。");
            return false;
        }
        if (candTimes.isEmpty()) {
            error = QStringLiteral("未能在反力曲线中找到任何极值点。");
            return false;
        }

        // 2) 跨副按时刻去重合并
        QVector<double> mergedTimes;
        QVector<QSet<int>> timeToJoints;
        mergeTimes(candTimes, candJointIdx, params.mergeTolerance,
            mergedTimes, timeToJoints);

        // 3) 按时刻生成工况：
        //    常规——该时刻下所有"取到极值"的副，取其在此时刻的力；
        //    连杆同步——每个特征时刻包含全部关联副在该时刻的力（机械臂单连杆静力分析用）。
        // 在该副曲线上找与 t 最接近的帧并取出载荷
        auto loadAtNearestFrame = [&joints](int ji, double t, CaseLoad& load) {
            const auto& series = joints[ji].series;
            int best = -1;
            double bestDt = 1e300;
            for (int i = 0; i < series.times.size(); ++i) {
                const double dt = std::fabs(series.times[i] - t);
                if (dt < bestDt) { bestDt = dt; best = i; }
            }
            if (best < 0)
                return false;
            load.jointId = joints[ji].jointId;
            load.jointName = joints[ji].jointName;
            load.force = series.forces[best];
            load.torque = series.torques[best];
            return true;
        };

        const QVector<int>& caseJointIdx = params.synchronizedAllJoints
            ? validJointIdx       // 同步：全部关联副
            : QVector<int>();     // 常规：按 timeToJoints 逐时刻取（见下）
        int caseNo = 1;
        for (int ti = 0; ti < mergedTimes.size(); ++ti) {
            const double t = mergedTimes[ti];
            LoadCase lc;
            lc.name = QStringLiteral("LC_%1").arg(caseNo++);
            lc.timeSeconds = t;

            if (params.synchronizedAllJoints) {
                for (int ji : caseJointIdx) {
                    CaseLoad load;
                    if (loadAtNearestFrame(ji, t, load))
                        lc.loads.push_back(load);
                }
            }
            else {
                for (int ji : timeToJoints[ti]) {
                    CaseLoad load;
                    if (loadAtNearestFrame(ji, t, load))
                        lc.loads.push_back(load);
                }
            }

            if (!lc.loads.isEmpty())
                cases.push_back(lc);
        }

        if (cases.isEmpty()) {
            error = QStringLiteral("筛选后未生成任何有效工况。");
            return false;
        }
        return true;
    }

    void mMotionLoadCaseSelector::mergeTimes(const QVector<double>& candTimes,
        const QVector<int>& candJointIdx,
        double tolerance,
        QVector<double>& mergedTimes,
        QVector<QSet<int>>& timeToJoints)
    {
        // 按时间升序，遍历合并相邻差 <= tolerance 的时刻
        QVector<int> order(candTimes.size());
        for (int i = 0; i < order.size(); ++i) order[i] = i;
        std::sort(order.begin(), order.end(),
            [&](int a, int b) { return candTimes[a] < candTimes[b]; });

        for (int oi : order) {
            const double t = candTimes[oi];
            const int ji = candJointIdx[oi];
            if (!mergedTimes.isEmpty() &&
                std::fabs(t - mergedTimes.last()) <= tolerance) {
                timeToJoints.last().insert(ji);
            } else {
                mergedTimes.push_back(t);
                QSet<int> s; s.insert(ji);
                timeToJoints.push_back(s);
            }
        }
    }
}
