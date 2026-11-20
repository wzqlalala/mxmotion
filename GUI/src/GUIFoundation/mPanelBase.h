#ifndef MPANELBASE_H
#define MPANELBASE_H

#include "guifoundation_global.h"
#include <QWidget>
#include <QColor>
#include <QShortcut>
#include <QDebug>
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QListWidget>
#include <QObject>
#include <type_traits>
#include <QFormLayout>
#include <QTimer>
#include <QLabel>

#include "mCalcLineEdit.h"

//折叠分组控件（FoldWidget/FoldSection/PanelLabel）与参数面板基座
//（mPanelBase 类本体，纯 Qt）均已下沉至 CommonWidgets 库——前处理/后处理/
//曲线三方参数栏共用同一实现与样式来源；此处转发包含，既有
//#include "mPanelBase.h" 的调用方无需改动
#include "mFoldSection.h"
#include "mParamPanelBase.h"

using namespace MxFunctions;

namespace MechMainWindow
{
    class GUIFOUNDATION_EXPORT DataBinder : public QObject {
        Q_OBJECT

    private:
        QList<QMetaObject::Connection> _connects;
    public:
        DataBinder(QObject* parent = nullptr) : QObject(parent) {}

        // 通用bind模板，根据控件类型调用对应的重载实现
        template<typename WidgetType, typename ObjectType, typename PropertyType>
        void bind(WidgetType* widget, ObjectType* object, PropertyType ObjectType::* member) {
            bindImpl(widget, object, member, std::integral_constant<int, widgetTypeId<WidgetType>()>{});
        }

        ~DataBinder()
        {
            for (auto conn : _connects)
            {
                disconnect(conn);
            }
        };

    private:
        // 利用类型映射给每个控件类型分配一个id
        template<typename T> static constexpr int widgetTypeId() { return 0; }

        // QComboBox绑定版本
        template<typename ObjectType, typename PropertyType>
        void bindImpl(QComboBox* widget, ObjectType* object, PropertyType ObjectType::* member, std::integral_constant<int, 1>) {
            // 控件值更新到对象
            _connects.append(connect(widget, &QComboBox::currentTextChanged, this, [=](const QString& val) {
                object->*member = val;
                }));
            // 对象值设置到控件
            widget->setCurrentText(object->*member);
        }

        // QLineEdit绑定版本
        template<typename ObjectType, typename PropertyType>
        void bindImpl(QLineEdit* widget, ObjectType* object, PropertyType ObjectType::* member, std::integral_constant<int, 2>) {
            // 控件值更新到对象
            _connects.append(connect(widget, &QLineEdit::textChanged, this, [=](const QString& val) {
                object->*member = val;
                }));
            widget->setText(object->*member);
        }

        // QCheckBox绑定版本（bool）
        template<typename ObjectType>
        void bindImpl(QCheckBox* widget, ObjectType* object, bool ObjectType::* member, std::integral_constant<int, 3>) {
            // 控件值更新到对象
            _connects.append(connect(widget, &QCheckBox::toggled, this, [=](bool val) {
                object->*member = val;
                }));
            widget->setChecked(object->*member);
        }

        // QSpinBox绑定版本（int）
        template<typename ObjectType>
        void bindImpl(QSpinBox* widget, ObjectType* object, int ObjectType::* member, std::integral_constant<int, 4>) {
            // 控件值更新到对象
            _connects.append(connect(widget, QOverload<int>(&QSpinBox::valueChanged), this, [=](int val) {
                object->*member = val;
                }));
            widget->setValue(object->*member);
        }

        // QDoubleSpinBox绑定版本（double）
        template<typename ObjectType>
        void bindImpl(QDoubleSpinBox* widget, ObjectType* object, double ObjectType::* member, std::integral_constant<int, 5>) {
            // 控件值更新到对象
            _connects.append(connect(widget, QOverload<double>(&QDoubleSpinBox::valueChanged), this, [=](double val) {
                object->*member = val;
                }));
            widget->setValue(object->*member);
        }

        // QLineEdit绑定版本
        template<typename ObjectType, typename PropertyType>
        void bindImpl(mCalcLineEdit* widget, ObjectType* object, PropertyType ObjectType::* member, std::integral_constant<int, 6>) {
            // 控件值更新到对象
            _connects.append(connect(widget, &QLineEdit::editingFinished, this, [=]() {
                object->*member = widget->text();
                }));
            widget->setExpression(object->*member);
        }

        // 其他控件不支持
        template<typename WidgetType, typename ObjectType, typename PropertyType>
        void bindImpl(WidgetType* /*widget*/, ObjectType* /*object*/, PropertyType ObjectType::* /*member*/, std::integral_constant<int, 0>) {
            static_assert(sizeof(WidgetType) == 0, "Unsupported widget type for binding");
        }
    };

        template<> constexpr int DataBinder::widgetTypeId<QComboBox>() { return 1; }
        template<> constexpr int DataBinder::widgetTypeId<QLineEdit>() { return 2; }
        template<> constexpr int DataBinder::widgetTypeId<QCheckBox>() { return 3; }
        template<> constexpr int DataBinder::widgetTypeId<QSpinBox>() { return 4; }
        template<> constexpr int DataBinder::widgetTypeId<QDoubleSpinBox>() { return 5; }
        template<> constexpr int DataBinder::widgetTypeId<mCalcLineEdit>() { return 6; }

    class GUIFOUNDATION_EXPORT UiController : public QObject {
        Q_OBJECT
    private:
        QList<QMetaObject::Connection> _connects;
    public:
        ~UiController()
        {
            for (auto conn : _connects)
            {
                disconnect(conn);
            }
        }
        template<typename Sender, typename Func>
        void bindVisibility(Sender* source, void (Sender::* signal)(const QString&), QWidget* target, Func func) {
            _connects.append(connect(source, signal, this, [=](const QString& value) {
                target->setVisible(func(value));
                }));
        }

        template<typename Sender, typename Func>
        void bindEnabled(Sender* source, void (Sender::* signal)(const QString&), QWidget* target, Func func) {
            _connects.append(connect(source, signal, this, [=](const QString& value) {
                target->setEnabled(func(value));
                }));
        }


    };

    //获取拾取的矢量数据
    GUIFOUNDATION_EXPORT bool getPickVecData(QVector3D& vec);

    //获取拾取的点数据
    GUIFOUNDATION_EXPORT bool getPickPointData(QVector3D& point);
}
#endif // MPANELBASE_H
