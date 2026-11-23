#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include <QThread>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace MechModelData { class mModelData; }
namespace MotionOcc { class LegacyMotionJsonAdapter; }

namespace MechMotionData {
    class mMotionBody;
    class mMotionJointBase;
    class mMotionDriveBase;
    class mMotionMarker;
    class mMotionVectorForce;
    class mMotionScalarForce;
    class mMotionThreePointForce;
    class mMotionThreePointTorque;
    class mMotionContact;
    class mMotionScenario;
    class mMotionCurveData;
    class mMotionCurveRequest;
    class mMotionSensorBase;
    class mMotionControllerBase;
    class mMotionControlInput;
    class mMotionScalarTorque;
    class mMotionAutoFlex;
    class mMotionSpringBase;
    class mMotionCouplingBase;
}

namespace MechIOFile
{
    class MIOFILE_EXPORT mIMotionJsonFile : public QThread
    {
        Q_OBJECT
    public:
        mIMotionJsonFile(const QString& filename, MechModelData::mModelData* modelData);
        ~mIMotionJsonFile();

        QString getFileName() const;
        bool isSuccess() const;

    signals:
        void importFinished(bool success, const QString& msg);

    private:
        friend class MotionOcc::LegacyMotionJsonAdapter;
        void run() override;

        bool clearExistingMotionData();

        MechMotionData::mMotionBody* jsonToBody(const QJsonObject& obj);
        MechMotionData::mMotionJointBase* jsonToJoint(const QJsonObject& obj);
        MechMotionData::mMotionDriveBase* jsonToDrive(const QJsonObject& obj);
        MechMotionData::mMotionMarker* jsonToMarker(const QJsonObject& obj);
        MechMotionData::mMotionVectorForce* jsonToVectorForce(const QJsonObject& obj);
        MechMotionData::mMotionThreePointTorque* jsonToThreePointTorque(const QJsonObject& obj);
        MechMotionData::mMotionThreePointForce* jsonToThreePointForce(const QJsonObject& obj);
        MechMotionData::mMotionScalarForce* jsonToScalarForce(const QJsonObject& obj);
        MechMotionData::mMotionScalarTorque* jsonToScalarTorque(const QJsonObject& obj);
        MechMotionData::mMotionScenario* jsonToScenario(const QJsonObject& obj);
        MechMotionData::mMotionCurveData* jsonToCurve(const QJsonObject& obj);
        MechMotionData::mMotionCurveRequest jsonToCurveRequest(const QJsonObject& obj);
        MechMotionData::mMotionContact* jsonToContact(const QJsonObject& obj);
        MechMotionData::mMotionSensorBase* jsonToSensor(const QJsonObject& obj);
        MechMotionData::mMotionControllerBase* jsonToController(const QJsonObject& obj);
        MechMotionData::mMotionControlInput* jsonToControlInput(const QJsonObject& obj);
        MechMotionData::mMotionAutoFlex* jsonToAutoFlex(const QJsonObject& obj);
        MechMotionData::mMotionSpringBase* jsonToSpring(const QJsonObject& obj);
        MechMotionData::mMotionCouplingBase* jsonToCoupling(const QJsonObject& obj);

        QString _fileName;
        MechModelData::mModelData* _modelData;
        bool _isSuccess = false;
    };
}
