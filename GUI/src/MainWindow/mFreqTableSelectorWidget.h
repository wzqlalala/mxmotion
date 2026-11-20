#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QIntValidator>
#include <QComboBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QItemSelectionModel>
#include <QApplication>
#include <functional>

namespace MechMainWindow
{
    class FreqTableSelectorEdit;

    // 保留多选选区并在单元格内直接编辑的表格控件
    class FreqTableSelectorEdit : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit FreqTableSelectorEdit(QWidget* parent = nullptr) : QTableWidget(parent) {}

    protected:
        void mousePressEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton)
            {
                QModelIndex idx = indexAt(event->pos());
                if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
                    && !(event->modifiers() & Qt::ControlModifier)
                    && !(event->modifiers() & Qt::ShiftModifier))
                {
                    // 在已选中的单元格上按下鼠标时不改变选区，只记录当前索引
                    m_lastPressedIndex = idx;
                    return;
                }
            }
            m_lastPressedIndex = QModelIndex();
            QTableWidget::mousePressEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (m_lastPressedIndex.isValid())
            {
                // 如果鼠标还在同一个单元格上释放，直接触发编辑
                QModelIndex idx = indexAt(event->pos());
                if (idx == m_lastPressedIndex)
                {
                    edit(idx);
                }
                m_lastPressedIndex = QModelIndex();
                return;
            }
            QTableWidget::mouseReleaseEvent(event);
        }

        void mouseDoubleClickEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton)
            {
                QModelIndex idx = indexAt(event->pos());
                if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
                    && !(event->modifiers() & Qt::ControlModifier)
                    && !(event->modifiers() & Qt::ShiftModifier))
                {
                    // 双击已选中的单元格，直接进入编辑，不改变选区
                    edit(idx);
                    return;
                }
            }
            QTableWidget::mouseDoubleClickEvent(event);
        }

        // 回车：结束编辑（如有）并移动到“最后一个选中单元格”的下一行同列，
        // 若已在最后一行，则请求外部添加新行并进入新行
        void keyPressEvent(QKeyEvent* event) override
        {
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            {
                QWidget* editor = qobject_cast<QWidget*>(qApp->focusWidget());
                if (editor && editor != this)
                {
                    commitData(editor);
                    closeEditor(editor, QAbstractItemDelegate::NoHint);
                }
                else if (state() == QAbstractItemView::EditingState)
                {
                    QTableWidget::keyPressEvent(event);
                }

                handleEnter();
                return;
            }
            QTableWidget::keyPressEvent(event);
        }

    private:
        void handleEnter()
        {
            QModelIndexList selected = selectedIndexes();
            if (selected.isEmpty()) return;

            QModelIndex lastIdx = selected.last();
            int nextRow = lastIdx.row() + 1;
            int col = lastIdx.column();

            // 当基准单元格位于最后一行时，自动请求添加新行
            if (nextRow >= rowCount())
            {
                emit requestAddRow();
                nextRow = rowCount() - 1;
            }

            QTableWidgetItem* nextItem = item(nextRow, col);
            if (!nextItem) return;

            setCurrentCell(nextRow, col, QItemSelectionModel::ClearAndSelect);
            scrollToItem(nextItem, QAbstractItemView::EnsureVisible);
        }

    signals:
        void requestAddRow();

    private:
        QModelIndex m_lastPressedIndex;
    };

    // 下拉框委托：多选后单击直接展开 QComboBox，选择一项后同步所有被选中单元格
    class FreqTableSelectorComboBoxDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using OptionCallback = std::function<QStringList()>;
        using BatchCallback = std::function<void(const QString&)>;

        explicit FreqTableSelectorComboBoxDelegate(OptionCallback optionCallback, BatchCallback batchCallback, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_optionCallback(optionCallback), m_batchCallback(batchCallback) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QComboBox* combo = new QComboBox(parent);
            combo->addItem("");
            if (m_optionCallback)
                combo->addItems(m_optionCallback());
            combo->setCurrentText(index.data(Qt::DisplayRole).toString());

            // 多选下拉框：创建时立即展开，选择任意项时同步到所有选中单元格
            QTimer::singleShot(0, combo, &QComboBox::showPopup);
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, [this](const QString& text) {
                        if (m_batchCallback) m_batchCallback(text);
                    });
            return combo;
        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override
        {
            QComboBox* combo = qobject_cast<QComboBox*>(editor);
            if (combo) combo->setCurrentText(index.data(Qt::DisplayRole).toString());
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            QComboBox* combo = qobject_cast<QComboBox*>(editor);
            QString text = combo ? combo->currentText() : QString();
            if (combo) model->setData(index, text, Qt::DisplayRole);

            // 批量同步到所有选中的同列单元格
            if (m_batchCallback) m_batchCallback(text);
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            editor->setGeometry(option.rect);
        }

    private:
        OptionCallback m_optionCallback;
        BatchCallback m_batchCallback;
    };

    class mFreqTableSelectorWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit mFreqTableSelectorWidget(QWidget* parent = nullptr);

        // ========== 对外接口 ==========
        void setComboBoxItems(const QStringList& items);

        // 获取所有行选中的名称
        QStringList getAllSelectedNames() const;
        void clearTableData();
    signals:
        // 新增行：传出新增的行号
        void rowAdded(int row);
        // 删除行：传出被删除的行号
        void rowRemoved(int row);
        // 下拉切换：行号 + 选中文本
        void comboChanged(int row, const QString& text);

    private slots:
        void addRow();
        void removeRow();
        void clearTable();
        void setRowByInput();
        void onComboChanged(int row, const QString& text); // 内部下拉槽

    private:
        void initUI();
        void setComboBoxRow(int row);
        // 批量设置当前列所有选中单元格，并同步数据
        void batchSetSelected(const QString& value);

    public:
        void addRows(int count);                // 新增
        bool setCurrentText(int row, const QString& text); // 新增
        bool setCurrentIndex(int row, int index);          // 新增

    private:
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QLineEdit* m_editRowNum;
        FreqTableSelectorEdit* m_table;

        // 下拉框选项列表
        QStringList m_comboItems;
    };
}
