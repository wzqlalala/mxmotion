#pragma once

#include "RigidMotion.h"

#include <array>
#include <optional>
#include <vector>

namespace motion { namespace utils { struct MnfData; } }

namespace MotionOcc {

RigidFlexResult makeMnfModePreview(const motion::utils::MnfData& data);
bool readMnfModePreview(const QString& path, RigidFlexResult& result, QString& error);

enum class MnfMatchConfidence { NoCandidates, Confident, Ambiguous, Review };

struct MnfInterfaceCandidate {
    int nodeId;
    std::array<double, 3> worldMm;
    double distanceMm;
};

struct MnfInterfaceMatch {
    std::vector<MnfInterfaceCandidate> candidates;
    MnfMatchConfidence confidence = MnfMatchConfidence::NoCandidates;
    std::optional<double> gapMm;
    double toleranceMm = 0;
};

// Rank the original interface node IDs in the CAD world frame. Confidence is
// only a geometric suggestion; callers retain the final binding decision.
MnfInterfaceMatch matchMnfInterfaceNodes(const RigidFlexResult& preview,
    const std::array<double, 3>& referenceTranslationMm,
    const std::array<double, 3>& referenceEuler321Deg,
    const std::array<double, 3>& jointOriginMm);

} // namespace MotionOcc
