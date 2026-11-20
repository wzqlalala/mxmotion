//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSurfaceBoundary.h"

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopAbs_Orientation.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_Surface.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace MechMainWindow
{
    int buildPosBoundaryWalls(const TopoDS_Face& face,
                              double offX, double offY, double offZ,
                              std::vector<double>& outNodes,
                              std::vector<int>& outWallEdges,
                              std::vector<double>& outInward)
    {
        if (face.IsNull())
            return 0;
        // 单参数版本已应用 face 的 Location，与 pcurve 参数求值一致
        const Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
        if (surf.IsNull())
            return 0;

        // 外轮廓（OCC 直接给出，替代"首条 wire"假设）
        const TopoDS_Wire outer = BRepTools::OuterWire(face);
        if (outer.IsNull())
            return 0;

        auto appendNode = [&](const gp_Pnt& p) -> int {
            const int cid = static_cast<int>(outNodes.size() / 3);
            outNodes.push_back(p.X() - offX);
            outNodes.push_back(p.Y() - offY);
            outNodes.push_back(p.Z() - offZ);
            return cid;
        };
        // 节点为 COM 局部坐标（平移不改变方向量）
        auto nodePt = [&](int cid) -> gp_Pnt {
            return gp_Pnt(outNodes[static_cast<size_t>(cid) * 3],
                          outNodes[static_cast<size_t>(cid) * 3 + 1],
                          outNodes[static_cast<size_t>(cid) * 3 + 2]);
        };

        // 每条边采样出的节点与 UV（局部于本边，两个数组索引对齐）
        struct EdgeSamples {
            std::vector<int> nodeIds;
            std::vector<std::pair<double, double>> uvs;
        };
        std::vector<EdgeSamples> edgeSamples;
        // 外轮廓 UV 有向面积（shoelace）：规范面外环应为正（相对自然 UV 逆时针）。
        // 个别来源面环存储方向不规范时按此整体翻向，代替逐段探针。
        double uvArea2 = 0.0;
        std::pair<double, double> prevUV(0.0, 0.0);
        bool havePrev = false;

        for (BRepTools_WireExplorer exE(outer, face); exE.More(); exE.Next()) {
            const TopoDS_Edge& edge = exE.Current();
            if (edge.IsNull() || BRep_Tool::Degenerated(edge))
                continue;
            // 接缝边（圆柱等封闭面的缝合线，面上有两条 pcurve）不是物理边界
            if (BRep_Tool::IsClosed(edge, face))
                continue;

            Standard_Real u0 = 0.0, u1 = 0.0;
            const Handle(Geom2d_Curve) c2d = BRep_Tool::CurveOnSurface(edge, face, u0, u1);
            if (c2d.IsNull() || u1 - u0 < 1e-12)
                continue;
            // 边在环内反向存储时参数降序行走，保证沿环存储方向遍历
            // （"材料在行走方向左侧"约定成立的前提）
            if (edge.Orientation() == TopAbs_REVERSED)
                std::swap(u0, u1);

            // 采样密度按 3D 弧长（约每 0.5mm 一点）
            BRepAdaptor_Curve curve(edge);
            const double eLen = GCPnts_AbscissaPoint::Length(
                curve, curve.FirstParameter(), curve.LastParameter());
            if (eLen < 1e-9)
                continue;
            const int nSamp = std::max(2, static_cast<int>(eLen / 0.5) + 1);

            EdgeSamples es;
            es.nodeIds.reserve(nSamp);
            es.uvs.reserve(nSamp);
            for (int k = 0; k < nSamp; ++k) {
                const double t = u0 + (u1 - u0) * k / (nSamp - 1);
                const gp_Pnt2d uv = c2d->Value(t);
                es.uvs.emplace_back(uv.X(), uv.Y());
                es.nodeIds.push_back(appendNode(surf->Value(uv.X(), uv.Y())));
                if (havePrev)
                    uvArea2 += prevUV.first * uv.Y() - uv.X() * prevUV.second;
                prevUV = es.uvs.back();
                havePrev = true;
            }
            edgeSamples.push_back(std::move(es));
        }

        const bool flipAll = (uvArea2 < -1e-9);

        int wallCount = 0;
        for (const EdgeSamples& es : edgeSamples) {
            if (es.nodeIds.size() < 2)
                continue;
            // 近共线合并（<30°，cos 阈值 0.866）：0.5mm 采样会产生大量段，
            // 墙数爆炸会破坏求解首步收敛（首帧跳变）
            std::vector<std::pair<size_t, size_t>> segs; // 弦两端在采样序列中的局部下标
            {
                size_t a0 = 0;
                while (a0 + 1 < es.nodeIds.size()) {
                    size_t a1 = a0 + 1;
                    while (a1 + 1 < es.nodeIds.size()) {
                        const gp_Pnt pA0 = nodePt(es.nodeIds[a0]);
                        const gp_Pnt pA1 = nodePt(es.nodeIds[a1]);
                        const gp_Pnt pA2 = nodePt(es.nodeIds[a1 + 1]);
                        const gp_Vec chord(pA0, pA2);
                        const gp_Vec nextSeg(pA1, pA2);
                        const double cl = chord.Magnitude(), sl = nextSeg.Magnitude();
                        if (cl < 1e-9 || sl < 1e-9)
                            break;
                        if (chord.Dot(nextSeg) / (cl * sl) < 0.866)
                            break;
                        ++a1;
                    }
                    segs.emplace_back(a0, a1);
                    a0 = a1;
                }
            }

            for (const auto& seg : segs) {
                const gp_Pnt A = nodePt(es.nodeIds[seg.first]);
                const gp_Pnt B = nodePt(es.nodeIds[seg.second]);
                gp_Vec t3(A, B);
                const double tlen = t3.Magnitude();
                if (tlen < 1e-9)
                    continue;
                t3.Divide(tlen);

                // 弦中点 UV 处曲面自然法向 N = Su × Sv
                const double umid = 0.5 * (es.uvs[seg.first].first + es.uvs[seg.second].first);
                const double vmid = 0.5 * (es.uvs[seg.first].second + es.uvs[seg.second].second);
                gp_Pnt pmid;
                gp_Vec su, sv;
                surf->D1(umid, vmid, pmid, su, sv);
                gp_Vec n = su.Crossed(sv);
                if (n.Magnitude() < 1e-12)
                    continue;
                n.Normalize();

                // 材料在行走方向左侧 ⇒ inward = N × T
                gp_Vec inward = n.Crossed(t3);
                if (inward.Magnitude() < 1e-9)
                    continue;
                inward.Normalize();
                if (flipAll)
                    inward.Reverse();

                outWallEdges.push_back(es.nodeIds[seg.first]);
                outWallEdges.push_back(es.nodeIds[seg.second]);
                outInward.push_back(inward.X());
                outInward.push_back(inward.Y());
                outInward.push_back(inward.Z());
                ++wallCount;
            }
        }
        return wallCount;
    }
}
