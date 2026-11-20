#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QLabel>
#include <QPushButton>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QMap>
#include <memory>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QStyleOptionButton>

#include "mCommonTableWidget.h"
#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

#include "mConnectorBind.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mElementFunction.h"
#include "mModelStruct.h"
#include "mOtherFunction.h"

#include "mPickWidgetController.h"

using namespace MechConnectorData;

namespace MechConnectorData
{
    class mConnectorBolt;
}

namespace MechMainWindow
{
    //控件组合
    struct InputWidget
    {
        QLabel* labelname = nullptr;
        mCalcLineEdit* lineeditvalue = nullptr;
        QLabel* labelunit = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            lineeditvalue->setDisabled(on);
            labelunit->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name, bool hasunit = true)
        {
            name_ = name;
            labelname = new PanelLabel();
            lineeditvalue = new mCalcLineEdit();
            labelunit = new QLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            lineeditvalue->setFixedHeight(30);
            labelname->setText(name);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(lineeditvalue);
            if (hasunit)layout->addWidget(labelunit);

            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (lineeditvalue)
            {
                delete lineeditvalue;
                lineeditvalue = nullptr;
            }
            if (labelunit)
            {
                delete labelunit;
                labelunit = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct InputWithBtnWidget
    {
        QLabel* labelname = nullptr;
        mCalcLineEdit* lineeditvalue = nullptr;
        QPushButton* tablebtn = nullptr;
        QLabel* labelunit = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            lineeditvalue->setDisabled(on);
            labelunit->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name, bool hasunit = true)
        {
            name_ = name;
            labelname = new QLabel();
            lineeditvalue = new mCalcLineEdit();
            tablebtn = new QPushButton();
            labelunit = new QLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            tablebtn->setFixedSize(30, 30);
            QIcon icon(":/mMainWidgets/Icon/table.png");
            tablebtn->setIcon(icon);
            tablebtn->setIconSize(tablebtn->size());
            tablebtn->setStyleSheet(
                "QPushButton:pressed {"
                "    background-color: #666666;"
                "    border: 2px solid #333333;"
                "}"
            );
            lineeditvalue->setFixedHeight(30);
            labelname->setFixedWidth(50);
            labelname->setText(name);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(lineeditvalue);
            if (hasunit)layout->addWidget(labelunit);
            layout->addWidget(tablebtn);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (lineeditvalue)
            {
                delete lineeditvalue;
                lineeditvalue = nullptr;
            }
            if (labelunit)
            {
                delete labelunit;
                labelunit = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
            if (tablebtn)
            {
                delete tablebtn;
                tablebtn = nullptr;
            }
        }
    };
    struct ComboWidget
    {
        QLabel* labelname = nullptr;
        QComboBox* combobox = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            combobox->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name)
        {
            combobox = new QComboBox();
            combobox->setView(new QListView);
            labelname = new PanelLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            combobox->setFixedHeight(30);
            labelname->setText(name);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(combobox);
            //不设整行最小宽度：行内控件随参数栏自然收缩，
            //最小宽度由参数栏 dock 统一兜底，防止窄栏下右侧被裁切
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (combobox)
            {
                delete combobox;
                combobox = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct PosPickWidget
    {
        QLabel* labelname = nullptr;
        QCheckBox* labelpicked = nullptr;
        QPushButton* btnok = nullptr;
        QPushButton* btnclear = nullptr;
        QPushButton* btnadd = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>nodes_;
        std::vector<std::pair<MxFunctions::PickDataType, int>>nodes_id;
        QVector<QVector3D> renderpos;
        QString PickDataEnum2Str(MxFunctions::PickDataType type)
        {
            switch (type)
            {
            case MxFunctions::PickDataType::None:
                return "";
            case MxFunctions::PickDataType::GeoPoint:
                return "几何点";
            case MxFunctions::PickDataType::GeoEdge:
                return "几何边";
            case MxFunctions::PickDataType::GeoFace:
                return "几何面";
            case MxFunctions::PickDataType::GeoSolid:
                return "几何实体";
            case MxFunctions::PickDataType::Node:
                return "节点";
            case MxFunctions::PickDataType::ElementEdge:
                return "单元边";
            case MxFunctions::PickDataType::ElementFace:
                return "单元面";
            case MxFunctions::PickDataType::Element:
                return "单元";
            case MxFunctions::PickDataType::Part:
                return "部件";
            default:
                break;
            }
            return "";
        }
        void setState(bool picked)
        {
            if (nodes_.size() == 0)
            {
                picked = false;
            }
            QString txt = "详情：";

            if (picked)
            {
                labelpicked->setText("已选择:" + MxFunctions::mNumberToString(nodes_.size()) + "个对象");

                QHash<QString, int>pickedcnt;
                for (int i = 0; i < nodes_.size(); ++i)
                {
                    pickedcnt[PickDataEnum2Str(nodes_[i].first)]++;
                }
                for (auto pick = pickedcnt.begin(); pick != pickedcnt.end(); pick++)
                {
                    txt += "\n" + pick.key()+"个数:"+MxFunctions::mNumberToString(pick.value());
                }
            }
            else
            {
                labelpicked->setText("当前未选择");
                txt = "当前未选择";
            }
            labelpicked->setDisabled(!picked);
            labelpicked->setToolTip(txt);
        }

        void setCheckedState(bool checked)
        {
            if (checked == true) {
                // 勾选状态：边框和字体都是 #1385F6
                labelpicked->setStyleSheet(R"(
            QWidget {
                border: 1px solid #1385F6;
                border-radius: 4px;
                color: #1385F6;
                padding-left: 7px;  /* 左内边距，内部所有内容右移 */
            }
        )");
            }
            else {
                // 非勾选状态：普通颜色
                labelpicked->setStyleSheet(R"(
            QWidget {
                border: 1px solid #DCDFE6;
                border-radius: 4px;
                color: #303133;
                padding-left: 7px;  /* 左内边距，内部所有内容右移 */
            }
        )");
            }
        }

        void initWidget(const QString& name, bool hasadd = false)
        {
            name_ = name;
            labelname = new PanelLabel();
            labelpicked = new QCheckBox();
            btnok = new QPushButton();
            btnclear = new QPushButton();
            btnadd = new QPushButton();
            layout = new QHBoxLayout();
            propwid = new QWidget();

            labelname->setText(name);
            labelpicked->setMinimumWidth(100);
            btnok->setFixedWidth(30);
            btnclear->setFixedWidth(30);
            btnadd->setFixedWidth(30);
            btnok->setIcon(QIcon("")); btnok->setToolTip("选择节点"); btnok->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
            btnclear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); btnclear->setToolTip("清空选择");
            btnadd->setIcon(QIcon("")); btnadd->setToolTip("添加节点"); btnadd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(labelpicked);
            layout->addSpacing(3);
            layout->addWidget(btnadd);
            btnadd->setVisible(false);
            layout->addWidget(btnok);
            layout->addWidget(btnclear);

            //不设整行最小宽度：行内控件随参数栏自然收缩，
            //最小宽度由参数栏 dock 统一兜底，防止窄栏下右侧被裁切
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);

            setCheckedState(false);

        }
        void hideAddBtn(bool hide)
        {
            btnadd->setVisible(!hide);
        }
        void clear()
        {
            nodes_.clear();
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (labelpicked)
            {
                delete labelpicked;
                labelpicked = nullptr;
            }
            if (btnok)
            {
                delete btnok;
                btnok = nullptr;
            }
            if (btnclear)
            {
                delete btnclear;
                btnclear = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct ComboBtnWidget
    {
        QLabel* labelname = nullptr;
        QComboBox* combobox = nullptr;
        QPushButton* btn = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            combobox->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name)
        {
            combobox = new QComboBox();
            combobox->setView(new QListView);
            labelname = new PanelLabel();
            btn = new QPushButton();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            combobox->setFixedHeight(30);
            btn->setFixedSize(30,30);
            btn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/cailiao.png")); btn->setToolTip("创建材料");
            layout->setMargin(0);
            layout->setSpacing(5);
            layout->addWidget(labelname);
            layout->addWidget(combobox);
            layout->addWidget(btn);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (combobox)
            {
                delete combobox;
                combobox = nullptr;
            }
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct CheckBtnWidget
    {
        QCheckBox* labelname = nullptr;
        QPushButton* btn = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        void initWidget(const QString& name)
        {
            labelname = new QCheckBox();
            btn = new QPushButton();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setFixedWidth(90);
            labelname->setText(name);
            btn->setFixedSize(30, 30);
            QIcon icon(":/mMainWidgets/Icon/table.png");
            btn->setIcon(icon);
            btn->setIconSize(btn->size());
            btn->setStyleSheet(
                "QPushButton:pressed {"
                "    background-color: #666666;"
                "    border: 2px solid #333333;"
                "}"
            );
            btn->setToolTip("输入参数");
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(btn);
            layout->addStretch();
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct CheckBoxsWidget
    {
        QVector<QCheckBox*> labels;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        void initWidget(const QVector<QString>& names)
        {
            layout = new QHBoxLayout();
            propwid = new QWidget();
            layout->setMargin(0);
            layout->setSpacing(0);
            for (auto name : names)
            {
                QCheckBox* label = new QCheckBox(name);
                labels.append(label);
                layout->addWidget(label);
            }
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            for(auto& label:labels)
            {
                delete label;
                label = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct CheckWithInput
    {
        QCheckBox* labelname = nullptr;
        mCalcLineEdit* lineeditvalue = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        void setInpDisabled(bool on)
        {
            lineeditvalue->setDisabled(on);
        }
        void initWidget(const QString& name)
        {
            name_ = name;
            labelname = new QCheckBox;
            lineeditvalue = new mCalcLineEdit();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            lineeditvalue->setFixedHeight(30);
            labelname->setFixedWidth(90);
            labelname->setText(name);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(lineeditvalue);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);

        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (lineeditvalue)
            {
                delete lineeditvalue;
                lineeditvalue = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct SingleBtn
    {
        QPushButton* tablebtn = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        void setBtnDisabled(bool on)
        {
            tablebtn->setDisabled(on);
        }
        void initWidget(const QString& name,bool right=false)
        {
            name_ = name;
            tablebtn = new QPushButton(name);
            layout = new QHBoxLayout();
            propwid = new QWidget();
            layout->setMargin(0);
            if (right)
                layout->addStretch();
            layout->addWidget(tablebtn);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
            if (tablebtn)
            {
                delete tablebtn;
                tablebtn = nullptr;
            }
        }
    };
    struct SingleLabel
    {
        QLabel* label = nullptr;
        QLabel* labelname = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        void initWidget(const QString& name)
        {
            name_ = name;
            label = new PanelLabel(name);
            labelname = new QLabel;
            layout = new QHBoxLayout();
            propwid = new QWidget();
            layout->setMargin(0);
            layout->addWidget(label);
            layout->addWidget(labelname);
            layout->addStretch();
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
            if (label)
            {
                delete label;
                label = nullptr;
            }
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
        }
    };
    /*************************************************自定义表头复选框类***************************************************************/
    class mCheckBoxHeader : public QHeaderView
    {
        Q_OBJECT
    public:
        mCheckBoxHeader(Qt::Orientation orientation, QWidget* parent = nullptr)
            : QHeaderView(orientation, parent)
        {
            setSectionsClickable(true);
        }

        ~mCheckBoxHeader() {}

        void setColumnCheckBox(int col, bool show, const QString& text = QString())
        {
            if (show)
            {
                if (!columnCheckBoxMap.contains(col))
                {
                    QCheckBox* cb = new QCheckBox(this);
                    cb->setText(text);
                    cb->setChecked(false);
                    cb->setTristate(false); // 永久2态，禁用三态
                    cb->setStyleSheet(R"(
					QCheckBox { background: transparent; border: none; color: #333; font-size: 12px; }
    QCheckBox::indicator { width: 16px; height: 16px; }
    QCheckBox::indicator::unchecked { border: 1px solid #DCDFE6; border-radius: 2px; background: #FFFFFF; }
    QCheckBox::indicator::checked { border: 1px solid #1385F6; border-radius: 2px; background: #1385F6; }
    QCheckBox::indicator::indeterminate { border: 1px solid #DCDFE6; border-radius: 2px; background: #E7F3FE; }
)");
                    connect(cb, &QCheckBox::stateChanged, this, [this, col](int state) {
                        emit columnCheckStateChanged(col, state);
                        });
                    columnCheckBoxMap[col] = cb;
                    updateCheckBoxPositions();
                }
            }
            else
            {
                if (columnCheckBoxMap.contains(col))
                {
                    delete columnCheckBoxMap[col];
                    columnCheckBoxMap.remove(col);
                }
            }
        }

        void updateCheckBoxPositions()
        {
            for (auto it = columnCheckBoxMap.begin(); it != columnCheckBoxMap.end(); ++it)
            {
                int col = it.key();
                QCheckBox* cb = it.value();
                int cbWidth = cb->sizeHint().width();
                cb->setFixedWidth(cbWidth);
                int x = sectionViewportPosition(col) + (sectionSize(col) - cbWidth) / 2;
                int y = (height() - cb->height()) / 2;
                cb->move(x, y);
                cb->setVisible(true);
            }
        }

        void setColumnCheckState(int col, int state)
        {
            if (columnCheckBoxMap.contains(col))
            {
                columnCheckBoxMap[col]->blockSignals(true);
                columnCheckBoxMap[col]->setChecked(state == Qt::Checked);
                columnCheckBoxMap[col]->blockSignals(false);
            }
        }

        int getColumnCheckState(int col) const
        {
            if (columnCheckBoxMap.contains(col))
                return columnCheckBoxMap[col]->isChecked() ? Qt::Checked : Qt::Unchecked;
            return Qt::Unchecked;
        }

        void clearAllCheckBoxes()
        {
            for (auto it = columnCheckBoxMap.begin(); it != columnCheckBoxMap.end(); ++it)
            {
                delete it.value();
            }
            columnCheckBoxMap.clear();
        }

    protected:
        void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override
        {
            QHeaderView::paintSection(painter, rect, logicalIndex);
            if (columnCheckBoxMap.contains(logicalIndex))
            {
                // 复选框已经在updateCheckBoxPositions中移动到正确位置
                // 这里不做额外绘制，让复选框浮在header上方
            }
        }

        void resizeEvent(QResizeEvent* event) override
        {
            QHeaderView::resizeEvent(event);
            updateCheckBoxPositions();
        }

        void sectionResized(int logicalIndex, int oldSize, int newSize)
        {
            QHeaderView::sectionResized(logicalIndex, oldSize, newSize);
            updateCheckBoxPositions();
        }

    signals:
        void columnCheckStateChanged(int col, int state);

    private:
        QMap<int, QCheckBox*> columnCheckBoxMap;
    };

    // 通用表格控件别名（支持回车进入下一行同列）
    using mConnectorTableWidget = mCommonTableWidget;

    /*************************************************表格复选框委托**************************************************************/
    class mCheckBoxDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        mCheckBoxDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        void setColumnText(int col, const QString& text)
        {
            columnTexts_[col] = text;
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());

            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);

            QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

            QString text = columnTexts_.value(index.column(), QString());

            QStyleOptionButton checkBoxOpt;
            QRect checkRect = style->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxOpt, opt.widget);

            int spacing = 4;
            int textWidth = 0;
            if (!text.isEmpty()) {
                textWidth = opt.fontMetrics.horizontalAdvance(text) + spacing;
            }

            int totalWidth = checkRect.width() + textWidth;
            int startX = opt.rect.left() + (opt.rect.width() - totalWidth) / 2;
            if (startX < opt.rect.left()) startX = opt.rect.left();

            // 绘制复选框
            checkBoxOpt.rect = checkRect;
            checkBoxOpt.rect.moveLeft(startX);
            checkBoxOpt.rect.moveTop(opt.rect.top() + (opt.rect.height() - checkRect.height()) / 2);
            checkBoxOpt.state = QStyle::State_Enabled | QStyle::State_Active;
            if (state == Qt::Checked)
                checkBoxOpt.state |= QStyle::State_On;
            else
                checkBoxOpt.state |= QStyle::State_Off;
            style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkBoxOpt, painter, opt.widget);

            // 绘制文字
            if (!text.isEmpty()) {
                QRect textRect = opt.rect;
                textRect.setLeft(startX + checkRect.width() + spacing);
                textRect.setRight(opt.rect.right());
                if (opt.state & QStyle::State_Selected)
                    painter->setPen(opt.palette.color(QPalette::HighlightedText));
                else
                    painter->setPen(opt.palette.color(QPalette::Text));
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
            }
        }

        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override
        {
            if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (option.rect.contains(mouseEvent->pos()))
                {
                    int currentState = index.data(Qt::CheckStateRole).toInt();
                    Qt::CheckState newState = (currentState == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                    model->setData(index, newState, Qt::CheckStateRole);
                    return true;
                }
            }
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }

    private:
        QMap<int, QString> columnTexts_;
    };

    /*************************************************连接类**************************************************************/
    class mConnectorBase;
    class mConnectorRigid;
    class mConnectorFlexible;
    class mConnectorMPC;
    class mConnectorSpringAndDamping;
    class mConnectorBind;
    class mConnectingDisperse;
    class mConnectorBoltPanel;
    class mConnectorDistributedCouplingPanel;
    class mDirectionSelectorWidget;
    class mConnectorBearingUnits;
    class mConnectorRigidbodyPanel;
    class MMAINWINDOW_EXPORT mConnector : public QObject
    {
        Q_OBJECT

    public:
        ~mConnector();

        /*
        * 获取单例指针
        */
        static mConnector* getInstance();

        /*
        * 根据类型获取连接控件
        */
        mConnectorBase* getConnectorByType(MxFunctions::ConnectorType type);

    private:
        mConnector(QObject* parent = nullptr);

    private:
        //单例指针
        static mConnector* _instance;

        mConnectorBase* m_ConnectorBase = nullptr;
        mConnectorRigid* m_ConnectorRigid = nullptr;
        mConnectorFlexible* m_ConnectorFlexible = nullptr;
        mConnectorMPC* m_ConnectorMPC = nullptr;
        mConnectingDisperse* m_ConnectingDisperse = nullptr;
        mConnectorBind* m_ConnectorBind = nullptr;
        mConnectorBoltPanel* m_ConnectorBolt = nullptr;
        mConnectorDistributedCouplingPanel* m_ConnectorDistributedCoupling = nullptr;
        mConnectorBearingUnits* m_ConnectorBearingUnits = nullptr;
        mConnectorRigidbodyPanel* m_ConnectorRigidbodyPanel = nullptr;
    };

    /*************************************************连接基类***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorBase : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorBase(QWidget * parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorBase();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 控件设值
        */
        virtual void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData) = 0;

        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;

        /*
        * 连接单位制响应信号
        */
        virtual void connectUnitSystemResponse() {};

        /*
        * 断开单位制响应信号
        */
        virtual void disconnectUnitSystemResponse() {};

        /*
        * 获取拾取矢量数据
        */
        bool getPickVecData(QVector3D& point);

        /*
        * 获取拾取的点数据
        */
        bool getPickPointData(QVector3D& point);

        /*
        * 设置连接数据
        */
        void setConnData(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData);

        /*
        * 目标枚举转字符串
        */
        QString PickDataEnum2Str(MxFunctions::PickDataType type);

        /*
        * 计算主节点
        */
        int calculateMasterNode(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickedData);

        /*
        * 临时节点渲染
        */
        virtual void rendFreeNodes(bool showhide) {};

        /*
        * 将选取对象转为临时节点 形心 or 多点等距
        */
        QVector<QVector3D>getNodesCenter(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);//获取多个从节点的中心
        void transPickedToNode(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);//主节点生成临时节点
        QVector<QVector3D> transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);//从节点取多个位置信息

        /*
        * 获取选择对象中的节点
        */
        void getNodesFromPicked(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);

        /*
        * 表格信号
        */
        virtual void setConnectTbaleSig(bool on) {};

        /*
        * 获取作用对象
        */
        virtual std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getPickedData()
        {
            return std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>();
        };

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        virtual void deactivatePickAndCommit() {};
    signals:
        /*
        * 连接器面板专用拾取完成信号
        */
        void sg_connectorPickFinished();

    protected:
        /*
        * 通知外部连接数据已变更，触发浏览器树/渲染刷新。
        * 改造后的局部保存统一使用此接口，不再通过 sg_editFinished 全量保存。
        */
        void notifyConnectorChanged();

    protected:

        std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData;

        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>_pickeddata;

        QString defaultStyle=
            "QPushButton {"
            "background-color: #FFFFFF;"  // 浅蓝色
            "border: 1px solid #DCDFE6;"  // 蓝色边框
            "color: #303133;"
            "padding: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #E7F3FE;"
            "border: 1px solid #B8DAFC;"
            "color: #1385F6;"
            "}"
            "QPushButton:pressed {"
            "background-color: #E7F3FE;"
            "border: 1px solid #1385F6;"  // 蓝色边框
            "color: #1385F6;"
            "}"
            "QPushButton:disabled {"
            "background-color: #F5F7FA;"
            "border: 1px solid #DCDFE6;"  // 蓝色边框
            "color: #A8ABB2;"
            "}";
        QString checkedStyle=
            "QPushButton {"
            "background-color: #E7F3FE;"  // 深蓝色
            "border: 1px solid #1385F6;"
            "color: #1385F6;"
            "padding: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #1385F6;"
            "border: 1px solid #1385F6;"
            "color: #FFFFFF;"
            "}"
            "QPushButton:pressed {"
            "background-color: #1178DE;"
            "border: 1px solid #DCDFE6;"  // 蓝色边框
            "color: #FFFFFF;"
            "}"
            "QPushButton:disabled {"
            "background-color: #F5F7FA;"
            "border: 1px solid #DCDFE6;"  // 蓝色边框
            "color: #A8ABB2;"
            "}";
    };

    /*************************************************绑定***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorBind : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorBind(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorBind();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 位置输入控件
        */
        QVector<QWidget*> getPosInputs(FoldSection* fapos);

        /*
        * 参数输入控件
        */
        QVector<QWidget*> getValInputs(FoldSection* faval);
    private slots:

        /*
        * 交换主从面
        */
        void slot_switchMainAndSubs();

        /*
        * 更新主面渲染高亮
        */
        void slot_commitMasterPickData();

        /*
        * 更新从面渲染高亮
        */
        void slot_commitSlavePickData();

        /*
        * 清空主面
        */
        void slot_clearMainNode();

        /*
        * 清空从面
        */
        void slot_clearSubNode();

        /*
        * 主面激活变化
        */
        void slot_masterPickActiveChanged(bool active);

        /*
        * 从面激活变化
        */
        void slot_slavePickActiveChanged(bool active);

        /*
        * 反显主/从面到控件
        */
        void loadBindDataToPickWidget(int mapType, mPickWidgetController* picker);

        /*
        * 刷新值区域（容差输入框显影等）
        */
        void refreshValueSection();

        /*
        * 清空red blue高亮
        */
        void clearFaceHighlight();

    private:
        /*
        * 局部保存：类型
        */
        void saveType();

        /*
        * 局部保存：容差类型
        */
        void saveAllowranceType();

        /*
        * 局部保存：容差值
        */
        void saveAllowrance();

        /*
        * 局部保存：调整选项
        */
        void saveAdjust();

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;
    private:
        std::shared_ptr<MechConnectorData::mConnectorBind> connectorPtr;
        FoldSection* fapos_;
        FoldSection* faval_;

        // 绑定面板主/从面通用拾取控件
        mPickWidgetController* _masterPickController = nullptr;
        mPickWidgetController* _slavePickController = nullptr;

        QWidget* widinside;
        QWidget* widoutside;
        QVBoxLayout* vlayout;
        QHBoxLayout* hlayout;
        QPushButton* switchbtn;


        ComboWidget typewid;
        QStringList valtypes_;

        ComboWidget allowrancetype_;
        InputWidgetSet allowrance_;

        CheckBtnWidget adjust_;
    };

    /*************************************************刚性连接***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorRigid : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorRigid(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorRigid();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 位置输入控件
        */
        QVector<QWidget*> getPosInputs(FoldSection* fapos);

        /*
        * 自由度输入控件
        */
        QVector<QWidget*> getValInputs(FoldSection* faval);

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;
    private slots:
        /*
        * 方式改变
        */
        void slot_nodeWayChanged(int i);

        /*
        * 自动计算主节点
        */
        void slot_calMainNode();

    private:
        /*
        * 局部保存：方式
        */
        void saveWay();

        /*
        * 局部保存：自由度
        */
        void saveDegreeOfFreedom();
    private:
        /*
        * 按当前方式设置主/从控件拾取模式（一对一 PickOne，一对多 PickMore）
        */
        void applyPickMode();

        /*
        * 按当前方式设置拾取类型白名单
        */
        void applyAllowedPickTypes();

        /*
        * 删除连接主节点
        */
        void deleteMasterConnNodes();

        /*
        * 删除连接从节点
        */
        void deleteSubConnNodes();

        /*
        * 添加连接主节点
        */
        void appendMainConnNodes();
        /*
        * 添加连接从节点
        */
        void appendSubConnNodes();

        /*
        * 实时更新刚性连接符号（主从节点连线/标识），类似集中力的实时箭头渲染。
        * 基于当前拾取控件数据直接绘制，不依赖切出后的持久化数据。
        */
        void updateConnectorSymbolRealtime();

        /*
        * 将面板当前数据转换为 MxFunctions::PickDataType 形式
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getWidgetDataAsPickPairs() const;
        std::vector<std::pair<MxFunctions::PickDataType, int>> getWidgetDataAsPickIds() const;
    private:
        FoldSection* fapos_;
        FoldSection* faval_;

        ComboWidget poswaywid;
        ComboWidget choosewid;
        mPickWidgetController* _mainPickController = nullptr;
        mPickWidgetController* _subPickController = nullptr;
        QVector<CheckBoxsWidget>checkswid;
        QVector<QString>names1_;
        QVector<QString>names2_;

        int master_node = -1;
        std::vector<int> sub_node;
    };

    /*************************************************柔性连接***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorFlexible : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorFlexible(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorFlexible();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;

        /*
        * 位置输入控件
        */
        QVector<QWidget*> getPosInputs(FoldSection* fapos);

        /*
        * 主节点权重控件
        */
        QVector<QWidget*> getWightsAndValsInputs(FoldSection* faweight);

        /*
        * 主节点自由度控件
        */
        QVector<QWidget*> getMainValInputs(FoldSection* famainval);

        /*
        * 从节点自由度控件
        */
        QVector<QWidget*> getSubValInputs(FoldSection* fasubval);

        /*
        * 临时节点渲染
        */
        void rendFreeNodes(bool showhide)override;

        /*
        * 表格信号
        */
        void setConnectTbaleSig(bool on)override;

    private slots:
        /*
        * 方式改变
        */
        void slot_nodeWayChanged(int i);

        /*
        * 通用拾取控件主节点数据变化
        */
        void slot_mainPickDataChanged();

        /*
        * 通用拾取控件从节点数据变化
        */
        void slot_subPickDataChanged();

        /*
        * 勾选自由度
        */
        void slot_checkDegreeOfFreedom(int state);

        /*
        * 表格编辑完成
        */
        void slot_tableEditFinished(int row, int col);

        /*
        * 节点权重设置
        */
        void slot_nodeSetting();

        /*
        * 表头复选框状态变化
        */
        void slot_headerCheckBoxStateChanged(int col, int state);

    private:

        /*
        * 局部保存：方式
        */
        void saveWay();

        /*
        * 局部保存：主节点
        */
        void saveMainNode();

        /*
        * 局部保存：从节点
        */
        void saveSubNode();

        /*
        * 局部保存：权重
        */
        void saveWeight();

        /*
        * 局部保存：自由度
        */
        void saveDegreeOfFreedom();

        /*
        * 局部保存：表格数据
        */
        void saveTableData();

        /*
        * 局部保存：差异化设置模式
        */
        void saveTableMode();

        /*
        * 初始化表格
        */
        void initTable();

        /*
        * 表格设值
        */
        void setValueToTable(const QVector<QVector<QString>>& data);

        /*
        * 自动计算主节点
        */
        void calMainNode();

        /*
        * 计算主从节点对,返回部件上的节点ID
        */
        QSet<int> calMain2Subs(
            std::vector<std::pair<MxFunctions::PickDataType, int>>& areas,
            std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& parts);

        /*
        * 权重表格按钮控制
        */
        void updateTableBtnUsing();

        /*
        * 整列勾选/取消勾选
        */
        void toggleColumnCheckBoxes(int col, bool checked);

        /*
        * 根据表格数据更新表头复选框状态
        */
        void updateHeaderCheckBoxState();

        /*
        * 删除连接主节点
        */
        void deleteMainConnNodes();
        /*
        * 删除连接从节点
        */
        void deleteSubConnNodes();

        /*
        * 添加连接主节点
        */
        void appendMainConnNodes();
        /*
        * 添加连接从节点
        */
        void appendSubConnNodes();

        /*
        * 实时更新柔性连接符号（主从节点连线/标识）
        * 基于当前拾取控件数据直接绘制，不依赖切出后的持久化数据。
        */
        void updateConnectorSymbolRealtime();

        void slot_calMainNode();

    private:
        /*
        * 将当前拾取控件数据转换为 MxFunctions::PickDataType 形式
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getWidgetDataAsPickPairs(bool isMain) const;
        std::vector<std::pair<MxFunctions::PickDataType, int>> getWidgetDataAsPickIds(bool isMain) const;

        /*
        * 按当前方式设置主/从控件拾取模式
        */
        void applyPickMode();

        /*
        * 按当前方式设置拾取类型白名单
        */
        void applyAllowedPickTypes();

    private:
        QHash<QString, int>_mattable;

        FoldSection* fapos_;
        FoldSection* famainweight_;
        FoldSection* famainval_;
        FoldSection* fasubval_;

        ComboWidget poswaywid;
        ComboWidget choosewid;
        QVector <PosPickWidget> poswid;
        mPickWidgetController* _mainPickController = nullptr;
        mPickWidgetController* _subPickController = nullptr;
        InputWidget weights_;
        SingleBtn nodeseting_;
        QVector<CheckBoxsWidget>checkswid;
        QVector<QString>names1_;
        QVector<QString>names2_;
        QStringList header_;
        QTableWidget* table_;
        mCheckBoxHeader* checkBoxHeader_;
        mCheckBoxDelegate* checkBoxDelegate_ = nullptr;

        //主从节点对
        QHash<int, std::vector<int>>_main2subs;
    };

    /*************************************************MPC约束***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorMPC : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorMPC(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorMPC();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        void saveSubNode();
        void saveMainNode();

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 位置输入控件
        */
        QVector<QWidget*> getPosInputs(FoldSection* fapos);

        /*
        * 主节点自由度输入控件
        */
        QVector<QWidget*> getValInputs(FoldSection* faval);

        /*
        * 从节点自由度输入控件
        */
        QVector<QWidget*> getValInputs2(FoldSection* faval);

        /*
        * Coefficient
        */
        QVector<QWidget*> getValInputs3(FoldSection* faval);

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;

        /*
        * 临时节点渲染
        */
        void rendFreeNodes(bool showhide)override;

        /*
        * 表格信号
        */
        void setConnectTbaleSig(bool on)override;
    private slots:
        /*
        * 节点差异化设置
        */
        void slot_nodeSetting();

        /*
        * 表格编辑完成
        */
        void slot_tableEditFinished(int row, int col);

        /*
        * 表格多选下单击保持选区并进入编辑
        */
        void slot_cellClickedKeepSelection(int row, int col);

    private:
        /*
        * 初始化表格
        */
        void initTable();

        /*
        * 表格设值
        */
        void setValueToTable(const QVector<QVector<double>>& data);

        /*
        * 自动计算从节点（主节点形心临时节点）
        */
        void calMainNode();

        /*
        * 权重表格按钮控制
        */
        void updateTableBtnUsing();

        /*
        * 删除连接从节点（UI从节点位置）
        */
        void deleteSubConnNodes();

        /*
        * 删除连接主节点（UI主节点位置）
        */
        void deleteMainConnNodes();

        /*
        * 添加连接从节点
        */
        void appendSubConnNodes();

        /*
        * 添加连接主节点
        */
        void appendMainConnNodes();

        /*
        * 实时更新MPC连接符号
        */
        void updateConnectorSymbolRealtime();

        /*
        * 将当前拾取控件数据转换为 MxFunctions::PickDataType 形式
        * isSub=true 取从节点位置控件，false 取主节点位置控件
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getWidgetDataAsPickPairs(bool isSub) const;

        /*
        * 局部保存：从节点自由度
        */
        void saveSubDof();

        /*
        * 局部保存：从节点权重
        */
        void saveSubWeight();

        /*
        * 局部保存：主节点默认自由度权重
        */
        void saveDefaultWeights();

        /*
        * 局部保存：非齐次项系数
        */
        void saveCoefficient();

        /*
        * 局部保存：表格权重数据
        */
        void saveTableWeights();

    private:
        QHash<QString, int>_mattable;

        FoldSection* fapos_;
        FoldSection* faval_;
        FoldSection* faval2_;
        FoldSection* faval3_;

        // 通用拾取控件：从节点位置（单选）
        mPickWidgetController* _subPickController = nullptr;
        // 通用拾取控件：主节点位置（多选）
        mPickWidgetController* _mainPickController = nullptr;

        QVector<CheckWithInput> freedomandweights;
        SingleBtn nodeseting_;
        QStringList names_;
        QStringList header_;

        mConnectorTableWidget* table_;

        ComboWidget combo_;
        InputWidgetSet inputwid_;

        InputWidgetSet coefficient_;
    };

    /*************************************************离散刚体***************************************************************/
    class MMAINWINDOW_EXPORT mConnectingDisperse : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectingDisperse(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectingDisperse();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值-刷新界面数值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 参考点控件
        */
        QVector<QWidget*> getReferencePoint(FoldSection* value);

        /*
        * 刚体控件
        */
        QVector<QWidget*> getRigidBody(FoldSection* value);

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;
    private slots:

        /*
         * 单元集合选择变化
         */
        void slot_cellSetChanged(int idx);

        /*
         * 参考点拾取激活变化
         */
        void slot_refPointPickActiveChanged(bool active);

    private:
        /*
         * 局部保存：单元集合映射
         */
        void saveCellSet();

        FoldSection* referencePoint_ = nullptr;//参考点功能区
        FoldSection* rigidBody_ = nullptr;//刚体功能区

        // 参考点通用拾取控件（单点：几何点/节点）
        mPickWidgetController* _refPointPickController = nullptr;

        // 刚体单元集合下拉框
        singleComboBox _cellSetCombo;
    };

    /*************************************************解析刚体***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorRigidbodyPanel : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorRigidbodyPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorRigidbodyPanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值-刷新界面数值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 参考点控件
        */
        QVector<QWidget*> getReferencePoint(FoldSection* value);

        /*
        * 解析表面集控件
        */
        QVector<QWidget*> getSurfaceAnalysis(FoldSection* value);

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;
    private slots:

        /*
         * 单元集合选择变化
         */
        void slot_cellSetChanged(int idx);

        /*
         * 参考点拾取激活变化
         */
        void slot_refPointPickActiveChanged(bool active);

    private:
        /*
         * 局部保存：单元集合映射
         */
        void saveCellSet();

        FoldSection* referencePoint_ = nullptr;//参考点功能区
        FoldSection* surfaceBody_ = nullptr;//解析表面集功能区

        // 参考点通用拾取控件（单点：几何点/节点）
        mPickWidgetController* _refPointPickController = nullptr;

        // 解析表面集合下拉框
        singleComboBox _cellSetCombo;
    };

    /**************************************************螺栓约束**************************************************************/
    class MMAINWINDOW_EXPORT mConnectorBoltPanel : public mConnectorBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mConnectorBoltPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mConnectorBoltPanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值-刷新界面数值
        */
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;

        /*
        * 类型定义
        */
        QVector<QWidget*> getTypeDefine(FoldSection* value);

        /*
        * 作用对象
        */
        QVector<QWidget*> getTargetObject(FoldSection* value);

        /*
        * 矢量定义
        */
        QVector<QWidget*> getVectorDefine(FoldSection* value);

        /*
        * 获取当前作用对象拾取数据
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getPickedData();

        /*
        * 失活并提交拾取数据
        */
        void deactivatePickAndCommit()override;

        /*
         * 实时更新螺栓约束渲染符号
         */
        void updateConnectorSymbolRealtime();
    private slots:

        void slot_typeDefineChanged(int idx);
        void slot_targetPickActiveChanged(bool active);
        void slot_targetDataChanged();
    private:

        /*
         * 根据螺栓类型设置拾取类型白名单与可选集合
         */
        void applyAllowedPickTypes(int typeIndex);

        /*
         * 获取作用对象拾取数据（集合+拾取）
         */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getTargetPickPairs() const;

        /*
         * 过滤螺栓混合集合中的单元数据：一维螺栓仅保留一维单元
         */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> filterBoltMixSetData(
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& mixData,
            int boltType, mMeshData* meshData) const;

        /*
         * 懒创建预紧方向控件（参考 mLoadGravityPanel::ensureDirectionWidget）
         */
        void ensureDirectionWidget();

        /*
         * 将连接数据中的方向状态反显到方向控件（含 dirPickState_ 完整UI状态）
         */
        void applyDirStateToWidget(std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr);

        /*
         * 将方向控件当前状态保存到连接数据（含 dirPickState_ 完整UI状态）
         */
        void applyDirectionWidgetToConnector(std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr);

        FoldSection* typeDefine_ = nullptr;//类型定义
        FoldSection* targetObject_ = nullptr;//作用对象
        FoldSection* vectorDefine_ = nullptr;//预紧方向（原矢量定义）

        singleComboBox _typeCombo;

        // 作用对象通用拾取控件
        mPickWidgetController* _targetPickController = nullptr;

        // 预紧方向控件（替代原 方式/分量/矢量 按钮组）
        mDirectionSelectorWidget* _directionWidget = nullptr;

        QVector3D _dir;

    };

    /*************************************************分布式耦合连接***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorDistributedCouplingPanel : public mConnectorBase
    {
        Q_OBJECT

    public:
        mConnectorDistributedCouplingPanel(QWidget* parent = nullptr);
        ~mConnectorDistributedCouplingPanel();
        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;
        QVector<QWidget*> getPosInputs(FoldSection* fapos);
        QVector<QWidget*> getWeightAndDofInputs(FoldSection* fasec);
        void rendFreeNodes(bool showhide)override;
        void setConnectTbaleSig(bool on)override;

        void saveSubNode();
        void saveMainNode();
        /*
        * 失活并提交拾取数据
        */
        void deactivatePickAndCommit()override;
    private slots:
        void slot_nodeWayChanged(int i);
        void slot_nodeSetting();
        void slot_setTablDataToConnector();

    private:
        /*
        * 局部保存：方式
        */
        void saveWay();

        /*
        * 局部保存：权重
        */
        void saveWeight();

        /*
        * 局部保存：自由度
        */
        void saveDegreeOfFreedom();

        /*
        * 局部保存：表格数据
        */
        void saveTableData();

        /*
        * 局部保存：差异化设置模式
        */
        void saveTableMode();

        /*
        * 自动计算主节点（参考点为空且耦合点非空时）
        */
        void calMainNode();

        /*
        * 按当前方式设置参考点/耦合点控件拾取模式
        */
        void applyPickMode();

        /*
        * 按当前方式设置拾取类型白名单
        */
        void applyAllowedPickTypes();

        /*
        * 将当前拾取控件数据转换为 pair 形式（isRef=true 参考点，false 耦合点）
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getWidgetDataAsPickPairs(bool isRef) const;

        /*
        * 差异化设置按钮可用性同步
        */
        void updateNodeSettingButtonState();

        /*
        * 实时更新分布式耦合连接符号（参考点-耦合点连线）
        * 基于当前拾取控件数据直接绘制，不依赖切出后的持久化数据。
        */
        void updateConnectorSymbolRealtime();

    public:
        void initTable();
        void setValueToTable(const QVector<QVector<QString>>& data);
        void getTableData(QVector<double>& weight);
        void deleteMainConnNodes();
        void deleteSubConnNodes();
        void appendMainConnNodes();
        void appendSubConnNodes();

        // 自定义 QWidget 表格（仅 Node ID + Weight 两列，参考 LoadCaseTable 蓝色风格）
        class WeightTableWidget : public QWidget
        {
        public:
            WeightTableWidget(QWidget* parent = nullptr) : QWidget(parent)
            {
                auto* mainLay = new QVBoxLayout(this);
                mainLay->setContentsMargins(0, 0, 0, 0);
                mainLay->setSpacing(0);

                table_ = new mConnectorTableWidget(this);
                table_->setColumnCount(2);
                table_->setHorizontalHeaderLabels(QStringList() << "Node ID" << "Weight");
                table_->horizontalHeader()->setStretchLastSection(true);
                table_->setSelectionBehavior(QAbstractItemView::SelectItems);
                table_->setSelectionMode(QAbstractItemView::ExtendedSelection);

                // 多选同时修改：第二列(Weight)选中多个单元格时，修改一个会同步到其他选中的单元格
                connect(table_, &QTableWidget::itemChanged, this, [=](QTableWidgetItem* item) {
                    if (isUpdating_ || item->column() != 1) return;
                    isUpdating_ = true;
                    QString text = item->text();
                    for (auto* selectedItem : table_->selectedItems()) {
                        if (selectedItem->column() == 1 && selectedItem != item) {
                            selectedItem->setText(text);
                        }
                    }
                    isUpdating_ = false;
                    });
                table_->setAlternatingRowColors(true);
                table_->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #87cefa;
            color: #2c3e50;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #b0d4f1;
        }
        QTableWidget {
            gridline-color: #d0e6f7;
            alternate-background-color:#ECF5FF;
            selection-background-color: #add8e6;
            selection-color: black;
        }
        QTableWidget::item {
            background: transparent;
        }
        QHeaderView::section:horizontal
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-left: 0;
            padding:4px;
            font-weight:normal;
        }
        QHeaderView::section:vertical
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-top: 0;
            padding:4px;
            font-weight:normal;
        }
    )");
                mainLay->addWidget(table_);
            }
            QTableWidget* table() const { return table_; }
        private:
            mConnectorTableWidget* table_ = nullptr;
            bool isUpdating_ = false;
        };

    private:
        QHash<QString, int> _mattable;
        FoldSection* fapos_;
        FoldSection* fasec_;
        ComboWidget poswaywid;
        // 参考点（主节点）通用拾取控件：单点，几何点/节点
        mPickWidgetController* _refPickController = nullptr;
        // 耦合点（从节点）通用拾取控件：一对一单点 / 一对多多点
        mPickWidgetController* _couplingPickController = nullptr;
        InputWidget weightInput_;
        QVector<CheckBoxsWidget> checkswid_;
        SingleBtn nodeseting_;
        WeightTableWidget* weightTable_ = nullptr;
        QVector<QString> names1_;
        QVector<QString> names2_;
        QHash<int, std::vector<int>> _main2subs;
    };

    /*************************************************轴承单元***************************************************************/
    class MMAINWINDOW_EXPORT mConnectorBearingUnits : public mConnectorBase
    {
        Q_OBJECT

    public:
        mConnectorBearingUnits(QWidget* parent = nullptr);
        ~mConnectorBearingUnits();
        void initInputs()override;
        void clearInputs()override;
        void setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)override;
        void rendFreeNodes(bool showhide)override;

        /*
        * 失活并提交通用拾取控件（切出/切换连接器时由面板调用）
        */
        void deactivatePickAndCommit()override;
        /*
        * 轴承单元控件
        */
        QVector<QWidget*> getBearingUnits(FoldSection* BearingUnits);
        /*
        * 轴承参数控件
        */
        QVector<QWidget*> getBearingPara(FoldSection* BearingPara);
    private:
        /*
        * 局部保存：类型
        */
        void saveType();
        /*
        * 局部保存：节点1坐标系映射
        */
        void saveCoordinate1();
        /*
        * 局部保存：节点2坐标系映射
        */
        void saveCoordinate2();
        /*
        * 局部保存：旋转平面
        */
        void saveRotateFace();
        /*
        * 局部保存：定义方式
        */
        void saveDefMode();
        /*
        * 局部保存：固定轴承参数（K/C 8项）
        */
        void saveBearingPara();
        /*
        * 局部保存：随转速变化曲线
        */
        void saveCurveRotate();
        /*
        * 删除连接节点1
        */
        void deleteNode1();
        /*
        * 删除连接节点2
        */
        void deleteNode2();
        /*
        * 添加连接节点1
        */
        void appendNode1();
        /*
        * 添加连接节点2
        */
        void appendNode2();
        /*
        * 临时节点渲染（单个节点）
        */
        void rendFreeNode(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id, bool showhide);
        /*
        * 实时更新轴承单元连接符号（节点1-节点2连线）
        */
        void updateConnectorSymbolRealtime();
        /*
        * 将拾取控件当前数据转换为 pair 形式（isNode1 区分节点1/节点2）
        */
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getWidgetDataAsPickPairs(bool isNode1) const;
        /*
        * 按当前类型刷新轴承单元区显隐（节点2仅"几何体-几何体"显示）
        */
        void applyNode2Visible();
    private:
        void slot_tableEditFinished(QVector<QVector<double>> x, QVector<QVector<double>> y);
        void setCoordinateOptions(const QStringList& options);
    private slots:
        void slot_typeChanged();
        void slot_defModeChanged();
        /*
        * 通用拾取控件节点1数据变化
        */
        void slot_node1PickDataChanged();
        /*
        * 通用拾取控件节点2数据变化
        */
        void slot_node2PickDataChanged();

    private:
        ComboWidget typeWid_;
        ComboWidget coordinateWid1_;
        ComboWidget coordinateWid2_;
        ComboWidget rotateFaceWid_;
        // 节点1/节点2通用拾取控件（单点：几何点/节点）
        mPickWidgetController* _node1PickController = nullptr;
        mPickWidgetController* _node2PickController = nullptr;
        InputWidget K11Input_;
        InputWidget K22Input_;
        InputWidget K12Input_;
        InputWidget K21Input_;
        InputWidget C11Input_;
        InputWidget C22Input_;
        InputWidget C12Input_;
        InputWidget C21Input_;
        ComboWidget cbRotate_;
        FoldSection* bearingUnits_;
        FoldSection* bearingPara_;
        QStringList bearingHeaderText_;
    };
}