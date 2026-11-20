#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QString>
#include <QVector>
#include <QMap>
#include <QVector3D>

namespace MechMainWindow
{
    /**
     * @brief 多体载荷 → 隐式静力工况生成器（无 UI 内核）
     * @details
     * 输入"多体反力工况数据 + 每副几何面绑定"，直接在当前模型生成隐式静力工况：
     *   每副建 RBE3（分布式耦合，参考点=副初始原点）→ 所选面全为平面的副建均布
     *   压力 Set → 逐工况建 ImplicitLinearStatic 工况并施加载荷
     *   （力接近面法向→均布压力 p=|F·n|/S，否则→RBE3 参考点集中力；力矩→RBE3 参考点）。
     * 供多体载荷导入面板（JSON 流程）与载荷导出面板（免导入直生成）共用。
     *
     * 输入载荷必须是分析体初始姿态系（bodyInitial）下的最终值——导出侧已完成
     * R(t)^T 反变换与端别符号处理；本内核在固定几何模型上按全局系直接施加。
     * 生成依赖隐式网格已剖分（几何面需有节点/单元），否则报错返回。
     */
    class MMAINWINDOW_EXPORT mMotionLoadCaseGenerator
    {
    public:
        /// 参与生成的运动副元数据（内核只用 id/name/actionPoint）
        struct GenJoint {
            int jointId = -1;
            QString name;
            QVector3D actionPoint;   ///< 副初始原点（固定模型上的世界坐标）
        };
        /// 一条载荷：某副在某工况下的力/力矩矢量（分析体初始姿态系最终值）
        struct GenLoad {
            int jointId = -1;
            QVector3D force;
            QVector3D torque;
        };
        /// 一个工况：特征时刻 + 该时刻的载荷集合
        struct GenCase {
            QString name;             ///< 导出侧工况名（LC_i，仅溯源用，不用于命名隐式工况）
            double timeSeconds = 0.0;
            QVector<GenLoad> loads;
        };

        /**
         * 执行生成（面绑定与网格前置校验，通过后才创建任何对象，不留半成品）
         * @param joints        参与生成的运动副
         * @param cases         工况列表（载荷为最终值）
         * @param faceBindings  jointId -> 用户拾取的几何面 id 集合（必须每副非空）
         * @param error         失败原因
         * @param nCases/nLoads/nRbe3 生成统计出参（可传 nullptr）
         */
        static bool generate(const QVector<GenJoint>& joints,
                             const QVector<GenCase>& cases,
                             const QMap<int, QVector<int>>& faceBindings,
                             QString& error,
                             int* nCases = nullptr,
                             int* nLoads = nullptr,
                             int* nRbe3 = nullptr);

    private:
        /// 为某副+其全部几何面建 RBE3（分布式耦合），返回参考点节点 id（失败 -1）
        static int buildRbe3ForJoint(const GenJoint& joint,
                                     const QVector<int>& geoFaceIds,
                                     QString& error,
                                     const QString& connName);
        /// 为全部几何面建均布压力 Set（面单元表面集合），返回 Set id（失败 -1）。
        /// 压力 pa = |F·n| / S（S=全部面外侧离散网格单元总面积，经 setAreaCache 返回）
        static int buildPressureSetForJoint(const QVector<int>& geoFaceIds,
                                            QString& error,
                                            const QString& setName,
                                            double& setArea);
        /// 判断几何面是否为完整平面（几何引擎 surfaceType==Plane）
        static bool isPlanarFace(int geoFaceId);

        mMotionLoadCaseGenerator() = delete;
    };
}
