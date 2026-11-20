#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QStringList>
#include <QMetaType>
#include <set>

namespace MGUIMesh
{
    /**
     * @brief 批量网格剖分任务结构
     *        不直接持有 MeshParam，避免所有引用该头文件的模块都依赖 MeshEngine。
     *        执行时由 mBatchMeshWorker 根据这些原始字段构造 MeshParam。
     */
    struct BatchMeshTask
    {
        QString taskId;              // 任务唯一标识
        QString name;                // 任务名称（可选，用于报告）
        int dim = 1;                 // 几何维度 1/2/3
        std::set<int> geoIds;        // 待剖分几何 ID 集合
        QStringList folders;         // 该任务负责的文件夹（规范化后的绝对路径，可选）；为空时匹配所有未归属任务的文件
        QString meshShape;           // 剖分类型（可选）：二维取 三角形/四边形主导/全四边形，三维取 四面体/六面体；为空按维度取默认

        // 网格参数原始字段
        double meshSize = 2.0;
        double maxSize = 10.0;
        double minSize = 1.0;
        bool smoothness = false;
        double smoothnessRatio = 1.2;
        bool curvature = false;
        double curvatureEigenValue = 15.0;
        bool proximity = false;
        double proximityValue = 2.0;

        bool autoMesh = false;       // 是否使用一键剖分
        int priority = 0;            // 优先级，数值越大越优先
        bool defaultSize = true;
    };
}

Q_DECLARE_METATYPE(MGUIMesh::BatchMeshTask)
