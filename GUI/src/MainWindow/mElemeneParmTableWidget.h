#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QTableWidget>
#include <QMenu>
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QSignalBlocker>
#include <QApplication>
#include <QMouseEvent>
#include <QModelIndex>
#include "mElementFunction.h"

using namespace MxFunctions;
namespace MechMainWindow
{
    // ========== 保留多选选区的表格控件 ==========
    class ElemeneParmTableEdit : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit ElemeneParmTableEdit(QWidget* parent = nullptr) : QTableWidget(parent) {}

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

    private:
        QModelIndex m_lastPressedIndex;
    };

    enum ElemeneParmItemRole {
        Role_ControlType = Qt::UserRole + 1,
        Role_ComboOptions = Qt::UserRole + 2,
        Role_DataKey = Qt::UserRole + 3
    };
    struct ElemeneParmColumnConfig {
        QString headerName;
        enum ControlType { LineEdit, ComboBox } controlType;
        QStringList comboOptions;
        QString dataKey;
    };

    class mElemeneParmHeaderView : public QHeaderView {
        Q_OBJECT

    public:
        explicit mElemeneParmHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

        // 设置跨列表头：起始列索引、跨越的列数、合并后的表头文本
        void addSpan(int startColumn, int spanCount, const QString& text);

        // 切换表格类型时清空合并规则
        void clearSpans();

    protected:
        // 核心：重写分段绘制逻辑
        void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

    private:
        struct SpanConfig {
            int spanCount;
            QString text;
        };

        // Key: 起始列索引, Value: 合并配置
        QMap<int, SpanConfig> m_spanMap;
    };

    class mElemeneParmTableWidget;

    class mElemeneParmTableDelegate : public QStyledItemDelegate {
        Q_OBJECT
    public:
        using BatchCallback = std::function<void(int, const QString&)>;

        explicit mElemeneParmTableDelegate(QObject* parent = nullptr)
            : QStyledItemDelegate(parent) {}

        // 1. 绘制单元格：非编辑状态下统一按普通文本/行编辑样式绘制
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
            QStyledItemDelegate::paint(painter, option, index);
        }

        // 2. 当用户双击/编辑单元格时，创建真实的编辑器控件
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
            int controlType = index.data(Role_ControlType).toInt();
            int col = index.column();

            if (controlType == ElemeneParmColumnConfig::ComboBox) {
                QComboBox* editor = new QComboBox(parent);
                QStringList options = index.data(Role_ComboOptions).toStringList();
                editor->addItems(options);

                // 多选下拉框：创建时立即展开，选择任意项时同步到所有选中单元格
                QTimer::singleShot(0, editor, &QComboBox::showPopup);
                connect(editor, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                        this, [this, col](const QString& text) {
                            emit requestBatchUpdate(col, text);
                        });
                return editor;
            }

            // 普通输入框：编辑完成时同步到所有选中单元格
            QLineEdit* editor = new QLineEdit(parent);
            editor->setText(index.data(Qt::DisplayRole).toString());
            connect(editor, &QLineEdit::editingFinished,
                    this, [this, editor, col]() {
                        emit requestBatchUpdate(col, editor->text());
                    });
            return editor;
        }

        // 3. 将表格里的数据填充到刚创建的编辑器中
        void setEditorData(QWidget* editor, const QModelIndex& index) const override {
            if (index.data(Role_ControlType).toInt() == ElemeneParmColumnConfig::ComboBox) {
                QComboBox* combo = qobject_cast<QComboBox*>(editor);
                if (combo) combo->setCurrentText(index.data(Qt::DisplayRole).toString());
            }
            else {
                QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
                if (edit) edit->setText(index.data(Qt::DisplayRole).toString());
            }
        }

        // 4. 编辑完成后，将编辑器里的值存回表格模型
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
            if (index.data(Role_ControlType).toInt() == ElemeneParmColumnConfig::ComboBox) {
                QComboBox* combo = qobject_cast<QComboBox*>(editor);
                if (combo) model->setData(index, combo->currentText(), Qt::DisplayRole);
            }
            else {
                QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
                if (edit) model->setData(index, edit->text(), Qt::DisplayRole);
            }
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
            editor->setGeometry(option.rect);
        }

    signals:
        // 请求对第 col 列所有选中的单元格批量更新为 newValue
        void requestBatchUpdate(int col, const QString& newValue) const;

    private:
    };

    class mElemeneParmTableWidget : public QTableWidget {
        Q_OBJECT

    public:
        explicit mElemeneParmTableWidget(QWidget* parent = nullptr);
        ~mElemeneParmTableWidget();

        // 修正：支持传入多行ID集合
        void edit(const QList<int>& ids, ElementType eletype);
        void unedit();

    protected:
        // 重写鼠标事件：在已选中的单元格上单击时不改变选区，直接触发编辑
        void mousePressEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton)
            {
                QModelIndex idx = indexAt(event->pos());
                if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
                    && !(event->modifiers() & Qt::ControlModifier)
                    && !(event->modifiers() & Qt::ShiftModifier))
                {
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
                QModelIndex idx = indexAt(event->pos());
                if (idx == m_lastPressedIndex)
                {
                    QTableWidget::edit(idx);
                }
                m_lastPressedIndex = QModelIndex();
                return;
            }
            QTableWidget::mouseReleaseEvent(event);
        }

        // 键盘输入时直接对当前选中的输入框单元格进入编辑状态
        void keyPressEvent(QKeyEvent* event) override
        {
            // 回车：结束编辑（如有）并移动到“最后一个选中单元格”的下一行同列
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            {
                // 在编辑器关闭前先保存当前选区，避免 closeEditor 后选区被清空
                QModelIndex editIdx = currentIndex();
                QList<QTableWidgetItem*> prevSelected = selectedItems();
                bool editorCommitted = false;

                QWidget* editor = indexWidget(editIdx);
                if (editor)
                {
                    commitData(editor);
                    closeEditor(editor, QAbstractItemDelegate::NoHint);
                    editorCommitted = true;
                }
                else if (state() == QAbstractItemView::EditingState)
                {
                    QTableWidget::keyPressEvent(event);
                    editorCommitted = true;
                }

                if (editorCommitted && editIdx.isValid())
                {
                    batchSyncCurrentColumn(editIdx, prevSelected);
                }

                moveToNextRowSameColumn();
                return;
            }

            if (event->text().isEmpty() || event->key() == Qt::Key_Escape
                || event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
            {
                QTableWidget::keyPressEvent(event);
                return;
            }

            QModelIndexList selected = selectedIndexes();
            if (selected.isEmpty())
            {
                QTableWidget::keyPressEvent(event);
                return;
            }

            // 以最后选中的单元格作为当前编辑单元格
            QModelIndex idx = selected.last();
            int controlType = idx.data(Role_ControlType).toInt();
            if (controlType == ElemeneParmColumnConfig::ComboBox)
            {
                QTableWidget::keyPressEvent(event);
                return;
            }

            QTableWidget::edit(idx);
            QWidget* editor = indexWidget(idx);
            QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
            if (edit)
            {
                edit->setText(event->text());
                edit->setCursorPosition(edit->text().length());
            }
        }

    signals:
        // 每一个单元格变更，都精确带上所属行的具体独立ID
        void dataUpdated(int id, const QString& dataKey, const QVariant& newValue);
        // 请求外部根据特定ID和字段获取初始数据
        QVariant requestInitialData(int id, const QString& dataKey);
        // 请求外部更新下拉框选项
        void requestComboOptions(const QString& dataKey, QStringList& outOptions);

    public slots:
        void batchUpdateSelectedCells(int col, const QString& newValue);

    private slots:
        void onCellItemChanged(QTableWidgetItem* item);

    private:
        void initTable();
        void applyHeaders();
        void buildDynamicColumns();
        void loadData(); // 核心：多行数据加载
        void setCellControl(int row, int col, const ElemeneParmColumnConfig& config, const QVariant& value);
        void moveToNextRowSameColumn();
        void batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected);

    private:
        QList<int> m_currentIds;             // 存储当前表格所有行的ID集合
        ElementType m_currentType;
        bool m_isEditing;

        QList<ElemeneParmColumnConfig> m_columnConfigs;
        QModelIndex m_lastPressedIndex;
    };
}