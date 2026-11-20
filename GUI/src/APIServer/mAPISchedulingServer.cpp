#include "mAPISchedulingServer.h"
#include <QDateTime>
#include <QDebug>
#include <QUuid>
#include <QTimer>
#include <QJsonArray>

namespace APIServer
{
    mAPISchedulingServer::mAPISchedulingServer(QObject* parent)
        : QObject(parent)
    {
        qRegisterMetaType<APIExecutionResult>();
    }

    mAPISchedulingServer::~mAPISchedulingServer()
    {
        clearAllAPIs();
    }

    QJsonObject mAPISchedulingServer::getAllAPIJsonInfo() const
    {
        QJsonObject jsonobj;
        QMutexLocker locker(&m_registryMutex);
        QJsonArray apiArray;
        for (auto it = m_apiInfo.begin(); it != m_apiInfo.end(); ++it) {
            const APIInfo& info = it.value();
            QJsonObject apiObj;

            apiObj["apiKey"] = info._apiKey;
            apiObj["description"] = info._description;
            apiObj["callCount"] = info._callCount;

            // 必需参数列表
            QJsonArray requiredArray;
            for (const QString& param : info._requiredParams) {
                requiredArray.append(param);
            }
            apiObj["requiredParams"] = requiredArray;

            // 可选参数列表
            QJsonArray optionalArray;
            for (const QString& param : info._optionalParams) {
                optionalArray.append(param);
            }
            apiObj["optionalParams"] = optionalArray;

            apiArray.append(apiObj);
        }

        jsonobj["totalCount"] = m_apiInfo.size();
        jsonobj["apis"] = apiArray;

        return jsonobj;
    }

    APIExecutionResult mAPISchedulingServer::executeAPI(const QString& apiKey,
        const APIParams& params,
        const QString& requestId)
    {
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();

        // 发送执行前信号
        emit beforeApiExecution(apiKey, requestId);

        // 查找 API
        APIFunction func;
        {
            QMutexLocker locker(&m_registryMutex);
            auto it = m_apiRegistry.find(apiKey);
            if (it == m_apiRegistry.end())
            {
                QString error = QString("API not found: %1").arg(apiKey);
                qWarning() << "[API Scheduling]" << error;

                emit apiNotFound(apiKey, requestId);

                APIExecutionResult result = APIExecutionResult::failure(error);
                //updateStatistics(apiKey, false, 0);
                return result;
            }
            func = it.value();
        }

        // 执行 API
        QString error;
        QVariant result;

        try
        {
            result = func(params, error);
            // Existing CAE handlers commonly return false on failure. Treat it
            // as a dispatch failure even if the handler forgot to fill error.
            const bool isBooleanResult =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                result.metaType().id() == QMetaType::Bool;
#else
                result.type() == QVariant::Bool;
#endif
            if (error.isEmpty() && result.isValid() &&
                isBooleanResult && !result.toBool())
            {
                error = QString("API returned false: %1").arg(apiKey);
            }
        }
        catch (const std::exception& e)
        {
            error = QString("Exception: %1").arg(e.what());
        }
        catch (...)
        {
            error = "Unknown exception";
        }

        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;

        // 更新统计
        bool success = error.isEmpty();
        updateStatistics(apiKey, success, elapsed);

        // 发送执行后信号
        emit afterApiExecution(apiKey, requestId, success, elapsed);

        if (success)
        {
            qDebug() << "[API Scheduling] Executed:" << apiKey
                << "Request:" << requestId
                << "Time:" << elapsed << "ms";
            return APIExecutionResult::success(result);
        }
        else
        {
            qWarning() << "[API Scheduling] Failed:" << apiKey
                << "Error:" << error;
            emit apiExecutionError(apiKey, requestId, error);
            return APIExecutionResult::failure(error);
        }
    }
    APIExecutionResult mAPISchedulingServer::executeAPIWithValidation(
        const QString& apiKey,
        const APIParams& params,
        const QStringList& requiredParams,
        const QString& requestId)
    {
        // 验证必需参数
        QString missingParams;
        if (!validateRequiredParams(params, requiredParams, missingParams))
        {
            QString error = QString("Missing required parameters: %1").arg(missingParams);
            qWarning() << "[API Scheduling]" << error;
            return APIExecutionResult::failure(error);
        }

        // 执行 API
        return executeAPI(apiKey, params, requestId);
    }

    void mAPISchedulingServer::executeAPIAsync(const QString& apiKey,
        const APIParams& params,
        const QString& requestId)
    {
        // 异步执行：通过 Qt 的事件循环调用
        // 使用 QTimer::singleShot 确保不阻塞当前调用
        QString reqId = requestId.isEmpty() ? QUuid::createUuid().toString() : requestId;

        QTimer::singleShot(0, this, [this, apiKey, params, reqId]()
            {
                APIExecutionResult result = executeAPI(apiKey, params, reqId);
                emit apiExecutionResult(reqId, result);
            });
    }

    APIStatistics mAPISchedulingServer::getStatistics(const QString& apiKey) const
    {
        QMutexLocker locker(&m_statsMutex);

        if (apiKey.isEmpty())
        {
            return m_globalStatistics;
        }

        return m_apiStatistics.value(apiKey);
    }

    void mAPISchedulingServer::resetStatistics(const QString& apiKey)
    {
        QMutexLocker locker(&m_statsMutex);

        if (apiKey.isEmpty())
        {
            m_globalStatistics.reset();
            m_apiStatistics.clear();
        }
        else
        {
            m_apiStatistics[apiKey].reset();
        }
    }

    void mAPISchedulingServer::updateStatistics(const QString& apiKey,
        bool success,
        qint64 elapsedMs)
    {
        QMutexLocker locker(&m_statsMutex);

        // 更新全局统计
        m_globalStatistics._totalCalls++;
        if (success)
        {
            m_globalStatistics._successCount++;
        }
        else
        {
            m_globalStatistics._failureCount++;
        }
        m_globalStatistics._totalExecutionTime += elapsedMs;

        // 更新 API 信息中的调用次数
        {
            QMutexLocker locker2(&m_registryMutex);
            if (m_apiInfo.contains(apiKey))
            {
                m_apiInfo[apiKey]._callCount++;
            }
        }

        // 更新特定 API 统计
        APIStatistics& stats = m_apiStatistics[apiKey];
        stats._totalCalls++;
        if (success)
        {
            stats._successCount++;
        }
        else
        {
            stats._failureCount++;
            // 记录最后一次错误（需要从外部传入，这里简化处理）
        }
        stats._totalExecutionTime += elapsedMs;
    }

    // ==================== 参数辅助函数实现 ====================

    QStringList mAPISchedulingServer::getParamValue(const APIParams& params,
        const QString& key,
        const QStringList& defaultValue)
    {
        if (params.contains(key))
            return params[key];
        return defaultValue;
    }

    bool mAPISchedulingServer::validateRequiredParams(const APIParams& params,
        const QStringList& requiredParams,
        QString& missingParams)
    {
        QStringList missing;

        for (const QString& required : requiredParams)
        {
            if (!params.contains(required))
            {
                missing.append(required);
            }
        }

        missingParams = missing.join(", ");
        return missing.isEmpty();
    }

    // 获取队列大小
    int mAPISchedulingServer::getQueueSize() const
    {
        QMutexLocker locker(&m_queueMutex);
        return m_taskQueue.size();
    }

    // 添加任务到队列
    void mAPISchedulingServer::appendTask(const APITask& task)
    {
        if (!task.isValid()) {
            qWarning() << "[APIScheduling] Invalid task, ignored";
            return;
        }

        APITask newTask = task;
        newTask.enqueueTime = QDateTime::currentMSecsSinceEpoch();
        newTask.params["connectionId"] = QStringList({ newTask.connectionId });
        bool shouldStart = false;
        int queueSize = 0;
        {
            QMutexLocker locker(&m_queueMutex);
            m_taskQueue.enqueue(newTask);
            queueSize = m_taskQueue.size();
            if (!m_isExecuting.exchange(true))
                shouldStart = true;
        }

        qDebug() << "[APIScheduling] Task appended:" << task.apiKey
            << "Queue size:" << queueSize;

        if (shouldStart)
            QTimer::singleShot(0, this, &mAPISchedulingServer::startExecute);
    }

    void mAPISchedulingServer::startExecute()
    {
        APITask task;
        {
            QMutexLocker locker(&m_queueMutex);
            if (m_taskQueue.isEmpty())
            {
                m_isExecuting = false;
                return;
            }

            // 取出任务
            task = m_taskQueue.dequeue();
        }

        // 记录开始时间
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();

        qDebug() << "[APIScheduling] Processing task:" << task.apiKey
            << "Request:" << task.requestId
            << "Wait time:" << (startTime - task.enqueueTime) << "ms";

        // 执行 API
        APIExecutionResult result = executeAPI(
            task.apiKey,
            task.params,
            task.requestId
        );

        // 记录执行时间
        result._executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startTime;

        qDebug() << "[APIScheduling] Task finished:" << task.apiKey
            << "Success:" << result._success
            << "Exec time:" << result._executionTimeMs << "ms";

        // 发送完成信号（在主线程中处理响应）
        emit taskFinished(task.requestId, task.connectionId, result);

        // Avoid recursive growth and allow pending CAE callbacks to run.
        QTimer::singleShot(0, this, &mAPISchedulingServer::startExecute);
    }
} // namespace APIServer
