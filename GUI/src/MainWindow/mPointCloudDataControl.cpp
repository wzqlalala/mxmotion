#include "mPointCloudDataControl.h"

#include <QEvent>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>

#include <QApplication>

namespace MechMainWindow
{
    namespace
    {
        // 2.2.6 吸附式弹窗（标准档）D2 点云数据关键尺寸（与 D8 宽度规则一致）
        constexpr int kRowHeight = 30;        // 可编辑数据行高（D8：标准 30px）
        constexpr int kHeaderHeight = 26;     // 表头/单行控件高（2.2.6：26px）
        constexpr int kActionsHeight = 26;    // 表格末行操作行高（数据区：末行标准高 26px）
        constexpr int kColWidth = 100;        // 数据列最小宽（D8：每列 100px，10 位有效数字）
        constexpr int kOpColWidth = 38;       // 行操作列固定宽（D8）
        constexpr int kPopupMargin = 12;      // 数据区左右边距（D8：示例左右各 12px）
        constexpr int kMaxTableHeight = 360;  // 表格区域超过该高度后内部滚动
        constexpr int kEmptyRowHeight = 120;  // 空态占位行高：提示文案垂直居中，表格区保留内容感

        // D2 表格内容样式：叠加在基类弹窗公共样式之上。
        // 末行操作格：「＋」为首格（合并坐标与数值列，无分界），「清空」以左边框分隔并对齐删除列
        const QString kContentStyle = QStringLiteral(R"(
            /* 2.2.6 吸附式弹窗：末行操作行采用 panel 底色 #f5f7fa，与表格连续共边、无圆角无外框 */
            QWidget#tableBox { border: none; background: #ffffff; }
            /* 表格主体：普通数据行白底，数值列 12px 等宽字体（D8：不截断输入精度） */
            QTableView {
                background: #ffffff; border: 1px solid #e4e8ee;
                gridline-color: #e4e8ee;
                font-size: 12px; color: #424b57;
                selection-background-color: #e8f3ff; selection-color: #424b57;
            }
            QTableView::item { padding: 0 6px; }
            QHeaderView::section {
                background: #e9edf3; color: #687383; font-size: 11px; font-weight: normal;
                border: none; border-right: 1px solid #e4e8ee; border-bottom: 1px solid #e4e8ee;
                padding: 0 6px;
            }
            QLineEdit#cellEdit {
                border: 1px solid transparent; background: transparent;
                selection-background-color: #e8f3ff; padding: 0 6px;
                font-family: Consolas, monospace;
            }
            QLineEdit#cellEdit:focus { border: 1px solid #1890ff; background: #ffffff; }
            /* 末行操作格：panel 底色、共边无圆角；「＋」首格无分界，
               「清空」以左边框分隔并对齐删除列（D2：末列清空） */
            QWidget#actionsBar {
                background: #f5f7fa;
                border: 1px solid #e4e8ee;
                border-top: none;
            }
            QPushButton#addAction {
                background: transparent; border: none;
                color: #1890ff; font-size: 12px;
            }
            QPushButton#addAction:hover { background: #e8f3ff; }
            QPushButton#clearAction {
                background: transparent; border: none; border-left: 1px solid #e4e8ee;
                color: #d95151; font-size: 12px;
            }
            QPushButton#clearAction:hover { background: #fff5f5; }
            QPushButton#clearAction:disabled { color: #a6afbb; }
            /* 表头分量增减入口（D7/D8）：删除列表头右端 −/＋ 各占约 19px */
            QToolButton#headerMinusBtn, QToolButton#headerPlusBtn {
                background: transparent; border: none;
                color: #1890ff; font-size: 12px;
            }
            QToolButton#headerMinusBtn:hover, QToolButton#headerPlusBtn:hover { background: #e8f3ff; }
            QToolButton#headerMinusBtn:disabled { color: #a6afbb; }
        )");

        QPushButton* makeContentButton(const QString& text, const QString& tip, const QString& objName)
        {
            QPushButton* btn = new QPushButton(text);
            btn->setObjectName(objName);
            btn->setToolTip(tip);
            btn->setCursor(Qt::PointingHandCursor);
            return btn;
        }
    }

    // ------------------------------------------------------------------
    // 行表模型实现：数据常驻控件 _rows，模型只暴露访问接口（不复制）
    // ------------------------------------------------------------------
    QVariant mPointCloudRowTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        if (section < fixedColumnCount())
        {
            switch (section)
            {
            case 0: return QStringLiteral("X");
            case 1: return QStringLiteral("Y");
            case 2: return QStringLiteral("Z");
            }
            return QVariant();
        }
        // 可变分量列从「分量 1」起编号（D7：X/Y/Z 固定，分量只增减数值列）
        const int v = section - fixedColumnCount();
        if (_componentCount && v < *_componentCount)
            return QStringLiteral("分量 %1").arg(v + 1);
        return QVariant(); // 操作列无表头
    }

    QVariant mPointCloudRowTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || !_rows) return QVariant();
        const int r = index.row();
        const int c = index.column();

        // 空态占位行
        if (isEmptyPlaceholderRow(r))
        {
            if (role == Qt::DisplayRole && c == 0)
                return QStringLiteral("尚未添加%1").arg(_title ? *_title : QString());
            if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);
            if (role == Qt::ForegroundRole) return QColor(QStringLiteral("#788391"));
            return QVariant();
        }
        if (r >= _rows->size()) return QVariant();

        if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);

        // 删除列由自绘委托（mPointCloudDeleteDelegate）负责：
        // 空态行/无行一律给空数据，即便列绑定残留也不会画出垃圾桶图标
        if (isOpColumn(c))
        {
            if (role == Qt::UserRole)
                return _hoverRow == r;
            return QVariant();
        }

        if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
        return (c >= 0 && c < _rows->at(r).size()) ? _rows->at(r).at(c) : QString();
    }

    bool mPointCloudRowTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || !index.isValid() || !_rows) return false;
        if (isEmptyPlaceholderRow(index.row()) || index.row() >= _rows->size()) return false;
        if (isOpColumn(index.column())) return false;
        // 数据写回由控件层（cellEditedByUser 监听者）统一处理，避免模型直接改 _rows
        emit cellEditedByUser(index.row(), index.column(), value.toString().trimmed());
        return true;
    }

    Qt::ItemFlags mPointCloudRowTableModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid()) return Qt::NoItemFlags;
        if (isEmptyPlaceholderRow(index.row())) return Qt::NoItemFlags;
        // 删除列：可选中（供委托接收点击），不可编辑
        if (isOpColumn(index.column())) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        // 坐标列与分量列均可编辑（点云无标识列，D2）
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    void mPointCloudRowTableModel::setHoverRow(int row)
    {
        if (_hoverRow == row) return;
        const int oldRow = _hoverRow;
        _hoverRow = row;
        const int opCol = columnCount() - 1;
        if (oldRow >= 0) emit dataChanged(index(oldRow, opCol), index(oldRow, opCol));
        if (row >= 0) emit dataChanged(index(row, opCol), index(row, opCol));
    }

    // ------------------------------------------------------------------
    // 删除列自绘委托：绘制垃圾桶图标 + 处理点击（替代 setIndexWidget）
    // ------------------------------------------------------------------
    void mPointCloudDeleteDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        auto* rowModel = qobject_cast<const mPointCloudRowTableModel*>(index.model());
        // 仅操作列单元格绘制删除图标：空态占位行/无行时纯背景
        if (!rowModel || !rowModel->isOpColumn(index.column()) || rowModel->isEmptyPlaceholderRow(index.row()))
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyledItemDelegate::paint(painter, option, index);

        const bool hovered = index.data(Qt::UserRole).toBool();
        // 悬停时淡红底，与离散控件删除列一致
        if (hovered)
            painter->fillRect(option.rect, QColor(QStringLiteral("#fff5f5")));

        const QString glyph = QStringLiteral("🗑");
        painter->setPen(hovered ? QColor(QStringLiteral("#d95151")) : QColor(QStringLiteral("#788391")));
        QFont f = painter->font();
        f.setPixelSize(13);
        painter->setFont(f);
        painter->drawText(option.rect, Qt::AlignCenter, glyph);
    }

    bool mPointCloudDeleteDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
        const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        Q_UNUSED(option)
        if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            auto* rowModel = qobject_cast<const mPointCloudRowTableModel*>(index.model());
            if (mouseEvent->button() == Qt::LeftButton && index.isValid()
                && rowModel && rowModel->isOpColumn(index.column())
                && !rowModel->isEmptyPlaceholderRow(index.row()))
            {
                emit deleteRowRequested(index.row());
                return true;
            }
        }
        Q_UNUSED(model)
        return false;
    }

    // ------------------------------------------------------------------
    // 控件
    // ------------------------------------------------------------------
    mPointCloudDataControl::mPointCloudDataControl(const QString& title, QWidget* parent)
        : mAttachedPopupBase(title, parent)
    {
        // 渲染云图为该形态的可选能力（D2：表下提供「渲染云图」，不提供「绘制曲线」），
        // 默认隐藏由宿主开启
        setActionButtonVisible(false, QStringLiteral("渲染云图"), false);
        setActionHandler([this]() {
            showInlineMessage(QStringLiteral("渲染功能接口已预留，待宿主接入。"), false);
            });
        // 入口摘要初始态（标题由宿主行布局的 label 承担，按钮只显示摘要）
        updateEntrySummary();
    }

    mPointCloudDataControl::~mPointCloudDataControl()
    {
        // 弹窗子树随基类析构 delete _popup 级联销毁；表格视口上的过滤器须先摘除，
        // 否则级联销毁期间视口 Leave 事件会回调进正在析构的 eventFilter
        if (_popup)
        {
            if (_table && _table->viewport())
                _table->viewport()->removeEventFilter(this);
        }
    }

    // ------------------------------------------------------------------
    // 对外接口适配
    // ------------------------------------------------------------------
    void mPointCloudDataControl::detachEntryButton(QWidget* newParent)
    {
        detachEntryWidget(newParent);
    }

    void mPointCloudDataControl::setRenderButtonVisible(bool visible, const QString& text)
    {
        // 渲染云图为执行型按钮（非拓展区开关），checkable 恒 false
        setActionButtonVisible(visible, text, false);
    }

    void mPointCloudDataControl::setRenderHandler(const std::function<void()>& handler)
    {
        setActionHandler(handler);
    }

    void mPointCloudDataControl::setValueCellValidator(const std::function<QString(int, const QString& text)>& validator)
    {
        _valueCellValidator = validator;
    }

    void mPointCloudDataControl::setImportHandler(const std::function<void(const ImportParams&)>& handler)
    {
        mAttachedPopupBase::setImportHandler(handler);
    }

    void mPointCloudDataControl::setExportHandler(const std::function<void(const QString&)>& handler)
    {
        mAttachedPopupBase::setExportHandler(handler);
    }

    void mPointCloudDataControl::setComponentCount(int count)
    {
        count = qMax(1, count);
        if (count == _componentCount)
            return;
        _componentCount = count;
        const int total = mPointCloudRowTableModel::fixedColumnCount() + count;
        for (Row& row : _rows)
        {
            while (row.size() > total) row.removeLast();
            while (row.size() < total) row.append(QString());
        }
        rebuildTable();
    }

    // ------------------------------------------------------------------
    // 内容构建（mAttachedPopupBase 钩子：弹窗首次创建时调用，派生类已构造完成）
    // ------------------------------------------------------------------
    void mPointCloudDataControl::buildContent(QWidget* body, QVBoxLayout* layout)
    {
        // 表格内容样式叠加到弹窗公共样式之后（后者在前，后者同名规则优先）
        body->setStyleSheet(body->styleSheet() + kContentStyle);

        // ---- 表格区（2.2.6：数据区左右边距 12px，表格与末行操作连续共边）----
        QWidget* tableWrap = new QWidget(body);
        QVBoxLayout* wrapLayout = new QVBoxLayout(tableWrap);
        wrapLayout->setContentsMargins(kPopupMargin, 10, kPopupMargin, 10);
        wrapLayout->setSpacing(0);

        QWidget* tableBox = new QWidget(tableWrap);
        tableBox->setObjectName(QStringLiteral("tableBox"));
        QVBoxLayout* boxLayout = new QVBoxLayout(tableBox);
        boxLayout->setContentsMargins(0, 0, 0, 0);
        boxLayout->setSpacing(0);

        _table = new QTableView(tableBox);
        _tableModel = new mPointCloudRowTableModel(_table);
        _tableModel->bind(&_rows, &_componentCount, &_title);
        _table->setModel(_tableModel);
        _table->verticalHeader()->setVisible(false);
        _table->verticalHeader()->setDefaultSectionSize(kRowHeight);
        // 大数据量（万行级）下模型驱动 + 固定行高 + 关排序，避免逐行 item 创建与测量
        _table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        _table->horizontalHeader()->setFixedHeight(kHeaderHeight);
        _table->horizontalHeader()->setStretchLastSection(false);
        _table->horizontalHeader()->setHighlightSections(false);
        _table->setShowGrid(true);
        _table->setSelectionMode(QAbstractItemView::NoSelection);
        _table->setFocusPolicy(Qt::StrongFocus);
        _table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        _table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        // 数值列 12px 等宽字体（D8：容纳 10 位有效数字，不截断输入精度）
        {
            QFont tableFont = _table->font();
            tableFont.setFamily(QStringLiteral("Consolas"));
            _table->setFont(tableFont);
        }
        // 末行操作格与表格列对齐：表格横向滚动/列宽变化时同步清空格宽度
        connect(_table->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &mPointCloudDataControl::syncActionCellWidths);
        // 表头 −/＋ 分量增减按钮随横向滚动/列宽变化重新安放
        connect(_table->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &mPointCloudDataControl::updateHeaderButtons);
        connect(_table->horizontalHeader(), &QHeaderView::sectionResized,
            this, [this](int, int, int) { updateHeaderButtons(); });

        // 删除列：自绘委托替代 indexWidget（无 hide/reset 弃用问题），
        // 视口鼠标移动驱动悬停高亮；委托与操作列的绑定在 rebuildTable 中
        // 按当前列结构设置（初始模型列为空，不能在此绑定）
        _deleteDelegate = new mPointCloudDeleteDelegate(_table);
        _table->setMouseTracking(true);
        connect(_deleteDelegate, &mPointCloudDeleteDelegate::deleteRowRequested,
            this, &mPointCloudDataControl::removeRowAt);
        connect(_table, &QTableView::entered, this, [this](const QModelIndex& idx) {
            if (idx.isValid() && _tableModel->isOpColumn(idx.column()))
                _tableModel->setHoverRow(idx.row());
            else
                _tableModel->setHoverRow(-1);
            });
        _table->viewport()->installEventFilter(this);
        boxLayout->addWidget(_table);

        // 表头分量增减入口（D7/D8）：−/＋ 按钮安放在删除列表头右端（表头子部件），
        // 点击发 componentCountChangeRequested，由宿主统一驱动分量数量后经
        // setComponentCount 回写下发；列数变化/滚动时由 updateHeaderButtons 重新安放
        _headerMinusBtn = new QToolButton(_table->horizontalHeader());
        _headerMinusBtn->setObjectName(QStringLiteral("headerMinusBtn"));
        _headerMinusBtn->setText(QStringLiteral("−"));
        _headerMinusBtn->setFixedSize(18, 18);
        _headerMinusBtn->setToolTip(QStringLiteral("减少分量列"));
        _headerMinusBtn->setCursor(Qt::PointingHandCursor);
        _headerPlusBtn = new QToolButton(_table->horizontalHeader());
        _headerPlusBtn->setObjectName(QStringLiteral("headerPlusBtn"));
        _headerPlusBtn->setText(QStringLiteral("＋"));
        _headerPlusBtn->setFixedSize(18, 18);
        _headerPlusBtn->setToolTip(QStringLiteral("增加分量列"));
        _headerPlusBtn->setCursor(Qt::PointingHandCursor);
        connect(_headerMinusBtn, &QToolButton::clicked, this, [this]() {
            emit componentCountChangeRequested(-1);
            });
        connect(_headerPlusBtn, &QToolButton::clicked, this, [this]() {
            emit componentCountChangeRequested(1);
            });

        // 底部操作行（表格末行单元格，D2）：「＋」合并全部坐标与数值列（首格，拉伸）、
        // 「清空」对齐删除列；与表格连续共边、无按钮间距、无圆角（样式表 #actionsBar
        // 承担边框与 panel 底色）；点云为纯参数表，不设拾取格
        QWidget* actionsBar = new QWidget(tableBox);
        actionsBar->setObjectName(QStringLiteral("actionsBar"));
        actionsBar->setFixedHeight(kActionsHeight);
        QHBoxLayout* actionsLayout = new QHBoxLayout(actionsBar);
        actionsLayout->setContentsMargins(0, 0, 0, 0);
        actionsLayout->setSpacing(0);
        _addRowBtn = makeContentButton(QStringLiteral("＋"), QStringLiteral("添加数据行"), QStringLiteral("addAction"));
        _clearBtn = makeContentButton(QStringLiteral("清空"), QStringLiteral("清空全部数据"), QStringLiteral("clearAction"));
        _addRowBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _clearBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        _clearBtn->setFixedWidth(kOpColWidth);
        actionsLayout->addWidget(_addRowBtn, 1);
        actionsLayout->addWidget(_clearBtn);
        boxLayout->addWidget(actionsBar);

        wrapLayout->addWidget(tableBox);
        layout->addWidget(tableWrap, 1);

        connect(_addRowBtn, &QPushButton::clicked, this, [this]() { appendRow(true); });
        connect(_clearBtn, &QPushButton::clicked, this, [this]() {
            if (_rows.isEmpty()) return;
            _rows.clear();
            rebuildTable();
            updateEntrySummary();
            updateActionStates();
            emit dataChanged();
            });

        // 单元格编辑：模型 setData 时同步行数据并发出信号（业务逻辑在控件层处理）
        connect(_tableModel, &mPointCloudRowTableModel::cellEditedByUser, this,
            [this](int row, int col, const QString& text) {
                if (_syncingTable) return;
                if (row < 0 || row >= _rows.size()) return;
                if (_tableModel->isOpColumn(col)) return;
                QStringList& values = _rows[row];
                if (col < 0 || col >= values.size() || values.at(col) == text) return;
                // 值列校验钩子（点云 X/Y/Z 数值等）：非法输入不进 _rows，
                // 表格回显旧值并以底部内联消息提示
                if (_valueCellValidator)
                {
                    const QString validateError = _valueCellValidator(col, text);
                    if (!validateError.isEmpty())
                    {
                        rebuildTable();
                        showInlineMessage(validateError, true);
                        return;
                    }
                }
                values[col] = text;
                emit cellEdited(row, col, text);
                emit dataChanged();
            });

        // 隐藏期把行数据/空态一次性灌入模型，首开不再二次 reset
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
    }

    // ------------------------------------------------------------------
    // 内容尺寸（D8 宽度规则：W = max(480, 数据列数×100 + 38 操作列 + 框架 F)，
    // 最小宽度下限与视口约束由基类统一处理；点云无拾取态，宽度恒走普通态）
    // ------------------------------------------------------------------
    QSize mPointCloudDataControl::contentSizeHint() const
    {
        const int dataCols = mPointCloudRowTableModel::fixedColumnCount() + _componentCount;

        int tableWidth = dataCols * kColWidth + kOpColWidth;
        tableWidth += 2; // 表格自身 1px 边框

        const bool empty = _rows.isEmpty();
        const int rows = qMax(_rows.size(), 1); // 空态保留一行提示
        // 空态占位行加高（提示文案垂直居中，避免单行被压缩成贴表头的一条）
        const int rowsHeight = empty ? kEmptyRowHeight : rows * kRowHeight;
        int tableHeight = kHeaderHeight + rowsHeight + 2;
        const bool overflow = tableHeight > kMaxTableHeight;
        tableHeight = qMin(tableHeight, kMaxTableHeight);
        // 只为实际纵向滚动条补偿（D8：无溢出不预留空白槽）
        if (overflow)
            tableWidth += qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

        // 数据区左右边距 + 上下边距 + 末行操作行
        const int contentWidth = tableWidth + 2 * kPopupMargin;
        return QSize(contentWidth, 10 + tableHeight + kActionsHeight + 10);
    }

    void mPointCloudDataControl::onPopupResized()
    {
        mAttachedPopupBase::onPopupResized();
        // 弹窗 Resize 时同步末行操作格列宽（Stretch 列宽随动后清空格需重新对齐）
        syncActionCellWidths();
    }

    // ------------------------------------------------------------------
    // 事件过滤：表格视口悬停高亮收尾（其余公共路径由基类承担）
    // ------------------------------------------------------------------
    bool mPointCloudDataControl::eventFilter(QObject* watched, QEvent* event)
    {
        if (isShuttingDown())
            return QWidget::eventFilter(watched, event);

        // 视口鼠标离开：清除删除列悬停高亮
        if (watched == (_table ? _table->viewport() : nullptr) && event->type() == QEvent::Leave)
        {
            if (_tableModel) _tableModel->setHoverRow(-1);
            return false;
        }

        return mAttachedPopupBase::eventFilter(watched, event);
    }

    // ------------------------------------------------------------------
    // 表格
    // ------------------------------------------------------------------
    void mPointCloudDataControl::rebuildTable()
    {
        const int totalCols = qMax(1, _tableModel
            ? _tableModel->columnCount() : mPointCloudRowTableModel::fixedColumnCount() + _componentCount + 1);

        // 弹窗未建（构造期/setRows 在首次打开前调用）只保留数据，打开时一次性刷新
        if (!_popup || !_tableModel || !_table) return;

        // 弹窗隐藏期间不做视图刷新（万行级下即使模型驱动，通知/布局也有开销），
        // 只把行数据留在权威来源 _rows，下次打开时一次性 notifyReset + rebuild；
        // 删除列委托绑定只取决于列结构，与可见性无关，隐藏期也必须完成
        // （绑定只在可见分支执行时，「导入后直接打开弹窗」场景委托从未绑定：
        // 行末删除按钮不绘制、点击无响应）。
        if (!_popup->isVisible())
        {
            if (_deleteDelegate)
            {
                _table->setItemDelegateForColumn(totalCols - 1, _deleteDelegate);
                _tableModel->setHoverRow(-1);
            }
            // 空态占位行合并为一整行，使首次打开（弹窗尚未可见）也居中显示提示；
            // setSpan/clearSpans 不依赖视图可见性，在此同步处理保证两态一致
            if (_rows.isEmpty())
                _table->setSpan(0, 0, 1, totalCols);
            else
                _table->clearSpans();
            updateHeaderButtons();
            return;
        }

        _syncingTable = true;

        // 模型结构可能变化（分量列增减）：整体重置让视图重取行/列数与表头。
        // 删除列委托必须在本次 reset 之后绑定：QHeaderView 对"绑定到最后一列的列委托"
        // 会在模型 reset/结构变化时静默摘除（委托只按旧列索引保存，列数变化后索引失效
        // 即弃用），表现为导入数据后直接打开弹窗删除列不绘制、点击无响应。
        _tableModel->notifyReset();

        // 列宽/拉伸模式（reset 后视图保留列宽设置，这里按模式重设一次）
        for (int c = 0; c < totalCols; ++c)
        {
            if (c == totalCols - 1)
            {
                // 删除列固定宽度（D8：操作列固定 38px），不随弹窗拉伸
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
                _table->setColumnWidth(c, kOpColWidth);
            }
            else
            {
                // 坐标/分量列 Stretch：多余宽度均分（D8），弹窗拉大时列宽随动
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
            }
        }

        if (_deleteDelegate)
        {
            _table->setItemDelegateForColumn(totalCols - 1, _deleteDelegate);
            _tableModel->setHoverRow(-1);
        }

        // 空态占位行：合并所有列为一整行，提示文案在整个表格区水平+垂直居中。
        // 必须在 notifyReset 之后调用 setSpan——beginResetModel 会清空视图的 span 表
        if (_rows.isEmpty())
            _table->setSpan(0, 0, 1, totalCols);
        else
            _table->clearSpans();

        _syncingTable = false;
        // 空态占位行加高（提示文案垂直居中）；有数据时恢复统一数据行高
        _table->verticalHeader()->setDefaultSectionSize(_rows.isEmpty() ? kEmptyRowHeight : kRowHeight);
        // 列结构变化后重新对齐末行操作格与表头 −/＋ 按钮
        syncActionCellWidths();
        updateHeaderButtons();
        repositionPopup();
        // notifyReset 后布局（LayoutRequest）在下一次事件循环才把 Stretch 列的实际
        // 宽度分配给表头，此刻 sectionSize 仍是未定值，操作格固定宽度随之失真
        //（首开时按钮文字被压没，直到下一次 resize 才恢复）。延迟到布局完成后
        // 按最终列宽再对齐一次。
        QTimer::singleShot(0, this, [this]() {
            if (_popup && _popup->isVisible())
            {
                syncActionCellWidths();
                updateHeaderButtons();
            }
            });
    }

    void mPointCloudDataControl::onPopupShown()
    {
        // 首开 show 之后视图才真正完成列布局（Stretch 列在控件可见时才分配实际宽度）。
        // show 完成后强制再走一次 rebuildTable（此时弹窗可见，走可见分支），
        // 让 span/行高/列宽基于最终布局生效，保证首次打开即整块居中。
        rebuildTable();
        updateActionStates();
    }

    void mPointCloudDataControl::syncActionCellWidths()
    {
        if (!_table || !_clearBtn) return;
        QHeaderView* header = _table->horizontalHeader();
        // 末行操作格与表格列对齐（D2 数据区）：＋合并全部坐标与数值列（拉伸），
        // 清空格对齐删除列；表格横向溢出时按可视区换算：sectionSize。
        int clearWidth = kOpColWidth;
        const int opCol = header->count() - 1;
        if (opCol >= 0)
            clearWidth = header->sectionSize(opCol);
        _clearBtn->setFixedWidth(qMax(clearWidth, 0));
    }

    void mPointCloudDataControl::removeRowAt(int row)
    {
        if (row < 0 || row >= _rows.size()) return;
        // 行删除按 begin → 改 _rows → end 三段式：endRemoveRows 内视图会立即
        // 回查 rowCount()/data()，数据必须先于 end 同步，否则越界崩溃
        const bool notify = _popup && _popup->isVisible() && _tableModel;
        if (notify) _tableModel->beginRemoveRow(row);
        _rows.removeAt(row);
        if (notify) _tableModel->endRemoveRow();
        // 删到空表时占位行（「尚未添加点云数据」）需要一次整体重置才能出现：
        // begin/endRemoveRows 只表达行数 1→0，占位行 0→1 视图无从得知
        if (notify && _rows.isEmpty())
        {
            _tableModel->notifyReset();
            // notifyReset 清空视图 span 表，空态合并行需在其后重建
            const int totalCols = _tableModel->columnCount();
            _table->setSpan(0, 0, 1, totalCols);
            _table->verticalHeader()->setDefaultSectionSize(kEmptyRowHeight);
        }
        // 行数变化后按内容重新自适应弹窗高度（与 appendRow 对称：增量路径不走 rebuildTable）
        repositionPopup();
        updateEntrySummary();
        updateActionStates();
        emit dataChanged();
    }

    void mPointCloudDataControl::appendRow(bool focus)
    {
        Row row;
        const int total = mPointCloudRowTableModel::fixedColumnCount() + _componentCount;
        for (int i = 0; i < total; ++i) row.append(QString());
        // 行插入按 begin → 改 _rows → end 三段式：endInsertRows 内视图会立即
        // 回查 rowCount()/data()，数据必须先于 end 同步，否则越界崩溃
        const bool notify = _popup && _popup->isVisible() && _tableModel;
        // 空态 → 首行：rowCount 语义 1(占位)→1(数据)，增量 insert(1,1) 会得到占位+新行
        // 两行且占位残留为不可编辑的空行；必须走 rebuildTable 整体重置（同时重设
        // 删除列宽/委托绑定，否则空态弹窗打开后首个数据行的删除列不绘制）
        const bool replacingPlaceholder = notify && _rows.isEmpty();
        if (notify && !replacingPlaceholder) _tableModel->beginInsertRow(_rows.size());
        _rows.append(row);
        if (notify && !replacingPlaceholder) _tableModel->endInsertRow();
        if (replacingPlaceholder)
            rebuildTable();
        // 行数变化后按内容重新自适应弹窗高度：增量插入不走 rebuildTable，
        // 否则弹窗保持打开时按旧行数算出的高度（空态仅 1 行高），新行被挤压
        repositionPopup();
        updateEntrySummary();
        updateActionStates();
        emit dataChanged();
        if (focus)
        {
            // 新行从首列（X 坐标）开始录入
            const QModelIndex idx = _tableModel->index(_rows.size() - 1, 0);
            if (idx.isValid())
            {
                _table->scrollTo(idx);
                _table->setCurrentIndex(idx);
                _table->edit(idx);
            }
        }
    }

    void mPointCloudDataControl::updateEntrySummary()
    {
        if (QLabel* summary = entrySummaryLabel())
        {
            summary->setText(_rows.isEmpty()
                ? QStringLiteral("点击编辑数据表")
                : QStringLiteral("已定义 %1 行数据").arg(_rows.size()));
        }
        setMetaText(QStringLiteral("%1 行").arg(_rows.size()));
    }

    void mPointCloudDataControl::updateActionStates()
    {
        const bool hasRows = !_rows.isEmpty();
        if (_clearBtn) _clearBtn->setEnabled(hasRows);
        if (_exportBtn) _exportBtn->setEnabled(hasRows);
        setActionButtonEnabled(hasRows);
        // 分量只剩 1 列时禁用 −（与旧数据表对话框一致；宿主侧同样有 qMax(1,…) 兜底）
        if (_headerMinusBtn) _headerMinusBtn->setEnabled(_componentCount > 1);
    }

    void mPointCloudDataControl::updateHeaderButtons()
    {
        if (!_headerMinusBtn || !_headerPlusBtn || !_table || !_tableModel)
            return;
        QHeaderView* header = _table->horizontalHeader();
        const int columns = header->count();
        if (columns <= 0)
            return;
        // −/＋ 安放在删除列（末列）表头右端（D8：各占约 19px）；
        // sectionViewportPosition 已含横向滚动偏移，表内横向滚动时随动
        const int sectionPos = header->sectionViewportPosition(columns - 1);
        const int sectionWidth = header->sectionSize(columns - 1);
        const int y = (header->height() - 18) / 2;
        _headerMinusBtn->move(sectionPos + sectionWidth - 40, y);
        _headerPlusBtn->move(sectionPos + sectionWidth - 20, y);
        _headerMinusBtn->setEnabled(_componentCount > 1);
        _headerMinusBtn->raise();
        _headerPlusBtn->raise();
    }

    // ------------------------------------------------------------------
    // 数据接口
    // ------------------------------------------------------------------
    void mPointCloudDataControl::setRows(const QVector<Row>& rows)
    {
        _rows = rows;
        const int total = mPointCloudRowTableModel::fixedColumnCount() + _componentCount;
        for (Row& row : _rows)
        {
            while (row.size() < total) row.append(QString());
            while (row.size() > total) row.removeLast();
        }
        // 弹窗隐藏时 rebuildTable 会跳过模型通知，此处兜底 reset，
        // 保证模型行数与 _rows 一致（否则外部清空后视图仍按旧行数访问，滚动即崩）
        if (_popup && !_popup->isVisible() && _tableModel)
        {
            _tableModel->notifyReset();
        }
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
    }

    QVector<mPointCloudDataControl::Row> mPointCloudDataControl::rows() const
    {
        return _rows;
    }
}
