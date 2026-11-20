#pragma once

#include "RigidMotion.h"

namespace MotionOcc {

// .mxmotion v1, UTF-8 JSON. All fields below are required; no other fields are defined.
// Root: { format: "mxmotion", version: 1, nextPartId, parts: [...], model: {...} }.
// Part: { id, name, brep }; brep is standard padded Base64 of an OCCT V3 text BREP
// stream (exact topology including placement, in mm, with no polygon/triangle data).
// Model collections use these record fields:
//   bodies:    { id, name, partId, fixed, massKg, densityKgM3 }
//   joints:    { id, name, kind, movingBodyId, baseBodyId, originMm, axis }
//   drives:    { id, name, jointId, kind, value }
//   scenarios: { id, name, duration, step, gravityMS2 }
// Joint kinds are "revolute", "prismatic", "fixed"; drive kinds are "speed", "displacement".
// Vectors are arrays of three numbers. Units and references follow RigidMotion.h.
// nextPartId is greater than every part ID (including previously allocated IDs).
// Neither meshes, analysis results nor external geometry paths are stored.
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
