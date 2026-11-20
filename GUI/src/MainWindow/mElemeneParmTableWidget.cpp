#include "mElemeneParmTableWidget.h"
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QAction>
#include <QPainter>
#include <QSet>
#include <QSignalBlocker>


#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"

//BasicData
#include "mPropertyBase.h"
#include "mPropertyManager.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mCoordinateManager.h"
#include "mNodeValidor.h"
#include "mGlobalSignals.h"
#include "mModelEnum.h"
#include "mOtherFunction.h"

using namespace MechElementData;
using namespace MechCoordinateData;
namespace MechMainWindow
{
    mElemeneParmHeaderView::mElemeneParmHeaderView(Qt::Orientation orientation, QWidget* parent)
        : QHeaderView(orientation, parent) {
    }

    void mElemeneParmHeaderView::addSpan(int startColumn, int spanCount, const QString& text) {
        if (spanCount > 1) {
            m_spanMap.insert(startColumn, { spanCount, text });
        }
    }

    void mElemeneParmHeaderView::clearSpans() {
        m_spanMap.clear();
        this->viewport()->update();
    }

    void mElemeneParmHeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
        if (!painter) return;

        int spanStartCol = -1;
        SpanConfig currentSpan;

        // 1. 判断当前重绘的列是否在某个合并区间内
        for (auto it = m_spanMap.begin(); it != m_spanMap.end(); ++it) {
            int start = it.key();
            int count = it.value().spanCount;
            if (logicalIndex >= start && logicalIndex < start + count) {
                spanStartCol = start;
                currentSpan = it.value();
                break;
            }
        }

        // 2. 如果不在合并区间内，走默认绘制逻辑
        if (spanStartCol == -1) {
            QHeaderView::paintSection(painter, rect, logicalIndex);
            return;
        }

        // 3. 无论当前触发重绘的是合并区间内的哪一列，
        // 都要以起始列为基准，计算出整个跨列的绝对矩形坐标。
        int startX = this->sectionViewportPosition(spanStartCol);
        int totalWidth = 0;
        for (int i = 0; i < currentSpan.spanCount; ++i) {
            totalWidth += this->sectionSize(spanStartCol + i);
        }
        // 算出完整的大格子
        QRect mergedRect(startX, rect.y(), totalWidth, rect.height());

        painter->save();
        QStyleOptionHeader opt;
        initStyleOption(&opt); // 获取原生操作系统的表头样式基础配置

        // 4. 【体验优化：联动高亮】
        // 检查这个大表头涵盖的几列中，只要有任意一列的单元格被选中了，
        // 我们就让整个大表头呈现“高亮/按下”的状态，防止出现半边白半边灰的撕裂感。
        bool isAnyColSelected = false;
        if (this->selectionModel()) {
            for (int i = 0; i < currentSpan.spanCount; ++i) {
                if (this->selectionModel()->columnIntersectsSelection(spanStartCol + i, this->rootIndex())) {
                    isAnyColSelected = true;
                    break;
                }
            }
        }

        // 强制覆盖状态
        if (isAnyColSelected) {
            opt.state |= QStyle::State_Sunken; // 选中时的下凹高亮状态
            opt.state |= QStyle::State_On;
        }
        else {
            opt.state &= ~QStyle::State_Sunken;
            opt.state &= ~QStyle::State_On;
        }

        opt.rect = mergedRect;
        opt.text = currentSpan.text;
        opt.textAlignment = Qt::AlignCenter;

        // 借助 QStyle 绘制，保留操作系统原生的表头凸起、边框样式
        style()->drawControl(QStyle::CE_Header, &opt, painter, this);
        painter->restore();
    }

    mElemeneParmTableWidget::mElemeneParmTableWidget(QWidget* parent)
        : QTableWidget(parent), m_isEditing(false) {
        initTable();
    }

    mElemeneParmTableWidget::~mElemeneParmTableWidget() {}

    void mElemeneParmTableWidget::initTable() {
        this->setSelectionBehavior(QAbstractItemView::SelectItems);
        this->setSelectionMode(QAbstractItemView::ExtendedSelection);
        this->setAlternatingRowColors(true);

        // 使用自定义委托，让多选下拉/输入编辑时自动同步所有选中单元格
        mElemeneParmTableDelegate* delegate = new mElemeneParmTableDelegate(this);
        connect(delegate, &mElemeneParmTableDelegate::requestBatchUpdate,
                this, &mElemeneParmTableWidget::batchUpdateSelectedCells);
        this->setItemDelegate(delegate);

        this->setEditTriggers(QAbstractItemView::NoEditTriggers);

        mElemeneParmHeaderView* customHeader = new mElemeneParmHeaderView(Qt::Horizontal, this);
        this->setHorizontalHeader(customHeader);

        this->setStyleSheet(R"(
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

        // 取消右键批量修改菜单，改为多选后直接在单元格上编辑
        this->setContextMenuPolicy(Qt::NoContextMenu);

        // 监听 QTableWidgetItem (LineEdit 类型的单元格) 的实时修改
        connect(this, &QTableWidget::itemChanged, this, &mElemeneParmTableWidget::onCellItemChanged);
    }

    void mElemeneParmTableWidget::unedit() {
        m_isEditing = false;
        m_currentIds.clear();
        this->clear();
        this->setRowCount(0);
        this->setColumnCount(0);
        m_columnConfigs.clear();

        mTableDataWindow::getInstance()->resetInitialTable();
        //mTableDataWindow::getInstance()->clearAndReset(0, 0);
    }

    void mElemeneParmTableWidget::edit(const QList<int>&ids, ElementType eletype)
    {
        m_isEditing = false; // 挂起监听
        this->clear();

        m_currentIds = ids;
        qSort(m_currentIds.begin(), m_currentIds.end());
        m_currentType = eletype;
        m_columnConfigs.clear();

        // 提前分配好行数
        this->setRowCount(m_currentIds.size());

        mElemeneParmHeaderView* headerView = qobject_cast<mElemeneParmHeaderView*>(this->horizontalHeader());
        if (headerView) {
            headerView->clearSpans();
        }

        mDataElement* elementData = nullptr;
        if (getCVM->getModelData())
        {
            elementData = getCVM->getModelData()->getElementData();
        }
        else
        {
            return;
        }

        auto& coordManager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);

        // 提取公共资源（如坐标系名称列表）
        QStringList coordNames;
        coordNames = coordManager.getAllCoordinateNames();

        // ================= 核心合并区域 =================
        // 在同一个类型判断分支中，完成：配置表头 -> 应用表头 -> 遍历获取真实数据 -> 填充单元格

        if (m_currentType == ElementType::D3B2) {
            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "参考节点编号", ElemeneParmColumnConfig::LineEdit, {}, "refNode" });
            m_columnConfigs.append({ "参考矢量", ElemeneParmColumnConfig::LineEdit, {}, "refVector" });
            m_columnConfigs.append({ "偏置类型", ElemeneParmColumnConfig::ComboBox, {"BGG", "GGG"}, "offsetType" });
            m_columnConfigs.append({ "偏置X1", ElemeneParmColumnConfig::LineEdit, {}, "x1" });
            m_columnConfigs.append({ "偏置Y1", ElemeneParmColumnConfig::LineEdit, {}, "y1" });
            m_columnConfigs.append({ "偏置Z1", ElemeneParmColumnConfig::LineEdit, {}, "z1" });
            m_columnConfigs.append({ "偏置X2", ElemeneParmColumnConfig::LineEdit, {}, "x2" });
            m_columnConfigs.append({ "偏置Y2", ElemeneParmColumnConfig::LineEdit, {}, "y2" });
            m_columnConfigs.append({ "偏置Z2", ElemeneParmColumnConfig::LineEdit, {}, "z2" });
           
            applyHeaders(); // 提炼的一个小函数，用于立即生效表头

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mBeamElement* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mBeamElement*>(elementBase);

                    // 直接使用真实数据进行填充，无需字符串比对
                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], element->_node);
                    setCellControl(row, 2, m_columnConfigs[2], element->_beamValue);
                    setCellControl(row, 3, m_columnConfigs[3], element->_biasType);
                    setCellControl(row, 4, m_columnConfigs[4], element->_X1);
                    setCellControl(row, 5, m_columnConfigs[5], element->_Y1);
                    setCellControl(row, 6, m_columnConfigs[6], element->_Z1);
                    setCellControl(row, 7, m_columnConfigs[7], element->_X2);
                    setCellControl(row, 8, m_columnConfigs[8], element->_Y2);
                    setCellControl(row, 9, m_columnConfigs[9], element->_Z2);
                }
            }
        }

        else if (m_currentType == ElementType::CPDSG || m_currentType == ElementType::T3 || m_currentType == ElementType::TRIA3 || m_currentType == ElementType::DSG || m_currentType == ElementType::MITC3 || m_currentType == ElementType::MITC3D ||
            m_currentType == ElementType::CTRIA3 || m_currentType == ElementType::CQUAD4 || m_currentType == ElementType::Q4 ||
            m_currentType == ElementType::MITC4 || m_currentType == ElementType::MITC4D || m_currentType == ElementType::GE4) {

            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "方向(坐标系)", ElemeneParmColumnConfig::ComboBox, coordNames, "coordinateValue" });
            m_columnConfigs.append({ "方向(转角值)", ElemeneParmColumnConfig::LineEdit, {}, "angleValue" });
            m_columnConfigs.append({ "偏置值", ElemeneParmColumnConfig::LineEdit, {}, "biasValue" });

            applyHeaders();

            if (headerView) {
                headerView->addSpan(1, 2, "方向设定");
            }

            // 2. 遍历填充数据
            // 假设这里获取对应类型单元的 Manager
            // auto& elemManager = getCVM->getModelData()->getManager<...>(...);

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mShellElement* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mShellElement*>(elementBase);

                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], coordManager.getCoordinateNameByID(element->_coordID));
                    setCellControl(row, 2, m_columnConfigs[2], element->_angle);
                    setCellControl(row, 3, m_columnConfigs[3], element->_bias);
                }
            }
        }
        else if (m_currentType == ElementType::Mass1)
        {
            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "质量", ElemeneParmColumnConfig::LineEdit, {}, "massValue" });
            m_columnConfigs.append({ "X", ElemeneParmColumnConfig::LineEdit, {}, "X" });
            m_columnConfigs.append({ "Y", ElemeneParmColumnConfig::LineEdit, {}, "Y" });
            m_columnConfigs.append({ "Z", ElemeneParmColumnConfig::LineEdit, {}, "Z" });
            m_columnConfigs.append({ "I11", ElemeneParmColumnConfig::LineEdit, {}, "I11" });
            m_columnConfigs.append({ "I21", ElemeneParmColumnConfig::LineEdit, {}, "I21" });
            m_columnConfigs.append({ "I22", ElemeneParmColumnConfig::LineEdit, {}, "I22" });
            m_columnConfigs.append({ "I31", ElemeneParmColumnConfig::LineEdit, {}, "I31" });
            m_columnConfigs.append({ "I32", ElemeneParmColumnConfig::LineEdit, {}, "I32" });
            m_columnConfigs.append({ "I33", ElemeneParmColumnConfig::LineEdit, {}, "I33" });

            applyHeaders();

            // 2. 遍历填充数据
            // 假设这里获取对应类型单元的 Manager
            // auto& elemManager = getCVM->getModelData()->getManager<...>(...);

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mMassElement* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mMassElement*>(elementBase);

                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], element->_mass);
                    setCellControl(row, 2, m_columnConfigs[2], element->_x);
                    setCellControl(row, 3, m_columnConfigs[3], element->_y);
                    setCellControl(row, 4, m_columnConfigs[4], element->_z);
                    setCellControl(row, 5, m_columnConfigs[5], element->_I11);
                    setCellControl(row, 6, m_columnConfigs[6], element->_I21);
                    setCellControl(row, 7, m_columnConfigs[7], element->_I22);
                    setCellControl(row, 8, m_columnConfigs[8], element->_I31);
                    setCellControl(row, 9, m_columnConfigs[9], element->_I32);
                    setCellControl(row, 10, m_columnConfigs[10], element->_I33);
                }
            }
        }
        else if (m_currentType == ElementType::CMASS2)
        {
            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "质量", ElemeneParmColumnConfig::LineEdit, {}, "massValue" });
            m_columnConfigs.append({ "自由度", ElemeneParmColumnConfig::ComboBox, {"1", "2", "3", "4", "5", "6",}, "dofValue" });

            applyHeaders();

            // 2. 遍历填充数据
            // 假设这里获取对应类型单元的 Manager
            // auto& elemManager = getCVM->getModelData()->getManager<...>(...);

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mCmass2Element* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mCmass2Element*>(elementBase);

                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], element->_mass);
                    setCellControl(row, 2, m_columnConfigs[2], element->_dof);
                }
            }
        }
        else if (m_currentType == ElementType::CBUSH || m_currentType == ElementType::CBUSH1)
        {
            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "坐标系", ElemeneParmColumnConfig::ComboBox, coordNames, "coordinateValue" });
            m_columnConfigs.append({ "偏置值", ElemeneParmColumnConfig::LineEdit, {}, "biasValue" });
            m_columnConfigs.append({ "偏置向量坐标系",  ElemeneParmColumnConfig::ComboBox, coordNames, "coordinateBiasValue" });
            m_columnConfigs.append({ "方向向量X1",  ElemeneParmColumnConfig::LineEdit, {}, "vectorValue1" });
            m_columnConfigs.append({ "方向向量X2",  ElemeneParmColumnConfig::LineEdit, {}, "vectorValue2" });
            m_columnConfigs.append({ "方向向量X3",  ElemeneParmColumnConfig::LineEdit, {}, "vectorValue3" });
            //m_columnConfigs.append({ "分量偏移量",  ElemeneParmColumnConfig::LineEdit, {}, "vectorBiasValue" });//暂时不用分量偏移量

            applyHeaders();

            // 2. 遍历填充数据
            // 假设这里获取对应类型单元的 Manager
            // auto& elemManager = getCVM->getModelData()->getManager<...>(...);

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mCbushElement* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mCbushElement*>(elementBase);

                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], coordManager.getCoordinateNameByID(element->_cid));
                    setCellControl(row, 2, m_columnConfigs[2], element->_s);
                    setCellControl(row, 3, m_columnConfigs[3], coordManager.getCoordinateNameByID(element->_ocid));
                    setCellControl(row, 4, m_columnConfigs[4], element->_x1);
                    setCellControl(row, 5, m_columnConfigs[5], element->_x2);
                    setCellControl(row, 6, m_columnConfigs[6], element->_x3);
                    //setCellControl(row, 5, m_columnConfigs[5], element->_si);
                }
            }
        }
        else if (m_currentType == ElementType::ELAS1 || m_currentType == ElementType::VISCOUSDAMP || m_currentType == ElementType::DASHPOT1 || m_currentType == ElementType::DASHPOT2)
        {
            // 1. 构建列配置
            m_columnConfigs.append({ "ID", ElemeneParmColumnConfig::LineEdit, {}, "id" });
            m_columnConfigs.append({ "自由度", ElemeneParmColumnConfig::ComboBox, {"1", "2", "3", "4", "5", "6",}, "dofValue" });
           
            applyHeaders();

            // 2. 遍历填充数据
            // 假设这里获取对应类型单元的 Manager
            // auto& elemManager = getCVM->getModelData()->getManager<...>(...);

            for (int row = 0; row < m_currentIds.size(); ++row) {
                int rowId = m_currentIds[row];

                mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
                mElas1Element* element = nullptr;
                if (elementBase != nullptr)
                {
                    element = dynamic_cast<mElas1Element*>(elementBase);

                    setCellControl(row, 0, m_columnConfigs[0], rowId);
                    setCellControl(row, 1, m_columnConfigs[1], element->_dof);
                }
            }
        }

        m_isEditing = true; // 激活实时监听
        mTableDataWindow::getInstance()->setCustomWidget(this);
    }

    // 提炼出的辅助函数，用于将 config 转化为真实的 Qt 表头
    void mElemeneParmTableWidget::applyHeaders() {
        this->setColumnCount(m_columnConfigs.size());
        QStringList headers;
        for (const auto& config : m_columnConfigs) {
            headers << config.headerName;
        }
        this->setHorizontalHeaderLabels(headers);
    }

    void mElemeneParmTableWidget::setCellControl(int row, int col, const ElemeneParmColumnConfig& config, const QVariant& value) {
        if (col == 0) {
            QTableWidgetItem* idItem = new QTableWidgetItem(MxFunctions::mNumberToString(value.toInt()));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable); // ID 只读
            idItem->setTextAlignment(Qt::AlignCenter);
            this->setItem(row, col, idItem);
            return;
        }

        // 先将传入的初始值转为字符串
        QString displayText = value.toString();

        // 如果是下拉框类型，我们需要做特殊的空值兜底处理
        if (config.controlType == ElemeneParmColumnConfig::ComboBox) {
            QStringList options = config.comboOptions;
            emit requestComboOptions(config.dataKey, options);

            // 【修复2：兜底逻辑】：如果外部传入的初始值为空，且下拉列表有选项
            // 我们强制让它显示下拉列表的第一个选项，模拟真实 QComboBox 的默认行为
            if (displayText.isEmpty() && !options.isEmpty()) {
                displayText = options.first();

                // 可选操作：既然我们给了默认值，是否需要立马把这个默认值通过信号存入底层数据？
                // 如果需要，可以反向触发一次更新信号，保证底层数据和 UI 此时一致：
                // emit dataUpdated(m_currentIds[row], config.dataKey, displayText);
            }

            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setData(Role_ControlType, config.controlType);
            item->setData(Role_DataKey, config.dataKey);
            item->setData(Role_ComboOptions, options);
            this->setItem(row, col, item);
        }
        // 如果是普通的输入框类型
        else {
            QTableWidgetItem* item = new QTableWidgetItem(displayText);
            item->setData(Role_ControlType, config.controlType);
            item->setData(Role_DataKey, config.dataKey);
            this->setItem(row, col, item);
        }
    }

    // 批量同步：将基准单元格所在列的值同步到 prevSelected 中所有同列的选中单元格
    void mElemeneParmTableWidget::batchSyncCurrentColumn(const QModelIndex& editIdx, const QList<QTableWidgetItem*>& prevSelected)
    {
        if (!editIdx.isValid()) return;

        int col = editIdx.column();
        if (col <= 0) return; // 第0列为 ID 列，不批量同步

        QTableWidgetItem* sourceItem = item(editIdx.row(), col);
        if (!sourceItem) return;

        QString newText = sourceItem->text();
        bool blocked = blockSignals(true);
        for (QTableWidgetItem* selectedItem : prevSelected)
        {
            if (selectedItem && selectedItem->column() == col && selectedItem->row() != editIdx.row())
            {
                selectedItem->setText(newText);
            }
        }
        blockSignals(blocked);

        // 强制刷新视图，确保批量修改后的值显示出来
        viewport()->update();

        // 手动触发数据回写，使所有被修改的单元格同步到底层数据
        for (QTableWidgetItem* selectedItem : prevSelected)
        {
            if (selectedItem && selectedItem->column() == col && selectedItem->row() != editIdx.row())
            {
                onCellItemChanged(selectedItem);
            }
        }
    }

    void mElemeneParmTableWidget::moveToNextRowSameColumn()
    {
        QModelIndexList selected = selectedIndexes();
        if (selected.isEmpty()) return;

        // 以最后选中的单元格作为基准
        QModelIndex lastIdx = selected.last();
        int nextRow = lastIdx.row() + 1;
        int col = lastIdx.column();

        // 到达最后一行则保持在当前最后一行（同 Excel 习惯）
        if (nextRow >= rowCount()) {
            nextRow = rowCount() - 1;
        }

        QTableWidgetItem* nextItem = item(nextRow, col);
        if (!nextItem) return;

        // 清除旧选区，选中目标单元格，并滚动到视野内
        setCurrentCell(nextRow, col, QItemSelectionModel::ClearAndSelect);
        scrollToItem(nextItem, QAbstractItemView::EnsureVisible);
    }

    // ========== 实时监听与数据回写 ==========

    void mElemeneParmTableWidget::onCellItemChanged(QTableWidgetItem* item) {
        // 1. 安全校验：非编辑状态（例如正在 loadData 时）或改了只读的 ID 列，直接跳过
        if (!m_isEditing || !item || item->column() == 0) return;

        // 2. 行号越界校验
        if (item->row() < 0 || item->row() >= m_currentIds.size()) return;

        // 3. 提取身份信息和修改后的新值
        int rowId = m_currentIds[item->row()];
        QString dataKey = item->data(Role_DataKey).toString();
        QString newValue = item->text();

        if (!getCVM)
            return;

        mDataElement* elementData = getCVM->getModelData()->getElementData();
        mElementBase* elementBase = elementData->getElementDataByMeshID(rowId);
        if (!elementBase)
            return;

        auto& coordManager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate);
        // 3. 根据表格类型，路由到具体的 Manager 进行持久化存储
        if (m_currentType == ElementType::D3B2) {

            mBeamElement* element = dynamic_cast<mBeamElement*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "refNode") {
                element->_node = newValue;
            }
            else if (dataKey == "refVector") {
                element->_beamValue = newValue;
            }
            else if (dataKey == "offsetType") {
                element->_biasType = newValue;
            }
            else if (dataKey == "x1") {
                element->_X1 = newValue;
            }
            else if (dataKey == "y1") {
                element->_Y1 = newValue;
            }
            else if (dataKey == "z1") {
                element->_Z1 = newValue;
            }
            else if (dataKey == "x2") {
                element->_X2 = newValue;
            }
            else if (dataKey == "y2") {
                element->_Y2 = newValue;
            }
            else if (dataKey == "z2") {
                element->_Z2 = newValue;
            }
        }
        else if (m_currentType == ElementType::CPDSG || m_currentType == ElementType::T3 || m_currentType == ElementType::TRIA3 || m_currentType == ElementType::DSG || m_currentType == ElementType::MITC3 || m_currentType == ElementType::MITC3D ||
            m_currentType == ElementType::CTRIA3 || m_currentType == ElementType::CQUAD4 || m_currentType == ElementType::Q4 ||
            m_currentType == ElementType::MITC4 || m_currentType == ElementType::MITC4D || m_currentType == ElementType::GE4)
        {
            mShellElement* element = dynamic_cast<mShellElement*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "angleValue") {
                element->_angle = newValue;
            }
            else if (dataKey == "coordinateValue") {
                element->_coordID = coordManager.getIDByCoordinateName(newValue);
            }
            else if (dataKey == "biasValue") {
                element->_bias = newValue;
            }
            // elemManager.updateData(rowId, realData);
        }
        else if (m_currentType == ElementType::Mass1)
        {
            mMassElement* element = dynamic_cast<mMassElement*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "massValue") {
                element->_mass = newValue;
            }
            else if (dataKey == "x") {
                element->_x = newValue;
            }
            else if (dataKey == "y") {
                element->_y = newValue;
            }
            else if (dataKey == "z") {
                element->_z = newValue;
            }
            else if (dataKey == "I11") {
                element->_I11 = newValue;
            }
            else if (dataKey == "I21") {
                element->_I21 = newValue;
            }
            else if (dataKey == "I22") {
                element->_I22 = newValue;
            }
            else if (dataKey == "I31") {
                element->_I31 = newValue;
            }
            else if (dataKey == "I32") {
                element->_I32 = newValue;
            }
            else if (dataKey == "I33") {
                element->_I33 = newValue;
            }
        }
        else if (m_currentType == ElementType::CMASS2)
        {
            mCmass2Element* element = dynamic_cast<mCmass2Element*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "massValue") {
                element->_mass = newValue;
            }
            else if (dataKey == "dofValue") {
                element->_dof = newValue;
            }
        }
        else if (m_currentType == ElementType::CBUSH || m_currentType == ElementType::CBUSH1)
        {
            mCbushElement* element = dynamic_cast<mCbushElement*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "coordinateValue") {
                element->_cid = coordManager.getIDByCoordinateName(newValue);
            }
            else if (dataKey == "biasValue") {
                element->_s = newValue;
            }
            else if (dataKey == "coordinateBiasValue") {
                element->_ocid = coordManager.getIDByCoordinateName(newValue);
            }
            else if (dataKey == "vectorValue1") {
                element->_x1 = newValue;          
            }
            else if (dataKey == "vectorValue2") {
                element->_x2 = newValue;
            }
            else if (dataKey == "vectorValue3") {
                element->_x3 = newValue;
            }
            else if (dataKey == "vectorBiasValue") {
                element->_si = newValue;
            }
        }
        else if (m_currentType == ElementType::ELAS1 || m_currentType == ElementType::VISCOUSDAMP || m_currentType == ElementType::DASHPOT1 || m_currentType == ElementType::DASHPOT2)
        {
            mElas1Element* element = dynamic_cast<mElas1Element*>(elementBase);
            if (element == nullptr)
                return;

            if (dataKey == "dofValue") {
                element->_dof = newValue;
            }
        }
        // (可选) 如果外部还需要知道数据变了（比如要刷新三维视图 OSG/OpenGL），可以在这里抛出信号
        // emit dataUpdated(rowId, dataKey, newValue); 
    }

    // ========== 多选编辑同步批量修改 ==========

    void mElemeneParmTableWidget::batchUpdateSelectedCells(int col, const QString& newValue)
    {
        if (!m_isEditing || col <= 0 || col >= m_columnConfigs.size()) return;

        // 避免循环触发：先把 itemChanged 短暂屏蔽，再统一修改
        QSignalBlocker blocker(this);
        QSet<int> updatedRows;
        QList<QTableWidgetSelectionRange> ranges = this->selectedRanges();
        for (const auto& range : ranges) {
            for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                QTableWidgetItem* item = this->item(row, col);
                if (item && item->text() != newValue) {
                    item->setText(newValue);
                    updatedRows.insert(row);
                }
            }
        }

        // 更新视图
        this->viewport()->update();

        // 手动触发数据回写，所有被修改的单元格都会通过 onCellItemChanged 同步到底层
        for (int row : updatedRows) {
            QTableWidgetItem* item = this->item(row, col);
            if (item) {
                onCellItemChanged(item);
            }
        }
    }
}