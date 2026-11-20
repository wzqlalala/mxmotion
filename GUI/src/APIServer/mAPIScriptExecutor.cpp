#include "mAPIScriptExecutor.h"
#include "mAPISchedulingServer.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>

#include <exception>
#include <utility>

namespace APIServer
{
    namespace
    {
        const QString kCurrentGeometryIdMode = QStringLiteral("current");
        const QString kOriginalGeometryIdMode = QStringLiteral("original");
        const QString kOriginalFaceIdContextParam =
            QStringLiteral("__directModelingOriginalFaceIdContext");

        // 校验对象中是否存在协议未定义的字段，防止拼写错误被静默忽略。
        bool validateKeys(const QJsonObject& object, const QStringList& allowed,
            const QString& context, QString* error)
        {
            for (auto it = object.begin(); it != object.end(); ++it)
            {
                if (!allowed.contains(it.key()))
                {
                    if (error)
                        *error = QStringLiteral("%1 contains unknown field '%2'")
                        .arg(context, it.key());
                    return false;
                }
            }
            return true;
        }

        // 将 JSON 标量统一转换成现有 APIParams 使用的字符串表示。
        QString jsonScalarToString(const QJsonValue& value, bool* ok)
        {
            *ok = true;
            if (value.isString())
                return value.toString();
            if (value.isBool())
                return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            if (value.isDouble())
                return QString::number(value.toDouble(), 'g', 16);
            if (value.isNull())
                return QString();

            *ok = false;
            return QString();
        }

        // 超时、轮询间隔和重试次数必须是非负整数。
        bool isNonNegativeInteger(const QJsonValue& value)
        {
            if (!value.isDouble())
                return false;
            const double number = value.toDouble();
            return number >= 0.0 && number == static_cast<int>(number);
        }
    }

    mAPIScriptExecutor::mAPIScriptExecutor(mAPISchedulingServer* scheduler,
        QObject* parent)
        : QObject(parent), m_scheduler(scheduler)
    {
        // 单次定时器保证每轮轮询完成后再安排下一轮，避免回调重入。
        m_pollTimer.setSingleShot(true);
        connect(&m_pollTimer, &QTimer::timeout,
            this, &mAPIScriptExecutor::pollCompletionCondition);
    }

    // 注册命名完成条件，脚本通过 waitFor 使用该名称。
    void mAPIScriptExecutor::registerCompletionCondition(const QString& name,
        ScriptCompletionProbe probe)
    {
        const QString normalized = name.trimmed();
        if (!normalized.isEmpty() && probe)
            m_completionConditions.insert(normalized, std::move(probe));
    }

    // 策略由代码统一管理，避免每个 JSON 重复填写超时和重试参数。
    bool mAPIScriptExecutor::validateExecutionPolicy(
        const ScriptExecutionPolicy& policy, QString* error)
    {
        const bool valid =
            policy.timeoutMs >= 1 && policy.timeoutMs <= 86400000 &&
            policy.pollIntervalMs >= 10 && policy.pollIntervalMs <= 60000 &&
            policy.retries >= 0 && policy.retries <= 10 &&
            policy.retryDelayMs >= 0 && policy.retryDelayMs <= 600000 &&
            (policy.onError == QStringLiteral("stop") ||
                policy.onError == QStringLiteral("continue"));
        if (!valid && error)
        {
            *error = QStringLiteral(
                "Invalid execution policy: timeoutMs=[1,86400000], "
                "pollIntervalMs=[10,60000], retries=[0,10], "
                "retryDelayMs=[0,600000], onError=stop|continue");
        }
        return valid;
    }

    bool mAPIScriptExecutor::registerExecutionPolicy(const QString& apiKey,
        const ScriptExecutionPolicy& policy, QString* error)
    {
        const QString normalized = apiKey.trimmed();
        if (normalized.isEmpty())
        {
            if (error)
                *error = QStringLiteral("API key for execution policy cannot be empty");
            return false;
        }
        if (!validateExecutionPolicy(policy, error))
            return false;

        ScriptExecutionPolicy normalizedPolicy = policy;
        normalizedPolicy.completionCondition =
            normalizedPolicy.completionCondition.trimmed();
        normalizedPolicy.onError = normalizedPolicy.onError.trimmed().toLower();
        m_executionPolicies.insert(normalized, normalizedPolicy);
        return true;
    }

    bool mAPIScriptExecutor::setDefaultExecutionPolicy(
        const ScriptExecutionPolicy& policy, QString* error)
    {
        if (!validateExecutionPolicy(policy, error))
            return false;

        m_defaultExecutionPolicy = policy;
        m_defaultExecutionPolicy.completionCondition =
            m_defaultExecutionPolicy.completionCondition.trimmed();
        m_defaultExecutionPolicy.onError =
            m_defaultExecutionPolicy.onError.trimmed().toLower();
        return true;
    }

    // 为常用异步 API 提供默认条件，减少脚本中的重复配置。
    void mAPIScriptExecutor::registerDefaultCompletionCondition(
        const QString& apiKey, const QString& conditionName)
    {
        const QString normalizedApi = apiKey.trimmed();
        const QString normalizedCondition = conditionName.trimmed();
        if (normalizedApi.isEmpty() || normalizedCondition.isEmpty())
            return;

        ScriptExecutionPolicy policy = m_executionPolicies.value(
            normalizedApi, m_defaultExecutionPolicy);
        policy.completionCondition = normalizedCondition;
        m_executionPolicies.insert(normalizedApi, policy);
    }

    // 提取 --option value 或 --option=value，并拒绝重复选项。
    QString mAPIScriptExecutor::explicitOptionValue(const QStringList& arguments,
        const QString& option, QString* error)
    {
        QString value;
        for (int i = 1; i < arguments.size(); ++i)
        {
            const QString argument = arguments.at(i);
            if (argument == option)
            {
                if (i + 1 >= arguments.size() || arguments.at(i + 1).startsWith('-'))
                {
                    if (error)
                        *error = QStringLiteral("%1 requires a value").arg(option);
                    return QString();
                }
                if (!value.isEmpty())
                {
                    if (error)
                        *error = QStringLiteral("%1 was specified more than once").arg(option);
                    return QString();
                }
                value = arguments.at(++i);
            }
            else if (argument.startsWith(option + '='))
            {
                if (!value.isEmpty())
                {
                    if (error)
                        *error = QStringLiteral("%1 was specified more than once").arg(option);
                    return QString();
                }
                value = argument.mid(option.size() + 1);
                if (value.isEmpty() && error)
                    *error = QStringLiteral("%1 requires a value").arg(option);
            }
        }
        return value;
    }

    // 兼容 MechMainWindow.exe workflow.json 的位置参数形式。
    QString mAPIScriptExecutor::positionalJsonPath(const QStringList& arguments)
    {
        for (int i = 1; i < arguments.size(); ++i)
        {
            const QString argument = arguments.at(i);
            if (argument == QStringLiteral("--script") ||
                argument == QStringLiteral("-e") ||
                argument == QStringLiteral("--report"))
            {
                ++i;
                continue;
            }
            if (!argument.startsWith('-') &&
                argument.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
                return argument;
        }
        return QString();
    }

    // 判断是否进入自动化脚本模式，普通 GUI 启动不受影响。
    bool mAPIScriptExecutor::commandLineRequestsScript(const QStringList& arguments)
    {
        for (const QString& argument : arguments)
        {
            if (argument == QStringLiteral("--script") ||
                argument.startsWith(QStringLiteral("--script=")) ||
                argument == QStringLiteral("-e") ||
                argument.startsWith(QStringLiteral("-e=")))
                return true;
        }
        return !positionalJsonPath(arguments).isEmpty();
    }

    // 解析命令行并加载脚本，报告路径可由 --report 覆盖。
    bool mAPIScriptExecutor::configureFromCommandLine(const QStringList& arguments,
        QString* error)
    {
        QString parseError;
        QString script = explicitOptionValue(arguments, QStringLiteral("--script"),
            &parseError);
        if (!parseError.isEmpty())
        {
            if (error)
                *error = parseError;
            return false;
        }
        const QString shortScript = explicitOptionValue(
            arguments, QStringLiteral("-e"), &parseError);
        if (!parseError.isEmpty())
        {
            if (error)
                *error = parseError;
            return false;
        }
        if (!script.isEmpty() && !shortScript.isEmpty())
        {
            if (error)
                *error = QStringLiteral("Use either -e or --script, not both");
            return false;
        }
        if (script.isEmpty())
            script = shortScript;
        if (script.isEmpty())
            script = positionalJsonPath(arguments);
        if (script.isEmpty())
        {
            if (error)
                *error = QStringLiteral("No JSON script was supplied");
            return false;
        }

        QString report = explicitOptionValue(arguments, QStringLiteral("--report"),
            &parseError);
        if (!parseError.isEmpty())
        {
            if (error)
                *error = parseError;
            return false;
        }

        if (!loadScript(script, error))
            return false;
        if (!report.isEmpty())
            setReportPath(report);
        return true;
    }

    // 读取脚本文件并建立与脚本目录相关的默认报告路径。
    bool mAPIScriptExecutor::loadScript(const QString& scriptPath, QString* error)
    {
        if (m_running)
        {
            if (error)
                *error = QStringLiteral("Cannot load a script while execution is running");
            return false;
        }

        const QFileInfo info(scriptPath);
        const QString absolutePath = info.absoluteFilePath();
        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            if (error)
                *error = QStringLiteral("Cannot open script '%1': %2")
                .arg(QDir::toNativeSeparators(absolutePath), file.errorString());
            return false;
        }

        m_loaded = false;
        m_scriptPath = absolutePath;
        m_scriptDir = info.absolutePath();
        if (!parseDocument(file.readAll(), error))
            return false;

        m_reportPath = info.dir().filePath(info.completeBaseName() +
            QStringLiteral(".report.json"));
        m_loaded = true;
        return true;
    }

    // 显式报告路径相对于当前工作目录解析。
    void mAPIScriptExecutor::setReportPath(const QString& reportPath)
    {
        if (!reportPath.trimmed().isEmpty())
            m_reportPath = QFileInfo(reportPath).absoluteFilePath();
    }

    void mAPIScriptExecutor::setReportEnabled(bool enabled)
    {
        m_reportEnabled = enabled;
    }

    // 解析根对象、变量、默认配置和步骤数组。
    bool mAPIScriptExecutor::parseDocument(const QByteArray& json, QString* error)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            if (error)
                *error = QStringLiteral("JSON parse error at offset %1: %2")
                .arg(parseError.offset).arg(parseError.errorString());
            return false;
        }
        if (!document.isObject())
        {
            if (error)
                *error = QStringLiteral("The JSON root must be an object");
            return false;
        }

        const QJsonObject root = document.object();
        if (!validateKeys(root,
            { QStringLiteral("$schema"), QStringLiteral("version"),
              QStringLiteral("name"), QStringLiteral("variables"),
              QStringLiteral("defaults"), QStringLiteral("closeOnFinish"),
              QStringLiteral("geometryIdMode"), QStringLiteral("steps") },
            QStringLiteral("Script root"), error))
            return false;
        if (root.contains(QStringLiteral("version")) &&
            (!root.value(QStringLiteral("version")).isString() ||
                root.value(QStringLiteral("version")).toString() != QStringLiteral("1.0")))
        {
            if (error)
                *error = QStringLiteral("Unsupported script version; expected '1.0'");
            return false;
        }
        if (!root.value(QStringLiteral("steps")).isArray())
        {
            if (error)
                *error = QStringLiteral("'steps' must be an array");
            return false;
        }
        if (root.contains(QStringLiteral("name")) &&
            (!root.value(QStringLiteral("name")).isString() ||
                root.value(QStringLiteral("name")).toString().trimmed().isEmpty()))
        {
            if (error)
                *error = QStringLiteral("'name' must be a non-empty string");
            return false;
        }
        if (root.contains(QStringLiteral("variables")) &&
            !root.value(QStringLiteral("variables")).isObject())
        {
            if (error)
                *error = QStringLiteral("'variables' must be an object");
            return false;
        }
        if (root.contains(QStringLiteral("defaults")) &&
            !root.value(QStringLiteral("defaults")).isObject())
        {
            if (error)
                *error = QStringLiteral("'defaults' must be an object");
            return false;
        }
        if (root.contains(QStringLiteral("closeOnFinish")) &&
            !root.value(QStringLiteral("closeOnFinish")).isBool())
        {
            if (error)
                *error = QStringLiteral("'closeOnFinish' must be a boolean");
            return false;
        }
        if (root.contains(QStringLiteral("geometryIdMode"))
            && !root.value(QStringLiteral("geometryIdMode")).isString())
        {
            if (error)
                *error = QStringLiteral("'geometryIdMode' must be 'current' or 'original'");
            return false;
        }
        const QString geometryIdMode = root.value(QStringLiteral("geometryIdMode"))
            .toString(kCurrentGeometryIdMode).trimmed().toLower();
        if (geometryIdMode != kCurrentGeometryIdMode
            && geometryIdMode != kOriginalGeometryIdMode)
        {
            if (error)
                *error = QStringLiteral("'geometryIdMode' must be 'current' or 'original'");
            return false;
        }

        m_scriptName = root.value(QStringLiteral("name")).toString(
            QFileInfo(m_scriptPath).completeBaseName());
        // 根级开关每次加载脚本时重新赋值，防止复用执行器时沿用上一个脚本的设置。
        m_closeOnFinish = root.value(QStringLiteral("closeOnFinish")).toBool(false);
        m_geometryIdMode = geometryIdMode;
        m_variables.clear();
        const QJsonObject variables = root.value(QStringLiteral("variables")).toObject();
        for (auto it = variables.begin(); it != variables.end(); ++it)
        {
            bool ok = false;
            const QString value = jsonScalarToString(it.value(), &ok);
            if (!ok)
            {
                if (error)
                    *error = QStringLiteral("Variable '%1' must be a scalar value").arg(it.key());
                return false;
            }
            m_variables.insert(it.key(), value);
        }

        const QJsonObject defaults = root.value(QStringLiteral("defaults")).toObject();
        if (!validateKeys(defaults,
            { QStringLiteral("enabled"), QStringLiteral("waitFor"),
              QStringLiteral("timeoutMs"), QStringLiteral("pollIntervalMs"),
              QStringLiteral("retries"), QStringLiteral("retryDelayMs"),
              QStringLiteral("onError") },
            QStringLiteral("'defaults'"), error))
            return false;
        const QJsonArray rawSteps = root.value(QStringLiteral("steps")).toArray();
        if (rawSteps.isEmpty())
        {
            if (error)
                *error = QStringLiteral("'steps' cannot be empty");
            return false;
        }

        m_steps.clear();
        // 默认配置先合入步骤，再由步骤自身字段覆盖。
        for (int i = 0; i < rawSteps.size(); ++i)
        {
            if (!rawSteps.at(i).isObject())
            {
                if (error)
                    *error = QStringLiteral("steps[%1] must be an object").arg(i);
                return false;
            }

            QJsonObject merged = defaults;
            const QJsonObject rawStep = rawSteps.at(i).toObject();
            for (auto it = rawStep.begin(); it != rawStep.end(); ++it)
                merged.insert(it.key(), it.value());

            Step step;
            if (!parseStep(merged, i, &step, error))
                return false;
            m_steps.append(step);
        }
        return true;
    }

    // 解析单个步骤，内部 ID 仅由数组下标生成，不接受 JSON 指定。
    bool mAPIScriptExecutor::parseStep(const QJsonObject& object, int index,
        Step* step, QString* error) const
    {
        if (!validateKeys(object,
            { QStringLiteral("api"), QStringLiteral("params"),
              QStringLiteral("enabled"),
              QStringLiteral("waitFor"), QStringLiteral("timeoutMs"),
              QStringLiteral("pollIntervalMs"), QStringLiteral("retries"),
              QStringLiteral("retryDelayMs"), QStringLiteral("onError") },
            QStringLiteral("steps[%1]").arg(index), error))
            return false;
        if (!object.value(QStringLiteral("api")).isString() ||
            (object.contains(QStringLiteral("enabled")) &&
                !object.value(QStringLiteral("enabled")).isBool()) ||
            (object.contains(QStringLiteral("waitFor")) &&
                !object.value(QStringLiteral("waitFor")).isString()) ||
            (object.contains(QStringLiteral("onError")) &&
                !object.value(QStringLiteral("onError")).isString()))
        {
            if (error)
                *error = QStringLiteral("steps[%1] has an invalid field type").arg(index);
            return false;
        }
        step->id = QStringLiteral("step-%1")
            .arg(index + 1, 3, 10, QLatin1Char('0'));
        step->apiKey = object.value(QStringLiteral("api")).toString().trimmed();

        if (step->apiKey.isEmpty())
        {
            if (error)
                *error = QStringLiteral("steps[%1] requires a non-empty 'api'").arg(index);
            return false;
        }

        const ScriptExecutionPolicy policy = m_executionPolicies.value(
            step->apiKey, m_defaultExecutionPolicy);
        step->enabled = object.value(QStringLiteral("enabled")).toBool(true);
        step->waitFor = object.contains(QStringLiteral("waitFor"))
            ? object.value(QStringLiteral("waitFor")).toString().trimmed()
            : policy.completionCondition;
        step->onError = object.contains(QStringLiteral("onError"))
            ? object.value(QStringLiteral("onError")).toString().trimmed().toLower()
            : policy.onError;
        step->timeoutMs = policy.timeoutMs;
        step->pollIntervalMs = policy.pollIntervalMs;
        step->retries = policy.retries;
        step->retryDelayMs = policy.retryDelayMs;

        if (object.contains(QStringLiteral("waitFor")) && step->waitFor.isEmpty())
        {
            if (error)
                *error = QStringLiteral("Step '%1': waitFor cannot be empty").arg(step->id);
            return false;
        }
        if (object.contains(QStringLiteral("params")) &&
            !object.value(QStringLiteral("params")).isObject())
        {
            if (error)
                *error = QStringLiteral("Step '%1': 'params' must be an object").arg(step->id);
            return false;
        }
        step->rawParams = object.value(QStringLiteral("params")).toObject();

        struct IntegerField { const char* name; int defaultValue; int minimum; int maximum; int* target; };
        IntegerField fields[] = {
            { "timeoutMs", step->timeoutMs, 1, 86400000, &step->timeoutMs },
            { "pollIntervalMs", step->pollIntervalMs, 10, 60000, &step->pollIntervalMs },
            { "retries", step->retries, 0, 10, &step->retries },
            { "retryDelayMs", step->retryDelayMs, 0, 600000, &step->retryDelayMs }
        };
        for (const IntegerField& field : fields)
        {
            const QString name = QString::fromLatin1(field.name);
            const QJsonValue value = object.value(name);
            if (value.isUndefined())
            {
                *field.target = field.defaultValue;
                continue;
            }
            if (!isNonNegativeInteger(value) || value.toInt() < field.minimum ||
                value.toInt() > field.maximum)
            {
                if (error)
                    *error = QStringLiteral("Step '%1': '%2' must be an integer in [%3, %4]")
                    .arg(step->id, name).arg(field.minimum).arg(field.maximum);
                return false;
            }
            *field.target = value.toInt();
        }

        if (step->onError != QStringLiteral("stop") &&
            step->onError != QStringLiteral("continue"))
        {
            if (error)
                *error = QStringLiteral("Step '%1': onError must be 'stop' or 'continue'")
                .arg(step->id);
            return false;
        }
        return true;
    }

    // 递归展开脚本目录、程序目录、环境变量和用户变量。
    bool mAPIScriptExecutor::resolveString(const QString& input, QString* output,
        QString* error, int depth) const
    {
        if (depth > 10)
        {
            if (error)
                *error = QStringLiteral("Variable expansion is recursive or exceeds 10 levels");
            return false;
        }

        static const QRegularExpression expression(QStringLiteral("\\$\\{([^{}]+)\\}"));
        QRegularExpressionMatchIterator iterator = expression.globalMatch(input);
        QString resolved;
        int offset = 0;
        bool matched = false;
        while (iterator.hasNext())
        {
            matched = true;
            const QRegularExpressionMatch match = iterator.next();
            resolved += input.mid(offset, match.capturedStart() - offset);
            const QString key = match.captured(1);
            QString replacement;
            if (key == QStringLiteral("scriptDir"))
                replacement = QDir::toNativeSeparators(m_scriptDir);
            else if (key == QStringLiteral("exeDir"))
                replacement = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
            else if (key.startsWith(QStringLiteral("env:")))
            {
                const QString name = key.mid(4);
                if (name.isEmpty() || !qEnvironmentVariableIsSet(name.toLocal8Bit().constData()))
                {
                    if (error)
                        *error = QStringLiteral("Environment variable '%1' is not set").arg(name);
                    return false;
                }
                replacement = qEnvironmentVariable(name.toLocal8Bit().constData());
            }
            else
            {
                const QString variableName = key.startsWith(QStringLiteral("var:"))
                    ? key.mid(4) : key;
                if (!m_variables.contains(variableName))
                {
                    if (error)
                        *error = QStringLiteral("Unknown variable '%1'").arg(key);
                    return false;
                }
                replacement = m_variables.value(variableName);
            }
            resolved += replacement;
            offset = match.capturedEnd();
        }

        if (!matched)
        {
            *output = input;
            return true;
        }
        resolved += input.mid(offset);
        if (expression.match(resolved).hasMatch())
            return resolveString(resolved, output, error, depth + 1);
        *output = resolved;
        return true;
    }

    // 标量参数转为单元素列表，数组参数保持原有顺序。
    bool mAPIScriptExecutor::convertParamValue(const QJsonValue& value,
        QStringList* values, QString* error) const
    {
        values->clear();
        const QJsonArray array = value.isArray() ? value.toArray() : QJsonArray{ value };
        for (const QJsonValue& item : array)
        {
            bool ok = false;
            const QString scalar = jsonScalarToString(item, &ok);
            if (!ok)
            {
                if (error)
                    *error = QStringLiteral("Parameter values must be scalars or arrays of scalars");
                return false;
            }
            QString resolved;
            if (!resolveString(scalar, &resolved, error))
                return false;
            values->append(resolved);
        }
        return true;
    }

    // 对一个步骤的全部参数执行类型转换和变量展开。
    bool mAPIScriptExecutor::buildParams(const Step& step, APIParams* params,
        QString* error) const
    {
        params->clear();
        for (auto it = step.rawParams.begin(); it != step.rawParams.end(); ++it)
        {
            QStringList values;
            if (!convertParamValue(it.value(), &values, error))
            {
                if (error)
                    *error = QStringLiteral("Step '%1', parameter '%2': %3")
                    .arg(step.id, it.key(), *error);
                return false;
            }
            params->insert(it.key(), values);
        }
        return true;
    }

    // 在第一条 API 执行前完成注册、必填参数和完成条件检查。
    bool mAPIScriptExecutor::preflight(QString* error)
    {
        if (!m_scheduler)
        {
            if (error)
                *error = QStringLiteral("API scheduler is null");
            return false;
        }

        for (const Step& step : m_steps)
        {
            if (!step.enabled)
                continue;
            if (step.rawParams.contains(kOriginalFaceIdContextParam))
            {
                if (error)
                    *error = QStringLiteral(
                        "Step '%1': parameter '%2' is reserved for the script executor")
                    .arg(step.id, kOriginalFaceIdContextParam);
                return false;
            }
            if (m_geometryIdMode == kOriginalGeometryIdMode
                && step.apiKey.startsWith(QStringLiteral("directModeling"))
                && step.apiKey != QStringLiteral("directModelingOffsetFace"))
            {
                if (error)
                {
                    *error = QStringLiteral(
                        "Step '%1': geometryIdMode='original' currently supports only "
                        "directModelingOffsetFace")
                        .arg(step.id);
                }
                return false;
            }
            if (!m_scheduler->hasAPI(step.apiKey))
            {
                if (error)
                    *error = QStringLiteral("Step '%1': API '%2' is not registered")
                    .arg(step.id, step.apiKey);
                return false;
            }

            APIParams params;
            if (!buildParams(step, &params, error))
                return false;
            const APIInfo info = m_scheduler->getAPIInfo(step.apiKey);
            QString missing;
            if (!mAPISchedulingServer::validateRequiredParams(
                params, info._requiredParams, missing))
            {
                if (error)
                    *error = QStringLiteral("Step '%1': missing required parameters: %2")
                    .arg(step.id, missing);
                return false;
            }

            const QString condition = step.waitFor;
            if (!condition.isEmpty() && !m_completionConditions.contains(condition))
            {
                if (error)
                    *error = QStringLiteral("Step '%1': completion condition '%2' is not registered")
                    .arg(step.id, condition);
                return false;
            }
        }
        return true;
    }

    // 初始化报告和状态机，并在下一次事件循环中执行第一步。
    bool mAPIScriptExecutor::start(QString* error)
    {
        if (!m_loaded)
        {
            if (error)
                *error = QStringLiteral("No valid script has been loaded");
            return false;
        }
        if (m_running)
        {
            if (error)
                *error = QStringLiteral("The script is already running");
            return false;
        }
        if (!preflight(error))
            return false;

        m_currentStep = 0;
        m_currentAttempt = 0;
        m_finalFailureCode = Succeeded;
        m_cancelRequested = false;
        // requestId 每次重试都会改变，不能用作跨步骤拓扑映射的身份；单独生成
        // 一个在本次脚本整个生命周期内稳定的 executionId。
        m_scriptExecutionId =
            QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_stepReports = QJsonArray();
        m_report = QJsonObject{
            { QStringLiteral("schemaVersion"), QStringLiteral("1.0") },
            { QStringLiteral("scriptName"), m_scriptName },
            { QStringLiteral("scriptPath"), QDir::toNativeSeparators(m_scriptPath) },
            { QStringLiteral("scriptExecutionId"), m_scriptExecutionId },
            { QStringLiteral("geometryIdMode"), m_geometryIdMode },
            { QStringLiteral("startedAtUtc"), utcNow() },
            { QStringLiteral("status"), QStringLiteral("running") }
        };
        m_running = true;
        publishEvent(QStringLiteral("script_started"), {
            { QStringLiteral("scriptName"), m_scriptName },
            { QStringLiteral("scriptExecutionId"), m_scriptExecutionId },
            { QStringLiteral("geometryIdMode"), m_geometryIdMode },
            { QStringLiteral("stepCount"), m_steps.size() }
            });
        QTimer::singleShot(0, this, &mAPIScriptExecutor::executeCurrentStep);
        return true;
    }

    // 取消只改变状态并停止轮询，不强制终止正在运行的同步业务函数。
    void mAPIScriptExecutor::cancel()
    {
        if (!m_running)
            return;
        m_cancelRequested = true;
        m_pollTimer.stop();
        QTimer::singleShot(0, this, [this]() {
            if (m_running)
                finishScript(Cancelled, QStringLiteral("Execution cancelled"));
            });
    }

    // 串行派发当前步骤；同步完成或进入异步完成条件等待。
    void mAPIScriptExecutor::executeCurrentStep()
    {
        if (!m_running)
            return;
        if (m_cancelRequested)
        {
            finishScript(Cancelled, QStringLiteral("Execution cancelled"));
            return;
        }
        if (m_currentStep >= m_steps.size())
        {
            const int code = m_finalFailureCode == Succeeded
                ? Succeeded : m_finalFailureCode;
            finishScript(code, code == Succeeded
                ? QStringLiteral("All steps completed")
                : QStringLiteral("Execution completed with failed steps"));
            return;
        }

        const Step& step = m_steps.at(m_currentStep);
        if (!step.enabled)
        {
            m_currentAttempt = 0;
            m_currentRequestId.clear();
            m_currentDispatchStatus = QStringLiteral("skipped");
            m_stepTimer.restart();
            appendStepReport(QStringLiteral("skipped"), QStringLiteral("Step is disabled"));
            publishEvent(QStringLiteral("step_skipped"), {
                { QStringLiteral("stepId"), step.id },
                { QStringLiteral("api"), step.apiKey }
                });
            ++m_currentStep;
            QTimer::singleShot(0, this, &mAPIScriptExecutor::executeCurrentStep);
            return;
        }

        ++m_currentAttempt;
        m_stepTimer.restart();
        m_currentDispatchStatus = QStringLiteral("not_started");
        m_currentRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        QString error;
        if (!buildParams(step, &m_currentParams, &error))
        {
            failCurrentStep(error, PreflightFailed);
            return;
        }
        if (m_geometryIdMode == kOriginalGeometryIdMode)
        {
            // 公开脚本仍只传入最初导入时的几何 ID。这个保留参数
            // 把同一次脚本运行的所有步骤关联到 Controller 中同一份
            // “最初 ID -> 当前 ID”映射。后续的边界、载荷和接触步骤也需要
            // 使用这份映射，因此不能只在直接建模步骤中注入。
            // 参数名在 preflight() 中被保留，用户无法伪造该上下文；
            // geometryIdMode="current" 的普通脚本不会进入此分支。
            m_currentParams.insert(
                kOriginalFaceIdContextParam,
                QStringList{ m_scriptExecutionId });
        }

        publishEvent(QStringLiteral("step_started"), {
            { QStringLiteral("stepId"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("index"), m_currentStep },
            { QStringLiteral("attempt"), m_currentAttempt },
            { QStringLiteral("requestId"), m_currentRequestId }
            });

        // 真实 APIExecutionResult 使用 _success 和 _errorMessage 保存派发结果；
        // success()/failure() 是静态工厂函数，不能当作布尔状态读取。
        const APIExecutionResult result = m_scheduler->executeAPI(
            step.apiKey, m_currentParams, m_currentRequestId);
        if (!result._success)
        {
            m_currentDispatchStatus = QStringLiteral("failed");
            failCurrentStep(result._errorMessage.isEmpty()
                ? QStringLiteral("API dispatch failed")
                : result._errorMessage,
                StepFailed);
            return;
        }
        m_currentDispatchStatus = QStringLiteral("succeeded");

        publishEvent(QStringLiteral("step_dispatched"), {
            { QStringLiteral("stepId"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("requestId"), m_currentRequestId }
            });

        const QString condition = step.waitFor;
        if (condition.isEmpty())
        {
            completeCurrentStep(QStringLiteral("Synchronous API completed"));
            return;
        }

        publishEvent(QStringLiteral("step_waiting"), {
            { QStringLiteral("stepId"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("condition"), condition },
            { QStringLiteral("timeoutMs"), step.timeoutMs }
            });
        m_pollTimer.start(step.pollIntervalMs);
    }

    // 查询异步业务终态；Pending 会继续定时轮询，且受 timeoutMs 限制。
    void mAPIScriptExecutor::pollCompletionCondition()
    {
        if (!m_running || m_currentStep >= m_steps.size())
            return;
        if (m_cancelRequested)
        {
            finishScript(Cancelled, QStringLiteral("Execution cancelled"));
            return;
        }

        const Step& step = m_steps.at(m_currentStep);
        const QString condition = step.waitFor;
        ScriptConditionResult result;
        try
        {
            result = m_completionConditions.value(condition)(m_currentParams);
        }
        catch (const std::exception& exception)
        {
            result = ScriptConditionResult::failed(
                QStringLiteral("Completion probe exception: %1").arg(exception.what()));
        }
        catch (...)
        {
            result = ScriptConditionResult::failed(
                QStringLiteral("Unknown completion probe exception"));
        }

        if (result.state == ScriptConditionState::Completed)
        {
            completeCurrentStep(result.message.isEmpty()
                ? QStringLiteral("Completion condition satisfied") : result.message);
            return;
        }
        if (result.state == ScriptConditionState::Failed)
        {
            failCurrentStep(result.message.isEmpty()
                ? QStringLiteral("Completion condition reported failure") : result.message,
                StepFailed);
            return;
        }
        if (m_stepTimer.elapsed() >= step.timeoutMs)
        {
            failCurrentStep(QStringLiteral("Timed out after %1 ms waiting for '%2'")
                .arg(step.timeoutMs).arg(condition), StepTimedOut);
            return;
        }
        m_pollTimer.start(step.pollIntervalMs);
    }

    // 记录完成结果并将状态机推进到下一步骤。
    void mAPIScriptExecutor::completeCurrentStep(const QString& message)
    {
        const Step& step = m_steps.at(m_currentStep);
        appendStepReport(QStringLiteral("completed"), message);
        publishEvent(QStringLiteral("step_completed"), {
            { QStringLiteral("stepId"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("dispatchStatus"), m_currentDispatchStatus },
            { QStringLiteral("durationMs"), static_cast<double>(m_stepTimer.elapsed()) },
            { QStringLiteral("attempts"), m_currentAttempt }
            });
        ++m_currentStep;
        m_currentAttempt = 0;
        QTimer::singleShot(0, this, &mAPIScriptExecutor::executeCurrentStep);
    }

    // 统一处理派发失败、业务失败、超时、重试和 onError 策略。
    void mAPIScriptExecutor::failCurrentStep(const QString& message, int exitCode)
    {
        const Step& step = m_steps.at(m_currentStep);
        publishEvent(QStringLiteral("step_failed"), {
            { QStringLiteral("stepId"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("attempt"), m_currentAttempt },
            { QStringLiteral("message"), message },
            { QStringLiteral("dispatchStatus"), m_currentDispatchStatus },
            { QStringLiteral("exitCode"), exitCode }
            });

        if (m_currentAttempt <= step.retries)
        {
            publishEvent(QStringLiteral("step_retry_scheduled"), {
                { QStringLiteral("stepId"), step.id },
                { QStringLiteral("nextAttempt"), m_currentAttempt + 1 },
                { QStringLiteral("delayMs"), step.retryDelayMs }
                });
            QTimer::singleShot(step.retryDelayMs,
                this, &mAPIScriptExecutor::executeCurrentStep);
            return;
        }

        appendStepReport(exitCode == StepTimedOut
            ? QStringLiteral("timed_out") : QStringLiteral("failed"), message);
        if (m_finalFailureCode == Succeeded || exitCode == StepTimedOut)
            m_finalFailureCode = exitCode;

        if (step.onError == QStringLiteral("continue"))
        {
            ++m_currentStep;
            m_currentAttempt = 0;
            QTimer::singleShot(0, this, &mAPIScriptExecutor::executeCurrentStep);
            return;
        }
        finishScript(exitCode, message);
    }

    // 报告同时保留派发状态和业务完成状态，避免把“已接受”误认为“已完成”。
    void mAPIScriptExecutor::appendStepReport(const QString& status,
        const QString& message)
    {
        const Step& step = m_steps.at(m_currentStep);
        QJsonObject item{
            { QStringLiteral("index"), m_currentStep },
            { QStringLiteral("id"), step.id },
            { QStringLiteral("api"), step.apiKey },
            { QStringLiteral("status"), status },
            { QStringLiteral("attempts"), m_currentAttempt },
            { QStringLiteral("durationMs"), static_cast<double>(m_stepTimer.elapsed()) }
        };

        QJsonObject dispatch{
            { QStringLiteral("status"), m_currentDispatchStatus }
        };
        if (!m_currentRequestId.isEmpty() &&
            m_currentDispatchStatus != QStringLiteral("skipped"))
            dispatch.insert(QStringLiteral("requestId"), m_currentRequestId);

        QString completionStatus = status;
        if (m_currentDispatchStatus == QStringLiteral("failed") ||
            m_currentDispatchStatus == QStringLiteral("not_started"))
        {
            completionStatus = QStringLiteral("not_started");
            if (!message.isEmpty())
                dispatch.insert(QStringLiteral("message"), message);
        }
        else if (!message.isEmpty())
        {
            item.insert(QStringLiteral("message"), message);
        }

        QJsonObject completion{
            { QStringLiteral("status"), completionStatus }
        };
        if (!message.isEmpty() && completionStatus != QStringLiteral("not_started"))
            completion.insert(QStringLiteral("message"), message);
        item.insert(QStringLiteral("dispatch"), dispatch);
        item.insert(QStringLiteral("completion"), completion);
        m_stepReports.append(item);
    }

    // 所有时间戳使用 UTC ISO 8601，便于跨机器汇总。
    QString mAPIScriptExecutor::utcNow() const
    {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    }

    // 为实时 JSONL 输出补充统一事件名和时间戳。
    void mAPIScriptExecutor::publishEvent(const QString& name,
        const QJsonObject& fields)
    {
        QJsonObject event = fields;
        event.insert(QStringLiteral("event"), name);
        event.insert(QStringLiteral("timestampUtc"), utcNow());
        emit eventReady(event);
    }

    // QSaveFile 先写临时文件再原子替换，避免留下半份报告。
    bool mAPIScriptExecutor::writeReport(QString* error) const
    {
        const QFileInfo reportInfo(m_reportPath);
        if (!QDir().mkpath(reportInfo.absolutePath()))
        {
            if (error)
                *error = QStringLiteral("Cannot create report directory '%1'")
                .arg(QDir::toNativeSeparators(reportInfo.absolutePath()));
            return false;
        }

        QSaveFile file(m_reportPath);
        if (!file.open(QIODevice::WriteOnly))
        {
            if (error)
                *error = QStringLiteral("Cannot open report '%1': %2")
                .arg(QDir::toNativeSeparators(m_reportPath), file.errorString());
            return false;
        }
        if (file.write(QJsonDocument(m_report).toJson(QJsonDocument::Indented)) < 0 ||
            !file.commit())
        {
            if (error)
                *error = QStringLiteral("Cannot write report '%1': %2")
                .arg(QDir::toNativeSeparators(m_reportPath), file.errorString());
            return false;
        }
        return true;
    }

    // 收敛最终状态、写报告并发布结束事件；是否退出由应用层自行决定。
    void mAPIScriptExecutor::finishScript(int exitCode, const QString& message)
    {
        if (!m_running)
            return;
        m_pollTimer.stop();
        m_running = false;

        m_report.insert(QStringLiteral("finishedAtUtc"), utcNow());
        m_report.insert(QStringLiteral("steps"), m_stepReports);
        m_report.insert(QStringLiteral("exitCode"), exitCode);
        m_report.insert(QStringLiteral("status"), exitCode == Succeeded
            ? QStringLiteral("completed")
            : (exitCode == Cancelled ? QStringLiteral("cancelled") : QStringLiteral("failed")));
        m_report.insert(QStringLiteral("message"), message);
        m_report.insert(QStringLiteral("reportEnabled"), m_reportEnabled);
        m_report.insert(QStringLiteral("closeOnFinish"), m_closeOnFinish);

        QString reportError;
        if (m_reportEnabled && !writeReport(&reportError))
        {
            exitCode = ReportWriteFailed;
            m_report.insert(QStringLiteral("exitCode"), exitCode);
            m_report.insert(QStringLiteral("status"), QStringLiteral("failed"));
            m_report.insert(QStringLiteral("reportError"), reportError);
        }

        publishEvent(QStringLiteral("script_finished"), {
            { QStringLiteral("exitCode"), exitCode },
            { QStringLiteral("status"), m_report.value(QStringLiteral("status")) },
            { QStringLiteral("message"), m_report.value(QStringLiteral("message")) },
            { QStringLiteral("scriptExecutionId"), m_scriptExecutionId },
            { QStringLiteral("geometryIdMode"), m_geometryIdMode },
            { QStringLiteral("closeOnFinish"), m_closeOnFinish },
            { QStringLiteral("reportEnabled"), m_reportEnabled },
            { QStringLiteral("reportPath"), m_reportEnabled
                ? QDir::toNativeSeparators(m_reportPath) : QString() },
            { QStringLiteral("reportError"), m_report.value(QStringLiteral("reportError")) }
            });
        emit finished(exitCode, m_report);
    }
}
