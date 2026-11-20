#pragma execution_character_set("utf-8")
#include "mBatchMeshWorker.h"
#include "mMeshThread.h"
#include "mGlobalSignals.h"
#include "MXMeshParameters.h"

#include <QMetaObject>
#include <QMetaType>

Q_DECLARE_METATYPE(MeshParam)
Q_DECLARE_METATYPE(std::set<int>)

namespace MGUIMesh
{
    static MeshParam buildMeshParam(const BatchMeshTask& task)
    {
        return MeshParam(task.maxSize, task.minSize,
            task.smoothness, task.smoothnessRatio,
            task.proximity, task.proximityValue,
            task.curvature, task.curvatureEigenValue);
    }

    mBatchMeshWorker::mBatchMeshWorker(QObject* parent)
        : QObject(parent)
    {
        qRegisterMetaType<MeshParam>();
        qRegisterMetaType<std::set<int>>();
        qRegisterMetaType<MGUIMesh::BatchMeshTask>("MGUIMesh::BatchMeshTask");

        _meshThread = new mMeshThread(this);
        _thread = new QThread(this);
        _meshThread->moveToThread(_thread);

        connect(_meshThread, &mMeshThread::done, this, &mBatchMeshWorker::onTaskDone,
                Qt::QueuedConnection);

        _thread->start();
    }

    mBatchMeshWorker::~mBatchMeshWorker()
    {
        if (_thread)
        {
            _thread->quit();
            _thread->wait(3000);
            if (_thread->isRunning())
            {
                _thread->terminate();
            }
        }
    }

    void mBatchMeshWorker::submitTask(const BatchMeshTask& task)
    {
        if (!_idle)
            return;

        _idle = false;
        _currentTaskId = task.taskId;
        _elapsedTimer.start();

        emit taskStarted(task.taskId);

        // 批量 CLI 不再区分 autoMesh/dim/geoIds，统一走全几何批量剖分；
        // 任务参数（maxSize 等）随槽函数传入，由剖分线程决定是否覆盖自动尺寸
        QMetaObject::invokeMethod(_meshThread, "slot_createMeshBatch",
            Qt::QueuedConnection,
            Q_ARG(MGUIMesh::BatchMeshTask, task));
    }

    void mBatchMeshWorker::onTaskDone(const QString& result, std::vector<int> partIds)
    {
        Q_UNUSED(partIds)
        bool ok = (result == "OK");
        QString msg = ok ? QString("剖分完成") : QString("剖分失败：%1").arg(result);
        qint64 elapsedMs = _elapsedTimer.isValid() ? _elapsedTimer.elapsed() : 0;

        // _idle 的复位由 mBatchMeshController::onWorkerFinished 在派发下一任务前完成，
        // 避免信号槽顺序导致 worker 仍处于忙碌状态时 controller 提前尝试派发
        emit taskFinished(_currentTaskId, ok, msg, elapsedMs);
    }

    void mBatchMeshWorker::reset()
    {
        _idle = true;
        _currentTaskId.clear();
        _elapsedTimer.invalidate();
    }
}
