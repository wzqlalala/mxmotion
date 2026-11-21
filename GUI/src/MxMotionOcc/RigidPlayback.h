#pragma once

#include "RigidMotion.h"
#include "NativeMotionSolver.h"
#include "FemMesh.h"

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <set>

namespace MotionOcc {

// A slider position is a result frame index, not an assumed uniform time step.
// Interpolation between frames follows mMotionResultQueryService: world COM
// translation plus quaternion slerp, then conversion from the original CAD frame.
struct RigidPlaybackSample {
    double seconds = 0.0;
    int frameIndex = 0;
    std::map<int, QMatrix4x4> partPoses;
    std::map<int, FemMesh> flexMeshes;
    std::set<int> flexHiddenParts;
};

// Standalone construction curves are legitimate global CAD references. The
// solver converts their coordinates into the selected body's local frame, so
// their display must use that same body's transform. Physical CAD ownership
// always takes precedence over these display-only reference associations.
inline bool nativePlaybackCurveReferences(const RigidInput& input, std::map<int,int>& references,
                                          QString& error)
{
    std::map<int,int> next;
    if (!input.nativeInput) { references=std::move(next); return true; }
    const auto& document=input.nativeInput->document;
    std::set<int> owned, bodies;
    for (const auto& parts : document.bodyPartBindings) for (int part : parts) owned.insert(part);
    for (const auto& body : input.bodies) bodies.insert(body.body.id);
    const auto add=[&](const QJsonObject& joint, const char* field, const char* bodyField) {
        const int bodyId=joint.value(bodyField).toInt(-1);
        for (const auto& value : joint.value(field).toArray()) {
            const int partId=value.toObject().value("partId").toInt(-1);
            if (partId<1) { error=QStringLiteral("独立曲线动画引用的 CAD 部件编号无效。"); return false; }
            if (owned.count(partId)) continue;
            if (bodyId!=-1 && !bodies.count(bodyId)) {
                error=QStringLiteral("独立曲线部件 %1 引用的运动体 %2 没有动画状态。").arg(partId).arg(bodyId); return false;
            }
            const auto inserted=next.emplace(partId,bodyId);
            if (!inserted.second && inserted.first->second!=bodyId) {
                error=QStringLiteral("独立曲线部件 %1 同时引用运动体 %2 与 %3，无法确定唯一动画姿态；请使用分别绑定的曲线部件。")
                    .arg(partId).arg(inserted.first->second).arg(bodyId); return false;
            }
        }
        return true;
    };
    for (const auto& value : document.collection("joints")) {
        const auto joint=value.toObject(); const auto type=joint.value("jointType").toString();
        if (type=="PointOnCurve" && !add(joint,"occCurveEdges","baseBodyId")) return false;
        if (type=="CurveOnCurve" && (!add(joint,"occCurveIEdges","actionBodyId")
            || !add(joint,"occCurveJEdges","baseBodyId"))) return false;
    }
    references=std::move(next); return true;
}

// Validate a result once when entering animation. Keeping this scan out of the
// per-frame sampler avoids an O(frameCount) pass on every 16 ms timer tick.
inline bool validateRigidPlayback(const RigidInput& input, const RigidResult& result,
                                  QString& error)
{
    if (input.scenario.id < (input.nativeInput ? 0 : 1) || result.scenarioId != input.scenario.id) {
        error = QStringLiteral("动画结果与求解方案编号不一致。");
        return false;
    }
    if (result.frames.empty()) {
        error = QStringLiteral("动画结果为空。");
        return false;
    }
    for (std::size_t i = 0; i < result.frames.size(); ++i) {
        if (!std::isfinite(result.frames[i].seconds)
            || (i && result.frames[i].seconds <= result.frames[i - 1].seconds)) {
            error = QStringLiteral("动画结果时间必须为严格递增的有限数值。");
            return false;
        }
    }
    std::map<int,int> curveReferences;
    if (!nativePlaybackCurveReferences(input,curveReferences,error)) return false;
    for (const auto& flex : result.flexBodies) {
        if (!input.nativeInput || flex.referenceNodes.empty() || flex.referenceNodes.size() % 3
            || flex.surfaceTriangles.empty() || flex.surfaceTriangles.size() % 3
            || flex.nodePositions.size() != result.frames.size()
            || input.nativeInput->document.bodyPartBindings.value(flex.bodyId).isEmpty()) {
            error = QStringLiteral("柔性体 %1 的动画网格或结果帧不完整。").arg(flex.flexId); return false;
        }
        for (int index : flex.surfaceTriangles)
            if (index < 0 || std::size_t(index) >= flex.referenceNodes.size() / 3) {
                error = QStringLiteral("柔性体 %1 的表面网格索引无效。").arg(flex.flexId); return false;
            }
        for (const auto& nodes : flex.nodePositions)
            if (nodes.size() != flex.referenceNodes.size()
                || !std::all_of(nodes.begin(), nodes.end(), [](double value) { return std::isfinite(value); })) {
                error = QStringLiteral("柔性体 %1 的位移结果不完整或非有限。").arg(flex.flexId); return false;
            }
    }
    error.clear();
    return true;
}

// Requires a result accepted by validateRigidPlayback. The queried states are
// still checked on each sample, so missing IDs never turn into out-of-bounds
// accesses or a transform belonging to another body.
inline bool sampleRigidPlayback(const RigidInput& input, const RigidResult& result,
                                double requestedTime, RigidPlaybackSample& sample,
                                QString& error)
{
    const auto fail = [&error](const QString& reason) { error = reason; return false; };
    if (input.scenario.id < (input.nativeInput ? 0 : 1) || result.scenarioId != input.scenario.id)
        return fail(QStringLiteral("动画结果与求解方案编号不一致。"));
    const auto& frames = result.frames;
    if (frames.empty() || !std::isfinite(requestedTime))
        return fail(QStringLiteral("动画结果为空或时间无效。"));
    if (!std::isfinite(frames.front().seconds) || !std::isfinite(frames.back().seconds)
        || frames.front().seconds > frames.back().seconds)
        return fail(QStringLiteral("动画结果时间范围无效。"));

    RigidPlaybackSample next;
    next.seconds = std::clamp(requestedTime, frames.front().seconds, frames.back().seconds);
    const auto right = std::lower_bound(frames.begin(), frames.end(), next.seconds,
        [](const RigidFrame& frame, double time) { return frame.seconds < time; });
    const auto second = right == frames.end() ? std::prev(frames.end()) : right;
    // Exact physical frames must not depend on the contents of a neighbour.
    const auto first = second == frames.begin() || second->seconds == next.seconds
        ? second : std::prev(second);
    next.frameIndex = int(std::distance(frames.begin(), second));
    if (!std::isfinite(first->seconds) || !std::isfinite(second->seconds)
        || (first != second && second->seconds <= first->seconds))
        return fail(QStringLiteral("动画结果帧时间无效。"));
    const float ratio = first == second ? 0.0f
        : float((next.seconds - first->seconds) / (second->seconds - first->seconds));

    std::map<int, const BodyState*> leftBodies, rightBodies;
    for (const auto& body : first->bodies)
        if (!leftBodies.emplace(body.bodyId, &body).second)
            return fail(QStringLiteral("动画结果包含重复的运动体编号。"));
    for (const auto& body : second->bodies)
        if (!rightBodies.emplace(body.bodyId, &body).second)
            return fail(QStringLiteral("动画结果包含重复的运动体编号。"));

    const auto finite = [](const auto& values) {
        return std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); });
    };
    const auto rotation = [](const Quaterniond& q) {
        return QQuaternion(float(q[0]), float(q[1]), float(q[2]), float(q[3]));
    };
    std::map<int,int> curveReferences;
    if (!nativePlaybackCurveReferences(input,curveReferences,error)) return false;
    std::map<int,QMatrix4x4> bodyPoses;
    for (const auto& body : input.bodies) {
        const auto a = leftBodies.find(body.body.id), b = rightBodies.find(body.body.id);
        if (a == leftBodies.end() || b == rightBodies.end())
            return fail(QStringLiteral("动画帧缺少运动体 %1 的结果。").arg(body.body.id));
        const auto& left = *a->second;
        const auto& rightBody = *b->second;
        if (!finite(left.positionMm) || !finite(rightBody.positionMm)
            || !finite(left.rotation) || !finite(rightBody.rotation)
            || !finite(body.properties.centerMm))
            return fail(QStringLiteral("运动体 %1 的动画位姿包含非有限数值。").arg(body.body.id));
        const auto qa = rotation(left.rotation), qb = rotation(rightBody.rotation);
        if (!std::isfinite(qa.lengthSquared()) || !std::isfinite(qb.lengthSquared())
            || qa.lengthSquared() <= 1e-12f || qb.lengthSquared() <= 1e-12f)
            return fail(QStringLiteral("运动体 %1 的动画姿态四元数无效。").arg(body.body.id));

        QMatrix4x4 pose;
        pose.translate(float(left.positionMm[0] + (rightBody.positionMm[0] - left.positionMm[0]) * ratio),
                       float(left.positionMm[1] + (rightBody.positionMm[1] - left.positionMm[1]) * ratio),
                       float(left.positionMm[2] + (rightBody.positionMm[2] - left.positionMm[2]) * ratio));
        pose.rotate(QQuaternion::slerp(qa.normalized(), qb.normalized(), ratio));
        // CAD vertices and the stored initial COM are already in global mm.
        // Subtract the initial COM once, after applying the current world pose.
        const auto& center = body.properties.centerMm;
        pose.translate(-float(center[0]), -float(center[1]), -float(center[2]));
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                if (!std::isfinite(pose(row, column)))
                    return fail(QStringLiteral("运动体 %1 的动画位姿超出渲染范围。").arg(body.body.id));
        bodyPoses.emplace(body.body.id,pose);
        const auto parts = input.nativeInput ? input.nativeInput->document.bodyPartBindings.value(body.body.id)
            : QVector<int>{body.body.partId};
        if (parts.isEmpty()) {
            // A virtual body has physical states and may drive other bodies,
            // but deliberately has no CAD instance to animate.
            const auto source = input.nativeInput ? input.nativeInput->document.entity("bodies", body.body.id) : QJsonObject{};
            if (!source.isEmpty() && source.value("geoEntityIds").toArray().isEmpty()) continue;
            return fail(QStringLiteral("运动体 %1 缺少几何绑定。").arg(body.body.id));
        }
        for (int part : parts)
            if (!next.partPoses.emplace(part, pose).second)
                return fail(QStringLiteral("动画结果中一个部件关联了多个运动体。"));
    }
    for (const auto& reference : curveReferences)
        next.partPoses.emplace(reference.first,reference.second==-1 ? QMatrix4x4{} : bodyPoses.at(reference.second));
    const auto leftIndex = std::size_t(std::distance(frames.begin(), first));
    const auto rightIndex = std::size_t(std::distance(frames.begin(), second));
    for (const auto& flex : result.flexBodies) {
        if (!input.nativeInput || rightIndex >= flex.nodePositions.size())
            return fail(QStringLiteral("柔性体动画帧缺失。"));
        const auto parts = input.nativeInput->document.bodyPartBindings.value(flex.bodyId);
        const auto& a = flex.nodePositions[leftIndex]; const auto& b = flex.nodePositions[rightIndex];
        if (parts.isEmpty() || a.size() != b.size() || a.size() != flex.referenceNodes.size() || a.size() % 3)
            return fail(QStringLiteral("柔性体动画网格与绑定不一致。"));
        FemMesh mesh; mesh.nodes.reserve(a.size() / 3); mesh.surface.reserve(flex.surfaceTriangles.size() / 3);
        for (std::size_t i = 0; i < a.size(); i += 3) {
            std::array<double, 3> point{};
            for (int j = 0; j < 3; ++j) {
                point[std::size_t(j)] = a[i + std::size_t(j)] + (b[i + std::size_t(j)] - a[i + std::size_t(j)]) * ratio;
                if (!std::isfinite(point[std::size_t(j)])) return fail(QStringLiteral("柔性体动画包含非有限位置。"));
            }
            mesh.nodes.push_back(point);
        }
        for (std::size_t i = 0; i + 2 < flex.surfaceTriangles.size(); i += 3) {
            std::array<std::uint32_t, 3> triangle{};
            for (int j = 0; j < 3; ++j) {
                const int index = flex.surfaceTriangles[i + std::size_t(j)];
                if (index < 0 || std::size_t(index) >= mesh.nodes.size()) return fail(QStringLiteral("柔性体动画表面索引无效。"));
                triangle[std::size_t(j)] = std::uint32_t(index);
            }
            mesh.surface.push_back(triangle);
        }
        next.flexMeshes.emplace(parts.first(), std::move(mesh));
        for (int part : parts) { next.partPoses.erase(part); if (part != parts.first()) next.flexHiddenParts.insert(part); }
    }
    sample = std::move(next);
    error.clear();
    return true;
}

} // namespace MotionOcc
