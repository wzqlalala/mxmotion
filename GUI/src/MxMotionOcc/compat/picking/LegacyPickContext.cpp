#include "LegacyPickContext.h"
#include "OccPickController.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <Standard_Failure.hxx>
#include <QVariant>
#include <algorithm>
#include <cmath>
#include <tuple>

namespace MotionOcc {
namespace {
using Type = MxFunctions::PickDataType;
using Key = std::tuple<Type,int,int,int>;
std::array<double,3> xyz(const gp_Pnt& p) { return {p.X(),p.Y(),p.Z()}; }
QVector3D qvec(const std::array<double,3>& p) { return {float(p[0]),float(p[1]),float(p[2])}; }
int category(Type type)
{
    switch (type) { case Type::GeoPoint: return 0; case Type::GeoEdge: return 1;
    case Type::GeoFace: return 2; case Type::GeoSolid: return 3; default: return -1; }
}
Type typeOf(PickKind kind)
{
    switch (kind) { case PickKind::Vertex: return Type::GeoPoint; case PickKind::Edge: return Type::GeoEdge;
    case PickKind::Face: return Type::GeoFace; case PickKind::Node: return Type::Node;
    case PickKind::MeshEdge: return Type::ElementEdge; case PickKind::MeshFace: return Type::ElementFace;
    case PickKind::Element: return Type::Element; case PickKind::Solid: return Type::GeoSolid; default: return Type::Part; }
}
bool normalize(std::array<double,3>& v)
{
    const double n = std::hypot(v[0],std::hypot(v[1],v[2]));
    if (!std::isfinite(n) || n <= 1e-14) return false;
    for (auto& x : v) x /= n;
    return true;
}
Key key(Type type, const RenderPickResultData& value) { return {type,value.part_id,value.result_id,value.result_id2}; }
}

struct LegacyPickContext::GeometryIndex {
    struct Location { int partId; int localIndex; };
    std::vector<std::pair<int,TopoDS_Shape>> parts;
    std::array<TopTools_IndexedMapOfShape,4> globals;
    std::array<std::map<int,std::vector<Location>>,4> locations;
    std::map<Key,PickResult> sampled;
};
QPointer<LegacyPickContext> LegacyPickContext::s_current;

LegacyPickContext::LegacyPickContext(QWidget* owner, OccViewport* viewport, const CadDocument& document,
                                     const std::map<int,FemMesh>* meshes)
    : QObject(owner), m_index(std::make_unique<GeometryIndex>()), m_viewport(viewport), m_document(document), m_meshes(meshes)
{
    setObjectName(QStringLiteral("originalGuiPickContext"));
    if (owner) owner->setProperty("motionOccLegacyPickContext", QVariant::fromValue(static_cast<QObject*>(this)));
    s_current = this;
    ensureGeometry();
    connect(viewport,&OccViewport::pickSelectionChanged,this,[this](const QVector<PickResult>& picks) {
        if (!m_controller || !m_controller->isPickActive() || !m_viewport ||
            m_viewport->property("legacyPickOwner").value<QObject*>() != this) return;
        std::vector<RenderPickResultData> values;
        for (const auto& pick : picks) {
            const auto converted = toLegacy(pick,m_type);
            if (converted.first != Type::None && converted.second.result_id >= 0) values.push_back(converted.second);
        }
        m_controller->receiveViewportSelection(m_type, values);
    });
    connect(viewport,&OccViewport::pickCancelled,this,[this] {
        // Original controllers commit accepted data when leaving the picker.
        // Right mouse remains the per-item undo operation.
        if (m_controller) m_controller->setPickActive(false);
    });
}
LegacyPickContext::~LegacyPickContext()
{
    if (m_controller) m_controller->detachActive();
    if (m_viewport) {
        for (const auto& key : m_layers) m_viewport->clearAuxiliaryPickHighlights(key);
        for (const auto& key : m_previews) m_viewport->clearAuxiliaryPoints(key);
    }
    if (parent() && parent()->property("motionOccLegacyPickContext").value<QObject*>()==this)
        parent()->setProperty("motionOccLegacyPickContext",QVariant());
    if (s_current==this) s_current=nullptr;
}
LegacyPickContext* LegacyPickContext::forObject(const QObject* object,bool useCurrentFallback)
{
    for (auto* p = object; p; p = p->parent())
        if (auto* context = qobject_cast<LegacyPickContext*>(p->property("motionOccLegacyPickContext").value<QObject*>())) return context;
    return useCurrentFallback ? s_current.data() : nullptr;
}
LegacyPickContext* LegacyPickContext::current() { return s_current; }

LegacyPickContext* LegacyPickContext::activeForViewport(const OccViewport& viewport)
{
    if (!viewport.isPicking() || viewport.property("nativePickOwner").isValid()) return nullptr;
    const auto* controller = MechMainWindow::mPickWidgetController::activeController();
    auto* context = controller ? controller->context() : nullptr;
    if (!context || context->m_viewport.data() != &viewport || context->m_controller.data() != controller ||
        viewport.property("legacyPickOwner").value<QObject*>() != context) return nullptr;
    return context;
}

void LegacyPickContext::setMeshes(const std::map<int,FemMesh>* meshes)
{
    m_meshes=meshes;m_index->sampled.clear();
    for(auto it=m_nodeIds.begin();it!=m_nodeIds.end();) {
        const auto mesh=meshes?meshes->find(it->first):std::map<int,FemMesh>::const_iterator{};
        if(!meshes || mesh==meshes->end() || it->second.size()!=int(mesh->second.nodes.size()))it=m_nodeIds.erase(it);
        else ++it;
    }
}
void LegacyPickContext::setNodeIds(int partId,const QVector<int>& ids)
{
    if(ids.isEmpty()) {
        m_nodeIds.erase(partId);
        m_index->sampled.clear();
        if(m_viewport&&m_meshes) {
            const auto mesh=m_meshes->find(partId);
            if(mesh!=m_meshes->end())m_viewport->setPickNodes(partId,mesh->second);
        }
        return;
    }
    if(!m_meshes)return;
    const auto mesh=m_meshes->find(partId);
    if(mesh==m_meshes->end() || ids.size()!=int(mesh->second.nodes.size()))return;
    std::set<int> unique;
    for(int id:ids)if(id<=0 || !unique.insert(id).second)return;
    m_nodeIds[partId]=ids;
    m_index->sampled.clear();
    if(m_viewport)m_viewport->setPickNodes(partId,mesh->second,ids);
}
QVector<int> LegacyPickContext::nodeIds(int partId) const
{
    const auto explicitIds=m_nodeIds.find(partId);
    if(explicitIds!=m_nodeIds.end())return explicitIds->second;
    QVector<int> result;
    if(!m_meshes)return result;
    const auto mesh=m_meshes->find(partId);
    if(mesh==m_meshes->end())return result;
    result.reserve(int(mesh->second.nodes.size()));
    for(int i=0;i<int(mesh->second.nodes.size());++i)result.append(i+1);
    return result;
}
void LegacyPickContext::refreshGeometry() { m_index=std::make_unique<GeometryIndex>(); ensureGeometry(); }
void LegacyPickContext::ensureGeometry() const
{
    const auto& parts = m_document.parts();
    bool same = parts.size() == m_index->parts.size();
    if (same) for (std::size_t i=0;i<parts.size();++i)
        if (parts[i].id != m_index->parts[i].first || !parts[i].shape.IsEqual(m_index->parts[i].second)) { same = false; break; }
    if (same) return;
    m_index->parts.clear(); m_index->sampled.clear();
    for (auto& map : m_index->globals) map.Clear();
    for (auto& map : m_index->locations) map.clear();
    const TopAbs_ShapeEnum kinds[] = {TopAbs_VERTEX,TopAbs_EDGE,TopAbs_FACE,TopAbs_SOLID};
    for (const auto& part : parts) {
        m_index->parts.emplace_back(part.id,part.shape);
        for (int c=0;c<4;++c) {
            TopTools_IndexedMapOfShape local; TopExp::MapShapes(part.shape,kinds[c],local);
            for (int index=1;index<=local.Extent();++index) {
                const int globalId=m_index->globals[c].Add(local(index));
                m_index->locations[c][globalId].push_back({part.id,index});
            }
        }
    }
}
PickKind LegacyPickContext::toKind(Type type)
{
    switch (type) { case Type::GeoPoint: return PickKind::Vertex; case Type::GeoEdge: return PickKind::Edge;
    case Type::GeoFace: return PickKind::Face; case Type::Node: return PickKind::Node;
    case Type::ElementEdge: return PickKind::MeshEdge; case Type::ElementFace: return PickKind::MeshFace;
    case Type::Element: return PickKind::Element; case Type::GeoSolid: return PickKind::Solid; default: return PickKind::Part; }
}
int LegacyPickContext::globalTopologyId(const PickResult& pick) const
{
    ensureGeometry(); const int c=category(typeOf(pick.kind));
    if (c<0 || !m_document.part(pick.partId)) return -1;
    const TopAbs_ShapeEnum kinds[] = {TopAbs_VERTEX,TopAbs_EDGE,TopAbs_FACE,TopAbs_SOLID};
    TopTools_IndexedMapOfShape local; TopExp::MapShapes(m_document.part(pick.partId)->shape,kinds[c],local);
    return pick.index>0 && pick.index<=local.Extent() ? m_index->globals[c].FindIndex(local(pick.index)) : -1;
}
std::pair<Type,RenderPickResultData> LegacyPickContext::toLegacy(const PickResult& pick, Type selectedType)
{
    Q_UNUSED(selectedType);
    ensureGeometry(); if (!pick.isValid()) return {Type::None,{}};
    Type type=typeOf(pick.kind); RenderPickResultData result;
    result.part_id=pick.partId;
    if (category(type)>=0) result.result_id=globalTopologyId(pick);
    else if (type==Type::Node) result.result_id=pick.nodeId;
    else if (type==Type::ElementEdge) {
        if (pick.nodeIds.size()!=2) return {Type::None,{}};
        result.result_id=pick.nodeIds[0]; result.result_id2=pick.nodeIds[1];
    } else if (type==Type::Part) {
        result.result_id=pick.partId;
    } else result.result_id=pick.index;
    if (result.result_id<0) return {Type::None,{}};
    m_index->sampled[key(type,result)]=pick;
    return {type,result};
}

PickResult LegacyPickContext::resolveGeometry(Type type, int globalId, int partHint) const
{
    ensureGeometry(); const int c=category(type); PickResult out;
    if (c<0 || globalId<1 || globalId>m_index->globals[c].Extent()) return out;
    const auto found=m_index->locations[c].find(globalId);
    if (found==m_index->locations[c].end() || found->second.empty()) return out;
    auto location=found->second.front();
    if (partHint>0) {
        const auto match=std::find_if(found->second.begin(),found->second.end(),[partHint](const auto& p){return p.partId==partHint;});
        if (match==found->second.end()) return out;
        location=*match;
    }
    out.kind=toKind(type); out.partId=location.partId; out.index=location.localIndex;
    const auto& shape=m_index->globals[c](globalId);
    try {
        if (type==Type::GeoPoint) out.position=xyz(BRep_Tool::Pnt(TopoDS::Vertex(shape)));
        else if (type==Type::GeoEdge) {
            BRepAdaptor_Curve curve(TopoDS::Edge(shape));
            const double parameter=(curve.FirstParameter()+curve.LastParameter())*.5;
            gp_Pnt p; gp_Vec tangent; curve.D1(parameter,p,tangent);
            out.position=xyz(p); out.direction={tangent.X(),tangent.Y(),tangent.Z()};
            if (curve.GetType()==GeomAbs_Circle) {
                out.position=xyz(curve.Circle().Location()); const auto axis=curve.Circle().Axis().Direction(); out.direction={axis.X(),axis.Y(),axis.Z()};
            } else if (curve.GetType()==GeomAbs_Ellipse) {
                out.position=xyz(curve.Ellipse().Location()); const auto axis=curve.Ellipse().Axis().Direction(); out.direction={axis.X(),axis.Y(),axis.Z()};
            }
            out.hasDirection=normalize(out.direction);
        } else {
            GProp_GProps props;
            if (type==Type::GeoSolid) BRepGProp::VolumeProperties(shape,props); else BRepGProp::SurfaceProperties(shape,props);
            if (std::abs(props.Mass())<=1e-14) return {};
            out.position=xyz(props.CentreOfMass());
            if (type==Type::GeoFace) {
                BRepAdaptor_Surface surface(TopoDS::Face(shape));
                const double u=(surface.FirstUParameter()+surface.LastUParameter())*.5;
                const double v=(surface.FirstVParameter()+surface.LastVParameter())*.5;
                gp_Pnt p; gp_Vec du,dv; surface.D1(u,v,p,du,dv); gp_Vec normal=du.Crossed(dv);
                if (shape.Orientation()==TopAbs_REVERSED) normal.Reverse();
                out.direction={normal.X(),normal.Y(),normal.Z()};
                const auto use=[&](const gp_Dir& axis){out.direction={axis.X(),axis.Y(),axis.Z()};};
                if (surface.GetType()==GeomAbs_Cylinder) use(surface.Cylinder().Axis().Direction());
                else if (surface.GetType()==GeomAbs_Cone) use(surface.Cone().Axis().Direction());
                else if (surface.GetType()==GeomAbs_Torus) use(surface.Torus().Axis().Direction());
                out.hasDirection=normalize(out.direction);
            }
        }
        out.hitPosition=out.position;
    } catch (const Standard_Failure&) { return {}; }
    return out;
}
PickResult LegacyPickContext::resolve(Type type,const RenderPickResultData& value) const
{
    ensureGeometry();
    const auto sampled=m_index->sampled.find(key(type,value));
    if (category(type)>=0) {
        auto exact=resolveGeometry(type,value.result_id,value.part_id);
        // ID-based selection and import also call toLegacy with an unpopulated
        // PickResult. Geometry coordinates must always come from the topology;
        // only an actual sampled tangent/normal carries click-specific data.
        if(exact.isValid() && sampled!=m_index->sampled.end() && sampled->second.hasDirection) {
            exact.direction=sampled->second.direction;exact.hasDirection=true;
            exact.hitPosition=sampled->second.hitPosition;
        }
        return exact;
    }
    if (type!=Type::Part && sampled!=m_index->sampled.end() && m_document.part(sampled->second.partId)) return sampled->second;
    PickResult out; out.kind=toKind(type); out.partId=value.part_id;
    if (type==Type::Part) {
        out.partId=value.part_id>0 ? value.part_id : value.result_id;
        const auto* part=m_document.part(out.partId); if (!part) return {};
        try {
            GProp_GProps props; BRepGProp::VolumeProperties(part->shape,props);
            if (std::abs(props.Mass())<=1e-14) BRepGProp::SurfaceProperties(part->shape,props);
            if (std::abs(props.Mass())<=1e-14) BRepGProp::LinearProperties(part->shape,props);
            if (std::abs(props.Mass())<=1e-14) return {};
            out.position=out.hitPosition=xyz(props.CentreOfMass()); return out;
        } catch (const Standard_Failure&) { return {}; }
    }
    if (!m_meshes) return {};
    const auto found=m_meshes->find(out.partId);
    if (found==m_meshes->end()) return {};
    const auto& mesh=found->second;
    const auto ids=m_nodeIds.find(out.partId);
    const auto nodeIndex=[&](int id)->int {
        if (ids==m_nodeIds.end()) return id>0 && id<=int(mesh.nodes.size()) ? id-1 : -1;
        return ids->second.indexOf(id);
    };
    const auto nodeId=[&](std::uint32_t index)->int {
        return ids==m_nodeIds.end() ? int(index+1) : ids->second.value(int(index),-1);
    };
    std::vector<std::uint32_t> nodes;
    if (type==Type::Node) {
        out.index=nodeIndex(value.result_id); if (out.index<0 || out.index>=int(mesh.nodes.size())) return {};
        out.nodeId=value.result_id; nodes.push_back(std::uint32_t(out.index));
    } else if (type==Type::ElementEdge) {
        const int a=nodeIndex(value.result_id),b=nodeIndex(value.result_id2);
        if (a<0 || b<0 || a==b) return {};
        std::set<std::array<std::uint32_t,2>> edges;
        const auto add=[&](std::uint32_t x,std::uint32_t y){edges.insert({std::min(x,y),std::max(x,y)});};
        for (const auto& face:mesh.surface) for (int i=0;i<3;++i) add(face[i],face[(i+1)%3]);
        for (const auto& cell:mesh.tetrahedra) for (int i=0;i<4;++i) for (int j=i+1;j<4;++j) add(cell[i],cell[j]);
        const auto edge=edges.find({std::uint32_t(std::min(a,b)),std::uint32_t(std::max(a,b))});
        if (edge==edges.end()) return {};
        out.index=int(std::distance(edges.begin(),edge))+1; nodes.assign(edge->begin(),edge->end());
    } else if (type==Type::Element && !mesh.tetrahedra.empty()) {
        if (value.result_id<1 || value.result_id>int(mesh.tetrahedra.size())) return {};
        out.index=value.result_id; const auto& cell=mesh.tetrahedra[std::size_t(value.result_id-1)]; nodes.assign(cell.begin(),cell.end());
    } else {
        if (value.result_id<1 || value.result_id>int(mesh.surface.size())) return {};
        out.index=value.result_id; const auto& face=mesh.surface[std::size_t(value.result_id-1)]; nodes.assign(face.begin(),face.end());
    }
    for (auto n:nodes) {
        if (n>=mesh.nodes.size()) return {};
        for (int k=0;k<3;++k) out.position[k]+=mesh.nodes[n][k]/double(nodes.size());
        out.nodeIds.append(nodeId(n));
    }
    if (nodes.size()==2) {
        for (int k=0;k<3;++k) out.direction[k]=mesh.nodes[nodes[1]][k]-mesh.nodes[nodes[0]][k];
        out.hasDirection=normalize(out.direction);
    } else if (type==Type::ElementFace && nodes.size()==3) {
        const QVector3D normal=QVector3D::crossProduct(qvec(mesh.nodes[nodes[1]])-qvec(mesh.nodes[nodes[0]]),qvec(mesh.nodes[nodes[2]])-qvec(mesh.nodes[nodes[0]]));
        out.direction={normal.x(),normal.y(),normal.z()}; out.hasDirection=normalize(out.direction);
    }
    out.hitPosition=out.position; return out;
}
QVector<PickResult> LegacyPickContext::resolve(const LegacyPickMap& data) const
{
    QVector<PickResult> picks;
    for (const auto& entry:data) for (const auto& value:entry.second) { const auto p=resolve(entry.first,value); if (p.isValid()) picks.append(p); }
    return picks;
}
QVector3D LegacyPickContext::averageCoordinate(const LegacyPickMap& data) const
{
    std::array<double,3> sum{}; int count=0;
    for (const auto& pick:resolve(data)) { for (int k=0;k<3;++k) sum[k]+=pick.position[k]; ++count; }
    if (!count) return {};
    for (auto& x:sum) x/=count;
    return qvec(sum);
}
QVector3D LegacyPickContext::direction(const LegacyPickMap& data) const
{
    for (const auto& pick:resolve(data)) if (pick.hasDirection) return qvec(pick.direction);
    return {};
}
bool LegacyPickContext::acceptsEdgeLoad(Type type,const RenderPickResultData& value) const
{
    const auto pick=resolve(type,value);
    if(!pick.isValid()) return false;
    if(type==Type::GeoEdge) {
        const auto* part=m_document.part(pick.partId);if(!part)return false;
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(part->shape,TopAbs_EDGE,edges);
        if(pick.index<1||pick.index>edges.Extent())return false;
        for(TopExp_Explorer face(part->shape,TopAbs_FACE);face.More();face.Next()) {
            TopTools_IndexedMapOfShape boundary;TopExp::MapShapes(face.Current(),TopAbs_EDGE,boundary);
            if(boundary.Contains(edges(pick.index)))return true;
        }
        return false;
    }
    if(type==Type::ElementEdge) {
        if(!m_meshes)return false;
        const auto found=m_meshes->find(pick.partId);
        // Surface triangles on tetrahedra are display boundaries, not shell
        // elements. Edge loads must not silently treat them as shell topology.
        return found!=m_meshes->end()&&found->second.tetrahedra.empty()&&!found->second.surface.empty();
    }
    return false;
}
void LegacyPickContext::setHighlightLayer(const QString& key,const QVector<PickResult>& picks,const QColor& color)
{
    if (!m_viewport) return;
    const QString scoped=QStringLiteral("legacy:%1:%2").arg(quintptr(this)).arg(key);
    m_layers.insert(scoped); m_viewport->setAuxiliaryPickHighlights(scoped,picks,color);
}
void LegacyPickContext::clearHighlightLayer(const QString& key)
{
    const QString scoped=QStringLiteral("legacy:%1:%2").arg(quintptr(this)).arg(key);
    m_layers.erase(scoped); if (m_viewport) m_viewport->clearAuxiliaryPickHighlights(scoped);
}
void LegacyPickContext::setPreviewPoints(const QString& key,const QVector<QVector3D>& points,const QColor& color,float pointSize)
{
    if (!m_viewport) return;
    const QString scoped=QStringLiteral("legacy:%1:%2").arg(quintptr(this)).arg(key);
    m_previews.insert(scoped); m_viewport->setAuxiliaryPoints(scoped,points,color,pointSize);
}
void LegacyPickContext::clearPreviewPoints(const QString& key)
{
    const QString scoped=QStringLiteral("legacy:%1:%2").arg(quintptr(this)).arg(key);
    m_previews.erase(scoped); if (m_viewport) m_viewport->clearAuxiliaryPoints(scoped);
}
void LegacyPickContext::begin(MechMainWindow::mPickWidgetController* controller,Type type,bool multiple,
                             const LegacyPickMap& initial,const std::set<int>& allowed)
{
    if (!controller || !m_viewport) return;
    m_controller=controller; m_type=type; s_current=this;
    // setScene rebuilds display topology. Restore the model's actual node IDs
    // before a new picker session, including IDs shared by multiple CAD parts.
    if(m_meshes)for(const auto& [part,ids]:m_nodeIds) {
        const auto mesh=m_meshes->find(part);
        if(mesh!=m_meshes->end())m_viewport->setPickNodes(part,mesh->second,ids);
    }
    m_viewport->setProperty("nativePickOwner",QVariant());
    m_viewport->setProperty("legacyPickOwner",QVariant::fromValue(static_cast<QObject*>(this)));
    m_viewport->setPickHighlightColor(controller->widget()->colorBlockColor());
    m_viewport->setPickFeedbackVisible(!controller->isCloudChartMode());
    m_viewport->beginPick(toKind(type),multiple,allowed,resolve(initial));
    emit allowedTypesChanged(); emit pickTypeChanged(type);
}
void LegacyPickContext::end(MechMainWindow::mPickWidgetController* controller)
{
    if (m_controller!=controller) return;
    m_controller=nullptr;
    if (m_viewport && m_viewport->property("legacyPickOwner").value<QObject*>()==this) {
        m_viewport->setProperty("legacyPickOwner",QVariant()); m_viewport->endPick(); m_viewport->setPickHighlightColor({});
    }
    emit allowedTypesChanged();
}
void LegacyPickContext::updateSelection(MechMainWindow::mPickWidgetController* controller,const LegacyPickMap& data)
{
    if (controller==m_controller && m_viewport) m_viewport->setPickSelection(resolve(data));
}
void LegacyPickContext::setPickFeedbackVisible(MechMainWindow::mPickWidgetController* controller,bool visible)
{
    if(controller==m_controller && m_viewport &&
       m_viewport->property("legacyPickOwner").value<QObject*>()==this)
        m_viewport->setPickFeedbackVisible(visible);
}
void LegacyPickContext::setPickType(Type type)
{
    if (!m_controller || !m_controller->allowedTypes().contains(type)) return;
    begin(m_controller,type,m_controller->pickMode()==MechMainWindow::PickMore,m_controller->getAllData().pickedData,m_controller->allowedParts());
}
QSet<Type> LegacyPickContext::allowedTypes() const { return m_controller ? m_controller->allowedTypes() : QSet<Type>{}; }
} // namespace MotionOcc
