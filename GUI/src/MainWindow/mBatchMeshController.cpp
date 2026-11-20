#pragma execution_character_set("utf-8")
#include "mBatchMeshController.h"
#include "mBatchMeshWorker.h"

#include <QPair>
#include <QMetaType>

namespace MGUIMesh
{
    mBatchMeshController::mBatchMeshController(QObject* parent)
        : QObject(parent)
    {
        qRegisterMetaType<BatchMeshTask>();
    }

    mBatchMeshController::~mBatchMeshController()
    {
        cancelAll();
        qDeleteAll(_workers);
        _workers.clear();
    }

    void mBatchMeshController::setMaxConcurrency(int n)
    {
        if (n < 1)
            n = 1;
        if (n > 8)
            n = 8; // 避免底层共享数据锁竞争导致性能下降
        _maxConcurrency = n;
    }

    void mBatchMeshController::submitTasks(const QList<BatchMeshTask>& tasks)
    {
        if (tasks.isEmpty())
        {
            emit allFinished(0, 0);
            return;
        }

        _cancelled = false;
        _active = true;
        _successCount = 0;
        _failCount = 0;
        _pendingTasks = tasks;
        _runningTasks.clear();
        _taskResults.clear();
        _taskElapsed.clear();

        // 确保 worker 池大小满足并发度
        while (_workers.size() < _maxConcurrency)
        {
            mBatchMeshWorker* worker = new mBatchMeshWorker(this);
            connect(worker, &mBatchMeshWorker::taskStarted, this, &mBatchMeshController::taskStarted);
            connect(worker, &mBatchMeshWorker::taskProgress, this, &mBatchMeshController::taskProgress);
            connect(worker, &mBatchMeshWorker::taskFinished, this, &mBatchMeshController::onWorkerFinished);
            _workers.append(worker);
        }

        dispatchTasks();
    }

    void mBatchMeshController::cancelAll()
    {
        _cancelled = true;
        _pendingTasks.clear();
        for (auto* worker : _workers)
        {
            // worker 当前已运行的任务无法安全中断，仅标记取消后续任务
            worker->deleteLater();
        }
        _workers.clear();
    }

    bool mBatchMeshController::isRunning() const
    {
        return _active;
    }

    void mBatchMeshController::onWorkerFinished(QString taskId, bool ok, QString msg, qint64 elapsedMs)
    {
        // 必须先通过 sender() 找到对应 worker 并复位其空闲标志，
        // 再 dispatchTasks()，否则 worker->_idle 仍为 false 会导致下一个任务无法派发
        mBatchMeshWorker* finishedWorker = qobject_cast<mBatchMeshWorker*>(sender());
        if (finishedWorker)
        {
            finishedWorker->reset();
        }

        emit taskFinished(taskId, ok, msg, elapsedMs);

        _taskResults.insert(taskId, qMakePair(ok, msg));
        _taskElapsed.insert(taskId, elapsedMs);
        _runningTasks.remove(taskId);
        if (ok)
            ++_successCount;
        else
            ++_failCount;

        if (!_cancelled && !_pendingTasks.isEmpty())
        {
            dispatchTasks();
        }

        finishBatchIfDone();
    }

    void mBatchMeshController::dispatchTasks()
    {
        for (mBatchMeshWorker* worker : _workers)
        {
            if (_pendingTasks.isEmpty())
                break;

            if (worker->isIdle())
            {
                BatchMeshTask task = _pendingTasks.takeFirst();
                _runningTasks.insert(task.taskId, task);
                worker->submitTask(task);
            }
        }
    }

    void mBatchMeshController::finishBatchIfDone()
    {
        if (!_cancelled && (_pendingTasks.isEmpty() && _runningTasks.isEmpty()))
        {
            _active = false;
            emit allFinished(_successCount, _failCount);
        }
    }
}
