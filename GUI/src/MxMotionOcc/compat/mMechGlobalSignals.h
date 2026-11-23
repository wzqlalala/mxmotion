#pragma once
#include "mPhysicalFieldEnum.h"
#include <QObject>
#include <QString>
#include <QVariantMap>
namespace MechCommon {
class mMechGlobalSignals : public QObject {
    Q_OBJECT
public:
    static mMechGlobalSignals* getInstance() { static mMechGlobalSignals value; return &value; }
signals:
    void clickCurveStyleSig();
    void activePickSig(int pickFilter);
    void viewportModelAboutToChange(int newViewportID);
    void viewportModelAboutToBeDeleted(int modelID);
    void outputPermanentInfoSig(const QString& msg);
    void createUpdateMaterialSig(bool isCreate, int matID);
    void createUpdateMotionBodySig(bool isCreate, int motionBodyID);
    void createUpdateMotionBodyBatchSig(const QVector<int>& motionBodyIDs);
    void createUpdateMotionJointSig(bool isCreate, int motionJointID);
    void createUpdateMotionCouplingSig(bool isCreate, int motionCouplingID);
    void createUpdateMotionDriveSig(bool isCreate, int motionDriveID);
    void createUpdateMotionMarkerSig(bool isCreate, int motionMarkerID);
    void createUpdateMotionVectorForceSig(bool isCreate, int motionVectorForceID);
    void createUpdateMotionContactSig(bool isCreate, int motionContactID);
    void createUpdateMotionSpringSig(bool isCreate, int motionSpringID);
    void createUpdateMotionControlInputSig(bool isCreate, int motionControlInputID);
    void createUpdateMotionElectroImportSig(bool isCreate, int motionElectroImportID);
    void createUpdateMotionElectroExportSig(bool isCreate, int motionElectroExportID);
    void createUpdateMotionHydraulicSig(bool isCreate, int motionHydraulicID);
    void createUpdateMotionRLSig(bool isCreate, int motionRLID);
    void createUpdateMotionSensorSig(bool isCreate, int motionSensorID);
    void createUpdateMotionControllerSig(bool isCreate, int motionControllerID);
    void createUpdateMotionAutoFlexSig(bool isCreate, int motionAutoFlexID);
    void createUpdateMotionScenarioSig(bool isCreate, int motionScenarioID);
    void createUpdateMotionResultSig(bool isCreate, int motionScenarioID);
    void mpcommImportMotionDataFinished(bool success, const QString& path, bool fromPlatform);
    void mpcommLoadCaseExported(const QString& path);
    void mpcommSolverStatusChanged(const QString& state, const QVariantMap& outputs);
};
}
