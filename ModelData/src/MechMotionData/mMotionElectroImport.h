#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>
#include <QPair>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT CoSimType {
        Mechatronics,   ///< 机电联合仿真
        Simulink,       ///< Simulink
        FMU,            ///< FMU
        Custom          ///< 自定义
    };

    class MXMOTIONDATA_EXPORT mMotionElectroImport {
    public:
        mMotionElectroImport(int id, const QString& name = "");
        mMotionElectroImport();
        ~mMotionElectroImport();

        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        void setCoSimType(CoSimType type);
        CoSimType getCoSimType() const;

        void setPlatform(const QString& platform);
        QString getPlatform() const;

        void setModelFile(const QString& file);
        QString getModelFile() const;

        void setHeaderFile(const QString& file);
        QString getHeaderFile() const;

        void setInputPorts(const QVector<QPair<QString, QString>>& ports);
        QVector<QPair<QString, QString>> getInputPorts() const;

        void setOutputPorts(const QVector<QPair<QString, QString>>& ports);
        QVector<QPair<QString, QString>> getOutputPorts() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionElectroImport& obj);
        friend QDataStream& operator<<(QDataStream& out, mMotionElectroImport& obj);

    private:
        int m_id;
        QString m_name;
        CoSimType m_coSimType;
        QString m_platform;
        QString m_modelFile;
        QString m_headerFile;
        QVector<QPair<QString, QString>> m_inputPorts;   // (portName, motionPort)
        QVector<QPair<QString, QString>> m_outputPorts;  // (portName, motionPort)
    };

}
