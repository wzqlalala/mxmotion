#include "VtkDataBuilder.h"

#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mElementFunction.h"
#include "mPostEnumType.h"

#include <QDebug>
#include <QHash>
#include <QSet>
#include <QVector3D>

#include <cmath>
#include <cstring>
#include <limits>

namespace MIOFile
{
namespace
{
using PostData::mPostModelData;
using PostData::mPostPartData;
using PostData::postelementdata;
using PostData::EleInfo;
using PostData::PostCelllType;
using MxFunctions::ElementType;

// 失败原因回填（首因优先）：同步保留 qWarning 便于调试日志双通道排查
void fillError(QString* error, const QString& message)
{
    if (error != nullptr && error->isEmpty())
        *error = message;
}

void initializeNodeMemory(PostData::postnodedata& nodeData)
{
    nodeData.ids = nullptr;
    nodeData.nodeposD = nullptr;
    nodeData.nodeposF = nullptr;
}

void destroyModel(mPostModelData* modelData)
{
    if (modelData == nullptr)
        return;
    modelData->clearModelData();
    delete modelData;
}

// 与 Op2PostDataBuilder/NativeReadersDataBuilder 相同的发布协议
bool publishComponentValues(QPointer<PostData::mPostDataBase> database, int frameIndex,
                            const QString& variableName, const QString& componentName,
                            int resultType, const QHash<int, double>& values)
{
    if (database.isNull() || values.isEmpty())
        return false;

    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::lowest();
    for (auto valueIt = values.constBegin(); valueIt != values.constEnd(); ++valueIt)
    {
        minimum = std::min(minimum, valueIt.value());
        maximum = std::max(maximum, valueIt.value());
    }

    PostData::Compones component;
    component.varName = variableName;
    component.compoName = componentName;
    component.isNodeFace = false;
    component.frameID = frameIndex;
    component.isD = true;
    component.flag = resultType;
    component.value1 = values;
    database->setComponeValues(component);
    database->distributeFrameIdAndResult(frameIndex, values);
    database->distributeFrameIdAndResultType(frameIndex, resultType);
    database->setResultMaxMin(frameIndex,
                              {QPair<int, double>(0, maximum), QPair<int, double>(0, minimum)});
    return true;
}

// DataArray 数值统一按 double 取出（VTK 数组可为任意整型/浮点型）
bool arrayComponent(const vfr::DataArray& array, std::size_t tuple, std::size_t comp, double* out)
{
    switch (array.type())
    {
    case vfr::ScalarType::Float32: { auto r = array.component<float>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Float64: { auto r = array.component<double>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Int8: { auto r = array.component<std::int8_t>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt8: { auto r = array.component<std::uint8_t>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Int16: { auto r = array.component<std::int16_t>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt16: { auto r = array.component<std::uint16_t>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Int32: { auto r = array.component<std::int32_t>(tuple, comp); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt32: { auto r = array.component<std::uint32_t>(tuple, comp); if (!r) return false; *out = static_cast<double>(r.value()); return true; }
    case vfr::ScalarType::Int64: { auto r = array.component<std::int64_t>(tuple, comp); if (!r) return false; *out = static_cast<double>(r.value()); return true; }
    case vfr::ScalarType::UInt64: { auto r = array.component<std::uint64_t>(tuple, comp); if (!r) return false; *out = static_cast<double>(r.value()); return true; }
    }
    return false;
}

// 连接表/偏移按整数取出（扁平下标）
bool arrayIntValue(const vfr::DataArray& array, std::size_t index, int* out)
{
    switch (array.type())
    {
    case vfr::ScalarType::Int8: { auto r = array.value<std::int8_t>(index); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt8: { auto r = array.value<std::uint8_t>(index); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Int16: { auto r = array.value<std::int16_t>(index); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt16: { auto r = array.value<std::uint16_t>(index); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::Int32: { auto r = array.value<std::int32_t>(index); if (!r) return false; *out = r.value(); return true; }
    case vfr::ScalarType::UInt32: { auto r = array.value<std::uint32_t>(index); if (!r) return false; *out = static_cast<int>(r.value()); return true; }
    case vfr::ScalarType::Int64: { auto r = array.value<std::int64_t>(index); if (!r) return false; *out = static_cast<int>(r.value()); return true; }
    case vfr::ScalarType::UInt64: { auto r = array.value<std::uint64_t>(index); if (!r) return false; *out = static_cast<int>(r.value()); return true; }
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// VTK 单元类型码 → GUI ElementType/节点数。PostCelllType 枚举值与 VTK 类型码
// 同源（MX_TRIANGLE=5..MX_QUADRATIC_WEDGE=26），cell_types 直接透传类型码。
// PIXEL(8)/VOXEL(11) 节点顺序与 QUAD/HEXAHEDRON 不同，经 reorder 表调整。
// ---------------------------------------------------------------------------
struct VtkCellMapEntry
{
    ElementType elementType;
    int nodeCount;
    const int* reorder;   // 本地槽位 → 连接表槽位；nullptr = 原序
};

const int kPixelToQuad[4] = {0, 1, 3, 2};
const int kVoxelToHexa[8] = {0, 1, 3, 2, 4, 5, 7, 6};

bool vtkCellMap(int vtkType, VtkCellMapEntry* out)
{
    switch (vtkType)
    {
    case 1: *out = {ElementType::Mass1, 1, nullptr}; return true;      // VTK_VERTEX
    case 3: *out = {ElementType::D3R2, 2, nullptr}; return true;       // VTK_LINE
    case 5: *out = {ElementType::TRIA3, 3, nullptr}; return true;      // VTK_TRIANGLE
    case 8: *out = {ElementType::Q4, 4, kPixelToQuad}; return true;    // VTK_PIXEL
    case 9: *out = {ElementType::Q4, 4, nullptr}; return true;         // VTK_QUAD
    case 10: *out = {ElementType::D3T4, 4, nullptr}; return true;      // VTK_TETRA
    case 11: *out = {ElementType::D3H8, 8, kVoxelToHexa}; return true; // VTK_VOXEL
    case 12: *out = {ElementType::D3H8, 8, nullptr}; return true;      // VTK_HEXAHEDRON
    case 13: *out = {ElementType::D3P6, 6, nullptr}; return true;      // VTK_WEDGE
    case 14: *out = {ElementType::D3P5, 5, nullptr}; return true;      // VTK_PYRAMID
    case 21: *out = {ElementType::D3R3, 3, nullptr}; return true;      // VTK_QUADRATIC_EDGE
    case 22: *out = {ElementType::TRIA6, 6, nullptr}; return true;     // VTK_QUADRATIC_TRIANGLE
    case 23: *out = {ElementType::SSQ8, 8, nullptr}; return true;      // VTK_QUADRATIC_QUAD
    case 24: *out = {ElementType::D3T10, 10, nullptr}; return true;    // VTK_QUADRATIC_TETRA
    case 25: *out = {ElementType::D3H20, 20, nullptr}; return true;    // VTK_QUADRATIC_HEXAHEDRON
    case 26: *out = {ElementType::D3P15, 15, nullptr}; return true;    // VTK_QUADRATIC_WEDGE
    default:
        return false;
    }
}

// 展平数据集为 part 列表：MultiBlock（.vtm）递归展开子 block（实名取自父级
// block_metadata）；parallel XML 已由 vfr 聚合为单数据集；结构化类型与
// .pvd 集合明确不支持。suggestedName 为父级 block 名（空则退化命名）
bool collectParts(const vfr::DataSet& dataSet, const QString& suggestedName,
                  std::vector<const vfr::DataSet*>* parts, QStringList* partNames,
                  QString* error)
{
    switch (dataSet.kind)
    {
    case vfr::DataSetKind::MultiBlock:
    case vfr::DataSetKind::Collection:
        for (std::size_t index = 0; index < dataSet.blocks.size(); ++index)
        {
            QString childName;
            if (index < dataSet.block_metadata.size())
                childName = QString::fromStdString(dataSet.block_metadata[index].name).trimmed();
            if (!collectParts(dataSet.blocks[index], childName, parts, partNames, error))
                return false;
        }
        return true;
    case vfr::DataSetKind::StructuredPoints:
    case vfr::DataSetKind::StructuredGrid:
    case vfr::DataSetKind::RectilinearGrid:
    case vfr::DataSetKind::ImageData:
        fillError(error, QStringLiteral("暂不支持结构化网格类型（%1），仅支持 PolyData/UnstructuredGrid")
                             .arg(QString::fromLatin1(vfr::to_string(dataSet.kind))));
        return false;
    case vfr::DataSetKind::UnstructuredGrid:
    case vfr::DataSetKind::PolyData:
    {
        if (!dataSet.points.has_value() || dataSet.points->tuple_count() == 0)
        {
            fillError(error, QStringLiteral("数据集缺少 POINTS 几何数据"));
            return false;
        }
        const QString name = !suggestedName.isEmpty()
                                 ? suggestedName
                                 : QStringLiteral("Part_%1").arg(parts->size());
        partNames->append(name);
        parts->push_back(&dataSet);
        return true;
    }
    }
    return false;
}

// 单元装载计划：把 part 的全部 cell_block 归并为按序的受支持单元条目。
// General block（UnstructuredGrid）按 types 逐单元取类型；PolyData 各 block
// 按块类别 + 节点数归类（点/线/三角形/四边形）。不受支持（多段线、n 边形、
// 三角带、未知类型、节点数不匹配）跳过。计划是数据集的纯函数：几何构建与
// 懒读用同一入口保证各帧偏移一致
struct CellPlanEntry
{
    int localCell;      // 在本 part 内的全局单元序号（含被跳过单元，从 0 计）
    int blockIndex;     // 所属 cell_block 下标
    int cellInBlock;    // block 内单元序号
    int vtkType;
    int nodeCount;
    const int* reorder;
};

bool buildCellPlan(const vfr::DataSet& part, std::vector<CellPlanEntry>* plan, int* skippedCount)
{
    int localCell = 0;
    int skipped = 0;
    for (std::size_t blockIndex = 0; blockIndex < part.cell_blocks.size(); ++blockIndex)
    {
        const vfr::CellBlock& block = part.cell_blocks[blockIndex];
        const std::size_t cellCount = block.offsets.tuple_count();
        for (std::size_t cell = 0; cell < cellCount; ++cell, ++localCell)
        {
            int begin = 0;
            if (cell > 0 && !arrayIntValue(block.offsets, cell - 1, &begin))
                return false;
            int end = 0;
            if (!arrayIntValue(block.offsets, cell, &end))
                return false;
            const int nodeCount = end - begin;

            int vtkType = -1;
            if (block.kind == vfr::CellBlockKind::General)
            {
                if (!block.types.has_value())
                    return false;
                double typeValue = 0.0;
                if (!arrayComponent(*block.types, cell, 0, &typeValue))
                    return false;
                vtkType = static_cast<int>(typeValue);
            }
            else if (block.kind == vfr::CellBlockKind::Vertices)
            {
                vtkType = nodeCount == 1 ? 1 : -1;
            }
            else if (block.kind == vfr::CellBlockKind::Lines)
            {
                vtkType = nodeCount == 2 ? 3 : -1;
            }
            else if (block.kind == vfr::CellBlockKind::Polygons)
            {
                vtkType = nodeCount == 3 ? 5 : (nodeCount == 4 ? 9 : -1);
            }
            else
            {
                // TriangleStrips：需三角化展开，v1 不支持
                vtkType = -1;
            }

            VtkCellMapEntry mapped;
            if (vtkType <= 0 || !vtkCellMap(vtkType, &mapped) || mapped.nodeCount != nodeCount)
            {
                ++skipped;
                continue;
            }
            plan->push_back(CellPlanEntry{localCell, static_cast<int>(blockIndex),
                                          static_cast<int>(cell), vtkType, nodeCount, mapped.reorder});
        }
    }
    if (skippedCount != nullptr)
        *skippedCount = skipped;
    return true;
}

// 拓扑一致性：点数、受支持单元序列（类型+节点数）、连接值逐项比对；
// 多帧成组要求各帧拓扑一致（帧间仅变量值不同）
bool sameTopology(const vfr::DataSet& left, const vfr::DataSet& right)
{
    std::vector<const vfr::DataSet*> leftParts;
    std::vector<const vfr::DataSet*> rightParts;
    QStringList names;
    QString error;
    if (!collectParts(left, QString(), &leftParts, &names, &error) ||
        !collectParts(right, QString(), &rightParts, &names, &error))
        return false;
    if (leftParts.size() != rightParts.size())
        return false;
    for (std::size_t index = 0; index < leftParts.size(); ++index)
    {
        const vfr::DataSet* leftPart = leftParts[index];
        const vfr::DataSet* rightPart = rightParts[index];
        if (leftPart->points->tuple_count() != rightPart->points->tuple_count())
            return false;
        std::vector<CellPlanEntry> leftPlan;
        std::vector<CellPlanEntry> rightPlan;
        int skipped = 0;
        if (!buildCellPlan(*leftPart, &leftPlan, &skipped) ||
            !buildCellPlan(*rightPart, &rightPlan, &skipped))
            return false;
        if (leftPlan.size() != rightPlan.size())
            return false;
        for (std::size_t cell = 0; cell < leftPlan.size(); ++cell)
        {
            if (leftPlan[cell].vtkType != rightPlan[cell].vtkType ||
                leftPlan[cell].nodeCount != rightPlan[cell].nodeCount)
                return false;
        }
        // 连接值全量比对（各 block 的 connectivity 摊平比较）
        for (std::size_t block = 0; block < leftPart->cell_blocks.size(); ++block)
        {
            const auto& leftConn = leftPart->cell_blocks[block].connectivity;
            const auto& rightConn = rightPart->cell_blocks[block].connectivity;
            if (leftConn.value_count() != rightConn.value_count())
                return false;
            for (std::size_t value = 0; value < leftConn.value_count(); ++value)
            {
                int leftId = 0;
                int rightId = 0;
                if (!arrayIntValue(leftConn, value, &leftId) ||
                    !arrayIntValue(rightConn, value, &rightId) ||
                    leftId != rightId)
                    return false;
            }
        }
    }
    return true;
}

// 分量显示名：VTK 数组不带分量名，按分量数约定命名（与 GUI 矢量模/张量
// vonMises 派生口径对齐）
QStringList componentNames(const QString& variableName, int componentCount)
{
    switch (componentCount)
    {
    case 1:
        return QStringList{variableName};
    case 2:
        return QStringList{QStringLiteral("X"), QStringLiteral("Y")};
    case 3:
        return QStringList{QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")};
    case 6:
        return QStringList{QStringLiteral("XX"), QStringLiteral("YY"), QStringLiteral("ZZ"),
                           QStringLiteral("XY"), QStringLiteral("YZ"), QStringLiteral("ZX")};
    default:
    {
        QStringList names;
        for (int index = 1; index <= componentCount; ++index)
            names.append(QStringLiteral("C%1").arg(index));
        return names;
    }
    }
}

// 取一个元组的显示值：常规分量直取；Mag（X/Y/Z 模长）与 vonMises（六分量
// 对称张量，分量序 XX,YY,ZZ,XY,YZ,ZX）读取期派生。vonMises 统一用三维实体
// 公式（VTK 单元数据不区分壳/实体口径，不做逐单元壳公式回退）
bool resolveValue(const vfr::DataArray& array, std::size_t tuple,
                  int componentIndex, bool isDerivedVonMises, bool isDerivedMag,
                  double* out)
{
    double values[6] = {};
    const std::size_t count = isDerivedMag ? 3 : (isDerivedVonMises ? 6 : 1);
    const int offset = isDerivedMag || isDerivedVonMises ? 0 : componentIndex;
    for (std::size_t index = 0; index < count; ++index)
    {
        if (!arrayComponent(array, tuple, offset + index, &values[index]))
            return false;
    }
    if (isDerivedMag)
    {
        *out = std::sqrt(values[0] * values[0] + values[1] * values[1] + values[2] * values[2]);
        return true;
    }
    if (isDerivedVonMises)
    {
        const double sxx = values[0];
        const double syy = values[1];
        const double szz = values[2];
        const double sxy = values[3];
        const double syz = values[4];
        const double szx = values[5];
        *out = std::sqrt(0.5 * ((sxx - syy) * (sxx - syy) + (syy - szz) * (syy - szz) +
                                (szz - sxx) * (szz - sxx)) +
                         3.0 * (sxy * sxy + syz * syz + szx * szx));
        return true;
    }
    *out = values[0];
    return true;
}
} // namespace

// ---------------------------------------------------------------------------
// buildAll
// ---------------------------------------------------------------------------
bool VtkDataBuilder::buildAll(const std::vector<vfr::DataSet>& frames,
                              const QStringList& fileNames,
                              QPointer<PostData::mPostDataBase> db,
                              QString* error,
                              QStringList* notes)
{
    if (frames.empty() || frames.size() != static_cast<std::size_t>(fileNames.size()))
    {
        fillError(error, QStringLiteral("帧文件列表为空或与文件名数目不一致"));
        return false;
    }
    if (!buildGeometry(frames, db, error, notes))
        return false;
    return buildResultFrames(frames, fileNames, db, error, notes);
}

bool VtkDataBuilder::buildGeometry(const std::vector<vfr::DataSet>& frames,
                                   QPointer<PostData::mPostDataBase> db,
                                   QString* error,
                                   QStringList* notes)
{
    if (db.isNull())
    {
        fillError(error, QStringLiteral("post database is null"));
        return false;
    }

    std::vector<const vfr::DataSet*> parts;
    QStringList partNames;
    if (!collectParts(frames.front(), QString(), &parts, &partNames, error))
    {
        qWarning() << "VtkDataBuilder::buildGeometry: collect parts failed:" << *error;
        return false;
    }
    if (parts.empty())
    {
        fillError(error, QStringLiteral("文件中没有可显示的网格数据"));
        return false;
    }

    // 总点数（跨 part 顺序编号，节点真实 id = 全局连续序号 + 1）
    std::size_t totalPoints = 0;
    for (const vfr::DataSet* part : parts)
    {
        if (part->points->component_count() != 3)
        {
            fillError(error, QStringLiteral("POINTS 数组分量数异常（%1）")
                                 .arg(static_cast<int>(part->points->component_count())));
            return false;
        }
        totalPoints += part->points->tuple_count();
    }
    if (totalPoints == 0 || totalPoints > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        fillError(error, QStringLiteral("节点总数 %1 超出支持范围").arg(static_cast<qlonglong>(totalPoints)));
        return false;
    }

    QPointer<mPostModelData> modelData = new mPostModelData();
    modelData->setDataFlag(true);
    initializeNodeMemory(modelData->_nodedata);

    auto& nodeData = modelData->_nodedata;
    const int nodeCount = static_cast<int>(totalPoints);
    nodeData.nodeamount = nodeCount;
    nodeData.isDle = false;
    nodeData.ids = new int[nodeCount];
    nodeData.nodeposF = new float[nodeCount * 3];
    nodeData.nodeidmap.reserve(nodeCount);

    int nodeIndex = 0;
    for (const vfr::DataSet* part : parts)
    {
        const vfr::DataArray& points = *part->points;
        const std::size_t partPointCount = points.tuple_count();
        for (std::size_t local = 0; local < partPointCount; ++local, ++nodeIndex)
        {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
            if (!arrayComponent(points, local, 0, &x) ||
                !arrayComponent(points, local, 1, &y) ||
                !arrayComponent(points, local, 2, &z))
            {
                fillError(error, QStringLiteral("POINTS 数组数值类型不支持"));
                destroyModel(modelData.data());
                return false;
            }
            nodeData.ids[nodeIndex] = nodeIndex + 1;
            nodeData.nodeidmap.insert(nodeIndex + 1, nodeIndex);
            nodeData.nodeposF[nodeIndex * 3] = static_cast<float>(x);
            nodeData.nodeposF[nodeIndex * 3 + 1] = static_cast<float>(y);
            nodeData.nodeposF[nodeIndex * 3 + 2] = static_cast<float>(z);
        }
    }

    // 各 part 单元装载（单元真实 id 跨 part 全局递增，保证 cell_rid 唯一）
    int partId = 0;
    int globalElementId = 0;
    int skippedTotal = 0;
    QSet<QString> skipNotes;
    for (std::size_t partIndex = 0; partIndex < parts.size(); ++partIndex)
    {
        const vfr::DataSet* part = parts[partIndex];
        std::vector<CellPlanEntry> plan;
        int skipped = 0;
        if (!buildCellPlan(*part, &plan, &skipped))
        {
            fillError(error, QStringLiteral("单元连接数据解析失败（Part %1）").arg(partIndex));
            destroyModel(modelData.data());
            return false;
        }
        skippedTotal += skipped;
        if (plan.empty())
        {
            skipNotes.insert(QStringLiteral("part %1 has no supported cells").arg(partIndex));
            ++partId;
            continue;
        }

        QPointer<mPostPartData> partData = new mPostPartData(partId);
        partData->setPartName(partNames.value(static_cast<int>(partIndex),
                                              QStringLiteral("Part_%1").arg(partIndex)));

        postelementdata& elementData = partData->_parteledata;
        const int elementCount = static_cast<int>(plan.size());
        elementData.cell_types.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_rid.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_node_counts.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_node_offsets.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_info.resize(static_cast<std::size_t>(elementCount));

        std::size_t totalNodeIds = 0;
        for (const CellPlanEntry& entry : plan)
            totalNodeIds += static_cast<std::size_t>(entry.nodeCount);
        elementData.cell_node_ids.resize(totalNodeIds);

        // part 首节点全局偏移（前面 part 点数累加）
        std::size_t partNodeOffset = 0;
        for (std::size_t earlier = 0; earlier < partIndex; ++earlier)
            partNodeOffset += parts[earlier]->points->tuple_count();

        int nodeOffset = 0;
        for (int elementIndex = 0; elementIndex < elementCount; ++elementIndex)
        {
            const CellPlanEntry& entry = plan[static_cast<std::size_t>(elementIndex)];
            const vfr::CellBlock& block = part->cell_blocks[static_cast<std::size_t>(entry.blockIndex)];

            int begin = 0;
            if (entry.cellInBlock > 0 && !arrayIntValue(block.offsets, entry.cellInBlock - 1, &begin))
            {
                fillError(error, QStringLiteral("单元偏移解析失败"));
                destroyModel(modelData.data());
                return false;
            }

            VtkCellMapEntry mapped;
            vtkCellMap(entry.vtkType, &mapped);
            ++globalElementId;
            const int elementId = globalElementId;

            elementData.cell_rid_to_cid.insert({elementId, elementIndex});
            elementData.cell_rid[static_cast<std::size_t>(elementIndex)] = elementId;
            elementData.cell_types[static_cast<std::size_t>(elementIndex)] =
                static_cast<PostCelllType>(entry.vtkType);
            elementData.cell_node_counts[static_cast<std::size_t>(elementIndex)] = entry.nodeCount;
            elementData.cell_node_offsets[static_cast<std::size_t>(elementIndex)] = nodeOffset;

            EleInfo elementInfo;
            elementInfo.type = mapped.elementType;
            elementInfo.is_show = true;
            elementInfo.is_select = true;
            elementInfo.property = nullptr;
            elementData.cell_info[static_cast<std::size_t>(elementIndex)] = elementInfo;
            elementData.elementtype_elemcount[mapped.elementType] =
                elementData.elementtype_elemcount.value(mapped.elementType, 0) + 1;

            for (int local = 0; local < entry.nodeCount; ++local, ++nodeOffset)
            {
                const int sourceSlot = entry.reorder != nullptr ? entry.reorder[local] : local;
                int localNodeId = 0;
                if (!arrayIntValue(block.connectivity,
                                   static_cast<std::size_t>(begin + sourceSlot), &localNodeId))
                {
                    fillError(error, QStringLiteral("单元连接解析失败"));
                    destroyModel(modelData.data());
                    return false;
                }
                const int continuousNodeId = static_cast<int>(partNodeOffset) + localNodeId;
                if (continuousNodeId < 0 || continuousNodeId >= nodeCount)
                {
                    fillError(error, QStringLiteral("单元连接引用了不存在的节点（%1）").arg(continuousNodeId));
                    destroyModel(modelData.data());
                    return false;
                }
                elementData.cell_node_ids[static_cast<std::size_t>(nodeOffset)] = continuousNodeId;

                auto nodeElementCount = nodeData.nodeEleCountmap.find(continuousNodeId);
                if (nodeElementCount == nodeData.nodeEleCountmap.end())
                    nodeData.nodeEleCountmap.insert(continuousNodeId, 1);
                else
                    ++nodeElementCount.value();
            }
        }

        modelData->_partdata.push_back(partData);
        modelData->_partdataForFind.insert(partData->getPartID(), partData);
        ++partId;
    }

    if (modelData->_partdata.empty())
    {
        qWarning() << "VtkDataBuilder::buildGeometry: no supported elements," << skippedTotal << "skipped";
        fillError(error, QStringLiteral("没有受支持的单元（共跳过 %1 个：多段线/n 边形/三角带/高阶未列举类型）")
                             .arg(skippedTotal));
        destroyModel(modelData.data());
        return false;
    }
    if (skippedTotal > 0 && notes != nullptr)
    {
        notes->append(QStringLiteral("skipped %1 cell(s) with unsupported type/connectivity "
                                     "(polyline/n-gon/triangle-strip/unknown)")
                          .arg(skippedTotal));
    }
    for (const QString& note : skipNotes)
    {
        if (notes != nullptr)
            notes->append(note);
    }

    PostData::PostFrameInfo frameInfo;
    frameInfo._frameNum = 1;
    frameInfo.sameFrame = 0;
    frameInfo._frameName = QStringLiteral("1");
    frameInfo._frameCase = QStringLiteral("VTK");
    frameInfo._frameTime = 0.0;
    frameInfo.type = 0;
    modelData->_frameInfo = frameInfo;

    // 与 native 链路一致的注册方式：几何帧进 _modelDatabase[1] 并作为初始构型，
    // buildResultFrames 会复用该对象承载帧 1 的帧信息
    db->appendModelData(1, modelData);
    db->setSameflag(true);
    db->appendInitframe(modelData);
    db->distributeFrameIdAndResult(1, {});
    db->distributeFrameIdAndResultType(1, 0);
    return true;
}

bool VtkDataBuilder::buildResultFrames(const std::vector<vfr::DataSet>& frames,
                                       const QStringList& fileNames,
                                       QPointer<PostData::mPostDataBase> db,
                                       QString* error,
                                       QStringList* notes)
{
    if (db.isNull())
    {
        fillError(error, QStringLiteral("post database is null"));
        return false;
    }

    QPointer<mPostModelData> geometry = db->getInitframe();
    if (geometry == nullptr)
    {
        fillError(error, QStringLiteral("initial geometry frame is missing"));
        return false;
    }

    QList<PostData::PostFrameInfo*> frameInfos;
    for (std::size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex)
    {
        const vfr::DataSet& sourceFrame = frames[frameIndex];
        const int frameNo = static_cast<int>(frameIndex) + 1;

        if (frameIndex > 0 && !sameTopology(frames.front(), sourceFrame))
        {
            fillError(error, QStringLiteral("帧 %1（%2）与首帧拓扑不一致，无法组成帧序列")
                                 .arg(frameNo)
                                 .arg(fileNames.value(static_cast<int>(frameIndex))));
            return false;
        }

        QPointer<mPostModelData> modelData = frameNo == 1 ? geometry : cloneGeometry(geometry);
        if (modelData == nullptr)
        {
            fillError(error, QStringLiteral("frame %1: geometry clone failed").arg(frameNo));
            return false;
        }

        PostData::PostFrameInfo frameInfo;
        frameInfo._frameNum = frameNo;
        frameInfo.sameFrame = 1;
        frameInfo._frameName = QString::number(frameNo);
        frameInfo._frameCase = QStringLiteral("VTK");
        frameInfo._frameFile = fileNames.value(static_cast<int>(frameIndex));
        frameInfo._frameTime = static_cast<double>(frameIndex);
        frameInfo.type = 1;

        // 展平本帧 parts，注册 point_data/cell_data 变量（同帧各 part 按名合并，
        // 分量数不一致的以首个为准）
        std::vector<const vfr::DataSet*> parts;
        QStringList partNames;
        if (!collectParts(sourceFrame, QString(), &parts, &partNames, error))
            return false;

        QHash<QString, int> registeredComponentCount;
        for (const vfr::DataSet* part : parts)
        {
            for (const vfr::DataArray& array : part->point_data.arrays)
            {
                const QString name = QString::fromStdString(array.name());
                if (name.isEmpty() || registeredComponentCount.contains(name))
                    continue;
                if (array.tuple_count() != part->points->tuple_count())
                    continue;
                registeredComponentCount.insert(name, static_cast<int>(array.component_count()));

                PostData::PostVariableInfo variableInfo;
                variableInfo._resultType = 0;
                variableInfo._varName = name;
                variableInfo._varType = 0;
                variableInfo._varCount = 1;
                variableInfo._varLayer = {0};
                variableInfo._varLayerNames1 = {QStringList{}};
                variableInfo._isMag = array.component_count() == 3;
                variableInfo._compoNames = componentNames(name, static_cast<int>(array.component_count()));
                if (array.component_count() == 6 &&
                    !variableInfo._compoNames.contains(QStringLiteral("vonMises")))
                {
                    variableInfo._compoNames.append(QStringLiteral("vonMises"));
                }
                variableInfo._nCompo = variableInfo._compoNames.size();
                for (const QString& component : variableInfo._compoNames)
                    variableInfo._compoComplex.insert(component, false);
                frameInfo._frameVars.append(variableInfo);
            }
        }
        for (const vfr::DataSet* part : parts)
        {
            for (const vfr::DataArray& array : part->cell_data.arrays)
            {
                const QString name = QString::fromStdString(array.name());
                if (name.isEmpty() || registeredComponentCount.contains(name))
                    continue;
                // cell_data 元组数应对应 part 全部单元（含被跳过单元，下标即单元序号）
                std::size_t allCells = 0;
                for (const auto& block : part->cell_blocks)
                    allCells += block.offsets.tuple_count();
                if (array.tuple_count() != allCells)
                    continue;
                registeredComponentCount.insert(name, static_cast<int>(array.component_count()));

                PostData::PostVariableInfo variableInfo;
                variableInfo._resultType = 0;
                variableInfo._varName = name;
                variableInfo._varType = 1;
                variableInfo._varCount = 1;
                variableInfo._varLayer = {0};
                variableInfo._varLayerNames1 = {QStringList{}};
                variableInfo._isMag = false;
                variableInfo._compoNames = componentNames(name, static_cast<int>(array.component_count()));
                if (array.component_count() == 6 &&
                    !variableInfo._compoNames.contains(QStringLiteral("vonMises")))
                {
                    variableInfo._compoNames.append(QStringLiteral("vonMises"));
                }
                variableInfo._nCompo = variableInfo._compoNames.size();
                for (const QString& component : variableInfo._compoNames)
                    variableInfo._compoComplex.insert(component, false);
                frameInfo._frameVars.append(variableInfo);
            }
        }

        if (frameInfo._frameVars.isEmpty())
        {
            qWarning() << "VtkDataBuilder::buildResultFrames: frame" << frameNo << "has no fields";
            fillError(error, QStringLiteral("帧 %1（%2）没有可显示的节点/单元变量")
                                 .arg(frameNo)
                                 .arg(frameInfo._frameFile));
            return false;
        }

        modelData->_frameInfo = frameInfo;
        db->appendModelData(frameNo, modelData);
        db->setDisplacementDataByFrameID(frameNo, {});
        db->distributeFrameIdAndResult(frameNo, {});
        db->distributeFrameIdAndResultType(frameNo, frameInfo._frameVars.first()._varType);
        frameInfos.append(&modelData->_frameInfo);
    }

    // 发布首帧首分量（云图初显数据）：Mag > vonMises > 首分量
    const PostData::PostVariableInfo& firstVariable =
        geometry->_frameInfo._frameVars.first();
    QString firstComponent;
    if (firstVariable._isMag)
        firstComponent = QStringLiteral("Mag");
    else if (firstVariable._compoNames.contains(QStringLiteral("vonMises")))
        firstComponent = QStringLiteral("vonMises");
    else
        firstComponent = firstVariable._compoNames.first();
    if (!readVarDatas(frames, db, 1, firstVariable._varName, firstComponent))
    {
        qWarning() << "VtkDataBuilder::buildResultFrames: first result is empty";
        fillError(error, QStringLiteral("first result publish failed (variable '%1', component '%2')")
                             .arg(firstVariable._varName, firstComponent));
        return false;
    }

    db->setCaseFrameName(frameInfos);
    return true;
}

bool VtkDataBuilder::readVarDatas(const std::vector<vfr::DataSet>& frames,
                                  QPointer<PostData::mPostDataBase> db,
                                  int frameIndex,
                                  const QString& variableName,
                                  const QString& componentName)
{
    if (db.isNull())
    {
        qWarning() << "VtkDataBuilder::readVarDatas: database is null";
        return false;
    }
    QPointer<mPostModelData> modelData = db->getModelData(frameIndex);
    if (modelData == nullptr)
    {
        qWarning() << "VtkDataBuilder::readVarDatas: frame" << frameIndex << "is null";
        return false;
    }
    const int datasetFrameIndex = frameIndex - 1;
    if (datasetFrameIndex < 0 || datasetFrameIndex >= static_cast<int>(frames.size()))
    {
        qWarning() << "VtkDataBuilder::readVarDatas: frame" << frameIndex << "is out of range";
        return false;
    }

    std::vector<const vfr::DataSet*> parts;
    QStringList partNames;
    QString collectError;
    if (!collectParts(frames[static_cast<std::size_t>(datasetFrameIndex)], QString(),
                      &parts, &partNames, &collectError))
    {
        qWarning() << "VtkDataBuilder::readVarDatas:" << collectError;
        return false;
    }

    // 变量定位：point_data 优先，其次 cell_data；以首个命中 part 定义分量布局
    const vfr::DataArray* target = nullptr;
    bool isNodal = true;
    for (const vfr::DataSet* part : parts)
    {
        if (const vfr::DataArray* found = part->point_data.find(variableName.toStdString()))
        {
            if (found->tuple_count() == part->points->tuple_count())
            {
                target = found;
                isNodal = true;
                break;
            }
        }
    }
    if (target == nullptr)
    {
        for (const vfr::DataSet* part : parts)
        {
            std::size_t allCells = 0;
            for (const auto& block : part->cell_blocks)
                allCells += block.offsets.tuple_count();
            if (const vfr::DataArray* found = part->cell_data.find(variableName.toStdString()))
            {
                if (found->tuple_count() == allCells)
                {
                    target = found;
                    isNodal = false;
                    break;
                }
            }
        }
    }
    if (target == nullptr)
    {
        qWarning() << "VtkDataBuilder::readVarDatas: variable" << variableName << "not found";
        return false;
    }

    const int componentCount = static_cast<int>(target->component_count());
    const QStringList labels = componentNames(variableName, componentCount);
    const bool isDerivedVonMises = !labels.contains(componentName) &&
                                   componentName == QStringLiteral("vonMises") && componentCount == 6;
    const bool isDerivedMag = !labels.contains(componentName) &&
                              componentName == QStringLiteral("Mag") && componentCount == 3;
    int componentIndex = -1;
    for (int index = 0; index < labels.size(); ++index)
    {
        if (labels.at(index) == componentName)
        {
            componentIndex = index;
            break;
        }
    }
    if (componentIndex < 0 && !isDerivedVonMises && !isDerivedMag)
    {
        qWarning() << "VtkDataBuilder::readVarDatas: unknown component" << componentName;
        return false;
    }

    // Displacement 特例（大小写不敏感、节点位、3 分量）：同步发布变形场，
    // 供变形显示/动画使用（与 native 链路口径一致）
    const bool isDisplacement = isNodal && componentCount == 3 &&
        variableName.trimmed().compare(QStringLiteral("Displacement"), Qt::CaseInsensitive) == 0;

    // 逐 part 摊平。nodal：值按全局连续节点 id（partNodeOffset + local）发布；
    // cell：值按跨 part 全局递增的真实单元 id（与 buildGeometry 的分配一致）
    QHash<int, double> values;
    QHash<int, QVector3D> displacement;

    std::size_t partNodeOffset = 0;
    int globalElementOrdinal = 0;
    for (const vfr::DataSet* part : parts)
    {
        std::size_t partPointCount = part->points->tuple_count();
        const vfr::DataArray* array = nullptr;
        std::size_t allCells = 0;
        for (const auto& block : part->cell_blocks)
            allCells += block.offsets.tuple_count();
        if (isNodal)
        {
            array = part->point_data.find(variableName.toStdString());
            if (array == nullptr || array->tuple_count() != partPointCount ||
                array->component_count() != target->component_count())
            {
                // 该 part 缺此变量或分量数不一致：本 part 节点无值（注册口径允许）
            }
        }
        else
        {
            array = part->cell_data.find(variableName.toStdString());
            if (array == nullptr || array->tuple_count() != allCells ||
                array->component_count() != target->component_count())
            {
            }
        }

        if (array != nullptr)
        {
            if (isNodal)
            {
                for (std::size_t local = 0; local < partPointCount; ++local)
                {
                    double value = 0.0;
                    if (!resolveValue(*array, local, componentIndex,
                                      isDerivedVonMises, isDerivedMag, &value))
                        continue;
                    const int continuousId = static_cast<int>(partNodeOffset + local);
                    values.insert(continuousId, value);
                    if (isDisplacement)
                    {
                        double x = 0.0;
                        double y = 0.0;
                        double z = 0.0;
                        arrayComponent(*array, local, 0, &x);
                        arrayComponent(*array, local, 1, &y);
                        arrayComponent(*array, local, 2, &z);
                        displacement.insert(continuousId,
                                            QVector3D(static_cast<float>(x),
                                                      static_cast<float>(y),
                                                      static_cast<float>(z)));
                    }
                }
            }
            else
            {
                // part 的受支持单元计划：单元数据下标 = entry.localCell，真实单元
                // id = 跨 part 累计的受支持单元序号 + 1（与 buildGeometry 一致）
                std::vector<CellPlanEntry> plan;
                int skipped = 0;
                if (!buildCellPlan(*part, &plan, &skipped))
                    continue;
                for (std::size_t ordinal = 0; ordinal < plan.size(); ++ordinal, ++globalElementOrdinal)
                {
                    const std::size_t localCell = static_cast<std::size_t>(plan[ordinal].localCell);
                    double value = 0.0;
                    if (!resolveValue(*array, localCell, componentIndex,
                                      isDerivedVonMises, isDerivedMag, &value))
                        continue;
                    values.insert(globalElementOrdinal + 1, value);
                }
            }
        }
        partNodeOffset += partPointCount;
    }

    if (isDisplacement && !displacement.isEmpty())
    {
        modelData->DisplacementValuesR = displacement;
        modelData->DisplacementValuesV.clear();
        db->setDisplacementDataByFrameID(frameIndex, displacement);
    }

    return publishComponentValues(db, frameIndex, variableName, componentName,
                                  isNodal ? 0 : 1, values);
}

QPointer<PostData::mPostModelData> VtkDataBuilder::cloneGeometry(
    QPointer<PostData::mPostModelData> source) const
{
    if (source == nullptr)
        return nullptr;

    QPointer<mPostModelData> target = new mPostModelData();
    target->setDataFlag(source->getDataFlag());

    auto& sourceNodes = source->_nodedata;
    auto& targetNodes = target->_nodedata;
    initializeNodeMemory(targetNodes);
    targetNodes.nodeamount = sourceNodes.nodeamount;
    targetNodes.isDle = sourceNodes.isDle;
    targetNodes.nodeidmap = sourceNodes.nodeidmap;
    targetNodes.nodeEleCountmap = sourceNodes.nodeEleCountmap;
    if (sourceNodes.nodeamount > 0)
    {
        targetNodes.ids = new int[sourceNodes.nodeamount];
        std::memcpy(targetNodes.ids, sourceNodes.ids,
                    sizeof(int) * static_cast<std::size_t>(sourceNodes.nodeamount));
        if (sourceNodes.isDle)
        {
            targetNodes.nodeposD = new double[sourceNodes.nodeamount * 3];
            std::memcpy(targetNodes.nodeposD, sourceNodes.nodeposD,
                        sizeof(double) * 3 * static_cast<std::size_t>(sourceNodes.nodeamount));
        }
        else
        {
            targetNodes.nodeposF = new float[sourceNodes.nodeamount * 3];
            std::memcpy(targetNodes.nodeposF, sourceNodes.nodeposF,
                        sizeof(float) * 3 * static_cast<std::size_t>(sourceNodes.nodeamount));
        }
    }

    for (QPointer<mPostPartData> sourcePart : source->_partdata)
    {
        if (sourcePart == nullptr)
            continue;

        QPointer<mPostPartData> targetPart = new mPostPartData(sourcePart->getPartID());
        targetPart->setPartName(sourcePart->getPartName());
        targetPart->_parteledata = sourcePart->_parteledata;
        for (PostData::EleInfo& info : targetPart->_parteledata.cell_info)
            info.property = nullptr;
        target->_partdata.push_back(targetPart);
        target->_partdataForFind.insert(targetPart->getPartID(), targetPart);
    }

    return target;
}

} // namespace MIOFile
