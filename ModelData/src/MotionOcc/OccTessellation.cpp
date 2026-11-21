#include "OccTessellation.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <vector>

namespace MotionOcc {
namespace {

bool hasMesh(const TopoDS_Face& face)
{
    TopLoc_Location location;
    const auto mesh=BRep_Tool::Triangulation(face,location);
    return !mesh.IsNull()&&mesh->NbNodes()>=3&&mesh->NbTriangles()>0;
}

bool splitPlanarFace(const TopoDS_Face& original,double deflection)
{
    // An internally tangent inner/outer wire is a legitimate pinched planar
    // region, but OCCT's mesher can label its discretized wires intersecting.
    // Split a private copy on its exact support, mesh the pieces, then store
    // only their combined triangulation on the original, unchanged face.
    auto local=TopoDS::Face(BRepBuilderAPI_Copy(original,true,false).Shape());
    local.Location(TopLoc_Location());
    local.Orientation(TopAbs_FORWARD);
    BRepAdaptor_Surface surface(local);
    if(surface.GetType()!=GeomAbs_Plane)return false;
    double u0,u1,v0,v1;BRepTools::UVBounds(local,u0,u1,v0,v1);
    if(!std::isfinite(u0)||!std::isfinite(u1)||!std::isfinite(v0)||!std::isfinite(v1)||u1<=u0||v1<=v0)return false;
    GProp_GProps originalProperties;BRepGProp::SurfaceProperties(local,originalProperties);
    const double area=originalProperties.Mass();
    if(!std::isfinite(area)||area<=0)return false;
    const gp_Dir normal=surface.Plane().Axis().Direction();

    for(int axis=0;axis<2;++axis){
        const double middle=axis?(v0+v1)*.5:(u0+u1)*.5;
        const double margin=std::max(u1-u0,v1-v0)*.1;
        const gp_Pnt start=axis?surface.Value(u0-margin,middle):surface.Value(middle,v0-margin);
        const gp_Pnt end=axis?surface.Value(u1+margin,middle):surface.Value(middle,v1+margin);
        TopTools_ListOfShape arguments,tools;arguments.Append(local);tools.Append(BRepBuilderAPI_MakeEdge(start,end).Edge());
        BRepAlgoAPI_Splitter split;split.SetArguments(arguments);split.SetTools(tools);split.SetNonDestructive(true);split.Build();
        if(!split.IsDone()||split.HasErrors()||split.Shape().IsNull()||!BRepCheck_Analyzer(split.Shape()).IsValid())continue;
        GProp_GProps piecesProperties;BRepGProp::SurfaceProperties(split.Shape(),piecesProperties);
        // The exact B-rep area, not the faceted area, must be unchanged. No
        // removed slivers, filled holes, or enlarged tolerances are accepted.
        const double delta=std::abs(piecesProperties.Mass()-area);
        if(!std::isfinite(delta)||delta>std::max(1e-8,area*1e-10))continue;
        BRepMesh_IncrementalMesh mesher(split.Shape(),deflection,false,.15,false);
        std::vector<gp_Pnt> points;
        std::vector<std::array<int,3>> triangles;
        bool complete=true;
        int faceCount=0;
        for(TopExp_Explorer it(split.Shape(),TopAbs_FACE);it.More();it.Next()){
            ++faceCount;
            const auto part=TopoDS::Face(it.Current());TopLoc_Location location;
            const auto mesh=BRep_Tool::Triangulation(part,location);
            if(mesh.IsNull()||mesh->NbNodes()<3||mesh->NbTriangles()<1){complete=false;break;}
            if(points.size()+mesh->NbNodes()>3000000||triangles.size()+mesh->NbTriangles()>1000000){complete=false;break;}
            const int offset=static_cast<int>(points.size());
            for(int n=1;n<=mesh->NbNodes();++n)points.push_back(mesh->Node(n).Transformed(location.Transformation()));
            for(int n=1;n<=mesh->NbTriangles();++n){
                int a,b,c;mesh->Triangle(n).Get(a,b,c);
                const auto cross=gp_Vec(points[offset+a-1],points[offset+b-1]).Crossed(gp_Vec(points[offset+a-1],points[offset+c-1]));
                if(cross.SquareMagnitude()<=1e-24)continue;
                if(cross.Dot(gp_Vec(normal))<0)std::swap(b,c);
                triangles.push_back({offset+a,offset+b,offset+c});
            }
        }
        if(!complete||faceCount<2||triangles.empty())continue;
        Handle(Poly_Triangulation) merged=new Poly_Triangulation(static_cast<int>(points.size()),static_cast<int>(triangles.size()),false);
        merged->Deflection(deflection);
        for(int n=0;n<static_cast<int>(points.size());++n)merged->SetNode(n+1,points[n]);
        for(int n=0;n<static_cast<int>(triangles.size());++n){const auto& t=triangles[n];merged->SetTriangle(n+1,Poly_Triangle(t[0],t[1],t[2]));}
        BRep_Builder().UpdateFace(original,merged);
        return true;
    }
    return false;
}

} // namespace

bool ensureOccFaceTriangulation(const TopoDS_Face& face,double deflection,QString& error)
{
    try{
        if(face.IsNull()||!std::isfinite(deflection)||deflection<=0){error=QStringLiteral("曲面或离散精度无效。");return false;}
        if(hasMesh(face)){error.clear();return true;}
        BRepMesh_IncrementalMesh mesher(face,deflection,false,.15,false);
        if(hasMesh(face)||splitPlanarFace(face,deflection)){error.clear();return true;}
        error=QStringLiteral("曲面无法完整三角化；同几何拆分也未通过完整性与面积校验。");
    }catch(const Standard_Failure& failure){error=QStringLiteral("曲面三角化失败：%1").arg(QString::fromUtf8(failure.GetMessageString()));}
    catch(const std::exception& failure){error=QStringLiteral("曲面三角化失败：%1").arg(QString::fromUtf8(failure.what()));}
    return false;
}

} // namespace MotionOcc
