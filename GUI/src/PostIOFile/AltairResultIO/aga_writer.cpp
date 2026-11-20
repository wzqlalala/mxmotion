#include "aga_writer.h"

#include "utf8_file.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace altairresultio {

namespace {

// 浮点格式化：固定 precision 位有效数字
std::string formatDouble(double v, int precision)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*g", precision, v);
    return buf;
}

// 数据集 → 官方 $RESULT_TYPE 类型后缀：矢量 (v)、张量 (t)、标量 (s)
const char* resultTypeSuffix(AgaDatasetType type)
{
    switch (type)
    {
    case AgaDatasetType::Vector: return "v";
    case AgaDatasetType::Tensor: return "t";
    case AgaDatasetType::Scalar: return "s";
    }
    return "s";
}

const char* bindingKeyword(AgaDatasetLocation location)
{
    return location == AgaDatasetLocation::Nodal ? "NODE" : "ELEMENT";
}

int valuesPerPoint(AgaDatasetType type)
{
    switch (type)
    {
    case AgaDatasetType::Scalar: return 1;
    case AgaDatasetType::Vector: return 3;
    case AgaDatasetType::Tensor: return 6;
    }
    return 0;
}

int expectedConnectivityCount(AgaElemType type)
{
    switch (type)
    {
    case AgaElemType::Tri3:     return 3;
    case AgaElemType::Quad4:    return 4;
    case AgaElemType::Tet4:     return 4;
    case AgaElemType::Hex8:     return 8;
    case AgaElemType::Wedge6:   return 6;
    case AgaElemType::Pyramid5: return 5;
    case AgaElemType::Tet10:    return 10;
    case AgaElemType::Hex20:    return 20;
    case AgaElemType::Rod:      return 2;
    case AgaElemType::Beam:     return 3;    // 两端 + 方向节点
    }
    return 0;
}

} // namespace

// ---------------------------------------------------------------------------
// 校验
// ---------------------------------------------------------------------------

std::string AgaWriter::validate(const AgaModel& model)
{
    if (model.nodes.empty())
        return "model has no nodes";

    std::unordered_set<int> nodeIds;
    nodeIds.reserve(model.nodes.size());
    for (const auto& node : model.nodes)
    {
        if (!nodeIds.insert(node.id).second)
            return "duplicate node id: " + std::to_string(node.id);
    }

    std::unordered_set<int> elemIds;
    elemIds.reserve(model.elements.size());
    for (const auto& elem : model.elements)
    {
        if (!elemIds.insert(elem.id).second)
            return "duplicate element id: " + std::to_string(elem.id);

        const int expected = expectedConnectivityCount(elem.type);
        if (static_cast<int>(elem.connectivity.size()) != expected)
            return "element " + std::to_string(elem.id) + " connectivity size mismatch, expect "
                 + std::to_string(expected);

        for (int nid : elem.connectivity)
        {
            if (nodeIds.find(nid) == nodeIds.end())
                return "element " + std::to_string(elem.id)
                     + " references missing node " + std::to_string(nid);
        }
    }

    const size_t nodeCount = model.nodes.size();
    const size_t elemCount = model.elements.size();
    for (const auto& frame : model.frames)
    {
        for (const auto& ds : frame.datasets)
        {
            const size_t per = static_cast<size_t>(valuesPerPoint(ds.type));
            const size_t points = ds.location == AgaDatasetLocation::Nodal ? nodeCount : elemCount;
            if (ds.values.size() != per * points)
                return "frame " + std::to_string(frame.index) + " dataset '" + ds.name
                     + "' value count mismatch, expect " + std::to_string(per * points)
                     + ", got " + std::to_string(ds.values.size());
        }
    }

    return {};
}

// ---------------------------------------------------------------------------
// 写出（官方 Altair ASCII：只含结果，网格走 FemWriter）
// ---------------------------------------------------------------------------

bool AgaWriter::write(const AgaModel& model,
                      const std::string& outputPath,
                      const AgaWriteOptions& opt,
                      std::string& error)
{
    error = validate(model);
    if (!error.empty())
        return false;

    std::ofstream out = detail::openBinaryWrite(outputPath);
    if (!out)
    {
        error = "cannot open file for write: " + outputPath;
        return false;
    }

    // 用大字符串分块写出，减少 IO 次数
    std::string buf;
    buf.reserve(1 << 20);
    const std::string eol = opt.crlf ? "\r\n" : "\n";
    auto flushBuf = [&out, &buf]() {
        out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
        buf.clear();
    };
    auto append = [&buf, &flushBuf, &eol](const std::string& line) {
        buf += line;
        buf += eol;
        if (buf.size() > (1u << 20))
            flushBuf();
    };

    // 官方要求文件第一行必须包含 "ALTAIR ASCII FILE"（注释也不能在它前面）
    append("ALTAIR ASCII FILE");
    // 注意：不要输出 "# 注释" 和 "$TITLE" —— HyperWorks 2022 读入器遇到后会把后续
    // $COLUMN_INFO/$RESULT_TYPE 判为缺失（"header is mandatory"），实测 2026-09-29。
    // 全部帧归同一子工况，构成动画步序列（瞬态多步 / 模态多阶）
    append("$SUBCASE = 1 \"MxSim Animation\"");

    // HyperView 2022 实测（2026-09-29）：$RESULT_TYPE 逐帧重复时，第 2 个时间步起
    // 云图变 "No Result"（结果类型被当成新实例，未挂到既有时间步上）。
    // 必须按结果类型分组：每种类型只声明一次组头，其后连续排 $TIME+数据块（官方瞬态布局）。
    struct Group
    {
        AgaDatasetLocation location;
        std::string resultHead;                        // $RESULT_TYPE 完整行
        std::vector<std::pair<size_t, size_t>> blocks; // (帧下标, 数据集下标)
    };
    std::vector<Group> groups;
    for (size_t f = 0; f < model.frames.size(); ++f)
    {
        for (size_t d = 0; d < model.frames[f].datasets.size(); ++d)
        {
            const auto& ds = model.frames[f].datasets[d];
            if (ds.location == AgaDatasetLocation::Elemental && !opt.writeElementalDatasets)
                continue;

            // $RESULT_TYPE = 标签(v/t/s)；节点矢量标注 Anim Source 作为变形动画驱动
            //（官方规定不标注时第一个矢量类型默认作动画源，这里显式标注）
            std::string head = "$RESULT_TYPE = " + ds.name
                             + "(" + resultTypeSuffix(ds.type) + ")";
            if (ds.type == AgaDatasetType::Vector && ds.location == AgaDatasetLocation::Nodal)
                head += " Anim Source";

            size_t gi = groups.size();
            for (size_t k = 0; k < groups.size(); ++k)
            {
                if (groups[k].resultHead == head)
                {
                    gi = k;
                    break;
                }
            }
            if (gi == groups.size())
                groups.push_back({ds.location, head, {}});
            groups[gi].blocks.emplace_back(f, d);
        }
    }

    // 组头：每个组都必须显式重发 $BINDING/$COLUMN_INFO。官方"头部延续"规则只覆盖
    // Subcase/Simulation/DataType，不含 BINDING——实测（2026-09-29）第二组不写 BINDING
    // 时读入器按默认值 ELEMENT 处理，节点结果被当成单元结果（官方示例也是每块都重复）
    for (const auto& group : groups)
    {
        append(std::string("$BINDING = ") + bindingKeyword(group.location));
        append("$COLUMN_INFO = ENTITY_ID");
        append(group.resultHead);

        for (const auto& block : group.blocks)
        {
            const auto& frame = model.frames[block.first];
            // 帧类型头：瞬态 $TIME；模态 $MODE/$FREQUENCY；都未设置则按静态步
            if (frame.timeSeconds >= 0.0)
            {
                append("$TIME = " + formatDouble(frame.timeSeconds, opt.precision) + " sec");
            }
            else if (frame.modeNumber > 0)
            {
                append("$MODE = " + std::to_string(frame.modeNumber));
                if (frame.frequencyHz >= 0.0)
                    append("$FREQUENCY = " + formatDouble(frame.frequencyHz, opt.precision) + " Hz");
            }

            // 数据行：每行一个实体 —— ENTITY_ID + 每点结果值（矢量 3 列/张量 6 列/标量 1 列）
            // values 按 model.nodes / model.elements 顺序存放，ENTITY_ID 从对应表取
            const auto& ds = frame.datasets[block.second];
            const int per = valuesPerPoint(ds.type);
            const bool nodal = ds.location == AgaDatasetLocation::Nodal;
            const size_t pointCount = nodal ? model.nodes.size() : model.elements.size();
            for (size_t p = 0; p < pointCount; ++p)
            {
                const int entityId = nodal ? model.nodes[p].id : model.elements[p].id;
                std::string line = std::to_string(entityId);
                for (int k = 0; k < per; ++k)
                    line += " " + formatDouble(ds.values[p * static_cast<size_t>(per)
                                                   + static_cast<size_t>(k)], opt.precision);
                append(line);
            }
        }
    }

    flushBuf();

    out.flush();
    if (!out)
    {
        error = "write failed (disk full?): " + outputPath;
        return false;
    }
    return true;
}

} // namespace altairresultio
