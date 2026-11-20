#include "mMotionFlexMxExport.h"
#include "mMotionFlexBridge.h"

#include <QFile>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <array>
#include <map>

namespace mMotionFlexMxExport {

namespace {

//样例数字风格：节点 7 位小数、材料 8 位、密度科学计数
QString fmtNode(double v)
{
    return QString::number(v, 'f', 7);
}
QString fmtMat(double v)
{
    return QString::number(v, 'f', 8);
}
QString fmtSci(double v)
{
    return QString::number(v, 'e', 8);   // 7.8900000e-09 同款
}

//逗号列表折行（样例 slavenodes 每行 8 个左右；每行 8 项，缩进对齐）
void writeIntList(QTextStream& out, const std::vector<int>& vals, int perLine)
{
    for (size_t i = 0; i < vals.size(); ++i) {
        out << vals[i];
        if (i + 1 < vals.size()) {
            out << (",");
            if ((i + 1) % perLine == 0)
                out << "\r\n";
            else
                out << " ";
        }
    }
    out << "\r\n";
}

}  // namespace

bool exportMxForMnf(const mMotionFlexBridge::FlexInput& in, const QString& mxPath, QString* err)
{
    const int nNodes = (int)(in.nodeCoords.size() / 3);
    const int nTets = (int)(in.tetIds.size() / 4);
    if (nNodes < 4 || nTets < 1) {
        if (err) *err = QString::fromUtf8("体网格不可用（节点 %1 / 单元 %2）").arg(nNodes).arg(nTets);
        return false;
    }
    if (in.attachGroups.empty()) {
        if (err) *err = QString::fromUtf8("附着组为空：MNF 导出需要显式连接端面节点（master 由组质心自动生成）");
        return false;
    }
    if (!(in.E > 0) || !(in.rho > 0)) {
        if (err) *err = QString::fromUtf8("材料属性无效（E=%1, rho=%2）").arg(in.E).arg(in.rho);
        return false;
    }

    QFile file(mxPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (err) *err = QString::fromUtf8("无法写入文件：%1").arg(mxPath);
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");

    //节点 ID 口径：优先模型网格全局 ID（与界面拾取一致）；未提供时回退局部 1..N
    const bool useGlobalIds = (int)in.nodeGlobalIds.size() == nNodes;
    auto nodeId = [&](int localIdx) -> int {
        return useGlobalIds ? in.nodeGlobalIds[localIdx] : (localIdx + 1);
    };

    // ---- 接口 master 先算（每组质心 + id 从全局最大之后顺排）----
    //master 必须写进 *node（求解器校验"主节点不存在"；参考样例的 master 同样来自 fem GRID）
    struct MasterRec { int id; double x = 0, y = 0, z = 0; };
    std::vector<MasterRec> masters;
    masters.reserve(in.attachGroups.size());
    int nextMaster = 0;
    for (int i = 0; i < nNodes; ++i)
        nextMaster = std::max(nextMaster, nodeId(i));
    ++nextMaster;
    for (const auto& group : in.attachGroups) {
        if (group.empty())
            continue;
        MasterRec m;
        m.id = nextMaster++;
        for (int local : group) {
            m.x += in.nodeCoords[3 * local];
            m.y += in.nodeCoords[3 * local + 1];
            m.z += in.nodeCoords[3 * local + 2];
        }
        const double inv = 1.0 / group.size();
        m.x *= inv; m.y *= inv; m.z *= inv;
        masters.push_back(m);
    }

    // ---- 工况头（与样例逐卡同构）----
    out << "*implicit_analysis\r\n";
    out << "*subcase, name = \"cms\"\r\n";
    out << "*analysis, type = generic\r\n";
    out << "*cms\r\n";
    out << "\"cms\"\r\n\r\n";
    out << "*globaloutput, type = mnf\r\n";
    out << "disp\r\n\r\n";

    // ---- 节点（单卡一次写完，按 id 升序——同 mOMxFile::writeNodeData 的 map 有序口径；
    //      网格节点 + master 虚拟节点合并排序）----
    {
        std::map<int, std::array<double, 3>> table;
        for (int i = 0; i < nNodes; ++i)
            table[nodeId(i)] = { in.nodeCoords[3 * i], in.nodeCoords[3 * i + 1], in.nodeCoords[3 * i + 2] };
        for (const auto& m : masters)
            table[m.id] = { m.x, m.y, m.z };
        out << "*node\r\n";
        for (const auto& kv : table) {
            out << kv.first << ", "
                << fmtNode(kv.second[0]) << ", "
                << fmtNode(kv.second[1]) << ", "
                << fmtNode(kv.second[2]) << "\r\n";
        }
        out << "\r\n";
    }

    // ---- 单元（D3T4 一阶四面体；id 1..T）----
    out << "*element, type = D3T4, part = \"part1\", section = \"Property1\"\r\n";
    for (int t = 0; t < nTets; ++t) {
        out << (t + 1) << ", "
            << nodeId(in.tetIds[4 * t]) << ", "
            << nodeId(in.tetIds[4 * t + 1]) << ", "
            << nodeId(in.tetIds[4 * t + 2]) << ", "
            << nodeId(in.tetIds[4 * t + 3]) << "\r\n";
    }
    out << "\r\n";

    // ---- 接口 RBE2：每组一个 connector ----
    for (size_t g = 0; g < masters.size(); ++g) {
        const MasterRec& m = masters[g];
        out << "*connector_part, name = \"rbe2\"\r\n";
        out << "*connector, type = rigid, n = " << (500000 + (int)g) << "\r\n";
        out << "*masternodes\r\n" << m.id << "\r\n";
        out << "*slavenodes\r\n";
        std::vector<int> slaves;
        slaves.reserve(in.attachGroups[g].size());
        for (int local : in.attachGroups[g])
            slaves.push_back(nodeId(local));
        std::sort(slaves.begin(), slaves.end());
        writeIntList(out, slaves, 8);
        out << "*slavedofs\r\n";
        writeIntList(out, { 1, 2, 3, 4, 5, 6 }, 6);
        out << "\r\n";
    }

    // ---- 材料/截面 ----
    out << "*material, type = isolinearelastic, name = \"Material1\"\r\n";
    out << fmtMat(in.E) << ", " << fmtMat(in.nu) << "\r\n";
    out << "*density\r\n" << fmtSci(in.rho) << "\r\n\r\n";
    out << "*section, type = structural, name = \"Property1\", mat = \"Material1\"\r\n";
    out << "*solidsection\r\n\r\n";

    // ---- 界面约束：ASET 引 set1（全部 master），DOF 1-6 ----
    out << "*constraint, type = ASET, name = \"CONSTRAINTS_UID_1\", set = \"set1\"\r\n";
    for (int d = 1; d <= 6; ++d)
        out << d << "\r\n";
    out << "\r\n";

    // ---- 缩减方法：CB + 阶数（首字段上截止频率留空，同样例 ", 10"）----
    const int order = in.modeCount > 0 ? in.modeCount : 6;
    out << "*CMSMethod, method = CB, name = \"cms\"\r\n";
    out << ", " << order << "\r\n\r\n";

    //注：不写 *unit 卡——本机 MxSim 3.0.2.0-dev(10-21) 不识别该关键字（M-INPUT-001
    // 直接失败）；求解器内部固定 tonne-N-mm-s 口径，与我们导出的 E(MPa)/ρ(tonne/mm³)
    // 自洽。新版求解器若要求显式单位卡再恢复（参考样例为 *unit + MGG, N, MM, S）。

    // ---- 接口节点集 ----
    out << "*set, type = node, name = \"set1\"\r\n";
    for (const auto& m : masters)
        out << m.id << "\r\n";
    out << "\r\n";

    out << "*end*\r\n";
    file.close();
    return true;
}

}  // namespace mMotionFlexMxExport
