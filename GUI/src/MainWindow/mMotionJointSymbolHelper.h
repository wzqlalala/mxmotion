#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include <QVector3D>
#include <QString>

namespace mxr
{
    class mSymbolController;
    class CCSMRendWidget;
}
namespace MechMotionData
{
    class mMotionJointBase;
}

namespace MechMainWindow
{
    /**
     * @brief 连接副符号辅助类
     * 提供静态接口，用于创建/更新连接副符号，供运动副面板和动画模块调用。
     */
    class MMAINWINDOW_EXPORT mMotionJointSymbolHelper
    {
    public:
        /**
         * @brief 创建或更新连接副符号
         * @param symbolController 符号控制器
         * @param jointData 连接副数据（用于获取类型、ID、名称和基础位姿）
         * @param origin 符号中心（世界坐标）
         * @param axis 符号轴向（世界坐标，已归一化）
         */
        static void createOrUpdateJointSymbol(mxr::mSymbolController* symbolController,
            MechMotionData::mMotionJointBase* jointData,
            const QVector3D& origin,
            const QVector3D& axis,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 根据连接副类型创建/更新符号
         * @param symbolController 符号控制器
         * @param jointId 连接副 ID
         * @param type 连接副类型
         * @param name 连接副名称（用于文字标注，当前固定副暂未使用）
         * @param origin 符号中心（世界坐标）
         * @param axis 符号轴向（世界坐标，已归一化）
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         */
        static void createOrUpdateJointSymbol(mxr::mSymbolController* symbolController,
            int jointId,
            MxFunctions::JointType type,
            const QString& name,
            const QVector3D& origin,
            const QVector3D& axis,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 根据连接副类型创建/更新符号
         * @param symbolController 符号控制器
         * @param jointId 连接副 ID
         * @param type 连接副类型
         * @param name 连接副名称（用于文字标注，当前固定副暂未使用）
         * @param origin 符号中心（世界坐标）
         * @param spinAxisAction 运动体自旋矢量
         * @param crossPinAction 运动体十字销矢量
         * @param spinAxisBase 本体自旋矢量
         * @param crossPinBase 本体十字销矢量
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         */
        static void createOrUpdateUniversalSymbol(mxr::mSymbolController* symbolController,
            int jointId,
            const QString& name,
            const QVector3D& origin,
            const QVector3D& spinAxisAction,
            const QVector3D& crossPinAction,
            const QVector3D& spinAxisBase,
            const QVector3D& crossPinBase,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 根据垂直约束创建/更新符号
         * @param symbolController 符号控制器
         * @param jointId 连接副 ID
         * @param name 连接副名称
         * @param origin 符号中心（世界坐标）
         * @param actionDirection 运动体矢量（世界坐标）
         * @param baseDirection 基本体矢量（世界坐标）
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         */
        static void createOrUpdatePerpendicularSymbol(mxr::mSymbolController* symbolController,
            int jointId,
            const QString& name,
            const QVector3D& origin,
            const QVector3D& actionDirection,
            const QVector3D& baseDirection,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 根据恒速副创建/更新符号
         * @param symbolController 符号控制器
         * @param jointId 连接副 ID
         * @param name 连接副名称
         * @param origin 符号中心（世界坐标）
         * @param actionDirection 运动体矢量（世界坐标）
         * @param baseDirection 基本体矢量（世界坐标）
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         */
        static void createOrUpdateConstantVelocitySymbol(mxr::mSymbolController* symbolController,
            int jointId,
            const QString& name,
            const QVector3D& origin,
            const QVector3D& actionDirection,
            const QVector3D& baseDirection,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 删除连接副符号
         */
        static void clearJointSymbol(mxr::mSymbolController* symbolController, int jointId);

        /**
         * @brief 按数据对象分派创建运动副符号（仅在符号不存在时创建）
         * @param symbolController 符号控制器
         * @param jointData 连接副数据（按派生类型分派到对应创建接口）
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         * @note 供模型树父节点点击等"面板未编辑过也需渲染"的场景批量补建符号；
         *       显隐仍由树的 setSymbolShow 统一控制
         */
        static void ensureJointSymbolByData(mxr::mSymbolController* symbolController,
            MechMotionData::mMotionJointBase* jointData,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 创建或更新耦合副符号
         * @param symbolController 符号控制器
         * @param couplingId 耦合副 ID
         * @param type 耦合副类型
         * @param jointOrigin1 第一个运动副原点（世界坐标）
         * @param jointAxis1 第一个运动副轴向（世界坐标）
         * @param jointOrigin2 第二个运动副原点（世界坐标）
         * @param jointAxis2 第二个运动副轴向（世界坐标）
         * @param ratio 传动比 / 每弧度位移
         * @param rendWidget 视口控件，用于获取包围盒动态计算符号大小
         */
        static void createOrUpdateCouplingSymbol(mxr::mSymbolController* symbolController,
            int couplingId,
            MxFunctions::CouplingType type,
            const QVector3D& jointOrigin1,
            const QVector3D& jointAxis1,
            const QVector3D& jointOrigin2,
            const QVector3D& jointAxis2,
            double ratio,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 删除耦合副符号
         */
        static void clearCouplingSymbol(mxr::mSymbolController* symbolController, int couplingId);

    private:

        static void buildFixedSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& axis,
            float sceneScale);

        // 创建/更新柱面副符号：
        static void buildCylindricalSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& axis, float sceneScale);

        // 创建/更新移动副符号：使用通用线框符号绘制梯形及外伸短线
        static void buildTranslationalSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& axis, float sceneScale);

        static void buildScrewSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& axis, float sceneScale);

        static void buildRevoluteSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& axis, float sceneScale);

        // 创建/更新球面副符号
        static void buildSphericalSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& axis, float sceneScale);

        // 创建/更新点面副符号
        static void buildPointOnPlaneSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& planeNormal,
            float sceneScale);

        // 创建/更新点线副符号
        static void buildPointOnLineSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& lineDirection,
            float sceneScale);

        // 创建/更新点在线上副符号：弯曲弧线导轨（原点处切向 = curveDirection）
        static void buildPointOnCurveSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& curveDirection,
            float sceneScale);

        // 创建/更新线在线上副符号：两条相切曲线 + 接触点（原点处两切向平行）
        static void buildCurveOnCurveSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& contactTangent,
            float sceneScale);

        // 创建/更新点在面上副符号：三角剖分曲面片（离散网格语义，区别于点面副的规则网格）
        static void buildPointOnSurfaceSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& planeNormal,
            float sceneScale);

        // 创建/更新万向节符号
        static void buildUniversalSymbol(mxr::mSymbolController* symbolController,
            const QString& symbolName, const QVector3D& origin, const QVector3D& spinAxisAction, const QVector3D& crossPinAction, const QVector3D& spinAxisBase, const QVector3D& crossPinBase, float sceneScale);

        // 创建/更新齿轮齿条符号
        static void buildRackPinionSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& gearAxis,
            const QVector3D& rackDirection,
            float pitchRadius,
            float sceneScale);

        // 创建/更新平面副符号
        static void buildPlanarSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& planeNormal,
            bool allowRotation,
            float sceneScale);

        // 平行约束：原点 + 一个公共方向
        static void buildParallelSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& direction,
            float sceneScale);

        // 方向约束：原点 + 一个公共方向
        static void buildDirectionSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& direction,
            float sceneScale);

        // 垂直约束：原点 + 两个运动体各自的方向
        static void buildPerpendicularSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& actionDirection,
            const QVector3D& baseDirection,
            float sceneScale);

        // 恒速副：原点 + 两个运动体各自的方向
        static void buildConstantVelocitySymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& actionDirection,
            const QVector3D& baseDirection,
            float sceneScale);

        //创建/更新齿轮副
        static void buildGearCouplingSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& jointOrigin1,
            const QVector3D& jointAxis1,
            const QVector3D& jointOrigin2,
            const QVector3D& jointAxis2,
            double ratio,
            float sceneScale);

        //创建/更新齿轮齿条副
        static void buildRackPinionCouplingSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& gearOrigin,
            const QVector3D& gearAxis,
            const QVector3D& rackOrigin,
            const QVector3D& rackDirection,
            double displacementPerRadian,
            float sceneScale);

        // 工具：构造垂直于 axis 的两个正交方向
        static void buildOrthonormalBasis(const QVector3D& axis, QVector3D& u, QVector3D& v);

        // 根据视口包围盒计算符号基础尺度，rendWidget 为空时返回默认 0.3f
        static float computeSceneScale(mxr::CCSMRendWidget* rendWidget);

        mMotionJointSymbolHelper() = delete;
        ~mMotionJointSymbolHelper() = delete;
    };
}
