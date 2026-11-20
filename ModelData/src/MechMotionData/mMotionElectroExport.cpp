#include "mMotionElectroExport.h"

namespace MechMotionData {

    mMotionElectroExport::mMotionElectroExport()
        : m_id(-1),
        m_name(),
        m_exportType(ExportType::PortList),
        m_portListFile(),
        m_inputPorts(),
        m_outputPorts()
    {
    }

    mMotionElectroExport::mMotionElectroExport(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_exportType(ExportType::PortList),
        m_portListFile(),
        m_inputPorts(),
        m_outputPorts()
    {
        if (m_name.isEmpty()) {
            m_name = QString("ElectroExport_%1").arg(m_id);
        }
    }

    mMotionElectroExport::~mMotionElectroExport() {}

    void mMotionElectroExport::setId(int id) { m_id = id; }
    int mMotionElectroExport::getId() const { return m_id; }

    void mMotionElectroExport::setName(const QString& name) { m_name = name; }
    QString mMotionElectroExport::getName() const { return m_name; }

    void mMotionElectroExport::setExportType(ExportType type) { m_exportType = type; }
    ExportType mMotionElectroExport::getExportType() const { return m_exportType; }

    void mMotionElectroExport::setPortListFile(const QString& file) { m_portListFile = file; }
    QString mMotionElectroExport::getPortListFile() const { return m_portListFile; }

    void mMotionElectroExport::setInputPorts(const QVector<PortConfig>& ports) { m_inputPorts = ports; }
    QVector<mMotionElectroExport::PortConfig> mMotionElectroExport::getInputPorts() const { return m_inputPorts; }

    void mMotionElectroExport::setOutputPorts(const QVector<PortConfig>& ports) { m_outputPorts = ports; }
    QVector<mMotionElectroExport::PortConfig> mMotionElectroExport::getOutputPorts() const { return m_outputPorts; }

}
