#include "mAPIServer.h"
#include <QThread>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QMetaType>
#include "mOtherFunction.h"

namespace APIServer
{
    // 静态成员初始化
    mAPIServer* mAPIServer::m_instance = nullptr;
    QMutex mAPIServer::m_mutex;
    mAPIServer* mAPIServer::getInstance()
    {
        if (!m_instance) {
            QMutexLocker locker(&m_mutex);
            if (!m_instance) {
                m_instance = new mAPIServer();
            }
        }
        return m_instance;
    }

    void mAPIServer::registerMeta()
    {
        qRegisterMetaType<APIRequest>("APIRequest");
        qRegisterMetaType<APIResponse>("APIResponse");
        qRegisterMetaType<JsonPacket>("JsonPacket");
    }

    // ==================== mAPIServerWorker 实现====================
    mAPIServerWorker::mAPIServerWorker(QObject* parent) : QObject(parent)
    {
        m_networkServer = new mNetWorkServer(this);

        connect(m_networkServer, &mNetWorkServer::jsonPacketReceived,
            this, &mAPIServerWorker::onJsonPacketReceived);
        connect(m_networkServer, &mNetWorkServer::clientConnected,
            this, &mAPIServerWorker::onClientConnected);
        connect(m_networkServer, &mNetWorkServer::clientDisconnected,
            this, &mAPIServerWorker::onClientDisconnected);
        connect(m_networkServer, &mNetWorkServer::serverError,
            this, &mAPIServerWorker::onServerError);
        connect(m_networkServer, &mNetWorkServer::serverStarted,
            this, &mAPIServerWorker::onServerStarted);
        connect(m_networkServer, &mNetWorkServer::serverStopped,
            this, &mAPIServerWorker::onServerStopped);
    }

    mAPIServerWorker::~mAPIServerWorker() { stopServer(); }

    bool mAPIServerWorker::startServer(quint16 port, const QString& address)
    {
        if (m_isRunning) return true;
        bool success = m_networkServer->start(port, address);
        if (success) m_isRunning = true;
        return success;
    }

    void mAPIServerWorker::stopServer()
    {
        if (!m_isRunning) return;
        m_networkServer->stop();
        m_isRunning = false;
    }

    void mAPIServerWorker::onJsonPacketReceived(const JsonPacket& packet)
    {
        // 解析为 API 请求
        APIRequest request;
        request._apiKey = packet._data["API_Key"].toString();
        request._requestId = packet._data["Request_ID"].toString();
        request._connectionId = packet._connectionId;
        request._timestamp = packet._data["Timestamp"].toVariant().toLongLong();

        // 解析参数 - 支持新的多值格式
        QJsonValue paramsValue = packet._data["API_Param"];

        if (paramsValue.isArray()) {
            // 格式1: 数组格式 [{"name": "key1", "value": "val1"}, {"name": "key2", "value": "val2"}]
            QJsonArray paramsArray = paramsValue.toArray();
            for (const QJsonValue& item : paramsArray) {
                if (item.isObject()) {
                    QJsonObject paramObj = item.toObject();
                    QString key = paramObj["name"].toString();

                    // 检查 value 是字符串还是数组
                    QJsonValue value = paramObj["value"];
                    if (value.isArray()) {
                        // 值是数组: {"name": "create", "value": ["a", "b", "c"]}
                        QJsonArray valueArray = value.toArray();
                        QStringList values;
                        for (const QJsonValue& v : valueArray) {
                            values.append(v.toString());
                        }
                        request._params[key] = values;
                    }
                    else if (value.isString()) {
                        // 值是字符串: {"name": "key", "value": "val"}
                        request._params[key] = QStringList{ value.toString() };
                    }
                    else if (value.isDouble()) {
                        request._params[key] = QStringList{ MxFunctions::mNumberToString(value.toDouble()) };
                    }
                    else if (value.isBool()) {
                        request._params[key] = QStringList{ value.toBool() ? "true" : "false" };
                    }
                }
            }
        }
        else if (paramsValue.isObject()) {
            // 格式2: 对象格式 {"key1": "value1", "key2": ["a", "b", "c"]}
            QJsonObject paramsObj = paramsValue.toObject();
            for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it) {
                QString key = it.key();
                QJsonValue value = it.value();

                if (value.isArray()) {
                    // 值是数组: {"create": ["a", "b", "c"]}
                    QJsonArray valueArray = value.toArray();
                    QStringList values;
                    for (const QJsonValue& v : valueArray) {
                        values.append(v.toString());
                    }
                    request._params[key] = values;
                }
                else if (value.isString()) {
                    // 值是字符串: {"key": "value"}
                    request._params[key] = QStringList{ value.toString() };
                }
                else if (value.isDouble()) {
                    request._params[key] = QStringList{ MxFunctions::mNumberToString(value.toDouble()) };
                }
                else if (value.isBool()) {
                    request._params[key] = QStringList{ value.toBool() ? "true" : "false" };
                }
                else if (value.isObject()) {
                    // 值是嵌套对象，转为 JSON 字符串
                    QJsonObject nestedObj = value.toObject();
                    QJsonDocument doc(nestedObj);
                    request._params[key] = QStringList{ QString::fromUtf8(doc.toJson(QJsonDocument::Compact)) };
                }
            }
        }
        else if (paramsValue.isString()) {
            // 格式3: 直接是字符串
            request._params["value"] = QStringList{ paramsValue.toString() };
        }

        if (!request._apiKey.isEmpty()) {
            emit requestExecuteAPI(request, packet._connectionId);
        }
    }

    void mAPIServerWorker::onClientConnected(const QString& connectionId, const QString& address)
    {
        emit clientConnected(connectionId, address);
    }

    void mAPIServerWorker::onClientDisconnected(const QString& connectionId)
    {
        emit clientDisconnected(connectionId);
    }

    void mAPIServerWorker::onServerError(const QString& error)
    {
        emit serverError(error);
    }

    void mAPIServerWorker::onServerStarted(quint16 port)
    {
        emit serverStarted(port);
    }

    void mAPIServerWorker::onServerStopped()
    {
        emit serverStopped();
    }

    void mAPIServerWorker::sendResponse(const QString& connectionId, const QJsonObject& response)
    {
        if (m_networkServer && m_isRunning) m_networkServer->sendResponse(connectionId, response);
    }

    void mAPIServerWorker::broadcast(const QJsonObject& message)
    {
        if (m_networkServer && m_isRunning) m_networkServer->broadcast(message);
    }

    void mAPIServerWorker::disconnectClient(const QString& connectionId)
    {
        if (m_networkServer && m_isRunning) m_networkServer->disconnectClient(connectionId);
    }

    // ==================== mAPIServerThread 实现 ====================
    mAPIServerThread::mAPIServerThread(QObject* parent) : QThread(parent)
    {
        m_worker = new mAPIServerWorker();
        m_worker->moveToThread(this);

        connect(m_worker, &mAPIServerWorker::clientConnected,
            this, &mAPIServerThread::clientConnected);
        connect(m_worker, &mAPIServerWorker::clientDisconnected,
            this, &mAPIServerThread::clientDisconnected);
        connect(m_worker, &mAPIServerWorker::serverError,
            this, &mAPIServerThread::serverError);
        connect(m_worker, &mAPIServerWorker::serverStarted,
            this, &mAPIServerThread::serverStarted);
        connect(m_worker, &mAPIServerWorker::serverStopped,
            this, &mAPIServerThread::serverStopped);
        connect(m_worker, &mAPIServerWorker::requestExecuteAPI,
            this, &mAPIServerThread::requestExecuteAPI);

        connect(this, &mAPIServerThread::sendResponseInternal,
            m_worker, &mAPIServerWorker::sendResponse);
        connect(this, &mAPIServerThread::broadcastInternal,
            m_worker, &mAPIServerWorker::broadcast);
        connect(this, &mAPIServerThread::disconnectClientInternal,
            m_worker, &mAPIServerWorker::disconnectClient);
    }

    mAPIServerThread::~mAPIServerThread()
    {
        stopServer();
        quit();
        wait();
        delete m_worker;
    }

    bool mAPIServerThread::startServer(quint16 port, const QString& address)
    {
        bool result = false;
        QMetaObject::invokeMethod(m_worker, "startServer",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(quint16, port),
            Q_ARG(QString, address));
        return result;
    }

    void mAPIServerThread::stopServer()
    {
        QMetaObject::invokeMethod(m_worker, "stopServer", Qt::QueuedConnection);
    }

    void mAPIServerThread::sendResponse(const QString& connectionId, const QJsonObject& response)
    {
        emit sendResponseInternal(connectionId, response);
    }

    void mAPIServerThread::broadcast(const QJsonObject& message)
    {
        emit broadcastInternal(message);
    }

    void mAPIServerThread::disconnectClient(const QString& connectionId)
    {
        emit disconnectClientInternal(connectionId);
    }

    void mAPIServerThread::run()
    {
        qDebug() << "API Server thread started:" << QThread::currentThread(); QThread::run();
    }

    // ==================== mAPIServer 实现 ====================
    mAPIServer::mAPIServer(QObject* parent)
        : QObject(parent)
    {
       

        // 创建调度器（在主线程）
        m_schedulingServer = new mAPISchedulingServer(this);

        // 创建线程管理器
        m_thread = new mAPIServerThread(this);

        // 连接信号
        connect(m_thread, &mAPIServerThread::clientConnected,
            this, &mAPIServer::onClientConnected);
        connect(m_thread, &mAPIServerThread::clientDisconnected,
            this, &mAPIServer::onClientDisconnected);
        connect(m_thread, &mAPIServerThread::serverError,
            this, &mAPIServer::onServerError);
        connect(m_thread, &mAPIServerThread::serverStarted,
            this, &mAPIServer::onServerStarted);
        connect(m_thread, &mAPIServerThread::serverStopped,
            this, &mAPIServer::onServerStopped);
        connect(m_thread, &mAPIServerThread::requestExecuteAPI,
            this, &mAPIServer::onRequestExecuteAPI);

        // 转发调度器的信号
        connect(m_schedulingServer, &mAPISchedulingServer::apiRegistered,
            this, &mAPIServer::apiRegistered);
        connect(m_schedulingServer, &mAPISchedulingServer::apiUnregistered,
            this, &mAPIServer::apiUnregistered);
        connect(m_schedulingServer, &mAPISchedulingServer::beforeApiExecution,
            this, &mAPIServer::beforeApiExecution);
        connect(m_schedulingServer, &mAPISchedulingServer::afterApiExecution,
            this, &mAPIServer::afterApiExecution);

        // 连接调度器的任务完成信号
        connect(m_schedulingServer, &mAPISchedulingServer::taskFinished,
            this, &mAPIServer::onTaskFinished);
    }

    mAPIServer::~mAPIServer()
    {
        stop();
    }

    bool mAPIServer::start(quint16 port, const QString& address)
    {
        if (m_isRunning) return false;

        m_port = port;
        m_address = address;

        m_thread->start();
        if (!m_thread->startServer(port, address))
        {
            emit serverError("Failed to start server");
            return false;
        }

        m_isRunning = true;
        return true;
    }

    void mAPIServer::stop()
    {
        if (!m_isRunning) return;

        m_thread->stopServer();
        m_thread->quit();
        m_thread->wait();
        m_isRunning = false;
        emit serverStopped();
    }

    bool mAPIServer::isRunning() const
    {
        return m_isRunning && m_thread->isRunning();
    }

    quint16 mAPIServer::getPort() const
    {
        return m_port;
    }

    QString mAPIServer::getAddress() const
    {
        return m_address;
    }

    bool mAPIServer::sendMessage(const QString& connectionId, const QJsonObject& message)
    {
        if (!m_isRunning) return false;
        m_thread->sendResponse(connectionId, message);
        return true;
    }

    bool mAPIServer::broadcast(const QJsonObject& message)
    {
        if (!m_isRunning) return false;
        m_thread->broadcast(message);
        return true;
    }

    void mAPIServer::disconnectClient(const QString& connectionId)
    {
        if (m_isRunning)
        {
            m_thread->disconnectClient(connectionId);
        }
    }

    void mAPIServer::onClientConnected(const QString& connectionId, const QString& address)
    {
        //跟连接上的客户端发送API信息
        QJsonObject APIInfos = m_schedulingServer->getAllAPIJsonInfo();;
        sendMessage(connectionId, APIInfos);
        emit clientConnected(connectionId, address);
    }

    void mAPIServer::onClientDisconnected(const QString& connectionId)
    {
        emit clientDisconnected(connectionId);
    }

    void mAPIServer::onServerError(const QString& error)
    {
        emit serverError(error);
    }

    void mAPIServer::onServerStarted(quint16 port)
    {
        emit serverStarted(port);
    }

    void mAPIServer::onServerStopped()
    {
        emit serverStopped();
    }

    void mAPIServer::onRequestExecuteAPI(const APIRequest& request, const QString& connectionId)
    {
        // 构造任务
        APITask task(
            request._apiKey,
            request._params,
            request._requestId,
            connectionId
        );

        // 添加到调度器的队列
        m_schedulingServer->appendTask(task);

        qDebug() << "[APIServer] Task appended to scheduler queue:" << request._apiKey;
        //// 委托给调度器执行 API
        //APIExecutionResult result = m_schedulingServer->executeAPI(
        //    request._apiKey,
        //    request._params,
        //    request._requestId
        //);

        //// 如果是通知（没有 requestId），不需要发送响应
        //if (request._requestId.isEmpty()) {
        //    return;
        //}

        //// 构建 JSON-RPC 2.0 响应
        //QJsonObject response;
        //response["jsonrpc"] = "2.0";
        //response["id"] = request._requestId.toInt();  // requestId 就是 JSON-RPC 的 id

        //if (result.success) {
        //    // 转换结果为 JSON 值
        //    QJsonValue jsonResult;
        //    if (result._result.type() == QVariant::String) {
        //        jsonResult = result._result.toString();
        //    }
        //    else if (result._result.type() == QVariant::Int) {
        //        jsonResult = result._result.toInt();
        //    }
        //    else if (result._result.type() == QVariant::Double) {
        //        jsonResult = result._result.toDouble();
        //    }
        //    else if (result._result.type() == QVariant::Bool) {
        //        jsonResult = result._result.toBool();
        //    }
        //    else {
        //        jsonResult = result._result.toString();
        //    }
        //    response["result"] = jsonResult;
        //}
        //else {
        //    // 错误响应
        //    QJsonObject error;
        //    error["code"] = -32000;  // 自定义错误码
        //    error["message"] = result._errorMessage;
        //    response["error"] = error;
        //}

        //// 发送响应
        //sendResponse(connectionId, response);
    }

    void mAPIServer::onTaskFinished(const QString& requestId,
        const QString& connectionId,
        const APIExecutionResult& result)
    {
        // 构建 JSON-RPC 2.0 响应
        QJsonObject response;
        response["jsonrpc"] = "2.0";

        if (!requestId.isEmpty()) {
            response["id"] = requestId.toInt();
        }

        if (result._success) {
            // 转换结果为 JSON 值
            QJsonValue jsonResult;
            if (result._result.type() == QVariant::String) {
                jsonResult = result._result.toString();
            }
            else if (result._result.type() == QVariant::Int) {
                jsonResult = result._result.toInt();
            }
            else if (result._result.type() == QVariant::Double) {
                jsonResult = result._result.toDouble();
            }
            else if (result._result.type() == QVariant::Bool) {
                jsonResult = result._result.toBool();
            }
            else {
                jsonResult = result._result.toString();
            }
            response["result"] = jsonResult;
        }
        else {
            QJsonObject error;
            error["code"] = -32000;
            error["message"] = result._errorMessage;
            response["error"] = error;
        }

        // 发送响应
        sendResponse(connectionId, response);

        qDebug() << "[APIServer] Response sent for request:" << requestId
            << "Success:" << result.success
            << "Exec time:" << result._executionTimeMs << "ms";
    }

    void mAPIServer::sendResponse(const QString& connectionId, const QJsonObject& response)
    {
        if (m_isRunning) 
        {
            m_thread->sendResponse(connectionId, response);
        }
    }

} // namespace APIServer
