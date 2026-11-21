#include "MotionWindow.h"
#include "NativeMotionSolver.h"

#include <QProgressBar>
#include <QTimer>

namespace MotionOcc {

void MotionWindow::inspectFlexModes(int flexId)
{
    if(motionBusy() || m_jobPart>=0 || !m_model.nativeDocument)return;
    snapshotNativeFlexMeshes();
    NativeMotionDocument document=*m_model.nativeDocument;
    const auto flexible=document.entity("autoFlexBodies",flexId);
    if(flexible.isEmpty()){setNotice(tr("请先保存自动柔性体，再检查真实模态。"),true);return;}
    int scenarioId=0;
    auto scenarios=document.collection("scenarios");
    if(scenarios.isEmpty()){
        // Modal inspection does not require users to create a transient case.
        // This ephemeral scenario is only for the common CAD preparation API.
        auto data=document.root.value("motionData").toObject();
        data.insert("scenarios",QJsonArray{QJsonObject{{"id",0},{"name",tr("模态检查")},{"endTime",1.0},{"stepSize",.01}}});
        document.root.insert("motionData",data);
    }else scenarioId=scenarios.first().toObject().value("id").toInt();
    auto input=std::make_shared<NativeMotionInput>();QString error;
    if(!prepareNativeMotion(m_document,document,scenarioId,*input,error)){setNotice(error,true);return;}
    auto run=std::make_shared<RigidRun>();run->inspectionFlexId=flexId;
    run->input.nativeInput=input;run->input.scenario=input->scenario;run->input.bodies=input->bodies;
    closeEditor();leaveRigidAnimation();
    try{
        m_solverThread=std::thread([run,flexId]{
            try{
                RigidFlexResult result;
                run->status=inspectNativeFlexModes(*run->input.nativeInput,flexId,result,run->error,{
                    [run]{return run->stop.load(std::memory_order_relaxed);},
                    [run](double value){run->progress.store(value,std::memory_order_relaxed);}});
                if(run->status==SolveStatus::Completed)run->result.flexBodies.push_back(std::move(result));
            }catch(const std::exception& exception){run->error=QString::fromUtf8(exception.what());}
            catch(...){run->error=QStringLiteral("MotionEngine 模态检查异常。");}
            run->done.store(true,std::memory_order_release);
        });
    }catch(const std::exception& exception){setNotice(QString::fromUtf8(exception.what()),true);return;}
    m_running=std::move(run);m_progress->setRange(0,100);m_progress->setValue(0);m_progress->show();
    m_rigidPoll->start();updateActions();setNotice(tr("正在从真实网格、材料和附着区计算模态…"));
}

} // namespace MotionOcc
