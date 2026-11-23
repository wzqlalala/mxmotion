#include "NativeMnfPreview.h"

#include "motion/core/Frame.h"
#include "motion/utils/Parsermnf.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace MotionOcc {

RigidFlexResult makeMnfModePreview(const motion::utils::MnfData& data)
{
    const auto& nodes = data.matrices.at("SELNOD").data;
    const auto& modes = data.matrices.at("SELMOD").data;
    const auto& translation = data.matrices.at("T_MODE").data;
    const double lengthToMm = data.units.si_length * 1000.0;

    RigidFlexResult result;
    result.flexId = -1;
    result.bodyId = -1;
    result.modeCount = static_cast<int>(modes.rows());
    result.surfaceTriangles = data.surface_triangles;
    auto& basis = result.modalBasis;
    basis.requestedCount = data.total_modes - data.rigid_modes;
    basis.kind = QStringLiteral("MNF");
    result.referenceNodes.reserve(static_cast<std::size_t>(nodes.rows()) * 3);
    basis.shapes.reserve(static_cast<std::size_t>(translation.size()));
    std::vector<int> interfaceRows;
    for (Eigen::Index node = 0; node < nodes.rows(); ++node) {
        result.sourceNodeIds.push_back(static_cast<int>(nodes(node, 0)));
        if (nodes(node, 4) != 0.0)
            interfaceRows.push_back(static_cast<int>(node));
        for (Eigen::Index component = 0; component < 3; ++component) {
            result.referenceNodes.push_back(nodes(node, component + 1) * lengthToMm);
            for (Eigen::Index mode = 0; mode < modes.rows(); ++mode)
                basis.shapes.push_back(translation(component, node * modes.rows() + mode) * lengthToMm);
        }
    }
    if (!interfaceRows.empty())
        basis.attachmentGroups.push_back(std::move(interfaceRows));
    for (Eigen::Index mode = 0; mode < modes.rows(); ++mode) {
        basis.sourceModeIds.push_back(static_cast<int>(modes(mode, 0)));
        basis.frequenciesHz.push_back(modes(mode, 1) / data.units.si_time);
    }
    if (data.rigid_modes > 0)
        basis.diagnostics.append(QStringLiteral("文件中的 %1 阶近零刚体模态已从预览中筛除。").arg(data.rigid_modes));

    QString error;
    if (!validateFlexModes(result, error))
        throw std::runtime_error(error.toUtf8().constData());
    return result;
}

bool readMnfModePreview(const QString& path, RigidFlexResult& result, QString& error)
{
    try {
        motion::utils::MnfParseOptions options;
        options.include_all_nodes = true;
        options.include_surface_topology = true;
        options.include_quadratic_invariants = false;
        result = makeMnfModePreview(motion::utils::MotionParserMnf().Parse(path.toUtf8().toStdString(), options));
        error.clear();
        return true;
    } catch (const std::exception& exception) {
        error = QString::fromUtf8(exception.what());
        return false;
    }
}

MnfInterfaceMatch matchMnfInterfaceNodes(const RigidFlexResult& preview,
    const std::array<double, 3>& referenceTranslationMm,
    const std::array<double, 3>& referenceEuler321Deg,
    const std::array<double, 3>& jointOriginMm)
{
    const auto finite = [](const auto& xyz) {
        return std::all_of(xyz.begin(), xyz.end(), [](double value) { return std::isfinite(value); });
    };
    if (!finite(referenceTranslationMm) || !finite(referenceEuler321Deg) || !finite(jointOriginMm))
        throw std::invalid_argument("MNF 匹配的参考位姿或运动副位置包含无效数值。");
    if (preview.referenceNodes.size() % 3 != 0 ||
        preview.referenceNodes.size() / 3 != preview.sourceNodeIds.size())
        throw std::invalid_argument("MNF 匹配所需的节点坐标与编号数量不一致。");

    // Same intrinsic yaw-pitch-roll composition as NativeMotionSolver.
    constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
    const auto rotation = motion::Q_from_AngZ(referenceEuler321Deg[2] * degreesToRadians) *
        motion::Q_from_AngY(referenceEuler321Deg[1] * degreesToRadians) *
        motion::Q_from_AngX(referenceEuler321Deg[0] * degreesToRadians);
    const motion::MotionFrame<> referencePose(motion::MotionVector<>(
        referenceTranslationMm[0], referenceTranslationMm[1], referenceTranslationMm[2]), rotation);
    const auto distance = [](const std::array<double, 3>& first, const std::array<double, 3>& second) {
        return std::hypot(first[0] - second[0], first[1] - second[1], first[2] - second[2]);
    };

    MnfInterfaceMatch result;
    std::set<int> rows;
    for (const auto& group : preview.modalBasis.attachmentGroups)
        rows.insert(group.begin(), group.end());
    std::set<int> nodeIds;
    result.candidates.reserve(rows.size());
    for (int row : rows) {
        if (row < 0 || static_cast<std::size_t>(row) >= preview.sourceNodeIds.size())
            throw std::invalid_argument("MNF 接口节点索引越界。");
        const int nodeId = preview.sourceNodeIds[row];
        if (nodeId < 0 || !nodeIds.insert(nodeId).second)
            throw std::invalid_argument("MNF 接口节点编号无效或重复。");
        const auto* local = preview.referenceNodes.data() + 3 * static_cast<std::size_t>(row);
        const auto position = referencePose.TransformPointLocalToParent(motion::MotionVector<>(local[0], local[1], local[2]));
        const std::array<double, 3> worldMm{position.x(), position.y(), position.z()};
        const double distanceMm = distance(worldMm, jointOriginMm);
        if (!finite(worldMm) || !std::isfinite(distanceMm))
            throw std::invalid_argument("MNF 接口节点坐标无法转换到 CAD 世界坐标系。");
        result.candidates.push_back({nodeId, worldMm, distanceMm});
    }
    if (result.candidates.empty()) return result;

    std::sort(result.candidates.begin(), result.candidates.end(), [](const auto& first, const auto& second) {
        if (first.distanceMm != second.distanceMm) return first.distanceMm < second.distanceMm;
        return first.nodeId < second.nodeId;
    });
    const auto& nearest = result.candidates.front();
    result.toleranceMm = 0.05; // Used when the file exports only one interface node.
    if (result.candidates.size() > 1) {
        const auto& next = result.candidates[1];
        result.gapMm = next.distanceMm - nearest.distanceMm;
        // Require a close local match while keeping a bounded absolute tolerance.
        result.toleranceMm = std::clamp(0.05 * distance(nearest.worldMm, next.worldMm), 1e-5, 0.5);
        const double tieToleranceMm = std::max(1e-6, 0.1 * result.toleranceMm);
        if (*result.gapMm <= tieToleranceMm) {
            result.confidence = MnfMatchConfidence::Ambiguous;
            return result;
        }
    }
    result.confidence = nearest.distanceMm <= result.toleranceMm
        ? MnfMatchConfidence::Confident : MnfMatchConfidence::Review;
    return result;
}

} // namespace MotionOcc
