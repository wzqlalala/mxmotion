#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QVector3D>
#include <QString>

namespace mxr
{
    class mSymbolController;
    class CCSMRendWidget;
}
namespace MechMotionData
{
    class mMotionForceElement;
}

namespace MechMainWindow
{
    /**
     * @brief 多体矢量载荷符号辅助类
     * 提供静态接口，用于创建/更新 Bushing、矢量力、矢量力矩等载荷符号。
     */
    class MMAINWINDOW_EXPORT mMotionVectorSymbolHelper
    {
    public:
        /**
         * @brief 按数据对象分派补建力元符号（仅在符号不存在时创建，只建符号不做
         *        体高亮/作用点渲染）。供模型树父节点点击等面板未编辑过的场景
         *        批量渲染；显隐由树的 setSymbolShow 统一控制。
         * @param symbolController 符号控制器
         * @param forceData 力元数据（按派生类型分派）
         * @param rendWidget 视口控件，用于计算场景尺度
         */
        static void ensureForceElementSymbolByData(mxr::mSymbolController* symbolController,
            MechMotionData::mMotionForceElement* forceData,
            mxr::CCSMRendWidget* rendWidget = nullptr);

        /**
         * @brief 创建或更新轴套（Bushing）符号
         * @param symbolController 符号控制器
         * @param symbolName 符号名称（建议唯一）
         * @param origin 符号中心（世界坐标）
         * @param axis 符号轴向（世界坐标）
         * @param sceneScale 场景尺度，用于动态调整符号大小
         */
        static void buildBushingSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& axis,
            float sceneScale = 0.3f);

        /**
         * @brief 创建或更新矢量力符号
         * @param symbolController 符号控制器
         * @param symbolName 符号名称（建议唯一）
         * @param origin 力作用点（世界坐标）
         * @param forceVector 力矢量方向（世界坐标，决定箭头轴向）
         * @param sceneScale 场景尺度
         */
        static void buildVectorForceSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& forceVector,
            float sceneScale = 0.3f);

        /**
         * @brief 创建或更新标量力符号
         * @details 标量力复用矢量力箭头符号，仅在方向上按两点连线或固定方向显示。
         */
        static void buildScalarForceSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& forceVector,
            float sceneScale = 0.3f);

        /**
         * @brief 创建或更新矢量力矩符号
         * @param symbolController 符号控制器
         * @param symbolName 符号名称（建议唯一）
         * @param origin 力矩作用点（世界坐标）
         * @param torqueVector 力矩矢量方向（世界坐标，决定圆弧法向）
         * @param sceneScale 场景尺度
         */
        static void buildVectorTorqueSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName,
            const QVector3D& origin,
            const QVector3D& torqueVector,
            float sceneScale = 0.3f);

        /**
         * @brief 删除指定名称的矢量载荷符号
         */
        static void clearVectorSymbol(
            mxr::mSymbolController* symbolController,
            const QString& symbolName);

        // 根据视口包围盒计算符号基础尺度，rendWidget 为空时返回默认 0.3f
        static float computeSceneScale(mxr::CCSMRendWidget* rendWidget);

        // 构造垂直于 axis 的两个正交方向 u、v（与关节符号 helper 保持一致）
        static void buildOrthonormalBasis(
            const QVector3D& axis,
            QVector3D& u,
            QVector3D& v);

    private:
        mMotionVectorSymbolHelper() = delete;
        ~mMotionVectorSymbolHelper() = delete;
    };
}
