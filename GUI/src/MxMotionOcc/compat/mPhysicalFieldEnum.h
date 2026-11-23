#pragma once

// Source-compatible names for the original Motion GUI. These tags remain
// internal; persistent Motion data is translated by the original JSON codec,
// never by assuming the layout of an unavailable proprietary SDK binary.
#include "mBasicEnum.h"
#include <QMetaType>

namespace MxFunctions {
enum PhysicalFieldType {
    Physical_None = 0, Physical_Part, Physical_Set, Physical_Material,
    Physical_Case, Physical_Connector, Physical_LoadBoundary,
    Physical_Motion, Physical_MotionBody, Physical_MotionJoint,
    Physical_MotionCoupling, Physical_MotionDrive, Physical_MotionSpring,
    Physical_MotionVectorForce, Physical_MotionContact, Physical_MotionMarker,
    Physical_MotionSensor, Physical_MotionController, Physical_MotionControlInput,
    Physical_MotionScenario, Physical_MotionAutoFlex, Physical_MotionElectroImport,
    Physical_MotionElectroExport, Physical_MotionHydraulic, Physical_MotionRL
};
enum class JointType {
    Revolute, Translational, Cylindrical, Spherical, Fixed, Universal, PointPlane,
    Line, Parallel, Perpendicular, Direction, Centripetal, ConstantVelocity,
    Hooke, Screw, Planar, Coupling, Gear, PointLine, PointOnCurve, PointOnSurface,
    CurveOnCurve
};
enum class CouplingType { GearRack, Gear, Cable };
enum class DriveTargetType { Joint, Body };
inline uint qHash(PhysicalFieldType value, uint seed = 0) noexcept
{ return ::qHash(static_cast<int>(value), seed); }
}
Q_DECLARE_METATYPE(MxFunctions::PhysicalFieldType)
