#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT ExportType {
        PortList,    ///< 端口列表导出
        Signal       ///< 信号导出
    };

    class MXMOTIONDATA_EXPORT mMotionElectroExport {
    public:
        mMotionElectroExport(int id, const QString& name = "");
        mMotionElectroExport();
        ~mMotionElectroExport();

        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        void setExportType(ExportType type);
        ExportType getExportType() const;

        void setPortListFile(const QString& file);
        QString getPortListFile() const;

        struct PortConfig {
            QString portName;
            int order = 0;
            double scale = 1.0;
            double offset = 0.0;
            QString unit;
        };

        void setInputPorts(const QVector<PortConfig>& ports);
        QVector<PortConfig> getInputPorts() const;

        void setOutputPorts(const QVector<PortConfig>& ports);
        QVector<PortConfig> getOutputPorts() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionElectroExport& obj);
        friend QDataStream& operator<<(QDataStream& out, mMotionElectroExport& obj);

    private:
        int m_id;
        QString m_name;
        ExportType m_exportType;
        QString m_portListFile;
        QVector<PortConfig> m_inputPorts;
        QVector<PortConfig> m_outputPorts;
    };

}
