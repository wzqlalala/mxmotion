#include "mMultiSelectComboBox.h"
#include <QLineEdit>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QAbstractItemView>

namespace MechMainWindow
{
    mMultiSelectComboBox::mMultiSelectComboBox(QWidget* parent) : QComboBox(parent)
    {
        setEditable(true);
        lineEdit()->setReadOnly(true);

        m_model = new QStandardItemModel(this);
        setModel(m_model);
        setItemDelegate(new CheckBoxDelegate(this));

        // 禁止内部列表的默认高亮选中（防止它把顶部文本替换掉）
        view()->setSelectionMode(QAbstractItemView::NoSelection);
        view()->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // 给内部视口安装事件过滤器（核心）
        view()->viewport()->installEventFilter(this);

        connect(m_model, &QStandardItemModel::dataChanged, this, &mMultiSelectComboBox::updateDisplayText);
    }

    void mMultiSelectComboBox::addItem(const QString& text)
    {
        QStandardItem* item = new QStandardItem(text);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        m_model->appendRow(item);
    }


    void mMultiSelectComboBox::setItemChecked(int index, bool checked)
    {
        if (index >= 0 && index < m_model->rowCount()) {
            QStandardItem* item = m_model->item(index);
            if (item) {
                // 更新模型数据，这会自动触发 dataChanged 信号从而调用 updateDisplayText()
                item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            }
        }
    }

    void mMultiSelectComboBox::setCheckedItems(const QStringList& texts)
    {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item) {
                // 如果文本在列表中，则勾选，否则取消勾选
                item->setCheckState(texts.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
            }
        }
    }

    void mMultiSelectComboBox::setBaseStr(QString text)
    {
        baseText = text;
    }


    QStringList mMultiSelectComboBox::checkedItems() const
    {
        QStringList selectedTexts;
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item && item->data(Qt::CheckStateRole).toInt() == Qt::Checked) {
                selectedTexts << item->text();
            }
        }
        return selectedTexts;
    }

    QList<int> mMultiSelectComboBox::checkedIndexes() const
    {
        QList<int> selectedIndexes;
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item && item->data(Qt::CheckStateRole).toInt() == Qt::Checked) {
                selectedIndexes << i;
            }
        }
        return selectedIndexes;
    }

    // 拦截按下和释放事件，阻止自动关闭和文本替换
    bool mMultiSelectComboBox::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == view()->viewport())
        {
            // 同时拦截按下和释放
            if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                QModelIndex index = view()->indexAt(mouseEvent->pos());

                // 如果点击在有效项目上
                if (index.isValid())
                {
                    // 计算左侧复选框的大致区域
                    QRect checkRect = view()->visualRect(index).adjusted(4, 4, -4, -4);
                    checkRect.setWidth(20);
                    checkRect.setHeight(20);

                    if (event->type() == QEvent::MouseButtonPress)
                    {
                        return true;
                    }
                    else if (event->type() == QEvent::MouseButtonRelease)
                    {
                        // 释放时，执行我们的真实逻辑
                        if (checkRect.contains(mouseEvent->pos()))
                        {
                            // 点击了复选框区域，切换状态
                            Qt::CheckState state = (index.data(Qt::CheckStateRole).toInt() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                            m_model->setData(index, state, Qt::CheckStateRole);
                            updateDisplayText();
                        }
                        return true;
                    }
                }
            }
        }
        return QComboBox::eventFilter(watched, event);
    }

    void mMultiSelectComboBox::updateDisplayText()
    {
        QStringList selectedItems;
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item->data(Qt::CheckStateRole).toInt() == Qt::Checked) {
                selectedItems << item->text();
            }
        }

        if (selectedItems.isEmpty()) {
            lineEdit()->setText("");
        }
        else {
            lineEdit()->setText("[" + baseText + " - " + selectedItems.join("-") + "]");
        }
    }

    // 绘制委托不变（只负责画，不管事件）
    void mMultiSelectComboBox::CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionButton checkBoxOption;
        checkBoxOption.rect = option.rect.adjusted(4, 4, -4, -4);
        checkBoxOption.rect.setWidth(20);
        checkBoxOption.rect.setHeight(20);
        checkBoxOption.state = index.data(Qt::CheckStateRole).toInt() == Qt::Checked ? QStyle::State_On : QStyle::State_Off;

        QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);

        painter->drawText(option.rect.adjusted(24, 0, 0, 0), Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
    }
}