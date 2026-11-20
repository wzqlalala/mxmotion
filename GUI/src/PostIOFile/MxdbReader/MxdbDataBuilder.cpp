#include "MxdbDataBuilder.h"
#include "MxdbReaderCore/MxdbPrecisionPolicy.h"
#include "MxdbReaderCore/MxdbExtensionParser.h"

#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mElementFunction.h"
#include "mPostEnumType.h"

#include <QDebug>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <set>
#include <cmath>
#include <QString>
#include "mOtherFunction.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"

namespace MIOFile {

using namespace MxdbReader;
using PostData::mPostDataBase;
using PostData::mPostModelData;
using PostData::mPostPartData;
using PostData::postelementdata;
using PostData::EleInfo;
using PostData::EleProperty;
using PostData::PostCelllType;
using PostData::postElementFunction;
using MxFunctions::mElementFunction;

namespace {
    // 单元类型字符串 → ElementType 缓存。buildPartData 对每个单元做两遍
    // 字符串归一化（fromStdString/trimmed/toUpper）+ 链式字符串比较，
    // 十几万单元的大模型下是导入耗时的显著组成，而类型字符串集合极小。
    // 导入运行在工作线程，加锁保护。
    QMutex& elementTypeCacheMutex()
    {
        static QMutex mutex;
        return mutex;
    }
    QHash<QString, MxFunctions::ElementType>& elementTypeCache()
    {
        static QHash<QString, MxFunctions::ElementType> cache;
        return cache;
    }
    // 库枚举（读取端经 MxdbElementTypeRegistry 归一后的单元类型）→ GUI ElementType。
    // 库新增字符串别名时本表无需改动；仅新增单元种类（枚举值）时需按声明顺序补一行，
    // 下方 static_assert 依赖 MxdbModelData.h 的 Count 哨兵，漏补会编译失败。
    MxFunctions::ElementType elementTypeFromLibrary(MxdbElementType type)
    {
        static const MxFunctions::ElementType map[] = {
            MxFunctions::NoneElement, // Unknown
            MxFunctions::D3T4,        // Tetra4
            MxFunctions::D3T10,       // Tetra10
            MxFunctions::SOLID,       // Hexa8
            MxFunctions::D3H20,       // Hexa20
            MxFunctions::D3P5,        // Pyramid5
            MxFunctions::D3P6,        // Wedge6
            MxFunctions::D3P15,       // Wedge15
            MxFunctions::D3T4,        // D3T4
            MxFunctions::D3H8,        // D3H8
            MxFunctions::D3P15,       // D3P15
            MxFunctions::D3H20,       // D3H20
            MxFunctions::D3P5,        // D3P5
            MxFunctions::D3P6,        // D3P6
            MxFunctions::D3P13,       // D3P13
            MxFunctions::TRIA3,       // Shell3
            MxFunctions::Q4,          // Shell4
            MxFunctions::TRIA6,       // Shell6
            MxFunctions::SSQ8,        // Shell8
            MxFunctions::D3B2,        // Beam2
            MxFunctions::D3B3,        // Beam3
            MxFunctions::CY_,         // CY_
            MxFunctions::DP3D,        // DP3D
            MxFunctions::R3D3,        // Rigid
            MxFunctions::PST3,        // PlaneTri3
            MxFunctions::PST6,        // PlaneTri6
            MxFunctions::PSQ4,        // PlaneQuad4
            MxFunctions::PSQ8,        // PlaneQuad8
            MxFunctions::MBT3,        // MembraneTri3
            MxFunctions::MBT6,        // MembraneTri6
            MxFunctions::MBQ4,        // MembraneQuad4
            MxFunctions::MBQ8,        // MembraneQuad8
            MxFunctions::Mass1,       // Mass1
            MxFunctions::SPRING1,     // Spring
            MxFunctions::GAP,         // Gap
            MxFunctions::ROD,         // Rod2
            MxFunctions::D3R3,        // Rod3
        };
        static_assert(sizeof(map) / sizeof(map[0]) == static_cast<int>(MxdbReader::MxdbElementType::Count),
                      "MxdbElementType added new values; update elementTypeFromLibrary mapping");
        const int index = static_cast<int>(type);
        if (index <= 0 || index >= static_cast<int>(MxdbReader::MxdbElementType::Count))
            return MxFunctions::NoneElement;
        return map[index];
    }
    MxFunctions::ElementType cachedElementType(const MxdbElement& elem)
    {
        const QString raw = QString::fromStdString(elem.type);
        {
            QMutexLocker lock(&elementTypeCacheMutex());
            const auto it = elementTypeCache().constFind(raw);
            if (it != elementTypeCache().constEnd())
                return it.value();
        }
        QString typeStr = raw.trimmed().toUpper();
        // 库注册表优先：mxdb 文件的单元类型含义与节点数均由库归一（elem.elementType
        // 在读取端按注册表填充，节点数组也按库 nodeCount 读入）。GUI 主表存在同名
        // 不同义（"S3" 主表为壳、库为平面三角形）乃至节点数不一致（"S8R" 主表 4
        // 节点、库 8 节点）的条目，主表优先会与文件实际布局错位，必须以库为准。
        MxFunctions::ElementType elementType = elementTypeFromLibrary(elem.elementType);
        if (elementType == MxFunctions::NoneElement) {
            // 库未识别（求解器变体命名 D3T10M_N2/T3-3 等）时回退 GUI 主表
            elementType = mElementFunction::getElementType(typeStr);
        }
        QMutexLocker lock(&elementTypeCacheMutex());
        elementTypeCache().insert(raw, elementType);
        return elementType;
    }

    // buildPartData 在导入线程执行，无法识别的单元类型会中止整个导入；
    // 该错误必须经全局信号直达用户，避免导入"无提示失败"。
    void reportUnknownElementType(const MxdbPart& part, const MxdbElement& elem)
    {
        const QString partName = QString::fromStdString(part.name);
        const QString typeName = QString::fromStdString(elem.type);
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, QString("部件%1存在无法识别的单元类型：%2，导入中止！").arg(partName).arg(typeName), true);
        } else {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, QString("Part %1 contains unrecognized element type: %2, import aborted!").arg(partName).arg(typeName), true);
        }
    }
}

bool MxdbDataBuilder::inferMagForVariable(const MxdbVariable& var)
{
    if (var.componentCount < 2)
        return false;
    const QString varName = QString::fromStdString(var.name);
    static const QSet<QString> scalarMarkers = {
        QStringLiteral("vonMises"), QStringLiteral("mises"),
        QStringLiteral("Tresca"),   QStringLiteral("tresca"),
        QStringLiteral("MaxPrincipal"), QStringLiteral("MidPrincipal"),
        QStringLiteral("MinPrincipal"), QStringLiteral("MaxShear"),
        QStringLiteral("Intensity"), QStringLiteral("Equivalent")
    };
    for (const QString& marker : scalarMarkers)
    {
        if (varName.contains(marker, Qt::CaseInsensitive))
            return false;
    }
    static const QSet<QString> vectorCompos = {
        QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")
    };
    int vectorHits = 0;
    for (const auto& comp : var.components)
    {
        if (vectorCompos.contains(QString::fromStdString(comp.name).trimmed()))
            ++vectorHits;
    }
    return vectorHits >= 3;
}

bool MxdbDataBuilder::buildInitialFrame(const MxdbInitialFrame& frame, QPointer<mPostDataBase> db)
{
    if (db.isNull()) {
        qWarning() << "MxdbDataBuilder::buildInitialFrame: db is null";
        return false;
    }

    if (frame.nodes.empty()) {
        qWarning() << "MxdbDataBuilder::buildInitialFrame: no nodes";
        return false;
    }

    QPointer<mPostModelData> modelData = new mPostModelData();
    modelData->setDataFlag(true);

    if (!buildNodeData(modelData.data(), frame.nodes))
        return false;

    int partId = 0;
    QHash<QString, QVector<int>> materialParts;
    QHash<QString, QVector<int>> sectionParts;
    for (const auto& part : frame.parts) {
        if (part.elements.empty())
            continue;
        if (!buildPartData(modelData.data(), part, partId))
            return false;
        // 收集材料/截面与部件的对应关系（v05 及以前为空串，v06+ 无材料/截面为 default）。
        // 材料名 default 仍生成节点，便于用户知道哪些部件采用了默认材料；截面过滤 default。
        QString materialName = QString::fromStdString(part.materialName).trimmed();
        if (!materialName.isEmpty())
            materialParts[materialName].push_back(partId);
        QString sectionName = QString::fromStdString(part.sectionName).trimmed();
        if (!sectionName.isEmpty() && sectionName != QStringLiteral("default"))
            sectionParts[sectionName].push_back(partId);
        ++partId;
    }

    // 注册初始帧到 _modelDatabase[1]（旧版 1-based 约定，mTreeView/动画模块兼容）。
    // 不 appendInitframe：同一对象同时进 _initFrame 和 _modelDatabase 会导致
    // clearDataBasesData() 双重释放。
    PostData::PostFrameInfo initFrameInfo;
    initFrameInfo._frameNum = 1;
    initFrameInfo._frameName = QString("1");
    // 新版记录式格式（601/602）：初始帧（文件帧 0）无变量记录，工况置空使其不进
    // 工况/帧下拉（与旧链路 .mxdb0 caseName 为空同一隐式约定，见下方 appendInitframe
    // 注释）——否则帧名 "1" 与首个结果帧（重命名后同名）在 getFrameNumByCaseAndFrameName
    // 撞车，切到首帧时变量表被解析到无变量的初始帧只剩 SolidColor
    if (frame.version == 0xA1B1C601 || frame.version == 0xA1B1C602)
        initFrameInfo._frameCase = QString();
    else
        initFrameInfo._frameCase = QString::fromStdString(frame.caseName);
    initFrameInfo._frameTime = 0.0;
    initFrameInfo.type = 0;
    modelData->_frameInfo = initFrameInfo;
    db->appendModelData(1, modelData);
    db->setDisplacementDataByFrameID(1, {});
    db->distributeFrameIdAndResult(1, {});
    db->distributeFrameIdAndResultType(1, 0);
    // 填充分区数据（显式 v05+）
    for (const auto& partition : frame.partitions) {
        std::set<int> eleIds(partition.elementIds.begin(), partition.elementIds.end());
        modelData->_partitionElements.insert(partition.id, eleIds);
    }

    // 注册初始帧。与旧版 mIMxdbFile1 保持一致：初始帧不进入 _modelDatabase，
    // 因此不会出现在工况/帧下拉框中，也不参与帧切换逻辑。
    db->appendInitframe(modelData);

    if (!buildRigidWallData(db, frame.rigidWalls))
        return false;

    if (!buildLocalCoordinateData(db, frame.localCoordinateSystems))
        return false;

    // 集合块（仅 .mxdb0 初始构型）：type 0-节点集合 1-单元集合 2-单元边集合 3-单元面集合
    QHash<QString, std::vector<int>> elementSets;
    QHash<QString, std::vector<int>> nodeSets;
    QHash<QString, std::vector<std::pair<int, int>>> faceSets;
    QHash<QString, std::vector<std::pair<int, int>>> edgeSets;
    //集合成员有效性校验表：过滤文件里引用了模型中不存在的节点/单元 ID 的成员
    QSet<int> validNodeIds;
    for (const auto& node : frame.nodes)
        validNodeIds.insert(node.id);
    QSet<int> validElemIds;
    for (const auto& part : frame.parts) {
        for (const auto& elem : part.elements)
            validElemIds.insert(elem.id);
    }
    for (const auto& set : frame.sets) {
        if (set.members.empty())
            continue;
        QString setName = QString::fromStdString(set.name).trimmed();
        if (setName.isEmpty())
            continue;
        int invalidCount = 0;
        if (set.type == 0 || set.type == 1) {
            const QSet<int>& validIds = (set.type == 0) ? validNodeIds : validElemIds;
            std::vector<int> ids;
            ids.reserve(set.members.size());
            for (const auto& member : set.members) {
                if (!validIds.contains(member.globalId)) {
                    ++invalidCount;
                    continue;
                }
                ids.push_back(member.globalId);
            }
            if (set.type == 0)
                nodeSets[setName] = std::move(ids);
            else
                elementSets[setName] = std::move(ids);
        }
        else {
            //边/面集合成员带单元局部边号/面号
            std::vector<std::pair<int, int>> members;
            members.reserve(set.members.size());
            for (const auto& member : set.members) {
                if (!validElemIds.contains(member.globalId)) {
                    ++invalidCount;
                    continue;
                }
                members.emplace_back(member.globalId, member.index);
            }
            if (set.type == 3)
                faceSets[setName] = std::move(members);
            else if (set.type == 2)
                edgeSets[setName] = std::move(members);
        }
        if (invalidCount > 0)
            qWarning() << "MxdbDataBuilder::buildInitialFrame: set" << setName
                       << "contains" << invalidCount << "invalid member ids, filtered";
    }
    db->setMaterialParts(materialParts);
    db->setSectionParts(sectionParts);
    db->setElementSets(elementSets);
    db->setNodeSets(nodeSets);
    db->setFaceSets(faceSets);
    db->setEdgeSets(edgeSets);

    return true;
}

bool MxdbDataBuilder::buildNodeData(mPostModelData* modelData, const std::vector<MxdbNode>& nodes)
{
    const int nodeAmount = static_cast<int>(nodes.size());
    modelData->_nodedata.nodeamount = nodeAmount;
    modelData->_nodedata.isDle = true;
    modelData->_nodedata.ids = new int[nodeAmount];
    modelData->_nodedata.nodeposD = new double[nodeAmount * 3];

    int* ids = modelData->_nodedata.ids;
    double* pos = modelData->_nodedata.nodeposD;
    QHash<int, int> nodeidmap;
    nodeidmap.reserve(nodeAmount);

    for (int i = 0; i < nodeAmount; ++i) {
        const MxdbNode& node = nodes[i];
        ids[i] = node.id;
        nodeidmap.insert(node.id, i);
        pos[i * 3] = node.coord[0];
        pos[i * 3 + 1] = node.coord[1];
        pos[i * 3 + 2] = node.coord[2];
    }
    modelData->_nodedata.nodeidmap = nodeidmap;
    return true;
}

bool MxdbDataBuilder::buildPartData(mPostModelData* modelData, const MxdbPart& part, int partId)
{
    QPointer<mPostPartData> partData = new mPostPartData(partId);
    partData->setPartName(QString::fromStdString(part.name));

    const int elecount = static_cast<int>(part.elements.size());

    // 先统计所有单元节点总数，处理 D3H20 -> D3P15 的退化情况
    int allNodeNum = 0;
    for (const auto& elem : part.elements) {
        MxFunctions::ElementType elementType = cachedElementType(elem);
        if (elementType == MxFunctions::NoneElement) {
            qWarning() << "MxdbDataBuilder::buildPartData: unknown element type:" << elem.type.c_str();
            reportUnknownElementType(part, elem);
            return false;
        }
        PostCelllType postCellType = postElementFunction::MapFunctionTypeToPostCellType(elementType);
        int elenode = postElementFunction::getNodeCountByPostCellType(postCellType);

        if (elementType == MxFunctions::D3H20 &&
            isSameNode(elem.nodeIds, {4, 5, 12}) &&
            isSameNode(elem.nodeIds, {6, 7, 14}) &&
            isSameNode(elem.nodeIds, {13, 15})) {
            allNodeNum += 15;
        } else {
            allNodeNum += elenode;
        }
    }

    postelementdata& ped = partData->_parteledata;
    ped.cell_types.resize(elecount);
    ped.cell_rid.resize(elecount);
    ped.cell_node_counts.resize(elecount);
    ped.cell_node_offsets.resize(elecount);
    ped.cell_info.resize(elecount);
    ped.cell_node_ids.resize(allNodeNum);

    int nodeIndex = 0;
    for (int j = 0; j < elecount; ++j) {
        const MxdbElement& elem = part.elements[j];
        ped.cell_rid_to_cid.insert({elem.id, j});

        MxFunctions::ElementType elementType = cachedElementType(elem);
        if (elementType == MxFunctions::NoneElement) {
            qWarning() << "MxdbDataBuilder::buildPartData: unknown element type:" << elem.type.c_str();
            reportUnknownElementType(part, elem);
            return false;
        }

        PostCelllType postCellType = postElementFunction::MapFunctionTypeToPostCellType(elementType);
        int elenode = postElementFunction::getNodeCountByPostCellType(postCellType);

        // DP3D 球单元半径
        if (elementType == MxFunctions::DP3D) {
            ped.ballElementRadius.insert(elem.id, elem.ballRadius);
        }

        std::vector<int> resfortest;
        if (elementType == MxFunctions::D3H20 &&
            isSameNode(elem.nodeIds, {4, 5, 12}) &&
            isSameNode(elem.nodeIds, {6, 7, 14}) &&
            isSameNode(elem.nodeIds, {13, 15})) {
            elementType = MxFunctions::D3P15;
            d3h20ToD3p15(elem.nodeIds, resfortest);
            for (int& nodeId : resfortest) {
                nodeId = modelData->_nodedata.nodeidmap.value(nodeId);
            }
        } else {
            resfortest.reserve(elenode);
            for (int k = 0; k < elenode; ++k) {
                resfortest.push_back(modelData->_nodedata.nodeidmap.value(elem.nodeIds[k]));
            }
        }

        // 更新节点所属单元计数
        for (int cId : resfortest) {
            auto it = modelData->_nodedata.nodeEleCountmap.find(cId);
            if (it == modelData->_nodedata.nodeEleCountmap.end()) {
                modelData->_nodedata.nodeEleCountmap.insert(cId, 1);
            } else {
                it.value() = it.value() + 1;
            }
        }

        EleProperty* tempEle = new EleProperty();
        EleInfo eleInfo;
        eleInfo.type = elementType;
        eleInfo.is_show = true;
        eleInfo.is_select = true;
        eleInfo.property = tempEle;

        ped.elementtype_elemcount[elementType] = ped.elementtype_elemcount.value(elementType, 0) + 1;
        ped.cell_rid[j] = elem.id;
        ped.cell_node_counts[j] = static_cast<int>(resfortest.size());
        ped.cell_types[j] = postElementFunction::MapFunctionTypeToPostCellType(elementType);
        ped.cell_info[j] = eleInfo;
        ped.cell_node_offsets[j] = nodeIndex;

        for (int cId : resfortest) {
            ped.cell_node_ids[nodeIndex] = cId;
            ++nodeIndex;
        }
    }

    modelData->_partdata.push_back(partData);
    modelData->_partdataForFind.insert(partData->getPartID(), partData);
    return true;
}

bool MxdbDataBuilder::buildRigidWallData(mPostDataBase* db, const std::vector<MxdbRigidWall>& walls)
{
    for (const auto& wall : walls) {
        // 读取端兜底过滤：错位/损坏文件可产出零值或垃圾墙（如 EOF 补零后 type=0
        // 的零法线"无限大平面"），id 非法、法线含非有限值或法线段全零的墙不进
        // 模型库——零法线无限大平面在渲染端产生退化几何，足以污染整个视口。
        // 新版记录式格式（0xA1B1C601/602）的刚性墙仅类型无几何（规范 DR-014），
        // 法线恒零，在此被全部跳过（待规范补全几何定义后接入显示）
        bool normalFinite = true;
        bool normalNonZero = false;
        for (int k = 0; k < 6; ++k) {
            if (!std::isfinite(wall.normal[k]))
                normalFinite = false;
            if (wall.normal[k] != 0.0)
                normalNonZero = true;
        }
        if (wall.id <= 0 || !normalFinite || !normalNonZero) {
            qWarning() << "MxdbDataBuilder::buildRigidWallData: skip invalid rigid wall, id =" << wall.id
                       << " type =" << wall.type;
            continue;
        }

        double normal[6];
        memcpy(normal, wall.normal, 6 * sizeof(double));

        switch (wall.type) {
        case 0: {
            PostData::mPostInfinitelyLargePlaneRigidWall* rigidWall =
                new PostData::mPostInfinitelyLargePlaneRigidWall(wall.id, normal);
            db->appendRigidWallData(rigidWall);
            break;
        }
        case 1: {
            PostData::mPostLimitedLargePlaneRigidWall* rigidWall =
                new PostData::mPostLimitedLargePlaneRigidWall(wall.id, normal);
            memcpy(rigidWall->_lineEndVector, wall.edgeVectorEnd, 3 * sizeof(double));
            rigidWall->_L = wall.lengthL;
            rigidWall->_M = wall.lengthM;
            db->appendRigidWallData(rigidWall);
            break;
        }
        case 2: {
            PostData::mPostRectangularRigidWall* rigidWall =
                new PostData::mPostRectangularRigidWall(wall.id, normal);
            memcpy(rigidWall->_lineEndVector, wall.edgeVectorEnd, 3 * sizeof(double));
            rigidWall->_L = wall.lengthL;
            rigidWall->_M = wall.lengthM;
            rigidWall->_P = wall.lengthP;
            db->appendRigidWallData(rigidWall);
            break;
        }
        case 3: {
            PostData::mPostCylindricalRigidWall* rigidWall =
                new PostData::mPostCylindricalRigidWall(wall.id, normal);
            rigidWall->_radius = wall.radius;
            rigidWall->_length = wall.axialLength;
            db->appendRigidWallData(rigidWall);
            break;
        }
        case 4: {
            PostData::mPostSphericalRigidWall* rigidWall =
                new PostData::mPostSphericalRigidWall(wall.id, normal);
            rigidWall->_radius = wall.radius;
            db->appendRigidWallData(rigidWall);
            break;
        }
        default:
            qWarning() << "MxdbDataBuilder::buildRigidWallData: unknown rigid wall type:" << wall.type;
            break;
        }
    }
    return true;
}

bool MxdbDataBuilder::buildLocalCoordinateData(mPostDataBase* db,
                                             const std::vector<MxdbLocalCoordinateSystem>& coords)
{
    for (const auto& coord : coords) {
        QVector<double> Qposition;
        QVector<double> Qcoordinate;
        Qposition.reserve(3);
        Qcoordinate.reserve(9);
        for (int i = 0; i < 3; ++i)
            Qposition.append(coord.position[i]);
        for (int i = 0; i < 9; ++i)
            Qcoordinate.append(coord.vectors[i]);
        QString name = QString::fromStdString(coord.name);
        db->appendLocalCoordinate1(name, Qcoordinate, Qposition);
    }
    return true;
}

bool MxdbDataBuilder::isSameNode(const std::vector<int>& ids, const std::vector<int>& poss)
{
    if (ids.empty())
        return false;
    std::set<int> temp;
    for (int pos : poss) {
        if (pos < 0 || pos >= static_cast<int>(ids.size()))
            return false;
        temp.insert(ids[pos]);
    }
    return temp.size() == 1;
}

void MxdbDataBuilder::d3h20ToD3p15(const std::vector<int>& ids, std::vector<int>& nodeIds)
{
    if (ids.size() < 20)
        return;
    nodeIds.resize(15);
    nodeIds[0] = ids[3];
    nodeIds[1] = ids[2];
    nodeIds[2] = ids[6];
    nodeIds[3] = ids[0];
    nodeIds[4] = ids[1];
    nodeIds[5] = ids[5];
    nodeIds[6] = ids[10];
    nodeIds[7] = ids[18];
    nodeIds[8] = ids[19];
    nodeIds[9] = ids[8];
    nodeIds[10] = ids[17];
    nodeIds[11] = ids[16];
    nodeIds[12] = ids[11];
    nodeIds[13] = ids[9];
    nodeIds[14] = ids[13];
}

static QPointer<mPostModelData> cloneModelData(const mPostModelData* source)
{
    if (!source)
        return nullptr;

    QPointer<mPostModelData> clone = new mPostModelData();
    clone->setDataFlag(true);
    clone->_oldVersion = source->_oldVersion;

    clone->_nodedata.nodeamount = source->_nodedata.nodeamount;
    clone->_nodedata.isDle = source->_nodedata.isDle;
    clone->_nodedata.nodeidmap = source->_nodedata.nodeidmap;
    clone->_nodedata.nodeEleCountmap = source->_nodedata.nodeEleCountmap;

    if (source->_nodedata.isDle) {
        clone->_nodedata.nodeposD = new double[clone->_nodedata.nodeamount * 3];
        memcpy(clone->_nodedata.nodeposD, source->_nodedata.nodeposD,
               clone->_nodedata.nodeamount * sizeof(double) * 3);
    } else {
        clone->_nodedata.nodeposF = new float[clone->_nodedata.nodeamount * 3];
        memcpy(clone->_nodedata.nodeposF, source->_nodedata.nodeposF,
               clone->_nodedata.nodeamount * sizeof(float) * 3);
    }
    clone->_nodedata.ids = new int[clone->_nodedata.nodeamount];
    memcpy(clone->_nodedata.ids, source->_nodedata.ids, clone->_nodedata.nodeamount * sizeof(int));

    clone->_partitionElements = source->_partitionElements;

    // 深拷贝 _partdata，避免多帧共享 mPostPartData 导致 is_show 与 _partfacedata 跨帧污染
    clone->_partdata.clear();
    for (auto sourcePart : source->_partdata) {
        if (sourcePart == nullptr)
            continue;
        QPointer<mPostPartData> newPart = new mPostPartData(sourcePart->getPartID());
        newPart->setPartName(sourcePart->getPartName());
        // 深拷贝单元数据与面数据
        newPart->_parteledata = sourcePart->_parteledata;
        newPart->_partfacedata = sourcePart->_partfacedata;
        clone->_partdata.push_back(newPart);
        clone->_partdataForFind.insert(newPart->getPartID(), newPart);
    }

    return clone;
}

bool MxdbDataBuilder::buildResultFrame(const MxdbResultFrame& frame, QPointer<mPostDataBase> db, int frameIndex)
{
    if (db.isNull()) {
        qWarning() << "MxdbDataBuilder::buildResultFrame: db is null";
        return false;
    }
    if (frameIndex <= 0) {
        qWarning() << "MxdbDataBuilder::buildResultFrame: invalid frame index" << frameIndex;
        return false;
    }

    QPointer<mPostModelData> modelData;
    if (frame.sameFrame == -1) {
        if (frame.newModel.nodes.empty()) {
            qWarning() << "MxdbDataBuilder::buildResultFrame: new model is empty";
            return false;
        }
        modelData = new mPostModelData();
        modelData->setDataFlag(true);
        if (!buildNodeData(modelData.data(), frame.newModel.nodes))
            return false;
        int partId = 0;
        for (const auto& part : frame.newModel.parts) {
            if (part.elements.empty())
                continue;
            if (!buildPartData(modelData.data(), part, partId))
                return false;
            ++partId;
        }
        // 新模型帧：沿用初始帧的分区数据
        QPointer<mPostModelData> initFrame = db->getInitframe();
        if (!initFrame.isNull()) {
            modelData->_partitionElements = initFrame->_partitionElements;
        }
    } else if (frame.sameFrame == 0) {
        // 初始帧注册在 _modelDatabase[1]
        QPointer<mPostModelData> initFrame = db->getModelData(1);
        if (initFrame.isNull()) {
            qWarning() << "MxdbDataBuilder::buildResultFrame: init frame is null";
            return false;
        }
        modelData = cloneModelData(initFrame.data());
        db->setSameflag(true);
    } else {
        QPointer<mPostModelData> refFrame = db->getModelData(frame.sameFrame);
        if (refFrame.isNull()) {
            qWarning() << "MxdbDataBuilder::buildResultFrame: reference frame" << frame.sameFrame << "not found";
            return false;
        }
        modelData = cloneModelData(refFrame.data());
    }

    if (modelData.isNull()) {
        qWarning() << "MxdbDataBuilder::buildResultFrame: model data is null";
        return false;
    }

    // 构建帧信息。
    // _frameName 先使用全局帧序号作为占位符；mIMxdbFile1 读取结果帧后会根据
    // 结果文件名后缀（如 Cube_1.mxdb -> "1"）覆盖 _frameName，与旧版 mIMxdbFile1
    // 行为保持一致，保证 GUI 切换帧时能找到唯一帧。
    PostData::PostFrameInfo frameInfo;
    frameInfo._frameNum = frameIndex;
    frameInfo.sameFrame = frame.sameFrame;
    frameInfo._frameCase = QString::fromStdString(frame.caseName);
    frameInfo._frameName = MxFunctions::mNumberToString(frameIndex);
    frameInfo._frameFile = QString::fromStdString(frame.caseName);
    frameInfo.type = frame.timeOrFreqType;
    frameInfo._frameTime = frame.timeOrFreqValue;

    // 新版记录式格式（0xA1B1C601/602）的变量元信息由调用方（mMxdbThread::
    // convertFrameVariables）统一注册：单元变量的分量/层在结果分组内、定位键为
    // 值区偏移（var.varByte 恒 0），此处按旧字段注册会产生 varByte=0 的重复条目，
    // 被按需读取按变量名先命中导致定位失败
    const bool newFormat = (frame.version == 0xA1B1C601 || frame.version == 0xA1B1C602);
    if (!newFormat) {
        for (const auto& var : frame.variables) {
            PostData::PostVariableInfo varInfo;
            varInfo._resultType = var.resultType;
            varInfo._varName = QString::fromStdString(var.name).trimmed();
            varInfo._varType = var.varType;
            varInfo._varCount = static_cast<int>(var.shapes.size());
            varInfo._nCompo = var.componentCount;
            varInfo._isMag = var.isMag || inferMagForVariable(var);
            varInfo._varByte = var.varByte;

            for (const auto& shape : var.shapes) {
                QStringList layerNames;
                for (const auto& layer : shape.layers) {
                    layerNames.append(QString::fromStdString(layer.name));
                }
                varInfo._varLayerNames1.append(layerNames);
                varInfo._varLayer.append(static_cast<int>(shape.layers.size()));
                for (const auto& layer : shape.layers) {
                    varInfo._varLayerNames.append(QString::fromStdString(layer.name));
                }
            }

            for (const auto& comp : var.components) {
                varInfo._compoNames.append(QString::fromStdString(comp.name));
                varInfo._compoComplex.insert(QString::fromStdString(comp.name), comp.isComplex);
            }

            frameInfo._frameVars.append(varInfo);
        }
    }

    modelData->_frameInfo = frameInfo;

    // 位移数据
    if (!frame.displacementValues.empty()) {
        QHash<int, QVector3D> displacementR;
        QHash<int, QVector3D> displacementV;
        displacementR.reserve(static_cast<int>(frame.displacementValues.size()));
        displacementV.reserve(static_cast<int>(frame.displacementValues.size()));
        for (const auto& val : frame.displacementValues) {
            int cid = modelData->_nodedata.nodeidmap.value(val.entityId, -1);
            if (cid < 0)
                continue;
            displacementR.insert(cid, QVector3D(val.real[0], val.real[1], val.real[2]));
            displacementV.insert(cid, QVector3D(val.imag[0], val.imag[1], val.imag[2]));
        }
        modelData->DisplacementValuesR = displacementR;
        modelData->DisplacementValuesV = displacementV;
        db->setDisplacementDataByFrameID(frameIndex, displacementR);
    }

    // 刚性墙位置信息
    if (!frame.rigidWallPositions.empty()) {
        for (const auto& wall : frame.rigidWallPositions) {
            if (wall.type == 0) {
                frameInfo.setRigidWallEndPos(wall.id, QVector3D(wall.normal[0], wall.normal[1], wall.normal[2]));
            }
        }
    }

    // 失效单元（显式 v05+）。
    if (!frame.failedElementIds.empty()) {
        modelData->setLoseElems(std::set<int>(frame.failedElementIds.begin(), frame.failedElementIds.end()));
    } else {
        modelData->setLoseElems(std::set<int>());
    }

    db->appendModelData(frameIndex, modelData);
    return true;
}

bool MxdbDataBuilder::buildVariableValues(const MxdbVariable& var, const std::vector<MxdbVariableValue>& values,
                                          QPointer<mPostDataBase> db, int frameIndex)
{
    return buildVariableValues(var, values, 0, 0, 0, db, frameIndex);
}

bool MxdbDataBuilder::buildVariableValues(const MxdbVariable& var, const std::vector<MxdbVariableValue>& values,
                                          int componentIndex, int layerIndex, int shapeIndex,
                                          QPointer<mPostDataBase> db, int frameIndex)
{
    if (db.isNull()) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: db is null";
        return false;
    }
    if (frameIndex < 0) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: invalid frame index" << frameIndex;
        return false;
    }
    if (var.resultType != 0) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: unsupported result type" << var.resultType;
        return false;
    }
    if (shapeIndex < 0 || shapeIndex >= static_cast<int>(var.shapes.size())) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: invalid shape index" << shapeIndex;
        return false;
    }
    const MxdbShapeInfo& shape = var.shapes[shapeIndex];
    if (layerIndex < 0 || (shape.layers.size() > 0 && layerIndex >= static_cast<int>(shape.layers.size()))) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: invalid layer index" << layerIndex;
        return false;
    }
    if (componentIndex < 0 || componentIndex >= static_cast<int>(var.components.size())) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: invalid component index" << componentIndex;
        return false;
    }

    QPointer<mPostModelData> modelData = db->getModelData(frameIndex);
    if (modelData.isNull()) {
        qWarning() << "MxdbDataBuilder::buildVariableValues: model data for frame" << frameIndex << "is null";
        return false;
    }

    // 计算目标分量在 value.realValues 中的偏移
    auto effectiveLayerCount = [](const MxdbShapeInfo& s) {
        int c = static_cast<int>(s.layers.size());
        return c == 0 ? 1 : c;
    };

    int valueIndex = 0;
    for (int s = 0; s < shapeIndex; ++s) {
        valueIndex += effectiveLayerCount(var.shapes[s]) * var.componentCount;
    }
    valueIndex += layerIndex * var.componentCount + componentIndex;
    if (shape.layers.empty()) {
        // 当层数为 0 时，readVariableValuesCommon 按 1 层处理
        valueIndex = componentIndex;
    }

    const QString varName = QString::fromStdString(var.name).trimmed();
    const QString compoName = QString::fromStdString(var.components[componentIndex].name).trimmed();

    PostData::Compones comp;
    comp.isD = true;
    comp.isNodeFace = false;
    comp.frameID = frameIndex;
    comp.varName = varName;
    comp.compoName = compoName;
    comp.flag = var.varType; // 0 节点，1 单元

    // 节点变量：key 为节点连续 ID
    // 单元变量：key 为单元连续 ID
    for (const auto& val : values) {
        int cid = -1;
        if (var.varType == 0) {
            cid = modelData->_nodedata.nodeidmap.value(val.entityId, -1);
        } else if (var.varType == 1) {
            // 遍历所有 part 查找单元真实 ID 对应的连续 ID
            for (auto part : modelData->_partdata) {
                auto it = part->_parteledata.cell_rid_to_cid.find(val.entityId);
                if (it != part->_parteledata.cell_rid_to_cid.end()) {
                    cid = it->second;
                    break;
                }
            }
        }
        if (cid < 0)
            continue;
        if (valueIndex < 0 || valueIndex >= static_cast<int>(val.realValues.size()))
            continue;
        double v = val.realValues[valueIndex];
        comp.value1.insert(cid, v);
    }

    db->setComponeValues(comp);
    return true;
}

} // namespace MIOFile
