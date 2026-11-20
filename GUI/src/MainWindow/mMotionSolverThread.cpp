#include "mMotionSolverThread.h"

#include "mMotionFlexBridge.h"
#include "mMotionAutoFlex.h"
#include "motion/modal/ModalAssembly.h"
#include "motion/solver/IterativeSolverLS.h"
#include <fstream>
#include "mGlobalSignals.h"
#include "mMotionScenario.h"
#include "mMotionResultData.h"
#include "mMotionCurveData.h"
#include "mMotionScenarioManager.h"
#include <QDebug>
#include <QMatrix4x4>
#include <QDir>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QApplication>
#include <cmath>
#include <climits>
#include <algorithm>
#include <unordered_map>
#include <stdexcept>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

//【OCC摘除·引擎化】mMotionSurfaceBoundary（点在面上副边界墙）算法已迁入
// RebuildGeometry 求值服务（evaluation().faceBoundaryWalls，方案 E-37），
// 本文件改经引擎句柄调用；GUI 侧零 OCC（算法原文保档于源文件，退出构建清单）。

#include "mViewportModelManager.h"
#include "mViewportModel.h"
//【OCC摘除·引擎化】几何采样/离散化统一走引擎求值服务（E-35~E-39）：
// mGeoDataBase/mGeoDataModelSet（旧栈 MXGeoData，含 TopoDS 直引）退役。
#include "mGeoEngineData.h"
#include "mGeoEngineHandle.h"

//BasicData
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionControllerManager.h"
#include "mMotionControllerBase.h"
#include "mMotionPIDController.h"
#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"
#include "mMotionSensorManager.h"
#include "mMotionSensorBase.h"
#include "mMotionBodySensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"
#include <set>
#include "mMotionJointDrive.h"
#include "mMotionSpringBase.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionSpringManager.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingCable.h"
#include "mMotionCouplingManager.h"
#include "mMotionMarkerManager.h"
#include "mMotionMarker.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include "mMotionScalarTorque.h"
#include "mMotionBushing.h"
#include "mMotionForceElement.h"
#include "mMotionForceElementManager.h"
#include "mModelData.h"
#include "mMeshData.h"

#include "motion/utils/ParserAdams.h"
#include "motion/utils/ContactInputSettings.h"
#include "motion/physics/SystemSMC.h"
#include "motion/physics/LinkPointOnSurface.h"
//#include "Motion"

// 几何离散数据（用于 3D 接触网格）
//【OCC摘除·引擎化】mGeoManager/mGeometricFeatureAttribute/mGeoDataBase/mGeoDataModelSet
// 旧栈离散化改走 engine().partTessellation（E-39，部件级中性三角网格）。
#include "mMotionContact.h"
#include "mMotionContactManager.h"

namespace MechMainWindow
{
    namespace {
        // 安全归一化：零向量 / NaN 输入时回退到 (0,0,1)，杜绝把 NaN 写入求解器
        QVector3D safeNormalized(const QVector3D& v, const QVector3D& fallback = QVector3D(0.0f, 0.0f, 1.0f))
        {
            const float x = v.x(), y = v.y(), z = v.z();
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
                return fallback;
            }
            const float lenSq = x * x + y * y + z * z;
            if (lenSq < 1e-12f) {
                return fallback;
            }
            return v / std::sqrt(lenSq);
        }

        // 边按弦高容差离散为有序折线点（世界坐标，沿边参数方向）。
        //【OCC摘除·引擎化】原 BRepAdaptor_Curve + GCPnts 直调改走引擎求值服务
        // E-06 curveLength（弧长定采样策略）+ E-35 edgePolylineByDeflection（离散）。
        // 弦高容差取绝对 0.02mm（钳到 [1e-4, 边长的 1%]）：两曲线接触校验按折线进行，
        // 弦高会直接变成"假接触距离"——大圆 0.5mm 弦高在顶点错位时可放大成毫米级假距离，
        // 故按绝对精度而非边长比例采样（500mm 圆约 700 点，装配一次性开销可接受）。
        bool sampleEdgePolyline(const EventDispatcher::mGeoObjectRef& edgeRef,
                                 GeoServer::mGeoEngineHandle& geoEngine,
                                 std::vector<QVector3D>& outPts)
        {
            const double len = geoEngine.curveLength(edgeRef);
            if (!(len > 0.0) || !std::isfinite(len)) return false;
            const double defl = std::min(0.01 * len, std::max(1e-4, 0.02));
            const auto pts = geoEngine.edgePolylineByDeflection(edgeRef, defl);
            if (pts.size() < 2) return false;
            outPts.clear();
            outPts.reserve(pts.size());
            for (const auto& p : pts)
                outPts.emplace_back(float(p.x()), float(p.y()), float(p.z()));
            return true;
        }

        // 多段边折线串接：按端点容差首尾相接成一条链（需要时反转边方向），
        // 等价于 UG 的 Join/Connected Curves，免去用户手动合并。
        // 串不完整（存在接不上的边）返回 false；闭合链（首尾点重合）追加首点封口并置 outClosed。
        bool stitchEdgeChains(const std::vector<std::vector<QVector3D>>& chains, double tol,
            std::vector<QVector3D>& outPoly, bool& outClosed)
        {
            outClosed = false;
            if (chains.empty()) return false;
            const size_t n = chains.size();
            std::vector<char> used(n, 0);
            auto dist = [](const QVector3D& a, const QVector3D& b) { return double((a - b).length()); };

            outPoly = chains[0];
            used[0] = 1;

            bool grew = true;
            while (grew) {
                grew = false;
                for (size_t i = 0; i < n; ++i) {
                    if (used[i]) continue;
                    const auto& c = chains[i];
                    const bool tailMatchF = dist(outPoly.back(), c.front()) <= tol;
                    const bool tailMatchB = dist(outPoly.back(), c.back()) <= tol;
                    const bool headMatchF = dist(outPoly.front(), c.front()) <= tol;
                    const bool headMatchB = dist(outPoly.front(), c.back()) <= tol;
                    if (tailMatchF || tailMatchB) {
                        for (size_t k = tailMatchF ? 0 : c.size(); tailMatchF ? (k < c.size()) : (k > 0); tailMatchF ? ++k : --k) {
                            const QVector3D& p = c[tailMatchF ? k : k - 1];
                            if (dist(outPoly.back(), p) > 1e-9) outPoly.push_back(p);
                        }
                        used[i] = 1; grew = true;
                    }
                    else if (headMatchF || headMatchB) {
                        std::vector<QVector3D> prepend;
                        for (size_t k = headMatchF ? 0 : c.size(); headMatchF ? (k < c.size()) : (k > 0); headMatchF ? ++k : --k) {
                            const QVector3D& p = c[headMatchF ? k : k - 1];
                            if (!prepend.empty() || dist(p, outPoly.front()) > 1e-9) prepend.push_back(p);
                        }
                        // 去掉与现首点重合的接缝点后前插
                        while (!prepend.empty() && dist(prepend.back(), outPoly.front()) <= 1e-9) prepend.pop_back();
                        outPoly.insert(outPoly.begin(), prepend.begin(), prepend.end());
                        used[i] = 1; grew = true;
                    }
                }
            }
            for (size_t i = 0; i < n; ++i) {
                if (!used[i]) return false;
            }
            if (outPoly.size() > 2 && dist(outPoly.front(), outPoly.back()) <= tol) {
                outClosed = true;
                if (dist(outPoly.front(), outPoly.back()) > 1e-9) outPoly.push_back(outPoly.front());
            }
            return outPoly.size() >= 2;
        }

        // 点到折线的最小距离（逐段点到线段）
        double pointToPolylineDist(const QVector3D& p, const std::vector<QVector3D>& poly)
        {
            double best = std::numeric_limits<double>::max();
            for (size_t i = 0; i + 1 < poly.size(); ++i) {
                const QVector3D a = poly[i];
                const QVector3D b = poly[i + 1];
                const QVector3D ab = b - a;
                const double len2 = double(QVector3D::dotProduct(ab, ab));
                double t = 0.0;
                if (len2 > 1e-20)
                    t = double(QVector3D::dotProduct(p - a, ab)) / len2;
                t = std::max(0.0, std::min(1.0, t));
                const QVector3D q = a + ab * float(t);
                best = std::min(best, double((p - q).length()));
            }
            return best;
        }

        // 将一个几何 Part 的离散三角面追加到 body 的接触网格中。
        // offset 为全局点到 body 局部坐标系（通常以质心 Marker 为原点）的平移偏移。
        // nodes: xyzxyz...，elems: 每个三角形三个局部顶点索引。
        //【OCC摘除·引擎化】旧栈 GeoDataModelSet::getGeoDiscreteData 改走引擎
        // E-39 partTessellation（部件级中性三角网格，结构同构：globalPoints/facePoints）。
        void appendPartContactMesh(int partId,
            GeoServer::mGeoEngineHandle* geoEngine,
            const QVector3D& offset,
            std::vector<double>& nodes,
            std::vector<int>& elems,
            int& badTriCount,
            const std::set<int>* faceFilter = nullptr)
        {
            if (!geoEngine) return;
            EventDispatcher::mGeoTessellationRequest tessReq;
            tessReq.partID = static_cast<uint64_t>(partId);
            const EventDispatcher::mGeoTessellationResult tess = geoEngine->partTessellation(tessReq);
            if (!tess.succeeded()) return;
            const GeoNeutralData::GN_TessellationMesh& discrete = tess.mesh;
            if (discrete.globalPoints.empty() || discrete.facePoints.empty()) return;

            // 记录本 Part 加入前的顶点偏移
            const int nodeOffset = static_cast<int>(nodes.size() / 3);

            // 将全局离散点转换到 body 局部坐标系，并映射为 body 局部索引
            std::unordered_map<int, int> oldToNew;
            oldToNew.reserve(discrete.globalPoints.size());
            // 顶点按离散点 id（几何离散生成序）导出：HACD 凸分解对输入顺序敏感，
            // 几何连续序保持聚类图连通性（分解贴原网格）；坐标字典序会打散空间
            // 邻接（实测产生持续颤振的坏分解）。id 序保证同一模型每次导出字节
            // 恒定（行为确定）。
            {
                using PtRef = decltype(&*discrete.globalPoints.begin());
                std::vector<PtRef> pts;
                pts.reserve(discrete.globalPoints.size());
                for (const auto& kv : discrete.globalPoints)
                    pts.push_back(&kv);
                std::sort(pts.begin(), pts.end(),
                          [](PtRef a, PtRef b) { return a->first < b->first; });
                for (const auto* p : pts) {
                    oldToNew[p->first] = nodeOffset + static_cast<int>(oldToNew.size());
                    nodes.push_back(p->second.x() - offset.x());
                    nodes.push_back(p->second.y() - offset.y());
                    nodes.push_back(p->second.z() - offset.z());
                }
            }

            // 遍历该 Part 所有面的三角索引（每三个一组构成一个三角形）
            for (const auto& faceKV : discrete.facePoints) {
                if (faceFilter && !faceFilter->count(faceKV.first)) continue;
                const std::vector<int>& indices = faceKV.second;
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    auto it0 = oldToNew.find(indices[i]);
                    auto it1 = oldToNew.find(indices[i + 1]);
                    auto it2 = oldToNew.find(indices[i + 2]);
                    if (it0 == oldToNew.end() || it1 == oldToNew.end() || it2 == oldToNew.end())
                        continue;
                    // 清洗：跳过含 NaN 顶点或退化（零面积）的三角形。
                    // NaN 会污染 BVH 的 AABB 导致 Bullet 全场景误重叠、凭空出接触力；
                    // 退化三角形会让窄相求解不稳定。旧凸包路径会默默丢弃坏点，
                    // 三角网格路径必须显式清洗。
                    const double x0 = nodes[3 * it0->second], y0 = nodes[3 * it0->second + 1], z0 = nodes[3 * it0->second + 2];
                    const double x1 = nodes[3 * it1->second], y1 = nodes[3 * it1->second + 1], z1 = nodes[3 * it1->second + 2];
                    const double x2 = nodes[3 * it2->second], y2 = nodes[3 * it2->second + 1], z2 = nodes[3 * it2->second + 2];
                    const bool hasNaN = std::isnan(x0) || std::isnan(y0) || std::isnan(z0)
                        || std::isnan(x1) || std::isnan(y1) || std::isnan(z1)
                        || std::isnan(x2) || std::isnan(y2) || std::isnan(z2);
                    const double e1x = x1 - x0, e1y = y1 - y0, e1z = z1 - z0;
                    const double e2x = x2 - x0, e2y = y2 - y0, e2z = z2 - z0;
                    const double cx = e1y * e2z - e1z * e2y;
                    const double cy = e1z * e2x - e1x * e2z;
                    const double cz = e1x * e2y - e1y * e2x;
                    const bool degenerate = (cx * cx + cy * cy + cz * cz) < 1e-30;
                    if (hasNaN || degenerate) {
                        ++badTriCount;
                        continue;
                    }
                    elems.push_back(it0->second);
                    elems.push_back(it1->second);
                    elems.push_back(it2->second);
                }
            }
        }

        // 根据 bodyId 获取其关联的所有几何 Part 的合并接触网格
        // offset 为全局点到 body 局部坐标系的平移偏移（默认取 body 质心）。
        int buildBodyContactMesh(int bodyId,
            MechMotionData::mMotionBodyManager& bodyManager,
            GeoServer::mGeoEngineHandle* geoEngine,
            const QVector3D& offset,
            std::vector<double>& nodes,
            std::vector<int>& elems,
            MechPreData::mViewportModel* viewportModel,
            const std::set<int>* faceFilter = nullptr)
        {
            int badTriCount = 0;
            nodes.clear();
            elems.clear();
            auto* body = bodyManager.getData(bodyId);
            if (!body) return 0;

            std::set<int> partIds;
            if (viewportModel && viewportModel->getModelData())
                partIds = viewportModel->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_MotionBody, body->getId() }, Physical_Part);
            for (int partId : partIds) {
                appendPartContactMesh(partId, geoEngine, offset, nodes, elems, badTriCount, faceFilter);
            }

            return badTriCount;

        }


        // ===== 崩溃诊断转储（临时，定位后移除）=====
        // 求解前把送进引擎的完整输入原样落盘到 E:/motion_dump/run_<时间戳>/：
        // 部件/标记点/运动副/驱动/接触（含面片数组）+ 实际加载的引擎 DLL 路径，
        // 供离线字节级复现，无需再次启动 GUI。
        QString dumpG(double v) { return QString::number(v, 'g', 17); }

        void dumpVec3(QTextStream& s, const char* tag, const double v[3]) {
            s << "  " << tag << "=" << dumpG(v[0]) << " " << dumpG(v[1]) << " " << dumpG(v[2]) << "\n";
        }

        void dumpEngineDllInfo(QTextStream& s) {
            const wchar_t* names[] = { L"motionengined.dll", L"motionengine.dll" };
            for (const wchar_t* n : names) {
                HMODULE h = nullptr;
                if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, n, &h) && h) {
                    wchar_t path[MAX_PATH] = {};
                    GetModuleFileNameW(h, path, MAX_PATH);
                    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                    QString sizeTxt = QStringLiteral("?");
                    if (f != INVALID_HANDLE_VALUE) {
                        LARGE_INTEGER sz{};
                        if (GetFileSizeEx(f, &sz)) sizeTxt = QString::number(sz.QuadPart);
                        CloseHandle(f);
                    }
                    s << "engine_dll=" << QString::fromWCharArray(path)
                      << " size=" << sizeTxt << "\n";
                    return;
                }
            }
            s << "engine_dll=NOT_LOADED_YET\n";
        }

        void dumpSolverInputForOfflineRepro(const motion::MotionSystemSMC& sys) {
            const QString dir = QStringLiteral("E:/motion_dump/") +
                QDateTime::currentDateTime().toString(QStringLiteral("run_yyyyMMdd_HHmmss"));
            if (!QDir().mkpath(dir)) return;

            {
                QFile f(dir + QStringLiteral("/engine_dll.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream s(&f); s.setCodec("UTF-8");
                    dumpEngineDllInfo(s);
                }
            }
            {
                QFile f(dir + QStringLiteral("/parts.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream s(&f); s.setCodec("UTF-8");
                    for (const auto& kv : sys.parts_map) {
                        const auto& p = kv.second;
                        s << "PART " << QString::fromStdString(kv.first)
                          << " fixed=" << (p.fixed ? 1 : 0)
                          << " mass=" << dumpG(p.mass)
                          << " cm=" << QString::fromStdString(p.cm_marker_id) << "\n";
                        dumpVec3(s, "loc", p.loc);
                        dumpVec3(s, "rot", p.rot);
                        dumpVec3(s, "vel", p.vel);
                        s << "  inertia=" << dumpG(p.inertia[0]) << " " << dumpG(p.inertia[1])
                          << " " << dumpG(p.inertia[2]) << " " << dumpG(p.inertia[3]) << " "
                          << dumpG(p.inertia[4]) << " " << dumpG(p.inertia[5]) << "\n";
                    }
                }
            }
            {
                QFile f(dir + QStringLiteral("/markers.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream s(&f); s.setCodec("UTF-8");
                    for (const auto& kv : sys.markers_map) {
                        const auto& m = kv.second;
                        s << "MARKER " << QString::fromStdString(kv.first)
                          << " part=" << QString::fromStdString(m.part_id) << "\n";
                        dumpVec3(s, "loc", m.loc);
                        dumpVec3(s, "rot", m.rot);
                    }
                }
            }
            {
                QFile f(dir + QStringLiteral("/joints.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream s(&f); s.setCodec("UTF-8");
                    for (const auto& kv : sys.joints_map) {
                        if (!kv.second) continue;
                        const auto& j = *kv.second;
                        s << "JOINT " << QString::fromStdString(kv.first)
                          << " type=" << QString::fromStdString(j.type)
                          << " I=" << QString::fromStdString(j.marker_I)
                          << " J=" << QString::fromStdString(j.marker_J) << "\n";
                        s << "  has_friction=" << (j.has_friction ? 1 : 0)
                          << " mus=" << dumpG(j.mu_static) << " muk=" << dumpG(j.mu_kinetic)
                          << " visc=" << dumpG(j.viscous_friction)
                          << " radius=" << dumpG(j.friction_radius) << "\n";
                        s << "  has_limit=" << (j.has_limit ? 1 : 0)
                          << " lowerEn=" << (j.lower_limit_enabled ? 1 : 0)
                          << " upperEn=" << (j.upper_limit_enabled ? 1 : 0)
                          << " penaltyOnly=" << (j.limit_penalty_only ? 1 : 0) << "\n";
                        s << "  lower=" << dumpG(j.lower_limit) << " upper=" << dumpG(j.upper_limit)
                          << " lowCush=" << dumpG(j.lower_limit_cushion) << " upCush=" << dumpG(j.upper_limit_cushion)
                          << " lowK=" << dumpG(j.lower_limit_stiffness) << " upK=" << dumpG(j.upper_limit_stiffness)
                          << " lowC=" << dumpG(j.lower_limit_damping) << " upC=" << dumpG(j.upper_limit_damping) << "\n";
                        if (j.has_surface) {
                            s << "  SURFN " << j.surface_nodes.size() / 3 << "\n";
                            for (size_t i = 0; i + 2 < j.surface_nodes.size(); i += 3)
                                s << "    " << dumpG(j.surface_nodes[i]) << " " << dumpG(j.surface_nodes[i + 1])
                                  << " " << dumpG(j.surface_nodes[i + 2]) << "\n";
                            s << "  SURFE " << j.surface_elems.size() / 3 << "\n";
                            for (size_t i = 0; i + 2 < j.surface_elems.size(); i += 3)
                                s << "    " << j.surface_elems[i] << " " << j.surface_elems[i + 1]
                                  << " " << j.surface_elems[i + 2] << "\n";
                            s << "  SURFB " << j.surface_boundary_edges.size() / 2 << "\n";
                            for (size_t i = 0; i + 1 < j.surface_boundary_edges.size(); i += 2)
                                s << "    " << j.surface_boundary_edges[i] << " " << j.surface_boundary_edges[i + 1] << "\n";
                            s << "  SURFBI " << j.surface_boundary_inward.size() / 3 << "\n";
                            for (size_t i = 0; i + 2 < j.surface_boundary_inward.size(); i += 3)
                                s << "    " << dumpG(j.surface_boundary_inward[i]) << " "
                                  << dumpG(j.surface_boundary_inward[i + 1]) << " "
                                  << dumpG(j.surface_boundary_inward[i + 2]) << "\n";
                        }
                    }
                }
            }
            {
                QFile f(dir + QStringLiteral("/motions.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream s(&f); s.setCodec("UTF-8");
                    for (const auto& kv : sys.motions_map) {
                        const auto& m = kv.second;
                        s << "MOTION " << QString::fromStdString(kv.first)
                          << " I=" << QString::fromStdString(m.marker_I)
                          << " J=" << QString::fromStdString(m.marker_J)
                          << " is_rot=" << (m.is_rotation ? 1 : 0)
                          << " is_trans=" << (m.is_translation ? 1 : 0)
                          << " is_speed=" << (m.is_speed ? 1 : 0)
                          << " is_pos=" << (m.is_position ? 1 : 0)
                          << " func=" << dumpG(m.func_value)
                          << " expr=\"" << QString::fromStdString(m.function_expr) << "\""
                          << " has_table=" << (m.has_table ? 1 : 0) << "\n";
                        if (m.has_table) {
                            s << "  TABLE " << m.x_table.size() << "\n";
                            for (size_t i = 0; i < m.x_table.size(); ++i)
                                s << "    " << dumpG(m.x_table[i]) << " " << dumpG(m.f_table[i]) << "\n";
                        }
                    }
                }
            }
            {
                QDir().mkpath(dir + QStringLiteral("/contacts"));
                for (const auto& kv : sys.contacts_map) {
                    const auto& c = kv.second;
                    QFile f(dir + QStringLiteral("/contacts/") + QString::fromStdString(kv.first) + QStringLiteral(".txt"));
                    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) continue;
                    QTextStream s(&f); s.setCodec("UTF-8");
                    s << "CONTACT " << QString::fromStdString(kv.first)
                      << " I=" << QString::fromStdString(c.body_I)
                      << " J=" << QString::fromStdString(c.body_J) << "\n";
                    s << "  stiffness=" << dumpG(c.stiffness) << " hasK=" << (c.has_stiffness ? 1 : 0)
                      << " damping=" << dumpG(c.damping) << " hasC=" << (c.has_damping ? 1 : 0)
                      << " deadband=" << dumpG(c.penetration_deadband) << "\n";
                    s << "  friction=" << dumpG(c.friction) << " hasF=" << (c.has_friction ? 1 : 0)
                      << " static_friction=" << dumpG(c.static_friction) << " hasSF=" << (c.has_static_friction ? 1 : 0)
                      << " restitution=" << dumpG(c.restitution) << " hasR=" << (c.has_restitution ? 1 : 0) << "\n";
                    s << "  is_static_I=" << (c.is_static_I ? 1 : 0) << " is_convex_I=" << (c.is_convex_I ? 1 : 0)
                      << " is_static_J=" << (c.is_static_J ? 1 : 0) << " is_convex_J=" << (c.is_convex_J ? 1 : 0) << "\n";
                    s << "  mesh_file_I=\"" << QString::fromStdString(c.mesh_file_I)
                      << "\" mesh_file_J=\"" << QString::fromStdString(c.mesh_file_J) << "\"\n";
                    s << "NODES_I " << c.nodes_I.size() / 3 << "\n";
                    for (size_t i = 0; i + 2 < c.nodes_I.size(); i += 3)
                        s << "  " << dumpG(c.nodes_I[i]) << " " << dumpG(c.nodes_I[i + 1]) << " " << dumpG(c.nodes_I[i + 2]) << "\n";
                    s << "ELEMS_I " << c.elems_I.size() / 3 << "\n";
                    for (size_t i = 0; i + 2 < c.elems_I.size(); i += 3)
                        s << "  " << c.elems_I[i] << " " << c.elems_I[i + 1] << " " << c.elems_I[i + 2] << "\n";
                    s << "NODES_J " << c.nodes_J.size() / 3 << "\n";
                    for (size_t i = 0; i + 2 < c.nodes_J.size(); i += 3)
                        s << "  " << dumpG(c.nodes_J[i]) << " " << dumpG(c.nodes_J[i + 1]) << " " << dumpG(c.nodes_J[i + 2]) << "\n";
                    s << "ELEMS_J " << c.elems_J.size() / 3 << "\n";
                    for (size_t i = 0; i + 2 < c.elems_J.size(); i += 3)
                        s << "  " << c.elems_J[i] << " " << c.elems_J[i + 1] << " " << c.elems_J[i + 2] << "\n";
                }
            }
        }

        // 将界面层运动副的摩擦/限位配置复制到求解器中间结构。
        // adams_joint_struct 与 adams_motion_struct 需要具有同名字段，见随附 SolverCorePatch.md。
        template <typename TargetStruct>
        void fillJointFrictionAndLimit(const MechMotionData::mMotionJointBase* source,
            TargetStruct& target)
        {
            if (!source) return;

            const auto& friction = source->getFrictionData();
            target.has_friction = source->supportsFriction() && friction.enabled;
            target.mu_static = std::max(0.0, friction.staticCoefficient);
            target.mu_kinetic = std::max(0.0, friction.kineticCoefficient);
            target.viscous_friction = std::max(0.0, friction.viscousCoefficient);
            target.friction_radius = std::max(0.0, friction.effectiveRadius);

            const auto& limit = source->getLimitData();
            target.has_limit = source->supportsLimit() &&
                limit.enabled &&
                (limit.lowerEnabled || limit.upperEnabled);
            target.lower_limit_enabled = target.has_limit && limit.lowerEnabled;
            target.upper_limit_enabled = target.has_limit && limit.upperEnabled;
            // 限位角度量（旋转副下限/上限/缓冲区）数据层以"度"存储（与面板一致），
            // 引擎要求 rad：旋转副在此 ×π/180；平移类限位保持 mm 原样
            const double limitDeg2Rad = (source->getType() == JointType::Revolute)
                ? 3.14159265358979323846 / 180.0
                : 1.0;
            target.lower_limit = limit.lowerValue * limitDeg2Rad;
            target.upper_limit = limit.upperValue * limitDeg2Rad;
            target.limit_penalty_only = limit.penaltyOnly;
            target.lower_limit_cushion = std::max(0.0, limit.lowerCushion) * limitDeg2Rad;
            target.upper_limit_cushion = std::max(0.0, limit.upperCushion) * limitDeg2Rad;
            // 限位刚度/阻尼（面板口径：旋转 N·mm/° 与 N·mm/(°/s)，平移 N/mm 与 N·s/mm）：
            // 1) 旋转副按"度"口径 → 引擎 rad 口径 ×π/180（与扭簧刚度同法，见下方 springs 段）；
            // 2) N 系 → 引擎 mN 系 ×1000（此前漏乘，限位软 1000 倍）
            const double limitRotUnit = limitDeg2Rad * 1000.0;   // 旋转：°→rad + N→mN
            const double limitTranUnit = 1000.0;                 // 平移：仅 N→mN
            const double limitStiffUnit = (source->getType() == JointType::Revolute) ? limitRotUnit : limitTranUnit;
            target.lower_limit_stiffness = std::max(0.0, limit.lowerStiffness) * limitStiffUnit;
            target.upper_limit_stiffness = std::max(0.0, limit.upperStiffness) * limitStiffUnit;
            target.lower_limit_damping = std::max(0.0, limit.lowerDamping) * limitStiffUnit;
            target.upper_limit_damping = std::max(0.0, limit.upperDamping) * limitStiffUnit;

            if (target.has_limit && target.lower_limit_enabled && target.upper_limit_enabled &&
                target.lower_limit > target.upper_limit) {
                throw std::runtime_error(
                    QStringLiteral("运动副 %1 的限位下限大于上限")
                    .arg(source->getId())
                    .toStdString());
            }

            if (target.has_limit && target.limit_penalty_only) {
                bool invalidLower = target.lower_limit_enabled &&
                    (target.lower_limit_cushion <= 0.0 || target.lower_limit_stiffness <= 0.0);
                bool invalidUpper = target.upper_limit_enabled &&
                    (target.upper_limit_cushion <= 0.0 || target.upper_limit_stiffness <= 0.0);
                if (invalidLower || invalidUpper) {
                    throw std::runtime_error(
                        QStringLiteral("运动副 %1 使用纯软限位时，已启用侧的缓冲区和刚度必须大于 0")
                        .arg(source->getId())
                        .toStdString());
                }
            }
        }
    }

    mMotionSolverThread::mMotionSolverThread(int scenarioId, MechPreData::mViewportModel* viewportModel, QObject* parent)
        : QThread(parent), m_scenarioId(scenarioId), m_viewportModel(viewportModel)
    {
    }

    mMotionSolverThread::~mMotionSolverThread() {}

    int mMotionSolverThread::refId(MxFunctions::PhysicalFieldType src, int srcId, MxFunctions::PhysicalFieldType dst,
        MxFunctions::MapRelationType role) const
    {
        if (m_viewportModel == nullptr || m_viewportModel->getModelData() == nullptr)
            return -1;
        return m_viewportModel->getModelData()->getMapRelationManager().getMapRelation<int>({ src, srcId }, dst, role);
    }

    std::set<int> mMotionSolverThread::refSet(MxFunctions::PhysicalFieldType src, int srcId, MxFunctions::PhysicalFieldType dst,
        MxFunctions::MapRelationType role) const
    {
        if (m_viewportModel == nullptr || m_viewportModel->getModelData() == nullptr)
            return {};
        return m_viewportModel->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ src, srcId }, dst, role);
    }

    void mMotionSolverThread::run()
    {
        // 求解期间窗口可能失焦导致全局 getCVM 变为 nullptr，
        // 线程内只使用启动时捕获的 m_viewportModel，这里做一次兜底防护。
        if (m_viewportModel == nullptr)
        {
            emit messageLogged("求解失败：启动时的视口模型指针为空");
            emit solveFinished(m_scenarioId, false);
            return;
        }
        try
        {
            doTransInput();
            // 【关键】前处理失败时直接退出，不再进入求解阶段
            if (m_sys.parts_map.empty()) {
                emit messageLogged("前处理数据校验失败，求解中止");
                emit solveFinished(m_scenarioId, false);
                return;
            }
            doSolve();
        }
        catch (const std::exception& e)
        {
            emit messageLogged(QString("求解线程异常: %1").arg(QString::fromStdString(e.what())));
            emit solveFinished(m_scenarioId, false);
        }
        catch (...)
        {
            emit messageLogged(QString("求解线程发生未知异常"));
            emit solveFinished(m_scenarioId, false);
        }
    }
    void mMotionSolverThread::test()
    {
        emit messageLogged(QString("开始初始化求解器，方案 ID: %1...").arg(m_scenarioId));

        // ==========================================
        // 1. 模拟数据传递 (从 Manager 获取配置参数)
        // ==========================================
        auto scenario = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_scenarioId);

        // 模拟拿到了数据
        double endTime = 10.0;   // 假设 scenario->getEndTime()
        double stepSize = 0.01;  // 假设 scenario->getStepSize()

        if (stepSize <= 0) {
            emit messageLogged("错误：积分步长无效！");
            emit solveFinished(m_scenarioId, false);
            return;
        }

        int totalSteps = static_cast<int>(std::ceil(endTime / stepSize));
        emit messageLogged(QString("数据准备完毕。总计算步数: %1").arg(totalSteps));

        // ==========================================
        // 2. 准备接收结果的容器
        // ==========================================
        // 创建我们在上一节设计好的纯数据结果类
        auto* resultData = new MechMotionData::mMotionResultData();
        resultData->m_frames.reserve(totalSteps); // 预分配内存，极大提升性能

        //获取所有几何部件id
        auto partIds = m_viewportModel->getModelData()->getMeshData()->getAllPartIds();
        if (partIds.empty())
        {
            emit messageLogged("错误：没有模型");
            emit solveFinished(m_scenarioId, false);
            return;
        }

        // ==========================================
        // 3. 模拟求解过程 (数值积分循环)
        // ==========================================
        for (int step = 0; step <= totalSteps; ++step)
        {
            // 检查用户是否点击了“中止”按钮 (在UI中触发强制退出)
            if (this->isInterruptionRequested()) {
                emit messageLogged("用户手动中止了计算！");
                delete resultData; // 算了一半的数据直接丢弃
                emit solveFinished(m_scenarioId, false);
                return;
            }

            double currentTime = step * stepSize;

            // 【模拟物理计算耗时】：让线程睡 5 毫秒 (真实开发

            QThread::msleep(5);

            // ------------------------------------------
            // 4. 模拟生成单帧动画数据 (MotionFrame)
            // ------------------------------------------
            MechMotionData::mMotionFrame currentFrame;
            currentFrame.m_time = currentTime;

            // 模拟给 ID为 2001 和 2002 的两个运动体生成位姿
            QMatrix4x4 mat1, mat2;
            mat1.translate(0.0, -9.8 * currentTime * currentTime * 0.5, 0.0); // 模拟自由落体
            mat2.rotate(currentTime * 36.0, 0, 0, 1); // 模拟绕Z轴匀速旋转

            currentFrame.m_bodyTransforms.insert({ partIds.at(0), mat1 });
            if (partIds.size() >= 2)
            {
                currentFrame.m_bodyTransforms.insert({ partIds.at(1), mat2 });
            }

            // 将这一帧压入结果队列
            resultData->m_frames.push_back(currentFrame);

            // ------------------------------------------
            // 5. 更新进度条 (降频发送，防止信号洪峰卡死主线程)
            // ------------------------------------------
            if (step % std::max(1, totalSteps / 100) == 0 || step == totalSteps) {
                int percent = static_cast<int>((static_cast<double>(step) / totalSteps) * 100.0);
                emit progressUpdated(percent);
            }
        }

        // ==========================================
        // 6. 计算完成，挂载结果到大节点
        // ==========================================
        emit messageLogged("计算完成，正在保存结果数据...");

        // 伪代码：挂载结果
        scenario->attachResult(resultData);

        emit progressUpdated(100);
        emit solveFinished(m_scenarioId, true);
    }
    QStringList mMotionSolverThread::checkMotionDataIntegrity()
    {
        QStringList problems;
        auto* modelData = m_viewportModel->getModelData();
        auto& bodyManager = modelData->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
        auto& jointManager = modelData->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
        auto& forceElementManager = modelData->getManager<MechMotionData::mMotionForceElementManager>(Physical_MotionVectorForce);
        auto& springManager = modelData->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring);
        auto& contactManager = modelData->getManager<MechMotionData::mMotionContactManager>(Physical_MotionContact);
        auto& driveManager = modelData->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);

        // ---- 力元（矢量力/力矩/衬套/标量力/标量力矩） ----
        for (int feId : forceElementManager.getAllIDs()) {
            auto* fe = forceElementManager.getData(feId);
            if (!fe) continue;
            const QString name = forceElementManager.getNameByID(feId);

            if (auto* vf = dynamic_cast<MechMotionData::mMotionVectorForce*>(fe)) {
                const int bodyId = refId(Physical_MotionVectorForce, vf->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                if (bodyId == -1)
                    problems << QString("力元 '%1' 未选择作用体").arg(name);
                else if (!bodyManager.getData(bodyId))
                    problems << QString("力元 '%1' 的作用体不存在（可能已删除）").arg(name);

                if (auto* bushing = dynamic_cast<MechMotionData::mMotionBushing*>(fe)) {
                    const int baseId = refId(Physical_MotionVectorForce, bushing->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                    if (baseId >= 0 && !bodyManager.getData(baseId))
                        problems << QString("衬套 '%1' 的基本体不存在（可能已删除）").arg(name);
                }
            }
            else if (auto* sf = dynamic_cast<MechMotionData::mMotionScalarForce*>(fe)) {
                const int bodyIId = refId(Physical_MotionVectorForce, sf->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                const int bodyJId = refId(Physical_MotionVectorForce, sf->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                if (bodyIId < 0)
                    problems << QString("标量力 '%1' 未选择作用体").arg(name);
                else if (!bodyManager.getData(bodyIId))
                    problems << QString("标量力 '%1' 的作用体不存在（可能已删除）").arg(name);
                if (bodyJId >= 0 && !bodyManager.getData(bodyJId))
                    problems << QString("标量力 '%1' 的基本体不存在（可能已删除）").arg(name);
            }
            else if (auto* tpt = dynamic_cast<MechMotionData::mMotionThreePointTorque*>(fe)) {
                const int bodyIId = refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                if (bodyIId < 0)
                    problems << QString("三点力矩 '%1' 未选择应用体").arg(name);
                else if (!bodyManager.getData(bodyIId))
                    problems << QString("三点力矩 '%1' 的应用体不存在（可能已删除）").arg(name);
                if (refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionActionBody) >= 0 && !bodyManager.getData(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionActionBody)))
                    problems << QString("三点力矩 '%1' 的出发体不存在（可能已删除）").arg(name);
                if (refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) >= 0 && !bodyManager.getData(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBaseBody)))
                    problems << QString("三点力矩 '%1' 的目标体不存在（可能已删除）").arg(name);
            }
            else if (auto* tpf = dynamic_cast<MechMotionData::mMotionThreePointForce*>(fe)) {
                // 三点力：应用体/出发点宿主体/目标点宿主体均必选（可挂大地，体ID=-1 合法）
                const int bodyIId = refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                if (bodyIId < 0)
                    problems << QString("三点力 '%1' 未选择应用体").arg(name);
                else if (!bodyManager.getData(bodyIId))
                    problems << QString("三点力 '%1' 的应用体不存在（可能已删除）").arg(name);
                if (refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionActionBody) >= 0 && !bodyManager.getData(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionActionBody)))
                    problems << QString("三点力 '%1' 的出发体不存在（可能已删除）").arg(name);
                if (refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) >= 0 && !bodyManager.getData(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBaseBody)))
                    problems << QString("三点力 '%1' 的目标体不存在（可能已删除）").arg(name);
            }
            else if (auto* st = dynamic_cast<MechMotionData::mMotionScalarTorque*>(fe)) {
                const int bodyIId = refId(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                const int bodyJId = refId(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                if (bodyIId < 0)
                    problems << QString("标量力矩 '%1' 未选择作用体").arg(name);
                else if (!bodyManager.getData(bodyIId))
                    problems << QString("标量力矩 '%1' 的作用体不存在（可能已删除）").arg(name);
                if (bodyJId >= 0 && !bodyManager.getData(bodyJId))
                    problems << QString("标量力矩 '%1' 的基本体不存在（可能已删除）").arg(name);
            }
        }

        // ---- 弹簧（两点体弹簧 / 扭转弹簧） ----
        for (int springId : springManager.getAllIDs()) {
            auto* spring = springManager.getData(springId);
            if (!spring) continue;
            const QString name = springManager.getNameByID(springId);

            if (spring->getType() == MechMotionData::SpringType::BodyPoint) {
                auto* bpSpring = dynamic_cast<MechMotionData::mMotionBodyPointSpring*>(spring);
                if (bpSpring) {
                    const int bodyIId = refId(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                    const int bodyJId = refId(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                    if (bodyIId < 0 || bodyJId < 0)
                        problems << QString("弹簧 '%1' 两端体未设置").arg(name);
                    else if (!bodyManager.getData(bodyIId) || !bodyManager.getData(bodyJId))
                        problems << QString("弹簧 '%1' 的端部运动体不存在（可能已删除）").arg(name);
                }
            }
            else {
                const int targetJointId = refId(Physical_MotionSpring, spring->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None);
                if (targetJointId < 0) {
                    problems << QString("扭转弹簧 '%1' 未选择目标运动副").arg(name);
                }
                else {
                    auto* tj = jointManager.getData(targetJointId);
                    auto* tjBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(tj);
                    auto* tjScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(tj);
                    auto* tjUniversal = dynamic_cast<MechMotionData::mMotionJointUniversal*>(tj);
                    if (!tjBasic && !tjScrew && !tjUniversal)
                        problems << QString("扭转弹簧 '%1' 的目标运动副不存在或类型不支持").arg(name);
                }
            }
        }

        // ---- 3D 接触 ----
        for (int contactId : contactManager.getAllIDs()) {
            auto* contact = contactManager.getData(contactId);
            if (!contact) continue;
            const QString name = contactManager.getNameByID(contactId);

            const int actionBodyId = refId(Physical_MotionContact, contact->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
            const int baseBodyId = refId(Physical_MotionContact, contact->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
            if (actionBodyId < 0 || baseBodyId < 0)
                problems << QString("接触 '%1' 两端体未设置").arg(name);
            else if (!bodyManager.getData(actionBodyId) || !bodyManager.getData(baseBodyId))
                problems << QString("接触 '%1' 的端部运动体不存在（可能已删除）").arg(name);

            // 接触数值稳定性预估：惩罚接触要求 dt < 2*sqrt(m/k)，
            // 超限时接触会静默穿透/发散（不报错但结果无效），提前给出建议步长
            {
                auto* bodyA = (actionBodyId >= 0) ? bodyManager.getData(actionBodyId) : nullptr;
                auto* bodyB = (baseBodyId >= 0) ? bodyManager.getData(baseBodyId) : nullptr;
                if (bodyA && bodyB) {
                    const double mA = bodyA->getMass(), mB = bodyB->getMass();
                    double mEff = 0.0;
                    if (bodyA->getIsFixed() && !bodyB->getIsFixed()) mEff = mB;
                    else if (bodyB->getIsFixed() && !bodyA->getIsFixed()) mEff = mA;
                    else if (!bodyA->getIsFixed() && !bodyB->getIsFixed() && mA > 0 && mB > 0)
                        mEff = mA * mB / (mA + mB);
                    // 预检必须用引擎实际口径的刚度（面板 N/mm ×1000 = 引擎 mN/mm），
                    // 此前用未换算刚度导致 ω 偏小 √1000≈31.6 倍、误放行不稳定步长
                    const double kn = contact->getStiffness() * 1000.0;
                    if (mEff > 0.0 && kn > 0.0) {
                        const double omega = std::sqrt(kn / mEff);
                        auto* scenario = m_viewportModel->getModelData()
                            ->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_scenarioId);
                        const double dt = scenario ? scenario->getStepSize() : 0.0;
                        if (dt > 0.0) {
                            const double dtOmega = dt * omega;
                            if (dtOmega > 1.0) {
                                const double dtSuggest = 0.5 / omega;
                                problems << QString("接触 '%1' 步长过大：dt*sqrt(k/m) = %2（>1），接触会静默穿透/发散，"
                                    "建议步长 <= %3s（或降低刚度/增大质量）")
                                    .arg(name).arg(dtOmega, 0, 'g', 4).arg(dtSuggest, 0, 'g', 3);
                            }
                        }
                    }
                }
            }
        }

        // ---- 驱动（与装配期校验同规则，提前到求解前统一暴露） ----
        for (int driveId : driveManager.getAllIDs()) {
            auto* drive = driveManager.getData(driveId);
            auto* jointDrive = dynamic_cast<MechMotionData::mMotionJointDrive*>(drive);
            if (!jointDrive) continue;
            const QString name = driveManager.getNameByID(driveId);

            const int targetJointId = refId(Physical_MotionDrive, jointDrive->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None);
            auto* tj = jointManager.getData(targetJointId);
            auto* tjBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(tj);
            auto* tjScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(tj);
            const bool drivable = tjScrew
                || (tjBasic && (tjBasic->getType() == JointType::Revolute
                    || tjBasic->getType() == JointType::Translational
                    || tjBasic->getType() == JointType::Cylindrical));
            if (!drivable) {
                problems << QString("驱动 '%1' 的目标运动副 %2 不可驱动（仅支持旋转/移动/柱面/螺纹副）")
                    .arg(name).arg(targetJointId);
            }

            if (jointDrive->getControlMode() == MechMotionData::DriveControlMode::Acceleration) {
                problems << QString("驱动 '%1' 暂不支持加速度控制模式").arg(name);
            }
        }

        // ---- 耦合副（引用类型与参数，提前到求解前统一暴露） ----
        {
            auto& couplingManager = modelData->getManager<MechMotionData::mMotionCouplingManager>(Physical_MotionCoupling);
            for (int couplingId : couplingManager.getAllIDs()) {
                auto* coupling = couplingManager.getData(couplingId);
                if (!coupling) continue;
                const QString name = couplingManager.getNameByID(couplingId);

                auto* cable = dynamic_cast<MechMotionData::mMotionCouplingCable*>(coupling);
                if (!cable) continue;

                const double ratio = cable->getRatio();
                if (ratio == 0.0 || !std::isfinite(ratio))
                    problems << QString("线缆副 '%1' 位移比非法（必须为非零有限值）").arg(name);

                // 内核只接受移动副作线缆传动导引；未配置引用由装配期兜底提示，此处不重复
                const int firstJointId = refId(Physical_MotionCoupling, cable->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
                const int secondJointId = refId(Physical_MotionCoupling, cable->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
                auto checkGuide = [&](int jointId, const QString& side) {
                    if (jointId < 0) return;
                    auto* j = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointManager.getData(jointId));
                    if (!j || j->getType() != JointType::Translational)
                        problems << QString("线缆副 '%1' 的%2运动副 %3 不是移动副（线缆副只接受移动副作传动导引）")
                            .arg(name).arg(side).arg(jointId);
                };
                checkGuide(firstJointId, QStringLiteral("第一"));
                checkGuide(secondJointId, QStringLiteral("第二"));
            }
        }

        return problems;
    }

    void mMotionSolverThread::doTransInput()
    {
        try
        {
            // 注意：这里按照你的要求，全部使用 . 来调用 manager 的方法
            auto& scenarioManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario);
            auto* scenario = scenarioManager.getData(m_scenarioId);
            if (!scenario)
            {
                return;
            }

            emit messageLogged("开始执行前处理数据转换");
            emit progressUpdated(0);

            // 【求解前校验】一次性收集所有引用完整性问题，避免逐次求解逐个暴露
            {
                const QStringList problems = checkMotionDataIntegrity();
                if (!problems.isEmpty()) {
                    for (const QString& problem : problems)
                        emit messageLogged(QString("  [数据校验] %1").arg(problem));
                    throw std::runtime_error(QString("求解前数据校验发现 %1 处问题，已中止求解（详见上方日志）")
                        .arg(problems.size()).toStdString());
                }
                emit messageLogged("求解前数据校验通过");
            }

            // 1. 清空求解器旧数据，防止脏数据累积
            m_sys.parts_map.clear();
            m_sys.joints_map.clear();
            m_sys.markers_map.clear();
            m_sys.motions_map.clear();
            m_sys.forces_map.clear();
            m_sys.torques_map.clear();

            // 获取三大核心管理器 (按值/引用获取，使用 . 操作符)
            auto& bodyManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);
            auto& jointManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
            auto& driveManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);
            auto& couplingManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionCouplingManager>(Physical_MotionCoupling);

            // ==========================================
            // 2. 翻译运动体 (Body -> Part & CM Marker)
            // ==========================================
            for (int bodyId : bodyManager.getAllIDs())
            {
                auto* body = bodyManager.getData(bodyId);
                if (!body) continue;

                motion::adams_part_struct adams_part;

                adams_part.fixed = body->getIsFixed();
                adams_part.mass = body->getMass();

                // 命名 CM Marker 的 ID
                std::string cm_id = "CM_" + std::to_string(bodyId);
                adams_part.cm_marker_id = cm_id;

                // 获取实体全局位姿 (这里已经直接拿了质心坐标作为 Part 的原点)
                QVector3D loc = body->getCenterOfMass();
                adams_part.loc[0] = 0.0;
                adams_part.loc[1] = 0.0;
                adams_part.loc[2] = 0.0;

                // 假定有获取欧拉角或旋转的方法 (目前预设为0)
                adams_part.rot[0] = 0.0;
                adams_part.rot[1] = 0.0;
                adams_part.rot[2] = 0.0;

                QVector<double> inertia = body->getInertia();
                // 转动惯量无效（历史模型 JSON 只存了质量、惯量全 0）时按几何自动补算：
                // 引擎对无效惯量静默用 diag(1,1,1) 兜底，在大尺度模型上与真实值差
                // 数个数量级，转动自由度被瞬间打爆（表现为物体飞出/NaN）。
                if (!body->getIsFixed()
                    && (inertia.size() < 6
                        || inertia[0] < 1e-8 || inertia[1] < 1e-8 || inertia[2] < 1e-8)) {
                    const std::set<int> partIds = refSet(Physical_MotionBody, body->getId(), Physical_Part);
                    //【OCC摘除·引擎化】旧栈 MGeoManager::CalculateGeometricAttr::CalculatePartMass
                    //（mGeometricFeatureAttribute.h 退役头，经 mOCCDocument.h 拉 OCC 链）切引擎
                    // calcCalculatePartMass；密度默认值沿用原签名缺省 7.85e-6 kg/mm³（钢）。
                    const GeoNeutralData::GeoSolidMass geoProps =
                        m_viewportModel->getModelData()->getGeoEngineDataMgr()->engine()
                            .calcCalculatePartMass(std::vector<int>(partIds.begin(), partIds.end()), 7.85e-6);
                    if (geoProps.mass > 1e-12) {
                        inertia = { geoProps.Ixx, geoProps.Iyy, geoProps.Izz,
                                    geoProps.Ixy, geoProps.Ixz, geoProps.Iyz };
                        emit messageLogged(QString("  [质量属性] 运动体 %1（%2）转动惯量未设置，已按几何自动补算：Ixx=%3 Iyy=%4 Izz=%5")
                            .arg(bodyId).arg(body->getName())
                            .arg(geoProps.Ixx, 0, 'g', 6).arg(geoProps.Iyy, 0, 'g', 6).arg(geoProps.Izz, 0, 'g', 6));
                    }
                    else {
                        throw std::runtime_error(QString("运动体 %1（%2）的转动惯量无效且无法按几何补算（无几何或计算失败），已中止求解")
                            .arg(bodyId).arg(body->getName()).toStdString());
                    }
                }
                if (inertia.size() >= 6) {
                    for (int i = 0; i < 6; ++i) {
                        adams_part.inertia[i] = inertia[i];
                    }
                }

                // 初始线速度（全局系；POD 字段必须显式赋值，避免未初始化）
                const QVector3D initVel = body->getInitialVelocity();
                adams_part.vel[0] = initVel.x();
                adams_part.vel[1] = initVel.y();
                adams_part.vel[2] = initVel.z();

                // 初始角速度（全局系；界面 °/s → 引擎 rad/s）
                const QVector3D initOmega = body->getInitialAngularVelocity();
                constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
                adams_part.omega[0] = initOmega.x() * kDegToRad;
                adams_part.omega[1] = initOmega.y() * kDegToRad;
                adams_part.omega[2] = initOmega.z() * kDegToRad;

                // 1. 将 Part 存入容器
                m_sys.parts_map.insert({ std::to_string(bodyId), adams_part });

                // ==========================================
                // 【核心修复】：生成对应的质心 Marker 并存入容器
                // ==========================================
                motion::adams_marker_struct cm_marker;
                cm_marker.part_id = std::to_string(bodyId);

                // 因为 Part 的坐标系原点已经建立在质心上，
                // 所以质心 Marker 相对 Part 的局部坐标偏移为 0
                cm_marker.loc[0] = loc.x();
                cm_marker.loc[1] = loc.y();
                cm_marker.loc[2] = loc.z();

                // 相对姿态也无偏移（求解器把 marker.rot 当作方向向量，
                // 这里显式给 (0,0,1)，与 part.rot=(0,0,0) 的默认姿态一致）
                cm_marker.rot[0] = 0.0;
                cm_marker.rot[1] = 0.0;
                cm_marker.rot[2] = 1.0;

                // 2. 将 CM Marker 存入容器
                m_sys.markers_map.insert({ cm_id, cm_marker });
            }

            emit messageLogged(QString("运动体及质心Marker转换完成: 包含 %1 个刚体").arg(m_sys.parts_map.size()));

            // 大地：基本体/作用体未选（-1）的运动副、标量力/力矩、衬套统一挂到这个固定部件上，
            // 引擎按"固定体"处理——运动副连固定体等价于连大地
            {
                motion::adams_part_struct groundPart{};
                groundPart.fixed = true;
                groundPart.mass = 0.0;
                m_sys.parts_map.insert({ "ground", groundPart });
            }

            // ==========================================
             // 0. 预扫描：找出所有被驱动绑定的运动副 ID
             // ==========================================
            std::set<int> boundJointIds;
            for (int driveId : driveManager.getAllIDs())
            {
                auto* drive = driveManager.getData(driveId);
                auto* jointDrive = dynamic_cast<MechMotionData::mMotionJointDrive*>(drive);
                if (jointDrive) {
                    boundJointIds.insert(refId(Physical_MotionDrive, jointDrive->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None));
                }
            }

            //auto& springManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring);
            //for (int springId : springManager.getAllIDs())
            //{
            //    auto* spring = springManager.getData(springId);
            //    if (spring && refId(Physical_MotionSpring, spring->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None) >= 0) {
            //        boundJointIds.insert(refId(Physical_MotionSpring, spring->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None));
            //    }
            //}

            // ==========================================
            // 3. 翻译运动副 (Joint -> Joint + Markers)
            // ==========================================
            int markerCounter = 1;

            // 显式关节静摩擦约束（引擎新能力，界面不加开关按路径自动启用）：
            // 引擎要求 PSOR + 线性化隐式欧拉，预设均不满足；路径不匹配时保持
            // 默认旧静摩擦力模型，避免引擎装配期抛错中止
            const bool stictionPathOk = !scenario->getUsePreset()
                && scenario->getIntegratorType() == MechMotionData::IntegratorType::EULER_IMPLICIT_LINEARIZED
                && scenario->getSolverType() == MechMotionData::SolverType::PSOR;

            for (int jointId : jointManager.getAllIDs())
            {
                // 被驱动的运动副照常建约束：驱动马达复用其 Marker 对，引擎检测到同对 Marker
                // 后自动关闭马达自带导向（FREE），只保留一条驱动行——无重复约束，
                // 耦合副可引用被驱动副，其反力曲线也可输出。

                auto* jointData = jointManager.getData(jointId);
                if (!jointData) continue;

                auto* jointBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointData);
                auto* jointUniversal = dynamic_cast<MechMotionData::mMotionJointUniversal*>(jointData);
                auto* jointScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(jointData);
                auto* jointPerpendicular = dynamic_cast<MechMotionData::mMotionJointPerpendicular*>(jointData);
                auto* jointConstantVelocity = dynamic_cast<MechMotionData::mMotionJointConstantVelocity*>(jointData);
                if (!jointBasic && !jointUniversal && !jointScrew && !jointPerpendicular && !jointConstantVelocity) continue;

                int actionBodyId = jointBasic ? refId(Physical_MotionJoint, jointBasic->getId(), Physical_MotionBody, MapRelation_MotionActionBody) :
                    (jointUniversal ? refId(Physical_MotionJoint, jointUniversal->getId(), Physical_MotionBody, MapRelation_MotionActionBody) :
                        (jointScrew ? refId(Physical_MotionJoint, jointScrew->getId(), Physical_MotionBody, MapRelation_MotionActionBody) :
                            (jointPerpendicular ? refId(Physical_MotionJoint, jointPerpendicular->getId(), Physical_MotionBody, MapRelation_MotionActionBody) : refId(Physical_MotionJoint, jointConstantVelocity->getId(), Physical_MotionBody, MapRelation_MotionActionBody))));
                int baseBodyId = jointBasic ? refId(Physical_MotionJoint, jointBasic->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) :
                    (jointUniversal ? refId(Physical_MotionJoint, jointUniversal->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) :
                        (jointScrew ? refId(Physical_MotionJoint, jointScrew->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) :
                            (jointPerpendicular ? refId(Physical_MotionJoint, jointPerpendicular->getId(), Physical_MotionBody, MapRelation_MotionBaseBody) : refId(Physical_MotionJoint, jointConstantVelocity->getId(), Physical_MotionBody, MapRelation_MotionBaseBody))));

                // -1 表示大地（marker 挂到固定部件 "ground"）；仅当引用了不存在的体ID（体被删除）才跳过
                const bool actionBodyValid = (actionBodyId < 0) || (bodyManager.getData(actionBodyId) != nullptr);
                const bool baseBodyValid = (baseBodyId < 0) || (bodyManager.getData(baseBodyId) != nullptr);
                if (!actionBodyValid || !baseBodyValid) {
                    emit messageLogged(QString("警告：运动副 '%1'（ID=%2）引用的运动体不存在（可能已删除），该副已跳过")
                        .arg(jointManager.getNameByID(jointId)).arg(jointId));
                    continue;
                }

                QVector3D globalOrigin;
                QVector3D zAxisI;
                QVector3D zAxisJ;
                QVector3D spinAxisI;
                QVector3D spinAxisJ;
                if (jointBasic) {
                    globalOrigin = jointBasic->getOrigin();
                    zAxisI = safeNormalized(jointBasic->getAxis());
                    // 平面副要求两个 marker 法向相反（MotionLinkMatePlane 默认翻转 normal_I），
                    // 因此 marker_J 的 Z 轴需取反，使约束初始满足
                    if (jointBasic->getType() == JointType::Planar) {
                        zAxisJ = -zAxisI;
                    }
                    else {
                        zAxisJ = zAxisI;
                    }
                    spinAxisI = zAxisI;
                    spinAxisJ = zAxisJ;
                }
                else if (jointUniversal) {
                    globalOrigin = jointUniversal->getOrigin();
                    zAxisI = safeNormalized(jointUniversal->getActionBodyPinVector());
                    zAxisJ = safeNormalized(jointUniversal->getBaseBodyPinVector());
                    spinAxisI = safeNormalized(jointUniversal->getActionBodySpinVector());
                    spinAxisJ = safeNormalized(jointUniversal->getBaseBodySpinVector());
                    //zAxisJ = zAxisI;
                }
                else if (jointScrew) {
                    globalOrigin = jointScrew->getOrigin();
                    zAxisI = safeNormalized(jointScrew->getAxis());
                    zAxisJ = zAxisI;
                    spinAxisI = zAxisI;
                    spinAxisJ = zAxisJ;
                }
                else if (jointPerpendicular) {
                    globalOrigin = jointPerpendicular->getOrigin();
                    zAxisI = safeNormalized(jointPerpendicular->getActionBodyVector());
                    zAxisJ = safeNormalized(jointPerpendicular->getBaseBodyVector());
                    spinAxisI = zAxisI;
                    spinAxisJ = zAxisJ;
                }
                else if (jointConstantVelocity) {
                    globalOrigin = jointConstantVelocity->getOrigin();
                    zAxisI = safeNormalized(jointConstantVelocity->getActionBodyVector());
                    zAxisJ = safeNormalized(jointConstantVelocity->getBaseBodyVector());
                    if (QVector3D::dotProduct(zAxisI, zAxisJ) < 0.0)
                    {
                        zAxisJ = -zAxisJ;
                    }
                    spinAxisI = zAxisI;
                    spinAxisJ = zAxisJ;
                }

                // 3.1 & 3.2 生成 Marker I 和 Marker J
                // ----------------------------------------------------
                // 拿到用户在 UI 设定的真实矢量 (这就是 Marker 的局部 Z 轴)
                /*
                // 2. 构建绝对确定的局部正交坐标系 (X, Y, Z 基底)
                QVector3D upVector(0.0, 1.0, 0.0);
                // 防呆设计：如果 Z 轴恰好和 Y 轴平行，改用 X 轴作为参考
                if (std::abs(QVector3D::dotProduct(zAxis, upVector)) > 0.999) {
                    upVector = QVector3D(1.0, 0.0, 0.0);
                }

                QVector3D xAxis = QVector3D::crossProduct(upVector, zAxis).normalized();
                QVector3D yAxis = QVector3D::crossProduct(zAxis, xAxis).normalized();

                // 3. 将这三个轴向向量填入 3x3 旋转矩阵 (DCM)
                // 注意：这里的排列方式是把局部轴的全局投影作为矩阵的列
                double R11 = xAxis.x(), R12 = yAxis.x(), R13 = zAxis.x();
                double R21 = xAxis.y(), R22 = yAxis.y(), R23 = zAxis.y();
                double R31 = xAxis.z(), R32 = yAxis.z(), R33 = zAxis.z();

                // 4. 【核心数学】：从旋转矩阵提取 Z-X-Z (3-1-3) 欧拉角
                double q1 = 0.0, q2 = 0.0, q3 = 0.0; // 分别对应第一次绕Z, 绕X, 第二次绕Z的角度 (弧度)

                // 检查是否发生万向节死锁 (Gimbal Lock)
                if (R33 < 0.999999 && R33 > -0.999999) {
                    // 正常情况：直接利用三角函数关系反推
                    q1 = std::atan2(R13, -R23);
                    q2 = std::acos(R33);
                    q3 = std::atan2(R31, R32);
                }
                else {
                    // 奇异点情况 (Singularity)：Z轴和初始Z轴共线或反向
                    if (R33 >= 0.999999) {
                        // beta = 0
                        q1 = std::atan2(R21, R11);
                        q2 = 0.0;
                        q3 = 0.0;
                    }
                    else {
                        // beta = PI
                        q1 = std::atan2(R21, R11);
                        q2 = 3.14159265358979323846; // M_PI
                        q3 = 0.0;
                    }
                }
                */
                motion::adams_marker_struct marker_i;
                marker_i.part_id = (actionBodyId < 0) ? "ground" : std::to_string(actionBodyId);
                //QVector3D loc_i = globalOrigin - actionBody->getCenterOfMass();
                QVector3D loc_i = globalOrigin;
                marker_i.loc[0] = loc_i.x(); marker_i.loc[1] = loc_i.y(); marker_i.loc[2] = loc_i.z();

                // 赋值真实的欧拉角
                marker_i.rot[0] = zAxisI.x();
                marker_i.rot[1] = zAxisI.y();
                marker_i.rot[2] = zAxisI.z();

                std::string mk_i_id = "MK_" + std::to_string(markerCounter++);
                m_sys.markers_map.insert({ mk_i_id, marker_i });

                // Marker J 同理
                motion::adams_marker_struct marker_j;
                marker_j.part_id = (baseBodyId < 0) ? "ground" : std::to_string(baseBodyId);
                //QVector3D loc_j = globalOrigin - baseBody->getCenterOfMass();
                QVector3D loc_j = globalOrigin;
                marker_j.loc[0] = loc_j.x(); marker_j.loc[1] = loc_j.y(); marker_j.loc[2] = loc_j.z();

                // 初始状态下，两者的姿态是一致的
                marker_j.rot[0] = zAxisJ.x();
                marker_j.rot[1] = zAxisJ.y();
                marker_j.rot[2] = zAxisJ.z();

                std::string mk_j_id = "MK_" + std::to_string(markerCounter++);
                m_sys.markers_map.insert({ mk_j_id, marker_j });

                // ----------------------------------------------------
                // 3.3 生成 Joint
                // ----------------------------------------------------
                std::shared_ptr<motion::adams_joint_struct> adams_joint;
                JointType jointType = JointType::Revolute;
                if (jointBasic)
                    jointType = jointBasic->getType();
                else if (jointUniversal)
                    jointType = jointUniversal->getType();
                else if (jointScrew)
                    jointType = jointScrew->getType();
                else if (jointPerpendicular)
                    jointType = jointPerpendicular->getType();
                else if (jointConstantVelocity)
                    jointType = jointConstantVelocity->getType();

                if (jointType == JointType::Universal && jointUniversal) {
                    auto universal_joint = make_shared<motion::adams_universal_joint_struct>();
                    universal_joint->axial_I.Set(spinAxisI.x(), spinAxisI.y(), spinAxisI.z());
                    universal_joint->axial_J.Set(spinAxisJ.x(), spinAxisJ.y(), spinAxisJ.z());
                    universal_joint->has_axial_I = true;
                    universal_joint->has_axial_J = true;
                    adams_joint = universal_joint;
                }
                else if (jointType == JointType::Screw && jointScrew) {
                    auto screw_joint = make_shared<motion::adams_screw_joint_struct>();
                    screw_joint->pitch = jointScrew->getLead();
                    screw_joint->has_pitch = true;
                    adams_joint = screw_joint;
                }
                else {
                    adams_joint = make_shared<motion::adams_joint_struct>();
                }
                adams_joint->marker_I = mk_i_id;
                adams_joint->marker_J = mk_j_id;

                //if (jointId == 1)
                //{
                //    adams_joint->has_limit = true;
                //    adams_joint->lower_limit_enabled = true;
                //    adams_joint->upper_limit_enabled = true;
                //    adams_joint->lower_limit = -0.17;
                //    adams_joint->upper_limit = 0.17;
                //}

                adams_joint->id = std::to_string(jointId);
                fillJointFrictionAndLimit(jointData, *adams_joint);

                // 静摩擦约束随摩擦启用自动下发：引擎仅支持移动/旋转/圆柱副，
                // 其余类型保持旧力模型
                adams_joint->use_stiction_constraint = adams_joint->has_friction
                    && stictionPathOk
                    && (jointType == JointType::Revolute
                        || jointType == JointType::Translational
                        || jointType == JointType::Cylindrical);

                // 引擎尚无对应约束分支的运动副类型：显式中止，避免约束被静默丢弃后算出错误结果
                if (jointType == JointType::Line || jointType == JointType::Centripetal) {
                    throw std::runtime_error(QString("运动副 %1（%2）暂不支持求解，已中止")
                        .arg(jointId)
                        .arg(jointType == JointType::Line ? QStringLiteral("直线副") : QStringLiteral("向心约束"))
                        .toStdString());
                }

                switch (jointType) {
                case JointType::Revolute: adams_joint->type = "REVOLUTE"; break;
                case JointType::Translational: adams_joint->type = "TRANSLATIONAL"; break;
                case JointType::Cylindrical: adams_joint->type = "CYLINDRICAL"; break;
                case JointType::Spherical: adams_joint->type = "SPHERICAL"; break;
                case JointType::Fixed: adams_joint->type = "FIXED"; break;
                case JointType::Universal: adams_joint->type = "UNIVERSAL"; break;
                case JointType::PointPlane: adams_joint->type = "POINTPLANE"; break;
                case JointType::Line: adams_joint->type = "LINE"; break;
                case JointType::Parallel: adams_joint->type = "PARALLEL"; break;
                case JointType::Perpendicular: adams_joint->type = "PERPENDICULAR"; break;
                case JointType::Direction: adams_joint->type = "ORIENTATION"; break;
                case JointType::Centripetal: adams_joint->type = "CENTRIPETAL"; break;
                case JointType::ConstantVelocity: adams_joint->type = "CONVEL"; break;
                case JointType::Hooke: adams_joint->type = "HOOKE"; break;
                case JointType::Screw: adams_joint->type = "SCREW"; break;
                case JointType::Planar: adams_joint->type = "PLANAR"; break;
                case JointType::Coupling: adams_joint->type = "COUPLING"; break;
                case JointType::Gear: adams_joint->type = "GEAR"; break;
                case JointType::PointLine: adams_joint->type = "POINTLINE"; break;
                case JointType::PointOnSurface: adams_joint->type = "POINTONSURFACE"; break;
                case JointType::PointOnCurve: adams_joint->type = "POINTONCURVE"; break;
                case JointType::CurveOnCurve: adams_joint->type = "CURVEONCURVE"; break;
                default: adams_joint->type = "REVOLUTE"; break;
                }

                // 点在面上副：基本体离散曲面网格（COM 局部系，与接触网格同约定）
                if (jointType == JointType::PointOnSurface) {
                    const int baseBodyId = refId(Physical_MotionJoint, jointData->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                    if (baseBodyId < 0 || !bodyManager.getData(baseBodyId)) {
                        throw std::runtime_error(QString("点在面上副 %1 未选择基本体（曲面宿主），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    auto* baseBody = bodyManager.getData(baseBodyId);
                    auto& geoEngine = m_viewportModel->getModelData()->getGeoEngineDataMgr()->engine();
                    adams_joint->has_surface = true;
                    // 用户拾取了约束面则只离散选中面，否则回退基本体整体网格。
                    // 专用紧凑构建：直接按 facePoints 取选中面的三角形，只收用到的顶点，
                    // 不走接触网格路径（那条路会把整个 part 的点都倒进 nodes）。
                    //【OCC摘除·引擎化】旧栈 dset->getGeoDiscreteData() 改走引擎 E-39
                    // partTessellation（部件级网格，facePoints 键=中性面 ID，与拾取同源）。
                    std::set<int> faceFilter;
                    if (auto* posJoint = dynamic_cast<MechMotionData::mMotionJointPointOnSurface*>(jointData)) {
                        const auto& fids = posJoint->getSurfaceFaceIds();
                        faceFilter.insert(fids.begin(), fids.end());
                    }
                    const QVector3D meshOffset = baseBody->getCenterOfMass();
                    {
                        std::unordered_map<int, int> remap;
                        for (int partId : refSet(Physical_MotionBody, baseBody->getId(), Physical_Part)) {
                            EventDispatcher::mGeoTessellationRequest tessReq;
                            tessReq.partID = static_cast<uint64_t>(partId);
                            const EventDispatcher::mGeoTessellationResult tess = geoEngine.partTessellation(tessReq);
                            if (!tess.succeeded()) continue;
                            const GeoNeutralData::GN_TessellationMesh& disc = tess.mesh;
                            for (const auto& fk : disc.facePoints) {
                                if (!faceFilter.empty() && !faceFilter.count(fk.first)) continue;
                                int keptTri = 0;
                                const auto& idx = fk.second;
                                for (size_t i = 0; i + 2 < idx.size(); i += 3) {
                                    auto p0 = disc.globalPoints.find(idx[i]);
                                    auto p1 = disc.globalPoints.find(idx[i + 1]);
                                    auto p2 = disc.globalPoints.find(idx[i + 2]);
                                    if (p0 == disc.globalPoints.end() || p1 == disc.globalPoints.end() || p2 == disc.globalPoints.end())
                                        continue;
                                    const double ax = p0->second.x() - meshOffset.x(), ay = p0->second.y() - meshOffset.y(), az = p0->second.z() - meshOffset.z();
                                    const double bx = p1->second.x() - meshOffset.x(), by = p1->second.y() - meshOffset.y(), bz = p1->second.z() - meshOffset.z();
                                    const double cx2 = p2->second.x() - meshOffset.x(), cy2 = p2->second.y() - meshOffset.y(), cz2 = p2->second.z() - meshOffset.z();
                                    // 退化三角形跳过
                                    const double e1x = bx - ax, e1y = by - ay, e1z = bz - az;
                                    const double e2x = cx2 - ax, e2y = cy2 - ay, e2z = cz2 - az;
                                    const double nx = e1y * e2z - e1z * e2y, ny = e1z * e2x - e1x * e2z, nz = e1x * e2y - e1y * e2x;
                                    if (nx * nx + ny * ny + nz * nz < 1e-30) continue;
                                    int n0 = remap.emplace(idx[i], (int)remap.size()).first->second;
                                    int n1 = remap.emplace(idx[i + 1], (int)remap.size()).first->second;
                                    int n2 = remap.emplace(idx[i + 2], (int)remap.size()).first->second;
                                    if ((int)(adams_joint->surface_nodes.size() / 3) <= n0) {
                                        adams_joint->surface_nodes.push_back(ax); adams_joint->surface_nodes.push_back(ay); adams_joint->surface_nodes.push_back(az);
                                    }
                                    if ((int)(adams_joint->surface_nodes.size() / 3) <= n1) {
                                        adams_joint->surface_nodes.push_back(bx); adams_joint->surface_nodes.push_back(by); adams_joint->surface_nodes.push_back(bz);
                                    }
                                    if ((int)(adams_joint->surface_nodes.size() / 3) <= n2) {
                                        adams_joint->surface_nodes.push_back(cx2); adams_joint->surface_nodes.push_back(cy2); adams_joint->surface_nodes.push_back(cz2);
                                    }
                                    adams_joint->surface_elems.push_back(n0);
                                    adams_joint->surface_elems.push_back(n1);
                                    adams_joint->surface_elems.push_back(n2);
                                    ++keptTri;
                                }
                                emit messageLogged(QString("  [面采纳] part=%1 faceTag=%2 采纳三角形=%3")
                                    .arg(partId).arg(fk.first).arg(keptTri));
                                // 拓扑边界：pcurve 采样外轮廓 + 定向内法向（OCC"材料在行走方向
                                // 左侧"约定，inward = N×T，详见引擎 faceBoundaryWalls/E-37）；
                                // 接缝/退化边跳过；孔洞内环不建墙（半平面墙可行域为凸集，
                                // 孔的弦墙会拦到孔对面的合法材料）。
                                //【OCC摘除·引擎化】算法本体已迁 RebuildGeometry（E-37），
                                // 此处消费中性结果：世界坐标节点平移到 COM 局部系后接到
                                // surface_nodes 尾部（与原算法同域），墙索引平移 = 现有节点数。
                                {
                                    const EventDispatcher::mGeoObjectRef faceRef(
                                        GeoNeutralData::GN_ShapeType::Face, fk.first);
                                    const EventDispatcher::mGeoBoundaryWalls walls =
                                        geoEngine.faceBoundaryWalls(faceRef, 0.5);
                                    if (walls.ok && walls.wallCount > 0) {
                                        const int baseIdx = static_cast<int>(
                                            adams_joint->surface_nodes.size() / 3);
                                        for (const auto& n : walls.nodes) {
                                            adams_joint->surface_nodes.push_back(n.x() - meshOffset.x());
                                            adams_joint->surface_nodes.push_back(n.y() - meshOffset.y());
                                            adams_joint->surface_nodes.push_back(n.z() - meshOffset.z());
                                        }
                                        for (int w = 0; w < walls.wallCount; ++w) {
                                            adams_joint->surface_boundary_edges.push_back(
                                                baseIdx + walls.wallEdges[2 * w]);
                                            adams_joint->surface_boundary_edges.push_back(
                                                baseIdx + walls.wallEdges[2 * w + 1]);
                                            adams_joint->surface_boundary_inward.push_back(
                                                walls.inward[w].dx());
                                            adams_joint->surface_boundary_inward.push_back(
                                                walls.inward[w].dy());
                                            adams_joint->surface_boundary_inward.push_back(
                                                walls.inward[w].dz());
                                        }
                                        emit messageLogged(QString("  [面边界] part=%1 faceTag=%2 边界段=%3（点滑出面边界将终止计算，同 UG 语义）")
                                            .arg(partId).arg(fk.first).arg(walls.wallCount));
                                    }
                                }

                            }
                        }
                    }
                    if (adams_joint->surface_elems.empty()) {
                        throw std::runtime_error(QString("点在面上副 %1 的基本体没有可用的离散曲面（无几何或离散失败），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    // 装配数据落盘（demo_POS_test 同格式）：离线复现/回归用
                    {
                        QDir wDir(qApp->applicationDirPath() + "/../WorkingDir");
                        if (wDir.exists()) {
                            QFile posDump(wDir.absoluteFilePath(QString("pos_assembly_%1.txt").arg(jointId)));
                            if (posDump.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                                QTextStream ts(&posDump);
                                ts.setCodec("UTF-8");
                                const auto& nd = adams_joint->surface_nodes;
                                const auto& el = adams_joint->surface_elems;
                                const auto& be = adams_joint->surface_boundary_edges;
                                const auto& iw = adams_joint->surface_boundary_inward;
                                ts << "nodes " << nd.size() << "\n";
                                for (double v : nd) ts << v << " ";
                                ts << "\nelems " << el.size() << "\n";
                                for (int v : el) ts << v << " ";
                                ts << "\nbnd " << be.size() << "\n";
                                for (int v : be) ts << v << " ";
                                ts << "\ninw " << iw.size() << "\n";
                                for (double v : iw) ts << v << " ";
                                ts << "\ngcom " << meshOffset.x() << " " << meshOffset.y() << " " << meshOffset.z() << "\n";
                                posDump.close();
                            }
                        }
                    }
                    // 初始贴合校验：双边刚性约束要求应用点初始在曲面上，距离过大直接中止
                    // （否则约束力会在一步内把体拽向曲面，表现为"直接飞出"）
                    {
                        // 精确点到三角形最近点（Ericson 标准算法，与引擎侧同款）
                        auto closestOnTri = [](const QVector3D& p, const QVector3D& a, const QVector3D& b, const QVector3D& c) -> QVector3D {
                            const QVector3D ab = b - a, ac = c - a, ap = p - a;
                            const float d1 = QVector3D::dotProduct(ab, ap), d2 = QVector3D::dotProduct(ac, ap);
                            if (d1 <= 0 && d2 <= 0) return a;
                            const QVector3D bp = p - b;
                            const float d3 = QVector3D::dotProduct(ab, bp), d4 = QVector3D::dotProduct(ac, bp);
                            if (d3 >= 0 && d4 <= d3) return b;
                            const float vc = d1 * d4 - d3 * d2;
                            if (vc <= 0 && d1 >= 0 && d3 <= 0) return a + ab * (d1 / (d1 - d3));
                            const QVector3D cp = p - c;
                            const float d5 = QVector3D::dotProduct(ab, cp), d6 = QVector3D::dotProduct(ac, cp);
                            if (d6 >= 0 && d5 <= d6) return c;
                            const float vb = d5 * d2 - d1 * d6;
                            if (vb <= 0 && d2 >= 0 && d6 <= 0) return a + ac * (d2 / (d2 - d6));
                            const float va = d3 * d6 - d5 * d4;
                            if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
                            const float denom = 1.0f / (va + vb + vc);
                            return a + ab * (vb * denom) + ac * (vc * denom);
                        };

                        const QVector3D appPt = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointData)
                            ? dynamic_cast<MechMotionData::mMotionJointBasic*>(jointData)->getOrigin()
                            : QVector3D();
                        const QVector3D com = baseBody->getCenterOfMass();
                        const auto& nd = adams_joint->surface_nodes;
                        const auto& el = adams_joint->surface_elems;
                        double best2 = std::numeric_limits<double>::max();
                        double maxEdge2 = 0.0;
                        QVector3D bestTriA, bestTriB, bestTriC, bestQ;
                        for (size_t t = 0; t + 2 < el.size(); t += 3) {
                            const int ia = el[t] * 3, ib = el[t + 1] * 3, ic = el[t + 2] * 3;
                            if (ia < 0 || ib < 0 || ic < 0 || ia + 2 >= (int)nd.size() || ib + 2 >= (int)nd.size() || ic + 2 >= (int)nd.size()) continue;
                            const QVector3D a(nd[ia] + com.x(), nd[ia + 1] + com.y(), nd[ia + 2] + com.z());
                            const QVector3D b(nd[ib] + com.x(), nd[ib + 1] + com.y(), nd[ib + 2] + com.z());
                            const QVector3D cc(nd[ic] + com.x(), nd[ic + 1] + com.y(), nd[ic + 2] + com.z());
                            const QVector3D q = closestOnTri(appPt, a, b, cc);
                            const double d2 = double((appPt - q).lengthSquared());
                            if (d2 < best2) {
                                best2 = d2;
                                bestTriA = a; bestTriB = b; bestTriC = cc; bestQ = q;
                            }
                            maxEdge2 = std::max({ maxEdge2,
                                double((a - b).lengthSquared()),
                                double((b - cc).lengthSquared()),
                                double((cc - a).lengthSquared()) });
                        }
                        const double dist = std::sqrt(best2);
                        // 容差自适应：曲面离散成三角形后有弦高误差（点在真实曲面上、
                        // 离离散网格可差半个三角形边长的弓形高），按网格尺度放行；
                        // 轻微偏差交给约束稳定化逐帧收敛，只有明显不贴合才中止。
                        const double tol = std::max(0.5, 0.5 * std::sqrt(maxEdge2));
                        if (dist > tol) {
                            throw std::runtime_error(QString("点在面上副 %1 的应用点距曲面 %2 mm（网格容差 %3 mm），初始必须贴合在约束面上，已中止求解")
                                .arg(jointId).arg(dist, 0, 'f', 3).arg(tol, 0, 'f', 3).toStdString());
                        }


                    }
                }

                // 点在线上副：基本体离散曲线折线（COM 局部系，与点在面上副曲面网格同约定）
                if (jointType == JointType::PointOnCurve) {
                    auto* pocJoint = dynamic_cast<MechMotionData::mMotionJointPointOnCurve*>(jointData);
                    if (!pocJoint || pocJoint->getCurveEdgeIds().isEmpty()) {
                        throw std::runtime_error(QString("点在线上副 %1 未拾取约束曲线（边），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    // 去重：多选拾取可能重复提交同一条边
                    const QVector<int>& rawEdgeIds = pocJoint->getCurveEdgeIds();
                    const std::set<int> curveEdgeIds(rawEdgeIds.begin(), rawEdgeIds.end());
                    if (baseBodyId < 0 || !bodyManager.getData(baseBodyId)) {
                        throw std::runtime_error(QString("点在线上副 %1 未选择基本体（曲线宿主），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    auto* baseBody = bodyManager.getData(baseBodyId);
                    auto& geoEngine = m_viewportModel->getModelData()->getGeoEngineDataMgr()->engine();

                    // 1) 拾取边 ID → 引擎中性引用（拾取 result_id 即引擎中性 geoId）；
                    //    可解析性校验（curveLength>0）替代旧栈 getEdgeByTag 反查
                    //【OCC摘除·引擎化】原 dset->getEdgeByTag(TopoDS) 退役
                    std::vector<EventDispatcher::mGeoObjectRef> pickedRefs;
                    for (int eid : curveEdgeIds)
                        pickedRefs.emplace_back(EventDispatcher::mGeoObjectRef(
                            GeoNeutralData::GN_ShapeType::Edge, eid));
                    size_t resolvedEdges = 0;
                    for (const auto& ref : pickedRefs) {
                        if (geoEngine.curveLength(ref) > 0.0)
                            ++resolvedEdges;
                    }
                    if (resolvedEdges != curveEdgeIds.size()) {
                        throw std::runtime_error(QString("点在线上副 %1 的约束边不属于基本体几何（可能已删除或属于其他体），已中止求解")
                            .arg(jointId).toStdString());
                    }

                    // 2) 按弦高容差精确采样每条边（世界坐标，引擎 E-35）
                    std::vector<std::vector<QVector3D>> chains;
                    double maxEdgeLen = 0.0;
                    for (const auto& ref : pickedRefs) {
                        std::vector<QVector3D> pts;
                        if (!sampleEdgePolyline(ref, geoEngine, pts)) {
                            throw std::runtime_error(QString("点在线上副 %1 的约束边离散失败（可能为退化边），已中止求解")
                                .arg(jointId).toStdString());
                        }
                        maxEdgeLen = std::max(maxEdgeLen, double((pts.back() - pts.front()).length()));
                        chains.push_back(std::move(pts));
                    }

                    // 3) 按共端点串接成一条折线（等价 UG Join Curves；不连续则中止）
                    std::vector<QVector3D> poly;
                    bool closed = false;
                    const double stitchTol = std::max(1e-4, 0.01 * maxEdgeLen);
                    if (!stitchEdgeChains(chains, stitchTol, poly, closed)) {
                        throw std::runtime_error(QString("点在线上副 %1 的约束边不构成连续曲线（边之间未共享端点），已中止求解")
                            .arg(jointId).toStdString());
                    }

                    // 4) 写入 COM 局部系折线
                    const QVector3D meshOffset = baseBody->getCenterOfMass();
                    adams_joint->has_curve = true;
                    adams_joint->curve_nodes.reserve(poly.size() * 3);
                    double totalLen = 0.0;
                    for (size_t i = 0; i < poly.size(); ++i) {
                        adams_joint->curve_nodes.push_back(poly[i].x() - meshOffset.x());
                        adams_joint->curve_nodes.push_back(poly[i].y() - meshOffset.y());
                        adams_joint->curve_nodes.push_back(poly[i].z() - meshOffset.z());
                        if (i > 0) totalLen += double((poly[i] - poly[i - 1]).length());
                    }
                    emit messageLogged(QString("  [曲线采纳] 点在线上副 %1：%2 段边 -> %3 个折线点（%4，总长 %5 mm）")
                        .arg(jointId).arg(chains.size()).arg(poly.size())
                        .arg(closed ? QStringLiteral("闭合") : QStringLiteral("开链"))
                        .arg(totalLen, 0, 'f', 2));

                    // 装配数据落盘（同 pos_assembly 格式）：离线复现/回归用
                    {
                        QDir wDir(qApp->applicationDirPath() + "/../WorkingDir");
                        if (wDir.exists()) {
                            QFile ptcvDump(wDir.absoluteFilePath(QString("ptcv_assembly_%1.txt").arg(jointId)));
                            if (ptcvDump.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                                QTextStream ts(&ptcvDump);
                                ts.setCodec("UTF-8");
                                ts << "nodes " << adams_joint->curve_nodes.size() << "\n";
                                for (double v : adams_joint->curve_nodes) ts << v << " ";
                                ts << "\nclosed " << (closed ? 1 : 0) << "\n";
                                ts << "gcom " << meshOffset.x() << " " << meshOffset.y() << " " << meshOffset.z() << "\n";
                                ptcvDump.close();
                            }
                        }
                    }

                    // 初始贴合校验：双边刚性约束要求应用点初始在曲线上，距离过大直接中止
                    // （否则约束力会在一步内把体拽向曲线，表现为"直接飞出"）。
                    // 容差按弦高量级放行：采样弓高 ≤ 千分之一边长，取总长的 0.2%（下限 0.1）。
                    {
                        const QVector3D appPt = jointBasic ? jointBasic->getOrigin() : QVector3D();
                        const double dist = pointToPolylineDist(appPt, poly);
                        const double tol = std::max(0.1, 0.002 * totalLen);
                        if (dist > tol) {
                            throw std::runtime_error(QString("点在线上副 %1 的应用点距曲线 %2 mm（容差 %3 mm），初始必须贴合在约束曲线上，已中止求解")
                                .arg(jointId).arg(dist, 0, 'f', 3).arg(tol, 0, 'f', 3).toStdString());
                        }
                    }
                }

                // 线在线上副：动作体/基本体两侧离散曲线折线（各自 COM 局部系，同约定）
                if (jointType == JointType::CurveOnCurve) {
                    auto* cvcvJoint = dynamic_cast<MechMotionData::mMotionJointCurveOnCurve*>(jointData);
                    if (!cvcvJoint || cvcvJoint->getCurveIEdgeIds().isEmpty() || cvcvJoint->getCurveJEdgeIds().isEmpty()) {
                        throw std::runtime_error(QString("线在线上副 %1 未拾取双侧约束曲线（边），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    if (actionBodyId < 0 || !bodyManager.getData(actionBodyId)) {
                        throw std::runtime_error(QString("线在线上副 %1 未选择动作体（曲线I宿主），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    if (baseBodyId < 0 || !bodyManager.getData(baseBodyId)) {
                        throw std::runtime_error(QString("线在线上副 %1 未选择基本体（曲线J宿主），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    auto* actionBody = bodyManager.getData(actionBodyId);
                    auto* baseBody = bodyManager.getData(baseBodyId);
                    auto& geoEngine = m_viewportModel->getModelData()->getGeoEngineDataMgr()->engine();

                    // 通用：拾取边 → 中性引用、采样、串接、转 COM 局部系
                    //【OCC摘除·引擎化】原 dset->getEdgeByTag(TopoDS) 反查退役，
                    //可解析性校验（curveLength>0）等价替代（拾取 ID 即引擎中性 geoId）
                    auto buildCurvePolyline = [&](const QVector<int>& edgeIds, MechMotionData::mMotionBody* hostBody,
                                                  const QString& sideName) -> std::vector<QVector3D> {
                        std::vector<EventDispatcher::mGeoObjectRef> pickedRefs;
                        for (int eid : edgeIds)
                            pickedRefs.emplace_back(EventDispatcher::mGeoObjectRef(
                                GeoNeutralData::GN_ShapeType::Edge, eid));
                        size_t resolvedEdges = 0;
                        for (const auto& ref : pickedRefs) {
                            if (geoEngine.curveLength(ref) > 0.0)
                                ++resolvedEdges;
                        }
                        if (resolvedEdges != edgeIds.size()) {
                            throw std::runtime_error(QString("线在线上副 %1 的%2约束边不属于%3几何（可能已删除或属于其他体），已中止求解")
                                .arg(jointId).arg(sideName).arg(sideName).toStdString());
                        }

                        std::vector<std::vector<QVector3D>> chains;
                        double maxEdgeLen = 0.0;
                        for (const auto& ref : pickedRefs) {
                            std::vector<QVector3D> pts;
                            if (!sampleEdgePolyline(ref, geoEngine, pts)) {
                                throw std::runtime_error(QString("线在线上副 %1 的%2约束边离散失败（可能为退化边），已中止求解")
                                    .arg(jointId).arg(sideName).toStdString());
                            }
                            maxEdgeLen = std::max(maxEdgeLen, double((pts.back() - pts.front()).length()));
                            chains.push_back(std::move(pts));
                        }

                        std::vector<QVector3D> poly;
                        bool closed = false;
                        const double stitchTol = std::max(1e-4, 0.01 * maxEdgeLen);
                        if (!stitchEdgeChains(chains, stitchTol, poly, closed)) {
                            throw std::runtime_error(QString("线在线上副 %1 的%2约束边不构成连续曲线（边之间未共享端点），已中止求解")
                                .arg(jointId).arg(sideName).toStdString());
                        }

                        const QVector3D meshOffset = hostBody->getCenterOfMass();
                        std::vector<QVector3D> localPoly;
                        localPoly.reserve(poly.size());
                        for (const auto& p : poly)
                            localPoly.push_back(p - meshOffset);
                        return localPoly;
                    };

                    const std::vector<QVector3D> polyI = buildCurvePolyline(
                        cvcvJoint->getCurveIEdgeIds(), actionBody, QStringLiteral("动作体侧"));
                    const std::vector<QVector3D> polyJ = buildCurvePolyline(
                        cvcvJoint->getCurveJEdgeIds(), baseBody, QStringLiteral("基本体侧"));

                    // 允许滑动（UG Allow Sliding）：数据类 → 引擎
                    adams_joint->allow_sliding = cvcvJoint->getAllowSliding();

                    // 采样表（点/切向/曲率/速度，10 分量/点，COM 局部系）
                    // 引擎 ParserAdams 的 CURVEONCURVE 分支只读 curve_samples_I/J，
                    // 不读 curve_nodes_I/J（折线）。自由落体的根因就是这里没填。
                    //【OCC摘除·引擎化】原 BRepAdaptor+GCPnts 直调改走引擎求值服务：
                    // E-15 edgeEndPoint（闭合检测）+ E-06 curveLength（采样密度）
                    // + E-36 edgeSamplesUniformAbscissa（等弧长采样：点/单位切向/曲率向量）。
                    auto fillSamples = [&](const QVector<int>& edgeIds, MechMotionData::mMotionBody* hostBody,
                                           std::vector<double>& out, bool& outClosed) {
                        out.clear();
                        const QVector3D com = hostBody->getCenterOfMass();
                        for (int eid : edgeIds) {
                            const EventDispatcher::mGeoObjectRef edgeRef(
                                GeoNeutralData::GN_ShapeType::Edge, eid);
                            const double len = geoEngine.curveLength(edgeRef);
                            if (!(len > 0.0)) continue;
                            // 闭合边检测：首尾点重合（E-15）
                            const auto p0 = geoEngine.edgeEndPoint(edgeRef, true);
                            const auto p1 = geoEngine.edgeEndPoint(edgeRef, false);
                            const double dx = p0.x() - p1.x();
                            const double dy = p0.y() - p1.y();
                            const double dz = p0.z() - p1.z();
                            if (dx * dx + dy * dy + dz * dz < 1e-14) outClosed = true;
                            // 按弧长均匀采样（不是参数均匀！B-样条参数≠弧长，
                            // 参数均匀采样导致 Hermite 插值切向在跨点时不连续，
                            // 消元 G1=L·tA·u 跳变 → Newton 不收敛）
                            const int N = std::max(32, std::min(2048, (int)(len / 0.02)));
                            const auto samples = geoEngine.edgeSamplesUniformAbscissa(edgeRef, N);
                            for (const auto& s : samples) {
                                if (!s.ok) continue;  // 奇异参数点占位跳过（等价原 |D1| 守卫）
                                // COM 局部系
                                out.push_back(s.point.x() - com.x());
                                out.push_back(s.point.y() - com.y());
                                out.push_back(s.point.z() - com.z());
                                out.push_back(s.tangent.dx());
                                out.push_back(s.tangent.dy());
                                out.push_back(s.tangent.dz());
                                out.push_back(s.curvature.dx());
                                out.push_back(s.curvature.dy());
                                out.push_back(s.curvature.dz());
                                out.push_back(1.0);  // speed（弧长参数下 =1）
                            }
                        }
                    };

                    fillSamples(cvcvJoint->getCurveIEdgeIds(), actionBody,
                                adams_joint->curve_samples_I, adams_joint->curve_I_closed);
                    fillSamples(cvcvJoint->getCurveJEdgeIds(), baseBody,
                                adams_joint->curve_samples_J, adams_joint->curve_J_closed);

                    if (adams_joint->curve_samples_I.size() < 20 || adams_joint->curve_samples_J.size() < 20) {
                        throw std::runtime_error(QString("线在线上副 %1 曲线采样失败（点/切向/曲率表为空或过少），已中止求解")
                            .arg(jointId).toStdString());
                    }
                    emit messageLogged(QString("  [采样表] 线在线上副 %1：动作体侧 %2 点，基本体侧 %3 点（10分量/点）")
                        .arg(jointId)
                        .arg(adams_joint->curve_samples_I.size() / 10)
                        .arg(adams_joint->curve_samples_J.size() / 10));

                    // 写入 COM 局部系折线
                    adams_joint->has_curve_pair = true;
                    adams_joint->curve_nodes_I.reserve(polyI.size() * 3);
                    for (const auto& p : polyI) {
                        adams_joint->curve_nodes_I.push_back(p.x());
                        adams_joint->curve_nodes_I.push_back(p.y());
                        adams_joint->curve_nodes_I.push_back(p.z());
                    }
                    adams_joint->curve_nodes_J.reserve(polyJ.size() * 3);
                    for (const auto& p : polyJ) {
                        adams_joint->curve_nodes_J.push_back(p.x());
                        adams_joint->curve_nodes_J.push_back(p.y());
                        adams_joint->curve_nodes_J.push_back(p.z());
                    }

                    emit messageLogged(QString("  [曲线对采纳] 线在线上副 %1：动作体侧 %2 点，基本体侧 %3 点")
                        .arg(jointId).arg(polyI.size()).arg(polyJ.size()));

                    // 装配数据落盘（同 ptcv_assembly 格式）：离线复现/回归用
                    {
                        QDir wDir(qApp->applicationDirPath() + "/../WorkingDir");
                        if (wDir.exists()) {
                            QFile cvcvDump(wDir.absoluteFilePath(QString("cvcv_assembly_%1.txt").arg(jointId)));
                            if (cvcvDump.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                                QTextStream ts(&cvcvDump);
                                ts.setCodec("UTF-8");
                                ts << "curveI " << adams_joint->curve_nodes_I.size() << "\n";
                                for (double v : adams_joint->curve_nodes_I) ts << v << " ";
                                ts << "\ncurveJ " << adams_joint->curve_nodes_J.size() << "\n";
                                for (double v : adams_joint->curve_nodes_J) ts << v << " ";
                                ts << "\ngcomI " << actionBody->getCenterOfMass().x() << " " << actionBody->getCenterOfMass().y() << " " << actionBody->getCenterOfMass().z() << "\n";
                                ts << "gcomJ " << baseBody->getCenterOfMass().x() << " " << baseBody->getCenterOfMass().y() << " " << baseBody->getCenterOfMass().z() << "\n";
                                cvcvDump.close();
                            }
                        }
                    }

                    // 初始接触校验：两曲线最近点距离过大即中止（否则约束会在一步内把体拽向对方）。
                    // polyI/polyJ 已是各自 COM 局部系，比较前须加回体质心转世界系。
                    {
                        const QVector3D comI = actionBody->getCenterOfMass();
                        const QVector3D comJ = baseBody->getCenterOfMass();

                        // 段-段最近距离（Ericson 标准算法），同时记录最优段索引
                        auto segSegDist = [](const QVector3D& a, const QVector3D& b,
                                             const QVector3D& c, const QVector3D& d) {
                            const QVector3D ab = b - a, cd = d - c, ac = c - a;
                            const double d1 = QVector3D::dotProduct(ab, ac);
                            const double d2 = QVector3D::dotProduct(cd, ac);
                            const double d3 = QVector3D::dotProduct(ab, ab);
                            const double d4 = QVector3D::dotProduct(cd, cd);
                            const double d5 = QVector3D::dotProduct(ab, cd);
                            double s = 0, t = 0;
                            if (d3 > 1e-20 && d4 > 1e-20) {
                                const double denom = d3 * d4 - d5 * d5;
                                if (std::abs(denom) > 1e-20) {
                                    s = (d5 * d2 - d4 * d1) / denom;
                                    t = (d3 * d2 - d5 * d1) / denom;
                                }
                            }
                            s = std::max(0.0, std::min(1.0, s));
                            t = std::max(0.0, std::min(1.0, t));
                            const QVector3D closestI = a + ab * float(s);
                            const QVector3D closestJ = c + cd * float(t);
                            return double((closestI - closestJ).length());
                        };

                        double minDist = std::numeric_limits<double>::max();
                        size_t bestI = 0, bestJ = 0;
                        for (size_t i = 0; i + 1 < polyI.size(); ++i) {
                            const QVector3D a = polyI[i] + comI, b = polyI[i + 1] + comI;
                            for (size_t j = 0; j + 1 < polyJ.size(); ++j) {
                                const QVector3D c = polyJ[j] + comJ, d = polyJ[j + 1] + comJ;
                                const double dd = segSegDist(a, b, c, d);
                                if (dd < minDist) {
                                    minDist = dd;
                                    bestI = i;
                                    bestJ = j;
                                }
                            }
                        }

                        // 折线弦相对真实曲线有弓高内凹，两条相切曲线在接触区采样相位
                        // 错位时，段-段距离会混入弦高量级的假距离（大半径圆 0.5mm 弦高曾
                        // 放大出 7.4mm 假距离，误杀贴合算例）。补算"每侧顶点到对方折线"
                        // 的距离取三者最小——真实相切时每侧曲线顶点必然贴到对方折线上，
                        // 该度量不受两侧采样相位错位影响，残余误差仅对方单侧弦高。
                        auto pointToPolyDist = [](const QVector3D& p, const std::vector<QVector3D>& poly,
                                                  const QVector3D& off) {
                            double best = std::numeric_limits<double>::max();
                            for (size_t k = 0; k + 1 < poly.size(); ++k) {
                                const QVector3D a = poly[k] + off, b = poly[k + 1] + off;
                                const QVector3D ab = b - a;
                                const double len2 = QVector3D::dotProduct(ab, ab);
                                double t = 0.0;
                                if (len2 > 1e-20)
                                    t = QVector3D::dotProduct(p - a, ab) / len2;
                                t = std::max(0.0, std::min(1.0, t));
                                best = std::min(best, double((p - (a + ab * float(t))).length()));
                            }
                            return best;
                        };
                        for (const auto& p : polyI)
                            minDist = std::min(minDist, pointToPolyDist(p + comI, polyJ, comJ));
                        for (const auto& p : polyJ)
                            minDist = std::min(minDist, pointToPolyDist(p + comJ, polyI, comI));

                        // 容差：覆盖残余单侧弦高（采样弦高 ≤0.02mm）与建模贴合余量
                        const double tol = 0.2;
                        if (minDist > tol) {
                            throw std::runtime_error(QString("线在线上副 %1 的两曲线最近点距离 %2 mm（容差 %3 mm），初始必须接触，已中止求解")
                                .arg(jointId).arg(minDist, 0, 'f', 3).arg(tol, 0, 'f', 3).toStdString());
                        }
                    }
                }
                m_sys.joints_map.insert({ std::to_string(jointId), adams_joint });
            }
            emit messageLogged(QString("运动副及 Marker 转换完成: %1 个").arg(m_sys.joints_map.size()));

            // ==========================================
            // 3.5 翻译耦合副 (Coupling -> 派生 Joint)
            // ==========================================
            for (int couplingId : couplingManager.getAllIDs())
            {
                auto* coupling = couplingManager.getData(couplingId);
                if (!coupling) continue;

                int firstJointId = refId(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionFirstJoint);
                int secondJointId = refId(Physical_MotionCoupling, coupling->getId(), Physical_MotionJoint, MapRelation_MotionSecondJoint);
                // 实时校验只拦 -1；此处兜底未配置的引用（警告并跳过，装配期异常仍走抛错）
                if (firstJointId < 0 || secondJointId < 0) {
                    emit messageLogged(QString("警告：耦合副 '%1' 未配置完整的运动副引用，该耦合副已跳过")
                        .arg(couplingManager.getNameByID(couplingId)));
                    continue;
                }

                // 在 joints_map 中查找对应已转换的 Joint ID
                auto firstJointIt = m_sys.joints_map.find(std::to_string(firstJointId));
                auto secondJointIt = m_sys.joints_map.find(std::to_string(secondJointId));
                if (firstJointIt == m_sys.joints_map.end() || secondJointIt == m_sys.joints_map.end())
                {
                    // 防静默丢失：耦合约束丢掉后模型照算但结果错误，必须显式中止
                    auto explainMissing = [&](int jointId) -> QString {
                        if (boundJointIds.count(jointId) > 0)
                            return QString("运动副 %1 已被驱动占用（暂不支持驱动与耦合同时作用于同一运动副）").arg(jointId);
                        if (!jointManager.getData(jointId))
                            return QString("运动副 %1 不存在（可能已被删除）").arg(jointId);
                        return QString("运动副 %1 未能转换").arg(jointId);
                    };
                    const QString reason = (firstJointIt == m_sys.joints_map.end())
                        ? explainMissing(firstJointId) : explainMissing(secondJointId);
                    throw std::runtime_error(QString("耦合副 '%1' 无法装配：%2，已中止求解")
                        .arg(couplingManager.getNameByID(couplingId)).arg(reason).toStdString());
                }

                // 复用被耦合运动副各自 action body 上的 Marker I。
                // 注意：运动副的 marker_I 属于 actionBody（运动体），marker_J 属于 baseBody（固定体/ground）。
                // 齿轮齿条副需要连接齿轮（旋转副 actionBody）和齿条（移动副 actionBody），
                // 因此必须取两个运动副的 marker_I，而不是取第二个运动副的 marker_J。
                const std::string& mk_i_id = firstJointIt->second->marker_I;
                const std::string& mk_j_id = secondJointIt->second->marker_I;
                // 检查被耦合运动副是否有效
                if (mk_i_id.empty() || mk_j_id.empty()) {
                    throw std::runtime_error(QString("耦合副 '%1' 引用的运动副 Marker 缺失，已中止求解")
                        .arg(couplingManager.getNameByID(couplingId)).toStdString());
                }

                auto couplingType = coupling->getType();
                std::shared_ptr<motion::adams_joint_struct> adams_coupling_joint;
                std::string couplingTypeStr;

                if (couplingType == MxFunctions::CouplingType::GearRack) {
                    auto gearRack = dynamic_cast<MechMotionData::mMotionCouplingGearRack*>(coupling);
                    if (!gearRack) {
                        throw std::runtime_error(QString("耦合副 '%1' 类型与数据不匹配（齿轮齿条），已中止求解")
                            .arg(couplingManager.getNameByID(couplingId)).toStdString());
                    }

                    auto rackpin_joint = make_shared<motion::adams_rackpin_joint_struct>();
                    rackpin_joint->R = gearRack->getRadius();
                    rackpin_joint->has_R = true;
                    // alpha/beta/phase/checkphase 当前 UI 未提供，保持默认值
                    adams_coupling_joint = rackpin_joint;
                    couplingTypeStr = "RACKPIN";
                }
                else if (couplingType == MxFunctions::CouplingType::Gear) {
                    auto gear = dynamic_cast<MechMotionData::mMotionCouplingGear*>(coupling);
                    if (!gear) {
                        throw std::runtime_error(QString("耦合副 '%1' 类型与数据不匹配（齿轮），已中止求解")
                            .arg(couplingManager.getNameByID(couplingId)).toStdString());
                    }

                    auto gear_joint = make_shared<motion::adams_gear_joint_struct>();
                    const double signedRatio = gear->getGearRatio();
                    gear_joint->tau = std::fabs(signedRatio);
                    gear_joint->has_tau = true;
                    // 传动比符号即啮合方式（纯运动学语义，不依赖两轴几何/半径）：
                    // 负值 = 内啮合（epicyclic，两轮同向），正值 = 外啮合（两轮反向）。
                    gear_joint->epicyclic = signedRatio < 0.0;
                    gear_joint->z1 = static_cast<double>(gear->getToothCount1());
                    gear_joint->z2 = static_cast<double>(gear->getToothCount2());
                    gear_joint->has_z1 = true;
                    gear_joint->has_z2 = true;
                    gear_joint->r1 = gear->getPitchRadius1();
                    gear_joint->r2 = gear->getPitchRadius2();
                    gear_joint->has_r1 = true;
                    gear_joint->has_r2 = true;
                    // alpha/beta/phase/checkphase 当前 UI 未提供，保持默认值。
                    // 面板不提供齿数/分度圆半径输入时，z1/z2/r1/r2 仅为
                    // 从传动比同步出的等效值；约束的速比与旋向严格由带符号
                    // 传动比给出，工作半径只影响反力作用点。
                    adams_coupling_joint = gear_joint;
                    couplingTypeStr = "GEAR";
                }
                else if (couplingType == MxFunctions::CouplingType::Cable) {
                    auto cable = dynamic_cast<MechMotionData::mMotionCouplingCable*>(coupling);
                    if (!cable) {
                        throw std::runtime_error(QString("耦合副 '%1' 类型与数据不匹配（线缆），已中止求解")
                            .arg(couplingManager.getNameByID(couplingId)).toStdString());
                    }

                    // 线缆副引用两个已转换运动副的 ID（joints_map 键为运动副 ID 字符串），
                    // 约束为位移传动 (q2-q20) - ratio*(±1)*(q1-q10) = 0，不依赖 marker
                    auto cable_joint = make_shared<motion::adams_cable_joint_struct>();
                    cable_joint->joint_1 = std::to_string(firstJointId);
                    cable_joint->joint_2 = std::to_string(secondJointId);
                    cable_joint->ratio = cable->getRatio();
                    cable_joint->reverse = cable->getReverse();
                    adams_coupling_joint = cable_joint;
                    couplingTypeStr = "CABLE";
                }
                else {
                    adams_coupling_joint = make_shared<motion::adams_joint_struct>();
                    couplingTypeStr = "COUPLING";
                }

                adams_coupling_joint->id = std::to_string(couplingId);
                adams_coupling_joint->type = couplingTypeStr;
                adams_coupling_joint->marker_I = mk_i_id;
                adams_coupling_joint->marker_J = mk_j_id;

                // 耦合副 ID 与运动副 ID 可能重复，用前缀区分，避免覆盖 joints_map
                std::string jointKey = "CP_" + std::to_string(couplingId);
                m_sys.joints_map.insert({ jointKey, adams_coupling_joint });
            }
            emit messageLogged(QString("耦合副转换完成: %1 个").arg(couplingManager.getAllIDs().size()));

            // ==========================================
                // 4. 翻译驱动 (Drive -> Motion + 自带 Markers)
                // ==========================================
            for (int driveId : driveManager.getAllIDs())
            {
                auto* drive = driveManager.getData(driveId);
                if (!drive) continue;

                auto* jointDrive = dynamic_cast<MechMotionData::mMotionJointDrive*>(drive);
                if (!jointDrive) continue;

                const int targetJointId = refId(Physical_MotionDrive, jointDrive->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None);

                // 与 UG 一致：仅单自由度运动副（旋转/移动/柱面/螺纹）可作为驱动目标。
                // 面板已过滤，此处校验拦截旧工程数据里的非法挂载。
                {
                    auto* tj = jointManager.getData(targetJointId);
                    auto* tjBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(tj);
                    auto* tjScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(tj);
                    const bool drivable = tjScrew
                        || (tjBasic && (tjBasic->getType() == JointType::Revolute
                            || tjBasic->getType() == JointType::Translational
                            || tjBasic->getType() == JointType::Cylindrical));
                    if (!drivable) {
                        throw std::runtime_error(QString("驱动 '%1' 的目标运动副 %2 不可驱动（仅支持旋转/移动/柱面/螺纹副），已中止求解")
                            .arg(driveManager.getNameByID(driveId)).arg(targetJointId).toStdString());
                    }
                }

                // 复用目标运动副的 Marker 对：驱动"长在"运动副上。引擎 HasJointBetweenMarkers
                // 命中同对 Marker 后自动关闭马达自带导向（FREE），只保留一条驱动行，
                // 运动副约束照常存在——无重复约束，耦合副可引用被驱动副，反力曲线可输出。
                auto targetJointIt = m_sys.joints_map.find(std::to_string(targetJointId));
                if (targetJointIt == m_sys.joints_map.end()) {
                    throw std::runtime_error(QString("驱动 '%1' 的目标运动副 %2 不存在（可能已删除），已中止求解")
                        .arg(driveManager.getNameByID(driveId)).arg(targetJointId).toStdString());
                }

                // ----------------------------------------------------
                // 4.2 生成 Motion 结构体
                // ----------------------------------------------------
                motion::adams_motion_struct adams_motion;
                adams_motion.id = std::to_string(driveId);
                adams_motion.marker_I = targetJointIt->second->marker_I;
                adams_motion.marker_J = targetJointIt->second->marker_J;
                adams_motion.resolved_joint_id = std::to_string(targetJointId);

                // (顺手帮你清除了原代码里 is_rotation 赋值写了两遍的小 bug)
                adams_motion.is_rotation = (jointDrive->getMotionType() == MechMotionData::DriveMotionType::Rotation);
                adams_motion.is_translation = (jointDrive->getMotionType() == MechMotionData::DriveMotionType::Translation);

                // 加速度控制模式引擎尚无对应马达分支：显式中止，避免两标志皆 false 被静默丢弃
                if (jointDrive->getControlMode() == MechMotionData::DriveControlMode::Acceleration) {
                    throw std::runtime_error(QString("驱动 '%1' 暂不支持加速度控制模式，已中止求解")
                        .arg(driveManager.getNameByID(driveId)).toStdString());
                }
                adams_motion.is_speed = (jointDrive->getControlMode() == MechMotionData::DriveControlMode::Velocity);
                adams_motion.is_position = (jointDrive->getControlMode() == MechMotionData::DriveControlMode::Position);

                // 表格驱动：时间-值表优先于表达式/常数
                if (jointDrive->hasDriveTable()) {
                    const QVector<double> tx = jointDrive->getDriveTableX();
                    const QVector<double> ty = jointDrive->getDriveTableY();
                    adams_motion.x_table.assign(tx.begin(), tx.end());
                    adams_motion.f_table.assign(ty.begin(), ty.end());
                    adams_motion.has_table = true;
                }

                // 解析函数与常数值。转动驱动为"度"语义（°、°/s）：引擎保持纯弧度，
                // 常数值与表格 y 轴在此 ×π/180，表达式文本包装 (expr)·(π/180)
                if (adams_motion.is_rotation) {
                    const double d2r = 3.14159265358979323846 / 180.0;
                    for (double& yv : adams_motion.f_table) yv *= d2r;

                    QString expr = jointDrive->getExpression();
                    bool isNumeric = false;
                    const double constVal = expr.toDouble(&isNumeric);
                    if (isNumeric) {
                        adams_motion.func_value = constVal * d2r;
                        adams_motion.function_expr = "";
                    }
                    else {
                        adams_motion.func_value = 0.0;
                        adams_motion.function_expr = "(" + expr.toStdString() + ")*"
                            + std::to_string(d2r);
                    }
                }
                else {
                    QString expr = jointDrive->getExpression();
                    bool isNumeric;
                    double constVal = expr.toDouble(&isNumeric);
                    if (isNumeric) {
                        adams_motion.func_value = constVal;
                        adams_motion.function_expr = ""; // 置空表示是常数值
                    }
                    else {
                        adams_motion.func_value = 0.0;
                        adams_motion.function_expr = expr.toStdString();
                    }
                }

                m_sys.motions_map.insert({ std::to_string(driveId), adams_motion });
            }
            emit messageLogged(QString("驱动及专属 Marker 装配完成: %1 个").arg(m_sys.motions_map.size()));

            // ==========================================
            // 5. 翻译弹簧 (Spring -> ADMSpring + 专属 Markers)
            // ==========================================
            auto& springManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionSpringManager>(Physical_MotionSpring);
            for (int springId : springManager.getAllIDs())
            {
                auto* spring = springManager.getData(springId);
                if (!spring) continue;

                MechMotionData::SpringType springType = spring->getType();
                std::string mk_i_id, mk_j_id;

                if (springType == MechMotionData::SpringType::BodyPoint)
                {
                    MechMotionData::mMotionBodyPointSpring* bpSpring = dynamic_cast<MechMotionData::mMotionBodyPointSpring*>(spring);
                    if (!bpSpring) continue;

                    int bodyIId = refId(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                    int bodyJId = refId(Physical_MotionSpring, bpSpring->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                    // 必要性校验：两端体未选/悬空直接中止，不再静默丢弃
                    if (bodyIId < 0 || bodyJId < 0) {
                        throw std::runtime_error(QString("弹簧 '%1' 两端体未设置，已中止求解")
                            .arg(springManager.getNameByID(springId)).toStdString());
                    }
                    if (!bodyManager.getData(bodyIId) || !bodyManager.getData(bodyJId)) {
                        throw std::runtime_error(QString("弹簧 '%1' 的端部运动体不存在（可能已删除），已中止求解")
                            .arg(springManager.getNameByID(springId)).toStdString());
                    }

                    QVector3D localI = bpSpring->getLocalI();
                    QVector3D localJ = bpSpring->getLocalJ();
                    QVector3D eulerI = bpSpring->getLocalRotI().toEulerAngles();
                    QVector3D eulerJ = bpSpring->getLocalRotJ().toEulerAngles();

                    motion::adams_marker_struct marker_i;
                    marker_i.part_id = std::to_string(bodyIId);
                    marker_i.loc[0] = localI.x(); marker_i.loc[1] = localI.y(); marker_i.loc[2] = localI.z();
                    marker_i.rot[0] = eulerI.x(); marker_i.rot[1] = eulerI.y(); marker_i.rot[2] = eulerI.z();

                    mk_i_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_i_id, marker_i });

                    motion::adams_marker_struct marker_j;
                    marker_j.part_id = std::to_string(bodyJId);
                    marker_j.loc[0] = localJ.x(); marker_j.loc[1] = localJ.y(); marker_j.loc[2] = localJ.z();
                    marker_j.rot[0] = eulerJ.x(); marker_j.rot[1] = eulerJ.y(); marker_j.rot[2] = eulerJ.z();

                    mk_j_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_j_id, marker_j });
                }
                else
                {
                    int targetJointId = refId(Physical_MotionSpring, spring->getId(), Physical_MotionJoint, MxFunctions::MapRelation_None);
                    if (targetJointId < 0) {
                        throw std::runtime_error(QString("扭转弹簧 '%1' 未选择目标运动副，已中止求解")
                            .arg(springManager.getNameByID(springId)).toStdString());
                    }

                    auto* originJointBasic = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointManager.getData(targetJointId));
                    auto* originJointScrew = dynamic_cast<MechMotionData::mMotionJointScrew*>(jointManager.getData(targetJointId));
                    auto* originJointUniversal = dynamic_cast<MechMotionData::mMotionJointUniversal*>(jointManager.getData(targetJointId));
                    if (!originJointBasic && !originJointScrew && !originJointUniversal) {
                        throw std::runtime_error(QString("扭转弹簧 '%1' 的目标运动副不存在或类型不支持，已中止求解")
                            .arg(springManager.getNameByID(springId)).toStdString());
                    }

                    int actionBodyId = -1;
                    int baseBodyId = -1;
                    QVector3D globalOrigin;
                    QVector3D zAxis;
                    if (originJointBasic) {
                        actionBodyId = refId(Physical_MotionJoint, originJointBasic->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                        baseBodyId = refId(Physical_MotionJoint, originJointBasic->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                        globalOrigin = originJointBasic->getOrigin();
                        zAxis = safeNormalized(originJointBasic->getAxis());
                    }
                    else if (originJointScrew) {
                        actionBodyId = refId(Physical_MotionJoint, originJointScrew->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                        baseBodyId = refId(Physical_MotionJoint, originJointScrew->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                        globalOrigin = originJointScrew->getOrigin();
                        zAxis = safeNormalized(originJointScrew->getAxis());
                    }
                    else if (originJointUniversal) {
                        actionBodyId = refId(Physical_MotionJoint, originJointUniversal->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                        baseBodyId = refId(Physical_MotionJoint, originJointUniversal->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                        globalOrigin = originJointUniversal->getOrigin();
                        zAxis = safeNormalized(originJointUniversal->getBaseBodySpinVector());
                    }

                    // 生成弹簧 Marker I
                    motion::adams_marker_struct marker_i;
                    marker_i.part_id = std::to_string(actionBodyId);
                    marker_i.loc[0] = globalOrigin.x(); marker_i.loc[1] = globalOrigin.y(); marker_i.loc[2] = globalOrigin.z();
                    marker_i.rot[0] = zAxis.x(); marker_i.rot[1] = zAxis.y(); marker_i.rot[2] = zAxis.z();

                    mk_i_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_i_id, marker_i });

                    // 生成弹簧 Marker J
                    motion::adams_marker_struct marker_j;
                    marker_j.part_id = std::to_string(baseBodyId);
                    marker_j.loc[0] = globalOrigin.x(); marker_j.loc[1] = globalOrigin.y(); marker_j.loc[2] = globalOrigin.z();
                    marker_j.rot[0] = zAxis.x(); marker_j.rot[1] = zAxis.y(); marker_j.rot[2] = zAxis.z();

                    mk_j_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_j_id, marker_j });
                }

                // 生成 ADMSpring
                motion::ADMSpring adms;
                adms.id = std::to_string(springId);
                adms.marker_I = mk_i_id;
                adms.marker_J = mk_j_id;
                adms.model_type = "LINEAR";
                if (springType == MechMotionData::SpringType::Translational)
                    adms.dof_type = "AXIAL";
                else if (springType == MechMotionData::SpringType::Torsional)
                    adms.dof_type = "TORSION";
                else if (springType == MechMotionData::SpringType::BodyPoint)
                    adms.dof_type = "MARKER";

                // 扭簧用户单位为"度"（刚度 N·mm/°、角度 °、曲线 x 轴 °、公式自变量 °）：
                // 引擎全弧度——数值量在此统一换算，公式自变量由引擎按 angle_deg 标志转换。
                const bool isTorsionSpring = (springType == MechMotionData::SpringType::Torsional);
                const double deg2rad = 3.14159265358979323846 / 180.0;
                const double radPerDeg = 180.0 / 3.14159265358979323846;

                double stiffness = spring->getStiffness();
                double freeValue = spring->getFreeValue();
                double installValue = spring->getInstallValue();
                double dampingC = spring->isDampingEnabled() ? spring->getDamping() : 0.0;
                if (isTorsionSpring) {
                    stiffness *= radPerDeg;   // K: N·mm/° → N·mm/rad
                    dampingC *= radPerDeg;    // C: N·mm/(°/s) → N·mm/(rad/s)
                    freeValue *= deg2rad;     // 自由角 ° → rad
                    installValue *= deg2rad;  // 安装角 ° → rad
                }
                // 力单位对齐：面板/JSON 为 N 系（N/mm、N·s/mm、N·mm…），引擎内部
                // 单位制 mm·kg·s 的力单位是 mN——所有力值 ×1000，表达式/表格同
                stiffness *= 1000.0;
                dampingC *= 1000.0;
                double userPreload = spring->getPreload() * 1000.0; // 预紧力/力矩 N 系 → mN 系
                adms.pretesin = 0.0;
                adms.k = stiffness;
                adms.c_linear = dampingC;
                // 预紧力折算到等效自由长度：预拉力使等效自由长度缩短，等效自由长度 = L0 + F/k
                adms.free_length = (std::abs(stiffness) > 1e-12) ? (freeValue + userPreload / stiffness) : freeValue;
                adms.install_length = installValue;

                // 力-位移/转角曲线（扭簧 x 轴 ° → rad）
                QVector<double> tableX = spring->getTableX();
                QVector<double> tableY = spring->getTableY();
                if (!tableX.isEmpty() && tableX.size() == tableY.size())
                {
                    if (isTorsionSpring)
                        for (double& xv : tableX) xv *= deg2rad;
                    adms.x_table.assign(tableX.begin(), tableX.end());
                    // 表格 y 为力/力矩（N 系）→ mN 系
                    adms.f_table.reserve(tableY.size());
                    for (double yv : tableY) adms.f_table.push_back(yv * 1000.0);
                    adms.has_table = true;
                }

                // 力函数。扭簧的公式自变量 x（变形量）/v（变形速度）为"度"语义：
                // 引擎保持纯弧度，导出时把变量文本替换为 (x·180/π)/(v·180/π)
                //（词边界匹配，不会误伤 exp/max/abs/step 等函数名内的字母）
                auto torsionDegVariables = [isTorsionSpring](QString func) {
                    if (!isTorsionSpring || func.isEmpty())
                        return func;
                    static const QRegularExpression rx(QStringLiteral("\\b([xv])\\b"));
                    return func.replace(rx, "(\\1*57.29577951308232)");
                };

                // 力/阻尼函数返回值单位 N 系 → mN 系：整体 ×1000
                auto forceUnitScale = [](QString func) {
                    if (func.isEmpty())
                        return func;
                    return QStringLiteral("(%1)*1000").arg(func);
                };

                QString forceFunc = forceUnitScale(torsionDegVariables(spring->getForceFunction()));
                if (!forceFunc.isEmpty())
                {
                    adms.force_function = forceFunc.toStdString();
                    adms.has_function = true;
                }

                // 阻尼函数（同样按度语义替换自变量）
                QString dampFunc = forceUnitScale(torsionDegVariables(spring->getDampingFunction()));
                if (!dampFunc.isEmpty())
                    adms.damping_function = dampFunc.toStdString();

                // 建模方式按数据决定：曲线表 > 力函数 > 线性。
                // 引擎按 model_type 分发，此前恒为 LINEAR，函数/曲线分支不可达。
                if (adms.has_table)
                    adms.model_type = "TABLE";
                else if (adms.has_function)
                    adms.model_type = "FUNCTION";

                m_sys.springs_map.insert({ adms.id, adms });
            }
            emit messageLogged(QString("弹簧及专属 Marker 装配完成: %1 个").arg(m_sys.springs_map.size()));

            // ----------------------------------------------------
            // 6. 转换用户自定义标记点 (User Markers)
            // ----------------------------------------------------
            auto& markerManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionMarkerManager>(Physical_MotionMarker);
            for (int markerId : markerManager.getAllIDs())
            {
                auto* marker = markerManager.getData(markerId);
                if (!marker) continue;

                motion::adams_marker_struct am;
                am.part_id = std::to_string(refId(Physical_MotionMarker, marker->getId(), Physical_MotionBody, MxFunctions::MapRelation_None));

                QVector3D loc = marker->getLocalPosition();
                am.loc[0] = loc.x(); am.loc[1] = loc.y(); am.loc[2] = loc.z();

                // QQuaternion -> Euler angles (degrees)
                QVector3D euler = marker->getLocalRotation().toEulerAngles();
                am.rot[0] = euler.x(); am.rot[1] = euler.y(); am.rot[2] = euler.z();

                std::string um_id = "UM_" + std::to_string(markerId);
                m_sys.markers_map.insert({ um_id, am });
            }
            emit messageLogged(QString("用户标记点装配完成: %1 个").arg(markerManager.getAllIDs().size()));

            // ==========================================
            // 6. 翻译力元 (VectorForce / ScalarForce -> 求解器结构)
            // ==========================================
            auto& forceElementManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionForceElementManager>(Physical_MotionVectorForce);
            for (int feId : forceElementManager.getAllIDs())
            {
                auto* fe = forceElementManager.getData(feId);
                if (!fe) continue;

                // ---------- 标量力矩（先于标量力检查，避免被 loadType 过滤掉） ----------
                {
                    auto* st = dynamic_cast<MechMotionData::mMotionScalarTorque*>(fe);
                    if (st) {
                        const int stBodyIId = refId(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                        const int stBodyJId = refId(Physical_MotionVectorForce, st->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                        // 必要性校验：未选体/引用悬空直接中止（与驱动装配同风格），不再静默丢弃
                        if (stBodyIId < 0) {
                            throw std::runtime_error(QString("标量力矩 '%1' 未选择作用体，已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).toStdString());
                        }
                        if (!bodyManager.getData(stBodyIId)) {
                            throw std::runtime_error(QString("标量力矩 '%1' 的作用体不存在（可能已删除），已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).toStdString());
                        }
                        if (stBodyJId >= 0 && !bodyManager.getData(stBodyJId)) {
                            throw std::runtime_error(QString("标量力矩 '%1' 的基本体不存在（可能已删除），已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).toStdString());
                        }

                        // 生成 marker_I/J（两点）
                        motion::adams_marker_struct st_mk_i;
                        st_mk_i.part_id = std::to_string(stBodyIId);
                        QVector3D li = st->getLocalI();
                        st_mk_i.loc[0] = li.x(); st_mk_i.loc[1] = li.y(); st_mk_i.loc[2] = li.z();
                        st_mk_i.rot[0] = 0; st_mk_i.rot[1] = 0; st_mk_i.rot[2] = 1;
                        std::string st_mk_i_id = "MK_" + std::to_string(markerCounter++);
                        m_sys.markers_map.insert({ st_mk_i_id, st_mk_i });

                        // marker_J：-1 表示大地，与标量力一致写 "ground"
                        motion::adams_marker_struct st_mk_j;
                        st_mk_j.part_id = (stBodyJId < 0) ? "ground" : std::to_string(stBodyJId);
                        QVector3D lj = st->getLocalJ();
                        st_mk_j.loc[0] = lj.x(); st_mk_j.loc[1] = lj.y(); st_mk_j.loc[2] = lj.z();
                        st_mk_j.rot[0] = 0; st_mk_j.rot[1] = 0; st_mk_j.rot[2] = 1;
                        std::string st_mk_j_id = "MK_" + std::to_string(markerCounter++);
                        m_sys.markers_map.insert({ st_mk_j_id, st_mk_j });

                        motion::ADMScalarTorque admST;
                        admST.id = std::to_string(feId);
                        admST.marker_I = st_mk_i_id;
                        admST.marker_J = st_mk_j_id;
                        admST.axis_mode = static_cast<int>(st->getAxisMode());
                        QVector3D ax = st->getFixedAxis();
                        admST.axis = motion::MotionVector<>(ax.x(), ax.y(), ax.z());
                        admST.magnitude = st->getMagnitude() * 1000.0; // N·mm → mN·mm
                        const QString stFunc = st->getTorqueFunction();
                        admST.torque_function = stFunc.isEmpty()
                            ? std::string() : ("(" + stFunc + ")*1000").toStdString();
                        admST.action_only = st->getActionOnly();

                        m_sys.scalar_torques_map.insert({ admST.id, admST });
                        continue;
                    }
                }

                // ---------- 标量力 ----------
                if (fe->getLoadType() == MechMotionData::VectorLoadType::ScalarForce)
                {
                    auto* sf = dynamic_cast<MechMotionData::mMotionScalarForce*>(fe);
                    if (!sf) continue;
                    int bodyIId = refId(Physical_MotionVectorForce, sf->getId(), Physical_MotionBody, MapRelation_MotionBodyI);
                    int bodyJId = refId(Physical_MotionVectorForce, sf->getId(), Physical_MotionBody, MapRelation_MotionBodyJ);
                    // 必要性校验：未选体/引用悬空直接中止，不再静默丢弃
                    if (bodyIId < 0) {
                        throw std::runtime_error(QString("标量力 '%1' 未选择作用体，已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }

                    auto* bodyI = bodyManager.getData(bodyIId);
                    if (!bodyI) {
                        throw std::runtime_error(QString("标量力 '%1' 的作用体不存在（可能已删除），已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }
                    if (bodyJId >= 0 && !bodyManager.getData(bodyJId)) {
                        throw std::runtime_error(QString("标量力 '%1' 的基本体不存在（可能已删除），已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }

                    // 创建 marker_I（动作体局部点）
                    QVector3D localI = sf->getLocalI();
                    motion::adams_marker_struct marker_i;
                    marker_i.part_id = std::to_string(bodyIId);
                    marker_i.loc[0] = localI.x(); marker_i.loc[1] = localI.y(); marker_i.loc[2] = localI.z();
                    marker_i.rot[0] = 0.0;        marker_i.rot[1] = 0.0;        marker_i.rot[2] = 0.0;
                    std::string mk_i_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_i_id, marker_i });

                    // 创建 marker_J（基本体局部点，-1 表示大地）
                    QVector3D localJ = sf->getLocalJ();
                    motion::adams_marker_struct marker_j;
                    marker_j.part_id = (bodyJId < 0) ? "ground" : std::to_string(bodyJId);
                    marker_j.loc[0] = localJ.x(); marker_j.loc[1] = localJ.y(); marker_j.loc[2] = localJ.z();
                    marker_j.rot[0] = 0.0;        marker_j.rot[1] = 0.0;        marker_j.rot[2] = 0.0;
                    std::string mk_j_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_j_id, marker_j });

                    motion::ADMScalarForce admScalar;
                    admScalar.id = std::to_string(feId);
                    admScalar.marker_I = mk_i_id;
                    admScalar.marker_J = mk_j_id;
                    admScalar.direction_mode = static_cast<int>(sf->getDirectionMode());
                    QVector3D dir = sf->getFixedDirection();
                    admScalar.direction = motion::MotionVector<>(dir.x(), dir.y(), dir.z());
                    admScalar.magnitude = sf->getMagnitude() * 1000.0; // N → mN
                    const QString sfFunc = sf->getForceFunction();
                    admScalar.force_function = sfFunc.isEmpty()
                        ? std::string() : ("(" + sfFunc + ")*1000").toStdString();
                    admScalar.action_only = sf->getActionOnly();

                    m_sys.scalar_forces_map.insert({ admScalar.id, admScalar });
                    continue;
                }

                // ---------- 三点力 ----------
                if (auto* tpf = dynamic_cast<MechMotionData::mMotionThreePointForce*>(fe))
                {
                    // 必要性校验：应用体必选，方向参考两点可为大地
                    const auto requireBody = [&](int bodyId, const char* what) -> void {
                        if (bodyId < 0) {
                            throw std::runtime_error(QString("三点力 '%1' 未选择%2，已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).arg(what).toStdString());
                        }
                        if (!bodyManager.getData(bodyId)) {
                            throw std::runtime_error(QString("三点力 '%1' 的%2不存在（可能已删除），已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).arg(what).toStdString());
                        }
                    };
                    // 应用体必选；出发/目标体 -1 表示大地（合法），仅在其显式指定时校验存在性
                    requireBody(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBodyI), "应用体");
                    const auto requireRefBody = [&](int bodyId, const char* what) -> void {
                        if (bodyId >= 0 && !bodyManager.getData(bodyId)) {
                            throw std::runtime_error(QString("三点力 '%1' 的%2不存在（可能已删除），已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).arg(what).toStdString());
                        }
                    };
                    requireRefBody(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), "目标体");
                    requireRefBody(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionActionBody), "出发体");

                    auto makePointMarker = [&](int bodyId, const QVector3D& localPos) -> std::string {
                        motion::adams_marker_struct mk;
                        mk.part_id = (bodyId < 0) ? "ground" : std::to_string(bodyId);
                        mk.loc[0] = localPos.x(); mk.loc[1] = localPos.y(); mk.loc[2] = localPos.z();
                        mk.rot[0] = 0.0; mk.rot[1] = 0.0; mk.rot[2] = 0.0;
                        std::string mkId = "MK_" + std::to_string(markerCounter++);
                        m_sys.markers_map.insert({ mkId, mk });
                        return mkId;
                    };

                    motion::ADMThreePointForce admTp;
                    admTp.id = std::to_string(feId);
                    admTp.marker_I = makePointMarker(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBodyI), tpf->getLocalI());
                    admTp.marker_origin = makePointMarker(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionActionBody), tpf->getLocalOrigin());
                    admTp.marker_target = makePointMarker(refId(Physical_MotionVectorForce, tpf->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), tpf->getLocalTarget());
                    admTp.magnitude = tpf->getMagnitude() * 1000.0; // N → mN
                    const QString tpFunc = tpf->getForceFunction();
                    admTp.force_function = tpFunc.isEmpty()
                        ? std::string() : ("(" + tpFunc + ")*1000").toStdString();

                    m_sys.three_point_forces_map.insert({ admTp.id, admTp });
                    continue;
                }

                // ---------- 三点力矩 ----------
                if (auto* tpt = dynamic_cast<MechMotionData::mMotionThreePointTorque*>(fe))
                {
                    // 应用体必选；出发/目标体 -1 表示大地（合法），仅在其显式指定时校验存在性
                    if (refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBodyI) < 0) {
                        throw std::runtime_error(QString("三点力矩 '%1' 未选择应用体，已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }
                    if (!bodyManager.getData(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBodyI))) {
                        throw std::runtime_error(QString("三点力矩 '%1' 的应用体不存在（可能已删除），已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }
                    const auto checkRefBody = [&](int bodyId, const char* what) -> void {
                        if (bodyId >= 0 && !bodyManager.getData(bodyId)) {
                            throw std::runtime_error(QString("三点力矩 '%1' 的%2不存在（可能已删除），已中止求解")
                                .arg(forceElementManager.getNameByID(feId)).arg(what).toStdString());
                        }
                    };
                    checkRefBody(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), "目标体");
                    checkRefBody(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionActionBody), "出发体");

                    auto makePointMarker = [&](int bodyId, const QVector3D& localPos) -> std::string {
                        motion::adams_marker_struct mk;
                        mk.part_id = (bodyId < 0) ? "ground" : std::to_string(bodyId);
                        mk.loc[0] = localPos.x(); mk.loc[1] = localPos.y(); mk.loc[2] = localPos.z();
                        mk.rot[0] = 0.0; mk.rot[1] = 0.0; mk.rot[2] = 0.0;
                        std::string mkId = "MK_" + std::to_string(markerCounter++);
                        m_sys.markers_map.insert({ mkId, mk });
                        return mkId;
                    };

                    motion::ADMThreePointTorque admTt;
                    admTt.id = std::to_string(feId);
                    admTt.marker_I = makePointMarker(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBodyI), tpt->getLocalI());
                    admTt.marker_origin = makePointMarker(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionActionBody), tpt->getLocalOrigin());
                    admTt.marker_target = makePointMarker(refId(Physical_MotionVectorForce, tpt->getId(), Physical_MotionBody, MapRelation_MotionBaseBody), tpt->getLocalTarget());
                    admTt.magnitude = tpt->getMagnitude() * 1000.0; // N·mm → mN·mm
                    const QString ttFunc = tpt->getTorqueFunction();
                    admTt.torque_function = ttFunc.isEmpty()
                        ? std::string() : ("(" + ttFunc + ")*1000").toStdString();

                    m_sys.three_point_torques_map.insert({ admTt.id, admTt });
                    continue;
                }

                auto* vf = dynamic_cast<MechMotionData::mMotionVectorForce*>(fe);
                if (!vf) continue;

                int bodyId = refId(Physical_MotionVectorForce, vf->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                // 必要性校验：未选体/引用悬空直接中止，不再静默丢弃
                if (bodyId == -1) {
                    throw std::runtime_error(QString("力元 '%1' 未选择作用体，已中止求解")
                        .arg(forceElementManager.getNameByID(feId)).toStdString());
                }

                auto* body = bodyManager.getData(bodyId);
                if (!body) {
                    throw std::runtime_error(QString("力元 '%1' 的作用体不存在（可能已删除），已中止求解")
                        .arg(forceElementManager.getNameByID(feId)).toStdString());
                }

                // ref_marker 使用质心 Marker
                std::string ref_marker = "CM_" + std::to_string(bodyId);

                // 求解器使用世界坐标系，直接传递全局坐标
                QVector3D actionPt = vf->getActionPoint();
                QVector3D dir = safeNormalized(vf->getDirection());

                double magnitude = 0.0;
                QString magnitudeExpr;
                if (vf->getMagnitudeMode() == MechMotionData::MagnitudeMode::Constant)
                {
                    magnitude = vf->getMagnitude() * 1000.0; // N → mN
                }
                else
                {
                    // 表达式模式：幅值 = f(t)，由引擎侧 MotionForce 调制求值
                    // 返回值 N → mN：整体 ×1000
                    const QString vfExpr = vf->getExpression();
                    magnitudeExpr = vfExpr.isEmpty() ? vfExpr : QStringLiteral("(%1)*1000").arg(vfExpr);
                }

                // 衬套 (Bushing)：独立的 6 自由度黏弹性连接，写入 bushings_map
                if (vf->getLoadType() == MechMotionData::VectorLoadType::Bushing)
                {
                    auto* bushing = dynamic_cast<MechMotionData::mMotionBushing*>(vf);
                    if (!bushing) continue;

                    // marker_I：动作侧（作用体 + 作用点 + 定向矢量）
                    motion::adams_marker_struct marker_i;
                    marker_i.part_id = std::to_string(bodyId);
                    marker_i.loc[0] = actionPt.x(); marker_i.loc[1] = actionPt.y(); marker_i.loc[2] = actionPt.z();
                    marker_i.rot[0] = dir.x();      marker_i.rot[1] = dir.y();      marker_i.rot[2] = dir.z();

                    std::string mk_i_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_i_id, marker_i });

                    // marker_J：基本侧。衬套要求 I/J 同点同向（UG 中基本体只选运动体，
                    // 原点与矢量方向复用动作侧），故直接复用 marker_I 的 loc/rot，仅 part_id 不同。
                    motion::adams_marker_struct marker_j;
                    const int bushingBaseBodyId = refId(Physical_MotionVectorForce, bushing->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                    if (bushingBaseBodyId >= 0 && !bodyManager.getData(bushingBaseBodyId)) {
                        throw std::runtime_error(QString("衬套 '%1' 的基本体不存在（可能已删除），已中止求解")
                            .arg(forceElementManager.getNameByID(feId)).toStdString());
                    }
                    // baseBodyId=-1 表示大地，与标量力/标量力矩一致写 "ground"
                    marker_j.part_id = (bushingBaseBodyId < 0) ? "ground" : std::to_string(bushingBaseBodyId);
                    marker_j.loc[0] = marker_i.loc[0]; marker_j.loc[1] = marker_i.loc[1]; marker_j.loc[2] = marker_i.loc[2];
                    marker_j.rot[0] = marker_i.rot[0]; marker_j.rot[1] = marker_i.rot[1]; marker_j.rot[2] = marker_i.rot[2];

                    std::string mk_j_id = "MK_" + std::to_string(markerCounter++);
                    m_sys.markers_map.insert({ mk_j_id, marker_j });

                    // 填充 adams_bushing_struct
                    motion::adams_bushing_struct admBushing;
                    admBushing.id = std::to_string(feId);
                    admBushing.marker_I = mk_i_id;
                    admBushing.marker_J = mk_j_id;

                    // 力单位对齐：K/C/F0/T0 面板为 N 系（N/mm、N·s/mm、N、N·mm），
                    // 引擎 mN 系——平移与转动分量统一 ×1000
                    QVector<double> k = bushing->getStiffness();
                    for (int i = 0; i < 6 && i < k.size(); ++i) admBushing.K[i] = k[i] * 1000.0;
                    QVector<double> c = bushing->getDamping();
                    for (int i = 0; i < 6 && i < c.size(); ++i) admBushing.C[i] = c[i] * 1000.0;
                    QVector<double> f0 = bushing->getPreloadForce();
                    for (int i = 0; i < 3 && i < f0.size(); ++i) admBushing.F0[i] = f0[i] * 1000.0;
                    QVector<double> t0 = bushing->getPreloadTorque();
                    for (int i = 0; i < 3 && i < t0.size(); ++i) admBushing.T0[i] = t0[i] * 1000.0;

                    // 偏置（预位移 D0），仅当应用偏置时生效
                    if (bushing->getApplyOffset())
                    {
                        QVector3D offset = bushing->getOffset();
                        admBushing.D0[0] = offset.x();
                        admBushing.D0[1] = offset.y();
                        admBushing.D0[2] = offset.z();
                        admBushing.has_D0 = true;
                    }

                    // has_* 按是否存在非零分量判定
                    auto anyNonZero = [](const double* arr, int n) {
                        for (int i = 0; i < n; ++i) if (std::abs(arr[i]) > 1e-12) return true;
                        return false;
                        };
                    admBushing.has_K = anyNonZero(admBushing.K, 6);
                    admBushing.has_C = anyNonZero(admBushing.C, 6);
                    admBushing.has_F0 = anyNonZero(admBushing.F0, 3);
                    admBushing.has_T0 = anyNonZero(admBushing.T0, 3);

                    m_sys.bushings_map.insert({ admBushing.id, admBushing });
                    continue;
                }

                if (vf->getLoadType() == MechMotionData::VectorLoadType::Force)
                {
                    motion::ADMForce admForce;
                    admForce.id = std::to_string(feId);
                    admForce.ref_marker = ref_marker;
                    admForce.direction = motion::MotionVector<>(dir.x(), dir.y(), dir.z());
                    admForce.magnitude = magnitude;
                    admForce.force_function = magnitudeExpr.toStdString();
                    m_sys.forces_map.insert({ admForce.id, admForce });
                }
                else
                {
                    motion::ADMTorque admTorque;
                    admTorque.id = std::to_string(feId);
                    admTorque.ref_marker = ref_marker;
                    admTorque.direction = motion::MotionVector<>(dir.x(), dir.y(), dir.z());
                    admTorque.magnitude = magnitude;
                    admTorque.force_function = magnitudeExpr.toStdString();
                    m_sys.torques_map.insert({ admTorque.id, admTorque });
                }
            }
            emit messageLogged(QString("矢量力/力矩装配完成: 力 %1 个, 力矩 %2 个")
                .arg(m_sys.forces_map.size()).arg(m_sys.torques_map.size()));
            if (!m_sys.bushings_map.empty())
                emit messageLogged(QString("衬套装配完成: %1 个").arg(m_sys.bushings_map.size()));

            emit messageLogged(QString("标量力装配完成: %1 个").arg(m_sys.scalar_forces_map.size()));
            if (!m_sys.three_point_forces_map.empty())
                emit messageLogged(QString("三点力装配完成: %1 个").arg(m_sys.three_point_forces_map.size()));
            if (!m_sys.three_point_torques_map.empty())
                emit messageLogged(QString("三点力矩装配完成: %1 个").arg(m_sys.three_point_torques_map.size()));
            emit messageLogged(QString("标量力矩装配完成: %1 个").arg(m_sys.scalar_torques_map.size()));
            for (const auto& pair : m_sys.scalar_torques_map) {
                emit messageLogged(QString("  标量力矩 '%1': 轴模式=%2, 幅值=%3, 函数='%4'")
                    .arg(QString::fromStdString(pair.second.id))
                    .arg(pair.second.axis_mode)
                    .arg(pair.second.magnitude)
                    .arg(QString::fromStdString(pair.second.torque_function)));
            }

            // ==========================================
            // 7. 翻译 3D 接触 (MotionContact -> adams_contact_struct)
            // ==========================================
            {
                //【OCC摘除·引擎化】接触网格数据源改走引擎句柄（E-39 部件网格）
                auto* geoEngine = &m_viewportModel->getModelData()->getGeoEngineDataMgr()->engine();

                auto& contactManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionContactManager>(Physical_MotionContact);
                for (int contactId : contactManager.getAllIDs())
                {
                    auto* contact = contactManager.getData(contactId);
                    if (!contact) continue;

                    int actionBodyId = refId(Physical_MotionContact, contact->getId(), Physical_MotionBody, MapRelation_MotionActionBody);
                    int baseBodyId = refId(Physical_MotionContact, contact->getId(), Physical_MotionBody, MapRelation_MotionBaseBody);
                    // 必要性校验：两体未选/悬空直接中止，不再静默丢弃
                    if (actionBodyId < 0 || baseBodyId < 0) {
                        throw std::runtime_error(QString("接触 '%1' 两端体未设置，已中止求解")
                            .arg(contactManager.getNameByID(contactId)).toStdString());
                    }

                    auto* actionBody = bodyManager.getData(actionBodyId);
                    auto* baseBody = bodyManager.getData(baseBodyId);
                    if (!actionBody || !baseBody) {
                        throw std::runtime_error(QString("接触 '%1' 的端部运动体不存在（可能已删除），已中止求解")
                            .arg(contactManager.getNameByID(contactId)).toStdString());
                    }

                    motion::adams_contact_struct adams_contact;
                    adams_contact.id = std::to_string(contactId);
                    adams_contact.body_I = std::to_string(actionBodyId);
                    adams_contact.body_J = std::to_string(baseBodyId);

                    // 静态标志
                    adams_contact.is_static_I = actionBody->getIsFixed();
                    adams_contact.is_static_J = baseBody->getIsFixed();

                    adams_contact.is_convex_I = contact->getUseConvexHullAction();
                    adams_contact.is_convex_J = contact->getUseConvexHullBase();

                    // 材料/接触参数映射（引擎 ContactInputSettings 合同）：
                    // 1) 摩擦——has_*=true 表示"显式提供系数"。关闭摩擦必须显式置 0，
                    //    不能置 has_*=false（那表示"未提供"，引擎落回默认 μ≈0.6，
                    //    表现为"关了摩擦槽轮还在提前转"，实测主因之一）
                    motion::utils::ApplyContactFrictionSettings(adams_contact,
                        contact->getFrictionEnabled(),
                        contact->getStaticFriction(),
                        contact->getDynamicFriction());

                    // 2) 法向力律——面板刚度/阻尼按 NX/Adams IMPACT 语义映射：
                    //    力单位对齐 N 系→引擎 mN 系 ×1000（刚度 N/mm、阻尼 N·s/mm）；
                    //    "最大穿透深度"是阻尼全值深度（max_penetration_depth），
                    //    不是零力死区，deadband 置 0；深度未填（≤0）时用引擎默认
                    //    0.0254（IMPACT 要求全值深度 > 0）；指数取面板力指数
                    const double impactDmax = contact->getPenetrationDepth() > 0.0
                        ? contact->getPenetrationDepth() : 0.0254;
                    motion::utils::ApplyImpactContactSettings(adams_contact,
                        contact->getStiffness() * 1000.0,
                        contact->getMaterialDamping() * 1000.0,
                        contact->getForceExponent(),
                        impactDmax);

                    // 从离散几何数据构建碰撞网格（节点转换到 body 质心 Marker 局部坐标系）
                    const int badTrisI = buildBodyContactMesh(actionBodyId, bodyManager, geoEngine, actionBody->getCenterOfMass(), adams_contact.nodes_I, adams_contact.elems_I, m_viewportModel);
                    const int badTrisJ = buildBodyContactMesh(baseBodyId, bodyManager, geoEngine, baseBody->getCenterOfMass(), adams_contact.nodes_J, adams_contact.elems_J, m_viewportModel);

                    adams_contact.has_nodes_I = !adams_contact.nodes_I.empty();
                    adams_contact.has_elems_I = !adams_contact.elems_I.empty();
                    adams_contact.has_nodes_J = !adams_contact.nodes_J.empty();
                    adams_contact.has_elems_J = !adams_contact.elems_J.empty();

                    // 网格为空时不加入求解器，避免求解器崩溃
                    if (!adams_contact.has_nodes_I || !adams_contact.has_elems_I ||
                        !adams_contact.has_nodes_J || !adams_contact.has_elems_J) {
                        emit messageLogged(QString("警告：接触 %1 的碰撞网格为空，已跳过")
                            .arg(QString::fromStdString(adams_contact.id)));
                        continue;
                    }

                    // 【接触诊断】：几何/参数/稳定性信息，用于定位接触异常
                    {
                        auto logMeshAabb = [this](const char* tag, const std::vector<double>& nodes) {
                            double mn[3] = { 1e300,1e300,1e300 }, mx[3] = { -1e300,-1e300,-1e300 };
                            for (size_t i = 0; i + 2 < nodes.size(); i += 3) {
                                for (int d = 0; d < 3; ++d) {
                                    if (nodes[i + d] < mn[d]) mn[d] = nodes[i + d];
                                    if (nodes[i + d] > mx[d]) mx[d] = nodes[i + d];
                                }
                            }
                            if (nodes.empty()) {
                                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("  [接触诊断] %1: 网格为空").arg(tag));
                                return;
                            }
                            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("  [接触诊断] %1: 局部AABB [%2,%3,%4]~[%5,%6,%7]")
                                .arg(tag)
                                .arg(mn[0], 0, 'g', 6).arg(mn[1], 0, 'g', 6).arg(mn[2], 0, 'g', 6)
                                .arg(mx[0], 0, 'g', 6).arg(mx[1], 0, 'g', 6).arg(mx[2], 0, 'g', 6));
                            };
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("[接触诊断] '%1': I体=%2(质量=%3,凸包=%4,静态=%5,顶点=%6,三角=%7) J体=%8(质量=%9,凸包=%10,静态=%11,顶点=%12,三角=%13) 刚度=%14 阻尼=%15")
                            .arg(QString::fromStdString(adams_contact.id))
                            .arg(actionBody->getName()).arg(actionBody->getMass())
                            .arg(adams_contact.is_convex_I).arg(adams_contact.is_static_I)
                            .arg(adams_contact.nodes_I.size() / 3).arg(adams_contact.elems_I.size() / 3)
                            .arg(baseBody->getName()).arg(baseBody->getMass())
                            .arg(adams_contact.is_convex_J).arg(adams_contact.is_static_J)
                            .arg(adams_contact.nodes_J.size() / 3).arg(adams_contact.elems_J.size() / 3)
                            .arg(adams_contact.stiffness).arg(adams_contact.damping));
                        logMeshAabb("I端网格(局部)", adams_contact.nodes_I);
                        logMeshAabb("J端网格(局部)", adams_contact.nodes_J);
                        if (badTrisI > 0 || badTrisJ > 0) {
                            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("  [接触诊断] 已清洗坏三角形：I端 %1 个，J端 %2 个（NaN/零面积）")
                                .arg(badTrisI).arg(badTrisJ));
                        }
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("  [接触诊断] I体质心=[%1,%2,%3] J体质心=[%4,%5,%6]")
                            .arg(actionBody->getCenterOfMass().x(), 0, 'g', 6)
                            .arg(actionBody->getCenterOfMass().y(), 0, 'g', 6)
                            .arg(actionBody->getCenterOfMass().z(), 0, 'g', 6)
                            .arg(baseBody->getCenterOfMass().x(), 0, 'g', 6)
                            .arg(baseBody->getCenterOfMass().y(), 0, 'g', 6)
                            .arg(baseBody->getCenterOfMass().z(), 0, 'g', 6));
                    }


                    m_sys.contacts_map.insert({ adams_contact.id, adams_contact });
                }
                emit messageLogged(QString("3D 接触装配完成: %1 个").arg(m_sys.contacts_map.size()));
            }

            // ==========================================
            // 【数据完整性校验】：在提交给求解器前拦截致命错误
            // ==========================================
            if (m_sys.parts_map.empty()) {
                throw std::runtime_error("模型中没有任何运动体，无法进行求解");
            }

            bool hasMovableBody = false;
            //【刚柔耦合】将被 AutoFlex 柔性体替换的目标刚体不算"固定无自由度"
            //（替换后柔性体自带模态自由度），固定体悬臂场景因此放行。
            std::set<int> flexTargetBodyIds;
            {
                auto& flexMgr = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionAutoFlexManager>(Physical_MotionAutoFlex);
                auto& relMgr = m_viewportModel->getModelData()->getMapRelationManager();
                for (int flexId : flexMgr.getAllIDs()) {
                    auto* flex = flexMgr.getData(flexId);
                    if (!flex || !flex->isEnabled())
                        continue;
                    const int bodyId = relMgr.getMapRelation<int>({ Physical_MotionAutoFlex, flexId }, Physical_MotionBody, MapRelation_None);
                    if (bodyId >= 0)
                        flexTargetBodyIds.insert(bodyId);
                }
            }
            for (const auto& p : m_sys.parts_map) {
                if (!p.second.fixed) { hasMovableBody = true; break; }
            }
            if (!hasMovableBody && !flexTargetBodyIds.empty()) {
                hasMovableBody = true;  // 柔体模态自由度兜底
            }
            if (!hasMovableBody) {
                throw std::runtime_error("所有运动体均为固定约束，系统自由度为0，无需求解");
            }

            // 校验 Marker 引用的 part 存在
            for (const auto& mk : m_sys.markers_map) {
                if (m_sys.parts_map.find(mk.second.part_id) == m_sys.parts_map.end()) {
                    throw std::runtime_error(QString("Marker %1 引用了不存在的运动体 ID: %2")
                        .arg(QString::fromStdString(mk.first))
                        .arg(QString::fromStdString(mk.second.part_id)).toStdString());
                }
            }

            // 校验 Joint 引用的 Marker 存在
            for (const auto& jt : m_sys.joints_map) {
                if (m_sys.markers_map.find(jt.second->marker_I) == m_sys.markers_map.end() ||
                    m_sys.markers_map.find(jt.second->marker_J) == m_sys.markers_map.end()) {
                    throw std::runtime_error(QString("运动副 %1 引用了不存在的 Marker")
                        .arg(QString::fromStdString(jt.first)).toStdString());
                }
            }

            // 校验 Motion 引用的 Marker 存在
            // 【注意】：驱动马达复用目标运动副的 Marker（二者共存，运动副不再被跳过），
            // 因此 resolved_joint_id 不一定存在于 joints_map 中，此处仅校验 Marker 引用
            for (const auto& mt : m_sys.motions_map) {
                if (m_sys.markers_map.find(mt.second.marker_I) == m_sys.markers_map.end() ||
                    m_sys.markers_map.find(mt.second.marker_J) == m_sys.markers_map.end()) {
                    throw std::runtime_error(QString("驱动 %1 引用了不存在的 Marker")
                        .arg(QString::fromStdString(mt.first)).toStdString());
                }
                // resolved_joint_id 是 UI 层面的逻辑引用，被驱动的运动副不加入 m_sys.joints_map，
                // 因此不校验其在 joints_map 中的存在性。底层求解器通过 Marker I/J 建立约束。
            }

            // 校验 Bushing 引用的 Marker 存在
            for (const auto& bu : m_sys.bushings_map) {
                if (m_sys.markers_map.find(bu.second.marker_I) == m_sys.markers_map.end() ||
                    m_sys.markers_map.find(bu.second.marker_J) == m_sys.markers_map.end()) {
                    throw std::runtime_error(QString("衬套 %1 引用了不存在的 Marker")
                        .arg(QString::fromStdString(bu.first)).toStdString());
                }
            }

            // 校验 Force/Torque 引用的 Marker 存在
            for (const auto& fc : m_sys.forces_map) {
                if (m_sys.markers_map.find(fc.second.ref_marker) == m_sys.markers_map.end()) {
                    throw std::runtime_error(QString("矢量力 %1 引用了不存在的 Marker: %2")
                        .arg(QString::fromStdString(fc.first))
                        .arg(QString::fromStdString(fc.second.ref_marker)).toStdString());
                }
            }
            for (const auto& tq : m_sys.torques_map) {
                if (m_sys.markers_map.find(tq.second.ref_marker) == m_sys.markers_map.end()) {
                    throw std::runtime_error(QString("矢量力矩 %1 引用了不存在的 Marker: %2")
                        .arg(QString::fromStdString(tq.first))
                        .arg(QString::fromStdString(tq.second.ref_marker)).toStdString());
                }
            }

            emit messageLogged("前处理数据完整性校验通过，准备进入求解器...");
        }
        catch (const std::exception& e)
        {
            emit messageLogged(QString("前处理数据转换异常: %1").arg(QString::fromStdString(e.what())));
            emit solveFinished(m_scenarioId, false);
            return;
        }
        catch (...)
        {
            emit messageLogged(QString("前处理数据转换发生未知异常"));
            emit solveFinished(m_scenarioId, false);
            return;
        }
    }

    void mMotionSolverThread::doSolve()
    {
        // 1. 获取方案节点数据
        auto& scenarioManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario);
        auto* scenario = scenarioManager.getData(m_scenarioId);
        if (!scenario)
        {
            return;
        }

        // 从界面 UI 的配置中获取真实的仿真时间和步长
        double endTime = scenario->getEndTime();
        double stepSize = scenario->getStepSize();
        auto gravity = scenario->getGravity();

        // 防呆保护：防止步长为 0 导致死循环或除零错误
        if (stepSize <= 0.0) stepSize = 0.01;

        int totalSteps = static_cast<int>(std::ceil(endTime / stepSize));

        emit messageLogged(QString("开始物理求解... 总时长: %1s, 步长: %2s, 共计 %3 步。")
            .arg(endTime).arg(stepSize).arg(totalSteps));
        //emit progressUpdated(0);

        // ==========================================
        // 1. 初始化并装载物理引擎
        // ==========================================

        std::unique_ptr<motion::utils::MotionParserAdams> parser(new motion::utils::MotionParserAdams());
        parser->SetVisualizationType(motion::utils::MotionParserAdams::NONE);

        // 崩溃诊断：把送进引擎的完整输入落盘（E:/motion_dump/run_<时间戳>/），
        // 供离线字节级复现。问题定位完成后停用；需要离线复现时恢复下面三行。
        //dumpSolverInputForOfflineRepro(m_sys);
        //{
        //    QFile f(QStringLiteral("E:/motion_dump/") +
        //            QStringLiteral("latest_scenario.txt"));
        //    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //        QTextStream s(&f); s.setCodec("UTF-8");
        //        s << "gravity=" << dumpG(gravity.x()) << " " << dumpG(gravity.y()) << " " << dumpG(gravity.z()) << "\n";
        //        s << "endTime=" << dumpG(endTime) << " stepSize=" << dumpG(stepSize) << "\n";
        //        s << "usePreset=" << (scenario->getUsePreset() ? 1 : 0)
        //          << " preset=" << static_cast<int>(scenario->getPresetMode())
        //          << " integrator=" << static_cast<int>(scenario->getIntegratorType()) << "\n";
        //    }
        //}

        parser->Parse(m_sys);

        //设置重力（JSON 导出口径已为 mm/s²，直接传入引擎——不做换算。
        // 此前曾加 ×1000 是误判：GUI 面板和 JSON 里用户输入的就是 mm/s²，
        // 引擎 mm-tonne-s 制的正确重力值；再乘 1000 会把重力放大千倍导致
        // 柔体变形量暴增 30,000 倍）
        m_sys.Set_G_acc(motion::MotionVector<double>(gravity.x(), gravity.y(), gravity.z()));
        //【诊断】确认引擎收到的重力值
        {
            const auto& gEng = m_sys.Get_G_acc();
            emit messageLogged(QString("  [柔性体] 引擎重力 = (%1, %2, %3) mm/s²，|g|=%4")
                .arg(gEng.x(), 0, 'f', 1).arg(gEng.y(), 0, 'f', 1).arg(gEng.z(), 0, 'f', 1)
                .arg(std::sqrt(gEng.x()*gEng.x() + gEng.y()*gEng.y() + gEng.z()*gEng.z()), 0, 'f', 1));
        }


        // ==========================================
        // 组装求解配置
        // ==========================================
        motion::SimulationConfig config;
        config.step_size = stepSize;
        config.total_time = endTime;

        // 预设模式
        config.use_preset = scenario->getUsePreset();
        config.preset = static_cast<motion::SimulationConfig::Preset>(static_cast<int>(scenario->getPresetMode()));

        // 积分器配置
        switch (scenario->getIntegratorType())
        {
        case MechMotionData::IntegratorType::HHT:
            config.integrator.type = motion::MotionTimestepper::Type::HHT; break;
        case MechMotionData::IntegratorType::NEWMARK:
            config.integrator.type = motion::MotionTimestepper::Type::NEWMARK; break;
        case MechMotionData::IntegratorType::EULER_IMPLICIT_LINEARIZED:
            config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED; break;
        case MechMotionData::IntegratorType::EULER_IMPLICIT_PROJECTED:
            config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT_PROJECTED; break;
        case MechMotionData::IntegratorType::EULER_IMPLICIT:
            config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT; break;
        case MechMotionData::IntegratorType::TRAPEZOIDAL:
            config.integrator.type = motion::MotionTimestepper::Type::TRAPEZOIDAL; break;
        case MechMotionData::IntegratorType::TRAPEZOIDAL_LINEARIZED:
            config.integrator.type = motion::MotionTimestepper::Type::TRAPEZOIDAL_LINEARIZED; break;
        case MechMotionData::IntegratorType::HEUN:
            config.integrator.type = motion::MotionTimestepper::Type::HEUN; break;
        case MechMotionData::IntegratorType::RUNGEKUTTA45:
            config.integrator.type = motion::MotionTimestepper::Type::RUNGEKUTTA45; break;
        case MechMotionData::IntegratorType::EULER_EXPLICIT:
            config.integrator.type = motion::MotionTimestepper::Type::EULER_EXPLICIT; break;
        case MechMotionData::IntegratorType::LEAPFROG:
            config.integrator.type = motion::MotionTimestepper::Type::LEAPFROG; break;
        default:
            config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT; break;
        }

        // 高级积分器参数
        config.integrator.alpha = scenario->getIntegratorAlpha();
        config.integrator.beta = scenario->getIntegratorBeta();
        config.integrator.max_iters = scenario->getIntegratorMaxIters();
        config.integrator.tol = scenario->getIntegratorTol();

        // 求解器配置
        switch (scenario->getSolverType())
        {
        case MechMotionData::SolverType::PSOR:
            config.solver.type = motion::MotionSolver::Type::PSOR; break;
        case MechMotionData::SolverType::PSSOR:
            config.solver.type = motion::MotionSolver::Type::PSSOR; break;
        case MechMotionData::SolverType::PJACOBI:
            config.solver.type = motion::MotionSolver::Type::PJACOBI; break;
        case MechMotionData::SolverType::PMINRES:
            config.solver.type = motion::MotionSolver::Type::PMINRES; break;
        case MechMotionData::SolverType::BARZILAIBORWEIN:
            config.solver.type = motion::MotionSolver::Type::BARZILAIBORWEIN; break;
        case MechMotionData::SolverType::APGD:
            config.solver.type = motion::MotionSolver::Type::APGD; break;
        case MechMotionData::SolverType::ADDM:
            config.solver.type = motion::MotionSolver::Type::ADDM; break;
        case MechMotionData::SolverType::SPARSE_LU:
            config.solver.type = motion::MotionSolver::Type::SPARSE_LU; break;
        case MechMotionData::SolverType::SPARSE_QR:
            config.solver.type = motion::MotionSolver::Type::SPARSE_QR; break;
        case MechMotionData::SolverType::PARDISO_MKL:
            config.solver.type = motion::MotionSolver::Type::PARDISO_MKL; break;
        case MechMotionData::SolverType::PARDISO_PROJECT:
            config.solver.type = motion::MotionSolver::Type::PARDISO_PROJECT; break;
        case MechMotionData::SolverType::MUMPS:
            config.solver.type = motion::MotionSolver::Type::MUMPS; break;
        case MechMotionData::SolverType::GMRES:
            config.solver.type = motion::MotionSolver::Type::GMRES; break;
        case MechMotionData::SolverType::MINRES:
            config.solver.type = motion::MotionSolver::Type::MINRES; break;
        case MechMotionData::SolverType::BICGSTAB:
            config.solver.type = motion::MotionSolver::Type::BICGSTAB; break;
        default:
            config.solver.type = motion::MotionSolver::Type::PSOR; break;
        }

        config.solver.max_iters = scenario->getSolverMaxIters();
        config.solver.tolerance = scenario->getSolverTolerance();
        config.solver.verbose = scenario->getSolverVerbose();

        // 柔体创建移到 InitSolver 后（见下），此处用 AutoFlex 管理器 enabled 数预判
        int nFlexBuilt = 0;
        if (m_viewportModel && m_viewportModel->getModelData()) {
            auto& fm = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionAutoFlexManager>(Physical_MotionAutoFlex);
            for (int fid : fm.getAllIDs())
                if (auto* fd = fm.getData(fid); fd && fd->isEnabled())
                    ++nFlexBuilt;
        }
        // 【刚柔耦合】含柔体时强制 MINRES + Euler 隐式线性化：
        // PSOR 等 VI 求解器与 HHT 积分器同 modal 装配均不兼容（首步即失败，见刚柔耦合开发任务清单）。
        if (nFlexBuilt > 0) {
            config.solver.type = motion::MotionSolver::Type::MINRES;
            config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED;
            config.use_preset = false;  // 关键：预设(FAST 等)会在 InitSolver 里强制覆盖为 PSOR——对 modal 装配首步即 NaN
            if (config.solver.max_iters < 500)
                config.solver.max_iters = 500;
            // 每步后位置装配：Euler 线性化只在速度层闭约束，含模态装配体的机构副孔
            // 位置漂移实测 0.6~1.7mm（偏心）；每步投影回约束流形压到 ~0.01mm
            m_sys.SetPositionAssemblyPerStep(true);
            emit messageLogged(QString("  [柔性体] 检测到 %1 个柔性体：求解器已强制 MINRES + Euler 隐式线性化（每步位置装配）").arg(nFlexBuilt));
        }
        else {
            m_sys.SetPositionAssemblyPerStep(false);
        }

        // ==========================================
        // PID 控制器装配（须在 InitSolver 前完成配置装载）
        // ==========================================
        m_sys.resetPidState();
        {
            auto& controllerMgr = m_viewportModel->getModelData()
                ->getManager<MechMotionData::mMotionControllerManager>(Physical_MotionController);
            auto& driveMgr = m_viewportModel->getModelData()
                ->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);
            for (int cid : controllerMgr.getAllIDs()) {
                auto* pidc = dynamic_cast<MechMotionData::mMotionPIDController*>(controllerMgr.getData(cid));
                if (!pidc || !pidc->isEnabled()) continue;

                const int driveId = refId(Physical_MotionController, pidc->getId(), Physical_MotionDrive, MxFunctions::MapRelation_None);
                auto motionIt = m_sys.motions_map.find(std::to_string(driveId));
                auto* jointDrive = dynamic_cast<MechMotionData::mMotionJointDrive*>(driveMgr.getData(driveId));
                if (driveId < 0 || motionIt == m_sys.motions_map.end() || !jointDrive) {
                    emit messageLogged(QString("PID控制器 '%1' 未找到有效驱动，已跳过").arg(pidc->getName()));
                    continue;
                }
                if (jointDrive->getControlMode() != MechMotionData::DriveControlMode::Velocity) {
                    emit messageLogged(QString("PID控制器 '%1' 仅支持速度控制模式的驱动，已跳过").arg(pidc->getName()));
                    continue;
                }

                mMotionPidSystem::PidRuntime rt;
                rt.controllerId = cid;
                rt.driveId = motionIt->first;
                rt.markerI = motionIt->second.marker_I;
                rt.markerJ = motionIt->second.marker_J;
                auto mkI = m_sys.markers_map.find(rt.markerI);
                auto mkJ = m_sys.markers_map.find(rt.markerJ);
                if (mkI != m_sys.markers_map.end()) rt.bodyI = mkI->second.part_id;
                if (mkJ != m_sys.markers_map.end()) rt.bodyJ = mkJ->second.part_id;
                rt.feedbackType = pidc->getFeedbackType();
                rt.target = pidc->getTargetValue();
                // 目标轨迹（引用控制输入时每步按时求值，常数类型等价于多项式特例）
                const int inputId = refId(Physical_MotionController, pidc->getId(), Physical_MotionControlInput, MxFunctions::MapRelation_None);
                if (inputId >= 0) {
                    auto& inputMgr = m_viewportModel->getModelData()
                        ->getManager<MechMotionData::mMotionControlInputManager>(Physical_MotionControlInput);
                    if (auto* input = inputMgr.getData(inputId)) {
                        rt.trajKind = static_cast<int>(input->getInputType());
                        rt.trajTimeStart = input->getTimeStart();
                        rt.trajTimeEnd = input->getTimeEnd();
                        const auto& coeffs = input->getPolyCoeffs();
                        rt.trajCoeffs.assign(coeffs.begin(), coeffs.end());
                        const auto& pts = input->getCurvePoints();
                        rt.trajPoints.reserve(pts.size());
                        for (const auto& p : pts) {
                            rt.trajPoints.emplace_back(p.first, p.second);
                        }
                        emit messageLogged(QString("PID控制器 '%1' 目标为控制输入 '%2'")
                            .arg(pidc->getName()).arg(inputMgr.getNameByID(inputId)));
                    }
                    else {
                        emit messageLogged(QString("PID控制器 '%1' 引用的控制输入不存在，退回常数目标")
                            .arg(pidc->getName()));
                    }
                }
                rt.kp = pidc->getKp();
                rt.ki = pidc->getKi();
                rt.kd = pidc->getKd();
                rt.outputLimit = pidc->getOutputLimit();
                // 常数驱动的函数值即马达身份指纹（表达式驱动无法求值，跳过校验）
                if (motionIt->second.function_expr.empty()) {
                    rt.hasFuncFingerprint = true;
                    rt.expectedFuncValue = motionIt->second.func_value;
                }

                // 反馈源装配：传感器反馈拷贝定义并登记参与求值的体
                rt.feedbackSign = pidc->getFeedbackSign();
                if (pidc->getFeedbackSource() == MechMotionData::PidFeedbackSource::Sensor) {
                    auto& sensorMgr = m_viewportModel->getModelData()
                        ->getManager<MechMotionData::mMotionSensorManager>(Physical_MotionSensor);
                    const int sensorId = refId(Physical_MotionController, pidc->getId(), Physical_MotionSensor, MxFunctions::MapRelation_None);
                    auto* sensorBase = (sensorId >= 0) ? sensorMgr.getData(sensorId) : nullptr;

                    bool sensorOk = sensorBase && sensorBase->isEnabled();
                    if (sensorOk) {
                        // 仅支持单帧可求值的运动学类传感器（相对速度/加速度需序列差分，关节/驱动类需结果帧）
                        const auto st = sensorBase->getType();
                        sensorOk = (st == MechMotionData::SensorType::BodyDisplacement
                            || st == MechMotionData::SensorType::BodyVelocity
                            || st == MechMotionData::SensorType::BodyAcceleration
                            || st == MechMotionData::SensorType::Distance
                            || st == MechMotionData::SensorType::Angle);
                        if (sensorOk && st == MechMotionData::SensorType::Distance) {
                            const auto* ds = static_cast<const MechMotionData::mMotionDistanceSensor*>(sensorBase);
                            sensorOk = (ds->getVariable() == MechMotionData::DistanceVariable::Distance);
                        }
                    }
                    if (!sensorOk) {
                        emit messageLogged(QString("PID控制器 '%1' 的反馈传感器无效或类型不支持"
                            "（需启用，且为体/距离(仅距离值)/角度类），该控制器已跳过").arg(pidc->getName()));
                        continue;
                    }

                    rt.useSensorFeedback = true;
                    rt.sensorCopy.reset(sensorBase->copySensor(sensorId, sensorBase->getName()));
                    // 角度传感器输出为度：PID 数学域归一到弧度（与输出 rad/s 同量纲）
                    rt.feedbackDegToRad = (sensorBase->getType() == MechMotionData::SensorType::Angle);

                    std::set<int> bodyIds;
                    const auto st = sensorBase->getType();
                    if (st == MechMotionData::SensorType::BodyDisplacement
                        || st == MechMotionData::SensorType::BodyVelocity
                        || st == MechMotionData::SensorType::BodyAcceleration) {
                        const auto* bs = static_cast<const MechMotionData::mMotionBodySensor*>(sensorBase);
                        if (refId(Physical_MotionSensor, bs->getId(), Physical_MotionBody, MxFunctions::MapRelation_None) >= 0) bodyIds.insert(refId(Physical_MotionSensor, bs->getId(), Physical_MotionBody, MxFunctions::MapRelation_None));
                        if (refId(Physical_MotionSensor, sensorBase->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody) >= 0) bodyIds.insert(refId(Physical_MotionSensor, sensorBase->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody));
                    }
                    else if (st == MechMotionData::SensorType::Distance) {
                        const auto* ds = static_cast<const MechMotionData::mMotionDistanceSensor*>(sensorBase);
                        if (ds->getPointA().bodyId >= 0) bodyIds.insert(ds->getPointA().bodyId);
                        if (ds->getPointB().bodyId >= 0) bodyIds.insert(ds->getPointB().bodyId);
                    }
                    else if (st == MechMotionData::SensorType::Angle) {
                        const auto* as = static_cast<const MechMotionData::mMotionAngleSensor*>(sensorBase);
                        if (refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionAxisBody) >= 0) bodyIds.insert(refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
                        if (refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionBodyA) >= 0) bodyIds.insert(refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionBodyA));
                        if (refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionBodyB) >= 0) bodyIds.insert(refId(Physical_MotionSensor, as->getId(), Physical_MotionBody, MapRelation_MotionBodyB));
                    }
                    rt.sensorBodyIds.assign(bodyIds.begin(), bodyIds.end());

                    emit messageLogged(QString("PID控制器 '%1' 反馈源为传感器 '%2'")
                        .arg(pidc->getName()).arg(sensorBase->getName()));
                }
                m_sys.pidControllers.push_back(std::move(rt));

                emit messageLogged(QString("PID控制器 '%1' 已装配（驱动 %2）").arg(pidc->getName()).arg(driveId));
            }
        }

        m_sys.InitSolver(config);

        //【发散守卫】新一轮求解前重置发散标记
        m_flexDivergedTime = -1.0;

        // ==========================================
        // 【刚柔耦合】AutoFlex 柔性体接入（InitSolver 后建柔体，避免 InitSolver 重置缩减状态）
        // ==========================================
        {
            const auto flexInputs = mMotionFlexBridge::collectFlexInputs(
                &m_sys, m_viewportModel->getModelData(),
                [this](const QString& s) { emit messageLogged(s); });
            for (const auto& in : flexInputs) {
                if (!in.valid) {
                    emit messageLogged(QString("  [柔性体] %1（id=%2）跳过：%3").arg(in.name).arg(in.flexId).arg(in.error));
                    continue;
                }
                try {
                    auto out = mMotionFlexBridge::buildOne(m_sys, in);
                    const int nReattached = mMotionFlexBridge::reattachJointsToFlex(m_sys, in, out,
                        [this](const QString& s) { emit messageLogged(s); });
                    ++nFlexBuilt;
                    FlexCapture cap;
                    cap.input = in;
                    cap.output = out;
                    m_flexCaptures.push_back(std::move(cap));
                    emit messageLogged(QString("  [柔性体] %1（id=%2）已创建：节点 %3 / 四面体 %4 / 模态 %5 / 接口体 %6，副改挂 %7 个（目标刚体 %8 已由柔体替换）")
                        .arg(in.name).arg(in.flexId).arg(out.nodes).arg(out.tets).arg(out.modes).arg(out.interfaces)
                        .arg(nReattached >= 0 ? nReattached : 0).arg(in.bodyId));
                    //【模态清单】各阶固有频率打到信息窗口（UG 语义：柔体建好即可见）
                    if (out.assembly) {
                        const auto& freqs = out.assembly->Get_modes_frequencies();
                        if (freqs.size() > 0) {
                            QStringList fs;
                            for (int i = 0; i < freqs.size(); ++i)
                                fs << QString("第%1阶 %2 Hz").arg(i + 1).arg(freqs[i], 0, 'f', 1);
                            emit messageLogged(QString("  [柔性体] %1 模态频率（共 %2 阶）：%3")
                                .arg(in.name).arg(freqs.size()).arg(fs.join(QString::fromUtf8("，"))));
                        }
                    }
                } catch (const std::exception& e) {
                    emit messageLogged(QString("  [柔性体] %1（id=%2）创建失败：%3").arg(in.name).arg(in.flexId).arg(e.what()));
                }
            }
        }



        // 初始装配分析（UG/Adams 求解前标准步骤）：把运动副的初始约束残差
        // （线在线上副两曲线的初始法向/切向偏差等）在第一步动力学前消掉。
        // 不做的话，第一步残差修正会向系统注入一个初始冲量，FAST 预设的粗迭代
        // （EULER_IMPLICIT 只迭代 4 次）无法收敛干净，曲线副在第一步就被甩飞。
        // 提高装配迭代上限（默认 6 对曲线副的重锚+约束耦合偏少）以收敛干净。
        m_sys.SetMaxiter(50);
        if (!m_sys.DoAssembly(motion::AssemblyLevel::POSITION |
                              motion::AssemblyLevel::VELOCITY |
                              motion::AssemblyLevel::ACCELERATION)) {
            emit messageLogged(QStringLiteral("警告：初始装配分析未完全收敛，结果可能有初始偏差"));
        }

        // 引擎装配完成后立刻定位并接管 PID 马达：
        // 定位失败（马达已存在但全部候选未通过校验）直接中止求解，
        // 避免控制器静默失效开环空算；若引擎在 StartSolver 内才建 link，
        // 这里扑空后由每步末尾的钩子重试（见 mMotionPidSystem）。
        if (!m_sys.pidControllers.empty()) {
            if (m_sys.resolvePidMotors()) {
                for (const auto& msg : m_sys.pidMessages) {
                    emit messageLogged(QString::fromStdString(msg));
                }
                m_sys.pidMessages.clear();
                const bool anyFailed = std::any_of(
                    m_sys.pidControllers.begin(), m_sys.pidControllers.end(),
                    [](const mMotionPidSystem::PidRuntime& r) { return r.failed; });
                if (anyFailed) {
                    emit messageLogged(QStringLiteral("存在未能接管驱动马达的PID控制器，已中止求解（详见上方PID日志）。"));
                    emit solveFinished(m_scenarioId, false);
                    return;
                }
            }
        }

        //测试参数
        //m_sys.SetSolverMaxIterations(100);
        //m_sys.SetSolverForceTolerance(1e-6);
        //m_sys.SetTimestepperType(motion::MotionTimestepper::Type::EULER_IMPLICIT);
        m_sys.SetProgressCallback([this](double current_time, double total_time, double progress) {
            // 注意：此回调可能运行在非 Qt 的物理引擎内部线程（如 OpenMP worker）中，
            // 严禁在此调用任何 Qt API（包括 emit、QMetaObject、QTimer 等）。
            m_currentProgress.store(static_cast<int>(progress * 100));
            //【刚柔耦合】每步采集柔性体模态坐标与接口体位姿（纯 STL，回调线程安全）。
            // 回调在 RecordStepResults 之后触发，与 body_results 帧完全同步。
            for (auto& cap : m_flexCaptures) {
                if (!cap.output.assembly) continue;
                const auto& q = cap.output.assembly->Get_modal_q();
                //【发散守卫】模态坐标非有限/爆炸 → 优雅停机（与点在面上副越界同语义：
                // 保留已算结果）。发散帧不入库（先检测后采集），垃圾数据进不了
                // 动画/曲线。阈值 |q|²>1e6：物理模态坐标量级 ~1e-3..1e0，跨多个
                // 数量级仍安全
                double qn = 0.0;
                bool bad = false;
                for (int i = 0; i < q.size(); ++i) {
                    if (!std::isfinite(q(i))) { bad = true; break; }
                    qn += q(i) * q(i);
                }
                if (!bad && qn > 1.0e6)
                    bad = true;
                if (bad) {
                    if (m_flexDivergedTime < 0.0)
                        m_flexDivergedTime = current_time;
                    m_sys.RequestStopSolve();
                    continue;
                }
                cap.data.times.push_back(current_time);
                cap.data.qFlat.insert(cap.data.qFlat.end(), q.data(), q.data() + q.size());
                for (auto& b : cap.output.interfaceBodies) {
                    if (!b) { for (int k = 0; k < 7; ++k) cap.data.poseFlat.push_back(0.0); continue; }
                    const auto& c = b->GetCoord();
                    cap.data.poseFlat.push_back(c.pos.x());
                    cap.data.poseFlat.push_back(c.pos.y());
                    cap.data.poseFlat.push_back(c.pos.z());
                    const double* q4 = c.rot.data();  // [x,y,z,w]
                    cap.data.poseFlat.push_back(q4[0]);
                    cap.data.poseFlat.push_back(q4[1]);
                    cap.data.poseFlat.push_back(q4[2]);
                    cap.data.poseFlat.push_back(q4[3]);
                }
            }
            });
        try
        {
            m_sys.StartSolver(config);
        }
        catch (const std::exception& msg)
        {
            // 求解中止后丢弃陈旧结果：失败帧时间轴不足、残余态为 0/NaN，
            // 若保留上一轮的旧结果，曲线面板会按新方案时长画出"求解到一半后全零"的假数据
            scenario->attachResult(nullptr);
            // PID 求解期日志（含逐步调试行）在失败路径也要吐出，否则看不到发散前的控制环状态
            for (const auto& m : m_sys.pidMessages) {
                emit messageLogged(QString::fromStdString(m));
            }
            m_sys.pidMessages.clear();
            emit messageLogged(QString("求解器异常: %1").arg(QString::fromStdString(msg.what())));
            emit solveFinished(m_scenarioId, false);
            return;
        }

        // 注意：不把 m_sys.IsStopSolveRequested() 计入用户取消——引擎会在
        // 点在面上副越界（UG 语义）时自动请求停机并保留已算结果，那种情况
        // 要走下方"边界终止"分支继续提取结果，而不是当作失败丢弃。
        const bool canceled = m_cancelRequested.load(std::memory_order_acquire) || isInterruptionRequested();
        if (canceled)
        {
            m_sys.RequestStopSolve();
            scenario->attachResult(nullptr);
            emit messageLogged(QStringLiteral("求解已被用户中止。"));
            emit solveFinished(m_scenarioId, false);
            return;
        }

        // 点在面上副到达面边界：引擎已优雅停机（UG 语义），保留已算结果并提示
        {
            QString exitedJoints;
            for (const auto& link : m_sys.Get_linklist())
            {
                auto posLink = std::dynamic_pointer_cast<motion::MotionLinkPointOnSurface>(link);
                if (posLink && posLink->IsOutOfFace())
                {
                    if (!exitedJoints.isEmpty()) exitedJoints += ", ";
                    exitedJoints += QString::fromStdString(posLink->GetNameString());
                }
            }
            if (!exitedJoints.isEmpty())
            {
                double tStop = 0.0;
                if (!m_sys.body_results.empty() && !m_sys.body_results.begin()->second.data.empty())
                    tStop = m_sys.body_results.begin()->second.data.back().time;
                emit messageLogged(QString("点在面上副 %1 的作用点已到达面边界，计算在 t=%2 s 终止（与 UG 行为一致），已保留此前结果。")
                    .arg(exitedJoints).arg(tStop, 0, 'f', 3));
            }
        }

        //获取结果
        m_sys.body_results;    // 刚体结果
        m_sys.joint_results;    // 连接副
        m_sys.motion_results;    // 驱动
        m_sys.marker_results;    // marker点

        emit messageLogged("底层物理积分求解完成，正在提取结果数据...");

        // ==========================================
        // 3. 转换动画帧数据 (Time-centric 翻转)
        // ==========================================
        std::unique_ptr<MechMotionData::mMotionResultData> resultData(new MechMotionData::mMotionResultData());

        //【刚柔耦合】导出柔性体结果（一次性拓扑/振型 + 每帧模态坐标）
        for (auto& cap : m_flexCaptures) {
            if (!cap.output.assembly) continue;
            MechMotionData::mMotionFlexBodyData flexDest;
            mMotionFlexBridge::exportFlexResult(cap.input, cap.output, cap.data, flexDest);
            //【诊断】变形真值打印（NX 口径：质量加权最优刚体拟合偏差——参考系无关，
            // 不受接口体在 COM/铰点位置影响；弹性口径 Ψ·q 在体参考系在 COM 时
            // 混入力臂记账 ×1000，不可直接读）
            {
                const int N = flexDest.m_nodeX0.size() / 3;
                const int m = flexDest.m_nModes;
                const int K = flexDest.m_nInterfaces;
                const int cols = 6 * K + m;
                if (N > 0 && m > 0 && flexDest.m_psi.size() == (qint64)3 * N * cols) {
                    const double* psi = flexDest.m_psi.constData();
                    double maxDef = 0.0, maxDefT = 0.0;
                    // 逐帧重建节点位置（刚体 + 弹性），再对 (x0, x_t) 做质量加权 Umeyama 拟合
                    for (const auto& fr : flexDest.m_frames) {
                        if (fr.m_interfacePose.size() < 7 * K) continue;
                        const double* p0 = flexDest.m_interfacePose0.constData();
                        const double* pT = fr.m_interfacePose.constData();
                        QVector3D bodyPos0(p0[0], p0[1], p0[2]);
                        QVector3D bodyPosT(pT[0], pT[1], pT[2]);
                        QQuaternion q0(p0[3], p0[4], p0[5], p0[6]);
                        QQuaternion qT(pT[3], pT[4], pT[5], pT[6]);
                        QQuaternion dq = qT * q0.conjugated();
                        QVector<QVector3D> nodes(N);
                        for (int r = 0; r < N; ++r) {
                            QVector3D x0(flexDest.m_nodeX0[3*r], flexDest.m_nodeX0[3*r+1], flexDest.m_nodeX0[3*r+2]);
                            QVector3D rigid = dq.rotatedVector(x0 - bodyPos0) + bodyPosT;
                            const double* rowX = psi + (size_t)(3 * r) * cols + 6 * K;
                            const double* rowY = rowX + cols;
                            const double* rowZ = rowY + cols;
                            double ex = 0, ey = 0, ez = 0;
                            for (int c = 0; c < m; ++c) {
                                ex += rowX[c] * fr.m_modalQ[c];
                                ey += rowY[c] * fr.m_modalQ[c];
                                ez += rowZ[c] * fr.m_modalQ[c];
                            }
                            nodes[r] = rigid + dq.rotatedVector(QVector3D(ex, ey, ez));
                        }
                        // 质量加权拟合偏差（与渲染 NX 口径同式；均匀权近似）
                        QVector<double> uniform;   // 空 = 均匀权
                        QVector<double> nxVals = mMotionFlexBridge::nxContourValues(flexDest.m_nodeX0, nodes, uniform);
                        double frameMax = 0.0;
                        for (double v : nxVals) frameMax = std::max(frameMax, v);
                        if (frameMax > maxDef) { maxDef = frameMax; maxDefT = fr.m_time; }
                    }
                    
                }
            }
            resultData->m_flexBodies.push_back(std::move(flexDest));
        }

        // 以底层实际输出的记录点数量为准 (通常等于 totalSteps + 1，包含初始状态)
        int numOutputSteps = 0;
        if (!m_sys.body_results.empty()) {
            numOutputSteps = static_cast<int>(m_sys.body_results.begin()->second.data.size());
        }

        //【发散截尾】柔体模态发散触发的优雅停机：引擎的 RecordStepResults 先于回调，
        // body_results 尾部 1-2 帧是发散态——动画/曲线按柔体干净帧数截尾
        if (m_flexDivergedTime >= 0.0 && !m_flexCaptures.empty()) {
            int cleanFrames = INT_MAX;
            for (const auto& cap : m_flexCaptures)
                cleanFrames = std::min(cleanFrames, (int)cap.data.times.size());
            if (cleanFrames > 0 && cleanFrames < numOutputSteps)
                numOutputSteps = cleanFrames;
            emit messageLogged(QString("柔性体模态坐标在 t=%1 s 发散，求解提前终止（已保留此前结果；可尝试缩短求解时长）。")
                .arg(m_flexDivergedTime, 0, 'f', 3));
        }

        resultData->m_frames.reserve(numOutputSteps);

        // 提取共享时间轴
        std::vector<double> sharedTimeAxis;
        sharedTimeAxis.reserve(numOutputSteps);
        if (!m_sys.body_results.empty()) {
            for (const auto& res : m_sys.body_results.begin()->second.data) {
                sharedTimeAxis.push_back(res.time);
            }
        }
        QVector3D lastpos;

        // 提前拿到 bodyManager，用来获取真实的全局初始质心
        auto& bodyManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionBodyManager>(Physical_MotionBody);

        // 重组装配渲染矩阵
        for (int step = 0; step < numOutputSteps; ++step)
        {
            MechMotionData::mMotionFrame frame;
            frame.m_time = sharedTimeAxis[step];

            for (const auto& bodyPair : m_sys.body_results)
            {
                // 大地是固定部件（part 名 "ground"），不属于任何 GUI 运动体，跳过
                if (bodyPair.first == "ground") continue;
                int bodyId = std::stoi(bodyPair.first);
                if (step >= static_cast<int>(bodyPair.second.data.size())) continue;
                const auto& bodyRes = bodyPair.second.data[step];
                const auto& state = bodyRes.cm;

                // 1. 获取几何体的【真实全局初始质心】
                QVector3D initial_cm(0, 0, 0);
                auto* body = bodyManager.getData(bodyId);
                if (body) {
                    initial_cm = body->getCenterOfMass();
                }

                // 2. 计算当前帧相对于初始帧的【相对位移】
                lastpos = QVector3D(state.dpos.x, state.dpos.y, state.dpos.z);

                // 3. 核心：严格遵循"相对平移 + 原地自转"的矩阵叠乘法则
                QMatrix4x4 mat;
                mat.setToIdentity();
                mat.translate(lastpos);
                mat.translate(initial_cm);
                QQuaternion q(state.rpos.e0(), state.rpos.e1(), state.rpos.e2(), state.rpos.e3());
                mat.rotate(q);
                mat.translate(-initial_cm);
                frame.m_bodyTransforms.insert({ bodyId, mat });

                // 4. 填充运动体完整帧数据（供曲线后处理）
                MechMotionData::mMotionBodyFrame bf;
                bf.m_position = QVector3D(state.pos.x, state.pos.y, state.pos.z);
                bf.m_velocity = QVector3D(state.vel.x, state.vel.y, state.vel.z);
                bf.m_acceleration = QVector3D(state.acc.x, state.acc.y, state.acc.z);
                bf.m_rotation = QQuaternion(state.rpos.e0(), state.rpos.e1(), state.rpos.e2(), state.rpos.e3());
                bf.m_angularVelocity = QVector3D(state.rvel.x, state.rvel.y, state.rvel.z);
                bf.m_angularAcceleration = QVector3D(state.racc.x, state.racc.y, state.racc.z);
                bf.m_displacement = QVector3D(state.dpos.x, state.dpos.y, state.dpos.z);
                bf.m_potentialEnergy = bodyRes.potential_energy;
                bf.m_kineticEnergy = bodyRes.kinetic_energy;
                frame.m_bodyFrames.insert({ bodyId, bf });
            }

            // 提取标记点结果 (仅处理用户标记点 UM_ 前缀)
            for (const auto& markerPair : m_sys.marker_results)
            {
                if (markerPair.first.substr(0, 3) != "UM_") continue;

                int markerId = std::stoi(markerPair.first.substr(3));
                const auto& history = markerPair.second;

                if (step >= static_cast<int>(history.data.size())) continue;

                const auto& mres = history.data[step];
                const auto& kin = mres.kinematic;

                MechMotionData::mMotionMarkerFrame mf;
                mf.m_markerId = markerId;
                mf.m_worldPosition = QVector3D(kin.pos.x, kin.pos.y, kin.pos.z);
                mf.m_worldRotation = QQuaternion(kin.rpos.e0(), kin.rpos.e1(), kin.rpos.e2(), kin.rpos.e3());
                mf.m_linearVelocity = QVector3D(kin.vel.x, kin.vel.y, kin.vel.z);
                mf.m_linearAcceleration = QVector3D(kin.acc.x, kin.acc.y, kin.acc.z);
                mf.m_angularVelocity = QVector3D(kin.rvel.x, kin.rvel.y, kin.rvel.z);
                mf.m_angularAcceleration = QVector3D(kin.racc.x, kin.racc.y, kin.racc.z);

                frame.m_markerFrames.insert({ markerId, mf });
            }

            // 提取运动副结果（耦合副以 "CP_" 前缀存储在 joint_results 中）
            for (const auto& jointPair : m_sys.joint_results)
            {
                const auto& history = jointPair.second;
                if (step >= static_cast<int>(history.data.size())) continue;

                const auto& jres = history.data[step];
                MechMotionData::mMotionJointFrame jf;

                auto fillNode = [](const auto& kin) {
                    MechMotionData::mMotionMarkerFrame mf;
                    mf.m_worldPosition = QVector3D(kin.pos.x, kin.pos.y, kin.pos.z);
                    mf.m_worldRotation = QQuaternion(kin.rpos.e0(), kin.rpos.e1(), kin.rpos.e2(), kin.rpos.e3());
                    mf.m_linearVelocity = QVector3D(kin.vel.x, kin.vel.y, kin.vel.z);
                    mf.m_linearAcceleration = QVector3D(kin.acc.x, kin.acc.y, kin.acc.z);
                    mf.m_angularVelocity = QVector3D(kin.rvel.x, kin.rvel.y, kin.rvel.z);
                    mf.m_angularAcceleration = QVector3D(kin.racc.x, kin.racc.y, kin.racc.z);
                    return mf;
                    };

                jf.m_nodeI = fillNode(jres.node_I);
                jf.m_nodeJ = fillNode(jres.node_J);
                // 引擎反力为 mN 系，结果统一以 N 系（N、N·mm）存储/显示
                jf.m_reactionForce = QVector3D(jres.reaction_force.x, jres.reaction_force.y, jres.reaction_force.z) * 0.001;
                jf.m_reactionTorque = QVector3D(jres.reaction_torque.x, jres.reaction_torque.y, jres.reaction_torque.z) * 0.001;

                if (jointPair.first.size() > 3 && jointPair.first.substr(0, 3) == "CP_") {
                    int couplingId = std::stoi(jointPair.first.substr(3));
                    frame.m_couplingFrames.insert({ couplingId, jf });
                }
                else {
                    int jointId = std::stoi(jointPair.first);
                    frame.m_jointFrames.insert({ jointId, jf });
                }
            }

            // 提取驱动结果
            for (const auto& drivePair : m_sys.motion_results)
            {
                int driveId = std::stoi(drivePair.first);
                const auto& history = drivePair.second;
                if (step >= static_cast<int>(history.data.size())) continue;

                const auto& dres = history.data[step];
                MechMotionData::mMotionDriveFrame df;

                const auto& kin = dres.abscoord;
                df.m_absCoord.m_worldPosition = QVector3D(kin.pos.x, kin.pos.y, kin.pos.z);
                df.m_absCoord.m_worldRotation = QQuaternion(kin.rpos.e0(), kin.rpos.e1(), kin.rpos.e2(), kin.rpos.e3());
                df.m_absCoord.m_linearVelocity = QVector3D(kin.vel.x, kin.vel.y, kin.vel.z);
                df.m_absCoord.m_linearAcceleration = QVector3D(kin.acc.x, kin.acc.y, kin.acc.z);
                df.m_absCoord.m_angularVelocity = QVector3D(kin.rvel.x, kin.rvel.y, kin.rvel.z);
                df.m_absCoord.m_angularAcceleration = QVector3D(kin.racc.x, kin.racc.y, kin.racc.z);

                df.m_imposedValue = dres.imposed_value;
                df.m_imposedSpeed = dres.imposed_speed;
                df.m_imposedAcc = dres.imposed_acc;
                // 转动驱动：imposed 系列引擎值为弧度（°/s、°/s²），转成度语义
                //（与面板输入一致，曲线显示不再换算）
                auto& driveMgr = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionDriveManager>(Physical_MotionDrive);
                if (auto* driveData = dynamic_cast<MechMotionData::mMotionJointDrive*>(driveMgr.getData(driveId)))
                    df.m_isRotation = (driveData->getMotionType() == MechMotionData::DriveMotionType::Rotation);
                if (df.m_isRotation) {
                    df.m_imposedValue *= 57.29577951308232;
                    df.m_imposedSpeed *= 57.29577951308232;
                    df.m_imposedAcc *= 57.29577951308232;
                }
                // 驱动力/力矩/反力：引擎 mN 系 → 结果统一 N 系（×0.001，与关节反力同口径，
                // 此前漏乘导致驱动反力曲线比真实大 1000 倍）
                df.m_actuationForce = QVector3D(dres.actuation_force.x, dres.actuation_force.y, dres.actuation_force.z) * 0.001;
                df.m_actuationTorque = QVector3D(dres.actuation_torque.x, dres.actuation_torque.y, dres.actuation_torque.z) * 0.001;
                df.m_reactionForce = QVector3D(dres.reaction_force.x, dres.reaction_force.y, dres.reaction_force.z) * 0.001;
                df.m_reactionTorque = QVector3D(dres.reaction_torque.x, dres.reaction_torque.y, dres.reaction_torque.z) * 0.001;

                frame.m_driveFrames.insert({ driveId, df });
            }

            resultData->m_frames.push_back(frame);

            // 更新动画解析进度
            if (step % std::max(1, numOutputSteps / 30) == 0) {
                // emit progressUpdated(50 + (step * 30 / numOutputSteps));
            }
        }

        // PID 控制历史与求解期日志回填
        for (const auto& rt : m_sys.pidControllers) {
            if (!rt.history.empty()) {
                resultData->m_pidHistories.insert({ rt.controllerId, rt.history });
            }
        }
        for (const auto& msg : m_sys.pidMessages) {
            emit messageLogged(QString::fromStdString(msg));
        }

        scenario->attachResult(resultData.release());
        emit messageLogged(QString("动画数据提取完毕，共 %1 帧。").arg(numOutputSteps));

        emit progressUpdated(100);
        emit solveFinished(m_scenarioId, true);
    }

    void mMotionSolverThread::doTransOutput()
    {
        // ==========================================
        // 3. 模拟求解过程 (基于解析几何的真实运动学解算)
        // ==========================================
        // 提取驱动角速度 (rad/s)
        double omega = 0.0;
        auto motionIt = m_sys.motions_map.find("1");
        if (motionIt != m_sys.motions_map.end()) {
            omega = motionIt->second.func_value; // 1.5708 rad/s (90度每秒)
        }
        auto& jointManager = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint);
        auto scenario = m_viewportModel->getModelData()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(m_scenarioId);

        // 模拟拿到了数据
        double endTime = scenario->getEndTime();
        double stepSize = scenario->getStepSize();

        if (stepSize <= 0) {
            emit messageLogged("错误：积分步长无效！");
            emit solveFinished(m_scenarioId, false);
            return;
        }

        int totalSteps = static_cast<int>(std::ceil(endTime / stepSize));
        emit messageLogged(QString("数据准备完毕。总计算步数: %1").arg(totalSteps));

        // 创建我们在上一节设计好的纯数据结果类
        auto* resultData = new MechMotionData::mMotionResultData();
        resultData->m_frames.reserve(totalSteps); // 预分配内存，极大提升性能

        //获取所有几何部件id
        auto partIds = m_viewportModel->getModelData()->getMeshData()->getAllPartIds();
        if (partIds.empty())
        {
            emit messageLogged("错误：没有模型");
            emit solveFinished(m_scenarioId, false);
            return;
        }

        for (int step = 0; step <= totalSteps; ++step)
        {
            if (this->isInterruptionRequested()) {
                delete resultData;
                emit solveFinished(m_scenarioId, false);
                return;
            }

            double currentTime = step * stepSize;
            QThread::msleep(1); // 模拟耗时

            MechMotionData::mMotionFrame currentFrame;
            currentFrame.m_time = currentTime;

            // 3.1 默认所有 Part 初始都待在原地 (Identity 变换)
            for (const auto& partPair : m_sys.parts_map) {
                if (partPair.first == "ground") continue; // 大地固定，不参与动画
                int partId = std::stoi(partPair.first);
                QMatrix4x4 initialMat;
                initialMat.setToIdentity();
                currentFrame.m_bodyTransforms.insert({ partId, initialMat });
            }

            // 3.2 遍历所有驱动 (Motions)，计算驱动带来的位姿改变
            for (const auto& motionPair : m_sys.motions_map)
            {
                const auto& motion = motionPair.second;

                // 目前只模拟匀速旋转驱动
                if (!motion.is_rotation || !motion.is_speed) continue;

                double omega = motion.func_value; // 获取角速度 (rad/s)
                double theta_deg = (omega * currentTime) * (180.0 / 3.14159265358979323846);

                /// --------------------------------------------------
            // 【核心拓扑推导】：智能判定固定端与运动端
            // --------------------------------------------------
            // 1. 找到驱动绑定的 Joint
                auto jointIt = m_sys.joints_map.find(motion.resolved_joint_id);
                if (jointIt == m_sys.joints_map.end()) continue;
                const auto& joint = jointIt->second;

                // 2. 找到 Joint 对应的两个 Marker
                auto markerI_It = m_sys.markers_map.find(joint->marker_I);
                auto markerJ_It = m_sys.markers_map.find(joint->marker_J);
                if (markerI_It == m_sys.markers_map.end() || markerJ_It == m_sys.markers_map.end()) continue;

                // 3. 提取对应的 Part 及其 fixed 属性
                if (markerI_It->second.part_id == "ground" || markerJ_It->second.part_id == "ground")
                    continue; // 大地端固定，无驱动位姿
                int partI_Id = std::stoi(markerI_It->second.part_id);
                int partJ_Id = std::stoi(markerJ_It->second.part_id);

                auto partI_It = m_sys.parts_map.find(std::to_string(partI_Id));
                auto partJ_It = m_sys.parts_map.find(std::to_string(partJ_Id));
                if (partI_It == m_sys.parts_map.end() || partJ_It == m_sys.parts_map.end()) continue;

                bool isIFixed = partI_It->second.fixed;
                bool isJFixed = partJ_It->second.fixed;

                // 4. 核心逻辑：谁才是真正该动的那个？(Moving vs Base)
                int movingPartId = -1;
                int basePartId = -1;
                double actual_theta_deg = theta_deg; // 实际生效的旋转角度

                // 指向作为基准的那个 Marker (用于计算旋转原点)
                auto markerBase_It = markerJ_It;

                if (!isIFixed && isJFixed) {
                    /* 正常装配：I 动，J 固定 (I 相对 J 运动 theta) */
                    movingPartId = partI_Id;
                    basePartId = partJ_Id;
                    markerBase_It = markerJ_It;
                }
                else if (isIFixed && !isJFixed) {
                    /* 反向装配：I 固定，J 动。
                       根据相对运动原理，I 相对 J 转了 theta，意味着 J 相对 I 转了 -theta！ */
                    movingPartId = partJ_Id;
                    basePartId = partI_Id;
                    markerBase_It = markerI_It;
                    actual_theta_deg = -theta_deg; // 【关键】角度取反
                }
                else if (!isIFixed && !isJFixed) {
                    /* 两个都不固定：在真实的动力学求解中，这是一个极其复杂的相对运动方程。
                    在 V1.0 的模拟展示器中，暂且把 J 视作相对静止的参考系。 */
                    movingPartId = partI_Id;
                    basePartId = partJ_Id;
                    markerBase_It = markerJ_It;
                }
                else {
                    /* 两个都固定了：机构锁死，完全没有自由度，直接跳过本驱动的计算 */
                    continue;
                }

                // 4. 获取 Base Part 和 Marker J 的信息
                auto basePartIt = m_sys.parts_map.find(std::to_string(basePartId));
                if (basePartIt == m_sys.parts_map.end()) continue;
                // --------------------------------------------------
                // 【空间几何计算】：求出全局旋转中心和旋转轴
                // --------------------------------------------------
                // 旋转中心点 (Global Origin) = Base Part 的全局位置 + Marker J 的相对位置
                QVector3D globalPivot(
                    basePartIt->second.loc[0] + markerBase_It->second.loc[0],
                    basePartIt->second.loc[1] + markerBase_It->second.loc[1],
                    basePartIt->second.loc[2] + markerBase_It->second.loc[2]
                );

                // 2. 真实的全局旋转轴 (Global Axis)
                // 在多体动力学语义中，旋转副的轴永远是 Marker 的局部 Z 轴！
                // 我们通过组合 Part 和 Marker 的四元数，将局部 Z 轴转换为全局矢量
                QQuaternion qPart = QQuaternion::fromEulerAngles(
                    basePartIt->second.rot[0],
                    basePartIt->second.rot[1],
                    basePartIt->second.rot[2]
                );
                QQuaternion qMarker = QQuaternion::fromEulerAngles(
                    markerBase_It->second.rot[0],
                    markerBase_It->second.rot[1],
                    markerBase_It->second.rot[2]
                );

                QQuaternion qGlobal = qPart * qMarker;
                // 提取 Z 轴 (0,0,1) 经过全局旋转后的方向，这就是真正的旋转轴！
                QVector3D globalAxis = qGlobal * QVector3D(0.0, 0.0, 1.0);
                globalAxis.normalize();

                // 2. 获取真实的全局旋转轴 (Global Axis)
                //QVector3D globalAxis(0.0, 0.0, 1.0); // 兜底默认值（防止意外）
                // 利用推断出的 resolved_joint_id，动态回溯获取用户在前处理输入的真实矢量
                //auto* realJoint = dynamic_cast<MechMotionData::mMotionJointBasic*>(jointManager.getData(std::stoi(motion.resolved_joint_id)));
                //if (realJoint) {
                //    globalPivot = realJoint->getOrigin();
                //    // 拿到真实的轴，并强制归一化（非常重要，防止用户输入的矢量长度不为1导致矩阵变形）
                //    globalAxis = realJoint->getAxis().normalized();
                //}



                // --------------------------------------------------
                // 【矩阵变换】：应用任意点、真实轴旋转矩阵！
                // --------------------------------------------------
                QMatrix4x4 matMoving;
                matMoving.setToIdentity();
                // 严格按照：移回原位 -> 绕真实轴旋转 -> 移到世界原点 的顺序
                matMoving.translate(globalPivot);
                matMoving.rotate(actual_theta_deg, globalAxis); // 这里传入的就是真实的 globalAxis 了！
                matMoving.translate(-globalPivot);

                // 覆盖原有的 Identity 矩阵
                currentFrame.m_bodyTransforms[movingPartId] = matMoving;
            }

            resultData->m_frames.push_back(currentFrame);

            if (step % std::max(1, totalSteps / 100) == 0 || step == totalSteps) {
                emit progressUpdated(static_cast<int>((static_cast<double>(step) / totalSteps) * 100.0));
            }
        }

        // ==========================================
        // 6. 计算完成，挂载结果到大节点
        // ==========================================
        emit messageLogged("计算完成，正在保存结果数据...");

        // 伪代码：挂载结果
        scenario->attachResult(resultData);

        emit progressUpdated(100);
        emit solveFinished(m_scenarioId, true);
    }
}
