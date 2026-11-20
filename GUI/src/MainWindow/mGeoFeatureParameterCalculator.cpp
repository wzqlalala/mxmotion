#include "mGeoFeatureParameterCalculator.h"

#include <QObject>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include "mOtherFunction.h"

namespace
{
//【参数显示】最多保留 6 位小数（去尾零；整数不带小数点）。
static QString formatParameterValue(double value)
{
    if (!std::isfinite(value))
        return QString();
    QString text = QString::number(value, 'f', 6);
    if (text.contains(QLatin1Char('.')))
    {
        while (text.endsWith(QLatin1Char('0')))
            text.chop(1);
        if (text.endsWith(QLatin1Char('.')))
            text.chop(1);
    }
    return text;
}

    // 解析器不是 QObject 子类，统一借用 QObject 的翻译入口。
    QString tr(const char* sourceText)
    {
        return QObject::tr(sourceText);
    }

    bool IsLogicalTrue(double value)
    {
        return std::abs(value) > 1.0e-12;
    }

    QString NormalizedParameterName(const QString& name)
    {
        return name.trimmed().toCaseFolded();
    }

    bool IsValidParameterName(const QString& name)
    {
        if (name.isEmpty() || !(name.front().isLetter() || name.front() == QLatin1Char('_')))
            return false;
        for (const QChar ch : name)
        {
            if (!(ch.isLetterOrNumber() || ch == QLatin1Char('_')))
                return false;
        }
        return true;
    }

    /**
     * 使用已校验的“旧名 -> 最终新名”表精确改写参数引用。
     *
     * 这里特意不做普通子串 replace：只有被 {} 包围的完整标记才是
     * 参数引用。一次扫描也能避免 A -> B、B -> A 互换名称时的二次替换。
     */
    bool RewriteParameterReferences(
        const QString& expression,
        const QHash<QString, QString>& finalNameByOldNormalizedName,
        QString& rewrittenExpression,
        QString& errorMessage)
    {
        rewrittenExpression.clear();
        rewrittenExpression.reserve(expression.size());
        errorMessage.clear();

        for (int position = 0; position < expression.size();)
        {
            const QChar ch = expression.at(position);
            if (ch == QLatin1Char('}'))
            {
                errorMessage = tr("位置 %1 存在未配对的 '}'")
                    .arg(position + 1);
                return false;
            }
            if (ch != QLatin1Char('{'))
            {
                rewrittenExpression += ch;
                ++position;
                continue;
            }

            const int referenceStart = position;
            const int nameStart = ++position;
            while (position < expression.size()
                && expression.at(position) != QLatin1Char('}'))
            {
                if (expression.at(position) == QLatin1Char('{'))
                {
                    errorMessage = tr("参数引用不允许嵌套花括号");
                    return false;
                }
                ++position;
            }
            if (position >= expression.size())
            {
                errorMessage = tr("位置 %1 的参数引用缺少 '}'")
                    .arg(referenceStart + 1);
                return false;
            }

            const QString referencedName =
                expression.mid(nameStart, position - nameStart).trimmed();
            if (referencedName.isEmpty())
            {
                errorMessage = tr("参数引用 '{}' 不能为空");
                return false;
            }
            if (!IsValidParameterName(referencedName))
            {
                errorMessage = tr("花括号中的参数名无效：%1")
                    .arg(referencedName);
                return false;
            }

            const auto renamed = finalNameByOldNormalizedName.constFind(
                NormalizedParameterName(referencedName));
            if (renamed != finalNameByOldNormalizedName.constEnd())
                rewrittenExpression += QLatin1Char('{') + renamed.value() + QLatin1Char('}');
            else
            {
                // 未知名称由正式求值阶段给出“未知参数”错误；
                // 此处保留原始文本，不额外改变用户的公式格式。
                rewrittenExpression += expression.mid(
                    referenceStart, position - referenceStart + 1);
            }
            ++position; // 跳过已处理的 '}'。
        }
        return true;
    }

    /**
     * 直接建模标量表达式解析器。
     *
     * 不使用脚本引擎，公式只能访问显式传入的参数解析回调，不能读写文件、对象或
     * UI 状态。运算优先级从低到高为：三目、或、与、相等、比较、加减、乘除、
     * 一元、乘方、括号/函数。逻辑结果统一返回 0 或 1。
     */
    class DirectModelingExpressionParser
    {
    public:
        using Resolver = std::function<bool(const QString&, double&, QString&)>;

        DirectModelingExpressionParser(QString expression, Resolver resolver)
            : m_expression(std::move(expression))
            , m_resolver(std::move(resolver))
        {
            m_expression = m_expression.trimmed();
            if (m_expression.startsWith(QLatin1Char('=')))
                m_expression.remove(0, 1);
        }

        bool parse(double& value, QString& errorMessage)
        {
            if (m_expression.trimmed().isEmpty())
                return fail(tr("表达式不能为空"), errorMessage);
            if (!parseConditional(value, errorMessage))
                return false;
            skipSpaces();
            if (m_position != m_expression.size())
            {
                return fail(
                    tr("位置 %1 附近存在无法识别的内容").arg(m_position + 1),
                    errorMessage);
            }
            if (!std::isfinite(value))
                return fail(tr("表达式结果不是有限数值"), errorMessage);
            return true;
        }

    private:
        bool parseConditional(double& value, QString& errorMessage)
        {
            if (!parseLogicalOr(value, errorMessage))
                return false;
            if (!match(QStringLiteral("?")))
                return true;

            double trueValue = 0.0;
            double falseValue = 0.0;
            if (!parseConditional(trueValue, errorMessage))
                return false;
            if (!match(QStringLiteral(":")))
                return fail(tr("条件表达式缺少 ':'"), errorMessage);
            if (!parseConditional(falseValue, errorMessage))
                return false;
            value = IsLogicalTrue(value) ? trueValue : falseValue;
            return true;
        }

        bool parseLogicalOr(double& value, QString& errorMessage)
        {
            if (!parseLogicalAnd(value, errorMessage))
                return false;
            while (match(QStringLiteral("||")))
            {
                double right = 0.0;
                if (!parseLogicalAnd(right, errorMessage))
                    return false;
                value = IsLogicalTrue(value) || IsLogicalTrue(right) ? 1.0 : 0.0;
            }
            return true;
        }

        bool parseLogicalAnd(double& value, QString& errorMessage)
        {
            if (!parseEquality(value, errorMessage))
                return false;
            while (match(QStringLiteral("&&")))
            {
                double right = 0.0;
                if (!parseEquality(right, errorMessage))
                    return false;
                value = IsLogicalTrue(value) && IsLogicalTrue(right) ? 1.0 : 0.0;
            }
            return true;
        }

        bool parseEquality(double& value, QString& errorMessage)
        {
            if (!parseComparison(value, errorMessage))
                return false;
            for (;;)
            {
                bool equalOperator = false;
                if (match(QStringLiteral("==")))
                    equalOperator = true;
                else if (!match(QStringLiteral("!=")))
                    return true;

                double right = 0.0;
                if (!parseComparison(right, errorMessage))
                    return false;
                const bool equal = std::abs(value - right) <= 1.0e-12;
                value = (equalOperator ? equal : !equal) ? 1.0 : 0.0;
            }
        }

        bool parseComparison(double& value, QString& errorMessage)
        {
            if (!parseAdditive(value, errorMessage))
                return false;
            for (;;)
            {
                enum class Operator { None, Less, LessEqual, Greater, GreaterEqual };
                Operator op = Operator::None;
                if (match(QStringLiteral("<="))) op = Operator::LessEqual;
                else if (match(QStringLiteral(">="))) op = Operator::GreaterEqual;
                else if (match(QStringLiteral("<"))) op = Operator::Less;
                else if (match(QStringLiteral(">"))) op = Operator::Greater;
                else return true;

                double right = 0.0;
                if (!parseAdditive(right, errorMessage))
                    return false;
                bool result = false;
                switch (op)
                {
                case Operator::Less: result = value < right; break;
                case Operator::LessEqual: result = value <= right; break;
                case Operator::Greater: result = value > right; break;
                case Operator::GreaterEqual: result = value >= right; break;
                default: break;
                }
                value = result ? 1.0 : 0.0;
            }
        }

        bool parseAdditive(double& value, QString& errorMessage)
        {
            if (!parseMultiplicative(value, errorMessage))
                return false;
            for (;;)
            {
                const bool add = match(QStringLiteral("+"));
                if (!add && !match(QStringLiteral("-")))
                    return true;
                double right = 0.0;
                if (!parseMultiplicative(right, errorMessage))
                    return false;
                value = add ? value + right : value - right;
            }
        }

        bool parseMultiplicative(double& value, QString& errorMessage)
        {
            if (!parseUnary(value, errorMessage))
                return false;
            for (;;)
            {
                enum class Operator { None, Multiply, Divide, Modulo };
                Operator op = Operator::None;
                if (match(QStringLiteral("*"))) op = Operator::Multiply;
                else if (match(QStringLiteral("/"))) op = Operator::Divide;
                else if (match(QStringLiteral("%"))) op = Operator::Modulo;
                else return true;

                double right = 0.0;
                if (!parseUnary(right, errorMessage))
                    return false;
                if ((op == Operator::Divide || op == Operator::Modulo)
                    && std::abs(right) <= std::numeric_limits<double>::epsilon())
                {
                    return fail(tr("除数不能为 0"), errorMessage);
                }
                if (op == Operator::Multiply) value *= right;
                else if (op == Operator::Divide) value /= right;
                else value = std::fmod(value, right);
            }
        }

        bool parseUnary(double& value, QString& errorMessage)
        {
            if (match(QStringLiteral("+")))
                return parseUnary(value, errorMessage);
            if (match(QStringLiteral("-")))
            {
                if (!parseUnary(value, errorMessage))
                    return false;
                value = -value;
                return true;
            }
            if (match(QStringLiteral("!")))
            {
                if (!parseUnary(value, errorMessage))
                    return false;
                value = IsLogicalTrue(value) ? 0.0 : 1.0;
                return true;
            }
            return parsePower(value, errorMessage);
        }

        bool parsePower(double& value, QString& errorMessage)
        {
            if (!parsePrimary(value, errorMessage))
                return false;
            if (match(QStringLiteral("^")))
            {
                double exponent = 0.0;
                if (!parseUnary(exponent, errorMessage))
                    return false;
                value = std::pow(value, exponent);
                if (!std::isfinite(value))
                    return fail(tr("乘方结果超出有效数值范围"), errorMessage);
            }
            return true;
        }

        bool parsePrimary(double& value, QString& errorMessage)
        {
            skipSpaces();
            if (match(QStringLiteral("(")))
            {
                if (!parseConditional(value, errorMessage))
                    return false;
                if (!match(QStringLiteral(")")))
                    return fail(tr("缺少右括号 ')'"), errorMessage);
                return true;
            }

            if (m_position < m_expression.size()
                && (m_expression.at(m_position).isDigit()
                    || m_expression.at(m_position) == QLatin1Char('.')))
            {
                return parseNumber(value, errorMessage);
            }

            if (m_position < m_expression.size()
                && m_expression.at(m_position) == QLatin1Char('{'))
            {
                return parseParameterReference(value, errorMessage);
            }
            if (m_position < m_expression.size()
                && m_expression.at(m_position) == QLatin1Char('}'))
            {
                return fail(
                    tr("位置 %1 存在未配对的 '}'").arg(m_position + 1),
                    errorMessage);
            }

            QString identifier;
            if (!parseIdentifier(identifier))
                return fail(tr("位置 %1 需要数值、参数或函数").arg(m_position + 1), errorMessage);

            if (match(QStringLiteral("(")))
            {
                QVector<double> arguments;
                if (!match(QStringLiteral(")")))
                {
                    for (;;)
                    {
                        double argument = 0.0;
                        if (!parseConditional(argument, errorMessage))
                            return false;
                        arguments.push_back(argument);
                        if (match(QStringLiteral(")")))
                            break;
                        if (!match(QStringLiteral(",")))
                            return fail(tr("函数参数之间需要使用逗号 ','"), errorMessage);
                    }
                }
                return evaluateFunction(identifier, arguments, value, errorMessage);
            }

            const QString normalized = identifier.toCaseFolded();
            if (normalized == QStringLiteral("pi")) value = 3.14159265358979323846;
            else if (normalized == QStringLiteral("e")) value = 2.71828182845904523536;
            else if (normalized == QStringLiteral("true")) value = 1.0;
            else if (normalized == QStringLiteral("false")) value = 0.0;
            else
            {
                return fail(
                    tr("参数引用必须使用花括号，请将 %1 写为 {%1}")
                    .arg(identifier),
                    errorMessage);
            }
            return true;
        }

        bool parseParameterReference(double& value, QString& errorMessage)
        {
            // parsePrimary 进入此函数时已经 skipSpaces，当前字符必为 '{'。
            const int referenceStart = m_position++;
            const int nameStart = m_position;
            while (m_position < m_expression.size()
                && m_expression.at(m_position) != QLatin1Char('}'))
            {
                if (m_expression.at(m_position) == QLatin1Char('{'))
                    return fail(tr("参数引用不允许嵌套花括号"), errorMessage);
                ++m_position;
            }
            if (m_position >= m_expression.size())
            {
                return fail(
                    tr("位置 %1 的参数引用缺少 '}'")
                    .arg(referenceStart + 1),
                    errorMessage);
            }

            const QString referencedName =
                m_expression.mid(nameStart, m_position - nameStart).trimmed();
            ++m_position; // 跳过 '}'。
            if (referencedName.isEmpty())
                return fail(tr("参数引用 '{}' 不能为空"), errorMessage);
            if (!IsValidParameterName(referencedName))
            {
                return fail(
                    tr("花括号中的参数名无效：%1").arg(referencedName),
                    errorMessage);
            }
            if (!m_resolver)
                return fail(tr("当前表达式不支持参数引用"), errorMessage);
            return m_resolver(referencedName, value, errorMessage);
        }

        bool parseNumber(double& value, QString& errorMessage)
        {
            const int start = m_position;
            bool hasDigit = false;
            while (m_position < m_expression.size() && m_expression.at(m_position).isDigit())
            {
                hasDigit = true;
                ++m_position;
            }
            if (m_position < m_expression.size()
                && m_expression.at(m_position) == QLatin1Char('.'))
            {
                ++m_position;
                while (m_position < m_expression.size() && m_expression.at(m_position).isDigit())
                {
                    hasDigit = true;
                    ++m_position;
                }
            }
            if (!hasDigit)
                return fail(tr("无效数值"), errorMessage);

            if (m_position < m_expression.size()
                && (m_expression.at(m_position) == QLatin1Char('e')
                    || m_expression.at(m_position) == QLatin1Char('E')))
            {
                const int exponentPosition = m_position++;
                if (m_position < m_expression.size()
                    && (m_expression.at(m_position) == QLatin1Char('+')
                        || m_expression.at(m_position) == QLatin1Char('-')))
                {
                    ++m_position;
                }
                const int exponentDigits = m_position;
                while (m_position < m_expression.size() && m_expression.at(m_position).isDigit())
                    ++m_position;
                if (m_position == exponentDigits)
                {
                    m_position = exponentPosition;
                    return fail(tr("科学计数法指数无效"), errorMessage);
                }
            }

            bool ok = false;
            value = m_expression.mid(start, m_position - start).toDouble(&ok);
            if (!ok || !std::isfinite(value))
                return fail(tr("数值超出有效范围"), errorMessage);
            return true;
        }

        bool parseIdentifier(QString& identifier)
        {
            skipSpaces();
            if (m_position >= m_expression.size()
                || !(m_expression.at(m_position).isLetter()
                    || m_expression.at(m_position) == QLatin1Char('_')))
            {
                return false;
            }
            const int start = m_position++;
            while (m_position < m_expression.size())
            {
                const QChar ch = m_expression.at(m_position);
                if (!(ch.isLetterOrNumber() || ch == QLatin1Char('_')))
                    break;
                ++m_position;
            }
            identifier = m_expression.mid(start, m_position - start);
            return true;
        }

        bool evaluateFunction(
            const QString& functionName,
            const QVector<double>& arguments,
            double& value,
            QString& errorMessage)
        {
            const QString name = functionName.toCaseFolded();
            auto requireCount = [&](int count) {
                if (arguments.size() == count)
                    return true;
                errorMessage = tr("函数 %1 需要 %2 个参数，实际为 %3 个")
                    .arg(functionName).arg(count).arg(arguments.size());
                return false;
                };

            if (name == QStringLiteral("min") || name == QStringLiteral("max"))
            {
                if (arguments.isEmpty())
                    return fail(tr("函数 %1 至少需要一个参数").arg(functionName), errorMessage);
                value = arguments.front();
                for (int i = 1; i < arguments.size(); ++i)
                    value = name == QStringLiteral("min")
                    ? (std::min)(value, arguments.at(i))
                    : (std::max)(value, arguments.at(i));
                return true;
            }

            if (name == QStringLiteral("abs") && requireCount(1)) value = std::abs(arguments[0]);
            else if (name == QStringLiteral("sqrt") && requireCount(1))
            {
                if (arguments[0] < 0.0) return fail(tr("sqrt 的参数不能小于 0"), errorMessage);
                value = std::sqrt(arguments[0]);
            }
            else if (name == QStringLiteral("sin") && requireCount(1)) value = std::sin(arguments[0]);
            else if (name == QStringLiteral("cos") && requireCount(1)) value = std::cos(arguments[0]);
            else if (name == QStringLiteral("tan") && requireCount(1)) value = std::tan(arguments[0]);
            else if (name == QStringLiteral("sind") && requireCount(1)) value = std::sin(arguments[0] * 3.14159265358979323846 / 180.0);
            else if (name == QStringLiteral("cosd") && requireCount(1)) value = std::cos(arguments[0] * 3.14159265358979323846 / 180.0);
            else if (name == QStringLiteral("tand") && requireCount(1)) value = std::tan(arguments[0] * 3.14159265358979323846 / 180.0);
            else if (name == QStringLiteral("asin") && requireCount(1))
            {
                if (arguments[0] < -1.0 || arguments[0] > 1.0) return fail(tr("asin 的参数必须在 [-1, 1] 内"), errorMessage);
                value = std::asin(arguments[0]);
            }
            else if (name == QStringLiteral("acos") && requireCount(1))
            {
                if (arguments[0] < -1.0 || arguments[0] > 1.0) return fail(tr("acos 的参数必须在 [-1, 1] 内"), errorMessage);
                value = std::acos(arguments[0]);
            }
            else if (name == QStringLiteral("atan") && requireCount(1)) value = std::atan(arguments[0]);
            else if (name == QStringLiteral("atan2") && requireCount(2)) value = std::atan2(arguments[0], arguments[1]);
            else if (name == QStringLiteral("exp") && requireCount(1)) value = std::exp(arguments[0]);
            else if ((name == QStringLiteral("ln") || name == QStringLiteral("log")) && requireCount(1))
            {
                if (arguments[0] <= 0.0) return fail(tr("log 的参数必须大于 0"), errorMessage);
                value = std::log(arguments[0]);
            }
            else if (name == QStringLiteral("log10") && requireCount(1))
            {
                if (arguments[0] <= 0.0) return fail(tr("log10 的参数必须大于 0"), errorMessage);
                value = std::log10(arguments[0]);
            }
            else if (name == QStringLiteral("floor") && requireCount(1)) value = std::floor(arguments[0]);
            else if (name == QStringLiteral("ceil") && requireCount(1)) value = std::ceil(arguments[0]);
            else if (name == QStringLiteral("round") && requireCount(1)) value = std::round(arguments[0]);
            else if (name == QStringLiteral("sign") && requireCount(1)) value = arguments[0] > 0.0 ? 1.0 : (arguments[0] < 0.0 ? -1.0 : 0.0);
            else if (name == QStringLiteral("pow") && requireCount(2)) value = std::pow(arguments[0], arguments[1]);
            else if (name == QStringLiteral("deg2rad") && requireCount(1)) value = arguments[0] * 3.14159265358979323846 / 180.0;
            else if (name == QStringLiteral("rad2deg") && requireCount(1)) value = arguments[0] * 180.0 / 3.14159265358979323846;
            else if (name == QStringLiteral("not") && requireCount(1)) value = IsLogicalTrue(arguments[0]) ? 0.0 : 1.0;
            else if (name == QStringLiteral("and") && requireCount(2)) value = IsLogicalTrue(arguments[0]) && IsLogicalTrue(arguments[1]) ? 1.0 : 0.0;
            else if (name == QStringLiteral("or") && requireCount(2)) value = IsLogicalTrue(arguments[0]) || IsLogicalTrue(arguments[1]) ? 1.0 : 0.0;
            else if (name == QStringLiteral("if") && requireCount(3)) value = IsLogicalTrue(arguments[0]) ? arguments[1] : arguments[2];
            else if (name == QStringLiteral("clamp") && requireCount(3))
            {
                if (arguments[1] > arguments[2]) return fail(tr("clamp 的下限不能大于上限"), errorMessage);
                value = (std::max)(arguments[1], (std::min)(arguments[2], arguments[0]));
            }
            else
            {
                // requireCount 已经给出明确错误时直接返回；否则说明函数名不受支持。
                if (!errorMessage.isEmpty())
                    return false;
                return fail(tr("不支持的函数：%1").arg(functionName), errorMessage);
            }

            if (!std::isfinite(value))
                return fail(tr("函数 %1 的结果不是有限数值").arg(functionName), errorMessage);
            return true;
        }

        void skipSpaces()
        {
            while (m_position < m_expression.size() && m_expression.at(m_position).isSpace())
                ++m_position;
        }

        bool match(const QString& token)
        {
            skipSpaces();
            if (m_expression.mid(m_position, token.size()) != token)
                return false;
            m_position += token.size();
            return true;
        }

        bool fail(const QString& message, QString& errorMessage)
        {
            errorMessage = message;
            return false;
        }

        QString m_expression;
        Resolver m_resolver;
        int m_position = 0;
    };

    bool EvaluateParameterDefinitions(
        QVector<MechMainWindow::GeoFeatureParameter>& parameters,
        QString* firstError)
    {
        QHash<QString, int> indexByName;
        QVector<int> states(parameters.size(), 0); // 0 未计算，1 递归栈中，2 已完成。
        bool allValid = true;

        for (int i = 0; i < parameters.size(); ++i)
        {
            auto& parameter = parameters[i];
            parameter.name = parameter.name.trimmed();
            parameter.expression = parameter.expression.trimmed();
            parameter.value = 0.0;
            parameter.valid = false;
            parameter.errorMessage.clear();
            if (!IsValidParameterName(parameter.name))
            {
                parameter.errorMessage = tr("参数名必须以字母、中文或下划线开头，且只能包含字母、数字和下划线");
                allValid = false;
                continue;
            }
            const QString normalizedName = NormalizedParameterName(parameter.name);
            if (normalizedName == QStringLiteral("pi")
                || normalizedName == QStringLiteral("e")
                || normalizedName == QStringLiteral("true")
                || normalizedName == QStringLiteral("false"))
            {
                parameter.errorMessage = tr("参数名与内置常量重名");
                allValid = false;
                continue;
            }
            if (indexByName.contains(normalizedName))
            {
                const int previous = indexByName.value(normalizedName);
                parameter.errorMessage = tr("参数名重复");
                parameters[previous].errorMessage = tr("参数名重复");
                allValid = false;
                continue;
            }
            indexByName.insert(normalizedName, i);
        }

        std::function<bool(int, QString&)> evaluateIndex;
        evaluateIndex = [&](int index, QString& errorMessage) -> bool {
            auto& parameter = parameters[index];
            if (states[index] == 2)
            {
                errorMessage = parameter.errorMessage;
                return parameter.valid;
            }
            if (states[index] == 1)
            {
                errorMessage = tr("检测到包含参数 %1 的循环依赖").arg(parameter.name);
                return false;
            }
            if (!parameter.errorMessage.isEmpty())
            {
                states[index] = 2;
                errorMessage = parameter.errorMessage;
                return false;
            }

            states[index] = 1;
            DirectModelingExpressionParser parser(
                parameter.expression,
                [&](const QString& referencedName, double& value, QString& resolveError) {
                    const QString normalized = NormalizedParameterName(referencedName);
                    if (!indexByName.contains(normalized))
                    {
                        resolveError = tr("未知参数：%1").arg(referencedName);
                        return false;
                    }
                    const int referencedIndex = indexByName.value(normalized);
                    QString dependencyError;
                    if (!evaluateIndex(referencedIndex, dependencyError))
                    {
                        resolveError = tr("参数 %1 无效：%2")
                            .arg(parameters[referencedIndex].name, dependencyError);
                        return false;
                    }
                    value = parameters[referencedIndex].value;
                    return true;
                });

            QString parseError;
            double result = 0.0;
            parameter.valid = parser.parse(result, parseError);
            parameter.value = parameter.valid ? result : 0.0;
            parameter.errorMessage = parameter.valid ? QString() : parseError;
            states[index] = 2;
            errorMessage = parameter.errorMessage;
            return parameter.valid;
            };

        for (int i = 0; i < parameters.size(); ++i)
        {
            QString error;
            if (!evaluateIndex(i, error))
            {
                allValid = false;
                if (firstError && firstError->isEmpty())
                    *firstError = tr("参数 %1：%2").arg(parameters[i].name, error);
            }
        }
        return allValid;
    }
}

namespace MechMainWindow
{
    QVector<GeoFeatureParameter> mGeoFeatureParameterCalculator::parameters() const
    {
        QVector<GeoFeatureParameter> snapshot = m_parameters;
        for (GeoFeatureParameter& parameter : snapshot)
        {
            // 表达式错误优先显示；只有公式本身有效时才叠加几何回放错误。
            const auto geometryError = m_geometryErrors.constFind(parameter.id);
            if (parameter.valid && geometryError != m_geometryErrors.constEnd())
            {
                parameter.valid = false;
                parameter.errorMessage = geometryError.value();
            }
        }
        return snapshot;
    }

    int mGeoFeatureParameterCalculator::removeParameters(
        const QSet<QString>& parameterIds)
    {
        int removedCount = 0;
        for (int index = m_parameters.size() - 1; index >= 0; --index)
        {
            if (!parameterIds.contains(m_parameters.at(index).id))
                continue;
            m_geometryErrors.remove(m_parameters.at(index).id);
            m_parameters.removeAt(index);
            ++removedCount;
        }
        if (removedCount > 0)
        {
            // 删除本身即使令其他公式失效也必须生效。求值器会为直接引用和传递
            // 依赖写入明确错误；其余模型原有的几何校验状态继续保留。
            evaluateDefinitions(m_parameters, nullptr);
        }
        return removedCount;
    }

    void mGeoFeatureParameterCalculator::clearGeometryValidation()
    {
        m_geometryErrors.clear();
    }

    bool mGeoFeatureParameterCalculator::setGeometryInvalid(
        const QString& parameterId,
        const QString& errorMessage)
    {
        for (const GeoFeatureParameter& parameter : m_parameters)
        {
            if (parameter.id == parameterId)
            {
                m_geometryErrors.insert(
                    parameterId,
                    errorMessage.trimmed().isEmpty()
                    ? QObject::tr("对应的几何操作无法重新生成")
                    : errorMessage.trimmed());
                return true;
            }
        }
        return false;
    }

    bool mGeoFeatureParameterCalculator::evaluateDefinitions(
        QVector<GeoFeatureParameter>& parameters,
        QString* firstError)
    {
        if (firstError)
            firstError->clear();
        return EvaluateParameterDefinitions(parameters, firstError);
    }

    QString mGeoFeatureParameterCalculator::createUniqueParameterName(
        const QString& operationName) const
    {
        QString baseName = operationName.trimmed();
        if (baseName.isEmpty())
            baseName = QObject::tr("直接建模");
        for (QChar& ch : baseName)
        {
            if (!(ch.isLetterOrNumber() || ch == QLatin1Char('_')))
                ch = QLatin1Char('_');
        }
        baseName += QObject::tr("参数");

        QString candidate = baseName;
        int suffix = 2;
        auto nameExists = [this](const QString& name) {
            const QString normalized = name.toCaseFolded();
            for (const GeoFeatureParameter& parameter : m_parameters)
            {
                if (parameter.name.toCaseFolded() == normalized)
                    return true;
            }
            return false;
            };
        while (nameExists(candidate))
            candidate = QStringLiteral("%1_%2").arg(baseName).arg(suffix++);
        return candidate;
    }

    bool mGeoFeatureParameterCalculator::upsertOperationParameter(
        const QString& objectKey,
        const QString& operationName,
        const QString& expression,
        double fallbackValue,
        QString* parameterName,
        QString* errorMessage)
    {
        if (parameterName)
            parameterName->clear();
        if (errorMessage)
            errorMessage->clear();
        if (objectKey.trimmed().isEmpty() || operationName.trimmed().isEmpty()
            || !std::isfinite(fallbackValue))
        {
            if (errorMessage)
                *errorMessage = QObject::tr("自动参数的对象键、操作名称或数值无效");
            return false;
        }

        QVector<GeoFeatureParameter> candidates = m_parameters;
        int parameterIndex = -1;
        for (int i = 0; i < candidates.size(); ++i)
        {
            if (candidates.at(i).id == objectKey)
            {
                parameterIndex = i;
                break;
            }
        }
        if (parameterIndex < 0)
        {
            GeoFeatureParameter parameter;
            parameter.id = objectKey;
            parameter.name = createUniqueParameterName(operationName);
            parameter.operationName = operationName.trimmed();
            candidates.push_back(std::move(parameter));
            parameterIndex = candidates.size() - 1;
        }
        else
        {
            candidates[parameterIndex].operationName = operationName.trimmed();
        }

        const QString normalizedExpression = expression.trimmed();
        candidates[parameterIndex].expression = normalizedExpression.isEmpty()
            ? formatParameterValue(fallbackValue)
            : normalizedExpression;

        QString firstError;
        const bool success = evaluateDefinitions(candidates, &firstError);
        m_parameters = std::move(candidates);
        if (parameterName)
            *parameterName = m_parameters.at(parameterIndex).name;
        if (errorMessage)
            *errorMessage = firstError;
        return success;
    }

    bool mGeoFeatureParameterCalculator::updateDefinitions(
        const QHash<QString, QString>& nameById,
        const QHash<QString, QString>& expressionById,
        QString* errorMessage)
    {
        if (errorMessage)
            errorMessage->clear();

        // id 是参数与几何历史之间的稳定身份，始终只用 id 定位
        // 待编辑行。这样名称修改后，Controller 不需要重新绑定参数。
        QHash<QString, int> indexById;
        for (int index = 0; index < m_parameters.size(); ++index)
        {
            const QString& id = m_parameters.at(index).id;
            if (id.isEmpty() || indexById.contains(id))
            {
                if (errorMessage)
                    *errorMessage = QObject::tr("直接建模参数的稳定 ID 无效或重复");
                return false;
            }
            indexById.insert(id, index);
        }

        auto validateRequestedIds = [&](const QHash<QString, QString>& updates) {
            for (auto it = updates.constBegin(); it != updates.constEnd(); ++it)
            {
                if (!indexById.contains(it.key()))
                {
                    if (errorMessage)
                    {
                        *errorMessage = QObject::tr(
                            "试图修改不存在的直接建模参数：%1")
                            .arg(it.key());
                    }
                    return false;
                }
            }
            return true;
            };
        if (!validateRequestedIds(nameById)
            || !validateRequestedIds(expressionById))
        {
            return false;
        }

        QVector<GeoFeatureParameter> candidates = m_parameters;
        for (auto it = nameById.constBegin(); it != nameById.constEnd(); ++it)
            candidates[indexById.value(it.key())].name = it.value().trimmed();

        // 改写公式前先验证“最终名称集合”。特别是 A/B 互换名称
        // 在最终集合中并不重名，不能按逐行修改的中间状态误判。
        QHash<QString, int> ownerByFinalName;
        for (int index = 0; index < candidates.size(); ++index)
        {
            GeoFeatureParameter& parameter = candidates[index];
            parameter.name = parameter.name.trimmed();
            if (!IsValidParameterName(parameter.name))
            {
                if (errorMessage)
                {
                    *errorMessage = QObject::tr(
                        "参数名必须以字母、中文或下划线开头，"
                        "且只能包含字母、数字和下划线：%1")
                        .arg(parameter.name);
                }
                return false;
            }

            const QString normalizedName = NormalizedParameterName(parameter.name);
            if (normalizedName == QStringLiteral("pi")
                || normalizedName == QStringLiteral("e")
                || normalizedName == QStringLiteral("true")
                || normalizedName == QStringLiteral("false"))
            {
                if (errorMessage)
                {
                    *errorMessage = QObject::tr("参数名与内置常量重名：%1")
                        .arg(parameter.name);
                }
                return false;
            }
            if (ownerByFinalName.contains(normalizedName))
            {
                if (errorMessage)
                {
                    *errorMessage = QObject::tr("参数名重复：%1")
                        .arg(parameter.name);
                }
                return false;
            }
            ownerByFinalName.insert(normalizedName, index);
        }

        // 以更新前的名称作为键、最终名称作为值建立一次性
        // 映射。只记录真正变更的名称，避免无改名的公式被无意格式化。
        QHash<QString, QString> finalNameByOldNormalizedName;
        for (int index = 0; index < candidates.size(); ++index)
        {
            const QString oldName = m_parameters.at(index).name.trimmed();
            const QString& finalName = candidates.at(index).name;
            if (oldName != finalName)
            {
                const QString oldNormalizedName = NormalizedParameterName(oldName);
                if (finalNameByOldNormalizedName.contains(oldNormalizedName))
                {
                    if (errorMessage)
                        *errorMessage = QObject::tr("更新前的参数名存在歧义");
                    return false;
                }
                finalNameByOldNormalizedName.insert(oldNormalizedName, finalName);
            }
        }

        // 只有真正改名时才需要结构化改写存量引用。expressionById 中的公式是
        // 用户在本次最终名称空间内显式输入的新内容，不能再按旧名称二次改写；
        // 这也消除了 A/B 同时换名并编辑公式时的引用歧义。
        // 改名情况下，
        // 花括号缺失、嵌套或空引用会使引用边界不可判定，因此整批拒绝。
        // 纯公式更新则交给 evaluateDefinitions 处理，求值失败后照常保存
        // 无效公式，完全保留原 updateExpressions 的可编辑失效状态语义。
        if (!finalNameByOldNormalizedName.isEmpty())
        {
            for (GeoFeatureParameter& parameter : candidates)
            {
                if (expressionById.contains(parameter.id))
                    continue;
                QString rewrittenExpression;
                QString rewriteError;
                if (!RewriteParameterReferences(
                    parameter.expression,
                    finalNameByOldNormalizedName,
                    rewrittenExpression,
                    rewriteError))
                {
                    if (errorMessage)
                    {
                        *errorMessage = QObject::tr("参数 %1：%2")
                            .arg(parameter.name, rewriteError);
                    }
                    return false;
                }
                parameter.expression = rewrittenExpression.trimmed();
            }
        }

        // 新公式最后覆盖候选值，明确规定其引用已经使用最终参数名。公式本身的
        // 求值错误仍由 evaluateDefinitions 保存为 invalid，保持原编辑行为。
        for (auto it = expressionById.constBegin();
            it != expressionById.constEnd(); ++it)
        {
            candidates[indexById.value(it.key())].expression = it.value().trimmed();
        }

        QString firstError;
        const bool success = evaluateDefinitions(candidates, &firstError);
        // 与原 updateExpressions 保持一致：名称/引用结构合法时，
        // 即使未知引用、循环依赖或函数求值失败，也要保存用户输入
        // 并以 valid/errorMessage 显示失效状态。
        m_parameters = std::move(candidates);
        if (errorMessage)
            *errorMessage = firstError;
        return success;
    }

    bool mGeoFeatureParameterCalculator::updateExpressions(
        const QHash<QString, QString>& expressionById,
        QString* errorMessage)
    {
        return updateDefinitions({}, expressionById, errorMessage);
    }

    bool mGeoFeatureParameterCalculator::parameterValue(
        const QString& parameterId,
        double& value,
        QString* parameterName,
        QString* errorMessage) const
    {
        value = 0.0;
        if (parameterName)
            parameterName->clear();
        if (errorMessage)
            errorMessage->clear();

        for (const GeoFeatureParameter& parameter : m_parameters)
        {
            if (parameter.id != parameterId)
                continue;
            if (parameterName)
                *parameterName = parameter.name;
            if (!parameter.valid)
            {
                if (errorMessage)
                {
                    *errorMessage = parameter.errorMessage.isEmpty()
                        ? QObject::tr("参数公式无效")
                        : parameter.errorMessage;
                }
                return false;
            }
            if (!std::isfinite(parameter.value))
            {
                if (errorMessage)
                    *errorMessage = QObject::tr("参数的当前值不是有限数值");
                return false;
            }
            value = parameter.value;
            return true;
        }

        if (errorMessage)
            *errorMessage = QObject::tr("未找到指定的直接建模参数：%1")
            .arg(parameterId);
        return false;
    }

    bool mGeoFeatureParameterCalculator::evaluateExpression(
        const QString& expression,
        double& value,
        QString* errorMessage) const
    {
        QHash<QString, int> indexByName;
        for (int i = 0; i < m_parameters.size(); ++i)
        {
            const GeoFeatureParameter& parameter = m_parameters.at(i);
            if (!parameter.name.trimmed().isEmpty())
                indexByName.insert(NormalizedParameterName(parameter.name), i);
        }

        DirectModelingExpressionParser parser(
            expression,
            [&](const QString& referencedName,
                double& resolvedValue,
                QString& resolveError) {
                    const QString normalized = NormalizedParameterName(referencedName);
                    if (!indexByName.contains(normalized))
                    {
                        resolveError = QObject::tr("未知参数：%1").arg(referencedName);
                        return false;
                    }
                    const GeoFeatureParameter& parameter =
                        m_parameters.at(indexByName.value(normalized));
                    if (!parameter.valid)
                    {
                        resolveError = QObject::tr("参数 %1 无效：%2")
                            .arg(parameter.name, parameter.errorMessage);
                        return false;
                    }
                    resolvedValue = parameter.value;
                    return true;
            });

        QString parseError;
        const bool success = parser.parse(value, parseError);
        if (errorMessage)
            *errorMessage = parseError;
        return success;
    }
}
