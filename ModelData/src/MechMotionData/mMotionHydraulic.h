#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>
#include <QPair>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT HydraulicType {
        ValveControl,    ///< 阀控液压
        PumpControl,     ///< 泵控液压
        Custom           ///< 自定义
    };

    class MXMOTIONDATA_EXPORT mMotionHydraulic {
    public:
        mMotionHydraulic(int id, const QString& name = "");
        mMotionHydraulic();
        ~mMotionHydraulic();

        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        void setHydraulicType(HydraulicType type);
        HydraulicType getHydraulicType() const;

        void setModelFile(const QString& file);
        QString getModelFile() const;

        void setFluidDensity(double density);
        double getFluidDensity() const;

        void setFluidViscosity(double viscosity);
        double getFluidViscosity() const;

        void setBulkModulus(double modulus);
        double getBulkModulus() const;

        struct PortInfo {
            QString portName;
            double diameter = 10.0;
            double length = 100.0;
        };

        void setPorts(const QVector<PortInfo>& ports);
        QVector<PortInfo> getPorts() const;

        struct PipeInfo {
            QString pipeName;
            double diameter = 10.0;
            double length = 100.0;
        };

        void setPipes(const QVector<PipeInfo>& pipes);
        QVector<PipeInfo> getPipes() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionHydraulic& obj);
        friend QDataStream& operator<<(QDataStream& out, mMotionHydraulic& obj);

    private:
        int m_id;
        QString m_name;
        HydraulicType m_hydraulicType;
        QString m_modelFile;
        double m_fluidDensity;
        double m_fluidViscosity;
        double m_bulkModulus;
        QVector<PortInfo> m_ports;
        QVector<PipeInfo> m_pipes;
    };

}
