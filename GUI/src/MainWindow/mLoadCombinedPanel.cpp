#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "mGlobalSignals.h"
#include "mDataStatusManager.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSurface.h>
#include<mElementSet.h>

#include "mLoadCombinedPanel.h"
#include "mDockLayoutManager.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadCombinedPanel::mLoadCombinedPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadCombinedPanel::~mLoadCombinedPanel()
    {
        clearInputs();
    }
    void mLoadCombinedPanel::initInputs()
    {
        QStringList inpnames =
        {
           "总比例系数"
        };


        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], true);
            inp.labelname->setText(inpnames[i]);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveTotalRatio();
                });
        }

        _combinedBtn.initWidget("载荷组合");
        _combinedBtn.cb->setCheckable(false);
        _combinedBtn.cb->setStyleSheet(R"(
                QCheckBox::indicator{
                    width:0px;
                    height:0px;
                }
            )");

        connect(_combinedBtn.tablebtn, &QPushButton::clicked, this, &mLoadCombinedPanel::slot_combinedLoadTable);
        initTable();
    }

    void mLoadCombinedPanel::clearInputs()
    {
        inputWidgets.clear();
        if (table_) delete table_;
    }

    void mLoadCombinedPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        //设置延迟时间,相位数据 
        std::shared_ptr<MechCaseData::mLoadCombined> pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadCombined>(loadData);

        QVariant padata = pressurePtr->ktotal_;

        QString pastr = "";
        if (!padata.isNull() && padata.isValid())
        {
            pastr = MxFunctions::mNumberToString(padata.value<double>());
        }
        inputWidgets[0].lineeditvalue->setExpression(pastr);


        //先获取下系数容器，根据系数容器大小设置表格行数,避免读文件后无法填充
        QList<double> ratioList = pressurePtr->ratioList_;
        table_->setRowCount(ratioList.size());
        for (int i = 0; i < ratioList.size(); ++i)
        {
            table_->setRatio(i, ratioList[i]);
        }
        auto allTransientLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_TRANSIENT);
        auto allFreqdomaintLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_FREQDOMAIN);
        auto allAcousticFreqdomaintLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_ACOFREQDOMAIN);
        QStringList option;
        for (auto loadName : allTransientLoadName)
        {
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            option.append(loadName);
        }
        for (auto loadName : allFreqdomaintLoadName)
        {
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            option.append(loadName);
        }
        for (auto loadName : allAcousticFreqdomaintLoadName)
        {
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            option.append(loadName);
        }
        table_->setLoadOptions(option);
        checkLoadTypeConflict();

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        QList<int> loadIDs = mapRelation.getMapRelation<QList<int>>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_LoadBoundary);

        for (int i = 0; i < loadIDs.size(); ++i)
        {
            QString name = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(loadIDs[i]);
            table_->setCurrentComboText(i, name);
        }
    }

    QVector<QWidget*> mLoadCombinedPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(inputWidgets[0].propwid);
        wds.append(_combinedBtn.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadCombinedPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    void mLoadCombinedPanel::saveTotalRatio()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadCombined>(_loadData);
        if (!load)
            return;

        QString pastr = inputWidgets[0].lineeditvalue->text().trimmed();
        if (!pastr.isEmpty())
        {
            load->ktotal_ = QVariant::fromValue<double>(pastr.toDouble());
        }
        else
        {
            load->ktotal_ = QVariant();
        }
        notifyLoadChanged();
    }

    void mLoadCombinedPanel::saveTableData()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadCombined>(_loadData);
        if (!load)
            return;

        QStringList currentLoadName = table_->getAllCurrentLoadNames();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        for (auto row : delRows)
        {
            mapRelation.deleteTabelMapRelation({ Physical_LoadBoundary, load->getLoadBoundaryId() }, { Physical_LoadBoundary, -1 }, row);
        }
        for (int i = 0; i < currentLoadName.size(); i++)
        {
            int id = getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(currentLoadName[i]);
            mapRelation.deleteTabelMapRelation({ Physical_LoadBoundary, load->getLoadBoundaryId() }, { Physical_LoadBoundary, id }, i);
            mapRelation.appendTableMapRelation({ Physical_LoadBoundary, load->getLoadBoundaryId() }, { Physical_LoadBoundary, id }, i);
        }

        load->ratioList_ = table_->getAllRatios();
        notifyLoadChanged();
    }

    void mLoadCombinedPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadCombinedPanel::getPickedData()
    {
        return _picks;
    }

    void mLoadCombinedPanel::slot_combinedLoadTable()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table_);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mLoadCombinedPanel::initTable()
    {
        table_ = new LoadTableWidget();
        connect(table_, &LoadTableWidget::ratioEditFinished, this, [=](int row, double value)
            {
                Q_UNUSED(row)
                Q_UNUSED(value)
                saveTableData();
            });
        connect(table_, &LoadTableWidget::loadNameChanged, this, [=](int row, const QString& name)
            {
                Q_UNUSED(row)
                Q_UNUSED(name)
                saveTableData();
                checkLoadTypeConflict();
            });
        connect(table_, &LoadTableWidget::rowAdded, this, [=](int row)
            {
                Q_UNUSED(row)
                saveTableData();
                checkLoadTypeConflict();
            });
        connect(table_, &LoadTableWidget::rowRemoved, this, [=](int row)
            {
                delRows.push_back(row);
                saveTableData();
            });
    }

    void mLoadCombinedPanel::checkLoadTypeConflict()
    {
        auto allData = table_->getAllData();
        if (allData.isEmpty())
            return;

        bool hasTransient = false;
        bool hasFrequency = false;
        auto& mgr = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary);
        for (const auto& pair : allData)
        {
            const QString& loadName = pair.second;
            if (loadName.isEmpty())
                continue;
            int loadid = mgr.getIDByLoadBoundaryName(loadName);
            auto refLoad = mgr.getLoadBoundaryData(loadid);
            if (!refLoad)
                continue;
            MxFunctions::LoadBoundaryType type = refLoad->getLoadBoundaryType();
            if (type == MxFunctions::LOAD_IM_TRANSIENT)
                hasTransient = true;
            else if (type == MxFunctions::LOAD_IM_FREQDOMAIN || type == MxFunctions::LOAD_IM_ACOFREQDOMAIN)
                hasFrequency = true;
        }

        if (hasTransient && hasFrequency)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("组合载荷中不能同时包含时域载荷与频域/声学频域载荷！"), true);
        }
    }

    //表格类
    LoadTableWidget::LoadTableWidget(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void LoadTableWidget::initUI()
    {

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        QHBoxLayout* controlLayout = new QHBoxLayout;

        m_btnAdd = new QPushButton("");
        m_btnRemove = new QPushButton("");
        m_btnClear = new QPushButton("");
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
        m_table->setColumnCount(2);

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        // ========== 关键：关闭自动拉伸 ==========
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        m_table->setHorizontalHeaderLabels({ "比例系数","调用载荷名称" });
        m_table->setEditTriggers(QTableWidget::NoEditTriggers);

        m_table->setColumnWidth(0, 200);
        m_table->setColumnWidth(1, 400);

        // 整行选中 + 高亮样式（格子全部实心变色）
        m_table->setAlternatingRowColors(true);
        m_table->setSelectionBehavior(QTableWidget::SelectRows);
        m_table->setSelectionMode(QTableWidget::ExtendedSelection);
        m_table->setStyleSheet(R"(
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


        connect(m_btnAdd, &QPushButton::clicked, this, &LoadTableWidget::addRow);
        connect(m_btnRemove, &QPushButton::clicked, this, &LoadTableWidget::removeRow);
        connect(m_btnClear, &QPushButton::clicked, this, &LoadTableWidget::clearTable);
        connect(m_editRowNum, &QLineEdit::returnPressed, this, &LoadTableWidget::setRowByInput);

        mainLayout->addLayout(controlLayout);
        mainLayout->addWidget(m_table);
    }

    void LoadTableWidget::addRow()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        QLineEdit* edit = new QLineEdit;
        edit->setPlaceholderText("系数");
        connect(edit, &QLineEdit::editingFinished, this, &LoadTableWidget::onRatioEditFinished);
        m_table->setCellWidget(row, 0, edit);

        QComboBox* cbox = new QComboBox;
        cbox->setModel(new QStandardItemModel(cbox));
        cbox->clear();
        cbox->addItems(m_loadOptions);
        connect(cbox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &LoadTableWidget::onLoadNameChanged);
        m_table->setCellWidget(row, 1, cbox);

        setWidgetTransparent(row);

        emit rowAdded(row);
    }

    void LoadTableWidget::removeRow()
    {
        // 获取选中的行
        QModelIndexList selectedRows = m_table->selectionModel()->selectedRows();

        // 如果有选中行 → 删除选中（支持多行）
        if (!selectedRows.isEmpty())
        {
            // 倒序排序：大的行号排在前面
            std::sort(selectedRows.begin(), selectedRows.end(), [](const QModelIndex& a, const QModelIndex& b)
                {
                return a.row() > b.row();
                });

            // 循环删除
            for (const QModelIndex& idx : selectedRows)
            {
                int row = idx.row();
                m_table->removeRow(row);
                emit rowRemoved(row);
            }
            return;
        }

        // 没有选中 → 删除最后一行
        if (m_table->rowCount() > 0)
        {
            int row = m_table->rowCount() - 1;
            m_table->removeRow(row);
            emit rowRemoved(row);
        }
    }

    void LoadTableWidget::clearTable()
    {
        m_table->setRowCount(0);
        emit rowRemoved(0);
    }

    void LoadTableWidget::setRowByInput()
    {
        bool ok;
        int num = m_editRowNum->text().toInt(&ok);
        if (!ok || num < 1) return;
        m_table->setRowCount(0);

        m_table->setUpdatesEnabled(false);
        m_table->viewport()->setUpdatesEnabled(false);
        m_table->horizontalHeader()->setUpdatesEnabled(false);
        m_table->verticalHeader()->setUpdatesEnabled(false);

        for (int i = 0; i < num; i++)
            addRow();

        m_table->verticalHeader()->setUpdatesEnabled(true);
        m_table->horizontalHeader()->setUpdatesEnabled(true);
        m_table->viewport()->setUpdatesEnabled(true);
        m_table->setUpdatesEnabled(true);

        m_table->update();
    }
    // ===================== 对外接口实现 =====================
    QList<QPair<double, QString>> LoadTableWidget::getAllData() const
    {
        QList<QPair<double, QString>> list;
        for (int i = 0; i < m_table->rowCount(); i++) {
            QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 0));
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 1));
            double coeff = edit->text().toDouble();
            QString name = cbox->currentText();
            list.append(qMakePair(coeff, name));
        }
        return list;
    }

    void LoadTableWidget::setLoadOptions(const QStringList& options)
    {
        m_loadOptions = options;
        syncAllComboBox();
    }

    void LoadTableWidget::addLoadOption(const QString& option)
    {
        if (!m_loadOptions.contains(option)) {
            m_loadOptions.append(option);
            syncAllComboBox();
        }
    }

    void LoadTableWidget::removeLoadOption(const QString& option)
    {
        m_loadOptions.removeOne(option);
        syncAllComboBox();
    }

    void LoadTableWidget::clearLoadOptions()
    {
        m_loadOptions.clear();
        syncAllComboBox();
    }

    QStringList LoadTableWidget::getLoadOptions() const
    {
        return m_loadOptions;
    }

    QStringList LoadTableWidget::getAllCurrentLoadNames() const
    {
        QStringList nameList;

        //遍历每一行，取出当前选中的载荷名
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 1));
            if (cbox)
            {
                nameList << cbox->currentText();//取出当前值
            }
        }
        return nameList;
    }

    // 同步所有行的下拉框数据
    void LoadTableWidget::syncAllComboBox()
    {
        for (int i = 0; i < m_table->rowCount(); i++) {
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 1));
            if (cbox) {
                cbox->clear();
                cbox->addItems(m_loadOptions);
            }
        }
    }

    void LoadTableWidget::setWidgetTransparent(int row)
    {
        for (int col = 0; col < 2; ++col)
        {
            QWidget* w = m_table->cellWidget(row, col);
            if (!w) continue;

            w->setStyleSheet(R"(
            QWidget {
                background: transparent;
                border: none;
                outline: none;
            }
            QLineEdit {
                padding: 2px 4px;
                background: transparent;
                border: none;
            }
            QComboBox {
                background: transparent;
                border: none;
                padding: 2px 4px;
            }
            QComboBox::drop-down { border: none; }
            QComboBox::down-arrow { image: none; }
        )");
        }
    }

    void LoadTableWidget::setAllWidgetTransparent()
    {
        for (int row = 0; row < m_table->rowCount(); ++row)
            setWidgetTransparent(row);
    }

    // ==================== 信号槽 ====================
    void LoadTableWidget::onRatioEditFinished()
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
        if (!edit) return;

        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->cellWidget(row, 0) == edit) {
                double val = edit->text().toDouble();
                emit ratioEditFinished(row, val);
                break;
            }
        }
    }

    void LoadTableWidget::onLoadNameChanged(const QString& text)
    {
        QComboBox* cbox = qobject_cast<QComboBox*>(sender());
        if (!cbox) return;

        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->cellWidget(row, 1) == cbox) {
                emit loadNameChanged(row, text);
                break;
            }
        }
    }

    // 设置单行下拉框
    void LoadTableWidget::setCurrentComboText(int row, const QString& text)
    {
        if (row < 0 || row >= m_table->rowCount())
            return;

        QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1));
        if (cbox)
            cbox->setCurrentText(text);
    }

    QList<double> LoadTableWidget::getAllRatios() const
    {
        QList<double> ratios;
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 0));
            if (edit)
            {
                ratios << edit->text().toDouble();
            }
        }
        return ratios;
    }

    void LoadTableWidget::setRatio(int row, double value)
    {
        if (row < 0 || row >= m_table->rowCount())
            return;
        QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 0));
        if (edit)
        {
            edit->setText(MxFunctions::mNumberToString(value));
        }
    }

    void LoadTableWidget::setRowCount(int count)
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
        // 需要删除行（从最后一行往前删）
        else if (count < current)
        {
            for (int i = current - 1; i >= count; --i)
            {
                m_table->removeRow(i);
                emit rowRemoved(i); // 发送删除行号
            }
        }
    }
}