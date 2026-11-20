#pragma once
#pragma execution_character_set("utf-8")
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtGui/QVector3D>
#include <functional>
#include <QList>
#include <QDebug>
#include "mMechGlobalSignals.h"
#include "default.h"

namespace MechCommon
{
    enum class LineStatus
    {
        ActivatedNoObject,
        ActivatedHasObject,
        HasValueInactivated
    };

    template<typename ObjectType, typename LineType = DefaultLine>
    class mCalculateLine : public QWidget
    {
    public:
        using ValueChangedCallback = std::function<void(const LineType&)>;
        using PickFuncCallback = std::function<bool(QVector3D&)>;
        using PickingStateCallback = std::function<void(bool)>;

        mCalculateLine(const QString& text,
            ObjectType* object,
            LineType ObjectType::* member,
            PickFuncCallback pickFunc,
            QWidget* parent = nullptr)
            : QWidget(parent)
            , m_object(object)
            , m_member(member)
            , m_pickFunc(pickFunc)
        {
            //布局
            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(4);

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

            m_contentEdit = new QLineEdit(this);
            m_contentEdit->setReadOnly(true);
            m_contentEdit->setFixedSize(168, 24);
            m_contentEdit->setPlaceholderText("暂无生成内容");
            m_contentEdit->setStyleSheet(R"(
                QLineEdit {
                    border: 1px solid #ccc;
                    border-radius: 5px;
                    background-color: #ffffff;
                    padding: 2px 8px;
                }
            )");
            mainLayout->addWidget(m_contentEdit);

            setFixedWidth(168);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            connect(m_applyBtn, &QPushButton::clicked, this, &mCalculateLine::onApplyClicked);

            if (auto* sig = mMechGlobalSignals::getInstance()) {
                //connect(sig, &mMechGlobalSignals::finishedPick,
                //    this, &mCalculateLine::onPickFinished);
            }

            updateDisplay();
        }

        ~mCalculateLine() override {}

        void setValueChangedCallback(ValueChangedCallback cb) { m_valueChangedCallback = cb; }
        void setPickingStateCallback(PickingStateCallback cb) { m_pickingStateCallback = cb; }
        void refreshFromObject() { updateDisplay(); }
        int pickCount() const { return m_pickCount; }
        QPushButton* choiceButton() const { return m_choiceBtn; }

        void setTempLineData(const LineType& line) {
            m_tempLine = line;
            m_hasTempLine = true;
        }
        //清除数据
        void clearPickedPoints()
        {
            m_points.clear();
            m_pickCount = 0;
            m_hasTempLine = false;
            changeStatus(LineStatus::ActivatedNoObject);
            m_contentEdit->setText("暂无生成内容");
        }
        //确保返回对象
        void setLineData(const LineType& line)
        {
            if (m_object && m_member) m_object->*m_member = line;
            refreshFromObject();
        }
        //注入点
        void injectPickedPoint(const QVector3D& point)
        {
            if (m_points.size() >= 2) {
                qDebug() << "Already have 2 points, ignoring new one";
                return;
            }
            m_points.append(point);
            m_pickCount = m_points.size();
            changeStatus(LineStatus::ActivatedHasObject);
            m_contentEdit->setText(QString("已选%1个").arg(m_pickCount));
            if (m_pickCount == 2) {
                if constexpr (std::is_same<LineType, DefaultLine>::value) {
                    m_tempLine.start = m_points[0];
                    m_tempLine.end = m_points[1];
                }
                m_hasTempLine = true;
            }
        }
        //切换状态
        void setActive(bool active)
        {
            m_isActive = active;
            if (active) {
                if (m_pickCount == 0 && !m_hasTempLine)
                    changeStatus(LineStatus::ActivatedNoObject);
                else
                    changeStatus(LineStatus::ActivatedHasObject);
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
                m_contentEdit->setEnabled(true);
                if (m_pickingStateCallback) m_pickingStateCallback(false);
            }
        }

    public slots:
        //应用按钮槽函数
        void onApplyClicked()
        {
            if (!m_object || !m_member) return;
            LineType line;
            if (m_points.size() == 2) {
                if constexpr (std::is_same<LineType, DefaultLine>::value) {
                    line.start = m_points[0];
                    line.end = m_points[1];
                }
            }
            else if (m_hasTempLine) {
                line = m_tempLine;
                m_hasTempLine = false;
            }
            else {
                return;
            }
            if constexpr (std::is_copy_assignable<LineType>::value) {
                m_object->*m_member = line;
            }
            m_contentEdit->setText("已生成线");
            if (m_valueChangedCallback) m_valueChangedCallback(line);
            m_points.clear();
            m_pickCount = 0;
            changeStatus(LineStatus::HasValueInactivated);
        }

    private slots:
        //拾取完成的槽函数
        void onPickFinished()
        {
            if (!m_autoPickEnabled) return;
            if (!m_isActive) return;
            std::vector<QVector3D> allPoints;// = PickManager::getSelectedPoints();
            int pointCount = static_cast<int>(allPoints.size());
            if (pointCount >= 2) {
                QVector3D start = allPoints[pointCount - 2];
                QVector3D end = allPoints[pointCount - 1];
                if constexpr (std::is_same<LineType, DefaultLine>::value) {
                    m_tempLine.start = start;
                    m_tempLine.end = end;
                }
                m_hasTempLine = true;
                m_pickCount = 2;
                changeStatus(LineStatus::ActivatedHasObject);
                m_contentEdit->setText(QString("已选%1个").arg(m_pickCount));
            }
            else if (pointCount == 1) {
                m_pickCount = 1;
                changeStatus(LineStatus::ActivatedHasObject);
                m_contentEdit->setText(QString("已选%1个").arg(m_pickCount));
                m_hasTempLine = false;
            }
            else {
                clearPickedPoints();
            }
            if (m_pickingStateCallback) m_pickingStateCallback(false);
        }

    private:
        //展示界面
        void updateDisplay()
        {
            if (m_object && m_member) {
                LineType line = m_object->*m_member;
                if constexpr (std::is_same<LineType, DefaultLine>::value) {
                    if (line.isValid()) {
                        m_contentEdit->setText("已生成线");
                        m_contentEdit->setStyleSheet("background-color: #e6f2ff; border: 1px solid #409EFF;");
                        changeStatus(LineStatus::HasValueInactivated);
                        return;
                    }
                }
            }
            m_contentEdit->setText("暂无生成内容");
            m_contentEdit->setStyleSheet("background-color: #ffffff; border: 1px solid #ccc;");
            changeStatus(LineStatus::ActivatedNoObject);
        }
        //状态切换
        void changeStatus(LineStatus status)
        {
            if (!m_isActive && status != LineStatus::HasValueInactivated) return;
            m_status = status;
            switch (status) {
            case LineStatus::ActivatedNoObject:
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
                break;
            case LineStatus::ActivatedHasObject:
                m_applyBtn->setEnabled(m_points.size() == 2 || m_hasTempLine);
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
                break;
            case LineStatus::HasValueInactivated:
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
                break;
            }
        }

    private:
        ObjectType* m_object = nullptr;
        LineType ObjectType::* m_member = nullptr;
        PickFuncCallback m_pickFunc;
        ValueChangedCallback m_valueChangedCallback{ nullptr };
        PickingStateCallback m_pickingStateCallback = nullptr;

        QPushButton* m_choiceBtn = nullptr;
        QPushButton* m_applyBtn = nullptr;
        QLineEdit* m_contentEdit = nullptr;
        QList<QVector3D> m_points;
        int m_pickCount = 0;
        LineStatus m_status = LineStatus::ActivatedNoObject;
        LineType m_tempLine;
        bool m_hasTempLine = false;
        bool m_autoPickEnabled = true;
        bool m_isActive = true;   // 是否激活（响应拾取）
    };
} // namespace MechCommon