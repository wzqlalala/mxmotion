#pragma once
#pragma execution_character_set("utf-8")

#include "mAttachedPopupBase.h"

#include <QMap>
#include <QPair>
#include <QVector>
#include <QStringList>
#include <QSet>
#include <QHash>

#include <unordered_map>
#include <vector>

#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mGlobalSignals.h"

class QTableView;
class QHBoxLayout;

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

namespace MechMainWindow
{
    /**
     * @brief 控件内部行表模型（QAbstractTableModel）
     *
     * 大数据量（万行级）下以模型驱动 QTableView，数据仍在 mDiscreteDataControl::_rows，
     * 模型只暴露访问接口，不复制行数据；行结构/内容变化由控件调用对应 reset/insert/remove 通知。
     */
    class MMAINWINDOW_EXPORT mDiscreteRowTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit mDiscreteRowTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

        void bind(const QVector<QPair<QPair<int, int>, QStringList>>* rows,
            const QStringList* idHeaders, const QStringList* valueHeaders,
            const int* componentCount, const bool* picking)
        {
            beginResetModel();
            _rows = rows; _idHeaders = idHeaders; _valueHeaders = valueHeaders;
            _componentCount = componentCount; _picking = picking;
            endResetModel();
        }

        void notifyReset() { beginResetModel(); endResetModel(); }
        // 行增删必须按 begin → 改 _rows → end 三段式调用：
        // end* 内视图会立即回查 rowCount()/data()，数据必须先于 end 同步
        void beginInsertRow(int row) { beginInsertRows(QModelIndex(), row, row); }
        void endInsertRow() { endInsertRows(); }
        void beginRemoveRow(int row) { beginRemoveRows(QModelIndex(), row, row); }
        void endRemoveRow() { endRemoveRows(); }

        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() || !_rows) return 0;
            // 空表保留一行提示（「尚未添加…」），与 QTableWidget 版空态一致
            return _rows->isEmpty() ? 1 : _rows->size();
        }

        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() || !_rows) return 1;
            const int idCols = (*_picking) ? 1 : (_idHeaders ? _idHeaders->size() : 0);
            const int dataCols = (*_picking) ? 0 : (_componentCount ? *_componentCount : 1);
            const int opCol = (*_picking) ? 0 : 1;
            return qMax(1, idCols + dataCols + opCol);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        Qt::ItemFlags flags(const QModelIndex& index) const override;

    signals:
        /** 用户编辑了某单元格（row/col 为模型索引，text 为 trimmed 文本） */
        void cellEditedByUser(int row, int col, const QString& text);

    public:
        // 列语义查询（供控件编辑/删除逻辑判定列类型）
        bool isIdColumn(int col) const;
        bool isOpColumn(int col) const;
        int idColumnCount() const { return (*_picking) ? 1 : (_idHeaders ? _idHeaders->size() : 0); }
        int valueIndexOf(int col) const { return col - idColumnCount(); }
        bool isEmptyPlaceholderRow(int row) const { return _rows && _rows->isEmpty() && row == 0; }
        /** 删除列悬停行（自绘委托高亮用），-1 表示无悬停 */
        void setHoverRow(int row);

        // 空态提示文案（拾取中/非拾取中），由控件传入保证与面板标题一致
        void bindEmptyText(const QString* title) { _title = title; }

    private:
        const QVector<QPair<QPair<int, int>, QStringList>>* _rows = nullptr;
        const QStringList* _idHeaders = nullptr;
        const QStringList* _valueHeaders = nullptr;
        const int* _componentCount = nullptr;
        const bool* _picking = nullptr;
        const QString* _title = nullptr;
        int _hoverRow = -1;
    };

    /**
     * @brief 删除列自绘委托
     *
     * 用 QStyledItemDelegate 绘制 + editorEvent 处理点击，彻底替代 setIndexWidget：
     * indexWidget 部件在弹窗 hide/reparent/model reset 后会被视图静默弃用且不会重建，
     * 导致"二次打开弹窗删除按钮消失/点击无效"；委托绘制无此生命周期问题。
     */
    class MMAINWINDOW_EXPORT mDiscreteDeleteDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit mDiscreteDeleteDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        void paint(QPainter* painter, const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

        bool editorEvent(QEvent* event, QAbstractItemModel* model,
            const QStyleOptionViewItem& option, const QModelIndex& index) override;

    signals:
        /** 用户点击了 row 行的删除按钮 */
        void deleteRowRequested(int row);
    };

    /**
     * @brief 通用离散数据控件（《通用结构软件界面UI设计规则》2.2.6 吸附式弹窗 D3/D6 形态）
     *
     * 吸附式弹窗的外壳与公共行为（入口、窗口头、吸附定位、点击外部关闭、导入/导出
     * 文件拓展区、消息条、宿主销毁收尾）由 mAttachedPopupBase 承担；本类负责
     * D3 对象离散数据内容：
     *  - 完整数据表：标识列 + 分量 1..N 列 + 行删除列，底部操作行「拾取对象 / ＋ / 清空」，
     *    底部工具栏「导入 / 导出 / 渲染云图(可选)」；表格随数据量弹性伸缩，超出后内部滚动；
     *  - 窄列表（D6 拾取状态）：仅显示标识列与「完成拾取」，拾取期间弹窗不会因点击外部区域关闭。
     *
     * 数据以字符串保存，保留用户输入精度；分量值允许留空（空 = 未输入，不自动填 0）。
     *
     * 拾取由本控件直接驱动：
     * 点击「拾取对象」→ 按 PickSpec::pickTypes 激活视口拾取过滤器并订阅选择容器信号；
     * 视口每次拾取完成即按业务 key 去重追加为表格行；「完成拾取/关闭」退订并结束。
     * 本控件不渲染任何拾取高亮，高亮生命周期完全由宿主自行管理。
     * 导入/导出当前仅提供界面，通过 std::function 回调预留接口。
     */
    class MMAINWINDOW_EXPORT mDiscreteDataControl : public mAttachedPopupBase
    {
        Q_OBJECT

    public:
        /**
         * @brief 拾取对象描述
         * @param typeName   对象中文名，如 "节点"、"单元"、"单元面"
         * @param idHeaders  标识列标题，节点/单元为 {"节点编号"}，单元面为 {"单元编号","面编号"}
         * @param keyOf      由拾取结果生成业务 key（如单元面取 {result_id, result_id2}）
         * @param pickTypes  允许拾取的数据类型白名单（如仅 Node），空集合表示全部类型
         * @param pickFilter 视口拾取过滤器序号（与 mMechGlobalSignals::activePickSig 一致：
         *                   5=节点、6=单元边、7=单元面、8=单元），<0 表示不强制切换
         */
        struct PickSpec
        {
            QString typeName;
            QStringList idHeaders;
            std::function<QPair<int, int>(const RenderPickResultData&)> keyOf;
            QSet<MxFunctions::PickDataType> pickTypes;
            int pickFilter = -1;
        };

        /**
         * @param title        入口/弹窗标题，如 "节点数据"
         * @param valueHeaders 分量列标题（决定初始分量数），如 {"X分量","Y分量","Z分量"}
         * @param pickSpec     拾取对象描述；传入空指针表示不提供拾取（如点云手动录入）
         * @param parent       父对象
         */
        explicit mDiscreteDataControl(const QString& title,
            const QStringList& valueHeaders,
            const PickSpec* pickSpec,
            QWidget* parent = nullptr);
        ~mDiscreteDataControl() override;

        /** 一行数据结构：first=标识，second=分量(允许空串) */
        using Row = QPair<QPair<int, int>, QStringList>;

        /** 覆盖式设置全部行（不清空弹窗状态，外部数据变化时调用） */
        void setRows(const QVector<Row>& rows);
        /** 当前全部行 */
        QVector<Row> rows() const;
        int rowCount() const { return _rows.size(); }
        bool isPicking() const { return _picking; }

        /*
        * 运行期更新拾取对象描述（宿主"位置"联动：节点/单元/单元面）。
        * 描述未变化时直接返回（避免重复重建表格与选中态回种）；
        * 拾取会话进行中先结束会话（内部补发一次 dataChanged，宿主可先按旧类型同步）。
        */
        void setPickSpec(const PickSpec& pickSpec);

        /*
        * 运行期设置分量列数（宿主"分量数量"联动）：行数据按新列数截断/补空，
        * 纯数据/视图规整，不发射 dataChanged（行集合与宿主数据的同步由宿主负责）。
        */
        void setComponentCount(int count);

        /** 渲染按钮可选：默认隐藏；显示后点击触发回调（由宿主接入真实渲染） */
        void setRenderButtonVisible(bool visible, const QString& text = QString());

        /** 导入参数（界面当前值，随「导入」执行回调传给宿主）；类型统一为基类 ImportParams */
        using ImportParams = mAttachedPopupBase::ImportParams;
        /** 导入/导出预留接口：设置后点击面板中的执行按钮回调，由宿主实现真实读写 */
        void setImportHandler(const std::function<void(const ImportParams&)>& handler);
        void setExportHandler(const std::function<void(const QString& filePath)>& handler);
        /** 渲染按钮回调 */
        void setRenderHandler(const std::function<void()>& handler);
        /** 入口按钮从控件中剖分（所有权交给宿主布局），等价于 detachEntryWidget */
        void detachEntryButton(QWidget* newParent);
        /** 停止拾取（面板退出编辑/切换定义方式时调用） */
        void stopPicking();
        /** 关闭弹窗（面板重建/退出编辑时调用；deactivatePick 控制是否同时结束拾取） */
        void closePopup(bool deactivatePick = true);
        /** 清空视口残留的拾取选中态与高亮渲染层（选择容器数据与高亮一并清除，
         *  行数据保留；如云图开启时需要无遮挡的干净视口） */
        void clearPickingSelection();
        /** 把当前行写回视口选择容器并刷新选中态高亮：拾取会话外需要恢复高亮的
         *  场景显式调用（如云图关闭后）；拾取会话由 startPicking 自动回种 */
        void seedSelectionFromRows();

    signals:
        /** 行数据发生任何变化（编辑/增删/清空/拾取追加/外部 setRows） */
        void dataChanged();
        /** 单行分量被编辑：key 为业务标识，valueIndex 为分量序号 */
        void cellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        // mAttachedPopupBase 内容钩子
        void buildContent(QWidget* body, QVBoxLayout* layout) override;
        QSize contentSizeHint() const override;
        /** 拾取窄列表不应用 480px 普通态下限（D8 拾取模式） */
        int popupMinimumWidth() const override;
        /** 关闭弹窗前先结束拾取会话 */
        void prepareClosePopup() override;
        void onPopupResized() override;
        /** 首开 show 后重建表格（隐藏期 Stretch 列未分配实际宽度，span/行高需按最终布局生效） */
        void onPopupShown() override;

    private:
        void rebuildTable();
        void appendRow(const QPair<int, int>& key, bool focus = true);
        /** 删除指定行（供删除列委托与清空复用） */
        void removeRowAt(int row);
        void updateEntrySummary();
        void updateActionStates();
        /** 末行操作格与表格列对齐：拾取格对齐对象标识列、＋合并数值列（拉伸）、清空格对齐删除列 */
        void syncActionCellWidths();

        void startPicking();
        void finishPicking();
        /** 视口拾取完成：读取选择容器，按白名单过滤后追加为行 */
        void onViewportPickFinished();
        /** 拾取高亮与弹窗可见性同步：非拾取期间任何行变化（删除/清空/外部设值）后，
         *  弹窗打开时清空残留并按当前行回种，弹窗关闭时只清空不回种 */
        void syncSelectionToRows();
        /** 弹窗经任意路径关闭后熄灭拾取高亮（连接 popupClosed 信号） */
        void onPopupClosed();

        void changeComponents(int delta);
        QString idTextOf(const QPair<int, int>& key) const;
        QString componentHeader(int index) const;
        /** 查找与本行已填标识位完全相同的其它行（去重判定；除本行外无命中返回 -1） */
        int findDuplicateIdRow(const QPair<int, int>& key, int exceptRow) const;

        QStringList _valueHeaders;
        int _componentCount = 1;
        PickSpec _pickSpec;
        bool _hasPick = false;

        // 内容子树（弹窗的子对象，随弹窗销毁；QPointer 托管判空可靠）
        QPointer<QTableView> _table;
        QPointer<mDiscreteRowTableModel> _tableModel;
        QPointer<mDiscreteDeleteDelegate> _deleteDelegate;
        QPointer<QWidget> _actionsBar;
        QPointer<QPushButton> _pickBtn;
        QPointer<QPushButton> _addRowBtn;
        QPointer<QPushButton> _clearBtn;

        QVector<Row> _rows;
        bool _picking = false;
        bool _syncingTable = false;

        // 视口拾取订阅（直接驱动，不经过通用拾取控件）
        QMetaObject::Connection _viewportConn;
    };
}
