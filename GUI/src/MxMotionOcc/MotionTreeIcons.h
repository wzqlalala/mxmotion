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
        {"bodyDrives", "BrowserTree/motion_driveParentItem.png"},
        {"hydraulicCylinders", "Motion/mp_hyd_cylinder.svg"},
        {"hydraulicMotors", "Motion/mp_hyd_motor.svg"},
        {"hydraulicNodes", "Motion/mp_hyd_node_group.svg"},
        {"hydraulicComponents", "Motion/mp_hyd_comp_group.svg"},
        {"electricMotors", "Motion/mp_elec_motor.svg"},
        {"powerSupplies", "Motion/mp_power_supply.svg"},
        {"inverters", "Motion/mp_inverter.svg"},
        {"ehaActuators", "Motion/mp_eha_actuator.svg"},
        {"emaActuators", "Motion/mp_ema_actuator.svg"},
        {"springs", "Motion/force_spring.png"},
        {"forceElements", "BrowserTree/motion_liYuanParentItem.png"},
        {"sensors", "Motion/sensor.png"},
        {"controllers", "Motion/controlCalculation.png"},
        {"markers", "caotu/dian.png"},
        {"contacts", "Motion/contact.png"},
        {"controlInputs", "BrowserTree/motion_driveParentItem.png"},
        {"electroImports", "BrowserTree/motion.png"},
        {"electroExports", "BrowserTree/motion.png"},
        {"hydraulics", "BrowserTree/motion.png"},
        {"rlTasks", "BrowserTree/motion.png"},
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
    if (collection == "drives" || collection == "bodyDrives") return resource("Motion/drive.png");
    if (collection == "couplings") {
        static const QMap<QString, const char*> paths{
            {"Gear", "Motion/coupling_gear.png"}, {"GearRack", "Motion/coupling_gearRack.png"},
            {"Cable", "Motion/coupling_cable.png"}, {"Cam", "Motion/coupling_cam.svg"}
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
    if (collection == "hydraulicCylinders") return resource("Motion/mp_hyd_cylinder.svg");
    if (collection == "hydraulicMotors") return resource("Motion/mp_hyd_motor.svg");
    if (collection == "hydraulicNodes") {
        const QString nt = row.value("nodeType").toString("Chamber");
        if (nt == "Tank") return resource("Motion/mp_hyd_tank.svg");
        if (nt == "PressureSource") return resource("Motion/mp_hyd_pressure_source.svg");
        return resource("Motion/mp_hyd_chamber.svg");
    }
    if (collection == "hydraulicComponents") {
        static const QMap<QString, const char*> compPaths{
            {"Pump", "Motion/mp_hyd_pump.svg"},
            {"FlowSource", "Motion/mp_hyd_flow_source.svg"},
            {"DirectionalValve", "Motion/mp_valve_directional.svg"},
            {"ServoValve", "Motion/mp_valve_servo.svg"},
            {"ReliefValve", "Motion/mp_valve_relief.svg"},
            {"PressureReducingValve", "Motion/mp_valve_reducing.svg"},
            {"SequenceValve", "Motion/mp_valve_sequence.svg"},
            {"CheckValve", "Motion/mp_valve_check.svg"},
            {"ThrottleValve", "Motion/mp_valve_throttle.svg"},
            {"Pipe", "Motion/mp_hyd_pipe.svg"},
            {"Filter", "Motion/mp_hyd_filter.svg"},
            {"Accumulator", "Motion/mp_hyd_accumulator.svg"},
            {"PortConnection", "Motion/mp_port_connection.svg"}
        };
        return resource(compPaths.value(row.value("componentType").toString(), "Motion/mp_hyd_comp_group.svg"));
    }
    if (collection == "electricMotors") return resource("Motion/mp_elec_motor.svg");
    if (collection == "powerSupplies") return resource("Motion/mp_power_supply.svg");
    if (collection == "inverters") return resource("Motion/mp_inverter.svg");
    if (collection == "ehaActuators") return resource("Motion/mp_eha_actuator.svg");
    if (collection == "emaActuators") return resource("Motion/mp_ema_actuator.svg");
    return item(collection);
}

} // namespace MotionOcc::TreeIcons
