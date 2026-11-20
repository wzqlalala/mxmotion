#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QIntValidator>
#include <QStringList>
#include <QList>

namespace MechMainWindow
{
    class RotateACCTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit RotateACCTableWidget(QWidget* parent = nullptr);

        // 设置列头（与曲线类型对应）
        void setColumnHeaders(const QStringList& headers);

        // 获取列头
        QStringList getColumnHeaders() const;

        // 设置表格数据（按行）
        void setTableData(const QList<QStringList>& data);

        // 获取表格数据（按行）
        QList<QStringList> getTableData() const;

        // 设置行数
        void setRowCount(int count);

        // 设置指定行数据
        void setRowData(int row, const QStringList& data);

        // 为所有列设置实数输入校验（只允许输入实数）
        void setAllColumnsRealValidation();

    signals:
        void dataChanged();
        void rowAdded(int row);
        void rowRemoved(int row);

    private slots:
        void addRow();
        void removeRow();
        void clearTable();
        void setRowByInput();
        void onCellChanged(int row, int col);

    private:
        void initUI();
        void setRowInternal(int row, const QStringList& data = QStringList());

        QTableWidget* m_table;
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QLineEdit* m_editRowNum;
        QStringList m_headers;
    };
}
