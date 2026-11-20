#pragma once
#pragma execution_character_set("utf-8")

#include "mAPIServer_global.h"
#include "mAPIServerTypes.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QVector>

#include <functional>

namespace APIServer
{
    class mAPISchedulingServer;

    /// 异步 CAE 操作的业务完成状态。
    enum class ScriptConditionState
    {
        Pending,   ///< 操作仍在执行，需要继续轮询。
        Completed, ///< 操作已成功到达终态。
        Failed     ///< 操作已失败到达终态。
    };

    /// 完成条件探针的返回值，包含状态和可选的诊断信息。
    struct ScriptConditionResult
    {
        ScriptConditionState state = ScriptConditionState::Pending;
        QString message;

        ScriptConditionResult() = default;
        ScriptConditionResult(ScriptConditionState value, const QString& text)
            : state(value), message(text)
        {
        }

        /// 构造“仍在执行”结果。
        static ScriptConditionResult pending(const QString& message = QString())
        {
            return ScriptConditionResult(ScriptConditionState::Pending, message);
        }

        /// 构造“执行完成”结果。
        static ScriptConditionResult completed(const QString& message = QString())
        {
            return ScriptConditionResult(ScriptConditionState::Completed, message);
        }

        /// 构造“执行失败”结果。
        static ScriptConditionResult failed(const QString& message)
        {
            return ScriptConditionResult(ScriptConditionState::Failed, message);
        }
    };

    /// 根据当前步骤参数查询异步业务状态的回调函数。
    using ScriptCompletionProbe =
        std::function<ScriptConditionResult(const APIParams& params)>;

    /**
     * @brief API 的代码侧执行策略。
     *
     * 建议在业务 API 注册阶段统一配置。JSON 不填写对应字段时使用这里的值；
     * JSON 显式填写时可针对单个算例覆盖。
     */
    struct ScriptExecutionPolicy
    {
        QString completionCondition; ///< 异步完成条件；为空表示同步 API。
        QString onError = QStringLiteral("stop"); ///< stop 或 continue。
        int timeoutMs = 300000;       ///< 单次尝试的完成等待超时。
        int pollIntervalMs = 100;     ///< 完成条件轮询间隔。
        int retries = 0;              ///< 首次失败后的最大重试次数。
        int retryDelayMs = 500;       ///< 两次尝试之间的延迟。
    };

    /**
     * @brief 基于 Qt 事件循环的 CAE JSON 流程执行器。
     *
     * API 派发成功与 CAE 业务完成是两个独立状态。同步 API 在 executeAPI()
     * 返回后视为完成；异步 API 通过 JSON 中的 waitFor 或 API 默认完成条件
     * 查询真实终态。等待过程使用 QTimer，不阻塞 GUI 主线程和 Qt 事件循环。
     */
    class APISERVER_EXPORT mAPIScriptExecutor : public QObject
    {
        Q_OBJECT

    public:
        /// 脚本结果码；宿主可记录到报告，也可选择映射为进程退出码。
        enum ExitCode
        {
            Succeeded = 0,                ///< 所有启用步骤均成功完成。
            InvalidCommandLineOrJson = 2, ///< 命令行参数或 JSON 格式错误。
            PreflightFailed = 3,          ///< API、参数或完成条件预检失败。
            StepFailed = 4,               ///< API 派发或 CAE 业务执行失败。
            StepTimedOut = 5,             ///< 等待异步业务完成超时。
            ReportWriteFailed = 6,        ///< 最终 JSON 报告写入失败。
            Cancelled = 130               ///< 流程被外部取消。
        };

        /// scheduler 必须与执行器位于同一 Qt 线程，通常为 GUI 主线程。
        explicit mAPIScriptExecutor(mAPISchedulingServer* scheduler,
            QObject* parent = nullptr);

        /// 注册一个可由 JSON waitFor 引用的业务完成条件。
        void registerCompletionCondition(const QString& name,
            ScriptCompletionProbe probe);
        /// 注册指定 API 的代码侧超时、重试和完成条件策略。
        bool registerExecutionPolicy(const QString& apiKey,
            const ScriptExecutionPolicy& policy, QString* error = nullptr);
        /// 设置所有未单独配置 API 的代码侧默认策略。
        bool setDefaultExecutionPolicy(const ScriptExecutionPolicy& policy,
            QString* error = nullptr);
        /// 为指定 API 绑定默认完成条件，使脚本可以省略 waitFor。
        void registerDefaultCompletionCondition(const QString& apiKey,
            const QString& conditionName);

        /// 解析 -e/--script、--report 或位置参数形式的命令行。
        bool configureFromCommandLine(const QStringList& arguments,
            QString* error = nullptr);
        /// 读取并解析 JSON 脚本，但不开始执行。
        bool loadScript(const QString& scriptPath, QString* error = nullptr);
        /// 设置最终报告路径；未设置时写入脚本同目录。
        void setReportPath(const QString& reportPath);
        /// 设置是否将最终报告写入磁盘；关闭时仍保留内存报告和 finished 信号。
        void setReportEnabled(bool enabled);
        /// 返回当前是否允许将报告写入磁盘。
        bool reportEnabled() const { return m_reportEnabled; }

        QString scriptPath() const { return m_scriptPath; }
        QString reportPath() const { return m_reportPath; }
        /// 返回脚本结束后是否要求关闭主程序；未配置时为 false。
        bool closeOnFinish() const { return m_closeOnFinish; }
        bool isRunning() const { return m_running; }

        /// 判断当前命令行是否请求运行 JSON 脚本。
        static bool commandLineRequestsScript(const QStringList& arguments);

    public slots:
        /// 完成预检后异步启动流程；实际步骤从下一次事件循环开始。
        bool start(QString* error = nullptr);
        /// 请求取消当前流程，已进入的同步 API 无法被强制中断。
        void cancel();

    signals:
        /// 输出一条机器可读的运行事件，适合序列化为 JSONL。
        void eventReady(const QJsonObject& event);
        /// 流程终止信号，携带稳定结果码和最终报告对象。
        void finished(int exitCode, const QJsonObject& report);

    private slots:
        /// 派发当前步骤或推进到下一个步骤。
        void executeCurrentStep();
        /// 非阻塞轮询当前异步步骤的完成条件。
        void pollCompletionCondition();

    private:
        /// 解析后的单步配置；id 由数组顺序自动生成，不从 JSON 读取。
        struct Step
        {
            QString id;              ///< 内部标识，例如 step-001。
            QString apiKey;          ///< 调度器中注册的 API key。
            QJsonObject rawParams;   ///< 尚未进行变量展开的原始参数。
            QString waitFor;         ///< 异步业务完成条件名称。
            QString onError;         ///< stop 或 continue。
            int timeoutMs = 300000;  ///< 单次尝试的完成等待超时。
            int pollIntervalMs = 100; ///< 完成条件轮询间隔。
            int retries = 0;         ///< 首次失败后的最大重试次数。
            int retryDelayMs = 500;  ///< 两次尝试之间的延迟。
            bool enabled = true;     ///< false 时记录 skipped 并直接跳过。
        };

        // JSON 解析与启动前校验。
        bool parseDocument(const QByteArray& json, QString* error);
        bool parseStep(const QJsonObject& object, int index, Step* step,
            QString* error) const;
        bool preflight(QString* error);
        bool buildParams(const Step& step, APIParams* params,
            QString* error) const;
        bool convertParamValue(const QJsonValue& value, QStringList* values,
            QString* error) const;
        bool resolveString(const QString& input, QString* output,
            QString* error, int depth = 0) const;

        // 单步状态收敛、报告记录和流程终止。
        void completeCurrentStep(const QString& message = QString());
        void failCurrentStep(const QString& message, int exitCode);
        void appendStepReport(const QString& status, const QString& message);
        void finishScript(int exitCode, const QString& message);
        bool writeReport(QString* error) const;
        void publishEvent(const QString& name,
            const QJsonObject& fields = QJsonObject());
        QString utcNow() const;

        static QString explicitOptionValue(const QStringList& arguments,
            const QString& option, QString* error);
        static QString positionalJsonPath(const QStringList& arguments);
        static bool validateExecutionPolicy(const ScriptExecutionPolicy& policy,
            QString* error);

        // 外部依赖及静态脚本配置。
        mAPISchedulingServer* m_scheduler = nullptr;
        QMap<QString, ScriptCompletionProbe> m_completionConditions;
        QMap<QString, ScriptExecutionPolicy> m_executionPolicies;
        ScriptExecutionPolicy m_defaultExecutionPolicy;
        QMap<QString, QString> m_variables;
        QVector<Step> m_steps;

        // 当前脚本及步骤的运行上下文。
        QString m_scriptPath;
        QString m_scriptDir;
        QString m_reportPath;
        QString m_scriptName;
        // current：步骤参数使用当前几何 ID；original：脚本中的点、线、面、
        // 体 ID 始终表示脚本开始时的最初 ID。后者通过内部 contextId 与
        // Controller 共享“最初 ID -> 当前 ID”映射。
        QString m_geometryIdMode = QStringLiteral("current");
        QString m_scriptExecutionId;
        QString m_currentRequestId;
        QString m_currentDispatchStatus;
        APIParams m_currentParams;
        QJsonArray m_stepReports;
        QJsonObject m_report;
        QTimer m_pollTimer;
        QElapsedTimer m_stepTimer;
        bool m_reportEnabled = false; ///< 默认不将最终报告写入磁盘。
        bool m_closeOnFinish = false; ///< 脚本进入最终状态后是否关闭主程序。

        // 状态机游标与最终结果状态。
        int m_currentStep = 0;
        int m_currentAttempt = 0;
        int m_finalFailureCode = Succeeded;
        bool m_loaded = false;
        bool m_running = false;
        bool m_cancelRequested = false;
    };
}

// 允许 ScriptConditionResult 跨 Qt 队列连接传递。
Q_DECLARE_METATYPE(APIServer::ScriptConditionResult)
