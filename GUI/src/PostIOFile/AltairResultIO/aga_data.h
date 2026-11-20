#pragma once
// Altair 结果导出 - 数据结构
//
// 定义写出所需的最小数据模型：网格（节点/单元）与逐帧结果数据集（标量/矢量/张量）。
// 与求解器、后处理数据库、Qt 均无耦合。
//
// 写出端分工（见 doc/altair_ascii_format_notes.md）：
//   FemWriter 把网格部分写成 Nastran bulk .fem（官方 Altair ASCII 格式只含结果，网格必须由
//   单独的模型文件提供）；
//   AgaWriter 把逐帧结果写成官方 Altair ASCII 文本 .hwascii，按实体 ID 与 .fem 绑定。

#include <string>
#include <vector>

namespace altairresultio {

// 节点：未变形基准网格坐标
struct AgaNode
{
    int id = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

// 单元类型。
// 覆盖顺序即实现顺序：P1 支持前四种（壳 + 线性实体，覆盖绝大多数云图场景），
// P2 扩展高阶单元与梁杆。
enum class AgaElemType
{
    Tri3,     // 3 节点三角形壳
    Quad4,    // 4 节点四边形壳
    Tet4,     // 4 节点线性四面体
    Hex8,     // 8 节点线性六面体
    Wedge6,   // 6 节点线性五面体（楔形）
    Pyramid5, // 5 节点线性金字塔
    Tet10,    // 10 节点高阶四面体
    Hex20,    // 20 节点高阶六面体
    Rod,      // 2 节点杆
    Beam      // 2 节点梁
};

// 单元：类型 + 节点连接。
// connectivity 的节点顺序必须按 Nastran Bulk Data 对各类型的官方节点顺序给出
// （HEXA：G1-G4 底面逆时针、G5-G8 顶面对应；壳：法向 = 节点顺序右手定则）。
struct AgaElement
{
    int id = 0;
    int partId = 0;                // 所属部件（后处理部件 ID），FemWriter 按它归组属性卡
    AgaElemType type = AgaElemType::Quad4;
    std::vector<int> connectivity;
};

// 数据集类型
enum class AgaDatasetType
{
    Scalar,    // 每点 1 个值（云图）
    Vector,    // 每点 3 个值 X/Y/Z（矢量图、位移/变形）
    Tensor     // 每点 6 个值，分量顺序必须为 XX/YY/ZZ/XY/YZ/ZX（官方 Rules & Guidelines 明文，
               // 见 doc/altair_ascii_format_notes.md §6；复数结果需 (R/I)/(M/P) 后缀，暂未实现）
};

// 数据集位置
enum class AgaDatasetLocation
{
    Nodal,       // 节点结果，values 长度相对节点表
    Elemental    // 单元结果
};

// 一个数据集：一帧中的一类结果
// name 即官方 $RESULT_TYPE 的结果类型标签，同一帧内必须唯一
//（如矢量 "Displacement" 与标量 "Displacement Magnitude"）。
// values 数量约定：
//   Scalar + Nodal    : 节点数
//   Scalar + Elemental: 单元数
//   Vector + Nodal    : 节点数 * 3（X/Y/Z 依次排列）
//   Tensor + Nodal    : 节点数 * 6（Sxx/Syy/Szz/Sxy/Syz/Szx 依次排列）
// values 按对应实体表（AgaModel::nodes / elements）顺序存放，写出时带上 ENTITY_ID 列。
struct AgaDataset
{
    std::string name;                 // 结果类型标签，如 "Displacement"、"Stress-vonMises"
    AgaDatasetType type = AgaDatasetType::Scalar;
    AgaDatasetLocation location = AgaDatasetLocation::Nodal;
    std::vector<double> values;
};

// 一个结果帧：同一时刻/阶次下的一组数据集。
// 官方格式用 $TIME（瞬态）或 $MODE/$FREQUENCY（模态）区分帧，均未设置则按静态步写出。
struct AgaFrame
{
    int index = 0;                    // 帧序号（数据库帧号，用于注释与排序）
    std::string name;                 // 帧名（写进注释行，便于人工阅读）
    double timeSeconds = -1.0;        // >= 0 时写出 $TIME（瞬态）
    int modeNumber = 0;               // > 0 时写出 $MODE（模态阶次）
    double frequencyHz = -1.0;        // >= 0 时写出 $FREQUENCY（模态频率，配合 modeNumber）
    std::vector<AgaDataset> datasets;
};

// 顶层容器：网格 + 全部帧
struct AgaModel
{
    std::string modelName;            // 模型名，写入文件头
    std::vector<AgaNode> nodes;
    std::vector<AgaElement> elements;
    std::vector<AgaFrame> frames;
};

} // namespace altairresultio
