#pragma once

#include <QString>
#include <QVector>
#include <QPair>
#include <QJsonObject>
#include <QMetaType>
#include <QVariant>
#include <QTcpServer>

namespace APIServer
{
    /*********************************************************************API相关*****************************************************************************************/
    // API 参数类型
    using APIParams = QHash<QString, QStringList>;

    // API 函数类型：参数列表APIParams，错误信息QString，返回值QVariant
    /*
    * API命名规范：
    *   API_类名_功能名
    */
    using APIFunction = std::function<QVariant(const APIParams&, QString&)>;

    // API 执行结果
    struct APIExecutionResult
    {
        bool _success = false;
        QString _errorMessage;
        QVariant _result;
        qint64 _executionTimeMs = 0;  // 执行耗时（毫秒）
    
    APIExecutionResult() {};
    APIExecutionResult(bool s, const QString& err, const QVariant& re, qint64 tims) {
        _success = s;
        _errorMessage = err;
        _result = re;
        _executionTimeMs = tims;
    }
        static APIExecutionResult success(const QVariant& res = QVariant()) {
            return APIExecutionResult{ true, QString(), res, 0 };
        }

        static APIExecutionResult failure(const QString& error) {
            return APIExecutionResult{ false, error, false, 0 };
        }
    };

    // API 统计信息
    struct APIStatistics
    {
        qint64 _totalCalls = 0;           // 总调用次数
        qint64 _successCount = 0;          // 成功次数
        qint64 _failureCount = 0;          // 失败次数
        qint64 _totalExecutionTime = 0;    // 总执行时间（毫秒）
        QString _lastError;                // 最后一次错误

        double getSuccessRate() const {
            return _totalCalls > 0 ? (double)_successCount / _totalCalls * 100 : 0;
        }

        double getAverageExecutionTime() const {
            return _totalCalls > 0 ? (double)_totalExecutionTime / _totalCalls : 0;
        }

        void reset() {
            _totalCalls = 0;
            _successCount = 0;
            _failureCount = 0;
            _totalExecutionTime = 0;
            _lastError.clear();
        }
    };

    // API 信息（用于查询和调试）
    struct APIInfo
    {
        QString _apiKey;
        QString _description;
        QStringList _requiredParams;
        QStringList _optionalParams;
        qint64 _registeredTime;
        qint64 _callCount = 0;

        APIInfo() : _registeredTime(0), _callCount(0) {}

		APIInfo(const QString& key, const QString& desc = QString(), const QStringList& req = QStringList(), const QStringList& opt = QStringList())
			: _apiKey(key), _description(desc), _requiredParams(req), _optionalParams(opt), _registeredTime(0), _callCount(0) {}
    };

    // API 请求结构（用于内部传递）
    struct APIRequest
    {
        QString _apiKey;            // API 名称
        APIParams _params;          // 参数列表
        QString _requestId;         // 请求ID（用于响应匹配）
        QString _connectionId;      // 客户端连接ID
        qint64 _timestamp;          // 时间戳

        APIRequest() : _timestamp(0) {}

        bool isValid() const {
            return !_apiKey.isEmpty() && !_connectionId.isEmpty();
        }

        QString toString() const {
            return QString("API: %1, RequestId: %2, ConnectionId: %3, Params: %4")
                .arg(_apiKey).arg(_requestId).arg(_connectionId).arg(_params.size());
        }
    };

    // API 响应结构（用于内部传递）
    struct APIResponse
    {
        QString _requestId;
        QString _connectionId;
        bool _success;
        QString _errorMessage;
        QVariant _result;

        APIResponse() : _success(false) {}

        QJsonObject toJson() const {
            QJsonObject obj;
            obj["Request_ID"] = _requestId;
            obj["Success"] = _success;

            if (!_errorMessage.isEmpty()) {
                obj["Error"] = _errorMessage;
            }

            if (_result.isValid()) {
                if (_result.type() == QVariant::String) {
                    obj["Result"] = _result.toString();
                }
                else if (_result.type() == QVariant::Int) {
                    obj["Result"] = _result.toInt();
                }
                else if (_result.type() == QVariant::Double) {
                    obj["Result"] = _result.toDouble();
                }
                else if (_result.type() == QVariant::Bool) {
                    obj["Result"] = _result.toBool();
                }
                else {
                    obj["Result"] = _result.toString();
                }
            }

            return obj;
        }

        static APIResponse success(const QString& reqId, const QString& connId,
            const QVariant& res = QVariant()) {
            APIResponse resp;
            resp._requestId = reqId;
            resp._connectionId = connId;
            resp._success = true;
            resp._result = res;
            return resp;
        }

        static APIResponse error(const QString& reqId, const QString& connId,
            const QString& msg) {
            APIResponse resp;
            resp._requestId = reqId;
            resp._connectionId = connId;
            resp._success = false;
            resp._errorMessage = msg;
            return resp;
        }
    };

    // APITask 结构体
    struct APITask
    {
        QString apiKey;
        APIParams params;
        QString requestId;
        QString connectionId;      // 用于响应时知道发给哪个客户端
        qint64 enqueueTime;        // 入队时间（用于调试）

        APITask() : enqueueTime(0) {}

        APITask(const QString& key, const APIParams& p, const QString& reqId, const QString& connId)
            : apiKey(key), params(p), requestId(reqId), connectionId(connId), enqueueTime(0)
        {}

        bool isValid() const { return !apiKey.isEmpty(); }
    };

    /*********************************************************************网络相关*****************************************************************************************/

    // 数据包结构
    struct JsonPacket
    {
        QString _connectionId;      // 连接ID
        QJsonObject _data;          // JSON数据
        QByteArray _rawData;        // 原始数据
        bool _isValid;              // 是否有效
        QString _errorMessage;      // 错误信息
        JsonPacket() : _isValid(false) {}
    };

    // 客户端信息
    struct ClientInfo
    {
        QTcpSocket* _socket;
        QString _connectionId;
        QByteArray _buffer;         // 接收缓冲区（处理粘包）
        qint64 _lastActivity;       // 最后活动时间
        QString _clientAddress;     // 客户端地址

        ClientInfo() : _socket(nullptr), _lastActivity(0) {}
    };
} // namespace APIServer

// 注册元类型用于跨线程信号槽
Q_DECLARE_METATYPE(APIServer::JsonPacket)
Q_DECLARE_METATYPE(APIServer::APIRequest)
Q_DECLARE_METATYPE(APIServer::APIResponse)