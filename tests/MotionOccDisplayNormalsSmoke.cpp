#include "CadDocument.h"

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepTools.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

using namespace MotionOcc;
using Point = std::array<double, 3>;

void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

Point point(const QVector3D& p) { return {{p.x(), p.y(), p.z()}}; }
Point point(const gp_Pnt& p) { return {{p.X(), p.Y(), p.Z()}}; }
Point minus(const Point& a, const Point& b) { return {{a[0]-b[0], a[1]-b[1], a[2]-b[2]}}; }
double dot(const Point& a, const Point& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
Point unit(Point p)
{
    const double length = std::hypot(p[0], p[1], p[2]);
    require(std::isfinite(length) && length > 0.0, "Invalid reference direction");
    for (double& x : p) x /= length;
    return p;
}

void checkNormals(const CadPart& part)
{
    const auto& mesh = part.display;
    require(mesh.normals.size() == mesh.vertices.size(), "Display normals lost vertex alignment");
    require(!mesh.triangles.empty(), "Missing test surface");
    for (const auto& triangle : mesh.triangles) {
        const auto u = minus(point(mesh.vertices.at(triangle[1])), point(mesh.vertices.at(triangle[0])));
        const auto v = minus(point(mesh.vertices.at(triangle[2])), point(mesh.vertices.at(triangle[0])));
        const auto winding = unit({{u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]}});
        for (const auto index : triangle) {
            const Point n = point(mesh.normals.at(index));
            require(std::isfinite(n[0]) && std::isfinite(n[1]) && std::isfinite(n[2]), "Non-finite display normal");
            require(std::abs(dot(n,n)-1.0) < 2e-6, "Surface display normal is not a unit vector");
            require(dot(n,winding) > 0.0, "Display normal disagrees with placed face winding");
        }
    }
}

CadPart makePrimitive(Primitive type, double x, double y, double z)
{
    CadDocument document;
    int id = -1;
    QString error;
    if (!document.addPrimitive(type,x,y,z,id,error)) throw std::runtime_error(error.toStdString());
    require(document.part(id) != nullptr, "Primitive ID is missing");
    return *document.part(id);
}

CadPart importShape(const TopoDS_Shape& shape, const QDir& directory, const QString& name)
{
    const QString path = directory.filePath(name + QStringLiteral(".brep"));
    require(BRepTools::Write(shape,QFile::encodeName(path).constData()), "Could not write located CAD fixture");
    CadDocument document;
    std::vector<int> ids;
    QString error;
    if (!document.importFile(path,ids,error)) throw std::runtime_error(error.toStdString());
    require(ids.size()==1 && document.parts().size()==1, "Located CAD fixture changed part ownership");
    const auto& part = *document.part(ids.front());
    int before = 0, after = 0;
    for (TopExp_Explorer it(shape,TopAbs_FACE);it.More();it.Next()) ++before;
    for (TopExp_Explorer it(part.shape,TopAbs_FACE);it.More();it.Next()) ++after;
    require(before==after, "Display generation changed CAD face topology");
    return part;
}

void checkCylinder(const CadPart& part, const gp_Trsf& placement, double expectedRadialSign, bool caps)
{
    checkNormals(part);
    const Point origin = point(gp_Pnt(0,0,0).Transformed(placement));
    const gp_Vec axisVector = gp_Vec(0,0,1).Transformed(placement);
    const Point axis = unit({{axisVector.X(),axisVector.Y(),axisVector.Z()}});
    std::size_t sides = 0, ends = 0;
    for (const auto& triangle : part.display.triangles) {
        const Point a = minus(point(part.display.vertices[triangle[0]]),origin);
        const Point b = minus(point(part.display.vertices[triangle[1]]),origin);
        const Point c = minus(point(part.display.vertices[triangle[2]]),origin);
        const bool isCap = caps && std::abs(dot(a,axis)-dot(b,axis)) < 1e-4 &&
                                   std::abs(dot(a,axis)-dot(c,axis)) < 1e-4;
        for (const auto index : triangle) {
            const Point actual = point(part.display.normals[index]);
            if (isCap) {
                require(std::abs(dot(actual,axis)) > 1.0-2e-6, "Cylinder end cap was smoothed into its side wall");
                ++ends;
            } else {
                Point radial = minus(point(part.display.vertices[index]),origin);
                const double projection = dot(radial,axis);
                for (int k=0;k<3;++k) radial[k] -= projection*axis[k];
                radial = unit(radial);
                require(dot(actual,radial)*expectedRadialSign > 1.0-3e-6,
                        "Cylinder side normal is not the analytic placed radial direction");
                ++sides;
            }
        }
    }
    require(sides>0 && (!caps || ends>0), "Cylinder normal test did not exercise each expected surface");
}

void checkBox()
{
    const auto part = makePrimitive(Primitive::Box,10,20,30);
    checkNormals(part);
    std::set<std::array<int,3>> cornerNormals;
    for (const auto& triangle : part.display.triangles) {
        for (const auto index : triangle) {
            const Point n = point(part.display.normals[index]);
            int nonzero = 0;
            for (double x : n) {
                if (std::abs(x)>1e-6) {++nonzero;require(std::abs(std::abs(x)-1.0)<1e-6,"Box normal is not axis aligned");}
            }
            require(nonzero==1,"Shared CAD edges were smoothed across different faces");
            if (part.display.vertices[index].isNull())
                cornerNormals.insert({{int(std::lround(n[0])),int(std::lround(n[1])),int(std::lround(n[2]))}});
        }
    }
    require(cornerNormals==std::set<std::array<int,3>>{{{-1,0,0}},{{0,-1,0}},{{0,0,-1}}},
            "Box corner lost independent face normals");
}

void checkSmallSphere()
{
    const auto part = makePrimitive(Primitive::Sphere,0.001,0,0);
    checkNormals(part);
    for (const auto& triangle : part.display.triangles)
        for (const auto index : triangle) {
            const auto radial = unit(point(part.display.vertices[index]));
            require(dot(radial,point(part.display.normals[index]))>.98,
                    "Micrometre sphere lost its outward smooth normal");
        }
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir temporary;
        require(temporary.isValid(),"Could not create CAD fixture directory");
        const QDir directory(temporary.path());
        checkBox();
        checkCylinder(makePrimitive(Primitive::Cylinder,12,40,0),gp_Trsf(),1,true);

        gp_Trsf placement;
        placement.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,2,3)),.71);
        placement.SetTranslationPart(gp_Vec(31,-17,8));
        const auto located = BRepPrimAPI_MakeCylinder(12,40).Shape().Moved(TopLoc_Location(placement));
        checkCylinder(importShape(located,directory,QStringLiteral("located-cylinder")),placement,1,true);

        TopoDS_Face reversed = BRepBuilderAPI_MakeFace(
            gp_Cylinder(gp_Ax3(gp_Pnt(0,0,0),gp_Dir(0,0,1)),12),0,2*std::acos(-1.0),0,40).Face();
        reversed.Reverse();
        reversed.Move(TopLoc_Location(placement));
        checkCylinder(importShape(reversed,directory,QStringLiteral("reversed-cylinder-face")),placement,-1,false);

        gp_Trsf mirror;
        mirror.SetMirror(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(1,0,0)));
        const auto mirrored = BRepBuilderAPI_Transform(BRepPrimAPI_MakeCylinder(12,40).Shape(),mirror,true).Shape();
        checkCylinder(importShape(mirrored,directory,QStringLiteral("mirrored-cylinder")),mirror,1,true);
        checkSmallSphere();
        qInfo() << "Display normals: analytic cylinder, hard CAD edges, placed/reversed/mirrored geometry and micrometre sphere passed.";
        return 0;
    } catch (const Standard_Failure& error) {
        qCritical() << "Display normals OCCT failure:" << error.GetMessageString();
    } catch (const std::exception& error) {
        qCritical() << "Display normals failure:" << error.what();
    }
    return 1;
}
