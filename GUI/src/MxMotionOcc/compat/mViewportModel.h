#pragma once
#include "mModelData.h"
#include "mBasicEnum.h"
#include "render/LegacyRender.h"
#include <QObject>
#include <QMap>
#include <QVector>
#include <QJsonArray>
#include <functional>
namespace MotionOcc {
struct NativeLoadSource;
struct NativeLoadExportOptions;
struct NativeLoadCaseOptions;
}
namespace MechPreData {
class mViewportModel : public QObject {
    Q_OBJECT
public:
    mViewportModel(MechModelData::mModelData* data,mxr::CCSMRendWidget* render,QObject* parent=nullptr)
        :QObject(parent),data_(data),render_(render) {}
    MechModelData::mModelData* getModelData() const { return data_; }
    mxr::CCSMRendWidget* getPreRenderWidget() const { return render_; }
    MxFunctions::PickDataType getCurrentPickDataType(int& count) const {
        count=0;if(!render_)return MxFunctions::PickDataType::None;
        auto* container=render_->getSelectionContainer();
        for(const auto& entry:container->geometry.values)count+=int(entry.second.size());
        for(const auto& entry:container->mesh.values)count+=int(entry.second.size());
        return render_->legacyPickContext()->pickType();
    }
    // The original solver panel calls these through its adapter, on the GUI
    // thread. Callbacks operate the same NativeMotionDocument / engine run.
    std::function<void(int)> solveMotion;
    std::function<void()> stopMotion;
    std::function<void(int)> meshAutoFlex;
    std::function<void(int)> inspectAutoFlex;
    std::function<void()> flushMotion;
    std::function<QStringList(int)> validateMotion;
    std::function<void(int)> syncMotionResults;
    // Keep the original animation panel's playback, timeline and tracing
    // controls; only the renderer is supplied by the current OCC host.
    std::function<void(int,double,int,bool)> displayMotionTime;
    std::function<void()> resetMotionDisplay;
    // Original curve controls use the recorded native channel catalogue; the
    // backend remains responsible for accepted times, units and validation.
    std::function<QJsonArray(int)> motionCurveChannels;
    std::function<bool(int,const QString&,int,const QString&,int,
        QVector<double>&,QVector<double>&,QString&)> queryMotionCurve;
    std::function<bool(int,const QString&,const MotionOcc::NativeLoadSource&,
        const MotionOcc::NativeLoadExportOptions&,QString&)> exportMotionLoads;
    std::function<bool(int,const MotionOcc::NativeLoadCaseOptions&,QJsonObject&,QString&)> buildMotionLoadCases;
    std::function<bool(const QJsonObject&,const QMap<int,QVector<int>>&,QString&,int*,int*,int*)> generateMotionStaticCases;
    std::function<bool(const std::set<int>&,double,QString&)> ensureTetraMesh;
    std::function<void(int,bool)> previewBodyMesh;
    std::function<void()> closePanel;
    std::function<void(int,bool)> commitAutoFlexAttachments;
signals:
    void motionSolveProgress(int scenarioId,int percent);
    void motionSolveMessage(int scenarioId,const QString& message);
    void motionSolveFinished(int scenarioId,bool success);
private:
    MechModelData::mModelData* data_;
    QPointer<mxr::CCSMRendWidget> render_;
};
}
