#pragma once
#pragma execution_character_set("utf-8")

#include "mAttachedPopupBase.h"

#include <QPointer>
#include <QStringList>
#include <QVector>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "mmainwindow_global.h"

class QTableView;
class QToolButton;

namespace MechMainWindow
{
    /**
     * @brief 点云行表模型（QAbstractTableModel）
     *
     * 行数据常驻 mPointCloudDataControl::_rows，模型只暴露访问接口（不复制），
     * 行结构/内容变化由控件调用对应 reset/insert/remove 通知。
     * 列结构：X / Y / Z 固定坐标列 + 分量 1..N 可变列 + 末列行删除（D2/D7）。
     */
    class MMAINWINDOW_EXPORT mPointCloudRowTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit mPointCloudRowTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

        void bind(const QVector<QStringList>* rows, const int* componentCount, const QString* title)
        {
            beginResetModel();
            _rows = rows; _componentCount = componentCount; _title = title;
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
            // 空表保留一行提示（「尚未添加点云数据」），与离散控件空态一致
            return _rows->isEmpty() ? 1 : _rows->size();
        }

        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() || !_rows) return 1;
            // X/Y/Z + 分量列 + 删除列；无拾取态（点云无对象标识列，D2）
            return qMax(1, valueColumnCount() + 1);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

    signals:
        /** 用户编辑了某单元格（row/col 为模型索引，text 为 trimmed 文本） */
        void cellEditedByUser(int row, int col, const QString& text);

    public:
        // X/Y/Z 固定列 + 可变分量列总数
        int valueColumnCount() const { return fixedColumnCount() + (_componentCount ? *_componentCount : 1); }
        static int fixedColumnCount() { return 3; }
        /** 删除列（末列）判定 */
        bool isOpColumn(int col) const { return col == columnCount() - 1; }
        bool isEmptyPlaceholderRow(int row) const { return _rows && _rows->isEmpty() && row == 0; }
        /** 删除列悬停行（自绘委托高亮用），-1 表示无悬停 */
        void setHoverRow(int row);

    private:
        const QVector<QStringList>* _rows = nullptr;
        const int* _componentCount = nullptr;
        const QString* _title = nullptr;
        int _hoverRow = -1;
    };

    /**
     * @brief 删除列自绘委托（垃圾桶图标 + 点击删除）
     *
     * 与 mDiscreteDeleteDelegate 同一绘制方案；独立成类避免跨控件
     * qobject_cast 耦合（两个模型的行结构不同）。
     */
    class MMAINWINDOW_EXPORT mPointCloudDeleteDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit mPointCloudDeleteDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        void paint(QPainter* painter, const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

        bool editorEvent(QEvent* event, QAbstractItemModel* model,
            const QStyleOptionViewItem& option, const QModelIndex& index) override;

    signals:
        /** 用户点击了 row 行的删除按钮 */
        void deleteRowRequested(int row);
    };

    /**
     * @brief 点云数据控件（《通用结构软件界面UI设计规则》2.2.6 吸附式弹窗 D2 点云数据表格）
     *
     * 吸附式弹窗外壳与公共行为（入口、窗口头、吸附定位、点击外部关闭、导入/导出
     * 文件拓展区、消息条、宿主销毁收尾）由 mAttachedPopupBase 承担；本类负责
     * D2 点云内容：
     *  - X / Y / Z 固定坐标列 + 分量 1..N 可变列 + 行删除列，无序号列、无拾取入口；
     *  - 表格末行单元格操作：「＋」合并全部坐标与数值列（拉伸），「清空」对齐删除列，
     *    与表格连续共边、无按钮间距、无圆角（D2：末行坐标与数值列合并为＋）；
     *  - 底部工具栏「导入 / 导出 / 渲染云图(可选)」；不提供「绘制曲线」（D2）。
     *
     * 数据以字符串保存，保留用户输入精度；坐标与分量值允许留空
     * （空 = 未输入，不自动填 0；映射/渲染时由业务跳过无效行）。
     *
     * 与 mDiscreteDataControl（D3 对象离散数据）的差异：无拾取会话、无对象标识列、
     * 无拾取窄列表形态；X/Y/Z 为固定前缀列，可变分量列从「分量 1」起编号（D7：
     * 点云的 X/Y/Z 列保持固定，可变分量只增减数值列）。
     */
    class MMAINWINDOW_EXPORT mPointCloudDataControl : public mAttachedPopupBase
    {
        Q_OBJECT

    public:
        /** 一行数据：前 3 项为 X/Y/Z 坐标（字符串），其余为分量值（允许空串） */
        using Row = QStringList;

        /**
         * @param title  入口/弹窗标题，如 "点云数据"
         * @param parent 父对象
         */
        explicit mPointCloudDataControl(const QString& title, QWidget* parent = nullptr);
        ~mPointCloudDataControl() override;

        /** 覆盖式设置全部行（不清空弹窗状态，外部数据变化时调用） */
        void setRows(const QVector<Row>& rows);
        /** 当前全部行 */
        QVector<Row> rows() const;
        int rowCount() const { return _rows.size(); }

        /*
        * 运行期设置可变分量列数（宿主"分量数量"联动）：行数据按 3+count 规整
        * （X/Y/Z 固定，分量截断/补空），纯数据/视图规整，不发射 dataChanged。
        */
        void setComponentCount(int count);

        /** 渲染按钮可选：默认隐藏；显示后点击触发回调（由宿主接入真实渲染） */
        void setRenderButtonVisible(bool visible, const QString& text = QString());

        /** 值列单元格校验钩子（列 0/1/2 为 X/Y/Z）：返回空串合法，
            非空串为错误提示——拒绝写入并以内联消息显示，表格回显旧值 */
        void setValueCellValidator(const std::function<QString(int column, const QString& text)>& validator);

        /** 导入参数（界面当前值，随「导入」执行回调传给宿主）；类型统一为基类 ImportParams */
        using ImportParams = mAttachedPopupBase::ImportParams;
        /** 导入/导出预留接口：设置后点击面板中的执行按钮回调，由宿主实现真实读写 */
        void setImportHandler(const std::function<void(const ImportParams&)>& handler);
        void setExportHandler(const std::function<void(const QString& filePath)>& handler);
        /** 渲染按钮回调 */
        void setRenderHandler(const std::function<void()>& handler);
        /** 入口按钮从控件中剖分（所有权交给宿主布局），等价于 detachEntryWidget */
        void detachEntryButton(QWidget* newParent);

    signals:
        /** 行数据发生任何变化（编辑/增删/清空/外部 setRows） */
        void dataChanged();
        /** 单行单元格被编辑：column 为值列序号（0/1/2=X/Y/Z，3+=分量） */
        void cellEdited(int row, int column, const QString& text);
        /*
        * 表头分量增减入口（D7/D8：允许扩展的对象表在表头右端设 −/＋，各占约 19px）。
        * delta = ±1；分量数的唯一权威在宿主（如"分量数量"spin），
        * 宿主更新后经 setComponentCount 回写下发，形成闭环。
        */
        void componentCountChangeRequested(int delta);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        // mAttachedPopupBase 内容钩子
        void buildContent(QWidget* body, QVBoxLayout* layout) override;
        QSize contentSizeHint() const override;
        void onPopupResized() override;
        /** 首开 show 后重建表格（隐藏期 Stretch 列未分配实际宽度，span/行高需按最终布局生效） */
        void onPopupShown() override;

    private:
        void rebuildTable();
        void appendRow(bool focus = true);
        /** 删除指定行（供删除列委托与清空复用） */
        void removeRowAt(int row);
        void updateEntrySummary();
        void updateActionStates();
        /** 末行操作格与表格列对齐：＋合并坐标与数值列（拉伸）、清空格对齐删除列 */
        void syncActionCellWidths();
        /** 表头 −/＋ 分量增减按钮随删除列（末列）位置重新安放（D8） */
        void updateHeaderButtons();

        int _componentCount = 1;
        std::function<QString(int, const QString&)> _valueCellValidator;

        // 内容子树（弹窗的子对象，随弹窗销毁；QPointer 托管判空可靠）
        QPointer<QTableView> _table;
        QPointer<mPointCloudRowTableModel> _tableModel;
        QPointer<mPointCloudDeleteDelegate> _deleteDelegate;
        QPointer<QPushButton> _addRowBtn;
        QPointer<QPushButton> _clearBtn;
        // 表头分量增减按钮（删除列表头右端的子部件，随表头销毁）
        QPointer<QToolButton> _headerMinusBtn;
        QPointer<QToolButton> _headerPlusBtn;

        QVector<Row> _rows;
        bool _syncingTable = false;
    };
}
