#include "RigidMotion.h"

#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>
#include <Eigen/SVD>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace MotionOcc {
namespace {

[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }
bool finite(const std::vector<double>& values)
{
    return std::all_of(values.begin(),values.end(),[](double value){return std::isfinite(value);});
}
Eigen::Vector3d point(const std::vector<double>& flat,std::size_t index)
{
    return {flat[3*index],flat[3*index+1],flat[3*index+2]};
}

std::vector<double> tetrahedralMassFractions(const RigidFlexResult& result)
{
    const auto count=result.referenceNodes.size()/3;
    if(count<4 || result.referenceNodes.size()%3 || result.tetrahedra.empty() || result.tetrahedra.size()%4)
        fail("自动柔性体弹性位移需要完整的四面体网格。");
    std::vector<double> weights(count,0);
    double total=0;
    for(std::size_t t=0;t<result.tetrahedra.size();t+=4){
        for(int k=0;k<4;++k)if(result.tetrahedra[t+k]<0||static_cast<std::size_t>(result.tetrahedra[t+k])>=count)
            fail("柔性体四面体节点索引越界。");
        const auto a=point(result.referenceNodes,result.tetrahedra[t]);
        const auto b=point(result.referenceNodes,result.tetrahedra[t+1]);
        const auto c=point(result.referenceNodes,result.tetrahedra[t+2]);
        const auto d=point(result.referenceNodes,result.tetrahedra[t+3]);
        // Each node receives one quarter of the element mass. The material
        // density is uniform within an AutoFlex body and cancels on normalization.
        const double quarterVolume=std::abs((b-a).dot((c-a).cross(d-a)))/24.0;
        for(int k=0;k<4;++k)weights[result.tetrahedra[t+k]]+=quarterVolume;
        total+=4*quarterVolume;
    }
    if(!std::isfinite(total)||total<=0)fail("柔性体网格没有有效的正体积质量。");
    for(double& weight:weights)weight/=total;
    return weights;
}

struct ContourReference {
    Eigen::Vector3d center;
    Eigen::Matrix3d toRest;
};

ContourReference fitRigidReference(const std::vector<double>& rest,const std::vector<double>& current,
                                   const std::vector<double>& weights,const Eigen::Vector3d& restCenter)
{
    Eigen::Vector3d center=Eigen::Vector3d::Zero();
    for(std::size_t i=0;i<weights.size();++i)center+=weights[i]*point(current,i);
    Eigen::Matrix3d covariance=Eigen::Matrix3d::Zero();
    for(std::size_t i=0;i<weights.size();++i)
        covariance.noalias()+=weights[i]*(point(current,i)-center)*(point(rest,i)-restCenter).transpose();
    if(!covariance.allFinite())fail("自动柔性体刚体参考含有无效值。");
    const Eigen::JacobiSVD<Eigen::Matrix3d> fit(covariance,Eigen::ComputeFullU|Eigen::ComputeFullV);
    if(fit.info()!=Eigen::Success)fail("自动柔性体刚体参考拟合失败。");
    Eigen::Matrix3d correction=Eigen::Matrix3d::Identity();
    correction(2,2)=(fit.matrixV()*fit.matrixU().transpose()).determinant()<0 ? -1.0 : 1.0;
    return {center,fit.matrixV()*correction*fit.matrixU().transpose()};
}

void autoFlexElasticScalars(const RigidFlexResult& result,std::size_t left,std::size_t right,
                            double fraction,std::vector<double>& values)
{
    std::vector<double> uncached;
    if(result.contourMassFractions.empty())uncached=tetrahedralMassFractions(result);
    const auto& weights=result.contourMassFractions.empty()?uncached:result.contourMassFractions;
    if(weights.size()!=values.size())fail("自动柔性体云图的节点质量数目无效。");
    const auto& rest=result.referenceNodes;
    Eigen::Vector3d restCenter=Eigen::Vector3d::Zero();
    for(std::size_t i=0;i<weights.size();++i)restCenter+=weights[i]*point(rest,i);

    // Remove rigid motion at each accepted frame before interpolation. Blending
    // the world coordinates of a rotating beam first would introduce strain.
    const auto& first=result.nodePositions[fraction==1?right:left];
    const auto& last=result.nodePositions[right];
    const bool blend=left!=right && fraction>0 && fraction<1;
    const auto firstFit=fitRigidReference(rest,first,weights,restCenter);
    const auto lastFit=blend?fitRigidReference(rest,last,weights,restCenter):firstFit;
    for(std::size_t i=0;i<values.size();++i){
        Eigen::Vector3d aligned=firstFit.toRest*(point(first,i)-firstFit.center);
        if(blend)
            aligned=(1-fraction)*aligned+fraction*(lastFit.toRest*(point(last,i)-lastFit.center));
        values[i]=(aligned-(point(rest,i)-restCenter)).norm();
    }
}

std::vector<double> legacyProjection(const RigidFlexResult& result,const std::vector<double>& elastic)
{
    if(!std::isfinite(result.densityKgMm3)||result.densityKgMm3<=0)
        fail("自由基投影需要正的材料密度。");
    const auto masses=tetrahedralMassFractions(result);
    const auto count=masses.size();
    Eigen::Vector3d centroid=Eigen::Vector3d::Zero();
    for(std::size_t i=0;i<count;++i)centroid+=point(result.referenceNodes,i)*masses[i];
    Eigen::Matrix3d covariance=Eigen::Matrix3d::Zero();
    for(std::size_t i=0;i<count;++i){const auto delta=(point(result.referenceNodes,i)-centroid).eval();covariance+=masses[i]*delta*delta.transpose();}
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(covariance);
    if(eig.info()!=Eigen::Success||!eig.eigenvalues().allFinite())fail("自由基投影主轴计算失败。");
    const Eigen::Vector3d axis=eig.eigenvectors().col(2);
    double first=std::numeric_limits<double>::infinity(),last=-first;
    std::vector<double> coordinates(count);
    for(std::size_t i=0;i<count;++i){
        coordinates[i]=(point(result.referenceNodes,i)-centroid).dot(axis);
        first=std::min(first,coordinates[i]);last=std::max(last,coordinates[i]);
    }
    if(!(last>first))fail("自由基投影网格的主轴长度为零。");
    // Preserve the original UI's three mass-weighted cosine basis functions.
    // This is a visualization projection, not an eigensolver or stress field.
    constexpr double pi=3.1415926535897932384626433832795;
    std::vector<double> output(count,0),shape(count);
    for(int mode=1;mode<=3;++mode){
        double numerator=0,denominator=0;
        for(std::size_t i=0;i<count;++i){
            shape[i]=std::cos(2*mode*pi*(coordinates[i]-first)/(last-first));
            numerator+=masses[i]*shape[i]*elastic[i];
            denominator+=masses[i]*shape[i]*shape[i];
        }
        if(!(denominator>0))fail("自由基投影模态质量为零。");
        const double amplitude=numerator/denominator;
        for(std::size_t i=0;i<count;++i)output[i]+=std::abs(shape[i]*amplitude);
    }
    return output;
}

} // namespace

void prepareAutoFlexContour(RigidFlexResult& result)
{
    result.contourMassFractions=tetrahedralMassFractions(result);
}

bool flexNodeScalarsAt(const RigidFlexResult& result,std::size_t left,std::size_t right,double fraction,
                       FlexContourKind kind,std::vector<double>& values,QString& error)
{
    try{
        if(!std::isfinite(fraction)||fraction<0||fraction>1)fail("柔性体帧插值比例无效。");
        const auto coordinates=result.referenceNodes.size();
        if(!coordinates||coordinates%3||!finite(result.referenceNodes))fail("柔性体参考节点数据无效。");
        if(left>=result.nodePositions.size()||right>=result.nodePositions.size()
            ||result.nodePositions[left].size()!=coordinates||result.nodePositions[right].size()!=coordinates
            ||!finite(result.nodePositions[left])||!finite(result.nodePositions[right]))
            fail("柔性体结果帧不存在或节点数量不一致。");
        const auto interpolate=[fraction](double a,double b){return (1-fraction)*a+fraction*b;};
        std::vector<double> next(coordinates/3,0);
        if(kind==FlexContourKind::TotalDisplacement){
            for(std::size_t i=0;i<next.size();++i){
                double displacement[3]{};
                for(int component=0;component<3;++component){
                    const auto index=3*i+component;
                    displacement[component]=interpolate(result.nodePositions[left][index],result.nodePositions[right][index])-result.referenceNodes[index];
                }
                next[i]=std::hypot(displacement[0],displacement[1],displacement[2]);
            }
        }else if(kind==FlexContourKind::ElasticDisplacement && result.modalBasis.kind!=QLatin1String("MNF")){
            autoFlexElasticScalars(result,left,right,fraction,next);
        }else if(!result.reducedDynamics){
            // The legacy projection uses the first interface's original
            // material frame, including the deformation from any preload.
            if(result.interfaceCount<1 || result.initialInterfacePoses.size()!=std::size_t(7*result.interfaceCount)
                || left>=result.interfacePoses.size() || right>=result.interfacePoses.size())
                fail("全有限元弹性位移缺少参考接口位姿。");
            const auto& initial=result.initialInterfacePoses;
            if(!finite(initial))fail("柔性体参考接口位姿无效。");
            const Eigen::Vector3d origin(initial[0],initial[1],initial[2]);
            Eigen::Quaterniond initialRotation(initial[3],initial[4],initial[5],initial[6]);
            if(initialRotation.norm()<1e-12)fail("柔性体参考接口旋转无效。");
            initialRotation.normalize();
            const auto vectors=[&](std::size_t frame){
                const auto& pose=result.interfacePoses[frame];
                if(pose.size()!=initial.size() || !finite(pose))fail("全有限元接口位姿与接口数不一致。");
                Eigen::Quaterniond rotation(pose[3],pose[4],pose[5],pose[6]);
                if(rotation.norm()<1e-12)fail("柔性体接口旋转无效。");
                rotation.normalize();
                const Eigen::Vector3d current(pose[0],pose[1],pose[2]);
                std::vector<Eigen::Vector3d> values(coordinates/3);
                for(std::size_t node=0;node<values.size();++node)
                    values[node]=rotation.conjugate()*(point(result.nodePositions[frame],node)-current)
                        -initialRotation.conjugate()*(point(result.referenceNodes,node)-origin);
                return values;
            };
            const auto first=vectors(left),last=vectors(right);
            for(std::size_t node=0;node<next.size();++node)next[node]=((1-fraction)*first[node]+fraction*last[node]).norm();
            if(kind==FlexContourKind::LegacyFreeModeProjection)next=legacyProjection(result,next);
            else if(kind!=FlexContourKind::ElasticDisplacement)fail("未知柔性体云图类型。");
        }else{
            if(result.modeCount<=0||result.interfaceCount<0||left>=result.modalCoordinates.size()||right>=result.modalCoordinates.size())
                fail("柔性体结果缺少模态坐标。");
            const std::size_t modes=static_cast<std::size_t>(result.modeCount);
            const std::size_t offset=6*static_cast<std::size_t>(result.interfaceCount),columns=offset+modes;
            if(columns>std::numeric_limits<std::size_t>::max()/coordinates||result.psiInternal.size()!=coordinates*columns)
                fail("柔性体模态矩阵尺寸不一致。");
            const auto& firstQ=result.modalCoordinates[left];const auto& lastQ=result.modalCoordinates[right];
            if(firstQ.size()!=modes||lastQ.size()!=modes||!finite(firstQ)||!finite(lastQ))fail("柔性体模态坐标无效。");
            std::vector<double> q(modes);
            for(std::size_t mode=0;mode<modes;++mode)q[mode]=interpolate(firstQ[mode],lastQ[mode]);
            for(std::size_t i=0;i<next.size();++i){
                double displacement[3]{};
                for(std::size_t component=0;component<3;++component){
                    const auto* row=result.psiInternal.data()+(3*i+component)*columns+offset;
                    for(std::size_t mode=0;mode<modes;++mode){
                        if(!std::isfinite(row[mode]))fail("柔性体模态矩阵包含非有限值。");
                        displacement[component]+=row[mode]*q[mode];
                    }
                }
                next[i]=std::hypot(displacement[0],displacement[1],displacement[2]);
            }
            if(kind==FlexContourKind::LegacyFreeModeProjection)next=legacyProjection(result,next);
            else if(kind!=FlexContourKind::ElasticDisplacement)fail("未知柔性体云图类型。");
        }
        if(!finite(next))fail("柔性体云图值包含非有限值。");
        values=std::move(next);error.clear();return true;
    }catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}

bool flexNodeScalars(const RigidFlexResult& result,std::size_t frame,FlexContourKind kind,
                     std::vector<double>& values,QString& error)
{
    return flexNodeScalarsAt(result,frame,frame,0,kind,values,error);
}

} // namespace MotionOcc
