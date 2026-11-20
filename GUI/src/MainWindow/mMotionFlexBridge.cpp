#include "mMotionFlexBridge.h"

//【刚柔耦合】AutoFlex 柔性体桥接实现（数据收集 + 引擎构建）
#include "mModelData.h"
#include "mMeshData.h"
#include "mElementFunction.h"
#include "mPhysicalFieldEnum.h"

#include "mMotionAutoFlexManager.h"
#include "mMotionAutoFlex.h"
#include "mMotionJointManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
#include "mMotionBodyManager.h"
#include "mMotionBody.h"

#include "MeshEntity.h"
#include "MXGeoSolid.h"
#include "MXMeshTetrahedron.h"
#include "MXMeshVertex.h"
#include "MXMesh.h"
#include "mViewportModelManager.h"

#include "motion/modal/FlexBodyBuilder.h"
#include "motion/physics/System.h"
#include "motion/physics/Body.h"
#include "motion/physics/Marker.h"
#include "motion/physics/LinkMarkers.h"
#include "motion/fea/Mesh.h"
#include "motion/fea/NodeFEAxyz.h"
#include "mMotionResultData.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/mFlexBodyRender.h"
#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <set>
#include <fstream>
#include <cmath>
#include <iostream>

#include <map>
#include <set>
#include <fstream>
#include <cmath>
#include <iostream>

using namespace MxFunctions;

namespace mMotionFlexBridge {

// ---------- 材料取值：遍历所有 MaterialClass 找 E/ν/ρ（属性可能挂在任一 class 下） ----------
static bool getMaterialProps(MechModelData::mModelData* modelData, int materialId, double& E, double& nu, double& rho,
                             const std::function<void(const QString&)>& log)
{
    auto& matMgr = modelData->getManager<MechMaterialData::mMaterialManager>(Physical_Material);
    auto mat = matMgr.getMaterialData(materialId);
    if (!mat)
        return false;

    bool hasE = false, hasNu = false, hasRho = false;
    auto all = mat->getAllProps();
    for (auto itClass = all.constBegin(); itClass != all.constEnd(); ++itClass) {
        const auto& props = itClass.value();
        if (props.contains(MatProp_YoungsModulus) && !hasE) {
            const QVariant v = props.value(MatProp_YoungsModulus);
            if (v.isValid() && v.canConvert<double>()) { E = v.toDouble(); hasE = true; }
        }
        if (props.contains(MatProp_Botson) && !hasNu) {
            const QVariant v = props.value(MatProp_Botson);
            if (v.isValid() && v.canConvert<double>()) { nu = v.toDouble(); hasNu = true; }
        }
        if (props.contains(MatProp_Density) && !hasRho) {
            const QVariant v = props.value(MatProp_Density);
            if (v.isValid() && v.canConvert<double>()) { rho = v.toDouble(); hasRho = true; }
        }
    }
    if (!hasE || !hasNu || !hasRho) {
        log(QString("  [柔性体] 材料 %1 缺少弹性模量/泊松比/密度（E=%2 nu=%3 rho=%4），无法生成柔体")
                .arg(materialId).arg(hasE).arg(hasNu).arg(hasRho));
        return false;
    }
    return true;
}

// ---------- 网格收集：按 partIds 过滤 MeshEntity 的体网格 solid，抽节点/四面体 ----------
static bool collectPartTetraMesh(MechModelData::mModelData* modelData, const std::set<int>& partIds,
                                 std::vector<double>& coords, std::vector<int>& tets,
                                 std::map<int, int>& globalToLocal,
                                 const std::function<void(const QString&)>& log)
{
    // 读取路径：mMeshData 全局单元表（剖分产出经 ensureMesh 固化于此；
    // MXGeoSolid::_mTetrahedrons 对引擎化剖分为空——实测确认）。
    // TODO 多部件：按 part 过滤单元（当前收集全部 MeshTet，单柔体部件场景正确）。
    auto meshData = modelData->getMeshData();
    if (!meshData)
        return false;
    // 单元类型/连接查询在 mPartData 上（mMeshData::getPartData(partID)，头文件 :47/:988）
    mPartData* partData = partIds.empty() ? nullptr : meshData->getPartData((uint64_t)*partIds.begin());
    if (!partData) {
        log(QString::fromUtf8("  [柔性体] 未找到部件数据（partId=%1）").arg(partIds.empty() ? -1 : *partIds.begin()));
        return false;
    }
    auto elems = meshData->getAllElements();
    int nTet = 0;
    for (const auto& e : elems) {
        const int eid = e.result_id;
        if (partData->getMeshTypeByElement(eid) != MxFunctions::MeshType::MeshTet)
            continue;
        auto ns = partData->getNodesByElementId(eid);
        if (ns.size() != 4)
            continue;
        int local[4] = { -1, -1, -1, -1 };
        bool ok = true;
        for (int k = 0; k < 4; ++k) {
            auto found = globalToLocal.find(ns[k]);
            if (found == globalToLocal.end()) {
                const QVector3D p = meshData->getVectorById(ns[k]);
                if (std::isnan(p.x()) && std::isnan(p.y()))
                    { ok = false; break; }
                const int idx = (int)(coords.size() / 3);
                coords.push_back(p.x()); coords.push_back(p.y()); coords.push_back(p.z());
                globalToLocal[ns[k]] = idx;
                local[k] = idx;
            } else {
                local[k] = found->second;
            }
        }
        if (!ok)
            continue;
        tets.push_back(local[0]); tets.push_back(local[1]);
        tets.push_back(local[2]); tets.push_back(local[3]);
        ++nTet;
    }
    if (tets.empty()) {
        log(QString("  [柔性体] 部件 %1 未读取到四面体（单元表 tet=0，总单元 %2）")
                .arg(partIds.empty() ? -1 : *partIds.begin()).arg((int)elems.size()));
        return false;
    }

    //【旧网格碎片自愈】elementidmap 跨多次剖分累积不清旧单元：重剖后新旧 tet 混收，
    // 旧碎片（不同节点号）与新网格不连通 → CB 缩减 K_IIc 结构性奇异（悬浮刚体模式，
    // 实测秩差 90）。按共享节点的单元连通性取最大分量——即当前网格，旧碎片剔除
    {
        const int nT = (int)tets.size() / 4;
        std::vector<int> comp(nT, -1);
        std::vector<std::vector<int>> nodeTets((int)coords.size() / 3);
        for (int t = 0; t < nT; ++t)
            for (int k = 0; k < 4; ++k)
                nodeTets[tets[4 * t + k]].push_back(t);
        int bestComp = -1, bestSize = 0, nComp = 0;
        for (int t = 0; t < nT; ++t) {
            if (comp[t] != -1)
                continue;
            std::vector<int> stack{ t };
            comp[t] = nComp;
            int size = 0;
            while (!stack.empty()) {
                const int cur = stack.back();
                stack.pop_back();
                ++size;
                for (int k = 0; k < 4; ++k)
                    for (int nb : nodeTets[tets[4 * cur + k]])
                        if (comp[nb] == -1) { comp[nb] = nComp; stack.push_back(nb); }
            }
            if (size > bestSize) { bestSize = size; bestComp = nComp; }
            ++nComp;
        }
        if (nComp > 1) {
            std::vector<int> kept;
            kept.reserve(bestSize * 4);
            for (int t = 0; t < nT; ++t)
                if (comp[t] == bestComp)
                    kept.insert(kept.end(), tets.begin() + 4 * t, tets.begin() + 4 * t + 4);
            log(QString("  [柔性体] 检测到 %1 个不连通网格片段（历史剖分残留）：保留当前网格 %2 单元，剔除 %3 个残留单元")
                    .arg(nComp).arg(bestSize).arg(nT - bestSize));
            tets = std::move(kept);
        }
    }
    return true;

}

bool ensureTetraMesh(MechModelData::mModelData* modelData, const std::set<int>& partIds,
                     double elementSize, const std::function<void(const QString&)>& log)
{
    if (!modelData)
        return false;
    auto meshData = modelData->getMeshData();
    if (!meshData || !meshData->getMeshEntity())
        return false;

    // 已有体网格？
    {
        std::vector<double> coords; std::vector<int> tets; std::map<int, int> g2l;
        std::function<void(const QString&)> noop = [](const QString&) {};
        if (collectPartTetraMesh(modelData, partIds, coords, tets, g2l, noop))
            return true;
    }

    // 自动剖分：收集部件的 3D 几何 ID（多体形态无网格模块入口，AutoFlex 自行网格化）
    std::set<int> solidGeoIds;
    for (int partId : partIds) {
        std::vector<int> ids3D = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 3);
        for (int g : ids3D)
            solidGeoIds.insert(g);
    }
    if (solidGeoIds.empty()) {
        log(QString("  [柔性体] 部件 %1 无 3D 几何，无法自动生成体网格")
                .arg(partIds.empty() ? -1 : *partIds.begin()));
        return false;
    }

    MeshEntity* entity = meshData->getMeshEntity();
    const double lc = elementSize > 0 ? elementSize : MAX_LC;
    log(QString("  [柔性体] 自动生成体网格（目标尺寸 %1，体 %2 个）...").arg(elementSize).arg(solidGeoIds.size()));
    entity->resetPreMeshSize(3, lc);
    entity->setMeshPattern(MeshEntity::MeshPattern::AUTOMATED_MESHING);
    {
        auto mesh = std::make_shared<MXMesh>();
        mesh->setMeshData(meshData);
        mesh->MXMeshInterface(3, entity, solidGeoIds, {});
        meshData->ensureMesh();
    }
    // 复验
    std::vector<double> coords; std::vector<int> tets; std::map<int, int> g2l;
    if (!collectPartTetraMesh(modelData, partIds, coords, tets, g2l, log)) {
        log("  [柔性体] 自动体网格生成失败（无四面体产出）——见上方诊断");
        return false;
    }
    log(QString("  [柔性体] 体网格生成完成：节点 %1 / 四面体 %2")
            .arg(coords.size() / 3).arg(tets.size() / 4));
    return true;
}

std::vector<double> collectJointAttachPointsGui(MechModelData::mModelData* modelData, int bodyId)
{
    std::vector<double> pts;
    if (!modelData || bodyId < 0)
        return pts;
    auto& jointMgr = modelData->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
    auto& relMgr = modelData->getMapRelationManager();
    // 各副类型 origin 的取值链（与 mOMotionJsonFile 的关节导出同款 dynamic_cast 链）
    auto originOf = [](MechMotionData::mMotionJointBase* j) -> QVector3D {
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointBasic*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointPointOnSurface*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointUniversal*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointScrew*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointPerpendicular*>(j)) return v->getOrigin();
        if (auto* v = dynamic_cast<MechMotionData::mMotionJointConstantVelocity*>(j)) return v->getOrigin();
        return QVector3D();
    };
    for (int jid : jointMgr.getAllIDs()) {
        auto* joint = jointMgr.getData(jid);
        if (!joint)
            continue;
        const int actionId = relMgr.getMapRelation<int>({ Physical_MotionJoint, jid }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        const int baseId = relMgr.getMapRelation<int>({ Physical_MotionJoint, jid }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);
        if (actionId != bodyId && baseId != bodyId)
            continue;
        const QVector3D o = originOf(joint);
        bool dup = false;
        for (size_t k = 0; k + 2 < pts.size(); k += 3)
            if (std::abs(pts[k] - o.x()) < 1e-6 && std::abs(pts[k + 1] - o.y()) < 1e-6 && std::abs(pts[k + 2] - o.z()) < 1e-6) { dup = true; break; }
        if (dup)
            continue;
        pts.push_back(o.x());
        pts.push_back(o.y());
        pts.push_back(o.z());
    }
    return pts;
}

bool collectBodyTetraMesh(MechModelData::mModelData* modelData, const std::set<int>& partIds,
                          double elementSize,
                          std::vector<double>& coords, std::vector<int>& tets,
                          std::map<int, int>& globalToLocal,
                          const std::function<void(const QString&)>& log)
{
    if (collectPartTetraMesh(modelData, partIds, coords, tets, globalToLocal, log))
        return true;
    if (!ensureTetraMesh(modelData, partIds, elementSize, log))
        return false;
    return collectPartTetraMesh(modelData, partIds, coords, tets, globalToLocal, log);
}

std::vector<FlexInput> collectFlexInputs(motion::MotionSystem* sys, MechModelData::mModelData* modelData,
                                         const std::function<void(const QString&)>& log)
{
    std::vector<FlexInput> result;
    if (!modelData)
        return result;

    auto& flexMgr = modelData->getManager<MechMotionData::mMotionAutoFlexManager>(Physical_MotionAutoFlex);
    auto& bodyMgr = modelData->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
    auto& relMgr = modelData->getMapRelationManager();

    for (int flexId : flexMgr.getAllIDs()) {
        auto* flex = flexMgr.getData(flexId);
        if (!flex || !flex->isEnabled())
            continue;

        FlexInput in;
        in.flexId = flexId;
        in.name = flex->getName();
        in.modeCount = flex->getModeCount();
        in.connectionRadius = flex->getConnectionRadius();
        //【面板直通】阻尼/截断频率（引擎 FlexBodyBuilder 同名参数）
        in.dampingType = static_cast<int>(flex->getDampingType());
        in.modalDampingRatio = flex->getModalDampingRatio();
        in.rayleighAlpha = flex->getRayleighMass();
        in.rayleighBeta = flex->getRayleighStiffness();
        in.freqCutoff = flex->getFreqCutoff();

        // 目标刚体（一对一无 role 映射）
        in.bodyId = relMgr.getMapRelation<int>({ Physical_MotionAutoFlex, flexId }, Physical_MotionBody, MapRelation_None);
        if (in.bodyId < 0) {
            in.error = "未设置目标刚体";
            result.push_back(in);
            continue;
        }
        if (auto* body = bodyMgr.getData(in.bodyId))
            in.bodyFixed = body->getIsFixed();

        // 材料：取值失败/无材料 → 内置钢回退（多体形态无材料模块，与质量补算同口径）
        if (!getMaterialProps(modelData, flex->getMaterialId(), in.E, in.nu, in.rho,
                              [](const QString&) {})) {
            in.E = 2.1e5; in.nu = 0.3; in.rho = 7.85e-6;  // MPa / - / kg/mm³
            log(QString("  [柔性体] %1：材料 %2 缺有效属性，使用内置钢（E=2.1e5 MPa, nu=0.3, rho=7.85e-6 kg/mm³）")
                    .arg(flex->getName()).arg(flex->getMaterialId()));
        }
        //【单位制】引擎 mm 模型自洽制为 (mm, tonne, s)：密度 kg/mm³ → tonne/mm³（÷1000）。
        // 不换算则特征频率低 √1000≈31.6 倍（实测 12x12x200 钢悬臂梁 250.7 Hz → 39.9 Hz）
        in.rho /= 1000.0;

        // 几何：body → part 两跳
        std::set<int> partIds = relMgr.getMapRelation<std::set<int>>({ Physical_MotionBody, in.bodyId }, Physical_Part, MapRelation_None);
        std::map<int, int> globalToLocal;
        if (!collectPartTetraMesh(modelData, partIds, in.nodeCoords, in.tetIds, globalToLocal, log)) {
            // 多体形态无网格模块：AutoFlex 自动网格化（elementSize 来自面板参数）
            if (!ensureTetraMesh(modelData, partIds, flex->getElementSize(), log)) {
                in.error = "体网格不可用且自动生成失败";
                result.push_back(in);
                continue;
            }
            if (!collectPartTetraMesh(modelData, partIds, in.nodeCoords, in.tetIds, globalToLocal, log)) {
                in.error = "体网格读取失败";
                result.push_back(in);
                continue;
            }
        }
        // 全局节点 ID 表（与 nodeCoords 同序）：mx 导出按界面拾取的模型网格 ID 写出
        in.nodeGlobalIds.resize(in.nodeCoords.size() / 3);
        for (const auto& kv : globalToLocal)
            if (kv.second >= 0 && kv.second < (int)in.nodeGlobalIds.size())
                in.nodeGlobalIds[kv.second] = kv.first;

        // 附着点：attachPoints 是网格节点 ID；不在网格里则按坐标找最近节点；
        // 半径>0 时围绕各点球域扩展。全部选中点合并成集合后按体网格面连通分量
        // 分组（同一端面选多点归一组接口；不同端面自然分开）
        const int nNodes = (int)(in.nodeCoords.size() / 3);
        if (!flex->getAttachPoints().isEmpty()) {
            std::set<int> selected;
            for (int ptId : flex->getAttachPoints()) {
                int local = -1;
                auto found = globalToLocal.find(ptId);
                if (found != globalToLocal.end()) {
                    local = found->second;
                } else {
                    const QVector3D p = modelData->getMeshData()->getVectorById(ptId);
                    double bestD2 = 1e300;
                    for (int i = 0; i < nNodes; ++i) {
                        const double dx = in.nodeCoords[3 * i] - p.x();
                        const double dy = in.nodeCoords[3 * i + 1] - p.y();
                        const double dz = in.nodeCoords[3 * i + 2] - p.z();
                        const double d2 = dx * dx + dy * dy + dz * dz;
                        if (d2 < bestD2) { bestD2 = d2; local = i; }
                    }
                    if (local >= 0)
                        log(QString("  [柔性体] %1：附着点 %2 不在网格节点中，已吸附最近节点（距离²=%3）")
                                .arg(flex->getName()).arg(ptId).arg(bestD2, 0, 'g', 4));
                }
                if (local >= 0)
                    selected.insert(local);
            }
            // 半径>0：围绕每个选中节点收球内节点并入集合
            if (in.connectionRadius > 0) {
                std::set<int> expanded = selected;
                for (int c : selected) {
                    const double cx = in.nodeCoords[3 * c], cy = in.nodeCoords[3 * c + 1], cz = in.nodeCoords[3 * c + 2];
                    for (int i = 0; i < nNodes; ++i) {
                        const double dx = in.nodeCoords[3 * i] - cx, dy = in.nodeCoords[3 * i + 1] - cy, dz = in.nodeCoords[3 * i + 2] - cz;
                        if (dx * dx + dy * dy + dz * dz <= in.connectionRadius * in.connectionRadius)
                            expanded.insert(i);
                    }
                }
                selected = std::move(expanded);
            }
            // 体网格面连通分量分组（共享四面体面且都在集合内的节点连通）
            if (!selected.empty()) {
                std::map<std::array<int, 3>, int> faceCount;
                for (size_t t = 0; t + 3 < in.tetIds.size(); t += 4) {
                    const int a = in.tetIds[t], b = in.tetIds[t + 1], c = in.tetIds[t + 2], d = in.tetIds[t + 3];
                    for (auto f : { std::array<int, 3>{a, b, c}, {a, b, d}, {a, c, d}, {b, c, d} }) {
                        std::sort(f.begin(), f.end());
                        ++faceCount[f];
                    }
                }
                std::map<int, std::set<int>> adjacent;
                for (const auto& kv : faceCount)
                    for (int a : kv.first)
                        for (int b : kv.first)
                            if (a != b && selected.count(a) && selected.count(b))
                                adjacent[a].insert(b);
                while (!selected.empty()) {
                    std::vector<int> group;
                    std::set<int> pending{ *selected.begin() };
                    selected.erase(selected.begin());
                    while (!pending.empty()) {
                        const int v = *pending.begin();
                        pending.erase(pending.begin());
                        group.push_back(v);
                        for (int nb : adjacent[v])
                            if (selected.erase(nb))
                                pending.insert(nb);
                    }
                    std::sort(group.begin(), group.end());
                    in.attachGroups.push_back(std::move(group));
                }
            }
        }

        //【自动附着】运动副作用点 → 容差内节点组（UG 语义：连接位置由副声明，用户无需选择）。
        //手动附着组已建时跳过：两组会叠加成同一批节点的重复焊接（实测 16 节点被焊成
        // 96 约束行 = 2×48，冗余约束使 K_IIc 鞍形阵奇异、秩差 90）——"保存后求解崩溃"
        // 的根因；手选优先，自动仅作留空兜底
        if (sys && in.attachGroups.empty()) {
            auto rigidBody = sys->SearchBody(std::to_string(in.bodyId).c_str());
            if (rigidBody) {
                std::vector<std::pair<double, double>> pts;  // 去重用
                for (auto& link : sys->Get_linklist()) {
                    auto refLink = std::dynamic_pointer_cast<motion::MotionLinkMarkers>(link);
                    if (!refLink)
                        continue;
                    motion::MotionMarker* mks[2] = { refLink->GetMarker1(), refLink->GetMarker2() };
                    for (auto* mk : mks) {
                        if (!mk || mk->GetBody() != rigidBody.get())
                            continue;
                        const auto& p = mk->GetAbsCoord().pos;
                        bool dup = false;
                        for (auto& q : pts)
                            if (std::abs(q.first - p.x()) < 1e-6 && std::abs(q.second - p.y()) < 1e-6) { dup = true; break; }
                        if (dup) continue;
                        pts.push_back({ p.x(), p.y() });
                        in.autoAttachPositions.push_back(p.x());
                        in.autoAttachPositions.push_back(p.y());
                        in.autoAttachPositions.push_back(p.z());
                    }
                }
            }
        }

        in.valid = true;
        result.push_back(in);
    }
    return result;
}

FlexOutput buildOne(motion::MotionSystem& sys, const FlexInput& in)
{
    motion::modal::MotionFlexBodyParams P;
    P.node_coords = in.nodeCoords;
    P.attach_groups = in.attachGroups;

    // 自动附着：每个运动副作用点收容差内节点成组（容差=连接半径>0，否则自动=3×平均单元边长）
    if (!in.autoAttachPositions.empty()) {
        double tol = in.connectionRadius;
        if (tol <= 0) {
            // 平均单元边长（采样前 200 个四面体）
            double sum = 0; int cnt = 0;
            const auto& c = in.nodeCoords;
            const int nT = (int)in.tetIds.size() / 4;
            for (int t = 0; t < nT && t < 200; ++t) {
                const int n0 = in.tetIds[4 * t];
                for (int k = 1; k < 4; ++k) {
                    const int nk = in.tetIds[4 * t + k];
                    const double dx = c[3 * n0] - c[3 * nk], dy = c[3 * n0 + 1] - c[3 * nk + 1], dz = c[3 * n0 + 2] - c[3 * nk + 2];
                    sum += std::sqrt(dx * dx + dy * dy + dz * dz);
                    ++cnt;
                }
            }
            tol = cnt > 0 ? 3.0 * sum / cnt : 0.0;
        }
        const int nNodes = (int)(in.nodeCoords.size() / 3);
        for (size_t a = 0; a + 2 < in.autoAttachPositions.size(); a += 3) {
            std::vector<int> group;
            const double ax = in.autoAttachPositions[a], ay = in.autoAttachPositions[a + 1], az = in.autoAttachPositions[a + 2];
            for (int i = 0; i < nNodes; ++i) {
                const double dx = in.nodeCoords[3 * i] - ax, dy = in.nodeCoords[3 * i + 1] - ay, dz = in.nodeCoords[3 * i + 2] - az;
                if (dx * dx + dy * dy + dz * dz <= tol * tol)
                    group.push_back(i);
            }
            //【护栏】组罩住大半网格 = 结构被整体焊死（模态空间坍缩）：
            // 按距作用点排序，只取最近 min(32, nNodes/2) 个
            if ((int)group.size() > 16) {  // CB 夹持基修正后不再病态；仍限规模防过约束
                std::sort(group.begin(), group.end(), [&](int lhs, int rhs) {
                    const double dl = (in.nodeCoords[3*lhs]-ax)*(in.nodeCoords[3*lhs]-ax) + (in.nodeCoords[3*lhs+1]-ay)*(in.nodeCoords[3*lhs+1]-ay) + (in.nodeCoords[3*lhs+2]-az)*(in.nodeCoords[3*lhs+2]-az);
                    const double dr = (in.nodeCoords[3*rhs]-ax)*(in.nodeCoords[3*rhs]-ax) + (in.nodeCoords[3*rhs+1]-ay)*(in.nodeCoords[3*rhs+1]-ay) + (in.nodeCoords[3*rhs+2]-az)*(in.nodeCoords[3*rhs+2]-az);
                    return dl < dr;
                });
                group.resize(16);
            }
            if (!group.empty())
                P.attach_groups.push_back(std::move(group));
        }
    }

    P.tet_node_ids = in.tetIds;
    P.young_modulus = in.E;
    P.poisson_ratio = in.nu;
    P.density = in.rho;
    P.n_modes = in.modeCount > 0 ? in.modeCount : 6;
    //【面板直通】阻尼/截断频率
    P.damping_type = in.dampingType;
    P.modal_damping_ratio = in.modalDampingRatio;
    P.rayleigh_alpha = in.rayleighAlpha;
    P.rayleigh_beta = in.rayleighBeta;
    P.freq_cutoff_hz = in.freqCutoff;

    //【推荐模式】目标刚体本身作为接口体：副保持在原刚体上不失效（不改挂/不删除），
    // 每个附着组都焊到这同一个刚体上（external_interfaces 同组共享一个体）
    if (in.bodyId >= 0) {
        auto target = sys.SearchBody(std::to_string(in.bodyId).c_str());
        if (target) {
            //【参考系修正·全路径】parser 把体参考系放在 COM（JSON centerOfMass），距铰点
            // 可达 ~L/2。CB 缩减边界 DOF 跟参考系走——在 COM 时焊接约束带巨大力臂，
            // KKT 鞍形阵数值奇异（实测稀疏LU NumericalIssue、秩亏90）；即使分解成功，
            // 模态坐标也混入力臂记账（弹性变形虚增 160 倍）。修正：把参考系移到首个
            // 附着组质心（自动路径=首副点；手动路径=首组节点质心），再重设该体全部
            // marker 的绝对坐标（副约束方程在原位置闭合）
            bool moved = false;
            double ax = 0, ay = 0, az = 0;
            if (!in.autoAttachPositions.empty()) {
                ax = in.autoAttachPositions[0];
                ay = in.autoAttachPositions[1];
                az = in.autoAttachPositions[2];
                moved = true;
            }
            else if (!P.attach_groups.empty() && !P.attach_groups.front().empty()) {
                const auto& g = P.attach_groups.front();
                for (int local : g) {
                    ax += in.nodeCoords[3 * local];
                    ay += in.nodeCoords[3 * local + 1];
                    az += in.nodeCoords[3 * local + 2];
                }
                const double inv = 1.0 / g.size();
                ax *= inv; ay *= inv; az *= inv;
                moved = true;
            }
            if (moved) {
                const auto oldRot = target->GetRot();
                // 保存所有 marker 的绝对坐标
                std::vector<std::pair<motion::MotionMarker*, motion::MotionCoordsys<>>> savedMarkers;
                for (auto& mk : target->GetMarkerList()) {
                    if (mk)
                        savedMarkers.emplace_back(mk.get(), mk->GetAbsCoord());
                }
                // 移参考系（姿态不变）
                target->SetPos(motion::MotionVector<double>(ax, ay, az));
                target->SetRot(oldRot);
                // 重设 marker 绝对坐标（副约束方程在原位置闭合）
                for (auto& kv : savedMarkers)
                    kv.first->Impose_Abs_Coord(kv.second);
            }
            for (size_t gi = 0; gi < P.attach_groups.size(); ++gi)
                P.external_interfaces.push_back(target);
        }
    }

    motion::modal::MotionFlexBodyResult R = motion::modal::BuildFlexBody(sys, P);

    FlexOutput out;
    out.nodes = R.n_mesh_nodes;
    out.tets = R.n_mesh_elements;
    out.modes = R.n_modes;
    out.interfaces = (int)R.interface_bodies.size();
    out.interfaceBodies = R.interface_bodies;
    out.assembly = R.assembly;
    out.nodeX0 = std::move(R.node_x0);
    out.surfaceTris = std::move(R.surface_tris);
    out.psiInternal = std::move(R.psi_internal);
    return out;
}

bool previewFlexBodyMesh(MechModelData::mModelData* modelData, int bodyId, const QString& message)
{
    // 渲染入口：当前视口的前处理渲染器
    if (!getCVM || !getCVM->getPreRenderWidget())
        return false;
    mxr::mFlexBodyRender* rend = getCVM->getPreRenderWidget()->getPreRender()->getFlexBodyRender();
    if (!rend)
        return false;
    if (!modelData || bodyId < 0)
        return false;

    auto& relMgr = modelData->getMapRelationManager();
    std::set<int> partIds = relMgr.getMapRelation<std::set<int>>({ Physical_MotionBody, bodyId }, Physical_Part, MapRelation_None);

    std::vector<double> coords;
    std::vector<int> tets;
    std::map<int, int> g2l;
    std::function<void(const QString&)> noop = [](const QString&) {};
    if (!collectPartTetraMesh(modelData, partIds, coords, tets, g2l, noop)) {
        rend->deleteFlexBody(message);  // 无网格：清旧预览
        return false;
    }

    // 表面三角
    std::map<std::array<int, 3>, int> faceCount;
    auto addFace = [&faceCount](int a, int b, int c) {
        std::array<int, 3> f{ a, b, c };
        std::sort(f.begin(), f.end());
        ++faceCount[f];
    };
    for (size_t t = 0; t * 4 + 3 < tets.size(); ++t) {
        addFace(tets[4 * t], tets[4 * t + 1], tets[4 * t + 2]);
        addFace(tets[4 * t], tets[4 * t + 1], tets[4 * t + 3]);
        addFace(tets[4 * t], tets[4 * t + 2], tets[4 * t + 3]);
        addFace(tets[4 * t + 1], tets[4 * t + 2], tets[4 * t + 3]);
    }
    const int nNodes = (int)(coords.size() / 3);
    QVector<QVector3D> nodes;
    nodes.reserve(nNodes);
    for (int i = 0; i < nNodes; ++i)
        nodes.push_back(QVector3D(coords[3 * i], coords[3 * i + 1], coords[3 * i + 2]));
    QVector<uint32_t> tris;
    for (const auto& kv : faceCount)
        if (kv.second == 1) {
            tris.push_back((uint32_t)kv.first[0]);
            tris.push_back((uint32_t)kv.first[1]);
            tris.push_back((uint32_t)kv.first[2]);
        }
    // 半透明青色示意（区别于回放柔体的蓝色）
    rend->appendFlexBody(message, nodes, tris, QVector4D(0.3f, 0.85f, 0.8f, 0.55f));
    return true;
}

void clearFlexBodyPreview()
{
    if (!getCVM || !getCVM->getPreRenderWidget())
        return;
    if (auto* rend = getCVM->getPreRenderWidget()->getPreRender()->getFlexBodyRender())
        rend->deleteFlexBody(QStringLiteral("__flex_preview__"));
}

QVector<double> nxContourValues(const QVector<double>& x0Flat, const QVector<QVector3D>& xt,
                                const QVector<double>& weights, const QQuaternion* knownRot)
{
    const int N = xt.size();
    if (N < 3 || x0Flat.size() != 3 * N)
        return QVector<double>(N > 0 ? N : 0, 0.0);

    // 质量加权（集总体积权）质心
    Eigen::Vector3d c0 = Eigen::Vector3d::Zero(), cT = Eigen::Vector3d::Zero();
    double wSum = 0.0;
    auto wOf = [&](int i) { return (i < weights.size() && weights[i] > 0.0) ? weights[i] : 1.0; };
    for (int i = 0; i < N; ++i) {
        const double w = wOf(i);
        c0 += w * Eigen::Vector3d(x0Flat[3 * i], x0Flat[3 * i + 1], x0Flat[3 * i + 2]);
        cT += w * Eigen::Vector3d(xt[i].x(), xt[i].y(), xt[i].z());
        wSum += w;
    }
    if (wSum <= 0.0)
        return QVector<double>(N, 0.0);
    c0 /= wSum;
    cT /= wSum;

    // 旋转：优先用已知平滑旋转（接口体姿态），避免 SVD 帧间跳变；
    // 无已知旋转时回退到 SVD 拟合（Umeyama）
    Eigen::Matrix3d R;
    if (knownRot) {
        const QMatrix3x3 qm = knownRot->toRotationMatrix();
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                R(r, c) = qm(r, c);
    } else {
        Eigen::Matrix3d H = Eigen::Matrix3d::Zero();
        for (int i = 0; i < N; ++i) {
            const double w = wOf(i);
            const Eigen::Vector3d a(x0Flat[3 * i] - c0[0], x0Flat[3 * i + 1] - c0[1], x0Flat[3 * i + 2] - c0[2]);
            const Eigen::Vector3d b(xt[i].x() - cT[0], xt[i].y() - cT[1], xt[i].z() - cT[2]);
            H += w * (a * b.transpose());
        }
        Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
        R = svd.matrixV() * svd.matrixU().transpose();
        if (R.determinant() < 0.0) {
            Eigen::Matrix3d V = svd.matrixV();
            V.col(2) *= -1.0;
            R = V * svd.matrixU().transpose();
        }
    }
    // 最优平移（给定旋转下解析解，帧间平滑）
    const Eigen::Vector3d t = cT - R * c0;

    QVector<double> out(N);
    for (int i = 0; i < N; ++i) {
        const Eigen::Vector3d x0(x0Flat[3 * i], x0Flat[3 * i + 1], x0Flat[3 * i + 2]);
        const Eigen::Vector3d d = R * x0 + t - Eigen::Vector3d(xt[i].x(), xt[i].y(), xt[i].z());
        out[i] = d.norm();
    }
    return out;
}

//【UG 口径·自由基】从网格几何提取自由基前 3 阶弯曲模态（解析 Euler-Bernoulli 形状）
FreeModeData computeFreeMode(const QVector<double>& x0Flat, const QVector<int>& tetIds, double density)
{
    FreeModeData fm;
    const int N = x0Flat.size() / 3;
    if (N < 4 || tetIds.size() < 4)
        return fm;
    const int nT = tetIds.size() / 4;

    // 1) 节点集总质量
    fm.nodeMasses.fill(0.0, N);
    for (int t = 0; t < nT; ++t) {
        const int ia = tetIds[4*t], ib = tetIds[4*t+1], ic = tetIds[4*t+2], id = tetIds[4*t+3];
        if (ia<0||ia>=N||ib<0||ib>=N||ic<0||ic>=N||id<0||id>=N) continue;
        Eigen::Vector3d a(x0Flat[3*ia],x0Flat[3*ia+1],x0Flat[3*ia+2]);
        Eigen::Vector3d b(x0Flat[3*ib],x0Flat[3*ib+1],x0Flat[3*ib+2]);
        Eigen::Vector3d c(x0Flat[3*ic],x0Flat[3*ic+1],x0Flat[3*ic+2]);
        Eigen::Vector3d d(x0Flat[3*id],x0Flat[3*id+1],x0Flat[3*id+2]);
        const double vol = std::abs((b-a).dot((c-a).cross(d-a))) / 6.0;
        const double q = density * vol / 4.0;
        fm.nodeMasses[ia]+=q; fm.nodeMasses[ib]+=q; fm.nodeMasses[ic]+=q; fm.nodeMasses[id]+=q;
    }

    // 2) 主轴（梁方向）+ 次轴（弯曲方向）
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    double totalM = 0;
    for (int i = 0; i < N; ++i) {
        centroid += Eigen::Vector3d(x0Flat[3*i],x0Flat[3*i+1],x0Flat[3*i+2]) * fm.nodeMasses[i];
        totalM += fm.nodeMasses[i];
    }
    if (totalM <= 0) return fm;
    centroid /= totalM;

    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (int i = 0; i < N; ++i) {
        Eigen::Vector3d d(x0Flat[3*i]-centroid[0],x0Flat[3*i+1]-centroid[1],x0Flat[3*i+2]-centroid[2]);
        cov += fm.nodeMasses[i] * (d * d.transpose());
    }
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es3(cov);
    Eigen::Vector3d axis = es3.eigenvectors().col(2).normalized();    // 主轴 = 梁方向
    // 弯曲方向不需要：标量模态与方向无关

    // 3) 参数化 ξ ∈ [0,1]（从梁一端到另一端）
    double sMin = 1e30, sMax = -1e30;
    for (int i = 0; i < N; ++i) {
        Eigen::Vector3d d(x0Flat[3*i]-centroid[0],x0Flat[3*i+1]-centroid[1],x0Flat[3*i+2]-centroid[2]);
        const double sd = d.dot(axis);
        sMin = std::min(sMin, sd); sMax = std::max(sMax, sd);
    }
    const double L = std::max(sMax - sMin, 1e-10);

    // 4) 自由-自由梁前 3 阶弯曲模态形状（Euler-Bernoulli 解析公式）
    //    φₙ(ξ) = cosh(βₙξ) + cos(βₙξ) - σₙ[sinh(βₙξ) + sin(βₙξ)]
    //    β₁=4.730 (2 节点), β₂=7.853 (4 节点), β₃=10.996 (6 节点)
    //    归一化到 |φₙ(0)| = 1（两端值）
    

    // 余弦模态（数值无条件稳定，cosh(11)≈30000 有精度风险）：
    // 第 n 阶自由-自由弯曲模态 ≈ cos((n+1)·π·ξ)（两端非零、内部节点）
    // 第 1 阶 cos(2πξ)：两端=1、25% 和 75% 处=0（双蓝块）
    // 第 2 阶 cos(4πξ)：4 节点（随帧演化修正）
    // 第 3 阶 cos(6πξ)：6 节点（细节演化）
    const int modeOrders[FreeModeData::kNumModes] = {2, 4, 6};  // cos(nπξ) 的 n
    for (int k = 0; k < FreeModeData::kNumModes; ++k) {
        fm.modeShapes[k].resize(N);
        fm.modeMass[k] = 0;
        for (int i = 0; i < N; ++i) {
            Eigen::Vector3d d(x0Flat[3*i]-centroid[0],x0Flat[3*i+1]-centroid[1],x0Flat[3*i+2]-centroid[2]);
            const double sd = d.dot(axis);
            const double xi = (sd - sMin) / L;
            const double phi = std::cos(modeOrders[k] * 3.14159265358979323846 * xi);
            fm.modeShapes[k][i] = phi;
            fm.modeMass[k] += fm.nodeMasses[i] * phi * phi;
        }
    }

    return fm;
}

//【UG 口径·云图】把当前弹性变形投影到各阶自由基模态，取叠加贡献量（随帧演化）
QVector<double> freeModeContour(const FreeModeData& fm, const QVector<double>& elasticFlat)
{
    const int N = fm.nodeMasses.size();
    if (N < 3 || elasticFlat.size() != 3 * N)
        return QVector<double>(N, 0.0);

    // 各阶投影系数：用弹性变形模长（方向无关）投影到标量模态
    double q[FreeModeData::kNumModes] = {0, 0, 0};
    for (int k = 0; k < FreeModeData::kNumModes; ++k) {
        if (fm.modeMass[k] <= 0 || fm.modeShapes[k].size() != N) continue;
        double num = 0;
        for (int i = 0; i < N; ++i) {
            const double emag = std::sqrt(elasticFlat[3*i]*elasticFlat[3*i]
                                        + elasticFlat[3*i+1]*elasticFlat[3*i+1]
                                        + elasticFlat[3*i+2]*elasticFlat[3*i+2]);
            num += fm.nodeMasses[i] * fm.modeShapes[k][i] * emag;
        }
        q[k] = num / fm.modeMass[k];
    }

    // 叠加贡献量：Σ|φₖ(ξ)·qₖ| —— 各阶绝对值叠加（无正负抵消 → 帧间平滑，
    // 且各阶幅值不同步变化 → 形态随帧演化）
    QVector<double> out(N);
    for (int i = 0; i < N; ++i) {
        double sum = 0;
        for (int k = 0; k < FreeModeData::kNumModes; ++k) {
            if (fm.modeShapes[k].size() != N) continue;
            sum += std::abs(fm.modeShapes[k][i] * q[k]);
        }
        out[i] = sum;
    }
    return out;
}

void exportFlexResult(const FlexInput& in, const FlexOutput& out, const FlexCaptureFrameData& cap,
                      MechMotionData::mMotionFlexBodyData& dest)
{
    using namespace MechMotionData;
    dest.m_bodyId = in.bodyId;
    dest.m_name = in.name;
    dest.m_nModes = out.modes;
    dest.m_nInterfaces = out.interfaces;

    // 一次性数据全部由引擎 DLL 内收集带出（跨 DLL dynamic_cast 已实测丢节点导致错位爆炸）
    for (double v : out.nodeX0)
        dest.m_nodeX0.push_back(v);
    for (int t : in.tetIds)
        dest.m_tetIds.push_back(t);
    for (int t : out.surfaceTris)
        dest.m_surfaceTris.push_back(t);
    for (double v : out.psiInternal)
        dest.m_psi.push_back(v);

    // 一次性：接口初始位姿
    // 接口初始位姿：取第一帧采集值（求解结束再调本函数时 GetCoord() 已是终态，
    // 用终态当参考会让每帧 Δp/Δrot 全错——甩飞场景下爆炸的根源）
    if (!cap.times.empty() && 7 * out.interfaces <= (int)cap.poseFlat.size()) {
        // poseFlat 采集的是 Chrono data()=[w,x,y,z] 顺序，桥接/渲染端需保持一致
        for (int k = 0; k < 7 * out.interfaces; ++k)
            dest.m_interfacePose0.push_back(cap.poseFlat[k]);
    }
    else {
        for (auto& b : out.interfaceBodies) {
            if (!b) continue;
            const auto& coord = b->GetCoord();
            dest.m_interfacePose0.push_back(coord.pos.x());
            dest.m_interfacePose0.push_back(coord.pos.y());
            dest.m_interfacePose0.push_back(coord.pos.z());
            const double* q4 = coord.rot.data();  // Chrono: [w,x,y,z]（采集和渲染端须一致）
            for (int k = 0; k < 4; ++k)
                dest.m_interfacePose0.push_back(q4[k]);
        }
    }

    // 帧数据（采集于每步进度回调）
    const int nFrames = (int)cap.times.size();
    dest.m_frames.reserve(nFrames);
    for (int f = 0; f < nFrames; ++f) {
        mMotionFlexFrame fr;
        fr.m_time = cap.times[f];
        const double* q = cap.qFlat.data() + (size_t)f * out.modes;
        for (int i = 0; i < out.modes; ++i) fr.m_modalQ.push_back(q[i]);
        const double* p = cap.poseFlat.data() + (size_t)f * 7 * out.interfaces;
        for (int i = 0; i < 7 * out.interfaces; ++i) fr.m_interfacePose.push_back(p[i]);
        dest.m_frames.push_back(std::move(fr));
    }
}

int reattachJointsToFlex(motion::MotionSystem& sys, const FlexInput& in, FlexOutput& out,
                         const std::function<void(const QString&)>& log)
{
    // 引擎刚体命名 = parts_map key = std::to_string(bodyId)（ParserAdams.cpp:2078）
    auto rigidBody = sys.SearchBody(std::to_string(in.bodyId).c_str());
    if (!rigidBody) {
        // 外部接口体模式：刚体已被 BuildFlexBody 移入柔体装配（顶层搜不到属正常），无需改挂
        return 0;
    }

    // 最近的 interface body（按绝对位置）
    auto nearestInterface = [&out](const motion::MotionVector<>& pos) -> std::shared_ptr<motion::MotionBody> {
        std::shared_ptr<motion::MotionBody> best;
        double bestD2 = 1e300;
        for (auto& b : out.interfaceBodies) {
            if (!b) continue;
            const motion::MotionVector<> d = b->GetPos() - pos;
            const double d2 = d.Length2();
            if (d2 < bestD2) { bestD2 = d2; best = b; }
        }
        return best;
    };

    int reattached = 0;
    const auto links = sys.Get_linklist();  // 拷贝快照，改挂不增删 link
    for (auto& link : links) {
        // 只有 ReferenceMarkers 系（运动副/驱动/弹簧等双 marker link）可改挂
        auto refLink = std::dynamic_pointer_cast<motion::MotionLinkMarkers>(link);
        if (!refLink)
            continue;
        motion::MotionMarker* m[2] = { refLink->GetMarker1(), refLink->GetMarker2() };
        if (!m[0] && !m[1])
            continue;
        motion::MotionMarker* newM[2] = { nullptr, nullptr };
        bool touched = false;
        for (int i = 0; i < 2; ++i) {
            if (m[i] && m[i]->GetBody() == rigidBody.get()) {
                auto itf = nearestInterface(m[i]->GetAbsCoord().pos);
                if (!itf)
                    continue;
                auto mk = std::make_shared<motion::MotionMarker>();
                itf->AddMarker(mk);
                mk->Impose_Abs_Coord(m[i]->GetAbsCoord());  // 保持原 marker 的绝对坐标系
                newM[i] = mk.get();
                touched = true;
            }
        }
        if (!touched)
            continue;
        refLink->ReferenceMarkers(newM[0] ? newM[0] : m[0],
                                  newM[1] ? newM[1] : m[1]);
        refLink->Update(0.0, true);
        ++reattached;
        log(QString("  [柔性体] %1：副 %2 已改挂到柔性体接口").arg(in.name).arg(QString::fromStdString(refLink->GetNameString())));
    }

    //【外部接口体模式】原刚体保留：它是柔体的接口体，副保持在它身上，
    // 约束变量永远有效（早期"改挂+RemoveBody"方案会使副约束变量悬垂导致 NaN，已弃用）。
    // rigidBody 现在等价于 out.interfaceBodies[0]，不做任何事。

    // 兜底：没有任何 link 转挂且原刚体是固定的 → 固定首个 interface（悬臂）
    if (reattached == 0 && in.bodyFixed && !out.interfaceBodies.empty() && out.interfaceBodies[0]) {
        out.interfaceBodies[0]->SetBodyFixed(true);
        log(QString("  [柔性体] %1：目标刚体无运动副且为固定体，接口已固定（悬臂）").arg(in.name));
    }
    return reattached;
}

}  // namespace mMotionFlexBridge
