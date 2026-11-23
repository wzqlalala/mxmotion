#include "OriginalSelection.h"
#include "MotionWindow.h"
#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "compat/picking/OccPickController.h"
#include "compat/picking/LegacyPickContext.h"
#include "mGeneralPickUp.h"

#include <QRegularExpression>
#include <QTimer>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <Geom_Surface.hxx>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <tuple>

namespace MotionOcc {
namespace {
using Type = MxFunctions::PickDataType;
using Controller = MechMainWindow::mPickWidgetController;
using Index = std::uint32_t;
using Edge = std::array<Index, 2>;
constexpr double pi = 3.14159265358979323846;

Type legacyType(PickKind kind)
{
    switch (kind) {
    case PickKind::Vertex: return Type::GeoPoint;
    case PickKind::Edge: return Type::GeoEdge;
    case PickKind::Face: return Type::GeoFace;
    case PickKind::Solid: return Type::GeoSolid;
    case PickKind::Node: return Type::Node;
    case PickKind::MeshEdge: return Type::ElementEdge;
    case PickKind::MeshFace: return Type::ElementFace;
    case PickKind::Element: return Type::Element;
    default: return Type::Part;
    }
}

TopAbs_ShapeEnum shapeType(PickKind kind)
{
    switch (kind) {
    case PickKind::Vertex: return TopAbs_VERTEX;
    case PickKind::Edge: return TopAbs_EDGE;
    case PickKind::Face: return TopAbs_FACE;
    case PickKind::Solid: return TopAbs_SOLID;
    default: return TopAbs_SHAPE;
    }
}

bool geometryKind(PickKind kind)
{
    return kind == PickKind::Vertex || kind == PickKind::Edge
        || kind == PickKind::Face || kind == PickKind::Solid;
}

TopoDS_Shape pickedShape(const CadDocument& document, const PickResult& pick)
{
    const auto* part = document.part(pick.partId);
    if (!part) return {};
    if (pick.kind == PickKind::Part) return part->shape;
    if (!geometryKind(pick.kind)) return {};
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes(part->shape, shapeType(pick.kind), map);
    return pick.index > 0 && pick.index <= map.Extent() ? map(pick.index) : TopoDS_Shape{};
}

QVector3D position(const std::array<double, 3>& point)
{
    return {float(point[0]), float(point[1]), float(point[2])};
}

bool samePick(const PickResult& a, const PickResult& b)
{
    return a.partId == b.partId && a.kind == b.kind && a.index == b.index;
}

void unique(QVector<PickResult>& picks)
{
    std::set<std::tuple<int, int, int>> seen;
    picks.erase(std::remove_if(picks.begin(), picks.end(), [&seen](const PickResult& p) {
        return !p.isValid() || !seen.emplace(p.partId, int(p.kind), p.index).second;
    }), picks.end());
}

double scaleTolerance(const CadDocument& document)
{
    Bnd_Box box;
    for (const auto& part : document.parts()) BRepBndLib::Add(part.shape, box);
    if (box.IsVoid()) return 1e-7;
    double x0, y0, z0, x1, y1, z1;
    box.Get(x0, y0, z0, x1, y1, z1);
    return std::max(1e-7, std::sqrt((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0)+(z1-z0)*(z1-z0))*1e-7);
}

std::set<Edge> meshEdges(const FemMesh& mesh)
{
    std::set<Edge> edges;
    const auto add = [&edges](Index a, Index b) { edges.insert({std::min(a,b), std::max(a,b)}); };
    for (const auto& face : mesh.surface) for (int i=0; i<3; ++i) add(face[i], face[(i+1)%3]);
    for (const auto& cell : mesh.tetrahedra) for (int i=0; i<4; ++i) for (int j=i+1; j<4; ++j) add(cell[i], cell[j]);
    return edges;
}

Controller* selectionController(MotionWindow& window, LegacyPickContext* context)
{
    if (auto* active = Controller::activeController(); active && active->context() == context) return active;
    auto* controller = window.findChild<Controller*>(QStringLiteral("originalWorkspaceSelection"));
    if (!controller) {
        controller = MechMainWindow::mGeneralPickUp::createMorePickWidget(MechMainWindow::CBRed, &window);
        controller->setObjectName(QStringLiteral("originalWorkspaceSelection"));
        controller->widget()->hide();
        controller->setAllowedTypes([] { return QSet<Type>{Type::GeoPoint, Type::GeoEdge, Type::GeoFace,
            Type::GeoSolid, Type::Node, Type::ElementEdge, Type::ElementFace, Type::Element, Type::Part}; });
    }
    if(controller->property("workspacePickContext").value<QObject*>()!=context) {
        controller->setPickActive(false);
        controller->displayData(MechMainWindow::PickWidgetData{});
        controller->setContext(context);
        controller->setProperty("workspacePickContext",QVariant::fromValue(static_cast<QObject*>(context)));
        QObject::connect(context,&QObject::destroyed,controller,[controller,context] {
            if(controller->property("workspacePickContext").value<QObject*>()==context) {
                controller->setProperty("workspacePickContext",QVariant{});
                controller->displayData(MechMainWindow::PickWidgetData{});
            }
        });
    }
    controller->setPickActive(true);
    return controller;
}

QVector<PickResult> candidates(const CadDocument& document, const std::map<int,FemMesh>& meshes,
    LegacyPickContext& context, PickKind kind, const std::set<int>& hidden, const std::set<int>& allowed)
{
    QVector<PickResult> result;
    const Type type = legacyType(kind);
    for (const auto& part : document.parts()) {
        if (hidden.count(part.id) || (!allowed.empty() && !allowed.count(part.id))) continue;
        if (kind == PickKind::Part) {
            PickResult pick = context.resolve(type, RenderPickResultData(part.id, -1, part.id));
            if (pick.isValid()) result.append(pick);
        } else if (geometryKind(kind)) {
            TopTools_IndexedMapOfShape map;
            TopExp::MapShapes(part.shape, shapeType(kind), map);
            for (int i=1; i<=map.Extent(); ++i) {
                PickResult ref; ref.kind=kind; ref.partId=part.id; ref.index=i;
                const auto value = context.toLegacy(ref, type);
                auto pick = context.resolve(type, value.second);
                if (pick.isValid()) result.append(pick);
            }
        } else {
            const auto found = meshes.find(part.id);
            if (found == meshes.end()) continue;
            const auto& mesh = found->second;
            const auto ids = context.nodeIds(part.id);
            if (kind == PickKind::Node) {
                for (int i=0; i<ids.size(); ++i) {
                    auto pick=context.resolve(type, RenderPickResultData(ids[i],-1,part.id));
                    if (pick.isValid()) result.append(pick);
                }
            } else if (kind == PickKind::MeshEdge) {
                for (const auto& edge : meshEdges(mesh)) {
                    auto pick=context.resolve(type, RenderPickResultData(ids.value(edge[0],-1),ids.value(edge[1],-1),part.id));
                    if (pick.isValid()) result.append(pick);
                }
            } else {
                const int count = kind == PickKind::Element && !mesh.tetrahedra.empty()
                    ? int(mesh.tetrahedra.size()) : int(mesh.surface.size());
                for (int i=1; i<=count; ++i) {
                    auto pick=context.resolve(type, RenderPickResultData(i,-1,part.id));
                    if (pick.isValid()) result.append(pick);
                }
            }
        }
    }
    return result;
}

QVector<int> nodes(const PickResult& pick)
{
    return pick.kind == PickKind::Node ? QVector<int>{pick.nodeId} : pick.nodeIds;
}

bool containsShape(const TopoDS_Shape& owner, const TopoDS_Shape& part)
{
    if (owner.IsNull() || part.IsNull()) return false;
    if (owner.IsSame(part)) return true;
    TopTools_IndexedMapOfShape map; TopExp::MapShapes(owner, part.ShapeType(), map);
    return map.Contains(part);
}

bool pointOnShape(const gp_Pnt& point, const TopoDS_Shape& shape, double tolerance)
{
    if (shape.ShapeType() == TopAbs_SOLID) {
        BRepClass3d_SolidClassifier classifier(TopoDS::Solid(shape), point, tolerance);
        return classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON;
    }
    BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(point).Shape(), shape);
    return distance.IsDone() && distance.Value() <= tolerance;
}

bool related(const CadDocument& document, const std::map<int,FemMesh>& meshes, LegacyPickContext& context,
    const PickResult& source, const PickResult& target, double tolerance)
{
    if (source.partId != target.partId) return false;
    if (source.kind == PickKind::Part || target.kind == PickKind::Part) return true;
    const auto sourceShape = pickedShape(document, source), targetShape = pickedShape(document, target);
    if (!sourceShape.IsNull() && !targetShape.IsNull())
        return containsShape(sourceShape, targetShape) || containsShape(targetShape, sourceShape);
    if (sourceShape.IsNull() && targetShape.IsNull()) {
        const auto a=nodes(source), b=nodes(target);
        return !a.isEmpty() && !b.isEmpty()
            && (std::all_of(a.begin(),a.end(),[&](int id){return b.contains(id);})
                || std::all_of(b.begin(),b.end(),[&](int id){return a.contains(id);}));
    }
    const auto& shape = sourceShape.IsNull() ? targetShape : sourceShape;
    const auto& meshPick = sourceShape.IsNull() ? source : target;
    const auto found = meshes.find(meshPick.partId);
    if (found == meshes.end()) return false;
    const auto ids=context.nodeIds(meshPick.partId), selected=nodes(meshPick);
    if (selected.isEmpty()) return false;
    return std::all_of(selected.begin(),selected.end(),[&](int id) {
        const int index=ids.indexOf(id);
        if (index<0 || index>=int(found->second.nodes.size())) return false;
        const auto& xyz=found->second.nodes[std::size_t(index)];
        return pointOnShape(gp_Pnt(xyz[0],xyz[1],xyz[2]),shape,tolerance);
    });
}

double measure(const CadDocument& document, const PickResult& pick)
{
    auto shape=pickedShape(document,pick);
    if (shape.IsNull()) return 0;
    GProp_GProps props;
    if (pick.kind==PickKind::Edge) BRepGProp::LinearProperties(shape,props);
    else if (pick.kind==PickKind::Face) BRepGProp::SurfaceProperties(shape,props);
    else BRepGProp::VolumeProperties(shape,props);
    return std::abs(props.Mass());
}

gp_Vec faceNormal(const TopoDS_Face& face, const gp_Pnt& point)
{
    const auto surface=BRep_Tool::Surface(face);
    GeomAPI_ProjectPointOnSurf projection(point,surface);
    double u=0,v=0;
    if (projection.NbPoints()>0) projection.LowerDistanceParameters(u,v);
    else { double u1,u2,v1,v2; BRepTools::UVBounds(face,u1,u2,v1,v2); u=(u1+u2)*0.5;v=(v1+v2)*0.5; }
    gp_Pnt at;gp_Vec du,dv;surface->D1(u,v,at,du,dv);
    gp_Vec normal=du.Crossed(dv);
    if (face.Orientation()==TopAbs_REVERSED) normal.Reverse();
    if (normal.SquareMagnitude()>1e-24) normal.Normalize();
    return normal;
}

bool adjacentGeometry(const TopoDS_Shape& a, const TopoDS_Shape& b, double maxTurn,
    int concavity = 0)
{
    if (a.IsSame(b)) return false;
    if (a.ShapeType()==TopAbs_FACE && b.ShapeType()==TopAbs_FACE) {
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(b,TopAbs_EDGE,edges);
        for (TopExp_Explorer it(a,TopAbs_EDGE);it.More();it.Next()) if (edges.Contains(it.Current())) {
            const auto edge=TopoDS::Edge(it.Current());
            if (BRep_Tool::Degenerated(edge)) continue;
            BRepAdaptor_Curve curve(edge);gp_Pnt mid;gp_Vec tangent;
            curve.D1((curve.FirstParameter()+curve.LastParameter())*0.5,mid,tangent);
            if (edge.Orientation()==TopAbs_REVERSED) tangent.Reverse();
            const auto na=faceNormal(TopoDS::Face(a),mid),nb=faceNormal(TopoDS::Face(b),mid);
            if (na.SquareMagnitude()<1e-24 || nb.SquareMagnitude()<1e-24) continue;
            const double turn=std::acos(std::clamp(na.Dot(nb),-1.0,1.0))*180.0/pi;
            if (concavity!=0) {
                const double sign=tangent.Dot(na.Crossed(nb));
                if (turn<1e-5 || (concavity<0 ? sign<-1e-10 : sign>1e-10)) return true;
            } else if (turn<=maxTurn+1e-7) return true;
        }
    } else if (a.ShapeType()==TopAbs_EDGE && b.ShapeType()==TopAbs_EDGE) {
        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(b,TopAbs_VERTEX,vertices);
        for (TopExp_Explorer it(a,TopAbs_VERTEX);it.More();it.Next()) if (vertices.Contains(it.Current())) {
            const auto at=BRep_Tool::Pnt(TopoDS::Vertex(it.Current()));
            const auto away=[&](const TopoDS_Shape& shape) {
                BRepAdaptor_Curve curve(TopoDS::Edge(shape));
                const bool first=at.Distance(curve.Value(curve.FirstParameter()))<at.Distance(curve.Value(curve.LastParameter()));
                gp_Pnt p;gp_Vec tangent;curve.D1(first?curve.FirstParameter():curve.LastParameter(),p,tangent);
                if(!first)tangent.Reverse();if(tangent.SquareMagnitude()>1e-24)tangent.Normalize();return tangent;
            };
            const auto ta=away(a),tb=away(b);
            const double turn=std::acos(std::clamp(-ta.Dot(tb),-1.0,1.0))*180.0/pi;
            if(turn<=maxTurn+1e-7)return true;
        }
    }
    return false;
}

QVector<PickResult> extendGeometry(const CadDocument& document, const QVector<PickResult>& selected,
    const QVector<PickResult>& all, bool recursive, double maxTurn, int concavity=0)
{
    QVector<PickResult> result=selected;
    std::vector<TopoDS_Shape> shapes;shapes.reserve(std::size_t(all.size()));
    for(const auto& pick:all)shapes.push_back(pickedShape(document,pick));
    for(int i=0;i<(recursive?result.size():selected.size());++i) {
        const auto seed=result[i];const auto shape=pickedShape(document,seed);
        if(shape.IsNull())continue;
        for(int j=0;j<all.size();++j) {
            const auto& candidate=all[j];
            if(candidate.partId!=seed.partId || std::any_of(result.begin(),result.end(),[&](const auto& p){return samePick(p,candidate);}))continue;
            if(adjacentGeometry(shape,shapes[std::size_t(j)],maxTurn,concavity))result.append(candidate);
        }
    }
    return result;
}

bool idMatches(int id, const QVector<std::array<int,3>>& ranges)
{
    return std::any_of(ranges.begin(),ranges.end(),[id](const auto& r) {return id>=r[0]&&id<=r[1]&&(id-r[0])%r[2]==0;});
}

bool parseIds(const QString& text, QVector<std::array<int,3>>& ranges)
{
    const QRegularExpression expression(QStringLiteral("^\\s*(\\d+)\\s*(?::\\s*(\\d+)\\s*(?::\\s*(\\d+)\\s*)?)?$"));
    for(const auto& token:text.split(QLatin1Char(','),Qt::KeepEmptyParts)) {
        const auto match=expression.match(token);if(!match.hasMatch())return false;
        bool good=false;const int first=match.captured(1).toInt(&good);if(!good||first<1)return false;
        const int last=match.captured(2).isEmpty()?first:match.captured(2).toInt(&good);if(!good||last<first)return false;
        const int step=match.captured(3).isEmpty()?1:match.captured(3).toInt(&good);if(!good||step<1)return false;
        ranges.append({first,last,step});
    }
    return !ranges.isEmpty();
}

QVector<PickResult> meshPath(const std::map<int,FemMesh>& meshes, LegacyPickContext& context,
    const QVector<PickResult>& selected)
{
    if(selected.size()<2)return selected;
    const auto start=selected[selected.size()-2],end=selected.back();
    if(start.kind!=PickKind::Node||end.kind!=PickKind::Node||start.partId!=end.partId)return selected;
    const auto found=meshes.find(start.partId);if(found==meshes.end())return selected;
    const auto& mesh=found->second;const auto ids=context.nodeIds(start.partId);
    if(start.index<0||end.index<0||start.index>=int(mesh.nodes.size())||end.index>=int(mesh.nodes.size()))return selected;
    std::vector<std::vector<std::pair<int,double>>> neighbors(mesh.nodes.size());
    // The original path is traced on the real surface mesh, not through volume diagonals.
    std::set<Edge> edges;for(const auto& face:mesh.surface)for(int i=0;i<3;++i)edges.insert({std::min(face[i],face[(i+1)%3]),std::max(face[i],face[(i+1)%3])});
    for(const auto& edge:edges) {
        if(edge[0]>=mesh.nodes.size()||edge[1]>=mesh.nodes.size())continue;
        const double distance=(position(mesh.nodes[edge[0]])-position(mesh.nodes[edge[1]])).length();
        neighbors[edge[0]].push_back({int(edge[1]),distance});neighbors[edge[1]].push_back({int(edge[0]),distance});
    }
    using Item=std::pair<double,int>;std::priority_queue<Item,std::vector<Item>,std::greater<Item>> queue;
    std::vector<double> distances(mesh.nodes.size(),std::numeric_limits<double>::infinity());
    std::vector<int> previous(mesh.nodes.size(),-1);distances[std::size_t(start.index)]=0;queue.push({0,start.index});
    while(!queue.empty()) {
        const auto [distance,index]=queue.top();queue.pop();
        if(distance>distances[std::size_t(index)])continue;if(index==end.index)break;
        for(const auto& [next,length]:neighbors[std::size_t(index)]) if(distance+length<distances[std::size_t(next)]) {
            distances[std::size_t(next)]=distance+length;previous[std::size_t(next)]=index;queue.push({distance+length,next});
        }
    }
    if(!std::isfinite(distances[std::size_t(end.index)]))return selected;
    QVector<PickResult> result;
    for(int index=end.index;index!=-1;index=previous[std::size_t(index)]) {
        result.prepend(context.resolve(Type::Node,RenderPickResultData(ids.value(index,-1),-1,start.partId)));
        if(index==start.index)break;
    }
    return result;
}

QVector<PickResult> meshAngle(const std::map<int,FemMesh>& meshes,LegacyPickContext& context,
    const QVector<PickResult>& selected,bool faceMode,double threshold)
{
    if(selected.isEmpty())return selected;
    const auto seed=selected.back();const auto found=meshes.find(seed.partId);if(found==meshes.end())return selected;
    const auto& mesh=found->second;const auto ids=context.nodeIds(seed.partId);
    if(!faceMode) {
        if(seed.kind!=PickKind::Node)return selected;
        std::map<Index,std::set<Index>> adjacent;
        std::map<Edge,int> incidence;
        for(const auto& face:mesh.surface)for(int i=0;i<3;++i)++incidence[{std::min(face[i],face[(i+1)%3]),std::max(face[i],face[(i+1)%3])}];
        for(const auto& [edge,count]:incidence) {
            if(count!=1)continue;adjacent[edge[0]].insert(edge[1]);adjacent[edge[1]].insert(edge[0]);
        }
        std::set<Index> visited{Index(seed.index)};std::queue<std::pair<Index,Index>> queue;
        for(Index next:adjacent[Index(seed.index)])queue.push({Index(seed.index),next});
        while(!queue.empty()) {
            const auto [before,current]=queue.front();queue.pop();if(!visited.insert(current).second)continue;
            const auto incoming=(position(mesh.nodes[current])-position(mesh.nodes[before])).normalized();
            for(Index next:adjacent[current])if(!visited.count(next)) {
                const auto outgoing=(position(mesh.nodes[next])-position(mesh.nodes[current])).normalized();
                const double angle=std::acos(std::clamp(double(QVector3D::dotProduct(incoming,outgoing)),-1.0,1.0))*180/pi;
                if(angle<=threshold+1e-7)queue.push({current,next});
            }
        }
        QVector<PickResult> result=selected;
        for(Index node:visited)result.append(context.resolve(Type::Node,RenderPickResultData(ids.value(node,-1),-1,seed.partId)));
        return result;
    }
    std::vector<QVector3D> normals;std::map<Edge,std::vector<int>> edgeFaces;
    for(int i=0;i<int(mesh.surface.size());++i) {
        const auto& face=mesh.surface[std::size_t(i)];
        normals.push_back(QVector3D::crossProduct(position(mesh.nodes[face[1]])-position(mesh.nodes[face[0]]),position(mesh.nodes[face[2]])-position(mesh.nodes[face[0]])).normalized());
        for(int j=0;j<3;++j)edgeFaces[{std::min(face[j],face[(j+1)%3]),std::max(face[j],face[(j+1)%3])}].push_back(i);
    }
    std::set<int> visited;std::queue<int> queue;
    const auto seedNodes=nodes(seed);
    for(int i=0;i<int(mesh.surface.size());++i) {
        if(seed.kind==PickKind::MeshFace && seed.index==i+1)queue.push(i);
        else if(seed.kind!=PickKind::MeshFace) {
            const auto& face=mesh.surface[std::size_t(i)];
            if(std::any_of(face.begin(),face.end(),[&](Index index){return seedNodes.contains(ids.value(index,-1));})) {queue.push(i);break;}
        }
    }
    while(!queue.empty()) {
        const int i=queue.front();queue.pop();if(!visited.insert(i).second)continue;
        const auto& face=mesh.surface[std::size_t(i)];
        for(int j=0;j<3;++j)for(int next:edgeFaces[{std::min(face[j],face[(j+1)%3]),std::max(face[j],face[(j+1)%3])}]) {
            const double angle=std::acos(std::clamp(double(QVector3D::dotProduct(normals[std::size_t(i)],normals[std::size_t(next)])),-1.0,1.0))*180/pi;
            if(!visited.count(next)&&angle<=threshold+1e-7)queue.push(next);
        }
    }
    QVector<PickResult> result=selected;
    for(int i:visited) {
        if(seed.kind==PickKind::Node)for(Index node:mesh.surface[std::size_t(i)])result.append(context.resolve(Type::Node,RenderPickResultData(ids.value(node,-1),-1,seed.partId)));
        else if(seed.kind==PickKind::MeshFace)result.append(context.resolve(Type::ElementFace,RenderPickResultData(i+1,-1,seed.partId)));
        else if(seed.kind==PickKind::Element) {
            if(mesh.tetrahedra.empty())result.append(context.resolve(Type::Element,RenderPickResultData(i+1,-1,seed.partId)));
            else for(int j=0;j<int(mesh.tetrahedra.size());++j) {
                const auto& cell=mesh.tetrahedra[std::size_t(j)];const auto& face=mesh.surface[std::size_t(i)];
                if(std::all_of(face.begin(),face.end(),[&](Index n){return std::find(cell.begin(),cell.end(),n)!=cell.end();}))
                    result.append(context.resolve(Type::Element,RenderPickResultData(j+1,-1,seed.partId)));
            }
        }
    }
    return result;
}
}

bool dispatchOriginalSelection(MotionWindow& window, OccViewport& viewport,
    const CadDocument& document, const std::map<int,FemMesh>& meshes,const std::set<int>& hidden,
    int selectedPart,const QString& action,const QVariant& argument,QString& message)
{
    static const QStringList actions{
        "activePickSig","clearPick","slot_reverse","slot_nodeIDSelect","slot_eleIDSelect",
        "slot_mappingGeoPoint","slot_mappingGeoEdge","slot_mappingGeoFace","slot_mappingGeoSolid",
        "slot_mappingNode","slot_mappingMeshLine","slot_mappingMeshFace","slot_mappingEle","slot_mappingPart",
        "slot_adjacentFaceLine","slot_extendConnectFace","slot_faceCommonEdge","slot_cylinderFaceSelect",
        "slot_sunkenRaisedFaceSelect","slot_sameLocationGeoSelect","slot_sameLocationFaceGeoSelect",
        "slot_sizeGeoSelect","slot_growEles","setPickAssistMode","setPickAssistLineAngle","setPickAssistFaceAngle","applyPickAssist"};
    if(!actions.contains(action))return false;
    if(action==QLatin1String("setPickAssistLineAngle")||action==QLatin1String("setPickAssistFaceAngle")) {
        window.setProperty(action.toLatin1().constData(),argument.toDouble());return true;
    }
    if(action==QLatin1String("setPickAssistMode")) {
        window.setProperty("originalPickAssistMode",argument.toString());
        if(!viewport.property("originalAssistConnected").toBool()) {
            viewport.setProperty("originalAssistConnected",true);
            QObject::connect(&viewport,&OccViewport::pickSelectionChanged,&window,[&window] {
                if(window.property("originalPickAssistMode").toString()==QLatin1String("None"))return;
                QTimer::singleShot(0,&window,[&window]{window.dispatchOriginalMotionAction(QStringLiteral("applyPickAssist"));});
            });
        }
        return true;
    }
    auto* context=LegacyPickContext::forObject(&window,false);
    // Geometry selection is also available immediately after importing CAD,
    // before any motion body or solver document has been created.
    if(!context)context=new LegacyPickContext(&window,&viewport,document,&meshes);
    auto* controller=selectionController(window,context);
    auto selected=context->resolve(controller->getAllData().pickedData);
    if(selected.isEmpty()&&selectedPart>0) {
        PickResult part=context->resolve(Type::Part,RenderPickResultData(selectedPart,-1,selectedPart));
        if(part.isValid())selected.append(part);
    }
    Type type=context->pickType();
    PickKind kind=LegacyPickContext::toKind(type);
    const auto publish=[&](QVector<PickResult> result,Type resultType) {
        if(!controller->allowedTypes().isEmpty()&&!controller->allowedTypes().contains(resultType)) {
            message=QStringLiteral("当前拾取控件不接受%1；请选择对应的拾取控件。").arg(MechMainWindow::PickDataTypeToString(resultType));return;
        }
        unique(result);LegacyPickMap data;
        for(const auto& pick:result) {
            const auto converted=context->toLegacy(pick,resultType);
            if(converted.first!=Type::None)data[converted.first].push_back(converted.second);
        }
        context->setPickType(resultType);
        controller->replacePickedDataAndClearSets(data);
        message=QStringLiteral("已选择 %1 个%2。").arg(result.size()).arg(MechMainWindow::PickDataTypeToString(resultType));
    };
    try {
        if(action==QLatin1String("activePickSig")) {
            const int value=argument.toInt();if(value>=int(Type::GeoPoint)&&value<=int(Type::Part)) {
                const auto requested=static_cast<Type>(value);
                if(controller->allowedTypes().isEmpty()||controller->allowedTypes().contains(requested))context->setPickType(requested);
                else message=QStringLiteral("当前拾取控件不接受此实体类型。");
            }
            return true;
        }
        if(action==QLatin1String("clearPick")){controller->replacePickedDataAndClearSets({});return true;}
        const auto available=[&](PickKind candidateKind){return candidates(document,meshes,*context,candidateKind,hidden,controller->allowedParts());};
        auto all=available(kind);QVector<PickResult> result;
        if(action==QLatin1String("slot_reverse")) {
            for(const auto& candidate:all)if(std::none_of(selected.begin(),selected.end(),[&](const auto& p){return samePick(candidate,p);}))result.append(candidate);
        } else if(action==QLatin1String("slot_nodeIDSelect")||action==QLatin1String("slot_eleIDSelect")) {
            QVector<std::array<int,3>> ranges;
            if(!parseIds(argument.toString(),ranges)){message=QStringLiteral("请输入编号或范围，例如 1,3:12,20:40:2。");return true;}
            kind=action==QLatin1String("slot_nodeIDSelect")?PickKind::Node:PickKind::Element;type=legacyType(kind);
            for(const auto& candidate:available(kind))if(idMatches(kind==PickKind::Node?candidate.nodeId:candidate.index,ranges))result.append(candidate);
        } else if(action.startsWith(QLatin1String("slot_mapping"))) {
            const std::map<QString,PickKind> kinds{{"slot_mappingGeoPoint",PickKind::Vertex},{"slot_mappingGeoEdge",PickKind::Edge},
                {"slot_mappingGeoFace",PickKind::Face},{"slot_mappingGeoSolid",PickKind::Solid},{"slot_mappingNode",PickKind::Node},
                {"slot_mappingMeshLine",PickKind::MeshEdge},{"slot_mappingMeshFace",PickKind::MeshFace},{"slot_mappingEle",PickKind::Element},{"slot_mappingPart",PickKind::Part}};
            kind=kinds.at(action);type=legacyType(kind);const double tolerance=scaleTolerance(document);
            for(const auto& candidate:available(kind))if(std::any_of(selected.begin(),selected.end(),[&](const auto& p){return related(document,meshes,*context,p,candidate,tolerance);}))result.append(candidate);
        } else if(action==QLatin1String("slot_cylinderFaceSelect")) {
            type=Type::GeoFace;
            for(const auto& candidate:available(PickKind::Face))if(BRepAdaptor_Surface(TopoDS::Face(pickedShape(document,candidate))).GetType()==GeomAbs_Cylinder)result.append(candidate);
        } else if(action==QLatin1String("slot_faceCommonEdge")) {
            type=Type::GeoEdge;
            for(const auto& candidate:available(PickKind::Edge)) {
                const auto edge=pickedShape(document,candidate);int references=0;
                for(const auto& seed:selected)if(seed.kind==PickKind::Face&&seed.partId==candidate.partId&&containsShape(pickedShape(document,seed),edge))++references;
                if(references>=2)result.append(candidate);
            }
        } else if(action==QLatin1String("slot_adjacentFaceLine")||action==QLatin1String("slot_extendConnectFace")||action==QLatin1String("slot_sunkenRaisedFaceSelect")) {
            if(kind!=PickKind::Face&&kind!=PickKind::Edge){message=QStringLiteral("请先拾取几何面或几何线。");return true;}
            const bool connected=action==QLatin1String("slot_extendConnectFace"),sunken=action==QLatin1String("slot_sunkenRaisedFaceSelect");
            result=extendGeometry(document,selected,all,connected||sunken||argument.toBool(),connected?180.0:30.0,sunken?(argument.toBool()?-1:1):0);
        } else if(action==QLatin1String("slot_sizeGeoSelect")) {
            const bool range=argument.type()==QVariant::List;const auto args=argument.toList();
            if(range&&args.size()<4)return true;
            int compare=range?args[1].toInt():argument.toInt();
            if(range){kind=args[0].toString()==QLatin1String("几何线")?PickKind::Edge:args[0].toString()==QLatin1String("几何面")?PickKind::Face:PickKind::Solid;type=legacyType(kind);all=available(kind);}
            std::vector<double> references;
            if(range)references.push_back(args[3].toDouble());else for(const auto& seed:selected)if(seed.kind==kind)references.push_back(measure(document,seed));
            if(references.empty()){message=QStringLiteral("请先拾取用于比较尺寸的几何实体。");return true;}
            for(const auto& candidate:all) {
                const double value=measure(document,candidate);
                const bool accepted=std::any_of(references.begin(),references.end(),[&](double ref){return compare==0?value>ref:compare==1?std::abs(value-ref)<=std::max(1e-12,std::abs(ref)*0.001):compare==2?value<ref:value>=args[2].toDouble()&&value<=ref;});
                if(accepted)result.append(candidate);
            }
        } else if(action==QLatin1String("slot_sameLocationGeoSelect")||action==QLatin1String("slot_sameLocationFaceGeoSelect")) {
            if(selected.size()!=1){message=QStringLiteral("请拾取一个定位基准实体。");return true;}
            const int axis=argument.toInt();if(axis<0||axis>2)return true;const double tolerance=scaleTolerance(document);
            const bool coplanar=action==QLatin1String("slot_sameLocationFaceGeoSelect");
            auto commonFaces=coplanar?available(PickKind::Face):QVector<PickResult>{};
            if(coplanar)commonFaces.erase(std::remove_if(commonFaces.begin(),commonFaces.end(),[&](const auto& p){return !related(document,meshes,*context,selected[0],p,tolerance);}),commonFaces.end());
            for(const auto& candidate:all)if(std::abs(candidate.position[axis]-selected[0].position[axis])<=tolerance) {
                if(!coplanar||std::any_of(commonFaces.begin(),commonFaces.end(),[&](const auto& face){return related(document,meshes,*context,candidate,face,tolerance);}))result.append(candidate);
            }
        } else if(action==QLatin1String("slot_growEles")) {
            if(kind!=PickKind::Element){message=QStringLiteral("请先拾取单元。");return true;}
            for(const auto& candidate:all)if(std::any_of(selected.begin(),selected.end(),[&](const auto& seed){return seed.partId==candidate.partId&&std::any_of(seed.nodeIds.begin(),seed.nodeIds.end(),[&](int node){return candidate.nodeIds.contains(node);});}))result.append(candidate);
        } else if(action==QLatin1String("applyPickAssist")) {
            const auto mode=window.property("originalPickAssistMode").toString();
            if(mode.isEmpty()||mode==QLatin1String("None")||selected.isEmpty())return true;
            if(mode==QLatin1String("Path"))result=meshPath(meshes,*context,selected);
            else {
                const bool faceMode=mode==QLatin1String("FaceAngle");
                const auto stored=window.property(faceMode?"setPickAssistFaceAngle":"setPickAssistLineAngle");
                const double angle=stored.isValid()?stored.toDouble():60.0;
                if(kind==PickKind::Face||kind==PickKind::Edge)result=extendGeometry(document,selected,all,true,angle);
                else result=meshAngle(meshes,*context,selected,faceMode,angle);
            }
        }
        publish(result,type);
    } catch(const Standard_Failure& error) {message=QStringLiteral("几何选择失败：%1").arg(QString::fromUtf8(error.GetMessageString()));}
    return true;
}
}
