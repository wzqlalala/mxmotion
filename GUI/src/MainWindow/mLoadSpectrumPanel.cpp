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

#include "mLoadSpectrumPanel.h"
#include "mDockLayoutManager.h"
#include "mLoadSpectrum.h"
#include "mOtherFunction.h"

using namespace MechFunctionData;

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadSpectrumPanel::mLoadSpectrumPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadSpectrumPanel::~mLoadSpectrumPanel()
    {
        clearInputs();
    }
    void mLoadSpectrumPanel::initInputs()
    {
        QStringList type_list = {//谱载荷类型
            "位移D",
            "速度V",
            "加速度A"
        };
        _type_box.initWidget("谱载荷类型");
        _type_box.combobox->addItems(type_list);
        connect(_type_box.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadSpectrumPanel::slot_spectrumTypeChanged);

        initTable();//初始话表格
    }

    void mLoadSpectrumPanel::clearInputs()
    {
        inputWidgets.clear();
        if (table_) delete table_;
    }

    void mLoadSpectrumPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        controlModify = true;//控制数据修改
        //设置延迟时间,相位数据 
        std::shared_ptr<MechCaseData::mLoadSpectrum> pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadSpectrum>(loadData);
        table_->clearTableNoEmit();//清除数据
        //设置谱载荷类型
        QString type = pressurePtr->type_;
        if (type == "")//不存在类型值，则为默认
        {
            _type_box.combobox->setCurrentIndex(0);
        }
        else
        {
            _type_box.combobox->setCurrentText(type);
        }
        //获取所有函数名称
        auto allFunctionNames = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionNames();
        table_->setLoadOptions(allFunctionNames);
        QList<QString> dampList = pressurePtr->dampValueList_;//获取阻尼值
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int loadId = loadData->getLoadBoundaryId();
        QList<int> fucIDs = mapRelation.getMapRelation<QList<int>>(
                { Physical_LoadBoundary, loadId },
                Physical_Function
            );
        table_->setRowCount(dampList.size());
        for (int i = 0; i < fucIDs.size(); i++)
            {
                QString name = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(fucIDs[i]);
                table_->setCurrentComboText(i, name);//设置函数选项
            }
        for (int i = 0; i < dampList.size(); i++)
        {
            table_->setRatio(i, dampList.at(i).toDouble());//设置阻尼值
        }
        controlModify = false;//控制数据修改
    }

    QVector<QWidget*> mLoadSpectrumPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(_type_box.propwid);
        wds.append(table_);
        return wds;
    }

    QVector<QWidget*> mLoadSpectrumPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    void mLoadSpectrumPanel::saveSpectrumType()
    {
        if (controlModify)//反显过程中不回写，避免默认值覆盖模型数据
            return;
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadSpectrum>(_loadData);
        if (!load)
            return;
        load->type_ = _type_box.combobox->currentText();
        notifyLoadChanged();
    }

    void mLoadSpectrumPanel::saveTableData()
    {
        if (controlModify)//反显过程中不回写，避免默认值覆盖模型数据
            return;
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadSpectrum>(_loadData);
        if (!load)
            return;

        QList<QPair<double, QString>> tableData = table_->getAllData();
        QList<QString> validNames;
        for (const auto& pair : tableData)
        {
            QString curveName = pair.second;
            if (!curveName.isEmpty())
            {
                validNames.append(curveName);
            }
        }

        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

        for (int i = 0; i < validNames.size(); i++)
        {
            QString name = validNames[i];
            int id = getCVM->getModelData()
                ->getManager<MechFunctionData::mFunctionManager>(Physical_Function)
                .getIDByFunctionName(name);

            if (id != -1)
            {
                mapRelation.deleteTabelMapRelation(
                    { Physical_LoadBoundary, load->getLoadBoundaryId() },
                    { Physical_Function, id },
                    i
                );

                mapRelation.appendTableMapRelation(
                    { Physical_LoadBoundary, load->getLoadBoundaryId() },
                    { Physical_Function, id },
                    i
                );
            }
            else
            {
                qWarning() << "Function not found:" << name;
            }
        }

        load->dampValueList_ = table_->getAllRatiosQString();
        notifyLoadChanged();
    }

    void mLoadSpectrumPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadSpectrumPanel::getPickedData()
    {
        return _picks;
    }

    void mLoadSpectrumPanel::slot_combinedLoadTable()
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table_);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mLoadSpectrumPanel::initTable()
    {
        table_ = new LoadSpectrumTableWidget();
        connect(table_, &LoadSpectrumTableWidget::ratioEditFinished, this, [=](int row, double value)
            {
                Q_UNUSED(row)
                Q_UNUSED(value)
                saveTableData();
            });
        connect(table_, &LoadSpectrumTableWidget::loadNameChanged, this, [=](int row, const QString& name)
            {
                Q_UNUSED(row)
                Q_UNUSED(name)
                saveTableData();
            });
        connect(table_, &LoadSpectrumTableWidget::rowAdded, this, [=](int row)
            {
                Q_UNUSED(row)
                saveTableData();
            });
        connect(table_, &LoadSpectrumTableWidget::rowRemoved, this, [=](int row)
            {
                delRows.push_back(row);
                saveTableData();
            });
    }

    void mLoadSpectrumPanel::slot_spectrumTypeChanged(int idx)
    {
        Q_UNUSED(idx)
        saveSpectrumType();
    }

    //表格类
    LoadSpectrumTableWidget::LoadSpectrumTableWidget(QWidget* parent)
        : QWidget(parent)
    {
        initUI();
    }

    void LoadSpectrumTableWidget::initUI()
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

        m_table = new QTableWidget(this);
        m_table->setColumnCount(2);//列数为2
        m_table->setFixedHeight(400);
        //m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        // ========== 关键：关闭自动拉伸 ==========
        //m_table->horizontalHeader()->setStretchLastSection(false);
        //m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        m_table->setHorizontalHeaderLabels({ "谱曲线","阻尼值" });
        //m_table->setEditTriggers(QTableWidget::NoEditTriggers);


        // 获取水平表头
        QHeaderView* header = m_table->horizontalHeader();

        // 设置所有列为拉伸模式（各列按比例分配）
        header->setSectionResizeMode(QHeaderView::Stretch);

        // 整行选中 + 高亮样式（格子全部实心变色）
        //m_table->setAlternatingRowColors(true);
        //m_table->setSelectionBehavior(QTableWidget::SelectRows);
        //m_table->setSelectionMode(QTableWidget::ExtendedSelection);

        m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
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


        connect(m_btnAdd, &QPushButton::clicked, this, &LoadSpectrumTableWidget::addRow);
        connect(m_btnRemove, &QPushButton::clicked, this, &LoadSpectrumTableWidget::removeRow);
        connect(m_btnClear, &QPushButton::clicked, this, &LoadSpectrumTableWidget::clearTable);
        connect(m_editRowNum, &QLineEdit::returnPressed, this, &LoadSpectrumTableWidget::setRowByInput);

        mainLayout->addLayout(controlLayout);
        mainLayout->addWidget(m_table);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        this->setLayout(mainLayout);
    }
    
    void LoadSpectrumTableWidget::addRow()
    {
        addRowNoEmit();
        emit rowAdded(m_table->rowCount() - 1);
    }

    void LoadSpectrumTableWidget::addRowNoEmit()
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        //获取所有函数名称
        auto allFunctionNames = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionNames();

        //函数选项
        QComboBox* cbox = new QComboBox(this);
        cbox->setStyleSheet(
            "QComboBox QAbstractItemView::item:selected {"
            "    background-color: #3399FF;"
            "    color: white;"
            "}"
        );
        cbox->addItems(allFunctionNames);
        connect(cbox, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &LoadSpectrumTableWidget::onLoadNameChanged);
        m_table->setCellWidget(row, 0, cbox);

        //阻尼值
        QLineEdit* edit = new QLineEdit(this);
        edit->setPlaceholderText("阻尼值");
        edit->setText("0.0");
        connect(edit, &QLineEdit::editingFinished, this, &LoadSpectrumTableWidget::onRatioEditFinished);
        m_table->setCellWidget(row, 1, edit);
    }

    void LoadSpectrumTableWidget::removeRow()
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
                if (m_table->rowCount() > 1)
                {
                    m_table->removeRow(row);
                    emit rowRemoved(row);
                }
            }
            return;
        }

        // 没有选中 → 删除最后一行
        if (m_table->rowCount() > 1)
        {
            m_table->removeRow(m_table->rowCount() - 1);
            emit rowRemoved(m_table->rowCount() - 1);
        }
    }

    void LoadSpectrumTableWidget::clearTable()
    {
        m_table->setRowCount(0);
        emit rowRemoved(0);
    }

    void LoadSpectrumTableWidget::setRowByInput()
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
    QList<QPair<double, QString>> LoadSpectrumTableWidget::getAllData() const
    {
        QList<QPair<double, QString>> list;
        for (int i = 0; i < m_table->rowCount(); i++) {
            QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 1));
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 0));
            double coeff = edit->text().toDouble();
            QString name = cbox->currentText();
            list.append(qMakePair(coeff, name));
        }
        return list;
    }

    void LoadSpectrumTableWidget::setLoadOptions(const QStringList& options)
    {
        m_loadOptions = options;
        syncAllComboBox();
    }

    void LoadSpectrumTableWidget::addLoadOption(const QString& option)
    {
        if (!m_loadOptions.contains(option)) {
            m_loadOptions.append(option);
            syncAllComboBox();
        }
    }

    void LoadSpectrumTableWidget::removeLoadOption(const QString& option)
    {
        m_loadOptions.removeOne(option);
        syncAllComboBox();
    }

    void LoadSpectrumTableWidget::clearLoadOptions()
    {
        m_loadOptions.clear();
        syncAllComboBox();
    }

    QStringList LoadSpectrumTableWidget::getLoadOptions() const
    {
        return m_loadOptions;
    }

    QStringList LoadSpectrumTableWidget::getAllCurrentLoadNames() const
    {
        QStringList nameList;

        //遍历每一行，取出当前选中的载荷名
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 0));
            if (cbox)
            {
                nameList << cbox->currentText();//取出当前值
            }
        }
        return nameList;
    }

    // 同步所有行的下拉框数据
    void LoadSpectrumTableWidget::syncAllComboBox()
    {
        for (int i = 0; i < m_table->rowCount(); i++) {
            QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(i, 0));
            if (cbox) {
                cbox->clear();
                cbox->addItems(m_loadOptions);
            }
        }
    }

    void LoadSpectrumTableWidget::setWidgetTransparent(int row)
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

    void LoadSpectrumTableWidget::setAllWidgetTransparent()
    {
        for (int row = 0; row < m_table->rowCount(); ++row)
            setWidgetTransparent(row);
    }

    // ==================== 信号槽 ====================
    void LoadSpectrumTableWidget::onRatioEditFinished()
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
        if (!edit) return;

        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->cellWidget(row, 1) == edit) {
                double val = edit->text().toDouble();
                emit ratioEditFinished(row, val);
                break;
            }
        }
    }

    void LoadSpectrumTableWidget::onLoadNameChanged(const QString& text)
    {
        QComboBox* cbox = qobject_cast<QComboBox*>(sender());
        if (!cbox) return;

        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->cellWidget(row, 0) == cbox) {
                emit loadNameChanged(row, text);
                break;
            }
        }
    }

    // 设置单行下拉框
    void LoadSpectrumTableWidget::setCurrentComboText(int row, const QString& text)
    {
        if (row < 0 || row >= m_table->rowCount())
            return;

        QComboBox* cbox = qobject_cast<QComboBox*>(m_table->cellWidget(row, 0));
        if (cbox)
            cbox->setCurrentText(text);
    }

    QList<double> LoadSpectrumTableWidget::getAllRatios() const
    {
        QList<double> ratios;
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 1));
            if (edit)
            {
                ratios << edit->text().toDouble();
            }
        }
        return ratios;
    }

    QList<QString> LoadSpectrumTableWidget::getAllRatiosQString() const
    {
        QList<QString> ratios;
        for (int i = 0; i < m_table->rowCount(); ++i)
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(i, 1));
            if (edit)
            {
                ratios << edit->text();
            }
        }
        return ratios;
    }

    void LoadSpectrumTableWidget::setRatio(int row, double value)
    {
        if (row < 0 || row >= m_table->rowCount())
            return;
        QLineEdit* edit = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 1));
        if (edit)
        {
            edit->setText(MxFunctions::mNumberToString(value));
        }
    }

    void LoadSpectrumTableWidget::setRowCount(int count)
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
                addRowNoEmit();
            }
        }
        // 需要删除行（从最后一行往前删）
        else if (count < current)
        {
            for (int i = current - 1; i >= count; --i)
            {
                if (m_table->rowCount() >= 1)
                {
                    m_table->removeRow(i);
                }
            }
        }
    }
    void LoadSpectrumTableWidget::clearTableNoEmit()
    {
        m_table->setRowCount(0);
    }
}