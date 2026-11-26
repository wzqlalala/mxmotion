#include "NativeMotionRuntime.h"
#include "NativeMotionStatics.h"

#include "motion/utils/ParserAdams.h"
#include "motion/utils/ContactInputSettings.h"
#include "motion/collision/CollisionSystem.h"
#include "motion/physics/ContactContainer.h"
#include "motion/physics/MaterialSurfaceSMC.h"
#include "motion/physics/Force.h"
#include "motion/physics/LinkMotorLinear.h"
#include "motion/physics/LinkMotorRotation.h"
#include "motion/physics/Link.h"
#include "motion/physics/LinkPointOnSurface.h"
#include "motion/physics/LinkPointSpline.h"
#include "motion/solver/DirectSolverLS.h"
#include "motion/timestepper/TimestepperHHT.h"

#include <Eigen/SparseQR>

#include <QJsonArray>
#include <QQuaternion>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <Eigen/QR>

#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

namespace MotionOcc {
namespace {

constexpr double pi=3.14159265358979323846;
constexpr double d2r=pi/180.0, r2d=180.0/pi;
using Vec=motion::MotionVector<>;
[[noreturn]] void fail(const QString& message) { throw std::runtime_error(message.toStdString()); }
std::string key(int id) { return std::to_string(id); }
std::string bodyKey(int id) { return id<0?"ground":key(id); }
Vec vector(const QJsonValue& value, Vec fallback=motion::VNULL)
{
    if (value.isUndefined() || value.isNull()) return fallback;
    Vec out;
    if (value.isArray()) { const auto a=value.toArray(); if(a.size()!=3) fail("向量需要三个分量。"); out=Vec(a[0].toDouble(NAN),a[1].toDouble(NAN),a[2].toDouble(NAN)); }
    else { const auto o=value.toObject(); out=Vec(o.value("x").toDouble(NAN),o.value("y").toDouble(NAN),o.value("z").toDouble(NAN)); }
    if (!std::isfinite(out.x()) || !std::isfinite(out.y()) || !std::isfinite(out.z())) fail("向量包含无效数值。");
    return out;
}
Vec unit(const QJsonValue& value, Vec fallback=motion::VECT_Z)
{
    auto out=vector(value,fallback);
    if (out.Length()<1e-12) fail("方向向量不能为零。");
    out.Normalize(); return out;
}

class AnalyticContactHistory final : public motion::MotionContactContainer::ReportContactCallback,
                                     public motion::collision::MotionCollisionSystem::BroadphaseCallback {
    struct Pair {
        int id;
        double normal = 0.0;
        std::vector<double> history;
    };
    using Model=motion::collision::MotionCollisionModel;
    std::map<const Model*, int> bodies;
    std::map<std::pair<int,int>, Pair> pairs;

public:
    bool OnBroadphase(Model* a, Model* b) override
    {
        const auto i=bodies.find(a),j=bodies.find(b);
        if (i==bodies.end() && j==bodies.end()) return true;
        return i!=bodies.end() && j!=bodies.end() && pairs.count(std::minmax(i->second,j->second))!=0;
    }

    bool OnReportContact(const Vec&, const Vec&, const motion::MotionMatrix33<>&,
            const double&, const double&, const Vec& force, const Vec&,
            motion::MotionContactable* a, motion::MotionContactable* b) override
    {
        auto* action=dynamic_cast<motion::MotionBody*>(a);
        auto* base=dynamic_cast<motion::MotionBody*>(b);
        if (!action || !base) return true;
        const auto i=bodies.find(action->GetCollisionModel().get()),j=bodies.find(base->GetCollisionModel().get());
        if (i!=bodies.end() && j!=bodies.end()) {
            const auto pair=pairs.find(std::minmax(i->second,j->second));
            if (pair!=pairs.end()) pair->second.normal+=std::max(0.0,force.x())*.001;
        }
        return true;
    }

    bool install(NativeMotionSystem& system, const NativeMotionDocument& document)
    {
        std::map<int,QJsonObject> shapes;
        const auto contacts=document.collection("contacts");
        for (const auto& value:contacts) {
            const auto row=value.toObject();
            if (!row.value("enabled").toBool(true) || !row.contains("analyticContact")) continue;
            const auto options=row.value("analyticContact").toObject();
            const auto a=row.value("actionBodyId").toInt(-1), b=row.value("baseBodyId").toInt(-1);
            if (a==b || !pairs.emplace(std::minmax(a,b),Pair{row.value("id").toInt()}).second)
                fail(QStringLiteral("解析接触需要两个不同的运动体，且同一对运动体只能配置一次。"));
            auto material=motion_types::make_shared<motion::MotionMaterialSurfaceSMC>();
            material->SetKn(row.value("stiffness").toDouble()*1000);
            material->SetKt(options.value("tangentialStiffness").toDouble()*1000);
            material->SetGn(options.value("normalDampingRate").toDouble());
            material->SetGt(options.value("tangentialDampingRate").toDouble());
            const bool friction=row.value("frictionEnabled").toBool();
            material->SetSfriction(friction?row.value("staticFriction").toDouble():0);
            material->SetKfriction(friction?row.value("dynamicFriction").toDouble():0);
            QJsonObject materialKey{
                {"Kn",material->GetKn()},{"Kt",material->GetKt()},
                {"Gn",material->GetGn()},{"Gt",material->GetGt()},
                {"muS",material->GetSfriction()},{"muK",material->GetKfriction()},
                {"margin",options.value("collisionMargin").toDouble()}};
            const auto add=[&](int id,const QJsonObject& shape) {
                const auto existing=shapes.find(id);
                QJsonObject definition=materialKey;
                definition.insert("shape",shape);
                if (existing!=shapes.end()) {
                    if (existing->second!=definition)
                        fail(QStringLiteral("共享运动体的解析接触形状与材料设置须一致。"));
                    return;
                }
                const auto body=std::dynamic_pointer_cast<motion::MotionBody>(system.SearchBody(bodyKey(id).c_str()));
                if (!body) fail(QStringLiteral("解析接触只能用于刚体。"));
                const auto model=body->GetCollisionModel();
                const auto center=body->TransformPointParentToLocal(vector(shape.value("center")));
                const double margin=options.value("collisionMargin").toDouble();
                model->ClearModel();
                model->SetSafeMargin(margin);
                model->SetEnvelope(margin);
                bool built=false;
                if (shape.value("kind").toString()=="Sphere") {
                    built=model->AddSphere(material,shape.value("radius").toDouble(),center);
                } else {
                    const auto size=vector(shape.value("size"))*.5;
                    // 输入盒轴沿初始世界坐标系，随后随运动体姿态转动。
                    built=model->AddBox(material,size.x(),size.y(),size.z(),center,
                        motion::MotionMatrix33<>(body->GetRot().GetConjugate()));
                }
                if (!built) fail(QStringLiteral("创建解析接触形状失败。"));
                model->BuildModel();
                body->SetCollide(true);
                bodies.emplace(model.get(),id);
                shapes.emplace(id,std::move(definition));
            };
            add(a,options.value("actionShape").toObject());
            add(b,options.value("baseShape").toObject());
        }
        if (pairs.empty()) return false;
        // 接触模型是系统级设置，校验阶段已禁止与 CAD/面片接触混用。
        system.UseMaterialProperties(false);
        system.SetContactForceModel(motion::MotionSystemSMC::Hooke);
        return true;
    }

    void sample(const std::shared_ptr<AnalyticContactHistory>& self, NativeMotionSystem& system)
    {
        for (auto& [key,pair]:pairs) pair.normal=0.0;
        system.GetContactContainer()->ReportAllContacts(self);
        for (auto& [key,pair]:pairs) pair.history.push_back(pair.normal);
    }

    void transfer(RigidResult& result)
    {
        for (auto& [key,pair]:pairs) {
            if (pair.history.size()!=result.frames.size()) fail(QStringLiteral("解析接触时程与结果帧不一致。"));
            result.channels.push_back({"contacts",pair.id,"NormalForce","N",1,std::move(pair.history)});
        }
    }
};
void fill(double (&a)[3],const Vec& v) { for(int d=0;d<3;++d)a[d]=v[d]; }
std::vector<double> array(const QJsonValue& value,double scale=1)
{
    std::vector<double> out;
    for(const auto& v:value.toArray()) {const double x=v.toDouble(NAN)*scale;if(!std::isfinite(x))fail("表格包含非数值。");out.push_back(x);}
    return out;
}
std::string formula(const QString& expression,double scale=1)
{
    if(expression.trimmed().isEmpty())return {};
    return scale==1?expression.toStdString():QStringLiteral("(%1)*%2").arg(expression,QString::number(scale,'g',17)).toStdString();
}
bool active(const QJsonObject& o) { return o.value("enabled").toBool(true); }

#if defined(MX_HAS_BODY_DRIVES)
std::vector<motion::utils::BodyDrive> bodyDriveDefinitions(const NativeMotionDocument& document)
{
    using motion::utils::BodyDrive;
    using motion::utils::BodyDriveMode;
    std::vector<BodyDrive> definitions;
    for (const auto& entry : document.collection("bodyDrives")) {
        const auto row = entry.toObject();
        if (!active(row)) continue;
        BodyDrive drive;
        drive.id = "BD_" + key(row.value("id").toInt());
        drive.body_id = key(row.value("targetBodyId").toInt(-1));
        const int reference = row.value("referenceBodyId").toInt(-1);
        if (reference >= 0) drive.reference_body_id = key(reference);
        const auto mode = row.value("bodyDriveMode").toString();
        const bool pose = mode == "Pose";
        const bool rotation = mode == "RotationAngle" || mode == "RotationSpeed";
        if (pose) drive.mode = BodyDriveMode::POSE;
        else if (mode == "RotationAngle") drive.mode = BodyDriveMode::ROTATION_ANGLE;
        else if (mode == "RotationSpeed") drive.mode = BodyDriveMode::ROTATION_SPEED;
        else if (mode == "LinearPosition") drive.mode = BodyDriveMode::LINEAR_POSITION;
        else if (mode == "LinearSpeed") drive.mode = BodyDriveMode::LINEAR_SPEED;
        else fail(QStringLiteral("未知运动体驱动类型：%1").arg(mode));
        drive.guide = row.value("guide").toBool(true);
        drive.frame_abs.SetPos(vector(row.value("origin")));
        if (pose) {
            const auto a = vector(row.value("frameAngles")) * d2r;
            drive.frame_abs.SetRot(motion::Q_from_AngX(a.x()) * motion::Q_from_AngY(a.y()) * motion::Q_from_AngZ(a.z()));
        } else {
            Vec x, y, z;
            const auto axis = unit(row.value("axis"), rotation ? motion::VECT_Z : motion::VECT_X);
            if (rotation) axis.DirToDxDyDz(z, x, y, motion::VECT_X);
            else axis.DirToDxDyDz(x, y, z, motion::VECT_Y);
            drive.frame_abs.SetRot(motion::MotionMatrix33<>(x, y, z));
        }
        const auto channels = row.value("channels").toObject();
        const QString axes = pose ? QStringLiteral("XYZABC") : QStringLiteral("X");
        for (int i = 0; i < axes.size(); ++i) {
            auto& definition = drive.channels[i];
            const auto channel = channels.value(QString(axes[i])).toObject();
            const double scale = rotation || (pose && i >= 3) ? d2r : 1;
            definition.x_table = array(channel.value("driveTableX"));
            definition.f_table = array(channel.value("driveTableY"), scale);
            definition.has_table = !definition.x_table.empty() || !definition.f_table.empty();
            const auto expression = channel.value("expression").toString("0");
            bool constant = false;
            const double value = expression.toDouble(&constant);
            if (constant) definition.func_value = value * scale;
            else definition.function_expr = formula(expression, scale);
        }
        definitions.push_back(std::move(drive));
    }
    return definitions;
}
#endif

motion::SimulationConfig configuration(const QJsonObject& o)
{
    motion::SimulationConfig c;
    c.total_time=o.value("endTime").toDouble(1);
    c.step_size=o.value("stepSize").toDouble(.01);
    c.use_preset=o.value("usePreset").toBool(true);
    const QString preset=o.value("presetMode").toString("FAST");
    if(preset=="FAST")c.preset=motion::SimulationConfig::Preset::FAST;
    else if(preset=="CONTACT")c.preset=motion::SimulationConfig::Preset::CONTACT;
    else if(preset=="ROBUST")c.preset=motion::SimulationConfig::Preset::ROBUST;
    else if(preset=="HIGH_PRECISION")c.preset=motion::SimulationConfig::Preset::HIGH_PRECISION;
    else fail("未知求解预设："+preset);
    const std::map<QString,motion::MotionTimestepper::Type> integrators{
#define INTEGRATOR(x) {#x,motion::MotionTimestepper::Type::x}
        INTEGRATOR(EULER_IMPLICIT_LINEARIZED),INTEGRATOR(EULER_IMPLICIT_PROJECTED),INTEGRATOR(EULER_IMPLICIT),
        INTEGRATOR(TRAPEZOIDAL),INTEGRATOR(TRAPEZOIDAL_LINEARIZED),INTEGRATOR(HHT),INTEGRATOR(HEUN),
        INTEGRATOR(RUNGEKUTTA45),INTEGRATOR(EULER_EXPLICIT),INTEGRATOR(LEAPFROG),INTEGRATOR(NEWMARK)
#undef INTEGRATOR
    };
    const std::map<QString,motion::MotionSolver::Type> solvers{
#define SOLVER(x) {#x,motion::MotionSolver::Type::x}
        SOLVER(PSOR),SOLVER(PSSOR),SOLVER(PJACOBI),SOLVER(PMINRES),SOLVER(BARZILAIBORWEIN),SOLVER(APGD),
        SOLVER(SPARSE_LU),SOLVER(SPARSE_QR),SOLVER(GMRES),SOLVER(MINRES)
#undef SOLVER
    };
    const auto it=integrators.find(o.value("integratorType").toString("EULER_IMPLICIT_LINEARIZED"));
    const QString solver=o.value("solverType").toString("SPARSE_QR");
    const auto st=solvers.find(solver);
    if(!c.use_preset && (it==integrators.end() || st==solvers.end()))
        fail(QStringLiteral("所选积分器或求解器在当前开放引擎构建中不可用：%1 / %2。ADDM/BICGSTAB 尚无系统工厂实现，PARDISO/MUMPS 需要对应第三方构建，不能静默替换。")
             .arg(o.value("integratorType").toString(),solver));
    if(it!=integrators.end())c.integrator.type=it->second;
    if(st!=solvers.end())c.solver.type=st->second;
    c.integrator.alpha=o.value("integratorAlpha").toDouble(-.2);
    c.integrator.beta=o.value("integratorBeta").toDouble(.25);
    c.integrator.max_iters=o.value("integratorMaxIters").toInt(50);
    c.integrator.tol=o.value("integratorTol").toDouble(1e-8);
    c.solver.max_iters=o.value("solverMaxIters").toInt(100);
    c.solver.tolerance=o.value("solverTolerance").toDouble(1e-8);
    c.solver.verbose=o.value("solverVerbose").toBool();
    return c;
}

class EquilibratedMotionQr final : public motion::MotionSolverSparseQR {
protected:
    bool FactorizeMatrix() override
    {
        if (A().rows() == 0 || A().rows() != A().cols()) return false;
        motion::MotionSparseMatrix scaled = A();
        m_scale = Eigen::VectorXd::Ones(A().rows());
        // Solve (D A D)y = Db and recover x = Dy. The rigid solver uses mm,
        // so large inertias can otherwise hide small constraint pivots.
        for (int pass = 0; pass < 12; ++pass) {
            Eigen::VectorXd factors(A().rows());
            for (int row = 0; row < scaled.outerSize(); ++row) {
                double largest = 0;
                for (motion::MotionSparseMatrix::InnerIterator entry(scaled, row); entry; ++entry) {
                    if (!std::isfinite(entry.value())) return false;
                    largest = std::max(largest, std::abs(entry.value()));
                }
                factors[row] = largest > 0 ? 1 / std::sqrt(largest) : 1;
            }
            for (int row = 0; row < scaled.outerSize(); ++row)
                for (motion::MotionSparseMatrix::InnerIterator entry(scaled, row); entry; ++entry) {
                    entry.valueRef() *= factors[entry.row()];
                    entry.valueRef() *= factors[entry.col()];
                }
            m_scale.array() *= factors.array();
            if (!m_scale.allFinite() || (m_scale.array() <= 0).any()) return false;
        }
        m_qr.compute(Eigen::SparseMatrix<double>(scaled));
        return m_qr.info() == Eigen::Success;
    }

    bool SolveSystem() override
    {
        if (b().size() != m_scale.size()) fail(QStringLiteral("平衡缩放稀疏 QR 的方程尺寸不匹配。"));
        const Eigen::VectorXd rhs = m_scale.array() * b().array();
        if (!rhs.allFinite()) fail(QStringLiteral("平衡缩放稀疏 QR 的右端项包含非有限数值。"));
        const Eigen::VectorXd solution = m_qr.solve(rhs);
        x() = m_scale.array() * solution.array();
        if (m_qr.info() != Eigen::Success || !x().allFinite())
            fail(QStringLiteral("平衡缩放稀疏 QR 未得到有限解。"));
        return true;
    }

private:
    Eigen::VectorXd m_scale;
    Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> m_qr;
};

bool isRigidPositionDriven(const NativeMotionInput& input, const NativeMotionSystem& system)
{
    if (system.motions_map.empty() || !system.effortMotors.empty() ||
        std::hypot(input.scenario.gravityMS2[0], input.scenario.gravityMS2[1], input.scenario.gravityMS2[2]) != 0)
        return false;
    for (const auto* category : {"autoFlexBodies", "springs", "contacts", "controllers", "couplings",
                                 "scalarForces", "scalarTorques", "threePointForces", "threePointTorques",
                                 "vectorForces", "tensionCables", "softStops", "hydraulicCylinders",
                                 "hydraulicMotors", "hydraulicNodes", "hydraulicComponents",
                                 "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"})
        if (!input.document.collection(QLatin1String(category)).isEmpty()) return false;
    return std::all_of(system.motions_map.begin(), system.motions_map.end(), [](const auto& drive) {
        return drive.second.is_position;
    }) && std::all_of(system.joints_map.begin(), system.joints_map.end(), [](const auto& item) {
        const auto& joint = *item.second;
        return !joint.has_friction && !joint.has_limit &&
            (joint.type == "FIXED" || joint.type == "REVOLUTE" || joint.type == "CYLINDRICAL" || joint.type == "TRANSLATIONAL");
    });
}

bool useEquilibratedMotionQr(const NativeMotionInput& input, const NativeMotionSystem& system,
                             const motion::SimulationConfig& config)
{
    return !config.use_preset && config.solver.type == motion::MotionSolver::Type::SPARSE_QR &&
        config.integrator.type == motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED &&
        isRigidPositionDriven(input, system);
}

class NativeMnfSystem final : public NativeMotionSystem {
public:
    bool linearizedMnf = false;

    void StateScatter(const motion::MotionState& position, const motion::MotionStateDelta& velocity,
                      double time, bool updateAssets) override
    {
        NativeMotionSystem::StateScatter(position, velocity, time, updateAssets);
        if (weightIndependentFlexProjectionWithStiffness) {
            // 引擎先更新顶层运动副，再写入嵌套装配的柔性接口位姿。
            // 位姿散布完成后重算顶层约束，以免位置迭代读取上一轮的接口坐标。
            for (const auto& link : Get_linklist())
                if (link->IsActive()) link->Update(time, false);
        }
    }

    bool StateSolveCorrection(motion::MotionStateDelta& correction,
                              motion::MotionVectorDynamic<>& multipliers,
                              const motion::MotionVectorDynamic<>& residual,
                              const motion::MotionVectorDynamic<>& constraints,
                              double mass, double damping, double stiffness,
                              const motion::MotionState& position,
                              const motion::MotionStateDelta& velocity, double time,
                              bool scatter, bool update, bool setup) override
    {
        const auto solve = [&] {
            return NativeMotionSystem::StateSolveCorrection(correction, multipliers, residual, constraints,
                mass, damping, stiffness, position, velocity, time, scatter, update, setup);
        };
        if (!linearizedMnf)
            return solve();
        // 线性化积分直接使用已组装的模态质量、刚度和阻尼，保留实际约束雅可比。
        QScopedValueRollback<bool> tangent(implicit_tangent_enabled, false);
        return solve();
    }
};

class ImportedMnfTimestepper final : public motion::MotionTimestepperHHT {
public:
    static constexpr double maxStepSeconds = 0.002;

    explicit ImportedMnfTimestepper(const motion::MotionTimestepperHHT& configured)
        : motion::MotionTimestepperHHT(configured) {}

    void Advance(double outputInterval) override
    {
        // A coarse step can produce spurious interface deformation even when
        // Newton converges. Bound the integration step without adding output
        // frames. The engine can still subdivide each step on nonconvergence.
        const double count = std::ceil(outputInterval / maxStepSeconds);
        const double step = outputInterval / count;
        for (double index = 0; index < count; ++index)
            motion::MotionTimestepperHHT::Advance(step);
    }
};

class Assembly {
public:
    Assembly(NativeMotionSystem& system,const NativeMotionInput& source,motion::SimulationConfig config)
        :s(system),in(source),config(config){}
    std::vector<NativeFlexRuntime> build()
    {
        readImportedDefinitions();
        bodies(); joints(); couplings(); drives(); markers(); springs(); forces(); contacts(); unilateral();
        return std::move(importedFlex);
    }
    void configureRuntimeDirections()
    {
        for(const auto& value:in.document.collection("vectorForces")) {
            const auto o=value.toObject();if(!active(o)||o.value("loadType").toString()=="Bushing")continue;
            const auto body=s.SearchBody(bodyKey(o.value("bodyId").toInt(-1)).c_str());
            if(!body)fail("矢量力作用体未装配。");
            const auto name=key(o.value("id").toInt());
            bool found=false;
            for(const auto& f:body->GetForceList())if(f->GetNameString()==name){
                const bool local=o.value("directionMode").toString()=="BodyFixed";
                f->SetAlign(local?motion::MotionForce::BODY_DIR:motion::MotionForce::WORLD_DIR);
                if(local)f->SetRelDir(unit(o.value("direction")));
                else f->SetDir(unit(o.value("direction")));
                found=true;
            }
            if(!found)fail(QStringLiteral("矢量力 %1 未在引擎中创建。").arg(o.value("id").toInt()));
        }
        // The container marker format stores a Z direction. Restore the full
        // user quaternion after parsing, preserving user X/Y orientation too.
        for(const auto& value:in.document.collection("markers")){
            const auto o=value.toObject();const auto q=o.value("localRotation").toObject();
            const auto marker=s.SearchMarker(("UM_"+key(o.value("id").toInt())).c_str());
            if(!marker)fail("用户 Marker 未装配。");
            auto coord=marker->GetAbsCoord();
            coord.rot=motion::MotionQuaternion<>(q.value("w").toDouble(1),q.value("x").toDouble(),q.value("y").toDouble(),q.value("z").toDouble());
            coord.rot.Normalize();marker->Impose_Abs_Coord(coord);
        }
    }
private:
    NativeMotionSystem& s;const NativeMotionInput& in;motion::SimulationConfig config;int markerCounter=0;
    std::map<int,QJsonObject> importedDefinitions;
    std::vector<NativeFlexRuntime> importedFlex;

    void readImportedDefinitions()
    {
        std::set<int> ids;
        for (const auto& value : in.document.collection("importedMnfBodies")) {
            const auto definition = value.toObject();
            if (!active(definition)) continue;
            const int id = definition.value("id").toInt(-1);
            const int target = definition.value("targetBodyId").toInt(-1);
            if (id < 0 || !ids.insert(id).second || target < 0 || !importedDefinitions.emplace(target,definition).second)
                fail("导入 MNF 柔性体的编号或目标运动体无效，或有重复绑定。");
            const auto body = std::find_if(in.bodies.begin(),in.bodies.end(),[target](const PreparedBody& value){return value.body.id==target;});
            if (body == in.bodies.end() || body->body.fixed)
                fail(QStringLiteral("MNF 柔性体 %1 需要有效的非固定目标运动体。").arg(id));
            if (std::any_of(in.geometry->flexBodies.begin(),in.geometry->flexBodies.end(),[target](const NativeFlexGeometry& autoFlex){return autoFlex.bodyId==target;}))
                fail(QStringLiteral("运动体 %1 同时绑定了自动柔性体和 MNF 柔性体。").arg(target));
            const auto bindings = definition.value("jointBindings").toObject();
            if (bindings.isEmpty())
                fail(QStringLiteral("MNF 柔性体 %1 需要至少一个已绑定接口节点的转动副。").arg(id));
            for (auto binding=bindings.begin(); binding!=bindings.end(); ++binding) {
                bool numeric = false;
                const int joint = binding.key().toInt(&numeric);
                const auto jointDefinition = in.document.entity("joints",joint);
                const int action = jointDefinition.value("actionBodyId").toInt(-1);
                const int base = jointDefinition.value("baseBodyId").toInt(-1);
                if (!numeric || binding.key()!=QString::number(joint) || (action==target)==(base==target))
                    fail(QStringLiteral("MNF 柔性体 %1 的接口映射引用了无效运动副 %2。").arg(id).arg(binding.key()));
            }
        }
    }

    bool usesImportedMnf(const motion::adams_joint_struct& joint) const
    {
        return !s.markers_map.at(joint.marker_I).flex_id.empty() || !s.markers_map.at(joint.marker_J).flex_id.empty();
    }

    std::string marker(int body,const Vec& point,const Vec& axis=motion::VECT_Z,const std::string& requested={},int mnfNode=-1)
    {
        motion::adams_marker_struct m{};
        if (importedDefinitions.count(body)) {
            if (mnfNode < 0)
                fail(QStringLiteral("运动体 %1 使用导入的 MNF，当前只支持带显式接口节点绑定的普通转动副。").arg(body));
            m.flex_id = bodyKey(body);
            m.node_id = key(mnfNode);
        } else {
            if(!s.parts_map.count(bodyKey(body)))fail(QStringLiteral("Marker 引用不存在的运动体 %1。").arg(body));
            m.part_id=bodyKey(body);
        }
        fill(m.loc,point);fill(m.rot,axis);
        const auto id=requested.empty()?"NATIVE_MK_"+key(markerCounter++):requested;
        if(!s.markers_map.emplace(id,m).second)fail("重复 Marker ID。");return id;
    }
    std::pair<std::string,std::string> endpoints(const QJsonObject& o)
    {
        if(o.contains("markerIId")||o.contains("markerJId")){
            const auto a="UM_"+key(o.value("markerIId").toInt(-1));
            const auto b="UM_"+key(o.value("markerJId").toInt(-1));
            if(!s.markers_map.count(a)||!s.markers_map.count(b))fail("单边力元引用不存在的 Marker。");
            return {a,b};
        }
        return {marker(o.value("bodyIId").toInt(-1),vector(o.value("localI"))),
                marker(o.value("bodyJId").toInt(-1),vector(o.value("localJ")))};
    }
    void bodies()
    {
        motion::adams_part_struct ground{};ground.fixed=true;s.parts_map.emplace("ground",ground);
        for(const auto& prepared:in.bodies){
            const auto& b=prepared.body;const auto& p=prepared.properties;const auto src=in.document.entity("bodies",b.id);
            const auto imported = importedDefinitions.find(b.id);
            if (imported != importedDefinitions.end()) {
                const auto& definition = imported->second;
                const auto file = definition.value("mnfFile").toString();
                if (file.trimmed().isEmpty()) fail(QStringLiteral("MNF 柔性体 %1 缺少文件路径。").arg(b.id));
                const auto path = in.document.assetPath(file);
                const auto translation = vector(definition.value("referenceTranslationMm"));
                const auto euler = vector(definition.value("referenceEuler321Deg")) * d2r;
                const auto rotation = motion::Q_from_AngZ(euler.z()) * motion::Q_from_AngY(euler.y()) * motion::Q_from_AngX(euler.x());
                importedFlex.push_back(buildImportedMnfFlex(s,prepared,definition,path,motion::MotionFrame<>(translation,rotation),
                    vector(src.value("initialVelocity")),vector(src.value("initialAngularVelocity"))*d2r));
                s.importedMnfBodyIds.insert(b.id);
                continue;
            }
            motion::adams_part_struct part{};part.fixed=b.fixed;part.mass=p.massKg;
            for(int i=0;i<6;++i)part.inertia[i]=p.inertiaKgM2[i]*1e6;
            fill(part.vel,vector(src.value("initialVelocity")));
            fill(part.omega,vector(src.value("initialAngularVelocity"))*d2r);
            part.cm_marker_id="CM_"+key(b.id);s.parts_map.emplace(key(b.id),part);
            marker(b.id,Vec(p.centerMm[0],p.centerMm[1],p.centerMm[2]),motion::VECT_Z,part.cm_marker_id);
        }
    }
    void frictionLimit(const QJsonObject& o,motion::adams_joint_struct& target)
    {
        const auto type=o.value("jointType").toString();const auto f=o.value("friction").toObject();
        const bool supportsFriction=type=="Revolute"||type=="Translational"||type=="Cylindrical";
        target.has_friction=supportsFriction&&f.value("enabled").toBool();
        target.mu_static=f.value("staticCoefficient").toDouble();target.mu_kinetic=f.value("kineticCoefficient").toDouble();
        // This deprecated serialized coefficient was passed through in the
        // original solver; the current original panel fixes it to zero.
        target.viscous_friction=f.value("viscousCoefficient").toDouble();
        target.friction_radius=f.value("effectiveRadius").toDouble();
        target.use_stiction_constraint=target.has_friction&&!config.use_preset&&config.integrator.type==motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED&&config.solver.type==motion::MotionSolver::Type::PSOR;
        const auto l=o.value("limit").toObject();
        target.has_limit=l.value("enabled").toBool()&&(type=="Revolute"||type=="Translational");
        target.lower_limit_enabled=target.has_limit&&l.value("lowerEnabled").toBool();
        target.upper_limit_enabled=target.has_limit&&l.value("upperEnabled").toBool();
        const double angle=type=="Revolute"?d2r:1;
        const double force=type=="Revolute"?1000*r2d:1000;
        target.lower_limit=l.value("lowerValue").toDouble()*angle;target.upper_limit=l.value("upperValue").toDouble()*angle;
        target.limit_penalty_only=l.value("penaltyOnly").toBool();
        target.lower_limit_cushion=l.value("lowerCushion").toDouble()*angle;target.upper_limit_cushion=l.value("upperCushion").toDouble()*angle;
        target.lower_limit_stiffness=l.value("lowerStiffness").toDouble()*force;target.upper_limit_stiffness=l.value("upperStiffness").toDouble()*force;
        target.lower_limit_damping=l.value("lowerDamping").toDouble()*force;target.upper_limit_damping=l.value("upperDamping").toDouble()*force;
        if(target.lower_limit_enabled&&target.upper_limit_enabled&&target.lower_limit>target.upper_limit)fail("限位下限大于上限。");
        if(target.limit_penalty_only&&((target.lower_limit_enabled&&(target.lower_limit_cushion<=0||target.lower_limit_stiffness<=0))||(target.upper_limit_enabled&&(target.upper_limit_cushion<=0||target.upper_limit_stiffness<=0))))fail("纯软限位的启用侧需要正刚度和正缓冲区宽度。");
    }
    void joints()
    {
        const std::map<QString,std::string> types{{"Revolute","REVOLUTE"},{"Translational","TRANSLATIONAL"},{"Cylindrical","CYLINDRICAL"},{"Spherical","SPHERICAL"},{"Fixed","FIXED"},{"Universal","UNIVERSAL"},{"PointPlane","POINTPLANE"},{"Parallel","PARALLEL"},{"Perpendicular","PERPENDICULAR"},{"Direction","ORIENTATION"},{"ConstantVelocity","CONVEL"},{"Hooke","HOOKE"},{"Screw","SCREW"},{"Planar","PLANAR"},{"PointLine","POINTLINE"},{"PointOnSurface","POINTONSURFACE"},{"PointOnCurve","POINTONCURVE"},{"CurveOnCurve","CURVEONCURVE"}};
        for(const auto& value:in.document.collection("joints")){
            const auto o=value.toObject();const int id=o.value("id").toInt();const QString type=o.value("jointType").toString();
            if(type=="Gear")fail(QStringLiteral("运动副 %1 是旧裸 Gear 类型，原格式缺少两侧齿轮轴心定义。请使用 couplings 中引用两个旋转副的 Gear 齿轮耦合；不能假设两个轴心重合。").arg(id));
            auto found=types.find(type);
            if(found==types.end())fail(QStringLiteral("运动副 %1 的类型 %2 在引擎中没有有效约束实现；没有忽略该运动副。").arg(id).arg(type));
            std::shared_ptr<motion::adams_joint_struct> joint;
            Vec axisI=unit(o.value("axis")),axisJ=axisI;
            if(type=="Universal"||type=="Hooke"){
                auto u=std::make_shared<motion::adams_universal_joint_struct>();
                u->axial_I=unit(o.value("actionSpin"));u->axial_J=unit(o.value("baseSpin"));u->has_axial_I=u->has_axial_J=true;
                axisI=unit(o.value("actionPin"),motion::VECT_X);axisJ=unit(o.value("basePin"),motion::VECT_Y);joint=u;
            }else if(type=="Screw"){
                auto screw=std::make_shared<motion::adams_screw_joint_struct>();screw->pitch=o.value("lead").toDouble();screw->has_pitch=true;joint=screw;
            }else joint=std::make_shared<motion::adams_joint_struct>();
            if(type=="Planar")axisJ=-axisI;
            if(type=="Perpendicular"||type=="ConstantVelocity"){
                axisI=unit(o.value("actionVector"));axisJ=unit(o.value("baseVector"),type=="Perpendicular"?motion::VECT_X:motion::VECT_Z);
                if(type=="ConstantVelocity"&&axisI.Dot(axisJ)<0)axisJ=-axisJ;
            }
            const auto origin=vector(o.value("origin"));
            joint->id=key(id);joint->type=found->second;
            frictionLimit(o,*joint);
            const int actionBody = o.value("actionBodyId").toInt(-1), baseBody = o.value("baseBodyId").toInt(-1);
            const bool importedAction = importedDefinitions.count(actionBody), importedBase = importedDefinitions.count(baseBody);
            if ((importedAction || importedBase) && (type!="Revolute" || joint->has_friction || joint->has_limit || (importedAction && importedBase)))
                fail(QStringLiteral("运动副 %1 需要是 MNF 柔性体与刚体或地面之间无摩擦、无限位的转动副。").arg(id));
            const auto mnfNode = [&](int body) {
                const auto binding = importedDefinitions.find(body);
                if (binding == importedDefinitions.end()) return -1;
                const int node = binding->second.value("jointBindings").toObject().value(QString::number(id)).toInt(-1);
                if (node < 0) fail(QStringLiteral("运动副 %1 缺少 MNF 接口节点绑定。").arg(id));
                return node;
            };
            joint->marker_I=marker(actionBody,origin,axisI,{},mnfNode(actionBody));
            joint->marker_J=marker(baseBody,origin,axisJ,{},mnfNode(baseBody));
            const auto geo=in.geometry->joints.find(id);
            if(geo!=in.geometry->joints.end()){
                const auto& g=geo->second;
                if(type=="PointOnSurface"){
                    joint->has_surface=true;joint->surface_nodes=g.surface.nodes;joint->surface_elems=g.surface.triangles;
                    joint->surface_boundary_edges=g.surface.boundaryEdges;joint->surface_boundary_inward=g.surface.boundaryInward;
                }else if(type=="PointOnCurve"){
                    joint->has_curve=true;joint->curve_nodes=g.curveJ.nodes;
                }else if(type=="CurveOnCurve"){
                    joint->has_curve_pair=true;joint->curve_nodes_I=g.curveI.nodes;joint->curve_nodes_J=g.curveJ.nodes;
                    joint->curve_samples_I=g.curveI.samples;joint->curve_samples_J=g.curveJ.samples;
                    joint->curve_I_closed=g.curveI.closed;joint->curve_J_closed=g.curveJ.closed;joint->allow_sliding=o.value("allowSliding").toBool();
                }
            }
            s.joints_map.emplace(key(id),std::move(joint));
        }
    }
    void couplings()
    {
        for(const auto& value:in.document.collection("couplings")){
            const auto o=value.toObject();const int id=o.value("id").toInt();
            const auto a=s.joints_map.find(key(o.value("firstJointId").toInt(-1))),b=s.joints_map.find(key(o.value("secondJointId").toInt(-1)));
            if(a==s.joints_map.end()||b==s.joints_map.end())fail(QStringLiteral("耦合副 %1 缺少目标运动副。").arg(id));
            if (usesImportedMnf(*a->second) || usesImportedMnf(*b->second))
                fail(QStringLiteral("耦合副 %1 不能引用 MNF 柔性体的转动副。").arg(id));
            const auto type=o.value("couplingType").toString();std::shared_ptr<motion::adams_joint_struct> coupling;
            if(type=="GearRack"){
                auto p=std::make_shared<motion::adams_rackpin_joint_struct>();p->R=o.value("radius").toDouble();p->has_R=true;
                p->alpha=o.value("pressureAngleDeg").toDouble(0.0)*d2r;p->beta=o.value("helixAngleDeg").toDouble(0.0)*d2r;
                p->phase=o.value("phaseDeg").toDouble(0.0)*d2r;p->checkphase=o.value("checkPhase").toBool(false);
                p->type="RACKPIN";coupling=p;
            }else if(type=="Gear"){
                auto p=std::make_shared<motion::adams_gear_joint_struct>();const double ratio=o.value("gearRatio").toDouble(1);
                p->tau=std::abs(ratio);p->has_tau=true;p->epicyclic=ratio<0;
                p->alpha=o.value("pressureAngleDeg").toDouble(0.0)*d2r;p->beta=o.value("helixAngleDeg").toDouble(0.0)*d2r;
                p->phase=o.value("phaseDeg").toDouble(0.0)*d2r;p->checkphase=o.value("checkPhase").toBool(false);
                p->z1=o.value("toothCount1").toDouble();p->z2=o.value("toothCount2").toDouble();p->r1=o.value("pitchRadius1").toDouble();p->r2=o.value("pitchRadius2").toDouble();
                p->has_z1=p->z1>0;p->has_z2=p->z2>0;p->has_r1=p->r1>0;p->has_r2=p->r2>0;p->type="GEAR";coupling=p;
            }else if(type=="Cable"){
                auto p=std::make_shared<motion::adams_cable_joint_struct>();p->joint_1=a->first;p->joint_2=b->first;p->ratio=o.value("ratio").toDouble(1);p->reverse=o.value("reverse").toBool();p->type="CABLE";coupling=p;
            }else fail("未知耦合副类型："+type);
            coupling->id="CP_"+key(id);coupling->marker_I=a->second->marker_I;coupling->marker_J=b->second->marker_I;
            s.joints_map.emplace(coupling->id,coupling);
        }
    }
    void drives()
    {
        std::set<std::pair<int,bool>> controlled;
        for(const auto& value:in.document.collection("drives")){
            const auto o=value.toObject();const int id=o.value("id").toInt(),target=o.value("targetJointId").toInt(-1);
            const auto joint=s.joints_map.find(key(target));
            if(joint==s.joints_map.end())fail(QStringLiteral("驱动 %1 引用的运动副不存在。").arg(id));
            if (usesImportedMnf(*joint->second))
                fail(QStringLiteral("驱动 %1 不能施加在 MNF 柔性体的转动副上。").arg(id));
            if(joint->second->type!="REVOLUTE"&&joint->second->type!="TRANSLATIONAL"&&joint->second->type!="CYLINDRICAL"&&joint->second->type!="SCREW")fail(QStringLiteral("驱动 %1 的目标副不可驱动。").arg(id));
            motion::adams_motion_struct d;d.id=key(id);d.marker_I=joint->second->marker_I;d.marker_J=joint->second->marker_J;d.resolved_joint_id=joint->first;
            d.is_rotation=o.value("motionType").toString()=="Rotation";d.is_translation=!d.is_rotation;
            if((joint->second->type=="REVOLUTE"&&!d.is_rotation)||(joint->second->type=="TRANSLATIONAL"&&d.is_rotation))
                fail(QStringLiteral("驱动 %1 的方向不属于目标运动副的自由度。").arg(id));
            const auto mode=o.value("controlMode").toString("Position");d.is_speed=mode=="Velocity";d.is_position=mode=="Position";
            const bool effort=mode=="Force"||mode=="Torque";
            if(!d.is_speed&&!d.is_position&&!effort)fail(QStringLiteral("驱动 %1 的加速度模式没有引擎接口（原求解线程也不支持）。").arg(id));
            if(!controlled.emplace(target,d.is_rotation).second)fail("同一自由度不能被两个驱动重复控制。");
            if(effort)continue; // Actual force motors are created after parser assembly.
            const double scale=d.is_rotation?d2r:1;
            d.x_table=array(o.value("driveTableX"));d.f_table=array(o.value("driveTableY"),scale);d.has_table=!d.x_table.empty()||!d.f_table.empty();
            bool number=false;const QString expression=o.value("expression").toString("0");const double constant=expression.toDouble(&number);
            if(number)d.func_value=constant*scale;else d.function_expr=formula(expression,scale);
            s.motions_map.emplace(d.id,std::move(d));
        }
    }
    void markers()
    {
        for(const auto& value:in.document.collection("markers")){
            const auto o=value.toObject();const auto q=o.value("localRotation").toObject();
            QQuaternion rotation(q.value("w").toDouble(1),q.value("x").toDouble(),q.value("y").toDouble(),q.value("z").toDouble());
            if(rotation.lengthSquared()<1e-20)fail("Marker 四元数不能为零。");
            const auto z=rotation.normalized().rotatedVector(QVector3D(0,0,1));
            marker(o.value("bodyId").toInt(-1),vector(o.value("localPosition")),Vec(z.x(),z.y(),z.z()),"UM_"+key(o.value("id").toInt()));
        }
    }
    void springs()
    {
        for(const auto& value:in.document.collection("springs")){
            const auto o=value.toObject();if(!active(o))continue;
            motion::ADMSpring spring;spring.id=key(o.value("id").toInt());
            const auto type=o.value("springType").toString();const bool torsion=type=="Torsional";
            if(type=="BodyPoint"){
                const auto ends=endpoints(o);spring.marker_I=ends.first;spring.marker_J=ends.second;spring.dof_type="MARKER";
            }else{
                const auto joint=s.joints_map.find(key(o.value("jointId").toInt(-1)));
                if(joint==s.joints_map.end())fail("弹簧引用不存在的运动副。");
                if (usesImportedMnf(*joint->second)) fail("弹簧不能施加在 MNF 柔性体的转动副上。");
                if(torsion){spring.marker_I=joint->second->marker_I;spring.marker_J=joint->second->marker_J;spring.dof_type="TORSION";}
                else{
                    // The generic AXIAL helper in Parser uses marker local X;
                    // the UI joint direction is its global Z axis. Specify the
                    // actual endpoints so a spring follows its selected joint.
                    const auto definition=in.document.entity("joints",o.value("jointId").toInt(-1));
                    const auto origin=vector(definition.value("origin"));
                    const auto axis=unit(definition.value("axis"),unit(definition.value("baseSpin")));
                    spring.marker_I=marker(definition.value("actionBodyId").toInt(-1),origin,axis);
                    spring.marker_J=marker(definition.value("baseBodyId").toInt(-1),origin+axis*o.value("installValue").toDouble(),axis);
                    spring.dof_type="MARKER";
                }
            }
            spring.k=o.value("stiffness").toDouble()*1000*(torsion?r2d:1);
            spring.c_linear=o.value("dampingEnabled").toBool(true)?o.value("damping").toDouble()*1000*(torsion?r2d:1):0;
            spring.free_length=o.value("freeValue").toDouble()*(torsion?d2r:1);
            const double preload=o.value("preload").toDouble()*1000;
            if(std::abs(spring.k)>1e-12)spring.free_length+=preload/spring.k;
            else if(std::abs(preload)>1e-12)fail("零刚度弹簧的预载不能折算；请使用显式力函数。");
            spring.install_length=o.value("installValue").toDouble()*(torsion?d2r:1);
            spring.x_table=array(o.value("tableX"),torsion?d2r:1);spring.f_table=array(o.value("tableY"),1000);
            spring.has_table=!spring.x_table.empty()||!spring.f_table.empty();
            auto springFormula=[&](QString expression){
                if(torsion)expression.replace(QRegularExpression(QStringLiteral("\\b([xv])\\b")),QStringLiteral("(\\1*57.29577951308232)"));
                return formula(expression,1000);
            };
            spring.force_function=springFormula(o.value("forceFunction").toString());
            if(o.value("dampingEnabled").toBool(true))spring.damping_function=springFormula(o.value("dampingFunction").toString());
            spring.has_function=!spring.force_function.empty();spring.model_type=spring.has_table?"TABLE":spring.has_function?"FUNCTION":"LINEAR";
            // Original solver admits direction settings but never forwards them.
            // Do not pretend an unused one-sided flag changes the spring law.
            const auto direction=o.value("springDirectionMode").toString("Bidirectional");
            const auto dampingDirection=o.value("dampingDirectionMode").toString("Bidirectional");
            if(direction!="Bidirectional"||dampingDirection!="Bidirectional")fail("原弹簧的单向选项没有引擎容器语义；只拉连接请使用独立只拉绳索，止挡请使用独立止挡。");
            s.springs_map.emplace(spring.id,std::move(spring));
        }
    }
    void forces()
    {
        for(const auto& value:in.document.collection("scalarForces")){
            const auto o=value.toObject();if(!active(o))continue;const auto p=endpoints(o);
            motion::ADMScalarForce f;f.id=key(o.value("id").toInt());f.marker_I=p.first;f.marker_J=p.second;
            f.direction_mode=o.value("directionMode").toInt();f.direction=vector(o.value("fixedDirection"),motion::VECT_Z);
            f.magnitude=o.value("magnitude").toDouble()*1000;f.force_function=formula(o.value("forceFunction").toString(),1000);f.action_only=o.value("actionOnly").toBool();s.scalar_forces_map.emplace(f.id,f);
        }
        for(const auto& value:in.document.collection("scalarTorques")){
            const auto o=value.toObject();if(!active(o))continue;const auto p=endpoints(o);
            motion::ADMScalarTorque f;f.id=key(o.value("id").toInt());f.marker_I=p.first;f.marker_J=p.second;
            f.axis_mode=o.value("axisMode").toInt();f.axis=vector(o.value("fixedAxis"),motion::VECT_Z);
            f.magnitude=o.value("magnitude").toDouble()*1000;f.torque_function=formula(o.value("torqueFunction").toString(),1000);f.action_only=o.value("actionOnly").toBool();s.scalar_torques_map.emplace(f.id,f);
        }
        for(const auto& value:in.document.collection("threePointForces")){
            const auto o=value.toObject();if(!active(o))continue;motion::ADMThreePointForce f;f.id=key(o.value("id").toInt());
            f.marker_I=marker(o.value("bodyIId").toInt(-1),vector(o.value("localI")));
            f.marker_origin=marker(o.value("originBodyId").toInt(-1),vector(o.value("localOrigin")));
            f.marker_target=marker(o.value("targetBodyId").toInt(-1),vector(o.value("localTarget")));
            f.magnitude=o.value("magnitude").toDouble()*1000;f.force_function=formula(o.value("forceFunction").toString(),1000);s.three_point_forces_map.emplace(f.id,f);
        }
        for(const auto& value:in.document.collection("threePointTorques")){
            const auto o=value.toObject();if(!active(o))continue;motion::ADMThreePointTorque f;f.id=key(o.value("id").toInt());
            f.marker_I=marker(o.value("bodyIId").toInt(-1),vector(o.value("localI")));
            f.marker_origin=marker(o.value("originBodyId").toInt(-1),vector(o.value("localOrigin")));
            f.marker_target=marker(o.value("targetBodyId").toInt(-1),vector(o.value("localTarget")));
            f.magnitude=o.value("magnitude").toDouble()*1000;f.torque_function=formula(o.value("torqueFunction").toString(),1000);s.three_point_torques_map.emplace(f.id,f);
        }
        for(const auto& value:in.document.collection("vectorForces")){
            const auto o=value.toObject();if(!active(o))continue;const auto type=o.value("loadType").toString("Force");
            const int body=o.value("bodyId").toInt(-1);const auto point=vector(o.value("actionPoint")),axis=unit(o.value("direction"));
            const auto m=marker(body,point,axis);const auto id=key(o.value("id").toInt());
            if(type=="Bushing"){
                motion::adams_bushing_struct b{};b.id=id;b.marker_I=m;b.marker_J=marker(o.value("baseBodyId").toInt(-1),point,axis);
                const auto k=array(o.value("stiffness"),1000),c=array(o.value("damping"),1000),f=array(o.value("preloadForce"),1000),t=array(o.value("preloadTorque"),1000);
                for(int i=0;i<6;++i){b.K[i]=i<(int)k.size()?k[i]:0;b.C[i]=i<(int)c.size()?c[i]:0;}
                for(int i=0;i<3;++i){b.F0[i]=i<(int)f.size()?f[i]:0;b.T0[i]=i<(int)t.size()?t[i]:0;}
                b.has_K=!k.empty();b.has_C=!c.empty();b.has_F0=!f.empty();b.has_T0=!t.empty();b.has_D0=o.value("applyOffset").toBool();fill(b.D0,vector(o.value("offset")));s.bushings_map.emplace(id,b);
            }else if(type=="Force"||type=="Torque"){
                const bool constant=o.value("magnitudeMode").toString("Constant")=="Constant";
                const auto expr=constant?std::string():formula(o.value("expression").toString(),1000);
                if(type=="Force"){motion::ADMForce f;f.id=id;f.ref_marker=m;f.direction=axis;f.magnitude=constant?o.value("magnitude").toDouble()*1000:0;f.force_function=expr;s.forces_map.emplace(id,f);}
                else{motion::ADMTorque f;f.id=id;f.ref_marker=m;f.direction=axis;f.magnitude=constant?o.value("magnitude").toDouble()*1000:0;f.force_function=expr;s.torques_map.emplace(id,f);}
            }else fail(QStringLiteral("力元 %1 的类型 %2 没有实际引擎映射，不能按力矩冒充求解。").arg(QString::fromStdString(id),type));
        }
    }
    void contacts()
    {
        for(const auto& value:in.document.collection("contacts")){
            const auto o=value.toObject();if(!active(o)||o.contains("analyticContact"))continue;motion::adams_contact_struct c;
            const int a=o.value("actionBodyId").toInt(-1),b=o.value("baseBodyId").toInt(-1);
            if (importedDefinitions.count(a) || importedDefinitions.count(b)) fail("导入的 MNF 柔性体暂不支持接触。");
            c.id=key(o.value("id").toInt());c.body_I=bodyKey(a);c.body_J=bodyKey(b);
            c.is_static_I=s.parts_map.at(c.body_I).fixed;c.is_static_J=s.parts_map.at(c.body_J).fixed;
            c.is_convex_I=o.value("useConvexHullAction").toBool();c.is_convex_J=o.value("useConvexHullBase").toBool();
            motion::utils::ApplyContactFrictionSettings(c,o.value("frictionEnabled").toBool(),o.value("staticFriction").toDouble(.3),o.value("dynamicFriction").toDouble(.3));
            // Original mMotionSolverThread treats an unset/zero full-damping
            // depth as 0.0254 mm. This is a damping ramp, not a penetration cap.
            const double depth=o.value("penetrationDepth").toDouble(.0254);
            motion::utils::ApplyImpactContactSettings(c,o.value("stiffness").toDouble()*1000,o.value("materialDamping").toDouble()*1000,o.value("forceExponent").toDouble(1.5),depth>0?depth:.0254);
            const auto& ma=in.geometry->bodyMeshes.at(a);const auto& mb=in.geometry->bodyMeshes.at(b);
            c.nodes_I=ma.nodes;c.elems_I=ma.triangles;c.nodes_J=mb.nodes;c.elems_J=mb.triangles;c.has_nodes_I=c.has_elems_I=c.has_nodes_J=c.has_elems_J=true;s.contacts_map.emplace(c.id,c);
        }
    }
    void unilateral()
    {
        for(const auto& value:in.document.collection("tensionCables")){
            const auto o=value.toObject();if(!active(o))continue;const auto p=endpoints(o);motion::ADMTensionCable cable;
            cable.id=key(o.value("id").toInt());cable.marker_I=p.first;cable.marker_J=p.second;
            cable.free_length=o.value("freeLength").toDouble();cable.stiffness=o.value("stiffness").toDouble()*1000;
            cable.damping=o.value("damping").toDouble()*1000;cable.engagement_width=o.value("engagementWidth").toDouble();s.tension_cables_map.emplace(cable.id,cable);
        }
        for(const auto& value:in.document.collection("softStops")){
            const auto o=value.toObject();if(!active(o))continue;const auto p=endpoints(o);motion::ADMSoftStop stop;
            stop.id=key(o.value("id").toInt());stop.marker_I=p.first;stop.marker_J=p.second;
            stop.direction=unit(o.value("direction"),motion::VECT_X);stop.direction_mode=o.value("directionMode").toInt(1);stop.side=o.value("side").toInt(1);
            stop.stop_position=o.value("stopPosition").toDouble();stop.stiffness=o.value("stiffness").toDouble()*1000;stop.exponent=o.value("exponent").toDouble(1.5);
            stop.damping=o.value("damping").toDouble()*1000;stop.damping_depth=o.value("dampingDepth").toDouble();
            stop.x_table=array(o.value("tableX"));stop.f_table=array(o.value("tableY"),1000);stop.has_table=o.value("hasTable").toBool(!stop.x_table.empty()||!stop.f_table.empty());s.soft_stops_map.emplace(stop.id,stop);
        }
    }
};

void recordNestedBodies(NativeMotionSystem& system)
{
    recordNativeEffortMotors(system);
    recordNativeHydraulics(system);
    // Modal assemblies take ownership of their interface bodies. The engine's
    // standard recorder visits top-level bodies only, so record these same real
    // body/marker states here without duplicating top-level samples.
    const double time=system.GetMotionTime();
    for(const auto& item:system.nativeBodies){
        auto& history=system.body_results[key(item.first)];
        if(history.data.empty()||history.data.back().time<time){
            motion::body_result value{};value.time=time;system.FillKinematicsFromFrame(*item.second,value.cm);
            const auto delta=item.second->GetPos()-item.second->GetOriginPos();value.cm.dpos={delta.x(),delta.y(),delta.z()};
            value.kinetic_energy=item.second->GetKineticEnergy();value.potential_energy=-item.second->GetMass()*system.Get_G_acc().Dot(item.second->GetPos());
            history.body_id=key(item.first);history.data.push_back(value);
        }
        for(const auto& marker:item.second->GetMarkerList()){
            const auto name=marker->GetNameString();if(name.rfind("UM_",0)!=0)continue;
            auto& mh=system.marker_results[name];if(!mh.data.empty()&&mh.data.back().time>=time)continue;
            motion::marker_result value{};value.time=time;system.FillKinematicsFromMarker(marker.get(),value.kinematic);mh.marker_id=name;mh.data.push_back(value);
        }
    }
    for(const auto& item:system.nativeMotors){
        const auto definition=system.motions_map.find(key(item.first));
        if(definition==system.motions_map.end()||!definition->second.is_speed)continue;
        auto& history=system.motion_results[key(item.first)];if(history.data.empty())continue;
        auto& sample=history.data.back();
        // Speed motors do not fill imposed_value in the engine recorder. Their
        // integrated physical coordinate is available directly on the motor.
        if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(item.second))sample.imposed_value=motor->GetMotorRot();
        else if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(item.second))sample.imposed_value=motor->GetMotorPos();
    }
    for(const auto& item:system.nativeJoints){
        auto& history=system.joint_results[item.first];
        if(!history.data.empty()&&history.data.back().time>=time)continue;
        const auto definition=system.joints_map.at(item.first);
        const auto findMarker=[&](const std::string& id){
            auto marker=system.SearchMarker(id.c_str());
            if(!marker)for(const auto& body:system.nativeBodies)for(const auto& candidate:body.second->GetMarkerList())if(candidate->GetNameString()==id)return candidate;
            return marker;
        };
        const auto a=findMarker(definition->marker_I),b=findMarker(definition->marker_J);
        if(!a||!b)fail("几何运动副结果的 Marker 不存在。");
        motion::joint_result value{};value.time=time;value.marker_I=definition->marker_I;value.marker_J=definition->marker_J;
        system.FillKinematicsFromMarker(a.get(),value.node_I);system.FillKinematicsFromMarker(b.get(),value.node_J);
        if(const auto link=std::dynamic_pointer_cast<motion::MotionLink>(item.second)){
            const auto rotation=link->GetLinkAbsoluteCoords().rot;
            const auto force=rotation.Rotate(link->Get_react_force()),torque=rotation.Rotate(link->Get_react_torque());
            value.reaction_force={force.x(),force.y(),force.z()};value.reaction_torque={torque.x(),torque.y(),torque.z()};
        }
        history.id=item.first;history.type=definition->type;history.data.push_back(value);
    }
}

void assembleInitialState(NativeMotionSystem& system)
{
    system.DoAssembly(motion::AssemblyLevel::POSITION);
    motion::MotionState position;
    motion::MotionStateDelta compatible;
    double time=0;
    system.projectConstraintVelocity(position,compatible,time);
    if (system.hasNativeHydraulics()) {
        motion::MotionStateDelta acc(system.GetNcoords_v(), &system);
        motion::MotionVectorDynamic<> reactions(system.GetNconstr());
        acc.setZero(system.GetNcoords_v(), &system);
        reactions.setZero();
        if (system.StateSolveA(acc, reactions, position, compatible, time, 1e-6, false, true)) {
            system.StateScatter(position, compatible, time, true);
            system.StateScatterAcceleration(acc);
            system.StateScatterReactions(reactions);
            return;
        }
    }
    // The engine estimates acceleration with a 1e-6 s virtual dynamics step.
    // Keep its acceleration/reaction estimate, restore the accepted t=0 state.
    system.DoAssembly(motion::AssemblyLevel::ACCELERATION);
    system.StateScatter(position,compatible,time,true);
}

} // namespace

std::shared_ptr<motion::MotionSolver> makeNativeEquilibratedQr()
{
    return std::make_shared<EquilibratedMotionQr>();
}

void rejectNativeGeometryBoundary(const NativeMotionSystem& system)
{
    for(const auto& item:system.nativeJoints){
        double distance=0;QString kind;
        if(const auto joint=std::dynamic_pointer_cast<motion::MotionLinkPointOnSurface>(item.second)){
            if(joint->IsOutOfFace()){distance=joint->GetOutOfFaceDistance();kind=QStringLiteral("点在面上");}
        }else if(const auto joint=std::dynamic_pointer_cast<motion::MotionLinkPointSpline>(item.second)){
            if(joint->IsOutOfCurve()){distance=joint->GetOutOfCurveDistance();kind=QStringLiteral("点在线上");}
        }
        if(!kind.isEmpty())throw NativeGeometryBoundary{QString::fromStdString(item.first),kind,system.GetMotionTime(),distance};
    }
}

SolveStatus solveNativeMotion(const NativeMotionInput& input,RigidResult& result,QString& error,const SolverCallbacks& callbacks)
{
    try{
        if(!input.geometry)fail("原生模型尚未准备几何输入。");
        if(callbacks.stopRequested&&callbacks.stopRequested()){error.clear();return SolveStatus::Stopped;}
        NativeMnfSystem system;
        system.sourceInput=&input;
        const auto scenario=input.document.entity("scenarios",input.scenario.id);
        auto config=configuration(scenario);
        Assembly assembly(system,input,config);
        auto importedFlex=assembly.build();
        const bool hasMnf=!importedFlex.empty();
        system.linearizedMnf=hasMnf&&scenario.value("mnfIntegratorType").toString()=="EULER_IMPLICIT_LINEARIZED";
        const auto analysisType=scenario.value("analysisType").toString("Dynamic");
        if (hasMnf && analysisType!="Dynamic")
            fail("导入的 MNF 柔性体当前支持动力学方案。");
        motion::utils::MotionParserAdams parser;parser.SetVisualizationType(motion::utils::MotionParserAdams::NONE);
#if defined(MX_HAS_BODY_DRIVES)
        parser.Parse(system, bodyDriveDefinitions(input.document));
#else
        for (const auto& value : input.document.collection("bodyDrives")) {
            if (active(value.toObject())) fail(QStringLiteral("当前 MotionEngine 版本不支持运动体驱动。"));
        }
        parser.Parse(system);
#endif
        createNativeEffortMotors(system,input);
        createNativeHydraulicCylinders(system,input);
        createNativeHydraulicMotors(system,input);
        createNativeHydraulicCircuit(system,input);
        createNativeElectricAndIntegratedActuators(system,input);
        for(const auto& joint:system.joints_map){
            bool created=false;for(const auto& link:system.Get_linklist())if(!std::dynamic_pointer_cast<motion::MotionLinkMotor>(link)&&link->GetNameString()==joint.first){created=true;system.nativeJoints.emplace(joint.first,link);break;}
            if(!created)fail(QStringLiteral("引擎没有创建运动副 %1 (%2)，已中止，避免约束被静默丢弃。").arg(QString::fromStdString(joint.first),QString::fromStdString(joint.second->type)));
        }
        for(const auto& item:system.nativeJoints){
            // Match result acceptance to the geometry's 0.02 mm sampling budget
            // plus numerical margin. The engine's default 0.5 mm would retain
            // several visibly out-of-domain frames at small output steps.
            if(const auto link=std::dynamic_pointer_cast<motion::MotionLinkPointOnSurface>(item.second))
                link->SetOutOfFaceTolerance(NativeGeometryBoundaryToleranceMm);
            if(const auto link=std::dynamic_pointer_cast<motion::MotionLinkPointSpline>(item.second))
                link->SetOutOfCurveTolerance(NativeGeometryBoundaryToleranceMm);
        }
        assembly.configureRuntimeDirections();
        for(const auto& body:input.bodies){
            const auto name=key(body.body.id);
            const auto native=system.importedMnfBodyIds.count(body.body.id)?system.SearchFlex(name.c_str()):system.SearchBody(name.c_str());
            if(!native)fail("引擎未创建声明的运动体。");system.nativeBodies.emplace(body.body.id,native);
        }
        for(const auto& link:system.Get_linklist())if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotor>(link)){
            bool numeric=false;const int id=QString::fromStdString(motor->GetNameString()).toInt(&numeric);
            if(numeric)system.nativeMotors.emplace(id,motor);
        }
        system.Set_G_acc(Vec(input.scenario.gravityMS2[0]*1000,input.scenario.gravityMS2[1]*1000,input.scenario.gravityMS2[2]*1000));
        if (hasMnf) {
            // 旧工程继续使用 HHT。运动方案可以显式选用面向闭环机构的线性化积分。
            config.use_preset=false;
            config.integrator.type=system.linearizedMnf ? motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED
                                                       : motion::MotionTimestepper::Type::HHT;
            config.solver.type=motion::MotionSolver::Type::SPARSE_QR;
        } else if(!input.geometry->flexBodies.empty()){
            config.use_preset=false;config.integrator.type=motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED;
            const bool independent=std::any_of(input.geometry->flexBodies.begin(),input.geometry->flexBodies.end(),[](const NativeFlexGeometry& body){return body.independentInterfaces;});
            system.weightIndependentFlexProjectionWithStiffness=independent &&
                scenario.value("independentPositionProjection").toString()==QStringLiteral("MASS_STIFFNESS");
            config.solver.type=independent?motion::MotionSolver::Type::SPARSE_QR:motion::MotionSolver::Type::MINRES;
            config.solver.max_iters=std::max(500,config.solver.max_iters);system.SetPositionAssemblyPerStep(true);
        } else if (system.hasNativeHydraulics()) {
            if (config.use_preset) {
                config.use_preset = false;
                config.integrator.type = motion::MotionTimestepper::Type::HHT;
                config.integrator.alpha = -0.05;
                config.solver.type = motion::MotionSolver::Type::SPARSE_QR;
            } else {
                if (config.integrator.type == motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED
                    || config.integrator.type == motion::MotionTimestepper::Type::EULER_IMPLICIT_PROJECTED)
                    config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT;
                if (config.solver.type != motion::MotionSolver::Type::SPARSE_QR
                    && config.solver.type != motion::MotionSolver::Type::SPARSE_LU
                    && config.solver.type != motion::MotionSolver::Type::GMRES)
                    config.solver.type = motion::MotionSolver::Type::SPARSE_QR;
            }
        }
        system.InitSolver(config);
        const auto contactHistory=std::make_shared<AnalyticContactHistory>();
        const bool analyticContacts=contactHistory->install(system,input.document);
        if (analyticContacts) system.GetCollisionSystem()->RegisterBroadphaseCallback(contactHistory);
        if(system.GetSolver()->GetType()!=system.config.solver.type||system.GetTimestepper()->GetType()!=system.config.integrator.type)
            fail("底层引擎没有安装方案指定的求解器或积分器，已中止；不能用默认类型代替所请求的算法。");
        const bool equilibratedQr = analysisType == "Dynamic" &&
            (system.linearizedMnf || (!hasMnf && useEquilibratedMotionQr(input, system, config)));
        if (equilibratedQr) {
            auto solver = makeNativeEquilibratedQr();
            solver->SetVerbose(config.solver.verbose);
            system.SetSolver(solver);
        }
        if (hasMnf && !system.linearizedMnf) {
            const auto hht = std::dynamic_pointer_cast<motion::MotionTimestepperHHT>(system.GetTimestepper());
            if (!hht) fail("MNF 动力学需要 HHT 积分器。");
            system.SetTimestepper(std::make_shared<ImportedMnfTimestepper>(*hht));
        }
        const auto chosen=system.config.solver.type;
        const bool linearSolver=chosen==motion::MotionSolver::Type::SPARSE_LU||chosen==motion::MotionSolver::Type::SPARSE_QR||chosen==motion::MotionSolver::Type::GMRES||chosen==motion::MotionSolver::Type::MINRES||chosen==motion::MotionSolver::Type::BICGSTAB;
        if(linearSolver)for(const auto& joint:system.joints_map){
            const auto& j=*joint.second;
            if((j.has_limit&&!j.limit_penalty_only&&(j.lower_limit_enabled||j.upper_limit_enabled))||j.use_stiction_constraint)
                fail(QStringLiteral("运动副 %1 含硬单边限位/静摩擦约束，当前线性求解器不支持该约束。请选择 PSOR 等投影求解器，或明确改用纯软限位。").arg(QString::fromStdString(joint.first)));
        }
        auto flex=buildNativeFlex(system,input,callbacks);
        for (auto& imported : importedFlex) flex.push_back(std::move(imported));
        if(callbacks.stopRequested&&callbacks.stopRequested()){error.clear();return SolveStatus::Stopped;}
        system.SetMaxiter(50);
        const bool staticOnly=analysisType=="Static",preload=staticOnly||analysisType=="PreloadedDynamics";
        if(staticOnly)for(const auto& value:input.document.collection("controllers"))
            if(value.toObject().value("enabled").toBool(true))fail("静力方案不能运行离散 PID 反馈；请选择“预载后动力学”，在静力平衡后启动控制器。");
        assembleInitialState(system);
        std::optional<NativeStaticReport> staticReport;
        if(preload)staticReport=equilibrateNativeMotion(system,scenario,callbacks);
        const QString hydraulicSteadyReport=initializeSteadyNativeHydraulics(system,scenario);
        if(!staticOnly){
            system.configureControllers(input);
            if(!system.controllers.empty()||preload||!hydraulicSteadyReport.isEmpty())assembleInitialState(system);
        }
        const bool projectPositions=scenario.value("positionAssemblyPerStep").toBool(!system.joints_map.empty())||!flex.empty();
        // The engine's fixed-count assembly cannot detect whether a closed
        // mechanism has converged. The adapter performs the same mass-weighted
        // correction before PID/recording, stopping early on actual residuals.
        system.SetPositionAssemblyPerStep(false);
        system.projectPositions=projectPositions;
        system.allowFullRankPositionProjection = projectPositions && analysisType == "Dynamic" &&
            flex.empty() && config.use_preset && config.preset == motion::SimulationConfig::Preset::FAST &&
            input.document.collection(QStringLiteral("bodyDrives")).isEmpty() && isRigidPositionDriven(input, system);
        if (!hasMnf) {
            for (const auto& body : flex) {
                if (body.reducedDynamics && body.result.assembly && body.result.interface_bodies.size() == 1)
                    system.rigidInterfaceFlex.push_back(body.result.assembly.get());
            }
        }
        system.positionIterations=std::clamp(scenario.value("positionAssemblyIterations").toInt(50),1,50);
        validateNativeAcceptedConstraints(system);
        try { rejectNativeGeometryBoundary(system); }
        catch(const NativeGeometryBoundary& initial) {
            fail(QStringLiteral("%1副 %2 的初始装配状态已在有限几何之外（%3 mm）；没有可保存的有效初始帧，请核对作用点和约束几何。")
                .arg(initial.kind,initial.jointId).arg(initial.distanceMm,0,'g',10));
        }
        try { captureNativeFlex(flex,0.0); }
        catch(const NativeFlexDivergence& initial) {
            fail(QStringLiteral("柔性体 %1 的初始状态无效：%2；没有可保存的有效初始帧。")
                .arg(initial.flexId).arg(initial.reason));
        }
        system.RecordStepResults();recordNestedBodies(system);
        if (analyticContacts) contactHistory->sample(contactHistory,system);
        system.acceptedStepSize=config.step_size;
        system.acceptedEndTime=config.total_time;
        system.validateFlexState=[&](double time){validateNativeFlex(flex,time);};
        system.captureFlexState=[&](double time){captureNativeFlex(flex,time);};
        bool stopped=false;
        system.SetProgressCallback([&](double current,double,double progress){
            if(callbacks.stopRequested&&callbacks.stopRequested()){stopped=true;system.RequestStopSolve();return;}
            recordNestedBodies(system);
            if (analyticContacts) contactHistory->sample(contactHistory,system);
            if(callbacks.progress)callbacks.progress(std::clamp(progress,0.0,1.0));
        });
        std::optional<NativeGeometryBoundary> boundary;
        std::optional<NativeFlexDivergence> divergence;
        try { if(!staticOnly)system.StartSolver(config); }
        catch(const NativeGeometryBoundary& reached) { boundary=reached; }
        catch(const NativeFlexDivergence& rejected) { divergence=rejected; }
        if(stopped||(callbacks.stopRequested&&callbacks.stopRequested())){error.clear();return SolveStatus::Stopped;}
        RigidResult next;extractNativeResults(input,system,flex,next);
        if (analyticContacts) contactHistory->transfer(next);
        next.diagnostics=input.geometry->diagnostics;
        if (equilibratedQr) next.diagnostics.append(QStringLiteral("双边位置驱动使用平衡缩放稀疏 QR。"));
        if (system.usedFullRankPositionProjection)
            next.diagnostics.append(QStringLiteral("全约束刚体使用稀疏雅可比位置投影，按全部原约束验收。"));
        if(system.usedBilateralProjectionSolver)
            next.diagnostics.append(QStringLiteral("双边约束的位置投影使用平衡缩放稀疏 QR，动力学沿用方案指定算法。"));
        if (system.linearizedMnf) next.diagnostics.append(QStringLiteral("MNF 动力学使用线性化隐式欧拉、模态刚度加权位置投影和平衡缩放稀疏 QR。"));
        else if (hasMnf) next.diagnostics.append(QStringLiteral("MNF 动力学使用 HHT 自适应积分（内部步长上限 %1 ms）和稀疏 QR 求解。")
            .arg(1000 * ImportedMnfTimestepper::maxStepSeconds));
        if(staticReport)next.diagnostics.append(staticReport->description());
        if(!hydraulicSteadyReport.isEmpty())next.diagnostics.append(hydraulicSteadyReport);
        if(staticOnly){next.termination=ResultTermination::StaticEquilibrium;if(callbacks.progress)callbacks.progress(1);}
        if(next.frames.empty())fail("引擎未生成任何有效结果帧。");
        if(boundary){
            next.termination=ResultTermination::GeometryBoundary;
            next.diagnostics.append(QStringLiteral("%1副 %2 在试算 t=%3 s 时越过几何边界 %4 mm，该步未被接受；已保留截至 t=%5 s 的 %6 帧真实结果，可继续查看动画和曲线。")
                .arg(boundary->kind,boundary->jointId).arg(boundary->attemptedTime,0,'g',12).arg(boundary->distanceMm,0,'g',10)
                .arg(next.frames.back().seconds,0,'g',12).arg(next.frames.size()));
        }
        if(divergence){
            next.termination=ResultTermination::FlexDivergence;
            next.diagnostics.append(QStringLiteral("柔性体 %1 在试算 t=%2 s 时出现异常：%3；该步未被接受，已保留截至 t=%4 s 的 %5 帧有效结果，动画、曲线与模态数据保持同步。")
                .arg(divergence->flexId).arg(divergence->attemptedTime,0,'g',12).arg(divergence->reason)
                .arg(next.frames.back().seconds,0,'g',12).arg(next.frames.size()));
        }
        const double endTolerance=256*std::numeric_limits<double>::epsilon()*std::max(1.0,input.scenario.duration);
        if(next.termination==ResultTermination::EndTime&&std::abs(next.frames.back().seconds-input.scenario.duration)>endTolerance)
            fail(QStringLiteral("求解在 t=%1 s 提前结束，尚未达到方案终时 %2 s，且没有几何边界终止原因；未将不完整结果标为正常完成。")
                .arg(next.frames.back().seconds,0,'g',12).arg(input.scenario.duration,0,'g',12));
        result=std::move(next);error.clear();if(callbacks.progress)callbacks.progress(1);return SolveStatus::Completed;
    }catch(const NativeStaticCancelled&){error.clear();return SolveStatus::Stopped;}
     catch(const std::exception& e){error=QString::fromUtf8(e.what());}
     catch(...){error=QStringLiteral("原生运动求解失败（未知引擎异常）。");}
    return SolveStatus::Failed;
}

bool evaluateNativeConstraintSystem(const NativeMotionInput& input,
                                    NativeConstraintSummary& summary,
                                    QString& error)
{
    summary = {};
    try {
        if (!input.geometry) fail(QStringLiteral("原生模型尚未准备几何输入。"));
        NativeMnfSystem system;
        system.sourceInput = &input;
        const auto scenario = input.document.entity("scenarios", input.scenario.id);
        auto config = configuration(scenario);
        Assembly assembly(system, input, config);
        assembly.build();
        motion::utils::MotionParserAdams parser;
        parser.SetVisualizationType(motion::utils::MotionParserAdams::NONE);
#if defined(MX_HAS_BODY_DRIVES)
        parser.Parse(system, bodyDriveDefinitions(input.document));
#else
        parser.Parse(system);
#endif
        createNativeEffortMotors(system, input);
        for (const auto& joint : system.joints_map) {
            for (const auto& link : system.Get_linklist()) {
                if (!std::dynamic_pointer_cast<motion::MotionLinkMotor>(link) && link->GetNameString() == joint.first) {
                    system.nativeJoints.emplace(joint.first, link);
                    break;
                }
            }
        }
        SolverCallbacks silent;
        buildNativeFlex(system, input, silent, true);
        system.InitSolver(config);
        system.Setup();
        system.Update(0.0, false);
        system.prepareStaticDescriptor();
        system.ConstraintsLoadJacobians();

        summary.velocityDofs = system.GetNcoords_v();
        const int m = system.GetNconstr();
        summary.constraintEquations = m;

        motion::MotionVectorDynamic<> c0(m);
        c0.setZero();
        if (m > 0) {
            system.LoadConstraint_C(c0, 1.0, false);
            if (c0.allFinite()) summary.maxPositionViolation = c0.cwiseAbs().maxCoeff();
        }

        motion::MotionSparseMatrix jacobian;
        system.GetSystemDescriptor()->ConvertToMatrixForm(&jacobian, nullptr, nullptr, nullptr, nullptr, nullptr);
        if (jacobian.rows() > 0 && jacobian.cols() > 0 && jacobian.nonZeros() > 0) {
            Eigen::MatrixXd dense(jacobian);
            Eigen::ColPivHouseholderQR<Eigen::MatrixXd> qr(dense);
            qr.setThreshold(1e-9);
            summary.jacobianRank = static_cast<int>(qr.rank());
        } else {
            summary.jacobianRank = 0;
        }
        summary.redundantEquations = std::max(0, summary.constraintEquations - summary.jacobianRank);

        for (const auto& item : system.nativeJoints) {
            bool ok = false;
            const int jid = QString::fromStdString(item.first).toInt(&ok);
            if (!ok || !item.second || !item.second->IsActive()) continue;
            const auto cv = item.second->GetConstraintViolation();
            if (cv.size() > 0 && cv.allFinite()) {
                summary.jointViolations.push_back({jid, cv.cwiseAbs().maxCoeff()});
            }
        }
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = QString::fromUtf8(e.what());
    } catch (...) {
        error = QStringLiteral("约束雅可比分析发生未知异常。");
    }
    return false;
}

} // namespace MotionOcc
