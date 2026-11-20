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
    class mCaseImplicitRandomVibration;
}


namespace MechMainWindow
{
    class RandomVibrationTableEdit;
    class CaseTableWidget;
    class OutputTableWidget;

    // 功率谱参数表格列定义
    enum RandomVibrationCaseColumnIndex {
        COL_WORK1 = 0,    // 关联工况1
        COL_WORK2 = 1,    // 关联工况2
        COL_X = 2,        // 功率谱实部因子
        COL_Y = 3,        // 功率谱虚部因子
        COL_POWER = 4     // 功率谱曲线
    };

    // 输出参数表格列定义
    enum RandomVibrationOutputColumnIndex
    {
        Out_Type = 0,    // 输出类型
        Radom_Type = 1,    // 随机类型
        Node_Id = 2,        // 节点编号
        Freedom_Id = 3,        // 自由度编号
    };

    // 保留多选选区并在单元格内直接编辑的表格控件
    class RandomVibrationTableEdit : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit RandomVibrationTableEdit(QWidget* parent = nullptr) : QTableWidget(parent) {}

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
            if (col == COL_WORK1 || col == COL_WORK2 || col == COL_POWER
                || col == Out_Type || col == Radom_Type)
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
    class RandomVibrationComboBoxDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using OptionCallback = std::function<QStringList(int)>;
        using BatchCallback = std::function<void(int, const QString&)>;

        explicit RandomVibrationComboBoxDelegate(OptionCallback optionCallback, BatchCallback batchCallback, QObject* parent = nullptr)
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
            if (combo) model->setData(index, combo->currentText(), Qt::DisplayRole);

            // 批量同步到所有选中的同列单元格
            if (m_batchCallback) m_batchCallback(index.column(), combo ? combo->currentText() : QString());
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            editor->setGeometry(option.rect);
        }

    private:
        OptionCallback m_optionCallback;
        BatchCallback m_batchCallback;
    };

    // 数字输入框委托：编辑完成后同步所有被选中单元格
    class RandomVibrationNumberDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using BatchCallback = std::function<void(int, const QString&)>;

        enum NumberType { Double, Int };

        explicit RandomVibrationNumberDelegate(NumberType type, BatchCallback batchCallback, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_type(type), m_batchCallback(batchCallback) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QLineEdit* edit = new QLineEdit(parent);
            edit->setText(index.data(Qt::DisplayRole).toString());
            edit->selectAll();
            if (m_type == Double) {
                edit->setValidator(new QDoubleValidator(edit));
            }
            else {
                edit->setValidator(new QIntValidator(edit));
            }
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
        NumberType m_type;
        BatchCallback m_batchCallback;
    };

    //随机振动分析面板
    class MMAINWINDOW_EXPORT mCaseRandomVibrationPanel :public mCaseSettingPanel
    {
    public:

        mCaseRandomVibrationPanel(QWidget* parent = Q_NULLPTR);

        ~mCaseRandomVibrationPanel();

        void newCasePanel() override;

        //根据数据初始化面板
        void updatePanel() override;

    private:

        CaseTableWidget* table1;
        OutputTableWidget* table2;
    };

    //功率谱参数表格类
    class CaseTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit CaseTableWidget(MechCaseData::mCaseImplicitRandomVibration* caseData, QWidget* parent = nullptr);

        //更新表格
        void updateTableWidget();

    private:
        // 初始化函数
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

        // 实时更新工况列表
        void updateWorkConditionList();
        // 实时更新功率谱曲线列表
        void updatePowerSpectrumList();
        //刷新表格所有数据
        void updateTableData();
        //新增行时设置一个单元格（文本/下拉）
        void setCellItem(int row, int col, const QString& value);
        // 下拉选项动态获取
        void updateWorkList();
        void updateFuncList();

        void connectTableSig();
        void disconnectTableSig();

    private:
        // 核心控件
        RandomVibrationTableEdit* table_;
        QPushButton* btn_add;    // + 增加行
        QPushButton* btn_del;    // - 删除当前行
        QPushButton* btn_clear;  // 清空所有行
        mCalcLineEdit* edit_row;     // 行数输入框

        QStringList m_workList;//工况名称缓存池
        QStringList m_funcList;//功率谱曲线名称缓存池

        QMetaObject::Connection m_tableChangeConn;

        //分析工况数据
        MechCaseData::mCaseImplicitRandomVibration* _caseData = nullptr;
    };



    //输出表格类
    class OutputTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputTableWidget(MechCaseData::mCaseImplicitRandomVibration* caseData, QWidget* parent = nullptr);

        //更新表格
        void updateTableWidget();

    private:
        // 初始化函数
        void initUI();          // 初始化界面布局
        void initTableHeader(); // 初始化表格表头
        void addOneRow();       // 添加一行（带控件）
        void deleteSelectedRows();// 删除当前选中行
        void clearAllRows();    // 清空所有行
        void setRowByInput();    // 根据输入框设置行数
        void slot_cellChanged(int row, int col);   // 数据改变

        // 批量设置当前列所有选中单元格，并同步数据
        void batchSetSelectedColumn(int col, const QString& value);

        // 根据列获取下拉选项
        QStringList getComboOptions(int col) const;

        //刷新表格所有数据
        void updateTableData();
        //新增行时设置一个单元格（文本/下拉）
        void setCellItem(int row, int col, const QString& value);

        void connectTableSig();
        void disconnectTableSig();

    private:

        // 核心控件
        RandomVibrationTableEdit* table_;
        QPushButton* btn_add;    // + 增加行
        QPushButton* btn_del;    // - 删除当前行
        QPushButton* btn_clear;  // 清空所有行
        mCalcLineEdit* edit_row;     // 行数输入框
        //分析工况数据
        MechCaseData::mCaseImplicitRandomVibration* _caseData = nullptr;
        QMetaObject::Connection m_tableChangeConn;

        QStringList list_work1 = { "disp", "vel","acc" };
        QStringList list_work2 = { "psd", "disp","auto" };
    };

}
