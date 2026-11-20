#include "mMotionElectroImport.h"

namespace MechMotionData {

    mMotionElectroImport::mMotionElectroImport()
        : m_id(-1),
        m_name(),
        m_coSimType(CoSimType::Mechatronics),
        m_platform("Simulink"),
        m_modelFile(),
        m_headerFile(),
        m_inputPorts(),
        m_outputPorts()
    {
    }

    mMotionElectroImport::mMotionElectroImport(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_coSimType(CoSimType::Mechatronics),
        m_platform("Simulink"),
        m_modelFile(),
        m_headerFile(),
        m_inputPorts(),
        m_outputPorts()
    {
        if (m_name.isEmpty()) {
            m_name = QString("ElectroImport_%1").arg(m_id);
        }
    }

    mMotionElectroImport::~mMotionElectroImport() {}

    void mMotionElectroImport::setId(int id) { m_id = id; }
    int mMotionElectroImport::getId() const { return m_id; }

    void mMotionElectroImport::setName(const QString& name) { m_name = name; }
    QString mMotionElectroImport::getName() const { return m_name; }

    void mMotionElectroImport::setCoSimType(CoSimType type) { m_coSimType = type; }
    CoSimType mMotionElectroImport::getCoSimType() const { return m_coSimType; }

    void mMotionElectroImport::setPlatform(const QString& platform) { m_platform = platform; }
    QString mMotionElectroImport::getPlatform() const { return m_platform; }

    void mMotionElectroImport::setModelFile(const QString& file) { m_modelFile = file; }
    QString mMotionElectroImport::getModelFile() const { return m_modelFile; }

    void mMotionElectroImport::setHeaderFile(const QString& file) { m_headerFile = file; }
    QString mMotionElectroImport::getHeaderFile() const { return m_headerFile; }

    void mMotionElectroImport::setInputPorts(const QVector<QPair<QString, QString>>& ports) { m_inputPorts = ports; }
    QVector<QPair<QString, QString>> mMotionElectroImport::getInputPorts() const { return m_inputPorts; }

    void mMotionElectroImport::setOutputPorts(const QVector<QPair<QString, QString>>& ports) { m_outputPorts = ports; }
    QVector<QPair<QString, QString>> mMotionElectroImport::getOutputPorts() const { return m_outputPorts; }

}
