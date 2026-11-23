#pragma once

#include "RigidMotion.h"
#include <QJsonObject>

namespace MotionOcc {

struct NativeLoadSource {
    QString collection; // "joints", "couplings" or "drives".
    int id = -1;
};
enum class NativeLoadEnd { I, J };
enum class NativeLoadFrames { Maximum, Specified, All };
enum class NativeLoadFormat { Csv, NastranBdf };

struct NativeLoadExportOptions {
    QString name = QStringLiteral("LoadCase_1");
    NativeLoadEnd end = NativeLoadEnd::I;
    NativeLoadFrames frames = NativeLoadFrames::Maximum;
    NativeLoadFormat format = NativeLoadFormat::Csv;
    std::size_t frameIndex = 0;
    int nodeId = 0; // Positive FEM grid ID required for BDF.
    double scale = 1;
    std::array<bool,6> components{{true,true,true,true,true,true}};
    // If an actual target point is supplied, transfer the moment with r x F.
    // Otherwise moments are about the selected end's marker, stated in output.
    bool useTargetPoint = false;
    Vector3d targetPointMm{};
    // -1: explicit fixed global point, preserving the original API.
    // >=0: point is initial CAD world coordinates on this rigid body; every
    // accepted frame transforms it with the body's actual pose and initial COM.
    // Flexible-node mapping is a separate capability and must not be inferred
    // from an interface rigid-body pose.
    int targetBodyId = -1;
};

struct NativeLoadSample {
    double seconds = 0;
    Vector3d pointMm{}, forceN{}, torqueNmm{};
};

// Physical constraint wrench acting on the selected end, in the global frame.
// Raw ReactionForce/ReactionTorque channels remain unchanged for compatibility.
bool nativeLoadSeries(const NativeMotionInput& input, const RigidResult& result,
                      const NativeLoadSource& source, NativeLoadEnd end,
                      std::vector<NativeLoadSample>& samples, QString& error);
bool exportNativeLoads(const QString& path, const NativeMotionInput& input,
                       const RigidResult& result, const NativeLoadSource& source,
                       const NativeLoadExportOptions& options, QString& error);

struct NativeLoadCaseOptions {
    int analysisBodyId = -1;
    std::vector<int> jointIds;
    int peaksPerSignal = 4;
    double mergeTolerance = 1e-9; // Seconds; merged frame uses all loads at one time.
    bool synchronized = false;
};

// Matches the original info/bodies/joints/cases JSON structure. Feature mode
// takes strict internal extrema of XYZ force/torque and maxima of magnitudes;
// synchronized mode evaluates every selected joint at each merged frame.
bool buildNativeLoadCases(const NativeMotionInput& input, const RigidResult& result,
                          const NativeLoadCaseOptions& options, QJsonObject& output, QString& error);
bool exportNativeLoadCases(const QString& path, const NativeMotionInput& input,
                           const RigidResult& result, const NativeLoadCaseOptions& options, QString& error);

} // namespace MotionOcc
