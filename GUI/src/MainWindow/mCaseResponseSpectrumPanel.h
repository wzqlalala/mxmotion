#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>
#include <functional>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QToolButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QList>
#include <QPointF>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QTimer>
#include <QItemSelectionModel>
#include <QApplication>

#include "mCalcLineEdit.h"
#include "mCaseSettingPanel.h"

#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;

namespace MechCaseData
{
    class mCaseImplicitResponseSpectrum;
}
namespace MechMainWindow
{
    class ResponseSpectrumTableEdit;
    class ResponseSpectrumTableWidget;

    // 列定义（方便维护）
    enum ResponseSpectrumColumnIndex {
        column_name = 0,    // 谱载荷名称
        column_scale = 1,    // 放大系数
        column_x1 = 2,        // 激励矢量
        column_x2 = 3,        // 激励矢量
        column_x3 = 4,        // 激励矢量
    };

    // 自定义角色：标记单元格控件类型（保留扩展）
    // 注意：常量名已加 RS_ 前缀，避免与 mElemeneParmTableWidget.h 中的同名枚举器冲突
    enum ResponseSpectrumItemRole {
        RS_Role_ControlType = Qt::UserRole + 10,
    };

    // 保留多选选区并在单元格内直接编辑的表格控件
    class ResponseSpectrumTableEdit : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit ResponseSpectrumTableEdit(QWidget* parent = nullptr) : QTableWidget(parent) {}

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

        // 键盘输入时直接对当前选中的输入框单元格进入编辑状态
        void keyPressEvent(QKeyEvent* event) override
        {
            // 回车：结束编辑（如有）并移动到“最后一个选中单元格”的下一行同列，
            // 若已在最后一行，则请求外部添加新行并进入新行
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
            int col = idx.column();
            // 下拉框列不支持键盘直接输入
            if (col == column_name || col == column_x1 || col == column_x2 || col == column_x3)
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
    class ResponseSpectrumComboBoxDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using OptionCallback = std::function<QStringList(int)>;
        using BatchCallback = std::function<void(int, const QString&)>;

        explicit ResponseSpectrumComboBoxDelegate(OptionCallback optionCallback, BatchCallback batchCallback, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_optionCallback(optionCallback), m_batchCallback(batchCallback) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QComboBox* combo = new QComboBox(parent);
            if (m_optionCallback)
                combo->addItems(m_optionCallback(index.column()));
            combo->setCurrentText(index.data(Qt::DisplayRole).toString());

            // 多选下拉框：创建时立即展开，选择任意项时同步到所有选中单元格
            QTimer::singleShot(0, combo, &QComboBox::showPopup);
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, [this, index](const QString& text) {
                        if (m_batchCallback) m_batchCallback(index.column(), text);
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
            if (m_batchCallback) m_batchCallback(index.column(), text);
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            editor->setGeometry(option.rect);
        }

    private:
        OptionCallback m_optionCallback;
        BatchCallback m_batchCallback;
    };

    // 输入框委托：编辑完成后（或键盘输入）同步所有被选中单元格
    class ResponseSpectrumLineEditDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using BatchCallback = std::function<void(int, const QString&)>;

        explicit ResponseSpectrumLineEditDelegate(BatchCallback batchCallback, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_batchCallback(batchCallback) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QLineEdit* edit = new QLineEdit(parent);
            edit->setText(index.data(Qt::DisplayRole).toString());
            edit->selectAll();
            connect(edit, &QLineEdit::editingFinished,
                    this, [this, edit, index]() {
                        if (m_batchCallback) m_batchCallback(index.column(), edit->text());
                    });
            return edit;
        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
            if (edit)
            {
                edit->setText(index.data(Qt::DisplayRole).toString());
                edit->selectAll();
            }
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
            QString text = edit ? edit->text() : QString();
            if (edit) model->setData(index, text, Qt::DisplayRole);

            // 批量同步到所有选中的同列单元格
            if (m_batchCallback) m_batchCallback(index.column(), text);
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            editor->setGeometry(option.rect);
        }

    private:
        BatchCallback m_batchCallback;
    };


    //响应谱工况分析面板
    class MMAINWINDOW_EXPORT mCaseResponseSpectrumPanel :public mCaseSettingPanel
    {
        Q_OBJECT
    public:

        mCaseResponseSpectrumPanel(QWidget* parent = Q_NULLPTR);

        ~mCaseResponseSpectrumPanel();

        void newCasePanel() override;

        //根据数据初始化面板
        void updatePanel() override;

    private:

        ResponseSpectrumTableWidget* _table;
    };


    //响应谱激励数表格
    class  ResponseSpectrumTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit ResponseSpectrumTableWidget(MechCaseData::mCaseImplicitResponseSpectrum* caseData, QWidget* parent = nullptr);

        //更新表格
        void updateTableWidget();
    private:

        // 初始化函数
        // 实时更新载荷列表
        void updateLoadList();
        void initUI();          // 初始化界面布局
        void initTableHeader(); // 初始化表格表头
        void addOneRow();       // 添加一行（带控件）
        void deleteSelectedRows();// 删除当前选中行
        void clearAllRows();    // 清空所有行
        void setRowByInput();    // 根据输入框设置行数

        // 单元格变更后同步到底层数据
        void slot_cellChanged(int row, int col);

        // 批量设置当前列所有选中单元格，并同步数据
        void batchSetSelectedColumn(int col, const QString& value);

        // 根据列获取下拉选项
        QStringList getComboOptions(int col) const;

        // 下拉选项刷新后，若当前单元格值不再列表中，则默认选第一个
        void refreshComboOptions(int col, const QStringList& newOptions);

        //刷新表格所有数据
        void updateTableData();

        // 新增行时设置一个单元格（文本/下拉）
        void setCellItem(int row, int col, const QString& value);

        // 下拉选项动态获取
        void updateNameList();

        void connectTableSig();
        void disconnectTableSig();

        // 核心控件
        ResponseSpectrumTableEdit* table_;
        QPushButton* btn_add;    // + 增加行
        QPushButton* btn_del;    // - 删除当前行
        QPushButton* btn_clear;  // 清空所有行
        mCalcLineEdit* edit_row;     // 行数输入框

        QStringList m_loadList;//载荷名称缓存池

        QMetaObject::Connection m_tableChangeConn;
        //响应谱工况数据
        MechCaseData::mCaseImplicitResponseSpectrum* _caseData = nullptr;

    };



}
