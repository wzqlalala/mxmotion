#include "NativeMotionRuntime.h"
#include "FemMesh.h"

#include "motion/fea/Mesh.h"
#include "motion/fea/NodeFEAxyz.h"

#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QElapsedTimer>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace MotionOcc {

std::vector<NativeFlexRuntime> buildNativeFlex(NativeMotionSystem& system,const NativeMotionInput& input,
                                             const SolverCallbacks& callbacks)
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
        p.node_coords=source.nodes;p.tet_node_ids=source.tetrahedra;
        p.young_modulus=source.youngModulus;p.poisson_ratio=source.poissonRatio;p.density=source.density;
        p.n_modes=source.definition.value("modeCount").toInt(6);
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
            for(std::size_t i=0;i<source.nodes.size()/3;++i){
                const double d=std::hypot(source.nodes[3*i]-source.attachPositions[a],source.nodes[3*i+1]-source.attachPositions[a+1],source.nodes[3*i+2]-source.attachPositions[a+2]);
                if(d<=radius)nearest.emplace_back(d,static_cast<int>(i));
            }
            if(nearest.empty()){
                if(!source.attachGroups.empty())continue;
                throw std::runtime_error(QStringLiteral("柔性体 %1 的运动副作用点附近没有附着网格节点，请增大 connectionRadius 或显式指定 attachGroups。").arg(source.id).toStdString());
            }
            std::sort(nearest.begin(),nearest.end());if(nearest.size()>16)nearest.resize(16);
            std::vector<int> group;for(const auto& item:nearest)group.push_back(item.second);
            std::sort(group.begin(),group.end());
            if(std::find(p.attach_groups.begin(),p.attach_groups.end(),group)==p.attach_groups.end())p.attach_groups.push_back(std::move(group));
        }
        const auto body=system.SearchBody(std::to_string(source.bodyId).c_str());
        if(!body)throw std::runtime_error("柔性体目标运动体未创建。");
        if(p.attach_groups.empty()&&body->GetBodyFixed()){
            // A fixed target still needs an explicit finite attachment patch.
            throw std::runtime_error("固定柔性体没有附着节点组；请指定 attachGroups，避免把整件柔体误当成固定刚体。");
        }
        for(std::size_t i=0;i<p.attach_groups.size();++i)p.external_interfaces.push_back(body);
        NativeFlexRuntime runtime;runtime.id=source.id;runtime.bodyId=source.bodyId;
        runtime.result=motion::modal::BuildFlexBody(system,p);
        if(!runtime.result.assembly||runtime.result.n_modes<=0)throw std::runtime_error("柔性体模态缩减没有生成有效模态。");
        runtime.result.assembly->SetInternalNodesUpdate(true);
        out.push_back(std::move(runtime));
    }
    return out;
}

void captureNativeFlex(std::vector<NativeFlexRuntime>& flex,double time)
{
    for(auto& runtime:flex){
        if(!runtime.times.empty()&&time<=runtime.times.back())continue;
        auto& assembly=*runtime.result.assembly;
        const auto& q=assembly.Get_modal_q();
        if(!q.allFinite()||q.squaredNorm()>1e6)throw std::runtime_error(QStringLiteral("自动柔性体 %1 的模态坐标在 t=%2 s 发散，已终止，未返回伪造结果。").arg(runtime.id).arg(time).toStdString());
        assembly.SetInternalStateWithModes(true);
        std::vector<double> positions;
        for(const auto& mesh:assembly.Get_internal_meshlist())for(const auto& node:mesh->GetNodes()){
            const auto xyz=std::dynamic_pointer_cast<motion::fea::MotionNodeFEAxyz>(node);
            if(!xyz)throw std::runtime_error("自动柔性体网格节点类型不受支持。");
            const auto& p=xyz->GetPos();
            if(!std::isfinite(p.x())||!std::isfinite(p.y())||!std::isfinite(p.z()))throw std::runtime_error("柔性体节点位置出现非有限值。");
            positions.insert(positions.end(),{p.x(),p.y(),p.z()});
        }
        if(positions.size()!=runtime.result.node_x0.size())throw std::runtime_error("柔性体结果节点数量与参考网格不一致。");
        if(positions.size()*(runtime.times.size()+1)>60000000)throw std::runtime_error("柔性体动画结果超过 60000000 个坐标预算；请增大步长或简化网格。");
        runtime.times.push_back(time);runtime.modalCoordinates.emplace_back(q.data(),q.data()+q.size());runtime.nodePositions.push_back(std::move(positions));
    }
}

} // namespace MotionOcc
