#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "motion/physics/SystemSMC.h"
#include "motion/physics/LinkMotor.h"
#include "motion/physics/LinkMotorLinear.h"
#include "motion/physics/LinkMotorRotation.h"
#include "motion/physics/Link.h"
#include "motion/functions/Function_Setpoint.h"

#include "mMotionResultData.h"
#include "mMotionControllerType.h"
#include "mMotionSensorBase.h"
#include "mMotionResultQueryService.h"
#include "mMotionSensorEvaluator.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cstdio>

namespace MechMainWindow {

    /**
     * @brief 带 PID 控制钩子的物理系统
     * @details 继承 MotionSystemSMC，重写 CustomEndOfStep：每个求解步末尾
     *          读取被控驱动自由度的实际值，与目标值（常数或轨迹）求差后经
     *          PID 计算输出，注入为驱动的速度设定值（零阶保持），实现闭环伺服控制。
     *          引擎本身零改动。
     *
     * PID 实现为自研采样式步进（不用引擎的 MotionControllerPID）：
     *  - 微分对反馈量求导（D on measurement），目标突变不产生输出尖刺；
     *  - 条件积分抗饱和：输出已饱和且误差仍同向时冻结积分，防止 windup。
     *
     * 线程约束：CustomEndOfStep 运行在引擎求解线程内，禁止调用任何 Qt API；
     *           日志先攒到 std::string 容器，求解结束后由求解线程统一 emit。
     */
    class mMotionPidSystem : public motion::MotionSystemSMC {
    public:
        /**
         * @brief 单个 PID 控制器的运行期状态
         */
        struct PidRuntime {
            // --- 求解前由求解线程填充的配置 ---
            int controllerId = -1;                                  ///< 控制器ID（结果回填用）
            std::string driveId;                                    ///< 驱动ID（motions_map 键）
            std::string markerI;                                    ///< 驱动 Marker I 名（定位 link 用）
            std::string markerJ;                                    ///< 驱动 Marker J 名
            std::string bodyI;                                      ///< 驱动作用体（Marker I 所属 part）
            std::string bodyJ;                                      ///< 驱动基体（Marker J 所属 part）
            MechMotionData::PidFeedbackType feedbackType = MechMotionData::PidFeedbackType::Position;
            double target = 0.0;                                    ///< 常数目标（无轨迹时生效）
            double kp = 1.0;
            double ki = 0.0;
            double kd = 0.0;
            double outputLimit = 0.0;                               ///< <=0 不限制

            // --- 量纲归一：角度传感器反馈为度，PID 数学域统一为弧度（输出 rad/s） ---
            bool feedbackDegToRad = false;                          ///< 角度(度)反馈时误差/前馈域 ×π/180

            // --- 马达身份指纹（常数驱动可用：接管前核验候选马达的函数值） ---
            bool hasFuncFingerprint = false;                        ///< 常数驱动才有指纹（表达式驱动无法求值）
            double expectedFuncValue = 0.0;                         ///< 装配时填入的 func_value

            // --- 目标轨迹（来自控制输入，常数/多项式/自定义曲线） ---
            int trajKind = -1;                                      ///< -1=常数；0..4 对应 ControlInputType
            double trajTimeStart = 0.0;
            double trajTimeEnd = 0.0;
            std::vector<double> trajCoeffs;                         ///< 多项式系数 a + b*t + c*t^2 + d*t^3
            std::vector<std::pair<double, double>> trajPoints;      ///< 自定义曲线点 (t, value)

            // --- 传感器反馈（装配期填充定义拷贝，求解期对引擎活状态求值） ---
            bool useSensorFeedback = false;                        ///< 反馈源为传感器
            double feedbackSign = 1.0;                              ///< 反馈符号（+1/-1）
            std::shared_ptr<MechMotionData::mMotionSensorBase> sensorCopy; ///< 传感器定义拷贝（求解期独立于GUI数据）
            std::vector<int> sensorBodyIds;                         ///< 参与求值的体ID（大地-1不入表）
            std::map<int, motion::MotionBody*> sensorBodies;        ///< 运行期定位结果（体ID->引擎体）
            // 角度传感器展开累积：signedAngleAboutAxis 只给 (-180°,180°] 主值，
            // 跨 ±180° 会跳变 360°；展开后的连续角才与体转角/Rigid_Z 一致
            double lastRawSensorAngle = 0.0;                        ///< 上一次未展开的原始角度
            double unwrappedSensorAngle = 0.0;                      ///< 展开后的连续角度
            bool hasLastSensorSample = false;                       ///< 是否已有上一步角度样本

            // --- 运行期状态 ---
            std::shared_ptr<motion::MotionLinkMotor> motorLink;    ///< 被接管的马达 link
            std::shared_ptr<motion::MotionFunction_Setpoint> setpoint; ///< 注入的速度设定值函数
            double integ = 0.0;                                     ///< 积分累加器 ∫e·dt
            double lastFeedback = 0.0;                              ///< 上一步反馈值（微分对测量）
            double lastTime = 0.0;                                  ///< 上一步时刻
            bool hasLastSample = false;                             ///< 是否已有上一步样本
            bool resolved = false;                                  ///< 是否已定位并接管马达
            bool failed = false;                                    ///< 定位/执行失败（跳过）
            std::vector<MechMotionData::mMotionPidSample> history;  ///< 每步采样（调参曲线数据源）

            /**
             * @brief t 时刻的目标值（轨迹模式下按时求值，区间外保持端点值）
             */
            double targetAt(double t) const {
                if (trajKind < 0)
                    return target;

                // 多项式：t 截断到时间窗内（区间外保持端点值）
                if (trajKind != 4) {
                    const double tc = std::max(trajTimeStart, std::min(trajTimeEnd, t));
                    double v = 0.0;
                    for (size_t i = 0; i < trajCoeffs.size(); ++i) {
                        v += trajCoeffs[i] * std::pow(tc, static_cast<double>(i));
                    }
                    return v;
                }

                // 自定义曲线：线性插值，区间外保持端点值
                if (trajPoints.empty())
                    return target;
                if (t <= trajPoints.front().first)
                    return trajPoints.front().second;
                if (t >= trajPoints.back().first)
                    return trajPoints.back().second;
                for (size_t i = 1; i < trajPoints.size(); ++i) {
                    if (t <= trajPoints[i].first) {
                        const auto& p0 = trajPoints[i - 1];
                        const auto& p1 = trajPoints[i];
                        const double span = p1.first - p0.first;
                        if (span <= 0.0)
                            return p1.second;
                        const double ratio = (t - p0.first) / span;
                        return p0.second + ratio * (p1.second - p0.second);
                    }
                }
                return trajPoints.back().second;
            }

            /**
             * @brief t 时刻目标的速度（前馈项）：多项式解析求导，曲线取段斜率，窗外保持常值故为 0
             */
            double targetRateAt(double t) const {
                if (trajKind < 0)
                    return 0.0;

                if (trajKind != 4) {
                    // 时间窗外目标保持端点值（常值），速度为 0
                    if (t <= trajTimeStart || t >= trajTimeEnd)
                        return 0.0;
                    double v = 0.0;
                    for (size_t i = 1; i < trajCoeffs.size(); ++i) {
                        v += static_cast<double>(i) * trajCoeffs[i]
                            * std::pow(t, static_cast<double>(i - 1));
                    }
                    return v;
                }

                // 自定义曲线：取 t 所在线性段的斜率，窗外为 0
                if (trajPoints.empty())
                    return 0.0;
                if (t <= trajPoints.front().first || t >= trajPoints.back().first)
                    return 0.0;
                for (size_t i = 1; i < trajPoints.size(); ++i) {
                    if (t <= trajPoints[i].first) {
                        const double span = trajPoints[i].first - trajPoints[i - 1].first;
                        if (span <= 0.0)
                            return 0.0;
                        return (trajPoints[i].second - trajPoints[i - 1].second) / span;
                    }
                }
                return 0.0;
            }

            /**
             * @brief 单步控制律：速度前馈 + PID（抗饱和条件积分 + 微分对测量）
             * @details u = ṙ*(t) + Kp·e + Ki·∫e·dt + Kd·(−ḟb)
             *          前馈承担目标速度，PID 只修正残差——跟踪滞后与目标突变
             *          后的积分超调同时消除；常数目标时前馈为 0，退化为纯 PID。
             * @return 本步输出（已限幅）
             */
            double stepPid(double t, double fb) {
                // PID 数学域统一为弧度/米（输出 rad/s 或 m/s）：
                // 角度传感器反馈为度，误差与前馈先归一到弧度，保证 Kp 量纲恒为 1/s
                const double k = feedbackDegToRad ? 0.017453292519943295 : 1.0; // π/180
                const double tgtS = targetAt(t) * k;
                const double ffS = targetRateAt(t) * k;
                const double fbS = fb * k;
                const double err = tgtS - fbS;

                double dt = 0.0;
                double dTerm = 0.0;
                if (hasLastSample) {
                    dt = t - lastTime;
                    if (dt > 1e-12) {
                        // 微分对反馈量求导（目标常数时与对误差求导等价，目标突变时无尖刺）
                        dTerm = -kd * (fbS - lastFeedback) / dt;
                    }
                    else {
                        dt = 0.0;
                    }
                }

                // 条件积分抗饱和：候选积分先算，若致输出越限且误差同向则冻结
                const double integCandidate = integ + err * dt;
                double uUnsat = ffS + kp * err + ki * integCandidate + dTerm;
                if (outputLimit > 0.0 && std::abs(uUnsat) > outputLimit
                    && err * uUnsat > 0.0) {
                    uUnsat = ffS + kp * err + ki * integ + dTerm;
                }
                else {
                    integ = integCandidate;
                }

                double u = uUnsat;
                if (outputLimit > 0.0) {
                    u = std::max(-outputLimit, std::min(outputLimit, u));
                }

                lastFeedback = fbS;
                lastTime = t;
                hasLastSample = true;
                return u;
            }
        };

        std::vector<PidRuntime> pidControllers;                     ///< 活跃控制器
        std::vector<std::string> pidMessages;                       ///< 求解期日志（结束后统一上报）

        /**
         * @brief 每步末尾执行 PID 控制律
         */
        void CustomEndOfStep() override {
            if (pidControllers.empty()) return;

            const double t = GetTimestepper()->GetTime();

            if (!m_linksResolved) {
                // 引擎若在 StartSolver 内才建 link，这里首步会扑空，下一步重试
                if (!resolvePidMotors())
                    return;
            }

            for (auto& rt : pidControllers) {
                if (rt.failed || !rt.resolved) continue;

                const double fb = readFeedback(rt);
                const double tgt = rt.targetAt(t);
                // 输出符号补偿：传感器反馈取反时 readFeedback 已乘 feedbackSign，
                // 这里再乘一次把 u 翻回马达约束的物理方向（见 outputSign 注释）
                const double u = rt.stepPid(t, fb) * outputSign(rt);
                rt.setpoint->SetSetpoint(u, t);

                // 驱动自由度反馈时马达测角即反馈真值，写回对齐；
                // 传感器反馈为角度时在 readSensorFeedback 内已写回
                if (!rt.useSensorFeedback) {
                    if (auto rot = std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motorLink)) {
                        rot->SetMotorRot(fb * rt.feedbackSign);
                    }
                }

                MechMotionData::mMotionPidSample sample;
                sample.m_time = t;
                sample.m_target = tgt;
                sample.m_feedback = fb;
                sample.m_error = tgt - fb;
                sample.m_output = u;
                rt.history.push_back(sample);

                // 求解线程内严禁 Qt 日志；前 30 步及每 50 步把控制环关键量
                // 攒进 pidMessages，求解结束后由求解线程统一 emit 到信息窗口
                const int stepIdx = static_cast<int>(rt.history.size()) - 1;
                if (stepIdx < 30 || stepIdx % 50 == 0) {
                    double mrotDbg = 0.0;
                    if (rt.motorLink) {
                        if (auto rot = std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motorLink))
                            mrotDbg = rot->GetMotorRot();
                    }
                    char buf[192];
                    snprintf(buf, sizeof(buf),
                        "[PID dbg] c%d t=%.3f fb=%.6f tgt=%.6f u=%.6f mrot=%.6f",
                        rt.controllerId, t, fb, tgt, u, mrotDbg);
                    pidMessages.emplace_back(buf);
                }
            }
        }

        /**
         * @brief 重置控制器运行状态（每次求解前调用）
         */
        void resetPidState() {
            pidControllers.clear();
            pidMessages.clear();
            m_linksResolved = false;
        }

    private:
        bool m_linksResolved = false;

        /**
         * @brief 读取反馈值：驱动自由度或传感器
         * @details 反馈符号翻转只作用于"数学域误差"（传感器原始值与目标同向、
         *          通过取反构造正误差），不应翻转最终控制输出 u——否则在传感器
         *          反馈模式下 u 与马达速度约束方向相反，误差越大反方向转得越快，
         *          正反馈失控。轨迹跟踪（驱动自由度反馈）未乘 feedbackSign 即正确，
         *          已验证，故符号不污染输出。
         */
        double readFeedback(PidRuntime& rt) {
            return (rt.useSensorFeedback ? readSensorFeedback(rt) : readDriveDofFeedback(rt))
                * rt.feedbackSign;
        }

        /**
         * @brief 输出符号修正：传感器反馈且取反时，马达速度命令需再乘 -1
         *         抵消 readFeedback 里的 feedbackSign，保证 u 的物理方向不变
         */
        double outputSign(const PidRuntime& rt) const {
            return rt.useSensorFeedback ? rt.feedbackSign : 1.0;
        }

        /**
         * @brief 读取被控驱动自由度的实际值
         */
        double readDriveDofFeedback(const PidRuntime& rt) const {
            auto rot = std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motorLink);
            if (rot) {
                return (rt.feedbackType == MechMotionData::PidFeedbackType::Velocity)
                    ? rot->GetMotorRot_dt() : rot->GetMotorRot();
            }
            auto lin = std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(rt.motorLink);
            if (lin) {
                return (rt.feedbackType == MechMotionData::PidFeedbackType::Velocity)
                    ? lin->GetMotorPos_dt() : lin->GetMotorPos();
            }
            return 0.0;
        }

        /**
         * @brief 传感器反馈：从引擎活体合成单帧，复用查询期同一套求值逻辑
         * @details 与曲线面板显示的传感器值同源同语义（mMotionResultQueryService::
         *          sensorValueAtFrame），大地引用（体不在表）自动落到全局语义；
         *          角度传感器在帧序列上展开主值（±180° 回绕补整圈），保证反馈连续
         */
        double readSensorFeedback(PidRuntime& rt) {
            if (!rt.sensorCopy || rt.sensorBodies.size() != rt.sensorBodyIds.size())
                return 0.0;

            // 合成只含相关体的单帧（位置/姿态/速度/加速度均取引擎当前状态）
            MechMotionData::mMotionFrame frame;
            for (const auto& kv : rt.sensorBodies) {
                const auto& b = *kv.second;
                MechMotionData::mMotionBodyFrame bf;
                const auto& pos = b.GetPos();
                bf.m_position = QVector3D(pos.x(), pos.y(), pos.z());
                const auto& vel = b.GetPos_dt();
                bf.m_velocity = QVector3D(vel.x(), vel.y(), vel.z());
                const auto& acc = b.GetPos_dtdt();
                bf.m_acceleration = QVector3D(acc.x(), acc.y(), acc.z());
                const auto& q = b.GetRot();
                bf.m_rotation = QQuaternion(q.e0(), q.e1(), q.e2(), q.e3());
                const auto& w = b.GetWvel_par();
                bf.m_angularVelocity = QVector3D(w.x(), w.y(), w.z());
                const auto& wa = b.GetWacc_par();
                bf.m_angularAcceleration = QVector3D(wa.x(), wa.y(), wa.z());
                frame.m_bodyFrames.emplace(kv.first, bf);
            }

            // 传感器自身分量模式 -> 曲线分量（全部→幅值）
            MechMotionData::CurveComponent comp = MechMotionData::CurveComponent::Magnitude;
            switch (rt.sensorCopy->getComponentMode()) {
            case MechMotionData::ComponentMode::X: comp = MechMotionData::CurveComponent::X; break;
            case MechMotionData::ComponentMode::Y: comp = MechMotionData::CurveComponent::Y; break;
            case MechMotionData::ComponentMode::Z: comp = MechMotionData::CurveComponent::Z; break;
            default: break;
            }

            bool ok = false;
            const double vRaw = mMotionResultQueryService::sensorValueAtFrame(
                rt.sensorCopy.get(), frame, comp, ok);
            if (!ok)
                return 0.0;

            // 角度传感器：主值角度展开成连续角（跨 ±180° 补整圈），否则反馈在 180° 处
            // 跳变 360°，PID 会把跳变当误差反向打满，造成持续振荡且与体转角曲线对不上
            if (rt.sensorCopy->getType() == MechMotionData::SensorType::Angle) {
                constexpr double kPi = 3.14159265358979323846;
                const double rawRad = vRaw * kPi / 180.0;
                const double unwrappedRad = rt.hasLastSensorSample
                    ? mMotionSensorEvaluator::unwrapAngle(
                        rawRad, rt.unwrappedSensorAngle * kPi / 180.0, true)
                    : rawRad;
                rt.unwrappedSensorAngle = unwrappedRad * 180.0 / kPi;
                rt.lastRawSensorAngle = vRaw;
                rt.hasLastSensorSample = true;

                // 反馈真值展开为连续角后，同步马达内部累计转角：
                // 马达 Update() 用自身坐标系测角（跨 ±π 回绕后丢整圈），后续 GUI 的
                // 驱动自由度曲线/马达定位校验都会读到漂移值。传感器反馈为角度时
                // 它就是驱动自由度的真值（经 feedbackSign），直接写回对齐。
                if (auto rot = std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motorLink)) {
                    rot->SetMotorRot(rt.unwrappedSensorAngle * kPi / 180.0 * rt.feedbackSign);
                }
                return rt.unwrappedSensorAngle;
            }
            return vRaw;
        }

        /**
         * @brief 定位每个控制器对应的马达 link 并接管其驱动函数
         * @return 系统中是否已存在马达 link（false=引擎尚未建 link，需稍后重试）
         * @details 马达 link 由闭源引擎从 motions_map 构建，我方无创建期指针，
         *          只能事后定位。定位顺序：按驱动ID名字 SearchLink → 按 I/J 体对
         *          匹配 → 唯一马达兜底；每个候选接管前用装配期填入的常数函数值
         *          做指纹校验，校验不过不予接管。任一控制器定位失败置 failed，
         *          由求解线程决定是否中止求解。
         */
    public:
        bool resolvePidMotors() {
            // 收集系统中所有马达类 link 及其体名
            struct MotorInfo {
                std::shared_ptr<motion::MotionLinkMotor> link;
                std::string name;
                std::string body1;
                std::string body2;
            };
            std::vector<MotorInfo> motors;
            for (const auto& link : Get_linklist()) {
                auto motor = std::dynamic_pointer_cast<motion::MotionLinkMotor>(link);
                if (!motor) continue;
                MotorInfo info;
                info.link = motor;
                if (auto* obj = dynamic_cast<motion::MotionObj*>(link.get())) {
                    info.name = obj->GetNameString();
                }
                if (auto* l = dynamic_cast<motion::MotionLink*>(link.get())) {
                    if (auto* b1 = dynamic_cast<motion::MotionBody*>(l->GetBody1()))
                        info.body1 = b1->GetNameString();
                    if (auto* b2 = dynamic_cast<motion::MotionBody*>(l->GetBody2()))
                        info.body2 = b2->GetNameString();
                }
                motors.push_back(info);
            }

            // ---- 传感器反馈体定位（体名=part id 字符串，与马达定位同一约定） ----
            {
                std::map<std::string, motion::MotionBody*> bodyByName;
                for (const auto& body : Get_bodylist()) {
                    if (auto* obj = dynamic_cast<motion::MotionObj*>(body.get())) {
                        bodyByName[obj->GetNameString()] = body.get();
                    }
                }
                for (auto& rt : pidControllers) {
                    if (!rt.useSensorFeedback)
                        continue;
                    for (int id : rt.sensorBodyIds) {
                        auto it = bodyByName.find(std::to_string(id));
                        if (it != bodyByName.end()) {
                            rt.sensorBodies[id] = it->second;
                        }
                    }
                    if (rt.sensorBodies.size() == rt.sensorBodyIds.size()) {
                        pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                            + " 传感器反馈体定位成功 " + std::to_string(rt.sensorBodies.size()) + " 个");
                    }
                    else if (Get_bodylist().size() == 1 && rt.sensorBodyIds.size() == 1) {
                        // 兜底：系统只有一个体且只引用一个体
                        rt.sensorBodies[rt.sensorBodyIds.front()] = Get_bodylist().front().get();
                        pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                            + " 传感器反馈体以唯一体兜底定位");
                    }
                    else {
                        rt.failed = true;
                        pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                            + " 传感器反馈体定位失败（" + std::to_string(rt.sensorBodies.size())
                            + "/" + std::to_string(rt.sensorBodyIds.size()) + "），已中止该控制器的接管");
                    }
                }
            }

            pidMessages.push_back("PID: 系统中共发现 " + std::to_string(motors.size()) + " 个马达类link"
                + (motors.empty() ? "" : "（如 " + motors.front().name + " / 体 " + motors.front().body1 + "-" + motors.front().body2 + "）"));

            for (auto& rt : pidControllers) {
                if (rt.failed) continue;

                // 策略1：按名字找
                std::shared_ptr<motion::MotionLinkMotor> found;
                const std::string candidates[] = {
                    rt.driveId,
                    "MOTION_" + rt.driveId,
                    "motion_" + rt.driveId,
                    "Motion_" + rt.driveId,
                };
                for (const auto& cand : candidates) {
                    auto link = SearchLink(cand.c_str());
                    if (link) {
                        auto motor = std::dynamic_pointer_cast<motion::MotionLinkMotor>(link);
                        if (motor && motorMatchesFingerprint(rt, *motor)) {
                            found = motor;
                            pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                                + " 按名字 '" + cand + "' 定位马达（指纹校验通过）");
                            break;
                        }
                    }
                }

                // 策略2：按 I/J 体名匹配
                if (!found) {
                    for (const auto& m : motors) {
                        if ((m.body1 == rt.bodyI && m.body2 == rt.bodyJ) ||
                            (m.body1 == rt.bodyJ && m.body2 == rt.bodyI)) {
                            if (motorMatchesFingerprint(rt, *m.link)) {
                                found = m.link;
                                pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                                    + " 按体对 (" + rt.bodyI + "," + rt.bodyJ + ") 定位马达 '" + m.name + "'（指纹校验通过）");
                                break;
                            }
                        }
                    }
                }

                // 策略3：系统里只有一个马达且只有一个控制器时直接配对
                if (!found && motors.size() == 1 && pidControllers.size() == 1
                    && motorMatchesFingerprint(rt, *motors.front().link)) {
                    found = motors.front().link;
                    pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                        + " 以唯一马达兜底定位 '" + motors.front().name + "'（指纹校验通过）");
                }

                if (!found) {
                    rt.failed = true;
                    pidMessages.push_back("PID: 控制器" + std::to_string(rt.controllerId)
                        + " 未能定位驱动马达（名字/体对候选均未通过指纹校验），已中止该控制器的接管");
                    continue;
                }

            // 接管：换速度函数为外部设定值，并立刻计算第一步输出
            rt.integ = 0.0;
            rt.hasLastSample = false;
            rt.hasLastSensorSample = false;
                rt.setpoint = std::make_shared<motion::MotionFunction_Setpoint>();
                found->SetMotorFunction(rt.setpoint);
                rt.motorLink = found;
                rt.resolved = true;

                const double t0 = GetTimestepper()->GetTime();
                // 与 CustomEndOfStep 一致：输出需乘 outputSign 抵消反馈取反
                const double u0 = rt.stepPid(t0, readFeedback(rt)) * outputSign(rt);
                rt.setpoint->SetSetpoint(u0, t0);
            }

            m_linksResolved = true;
            return true;
        }

    private:
        /**
         * @brief 马达身份指纹校验：常数驱动装配时的函数值应等于候选马达当前函数在 t=0 的值
         * @details 表达式驱动无法在 GUI 侧求值，跳过校验（仅靠名字/体对定位）
         */
        bool motorMatchesFingerprint(const PidRuntime& rt, motion::MotionLinkMotor& motor) const {
            if (!rt.hasFuncFingerprint)
                return true;
            auto func = motor.GetMotorFunction();
            if (!func) {
                // 无函数对象（引擎默认态）：无法证伪，放行由名字/体对定位兜底
                return true;
            }
            const double actual = func->Get_y(0.0);
            const double expect = rt.expectedFuncValue;
            const double tol = 1e-9 * std::max(1.0, std::abs(expect));
            return std::abs(actual - expect) <= tol;
        }
    };

}
