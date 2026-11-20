#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QTableWidget>
#include <QLineEdit>
#include <QIntValidator>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QItemSelectionModel>
#include <QApplication>
#include <QItemDelegate>
#include <QLineEdit>

#include "mPanelBase.h"
#include "mFreqSamplPointBase.h"
#include "mOtherFunction.h"

using namespace MechCaseData;
namespace MechMainWindow
{
    class FreqPointTableWidget;
    class FreqPointTableWidget1;
    class FreqPointTableWidget2;
    class FreqPointTableWidget3;
    class FreqPointTableWidget4;
    //FREQ面板
    class MMAINWINDOW_EXPORT mFreqSamplPointPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mFreqSamplPointPanel(QWidget* parent = nullptr);

        // 勾选状态接口
        bool isCheckBoxChecked(int index) const;
        QList<bool> getAllCheckBoxStates() const;
        void setCheckBoxChecked(int index, bool checked);
        void setAllCheckBoxStates(const QList<bool>& states);

        // 核心函数
        void createFREQ();
        void editFREQ(int freqId);

    private slots:
        // 勾选框状态改变，控制按钮启用/禁用
        void onCheckBoxToggled(bool checked, QPushButton* btn);
        // 5个按钮独立槽函数
        void onFreqClicked();
        void onFreq1Clicked();
        void onFreq2Clicked();
        void onFreq3Clicked();
        void onFreq4Clicked();
        //表格变化响应
        void onTableDataChanged();


    private:
        // 创建一行：勾选框 + 按钮
        QWidget* createRow(const QString& btnText, QPushButton*& btn, QCheckBox*& check);
        void loadTableDataFromDataClass();
        void saveTableDataToDataClass();

    private:
        //标记是否正在通过代码设置勾选状态
        bool m_isSettingCheckBoxByCode = false;

        // 5组控件
        QPushButton* m_btnFreq;
        QPushButton* m_btnFreq1;
        QPushButton* m_btnFreq2;
        QPushButton* m_btnFreq3;
        QPushButton* m_btnFreq4;

        QCheckBox* m_checkFreq;
        QCheckBox* m_checkFreq1;
        QCheckBox* m_checkFreq2;
        QCheckBox* m_checkFreq3;
        QCheckBox* m_checkFreq4;

        //表格
        FreqPointTableWidget* m_freqTableWidget = nullptr;
        FreqPointTableWidget1* m_freqTableWidget1 = nullptr;
        FreqPointTableWidget2* m_freqTableWidget2 = nullptr;
        FreqPointTableWidget3* m_freqTableWidget3 = nullptr;
        FreqPointTableWidget4* m_freqTableWidget4 = nullptr;

        //数据
        mFreqSamplPointBase* _freqSamplPointData = nullptr;
    };

    // 支持多选+单击直接编辑的表格
    class FreqMultiSelectTable : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit FreqMultiSelectTable(QWidget* parent = nullptr);

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void handleEnter();
        void batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected);

    signals:
        void requestAddRow();
    };

    // 使用 QLineEdit 作为编辑器，不限制小数位数
    class FreqLineEditDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        explicit FreqLineEditDelegate(QObject* parent = nullptr)
            : QItemDelegate(parent) {}

        QWidget* createEditor(QWidget* parent,
                              const QStyleOptionViewItem&,
                              const QModelIndex&) const override
        {
            return new QLineEdit(parent);
        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override
        {
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
            if (!lineEdit)
            {
                QItemDelegate::setEditorData(editor, index);
                return;
            }

            // 优先使用 EditRole 的 double 值，以完整精度显示
            bool ok = false;
            double value = index.data(Qt::EditRole).toDouble(&ok);
            if (ok)
            {
                lineEdit->setText(MxFunctions::mNumberToString(value, 'g', 16));
            }
            else
            {
                lineEdit->setText(index.data(Qt::EditRole).toString());
            }
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override
        {
            QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
            if (!lineEdit)
            {
                QItemDelegate::setModelData(editor, model, index);
                return;
            }

            QString text = lineEdit->text();
            bool ok = false;
            double value = text.toDouble(&ok);
            if (ok)
            {
                model->setData(index, value, Qt::EditRole);
                model->setData(index, MxFunctions::mNumberToString(value, 'g', 16), Qt::DisplayRole);
            }
            else
            {
                model->setData(index, text, Qt::EditRole);
                model->setData(index, text, Qt::DisplayRole);
            }
        }
    };

    //FREQ表格
    class FreqPointTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit FreqPointTableWidget(QWidget* parent = nullptr);

        //获取数据
        QList<double> getAllPoints() const;
        // 设置数据
        void setData(const QList<double>& data);

    private slots:
        void addRow();
        void removeRow();
        void clearTable();
        void setRowByInput();
        void onCellChanged(int row, int column);

    signals:
        // 数据变化信号
        void dataChanged();

    private:
        void initUI();
        void setDoubleItem(int row);

    private:
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QLineEdit* m_editRowNum;
        FreqMultiSelectTable* m_table;
    };
    //FREQ1表格
    class FreqPointTableWidget1 : public QWidget
    {
        Q_OBJECT
    public:
        explicit FreqPointTableWidget1(QWidget* parent = nullptr);

        // 获取所有数据（三列）
        QList<QVector<double>> getAllPoints() const;
        // 设置数据
        void setData(const QList<QVector<double>>& data);

    signals:
        // 数据变化信号
        void dataChanged();

    private slots:
        void addRow();
        void onCellChanged(int row, int column);

    private:
        void initUI();
        void setDoubleItems(int row);

    private:
        QTableWidget* m_table;
    };
    //FREQ2表格
    class FreqPointTableWidget2 : public QWidget
    {
        Q_OBJECT
    public:
        explicit FreqPointTableWidget2(QWidget* parent = nullptr);

        // 获取所有数据（三列：起始频率、频率增量、采样点总数）
        QList<QVector<double>> getAllPoints() const;
        // 设置数据
        void setData(const QList<QVector<double>>& data);

    signals:
        // 数据变化信号
        void dataChanged();

    private slots:
        void addRow();
        void onCellChanged(int row, int column);

    private:
        void initUI();
        void setDoubleItems(int row);

    private:
        QTableWidget* m_table;
    };
    //FREQ3表格
    class FreqPointTableWidget3 : public QWidget
    {
        Q_OBJECT
    public:
        explicit FreqPointTableWidget3(QWidget* parent = nullptr);

        // 获取所有数据（5列：起始频率、终止频率、加密个数、拓展量、差值类型）
        QList<QVector<double>> getAllPoints() const;
        // 设置数据
        void setData(const QList<QVector<double>>& data);

    signals:
        // 数据变化信号
        void dataChanged();

    private slots:
        void addRow();
        void onCellChanged(int row, int column);

    private:
        void initUI();
        void setRowItems(int row);

    private:
        QTableWidget* m_table;
    };
    //FREQ4表格
    class FreqPointTableWidget4 : public QWidget
    {
        Q_OBJECT
    public:
        explicit FreqPointTableWidget4(QWidget* parent = nullptr);

        // 获取所有数据（4列：下限频率、频率增量、频移量、采样点数量）
        QList<QVector<double>> getAllPoints() const;
        // 设置数据
        void setData(const QList<QVector<double>>& data);

    signals:
        // 数据变化信号
        void dataChanged();

    private slots:
        void addRow();
        void onCellChanged(int row, int column);

    private:
        void initUI();
        void setRowItems(int row);

    private:
        QTableWidget* m_table;
    };
}
