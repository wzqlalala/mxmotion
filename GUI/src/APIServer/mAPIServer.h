#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QJsonObject>
#include <QThread>
#include "mAPIServer_global.h"
#include "mAPISchedulingServer.h"
#include "mNetWorkServer.h"
#include "mAPIServerTypes.h"

namespace APIServer
{
    // 网络通信服务器工作线程 Worker 类
    class APISERVER_EXPORT mAPIServerWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit mAPIServerWorker(QObject* parent = nullptr);
        ~mAPIServerWorker();

        //发送响应包
        void sendResponse(const QString& connectionId, const QJsonObject& response);
        
        //广播信息
        void broadcast(const QJsonObject& message);
        
        //断开客户端连接
        void disconnectClient(const QString& connectionId);

    signals:

        //客户端已连接
        void clientConnected(const QString& connectionId, const QString& address);
        
        //客户端断连
        void clientDisconnected(const QString& connectionId);
        
        //服务器错误
        void serverError(const QString& error);
        
        //网络服务开始
        void serverStarted(quint16 port);
        
        //服务器终止
        void serverStopped();
        
        //请求执行API
        void requestExecuteAPI(const APIRequest& request, const QString& connectionId);

    private slots:
        //开始网络通信服务
        bool startServer(quint16 port, const QString& address);
        
        //停止网络服务
        void stopServer();

        //收到Json包
        void onJsonPacketReceived(const JsonPacket& packet);
        
        //客户端已连接
        void onClientConnected(const QString& connectionId, const QString& address);
        
        //客户端断连
        void onClientDisconnected(const QString& connectionId);
        
        //服务器错误
        void onServerError(const QString& error);
        
        //服务开始
        void onServerStarted(quint16 port);
        
        //服务终止
        void onServerStopped();

    private:
        mNetWorkServer* m_networkServer = nullptr;
        bool m_isRunning = false;
    };

    // 网络通信服务器工作线程
    class APISERVER_EXPORT mAPIServerThread : public QThread
    {
        Q_OBJECT

    public:
        explicit mAPIServerThread(QObject* parent = nullptr);
        ~mAPIServerThread();

        mAPIServerWorker* getWorker() const { return m_worker; }

        //开始服务
        bool startServer(quint16 port, const QString& address);
        
        //终止服务
        void stopServer();
        
        //发送回复消息
        void sendResponse(const QString& connectionId, const QJsonObject& response);
        
        //广播消息
        void broadcast(const QJsonObject& message);
        
        //客户端断连
        void disconnectClient(const QString& connectionId);

    signals:
        //客户端已连接
        void clientConnected(const QString& connectionId, const QString& address);
        //客户端断开连接
        void clientDisconnected(const QString& connectionId);
        //服务器错误
        void serverError(const QString& error);
        //服务已启动
        void serverStarted(quint16 port);
        //服务已停止
        void serverStopped();
        //请求执行API
        void requestExecuteAPI(const APIRequest& request, const QString& connectionId);

        //发送响应json
        void sendResponseInternal(const QString& connectionId, const QJsonObject& response);
        //广播响应json
        void broadcastInternal(const QJsonObject& message);
        //断开客户端连接
        void disconnectClientInternal(const QString& connectionId);

    protected:
        void run() override;

    private:
        mAPIServerWorker* m_worker = nullptr;
    };
    /**
     * @brief API 服务器主控类
     * 负责线程管理、网络通信调度和消息转发
     * API 注册和执行委托给 mAPISchedulingServer
     */
    class APISERVER_EXPORT mAPIServer : public QObject
    {
        Q_OBJECT

    public:
        // 获取单例实例
        static mAPIServer* getInstance();

        /*
         * 注册元数据类型
        */
        static void registerMeta();

        // 禁止拷贝和赋值
        mAPIServer(const mAPIServer&) = delete;
        mAPIServer& operator=(const mAPIServer&) = delete;


        // ==================== 服务器控制 ====================
        bool start(quint16 port = 9527, const QString& address = "127.0.0.1");
        void stop();
        bool isRunning() const;
        quint16 getPort() const;
        QString getAddress() const;

        // ==================== API 管理（委托给调度器）====================
        template<typename T>
        void registerAPI(const QString& apiKey, T* instance,
            QVariant(T::* method)(const APIParams&, QString&),
            const QString& description = QString(),
            const QStringList& requiredparams = QStringList(),
            const QStringList& optionalparams = QStringList()) {
			m_schedulingServer->registerAPI(apiKey, instance, method, description, requiredparams, optionalparams);
        }

        void registerAPI(const QString& apiKey, APIFunction func,
            const QString& description = QString()) {
            m_schedulingServer->registerAPI(apiKey, func, description);
        }

        void unregisterAPI(const QString& apiKey) {
            m_schedulingServer->unregisterAPI(apiKey);
        }

        bool hasAPI(const QString& apiKey) const {
            return m_schedulingServer->hasAPI(apiKey);
        }

        QStringList getRegisteredAPIs() const {
            return m_schedulingServer->getRegisteredAPIs();
        }
        
        // ==================== 消息发送 ====================
        bool sendMessage(const QString& connectionId, const QJsonObject& message);
        bool broadcast(const QJsonObject& message);
        void disconnectClient(const QString& connectionId);

        // ==================== 统计信息 ====================
        APIStatistics getAPIStatistics(const QString& apiKey = QString()) const {
            return m_schedulingServer->getStatistics(apiKey);
        }

        // ==================== 获取调度器 ====================
        mAPISchedulingServer* getSchedulingServer() const { return m_schedulingServer; }

    signals:
        // 客户端事件
        void clientConnected(const QString& connectionId, const QString& address);
        void clientDisconnected(const QString& connectionId);

        // 服务器事件
        void serverStarted(quint16 port);
        void serverStopped();
        void serverError(const QString& error);

        // API 事件（转发自调度器）
        void apiRegistered(const QString& apiKey, const QString& description);
        void apiUnregistered(const QString& apiKey);
        void beforeApiExecution(const QString& apiKey, const QString& requestId);
        void afterApiExecution(const QString& apiKey, const QString& requestId,
            bool success, qint64 elapsedMs);

    private slots:
        //客户端已连接
        void onClientConnected(const QString& connectionId, const QString& address);
        //客户端断连
        void onClientDisconnected(const QString& connectionId);
        //服务器错误
        void onServerError(const QString& error);
        //服务器启动
        void onServerStarted(quint16 port);
        //服务器终止
        void onServerStopped();
        //请求执行API
        void onRequestExecuteAPI(const APIRequest& request, const QString& connectionId);
        //API执行完毕
        void onTaskFinished(const QString& requestId,
            const QString& connectionId,
            const APIExecutionResult& result);

    private:
        explicit mAPIServer(QObject* parent = nullptr);
        ~mAPIServer();

        //发送响应json
        void sendResponse(const QString& connectionId, const QJsonObject& response);

    private:
        static mAPIServer* m_instance;
        static QMutex m_mutex;

        mAPIServerThread* m_thread = nullptr;
        mAPISchedulingServer* m_schedulingServer = nullptr;  // API 调度器
        bool m_isRunning = false;
        quint16 m_port = 0;
        QString m_address;
    };

} // namespace APIServer

#define getAPIServer APIServer::mAPIServer::getInstance()
#define getAPISchedulingServer APIServer::mAPIServer::getInstance()->getSchedulingServer()
