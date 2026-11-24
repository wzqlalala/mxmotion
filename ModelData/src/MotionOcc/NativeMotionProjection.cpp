#include "NativeMotionRuntime.h"
#include "motion/physics/LinkLock.h"
#include "motion/physics/LinkMotorRotation.h"

#include <QJsonArray>

#include <Eigen/SparseCholesky>

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace MotionOcc {

struct NativeMotionSystem::PositionProjectionFactor {
    using Sparse = Eigen::SparseMatrix<double>;

    bool solve(motion::MotionSystemDescriptor& descriptor,
               const motion::MotionVectorDynamic<>& constraints, motion::MotionStateDelta& correction);
    void preparePattern(const motion::MotionSparseMatrix& jacobian);

    motion::MotionSparseMatrix jacobianPattern;
    Sparse gram;
    std::vector<Eigen::Index> products;
    Eigen::SimplicialLDLT<Sparse> factor;
};

namespace {

using Vec=motion::MotionVector<>;
constexpr double radToDeg=57.2957795130823208768;
constexpr double positionTolerance = 1e-8;

struct AssemblySettings {
    NativeMotionSystem& system;
    const std::shared_ptr<motion::MotionSolver> primary;
    const int iterations;
    const double tolerance,step;
    AssemblySettings(NativeMotionSystem& value, const std::shared_ptr<motion::MotionSolver>& projectionSolver)
        :system(value),primary(value.GetSolver()),iterations(value.GetSolverMaxIterations()),
         tolerance(value.GetSolverTolerance()),step(value.GetStep())
    {
        if (projectionSolver) system.SetSolver(projectionSolver);
        else {
            system.SetSolverMaxIterations(std::max(iterations,300));
            system.SetSolverTolerance(1e-10);
        }
        system.SetStep(1e-6);
    }
    ~AssemblySettings()
    {
        if (system.GetSolver() != primary) system.SetSolver(primary);
        system.SetSolverMaxIterations(iterations);
        system.SetSolverTolerance(tolerance);
        system.SetStep(step);
    }
};

Vec vector(const QJsonValue& value,Vec fallback=motion::VECT_Z)
{
    if(value.isUndefined()||value.isNull())return fallback;
    if(value.isArray()){const auto a=value.toArray();return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
    const auto o=value.toObject();return {o.value("x").toDouble(),o.value("y").toDouble(),o.value("z").toDouble()};
}

} // namespace

void NativeMotionSystem::PositionProjectionFactor::preparePattern(const motion::MotionSparseMatrix& jacobian)
{
    // 每个约束行只占用少量坐标。预存每一对系数在下三角矩阵中的落点。
    const auto* rows = jacobian.outerIndexPtr();
    const auto* columns = jacobian.innerIndexPtr();
    std::vector<Eigen::Triplet<double>> entries;
    for (Eigen::Index row = 0; row < jacobian.rows(); ++row) {
        for (auto i = rows[row]; i < rows[row+1]; ++i) {
            for (auto j = rows[row]; j <= i; ++j)
                entries.emplace_back(columns[i], columns[j], 0.0);
        }
    }
    gram.resize(jacobian.cols(), jacobian.cols());
    gram.setFromTriplets(entries.begin(), entries.end());
    factor.analyzePattern(gram);
    products.clear();
    products.reserve(entries.size());
    for (const auto& entry : entries) {
        const auto first = gram.innerIndexPtr() + gram.outerIndexPtr()[entry.col()];
        const auto end = gram.innerIndexPtr() + gram.outerIndexPtr()[entry.col()+1];
        products.push_back(std::lower_bound(first, end, entry.row()) - gram.innerIndexPtr());
    }
    jacobianPattern = jacobian;
}

bool NativeMotionSystem::PositionProjectionFactor::solve(motion::MotionSystemDescriptor& descriptor,
                                                         const motion::MotionVectorDynamic<>& constraints,
                                                         motion::MotionStateDelta& correction)
{
    if (constraints.size() < correction.size() || correction.size() == 0) return false;
    motion::MotionSparseMatrix jacobian;
    descriptor.ConvertToMatrixForm(&jacobian, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (jacobian.rows() != constraints.size() || jacobian.cols() != correction.size() || jacobian.nonZeros() == 0)
        return false;
    jacobian.makeCompressed();

    const auto* rows = jacobian.outerIndexPtr();
    const auto* columns = jacobian.innerIndexPtr();
    auto* values = jacobian.valuePtr();
    Eigen::VectorXd columnScale = Eigen::VectorXd::Zero(jacobian.cols());
    for (Eigen::Index i = 0; i < jacobian.nonZeros(); ++i) {
        if (!std::isfinite(values[i])) return false;
        columnScale[columns[i]] = std::max(columnScale[columns[i]], std::abs(values[i]));
    }
    if (columnScale.minCoeff() == 0) return false;
    columnScale = columnScale.cwiseInverse();
    if (!columnScale.allFinite()) return false;

    const bool samePattern = jacobianPattern.rows() == jacobian.rows() && jacobianPattern.cols() == jacobian.cols()
        && jacobianPattern.nonZeros() == jacobian.nonZeros()
        && std::equal(rows, rows+jacobian.rows()+1, jacobianPattern.outerIndexPtr())
        && std::equal(columns, columns+jacobian.nonZeros(), jacobianPattern.innerIndexPtr());
    if (!samePattern) preparePattern(jacobian);

    // 用当前雅可比值组装 JᵀJ 和 -JᵀC。只复用结构和排序，每次都重算数值。
    Eigen::VectorXd rhs = Eigen::VectorXd::Zero(jacobian.cols());
    auto* gramValues = gram.valuePtr();
    std::fill(gramValues, gramValues+gram.nonZeros(), 0.0);
    std::size_t product = 0;
    for (Eigen::Index row = 0; row < jacobian.rows(); ++row) {
        for (auto i = rows[row]; i < rows[row+1]; ++i) {
            values[i] *= columnScale[columns[i]];
            rhs[columns[i]] -= values[i] * constraints[row];
            for (auto j = rows[row]; j <= i; ++j)
                gramValues[products[product++]] += values[i] * values[j];
        }
    }
    factor.factorize(gram);
    // 法方程放大病态程度。只试用数值上满列秩的分解，其余情况交还原质量加权投影。
    if (factor.info() != Eigen::Success || !factor.vectorD().allFinite() ||
        factor.vectorD().minCoeff() <= 1e-12 * factor.vectorD().maxCoeff()) return false;
    const Eigen::VectorXd scaled = factor.solve(rhs);
    correction = (columnScale.array() * scaled.array()).matrix();
    return factor.info() == Eigen::Success && correction.allFinite();
}

void NativeMotionSystem::projectConstraintVelocity(motion::MotionState& position,
                                                    motion::MotionStateDelta& velocity, double& time)
{
    position.setZero(GetNcoords_x(), this);
    velocity.setZero(GetNcoords_v(), this);
    StateGather(position, velocity, time);

    auto residual = motion::MotionVectorDynamic<>::Zero(GetNcoords_v()).eval();
    auto constraints = motion::MotionVectorDynamic<>::Zero(GetNconstr()).eval();
    auto reactions = motion::MotionVectorDynamic<>::Zero(GetNconstr()).eval();
    LoadResidual_Mv(residual, velocity, 1.0);
    LoadConstraint_Ct(constraints, 1.0);
    motion::MotionStateDelta compatible = velocity;
    if (!StateSolveCorrection(compatible, reactions, residual, constraints, 1.0, 0.0, 0.0,
                              position, velocity, time, false, false, true) || !compatible.allFinite())
        throw std::runtime_error("速度约束投影求解失败。");
    velocity = compatible;
    StateScatter(position, velocity, time, true);
}

bool NativeMotionSystem::tryFullRankPositionProjection(bool timeAdjusted)
{
    if (!allowFullRankPositionProjection || GetSolver()->GetType() != motion::MotionSolver::Type::PSOR)
        return false;
    auto& descriptor = *GetSystemDescriptor();
    const auto& rows = descriptor.GetConstraintsList();
    const auto invalid = [](const auto* row) {
        return row->IsActive() && (row->IsUnilateral() || !row->IsLinear() || row->Get_cfm_i() != 0);
    };
    if (rows.empty() || std::any_of(rows.begin(), rows.end(), invalid)) return false;

    // 复用隐式欧拉已更新的状态和描述符。若归一化改变了时间，
    // 在精确输出时刻重新评价运动表和约束。
    if (timeAdjusted) Update();
    motion::MotionState position;
    motion::MotionStateDelta velocity, correction;
    position.setZero(GetNcoords_x(), this);
    velocity.setZero(GetNcoords_v(), this);
    correction.setZero(GetNcoords_v(), this);
    double time = 0;
    StateGather(position, velocity, time);
    const auto integratedPosition = position;
    motion::MotionVectorDynamic<> constraints(GetNconstr());
    constraints.setZero();
    LoadConstraint_C(constraints, 1.0, false);
    if (constraints.allFinite() && (constraints.size() == 0 ||
        constraints.cwiseAbs().maxCoeff() <= positionTolerance)) return true;

    if (!m_positionProjectionFactor) m_positionProjectionFactor = std::make_shared<PositionProjectionFactor>();
    // 多余约束的一阶方程不一定相容。只采用经实际更新后满足原阈值的位姿，
    // 否则恢复积分位置并由原质量加权投影继续处理。
    for (int iteration = 0; iteration < std::min(positionIterations, 5); ++iteration) {
        if (!constraints.allFinite()) break;
        const double previous = constraints.cwiseAbs().maxCoeff();
        ConstraintsLoadJacobians();
        if (!m_positionProjectionFactor->solve(descriptor, constraints, correction)) break;
        position += correction;
        StateScatter(position, velocity, time, true);
        constraints.setZero();
        LoadConstraint_C(constraints, 1.0, false);
        if (!constraints.allFinite()) break;
        const double actual = constraints.cwiseAbs().maxCoeff();
        if (actual <= positionTolerance) {
            usedFullRankPositionProjection = true;
            return true;
        }
        if (actual > previous * 0.5) break;
    }
    StateScatter(integratedPosition, velocity, time, true);
    return false;
}

void NativeMotionSystem::projectAcceptedPositions(bool timeAdjusted)
{
    if (tryFullRankPositionProjection(timeAdjusted)) return;
    auto& system=*this;
    // Same equations as MotionAssemblyAnalysis::POSITION, with no force/time
    // integration and no change to the user's solver or integrator. Prepare the
    // descriptor once, not by repeating DoAssembly's complete setup per Newton
    // iteration. The cap remains explicitly configurable in the scenario.
    system.Setup();system.Update();
    motion::MotionState position;
    motion::MotionStateDelta velocity,correction;
    position.setZero(system.GetNcoords_x(),&system);
    velocity.setZero(system.GetNcoords_v(),&system);
    correction.setZero(system.GetNcoords_v(),&system);
    motion::MotionVectorDynamic<> residual(system.GetNcoords_v()),constraints(system.GetNconstr()),reactions(system.GetNconstr());
    residual.setZero();
    double time=0;
    {
        system.DescriptorPrepareInject(*system.GetSystemDescriptor());
        const auto& rows = system.GetSystemDescriptor()->GetConstraintsList();
        const bool useBilateralQr = system.GetSolver()->GetType() == motion::MotionSolver::Type::PSOR &&
            system.rigidInterfaceFlex.empty() && !rows.empty() && std::none_of(rows.begin(), rows.end(), [](const auto* row) {
                return row->IsActive() && (row->IsUnilateral() || !row->IsLinear());
            });
        // 位置级闭环是双边方程。大惯量差会使 PSOR 残差停滞，改用平衡缩放 QR
        // 矫正本步坐标。单边约束与刚柔接口继续使用原投影策略。
        if (useBilateralQr && !m_bilateralProjectionSolver) m_bilateralProjectionSolver = makeNativeEquilibratedQr();
        AssemblySettings settings(system, useBilateralQr ? m_bilateralProjectionSolver : nullptr);
        for(int iteration=0;iteration<system.positionIterations;++iteration){
            system.StateGather(position,velocity,time);
            constraints.setZero();system.LoadConstraint_C(constraints,1.0,false);
            if(!constraints.allFinite())throw std::runtime_error("位置约束残差不是有限数值。");
            // Native residual rows mix mm and angular direction components.
            // The acceptance check below reports each in its own unit.
            if(constraints.size()==0||constraints.cwiseAbs().maxCoeff()<=positionTolerance)break;
            if (useBilateralQr) usedBilateralProjectionSolver = true;
            correction.setZero(system.GetNcoords_v(),&system);reactions.setZero();
            if(!system.StateSolveCorrection(correction,reactions,residual,constraints,1.0,0.0,0.0,
                                            position,velocity,time,false,false,true)||!correction.allFinite())
                throw std::runtime_error("位置约束投影求解失败。请检查闭环约束与求解器设置。");
            // A joint on the rigid reference has no elastic position
            // coordinates. Closing it must preserve the computed deformation.
            for (auto* assembly : rigidInterfaceFlex) {
                const auto modalOffset = assembly->GetOffset_w() + assembly->GetN_boundary_coords_w();
                correction.segment(modalOffset, assembly->Get_n_modes_coords_w()).setZero();
            }
            position+=correction;
            system.StateScatter(position,velocity,time,true);
        }
    }
    if (!rigidInterfaceFlex.empty() && system.GetNconstr() != 0) {
        // Keep the velocity tangent to the constraints at the corrected
        // pose, using the full mass matrix including modal coupling.
        system.Setup();system.Update();
        system.DescriptorPrepareInject(*system.GetSystemDescriptor());
        system.projectConstraintVelocity(position,velocity,time);
    }
}

struct NativeMotionSystem::AcceptedConstraintValidation {
    enum class LinearCheck { None, Distance, Transverse, Normal };
    enum class AngularCheck { None, Axis, Pins, Orientation };

    struct Endpoint {
        int id = -1;
        const motion::MotionBody* body = nullptr;
        const Vec* center = nullptr;
    };

    struct Joint {
        int id;
        QString name;
        QString type;
        Endpoint action, base;
        LinearCheck linear = LinearCheck::None;
        AngularCheck angular = AngularCheck::None;
        Vec origin, axis, actionPin, basePin;
        bool usesModalMarkers = false;
        motion::MotionLinkLockRevolute* modalLink = nullptr;
    };

    struct Motor {
        int id;
        motion::MotionLinkMotor* link;
        bool effort, angular;
    };

    explicit AcceptedConstraintValidation(const NativeMotionSystem& system)
        : input(system.sourceInput)
    {
        double scale = 1;
        for (const auto& body : input->bodies) {
            const Vec center(body.properties.centerMm[0], body.properties.centerMm[1], body.properties.centerMm[2]);
            centers.emplace(body.body.id, center);
            scale = std::max(scale, center.Length());
        }
        linearTolerance = std::max(.05, scale * 1e-4);

        const auto endpoint = [&](int id) {
            const auto body = system.nativeBodies.find(id);
            const auto center = centers.find(id);
            return Endpoint{id, body == system.nativeBodies.end() ? nullptr : body->second.get(),
                            center == centers.end() ? nullptr : &center->second};
        };
        const auto definitions = input->document.collection("joints");
        joints.reserve(definitions.size());
        for (const auto& value : definitions) {
            const auto definition = value.toObject();
            const auto type = definition.value("jointType").toString();
            Joint joint{definition.value("id").toInt(), definition.value("name").toString(), type,
                        endpoint(definition.value("actionBodyId").toInt(-1)),
                        endpoint(definition.value("baseBodyId").toInt(-1))};
            joint.usesModalMarkers = system.importedMnfBodyIds.count(joint.action.id) ||
                                     system.importedMnfBodyIds.count(joint.base.id);
            if (joint.usesModalMarkers) {
                const auto found = system.nativeJoints.find(std::to_string(joint.id));
                if (found != system.nativeJoints.end())
                    joint.modalLink = dynamic_cast<motion::MotionLinkLockRevolute*>(found->second.get());
            } else {
                joint.origin = vector(definition.value("origin"), motion::VNULL);
                if (type == "Revolute" || type == "Spherical" || type == "Fixed" || type == "Universal" || type == "Hooke")
                    joint.linear = LinearCheck::Distance;
                else if (type == "Translational" || type == "Cylindrical" || type == "PointLine")
                    joint.linear = LinearCheck::Transverse;
                else if (type == "Planar" || type == "PointPlane")
                    joint.linear = LinearCheck::Normal;
                if (type == "Revolute" || type == "Cylindrical" || type == "Planar")
                    joint.angular = AngularCheck::Axis;
                else if (type == "Universal" || type == "Hooke")
                    joint.angular = AngularCheck::Pins;
                else if (type == "Fixed" || type == "Translational" || type == "Direction")
                    joint.angular = AngularCheck::Orientation;

                if (joint.linear == LinearCheck::Transverse || joint.linear == LinearCheck::Normal ||
                    joint.angular == AngularCheck::Axis) {
                    joint.axis = vector(definition.value("axis"));
                    joint.axis.Normalize();
                }
                if (joint.angular == AngularCheck::Pins) {
                    joint.actionPin = vector(definition.value("actionPin"), motion::VECT_X);
                    joint.basePin = vector(definition.value("basePin"), motion::VECT_Y);
                    joint.actionPin.Normalize();
                    joint.basePin.Normalize();
                }
            }
            joints.push_back(std::move(joint));
        }

        motors.reserve(system.nativeMotors.size());
        for (const auto& item : system.nativeMotors) {
            motors.push_back({item.first, item.second.get(), system.effortMotors.count(item.first) != 0,
                              dynamic_cast<motion::MotionLinkMotorRotation*>(item.second.get()) != nullptr});
        }
    }

    const NativeMotionInput* input;
    double linearTolerance;
    std::map<int, Vec> centers;
    std::vector<Joint> joints;
    std::vector<Motor> motors;
};

void validateNativeAcceptedConstraints(const NativeMotionSystem& system)
{
    if(!system.sourceInput)return;
    const auto& input=*system.sourceInput;
    using Validation = NativeMotionSystem::AcceptedConstraintValidation;
    // 装配完成后，验收使用同一份模型参数；每步仍读取当前位姿和约束残差。
    if (!system.m_acceptedConstraintValidation || system.m_acceptedConstraintValidation->input != &input)
        system.m_acceptedConstraintValidation = std::make_shared<Validation>(system);
    const auto& validation = *system.m_acceptedConstraintValidation;
    const double linearTolerance=validation.linearTolerance;
    constexpr double angularToleranceDeg=.01;
    const auto body=[&](const Validation::Endpoint& end) -> const motion::MotionBody& {
        return end.body ? *end.body : *system.nativeBodies.at(end.id);
    };
    const auto rotation=[&](const Validation::Endpoint& end){
        return end.id<0 ? motion::QUNIT : body(end).GetRot();
    };
    const auto point=[&](const Validation::Endpoint& end,const Vec& original){
        if(end.id<0)return original;
        const auto& referenceCenter = end.center ? *end.center : validation.centers.at(end.id);
        return body(end).GetPos()+rotation(end).Rotate(original-referenceCenter);
    };
    const auto axis=[&](const Validation::Endpoint& end,const Vec& direction){return rotation(end).Rotate(direction);};
    const auto report=[&](const Validation::Joint& joint,double error,double tolerance,const QString& unit,const QString& kind){
        if(std::isfinite(error)&&error<=tolerance)return;
        const QString message=QStringLiteral("求解方案 %1：运动副 %2（%3，%4）在 t=%5 s 的%6误差为 %7 %8，超过 %9 %8。已中止，未输出违反约束的动画。请核对闭环/冗余运动副的原点与轴向；也可减小时间步或增加位置迭代次数。")
            .arg(input.scenario.id).arg(joint.id).arg(joint.name,joint.type)
            .arg(system.GetMotionTime(),0,'g',12).arg(kind).arg(error,0,'g',12).arg(unit).arg(tolerance,0,'g',12);
        throw std::runtime_error(message.toStdString());
    };
    for(const auto& joint:validation.joints){
        const auto& a=joint.action;
        const auto& b=joint.base;
        if (joint.usesModalMarkers) {
            // MNF 仍使用实际 Marker 的实时位姿，包含弹性平移和转动。
            const auto link=joint.modalLink;
            if (!link || !link->GetMarker1() || !link->GetMarker2())
                throw std::runtime_error("MNF 转动副的实际 Marker 不存在。");
            const auto& first=link->GetMarker1()->GetAbsCoord();
            const auto& second=link->GetMarker2()->GetAbsCoord();
            const auto i=first.rot.Rotate(motion::VECT_Z),j=second.rot.Rotate(motion::VECT_Z);
            report(joint,(first.pos-second.pos).Length(),linearTolerance,QStringLiteral("mm"),QStringLiteral("闭合"));
            report(joint,std::atan2(i.Cross(j).Length(),i.Dot(j))*radToDeg,angularToleranceDeg,QStringLiteral("°"),QStringLiteral("方向"));
            continue;
        }
        const Vec difference=point(a,joint.origin)-point(b,joint.origin);
        double linearError=0,angularError=0;
        if(joint.linear==Validation::LinearCheck::Distance)
            linearError=difference.Length();
        else if(joint.linear==Validation::LinearCheck::Transverse){
            const auto direction=axis(b,joint.axis);
            linearError=(difference-direction*difference.Dot(direction)).Length();
        }else if(joint.linear==Validation::LinearCheck::Normal)
            linearError=std::abs(difference.Dot(axis(b,joint.axis)));
        if(joint.angular==Validation::AngularCheck::Axis){
            const auto i=axis(a,joint.axis),j=axis(b,joint.axis);
            angularError=std::atan2(i.Cross(j).Length(),i.Dot(j))*radToDeg;
        }else if(joint.angular==Validation::AngularCheck::Pins){
            const auto i=axis(a,joint.actionPin),j=axis(b,joint.basePin);
            angularError=std::asin(std::clamp(std::abs(i.Dot(j)),0.0,1.0))*radToDeg;
        }else if(joint.angular==Validation::AngularCheck::Orientation){
            const auto i=rotation(a),j=rotation(b);
            const double dot=std::abs(i.e0()*j.e0()+i.e1()*j.e1()+i.e2()*j.e2()+i.e3()*j.e3());
            angularError=2*std::acos(std::clamp(dot,0.0,1.0))*radToDeg;
        }
        report(joint,linearError,linearTolerance,QStringLiteral("mm"),QStringLiteral("闭合"));
        report(joint,angularError,angularToleranceDeg,QStringLiteral("°"),QStringLiteral("方向"));
    }
    // ParserAdams creates FREE-guide motors because their passive joint
    // already supplies the other constraints. Their sole residual is mm for
    // translation or sin(angle-error/2) for rotation, including speed motors'
    // integrated target. This detects contradictory drives even when passive
    // joints happen to close after a least-squares solve.
    for(const auto& motor:validation.motors){
        const auto residual=motor.link->GetConstraintViolation();
        if(motor.effort){
            if(residual.size()!=0)throw std::runtime_error("力/力矩执行器意外增加了运动约束，已中止以避免伪伺服。");
            continue;
        }
        if(residual.size()!=1)throw std::runtime_error("驱动的约束维度与原生适配约定不一致。");
        const bool angular=motor.angular;
        const double error=angular?2*std::asin(std::clamp(std::abs(residual[0]),0.0,1.0))*radToDeg:std::abs(residual[0]);
        const double tolerance=angular?angularToleranceDeg:linearTolerance;
        if(std::isfinite(residual[0])&&std::isfinite(error)&&error<=tolerance)continue;
        const auto drive=input.document.entity("drives",motor.id);
        throw std::runtime_error(QStringLiteral("求解方案 %1：驱动 %2（目标运动副 %3）在 t=%4 s 的约束误差为 %5 %6，超过 %7 %6。驱动与其他约束可能冲突，已中止，未输出违反约束的动画。")
            .arg(input.scenario.id).arg(motor.id).arg(drive.value("targetJointId").toInt(-1))
            .arg(system.GetMotionTime(),0,'g',12).arg(error,0,'g',12).arg(angular?QStringLiteral("°"):QStringLiteral("mm")).arg(tolerance,0,'g',12).toStdString());
    }
}

} // namespace MotionOcc
