#include "mCaseRandomVibrationPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mCaseFrequencyPanel.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "mEditDiagnosis.h"


#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QLineEdit>

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitRandomVibration.h"
#include "mFunctionManager.h"
#include "mFunctionBase.h"
#include "mSetManager.h"
#include "mSetBase.h"

#include "mModelData.h"
#include "mMeshData.h"


using namespace MechPreData;
using namespace MechCaseData;
using namespace MechSetData;
using namespace MechFunctionData;
using namespace MxFunctions;
namespace MechMainWindow
{

    mCaseRandomVibrationPanel::mCaseRandomVibrationPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "随机振动分析")
    {
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };
        _typeOfoutputCheckBoxs["mxdb"]["PSD谱分析"] = QStringList{ "节点位移功率谱","节点速度功率谱","节点加速度功率谱" ,"节点应力功率谱" ,"节点应变功率谱" ,"单元磨平应力功率谱" ,"单元磨平应变功率谱"
                                                    ,"单元应力功率谱" ,"单元应变功率谱" };
        _typeOfoutputCheckBoxs["mxdb"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根", "节点位移2σ均方根", "节点位移3σ均方根",
                                                    "单元磨平应力1σ均方根", "单元磨平应力2σ均方根", "单元磨平应力3σ均方根", "单元应力均方根" };


        _typeOfoutputCheckBoxs["op2"]["PSD谱分析"] = QStringList{ "节点位移功率谱","节点速度功率谱","节点加速度功率谱" ,"节点应力功率谱" ,"节点应变功率谱" ,"单元磨平应力功率谱" ,"单元磨平应变功率谱"
                                                    ,"单元应力功率谱" ,"单元应变功率谱" };
        _typeOfoutputCheckBoxs["op2"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根", "单元磨平应力1σ均方根", "单元应力均方根" };


        _typeOfoutputCheckBoxs["h5"]["PSD谱分析"] = QStringList{ "节点位移功率谱","节点速度功率谱","节点加速度功率谱" ,"节点应力功率谱" ,"节点应变功率谱" ,"单元磨平应力功率谱" ,"单元磨平应变功率谱"
                                                    ,"单元应力功率谱" ,"单元应变功率谱" };
        _typeOfoutputCheckBoxs["h5"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根", "节点位移2σ均方根", "节点位移3σ均方根",
                                                    "单元磨平应力1σ均方根", "单元磨平应力2σ均方根", "单元磨平应力3σ均方根", "单元应力均方根" };

        _typeOfDefault["mxdb"]["PSD谱分析"] = QStringList{ "节点位移功率谱" };
        _typeOfDefault["mxdb"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根" };
        _typeOfDefault["op2"]["PSD谱分析"] = QStringList{ "节点位移功率谱" };
        _typeOfDefault["op2"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根" };
        _typeOfDefault["h5"]["PSD谱分析"] = QStringList{ "节点位移功率谱" };
        _typeOfDefault["h5"]["RMS谱分析"] = QStringList{ "节点位移1σ均方根" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];
    }

    mCaseRandomVibrationPanel::~mCaseRandomVibrationPanel()
    {

    }

    void mCaseRandomVibrationPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitRandomVibration(caseID, ImplicitRandomVibration);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseRandomVibrationPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitRandomVibration>(_caseData);
        if (!linearData) return;
        
        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // =========================================================================
        // 随机振动功率谱参数
        // =========================================================================
        auto foldPower = new FoldSection("随机振动功率谱参数", this);
        m_foldWidgets.append(foldPower);
        _mainLayout->addWidget(foldPower);

        table1 = new CaseTableWidget(linearData);

        auto powerBtn = new QPushButton(foldPower);
        powerBtn->setMaximumWidth(30);
        powerBtn->setMaximumHeight(30);
        
        powerBtn->setIcon(QIcon(":/mMainWidgets/Icon/table.png"));
        foldPower->addRow(new QLabel("随机振动功率谱参数表格创建"), powerBtn);

        connect(powerBtn, &QPushButton::clicked, [this] {
            table1->updateTableWidget();
            });


        // =========================================================================
        // 自相关函数参数
        // =========================================================================
        auto foldAutocorrelationOutput = new FoldSection("自相关函数参数", this);
        m_foldWidgets.append(foldAutocorrelationOutput);
        _mainLayout->addWidget(foldAutocorrelationOutput);

        // 定义控件
        auto time0 = new mCalcLineEdit(this, true, false);
        auto timeMax = new mCalcLineEdit(this, true, false);
        auto nTime = new mCalcLineEdit(this, true, false, true);

        foldAutocorrelationOutput->addRow(new QLabel("起始时间"), time0);
        foldAutocorrelationOutput->addRow(new QLabel("最大时间"), timeMax);
        foldAutocorrelationOutput->addRow(new QLabel("时间间隔数"), nTime);

        binder->bind(time0, linearData, &mCaseImplicitRandomVibration::_time0);
        binder->bind(timeMax, linearData, &mCaseImplicitRandomVibration::_timeMax);
        binder->bind(nTime, linearData, &mCaseImplicitRandomVibration::_nTime);

        // =========================================================================
        // 指定输出参数
        // =========================================================================
        auto foldOutput = new FoldSection("指定输出参数", this);
        m_foldWidgets.append(foldOutput);
        _mainLayout->addWidget(foldOutput);

        table2 = new OutputTableWidget(linearData);
        

        auto outputBtn = new QPushButton(foldOutput);
        outputBtn->setMaximumWidth(30);
        outputBtn->setMaximumHeight(30);
        outputBtn->setIcon(QIcon(":/mMainWidgets/Icon/table.png"));
        foldOutput->addRow(new QLabel("指定输出参数表格创建"), outputBtn);

        connect(outputBtn, &QPushButton::clicked, [this] {

            table2->updateTableWidget();
            
            });


        // =========================================================================
        // 指定输出
        // =========================================================================
       
        initOutputSection(linearData->getOutputs());
        _outputFoldWidget->setCheckable(true);
        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);
        initPchOutputSection(linearData->getPchOutputs());
        m_foldWidgets.append(_outputPchFoldWidget);
        _mainLayout->addWidget(_outputPchFoldWidget);

        _mainLayout->addStretch();
    }


/*==============================================功率谱参数表格============================================*/


    CaseTableWidget::CaseTableWidget(MechCaseData::mCaseImplicitRandomVibration* caseData, QWidget* parent)
        : QWidget(parent) , _caseData(caseData)
    {
        initUI();       // 初始化布局
        initTableHeader(); // 初始化表头
    }

    void CaseTableWidget::updateTableWidget()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        if (table_->rowCount() != _caseData->_tableXYData.size())
        {
            updateTableData();
        }
        else
        {
            updateWorkConditionList();
            updatePowerSpectrumList();
        }
      
        mTableDataWindow::getInstance()->setCustomWidget(this);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void CaseTableWidget::updateWorkList()
    {
        m_workList = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getAllCaseNamesByTypes(ImplicitFrequency);
    }

    void CaseTableWidget::updateFuncList()
    {
        m_funcList = getCVM->getModelData()->getAllNames(Physical_Function);
    }

    // ====================== 实时更新工况列表 ======================
    void CaseTableWidget::updateWorkConditionList()
    {
        updateWorkList();
        QList<int> caseID1 = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableCase1);
        QList<int> caseID2 = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableCase2);
        // 若不存在映射缺失，则不需要刷新
        if (!((caseID1.contains(-1) || caseID2.contains(-1)) && m_workList.size() > 0))
            return;

        // 遍历所有行，刷新所有工况下拉框（使用 item 数据，不依赖 cellWidget）
        for (int row = 0; row < table_->rowCount(); ++row) {
            QTableWidgetItem* item1 = table_->item(row, COL_WORK1);
            QTableWidgetItem* item2 = table_->item(row, COL_WORK2);
            if (!item1 || !item2) continue;

            QString text1 = item1->text();
            if (m_workList.contains(text1) && caseID1[row] != -1)
            {
                // 保持当前值
            }
            else
            {
                text1 = m_workList.isEmpty() ? QString() : m_workList.first();
                item1->setText(text1);
                int phyID = getCVM->getModelData()->getPhyIDByName(Physical_Case, text1);
                getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, phyID }, row, MapRelation_TableCase1);
            }

            QString text2 = item2->text();
            if (m_workList.contains(text2) && caseID2[row] != -1)
            {
                // 保持当前值
            }
            else
            {
                text2 = m_workList.isEmpty() ? QString() : m_workList.first();
                item2->setText(text2);
                int phyID = getCVM->getModelData()->getPhyIDByName(Physical_Case, text2);
                getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, phyID }, row, MapRelation_TableCase2);
            }
        }
    }

    // ====================== 实时更新功率谱曲线列表 ======================
    void CaseTableWidget::updatePowerSpectrumList()
    {
        updateFuncList();
        QList<int> function = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableFunction);
        // 若不存在映射缺失，则不需要刷新
        if (!(function.contains(-1) && m_funcList.size() > 0))
            return;

        // 遍历所有行，刷新所有功率谱下拉框（使用 item 数据，不依赖 cellWidget）
        for (int row = 0; row < table_->rowCount(); ++row) {
            QTableWidgetItem* item = table_->item(row, COL_POWER);
            if (!item) continue;
            QString text = item->text();
            if (m_funcList.contains(text) && function[row] != -1)
            {
                continue;
            }
            text = m_funcList.isEmpty() ? QString() : m_funcList.first();
            item->setText(text);
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_Function, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, phyID }, row, MapRelation_TableFunction);
        }
    }

    void CaseTableWidget::setCellItem(int row, int col, const QString& value)
    {
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, col, item);
    }

    QStringList CaseTableWidget::getComboOptions(int col) const
    {
        if (col == COL_WORK1 || col == COL_WORK2)
            return m_workList;
        if (col == COL_POWER)
            return m_funcList;
        return QStringList();
    }

    void CaseTableWidget::updateTableData()
    {
        updateWorkList();
        updateFuncList();

        QList<int> caseID1 = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableCase1);
        QList<int> caseID2 = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableCase2);
        QList<int> function = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case,_caseData->getCaseId() }, Physical_Case, MapRelation_TableFunction);

        disconnectTableSig();
        table_->setRowCount(_caseData->_tableXYData.size());
        for (int row = 0; row < _caseData->_tableXYData.size(); row++)
        {
            QString caseName1 = getCVM->getModelData()->getNameByPhyID(Physical_Case, caseID1[row]);
            if (!m_workList.contains(caseName1) && !m_workList.isEmpty())
                caseName1 = m_workList.first();
            setCellItem(row, COL_WORK1, caseName1);

            QString caseName2 = getCVM->getModelData()->getNameByPhyID(Physical_Case, caseID2[row]);
            if (!m_workList.contains(caseName2) && !m_workList.isEmpty())
                caseName2 = m_workList.first();
            setCellItem(row, COL_WORK2, caseName2);

            setCellItem(row, COL_X, _caseData->_tableXYData[row]._x);
            setCellItem(row, COL_Y, _caseData->_tableXYData[row]._y);

            QString funcName = getCVM->getModelData()->getNameByPhyID(Physical_Function, function[row]);
            if (!m_funcList.contains(funcName) && !m_funcList.isEmpty())
                funcName = m_funcList.first();
            setCellItem(row, COL_POWER, funcName);
        }
        connectTableSig();
    }

    // 初始化顶部按钮+输入框 + 表格布局
    void CaseTableWidget::initUI()
    {       
        // ========== 顶部控制栏 ==========
        QHBoxLayout* layout_top = new QHBoxLayout();
        btn_add = new QPushButton(this);
        btn_add->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        btn_del = new QPushButton(this);
        btn_del->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        btn_clear = new QPushButton(this);
        btn_clear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        edit_row = new mCalcLineEdit("", this, true, true);
        edit_row->setPlaceholderText("输入行数");
        edit_row->setFixedWidth(80);

        layout_top->addWidget(btn_add);
        layout_top->addWidget(btn_del);
        layout_top->addWidget(btn_clear);
        layout_top->addWidget(edit_row);
        layout_top->addStretch(); // 占位，靠左对齐

        // ========== 表格 ==========
        table_ = new RandomVibrationTableEdit(this);
        table_->setColumnCount(5); // 固定5列
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        // 使用委托：多选后直接在单元格内编辑并同步所有选中单元格
        auto optionCallback = [this](int col) -> QStringList { return this->getComboOptions(col); };
        auto batchCallback = [this](int col, const QString& value) { this->batchSetSelectedColumn(col, value); };
        table_->setItemDelegateForColumn(COL_WORK1, new RandomVibrationComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(COL_WORK2, new RandomVibrationComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(COL_POWER, new RandomVibrationComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(COL_X, new RandomVibrationNumberDelegate(RandomVibrationNumberDelegate::Double, batchCallback, this));
        table_->setItemDelegateForColumn(COL_Y, new RandomVibrationNumberDelegate(RandomVibrationNumberDelegate::Double, batchCallback, this));

        // 设置编辑触发器：不通过默认双击/编辑键触发，而由自定义鼠标事件处理
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // ====================== 关键：开启多选模式 ======================
        table_->setSelectionBehavior(QAbstractItemView::SelectItems);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setStyleSheet(R"(
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

        // ========== 总布局 ==========
        QVBoxLayout* layout_main = new QVBoxLayout(this);
        layout_main->addLayout(layout_top);
        layout_main->addWidget(table_);
        layout_main->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout_main);

        // ========== 绑定信号槽 ==========
        connect(table_, &RandomVibrationTableEdit::requestAddRow, this, &CaseTableWidget::addOneRow);
        connect(btn_add, &QPushButton::clicked, this, &CaseTableWidget::addOneRow);
        connect(btn_del, &QPushButton::clicked, this, &CaseTableWidget::deleteSelectedRows);
        connect(btn_clear, &QPushButton::clicked, this, &CaseTableWidget::clearAllRows);
        connect(edit_row, &mCalcLineEdit::returnPressed, this, &CaseTableWidget::setRowByInput);
        //监听内容修改
        connectTableSig();
    }

    // 初始化表头文字
    void CaseTableWidget::initTableHeader()
    {
        QStringList headers = {
            "关联工况1",
            "关联工况2",
            "功率谱实部因子",
            "功率谱虚部因子",
            "功率谱曲线"
        };
        table_->setHorizontalHeaderLabels(headers);
    }

    void CaseTableWidget::addOneRow()
    {
        disconnectTableSig();
        int row = table_->currentRow() < 0 ? table_->rowCount() : table_->currentRow() + 1;
        table_->insertRow(row);

        updateWorkList();
        updateFuncList();
        QString defaultWork = m_workList.isEmpty() ? QString() : m_workList.first();
        QString defaultFunc = m_funcList.isEmpty() ? QString() : m_funcList.first();

        setCellItem(row, COL_WORK1, defaultWork);
        setCellItem(row, COL_WORK2, defaultWork);
        setCellItem(row, COL_X, QString());
        setCellItem(row, COL_Y, QString());
        setCellItem(row, COL_POWER, defaultFunc);

        int caseID1 = getCVM->getModelData()->getPhyIDByName(Physical_Case, defaultWork);
        int caseID2 = getCVM->getModelData()->getPhyIDByName(Physical_Case, defaultWork);
        int funID = getCVM->getModelData()->getPhyIDByName(Physical_Function, defaultFunc);

        getCVM->getModelData()->getMapRelationManager().appendTableMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, caseID1 }, row, MapRelation_TableCase1);
        if (caseID2 > -1) getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, caseID2 }, row, MapRelation_TableCase2);
        if (funID > -1) getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_Case, funID }, row, MapRelation_TableFunction);

        _caseData->_tableXYData.insert(row, RandomCase_RandPs());
        connectTableSig();
    }

    // 删除当前选中行
    void CaseTableWidget::deleteSelectedRows()
    {
        QModelIndexList indexList = table_->selectionModel()->selectedIndexes();
        if (indexList.isEmpty()) return;

        // 自动去重，拿到所有选中单元格所在的行
        QSet<int> rows;
        for (const QModelIndex& idx : indexList) {
            rows.insert(idx.row());
        }

        // 从下往上删
        QList<int> rowList = rows.values();
        std::sort(rowList.begin(), rowList.end(), std::greater<int>());

        for (int r : rowList) {
            table_->removeRow(r);
            getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation({ Physical_Case, _caseData->getCaseId() }, { Physical_Case, -1 }, r);
            _caseData->_tableXYData.removeAt(r);
        }
    }

    // 清空所有行
    void CaseTableWidget::clearAllRows()
    {
        int index = table_->rowCount() - 1;
        for (; index >= 0; --index)
        {
            getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation({ Physical_Case, _caseData->getCaseId() }, { Physical_Case, -1 }, index);
        }
        _caseData->_tableXYData.clear();
        table_->setRowCount(0);
    }

    // 根据输入框数字，生成指定行数
    void CaseTableWidget::setRowByInput()
    {
        bool ok;
        int row_num = edit_row->text().toInt(&ok);
        if (!ok || row_num < 0) return; // 非法数字直接返回

        //添加
        if (row_num > table_->rowCount())
        {
            table_->clearSelection();
            if (table_->rowCount())
            {
                table_->item(table_->rowCount() - 1, COL_X)->setSelected(true);
            }

            int addRow = row_num - table_->rowCount();
            for (int i = 0; i < addRow; ++i) {
                addOneRow();
            }
        }
        //移除
        else if(row_num < table_->rowCount())
        {
            table_->clearSelection();
            for (int i = row_num; i < table_->rowCount(); ++i) {
                table_->item(i, COL_X)->setSelected(true);
            }

            deleteSelectedRows();
        }
    }

    // 单元格内容修改后，同步到底层数据
    void CaseTableWidget::slot_cellChanged(int row, int col)
    {
        if (row < 0 || row >= table_->rowCount()) return;
        QTableWidgetItem* item = table_->item(row, col);
        if (!item) return;
        QString text = item->text();

        int caseID = _caseData->getCaseId();
        switch (col)
        {
        case COL_X:
            _caseData->_tableXYData[row]._x = text;
            break;
        case COL_Y:
            _caseData->_tableXYData[row]._y = text;
            break;
        case COL_WORK1:
        {
            int id = getCVM->getModelData()->getPhyIDByName(Physical_Case, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case, caseID }, { Physical_Case, id }, row, MapRelation_TableCase1);
            break;
        }
        case COL_WORK2:
        {
            int id = getCVM->getModelData()->getPhyIDByName(Physical_Case, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case, caseID }, { Physical_Case, id }, row, MapRelation_TableCase2);
            break;
        }
        case COL_POWER:
        {
            int id = getCVM->getModelData()->getPhyIDByName(Physical_Function, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case, caseID }, { Physical_Case, id }, row, MapRelation_TableFunction);
            break;
        }
        default:
            break;
        }
    }

    // 批量修改当前列所有选中单元格，并同步数据
    void CaseTableWidget::batchSetSelectedColumn(int col, const QString& value)
    {
        if (!table_ || !table_->selectionModel()) return;

        QModelIndexList selectedIndexes = table_->selectionModel()->selectedIndexes();

        disconnectTableSig();
        for (const QModelIndex& idx : selectedIndexes)
        {
            if (idx.column() != col) continue;
            QTableWidgetItem* item = table_->item(idx.row(), idx.column());
            if (item) item->setText(value);
        }
        connectTableSig();

        // 手动触发数据同步信号
        for (const QModelIndex& idx : selectedIndexes)
        {
            if (idx.column() != col) continue;
            slot_cellChanged(idx.row(), idx.column());
        }
    }

    void CaseTableWidget::connectTableSig()
    {
        disconnectTableSig();
        m_tableChangeConn = connect(table_, &QTableWidget::cellChanged, this, &CaseTableWidget::slot_cellChanged);
    }

    void CaseTableWidget::disconnectTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
    }

/*===============================================输出参数表格====================================================*/

    OutputTableWidget::OutputTableWidget(MechCaseData::mCaseImplicitRandomVibration* caseData, QWidget* parent)
        : QWidget(parent) , _caseData(caseData)
    {

        initUI();       // 初始化布局
        initTableHeader(); // 初始化表头
    }

    void OutputTableWidget::updateTableWidget()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        int a = table_->rowCount();
        int b = _caseData->m_outputData.size();
        if (table_->rowCount() != _caseData->m_outputData.size())
        {
            updateTableData();
        }
        // 显示在停靠窗口中
        mTableDataWindow::getInstance()->setCustomWidget(this);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    // 初始化顶部按钮+输入框 + 表格布局
    void OutputTableWidget::initUI()
    {

        // ========== 顶部控制栏 ==========
        QHBoxLayout* layout_top = new QHBoxLayout();
        btn_add = new QPushButton(this);
        btn_add->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        btn_del = new QPushButton(this);
        btn_del->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        btn_clear = new QPushButton(this);
        btn_clear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        edit_row = new mCalcLineEdit("", this, true, true);
        edit_row->setPlaceholderText("输入行数");
        edit_row->setFixedWidth(80);

        layout_top->addWidget(btn_add);
        layout_top->addWidget(btn_del);
        layout_top->addWidget(btn_clear);
        layout_top->addWidget(edit_row);
        layout_top->addStretch(); // 占位，靠左对齐

        // ========== 表格 ==========
        table_ = new RandomVibrationTableEdit(this);
        table_->setColumnCount(4); // 固定4列
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        // 使用委托：多选后直接在单元格内编辑并同步所有选中单元格
        auto optionCallback = [this](int col) -> QStringList { return this->getComboOptions(col); };
        auto batchCallback = [this](int col, const QString& value) { this->batchSetSelectedColumn(col, value); };
        table_->setItemDelegateForColumn(Out_Type, new RandomVibrationComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(Radom_Type, new RandomVibrationComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(Node_Id, new RandomVibrationNumberDelegate(RandomVibrationNumberDelegate::Int, batchCallback, this));
        table_->setItemDelegateForColumn(Freedom_Id, new RandomVibrationNumberDelegate(RandomVibrationNumberDelegate::Int, batchCallback, this));

        // 设置编辑触发器：不通过默认双击/编辑键触发，而由自定义鼠标事件处理
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // ====================== 关键：开启多选模式 ======================
        table_->setSelectionBehavior(QAbstractItemView::SelectItems);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setStyleSheet(R"(
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


        // ========== 总布局 ==========
        QVBoxLayout* layout_main = new QVBoxLayout(this);
        layout_main->addLayout(layout_top);
        layout_main->addWidget(table_);
        layout_main->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout_main);

        // ========== 绑定信号槽 ==========
        connect(table_, &RandomVibrationTableEdit::requestAddRow, this, &OutputTableWidget::addOneRow);
        connect(btn_add, &QPushButton::clicked, this, &OutputTableWidget::addOneRow);
        connect(btn_del, &QPushButton::clicked, this, &OutputTableWidget::deleteSelectedRows);
        connect(btn_clear, &QPushButton::clicked, this, &OutputTableWidget::clearAllRows);

        //控制行数
        connect(edit_row, &mCalcLineEdit::returnPressed, this, &OutputTableWidget::setRowByInput);
        //监听内容修改
        connectTableSig();
    }

    // 初始化表头文字
    void OutputTableWidget::initTableHeader()
    {
        QStringList headers = {
            "输出类型",
            "随机类型",
            "节点编号",
            "自由度编号"
        };
        table_->setHorizontalHeaderLabels(headers);
    }

    void OutputTableWidget::setCellItem(int row, int col, const QString& value)
    {
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, col, item);
    }

    QStringList OutputTableWidget::getComboOptions(int col) const
    {
        if (col == Out_Type)
            return list_work1;
        if (col == Radom_Type)
            return list_work2;
        return QStringList();
    }

    // 给当前工况表格插入一行新数据行
    void OutputTableWidget::addOneRow()
    {
        disconnectTableSig();

        int row = table_->currentRow() < 0 ? table_->rowCount() : table_->currentRow() + 1;
        table_->insertRow(row);

        RandomCase_OutPutType out;
        setCellItem(row, Out_Type, list_work1.isEmpty() ? QString() : list_work1.first());
        setCellItem(row, Radom_Type, list_work2.isEmpty() ? QString() : list_work2.first());
        setCellItem(row, Node_Id, QString());
        setCellItem(row, Freedom_Id, QString());

        out._outputType = table_->item(row, Out_Type)->text();
        out._randomType = table_->item(row, Radom_Type)->text();

        _caseData->m_outputData.insert(row, out);
       
        connectTableSig();
    }

    // 删除当前选中行
    void OutputTableWidget::deleteSelectedRows()
    {
        QModelIndexList indexList = table_->selectionModel()->selectedIndexes();
        if (indexList.isEmpty()) return;

        // 自动去重，提取所有的选中行
        QSet<int> rows;
        for (const QModelIndex& idx : indexList) {
            rows.insert(idx.row());
        }

        // 从下往上删
        QList<int> rowList = rows.values();
        std::sort(rowList.begin(), rowList.end(), std::greater<int>());

        for (int r : rowList)
        {
            table_->removeRow(r);
            _caseData->m_outputData.removeAt(r);

        }
    }

    // 清空所有行
    void OutputTableWidget::clearAllRows()
    {
        _caseData->m_outputData.clear();
        table_->setRowCount(0);
    }

    // 根据输入框数字，生成指定行数
    void OutputTableWidget::setRowByInput()
    {
        bool ok;
        int row_num = edit_row->text().toInt(&ok);
        if (!ok || row_num < 0) return; // 非法数字直接返回

        //目标行数>当前表格行数，批量加行
        if (row_num > table_->rowCount())
        {
            table_->clearSelection();
            if (table_->rowCount())
            {
                table_->item(table_->rowCount() - 1, Node_Id)->setSelected(true);
            }

            int addRow = row_num - table_->rowCount();
            for (int i = 0; i < addRow; ++i) {
                addOneRow();
            }
        }
        //移除
        else if (row_num < table_->rowCount())
        {
            table_->clearSelection();
            for (int i = row_num; i < table_->rowCount(); ++i) {
                table_->item(i, Node_Id)->setSelected(true);
            }

            deleteSelectedRows();
        }

    }

    // 单元格内容修改的槽函数
    void OutputTableWidget::slot_cellChanged(int row, int col)
    {
        if (row < 0 || row >= table_->rowCount()) return;
        QTableWidgetItem* item = table_->item(row, col);
        if (!item) return;
        QString text = item->text();

        switch (col)
        {
        case Out_Type:
            _caseData->m_outputData[row]._outputType = text;
            break;
        case Radom_Type:
            _caseData->m_outputData[row]._randomType = text;
            break;
        case Node_Id:
            _caseData->m_outputData[row]._nodeNum = text;
            break;
        case Freedom_Id:
            _caseData->m_outputData[row]._dofNum = text;
            break;
        default:
            break;
        }
    }

    // 批量修改当前列所有选中单元格，并同步数据
    void OutputTableWidget::batchSetSelectedColumn(int col, const QString& value)
    {
        if (!table_ || !table_->selectionModel()) return;

        QModelIndexList selectedIndexes = table_->selectionModel()->selectedIndexes();

        disconnectTableSig();
        for (const QModelIndex& idx : selectedIndexes)
        {
            if (idx.column() != col) continue;
            QTableWidgetItem* item = table_->item(idx.row(), idx.column());
            if (item) item->setText(value);
        }
        connectTableSig();

        // 手动触发数据同步信号
        for (const QModelIndex& idx : selectedIndexes)
        {
            if (idx.column() != col) continue;
            slot_cellChanged(idx.row(), idx.column());
        }
    }

    void OutputTableWidget::updateTableData()
    {
           disconnectTableSig();   // 避免刷新时触发保存
           table_->setRowCount(_caseData->m_outputData.size());
           for (int row = 0; row < _caseData->m_outputData.size(); row++)
           {
               setCellItem(row, Out_Type, _caseData->m_outputData[row]._outputType);
               setCellItem(row, Radom_Type, _caseData->m_outputData[row]._randomType);
               setCellItem(row, Node_Id, _caseData->m_outputData[row]._nodeNum);
               setCellItem(row, Freedom_Id, _caseData->m_outputData[row]._dofNum);
           }
           connectTableSig();
    }

    void OutputTableWidget::connectTableSig()
    {
        disconnectTableSig();
        m_tableChangeConn = connect(table_, &QTableWidget::cellChanged, this, &OutputTableWidget::slot_cellChanged);
    }

    void OutputTableWidget::disconnectTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
    }

}