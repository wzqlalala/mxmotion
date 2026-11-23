#pragma once

#include <QLineEdit>
#include <QLabel>
#include <QResizeEvent>
#include <QValidator>
#include <cmath>
#include <limits>

namespace MxFunctions {

class mCalcLineEdit : public QLineEdit
{
public:
    enum class CalcMode { Standard, NonNegative, NoEmptyAndPositive };
    explicit mCalcLineEdit(QWidget* parent = nullptr) : mCalcLineEdit(CalcMode::Standard, parent) {}
    explicit mCalcLineEdit(CalcMode mode, QWidget* parent = nullptr)
        : QLineEdit(parent), m_mode(mode), m_unit(new QLabel(this))
    {
        m_unit->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_unit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_unit->hide();
        class ExpressionValidator final : public QValidator {
        public:
            explicit ExpressionValidator(mCalcLineEdit* owner) : QValidator(owner), m_owner(owner) {}
            State validate(QString& input, int&) const override
            {
                double value = 0;
                return m_owner->evaluate(input, value) ? Acceptable : Intermediate;
            }
        private:
            mCalcLineEdit* m_owner;
        };
        setValidator(new ExpressionValidator(this));
        // This connection is registered before the original panel's listener:
        // its text().toDouble() sees the evaluated value, never an expression.
        connect(this, &QLineEdit::editingFinished, this, [this] {
            double value = 0;
            if (evaluate(QLineEdit::text(), value)) {
                m_expression = QLineEdit::text();
                setText(QString::number(value, 'g', 16));
            }
        });
    }
    mCalcLineEdit(const QString& expression, CalcMode mode = CalcMode::Standard, QWidget* parent = nullptr)
        : mCalcLineEdit(mode, parent) { setExpression(expression); }
    mCalcLineEdit(const QString& expression, QWidget* parent)
        : mCalcLineEdit(expression, CalcMode::Standard, parent) {}
    void setExpression(const QString& expression)
    {
        m_expression = expression;
        double value = 0;
        setText(evaluate(expression, value) ? QString::number(value, 'g', 16) : expression);
    }
    QString expression() const { return m_expression; }
    QString getExpression() const { return m_expression; }
    void setUnit(const QString& text) { m_unit->setText(text); m_unit->setVisible(!text.isEmpty()); layoutUnit(); }
    void setRange(double minimum, double maximum) { m_minimum = minimum; m_maximum = maximum; }
    void setMinimum(double value) { m_minimum = value; }
    void setMaximum(double value) { m_maximum = value; }
    void setNonNegative(bool enabled) { m_mode = enabled ? CalcMode::NonNegative : CalcMode::Standard; }
    double value() const { double result = 0; return evaluate(QLineEdit::text(), result) ? result : 0; }
    void setValue(double value) { setExpression(QString::number(value, 'g', 16)); }
    bool isValid() const { double value = 0; return evaluate(QLineEdit::text(), value); }
    static bool calculate(const QString& input, double& result)
    {
        struct Parser {
            const QString& text; int at = 0; int depth = 0; bool ok = true;
            void spaces() { while (at < text.size() && text[at].isSpace()) ++at; }
            bool take(QChar c) { spaces(); if (at < text.size() && text[at] == c) { ++at; return true; } return false; }
            double sum() { double a = product(); for (;;) { if (take('+')) a += product(); else if (take('-')) a -= product(); else return a; } }
            double product() { double a = unary(); for (;;) { if (take('*')) a *= unary(); else if (take('/')) { const double b = unary(); if (b == 0) ok = false; a /= b; } else return a; } }
            double unary() { if (++depth > 64) { ok = false; --depth; return 0; } double a; if (take('+')) a = unary(); else if (take('-')) a = -unary(); else { a = atom(); if (take('^')) a = std::pow(a, unary()); } --depth; return a; }
            double atom()
            {
                if (take('(')) { const double a = sum(); if (!take(')')) ok = false; return a; }
                spaces(); const int start = at;
                if (at < text.size() && text[at].isLetter()) {
                    while (at < text.size() && text[at].isLetterOrNumber()) ++at;
                    const QString name = text.mid(start, at - start).toLower();
                    if (name == "pi") return std::acos(-1.0);
                    if (name == "e") return std::exp(1.0);
                    if (!take('(')) { ok = false; return 0; }
                    const double a = sum(); if (!take(')')) ok = false;
                    if (name == "sqrt") return std::sqrt(a);
                    if (name == "sin") return std::sin(a);
                    if (name == "cos") return std::cos(a);
                    if (name == "tan") return std::tan(a);
                    if (name == "abs") return std::abs(a);
                    if (name == "exp") return std::exp(a);
                    if (name == "log" || name == "ln") return std::log(a);
                    if (name == "log10") return std::log10(a);
                    ok = false; return 0;
                }
                while (at < text.size() && (text[at].isDigit() || text[at] == '.')) ++at;
                if (at < text.size() && (text[at] == 'e' || text[at] == 'E')) {
                    ++at; if (at < text.size() && (text[at] == '+' || text[at] == '-')) ++at;
                    while (at < text.size() && text[at].isDigit()) ++at;
                }
                bool numeric = false; const double a = text.mid(start, at - start).toDouble(&numeric);
                ok = ok && numeric; return a;
            }
        };
        if (input.size() > 2048 || input.trimmed().isEmpty()) return false;
        Parser parser{input}; result = parser.sum(); parser.spaces();
        return parser.ok && parser.at == input.size() && std::isfinite(result);
    }
protected:
    void resizeEvent(QResizeEvent* event) override { QLineEdit::resizeEvent(event); layoutUnit(); }
private:
    bool evaluate(const QString& input, double& result) const
    {
        if (!calculate(input, result)) return false;
        if (m_mode == CalcMode::NonNegative && result < 0) return false;
        if (m_mode == CalcMode::NoEmptyAndPositive && result <= 0) return false;
        return result >= m_minimum && result <= m_maximum;
    }
    void layoutUnit()
    {
        const int unitWidth = m_unit->text().isEmpty() ? 0 : fontMetrics().horizontalAdvance(m_unit->text()) + 8;
        setTextMargins(0, 0, unitWidth, 0);
        m_unit->setGeometry(width() - unitWidth - 4, 1, unitWidth, height() - 2);
    }
    CalcMode m_mode;
    QLabel* m_unit;
    QString m_expression;
    double m_minimum = -std::numeric_limits<double>::infinity();
    double m_maximum = std::numeric_limits<double>::infinity();
};

} // namespace MxFunctions
