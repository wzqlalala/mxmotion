#include "NativeBodyMass.h"
#include "CadDocument.h"

#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Shell.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

#include <QJsonArray>

#include <cmath>
#include <set>
#include <stdexcept>

namespace MotionOcc {
namespace {
[[noreturn]] void fail(const QString& message)
{
    throw std::runtime_error(message.toStdString());
}

gp_Pnt integrationOrigin(const TopoDS_Shape& shape)
{
    Bnd_Box bounds;
    BRepBndLib::AddOptimal(shape, bounds, false, false);
    if (bounds.IsVoid() || bounds.IsOpen()) fail(QStringLiteral("自动质量计算：几何包围盒无效。"));
    double x0, y0, z0, x1, y1, z1;
    bounds.Get(x0, y0, z0, x1, y1, z1);
    for (double coordinate : {x0, y0, z0, x1, y1, z1})
        if (!std::isfinite(coordinate)) fail(QStringLiteral("自动质量计算：几何坐标不是有限数值。"));
    return gp_Pnt(x0 / 2 + x1 / 2, y0 / 2 + y1 / 2, z0 / 2 + z1 / 2);
}
}

bool nativeBodyMassProperties(const CadDocument& cad, const std::vector<int>& partIds,
                              double densityKgMm3, NativeBodyMassProperties& properties,
                              QString& error)
{
    try {
        if (!std::isfinite(densityKgMm3) || densityKgMm3 <= 0)
            fail(QStringLiteral("自动质量计算：密度必须为正的有限数值，单位 kg/mm³。"));
        const std::set<int> uniqueIds(partIds.begin(), partIds.end());
        if (uniqueIds.empty()) fail(QStringLiteral("自动质量计算：请先绑定至少一个封闭实体部件。"));

        // Locating the accumulator near the model avoids subtracting large
        // world-origin moments for a small object with a large CAD placement.
        const auto* first = cad.part(*uniqueIds.begin());
        if (!first || first->shape.IsNull()) fail(QStringLiteral("自动质量计算：绑定的部件不存在。"));
        GProp_GProps aggregate(integrationOrigin(first->shape));
        for (int id : uniqueIds) {
            const auto* part = cad.part(id);
            const QString context = QStringLiteral("自动质量计算：部件 %1").arg(id);
            if (!part || part->shape.IsNull()) fail(context + QStringLiteral("不存在。"));
            if (!BRepCheck_Analyzer(part->shape).IsValid()) fail(context + QStringLiteral("几何无效。"));
            bool haveSolid = false;
            for (TopExp_Explorer it(part->shape, TopAbs_SOLID); it.More(); it.Next()) {
                haveSolid = true;
                const auto& solid = it.Current();
                TopoDS_Iterator shell(solid);
                if (!shell.More()) fail(context + QStringLiteral("没有封闭边界。"));
                for (; shell.More(); shell.Next()) {
                    if (shell.Value().ShapeType() != TopAbs_SHELL ||
                        BRepCheck_Shell(TopoDS::Shell(shell.Value())).Closed() != BRepCheck_NoError)
                        fail(context + QStringLiteral("含有未封闭的壳。"));
                }
                GProp_GProps component(integrationOrigin(solid));
                const double integrationError = BRepGProp::VolumeProperties(solid, component, 1e-7, true, false);
                if (!std::isfinite(integrationError) || !std::isfinite(component.Mass()) || component.Mass() <= 1e-12)
                    fail(context + QStringLiteral("没有正的有限实体体积，请检查实体方向或退化几何。"));
                aggregate.Add(component);
            }
            if (!haveSolid) fail(context + QStringLiteral("不是封闭实体；请使用自定义质量属性。"));
        }

        NativeBodyMassProperties next;
        next.volumeMm3 = aggregate.Mass();
        next.massKg = next.volumeMm3 * densityKgMm3;
        const auto center = aggregate.CentreOfMass();
        next.centerMm = {center.X(), center.Y(), center.Z()};
        const auto tensor = aggregate.MatrixOfInertia();
        next.inertiaKgMm2 = {tensor(1,1) * densityKgMm3, tensor(2,2) * densityKgMm3,
            tensor(3,3) * densityKgMm3, tensor(1,2) * densityKgMm3,
            tensor(1,3) * densityKgMm3, tensor(2,3) * densityKgMm3};
        if (!std::isfinite(next.volumeMm3) || !std::isfinite(next.massKg) || next.massKg <= 0)
            fail(QStringLiteral("自动质量计算：计算出的质量无效。"));
        for (double value : next.centerMm)
            if (!std::isfinite(value)) fail(QStringLiteral("自动质量计算：计算出的质心无效。"));
        for (double value : next.inertiaKgMm2)
            if (!std::isfinite(value)) fail(QStringLiteral("自动质量计算：计算出的惯量无效。"));
        for (int i = 0; i < 3; ++i)
            if (next.inertiaKgMm2[i] <= 0) fail(QStringLiteral("自动质量计算：实体惯量退化。"));
        properties = next;
        error.clear();
        return true;
    } catch (const Standard_Failure& failure) {
        error = QStringLiteral("自动质量计算失败：%1").arg(QString::fromUtf8(failure.GetMessageString()));
    } catch (const std::exception& failure) {
        error = QString::fromUtf8(failure.what());
    }
    return false;
}

bool updateNativeBodyMass(const CadDocument& cad, const std::vector<int>& partIds,
                          QJsonObject& body, double densityKgMm3, QString& error)
{
    if (body.value(QStringLiteral("isCustomMassProps")).toBool()) {
        error.clear();
        return true;
    }
    NativeBodyMassProperties properties;
    if (!nativeBodyMassProperties(cad, partIds, densityKgMm3, properties, error)) return false;
    auto next = body;
    next.insert(QStringLiteral("mass"), properties.massKg);
    next.insert(QStringLiteral("centerOfMass"), QJsonObject{{"x", properties.centerMm[0]},
        {"y", properties.centerMm[1]}, {"z", properties.centerMm[2]}});
    QJsonArray inertia;
    for (double value : properties.inertiaKgMm2) inertia.append(value);
    next.insert(QStringLiteral("inertia"), inertia);
    next.insert(QStringLiteral("inertiaConvention"), QStringLiteral("tensor"));
    next.insert(QStringLiteral("densityKgMm3"), densityKgMm3);
    body = next;
    return true;
}

} // namespace MotionOcc
