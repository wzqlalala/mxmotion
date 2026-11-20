
#include "mMotionLoadCaseGenerator.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mGeoEngineHandle.h"
#include "mMeshData.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"

#include "mModelData.h"
#include "mConnectorManager.h"
#include "mConnectorDistributedCoupling.h"
#include "mLoadBoundaryManager.h"
#include "mLoadForce.h"
#include "mLoadPressure.h"
#include "mCaseManager.h"
#include "mCaseImplicitLinearStatic.h"
#include "mMapRelationManager.h"
#include "mMixSet.h"

#include <memory>
#include <unordered_map>

using namespace MxFunctions;

namespace MechMainWindow
{
    bool mMotionLoadCaseGenerator::generate(const QVector<GenJoint>& joints,
        const QVector<GenCase>& cases,
        const QMap<int, QVector<int>>& faceBindings,
        QString& error,
        int* nCases, int* nLoads, int* nRbe3)
    {
        if (joints.isEmpty() || cases.isEmpty()) {
            error = QStringLiteral("没有可生成的工况数据。");
            return false;
        }
        if (!getCVM || !getCVM->getModelData()) {
            error = QStringLiteral("当前没有打开的模型。");
            return false;
        }

        // 前置校验：每个参与副的几何面绑定必须非空（通过后才创建任何对象，不留半成品）
        for (const auto& j : joints) {
            if (faceBindings.value(j.jointId).isEmpty()) {
                error = QStringLiteral("运动副「%1」还没有拾取几何面。").arg(j.name);
                return false;
            }
        }

        auto* modelData = getCVM->getModelData();
        auto& connectorMgr = modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
        auto& loadMgr = modelData->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary);
        auto& caseMgr = modelData->getManager<MechCaseData::mCaseManager>(Physical_Case);
        auto& mapMgr = modelData->getMapRelationManager();

        // ---- 唯一命名：收集现有名字，生成时自动避让（重名则追加序号后缀） ----
        QSet<QString> existNames;
        for (int id : modelData->getAllPhyID(Physical_Case)) {
            if (auto* c = caseMgr.getCaseData(id)) existNames.insert(c->getCaseName());
        }
        for (int id : modelData->getAllPhyID(Physical_LoadBoundary)) {
            auto l = loadMgr.getLoadBoundaryData(id);
            if (l) existNames.insert(l->getLoadBoundaryName());
        }
        for (int id : modelData->getAllPhyID(Physical_Connector)) {
            auto cn = connectorMgr.getConnectorData(id);
            if (cn) existNames.insert(cn->getConnectorName());
        }
        for (int id : modelData->getAllPhyID(Physical_Set)) {
            auto* s = modelData->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(id);
            if (s) existNames.insert(s->getSetName());
        }
        auto makeUnique = [&existNames](const QString& base) {
            if (!existNames.contains(base)) { existNames.insert(base); return base; }
            for (int i = 2; ; ++i) {
                const QString cand = QStringLiteral("%1_%2").arg(base).arg(i);
                if (!existNames.contains(cand)) { existNames.insert(cand); return cand; }
            }
        };

        int generatedCases = 0;
        int generatedLoads = 0;
        QMap<int, double> pressureSetAreas;   // 压力 Set id -> 面外侧网格单元总面积

        // 为每个绑定建 RBE3（分布式耦合，供力矩施加），记录 jointId -> 参考点节点 id。
        // 力矩一律走 RBE3；力是否走 RBE3 取决于面是否平面（平面→均布压力）。
        QMap<int, int> jointRefNode;
        for (const auto& j : joints) {
            QString e2;
            const QString connName = makeUnique(QStringLiteral("RBE3_J%1").arg(j.jointId));
            const int refNode = buildRbe3ForJoint(j, faceBindings.value(j.jointId), e2, connName);
            if (refNode < 0) {
                error = QStringLiteral("运动副「%1」建 RBE3 失败：%2").arg(j.name, e2);
                return false;
            }
            jointRefNode.insert(j.jointId, refNode);
        }

        // 为每个"平面"绑定建均布压力 Set（含该副全部选中面），记录 jointId -> (Set id, 代表法向)。
        // 非平面的副不建（力仍走 RBE3）。代表法向取第一个面。
        QMap<int, QPair<int, QVector3D>> jointPressureSet;
        for (const auto& j : joints) {
            const QVector<int>& faceIds = faceBindings.value(j.jointId);
            // 只要有一个面非平面，该副就不走压力（力仍走 RBE3）
            bool allPlanar = true;
            for (int fid : faceIds) {
                if (!isPlanarFace(fid)) { allPlanar = false; break; }
            }
            if (!allPlanar)
                continue;
            QString e2;
            double setArea = 0.0;
            const QString setName = makeUnique(QStringLiteral("PRESS_J%1").arg(j.jointId));
            const int setId = buildPressureSetForJoint(faceIds, e2, setName, setArea);
            if (setId > 0) {
                QVector3D n;
                {
                    const auto attrs = getGEOServer.geoCalculate().CalculateFaceAttr({ faceIds.first() });
                    if (!attrs.empty()) n = attrs[0].m_dir;
                }
                jointPressureSet.insert(j.jointId, { setId, n });

                // 压力计算用面积（p = 法向投影 / S），随 Set 记录
                pressureSetAreas.insert(setId, setArea);
            }
        }

        // 逐工况生成静力工况并施加载荷。
        // 卡顿优化：只在数据层创建（工况+载荷+映射+RBE3），不在循环里逐个刷树；
        // 全部建完后调一次 updateModelTree() 全量刷新。
        int caseSeq = 0;
        for (const auto& lc : cases) {
            ++caseSeq;
            const int caseId = caseMgr.createID();
            auto* caseData = new MechCaseData::mCaseImplicitLinearStatic(caseId, ImplicitLinearStatic);
            // 工况名用 ASCII 且避让重名（MB_<序号>_t<时刻>[_n]）
            const QString caseName = makeUnique(QStringLiteral("MB_%1_t%2s")
                .arg(caseSeq).arg(lc.timeSeconds, 0, 'f', 4));
            caseData->setCaseName(caseName);
            // 惯性释放自动开启（多体反力在固定模型上不一定自平衡，需惯性释放约束刚体）
            caseData->_inertiaRelief = QStringLiteral("自动开启");
            modelData->addPhysicalField<MechCaseData::mCaseBase>(caseData);
            ++generatedCases;

            for (const auto& load : lc.loads) {
                const int refNode = jointRefNode.value(load.jointId, -1);
                if (refNode < 0) continue;   // 该副未绑定，跳过
                // 载荷名用 ASCII：MB<序号>_J<副id>_<时刻>_F/T，唯一可辨
                const QString baseName = QStringLiteral("MB%1_J%2_t%3s")
                    .arg(caseSeq).arg(load.jointId).arg(lc.timeSeconds, 0, 'f', 4);

                const QVector3D& F = load.force;
                const double fm = F.length();
                const double tm = load.torque.length();

                // 力：平面且力接近垂直面→均布压力（p=|F·n|/S）；
                // 否则（非平面，或力有明显面内剪切分量）→独立赋值到 RBE3 参考点（完整不丢剪切）。
                bool appliedForce = false;
                if (fm > 0.0 && jointPressureSet.contains(load.jointId)) {
                    const auto ps = jointPressureSet.value(load.jointId);
                    const int setId = ps.first;
                    const QVector3D faceNormal = ps.second;
                    const double S = pressureSetAreas.value(setId, 0.0);
                    const double fNormal = std::abs(QVector3D::dotProduct(F, faceNormal));
                    // 仅当力基本垂直面（法向分量占比足够高，剪切可忽略）才用压力。
                    // 阈值 cos(10°)≈0.985：法向占比 < 0.985 说明剪切不可忽略，退回 RBE3。
                    const bool forceMostlyNormal = (fm > 0.0) && (fNormal / fm >= 0.985);
                    if (S > 0.0 && forceMostlyNormal) {
                        const double p = fNormal / S;
                        auto pressure = std::make_shared<MechCaseData::mLoadPressure>(
                            loadMgr.createID(), LOAD_IM_PRESSURE,
                            makeUnique(QStringLiteral("%1_P").arg(baseName)));
                        pressure->pressureType_ = 0;
                        pressure->way_ = 0;                 // 统一赋值（带 Set）
                        pressure->pa_ = p;
                        modelData->addPhysicalFieldP(pressure);
                        mapMgr.appendMapRelation(
                            { Physical_LoadBoundary, pressure->getLoadBoundaryId() },
                            { Physical_Set, setId });
                        mapMgr.appendMapRelation(
                            { Physical_LoadBoundary, pressure->getLoadBoundaryId() },
                            { Physical_Case, caseId });
                        ++generatedLoads;
                        appliedForce = true;
                    }
                }
                if (fm > 0.0 && !appliedForce) {
                    auto force = std::make_shared<MechCaseData::mLoadForce>(
                        loadMgr.createID(), LOAD_IM_FORCE,
                        makeUnique(QStringLiteral("%1_F").arg(baseName)));
                    force->way_ = 2;                         // 独立赋值（集中力）
                    force->force_x = F.x();
                    force->force_y = F.y();
                    force->force_z = F.z();
                    force->nodeForceMap_[RenderPickResultData(refNode, -1, -1)] =
                        QVector<QVariant>{ F.x(), F.y(), F.z() };
                    modelData->addPhysicalFieldP(force);
                    mapMgr.appendMapRelation(
                        { Physical_LoadBoundary, force->getLoadBoundaryId() },
                        { Physical_Case, caseId });
                    ++generatedLoads;
                }
                // 力矩：力矩独立赋值模式为 way_=1（区别于集中力的 2）
                if (tm > 0.0) {
                    auto torque = std::make_shared<MechCaseData::mLoadForce>(
                        loadMgr.createID(), LOAD_IM_TORQUE,
                        makeUnique(QStringLiteral("%1_T").arg(baseName)));
                    torque->way_ = 1;
                    torque->force_x = load.torque.x();
                    torque->force_y = load.torque.y();
                    torque->force_z = load.torque.z();
                    torque->nodeForceMap_[RenderPickResultData(refNode, -1, -1)] =
                        QVector<QVariant>{ load.torque.x(), load.torque.y(), load.torque.z() };
                    modelData->addPhysicalFieldP(torque);
                    mapMgr.appendMapRelation(
                        { Physical_LoadBoundary, torque->getLoadBoundaryId() },
                        { Physical_Case, caseId });
                    ++generatedLoads;
                }
            }
        }
        // 一次性全量刷新模型树（把 N 次单点树更新合并成 1 次整树重建）
        emit mGlobalSignals::getInstance()->updateModelTree();

        if (nCases) *nCases = generatedCases;
        if (nLoads) *nLoads = generatedLoads;
        if (nRbe3) *nRbe3 = jointRefNode.size();
        return true;
    }

    int mMotionLoadCaseGenerator::buildRbe3ForJoint(const GenJoint& joint,
        const QVector<int>& geoFaceIds, QString& error, const QString& connName)
    {
        auto* modelData = getCVM->getModelData();
        auto* meshData = modelData->getMeshData();
        auto& connectorMgr = modelData->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);

        // 1) 全部几何面 → 网格节点集（合并多个面）
        std::vector<RenderPickResultData> faceNodes;
        for (int fid : geoFaceIds) {
            const auto nodes = meshData->getNodeByGeoFace(fid);
            faceNodes.insert(faceNodes.end(), nodes.begin(), nodes.end());
        }
        if (faceNodes.empty()) {
            error = QStringLiteral("所选几何面没有对应的网格节点（请先剖分该部件）。");
            return -1;
        }

        // 2) 参考点：用副的 actionPoint 建自由节点（固定模型上的几何位置）
        const int refNode = getCVM->createFreeNode(joint.actionPoint);
        if (refNode < 0) {
            error = QStringLiteral("创建参考点节点失败。");
            return -1;
        }

        // 3) 组装分布式耦合（RBE3）
        auto dc = std::make_shared<MechConnectorData::mConnectorDistributedCoupling>(
            connectorMgr.createID(), Connector_DistributedCoupling);
        dc->setConnectorName(connName);
        dc->way_ = 1;                 // 一对多
        dc->singleweight_ = 1.0;
        dc->degreeoffreedom_main = { 1, 2, 3, 4, 5, 6 };

        // 参考点（subnodes_）
        dc->subnodes_.first = PickDataType::Node;
        dc->subnodes_.second = RenderPickResultData(refNode, -1, -1);

        // 耦合点（mainnodes_）= 面节点集（Node 类型）
        std::vector<RenderPickResultData> nodeData;
        nodeData.reserve(faceNodes.size());
        for (const auto& nd : faceNodes)
            nodeData.push_back(RenderPickResultData(nd.result_id, nd.part_id, nd.result_id2));
        dc->mainnodes_[PickDataType::Node] = nodeData;

        modelData->addPhysicalFieldP(dc);

        // 把参考点登记为连接节点（供符号渲染/清理）
        std::vector<int> refNodes{ refNode };
        meshData->appendConnectNodes(refNodes);

        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateConnectorSig(true, dc->getConnectorId());
        return refNode;
    }

    int mMotionLoadCaseGenerator::buildPressureSetForJoint(const QVector<int>& geoFaceIds,
        QString& error, const QString& setName, double& setArea)
    {
        auto* modelData = getCVM->getModelData();
        auto* meshData = modelData->getMeshData();
        auto& setMgr = modelData->getManager<MechSetData::mSetManager>(Physical_Set);

        // 1) 全部几何面 → 面单元（合并多个面）
        std::vector<RenderPickResultData> elemVec;
        for (int fid : geoFaceIds) {
            const auto elems = meshData->getElementByGeoFace(fid);
            elemVec.insert(elemVec.end(), elems.begin(), elems.end());
        }
        if (elemVec.empty()) {
            error = QStringLiteral("所选几何面没有对应的网格单元（请先剖分该部件）。");
            return -1;
        }

        // 2) 累计全部面外侧离散网格单元总面积 S
        double S = 0.0;
        QVector3D center;
        meshData->getElementAreaAndCenter(elemVec, S, center);
        if (S <= 0.0) {
            error = QStringLiteral("所选几何面的网格单元面积为 0。");
            return -1;
        }

        // 3) 建"几何面"集合（含全部选中面）。写出时 writeMixSetData 对每个 GeoFace
        //    都会用 getElementFaceByGeoFace 转成 elementsurface（单元+面序号），
        //    这是 *surfaceload 能正确识别的对象。internal=true + MixSet 便于面板反显。
        const int setId = setMgr.createID();
        auto* mixSet = new MechSetData::mMixSet(setId, MxFunctions::SetType::Mix,
            setName, true);
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data;
        std::vector<RenderPickResultData> facePicks;
        facePicks.reserve(geoFaceIds.size());
        for (int fid : geoFaceIds)
            facePicks.push_back(RenderPickResultData(fid, -1, -1));
        data[MxFunctions::PickDataType::GeoFace] = facePicks;
        mixSet->setMixSetData(data);
        modelData->addPhysicalField<MechSetData::mSetBase>(mixSet);

        setArea = S;
        return setId;
    }

    bool mMotionLoadCaseGenerator::isPlanarFace(int geoFaceId)
    {
        const EventDispatcher::mGeoObjectRef faceRef(GeoNeutralData::GN_ShapeType::Face, geoFaceId);
        return getGEOServer.surfaceType(faceRef) == EventDispatcher::mGeoSurfaceType::Plane;
    }
}
