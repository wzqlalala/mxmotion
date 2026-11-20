#pragma once
//【刚柔耦合】AutoFlex 柔性体桥接：GUI 数据（AutoFlex 参数/材料/已有体网格）→ 引擎 FlexBodyBuilder
// 数据链（M2.1 材料 / M2.2 网格 / M2.4 求解接入）的转换层。
// 单位约定：材料与网格坐标沿用 GUI 当前单位制原样传入（引擎不换算），结果同单位制。

#include <QString>
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace motion {
class MotionSystem;
class MotionBody;
namespace modal { class MotionModalAssembly; }
}

#include "mMotionResultData.h"

namespace MechModelData {
class mModelData;
}

namespace mMotionFlexBridge {

// 一个 AutoFlex 条目解析后的完整输入
struct FlexInput {
    int flexId = -1;
    QString name;
    int bodyId = -1;          // 目标刚体
    bool bodyFixed = false;   // 目标刚体是否固定（首通：固定体演示悬臂闭环）
    double E = 0, nu = 0, rho = 0;
    int modeCount = 6;
    double connectionRadius = 0;   // >0 时附着点按半径收蜘蛛节点组
    //【面板直通】阻尼/截断频率（对齐 MechMotionData::FlexDampingType：0=无 1=模态比 2=瑞利）
    int dampingType = 2;
    double modalDampingRatio = 0.02;
    double rayleighAlpha = 10.0;
    double rayleighBeta = 2.0e-4;
    double freqCutoff = 0.0;       // >0：丢弃高于它的模态；0=不截断
    std::vector<double> nodeCoords;           // 3N
    std::vector<int> nodeGlobalIds;           // N：与 nodeCoords 同序的模型网格全局节点 ID
                                              // （mx 导出用，界面拾取口径；空=未提供，导出回退局部 1..N）
    std::vector<int> tetIds;                  // 4T，局部节点索引
    std::vector<std::vector<int>> attachGroups;  // 每附着点一组（局部节点索引）
    std::vector<double> autoAttachPositions;     // 3M：运动副作用点世界坐标（自动附着，容差内收节点组）
    bool valid = false;
    QString error;
};

struct FlexOutput {
    int nodes = 0, tets = 0, modes = 0, interfaces = 0;
    std::vector<std::shared_ptr<motion::MotionBody>> interfaceBodies;  // 接口刚体（运动副改挂目标）
    std::shared_ptr<motion::modal::MotionModalAssembly> assembly;             // 模态装配（求解后取 q/Ψ）
    // 引擎 DLL 内收集的渲染数据（跨 DLL cast 不可靠，勿在 GUI 侧重新遍历）
    std::vector<double> nodeX0;              // 3N
    std::vector<int> surfaceTris;            // 3M
    std::vector<double> psiInternal;         // N x (6K+m)
};

// 求解期采集的每步数据（进度回调线程写，纯 STL）
struct FlexCaptureFrameData {
    std::vector<double> times;      // 每步时间
    std::vector<double> qFlat;      // 每步 n_modes 个模态坐标（平铺）
    std::vector<double> poseFlat;   // 每步 7*K 个接口体位姿（px,py,pz,qx,qy,qz,qw）
};

// 求解完成后导出一次性数据 + 采集帧 到结果结构（渲染/后处理用）
void exportFlexResult(const FlexInput& in, const FlexOutput& out, const FlexCaptureFrameData& cap,
                      MechMotionData::mMotionFlexBodyData& dest);

// 确保目标部件存在四面体体网格：无则自动调用 MXMesh 剖分（多体形态无网格模块入口，
// "自动柔性体"语义 = 自动网格化）。elementSize 为 AutoFlex 面板的目标单元尺寸（0=引擎默认）。
// 返回 false = 无几何或剖分失败（log 说明原因）。
bool ensureTetraMesh(MechModelData::mModelData* modelData, const std::set<int>& partIds,
                     double elementSize, const std::function<void(const QString&)>& log);

// 面板预览：在当前视口把目标刚体的体网格表面画出来（半透明示意色），
// 让用户在建 AutoFlex 时看到网格（无网格则提示返回 false）
bool previewFlexBodyMesh(MechModelData::mModelData* modelData, int bodyId, const QString& message);
void clearFlexBodyPreview();

//【云图参照系·NX 口径】质量加权最优刚体拟合后逐节点偏差：
// 值 = |x(t) − (R·x0 + t)|。knownRot 传入时用已知平滑旋转（接口体姿态）替代 SVD
// （帧间稳定，UG 同形态分布：两端偏大、中间偏小的对称模式）；未传时回退 SVD。
// weights 空/非法 → 均匀权。N<3 或尺寸不符 → 全零。
QVector<double> nxContourValues(const QVector<double>& x0Flat, const QVector<QVector3D>& xt,
                                const QVector<double>& weights, const QQuaternion* knownRot = nullptr);

//【UG 口径云图】自由基模态形状提取 + 逐帧投影（多模态叠加，随帧演化）：
// 1) computeFreeMode：从网格几何算自由基前 3 阶弯曲模态形状（解析公式），
//    返回逐节点模态位移 + 节点质量 + 各模态质量（投影归一化用）
// 2) freeModeContour：把当前弹性变形投影到各阶模态，取 |Σ φᵢ(x)·qᵢ(t)| 作云图
//    ——多模态幅值不同步变化 → 云图形态随帧演化（与 UG 行为一致）
struct FreeModeData {
    static constexpr int kNumModes = 3;
    QVector<double> modeShapes[kNumModes]; // 每阶 3N：有符号模态位移
    QVector<double> nodeMasses;            // N：节点集总质量
    double modeMass[kNumModes] = {0, 0, 0}; // 各阶 φ^T M φ
};
FreeModeData computeFreeMode(const QVector<double>& x0Flat, const QVector<int>& tetIds, double density);
QVector<double> freeModeContour(const FreeModeData& fm, const QVector<double>& elasticFlat);

// GUI 侧数据收集：遍历 enabled 的 AutoFlex，解析材料/网格/附着点。
// 单个条目失败不中断整体，错误写入 FlexInput::error 并由调用方记录日志。
// sys 传入已 Parse 的引擎系统：用于收集目标刚体上运动副的作用点（自动附着）
std::vector<FlexInput> collectFlexInputs(motion::MotionSystem* sys, MechModelData::mModelData* modelData,
                                         const std::function<void(const QString&)>& log);

//【MNF 导出·GUI 侧自动附着】目标刚体上全部运动副作用点的世界坐标（3M 平铺）。
// 与求解期 auto-attach（引擎 linklist marker 系）同语义，但离线导出无引擎系统——
// 直接从 GUI 数据取：副↔体映射（action/base 任一为目标体）+ 各副类型的 origin。
// 弹簧/耦合副的作用点不在此列（首版仅运动副，与悬臂/连杆类算例一致）。
std::vector<double> collectJointAttachPointsGui(MechModelData::mModelData* modelData, int bodyId);

//【面板侧自动附着】体网格收集（无网格时按 elementSize 自动剖分兜底）：
// 输出节点坐标(3N)/单元(4T 局部索引)/全局→局部映射，供面板做容差收组
bool collectBodyTetraMesh(MechModelData::mModelData* modelData, const std::set<int>& partIds,
                          double elementSize,
                          std::vector<double>& coords, std::vector<int>& tets,
                          std::map<int, int>& globalToLocal,
                          const std::function<void(const QString&)>& log);

// 引擎侧构建：调 modal::BuildFlexBody 缩减并加入系统。
// 返回 {}; 失败抛 motion::MotionException（调用方捕获记录）。
FlexOutput buildOne(motion::MotionSystem& sys, const FlexInput& in);

// 关节改挂：目标刚体上所有 link 的 marker 转挂到最近的 interface body，然后移除原刚体。
// 返回成功转挂的 marker 数（0=刚体上没有 link）；刚体未找到返回 -1。
// 若没有任何 link 转挂且目标刚体原为固定，则固定首个 interface body（悬臂兜底）。
int reattachJointsToFlex(motion::MotionSystem& sys, const FlexInput& in, FlexOutput& out,
                         const std::function<void(const QString&)>& log);

}  // namespace mMotionFlexBridge
