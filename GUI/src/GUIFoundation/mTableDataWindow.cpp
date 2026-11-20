#include "mTableDataWindow.h"
#include "mFramelessMessageBox.h"
#include "mGlobalSignals.h"
#include "mUiMenu.h"
#include "mTitleBar.h"
#include "mCalcLineEdit.h"
#include "mCurveWindow.h"

#include <QFont>
#include <QSet>
#include <QKeyEvent>
#include <QApplication>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QHeaderView>
#include <QVector>
#include <QSpinBox>
#include <QDebug>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include "mOtherFunction.h"

namespace MechMainWindow
{
    // ------------------------------------------------------------------
    // mTableWidget 列校验辅助函数
    // ------------------------------------------------------------------
    static bool validationTypeIsInteger(mTableDataWindow::ColumnValidationType type)
    {
        switch (type) {
        case mTableDataWindow::ColumnValidationType::Integer:
        case mTableDataWindow::ColumnValidationType::PositiveInteger:
        case mTableDataWindow::ColumnValidationType::NonNegativeInteger:
        case mTableDataWindow::ColumnValidationType::NonPositiveInteger:
        case mTableDataWindow::ColumnValidationType::NegativeInteger:
            return true;
        default:
            return false;
        }
    }

    static bool validationTypeIsReal(mTableDataWindow::ColumnValidationType type)
    {
        switch (type) {
        case mTableDataWindow::ColumnValidationType::NoValidation:
        case mTableDataWindow::ColumnValidationType::Real:
        case mTableDataWindow::ColumnValidationType::PositiveReal:
        case mTableDataWindow::ColumnValidationType::NonNegativeReal:
        case mTableDataWindow::ColumnValidationType::NonPositiveReal:
        case mTableDataWindow::ColumnValidationType::NegativeReal:
            return true;
        default:
            return false;
        }
    }

    // 实数范围校验器：输入阶段对越界值返回 Invalid（阻断），对中间态返回 Intermediate
    class RealRangeValidator : public QValidator
    {
    public:
        RealRangeValidator(mTableDataWindow::ColumnValidationType type, QObject* parent = nullptr)
            : QValidator(parent), m_type(type) {}

        State validate(QString& input, int& pos) const override
        {
            Q_UNUSED(pos)

            if (input.isEmpty())
                return Intermediate;

            // 允许中间符号
            if (input == "+" || input == "-" || input == "." || input == "+." || input == "-.")
                return Intermediate;

            // 允许小数点结尾（如 123.、0.、-0.）
            if (input.endsWith('.')) {
                QString prefix = input;
                prefix.chop(1);
                if (prefix == "+" || prefix == "-" || prefix.isEmpty())
                    return Intermediate;
                bool ok;
                prefix.toDouble(&ok);
                if (ok) return Intermediate;
            }

            // 允许科学计数法中间态（如 1e、1e-、-1e+）
            if (input.endsWith('e') || input.endsWith('E') ||
                input.endsWith("e-") || input.endsWith("E-") ||
                input.endsWith("e+") || input.endsWith("E+")) {
                QString prefix = input;
                if (prefix.endsWith('+') || prefix.endsWith('-'))
                    prefix.chop(1);
                if (prefix.endsWith('e') || prefix.endsWith('E'))
                    prefix.chop(1);
                if (prefix.isEmpty())
                    return Invalid;
                bool ok;
                prefix.toDouble(&ok);
                if (ok) return Intermediate;
            }

            // 单独的 0 / +0 / -0：对 PositiveReal / NegativeReal 不是最终合法值，
            // 但可作为中间态（如 0.5、-0.5）
            if (input == "0" || input == "+0" || input == "-0") {
                switch (m_type) {
                case mTableDataWindow::ColumnValidationType::PositiveReal:
                    // "0" 和 "+0" 可能是 0.x 的中间态；"-0" 不可能变成正数
                    return (input == "0" || input == "+0") ? Intermediate : Invalid;
                case mTableDataWindow::ColumnValidationType::NegativeReal:
                    // "-0" 可能是 -0.x 的中间态；"0" 和 "+0" 不可能变成负数
                    return (input == "-0") ? Intermediate : Invalid;
                default:
                    return Acceptable; // NonPositiveReal、NonNegativeReal、Real、NoValidation 均接受 0
                }
            }

            // 格式检查：必须是合法实数
            bool ok;
            double value = input.toDouble(&ok);
            if (!ok)
                return Invalid;

            // 对于 PositiveReal / NegativeReal，值恰好为 0.0 但输入含小数点，
            // 视为中间态（如 0.001、+0.01、-0.01 的输入过程中）
            if (value == 0.0 && input.contains('.')) {
                switch (m_type) {
                case mTableDataWindow::ColumnValidationType::PositiveReal:
                    if (input.startsWith('-'))
                        return Invalid;
                    return Intermediate;
                case mTableDataWindow::ColumnValidationType::NegativeReal:
                    if (!input.startsWith('-'))
                        return Invalid;
                    return Intermediate;
                default:
                    break;
                }
            }

            // 范围检查：越界值直接返回 Invalid，实现输入阶段阻断
            switch (m_type) {
            case mTableDataWindow::ColumnValidationType::PositiveReal:
                return value > 0.0 ? Acceptable : Invalid;
            case mTableDataWindow::ColumnValidationType::NonNegativeReal:
                return value >= 0.0 ? Acceptable : Invalid;
            case mTableDataWindow::ColumnValidationType::NonPositiveReal:
                return value <= 0.0 ? Acceptable : Invalid;
            case mTableDataWindow::ColumnValidationType::NegativeReal:
                return value < 0.0 ? Acceptable : Invalid;
            case mTableDataWindow::ColumnValidationType::NoValidation:
            case mTableDataWindow::ColumnValidationType::Real:
                return Acceptable;
            default:
                return Invalid;
            }
        }

    private:
        mTableDataWindow::ColumnValidationType m_type;
    };

    static QValidator* createValidatorForType(mTableDataWindow::ColumnValidationType type, QObject* parent)
    {
        // 整数类型使用 QIntValidator（输入时即阻断小数、字母等）
        if (validationTypeIsInteger(type)) {
            int bottom = INT_MIN;
            int top = INT_MAX;
            switch (type) {
            case mTableDataWindow::ColumnValidationType::Integer:
                bottom = INT_MIN;
                top = INT_MAX;
                break;
            case mTableDataWindow::ColumnValidationType::PositiveInteger:
                bottom = 1;
                top = INT_MAX;
                break;
            case mTableDataWindow::ColumnValidationType::NonNegativeInteger:
                bottom = 0;
                top = INT_MAX;
                break;
            case mTableDataWindow::ColumnValidationType::NonPositiveInteger:
                bottom = INT_MIN;
                top = 0;
                break;
            case mTableDataWindow::ColumnValidationType::NegativeInteger:
                bottom = INT_MIN;
                top = -1;
                break;
            default:
                break;
            }
            return new QIntValidator(bottom, top, parent);
        }

        // 实数类型：使用自定义 QValidator，输入阶段对越界值返回 Invalid 阻断
        if (validationTypeIsReal(type)) {
            return new RealRangeValidator(type, parent);
        }

        return nullptr;
    }

    // 严格数值校验（用于 setModelData / setEditorData 最终把关）
    static bool isTextValidForType(const QString& text, mTableDataWindow::ColumnValidationType type)
    {
        if (text.isEmpty())
            return true; // 允许留空

        bool ok = false;
        double value = text.toDouble(&ok);
        if (!ok)
            return false;

        switch (type) {
        case mTableDataWindow::ColumnValidationType::NoValidation:
        case mTableDataWindow::ColumnValidationType::Real:
            return true;
        case mTableDataWindow::ColumnValidationType::Integer:
            return qFuzzyCompare(value, qRound(value));
        case mTableDataWindow::ColumnValidationType::PositiveInteger:
            return value > 0.0 && qFuzzyCompare(value, qRound(value));
        case mTableDataWindow::ColumnValidationType::NonNegativeInteger:
            return value >= 0.0 && qFuzzyCompare(value, qRound(value));
        case mTableDataWindow::ColumnValidationType::NonPositiveInteger:
            return value <= 0.0 && qFuzzyCompare(value, qRound(value));
        case mTableDataWindow::ColumnValidationType::NegativeInteger:
            return value < 0.0 && qFuzzyCompare(value, qRound(value));
        case mTableDataWindow::ColumnValidationType::PositiveReal:
            return value > 0.0;
        case mTableDataWindow::ColumnValidationType::NonNegativeReal:
            return value >= 0.0;
        case mTableDataWindow::ColumnValidationType::NonPositiveReal:
            return value <= 0.0;
        case mTableDataWindow::ColumnValidationType::NegativeReal:
            return value < 0.0;
        default:
            return true;
        }
    }

    // ------------------------------------------------------------------
    // NumericItemDelegate
    // ------------------------------------------------------------------
    NumericItemDelegate::NumericItemDelegate(mTableWidget* tableWidget, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_tableWidget(tableWidget)
    {
    }

    void NumericItemDelegate::setValidationType(mTableDataWindow::ColumnValidationType type)
    {
        m_validationType = type;
    }

    mTableDataWindow::ColumnValidationType NumericItemDelegate::validationType() const
    {
        return m_validationType;
    }

    QWidget* NumericItemDelegate::createEditor(QWidget* parent,
        const QStyleOptionViewItem&,
        const QModelIndex&) const
    {
        QLineEdit* editor = new QLineEdit(parent);

        QValidator* v = createValidatorForType(m_validationType, editor);
        if (v) {
            editor->setValidator(v);
        }
        else {
            // Fallback：默认只允许实数格式
            auto* validator = new QDoubleValidator(editor);
            validator->setNotation(QDoubleValidator::ScientificNotation);
            validator->setDecimals(6);
            validator->setLocale(QLocale::C);
            editor->setValidator(validator);
        }

        return editor;
    }

    void NumericItemDelegate::setEditorData(QWidget* editor,
        const QModelIndex& index) const
    {
        auto* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit) return;

        QString text = index.data(Qt::EditRole).toString();
        // 如果当前已有内容不符合该列校验规则，则清空，防止用户直接回车保留非法值
        if (!isTextValidForType(text, m_validationType)) {
            lineEdit->clear();
        }
        else {
            lineEdit->setText(text);
        }
    }

    void NumericItemDelegate::setModelData(QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index) const
    {
        auto* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit) return;

        QString text = lineEdit->text();
        // 若设置了校验规则且最终文本不合法，则拒绝写入，保持原值
        if (m_validationType != mTableDataWindow::ColumnValidationType::NoValidation
            && !isTextValidForType(text, m_validationType)) {
            return;
        }

        // 由 mTableWidget 统一处理批量同步
        if (m_tableWidget) {
            m_tableWidget->applyValueToSelectedRanges(index.column(), text);
            return;
        }

        model->setData(index, text, Qt::EditRole);
    }

    // ------------------------------------------------------------------
    // mTableWidget
    // ------------------------------------------------------------------
    mTableWidget::mTableWidget(QWidget* parent)
        : QTableWidget(parent)
    {
        initUi();
        initActions();
        rebuildConnections(true);
    }

    void mTableWidget::initUi()
    {
        //setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        //horizontalHeader()->setStretchLastSection(true);
        //verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 10);

        // 默认至少一行（动态模式下更友好）
        ensureAtLeastOneRowForDynamic();

        //设置交替换行
        this->setAlternatingRowColors(true);

        // 美化表格样式 - 浅蓝主题
        this->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #87cefa;  /* 浅蓝色表头 */
            color: #2c3e50;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #b0d4f1;
        }
        QTableWidget {
            gridline-color: #d0e6f7;
            alternate-background-color:#ECF5FF;
            selection-background-color: #add8e6; /* 选中浅蓝 */
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

    }

    void mTableWidget::initActions()
    {
        m_ctxMenu = new mUiMenu(this);
        m_actAddRow = new QAction(tr("+ Add Row"), this);
        m_actRemoveRows = new QAction(tr("- Remove Selected Rows"), this);
        m_actClear = new QAction(tr("Clear All"), this);
        m_actSetFixed = new QAction(tr("Set Fixed Size..."), this);
        m_actSwitchToDynamic = new QAction(tr("Switch to Dynamic Size"), this);

        connect(m_actAddRow, &QAction::triggered, this, [this]() {
            addRow(currentRow() < 0 ? rowCount() : currentRow() + 1);
            });
        connect(m_actRemoveRows, &QAction::triggered, this, [this]() {
            removeSelectedRows();
            });
        connect(m_actClear, &QAction::triggered, this, [this]() {
            clearAll();
            });
        connect(m_actSetFixed, &QAction::triggered, this, [this]() {
            bool ok1 = false;
            int rows = MechMainWindow::mFramelessMessageBox::getInt(this, tr("Fixed Rows"), tr("Rows:"), qMax(1, rowCount()), 1, 100000, 1, &ok1);
            if (!ok1) return;
            //int cols = MechMainWindow::mFramelessMessageBox::getInt(this, tr("Fixed Cols"), tr("Cols:"), qMax(1, columnCount()), 1, 100000, 1, &ok2);
            //if (!ok2) return;
            setFixedRow(rows);
            });
        connect(m_actSwitchToDynamic, &QAction::triggered, this, [this]() {
            setSizeMode(SizeMode::DynamicSize);
            });
    }

    void mTableWidget::rebuildConnections(bool enable)
    {
        if (enable) {
            connect(this, &QTableWidget::itemChanged, this, &mTableWidget::onItemChanged);
        }
        else {
            disconnect(this, &QTableWidget::itemChanged, this, &mTableWidget::onItemChanged);
        }
    }

    void mTableWidget::setSizeMode(SizeMode mode)
    {
        if (m_mode == mode) return;
        m_mode = mode;

        // 互斥：FixedSize 禁止用户增删清
        if (m_mode == SizeMode::DynamicSize) {
            ensureAtLeastOneRowForDynamic();
        }
        setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        emit modeChanged(m_mode);
    }

    void mTableWidget::setFixedRow(int rows)
    {
        m_blockItemSignal = true;
        setRowCount(rows);
        //setColumnCount(cols);

        // 确保每个单元格有 item
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < this->columnCount(); ++c) {
                if (!item(r, c)) {
                    setItem(r, c, new QTableWidgetItem());
                }
            }
        }
        m_blockItemSignal = false;

        //setSizeMode(SizeMode::FixedSize);
        emit tableResized(rowCount(), columnCount());
    }

    int mTableWidget::addRow(int whereRow)
    {
        if (m_mode == SizeMode::FixedSize) {
            // 固定模式下不允许新增；静默返回 -1
            return -1;
        }
        if (columnCount() == 0)
        {
            return -1;
        }
        if (whereRow < 0 || whereRow > rowCount()) whereRow = rowCount();

        m_blockItemSignal = true;
        insertRow(whereRow);
        // 为新行创建 item
       
      
        for (int c = 0; c < columnCount(); ++c) {
                setItem(whereRow, c, new QTableWidgetItem());
        }
        m_blockItemSignal = false;

        emitRowsAddedSignal(QList<int>() << whereRow);
        emit tableResized(rowCount(), columnCount());
        return whereRow;
    }

    void mTableWidget::removeSelectedRows()
    {
        if (m_mode == SizeMode::FixedSize) {
            // 固定模式下不允许删除
            return;
        }
        QList<int> rows;
        for (const auto& idx : selectedIndexes()) rows << idx.row();
        rows = rows.toSet().values();
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        if (rows.isEmpty())
            rows.append(rowCount() - 1);

        m_blockItemSignal = true;
        for (int r : rows) {
            if (r >= 0 && r < rowCount()) 
                removeRow(r);
        }
        m_blockItemSignal = false;

        emitRowsRemovedSignal(rows);
        emit tableResized(rowCount(), columnCount());
        ensureAtLeastOneRowForDynamic();
    }

    void mTableWidget::clearAll()
    {
        if (m_mode == SizeMode::FixedSize) {
            // 固定模式下允许清空内容（但不改变结构）
            m_blockItemSignal = true;
            for (int r = 0; r < rowCount(); ++r)
                for (int c = 0; c < columnCount(); ++c)
                    if (auto it = item(r, c)) it->setText(QString());
            m_blockItemSignal = false;
            emit cleared();
            return;
        }
        // 动态模式：清空所有行，但保留列数
        int cols = columnCount();
        m_blockItemSignal = true;
        setRowCount(0);
        m_blockItemSignal = false;
        ensureAtLeastOneRowForDynamic();
        setColumnCount(cols);
        emit tableResized(rowCount(), columnCount());
        emit cleared();
    }

    void mTableWidget::clearAndReset(int rows, int cols)
    {
        m_blockItemSignal = true;
        setRowCount(0);
        setColumnCount(0);
        setColumnCount(cols);
        setRowCount(rows);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                setItem(r, c, new QTableWidgetItem());
            }
        }
        m_blockItemSignal = false;
        emit tableResized(rowCount(), columnCount());
        emit cleared();
    }

    void mTableWidget::applyValueToSelectedRanges(int sourceColumn, const QString& text)
    {
        if (sourceColumn < 0 || sourceColumn >= columnCount()) return;

        // 使用 selectedRanges 更精确地处理多选区域，与 mElemeneParmTableWidget 保持一致
        QList<QTableWidgetSelectionRange> ranges = selectedRanges();
        if (ranges.isEmpty()) {
            if (auto it = item(currentRow(), sourceColumn)) {
                it->setText(text);
                emit cellDataUpdated(currentRow(), sourceColumn, text);
            }
            return;
        }

        // 校验：只针对当前被编辑列的校验规则
        mTableDataWindow::ColumnValidationType validation = columnValidation(sourceColumn);
        if (validation != mTableDataWindow::ColumnValidationType::NoValidation && !isTextValidForType(text, validation)) {
            return;
        }

        QSet<int> updatedRows;
        {
            QSignalBlocker blocker(this);
            for (const auto& range : ranges) {
                for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                    if (row < 0 || row >= rowCount()) continue;

                    // 只同步当前被编辑列
                    QTableWidgetItem* targetItem = item(row, sourceColumn);
                    if (!targetItem) {
                        targetItem = new QTableWidgetItem();
                        setItem(row, sourceColumn, targetItem);
                    }
                    if (targetItem->text() != text) {
                        targetItem->setText(text);
                        updatedRows.insert(row);
                    }
                }
            }
        }

        for (int row : updatedRows) {
            emit cellDataUpdated(row, sourceColumn, text);
        }
    }

    void mTableWidget::setCellText(int row, int col, const QString& text)
    {
        if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount()) return;
        m_blockItemSignal = true;
        if (!item(row, col)) setItem(row, col, new QTableWidgetItem());
        item(row, col)->setText(text);
        m_blockItemSignal = false;
        emit cellDataUpdated(row, col, text);
    }

    QString mTableWidget::cellText(int row, int col) const
    {
        if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount()) return {};
        auto it = item(row, col);
        return it ? it->text() : QString();
    }

    void mTableWidget::setHorizontalHeaders(const QStringList& labels)
    {
        setColumnCount(labels.size());
        setHorizontalHeaderLabels(labels);
    }

    void mTableWidget::setVerticalHeaders(const QStringList& labels)
    {
        setRowCount(labels.size());
        setVerticalHeaderLabels(labels);
    }

    void mTableWidget::setColumnValidation(int col, mTableDataWindow::ColumnValidationType type)
    {
        if (col < 0) return;
        if (type == mTableDataWindow::ColumnValidationType::NoValidation) {
            m_columnValidations.remove(col);
            setItemDelegateForColumn(col, nullptr);
        }
        else {
            m_columnValidations[col] = type;
            NumericItemDelegate* delegate = new NumericItemDelegate(this, this);
            delegate->setValidationType(type);
            setItemDelegateForColumn(col, delegate);
        }
    }

    void mTableWidget::setColumnValidations(const QMap<int, mTableDataWindow::ColumnValidationType>& validations)
    {
        for (auto it = validations.begin(); it != validations.end(); ++it) {
            if (it.value() == mTableDataWindow::ColumnValidationType::NoValidation) {
                m_columnValidations.remove(it.key());
                setItemDelegateForColumn(it.key(), nullptr);
            }
            else {
                m_columnValidations[it.key()] = it.value();
                NumericItemDelegate* delegate = new NumericItemDelegate(this, this);
                delegate->setValidationType(it.value());
                setItemDelegateForColumn(it.key(), delegate);
            }
        }
    }

    void mTableWidget::clearColumnValidations()
    {
        for (auto it = m_columnValidations.begin(); it != m_columnValidations.end(); ++it) {
            setItemDelegateForColumn(it.key(), nullptr);
        }
        m_columnValidations.clear();
    }

    mTableDataWindow::ColumnValidationType mTableWidget::columnValidation(int col) const
    {
        return m_columnValidations.value(col, mTableDataWindow::ColumnValidationType::NoValidation);
    }

    void mTableWidget::mousePressEvent(QMouseEvent* event)
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

    void mTableWidget::mouseReleaseEvent(QMouseEvent* event)
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

    void mTableWidget::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            const int r = currentRow();
            const int c = currentColumn();

            if (r < 0 || c < 0) {
                QTableWidget::keyPressEvent(event);
                return;
            }

            // 如果正在编辑单元格，先通过委托提交当前编辑器中的值。
            // 提交后 setModelData 内部会把值同步到所有选中单元格（含当前行）。
            QWidget* editor = qApp->focusWidget();
            if (editor && editor != this) {
                commitData(editor);
                closeEditor(editor, QAbstractItemDelegate::NoHint);
            }

            if (m_mode == SizeMode::DynamicSize) {
                int nextRow = r + 1;
                if (nextRow >= rowCount()) {
                    // 没有下一行则自动新增一行
                    nextRow = addRow();
                    if (nextRow < 0) nextRow = rowCount() - 1; // 防御
                }
                setCurrentCell(nextRow, c);
                emit enterNavigated(r, nextRow, c);
                return; // 吃掉事件，不触发默认行为
            }
            else {
                // FixedSize：仅换到下一行；若无下一行则停留
                int nextRow = qMin(r + 1, rowCount() - 1);
                if (nextRow != r) {
                    setCurrentCell(nextRow, c);
                    emit enterNavigated(r, nextRow, c);
                }
                return; // 也吃掉事件
            }
        }
        QTableWidget::keyPressEvent(event);
    }

    void mTableWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        m_ctxMenu->clear();

        if (m_mode == SizeMode::DynamicSize) {
            m_ctxMenu->addAction(m_actAddRow);
            m_ctxMenu->addAction(m_actRemoveRows);
            m_ctxMenu->addAction(m_actClear);
            m_ctxMenu->addSeparator();
            m_ctxMenu->addAction(m_actSetFixed);
        }
        else {
            // 固定模式：只允许设置固定尺寸（重设）和切回动态
            m_ctxMenu->addAction(m_actSetFixed);
            m_ctxMenu->addAction(m_actSwitchToDynamic);
        }
        //m_ctxMenu->popup(event->globalPos());//取消右键菜单
    }

    void mTableWidget::onItemChanged(QTableWidgetItem* item)
    {
        if (m_blockItemSignal || !item) return;
        emit cellDataUpdated(item->row(), item->column(), item->text());
    }

    void mTableWidget::ensureAtLeastOneRowForDynamic(int atLeast)
    {
        if (m_mode != SizeMode::DynamicSize) return;
        if (rowCount() >= atLeast) return;

        m_blockItemSignal = true;
        while (rowCount() < atLeast) {
            int r = rowCount();
            insertRow(r);
            for (int c = 0; c < columnCount(); ++c) {
                setItem(r, c, new QTableWidgetItem());
            }
        }
        m_blockItemSignal = false;
    }

    void mTableWidget::emitRowsAddedSignal(const QList<int>& indexes)
    {
        if (!indexes.isEmpty()) emit rowsAdded(indexes);
    }

    void mTableWidget::emitRowsRemovedSignal(const QList<int>& indexes)
    {
        if (!indexes.isEmpty()) emit rowsRemoved(indexes);
    }

    // ------------------------------------------------------------------
    // mTableDataWindow
    // ------------------------------------------------------------------
	mTableDataWindow* mTableDataWindow::_instance = nullptr;
	mTableDataWindow* mTableDataWindow::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mTableDataWindow;
			//_instance->setTitleBarWidget(new TitleBarWidget(_instance, "表格数据", 30, 240, 243, 246));
			_instance->setObjectName("TableDock");
		}
		return _instance;
	}

    void mTableDataWindow::clearAll()
    {
        _tableWidget->clearContents();
        _tableWidget->setRowCount(0);

        emit _tableWidget->tableResized(_tableWidget->rowCount(), _tableWidget->columnCount());

        // 清空后更新曲线
        QVector<QVector<double>> xs;
        QVector<QVector<double>> ys;
        QStringList headers;
        getTableData(xs, ys, headers);
        if (_usecurve)
            mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
    }

    void mTableDataWindow::clearAndReset(int rows, int cols)
    {
        _tableWidget->clear();
        _tableWidget->setRowCount(rows);
        _tableWidget->setColumnCount(cols);

        emit _tableWidget->tableResized(_tableWidget->rowCount(), _tableWidget->columnCount());

        // 清空并重设后更新曲线
        QVector<QVector<double>> xs;
        QVector<QVector<double>> ys;
        QStringList headers;
        getTableData(xs, ys, headers);
        if (_usecurve)
            mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
    }

    void mTableDataWindow::getTableData(QVector<QVector<double>>& xs, QVector<QVector<double>>& ys)
    {
        int rows = _tableWidget->rowCount();
        int cols = _tableWidget->columnCount();

        xs.clear();
        ys.clear();

        xs.resize(cols > 1 ? cols - 1 : 1);
        ys.resize(cols > 1 ? cols - 1 : 0);

        for (int r = 0; r < rows; ++r)
        {
            double xVal = 0.0;
            bool xOk = (_tableWidget->item(r, 0) &&
                parseValidDouble(_tableWidget->item(r, 0)->text(), xVal));

            // x 无效 → 整行跳过
            if (!xOk) continue;

            if (cols == 1)
                xs[0] << xVal;

            for (int c = 1; c < cols; ++c)
            {
                double yVal = 0.0;
                bool yOk = (_tableWidget->item(r, c) &&
                    parseValidDouble(_tableWidget->item(r, c)->text(), yVal));

                if (yOk)
                {
                    xs[c - 1] << xVal;
                    ys[c - 1] << yVal;
                }
            }
        }
    }


    void mTableDataWindow::setHorizontalHeaders(const QStringList & labels)
    {
        _tableWidget->setColumnCount(labels.size());
        _tableWidget->setHorizontalHeaderLabels(labels);
        
        // 更新曲线
        QVector<QVector<double>> xs;
        QVector<QVector<double>> ys;
        QStringList headers;
        getTableData(xs, ys, headers);
        if (_usecurve)
            mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
    }

    QStringList mTableDataWindow::getTableHeader()
    {
        int rows = _tableWidget->rowCount();
        int cols = _tableWidget->columnCount();
        if (cols < 1 || rows < 1) return QStringList{};
        QStringList headers;
        for (int c = 0; c < cols; ++c) {
            QTableWidgetItem* headerItem = _tableWidget->horizontalHeaderItem(c);
            if (headerItem) {
                headers << headerItem->text();
            }
            else {
                headers << QString("Column %1").arg(c + 1);
            }
        }
        return headers;
    }

    void mTableDataWindow::setUseCurve(bool use)
    {
        _usecurve = use;
    }

    mTableWidget* mTableDataWindow::getTableWidget()
    {
        return _tableWidget;
    }

    void mTableDataWindow::setColumnValidation(int col, ColumnValidationType type)
    {
        if (_tableWidget)
            _tableWidget->setColumnValidation(col, type);
    }

    void mTableDataWindow::setColumnValidations(const QMap<int, ColumnValidationType>& validations)
    {
        if (_tableWidget)
            _tableWidget->setColumnValidations(validations);
    }

    void mTableDataWindow::clearColumnValidations()
    {
        if (_tableWidget)
            _tableWidget->clearColumnValidations();
    }

    void mTableDataWindow::setTableData(const QVector<QVector<double>>& xs,
        const QVector<QVector<double>>& ys)
    {
        if (xs.size() != ys.size())
            return;

        int seriesCount = xs.size();

        QSet<double> xSet;
        QVector<double> mergedX;
        // 1. 合并所有X，去重并排序
        for (int i = 0; i < seriesCount; ++i)
        {
            int n = std::min(xs[i].size(), ys[i].size());
            for (int j = 0; j < n; ++j)
            {
                xSet.insert(xs[i][j]);
            }
        }
        mergedX = xSet.values().toVector();
        std::sort(mergedX.begin(), mergedX.end());

        int rows = mergedX.size();
        int cols = seriesCount + 1; // 第0列是X，其余是Y
        _tableWidget->rebuildConnections(false);
        // 2. 清空并重建表格
        _tableWidget->clear();
        _tableWidget->setRowCount(rows);
        _tableWidget->setColumnCount(cols);

        // 设置表头
        //QStringList headers;
        //headers << "X";
        //for (int i = 0; i < seriesCount; ++i)
            //headers << QString("Y%1").arg(i + 1);
        //_tableWidget->setHorizontalHeaderLabels(headers);

        // 3. 填充X列
        for (int r = 0; r < rows; ++r)
        {
            _tableWidget->setItem(r, 0, new QTableWidgetItem(MxFunctions::mNumberToString(mergedX[r])));
        }

        // 4. 填充Y列
        for (int i = 0; i < seriesCount; ++i)
        {
            int n = std::min(xs[i].size(), ys[i].size());
            // 建立map，x -> y
            QMap<double, double> xyMap;
            for (int j = 0; j < n; ++j)
            {
                xyMap[xs[i][j]] = ys[i][j];
            }

            for (int r = 0; r < rows; ++r)
            {
                double xVal = mergedX[r];
                if (xyMap.contains(xVal))
                {
                    _tableWidget->setItem(r, i + 1, new QTableWidgetItem(MxFunctions::mNumberToString(xyMap[xVal])));
                }
                else
                {
                    _tableWidget->setItem(r, i + 1, new QTableWidgetItem("")); // 跳点显示为空
                }
            }
        }
        _tableWidget->rebuildConnections(true);

        emit _tableWidget->tableResized(rows, cols);

        // 更新曲线
        QStringList headers;

        headers = getHorizontalHeaders();
        if (_usecurve)
            mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
    }

    void mTableDataWindow::setTableData(const QVector<QVector<double>>& datas)
    {
        if (datas.size() == 0)
        {
            return;
        }
        int rows = datas[0].size();
        int cols = datas.size();
        _tableWidget->rebuildConnections(false);
        _tableWidget->clear();
        _tableWidget->setRowCount(rows);
        _tableWidget->setColumnCount(cols);

        for (int c = 0; c < cols; ++c)
        {
            for (int r = 0; r < rows; ++r)
            {
                _tableWidget->setItem(r, c, new QTableWidgetItem(MxFunctions::mNumberToString(datas[c][r])));
            }
        }
        _tableWidget->rebuildConnections(true);

        emit _tableWidget->tableResized(rows, cols);
    }


    mTableDataWindow::mTableDataWindow(QWidget* parent) :QWidget(parent)
	{
		this->setStyleSheet("background-color: rgb(255,255,255)");

        _scrollArea = new QScrollArea(this);
        _scrollArea->setWidgetResizable(true);
        _scrollArea->setFrameShape(QFrame::NoFrame);
        _scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // 只要垂直条
        _scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        _customWidget = nullptr;
        _mainLayout = new QVBoxLayout(this);
        _tableWidget = new mTableWidget();
        _tableWidget->setItemDelegate(new NumericItemDelegate(_tableWidget, _tableWidget));
        //_tableWidget->setHorizontalHeaders({ "x", "y1", "y2"});//測試表頭
        //_tableWidget->setVerticalHeaders({ "1", "2" });
        
        _scrollArea->setWidget(_tableWidget);

        // === dynamic controls ===
        _dynamicControls = new QWidget();
        auto dynLayout = new QHBoxLayout(_dynamicControls);
        dynLayout->setContentsMargins(0, 0, 0, 0);

        auto m_addBtn = new QPushButton("", _dynamicControls);
        auto m_removeBtn = new QPushButton("", _dynamicControls);
        auto m_clearBtn = new QPushButton("", _dynamicControls);
        m_addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_removeBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        QIcon icon1(":/mMainWidgets/Icon/shanchu3x.png");
        m_clearBtn->setIcon(icon1);
        m_clearBtn->setIconSize(QSize(17, 17));
        m_addBtn->setStyleSheet("QPushButton::hover{background:#299DFF}");
        m_removeBtn->setStyleSheet("QPushButton::hover{background:#299DFF}");
        m_clearBtn->setStyleSheet("QPushButton::hover{background:#299DFF}");
        _rowInput = new MxFunctions::mCalcLineEdit("0", _dynamicControls, false, true);
        //auto _rowInput = new QLineEdit(m_dynamicControls);
        _rowInput->setFixedWidth(30);
        _rowInput->setPlaceholderText("行数");
        //_rowInput->setButtonSymbols(QAbstractSpinBox::NoButtons);

        dynLayout->addWidget(m_addBtn);
        dynLayout->addWidget(m_removeBtn);
        dynLayout->addWidget(m_clearBtn);

        dynLayout->addWidget(_rowInput);
        dynLayout->addStretch();
        _mainLayout->addWidget(_dynamicControls);
        connect(m_addBtn, &QPushButton::clicked, _tableWidget, [this]() {
            _tableWidget->addRow(_tableWidget->currentRow() < 0 ? _tableWidget->rowCount() : _tableWidget->currentRow() + 1);
            });
        connect(m_removeBtn, &QPushButton::clicked, _tableWidget, [this]() {
            _tableWidget->removeSelectedRows();
            QVector<QVector<double>> xs;
            QVector<QVector<double>> ys;
            QStringList headers;
            getTableData(xs, ys, headers);
            if(_usecurve && xs.size() > 0 && ys.size() > 0)
                mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
            emit tableDataChangedSig(xs, ys, _title);
            });
        connect(m_clearBtn, &QPushButton::clicked, _tableWidget, [this]() {
            _tableWidget->clearAll();
            QVector<QVector<double>> xs;
            QVector<QVector<double>> ys;
            QStringList headers;
            getTableData(xs, ys, headers);
            if (_usecurve && xs.size() > 0 && ys.size() > 0)
                mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
            emit tableDataChangedSig(xs, ys, _title);
            });
        connect(_rowInput, &QLineEdit::editingFinished, _tableWidget, [this]() {
            _tableWidget->setFixedRow(_rowInput->text().toInt());
            QVector<QVector<double>> xs;
            QVector<QVector<double>> ys;
            QStringList headers;
            getTableData(xs, ys, headers);
            if (_usecurve && xs.size() > 0 && ys.size() > 0)
                mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
            emit tableDataChangedSig(xs, ys, _title);
            });

        connect(this->_tableWidget, &mTableWidget::cellDataUpdated,
            [this](int row, int col, const QString& text) {

                qDebug() << "Cell updated:" << row << col << text;
                QVector<QVector<double>> xs;
                QVector<QVector<double>> ys;
                QStringList headers;
                getTableData(xs, ys, headers);
                if (_usecurve && xs.size() > 0 && ys.size() > 0)
                    mCurveWindow::getInstance()->updateMultiCurves(xs, ys, LineChart, headers, true, true);
                emit tableDataChangedSig(xs, ys, _title);
            });

        connect(_tableWidget, &mTableWidget::modeChanged,
            [this](mTableWidget::SizeMode mode) {
                _dynamicControls->setVisible(mode == mTableWidget::SizeMode::DynamicSize);
            });

        connect(_tableWidget, &mTableWidget::tableResized,
            [this](int rows, int cols) {
                Q_UNUSED(cols)
                syncRowInputToTable();
            });

        // 初始化行数输入控件
        syncRowInputToTable();

        _mainLayout->addWidget(_scrollArea);
        //_mainLayout->addStretch();

        // 启动时默认不显示行（mTableWidget 构造时会补一行空行，这里清掉）
        _tableWidget->setRowCount(0);
        emit _tableWidget->tableResized(0, _tableWidget->columnCount());

		//切换界面语言
		connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
	}

    void mTableDataWindow::syncRowInputToTable()
    {
        if (!_rowInput || !_tableWidget)
            return;
        _rowInput->setExpression(MxFunctions::mNumberToString(_tableWidget->rowCount()));
    }

    void mTableDataWindow::getTableData(QVector<QVector<double>>& x, QVector<QVector<double>>& ys, QStringList& headers)
    {
        if (!_tableWidget) return;

        x.clear();
        ys.clear();
        headers.clear();

        int rows = _tableWidget->rowCount();
        int cols = _tableWidget->columnCount();

        if (rows < 1) return;

        // 获取横向y表头
        for (int c = 1; c < cols; ++c) {
            QTableWidgetItem* headerItem = _tableWidget->horizontalHeaderItem(c);
            if (headerItem) {
                headers << headerItem->text();
            }
            else {
                headers << QString("Column %1").arg(c + 1);
            }
        }

        getTableData(x, ys);
    }

    bool mTableDataWindow::parseValidDouble(const QString& text, double& value)
    {
        QString s = text.trimmed();
        if (s.isEmpty())
            return false;

        bool ok = false;
        value = s.toDouble(&ok);

        if (!ok)
            return false;

        // 支持: 整数, 小数, 科学计数法
        // 例子: -123, +45.67, 8.9e10, -1.23E-4
        QRegExp regex("^[-+]?\\d*(\\.\\d+)?([eE][-+]?\\d+)?$");
        return regex.exactMatch(s);
    }

    QStringList mTableDataWindow::getHorizontalHeaders()
    {
        QStringList headers;

        int rows = _tableWidget->rowCount();
        int cols = _tableWidget->columnCount();

        if (cols < 1 || rows < 1) return headers;

        // 获取横向y表头
        for (int c = 1; c < cols; ++c) {
            QTableWidgetItem* headerItem = _tableWidget->horizontalHeaderItem(c);
            if (headerItem) {
                headers << headerItem->text();
            }
            else {
                headers << QString("Column %1").arg(c + 1);
            }
        }
        return headers;
    }

	void mTableDataWindow::slot_changeLanguage(int language)
	{
		//if (language == 0)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("表格数据"));
		//}
		//else if (language == 1)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("CureveData"));
		//}
	}

	mTableDataWindow::~mTableDataWindow()
	{
        if (_tableWidget)
        {
            delete _tableWidget;
            _tableWidget = nullptr;
        }
        if (_mainLayout)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
	}
    void mTableDataWindow::setCustomWidget(QWidget* widget)
    {
        if (_tableWidget)
        {
            _scrollArea->setParent(nullptr);
            _mainLayout->removeWidget(_scrollArea);
            _scrollArea->hide();
        }
        _dynamicControls->hide();
        _customWidget = widget;
        _mainLayout->addWidget(_customWidget);
        _customWidget->show();
    }
    void mTableDataWindow::resetInitialTable()
    {
        if (_customWidget)
        {
            _customWidget->setParent(nullptr);
            _mainLayout->removeWidget(_customWidget);
            _customWidget->hide();
        }
        _tableWidget->setSizeMode(mTableWidget::SizeMode::DynamicSize);
        _dynamicControls->setVisible(_tableWidget->sizeMode() == mTableWidget::SizeMode::DynamicSize);
        _customWidget = nullptr;
        _mainLayout->addWidget(_scrollArea);
        _scrollArea->show();

        // 重置表格时取消所有列的校验和输入限制
        clearColumnValidations();
    }
    void mTableDataWindow::setTableTitle(const QString& title)
    {
        _title = title;
        if (_usecurve)
            mCurveWindow::getInstance()->setPlotTitle(_title);
    }
    QString mTableDataWindow::getTableTitle()
    {
        return _title;
    }
}
