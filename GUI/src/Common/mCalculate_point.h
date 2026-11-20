#pragma once
#include "mCommon_global.h"
#pragma execution_character_set("utf-8")
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtCore/QStringList>
#include <QtGui/QVector3D>
#include <functional>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QDebug>
#include <QtGui/QMouseEvent>
#include <QtCore/QRegularExpression>
#include <stack>
#include <cmath>
#include <QtCore/QtMath>
#include "mMechGlobalSignals.h"   // 全局信号类，需确保包含
//#include "pickManager.h"
//注意：在面板中实例化时需要先创建这两个类出来
//class TestPointOwner
//{
//public:
//    QVector3D centerPoint;
//};
//
//class MyObject
//{
//public:
//    MechCommon::DefaultLine axisLine; //存储线
//
//};
//emit mMechGlobalSignals::getInstance()->finishedPick(); 拾取完成后要在面板的槽函数后发射这个全局信号
namespace MechCommon
{
    //四种状态
    enum class Status
    {
        activatedNoObject,
        activatedHasObject,
        noValueInactivated,
        hasValueInactivated
    };

    // 表达式求值（与原始代码相同）
    static double evaluateExpression(const QString& expr, bool* ok)
    {
        *ok = false;
        QString str = expr.trimmed();
        if (str.isEmpty()) return 0.0;
        auto precedence = [](char op) -> int {
            if (op == '+' || op == '-') return 1;
            if (op == '*' || op == '/') return 2;
            return 0;
            };
        auto applyOp = [](double a, double b, char op) -> double {
            switch (op) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return (b != 0.0) ? a / b : 0.0;
            default: return 0.0;
            }
            };
        std::stack<double> values;
        std::stack<char> ops;
        int i = 0, len = str.length();
        while (i < len) {
            QChar ch = str[i];
            if (ch.isSpace()) { ++i; continue; }
            if (ch.isDigit() || ch == '.') {
                int start = i;
                while (i < len && (str[i].isDigit() || str[i] == '.')) ++i;
                double val = str.mid(start, i - start).toDouble();
                values.push(val);
                continue;
            }
            else if (ch == '(') {
                ops.push('(');
                ++i;
            }
            else if (ch == ')') {
                while (!ops.empty() && ops.top() != '(') {
                    double b = values.top(); values.pop();
                    double a = values.top(); values.pop();
                    char op = ops.top(); ops.pop();
                    values.push(applyOp(a, b, op));
                }
                if (!ops.empty() && ops.top() == '(') ops.pop();
                ++i;
            }
            else if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
                while (!ops.empty() && ops.top() != '(' && precedence(ops.top()) >= precedence(ch.toLatin1())) {
                    double b = values.top(); values.pop();
                    double a = values.top(); values.pop();
                    char op = ops.top(); ops.pop();
                    values.push(applyOp(a, b, op));
                }
                ops.push(ch.toLatin1());
                ++i;
            }
            else {
                *ok = false;
                return 0.0;
            }
        }
        while (!ops.empty()) {
            double b = values.top(); values.pop();
            double a = values.top(); values.pop();
            char op = ops.top(); ops.pop();
            values.push(applyOp(a, b, op));
        }
        if (values.size() == 1) {
            *ok = true;
            return values.top();
        }
        else {
            *ok = false;
            return 0.0;
        }
    }

    template<typename ObjectType, typename PropertyType>
    class mCalculatePoint : public QWidget
    {
    public:
        using ValueChangedCallback = std::function<void(const QVector3D&)>;
        using PickFuncCallback = std::function<bool(QVector3D&)>;
        using PickingStateCallback = std::function<void(bool)>;
        using FocusCallback = std::function<void()>;

        mCalculatePoint(const QString& text,
            ObjectType* object,
            PropertyType ObjectType::* member,
            PickFuncCallback pickFunc,
            QWidget* parent = nullptr)
            : QWidget(parent)
            , m_object(object)
            , m_member(member)
            , m_pickFunc(pickFunc)
        {
            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(4);

            // 按钮行
            QHBoxLayout* btnLayout = new QHBoxLayout();
            btnLayout->setContentsMargins(0, 0, 0, 0);
            btnLayout->setSpacing(4);

            m_choiceBtn = new QPushButton("请选择", this);
            m_choiceBtn->setFixedSize(124, 24);
            m_choiceBtn->setStyleSheet(R"(
                QPushButton {
                    text-align: left;
                    padding-left: 8px;
                    border: 1px solid black;
                    border-radius: 5px;
                    background-color: #E6F2FF;
                    color: #0078D7;
                }
            )");

            m_applyBtn = new QPushButton("应用", this);
            m_applyBtn->setFixedSize(40, 24);
            m_applyBtn->setStyleSheet(R"(
                QPushButton {
                    border: 1px solid #ccc;
                    border-radius: 5px;
                    background-color: #f5f5f5;
                    color: #000000;
                }
            )");

            btnLayout->addWidget(m_choiceBtn);
            btnLayout->addWidget(m_applyBtn);
            mainLayout->addLayout(btnLayout);

            // 编辑框
            m_positionEdit = new QLineEdit(this);
            m_positionEdit->setFixedSize(168, 24);
            m_positionEdit->setPlaceholderText("0,0,0");
            m_positionEdit->setStyleSheet(R"(
                QLineEdit {
                    border: 1px solid #ccc;
                    border-radius: 5px;
                    background-color: #ffffff;
                    padding: 2px 8px;
                }
            )");
            mainLayout->addWidget(m_positionEdit);

            setFixedWidth(168);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            connect(m_applyBtn, &QPushButton::clicked, this, &mCalculatePoint::onApplyClicked);
            connect(m_positionEdit, &QLineEdit::editingFinished, this, &mCalculatePoint::onEditingFinished);

            // 自动拾取信号
            if (auto* sig = mMechGlobalSignals::getInstance()) {
                //connect(sig, &mMechGlobalSignals::finishedPick,
                //    this, &mCalculatePoint::onPickFinished);
            }

            updateDisplay();
        }

        ~mCalculatePoint() override {}

        void setValueChangedCallback(ValueChangedCallback cb) { m_valueChangedCallback = cb; }
        void setPickingStateCallback(PickingStateCallback cb) { m_pickingStateCallback = cb; }
        void setFocusCallback(FocusCallback cb) { m_focusCallback = cb; }
        QVector3D getPoint() const { return m_currentPoint; }
        void refreshFromObject() { updateDisplay(); }
        int pickCount() const { return m_pickCount; }
        QPushButton* choiceButton() const { return m_choiceBtn; }
        //清空数据
        void clearPickedPoints()
        {
            m_pointList.clear();
            m_pickCount = 0;
            changeStatus(Status::activatedNoObject);
            m_positionEdit->setText("0,0,0");
            if (m_pickingStateCallback) m_pickingStateCallback(false);
        }
        //注入数据
        void injectPickedPoint(const QVector3D& point)
        {
            m_pointList.append(point);
            m_pickCount = m_pointList.size();
            changeStatus(Status::activatedHasObject);
            m_positionEdit->setText("0,0,0");
        }
        //切换状态
        void setActive(bool active)
        {
            m_isActive = active;
            if (active) {
                if (m_pointList.isEmpty())
                    changeStatus(Status::activatedNoObject);
                else
                    changeStatus(Status::activatedHasObject);
                if (m_pickingStateCallback) m_pickingStateCallback(true);
            }
            else {
                m_applyBtn->setEnabled(false);
                m_choiceBtn->setText("点击去选择");
                m_choiceBtn->setStyleSheet(R"(
                    QPushButton {
                        border: 1px solid #ccc;
                        border-radius: 3px;
                        background-color: #ffffff;
                        padding: 2px 8px;
                        color: #409EFF;
                    }
                )");
                m_positionEdit->setEnabled(true);
                if (m_pickingStateCallback) m_pickingStateCallback(false);
            }
        }

    public slots:
        //应用按钮槽函数
        void onApplyClicked()
        {
            if (!m_object || !m_member) return;
            if (m_pointList.isEmpty()) return;

            QVector3D sum(0, 0, 0);
            for (const auto& p : m_pointList) sum += p;
            QVector3D resultPoint = sum / m_pointList.size();
            setMemberValue(resultPoint);
            m_currentPoint = resultPoint;

            bool isZero = qFuzzyIsNull(resultPoint.x()) && qFuzzyIsNull(resultPoint.y()) && qFuzzyIsNull(resultPoint.z());
            if (isZero) m_positionEdit->setText("0,0,0");
            else m_positionEdit->setText(QString("%1, %2, %3")
                .arg(resultPoint.x(), 0, 'f', 5)
                .arg(resultPoint.y(), 0, 'f', 5)
                .arg(resultPoint.z(), 0, 'f', 5));

            if (m_valueChangedCallback) m_valueChangedCallback(resultPoint);
            m_pointList.clear();
            m_pickCount = 0;
            changeStatus(Status::hasValueInactivated);
        }
        //编辑栏槽函数
        void onEditingFinished()
        {
            if (!m_object || !m_member) return;
            if (!m_positionEdit->isEnabled()) return;
            QString text = m_positionEdit->text().trimmed();
            if (text.isEmpty()) {
                updateDisplay();
                return;
            }
            QStringList parts = text.split(',', QString::SkipEmptyParts);
            if (parts.size() == 3) {
                double x = 0, y = 0, z = 0;
                bool ok1 = false, ok2 = false, ok3 = false;
                if (!parts[0].trimmed().isEmpty()) x = evaluateExpression(parts[0].trimmed(), &ok1);
                if (!parts[1].trimmed().isEmpty()) y = evaluateExpression(parts[1].trimmed(), &ok2);
                if (!parts[2].trimmed().isEmpty()) z = evaluateExpression(parts[2].trimmed(), &ok3);
                if (ok1 && ok2 && ok3) {
                    QVector3D newPoint(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    setMemberValue(newPoint);
                    m_currentPoint = newPoint;
                    bool isZero = qFuzzyIsNull(newPoint.x()) && qFuzzyIsNull(newPoint.y()) && qFuzzyIsNull(newPoint.z());
                    if (isZero) m_positionEdit->setText("0,0,0");
                    else m_positionEdit->setText(QString("%1, %2, %3")
                        .arg(newPoint.x(), 0, 'f', 5)
                        .arg(newPoint.y(), 0, 'f', 5)
                        .arg(newPoint.z(), 0, 'f', 5));
                    if (m_valueChangedCallback) m_valueChangedCallback(newPoint);
                    m_pointList.clear();
                    m_pickCount = 0;
                    changeStatus(Status::activatedNoObject);
                    return;
                }
            }
            updateDisplay();
        }

    private slots:
        //拾取完成槽函数
        void onPickFinished()
        {
            if (!m_autoPickEnabled) return;
            if (!m_isActive) return;
            std::vector<QVector3D> allPoints;// = PickManager::getSelectedPoints();
            if (allPoints.empty()) {
                clearPickedPoints();
                return;
            }
            m_pointList.clear();
            for (const auto& p : allPoints) m_pointList.append(p);
            m_pickCount = m_pointList.size();
            changeStatus(Status::activatedHasObject);
            m_positionEdit->setText("0,0,0");
            if (m_pickingStateCallback) m_pickingStateCallback(false);
        }

    private:
        //展示数据
        void updateDisplay()
        {
            if (m_object && m_member) {
                QVector3D currentPoint = m_object->*m_member;
                m_currentPoint = currentPoint;
                bool isZero = qFuzzyIsNull(currentPoint.x()) && qFuzzyIsNull(currentPoint.y()) && qFuzzyIsNull(currentPoint.z());
                if (isZero) m_positionEdit->setText("0,0,0");
                else m_positionEdit->setText(QString("%1, %2, %3")
                    .arg(currentPoint.x(), 0, 'f', 5)
                    .arg(currentPoint.y(), 0, 'f', 5)
                    .arg(currentPoint.z(), 0, 'f', 5));
                changeStatus(Status::activatedNoObject);
            }
            else {
                m_positionEdit->setText("0,0,0");
                changeStatus(Status::activatedNoObject);
            }
        }
        //确保有点对象
        void setMemberValue(const QVector3D& point)
        {
            m_object->*m_member = static_cast<PropertyType>(point);
        }
        //四种状态切换
        void changeStatus(Status status)
        {
            if (!m_isActive && status != Status::hasValueInactivated) return;
            m_cuState = status;
            switch (status) {
            case Status::activatedNoObject:
                m_applyBtn->setEnabled(false);
                m_choiceBtn->setText("请选择");
                m_choiceBtn->setStyleSheet(R"(
                    QPushButton {
                        border: 1px solid black;
                        border-radius: 5px;
                        background-color: #E6F2FF;
                        padding: 2px 8px;
                        text-align: left;
                        color: #0078D7;
                    }
                )");
                m_positionEdit->setEnabled(true);
                break;
            case Status::activatedHasObject:
                m_applyBtn->setEnabled(!m_pointList.isEmpty());
                m_choiceBtn->setText(QString("已选%1个").arg(m_pickCount));
                m_choiceBtn->setStyleSheet(R"(
                    QPushButton {
                        border: 1px solid black;
                        border-radius: 3px;
                        background-color: #eeeeee;
                        padding: 2px 8px;
                        text-align: left;
                        color: #000000;
                    }
                )");
                m_positionEdit->setEnabled(false);
                break;
            case Status::hasValueInactivated:
                m_applyBtn->setEnabled(false);
                m_choiceBtn->setText(QString("已选%1个").arg(m_pickCount));
                m_choiceBtn->setStyleSheet(R"(
                    QPushButton {
                        border: 1px solid #f8a8a8;
                        border-radius: 3px;
                        background-color: #ffd6d6;
                        padding: 2px 8px;
                        text-align: left;
                        color: #000000;
                    }
                )");
                m_positionEdit->setEnabled(false);
                break;
            default:
                break;
            }
        }

    private:
        ObjectType* m_object;
        PropertyType ObjectType::* m_member = nullptr;
        PickFuncCallback m_pickFunc;
        ValueChangedCallback m_valueChangedCallback{ nullptr };
        PickingStateCallback m_pickingStateCallback{ nullptr };
        FocusCallback m_focusCallback{ nullptr };

        QPushButton* m_choiceBtn = nullptr;
        QPushButton* m_applyBtn = nullptr;
        QLineEdit* m_positionEdit = nullptr;
        QVector3D m_currentPoint;
        QList<QVector3D> m_pointList;
        int m_pickCount = 0;
        Status m_cuState;
        bool m_autoPickEnabled = true;
        bool m_isActive = true;   // 是否处于激活状态（响应拾取）
    };
} // namespace MechCommon