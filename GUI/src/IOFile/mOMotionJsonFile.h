#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include <QThread>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace MechModelData { class mModelData; }

namespace MechMotionData {
    class mMotionBody;
    class mMotionJointBase;
    class mMotionDriveBase;
    class mMotionMarker;
    class mMotionScenario;
    class mMotionCurveData;
    class mMotionCurveRequest;
    class mMotionVectorForce;
    class mMotionScalarForce;
    class mMotionThreePointForce;
    class mMotionThreePointTorque;
    class mMotionScalarTorque;
    class mMotionContact;
    class mMotionSensorBase;
    class mMotionControllerBase;
    class mMotionControlInput;
    class mMotionAutoFlex;
    class mMotionSpringBase;
    class mMotionCouplingBase;
}

namespace MechIOFile
{
    class MIOFILE_EXPORT mOMotionJsonFile : public QThread
    {
        Q_OBJECT
    public:
        mOMotionJsonFile(const QString& filename, MechModelData::mModelData* modelData);
        ~mOMotionJsonFile();

        QString getFileName() const;
        bool isSuccess() const;

    signals:
        void exportFinished(bool success, const QString& msg);

    private:
        void run() override;

        QJsonObject bodyToJson(MechMotionData::mMotionBody* body) const;
        QJsonObject jointToJson(MechMotionData::mMotionJointBase* joint) const;
        QJsonObject driveToJson(MechMotionData::mMotionDriveBase* drive) const;
        QJsonObject markerToJson(MechMotionData::mMotionMarker* marker) const;
        QJsonObject vectorForceToJson(MechMotionData::mMotionVectorForce* force) const;
        QJsonObject threePointTorqueToJson(MechMotionData::mMotionThreePointTorque* torque) const;
        QJsonObject threePointForceToJson(MechMotionData::mMotionThreePointForce* force) const;
        QJsonObject scalarForceToJson(MechMotionData::mMotionScalarForce* force) const;
        QJsonObject scalarTorqueToJson(MechMotionData::mMotionScalarTorque* torque) const;
        QJsonObject scenarioToJson(MechMotionData::mMotionScenario* scenario) const;
        QJsonObject curveToJson(MechMotionData::mMotionCurveData* curve) const;
        QJsonObject curveRequestToJson(const MechMotionData::mMotionCurveRequest& req) const;
        QJsonObject contactToJson(MechMotionData::mMotionContact* contact) const;
        QJsonObject sensorToJson(MechMotionData::mMotionSensorBase* sensor) const;
        QJsonObject controllerToJson(MechMotionData::mMotionControllerBase* controller) const;
        QJsonObject controlInputToJson(MechMotionData::mMotionControlInput* input) const;
        QJsonObject autoFlexToJson(MechMotionData::mMotionAutoFlex* autoFlex) const;
        QJsonObject springToJson(MechMotionData::mMotionSpringBase* spring) const;
        QJsonObject couplingToJson(MechMotionData::mMotionCouplingBase* coupling) const;

        QString _fileName;
        MechModelData::mModelData* _modelData;
        bool _isSuccess = false;
    };
}
