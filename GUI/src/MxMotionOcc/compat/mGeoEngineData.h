#pragma once
#include "NativeBodyMass.h"
#include <QVector3D>
#include <stdexcept>

namespace GeoNeutralData {
struct GeoSolidMass {
    double mass=-1;
    QVector3D centroid;
    double Ixx=0,Iyy=0,Izz=0,Ixy=0,Ixz=0,Iyz=0;
};
class GeoEngineHandle {
public:
    explicit GeoEngineHandle(const MotionOcc::CadDocument& document):document_(document) {}
    GeoSolidMass calcCalculatePartMass(const std::vector<int>& ids,double density) const {
        MotionOcc::NativeBodyMassProperties p;QString error;
        if(!MotionOcc::nativeBodyMassProperties(document_,ids,density,p,error))return {};
        return {p.massKg,QVector3D(p.centerMm[0],p.centerMm[1],p.centerMm[2]),
            p.inertiaKgMm2[0],p.inertiaKgMm2[1],p.inertiaKgMm2[2],
            p.inertiaKgMm2[3],p.inertiaKgMm2[4],p.inertiaKgMm2[5]};
    }
private: const MotionOcc::CadDocument& document_;
};
class GeoEngineData {
public:
    explicit GeoEngineData(const MotionOcc::CadDocument& document):engine_(document) {}
    GeoEngineHandle& engine() { return engine_; }
private: GeoEngineHandle engine_;
};
}
