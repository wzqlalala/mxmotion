#include "mRotateACCTableWidget.h"

#include <QIcon>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDoubleValidator>

namespace
{
    // 实数输入委托：编辑器内只允许输入实数（含科学计数法）
    class RealNumberDelegate : public QStyledItemDelegate
    {
    public:
        explicit RealNumberDelegate(QObject* parent = nullptr)
            : QStyledItemDelegate(parent)
        {
        }

        QWidget* createEditor(QWidget* parent,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(QStyledItemDelegate::createEditor(parent, option, index));
            if (edit)
            {
                QDoubleValidator* validator = new QDoubleValidator(edit);
                validator->setNotation(QDoubleValidator::ScientificNotation);
                edit->setValidator(validator);
            }
            return edit;
        }
    };
}

namespace MechMainWindow
{
    RotateACCTableWidget::RotateACCTableWidget(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void RotateACCTableWidget::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

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
        controlLayout->addStretch();

        m_table = new QTableWidget;
        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        m_table->setEditTriggers(QTableWidget::AllEditTriggers);
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

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

        connect(m_btnAdd, &QPushButton::clicked, this, &RotateACCTableWidget::addRow);
        connect(m_btnRemove, &QPushButton::clicked, this, &RotateACCTableWidget::removeRow);
        connect(m_btnClear, &QPushButton::clicked, this, &RotateACCTableWidget::clearTable);
        connect(m_editRowNum, &QLineEdit::returnPressed, this, &RotateACCTableWidget::setRowByInput);
        connect(m_table, &QTableWidget::cellChanged, this, &RotateACCTableWidget::onCellChanged);

        mainLayout->addLayout(controlLayout);
        mainLayout->addWidget(m_table);
    }

    void RotateACCTableWidget::setColumnHeaders(const QStringList& headers)
    {
        m_headers = headers;
        m_table->setColumnCount(headers.size());
        m_table->setHorizontalHeaderLabels(headers);

        for (int col = 0; col < headers.size(); ++col)
        {
            m_table->setColumnWidth(col, 220);
        }
    }

    QStringList RotateACCTableWidget::getColumnHeaders() const
    {
        return m_headers;
    }

    void RotateACCTableWidget::setTableData(const QList<QStringList>& data)
    {
        m_table->blockSignals(true);
        m_table->setRowCount(0);
        for (int i = 0; i < data.size(); ++i)
        {
            setRowInternal(i, data[i]);
        }
        m_table->blockSignals(false);
        emit dataChanged();
    }

    QList<QStringList> RotateACCTableWidget::getTableData() const
    {
        QList<QStringList> result;
        int colCount = m_table->columnCount();
        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            QStringList rowData;
            for (int col = 0; col < colCount; ++col)
            {
                QTableWidgetItem* item = m_table->item(row, col);
                rowData.append(item ? item->text() : QString());
            }
            result.append(rowData);
        }
        return result;
    }

    void RotateACCTableWidget::setRowCount(int count)
    {
        if (count < 0)
            return;

        int current = m_table->rowCount();
        if (count > current)
        {
            m_table->blockSignals(true);
            for (int i = current; i < count; ++i)
            {
                setRowInternal(i);
            }
            m_table->blockSignals(false);
        }
        else if (count < current)
        {
            m_table->blockSignals(true);
            for (int i = current - 1; i >= count; --i)
            {
                m_table->removeRow(i);
            }
            m_table->blockSignals(false);
        }
    }

    void RotateACCTableWidget::setRowData(int row, const QStringList& data)
    {
        if (row < 0 || row >= m_table->rowCount())
            return;
        setRowInternal(row, data);
    }

    void RotateACCTableWidget::setAllColumnsRealValidation()
    {
        for (int col = 0; col < m_table->columnCount(); ++col)
        {
            m_table->setItemDelegateForColumn(col, new RealNumberDelegate(m_table));
        }
    }

    void RotateACCTableWidget::setRowInternal(int row, const QStringList& data)
    {
        if (row >= m_table->rowCount())
        {
            m_table->insertRow(row);
        }

        int colCount = m_table->columnCount();
        for (int col = 0; col < colCount; ++col)
        {
            QTableWidgetItem* item = m_table->item(row, col);
            if (!item)
            {
                item = new QTableWidgetItem();
                m_table->setItem(row, col, item);
            }
            if (col < data.size())
            {
                item->setText(data[col]);
            }
        }
    }

    void RotateACCTableWidget::addRow()
    {
        int row = m_table->rowCount();
        setRowInternal(row);
        emit rowAdded(row);
        emit dataChanged();
    }

    void RotateACCTableWidget::removeRow()
    {
        QModelIndexList selectedRows = m_table->selectionModel()->selectedRows();
        if (!selectedRows.isEmpty())
        {
            std::sort(selectedRows.begin(), selectedRows.end(), [](const QModelIndex& a, const QModelIndex& b)
                {
                    return a.row() > b.row();
                });
            m_table->blockSignals(true);
            for (const QModelIndex& idx : selectedRows)
            {
                int row = idx.row();
                m_table->removeRow(row);
                emit rowRemoved(row);
            }
            m_table->blockSignals(false);
        }
        else if (m_table->rowCount() > 0)
        {
            int row = m_table->rowCount() - 1;
            m_table->blockSignals(true);
            m_table->removeRow(row);
            m_table->blockSignals(false);
            emit rowRemoved(row);
        }
        emit dataChanged();
    }

    void RotateACCTableWidget::clearTable()
    {
        m_table->blockSignals(true);
        m_table->setRowCount(0);
        m_table->blockSignals(false);
        emit rowRemoved(0);
        emit dataChanged();
    }

    void RotateACCTableWidget::setRowByInput()
    {
        bool ok;
        int num = m_editRowNum->text().toInt(&ok);
        if (!ok || num < 1) return;
        setRowCount(num);
    }

    void RotateACCTableWidget::onCellChanged(int row, int col)
    {
        emit dataChanged();
    }
}
