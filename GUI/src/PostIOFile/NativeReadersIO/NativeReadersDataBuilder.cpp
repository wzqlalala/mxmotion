#include "NativeReadersDataBuilder.h"

#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mElementFunction.h"
#include "mPostEnumType.h"

#include <QDebug>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QVector3D>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>

namespace MIOFile
{
namespace
{
using PostData::mPostModelData;
using PostData::mPostPartData;
using PostData::postelementdata;
using PostData::EleInfo;
using PostData::PostCelllType;
using PostData::postElementFunction;
using MxFunctions::ElementType;

void initializeNodeMemory(PostData::postnodedata& nodeData)
{
    nodeData.ids = nullptr;
    nodeData.nodeposD = nullptr;
    nodeData.nodeposF = nullptr;
}

// 失败原因回填（首因优先）：同步保留 qWarning 便于调试日志双通道排查
void fillError(QString* error, const QString& message)
{
    if (error != nullptr && error->isEmpty())
        *error = message;
}

void destroyModel(mPostModelData* modelData)
{
    if (modelData == nullptr)
        return;

    modelData->clearModelData();
    delete modelData;
}

// 库产出的 mxdb_type 数字编码与 PostDataType::EleType 同源（CTRIA3=230/CQUAD4=240/
// D3T4=340/D3H8=380 等），直接走现成的数字重载；个别编码在该重载中无 case，
// 这里补齐后回退字符串匹配。MxNativeReaders 0.3.0 新增固定拓扑编码 110（点，
// OP2 CONM2/ODB RNODE3D）、231（六节点三角）、241（八节点四边形，此前误编 240），
// 且这些 reader 透传的求解器类型名（RNODE3D、MAPDL 单元名等）不在 ELEMENT_TABLE，
// 字符串兜底救不了，必须在此直映射。
ElementType elementTypeFromDataset(const native_readers::Element& element)
{
    if (element.mxdb_type > 0)
    {
        if (element.mxdb_type == 122)
            return ElementType::ROD;
        if (element.mxdb_type == 110)
            return ElementType::Mass1;
        if (element.mxdb_type == 231)
            return ElementType::TRIA6;
        if (element.mxdb_type == 241)
            return ElementType::SSQ8;
        const ElementType byCode = MxFunctions::mElementFunction::getElementType(
            static_cast<PostDataType::EleType>(element.mxdb_type));
        if (byCode != ElementType::NoneElement)
            return byCode;
    }

    const ElementType byName = MxFunctions::mElementFunction::getElementType(
        QString::fromStdString(element.type).trimmed().toUpper());
    return byName;
}

bool isShellElementType(ElementType type)
{
    switch (type)
    {
    case ElementType::TRIA3:
    case ElementType::Q4:
    case ElementType::TRIA6:
    case ElementType::SSQ8:
        return true;
    default:
        return false;
    }
}

// 分量标签规范化：部分格式的分量标签由库原样透传求解器命名（Abaqus ODB 的
// S11/E11 张量系与矢量数值标签 1/2/3；H3D 标签表大小写不定），派生分量
// （vonMises/Mag）的判定统一在规范名空间进行；注册进数据库的分量显示名
// 仍用原始标签，不受此处影响
QString canonicalComponentName(const QString& rawName)
{
    const QString name = rawName.trimmed().toUpper();
    static const QHash<QString, QString> tensorMap = {
        {QStringLiteral("S11"), QStringLiteral("XX")}, {QStringLiteral("E11"), QStringLiteral("XX")},
        {QStringLiteral("S22"), QStringLiteral("YY")}, {QStringLiteral("E22"), QStringLiteral("YY")},
        {QStringLiteral("S33"), QStringLiteral("ZZ")}, {QStringLiteral("E33"), QStringLiteral("ZZ")},
        {QStringLiteral("S12"), QStringLiteral("XY")}, {QStringLiteral("E12"), QStringLiteral("XY")},
        {QStringLiteral("S23"), QStringLiteral("YZ")}, {QStringLiteral("E23"), QStringLiteral("YZ")},
        {QStringLiteral("S13"), QStringLiteral("ZX")}, {QStringLiteral("E13"), QStringLiteral("ZX")},
        {QStringLiteral("S31"), QStringLiteral("ZX")}, {QStringLiteral("E31"), QStringLiteral("ZX")},
        {QStringLiteral("1"), QStringLiteral("X")},
        {QStringLiteral("2"), QStringLiteral("Y")},
        {QStringLiteral("3"), QStringLiteral("Z")},
    };
    const auto mapped = tensorMap.constFind(name);
    if (mapped != tensorMap.constEnd())
        return mapped.value();
    return name;
}

// 标准六分量应力/应变张量的规范分量名集合（各 reader 统一 XX..ZX；Abaqus ODB 的
// S11/E11 系经 canonicalComponentName 归一），顺序不限，用于派生 vonMises
bool isStandardStressComponents(const QStringList& components)
{
    if (components.size() != 6)
        return false;
    static const QSet<QString> expected = { QStringLiteral("XX"), QStringLiteral("YY"),
                                            QStringLiteral("ZZ"), QStringLiteral("XY"),
                                            QStringLiteral("YZ"), QStringLiteral("ZX") };
    QSet<QString> names;
    for (const QString& component : components)
        names.insert(canonicalComponentName(component));
    return names == expected;
}

// 是否启用矢量模显示（规范名 X/Y/Z 齐全即可；Abaqus ODB 的数值标签 1/2/3 亦归入），
// 逻辑同 MxdbDataBuilder::inferMagForVariable
bool inferMagForComponents(const std::vector<std::string>& components)
{
    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    for (const auto& component : components)
    {
        const QString name = canonicalComponentName(QString::fromStdString(component));
        if (name == QStringLiteral("X"))
            hasX = true;
        else if (name == QStringLiteral("Y"))
            hasY = true;
        else if (name == QStringLiteral("Z"))
            hasZ = true;
    }
    return hasX && hasY && hasZ;
}

bool containsComponentNamed(const std::vector<std::string>& components, const QString& expected)
{
    for (const auto& component : components)
    {
        if (QString::fromStdString(component).trimmed().compare(expected, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// GUI 结果模型只承载节点/单元位场（varType 0/1，值按节点或单元 ID 发布）。
// MxNativeReaders 0.3.0 起 d3plot 还产出 global（整体标量，ids={1}）与
// part（ids=源 part ID）位场：按单元场误发布会把标量挂到同号单元上，
// 首帧首显若选中更会因取不到值导致整体导入失败，统一不注册、计入降级提示。
// element_nodal 位场（0.3.12 起 OP2/RST/H3D/ODB 实际产出）按单元位注册，
// 读取侧以节点等权平均归一到单元值（见 readVarDatasFrame）。
bool isDisplayableLocation(const std::string& location)
{
    return location == "nodal" || location == "element" || location == "element_nodal";
}

// ---------------------------------------------------------------------------
// 角点→节点平均伪变量（"<base> (nodal averaged)"）
// 源场两种形态（库 0.3.0 起提供，均为 element 位 + 纯角点层）：
//  - OP2 "<Stress/Strain> / <CTETRA|CHEXA|CPENTA> Corner Samples (source record)"：
//    components[0]=="GRID"，每个 (单元, Corner_N) 记录显式携带角点节点号，
//    分量 1..6 为 XX..ZX 张量——节点对应关系不依赖连接表推断
//  - RST "<name> at corners (ANSYS <N>)"：components 恰为六张量分量，角点层
//    序号 k 对应单元连接第 k 槽位（退化单元按 source_node_slots 原槽位，
//    零槽跳过）
// ---------------------------------------------------------------------------
const QString kNodalAverageSuffix = QStringLiteral(" (nodal averaged)");

// 伪变量固定注册的规范六张量分量名（与两种源场的张量分量顺序一致）
const char* const kTensorComponentNames[6] = {"XX", "YY", "ZZ", "XY", "YZ", "ZX"};

bool isCornerLayerLabel(const QString& label)
{
    static const QRegularExpression pattern(QStringLiteral("^Corner[ _]?\\d+$"));
    return pattern.match(label).hasMatch();
}

bool hasOnlyCornerLayers(const native_readers::Field& field)
{
    if (field.location != "element" || field.layers.empty())
        return false;
    for (const auto& layer : field.layers)
    {
        if (!isCornerLayerLabel(QString::fromStdString(layer)))
            return false;
    }
    return true;
}

// 返回伪变量的 base 名（即源场名剥去角点后缀）；非角点源场返回空串
QString nodalAverageBaseName(const native_readers::Field& field)
{
    if (!hasOnlyCornerLayers(field))
        return QString();
    const QString name = QString::fromStdString(field.name);

    // OP2 形态：显式 GRID 前导
    if (name.endsWith(QStringLiteral("Corner Samples (source record)")) &&
        field.components.size() >= 7 &&
        QString::fromStdString(field.components[0]).compare(
            QStringLiteral("GRID"), Qt::CaseInsensitive) == 0)
    {
        const int cut = name.lastIndexOf(QStringLiteral(" / "));
        return cut > 0 ? name.left(cut) : QString();
    }

    // RST 形态：六张量分量 + " at corners (ANSYS N)" 后缀
    if (field.components.size() == 6)
    {
        QStringList comps;
        for (const auto& component : field.components)
            comps.append(QString::fromStdString(component));
        if (isStandardStressComponents(comps))
        {
            static const QRegularExpression ansysCorner(
                QStringLiteral(" at corners \\(ANSYS \\d+\\)$"));
            const auto match = ansysCorner.match(name);
            if (match.hasMatch() && match.capturedStart() > 0)
                return name.left(match.capturedStart());
        }
    }
    return QString();
}

// 单场变量注册（含层注册、vonMises 追加、角点平均伪变量）——全量路径逐帧逐场
// 与流式路径（目录项转临时 Field）共用；field 仅读 name/location/components/
// layers，不触碰 ids/values
void appendFieldVariable(const native_readers::Field& field,
                         PostData::PostFrameInfo& frameInfo,
                         QSet<QString>& registeredPseudoNames,
                         int& skippedLocationFields,
                         QString& skippedLocationExample)
{
    if (!isDisplayableLocation(field.location))
    {
        ++skippedLocationFields;
        if (skippedLocationExample.isEmpty())
        {
            skippedLocationExample = QStringLiteral("%1 (%2)")
                                         .arg(QString::fromStdString(field.name))
                                         .arg(QString::fromStdString(field.location));
        }
        return;
    }
    PostData::PostVariableInfo variableInfo;
    variableInfo._resultType = 0;
    variableInfo._varName = QString::fromStdString(field.name);
    variableInfo._varType = field.location == "nodal" ? 0 : 1;
    variableInfo._varCount = 1;
    // 层注册（复合材料铺层/壳 Z1·Z2/角点/ODB facet），层名直接用库标签：
    // 云图/曲线面板的层下拉框经 getVariableLayer 读 _varLayerNames(_1) 自动
    // 出现；读取侧选中层取切片、未选层保持跨层平均（与旧 OP2 链路对壳
    // z1/z2 取平均的显示口径一致）
    if (field.layers.empty())
    {
        variableInfo._varLayer = {0};
        variableInfo._varLayerNames1 = {QStringList{}};
    }
    else
    {
        QStringList layerNames;
        layerNames.reserve(static_cast<int>(field.layers.size()));
        for (const auto& layer : field.layers)
            layerNames.append(QString::fromStdString(layer));
        variableInfo._varLayer = {layerNames.size()};
        variableInfo._varLayerNames1 = {layerNames};
        variableInfo._varLayerNames = layerNames;
    }
    variableInfo._isMag = inferMagForComponents(field.components);

    variableInfo._compoNames.reserve(static_cast<int>(field.components.size()) + 1);
    for (const auto& component : field.components)
        variableInfo._compoNames.append(QString::fromStdString(component));
    // 六分量应力场补派生 vonMises 分量（读取期按实体/壳公式现算）
    if (isStandardStressComponents(variableInfo._compoNames) &&
        !variableInfo._compoNames.contains(QStringLiteral("vonMises")))
    {
        variableInfo._compoNames.append(QStringLiteral("vonMises"));
    }
    variableInfo._nCompo = variableInfo._compoNames.size();
    for (const QString& component : variableInfo._compoNames)
        variableInfo._compoComplex.insert(component, false);
    frameInfo._frameVars.append(variableInfo);

    // 角点源场额外注册节点平均伪变量（同 base 多个源场——如 OP2 混合
    // CTETRA/CHEXA——只注册一次，读取期聚合全部源场）
    const QString nodalBase = nodalAverageBaseName(field);
    if (!nodalBase.isEmpty())
    {
        const QString pseudoName = nodalBase + kNodalAverageSuffix;
        if (!registeredPseudoNames.contains(pseudoName))
        {
            registeredPseudoNames.insert(pseudoName);
            PostData::PostVariableInfo nodalInfo;
            nodalInfo._resultType = 0;
            nodalInfo._varName = pseudoName;
            nodalInfo._varType = 0; // 节点位
            nodalInfo._varCount = 1;
            nodalInfo._varLayer = {0};
            nodalInfo._varLayerNames1 = {QStringList{}};
            nodalInfo._isMag = false;
            for (const char* component : kTensorComponentNames)
                nodalInfo._compoNames.append(QString::fromLatin1(component));
            nodalInfo._compoNames.append(QStringLiteral("vonMises"));
            nodalInfo._nCompo = nodalInfo._compoNames.size();
            for (const QString& component : nodalInfo._compoNames)
                nodalInfo._compoComplex.insert(component, false);
            frameInfo._frameVars.append(nodalInfo);
        }
    }
}

// 云图初显分量选择（Mag > vonMises > 首分量）：field 仅读 name/location/components
bool selectFirstDisplayableFromFields(const std::vector<native_readers::Field>& fields,
                                      QString* variableName,
                                      QString* componentName)
{
    for (const auto& field : fields)
    {
        if (!isDisplayableLocation(field.location))
            continue;
        QString firstName = QString::fromStdString(field.name);
        QStringList components;
        components.reserve(static_cast<int>(field.components.size()));
        for (const auto& component : field.components)
            components.append(QString::fromStdString(component));
        QString firstComponent;
        if (inferMagForComponents(field.components))
            firstComponent = QStringLiteral("Mag");
        else if (isStandardStressComponents(components) ||
                 containsComponentNamed(field.components, QStringLiteral("vonMises")))
            firstComponent = QStringLiteral("vonMises");
        else if (!components.isEmpty())
            firstComponent = components.first();
        if (!firstComponent.isEmpty())
        {
            if (variableName != nullptr)
                *variableName = firstName;
            if (componentName != nullptr)
                *componentName = firstComponent;
            return true;
        }
    }
    return false;
}

// 与 Op2PostDataBuilder::publishComponentValues 相同的发布协议
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
} // namespace

bool NativeReadersDataBuilder::buildAll(const native_readers::Dataset& dataset,
                                        QPointer<PostData::mPostDataBase> db,
                                        QString* error,
                                        QStringList* notes)
{
    if (!buildGeometry(dataset, db, error, notes))
        return false;
    return buildResultFrames(dataset, db, error, notes);
}

bool NativeReadersDataBuilder::buildGeometry(const native_readers::Dataset& dataset,
                                             QPointer<PostData::mPostDataBase> db,
                                             QString* error,
                                             QStringList* notes)
{
    if (db.isNull())
    {
        qWarning() << "NativeReadersDataBuilder::buildGeometry: database is null";
        fillError(error, QStringLiteral("post database is null"));
        return false;
    }

    if (dataset.nodes.empty())
    {
        qWarning() << "NativeReadersDataBuilder::buildGeometry: no nodes";
        fillError(error, QStringLiteral("result file contains no nodes"));
        return false;
    }

    if (dataset.elements.empty())
    {
        qWarning() << "NativeReadersDataBuilder::buildGeometry: no elements";
        fillError(error, QStringLiteral("result file contains no elements"));
        return false;
    }

    QPointer<mPostModelData> modelData = new mPostModelData();
    modelData->setDataFlag(true);
    initializeNodeMemory(modelData->_nodedata);

    auto& nodeData = modelData->_nodedata;
    const int nodeCount = static_cast<int>(dataset.nodes.size());
    if (dataset.nodes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        qWarning() << "NativeReadersDataBuilder::buildGeometry: too many nodes";
        fillError(error, QStringLiteral("node count %1 exceeds the limit (%2)")
                             .arg(static_cast<qlonglong>(dataset.nodes.size()))
                             .arg(std::numeric_limits<int>::max()));
        destroyModel(modelData.data());
        return false;
    }
    nodeData.nodeamount = nodeCount;
    nodeData.isDle = false;
    nodeData.ids = new int[nodeCount];
    nodeData.nodeposF = new float[nodeCount * 3];
    nodeData.nodeidmap.reserve(nodeCount);

    for (int index = 0; index < nodeCount; ++index)
    {
        const auto& node = dataset.nodes[static_cast<std::size_t>(index)];
        if (node.id > static_cast<native_readers::Id>(std::numeric_limits<int>::max()))
        {
            qWarning() << "NativeReadersDataBuilder::buildGeometry: node id out of range" << node.id;
            fillError(error, QStringLiteral("node id %1 is out of the supported range")
                                 .arg(static_cast<qlonglong>(node.id)));
            destroyModel(modelData.data());
            return false;
        }
        nodeData.ids[index] = static_cast<int>(node.id);
        nodeData.nodeidmap.insert(static_cast<int>(node.id), index);
        nodeData.nodeposF[index * 3] = static_cast<float>(node.xyz[0]);
        nodeData.nodeposF[index * 3 + 1] = static_cast<float>(node.xyz[1]);
        nodeData.nodeposF[index * 3 + 2] = static_cast<float>(node.xyz[2]);
    }

    // 部件按 part_id 分组；OP2 的 part_id 即 Nastran Property 号（命名见下方
    // partNames 实名优先、猜测命名回退）
    const bool isOp2 = QString::fromStdString(dataset.source_format).startsWith(QStringLiteral("OP2"));
    std::map<native_readers::Id, std::vector<const native_readers::Element*>> elementGroups;
    // 跳过原因分类计数（导入失败时回填错误详情，帮助定位模型/结果与库支持面的差距）
    int skippedUnsupportedType = 0;
    int skippedNodeCountMismatch = 0;
    int skippedUnknownNodeId = 0;
    QSet<QString> unsupportedTypeLabels;
    for (const auto& element : dataset.elements)
    {
        const ElementType elementType = elementTypeFromDataset(element);
        bool usable = elementType != ElementType::NoneElement;
        if (usable)
        {
            const PostCelllType cellType = postElementFunction::MapFunctionTypeToPostCellType(elementType);
            const int expectedNodeCount = postElementFunction::getNodeCountByPostCellType(cellType);
            if (static_cast<int>(element.nodes.size()) != expectedNodeCount)
            {
                usable = false;
                ++skippedNodeCountMismatch;
            }
        }
        if (usable)
        {
            for (native_readers::Id nodeId : element.nodes)
            {
                if (!nodeData.nodeidmap.contains(static_cast<int>(nodeId)))
                {
                    usable = false;
                    ++skippedUnknownNodeId;
                    break;
                }
            }
        }
        if (!usable)
        {
            if (elementType == ElementType::NoneElement)
            {
                ++skippedUnsupportedType;
                QString label = QString::fromStdString(element.type).trimmed();
                if (label.isEmpty())
                    label = QStringLiteral("mxdb_type %1").arg(element.mxdb_type);
                unsupportedTypeLabels.insert(label);
            }
            continue;
        }
        elementGroups[element.part_id].push_back(&element);
    }

    if (elementGroups.empty())
    {
        QStringList sortedLabels = unsupportedTypeLabels.values();
        sortedLabels.sort();
        qWarning() << "NativeReadersDataBuilder::buildGeometry: no supported elements (skipped"
                   << skippedUnsupportedType << skippedNodeCountMismatch << skippedUnknownNodeId << ")";
        fillError(error,
                  QStringLiteral("no usable elements after filtering: unsupported element type %1 [%2], "
                                 "node-count mismatch %3, unknown node id %4")
                      .arg(skippedUnsupportedType)
                      .arg(sortedLabels.join(QStringLiteral(", ")))
                      .arg(skippedNodeCountMismatch)
                      .arg(skippedUnknownNodeId));
        destroyModel(modelData.data());
        return false;
    }
    const int skippedElements = skippedUnsupportedType + skippedNodeCountMismatch + skippedUnknownNodeId;
    if (skippedElements > 0)
    {
        qWarning() << "NativeReadersDataBuilder::buildGeometry: skipped" << skippedElements
                   << "elements with unsupported type/connectivity";
        // 部分降级：导入仍成功，但跳过统计要透出到输出窗口（用户可见模型缺单元的原因）
        if (notes != nullptr)
        {
            QStringList sortedLabels = unsupportedTypeLabels.values();
            sortedLabels.sort();
            notes->append(QStringLiteral("skipped %1 of %2 elements: unsupported type %3 [%4], "
                                         "node-count mismatch %5, unknown node id %6")
                              .arg(skippedElements)
                              .arg(static_cast<qlonglong>(dataset.elements.size()))
                              .arg(skippedUnsupportedType)
                              .arg(sortedLabels.join(QStringLiteral(", ")))
                              .arg(skippedNodeCountMismatch)
                              .arg(skippedUnknownNodeId));
        }
    }

    // 0.3.3 起库提供求解器部件实名（修复 d3plot 部件编号-名称关联，Part.name
    // 为 UTF-8）；空名显式记录源标题为空、不采用，未命中时回退旧链路猜测命名
    QHash<native_readers::Id, QString> partNames;
    for (const auto& part : dataset.parts)
    {
        const QString name = QString::fromUtf8(part.name.c_str()).trimmed();
        if (!name.isEmpty())
            partNames.insert(part.id, name);
    }

    int partId = 0;
    QHash<QString, QVector<int>> sectionParts;
    for (const auto& group : elementGroups)
    {
        QPointer<mPostPartData> partData = new mPostPartData(partId);
        const QString fallbackName = isOp2
                                         ? (group.first == 0
                                                ? QStringLiteral("Property_Unknown")
                                                : QStringLiteral("Property_%1").arg(group.first))
                                         : QStringLiteral("Part_%1").arg(group.first);
        const QString partName = partNames.value(group.first, fallbackName);
        partData->setPartName(partName);
        if (isOp2)
            sectionParts[partName].push_back(partId);

        const int elementCount = static_cast<int>(group.second.size());
        postelementdata& elementData = partData->_parteledata;
        elementData.cell_types.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_rid.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_node_counts.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_node_offsets.resize(static_cast<std::size_t>(elementCount));
        elementData.cell_info.resize(static_cast<std::size_t>(elementCount));

        std::size_t totalNodeIds = 0;
        for (const auto* element : group.second)
            totalNodeIds += element->nodes.size();
        elementData.cell_node_ids.resize(totalNodeIds);

        int nodeOffset = 0;
        for (int elementIndex = 0; elementIndex < elementCount; ++elementIndex)
        {
            const auto& element = *group.second[static_cast<std::size_t>(elementIndex)];
            const ElementType elementType = elementTypeFromDataset(element);
            const PostCelllType cellType = postElementFunction::MapFunctionTypeToPostCellType(elementType);

            elementData.cell_rid_to_cid.insert({static_cast<int>(element.id), elementIndex});
            elementData.cell_rid[static_cast<std::size_t>(elementIndex)] = static_cast<int>(element.id);
            elementData.cell_types[static_cast<std::size_t>(elementIndex)] = cellType;
            elementData.cell_node_counts[static_cast<std::size_t>(elementIndex)] =
                static_cast<int>(element.nodes.size());
            elementData.cell_node_offsets[static_cast<std::size_t>(elementIndex)] = nodeOffset;

            EleInfo elementInfo;
            elementInfo.type = elementType;
            elementInfo.is_show = true;
            elementInfo.is_select = true;
            elementInfo.property = nullptr;
            elementData.cell_info[static_cast<std::size_t>(elementIndex)] = elementInfo;
            elementData.elementtype_elemcount[elementType] =
                elementData.elementtype_elemcount.value(elementType, 0) + 1;

            for (native_readers::Id nodeId : element.nodes)
            {
                const int continuousNodeId = nodeData.nodeidmap.value(static_cast<int>(nodeId), -1);
                elementData.cell_node_ids[static_cast<std::size_t>(nodeOffset)] = continuousNodeId;
                ++nodeOffset;

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

    PostData::PostFrameInfo frameInfo;
    frameInfo._frameNum = 1;
    frameInfo.sameFrame = 0;
    frameInfo._frameName = QStringLiteral("1");
    frameInfo._frameCase = QString::fromStdString(dataset.source_format);
    frameInfo._frameTime = 0.0;
    frameInfo.type = 0;
    modelData->_frameInfo = frameInfo;

    // 与旧 OP2 链路一致的注册方式：几何帧进 _modelDatabase[1] 并作为初始构型，
    // buildResultFrames 会复用该对象承载首个结果帧的帧信息。
    db->appendModelData(1, modelData);
    db->setSameflag(true);
    db->appendInitframe(modelData);
    db->distributeFrameIdAndResult(1, {});
    db->distributeFrameIdAndResultType(1, 0);

    if (isOp2)
        db->setSectionParts(sectionParts);
    return true;
}

bool NativeReadersDataBuilder::buildResultFrames(const native_readers::Dataset& dataset,
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
        qWarning() << "NativeReadersDataBuilder::buildResultFrames: geometry is null";
        fillError(error, QStringLiteral("initial geometry frame is missing"));
        return false;
    }

    QString frameFile;
    if (!dataset.source_files.empty())
        frameFile = QString::fromStdString(dataset.source_files.front().u8string());

    QList<PostData::PostFrameInfo*> frameInfos;
    int skippedLocationFields = 0;
    QString skippedLocationExample;
    for (std::size_t frameIndex = 0; frameIndex < dataset.frames.size(); ++frameIndex)
    {
        const auto& sourceFrame = dataset.frames[frameIndex];
        const int frameNo = static_cast<int>(frameIndex) + 1;
        QPointer<mPostModelData> modelData = frameNo == 1 ? geometry : cloneGeometry(geometry);
        if (modelData == nullptr)
        {
            qWarning() << "NativeReadersDataBuilder::buildResultFrames: clone geometry failed";
            fillError(error, QStringLiteral("frame %1: geometry clone failed").arg(frameNo));
            return false;
        }

        PostData::PostFrameInfo frameInfo;
        frameInfo._frameNum = frameNo;
        frameInfo.sameFrame = 1;
        frameInfo._frameName = QString::number(frameNo);
        frameInfo._frameCase = sourceFrame.subcase > 0
                                   ? QStringLiteral("Subcase %1").arg(sourceFrame.subcase)
                                   : QString::fromStdString(dataset.name);
        frameInfo._frameFile = frameFile;
        frameInfo._frameTime = sourceFrame.time;
        frameInfo.type = 1;

        // 本帧已注册的伪变量名（角点→节点平均），同 base 多源场只注册一次
        QSet<QString> registeredPseudoNames;

        for (const auto& field : sourceFrame.fields)
        {
            appendFieldVariable(field, frameInfo, registeredPseudoNames,
                                skippedLocationFields, skippedLocationExample);
        }

        if (frameInfo._frameVars.isEmpty())
        {
            qWarning() << "NativeReadersDataBuilder::buildResultFrames: frame" << frameNo
                       << "has no fields";
            fillError(error, sourceFrame.fields.empty()
                                 ? QStringLiteral("frame %1 (%2) contains no result fields")
                                       .arg(frameNo)
                                       .arg(frameInfo._frameCase)
                                 : QStringLiteral("frame %1 (%2) has no displayable result "
                                                  "fields (non-mesh locations were skipped)")
                                       .arg(frameNo)
                                       .arg(frameInfo._frameCase));
            return false;
        }

        modelData->_frameInfo = frameInfo;
        db->appendModelData(frameNo, modelData);
        db->setDisplacementDataByFrameID(frameNo, {});
        db->distributeFrameIdAndResult(frameNo, {});
        db->distributeFrameIdAndResultType(frameNo, frameInfo._frameVars.first()._varType);
        frameInfos.append(&modelData->_frameInfo);
    }

    if (frameInfos.isEmpty())
    {
        qWarning() << "NativeReadersDataBuilder::buildResultFrames: no result frames";
        fillError(error, QStringLiteral("dataset contains no result frames"));
        return false;
    }

    // global/part 位场跳过属于部分降级，与几何跳单元同通道透出（用户可见
    // "变量比 HyperView 少"的原因）
    if (skippedLocationFields > 0 && notes != nullptr)
    {
        notes->append(QStringLiteral("skipped %1 non-mesh result field(s) with no GUI "
                                     "display location, e.g. %2")
                          .arg(skippedLocationFields)
                          .arg(skippedLocationExample));
    }

    // 发布首帧首分量（云图初显数据）：与 GUI 分量选择优先级一致（Mag > vonMises > 首分量）
    const auto& firstFrame = dataset.frames.front();
    QString firstName;
    QString firstComponent;
    if (!selectFirstDisplayableFromFields(firstFrame.fields, &firstName, &firstComponent))
        firstName.clear();
    if (!firstName.isEmpty() && !firstComponent.isEmpty())
    {
        if (!readVarDatas(dataset, db, 1, firstName, firstComponent))
        {
            qWarning() << "NativeReadersDataBuilder::buildResultFrames: first result is empty";
            fillError(error, QStringLiteral("first result publish failed (variable '%1', component '%2')")
                                 .arg(firstName, firstComponent));
            return false;
        }
    }

    db->setCaseFrameName(frameInfos);
    return true;
}

bool NativeReadersDataBuilder::buildAllFromStream(
    const native_readers::Dataset& model,
    const std::vector<native_readers::D3plotFrameInfo>& frameDirectory,
    const std::vector<native_readers::D3plotFieldInfo>& fieldCatalog,
    QPointer<PostData::mPostDataBase> db,
    QString* error,
    QStringList* notes)
{
    if (!buildGeometry(model, db, error, notes))
        return false;
    return buildResultFramesFromStream(model, frameDirectory, fieldCatalog, db, error, notes);
}

bool NativeReadersDataBuilder::buildResultFramesFromStream(
    const native_readers::Dataset& model,
    const std::vector<native_readers::D3plotFrameInfo>& frameDirectory,
    const std::vector<native_readers::D3plotFieldInfo>& fieldCatalog,
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
        qWarning() << "NativeReadersDataBuilder::buildResultFramesFromStream: geometry is null";
        fillError(error, QStringLiteral("initial geometry frame is missing"));
        return false;
    }

    // 场目录转临时 Field 复用全量路径的注册入口（appendFieldVariable 只读
    // name/location/components/layers，不触碰 ids/values）
    std::vector<native_readers::Field> catalogFields;
    catalogFields.reserve(fieldCatalog.size());
    for (const auto& info : fieldCatalog)
    {
        native_readers::Field field;
        field.name = info.name;
        field.location = info.location;
        field.components = info.components;
        field.layers = info.layers;
        catalogFields.push_back(std::move(field));
    }

    QList<PostData::PostFrameInfo*> frameInfos;
    int skippedLocationFields = 0;
    QString skippedLocationExample;
    for (std::size_t frameIndex = 0; frameIndex < frameDirectory.size(); ++frameIndex)
    {
        const auto& directoryFrame = frameDirectory[frameIndex];
        const int frameNo = static_cast<int>(frameIndex) + 1;
        QPointer<mPostModelData> modelData = frameNo == 1 ? geometry : cloneGeometry(geometry);
        if (modelData == nullptr)
        {
            qWarning() << "NativeReadersDataBuilder::buildResultFramesFromStream: "
                          "clone geometry failed";
            fillError(error, QStringLiteral("frame %1: geometry clone failed").arg(frameNo));
            return false;
        }

        PostData::PostFrameInfo frameInfo;
        frameInfo._frameNum = frameNo;
        frameInfo.sameFrame = 1;
        frameInfo._frameName = QString::number(frameNo);
        // 目录 subcase 与全量帧口径一致（d3plot 读者两者都默认 1）
        frameInfo._frameCase = directoryFrame.subcase > 0
                                   ? QStringLiteral("Subcase %1").arg(directoryFrame.subcase)
                                   : QString::fromStdString(model.name);
        QString frameFile;
        if (directoryFrame.source_file_index < model.source_files.size())
        {
            frameFile = QString::fromStdString(
                model.source_files[directoryFrame.source_file_index].u8string());
        }
        frameInfo._frameFile = frameFile;
        frameInfo._frameTime = directoryFrame.time;
        frameInfo.type = 1;

        // d3plot 各保存状态共享同一布局：每帧注册同一份场目录
        QSet<QString> registeredPseudoNames;
        for (const auto& field : catalogFields)
        {
            appendFieldVariable(field, frameInfo, registeredPseudoNames,
                                skippedLocationFields, skippedLocationExample);
        }

        if (frameInfo._frameVars.isEmpty())
        {
            qWarning() << "NativeReadersDataBuilder::buildResultFramesFromStream: frame"
                       << frameNo << "has no fields";
            fillError(error,
                      fieldCatalog.empty()
                          ? QStringLiteral("frame %1 (%2) contains no result fields")
                                .arg(frameNo)
                                .arg(frameInfo._frameCase)
                          : QStringLiteral("frame %1 (%2) has no displayable result "
                                           "fields (non-mesh locations were skipped)")
                                .arg(frameNo)
                                .arg(frameInfo._frameCase));
            return false;
        }

        modelData->_frameInfo = frameInfo;
        db->appendModelData(frameNo, modelData);
        db->setDisplacementDataByFrameID(frameNo, {});
        db->distributeFrameIdAndResult(frameNo, {});
        db->distributeFrameIdAndResultType(frameNo, frameInfo._frameVars.first()._varType);
        frameInfos.append(&modelData->_frameInfo);
    }

    if (frameInfos.isEmpty())
    {
        qWarning() << "NativeReadersDataBuilder::buildResultFramesFromStream: no result frames";
        fillError(error, QStringLiteral("d3plot stream directory contains no saved states"));
        return false;
    }

    if (skippedLocationFields > 0 && notes != nullptr)
    {
        notes->append(QStringLiteral("skipped %1 non-mesh result field(s) with no GUI "
                                     "display location, e.g. %2")
                          .arg(skippedLocationFields)
                          .arg(skippedLocationExample));
    }

    db->setCaseFrameName(frameInfos);
    return true;
}

bool NativeReadersDataBuilder::selectFirstDisplayableVariable(
    const std::vector<native_readers::D3plotFieldInfo>& catalog,
    QString* variableName,
    QString* componentName)
{
    std::vector<native_readers::Field> catalogFields;
    catalogFields.reserve(catalog.size());
    for (const auto& info : catalog)
    {
        native_readers::Field field;
        field.name = info.name;
        field.location = info.location;
        field.components = info.components;
        field.layers = info.layers;
        catalogFields.push_back(std::move(field));
    }
    return selectFirstDisplayableFromFields(catalogFields, variableName, componentName);
}

bool NativeReadersDataBuilder::readVarDatas(const native_readers::Dataset& dataset,
                                            QPointer<PostData::mPostDataBase> db,
                                            int frameIndex,
                                            const QString& variableName,
                                            const QString& componentName,
                                            const QString& layerName)
{
    const int datasetFrameIndex = frameIndex - 1;
    if (datasetFrameIndex < 0 || datasetFrameIndex >= static_cast<int>(dataset.frames.size()))
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: frame" << frameIndex
                   << "is out of range";
        return false;
    }

    const auto& frame = dataset.frames[static_cast<std::size_t>(datasetFrameIndex)];
    return readVarDatasFrame(dataset, frame, db, frameIndex, variableName, componentName,
                             layerName);
}

bool NativeReadersDataBuilder::readVarDatasFrame(const native_readers::Dataset& model,
                                                 const native_readers::Frame& frame,
                                                 QPointer<PostData::mPostDataBase> db,
                                                 int frameIndex,
                                                 const QString& variableName,
                                                 const QString& componentName,
                                                 const QString& layerName)
{
    if (db.isNull())
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: database is null";
        return false;
    }

    QPointer<mPostModelData> modelData = db->getModelData(frameIndex);
    if (modelData == nullptr)
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: frame" << frameIndex << "is null";
        return false;
    }

    const native_readers::Field* target = nullptr;
    for (const auto& field : frame.fields)
    {
        if (QString::fromStdString(field.name) == variableName)
        {
            target = &field;
            break;
        }
    }
    if (target == nullptr)
    {
        // 角点→节点平均伪变量：不是真实场名，转专用读取路径（聚合同 base 源场）
        if (variableName.endsWith(kNodalAverageSuffix))
        {
            const QString baseName = variableName.left(
                variableName.size() - kNodalAverageSuffix.size());
            return readNodalAverageDataFrame(model, frame, db, frameIndex, variableName,
                                             baseName, componentName);
        }
        qWarning() << "NativeReadersDataBuilder::readVarDatas: frame" << frameIndex
                   << "does not contain variable" << variableName;
        return false;
    }

    // global/part 位场没有可发布的实体值域（ids 为整体号/源 part 号），
    // 注册侧已跳过；此处拦截直连调用方按旧变量名请求的残留路径
    if (!isDisplayableLocation(target->location))
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: variable" << variableName
                   << "has non-mesh location" << QString::fromStdString(target->location);
        return false;
    }

    const bool isNodal = target->location == "nodal";
    const int varType = isNodal ? 0 : 1;
    const int componentCount = static_cast<int>(target->components.size());
    // H3D 的场名由库原样透传文件标签表（大小写不定），对齐旧 Altair 链路按大小写
    // 不敏感识别 Displacement；OP2/RST/d3plot/ODB 库内已统一为该名
    const bool isDisplacement = isNodal && componentCount >= 3 &&
        QString::fromStdString(target->name).trimmed().compare(
            QStringLiteral("Displacement"), Qt::CaseInsensitive) == 0;
    // 0.3.12 起 element_nodal 场携带显式单元-节点映射：ids=单元号（每单元一行）、
    // element_node_counts=每单元节点数、element_node_ids=顺排节点号，values 行主序
    // entity → node → layer → component（width() 为每节点宽度，节点数可变，不能再
    // 用 ids.size()*width() 矩形公式）；两数组为空时保持旧均匀行表示
    // （entity → layer → component）
    const bool pairedElementNodal = target->location == "element_nodal" &&
                                    !target->element_node_counts.empty();
    std::size_t mappedNodeTotal = 0;
    bool layoutConsistent = !pairedElementNodal ||
                            target->element_node_counts.size() == target->ids.size();
    if (pairedElementNodal && layoutConsistent)
    {
        for (std::size_t row = 0; row < target->ids.size(); ++row)
        {
            const std::size_t count = target->element_node_counts[row];
            if (count == 0 || mappedNodeTotal > target->element_node_ids.size() ||
                count > target->element_node_ids.size() - mappedNodeTotal)
            {
                layoutConsistent = false;
                break;
            }
            mappedNodeTotal += count;
        }
        layoutConsistent = layoutConsistent &&
                           mappedNodeTotal == target->element_node_ids.size() &&
                           target->values.size() == mappedNodeTotal * target->width();
    }
    else if (!pairedElementNodal)
    {
        layoutConsistent = target->values.size() == target->ids.size() * target->width();
    }
    if (componentCount == 0 || target->ids.empty() || !layoutConsistent)
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: field" << variableName
                   << "has inconsistent data";
        return false;
    }

    // 分量定位：常规分量取索引（用户从注册的原始标签中选，精确匹配）；
    // vonMises 与 Mag 为派生分量（读取期按规范名计算）
    int componentIndex = -1;
    for (int index = 0; index < componentCount; ++index)
    {
        if (QString::fromStdString(target->components[static_cast<std::size_t>(index)]) == componentName)
        {
            componentIndex = index;
            break;
        }
    }
    QStringList canonicalComponents;
    canonicalComponents.reserve(componentCount);
    for (int index = 0; index < componentCount; ++index)
        canonicalComponents.append(canonicalComponentName(
            QString::fromStdString(target->components[static_cast<std::size_t>(index)])));

    const bool isDerivedVonMises =
        componentIndex < 0 && componentName == QStringLiteral("vonMises");
    // Mag 为 GUI 对 _isMag 矢量变量（规范名 X/Y/Z 齐全）自动追加的矢量模分量，
    // 语义与 mxdb 链路一致（sqrt 三分量平方和）；不识别会让 slot_showCloud 早退，
    // 表现为切变量后无图例无颜色变化。判定与注册侧 inferMagForComponents 同口径，
    // 不限定分量总数（防 X/Y/Z+附加分量的场注册侧追加 Mag 而此处拒识）
    const bool isDerivedMag =
        componentIndex < 0 && componentName == QStringLiteral("Mag") &&
        canonicalComponents.contains(QStringLiteral("X")) &&
        canonicalComponents.contains(QStringLiteral("Y")) &&
        canonicalComponents.contains(QStringLiteral("Z"));
    if (componentIndex < 0 && !isDerivedVonMises && !isDerivedMag)
    {
        qWarning() << "NativeReadersDataBuilder::readVarDatas: unknown component" << componentName;
        return false;
    }

    // vonMises 派生需要六个张量分量索引（规范名定位，顺序无关）
    int xx = -1, yy = -1, zz = -1, xy = -1, yz = -1, zx = -1;
    if (isDerivedVonMises)
    {
        for (int index = 0; index < componentCount; ++index)
        {
            const QString name = canonicalComponents.at(index);
            if (name == QStringLiteral("XX")) xx = index;
            else if (name == QStringLiteral("YY")) yy = index;
            else if (name == QStringLiteral("ZZ")) zz = index;
            else if (name == QStringLiteral("XY")) xy = index;
            else if (name == QStringLiteral("YZ")) yz = index;
            else if (name == QStringLiteral("ZX")) zx = index;
        }
        if (xx < 0 || yy < 0 || zz < 0 || xy < 0 || yz < 0 || zx < 0)
        {
            qWarning() << "NativeReadersDataBuilder::readVarDatas: cannot derive vonMises for"
                       << variableName;
            return false;
        }
    }
    int magX = -1, magY = -1, magZ = -1;
    if (isDerivedMag)
    {
        for (int index = 0; index < componentCount; ++index)
        {
            const QString name = canonicalComponents.at(index);
            if (name == QStringLiteral("X") && magX < 0) magX = index;
            else if (name == QStringLiteral("Y") && magY < 0) magY = index;
            else if (name == QStringLiteral("Z") && magZ < 0) magZ = index;
        }
    }

    // 层定位：layerName 命中 field.layers 标签时只取该层切片（GUI 层下拉框选择）；
    // 为空时跨层取平均，与旧 OP2 链路对壳 z1/z2 取平均的处理一致。场本身无层而
    // 请求带层、或层名不匹配，均为调用方与注册表漂移，明确报错而非猜测回退
    const int layerCount = target->layers.empty() ? 1 : static_cast<int>(target->layers.size());
    int layerIndex = -1;
    if (!layerName.isEmpty())
    {
        if (target->layers.empty())
        {
            qWarning() << "NativeReadersDataBuilder::readVarDatas: variable" << variableName
                       << "has no layers";
            return false;
        }
        for (int index = 0; index < layerCount; ++index)
        {
            if (QString::fromStdString(target->layers[static_cast<std::size_t>(index)]) == layerName)
            {
                layerIndex = index;
                break;
            }
        }
        if (layerIndex < 0)
        {
            qWarning() << "NativeReadersDataBuilder::readVarDatas: unknown layer" << layerName
                       << "for variable" << variableName;
            return false;
        }
    }

    // 逐实体摊平：values 行主序 entity → layer → component；element_nodal 显式
    // 映射场在 entity 与 layer 之间多一层 node（每单元节点数可变），GUI 以节点
    // 等权平均归一到单元值显示（选中层取层切片后再平均节点，未选层跨层跨节点
    // 平均；派生分量 vonMises/Mag 一律在平均后的分量上计算，与层平均口径一致）
    QHash<int, double> values;
    values.reserve(static_cast<int>(target->ids.size()));
    QHash<int, QVector3D> displacement;
    if (isDisplacement)
        displacement.reserve(static_cast<int>(target->ids.size()));

    std::size_t rowValueOffset = 0;
    for (std::size_t entityIndex = 0; entityIndex < target->ids.size(); ++entityIndex)
    {
        const int entityId = static_cast<int>(target->ids[entityIndex]);
        const std::size_t entityNodeCount = pairedElementNodal
                                                ? target->element_node_counts[entityIndex]
                                                : 1;
        const double* entityValues = target->values.data() + rowValueOffset;
        rowValueOffset += entityNodeCount * target->width();

        // 选中层时取该层分量值，未选层时跨层平均（layerIndex 在上方定位）；
        // element_nodal 场再对节点行取等权平均（entityNodeCount 为 1 时即
        // 旧均匀行行为）
        auto layerValue = [&](int index) {
            double sum = 0.0;
            for (std::size_t node = 0; node < entityNodeCount; ++node)
            {
                const double* nodeValues = entityValues + node * target->width();
                if (layerIndex >= 0)
                {
                    sum += nodeValues[static_cast<std::size_t>(layerIndex) * componentCount + index];
                }
                else
                {
                    for (int layer = 0; layer < layerCount; ++layer)
                        sum += nodeValues[static_cast<std::size_t>(layer) * componentCount + index];
                }
            }
            const double divisor = static_cast<double>(entityNodeCount) *
                                   static_cast<double>(layerIndex >= 0 ? 1 : layerCount);
            return sum / divisor;
        };

        double value = 0.0;
        if (isDerivedVonMises)
        {
            const double sxx = layerValue(xx);
            const double syy = layerValue(yy);
            const double szz = layerValue(zz);
            const double sxy = layerValue(xy);
            const double syz = layerValue(yz);
            const double szx = layerValue(zx);
            const bool isShell = !isNodal &&
                                 isShellElementType(modelData->getEleTypeByEleId(entityId));
            value = isShell ? std::sqrt(sxx * sxx - sxx * syy + syy * syy + 3.0 * sxy * sxy)
                            : std::sqrt(0.5 * ((sxx - syy) * (sxx - syy) + (syy - szz) * (syy - szz) +
                                               (szz - sxx) * (szz - sxx)) +
                                       3.0 * (sxy * sxy + syz * syz + szx * szx));
        }
        else if (isDerivedMag)
        {
            value = std::sqrt(layerValue(magX) * layerValue(magX) +
                              layerValue(magY) * layerValue(magY) +
                              layerValue(magZ) * layerValue(magZ));
        }
        else
        {
            value = layerValue(componentIndex);
        }

        if (isNodal)
        {
            const int continuousId = modelData->_nodedata.nodeidmap.value(entityId, -1);
            if (continuousId < 0)
                continue;
            values.insert(continuousId, value);
            if (isDisplacement)
            {
                displacement.insert(continuousId,
                                    QVector3D(static_cast<float>(layerValue(0)),
                                              static_cast<float>(layerValue(1)),
                                              static_cast<float>(layerValue(2))));
            }
        }
        else
        {
            if (modelData->getEleTypeByEleId(entityId) == ElementType::NoneElement)
                continue;
            values.insert(entityId, value);
        }
    }

    if (isDisplacement)
    {
        modelData->DisplacementValuesR = displacement;
        modelData->DisplacementValuesV.clear();
        db->setDisplacementDataByFrameID(frameIndex, displacement);
    }

    return publishComponentValues(db, frameIndex, variableName, componentName, varType, values);
}

bool NativeReadersDataBuilder::readNodalAverageDataFrame(const native_readers::Dataset& model,
                                                         const native_readers::Frame& frame,
                                                         QPointer<PostData::mPostDataBase> db,
                                                         int frameIndex,
                                                         const QString& variableName,
                                                         const QString& baseName,
                                                         const QString& componentName)
{
    if (db.isNull())
    {
        qWarning() << "NativeReadersDataBuilder::readNodalAverageData: database is null";
        return false;
    }
    QPointer<mPostModelData> modelData = db->getModelData(frameIndex);
    if (modelData == nullptr)
    {
        qWarning() << "NativeReadersDataBuilder::readNodalAverageData: frame" << frameIndex
                   << "is null";
        return false;
    }

    // 分量定位：伪变量只注册规范六张量分量 + 派生 vonMises
    int componentIndex = -1;
    for (int index = 0; index < 6; ++index)
    {
        if (componentName == QString::fromLatin1(kTensorComponentNames[index]))
        {
            componentIndex = index;
            break;
        }
    }
    const bool isDerivedVonMises =
        componentIndex < 0 && componentName == QStringLiteral("vonMises");
    if (componentIndex < 0 && !isDerivedVonMises)
    {
        qWarning() << "NativeReadersDataBuilder::readNodalAverageData: unknown component"
                   << componentName;
        return false;
    }

    // 聚合 base 相同的全部角点源场（OP2 混合 CTETRA/CHEXA/CPENTA 时为多个）
    std::vector<const native_readers::Field*> sources;
    for (const auto& field : frame.fields)
    {
        if (nodalAverageBaseName(field) == baseName)
            sources.push_back(&field);
    }
    if (sources.empty())
    {
        qWarning() << "NativeReadersDataBuilder::readNodalAverageData: no corner source field"
                   << "for" << variableName;
        return false;
    }

    std::unordered_map<native_readers::Id, const native_readers::Element*> elementById;
    for (const auto& element : model.elements)
        elementById[element.id] = &element;

    // 节点累加（连续 id）：每个 (单元, 角点) 等权一份，共享节点自动平均；
    // vonMises 由节点平均后的张量派生（对齐 HyperView averaged-at-nodes 口径，
    // 节点处无壳/实体之分，统一用三维公式）
    QHash<int, std::array<double, 6>> tensorSums;
    QHash<int, int> tensorCounts;
    for (const native_readers::Field* source : sources)
    {
        const int componentCount = static_cast<int>(source->components.size());
        const int layerCount = static_cast<int>(source->layers.size());
        if (componentCount == 0 || source->ids.empty() ||
            source->values.size() != source->ids.size() * source->width())
        {
            qWarning() << "NativeReadersDataBuilder::readNodalAverageData: field"
                       << QString::fromStdString(source->name) << "has inconsistent data";
            return false;
        }
        const bool gridLeading = QString::fromStdString(source->components[0]).compare(
                                     QStringLiteral("GRID"), Qt::CaseInsensitive) == 0;

        for (std::size_t entityIndex = 0; entityIndex < source->ids.size(); ++entityIndex)
        {
            const double* entityValues =
                source->values.data() + entityIndex * source->width();
            for (int layer = 0; layer < layerCount; ++layer)
            {
                const double* corner = entityValues +
                    static_cast<std::size_t>(layer) * componentCount;
                int continuousId = -1;
                const double* tensor = nullptr;
                if (gridLeading)
                {
                    // OP2 形态：分量 0 即角点节点号
                    const int nodeId = static_cast<int>(corner[0]);
                    continuousId = modelData->_nodedata.nodeidmap.value(nodeId, -1);
                    tensor = corner + 1;
                }
                else
                {
                    // RST 形态：角点层序号 = 连接槽位（退化单元按原槽位，零槽跳过）；
                    // 注意不能命名为 slots——那是 Qt 关键字宏，会被预处理器吃掉
                    const auto elementIt = elementById.find(source->ids[entityIndex]);
                    if (elementIt == elementById.end())
                        continue;
                    const auto& cornerSlots = elementIt->second->source_node_slots.empty()
                                                  ? elementIt->second->nodes
                                                  : elementIt->second->source_node_slots;
                    if (static_cast<std::size_t>(layer) >= cornerSlots.size() ||
                        cornerSlots[layer] <= 0)
                        continue;
                    continuousId = modelData->_nodedata.nodeidmap.value(
                        static_cast<int>(cornerSlots[layer]), -1);
                    tensor = corner;
                }
                if (continuousId < 0 || tensor == nullptr)
                    continue;

                std::array<double, 6>& sum = tensorSums[continuousId];
                for (int index = 0; index < 6; ++index)
                    sum[static_cast<std::size_t>(index)] += tensor[index];
                ++tensorCounts[continuousId];
            }
        }
    }
    if (tensorSums.isEmpty())
    {
        qWarning() << "NativeReadersDataBuilder::readNodalAverageData: no corner value mapped"
                   << "to mesh nodes for" << variableName;
        return false;
    }

    QHash<int, double> values;
    values.reserve(tensorSums.size());
    for (auto it = tensorSums.constBegin(); it != tensorSums.constEnd(); ++it)
    {
        const int count = tensorCounts.value(it.key(), 0);
        if (count <= 0)
            continue;
        std::array<double, 6> tensor = it.value();
        for (double& component : tensor)
            component /= count;
        if (isDerivedVonMises)
        {
            values.insert(it.key(),
                          std::sqrt(0.5 * ((tensor[0] - tensor[1]) * (tensor[0] - tensor[1]) +
                                           (tensor[1] - tensor[2]) * (tensor[1] - tensor[2]) +
                                           (tensor[2] - tensor[0]) * (tensor[2] - tensor[0])) +
                                     3.0 * (tensor[3] * tensor[3] + tensor[4] * tensor[4] +
                                            tensor[5] * tensor[5])));
        }
        else
        {
            values.insert(it.key(), tensor[static_cast<std::size_t>(componentIndex)]);
        }
    }

    return publishComponentValues(db, frameIndex, variableName, componentName, 0, values);
}

QPointer<PostData::mPostModelData> NativeReadersDataBuilder::cloneGeometry(
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
