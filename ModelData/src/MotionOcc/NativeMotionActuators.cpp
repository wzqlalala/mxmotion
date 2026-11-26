#include "NativeMotionActuators.h"
#include "NativeMotionRuntime.h"

#include "motion/functions/Function_Expr.h"
#include "motion/functions/Function_Table.h"
#include "motion/physics/HydraulicCircuit.h"
#include "motion/physics/HydraulicCylinder.h"
#include "motion/physics/HydraulicMotor.h"
#include "motion/physics/ElectricMotor.h"
#include "motion/physics/IntegratedActuators.h"
#include "motion/physics/ElectricDrive.h"
#include "motion/physics/LinkMotorLinearForce.h"
#include "motion/physics/LinkMotorRotationTorque.h"
#include "motion/physics/FlexBodyAuxRef.h"

#include <Eigen/QR>
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

namespace MotionOcc {
namespace {
constexpr double degrees=180.0/3.14159265358979323846;
[[noreturn]] void fail(const QString& message){throw std::runtime_error(message.toStdString());}

motion::MotionVector<> jsonVec(const QJsonValue& value, motion::MotionVector<> fallback = motion::VNULL)
{
    if (value.isUndefined() || value.isNull()) return fallback;
    motion::MotionVector<> out;
    if (value.isArray()) {
        const auto a = value.toArray();
        if (a.size() != 3) fail(QStringLiteral("液压坐标向量需要三个分量。"));
        out = motion::MotionVector<>(a[0].toDouble(NAN), a[1].toDouble(NAN), a[2].toDouble(NAN));
    } else {
        const auto o = value.toObject();
        out = motion::MotionVector<>(o.value("x").toDouble(NAN), o.value("y").toDouble(NAN), o.value("z").toDouble(NAN));
    }
    if (!std::isfinite(out.x()) || !std::isfinite(out.y()) || !std::isfinite(out.z()))
        fail(QStringLiteral("液压坐标向量包含非有限数值。"));
    return out;
}

std::shared_ptr<motion::MotionBody> findSystemBody(NativeMotionSystem& system, int bodyId)
{
    const std::string key = bodyId < 0 ? std::string("ground") : std::to_string(bodyId);
    auto body = system.SearchBody(key.c_str());
    if (!body) body = std::dynamic_pointer_cast<motion::MotionBody>(system.SearchFlex(key.c_str()));
    return body;
}

std::shared_ptr<motion::MotionFunction> parseHydraulicFunction(
    const QJsonObject& row,
    const char* exprKey,
    const char* tableXKey,
    const char* tableYKey,
    const QString& fallbackExpr,
    const QString& ownerLabel,
    double& outConstValue,
    bool alwaysCreateFunction = false)
{
    std::vector<double> tx, ty;
    if (tableXKey && tableYKey) {
        for (const auto& v : row.value(QLatin1String(tableXKey)).toArray()) tx.push_back(v.toDouble());
        for (const auto& v : row.value(QLatin1String(tableYKey)).toArray()) ty.push_back(v.toDouble());
    }
    if (!tx.empty() || !ty.empty()) {
        auto table = std::make_shared<motion::MotionFunction_Table>(tx, ty);
        if (!table->IsValid()) fail(QStringLiteral("%1 控制时间表无效。").arg(ownerLabel));
        outConstValue = ty.front();
        return table;
    }
    QString expr = exprKey ? row.value(QLatin1String(exprKey)).toString(fallbackExpr).trimmed() : fallbackExpr;
    if (expr.isEmpty()) expr = fallbackExpr;
    bool isNumber = false;
    const double numeric = expr.toDouble(&isNumber);
    if (isNumber && std::isfinite(numeric)) {
        outConstValue = numeric;
        if (!alwaysCreateFunction) return nullptr;
    } else {
        outConstValue = fallbackExpr.toDouble();
    }
    auto fn = std::make_shared<motion::MotionFunction_Expr>(expr.toStdString());
    if (!fn->IsValid())
        fail(QStringLiteral("%1 函数表达式无效：%2").arg(ownerLabel, QString::fromStdString(fn->GetCompileError())));
    return fn;
}

class EffortFunction final : public motion::MotionFunction {
public:
    explicit EffortFunction(std::shared_ptr<NativeEffortRuntime> runtime):runtime(std::move(runtime)){}
    EffortFunction* Clone() const override{return new EffortFunction(*this);}
    FunctionType Get_Type() const override{return FUNCT_CUSTOM;}
    double Get_y(double time) const override{
        if(!runtime->command)fail("执行器没有力/力矩命令。");
        return 1000*limitNativeEffort(*runtime,runtime->command->Get_y(time));
    }
private:
    std::shared_ptr<NativeEffortRuntime> runtime;
};

motion::MotionFrame<> linearFrame(const motion::MotionFrame<>& frame)
{
    // Legacy CAD joints use local Z; a linear motor acts on local X. Keep a
    // right-handed frame with X=old Z, Y=old X and Z=old Y.
    const motion::MotionMatrix33<> rotation(frame.GetRot());
    motion::MotionMatrix33<> axes;
    axes.Set_A_axis(rotation.Get_A_Zaxis(),rotation.Get_A_Xaxis(),rotation.Get_A_Yaxis());
    return motion::MotionFrame<>(frame.GetPos(),axes.Get_A_quaternion());
}
}

double nativeEffortSpeed(const NativeEffortRuntime& runtime)
{
    const auto motor=runtime.motor.lock();
    if(!motor)fail("力/力矩执行器已失效。");
    if(runtime.angular){
        const auto rotary=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(motor);
        if(!rotary)fail("力矩执行器类型不匹配。");
        return rotary->GetMotorRot_dt()*degrees;
    }
    const auto linear=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(motor);
    if(!linear)fail("力执行器类型不匹配。");
    return linear->GetMotorPos_dt();
}

double limitNativeEffort(const NativeEffortRuntime& runtime,double requested)
{
    if(!std::isfinite(requested))fail("执行器命令为非有限数值。");
    const double speed=nativeEffortSpeed(runtime);
    if(!std::isfinite(speed))fail("执行器速度为非有限数值。");
    const double effort=runtime.definition.value("effortLimit").toDouble();
    const double power=runtime.definition.value("powerLimit").toDouble();
    const double noLoad=runtime.definition.value("noLoadSpeed").toDouble();
    double bound=effort>0?effort:std::numeric_limits<double>::infinity();
    // A linear torque-speed envelope limits motoring. At overspeed braking is
    // still possible; clipping both directions to zero would remove braking.
    if(noLoad>0&&requested*speed>0)bound=std::min(bound,effort*std::max(0.0,1-std::abs(speed)/noLoad));
    const double workRate=std::abs(speed)*(runtime.angular?1/degrees:1);
    if(power>0&&workRate>1e-15)bound=std::min(bound,power*1000/workRate);
    return std::clamp(requested,-bound,bound);
}

void createNativeEffortMotors(NativeMotionSystem& system,const NativeMotionInput& input)
{
    for(const auto& value:input.document.collection("drives")){
        const auto definition=value.toObject();
        const auto mode=definition.value("controlMode").toString();
        if(mode!="Force"&&mode!="Torque")continue;
        auto runtime=std::make_shared<NativeEffortRuntime>();
        runtime->driveId=definition.value("id").toInt();runtime->definition=definition;runtime->angular=mode=="Torque";
        if(runtime->angular!=(definition.value("motionType").toString()=="Rotation"))fail("力驱动仅用于平移；力矩驱动仅用于旋转。");
        const auto joint=system.joints_map.find(std::to_string(definition.value("targetJointId").toInt(-1)));
        if(joint==system.joints_map.end())fail("执行器目标副不存在。");
        const auto markerI=system.SearchMarker(joint->second->marker_I.c_str());
        const auto markerJ=system.SearchMarker(joint->second->marker_J.c_str());
        if(!markerI||!markerJ)fail("执行器目标副 Marker 不存在。");
        const auto bodyI=system.SearchBody(markerI->GetBody()->GetName());
        const auto bodyJ=system.SearchBody(markerJ->GetBody()->GetName());
        if(!bodyI||!bodyJ)fail("执行器端点运动体不存在。");
        std::vector<double> times,values;
        for(const auto& v:definition.value("driveTableX").toArray())times.push_back(v.toDouble());
        for(const auto& v:definition.value("driveTableY").toArray())values.push_back(v.toDouble());
        if(!times.empty()||!values.empty()){
            auto function=std::make_shared<motion::MotionFunction_Table>(times,values);
            if(!function->IsValid())fail("力/力矩驱动的表格无效。");
            runtime->command=function;
        }else{
            auto function=std::make_shared<motion::MotionFunction_Expr>(definition.value("expression").toString("0").toStdString());
            if(!function->IsValid())fail("力/力矩驱动表达式无效："+QString::fromStdString(function->GetCompileError()));
            runtime->command=function;
        }
        std::shared_ptr<motion::MotionLinkMotor> motor;
        if(runtime->angular){
            auto rotary=std::make_shared<motion::MotionLinkMotorRotationTorque>();
            rotary->SetSpindleConstraint(motion::MotionLinkMotorRotation::SpindleConstraint::FREE);
            rotary->Initialize(bodyI,bodyJ,false,motion::MotionFrame<>(markerI->GetAbsCoord()),motion::MotionFrame<>(markerJ->GetAbsCoord()));
            motor=rotary;
        }else{
            auto linear=std::make_shared<motion::MotionLinkMotorLinearForce>();
            linear->SetGuideConstraint(motion::MotionLinkMotorLinear::GuideConstraint::FREE);
            linear->Initialize(bodyI,bodyJ,false,linearFrame(motion::MotionFrame<>(markerI->GetAbsCoord())),linearFrame(motion::MotionFrame<>(markerJ->GetAbsCoord())));
            motor=linear;
        }
        motor->SetNameString(std::to_string(runtime->driveId));runtime->motor=motor;
        motor->SetMotorFunction(std::make_shared<EffortFunction>(runtime));
        system.AddLink(motor);system.effortMotors.emplace(runtime->driveId,runtime);
        motion::adams_motion_struct record;
        record.id=std::to_string(runtime->driveId);record.marker_I=joint->second->marker_I;record.marker_J=joint->second->marker_J;
        record.resolved_joint_id=joint->first;record.is_rotation=runtime->angular;record.is_translation=!runtime->angular;
        system.motions_map.emplace(record.id,record);
    }
}

void recordNativeEffortMotors(NativeMotionSystem& system)
{
    const double time=system.GetMotionTime();
    for(auto& entry:system.effortMotors){
        auto& runtime=*entry.second;
        if(!runtime.history.empty()&&runtime.history.back().time>=time)continue;
        const auto motor=runtime.motor.lock();if(!motor)fail("记录执行器结果时电机不存在。");
        NativeEffortSample sample;sample.time=time;sample.requested=runtime.command->Get_y(time);
        sample.applied=limitNativeEffort(runtime,sample.requested);sample.speed=nativeEffortSpeed(runtime);
        sample.power=sample.applied*sample.speed*(runtime.angular?1/degrees:1)*.001;
        sample.limited=std::abs(sample.applied-sample.requested)>1e-12*std::max(1.0,std::abs(sample.requested));
        runtime.history.push_back(sample);
        auto& history=system.motion_results[std::to_string(runtime.driveId)];
        if(history.data.empty()||std::abs(history.data.back().time-time)>1e-10)fail("执行器结果时间轴未与系统结果同步。");
        auto& value=history.data.back();
        const auto frame=motor->GetFrame2()>>*motor->GetBody2();
        const auto effort=frame.GetRot().Rotate(runtime.angular?motion::MotionVector<>(0,0,sample.applied*1000):motion::MotionVector<>(sample.applied*1000,0,0));
        if(runtime.angular){value.actuation_torque={-effort.x(),-effort.y(),-effort.z()};
            const auto rotary=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(motor);
            value.imposed_value=rotary->GetMotorRot();value.imposed_speed=rotary->GetMotorRot_dt();value.imposed_acc=rotary->GetMotorRot_dtdt();}
        else{value.actuation_force={-effort.x(),-effort.y(),-effort.z()};
            const auto linear=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(motor);
            value.imposed_value=linear->GetMotorPos();value.imposed_speed=linear->GetMotorPos_dt();value.imposed_acc=linear->GetMotorPos_dtdt();}
    }
}

void createNativeHydraulicCylinders(NativeMotionSystem& system, const NativeMotionInput& input)
{
    const auto fluid = input.document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double bulkModulus = fluid.value("bulkModulus").toDouble(1400.0);
    const double density = fluid.value("density").toDouble(860.0);
    const double supplyPressure = fluid.value("supplyPressure").toDouble(21.0);
    const double tankPressure = fluid.value("tankPressure").toDouble(0.1);

    for (const auto& value : input.document.collection("hydraulicCylinders")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const double stroke = row.value("stroke").toDouble(400.0);
        const double initialStroke = row.value("initialStroke").toDouble(200.0);

        std::shared_ptr<motion::MotionBody> bodyI, bodyJ;
        motion::MotionVector<> worldI, worldJ;
        if (row.value("mountMode").toString("Points") == QLatin1String("Joint")) {
            const int jointId = row.value("targetJointId").toInt(-1);
            const auto jointRow = input.document.entity("joints", jointId);
            if (jointRow.isEmpty()) fail(QStringLiteral("液压缸 %1 关联的运动副不存在。").arg(id));
            const auto jType = jointRow.value("jointType").toString();
            if (jType != QLatin1String("Translational") && jType != QLatin1String("Cylindrical"))
                fail(QStringLiteral("液压缸 %1 仅支持关联移动副或柱面副。").arg(id));
            bodyI = findSystemBody(system, jointRow.value("actionBodyId").toInt(-1));
            bodyJ = findSystemBody(system, jointRow.value("baseBodyId").toInt(-1));
            const auto origin = jsonVec(jointRow.value("origin"));
            auto axis = jsonVec(jointRow.value("axis"), motion::VECT_Z);
            if (axis.Length() < 1e-12) fail(QStringLiteral("液压缸 %1 关联运动副轴向为零。").arg(id));
            axis.Normalize();
            const double baseLen = std::max(50.0, stroke);
            worldJ = origin;
            worldI = origin + axis * (baseLen + initialStroke);
        } else {
            bodyI = findSystemBody(system, row.value("bodyIId").toInt(-1));
            bodyJ = findSystemBody(system, row.value("bodyJId").toInt(-1));
            worldI = jsonVec(row.value("localI"));
            worldJ = jsonVec(row.value("localJ"));
        }
        if (!bodyI || !bodyJ || bodyI == bodyJ)
            fail(QStringLiteral("液压缸 %1 端点运动体不存在或连接到同一运动体。").arg(id));
        if ((worldI - worldJ).Length() <= 1e-6)
            fail(QStringLiteral("液压缸 %1 初始两端点距离为零。").arg(id));

        auto cyl = std::make_shared<motion::MotionHydraulicCylinder>();
        cyl->SetNameString("HC_" + std::to_string(id));
        cyl->SetUnitScales(1.0, 1000.0);
        cyl->SetGeometry(row.value("pistonDiameter").toDouble(80.0),
                         row.value("rodDiameter").toDouble(45.0),
                         stroke, initialStroke);
        cyl->SetDeadVolumesMm3(row.value("deadVolumeA").toDouble(50000.0),
                               row.value("deadVolumeB").toDouble(50000.0));
        cyl->SetFluidProperties(bulkModulus, density);
        cyl->SetViscousFriction(row.value("viscousFriction").toDouble(1.0));
        cyl->SetInternalLeakage(row.value("internalLeakage").toDouble(0.0));
        cyl->SetSupplyPressures(supplyPressure, tankPressure);
        cyl->SetValveRating(row.value("ratedFlowLpm").toDouble(40.0),
                            row.value("ratedPressureDrop").toDouble(3.5));

        const QString supplyMode = row.value("supplyMode").toString("DirectionalValve");
        if (supplyMode == QLatin1String("ClosedPort"))
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::ClosedPort);
        else if (supplyMode == QLatin1String("FlowCommand"))
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::FlowCommand);
        else if (supplyMode == QLatin1String("ExternalCircuit"))
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::ExternalCircuit);
        else
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::DirectionalValve);

        cyl->SetInitialPressures(row.value("initialPressureA").toDouble(1.0),
                                 row.value("initialPressureB").toDouble(1.0));

        double constCmd = 0.0;
        auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                            QStringLiteral("0"), QStringLiteral("液压缸 %1").arg(id), constCmd, true);
        cyl->SetCommandFunction(cmdFn);

        cyl->Initialize(bodyI, bodyJ, false, worldI, worldJ);
        system.AddLink(cyl);

        auto runtime = std::make_shared<NativeHydraulicRuntime>();
        runtime->cylinderId = id;
        runtime->definition = row;
        runtime->cylinder = cyl;
        system.hydraulicCylinders.emplace(id, std::move(runtime));
    }
}

void createNativeHydraulicMotors(NativeMotionSystem& system, const NativeMotionInput& input)
{
    const auto fluid = input.document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double bulkModulus = fluid.value("bulkModulus").toDouble(1400.0);
    const double density = fluid.value("density").toDouble(860.0);
    const double supplyPressure = fluid.value("supplyPressure").toDouble(21.0);
    const double tankPressure = fluid.value("tankPressure").toDouble(0.1);

    for (const auto& value : input.document.collection("hydraulicMotors")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const int jointId = row.value("targetJointId").toInt(-1);
        const auto jointRow = input.document.entity("joints", jointId);
        if (jointRow.isEmpty()) fail(QStringLiteral("液压马达 %1 关联的运动副不存在。").arg(id));
        const QString jType = jointRow.value("jointType").toString();
        if (jType != QLatin1String("Revolute") && jType != QLatin1String("Cylindrical"))
            fail(QStringLiteral("液压马达 %1 仅支持关联旋转副或圆柱副。").arg(id));

        const auto jointIt = system.joints_map.find(std::to_string(jointId));
        if (jointIt == system.joints_map.end())
            fail(QStringLiteral("液压马达 %1 关联的运动副 %2 未启用。").arg(id).arg(jointId));
        const auto markerI = system.SearchMarker(jointIt->second->marker_I.c_str());
        const auto markerJ = system.SearchMarker(jointIt->second->marker_J.c_str());
        if (!markerI || !markerJ)
            fail(QStringLiteral("液压马达 %1 目标运动副 Marker 不存在。").arg(id));
        auto bodyI = system.SearchBody(markerI->GetBody()->GetName());
        auto bodyJ = system.SearchBody(markerJ->GetBody()->GetName());
        if (!bodyI || !bodyJ || bodyI == bodyJ)
            fail(QStringLiteral("液压马达 %1 需要连接两个不同的刚体（不支持直接挂接 MNF 模态体）。").arg(id));

        auto motor = std::make_shared<motion::MotionHydraulicMotor>();
        motor->SetNameString("HM_" + std::to_string(id));
        motor->SetTorqueFromNmmScale(1000.0); // 1 N*mm = 1000 kg*mm^2/s^2
        motor->SetDisplacementCcRev(row.value("displacementCcRev").toDouble(50.0));
        motor->SetMechanicalEfficiency(row.value("mechanicalEfficiency").toDouble(0.95));
        motor->SetChamberVolumesMm3(row.value("chamberVolumeA").toDouble(50000.0),
                                    row.value("chamberVolumeB").toDouble(50000.0));
        motor->SetFluidProperties(bulkModulus, density);
        motor->SetViscousDamping(row.value("viscousDamping").toDouble(100.0));
        motor->SetInternalLeakage(row.value("internalLeakage").toDouble(0.0));
        motor->SetSupplyPressures(supplyPressure, tankPressure);
        motor->SetValveRating(row.value("ratedFlowLpm").toDouble(40.0),
                              row.value("ratedPressureDrop").toDouble(3.5));

        const QString supplyMode = row.value("supplyMode").toString("DirectionalValve");
        if (supplyMode == QLatin1String("ClosedPort"))
            motor->SetSupplyMode(motion::MotionHydraulicMotor::SupplyMode::ClosedPort);
        else if (supplyMode == QLatin1String("FlowCommand"))
            motor->SetSupplyMode(motion::MotionHydraulicMotor::SupplyMode::FlowCommand);
        else if (supplyMode == QLatin1String("ExternalCircuit"))
            motor->SetSupplyMode(motion::MotionHydraulicMotor::SupplyMode::ExternalCircuit);
        else
            motor->SetSupplyMode(motion::MotionHydraulicMotor::SupplyMode::DirectionalValve);

        motor->SetInitialPressures(row.value("initialPressureA").toDouble(1.0),
                                   row.value("initialPressureB").toDouble(1.0));

        double constCmd = 0.0;
        auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                            QStringLiteral("0"), QStringLiteral("液压马达 %1").arg(id), constCmd, true);
        motor->SetCommandFunction(cmdFn);

        motor->Initialize(bodyI, bodyJ, false, markerI->GetAbsCoord(), markerJ->GetAbsCoord());
        system.AddLink(motor);

        auto runtime = std::make_shared<NativeHydraulicMotorRuntime>();
        runtime->motorId = id;
        runtime->definition = row;
        runtime->motor = motor;
        system.hydraulicMotors.emplace(id, std::move(runtime));
    }
}

void createNativeHydraulicCircuit(NativeMotionSystem& system, const NativeMotionInput& input)
{
    std::vector<QJsonObject> activeNodes;
    for (const auto& v : input.document.collection("hydraulicNodes")) {
        const auto row = v.toObject();
        if (row.value("enabled").toBool(true)) activeNodes.push_back(row);
    }
    std::vector<QJsonObject> activeComps;
    for (const auto& v : input.document.collection("hydraulicComponents")) {
        const auto row = v.toObject();
        if (row.value("enabled").toBool(true)) activeComps.push_back(row);
    }

    struct PortBinding {
        bool isMotor = false;
        int actuatorId = -1;
        bool portB = false;
        int nodeId = -1;
        double conductanceLpm = 200.0;
        int portConnCompId = -1;
        bool directlyFused = false;
    };
    std::vector<PortBinding> portBindings;
    const auto addOrMergeBinding = [&](bool isMotor, int actId, bool portB, int nodeId, double condLpm, int compId) {
        if (nodeId < 0) return;
        for (auto& b : portBindings) {
            if (b.isMotor == isMotor && b.actuatorId == actId && b.portB == portB) {
                if (b.nodeId != nodeId)
                    fail(QStringLiteral("%1 %2 的 %3 口同时连接到了两个不同的液压节点（%4 与 %5）。")
                             .arg(isMotor ? QStringLiteral("液压马达") : QStringLiteral("液压缸"))
                             .arg(actId)
                             .arg(portB ? QStringLiteral("B") : QStringLiteral("A"))
                             .arg(b.nodeId).arg(nodeId));
                if (compId >= 0) {
                    b.portConnCompId = compId;
                    b.conductanceLpm = condLpm;
                }
                return;
            }
        }
        portBindings.push_back({isMotor, actId, portB, nodeId, condLpm, compId, false});
    };

    for (const auto& [cid, rt] : system.hydraulicCylinders) {
        if (rt->definition.value("supplyMode").toString() != QLatin1String("ExternalCircuit")) continue;
        addOrMergeBinding(false, cid, false, rt->definition.value("portANodeId").toInt(-1), 200.0, -1);
        addOrMergeBinding(false, cid, true, rt->definition.value("portBNodeId").toInt(-1), 200.0, -1);
    }
    for (const auto& [mid, rt] : system.hydraulicMotors) {
        if (rt->definition.value("supplyMode").toString() != QLatin1String("ExternalCircuit")) continue;
        addOrMergeBinding(true, mid, false, rt->definition.value("portANodeId").toInt(-1), 200.0, -1);
        addOrMergeBinding(true, mid, true, rt->definition.value("portBNodeId").toInt(-1), 200.0, -1);
    }
    for (const auto& row : activeComps) {
        if (row.value("componentType").toString() != QLatin1String("PortConnection")) continue;
        const int compId = row.value("id").toInt();
        const int nodeId = row.value("targetNodeId").toInt(-1);
        const bool isMotor = row.value("actuatorType").toString("Cylinder") == QLatin1String("Motor");
        const int actId = row.value("actuatorId").toInt(-1);
        const bool portB = row.value("actuatorPort").toString("A") == QLatin1String("B");
        if (nodeId < 0 || actId < 0)
            fail(QStringLiteral("端口连接 [%1] 尚未指定目标液压节点或执行器。").arg(compId));
        if (isMotor && !system.hydraulicMotors.count(actId))
            fail(QStringLiteral("端口连接 [%1] 引用的液压马达 %2 不存在或未启用。").arg(compId).arg(actId));
        if (!isMotor && !system.hydraulicCylinders.count(actId))
            fail(QStringLiteral("端口连接 [%1] 引用的液压缸 %2 不存在或未启用。").arg(compId).arg(actId));
        addOrMergeBinding(isMotor, actId, portB, nodeId, row.value("conductanceLpm").toDouble(200.0), compId);
    }

    if (activeNodes.empty() && activeComps.empty() && portBindings.empty())
        return;

    const auto fluid = input.document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double bulkModulus = fluid.value("bulkModulus").toDouble(1400.0);
    const double density = fluid.value("density").toDouble(860.0);
    const double viscosity = fluid.value("kinematicViscosity").toDouble(46.0);
    const double defaultSupply = fluid.value("supplyPressure").toDouble(21.0);
    const double defaultTank = fluid.value("tankPressure").toDouble(0.1);

    auto netRuntime = std::make_shared<NativeHydraulicCircuitRuntime>();
    auto circuit = std::make_shared<motion::MotionHydraulicCircuit>();
    circuit->SetNameString("HydraulicCircuit");
    circuit->SetFluidProperties(bulkModulus, density, viscosity);

    std::map<int, std::vector<std::size_t>> bindingsPerNode;
    for (std::size_t i = 0; i < portBindings.size(); ++i)
        bindingsPerNode[portBindings[i].nodeId].push_back(i);

    for (const auto& row : activeNodes) {
        const int nid = row.value("id").toInt();
        const QString ntype = row.value("nodeType").toString("Chamber");
        netRuntime->nodeHistories[nid] = {};
        if (ntype == QLatin1String("Tank")) {
            circuit->AddBoundaryNode(nid, row.value("pressureMPa").toDouble(defaultTank), nullptr);
        } else if (ntype == QLatin1String("PressureSource")) {
            const QString defStr = QString::number(row.value("pressureMPa").toDouble(defaultSupply), 'g', 15);
            double constP = row.value("pressureMPa").toDouble(defaultSupply);
            auto pFn = parseHydraulicFunction(row, "pressureExpression", "pressureTableX", "pressureTableY",
                                              defStr, QStringLiteral("压力源节点 %1").arg(nid), constP, false);
            circuit->AddBoundaryNode(nid, constP, pFn);
        } else {
            const double volMm3 = row.value("volumeMl").toDouble(100.0) * 1000.0;
            const double initP = row.value("initialPressureMPa").toDouble(1.0);
            const auto bIt = bindingsPerNode.find(nid);
            if (bIt != bindingsPerNode.end() && bIt->second.size() == 1) {
                auto& pb = portBindings[bIt->second.front()];
                pb.directlyFused = true;
                if (pb.isMotor) {
                    auto& mot = *system.hydraulicMotors.at(pb.actuatorId)->motor;
                    if (pb.portB) mot.SetChamberVolumesMm3(mot.GetVolumeAMm3(), mot.GetVolumeBMm3() + volMm3);
                    else mot.SetChamberVolumesMm3(mot.GetVolumeAMm3() + volMm3, mot.GetVolumeBMm3());
                    circuit->BindMotorPortNode(nid, system.hydraulicMotors.at(pb.actuatorId)->motor, pb.portB);
                } else {
                    auto& cyl = *system.hydraulicCylinders.at(pb.actuatorId)->cylinder;
                    if (pb.portB) cyl.SetDeadVolumesMm3(cyl.GetDeadVolumeAMm3(), cyl.GetDeadVolumeBMm3() + volMm3);
                    else cyl.SetDeadVolumesMm3(cyl.GetDeadVolumeAMm3() + volMm3, cyl.GetDeadVolumeBMm3());
                    circuit->BindCylinderPortNode(nid, system.hydraulicCylinders.at(pb.actuatorId)->cylinder, pb.portB);
                }
                if (pb.portConnCompId >= 0)
                    netRuntime->portConnectionBranchIds[pb.portConnCompId] = 0;
            } else {
                circuit->AddVolumeNode(nid, volMm3, initP);
            }
        }
    }

    int internalSeq = 0;
    for (auto& pb : portBindings) {
        if (!circuit->HasNode(pb.nodeId))
            fail(QStringLiteral("执行器端口引用的液压节点 %1 不存在或未启用。").arg(pb.nodeId));
        if (pb.directlyFused) continue;
        ++internalSeq;
        const int internalNodeId = -10000 - internalSeq;
        if (pb.isMotor)
            circuit->BindMotorPortNode(internalNodeId, system.hydraulicMotors.at(pb.actuatorId)->motor, pb.portB);
        else
            circuit->BindCylinderPortNode(internalNodeId, system.hydraulicCylinders.at(pb.actuatorId)->cylinder, pb.portB);

        const int branchId = pb.portConnCompId >= 0 ? pb.portConnCompId : (-20000 - internalSeq);
        circuit->AddThrottleValve(branchId, pb.nodeId, internalNodeId,
                                  std::max(1.0, pb.conductanceLpm), 0.1, 1.0, nullptr);
        if (pb.portConnCompId >= 0)
            netRuntime->portConnectionBranchIds[pb.portConnCompId] = branchId;
    }

    const auto requireNode = [&](int compId, const QString& compName, const QJsonObject& row, const char* key) {
        const int nid = row.value(QLatin1String(key)).toInt(-1);
        if (nid < 0 || !circuit->HasNode(nid))
            fail(QStringLiteral("液压元件 %1 [%2] 的端口 %3 未连接有效液压节点。")
                     .arg(compName).arg(compId).arg(QLatin1String(key)));
        return nid;
    };

    for (const auto& row : activeComps) {
        const int cid = row.value("id").toInt();
        const QString cname = row.value("name").toString();
        const QString ctype = row.value("componentType").toString();
        netRuntime->componentTypes[cid] = ctype;
        netRuntime->componentHistories[cid] = {};

        if (ctype == QLatin1String("PortConnection")) {
            continue;
        } else if (ctype == QLatin1String("Accumulator")) {
            const int target = requireNode(cid, cname, row, "targetNodeId");
            circuit->AddAccumulator(cid, target,
                                    row.value("prechargePressureMPa").toDouble(10.0),
                                    row.value("gasVolumeL").toDouble(1.0),
                                    row.value("polytropicGamma").toDouble(1.3));
            netRuntime->accumulatorIds.insert(cid);
        } else if (ctype == QLatin1String("Pump")) {
            const int suc = requireNode(cid, cname, row, "suctionNodeId");
            const int del = requireNode(cid, cname, row, "deliveryNodeId");
            double constRpm = 1500.0;
            auto rpmFn = parseHydraulicFunction(row, "speedExpression", "commandTableX", "commandTableY",
                                                QStringLiteral("1500"), QStringLiteral("液压泵 %1").arg(cid), constRpm, false);
            circuit->AddPump(cid, suc, del,
                             row.value("displacementCcRev").toDouble(25.0),
                             constRpm, rpmFn,
                             row.value("volumetricEfficiency").toDouble(0.95),
                             row.value("mechanicalEfficiency").toDouble(0.90),
                             row.value("ratedPressureMPa").toDouble(21.0));
        } else if (ctype == QLatin1String("FlowSource")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            double constFlow = 20.0;
            auto flowFn = parseHydraulicFunction(row, "flowExpression", "commandTableX", "commandTableY",
                                                 QStringLiteral("20"), QStringLiteral("流量源 %1").arg(cid), constFlow, false);
            circuit->AddFlowSource(cid, inNode, outNode, constFlow, flowFn);
        } else if (ctype == QLatin1String("ReliefValve")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddReliefValve(cid, inNode, outNode,
                                    row.value("crackingPressureMPa").toDouble(21.0),
                                    row.value("pressureOverrideMPa").toDouble(1.0),
                                    row.value("ratedFlowLpm").toDouble(60.0));
        } else if (ctype == QLatin1String("PressureReducingValve")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddPressureReducingValve(cid, inNode, outNode,
                                              row.value("regulatedPressureMPa").toDouble(10.0),
                                              row.value("pressureBandMPa").toDouble(0.5),
                                              row.value("ratedFlowLpm").toDouble(50.0),
                                              row.value("ratedPressureDropMPa").toDouble(1.0));
        } else if (ctype == QLatin1String("SequenceValve")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddSequenceValve(cid, inNode, outNode,
                                      row.value("sequencePressureMPa").toDouble(15.0),
                                      row.value("pressureBandMPa").toDouble(0.5),
                                      row.value("ratedFlowLpm").toDouble(50.0),
                                      row.value("ratedPressureDropMPa").toDouble(1.0));
        } else if (ctype == QLatin1String("CheckValve")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddCheckValve(cid, inNode, outNode,
                                   row.value("crackingPressureMPa").toDouble(0.05),
                                   row.value("ratedFlowLpm").toDouble(60.0),
                                   row.value("ratedPressureDropMPa").toDouble(0.5));
        } else if (ctype == QLatin1String("ThrottleValve")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            double constOpen = 1.0;
            auto openFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                                 QStringLiteral("1"), QStringLiteral("节流阀 %1").arg(cid), constOpen, false);
            circuit->AddThrottleValve(cid, inNode, outNode,
                                      row.value("ratedFlowLpm").toDouble(40.0),
                                      row.value("ratedPressureDropMPa").toDouble(1.0),
                                      constOpen, openFn);
        } else if (ctype == QLatin1String("Pipe")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddPipe(cid, inNode, outNode,
                             row.value("lengthMm").toDouble(1000.0),
                             row.value("diameterMm").toDouble(12.0),
                             row.value("localLossCoeff").toDouble(0.5));
        } else if (ctype == QLatin1String("Filter")) {
            const int inNode = requireNode(cid, cname, row, "inletNodeId");
            const int outNode = requireNode(cid, cname, row, "outletNodeId");
            circuit->AddFilter(cid, inNode, outNode,
                               row.value("ratedFlowLpm").toDouble(80.0),
                               row.value("cleanPressureDropMPa").toDouble(0.15),
                               row.value("foulingFactor").toDouble(1.0));
        } else if (ctype == QLatin1String("DirectionalValve") || ctype == QLatin1String("ServoValve")) {
            const int np = requireNode(cid, cname, row, "portPNodeId");
            const int nt = requireNode(cid, cname, row, "portTNodeId");
            const int na = requireNode(cid, cname, row, "portANodeId");
            const int nb = requireNode(cid, cname, row, "portBNodeId");
            const QString cmStr = row.value("centerMode").toString("ClosedCenter");
            auto cm = motion::MotionHydraulicCircuit::CenterMode::ClosedCenter;
            if (cmStr == QLatin1String("TandemUnload"))
                cm = motion::MotionHydraulicCircuit::CenterMode::TandemUnloadPToT;
            else if (cmStr == QLatin1String("FloatAB"))
                cm = motion::MotionHydraulicCircuit::CenterMode::FloatABToT;
            const double tauS = ctype == QLatin1String("ServoValve")
                ? std::max(0.0, row.value("responseTimeMs").toDouble(15.0)) * 0.001
                : std::max(0.0, row.value("responseTimeMs").toDouble(0.0)) * 0.001;
            double constCmd = 0.0;
            auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                                QStringLiteral("0"), QStringLiteral("控制阀 %1").arg(cid), constCmd, false);
            circuit->AddFourWayValve(cid, np, nt, na, nb,
                                     row.value("ratedFlowLpm").toDouble(40.0),
                                     row.value("ratedPressureDropMPa").toDouble(3.5),
                                     cm, tauS, row.value("underlap").toDouble(0.0),
                                     constCmd, cmdFn);
            netRuntime->fourWayIds.insert(cid);
        }
    }

    circuit->Finalize();
    system.AddOtherPhysicsItem(circuit);
    netRuntime->circuit = std::move(circuit);
    system.hydraulicCircuit = std::move(netRuntime);
}

void createNativeElectricAndIntegratedActuators(NativeMotionSystem& system, const NativeMotionInput& input)
{
    const auto fluid = input.document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double bulkModulus = fluid.value("bulkModulus").toDouble(1400.0);

    const auto resolveLinearEndpoints = [&](const QJsonObject& row, const QString& label,
                                            double stroke, double initialStroke,
                                            std::shared_ptr<motion::MotionBody>& bodyI,
                                            std::shared_ptr<motion::MotionBody>& bodyJ,
                                            motion::MotionVector<>& worldI,
                                            motion::MotionVector<>& worldJ) {
        if (row.value("mountMode").toString("Points") == QLatin1String("Joint")) {
            const int jointId = row.value("targetJointId").toInt(-1);
            const auto jointRow = input.document.entity("joints", jointId);
            if (jointRow.isEmpty()) fail(QStringLiteral("%1 关联的运动副不存在。").arg(label));
            const auto jType = jointRow.value("jointType").toString();
            if (jType != QLatin1String("Translational") && jType != QLatin1String("Cylindrical"))
                fail(QStringLiteral("%1 仅支持关联移动副或柱面副。").arg(label));
            bodyI = findSystemBody(system, jointRow.value("actionBodyId").toInt(-1));
            bodyJ = findSystemBody(system, jointRow.value("baseBodyId").toInt(-1));
            const auto origin = jsonVec(jointRow.value("origin"));
            auto axis = jsonVec(jointRow.value("axis"), motion::VECT_Z);
            if (axis.Length() < 1e-12) fail(QStringLiteral("%1 关联运动副轴向为零。").arg(label));
            axis.Normalize();
            const double baseLen = std::max(50.0, stroke);
            worldJ = origin;
            worldI = origin + axis * (baseLen + initialStroke);
        } else {
            bodyI = findSystemBody(system, row.value("bodyIId").toInt(-1));
            bodyJ = findSystemBody(system, row.value("bodyJId").toInt(-1));
            worldI = jsonVec(row.value("localI"));
            worldJ = jsonVec(row.value("localJ"));
        }
        if (!bodyI || !bodyJ || bodyI == bodyJ)
            fail(QStringLiteral("%1 端点运动体不存在或连接到同一运动体。").arg(label));
        if ((worldI - worldJ).Length() <= 1e-6)
            fail(QStringLiteral("%1 初始两端点距离为零。").arg(label));
    };

    std::map<int, int> motorInverterBindings; // motorId -> inverterId
    std::map<int, int> ehaInverterBindings;   // ehaId -> inverterId
    std::map<int, int> emaInverterBindings;   // emaId -> inverterId
    std::set<int> usedInverters;

    // 1. Rotary Electric Servo Motors
    for (const auto& value : input.document.collection("electricMotors")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const int jointId = row.value("targetJointId").toInt(-1);
        const auto jointRow = input.document.entity("joints", jointId);
        if (jointRow.isEmpty()) fail(QStringLiteral("伺服电机 %1 关联的运动副不存在。").arg(id));
        const QString jType = jointRow.value("jointType").toString();
        if (jType != QLatin1String("Revolute") && jType != QLatin1String("Cylindrical"))
            fail(QStringLiteral("伺服电机 %1 仅支持关联旋转副或圆柱副。").arg(id));

        const auto jointIt = system.joints_map.find(std::to_string(jointId));
        if (jointIt == system.joints_map.end())
            fail(QStringLiteral("伺服电机 %1 关联的运动副 %2 未启用。").arg(id).arg(jointId));
        const auto markerI = system.SearchMarker(jointIt->second->marker_I.c_str());
        const auto markerJ = system.SearchMarker(jointIt->second->marker_J.c_str());
        if (!markerI || !markerJ)
            fail(QStringLiteral("伺服电机 %1 目标运动副 Marker 不存在。").arg(id));
        auto bodyI = system.SearchBody(markerI->GetBody()->GetName());
        auto bodyJ = system.SearchBody(markerJ->GetBody()->GetName());
        if (!bodyI || !bodyJ || bodyI == bodyJ)
            fail(QStringLiteral("伺服电机 %1 需要连接两个不同的刚体。").arg(id));

        auto motor = std::make_shared<motion::MotionElectricMotor>();
        motor->SetNameString("EM_" + std::to_string(id));
        motor->SetTorqueFromNmmScale(1000.0);
        motor->SetElectricalParameters(row.value("resistance").toDouble(0.45),
                                       row.value("inductanceMh").toDouble(3.0) * 1.0e-3,
                                       row.value("torqueConstant").toDouble(0.85),
                                       row.value("backEmfConstant").toDouble(0.85));
        motor->SetMechanicalParameters(row.value("gearRatio").toDouble(1.0),
                                       row.value("gearEfficiency").toDouble(0.96),
                                       row.value("viscousDamping").toDouble(10.0),
                                       row.value("maxCurrent").toDouble(60.0));
        motor->SetBusVoltageV(row.value("busVoltage").toDouble(48.0));
        motor->SetServoGains(row.value("kpPosition").toDouble(8.0),
                             row.value("kpSpeed").toDouble(0.25),
                             row.value("kiSpeed").toDouble(2.5),
                             row.value("kpCurrent").toDouble(6.0));

        const QString cm = row.value("controlMode").toString("SpeedClosedLoop");
        if (cm == QLatin1String("VoltageCommand"))
            motor->SetControlMode(motion::MotionElectricMotor::ControlMode::VoltageCommand);
        else if (cm == QLatin1String("CurrentClosedLoop"))
            motor->SetControlMode(motion::MotionElectricMotor::ControlMode::CurrentClosedLoop);
        else if (cm == QLatin1String("PositionClosedLoop"))
            motor->SetControlMode(motion::MotionElectricMotor::ControlMode::PositionClosedLoop);
        else if (cm == QLatin1String("ExternalInverter")) {
            motor->SetControlMode(motion::MotionElectricMotor::ControlMode::ExternalInverter);
            const int invId = row.value("inverterId").toInt(-1);
            if (invId < 0) fail(QStringLiteral("伺服电机 %1 设为外部逆变器驱动，但未选择逆变器。").arg(id));
            if (!usedInverters.insert(invId).second)
                fail(QStringLiteral("逆变器 %1 被多个执行器重复绑定。").arg(invId));
            motorInverterBindings[id] = invId;
        } else {
            motor->SetControlMode(motion::MotionElectricMotor::ControlMode::SpeedClosedLoop);
        }

        double constCmd = 0.0;
        auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                            QStringLiteral("0"), QStringLiteral("伺服电机 %1").arg(id), constCmd, true);
        motor->SetCommandFunction(cmdFn);
        motor->Initialize(bodyI, bodyJ, false, markerI->GetAbsCoord(), markerJ->GetAbsCoord());
        system.AddLink(motor);

        auto rt = std::make_shared<NativeElectricMotorRuntime>();
        rt->motorId = id;
        rt->definition = row;
        rt->motor = motor;
        system.electricMotors.emplace(id, std::move(rt));
    }

    // 2. Integrated EHA Actuators
    for (const auto& value : input.document.collection("ehaActuators")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const double stroke = row.value("stroke").toDouble(300.0);
        const double initialStroke = row.value("initialStroke").toDouble(150.0);

        std::shared_ptr<motion::MotionBody> bodyI, bodyJ;
        motion::MotionVector<> worldI, worldJ;
        resolveLinearEndpoints(row, QStringLiteral("EHA 作动器 %1").arg(id), stroke, initialStroke,
                               bodyI, bodyJ, worldI, worldJ);

        auto eha = std::make_shared<motion::MotionElectroHydraulicActuator>();
        eha->SetNameString("EHA_" + std::to_string(id));
        eha->SetUnitScales(1.0, 1000.0);
        eha->SetCylinderGeometry(row.value("pistonDiameter").toDouble(70.0),
                                 row.value("rodDiameter").toDouble(40.0),
                                 stroke, initialStroke,
                                 row.value("deadVolume").toDouble(50000.0));
        eha->SetPumpAndFluidParameters(row.value("pumpDisplacementCcRev").toDouble(4.5),
                                       bulkModulus,
                                       row.value("boostPressureMPa").toDouble(1.5),
                                       row.value("maxReliefPressureMPa").toDouble(28.0),
                                       row.value("internalLeakage").toDouble(0.02),
                                       row.value("pumpEfficiency").toDouble(0.92));
        eha->SetServoMotorParameters(row.value("resistance").toDouble(0.35),
                                     row.value("torqueConstant").toDouble(0.55),
                                     row.value("backEmfConstant").toDouble(0.55),
                                     row.value("rotorInertia").toDouble(800.0),
                                     row.value("busVoltage").toDouble(270.0),
                                     row.value("maxCurrent").toDouble(55.0));
        eha->SetViscousFriction(row.value("viscousFriction").toDouble(3.0));
        eha->SetServoGains(row.value("kpPosition").toDouble(60.0),
                           row.value("kpSpeed").toDouble(0.25),
                           row.value("kiSpeed").toDouble(3.0));

        const QString cm = row.value("controlMode").toString("PositionServo");
        if (cm == QLatin1String("SpeedCommand"))
            eha->SetControlMode(motion::MotionElectroHydraulicActuator::ControlMode::SpeedCommand);
        else if (cm == QLatin1String("VoltageCommand"))
            eha->SetControlMode(motion::MotionElectroHydraulicActuator::ControlMode::VoltageCommand);
        else if (cm == QLatin1String("ExternalInverter")) {
            eha->SetControlMode(motion::MotionElectroHydraulicActuator::ControlMode::ExternalInverter);
            const int invId = row.value("inverterId").toInt(-1);
            if (invId < 0) fail(QStringLiteral("EHA 作动器 %1 设为外部逆变器驱动，但未选择逆变器。").arg(id));
            if (!usedInverters.insert(invId).second)
                fail(QStringLiteral("逆变器 %1 被多个执行器重复绑定。").arg(invId));
            ehaInverterBindings[id] = invId;
        } else {
            eha->SetControlMode(motion::MotionElectroHydraulicActuator::ControlMode::PositionServo);
        }

        double constCmd = 0.0;
        auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                            QString::number(initialStroke, 'g', 12),
                                            QStringLiteral("EHA 作动器 %1").arg(id), constCmd, true);
        eha->SetCommandFunction(cmdFn);
        eha->Initialize(bodyI, bodyJ, false, worldI, worldJ);
        system.AddLink(eha);

        auto rt = std::make_shared<NativeEhaRuntime>();
        rt->actuatorId = id;
        rt->definition = row;
        rt->actuator = eha;
        system.ehaActuators.emplace(id, std::move(rt));
    }

    // 3. Integrated EMA Actuators
    for (const auto& value : input.document.collection("emaActuators")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const double stroke = row.value("stroke").toDouble(300.0);
        const double initialStroke = row.value("initialStroke").toDouble(150.0);

        std::shared_ptr<motion::MotionBody> bodyI, bodyJ;
        motion::MotionVector<> worldI, worldJ;
        resolveLinearEndpoints(row, QStringLiteral("EMA 作动器 %1").arg(id), stroke, initialStroke,
                               bodyI, bodyJ, worldI, worldJ);

        auto ema = std::make_shared<motion::MotionElectroMechanicalActuator>();
        ema->SetNameString("EMA_" + std::to_string(id));
        ema->SetUnitScales(1.0, 1000.0);
        ema->SetGeometry(row.value("screwLead").toDouble(10.0),
                         row.value("gearRatio").toDouble(2.0),
                         stroke, initialStroke);
        ema->SetMotorParameters(row.value("resistance").toDouble(0.40),
                                row.value("inductanceMh").toDouble(2.5) * 1.0e-3,
                                row.value("torqueConstant").toDouble(0.65),
                                row.value("backEmfConstant").toDouble(0.65),
                                row.value("busVoltage").toDouble(270.0),
                                row.value("maxCurrent").toDouble(45.0));
        ema->SetMechanicalLoss(row.value("mechanicalEfficiency").toDouble(0.90),
                               row.value("viscousFriction").toDouble(2.5));
        ema->SetServoGains(row.value("kpPosition").toDouble(12.0),
                           row.value("kpSpeed").toDouble(0.35),
                           row.value("kiSpeed").toDouble(2.0),
                           row.value("kpCurrent").toDouble(5.0));

        const QString cm = row.value("controlMode").toString("PositionServo");
        if (cm == QLatin1String("VelocityServo"))
            ema->SetControlMode(motion::MotionElectroMechanicalActuator::ControlMode::VelocityServo);
        else if (cm == QLatin1String("VoltageCommand"))
            ema->SetControlMode(motion::MotionElectroMechanicalActuator::ControlMode::VoltageCommand);
        else if (cm == QLatin1String("ExternalInverter")) {
            ema->SetControlMode(motion::MotionElectroMechanicalActuator::ControlMode::ExternalInverter);
            const int invId = row.value("inverterId").toInt(-1);
            if (invId < 0) fail(QStringLiteral("EMA 作动器 %1 设为外部逆变器驱动，但未选择逆变器。").arg(id));
            if (!usedInverters.insert(invId).second)
                fail(QStringLiteral("逆变器 %1 被多个执行器重复绑定。").arg(invId));
            emaInverterBindings[id] = invId;
        } else {
            ema->SetControlMode(motion::MotionElectroMechanicalActuator::ControlMode::PositionServo);
        }

        double constCmd = 0.0;
        auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                            QString::number(initialStroke, 'g', 12),
                                            QStringLiteral("EMA 作动器 %1").arg(id), constCmd, true);
        ema->SetCommandFunction(cmdFn);
        ema->Initialize(bodyI, bodyJ, false, worldI, worldJ);
        system.AddLink(ema);

        auto rt = std::make_shared<NativeEmaRuntime>();
        rt->actuatorId = id;
        rt->definition = row;
        rt->actuator = ema;
        system.emaActuators.emplace(id, std::move(rt));
    }

    // 4. Electric Power Supplies & Servo Inverters Network
    std::vector<QJsonObject> activeSupplies;
    for (const auto& v : input.document.collection("powerSupplies")) {
        const auto row = v.toObject();
        if (row.value("enabled").toBool(true)) activeSupplies.push_back(row);
    }
    std::vector<QJsonObject> activeInverters;
    for (const auto& v : input.document.collection("inverters")) {
        const auto row = v.toObject();
        if (row.value("enabled").toBool(true)) activeInverters.push_back(row);
    }

    if (!activeSupplies.empty() || !activeInverters.empty() || !usedInverters.empty()) {
        auto drvRt = std::make_shared<NativeElectricDriveRuntime>();
        auto net = std::make_shared<motion::MotionElectricDriveNetwork>();
        net->SetNameString("ELECTRIC_DRIVE_NETWORK");

        for (const auto& row : activeSupplies) {
            const int sid = row.value("id").toInt();
            const double nomV = row.value("nominalVoltage").toDouble(270.0);
            double constV = nomV;
            auto vFn = parseHydraulicFunction(row, "voltageExpression", "voltageTableX", "voltageTableY",
                                              QString::number(nomV, 'g', 12),
                                              QStringLiteral("直流电源 %1").arg(sid), constV);
            net->AddPowerSupply(sid, nomV, vFn,
                                row.value("internalResistance").toDouble(0.05),
                                row.value("capacitanceMf").toDouble(4.7) * 1.0e-3,
                                row.value("maxCurrent").toDouble(120.0),
                                row.value("chopperVoltage").toDouble(0.0),
                                row.value("chopperResistance").toDouble(8.0));
            drvRt->supplyHistories[sid];
        }

        for (const auto& row : activeInverters) {
            const int iid = row.value("id").toInt();
            const int sid = row.value("supplyId").toInt(-1);
            if (sid >= 0 && !net->HasPowerSupply(sid))
                fail(QStringLiteral("伺服逆变器 %1 关联的直流电源 %2 未启用或不存在。").arg(iid).arg(sid));
            const QString cmStr = row.value("controlMode").toString("SpeedClosedLoop");
            auto imode = motion::MotionElectricDriveNetwork::InverterMode::SpeedClosedLoop;
            if (cmStr == QLatin1String("DutyCommand"))
                imode = motion::MotionElectricDriveNetwork::InverterMode::DutyCommand;
            else if (cmStr == QLatin1String("CurrentClosedLoop"))
                imode = motion::MotionElectricDriveNetwork::InverterMode::CurrentClosedLoop;
            else if (cmStr == QLatin1String("PositionClosedLoop"))
                imode = motion::MotionElectricDriveNetwork::InverterMode::PositionClosedLoop;

            double constCmd = 0.0;
            auto cmdFn = parseHydraulicFunction(row, "commandExpression", "commandTableX", "commandTableY",
                                                QStringLiteral("0"),
                                                QStringLiteral("伺服逆变器 %1").arg(iid), constCmd);
            net->AddInverter(iid, sid,
                             row.value("nominalBusVoltage").toDouble(270.0),
                             imode,
                             row.value("efficiency").toDouble(0.96),
                             row.value("maxCurrent").toDouble(60.0),
                             row.value("kpPosition").toDouble(8.0),
                             row.value("kpSpeed").toDouble(0.30),
                             row.value("kiSpeed").toDouble(2.5),
                             row.value("kpCurrent").toDouble(5.0),
                             constCmd, cmdFn);
            drvRt->inverterHistories[iid];
        }

        for (const auto& kv : motorInverterBindings) {
            if (!net->HasInverter(kv.second))
                fail(QStringLiteral("伺服电机 %1 关联的逆变器 %2 未启用。").arg(kv.first).arg(kv.second));
            net->BindElectricMotor(kv.second, system.electricMotors.at(kv.first)->motor);
        }
        for (const auto& kv : ehaInverterBindings) {
            if (!net->HasInverter(kv.second))
                fail(QStringLiteral("EHA 作动器 %1 关联的逆变器 %2 未启用。").arg(kv.first).arg(kv.second));
            net->BindEha(kv.second, system.ehaActuators.at(kv.first)->actuator);
        }
        for (const auto& kv : emaInverterBindings) {
            if (!net->HasInverter(kv.second))
                fail(QStringLiteral("EMA 作动器 %1 关联的逆变器 %2 未启用。").arg(kv.first).arg(kv.second));
            net->BindEma(kv.second, system.emaActuators.at(kv.first)->actuator);
        }

        net->Finalize();
        system.AddOtherPhysicsItem(net);
        drvRt->network = std::move(net);
        system.electricDrive = std::move(drvRt);
    }
}

QString initializeSteadyNativeHydraulics(NativeMotionSystem& system, const QJsonObject& scenario)
{
    if (!scenario.value("hydraulicSteadyInit").toBool(false)
        || (system.hydraulicCylinders.empty() && system.hydraulicMotors.empty()
            && system.electricMotors.empty() && system.ehaActuators.empty() && system.emaActuators.empty()))
        return {};

    std::vector<NativeHydraulicRuntime*> cylRuntimes;
    for (auto& entry : system.hydraulicCylinders) {
        cylRuntimes.push_back(entry.second.get());
        entry.second->cylinder->SetDisabled(true);
    }
    std::vector<NativeHydraulicMotorRuntime*> motRuntimes;
    for (auto& entry : system.hydraulicMotors) {
        motRuntimes.push_back(entry.second.get());
        entry.second->motor->SetDisabled(true);
    }
    std::vector<NativeElectricMotorRuntime*> emRuntimes;
    for (auto& entry : system.electricMotors) {
        emRuntimes.push_back(entry.second.get());
        entry.second->motor->SetDisabled(true);
    }
    std::vector<NativeEhaRuntime*> ehaRuntimes;
    for (auto& entry : system.ehaActuators) {
        ehaRuntimes.push_back(entry.second.get());
        entry.second->actuator->SetDisabled(true);
    }
    std::vector<NativeEmaRuntime*> emaRuntimes;
    for (auto& entry : system.emaActuators) {
        emaRuntimes.push_back(entry.second.get());
        entry.second->actuator->SetDisabled(true);
    }

    system.Setup();
    system.Update(0.0, false);
    system.prepareStaticDescriptor();
    system.ConstraintsLoadJacobians();

    const int n = system.GetNcoords_v();
    const int m = system.GetNconstr();
    const int kCyl = static_cast<int>(cylRuntimes.size());
    const int kMot = static_cast<int>(motRuntimes.size());
    const int kEm = static_cast<int>(emRuntimes.size());
    const int kEha = static_cast<int>(ehaRuntimes.size());
    const int kEma = static_cast<int>(emaRuntimes.size());
    const int kTot = kCyl + kMot + kEm + kEha + kEma;

    if (n > 0 && kTot > 0) {
        motion::MotionVectorDynamic<> force(n);
        force.setZero();
        system.LoadResidual_F(force, 1.0);

        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, m + kTot);
        if (m > 0) {
            motion::MotionSparseMatrix cq;
            system.GetConstraintJacobianMatrix(&cq);
            if (cq.rows() == m && cq.cols() == n) {
                A.leftCols(m) = Eigen::MatrixXd(cq.transpose());
            }
        }
        const auto addLinearColumn = [&](motion::MotionLinkTSDA& link, int col) {
            auto* b1 = static_cast<motion::MotionBody*>(link.GetBody1());
            auto* b2 = static_cast<motion::MotionBody*>(link.GetBody2());
            const auto p1 = b1->TransformPointLocalToParent(link.GetPoint1Rel());
            const auto p2 = b2->TransformPointLocalToParent(link.GetPoint2Rel());
            const auto dir = (p1 - p2).GetNormalized();
            if (b1->Variables().IsActive()) {
                const int off1 = static_cast<int>(b1->GetOffset_w());
                const auto lt1 = b1->TransformDirectionParentToLocal(motion::Vcross(p1 - b1->GetCoord().pos, dir));
                A.block<3, 1>(off1, col) = dir.eigen();
                A.block<3, 1>(off1 + 3, col) = lt1.eigen();
            }
            if (b2->Variables().IsActive()) {
                const int off2 = static_cast<int>(b2->GetOffset_w());
                const auto lt2 = b2->TransformDirectionParentToLocal(motion::Vcross(p2 - b2->GetCoord().pos, -dir));
                A.block<3, 1>(off2, col) = (-dir).eigen();
                A.block<3, 1>(off2 + 3, col) = lt2.eigen();
            }
        };
        const auto addRotaryColumn = [&](motion::MotionLink& link, int col) {
            auto* b1 = static_cast<motion::MotionBody*>(link.GetBody1());
            auto* b2 = static_cast<motion::MotionBody*>(link.GetBody2());
            const auto nz = b2->TransformDirectionLocalToParent(link.GetLinkRelativeCoords().rot.GetZaxis()).GetNormalized();
            if (b1->Variables().IsActive()) {
                const int off1 = static_cast<int>(b1->GetOffset_w());
                A.block<3, 1>(off1 + 3, col) = b1->TransformDirectionParentToLocal(nz).eigen();
            }
            if (b2->Variables().IsActive()) {
                const int off2 = static_cast<int>(b2->GetOffset_w());
                A.block<3, 1>(off2 + 3, col) = b2->TransformDirectionParentToLocal(-nz).eigen();
            }
        };

        int curCol = m;
        for (int r = 0; r < kCyl; ++r) addLinearColumn(*cylRuntimes[r]->cylinder, curCol++);
        for (int j = 0; j < kMot; ++j) addRotaryColumn(*motRuntimes[j]->motor, curCol++);
        for (int j = 0; j < kEm; ++j) addRotaryColumn(*emRuntimes[j]->motor, curCol++);
        for (int r = 0; r < kEha; ++r) addLinearColumn(*ehaRuntimes[r]->actuator, curCol++);
        for (int r = 0; r < kEma; ++r) addLinearColumn(*emaRuntimes[r]->actuator, curCol++);

        const Eigen::VectorXd sol = A.completeOrthogonalDecomposition().solve(-force);
        curCol = m;
        for (int r = 0; r < kCyl; ++r, ++curCol) {
            auto& rt = *cylRuntimes[r];
            rt.cylinder->SetDisabled(false);
            const double fReqN = (sol.size() > curCol && std::isfinite(sol(curCol))) ? sol(curCol) * 0.001 : 0.0;
            const double preload = std::max(rt.cylinder->GetTankPressureMPa(),
                std::min(rt.definition.value("initialPressureA").toDouble(1.0),
                         rt.definition.value("initialPressureB").toDouble(1.0)));
            rt.cylinder->ApplySteadyInitialPressures(fReqN, preload);
        }
        for (int j = 0; j < kMot; ++j, ++curCol) {
            auto& rt = *motRuntimes[j];
            rt.motor->SetDisabled(false);
            const double tauReqNmm = (sol.size() > curCol && std::isfinite(sol(curCol))) ? sol(curCol) * 0.001 : 0.0;
            const double preload = std::max(rt.motor->GetTankPressureMPa(),
                std::min(rt.definition.value("initialPressureA").toDouble(1.0),
                         rt.definition.value("initialPressureB").toDouble(1.0)));
            rt.motor->ApplySteadyInitialPressures(tauReqNmm, preload);
        }
        for (int j = 0; j < kEm; ++j, ++curCol) {
            auto& rt = *emRuntimes[j];
            rt.motor->SetDisabled(false);
            const double tauReqNmm = (sol.size() > curCol && std::isfinite(sol(curCol))) ? sol(curCol) * 0.001 : 0.0;
            rt.motor->ApplySteadyInitialLoadTorque(tauReqNmm);
        }
        for (int r = 0; r < kEha; ++r, ++curCol) {
            auto& rt = *ehaRuntimes[r];
            rt.actuator->SetDisabled(false);
            const double fReqN = (sol.size() > curCol && std::isfinite(sol(curCol))) ? sol(curCol) * 0.001 : 0.0;
            rt.actuator->ApplySteadyInitialLoad(fReqN);
        }
        for (int r = 0; r < kEma; ++r, ++curCol) {
            auto& rt = *emaRuntimes[r];
            rt.actuator->SetDisabled(false);
            const double fReqN = (sol.size() > curCol && std::isfinite(sol(curCol))) ? sol(curCol) * 0.001 : 0.0;
            rt.actuator->ApplySteadyInitialLoad(fReqN);
        }
        if (system.electricDrive && system.electricDrive->network) {
            system.electricDrive->network->Finalize();
        }
    } else {
        for (auto* rt : cylRuntimes) rt->cylinder->SetDisabled(false);
        for (auto* rt : motRuntimes) rt->motor->SetDisabled(false);
        for (auto* rt : emRuntimes) rt->motor->SetDisabled(false);
        for (auto* rt : ehaRuntimes) rt->actuator->SetDisabled(false);
        for (auto* rt : emaRuntimes) rt->actuator->SetDisabled(false);
    }

    system.Setup();
    system.Update(0.0, true);
    return QStringLiteral("液压稳态初始化已完成：按 t=0 静载平衡 %1 个机电液执行器初始腔压/电流。")
        .arg(kTot);
}

void recordNativeHydraulicCylinders(NativeMotionSystem& system)
{
    recordNativeHydraulics(system);
}

void recordNativeHydraulics(NativeMotionSystem& system)
{
    const double time = system.GetMotionTime();
    for (auto& entry : system.hydraulicCylinders) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& cyl = *rt.cylinder;
        NativeHydraulicSample sample;
        sample.time = time;
        sample.pressureA = cyl.GetPressureAMPa();
        sample.pressureB = cyl.GetPressureBMPa();
        sample.forceN = cyl.GetTotalForceN();
        sample.strokeMm = cyl.GetStrokeMm();
        sample.velocityMmS = cyl.GetStrokeRateMmS();
        sample.flowALpm = cyl.GetFlowALpm();
        sample.flowBLpm = cyl.GetFlowBLpm();
        sample.command = cyl.GetCommandValue(time);
        sample.mechPowerW = cyl.GetMechanicalPowerW();
        sample.fluidPowerW = cyl.GetFluidPowerW();
        rt.history.push_back(sample);
    }
    for (auto& entry : system.hydraulicMotors) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& mot = *rt.motor;
        NativeHydraulicMotorSample sample;
        sample.time = time;
        sample.pressureA = mot.GetPressureAMPa();
        sample.pressureB = mot.GetPressureBMPa();
        sample.torqueNmm = mot.GetTotalTorqueNmm();
        sample.angleDeg = mot.GetAngleDeg();
        sample.speedRpm = mot.GetSpeedRpm();
        sample.flowALpm = mot.GetFlowALpm();
        sample.flowBLpm = mot.GetFlowBLpm();
        sample.command = mot.GetCommandValue(time);
        sample.mechPowerW = mot.GetMechanicalPowerW();
        sample.fluidPowerW = mot.GetFluidPowerW();
        rt.history.push_back(sample);
    }
    if (system.hydraulicCircuit && system.hydraulicCircuit->circuit) {
        auto& net = *system.hydraulicCircuit;
        const auto& c = *net.circuit;
        for (auto& [nid, hist] : net.nodeHistories) {
            if (!hist.empty() && hist.back().time >= time) continue;
            NativeHydraulicNodeSample sample;
            sample.time = time;
            sample.pressureMPa = c.GetNodePressureMPa(nid);
            sample.netFlowLpm = c.GetNodeNetFlowLpm(nid);
            hist.push_back(sample);
        }
        for (auto& [cid, hist] : net.componentHistories) {
            if (!hist.empty() && hist.back().time >= time) continue;
            NativeHydraulicComponentSample sample;
            sample.time = time;
            const QString ctype = net.componentTypes[cid];
            if (ctype == QLatin1String("Accumulator")) {
                sample.flowLpm = c.GetAccumulatorFlowLpm(cid);
                sample.gasPressureMPa = c.GetAccumulatorPressureMPa(cid);
                sample.gasVolumeL = c.GetAccumulatorGasVolumeL(cid);
                sample.oilVolumeL = c.GetAccumulatorOilVolumeL(cid);
            } else if (ctype == QLatin1String("PortConnection")) {
                const int bid = net.portConnectionBranchIds[cid];
                if (bid != 0) {
                    sample.flowLpm = c.GetComponentFlowLpm(bid);
                    sample.deltaPressureMPa = c.GetComponentDeltaPressureMPa(bid);
                    sample.powerW = c.GetComponentPowerW(bid);
                    sample.spoolPosition = 1.0;
                    sample.command = 1.0;
                } else if (system.sourceInput) {
                    const auto pc = system.sourceInput->document.entity("hydraulicComponents", cid);
                    const bool isMotor = pc.value("actuatorType").toString("Cylinder") == QLatin1String("Motor");
                    const int actId = pc.value("actuatorId").toInt(-1);
                    const bool portB = pc.value("actuatorPort").toString("A") == QLatin1String("B");
                    if (isMotor && system.hydraulicMotors.count(actId)) {
                        const auto& mot = *system.hydraulicMotors.at(actId)->motor;
                        sample.flowLpm = portB ? mot.GetFlowBLpm() : mot.GetFlowALpm();
                    } else if (!isMotor && system.hydraulicCylinders.count(actId)) {
                        const auto& cyl = *system.hydraulicCylinders.at(actId)->cylinder;
                        sample.flowLpm = portB ? cyl.GetFlowBLpm() : cyl.GetFlowALpm();
                    }
                    sample.spoolPosition = 1.0;
                    sample.command = 1.0;
                }
            } else {
                sample.flowLpm = c.GetComponentFlowLpm(cid);
                sample.deltaPressureMPa = c.GetComponentDeltaPressureMPa(cid);
                sample.spoolPosition = c.GetComponentSpoolPosition(cid);
                sample.command = c.GetComponentCommand(cid);
                sample.powerW = c.GetComponentPowerW(cid);
                if (net.fourWayIds.count(cid)) {
                    sample.flowALpm = c.GetComponentFlowALpm(cid);
                    sample.flowBLpm = c.GetComponentFlowBLpm(cid);
                }
            }
            hist.push_back(sample);
        }
    }
    for (auto& entry : system.electricMotors) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& m = *rt.motor;
        NativeElectricMotorSample s;
        s.time = time;
        s.currentA = m.GetCurrentA();
        s.voltageV = m.GetTerminalVoltageV();
        s.backEmfV = m.GetBackEmfV();
        s.torqueNmm = m.GetTotalTorqueNmm();
        s.angleDeg = m.GetAngleDeg();
        s.speedRpm = m.GetJointSpeedRpm();
        s.command = m.GetCommandValue(time);
        s.elecPowerW = m.GetElectricalPowerW();
        s.mechPowerW = m.GetMechanicalPowerW();
        s.copperLossW = m.GetCopperLossW();
        rt.history.push_back(s);
    }
    for (auto& entry : system.ehaActuators) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& a = *rt.actuator;
        NativeEhaSample s;
        s.time = time;
        s.pressureA = a.GetPressureAMPa();
        s.pressureB = a.GetPressureBMPa();
        s.pumpSpeedRpm = a.GetPumpSpeedRpm();
        s.currentA = a.GetCurrentA();
        s.voltageV = a.GetVoltageV();
        s.forceN = a.GetTotalForceN();
        s.strokeMm = a.GetStrokeMm();
        s.velocityMmS = a.GetVelocityMmS();
        s.command = a.GetCommandValue(time);
        s.elecPowerW = a.GetElectricalPowerW();
        s.mechPowerW = a.GetMechanicalPowerW();
        rt.history.push_back(s);
    }
    for (auto& entry : system.emaActuators) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& a = *rt.actuator;
        NativeEmaSample s;
        s.time = time;
        s.strokeMm = a.GetStrokeMm();
        s.velocityMmS = a.GetVelocityMmS();
        s.motorSpeedRpm = a.GetMotorSpeedRpm();
        s.currentA = a.GetCurrentA();
        s.voltageV = a.GetVoltageV();
        s.forceN = a.GetTotalForceN();
        s.command = a.GetCommandValue(time);
        s.elecPowerW = a.GetElectricalPowerW();
        s.mechPowerW = a.GetMechanicalPowerW();
        rt.history.push_back(s);
    }
    if (system.electricDrive && system.electricDrive->network) {
        auto& drv = *system.electricDrive;
        const auto& net = *drv.network;
        for (auto& [sid, hist] : drv.supplyHistories) {
            if (!hist.empty() && hist.back().time >= time) continue;
            NativePowerSupplySample s;
            s.time = time;
            s.busVoltageV = net.GetSupplyBusVoltageV(sid);
            s.sourceCurrentA = net.GetSupplySourceCurrentA(sid);
            s.loadCurrentA = net.GetSupplyLoadCurrentA(sid);
            s.chopperCurrentA = net.GetSupplyChopperCurrentA(sid);
            s.powerW = net.GetSupplyOutputPowerW(sid);
            s.storedEnergyJ = net.GetSupplyStoredEnergyJ(sid);
            hist.push_back(s);
        }
        for (auto& [iid, hist] : drv.inverterHistories) {
            if (!hist.empty() && hist.back().time >= time) continue;
            NativeInverterSample s;
            s.time = time;
            s.busVoltageV = net.GetInverterBusVoltageV(iid);
            s.outputVoltageV = net.GetInverterOutputVoltageV(iid);
            s.phaseCurrentA = net.GetInverterPhaseCurrentA(iid);
            s.dcCurrentA = net.GetInverterDcCurrentA(iid);
            s.modulationIndex = net.GetInverterModulationIndex(iid);
            s.outputPowerW = net.GetInverterOutputPowerW(iid);
            s.lossW = net.GetInverterLossW(iid);
            s.command = net.GetInverterCommand(iid);
            hist.push_back(s);
        }
    }
}
}
