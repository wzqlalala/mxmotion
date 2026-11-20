/*****************************************************************/
/*
    网络通信服务器

    功能：
    1、将收到的数据包装成json格式
    2、向客户端（支持广播）发送json格式数据

    注意：
    需异步运行，不能阻塞主线程

*/
/*****************************************************************/

#pragma once
#pragma execution_character_set("utf-8")

#include "mAPIServer_global.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTimer>

#include "mAPIServerTypes.h"

namespace APIServer
{
    class APISERVER_EXPORT mNetWorkServer : public QObject
    {
        Q_OBJECT

    public:
        explicit mNetWorkServer(QObject* parent = nullptr);
        ~mNetWorkServer();

        // 启动服务器
        bool start(quint16 port = 9527, const QString& address = "127.0.0.1");

        // 停止服务器
        void stop();

        // 服务器状态
        bool isRunning() const { return m_isRunning; }
        quint16 getPort() const { return m_port; }
        QString getAddress() const { return m_address; }
        int getClientCount() const { return m_clients.size(); }

        // 发送响应到指定客户端
        bool sendResponse(const QString& connectionId, const QJsonObject& response);

        // 广播给所有客户端
        bool broadcast(const QJsonObject& message);

        // 断开指定客户端
        void disconnectClient(const QString& connectionId);

        // 获取所有客户端ID
        QStringList getClientIds() const;

    signals:
        // 收到JSON数据包
        void jsonPacketReceived(const JsonPacket& packet);

        // 客户端连接/断开
        void clientConnected(const QString& connectionId, const QString& address);
        void clientDisconnected(const QString& connectionId);

        // 服务器状态变化
        void serverStarted(quint16 port);
        void serverStopped();
        void serverError(const QString& error);

        // 发送完成通知
        void responseSent(const QString& connectionId, bool success);

    private slots:
        //连接申请
        void onNewConnection();
        //报文消息到达缓冲区
        void onReadyRead();
        //连接断开
        void onClientDisconnected();
        //套接字错误
        void onSocketError(QAbstractSocket::SocketError error);
        // 检查空闲连接
        void checkIdleConnections();  

    private:
        // 生成唯一的连接ID
        QString generateConnectionId();

        // 解析JSON数据
        JsonPacket parseJsonPacket(const QString& connectionId, const QByteArray& data);

        // 处理接收到的数据（处理粘包）
        void processReceivedData(const QString& connectionId, QByteArray& buffer);

        // 清理客户端
        void cleanupClient(const QString& connectionId);

        // 发送数据
        bool sendData(QTcpSocket* socket, const QByteArray& data);

    private:
        QTcpServer* m_server = nullptr;
        QMap<QString, ClientInfo> m_clients;     // 连接ID -> 客户端信息
        QMap<QTcpSocket*, QString> m_socketToId; // Socket -> 连接ID

        quint16 m_port = 9527;
        QString m_address = "127.0.0.1";
        bool m_isRunning = false;

        QTimer* m_idleTimer = nullptr;           // 空闲连接检查定时器
        qint64 m_idleTimeout = 300000;            // 5分钟空闲超时（毫秒）

        // 统计信息
        qint64 m_totalPacketsReceived = 0;
        qint64 m_totalPacketsSent = 0;
        qint64 m_totalErrors = 0;


        //拆包用
    private:
        enum ParseState {
            ReadingLength,  // 读取长度头
            ReadingData     // 读取数据体
        };

        ParseState m_state = ReadingLength;
        quint32 m_expectedLength = 0;
        static constexpr quint32 MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10MB 限制
    };
}