#pragma once
#include "mCommon_global.h"
#pragma execution_character_set("utf-8")
//QT
#include <QStringList>
#include <QObject>
#include <QToolButton>
#include <QVector3D>
#include <QLabel>
#include "mOtherFunction.h"

namespace MechCommon
{
    /**
    * @brief 拾取点部件模板类
    * @tparam ObjectType 目标对象类型
    * @tparam PropertyType 目标属性类型
    */
    template<typename ObjectType, typename PropertyType>
    class mPickPointWidget : public QWidget
    {
    public:
        using ValueChangedCallback = std::function<void(const QVector3D&)>;
        using PickFuncCallback = std::function<bool(QVector3D&)>;
        /**
        * @brief 构造函数
        * @param text 显示的标签文本
        * @param object 绑定的目标对象指针
        * @param member 绑定的目标对象成员变量指针
        * @param pickFunc 拾取点数据的回调函数 (解耦外部依赖)
        * @param parent 父部件指针
        */
        mPickPointWidget(const QString& text,
            ObjectType* object,
            PropertyType ObjectType::* member,
            PickFuncCallback pickFunc,
            QWidget* parent = nullptr)
            : QWidget(parent)
            , m_object(object) // 修复：初始化目标对象
            , m_member(member) // 保存成员变量指针
            , m_pickFunc(pickFunc) // 保存回调函数
        {
            // 布局设计
            QHBoxLayout* layout = new QHBoxLayout(this);

            layout->setContentsMargins(0, 0, 0, 0);

            QLabel* label = new QLabel(text, this);
            label->setFixedWidth(90);
            layout->addWidget(label);

            // 颜色色块 Label (初始状态隐藏，直到调用 setMarkColor)
            m_colorLabel = new QLabel(this);
            m_colorLabel->setFixedSize(12, 12); // 色块大小 12x12
            m_colorLabel->setVisible(false);    // 默认不显示，保证通用性
            layout->addWidget(m_colorLabel);

            m_lineEdit = new QLineEdit(this);
            layout->addWidget(m_lineEdit);
            m_okbtn = new QToolButton(this);
            m_okbtn->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
            layout->addWidget(m_okbtn);
            m_clearbtn = new QToolButton(this);
            m_clearbtn->setIcon(QIcon(":/mMainWidgets/Icon/mSectionWindow/shanchu3x.png"));
            layout->addWidget(m_clearbtn);

            // 绑定获取按钮槽函数
            connect(m_okbtn, &QToolButton::clicked, this, [this]() {
                QVector3D point;
                // 使用回调函数获取数据
                if (m_pickFunc && m_pickFunc(point))
                {
                    if (m_object && m_member)
                    {
                        updateDataAndNotify(point);
                    }
                }
                });

            // 绑定清除按钮槽函数
            connect(m_clearbtn, &QToolButton::clicked, this, [this]() {
                if (m_object && m_member)
                {
                    m_object->*m_member = m_clearDefaultValue;
                    m_lineEdit->setText(QString("%1, %2, %3")
                        .arg(m_clearDefaultValue.x())
                        .arg(m_clearDefaultValue.y())
                        .arg(m_clearDefaultValue.z()));
                    if (m_object && m_member)
                    {
                        updateDataAndNotify(m_clearDefaultValue);
                    }
                }
                });

            // 绑定手动输入解析逻辑
            connect(m_lineEdit, &QLineEdit::editingFinished, this, [this]() {
                // 安全检查：确保对象和成员指针有效
                if (!m_object || !m_member)
                {
                    return;
                }

                QString text = m_lineEdit->text().trimmed();

                // 按逗号分割字符串，支持用户输入如 "1.2, 3.4, 5.6" 这样的格式
                QStringList parts = text.split(',', QString::SkipEmptyParts);

                if (parts.size() == 3)
                {
                    bool isXValid = false;
                    bool isYValid = false;
                    bool isZValid = false;

                    // 解析为浮点数，同时过滤掉可能的空格
                    float x = parts[0].trimmed().toFloat(&isXValid);
                    float y = parts[1].trimmed().toFloat(&isYValid);
                    float z = parts[2].trimmed().toFloat(&isZValid);

                    // 如果三个坐标全部解析成功
                    if (isXValid && isYValid && isZValid)
                    {
                        updateDataAndNotify(QVector3D(x, y, z));
                        return;
                    }
                }

                updateDisplay();
                });

            updateDisplay();
        }

        /**
        * @brief 设置绑定的目标对象
        * @param object 目标对象指针
        */
        void setObject(ObjectType* object)
        {
            m_object = object;
            updateDisplay();
            m_lineEdit->editingFinished();
        }

        /**
         * @brief 设置组件前面的颜色标识符
         * @param color 标识颜色
         */
        void setMarkColor(const QColor& color)
        {
            if (m_colorLabel)
            {
                // 使用 QSS 设置纯色背景，并加一个浅色边框让它更立体
                QString qss = QString("background-color: %1; border: 1px solid #888888; border-radius: 2px;")
                    .arg(color.name());
                m_colorLabel->setStyleSheet(qss);
                m_colorLabel->setVisible(true); // 设置颜色后将其显示出来
            }
        }

        /**
        * @brief 设置值改变时的回调函数
        */
       void setValueChangedCallback(ValueChangedCallback callback)
       {
           m_valueChangedCallback = callback;
       }

       /**
        * @brief 设置清除按钮点击时的默认值
        * @param defaultValue 清除按钮点击后重置的值，默认(0,0,0)
        */
       void setClearDefaultValue(const QVector3D& defaultValue)
       {
           m_clearDefaultValue = defaultValue;
       }

        /**
        * @brief 强制根据底层数据刷新 UI 显示
        */
        void updateDisplay()
        {
            if (m_object && m_member)
            {
                QVector3D currentPoint = m_object->*m_member;
                m_lineEdit->setText(QString("%1, %2, %3")
                    .arg(currentPoint.x())
                    .arg(currentPoint.y())
                    .arg(currentPoint.z()));
            }
            else
            {
                m_lineEdit->setText(QStringLiteral("未关联数据"));
            }
        }

        /**
        * @brief 析构函数
        */
        ~mPickPointWidget() override = default;

    private:

        /**
        * @brief 统一的数据更新与通知入口
        */
        void updateDataAndNotify(const QVector3D& point)
        {
            if (m_object && m_member)
            {
                // 更新底层数据
                m_object->*m_member = point;
                // 更新当前 UI
                m_lineEdit->setText(QString("%1, %2, %3").arg(point.x()).arg(point.y()).arg(point.z()));
                // 触发回调函数，通知外部
                if (m_valueChangedCallback)
                {
                    m_valueChangedCallback(point);
                }
            }
        }
    private:
        ObjectType* m_object; ///< 绑定的目标对象指针
        PropertyType ObjectType::* m_member; ///< 绑定的目标对象成员指针
        PickFuncCallback m_pickFunc; ///< 获取拾取数据的回调函数
        ValueChangedCallback m_valueChangedCallback{ nullptr };
        QVector3D m_clearDefaultValue{ 0, 0, 0 }; ///< 清除按钮默认值

        QLabel* m_colorLabel{ nullptr };    ///< 颜色提示色块
        QLineEdit* m_lineEdit{ nullptr }; ///< 坐标显示文本框
        QToolButton* m_okbtn{ nullptr }; ///< 拾取确认按钮
        QToolButton* m_clearbtn{ nullptr }; ///< 清除数据按钮
    };
    /**
     * @brief 紧凑型多实体ID拾取通用组件模板类 (单行文本框显示)
     * @tparam ObjectType 目标数据对象类型
     * @tparam IdType ID的类型 (默认为 int)
     */
    template<typename ObjectType, typename IdType = int>
    class mPickMultiIdWidget : public QWidget
    {
    public:
        using ContainerType = QVector<IdType>;
        using PickFuncCallback = std::function<bool(ContainerType&)>;
        using ValueChangedCallback = std::function<void(const ContainerType&)>;

        mPickMultiIdWidget(const QString& text,
            ObjectType* object,
            ContainerType ObjectType::* member,
            PickFuncCallback pickFunc,
            QWidget* parent = nullptr)
            : QWidget(parent)
            , m_object(object)
            , m_member(member)
            , m_pickFunc(pickFunc)
        {
            QHBoxLayout* layout = new QHBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);

            // 1. 文本标签
            QLabel* label = new QLabel(text, this);
            label->setFixedWidth(90);
            layout->addWidget(label);

            //2. 颜色色块
            m_colorLabel = new QLabel(this);
            m_colorLabel->setFixedSize(12, 12);
            m_colorLabel->setVisible(false);
            layout->addWidget(m_colorLabel);

            // 3. 多 ID 输入框 (用逗号分隔)
            m_lineEdit = new QLineEdit(this);
            m_lineEdit->setPlaceholderText("当前未选择");
            layout->addWidget(m_lineEdit);

            // 4. 拾取追加按钮
            m_okbtn = new QToolButton(this);
            m_okbtn->setToolTip("拾取并追加 ID");
            m_okbtn->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
            layout->addWidget(m_okbtn);

            // 5. 清除全部按钮
            m_clearbtn = new QToolButton(this);
            m_clearbtn->setToolTip("清空所有 ID");
            m_clearbtn->setIcon(QIcon(":/mMainWidgets/Icon/mSectionWindow/shanchu3x.png"));
            layout->addWidget(m_clearbtn);

            // --- 信号槽绑定逻辑 ---
            // 拾取动作 (追加不重复的 ID)
            connect(m_okbtn, &QToolButton::clicked, this, [this]() {
                ContainerType pickedIds;
                if (m_pickFunc && m_pickFunc(pickedIds))
                {
                    if (m_object && m_member)
                    {
                        m_object->*m_member = pickedIds;
                        updateDisplayAndNotify();
                    }
                }
                });

            // 清除动作
            connect(m_clearbtn, &QToolButton::clicked, this, [this]() {
                if (m_object && m_member)
                {
                    (m_object->*m_member).clear();
                    updateDisplayAndNotify();
                }
                });

            // 手动编辑动作 (支持用户直接在文本框里输入 "12, 15, 18" 并按回车)
            connect(m_lineEdit, &QLineEdit::editingFinished, this, [this]() {
                if (!m_object || !m_member) return;

                QString text = m_lineEdit->text().trimmed();
                if (text.isEmpty() || text == "当前未选择")
                {
                    (m_object->*m_member).clear();
                    updateDisplayAndNotify();
                    return;
                }

                // 按逗号分割并解析
                QStringList parts = text.split(QRegExp("[,，]"), QString::SkipEmptyParts); // 兼容中英文逗号
                ContainerType newContainer;
                for (const QString& part : parts)
                {
                    bool isValid = false;
                    // 这里默认 IdType 是 int，如果用其他类型需要相应修改
                    int id = part.trimmed().toInt(&isValid);
                    // 去重并校验有效性
                    if (isValid && !newContainer.contains(id))
                    {
                        newContainer.append(id);
                    }
                }

                // 更新底层数据
                m_object->*m_member = newContainer;
                updateDisplayAndNotify();
                });

            updateDisplay();
        }

        void setMarkColor(const QColor& color)
        {
            if (m_colorLabel)
            {
                QString qss = QString("background-color: %1; border: 1px solid #888888; border-radius: 2px;").arg(color.name());
                m_colorLabel->setStyleSheet(qss);
                m_colorLabel->setVisible(true);
            }
        }

        void setObject(ObjectType* object)
        {
            m_object = object;
            updateDisplay();
            m_lineEdit->editingFinished();
        }

        void setValueChangedCallback(ValueChangedCallback callback)
        {
            m_valueChangedCallback = callback;
        }

        /**
        * @brief 根据底层数据刷新文本框显示
        */
        void updateDisplay()
        {
            if (m_object && m_member)
            {
                const auto& container = m_object->*m_member;
                if (container.isEmpty())
                {
                    m_lineEdit->setText("当前未选择");
                }
                else
                {
                    // 将列表转换为逗号分隔的字符串
                    QStringList strList;
                    for (const auto& id : container)
                    {
                        strList << MxFunctions::mNumberToString(id);
                    }
                    m_lineEdit->setText(strList.join(", "));
                }
            }
            else
            {
                m_lineEdit->setText("未关联数据");
            }
        }private:
            void updateDisplayAndNotify()
            {
                updateDisplay();
                if (m_valueChangedCallback && m_object && m_member)
                {
                    m_valueChangedCallback(m_object->*m_member);
                }
            }private:
                ObjectType* m_object{ nullptr };
                ContainerType ObjectType::* m_member{ nullptr };
                PickFuncCallback m_pickFunc;
                ValueChangedCallback m_valueChangedCallback{ nullptr };

                QLabel* m_colorLabel{ nullptr };
                QLineEdit* m_lineEdit{ nullptr };
                QToolButton* m_okbtn{ nullptr };
                QToolButton* m_clearbtn{ nullptr };
    };
}
