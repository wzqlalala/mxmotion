#include "NativeMotionDocument.h"
#include "CadDocument.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Mat.hxx>
#include <gp_Pnt.hxx>
#include <math_Jacobi.hxx>
#include <math_Matrix.hxx>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace MotionOcc {
namespace {

constexpr qint64 MaxDocumentBytes = 64 * 1024 * 1024;
constexpr int MaxEntities = 100000;
constexpr int MaxReportedIssues = 100;

QStringList collectionNames()
{
    return {QStringLiteral("bodies"), QStringLiteral("joints"), QStringLiteral("drives"),
        QStringLiteral("markers"), QStringLiteral("springs"), QStringLiteral("couplings"),
        QStringLiteral("vectorForces"), QStringLiteral("scalarForces"), QStringLiteral("scalarTorques"),
        QStringLiteral("threePointForces"), QStringLiteral("threePointTorques"), QStringLiteral("contacts"),
        QStringLiteral("sensors"), QStringLiteral("controllers"), QStringLiteral("controlInputs"),
        QStringLiteral("autoFlexBodies"), QStringLiteral("scenarios"), QStringLiteral("tensionCables"),
        QStringLiteral("softStops")};
}

bool integer(const QJsonValue& value, int minimum, int* result = nullptr)
{
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < minimum || number > std::numeric_limits<int>::max()
        || std::floor(number) != number) return false;
    if (result) *result = int(number);
    return true;
}

class Validator {
public:
    Validator(const NativeMotionDocument& document, QStringList& issues) : doc(document), errors(issues) {}

    void issue(const QString& where, const QString& why)
    {
        if (errors.size() < MaxReportedIssues) errors.append(where + QStringLiteral("：") + why);
    }

    void number(const QJsonObject& object, const char* key, const QString& where,
                double minimum = -std::numeric_limits<double>::infinity(), bool required = false)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name) && !required) return;
        const auto value = object.value(name);
        if (!value.isDouble() || !std::isfinite(value.toDouble()) || value.toDouble() < minimum)
            issue(where + QLatin1Char('.') + name, QStringLiteral("需要有效数值%1").arg(std::isfinite(minimum)
                ? QStringLiteral("，且不得小于 %1").arg(minimum) : QString()));
    }

    void positive(const QJsonObject& object, const char* key, const QString& where, bool required = false)
    {
        number(object, key, where, 0.0, required);
        if (object.value(QLatin1String(key)).isDouble() && object.value(QLatin1String(key)).toDouble() <= 0)
            issue(where + QLatin1Char('.') + QLatin1String(key), QStringLiteral("必须大于零"));
    }

    void enumeration(const QJsonObject& object, const char* key, const QStringList& allowed,
                     const QString& where, bool required = false)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name) && !required) return;
        if (!object.value(name).isString() || !allowed.contains(object.value(name).toString()))
            issue(where + QLatin1Char('.') + name, QStringLiteral("未知类型 %1").arg(object.value(name).toVariant().toString()));
    }

    void vector(const QJsonObject& object, const char* key, const QString& where, bool required = false)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name) && !required) return;
        if (!object.value(name).isObject()) { issue(where + QLatin1Char('.') + name, QStringLiteral("需要 {x,y,z} 对象")); return; }
        for (const auto* axis : {"x", "y", "z"}) number(object.value(name).toObject(), axis, where + QLatin1Char('.') + name,
            -std::numeric_limits<double>::infinity(), true);
    }

    void nonzeroVector(const QJsonObject& object, const char* key, const QString& where, bool required = false)
    {
        vector(object, key, where, required);
        if (!object.value(QLatin1String(key)).isObject()) return;
        const auto value = object.value(QLatin1String(key)).toObject();
        const double x = value.value("x").toDouble(), y = value.value("y").toDouble(), z = value.value("z").toDouble();
        if (std::max({std::abs(x), std::abs(y), std::abs(z)}) <= 1e-15)
            issue(where + QLatin1Char('.') + QLatin1String(key), QStringLiteral("方向向量不能为零"));
    }

    void quaternion(const QJsonObject& object, const char* key, const QString& where)
    {
        if (!object.contains(QLatin1String(key))) return;
        vector(object, key, where);
        const auto value = object.value(QLatin1String(key)).toObject();
        number(value, "w", where + QLatin1Char('.') + QLatin1String(key), -std::numeric_limits<double>::infinity(), true);
        double maximum = 0;
        for (const auto* component : {"w", "x", "y", "z"}) maximum = std::max(maximum, std::abs(value.value(QLatin1String(component)).toDouble()));
        if (maximum <= 1e-15) issue(where + QLatin1Char('.') + QLatin1String(key), QStringLiteral("四元数不能全部为零"));
    }

    void ref(const QJsonObject& object, const char* key, const char* target, const QString& where,
             bool allowGround = false, bool required = false)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name) && !required) return;
        int id = -2;
        if (!integer(object.value(name), allowGround ? -1 : 0, &id)) {
            issue(where + QLatin1Char('.') + name, QStringLiteral("需要有效的引用 ID")); return;
        }
        if (allowGround && id == -1) return;
        if (!ids.value(QLatin1String(target)).contains(id))
            issue(where + QLatin1Char('.') + name, QStringLiteral("引用的 %1 ID=%2 不存在").arg(QLatin1String(target)).arg(id));
    }

    void idArray(const QJsonObject& object, const char* key, const QString& where, const char* target = nullptr)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name)) return;
        if (!object.value(name).isArray()) { issue(where + QLatin1Char('.') + name, QStringLiteral("需要 ID 数组")); return; }
        QSet<int> seen;
        for (const auto& value : object.value(name).toArray()) {
            int id;
            if (!integer(value, 0, &id)) { issue(where + QLatin1Char('.') + name, QStringLiteral("含无效 ID")); continue; }
            if (seen.contains(id)) issue(where + QLatin1Char('.') + name, QStringLiteral("重复 ID=%1").arg(id));
            seen.insert(id);
            if (target && !ids.value(QLatin1String(target)).contains(id))
                issue(where + QLatin1Char('.') + name, QStringLiteral("引用的 %1 ID=%2 不存在").arg(QLatin1String(target)).arg(id));
        }
    }

    void numericArray(const QJsonObject& object, const char* key, const QString& where, int count = -1)
    {
        const QString name = QString::fromLatin1(key);
        if (!object.contains(name)) return;
        if (!object.value(name).isArray()) { issue(where + QLatin1Char('.') + name, QStringLiteral("需要数值数组")); return; }
        const auto values = object.value(name).toArray();
        if (count >= 0 && !values.isEmpty() && values.size() != count)
            issue(where + QLatin1Char('.') + name, QStringLiteral("需要 %1 个分量").arg(count));
        for (const auto& value : values) if (!value.isDouble() || !std::isfinite(value.toDouble())) {
            issue(where + QLatin1Char('.') + name, QStringLiteral("含非数值或非有限值")); break;
        }
    }

    void table(const QJsonObject& object, const char* xKey, const char* yKey, const QString& where, bool restoring = false)
    {
        numericArray(object, xKey, where); numericArray(object, yKey, where);
        const auto x = object.value(QLatin1String(xKey)).toArray(), y = object.value(QLatin1String(yKey)).toArray();
        if (x.isEmpty() && y.isEmpty()) return;
        if (x.size() != y.size() || x.size() < 2) {
            issue(where, QStringLiteral("表格 X/Y 长度必须相等且至少有两个点")); return;
        }
        for (int i = 0; i < x.size(); ++i) {
            if (i && x[i].toDouble() <= x[i - 1].toDouble()) issue(where, QStringLiteral("表格 X 必须严格递增"));
            if (restoring && (x[i].toDouble() < 0 || y[i].toDouble() < 0)) issue(where, QStringLiteral("软限位压缩量和恢复力均不能为负"));
        }
        if (restoring && (x.first().toDouble() != 0 || y.first().toDouble() != 0))
            issue(where, QStringLiteral("软限位表格必须从 (0,0) 开始"));
    }

    void basic(const QJsonObject& object, const QString& where)
    {
        const QStringList booleans = {"isFixed", "isCustomMassProps", "isUserRename", "enabled", "visible", "actionOnly",
            "allowSliding", "dampingEnabled", "reverse", "applyOffset", "useConvexHullAction", "useConvexHullBase",
            "frictionEnabled", "actionUseFacetFile", "baseUseFacetFile", "actionUseMaxFacetSize", "baseUseMaxFacetSize",
            "usePreset", "solverVerbose", "hasTable", "outputModeShapes", "outputStressStrain", "rigidBodyModeFilter"};
        for (const auto& key : booleans)
            if (object.contains(key) && !object.value(key).isBool()) issue(where + QLatin1Char('.') + key, QStringLiteral("需要布尔值"));
        const QStringList strings = {"name", "expression", "forceFunction", "torqueFunction", "dampingFunction"};
        for (const auto& key : strings)
            if (object.contains(key) && !object.value(key).isString()) issue(where + QLatin1Char('.') + key, QStringLiteral("需要文本"));
    }

    void bodyPair(const QJsonObject& object, const QString& where, bool required = false)
    {
        ref(object, "bodyIId", "bodies", where, true, required);
        ref(object, "bodyJId", "bodies", where, true, required);
        vector(object, "localI", where, required); vector(object, "localJ", where, required);
        if (object.contains("bodyIId") && object.contains("bodyJId") && object.value("bodyIId") == object.value("bodyJId"))
            issue(where, QStringLiteral("两个端点必须属于不同运动体或大地"));
    }

    void topology(const QJsonObject& object, const char* key, const QString& where)
    {
        if (!object.contains(QLatin1String(key))) return;
        const auto value = object.value(QLatin1String(key));
        if (!value.isArray()) { issue(where + QLatin1Char('.') + QLatin1String(key), QStringLiteral("需要 {partId,index} 数组")); return; }
        for (const auto& entry : value.toArray()) {
            const auto binding = entry.toObject();
            if (!entry.isObject() || !integer(binding.value("partId"), 1) || !integer(binding.value("index"), 1))
                issue(where + QLatin1Char('.') + QLatin1String(key), QStringLiteral("OCC 部件 ID 和拓扑序号必须为正整数"));
        }
    }

    void curve(const QJsonObject& object, const QString& where)
    {
        QMap<QString, QString> sources{{"Body", "bodies"}, {"Joint", "joints"}, {"Marker", "markers"},
            {"Drive", "drives"}, {"Coupling", "couplings"}, {"Sensor", "sensors"}, {"Controller", "controllers"},
            {"Spring", "springs"}, {"TensionCable", "tensionCables"}, {"SoftStop", "softStops"},
            {"VectorForce", "vectorForces"}, {"ScalarForce", "scalarForces"}, {"ScalarTorque", "scalarTorques"},
            {"ThreePointForce", "threePointForces"}, {"ThreePointTorque", "threePointTorques"}, {"AutoFlexBody", "autoFlexBodies"}};
        // Original files use PascalCase enum aliases. New native curves use
        // the exact collection keys so every recorded force/flexible channel
        // survives .motion.json export without narrowing to the legacy enum.
        for (const auto& collection : sources.values()) sources.insert(collection, collection);
        enumeration(object, "sourceType", sources.keys(), where, true);
        const auto target = sources.value(object.value("sourceType").toString()).toLatin1();
        if (!target.isEmpty()) ref(object, "sourceId", target.constData(), where, false, true);
        if (object.value("component").isDouble()) {
            int component = -2;
            if (!integer(object.value("component"), -1, &component) || component > 1023)
                issue(where + QStringLiteral(".component"), QStringLiteral("分量必须是 -1（模长）或 0…1023 的整数索引"));
        } else enumeration(object, "component", {"X", "Y", "Z", "Magnitude"}, where);
        if (object.contains("visible") && !object.value("visible").isBool()) issue(where, QStringLiteral("visible 需要布尔值"));
    }

    void record(const QString& kind, const QJsonObject& object)
    {
        const QString where = QStringLiteral("%1[%2] %3").arg(kind).arg(object.value("id").toInt(-1)).arg(object.value("name").toString());
        basic(object, where);
        if (kind == "bodies") {
            idArray(object, "geoEntityIds", where);
            if (object.contains("geoEntityNames")) {
                const auto names = object.value("geoEntityNames").toArray();
                if (!object.value("geoEntityNames").isArray() || names.size() != object.value("geoEntityIds").toArray().size())
                    issue(where, QStringLiteral("geoEntityNames 必须与 geoEntityIds 逐项对应"));
                for (const auto& name : names) if (!name.isString()) issue(where, QStringLiteral("几何名称必须为文本"));
            }
            number(object, "mass", where, 0); numericArray(object, "inertia", where, 6);
            enumeration(object, "inertiaConvention", {"auto", "tensor", "products"}, where);
            vector(object, "centerOfMass", where); vector(object, "initialVelocity", where); vector(object, "initialAngularVelocity", where);
        } else if (kind == "joints") {
            enumeration(object, "jointType", {"Revolute", "Translational", "Cylindrical", "Spherical", "Fixed", "Universal",
                "PointPlane", "Line", "Parallel", "Perpendicular", "Direction", "Centripetal", "ConstantVelocity", "Hooke",
                "Screw", "Planar", "Coupling", "Gear", "PointLine", "PointOnCurve", "PointOnSurface", "CurveOnCurve"}, where, true);
            ref(object, "actionBodyId", "bodies", where, true); ref(object, "baseBodyId", "bodies", where, true);
            vector(object, "origin", where); nonzeroVector(object, "axis", where);
            for (const auto* key : {"actionSpin", "actionPin", "baseSpin", "basePin", "actionVector", "baseVector"}) nonzeroVector(object, key, where);
            number(object, "lead", where);
            for (const auto* key : {"surfaceFaceIds", "curveEdgeIds", "curveIEdgeIds", "curveJEdgeIds"}) idArray(object, key, where);
            for (const auto* key : {"occSurfaceFaces", "occCurveEdges", "occCurveIEdges", "occCurveJEdges"}) topology(object, key, where);
            for (const auto* key : {"friction", "limit"}) {
                if (!object.contains(QLatin1String(key))) continue;
                if (!object.value(QLatin1String(key)).isObject()) { issue(where, QStringLiteral("%1 必须为对象").arg(QLatin1String(key))); continue; }
                const auto nested = object.value(QLatin1String(key)).toObject();
                basic(nested, where + QLatin1Char('.') + QLatin1String(key));
                if (QLatin1String(key) == "friction") {
                    for (const auto* field : {"staticCoefficient", "kineticCoefficient", "viscousCoefficient", "effectiveRadius"}) number(nested, field, where, 0);
                } else {
                    for (const auto* field : {"lowerValue", "upperValue"}) number(nested, field, where);
                    for (const auto* field : {"lowerCushion", "upperCushion", "lowerStiffness", "upperStiffness", "lowerDamping", "upperDamping"}) number(nested, field, where, 0);
                    if (nested.value("enabled").toBool() && nested.value("lowerEnabled").toBool() && nested.value("upperEnabled").toBool()
                        && nested.value("lowerValue").toDouble() > nested.value("upperValue").toDouble()) issue(where, QStringLiteral("限位下限大于上限"));
                }
            }
        } else if (kind == "drives") {
            enumeration(object, "driveType", {"JointDrive"}, where);
            enumeration(object, "targetType", {"Joint", "Body"}, where);
            enumeration(object, "motionType", {"Rotation", "Translation"}, where);
            enumeration(object, "controlMode", {"Position", "Velocity", "Acceleration"}, where);
            ref(object, "targetJointId", "joints", where, false, object.value("targetType").toString("Joint") == "Joint");
            ref(object, "targetBodyId", "bodies", where);
            table(object, "driveTableX", "driveTableY", where);
        } else if (kind == "markers") {
            ref(object, "bodyId", "bodies", where, true);
            vector(object, "localPosition", where); quaternion(object, "localRotation", where); number(object, "size", where, 0);
        } else if (kind == "springs") {
            enumeration(object, "springType", {"Translational", "Torsional", "BodyPoint"}, where, true);
            if (object.value("springType").toString() == "BodyPoint") bodyPair(object, where, true);
            else ref(object, "jointId", "joints", where, false, true);
            quaternion(object, "localRotI", where); quaternion(object, "localRotJ", where);
            for (const auto* key : {"stiffness", "damping", "freeValue", "installValue"}) number(object, key, where, 0);
            number(object, "preload", where);
            for (const auto* key : {"springDirectionMode", "dampingDirectionMode"})
                enumeration(object, key, {"Bidirectional", "PositiveOnly", "NegativeOnly"}, where);
            table(object, "tableX", "tableY", where);
        } else if (kind == "couplings") {
            enumeration(object, "couplingType", {"Gear", "GearRack", "Cable"}, where, true);
            ref(object, "firstJointId", "joints", where, false, true); ref(object, "secondJointId", "joints", where, false, true);
            for (const auto* key : {"gearRatio", "ratio"}) number(object, key, where);
            for (const auto* key : {"radius", "pitchRadius1", "pitchRadius2", "toothCount1", "toothCount2"}) number(object, key, where, 0);
        } else if (kind == "vectorForces") {
            enumeration(object, "loadType", {"Force", "Torque", "UniForce", "UniTorque", "TriForce", "TriTorque", "GeneralForce",
                "Gravity", "ForceField", "LinearSpringDamper", "TireForce", "Bushing"}, where);
            ref(object, "bodyId", "bodies", where, true); ref(object, "baseBodyId", "bodies", where, true);
            vector(object, "actionPoint", where); vector(object, "direction", where); vector(object, "offset", where);
            number(object, "magnitude", where);
            enumeration(object, "magnitudeMode", {"Constant", "Expression"}, where);
            enumeration(object, "directionMode", {"GlobalFixed", "BodyFixed"}, where);
            for (const auto* key : {"stiffness", "damping"}) numericArray(object, key, where, 6);
            for (const auto* key : {"preloadForce", "preloadTorque"}) numericArray(object, key, where, 3);
        } else if (kind == "scalarForces" || kind == "scalarTorques") {
            bodyPair(object, where);
            vector(object, "fixedDirection", where); vector(object, "fixedAxis", where); number(object, "magnitude", where);
            for (const auto* key : {"directionMode", "axisMode"})
                if (object.contains(QLatin1String(key)) && (!integer(object.value(QLatin1String(key)), 0) || object.value(QLatin1String(key)).toInt() > 3))
                    issue(where, QStringLiteral("%1 无效").arg(QLatin1String(key)));
        } else if (kind == "threePointForces" || kind == "threePointTorques") {
            for (const auto* key : {"bodyIId", "targetBodyId", "originBodyId"}) ref(object, key, "bodies", where, true);
            for (const auto* key : {"localI", "localTarget", "localOrigin"}) vector(object, key, where);
            number(object, "magnitude", where);
        } else if (kind == "contacts") {
            enumeration(object, "contactType", {"CADContact", "FacetContact"}, where);
            ref(object, "actionBodyId", "bodies", where, false, true); ref(object, "baseBodyId", "bodies", where, false, true);
            for (const auto* key : {"stiffness", "forceExponent", "materialDamping", "penetrationDepth", "staticFriction", "dynamicFriction",
                "forceModelTolerance", "actionDistanceTolerance", "angleTolerance", "actionMaxFacetSize", "baseDistanceTolerance", "baseMaxFacetSize"}) number(object, key, where, 0);
        } else if (kind == "sensors") {
            enumeration(object, "sensorType", {"BodyDisplacement", "BodyVelocity", "BodyAcceleration", "JointReaction", "DriveActuation", "Distance", "Angle"}, where, true);
            enumeration(object, "coordSystem", {"GlobalFixed", "BodyFixed"}, where);
            enumeration(object, "componentMode", {"X", "Y", "Z", "Magnitude", "All"}, where);
            ref(object, "referenceBodyId", "bodies", where, true);
            const auto type = object.value("sensorType").toString();
            if (type.startsWith("Body")) {
                ref(object, "targetBodyId", "bodies", where, false, true); vector(object, "referencePoint", where);
                enumeration(object, "variable", {"Position", "Velocity", "Acceleration", "AngularVelocity", "AngularAcceleration", "Rotation"}, where);
            } else if (type == "JointReaction") {
                ref(object, "targetJointId", "joints", where, false, true);
                enumeration(object, "variable", {"ReactionForce", "ReactionTorque"}, where);
            } else if (type == "DriveActuation") {
                ref(object, "targetDriveId", "drives", where, false, true);
                enumeration(object, "variable", {"ActuationForce", "ActuationTorque", "ImposedValue", "ImposedSpeed", "ImposedAcc", "ReactionForce", "ReactionTorque"}, where);
            } else if (type == "Distance") {
                for (const auto* key : {"pointA", "pointB"}) {
                    if (!object.value(QLatin1String(key)).isObject()) { issue(where, QStringLiteral("距离传感器缺少端点 %1").arg(QLatin1String(key))); continue; }
                    const auto point = object.value(QLatin1String(key)).toObject();
                    ref(point, "bodyId", "bodies", where, true); vector(point, "localPos", where);
                }
                enumeration(object, "variable", {"Distance", "RelVelocity", "RelAcceleration"}, where);
            } else if (type == "Angle") {
                for (const auto* key : {"axisBodyId", "bodyAId", "bodyBId"}) ref(object, key, "bodies", where, true);
                nonzeroVector(object, "axisDirection", where);
            }
        } else if (kind == "controllers") {
            enumeration(object, "controllerType", {"PID"}, where, true);
            ref(object, "targetDriveId", "drives", where, false, true);
            ref(object, "targetInputId", "controlInputs", where, true);
            ref(object, "feedbackSensorId", "sensors", where, true);
            enumeration(object, "feedbackType", {"Position", "Velocity"}, where);
            enumeration(object, "feedbackSource", {"DriveDof", "Sensor"}, where);
            for (const auto* key : {"targetValue", "feedbackSign", "kp", "ki", "kd", "outputLimit"}) number(object, key, where);
        } else if (kind == "controlInputs") {
            enumeration(object, "inputType", {"Constant", "Linear", "Quadratic", "Cubic", "CustomCurve"}, where, true);
            number(object, "timeStart", where); number(object, "timeEnd", where); numericArray(object, "polyCoeffs", where);
            if (object.contains("curvePoints")) {
                if (!object.value("curvePoints").isArray()) issue(where, QStringLiteral("curvePoints 需要数值对数组"));
                double previous = -std::numeric_limits<double>::infinity();
                for (const auto& value : object.value("curvePoints").toArray()) {
                    const auto pair = value.toArray();
                    if (!value.isArray() || pair.size() != 2 || !pair[0].isDouble() || !pair[1].isDouble()
                        || !std::isfinite(pair[0].toDouble()) || !std::isfinite(pair[1].toDouble()) || pair[0].toDouble() <= previous)
                        issue(where, QStringLiteral("自定义输入曲线必须为有限数值对，时间严格递增"));
                    if (pair.size() == 2) previous = pair[0].toDouble();
                }
            }
        } else if (kind == "autoFlexBodies") {
            ref(object, "targetBodyId", "bodies", where, false, true);
            idArray(object, "attachPoints", where);
            // The original exporter contains only a materialId, not a material
            // database or tetra mesh. Keep these references/extension data intact;
            // preparing the flexible solve must resolve the actual dependencies.
        } else if (kind == "scenarios") {
            positive(object, "endTime", where); positive(object, "stepSize", where); vector(object, "gravity", where);
            enumeration(object, "presetMode", {"FAST", "CONTACT", "ROBUST", "HIGH_PRECISION"}, where);
            enumeration(object, "integratorType", {"EULER_IMPLICIT_LINEARIZED", "EULER_IMPLICIT_PROJECTED", "EULER_IMPLICIT",
                "TRAPEZOIDAL", "TRAPEZOIDAL_LINEARIZED", "HHT", "HEUN", "RUNGEKUTTA45", "EULER_EXPLICIT", "LEAPFROG", "NEWMARK", "CUSTOM_INTEGRATOR"}, where);
            enumeration(object, "solverType", {"PSOR", "PSSOR", "PJACOBI", "PMINRES", "BARZILAIBORWEIN", "APGD", "ADDM",
                "SPARSE_LU", "SPARSE_QR", "PARDISO_MKL", "PARDISO_PROJECT", "MUMPS", "GMRES", "MINRES", "BICGSTAB", "CUSTOM_SOLVER"}, where);
            for (const auto* key : {"integratorAlpha", "integratorBeta"}) number(object, key, where);
            for (const auto* key : {"integratorMaxIters", "integratorTol", "solverMaxIters", "solverTolerance"}) positive(object, key, where);
            idArray(object, "trackedMarkerIds", where, "markers");
            if (object.contains("curveRequests") && !object.value("curveRequests").isArray()) issue(where, QStringLiteral("curveRequests 需要数组"));
            for (const auto& request : object.value("curveRequests").toArray()) {
                if (!request.isObject()) issue(where, QStringLiteral("曲线请求必须为对象"));
                else curve(request.toObject(), where + QStringLiteral(".curveRequests"));
            }
        } else if (kind == "tensionCables" || kind == "softStops") {
            if (object.contains("markerIId") || object.contains("markerJId")) {
                ref(object, "markerIId", "markers", where, false, true); ref(object, "markerJId", "markers", where, false, true);
            } else bodyPair(object, where, true);
            number(object, "damping", where, 0);
            if (kind == "tensionCables") {
                positive(object, "freeLength", where, true); positive(object, "stiffness", where, true);
                number(object, "engagementWidth", where, 0);
            } else {
                nonzeroVector(object, "direction", where, true);
                if (!integer(object.value("directionMode"), 1) || object.value("directionMode").toInt() > 3)
                    issue(where, QStringLiteral("directionMode 必须为 1、2 或 3"));
                if (!integer(object.value("side"), -1) || std::abs(object.value("side").toInt()) != 1)
                    issue(where, QStringLiteral("side 必须为 -1 或 1"));
                number(object, "stopPosition", where, -std::numeric_limits<double>::infinity(), true);
                number(object, "dampingDepth", where, 0);
                const bool usesTable = object.value("hasTable").toBool(!object.value("tableX").toArray().isEmpty());
                if (usesTable) {
                    table(object, "tableX", "tableY", where, true);
                    if (object.value("tableX").toArray().size() < 2) issue(where, QStringLiteral("已选择表格软限位，但没有至少两个表格点"));
                } else { positive(object, "stiffness", where, true); number(object, "exponent", where, 1, true); }
            }
        }
    }

    void run()
    {
        errors.clear();
        if (!doc.root.value("motionData").isObject()) { issue("motionData", QStringLiteral("缺少原始 Motion 设置对象")); return; }
        if (doc.root.contains("formatVersion") && (!integer(doc.root.value("formatVersion"), 1) || doc.root.value("formatVersion").toInt() != 1))
            issue("formatVersion", QStringLiteral("仅支持原始格式版本 1"));
        const auto data = doc.root.value("motionData").toObject();
        int total = 0;
        QSet<int> forceIds;
        const QStringList forceCollections = {"vectorForces", "scalarForces", "scalarTorques", "threePointForces", "threePointTorques"};
        for (const auto& name : collectionNames()) {
            if (!data.contains(name)) continue;
            if (!data.value(name).isArray()) { issue(name, QStringLiteral("需要对象数组")); continue; }
            const auto items = data.value(name).toArray();
            total += items.size();
            if (total > MaxEntities) { issue(name, QStringLiteral("实体数量超过 100000")); return; }
            for (const auto& item : items) {
                int id;
                if (!item.isObject() || !integer(item.toObject().value("id"), 0, &id)) {
                    issue(name, QStringLiteral("每个对象必须有非负整数 id")); continue;
                }
                if (ids[name].contains(id)) issue(name, QStringLiteral("重复 ID=%1").arg(id));
                ids[name].insert(id);
                if (forceCollections.contains(name)) {
                    if (forceIds.contains(id)) issue(name, QStringLiteral("力元素共用 ID 空间，ID=%1 重复").arg(id));
                    forceIds.insert(id);
                }
            }
        }
        for (const auto& name : collectionNames())
            for (const auto& item : doc.collection(name)) if (item.isObject()) record(name, item.toObject());
    }

    const NativeMotionDocument& doc;
    QStringList& errors;
    QMap<QString, QSet<int>> ids;
};

struct ShapeSignature {
    double volume = 0;
    std::array<double, 3> center{};
    std::array<double, 6> inertia{}; // Unit-density centroid tensor: xx, yy, zz, xy, xz, yz.
};

std::array<double, 3> principalMoments(const std::array<double, 6>& tensor)
{
    math_Matrix matrix(1, 3, 1, 3);
    for (int i = 0; i < 3; ++i) matrix(i + 1, i + 1) = tensor[i];
    matrix(1, 2) = matrix(2, 1) = tensor[3];
    matrix(1, 3) = matrix(3, 1) = tensor[4];
    matrix(2, 3) = matrix(3, 2) = tensor[5];
    const math_Jacobi eigenvalues(matrix);
    if (!eigenvalues.IsDone()) return {};
    std::array<double, 3> result{eigenvalues.Value(1), eigenvalues.Value(2), eigenvalues.Value(3)};
    std::sort(result.begin(), result.end());
    return result;
}

ShapeSignature signature(const CadPart& part)
{
    ShapeSignature out;
    try {
        GProp_GProps properties;
        BRepGProp::VolumeProperties(part.shape, properties);
        out.volume = properties.Mass();
        if (!std::isfinite(out.volume) || out.volume <= 1e-12) return {};
        const auto center = properties.CentreOfMass();
        const auto inertia = properties.MatrixOfInertia();
        out.center = {center.X(), center.Y(), center.Z()};
        out.inertia = {inertia.Value(1, 1), inertia.Value(2, 2), inertia.Value(3, 3),
            inertia.Value(1, 2), inertia.Value(1, 3), inertia.Value(2, 3)};
        for (double x : out.center) if (!std::isfinite(x)) return {};
        for (double x : out.inertia) if (!std::isfinite(x)) return {};
    } catch (const Standard_Failure&) { return {}; }
    return out;
}

ShapeSignature combine(const QVector<int>& partIds, const std::map<int, ShapeSignature>& signatures)
{
    ShapeSignature out;
    for (int id : partIds) {
        const auto found = signatures.find(id);
        if (found == signatures.end() || found->second.volume <= 0) return {};
        const auto& item = found->second;
        out.volume += item.volume;
        for (int axis = 0; axis < 3; ++axis) out.center[axis] += item.volume * item.center[axis];
    }
    if (out.volume <= 0) return {};
    for (double& coordinate : out.center) coordinate /= out.volume;
    for (int id : partIds) {
        const auto& item = signatures.at(id);
        for (int axis = 0; axis < 3; ++axis) {
            const int j = (axis + 1) % 3, k = (axis + 2) % 3;
            const double dj = item.center[j] - out.center[j], dk = item.center[k] - out.center[k];
            out.inertia[axis] += item.inertia[axis] + item.volume * (dj * dj + dk * dk);
        }
        const double dx = item.center[0] - out.center[0], dy = item.center[1] - out.center[1], dz = item.center[2] - out.center[2];
        out.inertia[3] += item.inertia[3] - item.volume * dx * dy;
        out.inertia[4] += item.inertia[4] - item.volume * dx * dz;
        out.inertia[5] += item.inertia[5] - item.volume * dy * dz;
    }
    return out;
}

bool matchesBody(const ShapeSignature& shape, const QJsonObject& body, bool verifyInertia = true)
{
    if (shape.volume <= 0 || !body.value("centerOfMass").isObject()) return false;
    const auto center = body.value("centerOfMass").toObject();
    double extent = 1;
    for (int axis = 0; axis < 3; ++axis) extent = std::max(extent, std::sqrt(std::max(0.0, shape.inertia[axis] / shape.volume)));
    const double tolerance = std::max(1e-3, extent * 2e-5);
    for (int axis = 0; axis < 3; ++axis)
        if (std::abs(shape.center[axis] - center.value(QString(QChar('x' + axis))).toDouble()) > tolerance) return false;
    const double mass = body.value("mass").toDouble();
    const auto inertia = body.value("inertia").toArray();
    if (verifyInertia && mass > 0 && inertia.size() == 6 && inertia[0].toDouble() > 0 && inertia[1].toDouble() > 0 && inertia[2].toDouble() > 0) {
        // Historical native files contain both principal moments and global
        // diagonal moments with omitted products. Those representations differ
        // for a rotated/asymmetric body. Accept either verified fingerprint
        // when all products are zero; never alter the source dynamics here.
        std::array<double, 6> sourceTensor;
        for (int i = 0; i < 6; ++i) sourceTensor[i] = inertia[i].toDouble();
        const auto sameMoment = [](double actual, double expected) {
            return actual > 0 && expected > 0
                && std::abs(actual - expected) <= std::max(1e-3, std::max(actual, expected) * 2e-3);
        };
        if (sourceTensor[3] == 0 && sourceTensor[4] == 0 && sourceTensor[5] == 0) {
            bool globalDiagonalMatches = true;
            for (int axis = 0; axis < 3; ++axis)
                globalDiagonalMatches &= sameMoment(shape.inertia[axis] / shape.volume, sourceTensor[axis] / mass);
            if (globalDiagonalMatches) return true;
        }
        const auto sourcePrincipal = principalMoments(sourceTensor), shapePrincipal = principalMoments(shape.inertia);
        for (int axis = 0; axis < 3; ++axis) {
            const double expected = sourcePrincipal[axis] / mass;
            const double actual = shapePrincipal[axis] / shape.volume;
            if (!sameMoment(actual, expected)) return false;
        }
    }
    return true;
}

QString bodyLabel(const QJsonObject& body)
{
    return QStringLiteral("运动体 %1（%2）").arg(body.value("id").toInt()).arg(body.value("name").toString());
}

} // namespace

QJsonArray NativeMotionDocument::collection(const QString& name) const
{
    return root.value(QStringLiteral("motionData")).toObject().value(name).toArray();
}

QJsonObject NativeMotionDocument::entity(const QString& collectionName, int id) const
{
    for (const auto& value : collection(collectionName)) {
        const auto object = value.toObject();
        if (object.value(QStringLiteral("id")).toInt(-1) == id) return object;
    }
    return {};
}

bool validateNativeMotionDocument(const NativeMotionDocument& document, QStringList& issues)
{
    Validator(document, issues).run();
    return issues.isEmpty();
}

bool validateNativeMotionDocument(const NativeMotionDocument& document, QString& error)
{
    QStringList issues;
    const bool valid = validateNativeMotionDocument(document, issues);
    error = issues.join(QLatin1Char('\n'));
    return valid;
}

bool parseNativeMotionDocument(const QByteArray& bytes, NativeMotionDocument& out, QString& error, const QString& baseDirectory)
{
    error.clear();
    if (bytes.size() > MaxDocumentBytes) { error = QStringLiteral("Motion JSON 超过 64 MiB。 "); return false; }
    QJsonParseError parseError;
    const auto parsed = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
        error = QStringLiteral("Motion JSON 解析失败（字节 %1）：%2").arg(parseError.offset).arg(parseError.errorString()); return false;
    }
    NativeMotionDocument document;
    document.root = parsed.object();
    document.baseDirectory = baseDirectory;
    if (!validateNativeMotionDocument(document, error)) return false;
    out = std::move(document);
    return true;
}

bool readNativeMotionDocument(const QString& path, NativeMotionDocument& out, QString& error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { error = QStringLiteral("无法读取 Motion JSON：%1").arg(file.errorString()); return false; }
    if (file.size() > MaxDocumentBytes) { error = QStringLiteral("Motion JSON 超过 64 MiB。"); return false; }
    return parseNativeMotionDocument(file.readAll(), out, error, QFileInfo(path).absolutePath());
}

bool writeNativeMotionDocument(const QString& path, const NativeMotionDocument& document, QString& error)
{
    if (!validateNativeMotionDocument(document, error)) return false;
    const auto bytes = QJsonDocument(document.root).toJson(QJsonDocument::Indented);
    if (bytes.size() > MaxDocumentBytes) { error = QStringLiteral("Motion JSON 超过 64 MiB。"); return false; }
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = QStringLiteral("保存 Motion JSON 失败：%1").arg(file.errorString()); return false;
    }
    error.clear();
    return true;
}

bool setNativeMotionBodyBinding(const CadDocument& cad, NativeMotionDocument& document,
                                int bodyId, const QVector<int>& partIds, QString& error)
{
    error.clear();
    const auto body = document.entity(QStringLiteral("bodies"), bodyId);
    if (body.isEmpty()) { error = QStringLiteral("运动体 ID=%1 不存在。").arg(bodyId); return false; }
    if (partIds.isEmpty() && !body.value("geoEntityIds").toArray().isEmpty()) {
        error = bodyLabel(body) + QStringLiteral("需要选择对应的 CAD 部件。"); return false;
    }
    QSet<int> seen;
    for (int partId : partIds) {
        if (!cad.part(partId) || seen.contains(partId)) { error = bodyLabel(body) + QStringLiteral("引用的 CAD 部件不存在或重复。"); return false; }
        seen.insert(partId);
        for (auto it = document.bodyPartBindings.cbegin(); it != document.bodyPartBindings.cend(); ++it)
            if (it.key() != bodyId && it.value().contains(partId)) {
                error = QStringLiteral("CAD 部件 %1 已绑定到运动体 %2。").arg(partId).arg(it.key()); return false;
            }
    }
    document.bodyPartBindings.insert(bodyId, partIds);
    return true;
}

bool validateNativeMotionBindings(const CadDocument& cad, const NativeMotionDocument& document, QString& error)
{
    NativeMotionDocument validated;
    validated.root = document.root;
    for (auto it = document.bodyPartBindings.cbegin(); it != document.bodyPartBindings.cend(); ++it)
        if (!setNativeMotionBodyBinding(cad, validated, it.key(), it.value(), error)) return false;
    for (const auto& value : document.collection(QStringLiteral("bodies"))) {
        const auto body = value.toObject();
        if (!body.value("geoEntityIds").toArray().isEmpty() && !document.bodyPartBindings.contains(body.value("id").toInt())) {
            error = bodyLabel(body) + QStringLiteral("尚未绑定 CAD 部件，请在导入面板明确选择。"); return false;
        }
    }
    for (const auto& value : document.collection(QStringLiteral("joints"))) {
        const auto joint = value.toObject();
        const int jointId = joint.value("id").toInt(-1);
        struct Selector { const char* field; const char* body; TopAbs_ShapeEnum kind; };
        const Selector selectors[] = {{"occCurveEdges", "baseBodyId", TopAbs_EDGE},
            {"occSurfaceFaces", "baseBodyId", TopAbs_FACE}, {"occCurveIEdges", "actionBodyId", TopAbs_EDGE},
            {"occCurveJEdges", "baseBodyId", TopAbs_EDGE}};
        for (const auto& selector : selectors) {
            const int bodyId = joint.value(QLatin1String(selector.body)).toInt(-1);
            const auto bodyParts = document.bodyPartBindings.value(bodyId);
            for (const auto& value : joint.value(QLatin1String(selector.field)).toArray()) {
                const auto reference = value.toObject();
                int partId = -1, index = -1;
                if (!value.isObject() || !integer(reference.value("partId"), 1, &partId)
                    || !integer(reference.value("index"), 1, &index) || !bodyParts.contains(partId) || !cad.part(partId)) {
                    error = QStringLiteral("运动副 %1 的 %2 引用了不存在或不属于运动体 %3 的 CAD 部件。请重新选择约束几何。")
                        .arg(jointId).arg(QLatin1String(selector.field)).arg(bodyId);
                    return false;
                }
                TopTools_IndexedMapOfShape shapes;
                TopExp::MapShapes(cad.part(partId)->shape, selector.kind, shapes);
                if (index > shapes.Extent()) {
                    error = QStringLiteral("运动副 %1 的 %2 在部件 %3 上引用了越界的 OCC 拓扑序号 %4（实际 %5 个）。")
                        .arg(jointId).arg(QLatin1String(selector.field)).arg(partId).arg(index).arg(shapes.Extent());
                    return false;
                }
            }
        }
    }
    error.clear();
    return true;
}

bool autoBindNativeMotionBodies(const CadDocument& cad, NativeMotionDocument& document,
                                QVector<NativeMotionBindingIssue>& unresolved, QMap<int, QString>* bindingEvidence)
{
    unresolved.clear();
    if (bindingEvidence) {
        bindingEvidence->clear();
        for (auto it = document.bodyPartBindings.cbegin(); it != document.bodyPartBindings.cend(); ++it)
            bindingEvidence->insert(it.key(), QStringLiteral("已保存或明确选择的部件绑定"));
    }
    std::map<int, ShapeSignature> signatures;
    QSet<int> occupied;
    for (const auto& ids : document.bodyPartBindings) for (int id : ids) occupied.insert(id);
    for (const auto& part : cad.parts()) signatures.emplace(part.id, signature(part));
    QMap<QString, QVector<int>> occurrences;
    QSet<QString> sourceNames;
    for (const auto& part : cad.parts()) {
        if (!part.sourceName.isEmpty()) sourceNames.insert(part.sourceName);
        // XCAF may expose several solids/sheets inside one source CAD part.
        // CadDocument appends a final child ordinal to the stable instance path.
        // Recombine those siblings before comparing the source body's mass
        // signature; one geometry ID need not mean one OCC display part.
        const int separator = part.instancePath.lastIndexOf(QLatin1Char('/'));
        if (separator > 0) occurrences[part.instancePath.left(separator)].append(part.id);
    }

    struct Pending { QJsonObject body; QVector<QVector<int>> candidates; QMap<QVector<int>, QString> evidence; bool exactIdentity = false; };
    QVector<Pending> pending;
    for (const auto& value : document.collection(QStringLiteral("bodies"))) {
        const auto body = value.toObject();
        const int bodyId = body.value("id").toInt();
        if (document.bodyPartBindings.contains(bodyId)) continue;
        const auto sourceIds = body.value("geoEntityIds").toArray();
        if (sourceIds.isEmpty()) {
            document.bodyPartBindings.insert(bodyId, {});
            if (bindingEvidence) bindingEvidence->insert(bodyId, QStringLiteral("原始虚拟运动体，没有几何引用"));
            continue;
        }
        Pending item{body, {}, {}, false};
        auto addCandidate = [&](QVector<int> ids, bool exactSourceIdentity = false) {
            if (item.exactIdentity && !exactSourceIdentity) return;
            std::sort(ids.begin(), ids.end());
            if (ids.isEmpty() || std::adjacent_find(ids.cbegin(), ids.cend()) != ids.cend()) return;
            for (int id : ids) if (occupied.contains(id) || !cad.part(id)) return;
            const bool customIdentity = exactSourceIdentity && body.value("isCustomMassProps").toBool();
            if (!customIdentity && !matchesBody(combine(ids, signatures), body)) return;
            if (exactSourceIdentity) item.exactIdentity = true;
            if (!item.candidates.contains(ids)) item.candidates.append(ids);
            if (exactSourceIdentity || !item.evidence.contains(ids)) item.evidence[ids] = customIdentity
                ? QStringLiteral("原始几何名称逐项唯一匹配 STEP 实例；保留独立的自定义质量、质心和惯量")
                : exactSourceIdentity ? QStringLiteral("原始几何名称逐项唯一匹配 STEP 实例，并通过质量几何验证")
                : QStringLiteral("初始质心及可用惯量特征唯一匹配（含多部件聚合验证）");
        };
        // Names are occurrence names, not an assumed source ID renumbering.
        // Verify their aggregate geometry as well: duplicate products can be
        // placed at different positions in an assembly.
        const auto names = body.value("geoEntityNames").toArray();
        if (names.size() == sourceIds.size()) {
            QVector<int> named;
            bool exactSourceIdentity = true;
            for (const auto& name : names) {
                QMap<QString, QVector<int>> matchingOccurrences;
                if (!name.toString().isEmpty()) for (const auto& part : cad.parts()) {
                    if (part.sourceName != name.toString()) continue;
                    const int separator = part.instancePath.lastIndexOf(QLatin1Char('/'));
                    const QString key = separator > 0 ? part.instancePath.left(separator) : QString::number(part.id);
                    matchingOccurrences[key].append(part.id);
                }
                if (matchingOccurrences.size() == 1) named += matchingOccurrences.first();
                else {
                    // Generated display suffixes are not source identities.
                    // They can only propose a candidate for physical verification.
                    exactSourceIdentity = false;
                    QVector<int> matches;
                    for (const auto& part : cad.parts())
                        if (!name.toString().isEmpty() && part.name == name.toString()) matches.append(part.id);
                    if (matches.isEmpty()) {
                        // The old display used _2 and OCC uses " (2)". This
                        // proposes a group only; the complete group's COM and
                        // inertia must still prove it. An ordinal is not identity.
                        static const QRegularExpression suffix(QStringLiteral("^(.*)_([1-9][0-9]*)$"));
                        const auto match = suffix.match(name.toString());
                        if (match.hasMatch() && sourceNames.contains(match.captured(1))) {
                            const QString displayName = match.captured(1) + QStringLiteral(" (%1)").arg(match.captured(2));
                            for (const auto& part : cad.parts()) if (part.name == displayName) matches.append(part.id);
                        }
                    }
                    if (matches.size() != 1) { named.clear(); break; }
                    named.append(matches.front());
                }
            }
            if (!named.isEmpty()) addCandidate(named, exactSourceIdentity);
        }
        // Older files only have geometry IDs. A constant renumbering is a
        // candidate, never evidence: accept it only when the saved COM and
        // available inertia fingerprint agree, with no competing candidate.
        for (const auto& part : cad.parts()) {
            const qint64 offset = qint64(part.id) - sourceIds.first().toInt();
            QVector<int> ids;
            for (const auto& id : sourceIds) {
                const qint64 mapped = qint64(id.toInt()) + offset;
                if (mapped <= 0 || mapped > std::numeric_limits<int>::max() || !cad.part(int(mapped))) { ids.clear(); break; }
                ids.append(int(mapped));
            }
            if (!ids.isEmpty()) addCandidate(ids);
        }
        // Two-part groups are common (shaft + handle). XCAF can enumerate
        // members differently from the source geometry engine. Enumerate every
        // pair, then require the exact aggregate COM/inertia fingerprint;
        // numerical source-ID spacing is never treated as a binding rule.
        if (sourceIds.size() == 2) {
            const auto& parts = cad.parts();
            for (std::size_t i = 0; i < parts.size(); ++i)
                for (std::size_t j = i + 1; j < parts.size(); ++j)
                    addCandidate({parts[i].id, parts[j].id});
        }
        for (const auto& occurrence : occurrences)
            if (occurrence.size() > 1) addCandidate(occurrence);
        pending.append(std::move(item));
    }

    // An explicit unique source occurrence name is an identity, not a geometric
    // tie-break. Reserve those parts before testing anonymous candidates; two
    // source bodies explicitly naming the same part still remain a conflict.
    QSet<int> namedParts;
    for (const auto& item : pending) if (item.exactIdentity)
        for (const auto& candidate : item.candidates) for (int id : candidate) namedParts.insert(id);
    for (auto& item : pending) if (!item.exactIdentity) {
        item.candidates.erase(std::remove_if(item.candidates.begin(), item.candidates.end(), [&](const auto& candidate) {
            for (int id : candidate) if (namedParts.contains(id)) return true;
            return false;
        }), item.candidates.end());
    }

    // Do not let iteration order choose between overlapping bodies. A unique
    // candidate is accepted only when no other unresolved candidate claims its
    // parts; ambiguity remains visible to the caller for explicit selection.
    QMap<int, QSet<int>> claims;
    for (int index = 0; index < pending.size(); ++index)
        for (const auto& candidate : pending[index].candidates)
            for (int id : candidate) claims[id].insert(index);
    for (const auto& item : pending) {
        const int bodyId = item.body.value("id").toInt();
        bool unique = item.candidates.size() == 1;
        if (unique) for (int id : item.candidates.front()) if (claims.value(id).size() != 1) unique = false;
        QString error;
        if (unique && setNativeMotionBodyBinding(cad, document, bodyId, item.candidates.front(), error)) {
            if (bindingEvidence) bindingEvidence->insert(bodyId, item.evidence.value(item.candidates.front()));
            continue;
        }
        NativeMotionBindingIssue issue;
        issue.bodyId = bodyId;
        issue.message = bodyLabel(item.body) + (item.candidates.isEmpty()
            ? QStringLiteral("没有可由名称和质量几何信息唯一确认的部件，请明确选择 CAD 部件。")
            : QStringLiteral("对应多个或互相冲突的 CAD 部件候选，请明确选择，不能按导入顺序推断。"));
        QSet<int> candidates;
        for (const auto& candidate : item.candidates) for (int id : candidate) candidates.insert(id);
        issue.candidatePartIds = candidates.values().toVector();
        std::sort(issue.candidatePartIds.begin(), issue.candidatePartIds.end());
        unresolved.append(std::move(issue));
    }

    // A large source assembly may have one background body containing every
    // remaining occurrence. Do not search arbitrary subsets or infer duplicate
    // instance ordinals. Once all other bodies have unique bindings, require the
    // complete multiset of remaining XCAF occurrence names to equal this body's
    // source names. One occurrence can contain several displayed solids.
    if (unresolved.size() == 1 && unresolved.front().bodyId >= 0) {
        const int bodyId = unresolved.front().bodyId;
        const auto body = document.entity(QStringLiteral("bodies"), bodyId);
        const auto names = body.value("geoEntityNames").toArray();
        const auto ids = body.value("geoEntityIds").toArray();
        QSet<int> used;
        for (const auto& binding : document.bodyPartBindings) for (int id : binding) used.insert(id);
        QMap<QString, int> actualNames, expectedNames;
        QVector<int> remaining;
        bool complete = !names.isEmpty() && names.size() == ids.size();
        for (const auto& occurrence : occurrences) {
            int unused = 0;
            for (int id : occurrence) if (!used.contains(id)) ++unused;
            if (!unused) continue;
            if (unused != occurrence.size()) { complete = false; break; }
            const QString name = cad.part(occurrence.front())->sourceName;
            if (name.isEmpty()) { complete = false; break; }
            for (int id : occurrence) if (cad.part(id)->sourceName != name) complete = false;
            ++actualNames[name];
            remaining += occurrence;
        }
        // Extra unnamed CAD or partial occurrences cannot disappear from the
        // identity check merely because they were absent from the name index.
        if (std::size_t(remaining.size() + used.size()) != cad.parts().size()) complete = false;
        static const QRegularExpression suffix(QStringLiteral("^(.*)_([1-9][0-9]*)$"));
        for (const auto& value : names) {
            QString name = value.toString();
            if (!sourceNames.contains(name)) {
                const auto match = suffix.match(name);
                if (match.hasMatch() && sourceNames.contains(match.captured(1))) name = match.captured(1);
            }
            if (name.isEmpty()) complete = false;
            ++expectedNames[name];
        }
        const bool custom = body.value("isCustomMassProps").toBool();
        if (complete && expectedNames == actualNames
            && (custom || matchesBody(combine(remaining, signatures), body))) {
            QString error;
            if (setNativeMotionBodyBinding(cad, document, bodyId, remaining, error)) {
                if (bindingEvidence) bindingEvidence->insert(bodyId, custom
                    ? QStringLiteral("其他运动体已唯一确认；剩余全部 STEP 实例名称多重集合与原几何名称严格一致，保留独立的自定义质量、质心和惯量")
                    : QStringLiteral("其他运动体已唯一确认；剩余全部 STEP 实例名称多重集合及聚合质心、惯量与原几何一致"));
                unresolved.clear();
            }
        }
    }
    QString bindingError;
    if (unresolved.isEmpty() && !validateNativeMotionBindings(cad, document, bindingError))
        unresolved.append({-1, bindingError, {}});
    return unresolved.isEmpty();
}

} // namespace MotionOcc
