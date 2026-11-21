#pragma once

#include <QIcon>
#include <QJsonObject>
#include <QMap>
#include <QString>

namespace MotionOcc::TreeIcons {

inline QIcon resource(const char* path)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/") + QString::fromLatin1(path));
}

// Keep the original mTreeModel/mTreeView resource names for both tree adapters.
// Collection keys are stable even when the displayed group names are translated.
inline QIcon item(const QString& type)
{
    static const QMap<QString, const char*> paths{
        {"root", "BrowserTree/motion.png"},
        {"parts", "BrowserTree/genericPart.png"},
        {"bodies", "BrowserTree/motion_movingBodyParentItem.png"},
        {"autoFlexBodies", "Motion/mnf.png"},
        {"joints", "BrowserTree/motion_connectorParentItem.png"},
        {"couplings", "BrowserTree/motion_couplingParentItem.png"},
        {"drives", "BrowserTree/motion_driveParentItem.png"},
        {"springs", "Motion/force_spring.png"},
        {"forceElements", "BrowserTree/motion_liYuanParentItem.png"},
        {"sensors", "Motion/sensor.png"},
        {"controllers", "Motion/controlCalculation.png"},
        {"markers", "caotu/dian.png"},
        {"contacts", "Motion/contact.png"},
        {"controlInputs", "BrowserTree/motion_driveParentItem.png"},
        {"scenarios", "BrowserTree/motion_solveParentItem.png"},
        {"animation", "post/animation.png"},
        {"export", "BrowserTree/motion_solveParentItem.png"},
        {"curves", "BrowserTree/motion_curveChildItem.png"}
    };
    return resource(paths.value(type, "BrowserTree/motion.png"));
}

inline QIcon body(bool fixed)
{
    return resource(fixed ? "Motion/FixedDeputy.png" : "Motion/movingBody.png");
}

inline QIcon joint(const QString& type)
{
    static const QMap<QString, const char*> paths{
        {"Revolute", "Motion/joint_revolute.png"},
        {"Translational", "Motion/joint_prismatic.png"},
        {"Cylindrical", "Motion/joint_cylindrical.png"},
        {"Spherical", "Motion/joint_spherical.png"},
        {"Fixed", "Motion/joint_fixed.png"},
        {"Universal", "Motion/joint_universal.png"},
        {"PointPlane", "Motion/joint_pointPlane.png"},
        {"Line", "Motion/joint_line.png"},
        {"Parallel", "Motion/joint_parallel.png"},
        {"Perpendicular", "Motion/joint_perpendicular.png"},
        {"Direction", "Motion/joint_direction.png"},
        {"Centripetal", "Motion/joint_centripetal.png"},
        {"ConstantVelocity", "Motion/joint_cv.png"},
        {"Hooke", "Motion/joint_hooke.png"},
        {"Screw", "Motion/joint_screw.png"},
        {"Planar", "Motion/joint_planar.png"},
        {"Coupling", "Motion/joint_coupling.png"},
        {"Gear", "Motion/joint_gear.png"},
        {"PointLine", "Motion/joint_pointLine.png"},
        {"PointOnCurve", "Motion/joint_pointOnCurve.png"},
        {"PointOnSurface", "Motion/joint_pointOnSurface.png"},
        {"CurveOnCurve", "Motion/joint_curveOnCurve.png"}
    };
    return resource(paths.value(type, "BrowserTree/motion_connectorParentItem.png"));
}

inline QIcon entity(const QString& collection, const QJsonObject& row)
{
    if (collection == "bodies") return body(row.value("isFixed").toBool());
    if (collection == "joints") return joint(row.value("jointType").toString());
    if (collection == "drives") return resource("Motion/drive.png");
    if (collection == "couplings") {
        static const QMap<QString, const char*> paths{
            {"Gear", "Motion/coupling_gear.png"}, {"GearRack", "Motion/coupling_gearRack.png"},
            {"Cable", "Motion/coupling_cable.png"}
        };
        return resource(paths.value(row.value("couplingType").toString(), "BrowserTree/motion_couplingParentItem.png"));
    }
    if (collection == "springs") {
        const auto type = row.value("springType").toString();
        return resource(type == "Translational" ? "Motion/tcSpring.png" :
            type == "Torsional" ? "Motion/torsionSpring.png" : "Motion/force_spring.png");
    }
    if (collection == "sensors") {
        static const QMap<QString, const char*> paths{
            {"BodyDisplacement", "Motion/sensor_displacement.png"},
            {"BodyVelocity", "Motion/sensor_velocity.png"},
            {"BodyAcceleration", "Motion/sensor_acceleration.png"},
            {"JointReaction", "Motion/sensor_jointReaction.png"},
            {"DriveActuation", "Motion/sensor_driveActuation.png"},
            {"Distance", "Motion/sensor_distance.png"},
            {"Angle", "Motion/sensor_angle.png"}
        };
        return resource(paths.value(row.value("sensorType").toString(), "Motion/sensor.png"));
    }
    if (collection == "vectorForces") {
        static const QMap<QString, const char*> paths{
            {"Force", "Motion/force_vector.png"}, {"Torque", "Motion/force_torque.png"},
            {"UniForce", "Motion/force_vector.png"}, {"UniTorque", "Motion/force_torque.png"},
            {"TriForce", "Motion/force_tri.png"}, {"TriTorque", "Motion/force_tri_torque.png"},
            {"GeneralForce", "Motion/force_general.png"}, {"Gravity", "Motion/force_gravity.png"},
            {"ForceField", "Motion/force_field.png"}, {"LinearSpringDamper", "Motion/force_spring.png"},
            {"TireForce", "Motion/force_tire.png"}, {"Bushing", "Motion/bush.png"},
            {"ScalarForce", "Motion/force.png"}, {"ScalarTorque", "Motion/moment.png"}
        };
        return resource(paths.value(row.value("loadType").toString(), "BrowserTree/motion_liYuanParentItem.png"));
    }
    if (collection == "scalarForces") return resource("Motion/force.png");
    if (collection == "scalarTorques") return resource("Motion/moment.png");
    if (collection == "threePointForces") return resource("Motion/force_tri.png");
    if (collection == "threePointTorques") return resource("Motion/force_tri_torque.png");
    // New engine force elements reuse the existing cable and unilateral-force artwork.
    if (collection == "tensionCables") return resource("Motion/coupling_cable.png");
    if (collection == "softStops") return resource("Motion/force_uni.png");
    return item(collection);
}

} // namespace MotionOcc::TreeIcons
