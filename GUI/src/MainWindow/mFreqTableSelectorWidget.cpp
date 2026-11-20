#include "mFreqTableSelectorWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace MechMainWindow
{
    mFreqTableSelectorWidget::mFreqTableSelectorWidget(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void mFreqTableSelectorWidget::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 顶部控件（标准样式，完全一致） =====================
        QHBoxLayout* controlLayout = new QHBoxLayout;

        m_btnAdd = new QPushButton;
        m_btnRemove = new QPushButton;
        m_btnClear = new QPushButton;
        m_editRowNum = new QLineEdit;

        m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_btnClear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));

        m_btnAdd->setStyleSheet("QPushButton::hover{background:#299DFF}");
        m_btnRemove->setStyleSheet("QPushButton::hover{background:#299DFF}");
        m_btnClear->setStyleSheet("QPushButton::hover{background:#299DFF}");

        m_editRowNum->setPlaceholderText("行数");
        m_editRowNum->setFixedWidth(30);
        m_editRowNum->setValidator(new QIntValidator(1, 999));

        controlLayout->addWidget(m_btnAdd);
        controlLayout->addWidget(m_btnRemove);
        controlLayout->addWidget(m_btnClear);
        controlLayout->addWidget(m_editRowNum);
        controlLayout->addStretch(); // ✅ 这里的addStretch是对的，让按钮靠左对齐

        // ===================== 表格（1列，表头：名称） =====================
        m_table = new FreqTableSelectorEdit;
        m_table->setColumnCount(1);
        m_table->setHorizontalHeaderLabels({ "名称" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        m_table->setColumnWidth(0, 200); // 列宽和第一个频率表一致

        // 使用委托：多选后直接在单元格内编辑并同步所有选中单元格
        auto optionCallback = [this]() -> QStringList { return m_comboItems; };
        auto batchCallback = [this](const QString& value) { this->batchSetSelected(value); };
        m_table->setItemDelegate(new FreqTableSelectorComboBoxDelegate(optionCallback, batchCallback, this));

        // 设置编辑触发器：不通过默认双击/编辑键触发，而由自定义鼠标事件处理
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectItems);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 标准表格样式（完全一致） =====================
        m_table->setStyleSheet(R"(
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
    border-left:0;
    padding:4px;
    font-weight:normal;
}
QHeaderView::section:vertical
{
    background: #F7F8FA;
    border: 1px solid #dcdcdc;
    border-top:0;
    padding:4px;
    font-weight:normal;
}
)");

        // ===================== 信号槽 =====================
        connect(m_table, &FreqTableSelectorEdit::requestAddRow, this, &mFreqTableSelectorWidget::addRow);
        connect(m_btnAdd, &QPushButton::clicked, this, &mFreqTableSelectorWidget::addRow);
        connect(m_btnRemove, &QPushButton::clicked, this, &mFreqTableSelectorWidget::removeRow);
        connect(m_btnClear, &QPushButton::clicked, this, &mFreqTableSelectorWidget::clearTable);
        connect(m_editRowNum, &QLineEdit::returnPressed, this, &mFreqTableSelectorWidget::setRowByInput);

        // ===================== 布局（关键修改，只改这两行） =====================
        mainLayout->addLayout(controlLayout);
        mainLayout->addWidget(m_table, 1);
    }

    void mFreqTableSelectorWidget::setComboBoxRow(int row)
    {
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, item);
    }

    void mFreqTableSelectorWidget::setComboBoxItems(const QStringList& items)
    {
        m_comboItems = items;
        // 更新所有已存在行的下拉选项（通过委托自动获取，无需操作 cellWidget）
    }

    QStringList mFreqTableSelectorWidget::getAllSelectedNames() const
    {
        QStringList res;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            QTableWidgetItem* item = m_table->item(i, 0);
            if (item) {
                res << item->text();
            }
        }
        return res;
    }

    void mFreqTableSelectorWidget::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setComboBoxRow(row);
        emit rowAdded(row); //只发添加信号
    }

    void mFreqTableSelectorWidget::removeRow()
    {
        QList<int> rows;
        for (auto& r : m_table->selectedRanges())
        {
            for (int i = r.topRow(); i <= r.bottomRow(); ++i)
                rows << i;
        }

        // 没选中，默认删最后一行
        if (rows.isEmpty() && m_table->rowCount() > 0)
            rows << m_table->rowCount() - 1;

        // 倒序删，防止行号偏移
        std::sort(rows.rbegin(), rows.rend());
        for (int r : rows)
        {
            emit rowRemoved(r); // 每删一行，先发送被删行号，更新映射
            m_table->removeRow(r);
        }
    }

    void mFreqTableSelectorWidget::clearTable()
    {
        // 倒序发送删除信号，确保外部映射按正确索引逐行清理
        int rowCount = m_table->rowCount();
        for (int i = rowCount - 1; i >= 0; --i)
        {
            emit rowRemoved(i);
        }
        m_table->setRowCount(0);
    }

    void mFreqTableSelectorWidget::setRowByInput()
    {
        bool ok;
        int num = m_editRowNum->text().toInt(&ok);
        if (!ok || num <= 0) return;

        // 使用 addRows 增删差值行，保留已有数据并同步映射
        addRows(num);
    }

    void mFreqTableSelectorWidget::addRows(int count)
    {
        // 非法行数直接返回
        if (count < 0)
            return;

        int current = m_table->rowCount();

        // 需要增加行
        if (count > current)
        {
            for (int i = current; i < count; ++i)
            {
                addRow();
            }
        }
        // 需要删除行（从最后一行往前删，先通知映射再删UI）
        else if (count < current)
        {
            for (int i = current - 1; i >= count; --i)
            {
                emit rowRemoved(i);
                m_table->removeRow(i);
            }
        }
    }

    bool mFreqTableSelectorWidget::setCurrentText(int row, const QString& text)
    {
        // 行号越界判断
        if (row < 0 || row >= m_table->rowCount())
            return false;

        QTableWidgetItem* item = m_table->item(row, 0);
        if (!item)
            return false;

        item->setText(text);
        // 触发 comboChanged 信号，同步映射
        emit comboChanged(row, text);
        return true;
    }

    bool mFreqTableSelectorWidget::setCurrentIndex(int row, int index)
    {
        if (row < 0 || row >= m_table->rowCount())
            return false;

        QStringList options;
        options << "" << m_comboItems;
        if (index < 0 || index >= options.size())
            return false;

        return setCurrentText(row, options[index]);
    }

    // 批量设置所有选中单元格为同一值，并触发 comboChanged
    void mFreqTableSelectorWidget::batchSetSelected(const QString& value)
    {
        if (!m_table || !m_table->selectionModel()) return;

        QModelIndexList selectedIndexes = m_table->selectionModel()->selectedIndexes();

        // 批量设置文本
        for (const QModelIndex& idx : selectedIndexes)
        {
            QTableWidgetItem* item = m_table->item(idx.row(), idx.column());
            if (item) item->setText(value);
        }

        // 触发每个被修改单元格的 comboChanged 信号
        for (const QModelIndex& idx : selectedIndexes)
        {
            emit comboChanged(idx.row(), value);
        }
    }

    void mFreqTableSelectorWidget::onComboChanged(int row, const QString& text)
    {
        if (row < 0) return;
        emit comboChanged(row, text);
    }

    void mFreqTableSelectorWidget::clearTableData()
    {
        // 先阻塞整个表格model信号，防止清空乱发插入/删除信号
        m_table->model()->blockSignals(true);

        // 清空所有行
        m_table->setRowCount(0);

        // 恢复信号
        m_table->model()->blockSignals(false);
    }
}