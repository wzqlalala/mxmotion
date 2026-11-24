#pragma once

#include "RigidMotion.h"

namespace MotionOcc {

// .mxmotion v3 is UTF-8 JSON with lengthUnit "mm". Each part stores standard
// Base64 STEP of its local geometry and a row-major 3×4 local-to-world rigid placement.
// Persistent geometric selections use part IDs and geometric signatures.
// Motion definitions, AutoFlex meshes, imported MNF and curve requests persist.
// Result frames stay outside the project. The reader also accepts v1 (OCCT BREP)
// and v2 (STEP, with legacy external asset references).
// A failed load changes neither output. Save atomically replaces the destination.
bool saveRigidProject(const QString& path, const CadDocument& cad, const RigidModel& model, QString& error);
bool loadRigidProject(const QString& path, CadDocument& cad, RigidModel& model, QString& error);

// UTF-8 CSV, one row per body or joint per frame, in time order, bodies then
// joints by ascending ID. Body positions are world COMs; displacements are
// relative to the first frame. Non-applicable columns (including all coordinate
// and rate columns for fixed joints) are empty. Header, in order:
// time_s,entity_type,entity_id,name,joint_kind,
// position_x_mm,position_y_mm,position_z_mm,displacement_x_mm,displacement_y_mm,displacement_z_mm,
// velocity_x_mm_s,velocity_y_mm_s,velocity_z_mm_s,speed_mm_s,
// angular_velocity_x_deg_s,angular_velocity_y_deg_s,angular_velocity_z_deg_s,angular_speed_deg_s,
// joint_coordinate_mm,joint_rate_mm_s,joint_coordinate_deg,joint_rate_deg_s
bool exportRigidCsv(const QString& path, const RigidInput& input, const RigidResult& result, QString& error);

} // namespace MotionOcc
