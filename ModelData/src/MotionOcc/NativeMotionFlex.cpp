#include "NativeMotionRuntime.h"
#include "FemMesh.h"
#include "NativeMnfPreview.h"

#include "motion/fea/Mesh.h"
#include "motion/fea/NodeFEAxyz.h"
#include "motion/utils/Parsermnf.h"

#include <Eigen/Cholesky>

#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QElapsedTimer>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>

namespace MotionOcc {

NativeFlexRuntime buildImportedMnfFlex(NativeMotionSystem& system, const PreparedBody& target,
                                       const QJsonObject& definition, const QString& path,
                                       const motion::MotionFrame<>& referencePose,
                                       const motion::MotionVector<>& initialVelocity,
                                       const motion::MotionVector<>& initialAngularVelocity)
{
    const bool quadraticInvariants = definition.value("quadraticInvariants").toBool(false);
    motion::utils::MnfParseOptions options;
    options.include_all_nodes = true;
    options.include_surface_topology = true;
    options.include_quadratic_invariants = quadraticInvariants;
    auto data = motion::utils::MotionParserMnf().Parse(path.toUtf8().toStdString(), options);
    auto preview = makeMnfModePreview(data);
    auto& matrices = data.matrices;

    const auto& nodes = matrices.at("SELNOD").data;
    std::map<int, Eigen::Index> interfaceRows;
    for (Eigen::Index row = 0; row < nodes.rows(); ++row)
        if (nodes(row, 4) != 0)
            interfaceRows.emplace(preview.sourceNodeIds.at(static_cast<std::size_t>(row)), row);
    const auto bindings = definition.value("jointBindings").toObject();
    std::set<Eigen::Index> boundRows;
    for (auto binding = bindings.begin(); binding != bindings.end(); ++binding) {
        const int node = binding.value().toInt(-1);
        const auto found = interfaceRows.find(node);
        if (found == interfaceRows.end())
            throw std::runtime_error(QStringLiteral("MNF 运动副 %1 绑定的节点 %2 不是文件导出的接口节点。")
                .arg(binding.key()).arg(node).toStdString());
        boundRows.insert(found->second);
    }

    // Keep the file's modal coordinates, but express geometry and energetics
    // in mxmotion's kg, mm, s system. Its force unit is millinewton.
    const double massScale = data.units.si_mass;
    const double lengthScale = 1000 * data.units.si_length;
    const double inertiaScale = massScale * lengthScale * lengthScale;
    const double energyScale = 1000 * data.units.si_force * lengthScale;
    matrices.at("INVAR1").data *= massScale;
    matrices.at("INVAR2").data *= massScale * lengthScale;
    matrices.at("INVAR3").data *= massScale * lengthScale;
    for (const char* name : {"INVAR4", "INVAR6", "INVAR7", "INVAR8"})
        matrices.at(name).data *= inertiaScale;
    if (quadraticInvariants) {
        for (const char* name : {"INVAR5", "INVAR9"})
            if (matrices.count(name)) matrices.at(name).data *= inertiaScale;
    }
    matrices.at("GENSTIFF").data *= energyScale;

    // HHT updates the engine nodes repeatedly while evaluating the tangent.
    // Only the bound interface nodes need to be updated during integration.
    const auto modeCount = static_cast<Eigen::Index>(preview.modalBasis.frequenciesHz.size());
    const auto boundCount = static_cast<Eigen::Index>(boundRows.size());
    Eigen::MatrixXd selectedNodes(boundCount, nodes.cols());
    Eigen::MatrixXd translation(3, boundCount * modeCount), rotation(3, boundCount * modeCount);
    Eigen::Index outputRow = 0;
    for (const auto row : boundRows) {
        selectedNodes.row(outputRow) = nodes.row(row);
        translation.middleCols(outputRow * modeCount, modeCount) =
            matrices.at("T_MODE").data.middleCols(row * modeCount, modeCount) * lengthScale;
        rotation.middleCols(outputRow * modeCount, modeCount) =
            matrices.at("R_MODE").data.middleCols(row * modeCount, modeCount);
        ++outputRow;
    }
    selectedNodes.middleCols(1, 3) *= lengthScale;
    matrices.at("SELNOD").data = std::move(selectedNodes);
    matrices.at("T_MODE").data = std::move(translation);
    matrices.at("R_MODE").data = std::move(rotation);

    const double mass = matrices.at("INVAR1").data(0, 0);
    if (!(mass > 0) || !std::isfinite(mass))
        throw std::runtime_error("MNF 柔性体的质量无效。");
    const Eigen::Vector3d center = matrices.at("INVAR2").data.transpose() / mass;
    const Eigen::Matrix3d inertia = matrices.at("INVAR7").data - mass *
        (center.squaredNorm() * Eigen::Matrix3d::Identity() - center * center.transpose());
    if (!center.allFinite() || !inertia.allFinite() || !inertia.isApprox(inertia.transpose()) ||
        Eigen::LLT<Eigen::Matrix3d>(inertia).info() != Eigen::Success)
        throw std::runtime_error("MNF 柔性体关于质心的惯量无效。");

    auto flex = std::make_shared<motion::MotionFlexBodyAuxRef>();
    flex->setLevel(static_cast<int>(modeCount));
    flex->SetMass(mass);
    flex->SetInertia(motion::MotionMatrix33<>(inertia));
    flex->SetQuadraticInvariantsEnabled(quadraticInvariants);
    for (const auto& matrix : matrices)
        flex->set_matrix(matrix.first, matrix.second);
    const bool customDamping = definition.value("useCustomModalDamping").toBool(definition.contains("modalDampingRatio"));
    const double customRatio = definition.value("modalDampingRatio").toDouble(0.02);
    if (customDamping)
        flex->SetModalDampingRatio(customRatio);
    else
        flex->UseAdamsDefaultModalDamping();
    flex->SetNameString(std::to_string(target.body.id));
    flex->SetFrame_COG_to_REF(motion::MotionFrame<>(motion::MotionVector<>(center), motion::QUNIT));
    flex->SetFrame_REF_to_abs(referencePose);
    flex->SetOriginPos(flex->GetPos());
    flex->SetPos_dt(initialVelocity);
    flex->SetWvel_par(initialAngularVelocity);
    flex->SetCollide(false);
    system.AddFlex(flex);
    flex->Update(false);

    NativeFlexRuntime runtime;
    runtime.id = definition.value("id").toInt();
    runtime.bodyId = target.body.id;
    runtime.importedBody = std::move(flex);
    runtime.importedNodeIds = std::move(preview.sourceNodeIds);
    runtime.importedReferencePose = referencePose;
    runtime.result.surface_tris = std::move(preview.surfaceTriangles);
    runtime.modalBasis = std::move(preview.modalBasis);
    runtime.modalBasis.diagnostics.append(QStringLiteral("动力学使用%1与%2。")
        .arg(quadraticInvariants ? QStringLiteral("完整二阶惯性耦合 (INVAR1-9)") : QStringLiteral("部分惯性耦合"))
        .arg(customDamping ? QStringLiteral("常数模态阻尼比 %1").arg(customRatio, 0, 'g', 5)
                           : QStringLiteral("MotionEngine 默认模态阻尼")));
    runtime.result.n_modes = static_cast<int>(modeCount);
    const auto modes = static_cast<std::size_t>(modeCount);
    runtime.result.node_x0.reserve(preview.referenceNodes.size());
    for (std::size_t node = 0; node < runtime.importedNodeIds.size(); ++node) {
        const auto index = 3 * node;
        const auto position = referencePose.TransformPointLocalToParent(motion::MotionVector<>(
            preview.referenceNodes[index], preview.referenceNodes[index + 1], preview.referenceNodes[index + 2]));
        runtime.result.node_x0.insert(runtime.result.node_x0.end(), {position.x(), position.y(), position.z()});
        for (std::size_t mode = 0; mode < modes; ++mode) {
            auto& shapes = runtime.modalBasis.shapes;
            const auto column = index * modes + mode;
            const auto rotated = referencePose.TransformDirectionLocalToParent(
                motion::MotionVector<>(shapes[column], shapes[column + modes], shapes[column + 2 * modes]));
            for (std::size_t component = 0; component < 3; ++component)
                shapes[column + component * modes] = rotated[component];
        }
    }
    runtime.result.psi_internal = runtime.modalBasis.shapes;
    return runtime;
}

std::vector<NativeFlexRuntime> buildNativeFlex(NativeMotionSystem& system,const NativeMotionInput& input,
                                             const SolverCallbacks& callbacks,bool inspectionOnly)
{
    std::vector<NativeFlexRuntime> out;
    for(const auto& prepared:input.geometry->flexBodies){
        NativeFlexGeometry source=prepared;
        if(!source.sourceBrep.isEmpty()){
            if(!QFileInfo::exists(source.meshWorkerPath))throw std::runtime_error(QStringLiteral("自动体网格程序不存在：%1；请安装/构建 MxMotionMeshWorker，或提供 meshFile。").arg(source.meshWorkerPath).toStdString());
            QTemporaryDir directory;if(!directory.isValid())throw std::runtime_error("无法创建柔性体自动网格临时目录。");
            QFile brep(directory.filePath("source.brep"));if(!brep.open(QIODevice::WriteOnly)||brep.write(source.sourceBrep)!=source.sourceBrep.size())throw std::runtime_error("无法保存自动网格 CAD 快照。");brep.close();
            double size=source.definition.value("elementSize").toDouble();
            if(size<=0){
                // The worker requires a length. Estimate it from the original
                // binding's inertia scale, while preserving a user value.
                const auto found=std::find_if(input.bodies.begin(),input.bodies.end(),[&](const PreparedBody& b){return b.body.id==source.bodyId;});
                if(found==input.bodies.end())throw std::runtime_error("柔性体质量属性不存在。");
                const auto& p=found->properties;size=std::sqrt(std::max({p.inertiaKgM2[0],p.inertiaKgM2[1],p.inertiaKgM2[2]})*1e6/std::max(p.massKg,1e-12))*.3;
                if(!std::isfinite(size)||size<=0)throw std::runtime_error("请给自动柔性体指定正的 elementSize。");
            }
            QProcess process;process.setProgram(source.meshWorkerPath);process.setWorkingDirectory(directory.path());
            process.setArguments({"volume","source.brep","result.vol",QString::number(size,'g',16)});process.start();
            if(!process.waitForStarted(5000))throw std::runtime_error(QStringLiteral("无法启动自动体网格：%1").arg(process.errorString()).toStdString());
            QElapsedTimer elapsed;elapsed.start();
            while(!process.waitForFinished(250)){
                if(callbacks.stopRequested&&callbacks.stopRequested()){process.kill();process.waitForFinished(5000);return {};}
                if(elapsed.elapsed()>120000){process.kill();process.waitForFinished(5000);throw std::runtime_error("自动柔性体网格超过 120 秒；请增大单元尺寸。");}
            }
            if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0)throw std::runtime_error(QStringLiteral("自动体网格失败：%1").arg(QString::fromUtf8(process.readAllStandardError())).toStdString());
            FemMesh mesh;QString error;if(!readNetgenMesh(directory.filePath("result.vol"),MeshKind::Volume,mesh,error))throw std::runtime_error(error.toStdString());
            for(const auto& p:mesh.nodes)source.nodes.insert(source.nodes.end(),p.begin(),p.end());
            for(const auto& t:mesh.tetrahedra)for(auto i:t)source.tetrahedra.push_back(static_cast<int>(i));
            for(const auto& group:source.attachGroups)for(int i:group)if(i<0||static_cast<std::size_t>(i)>=source.nodes.size()/3)throw std::runtime_error("自动网格后的附着节点索引超出范围。");
        }
        motion::modal::MotionFlexBodyParams p;
        QStringList attachmentDiagnostics;
        p.node_coords=source.nodes;p.tet_node_ids=source.tetrahedra;
        p.young_modulus=source.youngModulus;p.poisson_ratio=source.poissonRatio;p.density=source.density;
        p.n_modes=source.definition.value("modeCount").toInt(6);
        p.full_fem_dynamics=source.independentInterfaces;
        p.inspection_only=inspectionOnly;
        p.damping_type=source.definition.value("dampingType").toInt(2);
        p.modal_damping_ratio=source.definition.value("modalDampingRatio").toDouble(.02);
        p.rayleigh_alpha=source.definition.value("rayleighMass").toDouble(10);
        p.rayleigh_beta=source.definition.value("rayleighStiffness").toDouble(.0002);
        p.freq_cutoff_hz=source.definition.value("freqCutoff").toDouble();
        p.attach_groups=source.attachGroups;
        double radius=source.definition.value("connectionRadius").toDouble();
        if(radius<=0){
            double total=0;int count=0;
            for(std::size_t t=0;t+3<source.tetrahedra.size()&&t<800;t+=4){
                const int a=source.tetrahedra[t];
                for(int k=1;k<4;++k){const int b=source.tetrahedra[t+k];
                    total+=std::hypot(source.nodes[3*a]-source.nodes[3*b],source.nodes[3*a+1]-source.nodes[3*b+1],source.nodes[3*a+2]-source.nodes[3*b+2]);++count;}
            }
            radius=count?3*total/count:0;
        }
        for(std::size_t a=0;a+2<source.attachPositions.size();a+=3){
            std::vector<std::pair<double,int>> nearest;
            int withinRadius=0;
            for(std::size_t i=0;i<source.nodes.size()/3;++i){
                const double d=std::hypot(source.nodes[3*i]-source.attachPositions[a],source.nodes[3*i+1]-source.attachPositions[a+1],source.nodes[3*i+2]-source.attachPositions[a+2]);
                if(d<=radius)++withinRadius;
                if(source.independentInterfaces){
                    bool belongs=true;
                    for(std::size_t other=0;other+2<source.attachPositions.size();other+=3){
                        if(other==a)continue;
                        const double competing=std::hypot(source.nodes[3*i]-source.attachPositions[other],source.nodes[3*i+1]-source.attachPositions[other+1],source.nodes[3*i+2]-source.attachPositions[other+2]);
                        // Equidistant internal nodes stay flexible. They must
                        // not be welded to either of two distinct rigid ports.
                        if(competing<=d+1e-10){belongs=false;break;}
                    }
                    if(!belongs)continue;
                }
                if(d<=radius)nearest.emplace_back(d,static_cast<int>(i));
            }
            if(nearest.empty()){
                if(!source.attachGroups.empty())continue;
                throw std::runtime_error(QStringLiteral("柔性体 %1 的运动副作用点附近没有附着网格节点，请增大 connectionRadius 或显式指定 attachGroups。").arg(source.id).toStdString());
            }
            const auto partitioned=nearest.size();
            std::sort(nearest.begin(),nearest.end());if(nearest.size()>16)nearest.resize(16);
            attachmentDiagnostics.append(QStringLiteral("自动接口 %1：半径 %2 mm 内 %3 个候选，%4 后 %5 个，实际保留最近的 %6 个节点（上限 16）。")
                .arg(a/3+1).arg(radius,0,'g',8).arg(withinRadius)
                .arg(source.independentInterfaces?QStringLiteral("独立接口距离分区，等距节点留作自由节点"):QStringLiteral("距离筛选"))
                .arg(partitioned).arg(nearest.size()));
            std::vector<int> group;for(const auto& item:nearest)group.push_back(item.second);
            std::sort(group.begin(),group.end());
            if(std::find(p.attach_groups.begin(),p.attach_groups.end(),group)==p.attach_groups.end())p.attach_groups.push_back(std::move(group));
        }
        const auto body=system.SearchBody(std::to_string(source.bodyId).c_str());
        if(!body)throw std::runtime_error("柔性体目标运动体未创建。");
        if(p.attach_groups.empty()&&!inspectionOnly&&body->GetBodyFixed()){
            // A fixed target still needs an explicit finite attachment patch.
            throw std::runtime_error("固定柔性体没有附着节点组；请指定 attachGroups，避免把整件柔体误当成固定刚体。");
        }
        if(p.attach_groups.empty()&&!inspectionOnly)
            throw std::runtime_error("自动柔性体没有实际附着接口，原运动体上的载荷和运动副无法传入自由网格。请指定附着节点或运动副接口；自由-自由模态仅可单独检查，不能把相互脱离的刚体和网格作为动态结果。");
        if(source.independentInterfaces&&source.interfaceBodyIds.size()!=p.attach_groups.size())
            throw std::runtime_error("独立柔性接口数量与附着节点组不一致。");
        for(std::size_t i=0;i<p.attach_groups.size();++i){
            const auto port=source.independentInterfaces?system.SearchBody(std::to_string(source.interfaceBodyIds[i]).c_str()):body;
            if(!port)throw std::runtime_error("独立柔性接口运动体尚未创建。");
            p.external_interfaces.push_back(port);
        }
        if(!source.independentInterfaces&&!p.attach_groups.empty()){
            // Shared patches belong to one six-coordinate rigid reference.
            // Their UNION may be a valid weld even if each listed group is a
            // single point. A point or a line alone cannot transmit all three
            // torque components, and makes the CMS static extension singular.
            std::set<int> unique;
            for(const auto& group:p.attach_groups)unique.insert(group.begin(),group.end());
            const auto at=[&](int i){return motion::MotionVector<>(source.nodes[3*i],source.nodes[3*i+1],source.nodes[3*i+2]);};
            double span2=0,area2=0;
            if(!unique.empty()){
                const auto origin=at(*unique.begin());
                for(int i:unique){const auto a=at(i)-origin;span2=std::max(span2,a.Length2());
                    for(int j:unique)area2=std::max(area2,(a%(at(j)-origin)).Length2());
                }
            }
            if(unique.size()<3||!(span2>0)||area2<=1e-16*span2*span2)
                throw std::runtime_error(QStringLiteral("自动柔性体 %1 的共享刚性附着区实际有 %2 个独立节点%3，不能传递完整三维力矩。至少需要 3 个非共线节点；请增大 connectionRadius 或重新选择附着节点。")
                    .arg(source.id).arg(unique.size()).arg(unique.size()>=3?QStringLiteral("，但节点共线或重合"):QString()).toStdString());
        }
        NativeFlexRuntime runtime;runtime.id=source.id;runtime.bodyId=source.bodyId;
        runtime.tetrahedra=source.tetrahedra;runtime.densityKgMm3=source.density;
        const auto initialOrigin=body->GetPos(),initialVelocity=body->GetPos_dt(),initialAngularVelocity=body->GetWvel_par();
        runtime.result=motion::modal::BuildFlexBody(system,p);
        if(!runtime.result.assembly||(!source.independentInterfaces&&runtime.result.n_modes<=0))throw std::runtime_error("柔性体模态缩减没有生成有效模态。");
        runtime.reducedDynamics=runtime.result.reduced_dynamics;
        runtime.modalBasis.requestedCount=p.n_modes;
        runtime.modalBasis.kind=p.attach_groups.empty()?"FreeBody":"FixedInterface";
        runtime.modalBasis.cutoffHz=p.freq_cutoff_hz;
        runtime.modalBasis.frequenciesHz=runtime.result.mode_frequencies_hz;
        runtime.modalBasis.attachmentGroups=p.attach_groups;
        runtime.modalBasis.diagnostics=attachmentDiagnostics;
        const int modes=runtime.result.n_modes;
        const std::size_t boundary=6*runtime.result.interface_bodies.size(),columns=boundary+static_cast<std::size_t>(modes);
        if(modes>0&&runtime.result.psi_internal.size()!=source.nodes.size()*columns)
            throw std::runtime_error("真实模态基的节点/接口/列数不一致，不能显示无效振型。");
        runtime.modalBasis.shapes.reserve(source.nodes.size()*modes);
        for(std::size_t row=0;row<source.nodes.size();++row)
            for(int mode=0;mode<modes;++mode)
                runtime.modalBasis.shapes.push_back(runtime.result.psi_internal[row*columns+boundary+mode]);
        for(const auto& port:runtime.result.interface_bodies){
            bool numeric=false;const int id=QString::fromStdString(port->GetNameString()).toInt(&numeric);
            runtime.modalBasis.interfaceBodyIds.push_back(numeric?id:-1);
            const auto pose=port->GetCoord();
            runtime.referenceInterfacePoses.insert(runtime.referenceInterfacePoses.end(),{pose.pos.x(),pose.pos.y(),pose.pos.z(),pose.rot.e0(),pose.rot.e1(),pose.rot.e2(),pose.rot.e3()});
        }
        if(modes<p.n_modes)runtime.modalBasis.diagnostics.append(QStringLiteral("请求 %1 阶，当前附着条件/频率截断后实际保留 %2 阶。").arg(p.n_modes).arg(modes));
        if(!runtime.reducedDynamics){
            runtime.modalBasis.diagnostics.append(p.attach_groups.empty()
                ?QStringLiteral("自由-自由基只用于模态检查；没有实际附着接口时，不支持把该网格替代运动体进行动力学求解。")
                :QStringLiteral("独立接口动力学使用全部 tetra4 节点自由度；本夹持模态基仅供检查，不是动态模态坐标。"));
            if(modes==0)runtime.modalBasis.diagnostics.append(QStringLiteral("所有内部坐标已夹持或全部检查频率高于截断值，当前没有可显示的内部模态；全有限元动力学仍保留各接口之间的弹性自由度。"));
            // Full FEM carries its own node velocities. Start with the same
            // physical rigid velocity field as the original body, then let
            // initial assembly project only the actual joint constraints.
            for(const auto& mesh:runtime.result.assembly->Get_internal_meshlist())for(const auto& node:mesh->GetNodes()){
                const auto xyz=std::dynamic_pointer_cast<motion::fea::MotionNodeFEAxyz>(node);
                if(xyz)xyz->SetPos_dt(initialVelocity+initialAngularVelocity%(xyz->GetPos()-initialOrigin));
            }
            runtime.result.n_modes=0;
            runtime.result.psi_internal.clear();
        }
        runtime.result.assembly->SetInternalNodesUpdate(true);
        out.push_back(std::move(runtime));
    }
    return out;
}

void validateNativeFlex(const std::vector<NativeFlexRuntime>& flex,double time)
{
    for(const auto& runtime:flex){
        if (runtime.importedBody) {
            const auto& q = runtime.importedBody->GetModalAmplitudes();
            if (q.size() != runtime.result.n_modes || !q.allFinite())
                throw NativeFlexDivergence{runtime.id, time, QStringLiteral("MNF 模态坐标无效")};
            continue;
        }
        if(!runtime.result.assembly)throw std::runtime_error("柔性体模态装配不存在。");
        if(!runtime.reducedDynamics)continue;
        const auto& q=runtime.result.assembly->Get_modal_q();
        if(q.size()!=runtime.result.n_modes)throw std::runtime_error("柔性体模态坐标数量改变。");
        if(!q.allFinite()||q.squaredNorm()>1e6)
            throw NativeFlexDivergence{runtime.id,time,QStringLiteral("模态坐标非有限或模长超过 1000")};
    }
}

void captureNativeFlex(std::vector<NativeFlexRuntime>& flex,double time)
{
    validateNativeFlex(flex,time);
    struct PendingFrame {
        NativeFlexRuntime* runtime;
        std::vector<double> q,positions,poses;
    };
    std::vector<PendingFrame> pending;
    pending.reserve(flex.size());
    // Validate and stage ALL flexible bodies before appending any history. A
    // bad second body must not append a frame to the first body's result.
    for(auto& runtime:flex){
        if(!runtime.times.empty()&&time<=runtime.times.back())continue;
        PendingFrame frame{&runtime,{},{},{}};
        auto& positions=frame.positions;
        positions.reserve(runtime.result.node_x0.size());
        const auto appendPosition = [&](const motion::MotionVector<>& p) {
            if(!std::isfinite(p.x())||!std::isfinite(p.y())||!std::isfinite(p.z()))
                throw NativeFlexDivergence{runtime.id,time,QStringLiteral("柔性体节点位置出现非有限值")};
            positions.insert(positions.end(),{p.x(),p.y(),p.z()});
        };
        if (const auto& imported = runtime.importedBody) {
            const auto& q = imported->GetModalAmplitudes();
            frame.q.assign(q.data(), q.data() + q.size());
            const auto& reference = runtime.result.node_x0;
            using RowMajorMatrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
            const Eigen::Map<const RowMajorMatrix> basis(runtime.result.psi_internal.data(), reference.size(), q.size());
            const Eigen::VectorXd displacement = basis * q;
            const auto& currentPose = imported->GetFrame_REF_to_abs();
            const auto& originalPose = runtime.importedReferencePose;
            const auto rotation = currentPose.GetRot() * originalPose.GetRot().GetConjugate();
            for (std::size_t row = 0; row < reference.size(); row += 3) {
                const motion::MotionVector<> deformed(reference[row] + displacement[row],
                    reference[row + 1] + displacement[row + 1], reference[row + 2] + displacement[row + 2]);
                appendPosition(currentPose.GetPos() + rotation.Rotate(deformed - originalPose.GetPos()));
            }
        } else {
            auto& assembly=*runtime.result.assembly;
            const auto& q=assembly.Get_modal_q();
            if(runtime.reducedDynamics) {
                assembly.SetInternalStateWithModes(true);
                frame.q.assign(q.data(),q.data()+q.size());
            }
            for(const auto& mesh:assembly.Get_internal_meshlist())for(const auto& node:mesh->GetNodes()){
                const auto xyz=std::dynamic_pointer_cast<motion::fea::MotionNodeFEAxyz>(node);
                if(!xyz)throw std::runtime_error("自动柔性体网格节点类型不受支持。");
                appendPosition(xyz->GetPos());
            }
        }
        if(positions.size()!=runtime.result.node_x0.size())throw std::runtime_error("柔性体结果节点数量与参考网格不一致。");
        if(!positions.empty()&&runtime.times.size()+1>60000000/positions.size())throw std::runtime_error("柔性体动画结果超过 60000000 个坐标预算；请增大步长或简化网格。");
        for(const auto& body:runtime.result.interface_bodies){
            if(!body)throw std::runtime_error("柔性体接口运动体不存在。");
            const auto& pose=body->GetCoord();
            frame.poses.insert(frame.poses.end(),{pose.pos.x(),pose.pos.y(),pose.pos.z(),
                pose.rot.e0(),pose.rot.e1(),pose.rot.e2(),pose.rot.e3()});
            if(!std::all_of(frame.poses.end()-7,frame.poses.end(),[](double v){return std::isfinite(v);})
                ||pose.rot.Length2()<1e-24)
                throw NativeFlexDivergence{runtime.id,time,QStringLiteral("柔性体接口位姿无效")};
        }
        pending.push_back(std::move(frame));
    }
    for(auto& frame:pending){
        auto& runtime=*frame.runtime;
        runtime.times.push_back(time);
        runtime.modalCoordinates.push_back(std::move(frame.q));
        runtime.nodePositions.push_back(std::move(frame.positions));
        runtime.interfacePoses.push_back(std::move(frame.poses));
    }
}

SolveStatus inspectNativeFlexModes(const NativeMotionInput& input,int flexId,RigidFlexResult& result,
                                  QString& error,const SolverCallbacks& callbacks)
{
    try{
        if(!input.geometry)throw std::runtime_error("尚未准备柔性体几何。");
        const auto found=std::find_if(input.geometry->flexBodies.begin(),input.geometry->flexBodies.end(),
            [flexId](const NativeFlexGeometry& source){return source.id==flexId;});
        if(found==input.geometry->flexBodies.end())throw std::runtime_error("所选自动柔性体不存在或未启用。");
        if(callbacks.stopRequested&&callbacks.stopRequested()){error.clear();return SolveStatus::Stopped;}
        NativeMotionInput selected=input;
        auto geometry=std::make_shared<NativeMotionGeometry>();geometry->flexBodies.push_back(*found);selected.geometry=geometry;
        NativeMotionSystem system;system.sourceInput=&selected;
        // Only the selected mesh and its actual interface frames are needed.
        // No joint parser, controller, transient integration or preload runs.
        std::set<int> ports(found->interfaceBodyIds.begin(),found->interfaceBodyIds.end());ports.insert(found->bodyId);
        for(const auto& source:input.bodies)if(ports.count(source.body.id)){
            auto body=std::make_shared<motion::MotionBody>();
            body->SetNameString(std::to_string(source.body.id));
            body->SetPos(motion::MotionVector<>(source.properties.centerMm[0],source.properties.centerMm[1],source.properties.centerMm[2]));
            body->SetBodyFixed(source.body.fixed);
            body->SetMass(source.properties.massKg);
            const auto& inertia=source.properties.inertiaKgM2;
            body->SetInertiaXX(motion::MotionVector<>(inertia[0]*1e6,inertia[1]*1e6,inertia[2]*1e6));
            system.AddBody(body);
        }
        if(callbacks.progress)callbacks.progress(.05);
        auto runtime=buildNativeFlex(system,selected,callbacks,true);
        if(callbacks.stopRequested&&callbacks.stopRequested()){error.clear();return SolveStatus::Stopped;}
        if(runtime.size()!=1)throw std::runtime_error("模态检查没有生成所选网格的有效基。");
        const auto& source=runtime.front();
        RigidFlexResult next;next.flexId=source.id;next.bodyId=source.bodyId;
        next.referenceNodes=source.result.node_x0;next.surfaceTriangles=source.result.surface_tris;
        next.tetrahedra=source.tetrahedra;next.densityKgMm3=source.densityKgMm3;
        next.reducedDynamics=source.reducedDynamics;next.modalBasis=source.modalBasis;
        next.interfaceCount=static_cast<int>(source.result.interface_bodies.size());
        next.initialInterfacePoses=source.referenceInterfacePoses;
        next.modeCount=source.result.n_modes;next.psiInternal=source.result.psi_internal;
        next.nodePositions.push_back(next.referenceNodes);next.interfacePoses.push_back(next.initialInterfacePoses);
        if(!validateFlexModes(next,error))return SolveStatus::Failed;
        result=std::move(next);error.clear();if(callbacks.progress)callbacks.progress(1);
        return SolveStatus::Completed;
    }catch(const std::exception& exception){error=QString::fromUtf8(exception.what());}
    catch(...){error=QStringLiteral("真实模态检查失败（未知引擎异常）。");}
    return SolveStatus::Failed;
}

} // namespace MotionOcc
