#pragma once

#include "RigidMotion.h"

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>

namespace MotionOcc {

// A slider position is a result frame index, not an assumed uniform time step.
// Interpolation between frames follows mMotionResultQueryService: world COM
// translation plus quaternion slerp, then conversion from the original CAD frame.
struct RigidPlaybackSample {
    double seconds = 0.0;
    int frameIndex = 0;
    std::map<int, QMatrix4x4> partPoses;
};

// Validate a result once when entering animation. Keeping this scan out of the
// per-frame sampler avoids an O(frameCount) pass on every 16 ms timer tick.
inline bool validateRigidPlayback(const RigidInput& input, const RigidResult& result,
                                  QString& error)
{
    if (input.scenario.id <= 0 || result.scenarioId != input.scenario.id) {
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
    if (input.scenario.id <= 0 || result.scenarioId != input.scenario.id)
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
        if (!next.partPoses.emplace(body.body.partId, pose).second)
            return fail(QStringLiteral("动画结果中一个部件关联了多个运动体。"));
    }
    sample = std::move(next);
    error.clear();
    return true;
}

} // namespace MotionOcc
