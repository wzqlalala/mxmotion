#include "mCaseResponseSpectrumPanel.h"
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

//BasicData
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCaseImplicitResponseSpectrum.h"
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
    mCaseResponseSpectrumPanel::mCaseResponseSpectrumPanel(QWidget* parent)
        : mCaseSettingPanel(parent, "响应谱分析")
    {
        _typeOfoutputCheckBoxs["mxdb"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["mxdb"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["mxdb"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        //_typeOfoutputCheckBoxs["mxdb"]["拓扑优化"] = QStringList{ "节点密度", "单元密度" };

        _typeOfoutputCheckBoxs["op2"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["op2"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["op2"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };
        _typeOfoutputCheckBoxs["op2"]["能量"] = QStringList{ "单元应变能", "简单磨平应变能", "应变能密度" };

        _typeOfoutputCheckBoxs["h5"]["基本结果"] = QStringList{ "位移", "速度", "加速度" };
        _typeOfoutputCheckBoxs["h5"]["应变"] = QStringList{ "节点应变", "单元应变", "简单磨平应变" };
        _typeOfoutputCheckBoxs["h5"]["应力"] = QStringList{ "节点应力", "单元应力", "简单磨平应力" };

        _typeOfDefault["mxdb"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["op2"]["基本结果"] = QStringList{ "位移" };
        _typeOfDefault["h5"]["基本结果"] = QStringList{ "位移" };

        _outputCheckboxs = _typeOfoutputCheckBoxs["mxdb"];

    }

    mCaseResponseSpectrumPanel::~mCaseResponseSpectrumPanel()
    {

    }

    void mCaseResponseSpectrumPanel::newCasePanel()
    {
        int caseID = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).createID();
        _caseData = new mCaseImplicitResponseSpectrum(caseID, ImplicitResponseSpectrum);
        auto name = getCVM->getModelData()->createNameByType(Physical_Case, _caseData->caseTypeToStringEn());
        _caseData->setCaseName(name);
        getCVM->getModelData()->addPhysicalField<mCaseBase>(_caseData);

        updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateCaseSig(true, _caseData->getCaseId());
    }

    void mCaseResponseSpectrumPanel::updatePanel()
    {
        auto linearData = toType<mCaseImplicitResponseSpectrum>(_caseData);
        if (!linearData) return;

        //工况定义折叠块
        m_foldWidgets.append(_defineFold);
        _mainLayout->addWidget(_defineFold);

        // =========================================================================
        // 1. 求解器控制 (Solver Control)
        // =========================================================================
        auto foldSolver = new FoldSection("求解控制", this);
        m_foldWidgets.append(foldSolver);
        _mainLayout->addWidget(foldSolver);

        //模态方法
        auto lblSolver = new QLabel("模态求解器");
        auto comboSolver = new QComboBox();
        comboSolver->addItems({ "自动多重子结构", "兰索斯" });

        // 绑定数据
        binder->bind(comboSolver, linearData, &mCaseImplicitResponseSpectrum::_modalSolver);

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        // 布局添加
        foldSolver->addRow(lblSolver, comboSolver);
        //foldSolver->addRow(checkRemnant);

        // =========================================================================
        // 频率范围
        // =========================================================================
        auto foldFrequency = new FoldSection("频率范围", this);
        m_foldWidgets.append(foldFrequency);
        _mainLayout->addWidget(foldFrequency);

        // 定义控件
        auto minFrequency = new mCalcLineEdit(this, true, false);
        auto maxFrequency = new mCalcLineEdit(this, true, false);
        auto order = new mCalcLineEdit(this, true, true, true);
        auto tolerance = new mCalcLineEdit(this, true, false);
        auto ampffact = new mCalcLineEdit(this, true, false, true);
        auto comboNormalization = new QComboBox();
        comboNormalization->addItems({ "无", "广义质量", "最大位移" });

        // 定义标签（为了显隐控制，需要指针）
        auto lblMinFreq = new QLabel("最小频率");
        auto lblMaxFreq = new QLabel("最大频率");
        auto lblOrder = new QLabel("阶数");
        auto lblTolerance = new QLabel("容差");
        auto lblAmpffact = new QLabel("放大系数");
        auto lblNormalization = new QLabel("特征向量");

        // 添加到折叠块
        foldFrequency->addRow(lblMinFreq, minFrequency);
        foldFrequency->addRow(lblMaxFreq, maxFrequency);
        foldFrequency->addRow(lblOrder, order);
        foldFrequency->addRow(lblTolerance, tolerance);
        foldFrequency->addRow(lblAmpffact, ampffact);
        foldFrequency->addRow(lblNormalization, comboNormalization);

        // 绑定数据
        binder->bind(minFrequency, linearData, &mCaseImplicitResponseSpectrum::_minFrequency);
        binder->bind(maxFrequency, linearData, &mCaseImplicitResponseSpectrum::_maxFrequency);
        binder->bind(order, linearData, &mCaseImplicitResponseSpectrum::_order);
        binder->bind(tolerance, linearData, &mCaseImplicitResponseSpectrum::_tolerance);
        binder->bind(ampffact, linearData, &mCaseImplicitResponseSpectrum::_ampffact);
        binder->bind(comboNormalization, linearData, &mCaseImplicitResponseSpectrum::_normalization);

        // =========================================================================
        // 阻尼参数 (Damping Parameters)
        // =========================================================================
        auto foldDamping = new FoldSection("阻尼参数", this);
        m_foldWidgets.append(foldDamping);
        _mainLayout->addWidget(foldDamping);

        auto dampingG = new mCalcLineEdit(this, true, false, true);
        auto dampingAlpha1 = new mCalcLineEdit(this, true, false);
        auto dampingAlpha2 = new mCalcLineEdit(this, true, false);

        auto comboFunction = new QComboBox();
        updateFunctionCombobox(comboFunction);

        auto btnFunction = new QToolButton();
        btnFunction->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        foldDamping->addRow(new QLabel("整体结构阻尼"), dampingG);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha1"), dampingAlpha1);
        foldDamping->addRow(new QLabel("瑞利阻尼 Alpha2"), dampingAlpha2);
        foldDamping->addRow(new QLabel("模态阻尼曲线"), comboFunction, btnFunction);

        binder->bind(dampingG, linearData, &mCaseImplicitResponseSpectrum::_dampingG);
        binder->bind(dampingAlpha1, linearData, &mCaseImplicitResponseSpectrum::_dampingAlpha1);
        binder->bind(dampingAlpha2, linearData, &mCaseImplicitResponseSpectrum::_dampingAlpha2);


        // =========================================================================
        // 响应谱分析参数 (Response spectrum analysis parameters)
        // =========================================================================
        auto foldResponseSpectrum = new FoldSection("响应谱分析参数", this);
        m_foldWidgets.append(foldResponseSpectrum);
        _mainLayout->addWidget(foldResponseSpectrum);

        auto comboDirction = new QComboBox();
        comboDirction->addItems({  "ALG", "SRSS" });
        auto modal = new QComboBox();
        modal->addItems({ "ABS", "SRSS","CQC","NRL","DSUM","ROSE"});
        auto modalFrequencyClose = new mCalcLineEdit(this, true, false, true);
        


        _table = new ResponseSpectrumTableWidget(linearData);

        auto powerBtn = new QPushButton(foldResponseSpectrum);
        powerBtn->setMaximumWidth(30);
        powerBtn->setMaximumHeight(30);

        powerBtn->setIcon(QIcon(":/mMainWidgets/Icon/table.png"));
        

        connect(powerBtn, &QPushButton::clicked, [this] {
            _table->updateTableWidget();         
            });
   
        foldResponseSpectrum->addRow(new QLabel("方向组合方法"), comboDirction);
        foldResponseSpectrum->addRow(new QLabel("模态组合方法"), modal);
        foldResponseSpectrum->addRow(new QLabel("模态频率接近系数"), modalFrequencyClose);
        foldResponseSpectrum->addRow(new QLabel("响应谱激励数"), powerBtn);

        binder->bind(comboDirction, linearData, &mCaseImplicitResponseSpectrum::_dirction);
        binder->bind(modal, linearData, &mCaseImplicitResponseSpectrum::_modal);
        binder->bind(modalFrequencyClose, linearData, &mCaseImplicitResponseSpectrum::_modalFrequencyClose);


        // =========================================================================
        // 输出控制
        // =========================================================================

        initOutputSection(linearData->getOutputs());

        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();

        // =========================================================================
        // 逻辑控制 (Logic & Connections)
        // =========================================================================

        // 定义Lambda函数统一处理显隐逻辑
        // TODO: 待按表核对 - 响应谱分析面板控件显隐规则
        // 当前规则：
        // - 自动多重子结构(AMLS)：显示 lblTolerance/tolerance、lblAmpffact/ampffact；
        //                        隐藏 lblNormalization/comboNormalization
        // - 兰索斯(Lanczos)：隐藏 lblAmpffact/ampffact；显示 lblNormalization/comboNormalization
        auto updateVisibility = [=]() {
            QString currentSolver = comboSolver->currentText();
            //bool isPreload = checkPreload->isChecked();

            if (currentSolver == "自动多重子结构")
            {
                // 自动多重子结构：放大系数；隐藏特征向量
                lblTolerance->setVisible(true);       tolerance->setVisible(true);

                lblAmpffact->setVisible(true);        ampffact->setVisible(true);
                foldFrequency->addRow(lblAmpffact, ampffact);
                lblNormalization->setVisible(false);  comboNormalization->setVisible(false);
                lblNormalization->setParent(nullptr);
                comboNormalization->setParent(nullptr);
            }
            else if (currentSolver == "兰索斯")
            {
                // 兰索斯：放大系数；显示特征向量
                lblAmpffact->setVisible(false);       ampffact->setVisible(false);
                lblNormalization->setVisible(true);   comboNormalization->setVisible(true);
                foldFrequency->addRow(lblNormalization, comboNormalization);
                lblAmpffact->setParent(nullptr);
                ampffact->setParent(nullptr);
            }
            foldSolver->insertRow(2, lblSolver, comboSolver);
            lblSolver->show();
            comboSolver->show();
                                  

            // 刷新布局防止留白
            foldSolver->refreshLayout();
            foldFrequency->refreshLayout();
            foldDamping->refreshLayout();
            };

        

        // 信号连接
        // 连接模态类型改变

        // 连接求解器改变
        connect(comboSolver, &QComboBox::currentTextChanged, this, updateVisibility);

        //点击函数新建
        connect(btnFunction, &QAbstractButton::clicked, this, [=]() {
            if (getCVM == nullptr)
            {
                return;
            }
            if (_caseData == nullptr)
            {
                return;
            }
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            updateFunctionCombobox(comboFunction);
            auto& functionManager = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function);

            auto functionIDs = functionManager.getAllFunctionID();
            std::sort(functionIDs.begin(), functionIDs.end());
            if (functionIDs.size() == 0)
            {
                return;
            }
            auto functionData = functionManager.getFunctionData(functionIDs.at(functionIDs.size() - 1));
            if (functionData == nullptr)
            {
                return;
            }
            comboFunction->setCurrentText(functionData->getFunctionName());
            //设置id
            int id = _caseData->getCaseId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Function, functionData->getFunctionId() });
            //emit MechCommon::mMechGlobalSignals::getInstance()->createUpdatePropertySig(false, id);
            //updateVisibility();
            });

        //函数选择变化
        connect(comboFunction, &QComboBox::currentTextChanged, this, [=](const QString& text) {
            if (getCVM == nullptr)
            {
                return;
            }
            if (_caseData == nullptr)
            {
                return;
            }
            auto& functionManager = getCVM->getModelData()->getManager<mFunctionManager>(Physical_Function);
            int functionId = functionManager.getIDByFunctionName(text);
            if (functionId == -1)
            {
                return;
            }
            comboFunction->setCurrentText(text);
            //设置id
            int id = _caseData->getCaseId();
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mapRelation.appendMapRelation({ Physical_Case, id }, { Physical_Function, functionId });

            });

        // 初始化调用一次以设置初始状态
        updateVisibility();
    }

    ResponseSpectrumTableWidget::ResponseSpectrumTableWidget(MechCaseData::mCaseImplicitResponseSpectrum* caseData, QWidget* parent) : QWidget(parent), _caseData(caseData)
    {
        initUI();
        initTableHeader();
    }

    void ResponseSpectrumTableWidget::updateNameList()
    {
        m_loadList = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_SPECTRUM);
    }

    void ResponseSpectrumTableWidget::updateLoadList()
    {
        updateNameList();
        QList<int> loadId = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, _caseData->getCaseId() }, Physical_LoadBoundary);
        // 如果载荷名称没有变化且映射不存在缺失，则不需要刷新
        bool hasMissing = loadId.contains(-1) && m_loadList.size() > 0;
        if (!hasMissing)
            return;

        // 遍历所有行，刷新所有载荷下拉框（使用 item 数据，不依赖 cellWidget）
        for (int row = 0; row < table_->rowCount(); ++row)
        {
            if (row < 0 || row >= loadId.size())
                continue;

            QTableWidgetItem* item = table_->item(row, column_name);
            if (!item) continue;
            QString text = item->text();
            if (m_loadList.contains(text) && loadId[row] != -1)
            {
                continue;
            }
            // 当前名称失效，默认选择第一个
            text = m_loadList.isEmpty() ? QString() : m_loadList.first();
            item->setText(text);
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_LoadBoundary, phyID }, row);
        }
    }

    void ResponseSpectrumTableWidget::updateTableWidget()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        if (table_->rowCount() != _caseData->_tableData.size())
        {
            updateTableData();
        }
        else
        {
            updateLoadList();
        }
        mTableDataWindow::getInstance()->setCustomWidget(this);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }
    void ResponseSpectrumTableWidget::initUI()
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
        table_ = new ResponseSpectrumTableEdit(this);
        table_->setColumnCount(5); // 固定5列
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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

        // 使用委托：多选后直接在单元格内编辑并同步所有选中单元格
        auto optionCallback = [this](int col) -> QStringList { return this->getComboOptions(col); };
        auto batchCallback = [this](int col, const QString& value) { this->batchSetSelectedColumn(col, value); };
        table_->setItemDelegateForColumn(column_name, new ResponseSpectrumComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(column_x1, new ResponseSpectrumComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(column_x2, new ResponseSpectrumComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(column_x3, new ResponseSpectrumComboBoxDelegate(optionCallback, batchCallback, this));
        table_->setItemDelegateForColumn(column_scale, new ResponseSpectrumLineEditDelegate(batchCallback, this));

        // 设置编辑触发器：不通过默认双击/编辑键触发，而由自定义鼠标事件处理
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // ========== 总布局 ==========
        QVBoxLayout* layout_main = new QVBoxLayout(this);
        layout_main->addLayout(layout_top);
        layout_main->addWidget(table_);
        layout_main->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout_main);

        // ========== 绑定信号槽 ==========
        connect(table_, &ResponseSpectrumTableEdit::requestAddRow, this, &ResponseSpectrumTableWidget::addOneRow);
        connect(btn_add, &QPushButton::clicked, this, &ResponseSpectrumTableWidget::addOneRow);
        connect(btn_del, &QPushButton::clicked, this, &ResponseSpectrumTableWidget::deleteSelectedRows);
        connect(btn_clear, &QPushButton::clicked, this, &ResponseSpectrumTableWidget::clearAllRows);
        // 输入框回车 或 失去焦点 都生效
        connect(edit_row, &QLineEdit::returnPressed, this, &ResponseSpectrumTableWidget::setRowByInput);
        connect(edit_row, &QLineEdit::editingFinished, this, &ResponseSpectrumTableWidget::setRowByInput);
        //监听内容修改
        connectTableSig();
    }
    void ResponseSpectrumTableWidget::initTableHeader()
    {
        QStringList headers = {
             "DTISPEC",
             "SCALE",
             "X(0)",
             "X(1)",
             "X(2)",
        };
        table_->setHorizontalHeaderLabels(headers);
    }

    void ResponseSpectrumTableWidget::setCellItem(int row, int col, const QString& value)
    {
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, col, item);
    }

    QStringList ResponseSpectrumTableWidget::getComboOptions(int col) const
    {
        if (col == column_name)
            return m_loadList;
        if (col == column_x1 || col == column_x2 || col == column_x3)
            return QStringList{ "0", "1" };
        return QStringList();
    }

    void ResponseSpectrumTableWidget::refreshComboOptions(int col, const QStringList& newOptions)
    {
        for (int row = 0; row < table_->rowCount(); ++row)
        {
            QTableWidgetItem* item = table_->item(row, col);
            if (!item) continue;
            QString text = item->text();
            if (!newOptions.contains(text) && !newOptions.isEmpty())
            {
                text = newOptions.first();
                item->setText(text);
            }
        }
    }

    void ResponseSpectrumTableWidget::addOneRow()
    {
        disconnectTableSig();
        int row = table_->rowCount();
        table_->insertRow(row);
        ResponseSpectrum_Data data;

        updateNameList();
        // 默认名称
        QString defaultName = m_loadList.isEmpty() ? QString() : m_loadList.first();
        setCellItem(row, column_name, defaultName);
        setCellItem(row, column_scale, data._scales);
        setCellItem(row, column_x1, data._loadx.isEmpty() ? "0" : data._loadx);
        setCellItem(row, column_x2, data._loady.isEmpty() ? "0" : data._loady);
        setCellItem(row, column_x3, data._loadz.isEmpty() ? "0" : data._loadz);

        int caseID1 = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, defaultName);
        getCVM->getModelData()->getMapRelationManager().appendTableMapRelation({ Physical_Case, _caseData->getCaseId() }, { Physical_LoadBoundary, caseID1 }, row);

        _caseData->_tableData.insert(row, data);
        connectTableSig();
    }
    void ResponseSpectrumTableWidget::deleteSelectedRows()
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
            getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation({ Physical_Case, _caseData->getCaseId() }, { Physical_LoadBoundary, -1 }, r);
            _caseData->_tableData.removeAt(r);
        }
    }
    void ResponseSpectrumTableWidget::clearAllRows()
    {
        //清除映射
        int index = table_->rowCount() - 1;
        for (; index >= 0; --index)
        {
            getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation({ Physical_Case, _caseData->getCaseId() }, { Physical_LoadBoundary, -1 }, index);
        }
        //清除表格数据
        table_->setRowCount(0);
        _caseData->_tableData.clear();
    }
    void ResponseSpectrumTableWidget::setRowByInput()
    {
        bool ok;
        int row_num = edit_row->text().toInt(&ok);
        if (!ok || row_num < 0) return; // 非法数字直接返回

        table_->setRowCount(0); // 先清空
        _caseData->_tableData.clear();
        for (int i = 0; i < row_num; i++) {
            addOneRow();
        }
    }

    // 单元格内容修改后，同步到底层数据
    void ResponseSpectrumTableWidget::slot_cellChanged(int row, int col)
    {
        if (row < 0 || row >= table_->rowCount()) return;
        QTableWidgetItem* item = table_->item(row, col);
        if (!item) return;
        QString text = item->text();

        switch (col)
        {
        case column_scale:
            _caseData->_tableData[row]._scales = text;
            break;
        case column_x1:
            _caseData->_tableData[row]._loadx = text;
            break;
        case column_x2:
            _caseData->_tableData[row]._loady = text;
            break;
        case column_x3:
            _caseData->_tableData[row]._loadz = text;
            break;
        case column_name:
        {
            int phyID = getCVM->getModelData()->getPhyIDByName(Physical_LoadBoundary, text);
            getCVM->getModelData()->getMapRelationManager().updateTabelMapRelation({ Physical_Case,_caseData->getCaseId() }, { Physical_LoadBoundary, phyID }, row);
            break;
        }
        default:
            break;
        }
    }

    // 批量修改当前列所有选中单元格，并同步数据
    void ResponseSpectrumTableWidget::batchSetSelectedColumn(int col, const QString& value)
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

    void ResponseSpectrumTableWidget::updateTableData()
    {
        disconnectTableSig();
        table_->setRowCount(_caseData->_tableData.size());
        //获取所有谱载荷名称
        updateNameList();
        QList<int> pulist = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, _caseData->getCaseId() }, Physical_LoadBoundary);//获取谱载荷ID
        for (int row = 0; row < _caseData->_tableData.size(); row++)
        {
            setCellItem(row, column_scale, _caseData->_tableData[row]._scales);
            setCellItem(row, column_x1, _caseData->_tableData[row]._loadx);
            setCellItem(row, column_x2, _caseData->_tableData[row]._loady);
            setCellItem(row, column_x3, _caseData->_tableData[row]._loadz);

            QString text;
            if (row >= 0 && row < pulist.size())
            {
                text = getCVM->getModelData()->getNameByPhyID(Physical_LoadBoundary, pulist[row]);//通过ID获取载荷名称
            }
            if (!m_loadList.contains(text) && !m_loadList.isEmpty())
                text = m_loadList.first();
            setCellItem(row, column_name, text);
        }
        connectTableSig();
    }

    void ResponseSpectrumTableWidget::connectTableSig()
    {
        disconnectTableSig();
        m_tableChangeConn = connect(table_, &QTableWidget::cellChanged, this, &ResponseSpectrumTableWidget::slot_cellChanged);
    }

    void ResponseSpectrumTableWidget::disconnectTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
    }
}