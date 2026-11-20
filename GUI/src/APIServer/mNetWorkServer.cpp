#include "mNetWorkServer.h"
#include <QHostAddress>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QtEndian>

namespace APIServer
{
    mNetWorkServer::mNetWorkServer(QObject* parent)
        : QObject(parent)
    {
        m_server = new QTcpServer(this);

        // 连接服务器信号
        connect(m_server, &QTcpServer::newConnection,
            this, &mNetWorkServer::onNewConnection);

        // 创建空闲连接检查定时器
        m_idleTimer = new QTimer(this);
        connect(m_idleTimer, &QTimer::timeout,
            this, &mNetWorkServer::checkIdleConnections);
        m_idleTimer->start(60000); // 每分钟检查一次
    }

    mNetWorkServer::~mNetWorkServer()
    {
        stop();
    }

    bool mNetWorkServer::start(quint16 port, const QString& address)
    {
        if (m_isRunning) 
        {
            qWarning() << "Server already running";
            return false;
        }

        m_port = port;
        m_address = address;

        // 监听指定地址和端口
        QHostAddress hostAddress;
        if (address == "127.0.0.1" || address == "localhost")
        {
            hostAddress = QHostAddress::LocalHost;
        }
        else if (address == "0.0.0.0") 
        {
            hostAddress = QHostAddress::Any;
        }
        else 
        {
            hostAddress = QHostAddress(address);
        }

        if (!m_server->listen(hostAddress, port))
        {
            QString error = QString("Failed to start server: %1").arg(m_server->errorString());
            qCritical() << error;
            emit serverError(error);
            return false;
        }

        m_isRunning = true;
        qDebug() << "Server started on" << address << ":" << port;
        emit serverStarted(port);

        return true;
    }

    void mNetWorkServer::stop()
    {
        if (!m_isRunning) 
        {
            return;
        }

        qDebug() << "Stopping server...";

        // 断开所有客户端
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            if (it->_socket) 
            {
                it->_socket->disconnectFromHost();
                it->_socket->deleteLater();
            }
        }

        m_clients.clear();
        m_socketToId.clear();

        // 停止服务器
        if (m_server) 
        {
            m_server->close();
        }

        m_isRunning = false;
        qDebug() << "Server stopped";
        emit serverStopped();
    }

    void mNetWorkServer::onNewConnection()
    {
        while (m_server->hasPendingConnections()) 
        {
            QTcpSocket* clientSocket = m_server->nextPendingConnection();

            if (!clientSocket) 
            {
                continue;
            }

            // 生成唯一连接ID
            QString connectionId = generateConnectionId();
            QString clientAddress = clientSocket->peerAddress().toString();
            quint16 clientPort = clientSocket->peerPort();

            // 保存客户端信息
            ClientInfo info;
            info._socket = clientSocket;
            info._connectionId = connectionId;
            info._clientAddress = QString("%1:%2").arg(clientAddress).arg(clientPort);
            info._lastActivity = QDateTime::currentMSecsSinceEpoch();

            m_clients[connectionId] = info;
            m_socketToId[clientSocket] = connectionId;

            // 连接信号
            connect(clientSocket, &QTcpSocket::readyRead,
                this, &mNetWorkServer::onReadyRead);
            connect(clientSocket, &QTcpSocket::disconnected,
                this, &mNetWorkServer::onClientDisconnected);
            connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this, &mNetWorkServer::onSocketError);

            qDebug() << "Client connected:" << info._clientAddress
                << "ID:" << connectionId;

            emit clientConnected(connectionId, info._clientAddress);
        }
    }

    void mNetWorkServer::onReadyRead()
    {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) 
        {
            return;
        }

        QString connectionId = m_socketToId.value(socket);
        if (connectionId.isEmpty()) 
        {
            qWarning() << "Received data from unknown socket";
            return;
        }

        ClientInfo& client = m_clients[connectionId];

        // 读取数据
        QByteArray data = socket->readAll();
        if (data.isEmpty()) 
        {
            return;
        }

        // 追加到缓冲区
        client._buffer.append(data);
        client._lastActivity = QDateTime::currentMSecsSinceEpoch();

        // 处理接收到的数据（处理粘包）
        processReceivedData(connectionId, client._buffer);
    }

    void mNetWorkServer::processReceivedData(const QString& connectionId, QByteArray& buffer)
    {
        // 使用状态机处理定长拆包
        while (true) {
            if (m_state == ParseState::ReadingLength) {
                // 需要至少4字节来读取长度
                if (buffer.size() < 4) {
                    break; // 等待更多数据
                }

                // 读取4字节长度（大端序）
                quint32 packetLength = qFromBigEndian<quint32>(
                    reinterpret_cast<const uchar*>(buffer.constData())
                );

                // 验证长度合法性
                if (packetLength == 0 || packetLength > MAX_PACKET_SIZE) {
                    qWarning() << "Invalid packet length:" << packetLength << "from" << connectionId;
                    // 跳过无效数据，尝试重新同步
                    buffer.remove(0, 1);
                    continue;
                }

                m_expectedLength = packetLength;
                m_state = ParseState::ReadingData;
                buffer.remove(0, 4); // 移除长度头
            }

            if (m_state == ParseState::ReadingData) {
                // 检查是否有足够的数据
                if (buffer.size() < m_expectedLength) {
                    break; // 等待更多数据
                }

                // 提取完整的消息数据
                QByteArray messageData = buffer.left(m_expectedLength);
                buffer.remove(0, m_expectedLength);

                // 解析 JSON-RPC 消息
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qWarning() << "JSON parse error from" << connectionId << ":" << parseError.errorString();
                    m_totalErrors++;
                    emit serverError(QString("JSON parse error from %1: %2")
                        .arg(connectionId).arg(parseError.errorString()));
                }
                else if (!doc.isObject()) {
                    qWarning() << "Invalid JSON-RPC message from" << connectionId << ": not an object";
                    m_totalErrors++;
                    emit serverError(QString("Invalid JSON-RPC message from %1: not an object").arg(connectionId));
                }
                else {
                    // 构造 JsonPacket
                    JsonPacket packet;
                    packet._connectionId = connectionId;
                    packet._data = doc.object();
                    packet._rawData = messageData;
                    packet._isValid = true;

                    //// 验证 JSON-RPC 2.0 格式
                    //QJsonObject obj = packet._data;
                    //if (obj.contains("jsonrpc") && obj["jsonrpc"].toString() == "2.0") {
                    //    packet._isJsonRpc = true;
                    //    packet._jsonRpcMethod = obj["method"].toString();
                    //    packet._jsonRpcId = obj.contains("id") ? obj["id"].toInt() : 0;
                    //    packet._isNotification = !obj.contains("id");

                    //    qDebug() << "Received JSON-RPC request from" << connectionId
                    //        << "- method:" << packet._jsonRpcMethod
                    //        << "id:" << packet._jsonRpcId;
                    //}
                    //else {
                    //    packet._isJsonRpc = false;
                    //    qDebug() << "Received JSON packet from" << connectionId;
                    //}

                    m_totalPacketsReceived++;
                    emit jsonPacketReceived(packet);
                }

                // 重置状态，准备下一条消息
                m_state = ParseState::ReadingLength;
                m_expectedLength = 0;
            }
        }
    }

    JsonPacket mNetWorkServer::parseJsonPacket(const QString& connectionId, const QByteArray& data)
    {
        JsonPacket packet;
        packet._connectionId = connectionId;
        packet._rawData = data;

        // 解析JSON
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) 
        {
            packet._isValid = false;
            packet._errorMessage = QString("JSON parse error at offset %1: %2")
                .arg(error.offset)
                .arg(error.errorString());
            return packet;
        }

        if (!doc.isObject())
        {
            packet._isValid = false;
            packet._errorMessage = "JSON must be an object";
            return packet;
        }

        packet._isValid = true;
        packet._data = doc.object();
        packet._errorMessage.clear();

        return packet;
    }

    void mNetWorkServer::onClientDisconnected()
    {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) 
        {
            return;
        }

        QString connectionId = m_socketToId.value(socket);
        if (!connectionId.isEmpty())
        {
            ClientInfo client = m_clients.value(connectionId);
            qDebug() << "Client disconnected:" << client._clientAddress
                << "ID:" << connectionId;

            emit clientDisconnected(connectionId);
            cleanupClient(connectionId);
        }

        socket->deleteLater();
    }

    void mNetWorkServer::onSocketError(QAbstractSocket::SocketError error)
    {
        QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
        if (!socket) 
        {
            return;
        }

        QString connectionId = m_socketToId.value(socket);
        QString errorString = socket->errorString();

        qWarning() << "Socket error for connection" << connectionId
            << ":" << errorString << "(code:" << error << ")";

        emit serverError(QString("Socket error (%1): %2")
            .arg(connectionId).arg(errorString));

        // 如果有连接ID，清理客户端
        if (!connectionId.isEmpty())
        {
            cleanupClient(connectionId);
        }
    }

    void mNetWorkServer::checkIdleConnections()
    {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        QList<QString> idleConnections;

        for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            if (now - it->_lastActivity > m_idleTimeout)
            {
                idleConnections.append(it.key());
            }
        }

        for (const QString& connectionId : idleConnections)
        {
            qDebug() << "Disconnecting idle client:" << connectionId;
            disconnectClient(connectionId);
        }
    }

    bool mNetWorkServer::sendResponse(const QString& connectionId, const QJsonObject& response)
    {
        auto it = m_clients.find(connectionId);
        if (it == m_clients.end()) 
        {
            qWarning() << "Client not found:" << connectionId;
            emit responseSent(connectionId, false);
            return false;
        }

        ClientInfo& client = it.value();
        if (!client._socket || client._socket->state() != QAbstractSocket::ConnectedState) 
        {
            qWarning() << "Client socket not connected:" << connectionId;
            emit responseSent(connectionId, false);
            return false;
        }

        // 序列化JSON
        QJsonDocument doc(response);
        QByteArray data = doc.toJson(QJsonDocument::Compact);

        //4字节长度前缀
        quint32 dataLength = data.size();
        QByteArray packet;
        packet.append(char((dataLength >> 24) & 0xFF));
        packet.append(char((dataLength >> 16) & 0xFF));
        packet.append(char((dataLength >> 8) & 0xFF));
        packet.append(char(dataLength & 0xFF));
        packet.append(data);

        bool success = sendData(client._socket, packet);

        if (success)
        {
            m_totalPacketsSent++;
            qDebug() << "Response sent to" << connectionId << ":" << response;
        }
        else 
        {
            m_totalErrors++;
            qWarning() << "Failed to send response to" << connectionId;
        }

        emit responseSent(connectionId, success);
        return success;
    }

    bool mNetWorkServer::broadcast(const QJsonObject& message)
    {
        QJsonDocument doc(message);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        data.append('\n');

        bool allSuccess = true;
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            if (it->_socket && it->_socket->state() == QAbstractSocket::ConnectedState) {
                if (!sendData(it->_socket, data)) 
                {
                    allSuccess = false;
                    qWarning() << "Broadcast failed to client:" << it.key();
                }
            }
        }

        return allSuccess;
    }

    bool mNetWorkServer::sendData(QTcpSocket* socket, const QByteArray& data)
    {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) 
        {
            return false;
        }

        qint64 bytesWritten = socket->write(data);
        if (bytesWritten != data.size())
        {
            qWarning() << "Failed to send all data. Expected:"
                << data.size() << "Sent:" << bytesWritten;
            return false;
        }

        bool flushed = socket->flush();
        return flushed;
    }

    void mNetWorkServer::disconnectClient(const QString& connectionId)
    {
        auto it = m_clients.find(connectionId);
        if (it != m_clients.end()) 
        {
            if (it->_socket)
            {
                it->_socket->disconnectFromHost();
            }
            cleanupClient(connectionId);
        }
    }

    void mNetWorkServer::cleanupClient(const QString& connectionId)
    {
        auto it = m_clients.find(connectionId);
        if (it != m_clients.end()) 
        {
            // 从映射中移除
            if (it->_socket) 
            {
                m_socketToId.remove(it->_socket);
            }
            m_clients.erase(it);
        }
    }

    QStringList mNetWorkServer::getClientIds() const
    {
        return m_clients.keys();
    }

    QString mNetWorkServer::generateConnectionId()
    {
        // 使用UUID生成唯一ID
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}