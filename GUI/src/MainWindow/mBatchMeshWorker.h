#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QThread>
#include <QElapsedTimer>
#include "mBatchMeshTask.h"

namespace MGUIMesh
{
    class mMeshThread;

    /**
     * @brief 批量网格剖分工作线程封装
     *        每个 worker 对应一个独立的 QThread 和一个 mMeshThread 实例，
     *        负责执行单个 BatchMeshTask。
     */
    class mBatchMeshWorker : public QObject
    {
        Q_OBJECT
        friend class mBatchMeshController;
    public:
        explicit mBatchMeshWorker(QObject* parent = nullptr);
        ~mBatchMeshWorker();

        /**
         * @brief 提交任务到工作线程执行
         */
        void submitTask(const BatchMeshTask& task);

        /**
         * @brief 是否空闲
         */
        bool isIdle() const { return _idle; }

        /**
         * @brief 当前任务 ID
         */
        QString currentTaskId() const { return _currentTaskId; }

    signals:
        void taskStarted(QString taskId);
        void taskProgress(QString taskId, int percent);
        void taskFinished(QString taskId, bool ok, QString msg, qint64 elapsedMs);

    private slots:
        void onTaskDone(const QString& result, std::vector<int> partIds);

    private:
        void reset();

    private:
        mMeshThread* _meshThread = nullptr;
        QThread* _thread = nullptr;
        QElapsedTimer _elapsedTimer;
        QString _currentTaskId;
        bool _idle = true;
    };
}
