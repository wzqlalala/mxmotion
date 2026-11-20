#pragma execution_character_set("utf-8")

#include "mFreqSamplPointPanel.h"
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "mMechGlobalSignals.h"

#include "mFreqSamplPointManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mTreeView.h"
#include "mBrowserWindow.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
/**********************************************面板*************************************************/
    mFreqSamplPointPanel::mFreqSamplPointPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        m_checkFreq = nullptr;
        m_checkFreq1 = nullptr;
        m_checkFreq2 = nullptr;
        m_checkFreq3 = nullptr;
        m_checkFreq4 = nullptr;

        // 设置面板标题
        setTitle("频率设置面板");

        // 主布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        // 添加5行
        mainLayout->addWidget(createRow("FREQ", m_btnFreq, m_checkFreq));
        mainLayout->addWidget(createRow("FREQ1", m_btnFreq1, m_checkFreq1));
        mainLayout->addWidget(createRow("FREQ2", m_btnFreq2, m_checkFreq2));
        mainLayout->addWidget(createRow("FREQ3", m_btnFreq3, m_checkFreq3));
        mainLayout->addWidget(createRow("FREQ4", m_btnFreq4, m_checkFreq4));
        // 底部弹簧
        mainLayout->addStretch();

        // ========== 绑定所有按钮的点击信号到槽 ==========
        connect(m_btnFreq, &QPushButton::clicked, this, &mFreqSamplPointPanel::onFreqClicked);
        connect(m_btnFreq1, &QPushButton::clicked, this, &mFreqSamplPointPanel::onFreq1Clicked);
        connect(m_btnFreq2, &QPushButton::clicked, this, &mFreqSamplPointPanel::onFreq2Clicked);
        connect(m_btnFreq3, &QPushButton::clicked, this, &mFreqSamplPointPanel::onFreq3Clicked);
        connect(m_btnFreq4, &QPushButton::clicked, this, &mFreqSamplPointPanel::onFreq4Clicked);

        m_freqTableWidget = new FreqPointTableWidget();
        m_freqTableWidget1 = new FreqPointTableWidget1();
        m_freqTableWidget2 = new FreqPointTableWidget2();
        m_freqTableWidget3 = new FreqPointTableWidget3();
        m_freqTableWidget4 = new FreqPointTableWidget4();

        //表格数据变化响应
        connect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);

    }

    void mFreqSamplPointPanel::createFREQ()
    {
        //创建FREQ数据
        if (!getCVM)
        {
            return;
        }

        {
            // 先断开所有表格信号
            disconnect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);

            // 清空所有表格
            m_freqTableWidget->setData({});
            m_freqTableWidget1->setData({});
            m_freqTableWidget2->setData({});
            m_freqTableWidget3->setData({});
            m_freqTableWidget4->setData({});

            // 清空所有勾选框（设标记避免触发保存）
            m_isSettingCheckBoxByCode = true;
            if (m_checkFreq) m_checkFreq->setChecked(false);
            if (m_checkFreq1) m_checkFreq1->setChecked(false);
            if (m_checkFreq2) m_checkFreq2->setChecked(false);
            if (m_checkFreq3) m_checkFreq3->setChecked(false);
            if (m_checkFreq4) m_checkFreq4->setChecked(false);
            m_isSettingCheckBoxByCode = false;

            // 重置表格窗口
            mTableDataWindow::getInstance()->resetInitialTable();
        }

        //新建
        int id = getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).createID();
        _freqSamplPointData = new mFreqSamplPointBase(id, getCVM->getModelData()->createNameByType(Physical_FreqSamplPoint, "Freq"));
        getCVM->getModelData()->addPhysicalField<mFreqSamplPointBase>(_freqSamplPointData);

        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建采样频率点 %1").arg(_freqSamplPointData->getFreqSamplPointName()), true);
        //发送创建完成信号
        emit MechCommon::mMechGlobalSignals::getInstance()->updateFreqSamplPointTree(true, _freqSamplPointData->getFreqSamplPointId());

        setAllCheckBoxStates(_freqSamplPointData->_checked);
        loadTableDataFromDataClass();

        connect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
    }

    void mFreqSamplPointPanel::editFREQ(int freqId)
    {
        if (_freqSamplPointData)
        {
            saveTableDataToDataClass(); // 强制保存旧数据到旧的数据类
        }

        {
            // 临时断开表格信号，避免清空时触发保存
            disconnect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            disconnect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);

            // 清空表格
            m_freqTableWidget->setData({});
            m_freqTableWidget1->setData({});
            m_freqTableWidget2->setData({});
            m_freqTableWidget3->setData({});
            m_freqTableWidget4->setData({});

            // 清空所有勾选框（先设标记，避免触发保存）
            m_isSettingCheckBoxByCode = true;
            if (m_checkFreq) m_checkFreq->setChecked(false);
            if (m_checkFreq1) m_checkFreq1->setChecked(false);
            if (m_checkFreq2) m_checkFreq2->setChecked(false);
            if (m_checkFreq3) m_checkFreq3->setChecked(false);
            if (m_checkFreq4) m_checkFreq4->setChecked(false);
            m_isSettingCheckBoxByCode = false;

            // 重新连接信号
            connect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            connect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            connect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            connect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
            connect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);

            // 重置表格窗口
            mTableDataWindow::getInstance()->resetInitialTable();
        }

        MechCaseData::mFreqSamplPointBase* newData =
            getCVM->getModelData()->getManager<MechCaseData::mFreqSamplPointManager>(Physical_FreqSamplPoint).getFreqSamplPointData(freqId);
        if (!newData) return;
        _freqSamplPointData = newData;
        setAllCheckBoxStates(_freqSamplPointData->_checked);
        loadTableDataFromDataClass();

        connect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
    }

    QWidget* mFreqSamplPointPanel::createRow(const QString& btnText, QPushButton*& btn, QCheckBox*& check)
    {
        QWidget* w = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);

        check = new QCheckBox("启用", this);
        check->setChecked(false);

        btn = new QPushButton(btnText, this);
        btn->setDisabled(true);
        btn->setMinimumWidth(110);

        // 关联：勾选框控制按钮
        connect(check, &QCheckBox::toggled, this, [=](bool checked) {
            onCheckBoxToggled(checked, btn);
            });

        layout->addWidget(check);
        layout->addWidget(btn);
        layout->addStretch();

        return w;
    }

    void mFreqSamplPointPanel::onCheckBoxToggled(bool checked, QPushButton* btn)
    {
        if (!btn) return;
        btn->setEnabled(checked); //可用or置灰
        if (m_isSettingCheckBoxByCode) return;

        if (checked) //如果可用则点击一次切换到相应表格
            btn->click();
        if(!checked) //如果不可用，则关闭表格
            mTableDataWindow::getInstance()->resetInitialTable();

        if (_freqSamplPointData)
        {
            _freqSamplPointData->_checked = getAllCheckBoxStates();
            emit MechCommon::mMechGlobalSignals::getInstance()->updateFreqSamplPointTree(false, _freqSamplPointData->getFreqSamplPointId());
        }
    }

    // ===================== 5个按钮独立槽函数 =====================
    void mFreqSamplPointPanel::onFreqClicked()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_freqTableWidget);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mFreqSamplPointPanel::onFreq1Clicked()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_freqTableWidget1);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mFreqSamplPointPanel::onFreq2Clicked()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_freqTableWidget2);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mFreqSamplPointPanel::onFreq3Clicked()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_freqTableWidget3);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mFreqSamplPointPanel::onFreq4Clicked()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(m_freqTableWidget4);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mFreqSamplPointPanel::onTableDataChanged()
    {
        saveTableDataToDataClass();
    }

    void mFreqSamplPointPanel::loadTableDataFromDataClass()
    {
        if (!_freqSamplPointData) return;

        // 先断开信号，避免加载时触发保存
        disconnect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        disconnect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        disconnect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        disconnect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        disconnect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);

        // FREQ表格
        {
            QList<double> tableData;
            for (const auto& item : _freqSamplPointData->_freqSamplPoint)
            {
                tableData << item;
            }
            m_freqTableWidget->setData(tableData);
        }

        // FREQ1表格
        {
            QList<QVector<double>> tableData;
            QVector<double> row;
            row << _freqSamplPointData->_freqSamplPoint1.startFreq;
            row << _freqSamplPointData->_freqSamplPoint1.stepFreq;
            row << (double)_freqSamplPointData->_freqSamplPoint1.intervalNum;
            tableData << row;
            m_freqTableWidget1->setData(tableData);
        }

        // FREQ2表格
        {
            QList<QVector<double>> tableData;
            QVector<double> row;
            row << _freqSamplPointData->_freqSamplPoint2.startFreq;
            row << _freqSamplPointData->_freqSamplPoint2.stepFreq;
            row << (double)_freqSamplPointData->_freqSamplPoint2.totalPoints;
            tableData << row;
            m_freqTableWidget2->setData(tableData);
        }

        // FREQ3表格
        {
            QList<QVector<double>> tableData;
            QVector<double> row;
            row << _freqSamplPointData->_freqSamplPoint3.startFreq;
            row << _freqSamplPointData->_freqSamplPoint3.endFreq;
            row << (double)_freqSamplPointData->_freqSamplPoint3.refineNum;
            row << _freqSamplPointData->_freqSamplPoint3.expandValue;
            row << (double)_freqSamplPointData->_freqSamplPoint3.diffType;
            tableData << row;
            m_freqTableWidget3->setData(tableData);
        }

        // FREQ4表格
        {
            QList<QVector<double>> tableData;
            QVector<double> row;
            row << _freqSamplPointData->_freqSamplPoint4.lowerFreq;
            row << _freqSamplPointData->_freqSamplPoint4.stepFreq;
            row << _freqSamplPointData->_freqSamplPoint4.shiftValue;
            row << (double)_freqSamplPointData->_freqSamplPoint4.sampleNum;
            tableData << row;
            m_freqTableWidget4->setData(tableData);
        }

        // 重新连接信号
        connect(m_freqTableWidget, &FreqPointTableWidget::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget1, &FreqPointTableWidget1::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget2, &FreqPointTableWidget2::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget3, &FreqPointTableWidget3::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
        connect(m_freqTableWidget4, &FreqPointTableWidget4::dataChanged, this, &mFreqSamplPointPanel::onTableDataChanged);
    }

    void mFreqSamplPointPanel::saveTableDataToDataClass()
    {
        if (!_freqSamplPointData) return;

        // FREQ表格
        {
            QList<double> tableData = m_freqTableWidget->getAllPoints();
            _freqSamplPointData->_freqSamplPoint.clear();
            for (double val : tableData)
            {
                _freqSamplPointData->_freqSamplPoint.append(val);
            }
        }

        // FREQ1表格
        {
            QList<QVector<double>> tableData = m_freqTableWidget1->getAllPoints();
            if (!tableData.isEmpty() && tableData[0].size() >= 3)
            {
                _freqSamplPointData->_freqSamplPoint1.startFreq = tableData[0][0];
                _freqSamplPointData->_freqSamplPoint1.stepFreq = tableData[0][1];
                _freqSamplPointData->_freqSamplPoint1.intervalNum = (int)tableData[0][2];
            }
        }

        // FREQ2表格
        {
            QList<QVector<double>> tableData = m_freqTableWidget2->getAllPoints();
            if (!tableData.isEmpty() && tableData[0].size() >= 3)
            {
                _freqSamplPointData->_freqSamplPoint2.startFreq = tableData[0][0];
                _freqSamplPointData->_freqSamplPoint2.stepFreq = tableData[0][1];
                _freqSamplPointData->_freqSamplPoint2.totalPoints = (int)tableData[0][2];
            }
        }

        // FREQ3表格
        {
            QList<QVector<double>> tableData = m_freqTableWidget3->getAllPoints();
            if (!tableData.isEmpty() && tableData[0].size() >= 5)
            {
                _freqSamplPointData->_freqSamplPoint3.startFreq = tableData[0][0];
                _freqSamplPointData->_freqSamplPoint3.endFreq = tableData[0][1];
                _freqSamplPointData->_freqSamplPoint3.refineNum = (int)tableData[0][2];
                _freqSamplPointData->_freqSamplPoint3.expandValue = tableData[0][3];
                _freqSamplPointData->_freqSamplPoint3.diffType = (int)tableData[0][4];
            }
        }

        // FREQ4表格
        {
            QList<QVector<double>> tableData = m_freqTableWidget4->getAllPoints();
            if (!tableData.isEmpty() && tableData[0].size() >= 4)
            {
                _freqSamplPointData->_freqSamplPoint4.lowerFreq = tableData[0][0];
                _freqSamplPointData->_freqSamplPoint4.stepFreq = tableData[0][1];
                _freqSamplPointData->_freqSamplPoint4.shiftValue = tableData[0][2];
                _freqSamplPointData->_freqSamplPoint4.sampleNum = (int)tableData[0][3];
            }
        }
    }

    bool mFreqSamplPointPanel::isCheckBoxChecked(int index) const
    {
        const QCheckBox* check = nullptr;
        switch (index) {
        case 0: check = m_checkFreq; break;
        case 1: check = m_checkFreq1; break;
        case 2: check = m_checkFreq2; break;
        case 3: check = m_checkFreq3; break;
        case 4: check = m_checkFreq4; break;
        default: return false;
        }
        return check ? check->isChecked() : false;
    }

    QList<bool> mFreqSamplPointPanel::getAllCheckBoxStates() const
    {
        QList<bool> states;
        states << (m_checkFreq ? m_checkFreq->isChecked() : false);
        states << (m_checkFreq1 ? m_checkFreq1->isChecked() : false);
        states << (m_checkFreq2 ? m_checkFreq2->isChecked() : false);
        states << (m_checkFreq3 ? m_checkFreq3->isChecked() : false);
        states << (m_checkFreq4 ? m_checkFreq4->isChecked() : false);
        return states;
    }

    void mFreqSamplPointPanel::setCheckBoxChecked(int index, bool checked)
    {
        QCheckBox* check = nullptr;
        switch (index)
        {
        case 0: check = m_checkFreq; break;
        case 1: check = m_checkFreq1; break;
        case 2: check = m_checkFreq2; break;
        case 3: check = m_checkFreq3; break;
        case 4: check = m_checkFreq4; break;
        default: return;
        }
        if (check)
        {
            m_isSettingCheckBoxByCode = true;
            check->setChecked(checked);
            m_isSettingCheckBoxByCode = false;
        }
    }

    void mFreqSamplPointPanel::setAllCheckBoxStates(const QList<bool>& states)
    {
        if (states.size() != 5) return;
        for (int i = 0; i < 5; ++i)
        {
            setCheckBoxChecked(i, states[i]);
        }
    }

    /**********************************************多选编辑表格*************************************************/
    FreqMultiSelectTable::FreqMultiSelectTable(QWidget* parent)
        : QTableWidget(parent)
    {
    }

    void FreqMultiSelectTable::keyPressEvent(QKeyEvent* event)
    {
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

            handleEnter();
            return;
        }
        QTableWidget::keyPressEvent(event);
    }

    void FreqMultiSelectTable::batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected)
    {
        if (!editIdx.isValid()) return;

        int col = editIdx.column();
        QTableWidgetItem* sourceItem = item(editIdx.row(), col);
        if (!sourceItem) return;

        // 优先使用 EditRole 读取原始数值，避免字符串转换失败
        bool ok = false;
        double newValue = sourceItem->data(Qt::EditRole).toDouble(&ok);
        if (!ok)
        {
            newValue = sourceItem->text().toDouble(&ok);
        }
        if (!ok) return;

        bool blocked = blockSignals(true);
        for (QTableWidgetItem* selectedItem : prevSelected)
        {
            if (selectedItem && selectedItem->column() == col && selectedItem->row() != editIdx.row())
            {
                selectedItem->setData(Qt::EditRole, newValue);
                    selectedItem->setText(MxFunctions::mNumberToString(newValue, 'g', 16));
            }
        }
        blockSignals(blocked);

        // 强制刷新视图，确保批量修改后的值显示出来
        viewport()->update();
    }

    void FreqMultiSelectTable::handleEnter()
    {
        QModelIndexList selected = selectedIndexes();
        if (selected.isEmpty()) return;

        QModelIndex lastIdx = selected.last();
        int nextRow = lastIdx.row() + 1;
        int col = lastIdx.column();

        // 当基准单元格位于最后一行时，自动添加新行
        if (nextRow >= rowCount())
        {
            // 先清除旧选区，避免 addRow 时触发的 cellChanged 把旧选区批量同步成新行的默认值 0
            clearSelection();
            emit requestAddRow();
            nextRow = rowCount() - 1;
        }

        QTableWidgetItem* nextItem = item(nextRow, col);
        if (!nextItem) return;

        setCurrentCell(nextRow, col, QItemSelectionModel::ClearAndSelect);
        scrollToItem(nextItem, QAbstractItemView::EnsureVisible);
    }

    void FreqMultiSelectTable::mousePressEvent(QMouseEvent* event)
    {
        // 获取当前点击的单元格
        QModelIndex index = indexAt(event->pos());
        if (!index.isValid())
        {
            QTableWidget::mousePressEvent(event);
            return;
        }

        // 如果点击的单元格已经在选区中，仅设置当前焦点单元格，不取消选区，也不进入编辑
        QItemSelectionModel* selection = selectionModel();
        if (selection->isSelected(index))
        {
            // 多选模式下，按下Ctrl或Shift时按原逻辑处理
            if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))
            {
                QTableWidget::mousePressEvent(event);
            }
            else
            {
                // 设置当前单元格，但不清除选区、不进入编辑
                setCurrentIndex(index);
            }
        }
        else
        {
            QTableWidget::mousePressEvent(event);
        }
    }

    /**********************************************FREQ表格*************************************************/
    FreqPointTableWidget::FreqPointTableWidget(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void FreqPointTableWidget::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 顶部控件（和你风格完全一样） =====================
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

        // ===================== 表格（完全对齐你的样式） =====================
        m_table = new FreqMultiSelectTable;
        m_table->setColumnCount(1);       // 1列
        m_table->setHorizontalHeaderLabels({ "点" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        m_table->setColumnWidth(0, 290); // 列宽和你保持一致

        // 允许编辑：双击或按键进入编辑，单击只选中
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                               | QAbstractItemView::EditKeyPressed
                               | QAbstractItemView::AnyKeyPressed);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 完全用你的表格样式 =====================
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

        // 编辑完成后同步所有选中的单元格
        connect(m_table, &QTableWidget::cellChanged, this, [this](int row, int column) {
            if (m_table->signalsBlocked()) return;
            // 获取刚才被编辑的值
            QTableWidgetItem* editedItem = m_table->item(row, column);
            if (!editedItem) return;
            bool ok;
            double newValue = editedItem->data(Qt::EditRole).toDouble(&ok);
            if (!ok)
            {
                newValue = editedItem->text().toDouble(&ok);
            }
            if (!ok) return;

            // 收集所有选中的单元格，并同步为编辑后的值
            QList<int> selectedRows;
            for (const QTableWidgetSelectionRange& range : m_table->selectedRanges())
            {
                for (int r = range.topRow(); r <= range.bottomRow(); ++r)
                {
                    if (!selectedRows.contains(r))
                    {
                        selectedRows.append(r);
                    }
                }
            }
            if (selectedRows.isEmpty())
            {
                selectedRows.append(row);
            }

            // 阻塞信号，避免每个单元格都触发 dataChanged
            m_table->blockSignals(true);
            for (int r : selectedRows)
            {
                if (r == row) continue; // 跳过刚编辑的单元格
                QTableWidgetItem* item = m_table->item(r, column);
                if (item)
                {
                    item->setData(Qt::EditRole, newValue);
                    item->setText(MxFunctions::mNumberToString(newValue, 'g', 16));
                }
            }
            m_table->blockSignals(false);

            // 强制刷新视图
            m_table->viewport()->update();

            // 发送一次数据变化信号
            onCellChanged(row, column);
            });

        // ===================== 信号槽 =====================
        connect(m_table, &FreqMultiSelectTable::requestAddRow, this, &FreqPointTableWidget::addRow);
        connect(m_btnAdd, &QPushButton::clicked, this, &FreqPointTableWidget::addRow);
        connect(m_btnRemove, &QPushButton::clicked, this, &FreqPointTableWidget::removeRow);
        connect(m_btnClear, &QPushButton::clicked, this, &FreqPointTableWidget::clearTable);
        connect(m_editRowNum, &QLineEdit::returnPressed, this, &FreqPointTableWidget::setRowByInput);

        // 点击表头时全选，便于多选后批量编辑
        m_table->horizontalHeader()->setSectionsClickable(true);

        // 使用 QLineEdit 作为编辑器，不限制小数位数
        m_table->setItemDelegate(new FreqLineEditDelegate(m_table));

        // ===================== 布局 =====================
        mainLayout->addLayout(controlLayout);
        mainLayout->addWidget(m_table, 1);
    }

    // ===================== 功能实现 =====================
    void FreqPointTableWidget::setDoubleItem(int row)
    {
        QTableWidgetItem* item = new QTableWidgetItem("0.000000");
        item->setData(Qt::EditRole, 0.0);
        m_table->setItem(row, 0, item);
    }

    void FreqPointTableWidget::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setDoubleItem(row);
    }

    void FreqPointTableWidget::removeRow()
    {
        QList<int> rows;
        for (auto& r : m_table->selectedRanges()) {
            for (int i = r.topRow(); i <= r.bottomRow(); ++i)
                rows << i;
        }

        std::sort(rows.rbegin(), rows.rend());
        for (int r : rows) m_table->removeRow(r);

        if (rows.isEmpty() && m_table->rowCount() > 0)
            m_table->removeRow(m_table->rowCount() - 1);
    }

    void FreqPointTableWidget::clearTable()
    {
        m_table->setRowCount(0);
    }

    void FreqPointTableWidget::setRowByInput()
    {
        bool ok;
        int num = m_editRowNum->text().toInt(&ok);
        if (!ok || num <= 0) return;

        clearTable();
        for (int i = 0; i < num; ++i)
            addRow();
    }

    void FreqPointTableWidget::onCellChanged(int row, int column)
    {
        Q_UNUSED(row)
        Q_UNUSED(column)
        emit dataChanged();
    }

    QList<double> FreqPointTableWidget::getAllPoints() const
    {
        QList<double> data;
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QTableWidgetItem* item = m_table->item(i, 0);
            if (item)
            {
                // 优先用EditRole读原始数据，避免字符串转换误差
                bool ok;
                double val = item->data(Qt::EditRole).toDouble(&ok);
                if (ok)
                {
                    data << val;
                }
                else
                {
                    data << item->text().toDouble();
                }
            }
        }
        return data;
    }

    void FreqPointTableWidget::setData(const QList<double>& data)
    {
        // 1. 先断开信号
        disconnect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget::onCellChanged);

        // 2. 完全清空表格（包括所有item）
        m_table->clearContents();
        m_table->setRowCount(0);

        // 3. 填充新数据
        for (int i = 0; i < data.size(); ++i)
        {
            m_table->insertRow(i);
            QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(data[i], 'g', 16));
            item->setData(Qt::EditRole, data[i]); // 用EditRole存原始数据
            m_table->setItem(i, 0, item);
        }

        // 4. 重新连接信号
        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget::onCellChanged);

        // 5. 强制刷新
        m_table->viewport()->update();
    }
    /**********************************************FREQ1表格*************************************************/
    FreqPointTableWidget1::FreqPointTableWidget1(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void FreqPointTableWidget1::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 表格（三列，表头修改） =====================
        m_table = new QTableWidget;
        m_table->setColumnCount(3);       // 改为3列
        m_table->setHorizontalHeaderLabels({ "起始频率", "频率增量", "间隔个数" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        // 设置三列宽度
        m_table->setColumnWidth(0, 150); // 起始频率
        m_table->setColumnWidth(1, 150); // 频率增量
        m_table->setColumnWidth(2, 150); // 间隔个数

        // 允许编辑
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 完全用你的表格样式 =====================
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

        // ===================== 布局 =====================
        mainLayout->addWidget(m_table, 1);

        // 使用 QLineEdit 作为编辑器，不限制小数位数
        m_table->setItemDelegate(new FreqLineEditDelegate(m_table));

        addRow();
        // 连接信号
        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget1::onCellChanged);
    }

    // ===================== 功能实现（三列） =====================
    void FreqPointTableWidget1::setDoubleItems(int row)
    {
        m_table->setItem(row, 0, new QTableWidgetItem("0.000000")); // 起始频率
        m_table->setItem(row, 1, new QTableWidgetItem("0.000000")); // 频率增量
        m_table->setItem(row, 2, new QTableWidgetItem("0"));        // 间隔个数（整数）
    }

    void FreqPointTableWidget1::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setDoubleItems(row);
    }

    void FreqPointTableWidget1::onCellChanged(int row, int column)
    {
        emit dataChanged();
    }

    QList<QVector<double>> FreqPointTableWidget1::getAllPoints() const
    {
        QList<QVector<double>> data;
        if (m_table->rowCount() > 0) {
            QVector<double> row;
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(0, col);
                if (item) {
                    // 优先用EditRole读原始数据
                    bool ok;
                    double val = item->data(Qt::EditRole).toDouble(&ok);
                    row << (ok ? val : item->text().toDouble());
                }
                else {
                    row << 0.0;
                }
            }
            data << row;
        }
        return data;
    }

    void FreqPointTableWidget1::setData(const QList<QVector<double>>& data)
    {
        // 1. 先断开信号
        disconnect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget1::onCellChanged);

        // 2. 完全清空表格
        m_table->clearContents();
        m_table->setRowCount(0);

        // 3. 填充新数据
        if (!data.isEmpty()) {
            m_table->insertRow(0);
            const QVector<double>& row = data[0];
            for (int col = 0; col < row.size() && col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(row[col], 'g', 16));
                item->setData(Qt::EditRole, row[col]); // 用EditRole存原始数据
                m_table->setItem(0, col, item);
            }
        }

        // 4. 重新连接信号
        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget1::onCellChanged);

        // 5. 强制刷新
        m_table->viewport()->update();
    }
    /**********************************************FREQ2表格*************************************************/
    FreqPointTableWidget2::FreqPointTableWidget2(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void FreqPointTableWidget2::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 表格（仅修改第三列表头） =====================
        m_table = new QTableWidget;
        m_table->setColumnCount(3);
        m_table->setHorizontalHeaderLabels({ "起始频率", "频率增量", "采样点总数" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        // 列宽保持一致
        m_table->setColumnWidth(0, 150);
        m_table->setColumnWidth(1, 150);
        m_table->setColumnWidth(2, 150);

        // 允许编辑
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 标准表格样式 =====================
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


        // ===================== 布局 =====================
        mainLayout->addWidget(m_table, 1);

        // 使用 QLineEdit 作为编辑器，不限制小数位数
        m_table->setItemDelegate(new FreqLineEditDelegate(m_table));

        addRow();
        // 连接信号
        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget2::onCellChanged);
    }

    // ===================== 功能实现 =====================
    void FreqPointTableWidget2::setDoubleItems(int row)
    {
        m_table->setItem(row, 0, new QTableWidgetItem("0.000000")); // 起始频率
        m_table->setItem(row, 1, new QTableWidgetItem("0.000000")); // 频率增量
        m_table->setItem(row, 2, new QTableWidgetItem("0"));        // 采样点总数
    }

    void FreqPointTableWidget2::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setDoubleItems(row);
    }

    void FreqPointTableWidget2::onCellChanged(int row, int column)
    {
        emit dataChanged();
    }

    QList<QVector<double>> FreqPointTableWidget2::getAllPoints() const
    {
        QList<QVector<double>> data;
        if (m_table->rowCount() > 0) {
            QVector<double> row;
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(0, col);
                if (item) {
                    bool ok;
                    double val = item->data(Qt::EditRole).toDouble(&ok);
                    row << (ok ? val : item->text().toDouble());
                }
                else {
                    row << 0.0;
                }
            }
            data << row;
        }
        return data;
    }

    void FreqPointTableWidget2::setData(const QList<QVector<double>>& data)
    {
        disconnect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget2::onCellChanged);
        m_table->clearContents();
        m_table->setRowCount(0);

        if (!data.isEmpty()) {
            m_table->insertRow(0);
            const QVector<double>& row = data[0];
            for (int col = 0; col < row.size() && col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(row[col], 'g', 16));
                item->setData(Qt::EditRole, row[col]);
                m_table->setItem(0, col, item);
            }
        }

        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget2::onCellChanged);
        m_table->viewport()->update();
    }
    /**********************************************FREQ3表格*************************************************/
    FreqPointTableWidget3::FreqPointTableWidget3(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void FreqPointTableWidget3::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 表格（5列，指定表头） =====================
        m_table = new QTableWidget;
        m_table->setColumnCount(5);
        m_table->setHorizontalHeaderLabels({ "起始频率", "终止频率", "加密个数", "拓展量", "差值类型" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        // 设置5列宽度
        m_table->setColumnWidth(0, 120); // 起始频率
        m_table->setColumnWidth(1, 120); // 终止频率
        m_table->setColumnWidth(2, 120); // 加密个数
        m_table->setColumnWidth(3, 120); // 拓展量
        m_table->setColumnWidth(4, 120); // 差值类型

        // 允许编辑
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 标准表格样式 =====================
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

        // ===================== 布局 =====================
        mainLayout->addWidget(m_table, 1);

        // 使用 QLineEdit 作为编辑器，不限制小数位数
        m_table->setItemDelegate(new FreqLineEditDelegate(m_table));

        addRow();
        // 连接信号
        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget3::onCellChanged);
    }

    // ===================== 功能实现（5列，按类型设置初始值） =====================
    void FreqPointTableWidget3::setRowItems(int row)
    {
        m_table->setItem(row, 0, new QTableWidgetItem("0.000000")); // 起始频率（double）
        m_table->setItem(row, 1, new QTableWidgetItem("0.000000")); // 终止频率（double）
        m_table->setItem(row, 2, new QTableWidgetItem("0"));        // 加密个数（int）
        m_table->setItem(row, 3, new QTableWidgetItem("0.000000")); // 拓展量（double）
        m_table->setItem(row, 4, new QTableWidgetItem("0"));        // 差值类型（int）
    }

    void FreqPointTableWidget3::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setRowItems(row);
    }

    void FreqPointTableWidget3::onCellChanged(int row, int column)
    {
        emit dataChanged();
    }

    QList<QVector<double>> FreqPointTableWidget3::getAllPoints() const
    {
        QList<QVector<double>> data;
        if (m_table->rowCount() > 0) {
            QVector<double> row;
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(0, col);
                if (item) {
                    bool ok;
                    double val = item->data(Qt::EditRole).toDouble(&ok);
                    row << (ok ? val : item->text().toDouble());
                }
                else {
                    row << 0.0;
                }
            }
            data << row;
        }
        return data;
    }

    void FreqPointTableWidget3::setData(const QList<QVector<double>>& data)
    {
        disconnect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget3::onCellChanged);
        m_table->clearContents();
        m_table->setRowCount(0);

        if (!data.isEmpty()) {
            m_table->insertRow(0);
            const QVector<double>& row = data[0];
            for (int col = 0; col < row.size() && col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(row[col], 'g', 16));
                item->setData(Qt::EditRole, row[col]);
                m_table->setItem(0, col, item);
            }
        }

        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget3::onCellChanged);
        m_table->viewport()->update();
    }
    /**********************************************FREQ4表格*************************************************/
    FreqPointTableWidget4::FreqPointTableWidget4(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void FreqPointTableWidget4::initUI()
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // ===================== 表格（4列，指定表头） =====================
        m_table = new QTableWidget;
        m_table->setColumnCount(4);
        m_table->setHorizontalHeaderLabels({ "下限频率", "频率增量", "频移量", "采样点数量" });

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        // 设置4列宽度
        m_table->setColumnWidth(0, 130); // 下限频率
        m_table->setColumnWidth(1, 130); // 频率增量
        m_table->setColumnWidth(2, 130); // 频移量
        m_table->setColumnWidth(3, 130); // 采样点数量

        // 允许编辑
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        // 选中行、交替行颜色
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        // ===================== 标准表格样式 =====================
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

        // ===================== 布局 =====================
        mainLayout->addWidget(m_table, 1);

        // 使用 QLineEdit 作为编辑器，不限制小数位数
        m_table->setItemDelegate(new FreqLineEditDelegate(m_table));

        addRow();

        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget4::onCellChanged);

    }

    // ===================== 功能实现（4列，按类型设置初始值） =====================
    void FreqPointTableWidget4::setRowItems(int row)
    {
        m_table->setItem(row, 0, new QTableWidgetItem("0.000000")); // 下限频率（double）
        m_table->setItem(row, 1, new QTableWidgetItem("0.000000")); // 频率增量（double）
        m_table->setItem(row, 2, new QTableWidgetItem("0.000000")); // 频移量（double）
        m_table->setItem(row, 3, new QTableWidgetItem("0"));        // 采样点数量（int）
    }

    void FreqPointTableWidget4::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        setRowItems(row);
    }

    void FreqPointTableWidget4::onCellChanged(int row, int column)
    {
        emit dataChanged();
    }

    QList<QVector<double>> FreqPointTableWidget4::getAllPoints() const
    {
        QList<QVector<double>> data;
        if (m_table->rowCount() > 0) {
            QVector<double> row;
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(0, col);
                if (item) {
                    bool ok;
                    double val = item->data(Qt::EditRole).toDouble(&ok);
                    row << (ok ? val : item->text().toDouble());
                }
                else {
                    row << 0.0;
                }
            }
            data << row;
        }
        return data;
    }
    void FreqPointTableWidget4::setData(const QList<QVector<double>>& data)
    {
        disconnect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget4::onCellChanged);
        m_table->clearContents();
        m_table->setRowCount(0);

        if (!data.isEmpty()) {
            m_table->insertRow(0);
            const QVector<double>& row = data[0];
            for (int col = 0; col < row.size() && col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem(MxFunctions::mNumberToString(row[col], 'g', 16));
                item->setData(Qt::EditRole, row[col]);
                m_table->setItem(0, col, item);
            }
        }

        connect(m_table, &QTableWidget::cellChanged, this, &FreqPointTableWidget4::onCellChanged);
        m_table->viewport()->update();
    }
}
