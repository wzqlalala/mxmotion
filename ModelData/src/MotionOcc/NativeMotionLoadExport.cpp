#include "NativeMotionLoadExport.h"
#include "NativeMotionSolver.h"

#include <Eigen/Geometry>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>

namespace MotionOcc {
namespace {
using Vec=Eigen::Vector3d;
[[noreturn]] void fail(const QString& message){throw std::runtime_error(message.toStdString());}
Vec vector(const Vector3d& value){return {value[0],value[1],value[2]};}
Vector3d array(const Vec& value){return {value.x(),value.y(),value.z()};}
QJsonArray json(const Vector3d& value){return {value[0],value[1],value[2]};}
double magnitude(const Vector3d& value){return std::hypot(value[0],value[1],value[2]);}
QString number(double value){return QString::number(value,'g',16);}
QString csv(QString value){value.replace('"',QStringLiteral("\"\""));return '"'+value+'"';}

void validateResult(const NativeMotionInput& input,const RigidResult& result)
{
    if(result.scenarioId!=input.scenario.id||result.frames.empty())fail(QStringLiteral("当前方案没有可导出的求解结果。"));
    double previous=-std::numeric_limits<double>::infinity();
    for(const auto& frame:result.frames){
        if(!std::isfinite(frame.seconds)||frame.seconds<0||frame.seconds<=previous)fail(QStringLiteral("载荷结果时间轴无效。"));
        previous=frame.seconds;
    }
}

const ResultChannel& channel(const RigidResult& result,const NativeLoadSource& source,
                             const QString& variable,const QString& unit)
{
    const ResultChannel* found=nullptr;
    for(const auto& entry:result.channels)if(entry.sourceType==source.collection&&entry.sourceId==source.id&&entry.variable==variable){
        if(found)fail(QStringLiteral("载荷结果通道重复。"));found=&entry;
    }
    if(!found||found->components!=3||found->values.size()!=3*result.frames.size()||found->unit!=unit)
        fail(QStringLiteral("%1 %2 缺少完整的 %3 通道或单位不匹配。")
            .arg(source.collection).arg(source.id).arg(variable));
    for(double value:found->values)if(!std::isfinite(value))fail(QStringLiteral("载荷结果包含非有限值。"));
    return *found;
}
Vec sample(const ResultChannel& source,std::size_t frame)
{
    return {source.values[3*frame],source.values[3*frame+1],source.values[3*frame+2]};
}
QJsonObject definition(const NativeMotionInput& input,const NativeLoadSource& source)
{
    if(source.id<0||(source.collection!="joints"&&source.collection!="couplings"&&source.collection!="drives"))
        fail(QStringLiteral("载荷来源必须为有效的运动副、耦合副或驱动器。"));
    const auto value=input.document.entity(source.collection,source.id);
    if(value.isEmpty())fail(QStringLiteral("载荷来源对象不存在。"));
    if(source.collection=="couplings"&&value.value("couplingType").toString()=="Cable")
        fail(QStringLiteral("绳索耦合包含两个导向副的四个作用端，现有结果仅含第一端口的旧反力通道。不能用整体取反伪造另一端载荷，请使用各导向副的反力，或补充逐体耦合反力结果。"));
    return value;
}

Eigen::Quaterniond quaternion(const Quaterniond& value)
{
    Eigen::Quaterniond q(value[0],value[1],value[2],value[3]);
    if(!q.coeffs().allFinite()||q.squaredNorm()<1e-24)fail(QStringLiteral("载荷坐标转换的运动体四元数无效。"));
    return q.normalized();
}
const BodyState& bodyState(const RigidFrame& frame,int id)
{
    const BodyState* found=nullptr;
    for(const auto& body:frame.bodies)if(body.bodyId==id){
        if(found)fail(QStringLiteral("载荷坐标转换遇到重复运动体结果。"));found=&body;
    }
    if(!found||!vector(found->positionMm).allFinite())fail(QStringLiteral("载荷坐标转换缺少完整运动体位姿。"));
    return *found;
}

Vec targetBodyOffset(const NativeMotionInput& input,const RigidResult& result,
                     const NativeLoadExportOptions& options)
{
    if(options.targetBodyId<0)return vector(options.targetPointMm);
    if(input.document.entity("bodies",options.targetBodyId).isEmpty())
        fail(QStringLiteral("目标节点绑定的运动体不存在。"));
    for(const auto& flex:result.flexBodies)if(flex.bodyId==options.targetBodyId)
        fail(QStringLiteral("目标节点属于柔性体，尚未建立该节点到真实逐帧变形网格的对应关系；不能用接口刚体位姿代替其节点位置。"));
    const PreparedBody* prepared=nullptr;
    for(const auto& body:input.bodies)if(body.body.id==options.targetBodyId) {
        if(prepared)fail(QStringLiteral("目标节点的运动体定义重复。"));
        prepared=&body;
    }
    if(!prepared || !vector(prepared->properties.centerMm).allFinite())
        fail(QStringLiteral("目标节点缺少求解时的初始质心数据。"));
    return vector(options.targetPointMm)-vector(prepared->properties.centerMm);
}

void writeAtomic(const QString& path,const QByteArray& data)
{
    if(path.isEmpty())fail(QStringLiteral("导出路径为空。"));
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())
        fail(QStringLiteral("无法完整保存载荷文件：%1").arg(file.errorString()));
}

std::vector<NativeLoadSample> series(const NativeMotionInput& input,const RigidResult& result,
                                    const NativeLoadSource& source,NativeLoadEnd end)
{
    validateResult(input,result);const auto sourceDefinition=definition(input,source);
    if(end!=NativeLoadEnd::I&&end!=NativeLoadEnd::J)fail(QStringLiteral("载荷作用端无效。"));
    const auto& force=channel(result,source,"ReactionForce",QStringLiteral("N"));
    const auto& torque=channel(result,source,"ReactionTorque",QStringLiteral("N·mm"));
    // A motor's Position is its J frame. Use its driven joint's two actual
    // marker histories rather than treating one point as both endpoints.
    NativeLoadSource markerSource=source;
    if(source.collection=="drives")markerSource={"joints",sourceDefinition.value("targetJointId").toInt(-1)};
    const auto& pointI=channel(result,markerSource,"Position",QStringLiteral("mm"));
    const auto& pointJ=channel(result,markerSource,"PositionJ",QStringLiteral("mm"));
    std::vector<NativeLoadSample> output;output.reserve(result.frames.size());
    for(std::size_t frame=0;frame<result.frames.size();++frame){
        const Vec pI=sample(pointI,frame),pJ=sample(pointJ,frame);
        const Vec fJ=sample(force,frame),mJ=sample(torque,frame);
        // MotionEngine's Get_react_* is the wrench on body J, about marker J.
        // A fixed 1 kg body in 9.81 m/s² gravity gives raw FJz=-9.81 N;
        // equilibrium requires FIz=+9.81 N. Keep this conversion at export.
        Vec p=pJ,f=fJ,m=mJ;
        if(end==NativeLoadEnd::I){p=pI;f=-fJ;m=-mJ+(pJ-pI).cross(f);}
        output.push_back({result.frames[frame].seconds,array(p),array(f),array(m)});
    }
    return output;
}

std::vector<std::size_t> selectedFrames(const std::vector<NativeLoadSample>& samples,const NativeLoadExportOptions& options)
{
    if(options.frames==NativeLoadFrames::Specified){
        if(options.frameIndex>=samples.size())fail(QStringLiteral("指定载荷帧超出已接受的求解结果范围。"));
        return {options.frameIndex};
    }
    if(options.frames==NativeLoadFrames::All){std::vector<std::size_t> indices(samples.size());std::iota(indices.begin(),indices.end(),0);return indices;}
    if(options.frames!=NativeLoadFrames::Maximum)fail(QStringLiteral("未知载荷帧选择模式。"));
    double maxForce=0,maxTorque=0;
    for(const auto& value:samples){maxForce=std::max(maxForce,magnitude(value.forceN));maxTorque=std::max(maxTorque,magnitude(value.torqueNmm));}
    std::size_t best=0;double score=-1;
    for(std::size_t i=0;i<samples.size();++i){
        const double current=(maxForce>0?magnitude(samples[i].forceN)/maxForce:0)
            +(maxTorque>0?magnitude(samples[i].torqueNmm)/maxTorque:0);
        if(current>score){score=current;best=i;}
    }
    return {best};
}

} // namespace

bool nativeLoadSeries(const NativeMotionInput& input,const RigidResult& result,const NativeLoadSource& source,
                      NativeLoadEnd end,std::vector<NativeLoadSample>& samples,QString& error)
{
    try{auto next=series(input,result,source,end);samples=std::move(next);error.clear();return true;}
    catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}

bool exportNativeLoads(const QString& path,const NativeMotionInput& input,const RigidResult& result,
                       const NativeLoadSource& source,const NativeLoadExportOptions& options,QString& error)
{
    try{
        if(!std::isfinite(options.scale)||!std::any_of(options.components.begin(),options.components.end(),[](bool enabled){return enabled;}))
            fail(QStringLiteral("载荷缩放系数无效或未选择任何分量。"));
        if(options.format==NativeLoadFormat::NastranBdf&&options.nodeId<=0)fail(QStringLiteral("BDF 载荷导出需要正的有限元节点编号。"));
        if(options.useTargetPoint&&!vector(options.targetPointMm).allFinite())fail(QStringLiteral("载荷目标点坐标无效。"));
        if(options.targetBodyId < -1 || (options.targetBodyId>=0&&!options.useTargetPoint))
            fail(QStringLiteral("随体载荷目标需要有效的初始节点坐标和运动体编号。"));
        auto values=series(input,result,source,options.end);
        const Vec targetOffset=options.useTargetPoint ? targetBodyOffset(input,result,options) : Vec::Zero();
        for(std::size_t frame=0;frame<values.size();++frame){
            auto& value=values[frame];
            if(options.useTargetPoint){
                Vec target=targetOffset;
                if(options.targetBodyId>=0) {
                    const auto& body=bodyState(result.frames[frame],options.targetBodyId);
                    target=vector(body.positionMm)+quaternion(body.rotation)*targetOffset;
                }
                if(!target.allFinite())fail(QStringLiteral("载荷目标节点的逐帧坐标不是有限值。"));
                value.torqueNmm=array(vector(value.torqueNmm)+(vector(value.pointMm)-target).cross(vector(value.forceN)));
                value.pointMm=array(target);
            }
            for(int component=0;component<3;++component){
                value.forceN[component]*=options.components[component]?options.scale:0;
                value.torqueNmm[component]*=options.components[component+3]?options.scale:0;
            }
            if(!vector(value.forceN).allFinite()||!vector(value.torqueNmm).allFinite())fail(QStringLiteral("缩放后的载荷不是有限值。"));
        }
        const auto frames=selectedFrames(values,options);
        QString text;QTextStream output(&text);
        const auto sourceName=definition(input,source).value("name").toString(QString::number(source.id));
        const auto end=options.end==NativeLoadEnd::I?QStringLiteral("I"):QStringLiteral("J");
        const QStringList labels{"Fx_N","Fy_N","Fz_N","Mx_Nmm","My_Nmm","Mz_Nmm"};
        if(options.format==NativeLoadFormat::Csv){
            QStringList headers{"LoadCase","SourceType","SourceName","FrameIndex","Time_s","NodeID","End","PointX_mm","PointY_mm","PointZ_mm"};
            for(int component=0;component<6;++component)if(options.components[component])headers.append(labels[component]);
            output<<headers.join(',')<<'\n';
            for(auto frame:frames){
                const auto& value=values[frame];
                QStringList row{csv(options.name),csv(source.collection),csv(sourceName),QString::number(frame),number(value.seconds),QString::number(options.nodeId),end,
                    number(value.pointMm[0]),number(value.pointMm[1]),number(value.pointMm[2])};
                for(int component=0;component<6;++component)if(options.components[component])row.append(number(component<3?value.forceN[component]:value.torqueNmm[component-3]));
                output<<row.join(',')<<'\n';
            }
        }else if(options.format==NativeLoadFormat::NastranBdf){
            QString name=options.name;name.replace('\r',' ');name.replace('\n',' ');
            output<<"$ MxSim.Motion constraint loads\n$ LoadCase: "<<name<<"\n$ End: "<<end
                <<"\n$ Units: N, mm, N*mm, s; CID=0\n$ Physical wrench acting on the selected body end.\n"
                <<"$ Moment reference: "<<(options.targetBodyId>=0
                    ? "picked node transformed with body pose each frame"
                    : options.useTargetPoint?"explicit fixed global target point":"selected marker; target grid must be located there")<<'\n';
            if(options.targetBodyId>=0)output<<"$ Target body ID: "<<options.targetBodyId<<'\n';
            for(auto frame:frames){
                const auto& value=values[frame];const auto sid=frame+1;
                output<<"$ Frame "<<frame<<" Time "<<number(value.seconds)<<" Point_mm "
                    <<number(value.pointMm[0])<<' '<<number(value.pointMm[1])<<' '<<number(value.pointMm[2])<<'\n';
                if(std::any_of(options.components.begin(),options.components.begin()+3,[](bool selected){return selected;}))
                    output<<"FORCE,"<<sid<<','<<options.nodeId<<",0,1.0,"<<number(value.forceN[0])<<','<<number(value.forceN[1])<<','<<number(value.forceN[2])<<'\n';
                if(std::any_of(options.components.begin()+3,options.components.end(),[](bool selected){return selected;}))
                    output<<"MOMENT,"<<sid<<','<<options.nodeId<<",0,1.0,"<<number(value.torqueNmm[0])<<','<<number(value.torqueNmm[1])<<','<<number(value.torqueNmm[2])<<'\n';
            }
            output<<"ENDDATA\n";
        }else fail(QStringLiteral("未知载荷输出格式。"));
        output.flush();writeAtomic(path,text.toUtf8());error.clear();return true;
    }catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}

bool buildNativeLoadCases(const NativeMotionInput& input,const RigidResult& result,const NativeLoadCaseOptions& options,
                          QJsonObject& output,QString& error)
{
    try{
        validateResult(input,result);
        if(options.peaksPerSignal<=0||options.peaksPerSignal>32||!std::isfinite(options.mergeTolerance)||options.mergeTolerance<0)
            fail(QStringLiteral("极值数量必须为 1–32，时刻合并容差必须为非负秒数。"));
        if(options.jointIds.empty()||options.analysisBodyId<0||input.document.entity("bodies",options.analysisBodyId).isEmpty())
            fail(QStringLiteral("请选择分析运动体及其关联运动副。"));
        struct Joint {int id;QJsonObject definition;NativeLoadEnd end;std::vector<NativeLoadSample> samples;};
        std::vector<Joint> joints;std::set<int> uniqueIds;
        for(int id:options.jointIds){
            if(!uniqueIds.insert(id).second)fail(QStringLiteral("工况运动副列表含重复编号。"));
            const auto source=definition(input,{"joints",id});NativeLoadEnd end;
            if(source.value("actionBodyId").toInt(-1)==options.analysisBodyId)end=NativeLoadEnd::I;
            else if(source.value("baseBodyId").toInt(-1)==options.analysisBodyId)end=NativeLoadEnd::J;
            else fail(QStringLiteral("运动副 %1 不连接所选分析体。").arg(id));
            joints.push_back({id,source,end,series(input,result,{"joints",id},end)});
        }
        // Candidates are frame indices of strict local extrema. Do not invent
        // an endpoint peak for a flat/monotone record; preserve original rule.
        std::map<std::size_t,std::set<std::size_t>> candidates;
        for(std::size_t ji=0;ji<joints.size();++ji){
            const auto& samples=joints[ji].samples;
            for(int component=0;component<8;++component){
                const auto value=[component](const NativeLoadSample& load){
                    if(component<3)return load.forceN[component];if(component==3)return magnitude(load.forceN);
                    if(component<7)return load.torqueNmm[component-4];return magnitude(load.torqueNmm);
                };
                std::vector<std::pair<double,std::size_t>> peaks;
                for(std::size_t frame=1;frame+1<samples.size();++frame){
                    const double a=value(samples[frame-1]),b=value(samples[frame]),c=value(samples[frame+1]);
                    if((b>a&&b>c)||((component!=3&&component!=7)&&b<a&&b<c))peaks.emplace_back(std::abs(b),frame);
                }
                std::sort(peaks.begin(),peaks.end(),[](const auto& a,const auto& b){return a.first!=b.first?a.first>b.first:a.second<b.second;});
                const auto count=std::min(peaks.size(),static_cast<std::size_t>(options.peaksPerSignal));
                for(std::size_t peak=0;peak<count;++peak)candidates[peaks[peak].second].insert(ji);
            }
        }
        if(candidates.empty())fail(QStringLiteral("已接受结果中没有严格的内部极值点；恒定或单调曲线可使用手动指定帧／最大载荷帧导出。"));
        std::vector<std::pair<std::size_t,std::set<std::size_t>>> merged;
        for(const auto& candidate:candidates){
            if(!merged.empty()&&result.frames[candidate.first].seconds-result.frames[merged.back().first].seconds<=options.mergeTolerance)
                merged.back().second.insert(candidate.second.begin(),candidate.second.end());
            else merged.push_back(candidate);
        }
        const auto& initialBody=bodyState(result.frames.front(),options.analysisBodyId);
        const auto initialQ=quaternion(initialBody.rotation);
        QJsonArray cases;int ordinal=1;
        for(const auto& selected:merged){
            const auto frame=selected.first;const auto& currentBody=bodyState(result.frames[frame],options.analysisBodyId);
            const auto inverse=(quaternion(currentBody.rotation)*initialQ.conjugate()).conjugate();
            QJsonArray loads;
            for(std::size_t ji=0;ji<joints.size();++ji){
                if(!options.synchronized&&!selected.second.count(ji))continue;
                const auto& joint=joints[ji];const auto& load=joint.samples[frame];
                const Vec force=inverse*vector(load.forceN),torque=inverse*vector(load.torqueNmm);
                const Vec actionPoint=vector(initialBody.positionMm)+inverse*(vector(load.pointMm)-vector(currentBody.positionMm));
                if(!force.allFinite()||!torque.allFinite()||!actionPoint.allFinite())fail(QStringLiteral("初始姿态载荷转换产生非有限值。"));
                QJsonObject components{{"FX",force.x()},{"FY",force.y()},{"FZ",force.z()},{"FM",force.norm()},
                    {"TX",torque.x()},{"TY",torque.y()},{"TZ",torque.z()},{"TM",torque.norm()}};
                loads.append(QJsonObject{{"jointId",joint.id},{"jointName",joint.definition.value("name")},
                    {"bodyId",options.analysisBodyId},{"end",joint.end==NativeLoadEnd::I?"I":"J"},
                    {"actionPoint",json(array(actionPoint))},{"components",components}});
            }
            cases.append(QJsonObject{{"name",QStringLiteral("LC_%1").arg(ordinal++)},{"timeSeconds",result.frames[frame].seconds},
                {"frameIndex",static_cast<int>(frame)},{"loads",loads}});
        }
        const auto parts=[&](int id){QJsonArray values;for(int part:input.document.bodyPartBindings.value(id))values.append(part);return values;};
        std::set<int> bodyIds{options.analysisBodyId};QJsonArray jointData;
        for(const auto& joint:joints){
            const int action=joint.definition.value("actionBodyId").toInt(-1),base=joint.definition.value("baseBodyId").toInt(-1);
            bodyIds.insert(action);bodyIds.insert(base);
            jointData.append(QJsonObject{{"id",joint.id},{"name",joint.definition.value("name")},{"type",joint.definition.value("jointType")},
                {"actionBodyId",action},{"baseBodyId",base},{"actionBodyPartIds",parts(action)},{"baseBodyPartIds",parts(base)},
                {"actionPoint",json(joint.samples.front().pointMm)}});
        }
        QJsonArray bodies;
        for(int id:bodyIds)if(id>=0){const auto body=input.document.entity("bodies",id);bodies.append(QJsonObject{{"id",id},{"name",body.value("name")},{"partIds",parts(id)},{"isFixed",body.value("isFixed")}});}
        const auto analysis=input.document.entity("bodies",options.analysisBodyId);
        QJsonObject source{{"scenarioId",input.scenario.id},{"scenarioName",input.scenario.name},
            {"analysisBodyId",options.analysisBodyId},{"analysisBodyName",analysis.value("name")},
            {"peaksPerSignal",options.peaksPerSignal},{"mergeTolerance",options.mergeTolerance}};
        QJsonObject info{{"format","MxMotionLoadCases"},{"version",1},
            {"method",options.synchronized?"synchronizedLinkLoads":"extremeJointsOnly"},
            {"units","length=mm; force=N; moment=N*mm; time=s"},
            {"unitSystem",QJsonObject{{"length","mm"},{"force","N"},{"moment","N*mm"},{"time","s"}}},
            {"coordinateSystem","bodyInitial"},{"source",source},
            {"forceConvention","Physical constraint wrench acting on analysisBodyId; moment about each load.actionPoint; I/J action-reaction and lever-arm transfer applied."},
            {"description",options.synchronized?"All selected joint loads evaluated at each merged extremum time.":"Only joints contributing an extremum evaluated at each merged time."}};
        output=QJsonObject{{"info",info},{"bodies",bodies},{"joints",jointData},{"cases",cases}};error.clear();return true;
    }catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}

bool exportNativeLoadCases(const QString& path,const NativeMotionInput& input,const RigidResult& result,
                           const NativeLoadCaseOptions& options,QString& error)
{
    QJsonObject document;if(!buildNativeLoadCases(input,result,options,document,error))return false;
    try{writeAtomic(path,QJsonDocument(document).toJson(QJsonDocument::Indented));error.clear();return true;}
    catch(const std::exception& exception){error=QString::fromUtf8(exception.what());return false;}
}

} // namespace MotionOcc
