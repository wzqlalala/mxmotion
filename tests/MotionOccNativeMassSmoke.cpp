#include "CadDocument.h"
#include "NativeBodyMass.h"

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <cmath>
#include <limits>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool ok, const QString& message) { if (!ok) throw std::runtime_error(message.toStdString()); }
void near(double value, double expected, double tolerance, const QString& name)
{
    require(std::isfinite(value) && std::abs(value - expected) <= tolerance,
        QStringLiteral("%1: got %2, expected %3").arg(name).arg(value, 0, 'g', 17).arg(expected, 0, 'g', 17));
}
int import(CadDocument& cad, const TopoDS_Shape& shape, const QString& path)
{
    require(BRepTools::Write(shape, path.toUtf8().constData()), "write fixture BREP");
    std::vector<int> ids; QString error;
    require(cad.importFile(path, ids, error), error);
    require(ids.size() == 1, "fixture must be one part");
    return ids.front();
}
}

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    try {
        CadDocument cad; QString error; int a = 0;
        require(cad.addPrimitive(Primitive::Box, 10, 20, 30, a, error), error);
        QTemporaryDir directory; require(directory.isValid(), "temporary directory");
        gp_Trsf shift; shift.SetTranslation(gp_Vec(40, 20, 0));
        const int b = import(cad, BRepBuilderAPI_Transform(cad.part(a)->shape, shift, true).Shape(), directory.filePath("second.brep"));
        NativeBodyMassProperties properties;
        require(nativeBodyMassProperties(cad, {a, b}, kDefaultNativeBodyDensityKgMm3, properties, error), error);
        // Independent analytic values for two 10 x 20 x 30 mm boxes whose
        // centroids differ by (40,20,0) mm. Includes a nonzero product term.
        near(properties.volumeMm3, 12000, 1e-8, "aggregate volume mm3");
        near(properties.massKg, .0942, 1e-12, "steel mass kg");
        near(properties.centerMm[0], 25, 1e-10, "global COM x");
        near(properties.centerMm[1], 20, 1e-10, "global COM y");
        near(properties.centerMm[2], 15, 1e-10, "global COM z");
        const std::array<double, 6> expected{{19.625, 45.53, 51.025, -18.84, 0, 0}};
        for (int i = 0; i < 6; ++i) near(properties.inertiaKgMm2[i], expected[i], 1e-8, QStringLiteral("centroid inertia %1").arg(i));
        require(nativeBodyMassProperties(cad, {a, b, a}, kDefaultNativeBodyDensityKgMm3, properties, error), error);
        near(properties.massKg, .0942, 1e-12, "repeat selection is a set");
        require(nativeBodyMassProperties(cad, {a, b}, 2.7e-6, properties, error), error);
        near(properties.massKg, .0324, 1e-12, "explicit material density");

        QJsonObject body{{"id", 7}, {"name", "compound"}, {"isCustomMassProps", false},
            {"isFixed", true}, {"centerOfMass", QJsonObject{{"x", -8}, {"y", -9}, {"z", -10}}},
            {"initialVelocity", QJsonObject{{"x", 3}, {"y", 4}, {"z", 5}}},
            {"vendorExtension", QJsonArray{1, "retained"}}, {"geoEntityIds", QJsonArray{a, b}}};
        const auto original = body;
        require(updateNativeBodyMass(cad, std::vector<int>{a, b}, body, kDefaultNativeBodyDensityKgMm3, error), error);
        near(body.value("mass").toDouble(), .0942, 1e-12, "automatic record mass");
        near(body.value("centerOfMass").toObject().value("x").toDouble(), 25, 1e-10, "automatic record COM");
        require(body.value("inertiaConvention").toString() == "tensor", "tensor convention must be explicit");
        for (const auto& key : {"id", "name", "isFixed", "initialVelocity", "vendorExtension", "geoEntityIds"})
            require(body.value(key) == original.value(key), QStringLiteral("unrelated field changed: %1").arg(key));
        const auto autoRecord = body;
        require(!updateNativeBodyMass(cad, std::vector<int>{a, 123456}, body, kDefaultNativeBodyDensityKgMm3, error), "missing CAD must fail");
        require(body == autoRecord, "failed calculation changed record");
        require(!updateNativeBodyMass(cad, std::vector<int>{a}, body, std::numeric_limits<double>::quiet_NaN(), error), "nonfinite density must fail");
        require(body == autoRecord, "invalid density changed record");
        body["isCustomMassProps"] = true;
        const auto customRecord = body;
        require(updateNativeBodyMass(cad, std::vector<int>{a}, body, 2.7e-6, error), error);
        require(body == customRecord, "custom/imported mass properties must not be overwritten");

        const auto face = BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)), 0, 10, 0, 20).Face();
        const int surface = import(cad, face, directory.filePath("surface.brep"));
        body["isCustomMassProps"] = false;
        const auto beforeSurface = body;
        require(!updateNativeBodyMass(cad, std::vector<int>{surface}, body, kDefaultNativeBodyDensityKgMm3, error), "surface must not acquire invented mass");
        require(body == beforeSurface, "surface failure changed record");
        qInfo() << "Native automatic mass: analytic COM/tensor, density, atomicity and custom preservation passed";
        return 0;
    } catch (const std::exception& failure) { qCritical() << failure.what(); return 1; }
}
