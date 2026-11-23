#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mMotionResultData.h"
#include "mMotionCurveRequest.h"
#include "mMotionSensorBase.h"

#include <QMatrix4x4>
#include <QVector3D>

#include <vector>

namespace MechMainWindow
{
    /**
     * @brief 多体结果统一查询服务
     * @details 只做多体结果的帧定位、插值和数据提取，不持有结果数据和界面状态。
     */
    class mMotionResultQueryService
    {
    public:
        struct FrameSpan {
            int firstIndex = -1;
            int secondIndex = -1;
            double ratio = 0.0;
            bool isValid() const { return firstIndex >= 0 && secondIndex >= firstIndex; }
        };

        struct MarkerPosition {
            QVector3D position;
            QQuaternion rotation;
            bool isValid = false;
        };

        struct ResultSeries {
            QVector<double> times;
            QVector<double> values;
            bool isValid() const { return !times.isEmpty() && times.size() == values.size(); }
        };

        struct LoadSeries {
            QVector<double> times;
            QVector<QVector3D> forces;
            QVector<QVector3D> torques;
            bool isValid() const {
                return !times.isEmpty() &&
                    times.size() == forces.size() &&
                    times.size() == torques.size();
            }
        };

        static FrameSpan locateTime(
            const MechMotionData::mMotionResultData* result,
            double time,
            bool clampToRange = true);

        static bool getMarkerPosition(
            const MechMotionData::mMotionResultData* result,
            int markerId,
            double time,
            MarkerPosition& position);

        static bool getMarkerPositions(
            const MechMotionData::mMotionResultData* result,
            int markerId,
            QVector<QVector3D>& positions);

        /**
         * @brief 取运动体在任意时刻的世界矩阵（播放/暂停等落在物理帧之间的时刻）
         * @param initialCenterOfMass 运动体初始质心（与求解输出端构造矩阵所用枢轴一致，
         *        调用方取 mMotionBody::getCenterOfMass()）
         * @details 帧间按生成侧参数化插值：质心位移 lerp + 姿态四元数 slerp，再按
         *          T(dpos)*T(cm)*R*T(-cm) 重组。不能对矩阵平移列直接 lerp：其中的
         *          -R*cm 项走圆弧，线性插值误差按 |cm|*(1-cos(Δθ/2)) 放大，模型离
         *          世界原点越远、相邻帧转角越大，暂停时刻刚体偏得越离谱。
         *          帧内无体运动学数据时退回矩阵分量插值兜底。
         */
        static bool getBodyMatrix(
            const MechMotionData::mMotionResultData* result,
            int bodyId,
            const QVector3D& initialCenterOfMass,
            double time,
            QMatrix4x4& matrix);

        static ResultSeries getResultSeries(
            const MechMotionData::mMotionResultData* result,
            const MechMotionData::mMotionCurveRequest& request);

        /**
         * @brief 传感器时序曲线（查询期求值，纯 GUI 后处理）
         * @param component 曲线分量（X/Y/Z/幅值；仅对矢量型传感器变量有效）
         * @details 任一帧取不到所需数据（目标已删/无帧）时整条返回空序列
         */
        static ResultSeries getSensorSeries(
            const MechMotionData::mMotionResultData* result,
            const MechMotionData::mMotionSensorBase* sensor,
            MechMotionData::CurveComponent component);

        /**
         * @brief 单帧传感器值（查询期曲线与求解期 PID 反馈共用）
         * @param ok 输出：该帧是否能取到有效值
         * @details 支持体位移/体速度/体加速度、距离及其真实点运动学导数、角度传感器；
         *          关节反力/驱动输出传感器依赖结果帧中的 link 数据，返回 ok=false。
         *          数学与 getSensorSeries 完全一致（同一套求值器）。
         */
        static double sensorValueAtFrame(
            const MechMotionData::mMotionSensorBase* sensor,
            const MechMotionData::mMotionFrame& frame,
            MechMotionData::CurveComponent component,
            bool& ok);

        /**
         * @brief 控制器时序曲线（读取求解期记录的 PID 历史）
         * @param variable 曲线变量（目标值/反馈值/误差/输出）
         * @details 无该控制器历史（未求解/未装配）时返回空序列
         */
        static ResultSeries getControllerSeries(
            const MechMotionData::mMotionResultData* result,
            int controllerId,
            MechMotionData::CurveVariable variable);

        static LoadSeries getLoadSeries(
            const MechMotionData::mMotionResultData* result,
            MechMotionData::CurveSourceType sourceType,
            int sourceId);

    private:
        static double getMarkerValue(
            const MechMotionData::mMotionMarkerFrame& frame,
            MechMotionData::CurveVariable variable,
            MechMotionData::CurveComponent component);

        static double getVectorValue(
            const QVector3D& vector,
            MechMotionData::CurveComponent component);

        static bool appendFrameValue(
            const MechMotionData::mMotionFrame& frame,
            const MechMotionData::mMotionCurveRequest& request,
            double& value);

        static void interpolateBodyMatrix(
            const QMatrix4x4& first,
            const QMatrix4x4& second,
            double ratio,
            QMatrix4x4& matrix);
    };
}
