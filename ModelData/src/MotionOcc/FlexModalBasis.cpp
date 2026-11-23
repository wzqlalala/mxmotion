#include "RigidMotion.h"

#include <QSaveFile>
#include <QTextStream>
#include <QLocale>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace MotionOcc {

bool validateFlexModes(const RigidFlexResult& result, QString& error)
{
    const auto fail=[&](const QString& why){error=why;return false;};
    const auto rows=result.referenceNodes.size(), modes=result.modalBasis.frequenciesHz.size();
    if(!rows || rows%3) return fail(QStringLiteral("模态检查缺少完整的参考节点。"));
    if(!modes) return fail(QStringLiteral("本次计算没有保留可显示的真实模态，请检查附着区、模态数量及截止频率。"));
    if(modes>std::numeric_limits<std::size_t>::max()/rows || result.modalBasis.shapes.size()!=rows*modes)
        return fail(QStringLiteral("真实振型矩阵的节点数或模态数不一致。"));
    if ((!result.sourceNodeIds.empty() && result.sourceNodeIds.size() != rows / 3) ||
        (!result.modalBasis.sourceModeIds.empty() && result.modalBasis.sourceModeIds.size() != modes))
        return fail(QStringLiteral("外部节点号或模态号与振型矩阵不一致。"));
    const auto finite=[](const auto& values){return std::all_of(values.begin(),values.end(),[](double value){return std::isfinite(value);});};
    if(!finite(result.referenceNodes) || !finite(result.modalBasis.shapes) || !finite(result.modalBasis.frequenciesHz))
        return fail(QStringLiteral("模态检查数据包含非有限值。"));
    for(std::size_t mode=0;mode<modes;++mode){
        if(result.modalBasis.frequenciesHz[mode]<0) return fail(QStringLiteral("模态频率不能为负数。"));
        double maximum=0;
        for(std::size_t node=0;node<rows/3;++node)
            maximum=std::max(maximum,std::hypot(result.modalBasis.shapes[(3*node)*modes+mode],
                result.modalBasis.shapes[(3*node+1)*modes+mode],result.modalBasis.shapes[(3*node+2)*modes+mode]));
        if(!std::isfinite(maximum) || (maximum==0 && result.modalBasis.kind!="MNF"))
            return fail(QStringLiteral("第 %1 阶振型为零或无效。").arg(mode+1));
    }
    for(const auto& group:result.modalBasis.attachmentGroups){
        if(group.empty()) return fail(QStringLiteral("模态附着区包含空节点组。"));
        std::set<int> ids;
        for(int id:group) if(id<0 || std::size_t(id)>=rows/3 || !ids.insert(id).second)
            return fail(QStringLiteral("模态附着区存在越界或重复的节点。"));
    }
    error.clear();return true;
}

bool sampleFlexMode(const RigidFlexResult& result,int mode,double amplitudeMm,double phase,
                    FlexModeSample& sample,QString& error)
{
    if(!validateFlexModes(result,error)) return false;
    const auto modes=result.modalBasis.frequenciesHz.size();
    if(mode<0 || std::size_t(mode)>=modes || !std::isfinite(amplitudeMm) || amplitudeMm<0
        || !std::isfinite(phase) || std::abs(phase)>1){
        error=QStringLiteral("振型序号、显示幅值或动画相位无效。");return false;
    }
    FlexModeSample next;next.frequencyHz=result.modalBasis.frequenciesHz[std::size_t(mode)];
    next.magnitudes.resize(result.referenceNodes.size()/3);
    for(std::size_t node=0;node<next.magnitudes.size();++node){
        const auto at=[&](int component){return result.modalBasis.shapes[(3*node+component)*modes+std::size_t(mode)];};
        next.magnitudes[node]=std::hypot(at(0),at(1),at(2));
        next.rawMaximum=std::max(next.rawMaximum,next.magnitudes[node]);
    }
    next.positions=result.referenceNodes;
    if (next.rawMaximum == 0.0) {
        sample = std::move(next);
        error.clear();
        return true;
    }
    for(std::size_t row=0;row<next.positions.size();++row)
        next.positions[row]+=amplitudeMm*phase*(result.modalBasis.shapes[row*modes+std::size_t(mode)]/next.rawMaximum);
    for(auto& value:next.magnitudes)value=std::abs(phase)*(value/next.rawMaximum);
    if(!std::all_of(next.positions.begin(),next.positions.end(),[](double v){return std::isfinite(v);})){
        error=QStringLiteral("振型显示幅值过大，预览坐标溢出。");return false;
    }
    sample=std::move(next);error.clear();return true;
}

bool exportFlexModesCsv(const QString& path,const RigidFlexResult& result,QString& error)
{
    if(!validateFlexModes(result,error))return false;
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text)){error=file.errorString();return false;}
    QTextStream out(&file);out.setCodec("UTF-8");out.setLocale(QLocale::c());out.setRealNumberPrecision(17);
    const bool hasSourceIds = !result.sourceNodeIds.empty() && !result.modalBasis.sourceModeIds.empty();
    out << "basis,mode,frequency_hz,node_index_0,x_mm,y_mm,z_mm,phi_x_raw,phi_y_raw,phi_z_raw,phi_x_normalized,phi_y_normalized,phi_z_normalized";
    if (hasSourceIds) out << ",source_mode_id,source_node_id";
    out << '\n';
    const auto modes=result.modalBasis.frequenciesHz.size();
    for(std::size_t mode=0;mode<modes;++mode){
        FlexModeSample sample;
        if(!sampleFlexMode(result,int(mode),1,1,sample,error)){file.cancelWriting();return false;}
        for(std::size_t node=0;node<result.referenceNodes.size()/3;++node){
            out << result.modalBasis.kind << ',' << mode+1 << ',' << sample.frequencyHz << ',' << node;
            for(int c=0;c<3;++c)out << ',' << result.referenceNodes[3*node+c];
            for(int c=0;c<3;++c)out << ',' << result.modalBasis.shapes[(3*node+c)*modes+mode];
            const double normalization = sample.rawMaximum > 0.0 ? sample.rawMaximum : 1.0;
            for(int c=0;c<3;++c)out << ',' << result.modalBasis.shapes[(3*node+c)*modes+mode]/normalization;
            if (hasSourceIds) out << ',' << result.modalBasis.sourceModeIds[mode] << ',' << result.sourceNodeIds[node];
            out << '\n';
        }
    }
    out.flush();
    if(out.status()!=QTextStream::Ok || !file.commit()){error=file.errorString();return false;}
    error.clear();return true;
}

} // namespace MotionOcc
