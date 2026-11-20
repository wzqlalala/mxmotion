#pragma once
#pragma execution_character_set("utf-8")

#include "mAttachedPopupBase.h"

#include <QVector>
#include <QStringList>
#include <QPointF>

class QTableView;
class QHBoxLayout;
class QComboBox;

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

namespace MechMainWindow
{
    /**
     * @brief 吸附式弹窗数据表格的行表模型（QAbstractTableModel）
     *
     * 数据常驻宿主控件的 rows 容器（构造时绑定一次指针，之后容器只允许改内容、
     * 不允许整体替换对象），模型只暴露访问接口不复制数据；
     * 行结构/内容变化由控件调用对应 reset/insert/remove 通知。
     */
    class MMAINWINDOW_EXPORT mAttachedTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit mAttachedTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

        void bind(const QVector<QStringList>* rows, const QStringList* headers, const QString* emptyHint)
        {
            beginResetModel();
            _rows = rows; _headers = headers; _emptyHint = emptyHint;
            endResetModel();
        }

        void notifyReset() { beginResetModel(); endResetModel(); }
        // 行增删必须按 begin → 改数据 → end 三段式调用：
        // end* 内视图会立即回查 rowCount()/data()，数据必须先于 end 同步
        void beginInsertRow(int row) { beginInsertRows(QModelIndex(), row, row); }
        void endInsertRow() { endInsertRows(); }
        void beginRemoveRow(int row) { beginRemoveRows(QModelIndex(), row, row); }
        void endRemoveRow() { endRemoveRows(); }

        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() || !_rows) return 0;
            // 空表保留一行提示（「尚未添加…」）
            return _rows->isEmpty() ? 1 : _rows->size();
        }

        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            if (parent.isValid() || !_rows) return 1;
            return qMax(1, (_headers ? _headers->size() : 0) + 1); // 数据列 + 删除列
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;

    signals:
        /** 用户编辑了某单元格（row/col 为模型索引，text 为 trimmed 文本） */
        void cellEditedByUser(int row, int col, const QString& text);

    public:
        bool isOpColumn(int col) const { return col == columnCount() - 1; }
        bool isEmptyPlaceholderRow(int row) const { return _rows && _rows->isEmpty() && row == 0; }
        /** 删除列悬停行（自绘委托高亮用），-1 表示无悬停 */
        void setHoverRow(int row);

    private:
        const QVector<QStringList>* _rows = nullptr;
        const QStringList* _headers = nullptr;
        const QString* _emptyHint = nullptr;
        int _hoverRow = -1;
    };

    /**
     * @brief 删除列自绘委托
     *
     * 用 QStyledItemDelegate 绘制 + editorEvent 处理点击，替代 setIndexWidget：
     * indexWidget 部件在弹窗 hide/reset 后会被视图静默弃用且不会重建；委托绘制无此问题。
     */
    class MMAINWINDOW_EXPORT mAttachedDeleteDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit mAttachedDeleteDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        void paint(QPainter* painter, const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

        bool editorEvent(QEvent* event, QAbstractItemModel* model,
            const QStyleOptionViewItem& option, const QModelIndex& index) override;

    signals:
        /** 用户点击了 row 行的删除按钮 */
        void deleteRowRequested(int row);
    };

    /**
     * @brief 曲线预览画布：只绘制，不持有数据
     *
     * 数据由宿主控件在重绘前写入（点集 + 轴标题）；空点集时绘制提示文本。
     * 无第三方图表依赖，QPainter 自绘坐标轴/刻度/折线/图例，
     * 样式对齐函数曲线窗口（mCurveWindow/mPlotView）的默认观感。
     * 滚轮以视图中心为锚缩放（对齐 QwtPlotMagnifier），双击复位全貌；
     * 数据更新（setData）时视图自动复位。
     */
    class MMAINWINDOW_EXPORT mAttachedPlotCanvas : public QWidget
    {
    public:
        explicit mAttachedPlotCanvas(QWidget* parent = nullptr) : QWidget(parent) {}

        void setData(const QVector<QPointF>& pts, const QString& xTitle, const QString& yTitle)
        {
            _pts = pts; _xTitle = xTitle; _yTitle = yTitle; resetView(); update();
        }
        void setHint(const QString& hint) { _hint = hint; update(); }

    protected:
        void paintEvent(QPaintEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        /** 视图复位为全部数据的整刻度范围（数据更新/双击复位时调用） */
        void resetView();
        /** 以视图中心为锚缩放显示范围（factor<1 放大、>1 缩小） */
        void zoomView(double factor);

        QVector<QPointF> _pts;
        QString _xTitle, _yTitle, _hint;

        // 当前视图范围（滚轮缩放改写）；基准跨度限制缩放极限，防塌缩/防飘走
        double _viewXmin = 0, _viewXmax = 0, _viewYmin = 0, _viewYmax = 0;
        double _baseXSpan = 1.0, _baseYSpan = 1.0;
        bool _viewValid = false;
    };

    /**
     * @brief 吸附式参数曲线控件（《通用结构软件界面UI设计规则》2.2.6 吸附式弹窗 D1 形态）
     *
     * 通用形态：自变量列（如「温度 / °C」）+ 若干参数列（如「杨氏模量 / MPa」「泊松比」）。
     * 吸附式弹窗的外壳与公共行为（入口、窗口头、吸附定位、点击外部关闭、导入/导出
     * 文件拓展区、消息条、宿主销毁收尾）由 mAttachedPopupBase 承担；本类负责
     * D1 数据表格内容：数据表 + 末行操作行（＋合并数据列 / 清空对齐删除列）+
     * 「绘制曲线」向下拓展预览（D4）。
     *
     * 尺寸/配色遵循 2.2.6 标准档：数据列 100px、操作列 38px、表头/控件 26px、
     * 数据行 30px、弹窗最小宽 480px、数据区左右边距 12px、视口安全边距 12px。
     *
     * 数据以字符串保存（保留输入精度），单元格允许留空（空 = 未输入）；对外通过
     * setCurve/curve 与 QVector<QVector<double>>（列主序，第一列为自变量，
     * 其后每列一个参数）互转，空单元格与非法数值按未输入处理。
     */
    class MMAINWINDOW_EXPORT mParamCurveControl : public mAttachedPopupBase
    {
        Q_OBJECT

    public:
        /**
         * @param title             入口/弹窗标题，如 "温度-各向同性参数曲线"
         * @param independentHeader 自变量列标题（含单位），如 "温度 / °C"
         * @param paramHeaders      参数列标题（含单位），如 {"杨氏模量 / MPa", "泊松比"}
         * @param parent            父对象
         */
        explicit mParamCurveControl(const QString& title,
            const QString& independentHeader,
            const QStringList& paramHeaders,
            QWidget* parent = nullptr);
        ~mParamCurveControl() override;

        /** 覆盖式设置曲线数据：curves[0] 为自变量列，其后每列一个参数；短列缺位留空 */
        void setCurve(const QVector<QVector<double>>& curves);
        /** 当前曲线数据：第一列自变量，其后每列一个参数；
         *  仅收集「自变量有效」的行，参数列空/非法单元格以 NaN 占位（宿主按各自规则过滤） */
        QVector<QVector<double>> curve() const;
        /** 清空全部行 */
        void clearCurve();

        /** 单位切换时刷新列标题（含单位部分随单位制更新） */
        void setColumnHeaders(const QString& independentHeader, const QStringList& paramHeaders);

        /** 覆盖式设置全部行（字符串级，保留输入精度） */
        void setRows(const QVector<QStringList>& rows);
        /** 当前全部行 */
        QVector<QStringList> rows() const { return _rows; }
        int rowCount() const { return _rows.size(); }

        /** 导入参数（界面当前值，随「导入」执行回调传给宿主）；类型统一为基类 ImportParams */
        using ImportParams = mAttachedPopupBase::ImportParams;
        /** 导入/导出回调：设置后点击面板中的执行按钮回调，由宿主实现真实读写 */
        void setImportHandler(const std::function<void(const ImportParams&)>& handler);
        void setExportHandler(const std::function<void(const QString& filePath)>& handler);

        /** 入口按钮从控件中剖分（所有权交给宿主布局），等价于 detachEntryWidget */
        void detachEntryButton(QWidget* newParent);

    signals:
        /** 行数据发生任何变化（编辑/增删/清空/外部 setCurve/setRows） */
        void dataChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        /** 单元格编辑写回后的业务钩子（默认只发 dataChanged） */
        virtual void onCellEdited(int row, int col, const QString& text);

        // mAttachedPopupBase 内容钩子
        void buildContent(QWidget* body, QVBoxLayout* layout) override;
        QSize contentSizeHint() const override;
        void onFilePanelToggled(bool importPanel, bool open) override;
        void onPopupResized() override;

        // 子类可复用的操作（弹窗未建时安全跳过视图刷新）
        void rebuildTable();
        void appendRow(bool focus = true);
        void removeRowAt(int row);
        void clearRows();
        void updateEntrySummary();
        void updateActionStates();

        QStringList _headers;
        QString _emptyHint;
        QVector<QStringList> _rows;

        // 内容子树（弹窗的子对象，随弹窗销毁；QPointer 托管判空可靠）
        QPointer<QTableView> _table;
        QPointer<mAttachedTableModel> _tableModel;
        QPointer<mAttachedDeleteDelegate> _deleteDelegate;
        QPointer<QPushButton> _addRowBtn;
        QPointer<QPushButton> _clearBtn;

        // 曲线预览区（绘制曲线按钮展开，数据变化时联动刷新）
        QPointer<QWidget> _plotPreview;
        QPointer<QComboBox> _plotVarCombo;
        QPointer<QWidget> _plotChart;

        /** 预览变量下拉显示名（默认取表头） */
        virtual QString plotColumnName(int col) const;
        /** 绘制前校验，返回错误文本（空串=通过）；默认检查 ≥2 有效点 */
        virtual QString plotValidate(int xCol, int yCol) const;
        /** 提取一条曲线的有效点（默认逐格 toDouble、跳过非法格） */
        virtual QVector<QPointF> plotSeriesData(int xCol, int yCol) const;
        /** 数据变化后若预览展开则联动重绘 */
        void refreshPlotPreview();
        /** 预览当前选中参数列 */
        int plotSelectedColumn() const;
        void togglePlotPreview();
        /** 预览轴选择行构建/惰性重建（首开/表头变更时刷新） */
        QHBoxLayout* buildPlotAxisRow();
        void rebuildPlotAxisRow();
        /** 表头变更后预览轴选择行失效，下次展开按新列数重建 */
        void invalidatePlotAxisRow() { _axisRowBuilt = false; }

    private:
        /** 末行操作格与表格列对齐：＋合并数据列（拉伸）、清空格对齐删除列 */
        void syncActionCellWidths();

        /** 预览轴选择行是否已重建过 */
        bool _axisRowBuilt = false;
    };
}
