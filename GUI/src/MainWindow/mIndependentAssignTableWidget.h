#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QAbstractItemView>
#include <map>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <QVariant>
#include <QVector>

#include "mCommonTableWidget.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

namespace MechMainWindow
{
    /**
     * @brief 通用独立赋值表格控件
     *
     * 数据结构统一为 std::map<RenderPickResultData, QVector<QVariant>>。
     * 表格内部使用 TableRowKey 作为逻辑 key 进行排序、去重、数据匹配，
     * 同时保留原始 RenderPickResultData 作为 fullKey，用于对外信号和回写数据类，
     * 避免 part_id / result_id2 被随意填充为 -1 / 0 导致数据完整性问题。
     *
     * 表格按行组织：
     *   - 固定列：与 key 对应的 ID 列（1~3 列），由 pickKeyColumns 指定
     *   - 可编辑列：value 中的每个分量
     *
     * 参数说明（见 initTable）：
     *   - pickKeyColumns: 每行固定列的表头，如 {"节点ID"}、{"单元编号","单元面编号"}
     *   - pickKeyExtractors: 从 RenderPickResultData 中提取列值的 lambda，数量须与 pickKeyColumns 一致
     *   - valueHeaders: 可编辑值列的表头，如 {"X分量","Y分量","Z分量"}
     *   - valueConverters: 每个可编辑列的 QVariant<->QString 转换器
     */
    class MMAINWINDOW_EXPORT mIndependentAssignTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        struct ColumnConverter
        {
            std::function<QString(const QVariant&)> toString;
            std::function<QVariant(const QString&)> fromString;
        };

        explicit mIndependentAssignTableWidget(QWidget* parent = nullptr)
            : QWidget(parent)
        {
            initUI();
        }

        ~mIndependentAssignTableWidget() override = default;

        /**
         * @brief 初始化表格结构
         * @param pickKeyColumns 固定 ID 列表头
         * @param pickKeyExtractors 从 RenderPickResultData 提取每列值的函数，返回 int
         * @param valueHeaders 可编辑值列表头
         * @param valueConverters 每个可编辑列的转换器（可空：默认数值转换）
         */
        void initTable(const QStringList& pickKeyColumns,
            const std::vector<std::function<int(const RenderPickResultData&)>>& pickKeyExtractors,
            const QStringList& valueHeaders,
            const std::vector<ColumnConverter>& valueConverters = {})
        {
            _pickKeyColumns = pickKeyColumns;
            _pickKeyExtractors = pickKeyExtractors;
            _valueHeaders = valueHeaders;
            _valueConverters = valueConverters;
            if (_valueConverters.size() < _valueHeaders.size())
            {
                _valueConverters.resize(_valueHeaders.size(), defaultNumberConverter());
            }

            buildTableHeader();
        }

        /**
         * @brief 按拾取数据更新表格；程序化重建，不触发 dataChanged。
         * @param targetType 本表格关心的 SetType
         * @param picks 当前拾取数据（可包含节点/单元/面等）
         * @param data 独立赋值数据（std::map）
         */
        void updateTableData(MxFunctions::PickDataType targetType,
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks,
            const std::map<RenderPickResultData, QVector<QVariant>>& data)
        {
            _targetType = targetType;
            _data = data;
            _rows.clear();

            _table->blockSignals(true);
            _table->setUpdatesEnabled(false);
            _table->setRowCount(0);

            // 1. 收集目标类型的拾取，并按业务 key 去重，保留完整 RenderPickResultData
            std::map<TableRowKey, RenderPickResultData> uniqueFullKeys;
            auto itPicks = picks.find(targetType);
            if (itPicks != picks.end())
            {
                for (const auto& pick : itPicks->second)
                {
                    TableRowKey key = toTableRowKey(targetType, pick);
                    auto it = uniqueFullKeys.find(key);
                    if (it == uniqueFullKeys.end())
                    {
                        uniqueFullKeys[key] = pick;
                    }
                    else
                    {
                        // 同一业务 key 已存在：优先保留 part_id 非 -1 的完整 key
                        if (it->second.part_id == -1 && pick.part_id != -1)
                        {
                            it->second = pick;
                        }
                    }
                }
            }

            // 2. 将独立赋值数据也按业务 key 索引，便于匹配
            std::map<TableRowKey, QVector<QVariant>> dataByKey;
            for (const auto& p : _data)
            {
                dataByKey[toTableRowKey(targetType, p.first)] = p.second;
            }

            // 3. 构建内部行数据（已按 TableRowKey 升序排列）
            _rows.reserve(static_cast<int>(uniqueFullKeys.size()))
;            for (const auto& entry : uniqueFullKeys)
            {
                TableRow row;
                row.logicalKey = entry.first;
                row.fullKey = entry.second;

                auto itData = dataByKey.find(entry.first);
                if (itData != dataByKey.end())
                {
                    row.values = itData->second;
                }

                _rows.push_back(row);
            }

            // 4. 重建 QTableWidget 显示
            const int keyColCount = static_cast<int>(_pickKeyColumns.size());
            const int valueColCount = static_cast<int>(_valueHeaders.size());
            _table->setRowCount(static_cast<int>(_rows.size()));

            int rowIndex = 0;
            for (const auto& row : _rows)
            {
                // 固定 ID 列
                for (int c = 0; c < keyColCount; ++c)
                {
                    int val = _pickKeyExtractors[c](row.fullKey);
                    QTableWidgetItem* item = new QTableWidgetItem(QString::number(val));
                    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                    item->setTextAlignment(Qt::AlignCenter);
                    _table->setItem(rowIndex, c, item);
                }

                // 可编辑值列
                for (int c = 0; c < valueColCount; ++c)
                {
                    const int col = keyColCount + c;
                    QString text;
                    if (c < row.values.size())
                    {
                        const QVariant& val = row.values[c];
                        if (!val.isNull() && val.isValid())
                        {
                            text = _valueConverters[c].toString(val);
                        }
                    }
                    QTableWidgetItem* item = new QTableWidgetItem(text);
                    item->setTextAlignment(Qt::AlignCenter);
                    _table->setItem(rowIndex, col, item);
                }

                ++rowIndex;
            }

            _table->setUpdatesEnabled(true);
            _table->blockSignals(false);
        }

        int rowCount() const { return _table ? _table->rowCount() : 0; }

        /**
         * @brief 读取表格数据到 std::map
         * @param outData 输出数据
         * @param picks 当前拾取数据（用于必要时补回 part_id 等完整 key）
         */
        void getTableData(std::map<RenderPickResultData, QVector<QVariant>>& outData,
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks)
        {
            outData.clear();

            const int keyColCount = static_cast<int>(_pickKeyColumns.size());
            const int valueColCount = static_cast<int>(_valueHeaders.size());

            int rowIndex = 0;
            for (const auto& row : _rows)
            {
                RenderPickResultData key = row.fullKey;

                // 防御：如果 fullKey 的 part_id 为 -1 但业务上需要 part_id，尝试从 picks 补回
                if (key.part_id == -1 && _targetType != MxFunctions::PickDataType::Node)
                {
                    int partId = findPartIdFromPicks(key.result_id, key.result_id2, picks);
                    if (partId != -1)
                        key.part_id = partId;
                }

                QVector<QVariant> values;
                values.reserve(valueColCount);
                for (int c = 0; c < valueColCount; ++c)
                {
                    const int col = keyColCount + c;
                    QTableWidgetItem* item = _table->item(rowIndex, col);
                    QString text = item ? item->text().trimmed() : QString();
                    if (text.isEmpty())
                    {
                        values.append(QVariant());
                    }
                    else
                    {
                        values.append(_valueConverters[c].fromString(text));
                    }
                }

                outData[key] = values;
                ++rowIndex;
            }
        }

        const std::map<RenderPickResultData, QVector<QVariant>>& data() const
        {
            return _data;
        }

    signals:
        void dataChanged();
        /**
         * @brief 单元格增量编辑信号
         * @param key 被编辑行对应的原始完整 RenderPickResultData
         * @param valueIndex 被编辑的值列索引（0-based）
         * @param value 新值；无效 QVariant 表示清空
         */
        void cellEdited(const RenderPickResultData& key, int valueIndex, const QVariant& value);

    private slots:
        void onCellClickedKeepSelection(int row, int col)
        {
            const int keyColCount = static_cast<int>(_pickKeyColumns.size());
            if (col < keyColCount || col >= _table->columnCount())
                return;
            QTableWidgetItem* item = _table->item(row, col);
            if (!item)
                return;
            if (_table->selectedItems().size() > 1)
            {
                _table->editItem(item);
            }
        }

        void onCellChangedSync(int row, int col)
        {
            const int keyColCount = static_cast<int>(_pickKeyColumns.size());
            if (col < keyColCount || col >= _table->columnCount())
                return;

            QTableWidgetItem* currentItem = _table->item(row, col);
            if (!currentItem)
                return;

            QString newText = currentItem->text().trimmed();
            bool blocked = _table->signalsBlocked();
            _table->blockSignals(true);

            QList<int> affectedRows;
            affectedRows.append(row);
            for (QTableWidgetItem* item : _table->selectedItems())
            {
                if (item && item->column() == col && item->row() != row)
                {
                    item->setText(newText);
                    affectedRows.append(item->row());
                }
            }
            _table->blockSignals(blocked);

            for (int r : affectedRows)
            {
                emitCellEditedForRow(r, col);
            }
            emit dataChanged();
        }

    private:
        /**
         * @brief 表格内部使用的逻辑 key，只包含当前 SetType 下真正参与业务匹配的字段
         */
        struct TableRowKey
        {
            int id = -1;      // 主 ID：节点/单元/几何对象 ID
            int subId = -1;   // 二级 ID：面序号等
            int partId = -1;  // 部件 ID（仅当需要参与匹配时）

            bool operator==(const TableRowKey& o) const
            {
                return id == o.id && subId == o.subId && partId == o.partId;
            }

            bool operator<(const TableRowKey& o) const
            {
                if (id != o.id) return id < o.id;
                if (subId != o.subId) return subId < o.subId;
                return partId < o.partId;
            }
        };

        struct TableRow
        {
            TableRowKey logicalKey;            // 用于排序、去重、匹配数据
            RenderPickResultData fullKey;      // 从原始拾取中保留的完整 key
            QVector<QVariant> values;          // 当前行的值（可能为空）
        };

        static TableRowKey toTableRowKey(MxFunctions::PickDataType type, const RenderPickResultData& d)
        {
            TableRowKey key;
            switch (type)
            {
            case MxFunctions::PickDataType::Node:
                key.id = d.result_id;
                key.subId = -1;
                key.partId = -1;
                break;

            case MxFunctions::PickDataType::Element:
                key.id = d.result_id;
                key.subId = -1;
                key.partId = d.part_id;
                break;

            case MxFunctions::PickDataType::ElementFace:
                key.id = d.result_id;
                key.subId = d.result_id2;
                // 单元面业务匹配不依赖 part_id
                key.partId = -1;
                break;

            case MxFunctions::PickDataType::GeoPoint:
            case MxFunctions::PickDataType::GeoEdge:
            case MxFunctions::PickDataType::GeoFace:
            case MxFunctions::PickDataType::GeoSolid:
                key.id = d.result_id;
                key.subId = -1;
                key.partId = d.part_id;
                break;

            default:
                key.id = d.result_id;
                key.subId = d.result_id2;
                key.partId = d.part_id;
                break;
            }
            return key;
        }

        static RenderPickResultData fromTableRowKey(MxFunctions::PickDataType type, const TableRowKey& key)
        {
            switch (type)
            {
            case MxFunctions::PickDataType::Node:
                return RenderPickResultData(key.id, -1, -1);

            case MxFunctions::PickDataType::Element:
                return RenderPickResultData(key.id, key.partId, -1);

            case MxFunctions::PickDataType::ElementFace:
                return RenderPickResultData(key.id, key.partId, key.subId);

            case MxFunctions::PickDataType::GeoPoint:
            case MxFunctions::PickDataType::GeoEdge:
            case MxFunctions::PickDataType::GeoFace:
            case MxFunctions::PickDataType::GeoSolid:
                return RenderPickResultData(key.id, key.partId, -1);

            default:
                return RenderPickResultData(key.id, key.partId, key.subId);
            }
        }

        void emitCellEditedForRow(int row, int col)
        {
            if (row < 0 || row >= static_cast<int>(_rows.size()))
                return;

            const int keyColCount = static_cast<int>(_pickKeyColumns.size());
            const RenderPickResultData& fullKey = _rows[row].fullKey;

            QTableWidgetItem* valueItem = _table->item(row, col);
            QString text = valueItem ? valueItem->text().trimmed() : QString();

            const int valueIndex = col - keyColCount;
            if (text.isEmpty())
            {
                emit cellEdited(fullKey, valueIndex, QVariant());
            }
            else
            {
                emit cellEdited(fullKey, valueIndex, _valueConverters[valueIndex].fromString(text));
            }
        }

        int findPartIdFromPicks(int resultId, int resultId2,
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks) const
        {
            auto it = picks.find(_targetType);
            if (it == picks.end()) return -1;

            TableRowKey targetKey = toTableRowKey(_targetType, RenderPickResultData(resultId, -1, resultId2));
            for (const auto& p : it->second)
            {
                if (toTableRowKey(_targetType, p) == targetKey)
                {
                    return p.part_id;
                }
            }
            return -1;
        }

        void buildTableHeader()
        {
            QStringList headers = _pickKeyColumns;
            headers.append(_valueHeaders);
            _table->setColumnCount(headers.size());
            _table->setHorizontalHeaderLabels(headers);
        }

        void initUI()
        {
            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(5);

            _table = new mCommonTableWidget(this);
            _table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            _table->verticalHeader()->setVisible(false);
            _table->setAlternatingRowColors(true);
            _table->setSelectionBehavior(QAbstractItemView::SelectItems);
            _table->setSelectionMode(QAbstractItemView::ExtendedSelection);
            _table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
            _table->setStyleSheet(R"(
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
            mainLayout->addWidget(_table);

            connect(_table, &QTableWidget::cellClicked, this, &mIndependentAssignTableWidget::onCellClickedKeepSelection);
            connect(_table, &QTableWidget::cellChanged, this, &mIndependentAssignTableWidget::onCellChangedSync);
        }

        static ColumnConverter defaultNumberConverter()
        {
            return {
                [](const QVariant& v) { return QString::number(v.toDouble()); },
                [](const QString& s) { return QVariant(s.toDouble()); }
            };
        }

        mCommonTableWidget* _table = nullptr;

        QStringList _pickKeyColumns;
        std::vector<std::function<int(const RenderPickResultData&)>> _pickKeyExtractors;
        QStringList _valueHeaders;
        std::vector<ColumnConverter> _valueConverters;

        MxFunctions::PickDataType _targetType = MxFunctions::PickDataType::None;
        std::map<RenderPickResultData, QVector<QVariant>> _data;
        std::vector<TableRow> _rows;
    };
}
