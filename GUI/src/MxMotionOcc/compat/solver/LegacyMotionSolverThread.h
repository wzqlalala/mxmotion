#pragma once

#include "mViewportModel.h"
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <atomic>

namespace MechMainWindow {

// Source-compatible endpoint for the original scenario panel. The native
// backend already owns its asynchronous engine run; this object observes that
// real run instead of starting a second physics system or blocking the GUI.
class mMotionSolverThread final : public QObject {
    Q_OBJECT
public:
    mMotionSolverThread(int scenarioId, MechPreData::mViewportModel* viewportModel, QObject* parent=nullptr)
        : QObject(parent), m_scenarioId(scenarioId), m_viewportModel(viewportModel) {
        if (!viewportModel) return;
        connect(viewportModel,&MechPreData::mViewportModel::motionSolveProgress,this,[this](int id,int percent) {
            if (!m_running || id!=m_scenarioId) return;
            m_currentProgress.store(qBound(0,percent,100)); emit progressUpdated(m_currentProgress.load());
        });
        connect(viewportModel,&MechPreData::mViewportModel::motionSolveMessage,this,[this](int id,const QString& text) {
            if (m_running && id==m_scenarioId) emit messageLogged(text);
        });
        connect(viewportModel,&MechPreData::mViewportModel::motionSolveFinished,this,[this](int id,bool success) {
            if (m_running && id==m_scenarioId) {
                if (success) doTransOutput();
                complete(success);
            }
        });
        connect(viewportModel,&QObject::destroyed,this,[this] {
            if (m_running) { emit messageLogged(tr("求解所属模型已关闭。")); complete(false); }
        });
    }
    ~mMotionSolverThread() override { if (m_running) requestStopSolve(); }

    void doTransInput() { if (m_viewportModel && m_viewportModel->flushMotion) m_viewportModel->flushMotion(); }
    void doTransOutput() { if (m_viewportModel && m_viewportModel->syncMotionResults) m_viewportModel->syncMotionResults(m_scenarioId); }
    QStringList checkMotionDataIntegrity() {
        if (!m_viewportModel) return {tr("当前运动模型不可用。")};
        if (!m_viewportModel->validateMotion) return {tr("当前模型未连接运动数据检查接口。")};
        return m_viewportModel->validateMotion(m_scenarioId);
    }
    void doSolve() { start(); }
    void start() {
        if (m_running) return;
        m_running=true; m_started=false; m_cancelRequested=false; m_currentProgress.store(0);
        // Defer validation/completion to preserve the original panel's queued
        // progress-dialog teardown even when validation rejects the request.
        QTimer::singleShot(0,this,[this] {
            if (!m_running) return;
            if (m_cancelRequested) { emit messageLogged(tr("求解已取消。")); complete(false); return; }
            doTransInput();
            const auto errors=checkMotionDataIntegrity();
            if (!errors.isEmpty()) {
                for (const auto& error:errors) emit messageLogged(error);
                complete(false); return;
            }
            if (!m_viewportModel || !m_viewportModel->solveMotion) {
                emit messageLogged(tr("当前模型未连接运动求解接口。")); complete(false); return;
            }
            m_started=true;
            m_viewportModel->solveMotion(m_scenarioId);
        });
    }
    void requestStopSolve() noexcept {
        if (!m_running) return;
        m_cancelRequested=true;
        if (m_started && m_viewportModel && m_viewportModel->stopMotion) m_viewportModel->stopMotion();
    }
    bool isRunning() const { return m_running; }
    std::atomic<int> m_currentProgress{0};

signals:
    void progressUpdated(int percent);
    void messageLogged(const QString& message);
    void solveFinished(int scenarioId,bool success);
    void finished();

private:
    void complete(bool success) {
        if (!m_running) return;
        m_running=false;
        if (success) m_currentProgress.store(100);
        emit solveFinished(m_scenarioId,success);
        emit finished();
    }
    int m_scenarioId;
    QPointer<MechPreData::mViewportModel> m_viewportModel;
    bool m_running=false;
    bool m_started=false;
    bool m_cancelRequested=false;
};
} // namespace MechMainWindow
