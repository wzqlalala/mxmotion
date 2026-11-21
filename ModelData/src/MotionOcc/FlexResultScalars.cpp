#include "RigidMotion.h"

#include <Eigen/Eigenvalues>

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

std::vector<double> legacyProjection(const RigidFlexResult& result,const std::vector<double>& elastic)
{
    const auto count=result.referenceNodes.size()/3;
    if(count<4||result.tetrahedra.empty()||result.tetrahedra.size()%4)
        fail("自由基投影需要完整的四面体网格。");
    if(!std::isfinite(result.densityKgMm3)||result.densityKgMm3<=0)
        fail("自由基投影需要正的材料密度。");
    std::vector<double> masses(count,0);
    for(std::size_t t=0;t<result.tetrahedra.size();t+=4){
        for(int k=0;k<4;++k)if(result.tetrahedra[t+k]<0||static_cast<std::size_t>(result.tetrahedra[t+k])>=count)
            fail("柔性体四面体节点索引越界。");
        const auto a=point(result.referenceNodes,result.tetrahedra[t]);
        const auto b=point(result.referenceNodes,result.tetrahedra[t+1]);
        const auto c=point(result.referenceNodes,result.tetrahedra[t+2]);
        const auto d=point(result.referenceNodes,result.tetrahedra[t+3]);
        const double mass=result.densityKgMm3*std::abs((b-a).dot((c-a).cross(d-a)))/24.0;
        if(!std::isfinite(mass))fail("柔性体单元质量不是有限值。");
        for(int k=0;k<4;++k)masses[result.tetrahedra[t+k]]+=mass;
    }
    Eigen::Vector3d centroid=Eigen::Vector3d::Zero();double total=0;
    for(std::size_t i=0;i<count;++i){centroid+=point(result.referenceNodes,i)*masses[i];total+=masses[i];}
    if(!std::isfinite(total)||total<=0)fail("自由基投影网格没有正的体积质量。");
    centroid/=total;
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
