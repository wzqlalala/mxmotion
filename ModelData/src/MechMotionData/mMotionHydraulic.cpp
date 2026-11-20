#include "mMotionHydraulic.h"

namespace MechMotionData {

    mMotionHydraulic::mMotionHydraulic()
        : m_id(-1),
        m_name(),
        m_hydraulicType(HydraulicType::ValveControl),
        m_modelFile(),
        m_fluidDensity(850.0),
        m_fluidViscosity(0.032),
        m_bulkModulus(1.6e9),
        m_ports(),
        m_pipes()
    {
    }

    mMotionHydraulic::mMotionHydraulic(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_hydraulicType(HydraulicType::ValveControl),
        m_modelFile(),
        m_fluidDensity(850.0),
        m_fluidViscosity(0.032),
        m_bulkModulus(1.6e9),
        m_ports(),
        m_pipes()
    {
        if (m_name.isEmpty()) {
            m_name = QString("Hydraulic_%1").arg(m_id);
        }
    }

    mMotionHydraulic::~mMotionHydraulic() {}

    void mMotionHydraulic::setId(int id) { m_id = id; }
    int mMotionHydraulic::getId() const { return m_id; }

    void mMotionHydraulic::setName(const QString& name) { m_name = name; }
    QString mMotionHydraulic::getName() const { return m_name; }

    void mMotionHydraulic::setHydraulicType(HydraulicType type) { m_hydraulicType = type; }
    HydraulicType mMotionHydraulic::getHydraulicType() const { return m_hydraulicType; }

    void mMotionHydraulic::setModelFile(const QString& file) { m_modelFile = file; }
    QString mMotionHydraulic::getModelFile() const { return m_modelFile; }

    void mMotionHydraulic::setFluidDensity(double density) { m_fluidDensity = density; }
    double mMotionHydraulic::getFluidDensity() const { return m_fluidDensity; }

    void mMotionHydraulic::setFluidViscosity(double viscosity) { m_fluidViscosity = viscosity; }
    double mMotionHydraulic::getFluidViscosity() const { return m_fluidViscosity; }

    void mMotionHydraulic::setBulkModulus(double modulus) { m_bulkModulus = modulus; }
    double mMotionHydraulic::getBulkModulus() const { return m_bulkModulus; }

    void mMotionHydraulic::setPorts(const QVector<PortInfo>& ports) { m_ports = ports; }
    QVector<mMotionHydraulic::PortInfo> mMotionHydraulic::getPorts() const { return m_ports; }

    void mMotionHydraulic::setPipes(const QVector<PipeInfo>& pipes) { m_pipes = pipes; }
    QVector<mMotionHydraulic::PipeInfo> mMotionHydraulic::getPipes() const { return m_pipes; }

}
