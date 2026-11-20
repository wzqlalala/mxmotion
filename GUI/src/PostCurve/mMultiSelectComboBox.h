#pragma once
#include <QComboBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

namespace MechMainWindow
{
    class mMultiSelectComboBox : public QComboBox
    {
        Q_OBJECT
    public:
        explicit mMultiSelectComboBox(QWidget* parent = nullptr);
        void addItem(const QString& text);

        // 提前勾选的方法 
        void setItemChecked(int index, bool checked = true);
        void setCheckedItems(const QStringList& texts);

        void setBaseStr(QString text);

        // 获取结果的方法 
        QStringList checkedItems() const;   // 获取所有被勾选的文本列表
        QList<int> checkedIndexes() const; // 获取所有被勾选的索引列表
    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        class CheckBoxDelegate : public QStyledItemDelegate
        {
        public:
            using QStyledItemDelegate::QStyledItemDelegate;
            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        };

        void updateDisplayText();
        QStandardItemModel* m_model;
        QString baseText;
    };
}