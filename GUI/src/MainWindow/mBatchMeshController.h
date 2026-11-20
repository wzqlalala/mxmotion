#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QObject>
#include <QList>
#include <QVector>
#include <QHash>
#include <QPair>
#include "mBatchMeshTask.h"

namespace MGUIMesh
{
    class mBatchMeshWorker;

    /**
     * @brief 批量网格剖分任务控制器
     *        负责任务队列管理与 worker 线程池调度。
     */
    class MMAINWINDOW_EXPORT mBatchMeshController : public QObject
    {
        Q_OBJECT
    public:
        explicit mBatchMeshController(QObject* parent = nullptr);
        ~mBatchMeshController();

        /**
         * @brief 设置最大并发 worker 数（首期建议保持 1，避免共享 meshdata 写冲突）
         */
        void setMaxConcurrency(int n);

        /**
         * @brief 提交批量任务
         */
        void submitTasks(const QList<BatchMeshTask>& tasks);

        /**
         * @brief 取消未开始任务，已运行任务按当前状态结束
         */
        void cancelAll();

        /**
         * @brief 是否还有任务在运行或等待
         */
        bool isRunning() const;

        /**
         * @brief 获取每个任务的结果
         */
        QHash<QString, QPair<bool, QString>> taskResults() const { return _taskResults; }

        /**
         * @brief 获取每个任务耗时（毫秒）
         */
        QHash<QString, qint64> taskElapsed() const { return _taskElapsed; }

    signals:
        void taskStarted(QString taskId);
        void taskProgress(QString taskId, int percent);
        void taskFinished(QString taskId, bool ok, QString msg, qint64 elapsedMs);
        void allFinished(int successCount, int failCount);

    private slots:
        void onWorkerFinished(QString taskId, bool ok, QString msg, qint64 elapsedMs);

    private:
        void dispatchTasks();
        void finishBatchIfDone();

    private:
        QVector<mBatchMeshWorker*> _workers;
        QList<BatchMeshTask> _pendingTasks;
        QHash<QString, BatchMeshTask> _runningTasks;
        QHash<QString, QPair<bool, QString>> _taskResults; // taskId -> {ok, msg}
        QHash<QString, qint64> _taskElapsed;             // taskId -> elapsedMs
        int _maxConcurrency = 1;
        int _successCount = 0;
        int _failCount = 0;
        bool _cancelled = false;
        bool _active = false;
    };
}
