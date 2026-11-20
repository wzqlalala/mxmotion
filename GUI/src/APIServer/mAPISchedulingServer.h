#pragma once
#pragma execution_character_set("utf-8")
#include "mAPIServer_global.h"
#include <QObject>
#include <QMap>
#include <QMutex>
#include <QVector>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QWaitCondition>
#include <QQueue>
#include <functional>
#include <atomic>

#include "mAPIServerTypes.h"

namespace APIServer
{
    /**
     * @brief API 调度服务器类
     * 负责 API 的注册、查找、执行和统计
     * 运行在主线程，所有 API 调用都在主线程执行
     */
    class APISERVER_EXPORT mAPISchedulingServer : public QObject
    {
        Q_OBJECT

    public:
        explicit mAPISchedulingServer(QObject* parent = nullptr);
        ~mAPISchedulingServer();

        // ==================== API 注册 ====================

        /**
         * @brief 注册成员函数为 API
         * @param apiKey API 唯一标识
         * @param instance 对象实例
         * @param method 成员函数指针
         * @param description API 描述（可选）
         */
        template<typename T>
        void registerAPI(
            const QString& apiKey,
            T* instance,
            QVariant(T::* method)(const APIParams&, QString&),
            const QString& description = QString(),
			const QStringList& requiredparams = QStringList(),
            const QStringList& optionalparams = QStringList())
        {
            QMutexLocker locker(&m_registryMutex);

            APIFunction func = [instance, method](const APIParams& params, QString& error) -> QVariant {
                return (instance->*method)(params, error);
                };

            m_apiRegistry[apiKey] = func;

            // 记录 API 信息
			APIInfo info(apiKey, description, requiredparams, optionalparams);
            //info.registeredTime = QDateTime::currentMSecsSinceEpoch();
            m_apiInfo[apiKey] = info;

            locker.unlock();

            qDebug() << "[API Scheduling] Registered:" << apiKey;
            emit apiRegistered(apiKey, description);
        }

        /**
         * @brief 注册 lambda 或自由函数为 API
         * @param apiKey API 唯一标识
         * @param func 函数对象
         * @param description API 描述（可选）
         */
        void registerAPI(
            const QString& apiKey,
            APIFunction func,
            const QString& description = QString())
        {
            QMutexLocker locker(&m_registryMutex);

            m_apiRegistry[apiKey] = func;

            APIInfo info(apiKey, description);
            info._registeredTime = QDateTime::currentMSecsSinceEpoch();
            m_apiInfo[apiKey] = info;

            locker.unlock();

            qDebug() << "[API Scheduling] Registered:" << apiKey;
            emit apiRegistered(apiKey, description);
        }

        /**
         * @brief 批量注册 API
         */
        void registerAPIs(const QMap<QString, APIFunction>& apis) {
            QMutexLocker locker(&m_registryMutex);

            for (auto it = apis.begin(); it != apis.end(); ++it) {
                m_apiRegistry[it.key()] = it.value();

                APIInfo info(it.key());
                info._registeredTime = QDateTime::currentMSecsSinceEpoch();
                m_apiInfo[it.key()] = info;

                emit apiRegistered(it.key(), QString());
            }

            qDebug() << "[API Scheduling] Registered" << apis.size() << "APIs";
        }

        /**
         * @brief 注销 API
         */
        void unregisterAPI(const QString& apiKey) {
            QMutexLocker locker(&m_registryMutex);

            m_apiRegistry.remove(apiKey);
            m_apiInfo.remove(apiKey);

            locker.unlock();

            qDebug() << "[API Scheduling] Unregistered:" << apiKey;
            emit apiUnregistered(apiKey);
        }

        /**
         * @brief 清除所有 API
         */
        void clearAllAPIs() {
            QMutexLocker locker(&m_registryMutex);

            m_apiRegistry.clear();
            m_apiInfo.clear();

            locker.unlock();

            qDebug() << "[API Scheduling] Cleared all APIs";
            emit allAPIsCleared();
        }

        // ==================== API 查询 ====================

        /**
         * @brief 检查 API 是否已注册
         */
        bool hasAPI(const QString& apiKey) const {
            QMutexLocker locker(&m_registryMutex);
            return m_apiRegistry.contains(apiKey);
        }

        /**
         * @brief 获取所有已注册的 API 列表
         */
        QStringList getRegisteredAPIs() const {
            QMutexLocker locker(&m_registryMutex);
            return m_apiRegistry.keys();
        }

        /**
         * @brief 获取 API 详细信息
         */
        APIInfo getAPIInfo(const QString& apiKey) const {
            QMutexLocker locker(&m_registryMutex);
            return m_apiInfo.value(apiKey);
        }

        /**
         * @brief 获取所有 API 信息
         */
        QMap<QString, APIInfo> getAllAPIInfo() const {
            QMutexLocker locker(&m_registryMutex);
            return m_apiInfo;
        }
		QJsonObject getAllAPIJsonInfo()const;

        /**
         * @brief 获取 API 数量
         */
        int getAPICount() const {
            QMutexLocker locker(&m_registryMutex);
            return m_apiRegistry.size();
        }

        // ==================== API 执行 ====================

        /**
         * @brief 执行 API
         * @param apiKey API 标识
         * @param params 参数列表
         * @param requestId 请求 ID（用于追踪）
         * @return 执行结果
         */
        APIExecutionResult executeAPI(const QString& apiKey,
            const APIParams& params,
            const QString& requestId = QString());

        /**
         * @brief 执行 API（带参数验证）
         * @param apiKey API 标识
         * @param params 参数列表
         * @param requiredParams 必需参数列表
         * @param requestId 请求 ID
         * @return 执行结果
         */
        APIExecutionResult executeAPIWithValidation(const QString& apiKey,
            const APIParams& params,
            const QStringList& requiredParams,
            const QString& requestId = QString());

        /**
         * @brief 异步执行 API（通过信号返回结果）
         * @param apiKey API 标识
         * @param params 参数列表
         * @param requestId 请求 ID
         */
        void executeAPIAsync(const QString& apiKey,
            const APIParams& params,
            const QString& requestId = QString());

        // ==================== 统计信息 ====================

        /**
         * @brief 获取 API 统计信息
         */
        APIStatistics getStatistics(const QString& apiKey = QString()) const;

        /**
         * @brief 重置统计信息
         */
        void resetStatistics(const QString& apiKey = QString());

        /**
         * @brief 获取全局统计信息
         */
        APIStatistics getGlobalStatistics() const { return m_globalStatistics; }

        // ==================== 参数辅助函数 ====================

        /**
         * @brief 从参数列表中获取参数值
         */
        static QStringList getParamValue(const APIParams& params, const QString& key,
            const QStringList& defaultValue = QStringList(""));

        /**
         * @brief 验证必需参数
         */
        static bool validateRequiredParams(const APIParams& params,
            const QStringList& requiredParams,
            QString& missingParams);

    signals:
        // API 生命周期信号
        void apiRegistered(const QString& apiKey, const QString& description);
        void apiUnregistered(const QString& apiKey);
        void allAPIsCleared();

        // API 执行信号
        void beforeApiExecution(const QString& apiKey, const QString& requestId);
        void afterApiExecution(const QString& apiKey, const QString& requestId,
            bool success, qint64 elapsedMs);

        // 异步执行结果
        void apiExecutionResult(const QString& requestId,
            const APIExecutionResult& result);

        // 错误信号
        void apiNotFound(const QString& apiKey, const QString& requestId);
        void apiExecutionError(const QString& apiKey, const QString& requestId,
            const QString& error);

    private:
        // API 注册表（线程安全）
        QMap<QString, APIFunction> m_apiRegistry;
        QMap<QString, APIInfo> m_apiInfo;
        mutable QMutex m_registryMutex;

        // 统计信息（每个 API 的统计）
        QMap<QString, APIStatistics> m_apiStatistics;
        mutable QMutex m_statsMutex;

        // 全局统计
        APIStatistics m_globalStatistics;

        // 辅助函数：更新统计
        void updateStatistics(const QString& apiKey, bool success, qint64 elapsedMs);


        /*********************************************************队列调用*********************************************************************/
	private:
		// 队列相关
		QQueue<APITask> m_taskQueue;
		mutable QMutex m_queueMutex;

        //正在执行中
        std::atomic<bool> m_isExecuting{ false };  // 原子变量，初始为 false

	public:
		// 添加任务到队列
		void appendTask(const APITask& task);

		// 获取队列大小
		int getQueueSize() const;

        //开始执行
        void startExecute();

	signals:
		// 任务执行完成信号（用于通知 APIServer 发送响应）
		void taskFinished(const QString& requestId, const QString& connectionId,
		const APIExecutionResult& result);
	};

} // namespace APIServer

// 注册元类型
Q_DECLARE_METATYPE(APIServer::APIExecutionResult)