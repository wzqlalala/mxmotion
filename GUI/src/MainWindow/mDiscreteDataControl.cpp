#include "mDiscreteDataControl.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "PickCommonFunction.h"
#include "mMeshData.h"
#include "mxr/data/SelectDataStruct.h"

using namespace MechPreData;
using namespace MechCommon;

namespace MechMainWindow
{
    namespace
    {
        // 进程级拾取 owner：任意时刻只允许一个控件处于拾取会话。
        // 11 个面板各自持有控件实例，全部连同一个视口选择容器并共享高亮层，
        // 容器不支持并发会话；新会话开始时先踢掉旧实例，避免跨面板数据串扰。
        // QPointer 托管：实例析构后自动置空，无需显式清理。
        QPointer<mDiscreteDataControl> g_pickingOwner;
    }

    // ------------------------------------------------------------------
    // 行表模型实现：数据常驻控件 _rows，模型只暴露访问接口（不复制）
    // ------------------------------------------------------------------
    QVariant mDiscreteRowTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        if (!(*_picking))
        {
            const int idCols = idColumnCount();
            if (section < idCols)
                return _idHeaders && section < _idHeaders->size() ? _idHeaders->at(section) : QVariant();
            const int v = section - idCols;
            if (_componentCount && v < *_componentCount)
                return (_valueHeaders && v < _valueHeaders->size())
                    ? _valueHeaders->at(v) : QStringLiteral("分量 %1").arg(v + 1);
            return QVariant(); // 操作列无表头
        }
        // 拾取窄列表：单列显示「标识 / 标识」
        return _idHeaders ? _idHeaders->join(QStringLiteral(" / ")) : QVariant();
    }

    QVariant mDiscreteRowTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || !_rows) return QVariant();
        const int r = index.row();
        const int c = index.column();

        // 空态占位行
        if (isEmptyPlaceholderRow(r))
        {
            if (role == Qt::DisplayRole && c == 0)
                return (*_picking) ? QStringLiteral("等待拾取对象")
                                   : QStringLiteral("尚未添加%1").arg(_title ? *_title : QString());
            if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);
            if (role == Qt::ForegroundRole) return QColor(QStringLiteral("#788391"));
            return QVariant();
        }
        if (r >= _rows->size()) return QVariant();

        if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);

        const int idCols = idColumnCount();
        if (c < idCols)
        {
            if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
            const auto& key = _rows->at(r).first;
            const int idVal = (c == 0 ? key.first : key.second);
            return idVal > 0 ? QString::number(idVal) : QString();
        }
        // 删除列由自绘委托（mDiscreteDeleteDelegate）负责：
        // 委托仅在"有删除行的完整表"下才应绘制，此处对窄列表/空态行/无行一律给空数据，
        // 即便列绑定残留也不会画出垃圾桶图标
        if (c == columnCount() - 1)
        {
            if (role == Qt::UserRole)
                return _hoverRow == r;
            return QVariant();
        }

        if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
        const int v = valueIndexOf(c);
        const QStringList& values = _rows->at(r).second;
        return (v >= 0 && v < values.size()) ? values.at(v) : QString();
    }

    bool mDiscreteRowTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || !index.isValid() || !_rows) return false;
        if (isEmptyPlaceholderRow(index.row()) || index.row() >= _rows->size()) return false;
        if (isOpColumn(index.column())) return false;
        // 数据写回由控件层（cellEditedByUser 监听者）统一处理，避免模型直接改 _rows
        emit cellEditedByUser(index.row(), index.column(), value.toString().trimmed());
        return true;
    }

    Qt::ItemFlags mDiscreteRowTableModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid()) return Qt::NoItemFlags;
        if (isEmptyPlaceholderRow(index.row())) return Qt::NoItemFlags;
        // 删除列：可选中（供委托接收点击），不可编辑
        if (isOpColumn(index.column())) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        // 标识列与分量列均可编辑
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    void mDiscreteRowTableModel::setHoverRow(int row)
    {
        if (_hoverRow == row) return;
        const int oldRow = _hoverRow;
        _hoverRow = row;
        const int opCol = columnCount() - 1;
        if (oldRow >= 0) emit dataChanged(index(oldRow, opCol), index(oldRow, opCol));
        if (row >= 0) emit dataChanged(index(row, opCol), index(row, opCol));
    }

    bool mDiscreteRowTableModel::isIdColumn(int col) const
    {
        return col >= 0 && col < idColumnCount();
    }

    bool mDiscreteRowTableModel::isOpColumn(int col) const
    {
        if (*_picking) return false;
        return col == columnCount() - 1;
    }

    // ------------------------------------------------------------------
    // 删除列自绘委托：绘制垃圾桶图标 + 处理点击（替代 setIndexWidget）
    // ------------------------------------------------------------------
    void mDiscreteDeleteDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        auto* rowModel = qobject_cast<const mDiscreteRowTableModel*>(index.model());
        // 仅完整表的操作列单元格绘制删除图标：窄列表/空态占位行/无行时纯背景
        if (!rowModel || !rowModel->isOpColumn(index.column()) || rowModel->isEmptyPlaceholderRow(index.row()))
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyledItemDelegate::paint(painter, option, index);

        const bool hovered = index.data(Qt::UserRole).toBool();
        // 悬停时淡红底，与原 deleteBtn:hover 一致
        if (hovered)
            painter->fillRect(option.rect, QColor(QStringLiteral("#fff5f5")));

        const QString glyph = QStringLiteral("🗑");
        painter->setPen(hovered ? QColor(QStringLiteral("#d95151")) : QColor(QStringLiteral("#788391")));
        QFont f = painter->font();
        f.setPixelSize(13);
        painter->setFont(f);
        painter->drawText(option.rect, Qt::AlignCenter, glyph);
    }

    bool mDiscreteDeleteDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
        const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        Q_UNUSED(option)
        if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            auto* rowModel = qobject_cast<const mDiscreteRowTableModel*>(index.model());
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

    namespace
    {
        // 2.2.6 吸附式弹窗（标准档）D3 对象离散数据关键尺寸
        constexpr int kRowHeight = 30;        // 可编辑数据行高（D8：标准 30px）
        constexpr int kHeaderHeight = 26;     // 表头/单行控件高（2.2.6：26px）
        constexpr int kActionsHeight = 26;    // 表格末行操作行高（数据区：末行标准高 26px）
        constexpr int kColWidth = 100;        // 数据列最小宽（D8：每列 100px，10 位有效数字）
        constexpr int kOpColWidth = 38;       // 行操作列固定宽（D8）
        constexpr int kPopupMargin = 12;      // 数据区左右边距（D8：示例左右各 12px）
        constexpr int kMaxTableHeight = 360;  // 表格区域超过该高度后内部滚动
        constexpr int kEmptyRowHeight = 120;  // 空态占位行高：提示文案垂直居中，表格区保留内容感
        constexpr int kMaxComponents = 32;

        // D3 表格内容样式：叠加在基类弹窗公共样式之上
        const QString kContentStyle = QStringLiteral(R"(
            /* 2.2.6 吸附式弹窗：末行操作及拾取完成行采用 panel 底色 #f5f7fa，与表格连续共边、无圆角无外框 */
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
            /* 末行操作格：panel 底色、共边无圆角；首格（拾取/完成拾取）无分界，
               其余格以左边框分隔（拾取格对齐标识列、＋合并数值列、清空格对齐删除列） */
            QWidget#actionsBar {
                background: #f5f7fa;
                border: 1px solid #e4e8ee;
                border-top: none;
            }
            QPushButton#pickAction {
                background: transparent; border: none;
                color: #1890ff; font-size: 12px;
            }
            QPushButton#pickAction:hover { background: #e8f3ff; }
            QPushButton#pickAction:disabled { color: #a6afbb; }
            QPushButton#tableAction {
                background: transparent; border: none; border-left: 1px solid #e4e8ee;
                color: #1890ff; font-size: 12px;
            }
            QPushButton#tableAction:hover { background: #e8f3ff; }
            QPushButton#tableAction:disabled { color: #a6afbb; }
            QPushButton#clearAction {
                background: transparent; border: none; border-left: 1px solid #e4e8ee;
                color: #d95151; font-size: 12px;
            }
            QPushButton#clearAction:hover { background: #fff5f5; }
            QPushButton#clearAction:disabled { color: #a6afbb; }
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

    mDiscreteDataControl::mDiscreteDataControl(const QString& title,
        const QStringList& valueHeaders,
        const PickSpec* pickSpec,
        QWidget* parent)
        : mAttachedPopupBase(title, parent)
        , _valueHeaders(valueHeaders)
        , _componentCount(qMax(1, valueHeaders.size()))
    {
        if (pickSpec && pickSpec->keyOf && !pickSpec->idHeaders.isEmpty())
        {
            _pickSpec = *pickSpec;
            _hasPick = true;
        }
        // 渲染云图为该形态的可选能力（D3：表下提供「渲染云图」，默认隐藏由宿主开启）
        setActionButtonVisible(false, QStringLiteral("渲染云图"), false);
        setActionHandler([this]() {
            showInlineMessage(QStringLiteral("渲染功能接口已预留，待宿主接入。"), false);
            });
        // 入口摘要初始态（标题由宿主行布局的 label 承担，按钮只显示摘要）
        updateEntrySummary();
    }

    mDiscreteDataControl::~mDiscreteDataControl()
    {
        if (_picking)
        {
            if (_viewportConn) disconnect(_viewportConn);
            _viewportConn = QMetaObject::Connection();
            _picking = false;
            if (g_pickingOwner == this)
                g_pickingOwner = nullptr;
            // 仅正常运行期需要恢复拾取工具栏；主窗口销毁期全局信号宿主
            // 可能已先行析构，发送即野指针调用
            if (!isShuttingDown())
            {
                emit mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
            }
        }
        // 弹窗子树随基类析构 delete _popup 级联销毁；表格视口上的过滤器须先摘除，
        // 否则级联销毁期间视口 Leave/Close 事件会回调进正在析构的 eventFilter
        if (_popup)
        {
            if (_table && _table->viewport())
                _table->viewport()->removeEventFilter(this);
        }
    }

    // ------------------------------------------------------------------
    // 对外接口适配
    // ------------------------------------------------------------------
    void mDiscreteDataControl::detachEntryButton(QWidget* newParent)
    {
        detachEntryWidget(newParent);
    }

    void mDiscreteDataControl::setRenderButtonVisible(bool visible, const QString& text)
    {
        // 渲染云图为执行型按钮（非拓展区开关），checkable 恒 false
        setActionButtonVisible(visible, text, false);
    }

    void mDiscreteDataControl::setRenderHandler(const std::function<void()>& handler)
    {
        setActionHandler(handler);
    }

    void mDiscreteDataControl::setImportHandler(const std::function<void(const ImportParams&)>& handler)
    {
        mAttachedPopupBase::setImportHandler(handler);
    }

    void mDiscreteDataControl::setExportHandler(const std::function<void(const QString&)>& handler)
    {
        mAttachedPopupBase::setExportHandler(handler);
    }

    void mDiscreteDataControl::setPickSpec(const PickSpec& pickSpec)
    {
        const bool valid = pickSpec.keyOf && !pickSpec.idHeaders.isEmpty();
        // 描述未变化（宿主显隐刷新入口每次都会下发）直接返回，避免重复重建
        // 表格与选中态回种；keyOf 闭包不可比较，按其余字段成组判定
        if (valid == _hasPick && _pickSpec.typeName == pickSpec.typeName
            && _pickSpec.idHeaders == pickSpec.idHeaders
            && _pickSpec.pickTypes == pickSpec.pickTypes
            && _pickSpec.pickFilter == pickSpec.pickFilter)
        {
            return;
        }
        // 拾取会话中对象类型即将切换：先结束会话（内部补发一次 dataChanged，
        // 宿主此刻仍按旧类型同步行数据），再切换描述
        if (_picking)
            stopPicking();
        _pickSpec = pickSpec;
        _hasPick = valid;
        rebuildTable();
        updateActionStates();
        // 选中态按新对象类型整体回种（旧类型选中态随选择容器清空）
        syncSelectionToRows();
    }

    void mDiscreteDataControl::setComponentCount(int count)
    {
        count = qMax(1, count);
        if (count == _componentCount)
            return;
        _componentCount = count;
        for (Row& row : _rows)
        {
            while (row.second.size() > count) row.second.removeLast();
            while (row.second.size() < count) row.second.append(QString());
        }
        rebuildTable();
    }

    // ------------------------------------------------------------------
    // 内容构建（mAttachedPopupBase 钩子：弹窗首次创建时调用，派生类已构造完成）
    // ------------------------------------------------------------------
    void mDiscreteDataControl::buildContent(QWidget* body, QVBoxLayout* layout)
    {
        // 表格内容样式叠加到弹窗公共样式之后（后者在前，后者同名规则优先）
        body->setStyleSheet(body->styleSheet() + kContentStyle);

        // 弹窗任意路径关闭（点击外部/✕/入口按钮/宿主 closePopup）时熄灭拾取高亮
        connect(this, &mDiscreteDataControl::popupClosed, this, &mDiscreteDataControl::onPopupClosed);

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
        _tableModel = new mDiscreteRowTableModel(_table);
        _tableModel->bind(&_rows, &_pickSpec.idHeaders, &_valueHeaders, &_componentCount, &_picking);
        _tableModel->bindEmptyText(&_title);
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
        // 末行操作格与表格列对齐：表格横向滚动/列宽变化时同步拾取格与清空格宽度
        connect(_table->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &mDiscreteDataControl::syncActionCellWidths);

        // 删除列：自绘委托替代 indexWidget（无 hide/reset 弃用问题），
        // 视口鼠标移动驱动悬停高亮；委托与操作列的绑定在 rebuildTable 中
        // 按当前列结构设置（初始模型列为空，不能在此绑定）
        _deleteDelegate = new mDiscreteDeleteDelegate(_table);
        _table->setMouseTracking(true);
        connect(_deleteDelegate, &mDiscreteDeleteDelegate::deleteRowRequested,
            this, &mDiscreteDataControl::removeRowAt);
        connect(_table, &QTableView::entered, this, [this](const QModelIndex& idx) {
            if (idx.isValid() && _tableModel->isOpColumn(idx.column()))
                _tableModel->setHoverRow(idx.row());
            else
                _tableModel->setHoverRow(-1);
            });
        _table->viewport()->installEventFilter(this);
        boxLayout->addWidget(_table);

        // 底部操作行（表格末行单元格，2.2.6）：拾取格对齐对象标识列、＋合并数值列、清空格对齐删除列；
        // 与表格连续共边、无按钮间距、无圆角（样式表 #actionsBar 承担边框与 panel 底色）
        _actionsBar = new QWidget(tableBox);
        _actionsBar->setObjectName(QStringLiteral("actionsBar"));
        _actionsBar->setFixedHeight(kActionsHeight);
        QHBoxLayout* actionsLayout = new QHBoxLayout(_actionsBar);
        actionsLayout->setContentsMargins(0, 0, 0, 0);
        actionsLayout->setSpacing(0);
        _pickBtn = makeContentButton(QString(), QString(), QStringLiteral("pickAction"));
        _addRowBtn = makeContentButton(QStringLiteral("＋"), QStringLiteral("添加数据行"), QStringLiteral("tableAction"));
        _clearBtn = makeContentButton(QStringLiteral("清空"), QStringLiteral("清空全部数据"), QStringLiteral("clearAction"));
        _pickBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        _addRowBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _clearBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        _clearBtn->setFixedWidth(kOpColWidth);
        actionsLayout->addWidget(_pickBtn);
        actionsLayout->addWidget(_addRowBtn, 1);
        actionsLayout->addWidget(_clearBtn);
        boxLayout->addWidget(_actionsBar);

        wrapLayout->addWidget(tableBox);
        layout->addWidget(tableWrap, 1);

        connect(_pickBtn, &QPushButton::clicked, this, [this]() {
            if (_picking) finishPicking(); else startPicking();
            });
        connect(_addRowBtn, &QPushButton::clicked, this, [this]() { appendRow(qMakePair(-1, -1), true); });
        connect(_clearBtn, &QPushButton::clicked, this, [this]() {
            if (_rows.isEmpty()) return;
            _rows.clear();
            rebuildTable();
            updateEntrySummary();
            updateActionStates();
            syncSelectionToRows(); // 高亮与行数据同步：清空后视口不再保留任何选中态
            emit dataChanged();
            });

        // 单元格编辑：模型 setData 时同步行数据并发出信号（业务逻辑在控件层处理）
        connect(_tableModel, &mDiscreteRowTableModel::cellEditedByUser, this,
            [this](int row, int col, const QString& text) {
                if (_syncingTable || _picking) return;
                if (row < 0 || row >= _rows.size()) return;

                // 标识列编辑：回填业务 key（供手动加行后录入编号），去重并同步选中态
                if (_tableModel->isIdColumn(col))
                {
                    bool ok = false;
                    const int id = text.toInt(&ok);
                    QPair<int, int>& key = _rows[row].first;
                    const int oldVal = (col == 0 ? key.first : key.second);
                    const int newVal = (ok && id > 0) ? id : -1;
                    if (oldVal == newVal) return;
                    // 去重按「所有已填标识位完全相同」判定（含单标识相等、另一列都未填），
                    // 倒序补录编号也能检出；判定用候选 key（本列取新值、其余列取旧值），
                    // 命中冲突时 _rows 尚未改动，视图回显旧值即可。
                    const QPair<int, int> candidate = (col == 0)
                        ? qMakePair(newVal, key.second) : qMakePair(key.first, newVal);
                    if (findDuplicateIdRow(candidate, row) >= 0)
                    {
                        rebuildTable();
                        showInlineMessage(QStringLiteral("对象编号与已有行重复。"), true);
                        return;
                    }
                    // 校验编号在当前网格中真实存在（仅首列标识：节点/单元 id）：
                    // 无效编号直接清空本单元格输入并提示，不进 _rows，避免后续
                    // 高亮回种把不存在的 id 送进渲染层（空网格/越界崩溃的来源之一）。
                    if (col == 0 && newVal > 0 && _hasPick)
                    {
                        auto* vm = getCVM;
                        auto* modelData = vm ? vm->getModelData() : nullptr;
                        mMeshData* meshData = modelData ? modelData->getMeshData() : nullptr;
                        const bool meshReady = meshData && meshData->_nodedata.xyz
                            && meshData->_nodedata.nodeamount > 0;
                        // 网格未加载：编号无从校验，按无效处理（清除输入并提示），
                        // 不允许「先存下来等模型加载」——该行数据保存/导出后即失去上下文
                        if (!meshReady)
                        {
                            rebuildTable();
                            showInlineMessage(QStringLiteral("当前未加载模型，无法录入%1编号。")
                                .arg(_pickSpec.typeName), true);
                            return;
                        }
                        bool exists = true;
                        if (_pickSpec.pickTypes.contains(PickDataType::Node))
                            exists = meshData->_nodedata.nodeidmap.contains(newVal);
                        else
                            exists = meshData->getPartIdByEleId(newVal) >= 0;
                        if (!exists)
                        {
                            rebuildTable(); // 回显旧值（清空用户刚输入的无效编号）
                            showInlineMessage(QStringLiteral("%1 %2 在当前模型中不存在，已清除输入。")
                                .arg(_pickSpec.typeName).arg(newVal), true);
                            return;
                        }
                    }
                    if (col == 0) key.first = newVal; else key.second = newVal;
                    showInlineMessage(QString(), false);
                    syncSelectionToRows();
                    emit dataChanged();
                    return;
                }

                QStringList& values = _rows[row].second;
                const int valueIndex = _tableModel->valueIndexOf(col);
                if (valueIndex >= 0 && valueIndex < values.size() && values[valueIndex] != text)
                {
                    values[valueIndex] = text;
                    emit cellEdited(_rows[row].first, valueIndex, text);
                    emit dataChanged();
                }
            });

        // 隐藏期把行数据/空态一次性灌入模型，首开不再二次 reset
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
    }

    // ------------------------------------------------------------------
    // 内容尺寸（D8 宽度规则；拾取窄列表见 popupMinimumWidth）
    // ------------------------------------------------------------------
    QSize mDiscreteDataControl::contentSizeHint() const
    {
        // D8 宽度规则：W = max(480, 数据列数×100 + 38 操作列 + 框架 F)，
        // 最小宽度下限与视口约束由基类统一处理；拾取态不应用 480px 下限。
        const int idCols = _picking ? 1 : (_hasPick ? _pickSpec.idHeaders.size() : 0);
        const int dataCols = _picking ? 0 : _componentCount;
        const int opCol = _picking ? 0 : 1;

        int tableWidth = idCols * kColWidth + dataCols * kColWidth + (opCol ? kOpColWidth : 0);
        tableWidth += 2; // 表格自身 1px 边框

        const bool empty = _rows.isEmpty();
        const int rows = qMax(_rows.size(), 1); // 空态保留一行提示
        // 空态占位行加高（提示文案垂直居中，避免单行被压缩成贴表头的一条）
        const int rowsHeight = empty ? kEmptyRowHeight : rows * kRowHeight;
        int tableHeight = kHeaderHeight + rowsHeight + 2;
        const bool overflow = tableHeight > kMaxTableHeight;
        tableHeight = qMin(tableHeight, kMaxTableHeight);
        // 只为实际纵向滚动条补偿（D8：无溢出不预留空白槽；拾取窄列表同样可能出现滚动条）
        if (overflow)
            tableWidth += qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

        // 拾取窄列表隐藏行数统计、底部工具栏与文件设置面板
        // （D6 原型仅保留：‹ 标题 ✕、标识列表、完成拾取）
        if (_rowCountLabel) _rowCountLabel->setVisible(!_picking);
        if (_footer) _footer->setVisible(!_picking);
        if (_importPanel) _importPanel->setVisible(_importPanel->isVisible() && !_picking);
        if (_exportPanel) _exportPanel->setVisible(_exportPanel->isVisible() && !_picking);

        // 数据区左右边距 + 上下边距 + 末行操作行
        // 拾取态弹窗最小宽度须容纳「完成拾取」按钮文字（D6：完成拾取占满全部标识列），
        // 否则按钮被压缩后文字截断不可见
        int contentWidth = tableWidth + 2 * kPopupMargin;
        if (_picking && _pickBtn)
        {
            const int pickTextWidth = _pickBtn->fontMetrics().horizontalAdvance(QStringLiteral("完成拾取")) + 24;
            contentWidth = qMax(contentWidth, pickTextWidth + 2 * kPopupMargin);
        }
        return QSize(contentWidth, 10 + tableHeight + kActionsHeight + 10);
    }

    int mDiscreteDataControl::popupMinimumWidth() const
    {
        // 拾取窄列表不采用 480px 普通态下限（D8：完成后恢复普通态宽度）
        if (_picking)
            return 176;
        return mAttachedPopupBase::popupMinimumWidth();
    }

    void mDiscreteDataControl::onPopupResized()
    {
        mAttachedPopupBase::onPopupResized();
        // 弹窗 Resize 时同步末行操作格列宽（Stretch 列宽随动后清空格需重新对齐）
        syncActionCellWidths();
    }

    // ------------------------------------------------------------------
    // 弹窗关闭（先结束拾取会话，再走基类隐藏）
    // ------------------------------------------------------------------
    void mDiscreteDataControl::prepareClosePopup()
    {
        // 主窗口销毁期：只做界面收尾（隐藏弹窗），拾取状态直接复位，
        // 不走 finishPicking（其清选/回种/补发 dataChanged 都会触碰已销毁的
        // 视口与宿主面板槽，正是关闭崩溃的来源）
        if (!_picking) return;
        if (isShuttingDown())
        {
            _picking = false;
            if (_viewportConn) disconnect(_viewportConn);
            _viewportConn = QMetaObject::Connection();
            return;
        }
        // finishPicking 内部 emit dataChanged，宿主槽可能 delete 本控件；
        // 该路径统一走 closePopup(true) 的 QPointer 防护
        finishPicking();
    }

    void mDiscreteDataControl::closePopup(bool deactivatePick)
    {
        if (!_popup) return;
        QPointer<mDiscreteDataControl> guard(this);
        if (deactivatePick)
        {
            prepareClosePopup();   // 内部 emit dataChanged，宿主槽可能 delete this
            if (!guard) return;
        }
        if (_popup && _popup->isVisible())
        {
            _popup->hide();
            emit popupClosed();
            // popupClosed 槽内宿主可能重建面板并 delete 本控件
            if (!guard) return;
        }
        if (entryButton()) entryButton()->setChecked(false);
        showInlineMessage(QString(), false);
        // 关闭弹窗时收起文件设置面板并退出入口选中态，下次打开为干净状态
        closeFilePanels();
    }

    // ------------------------------------------------------------------
    // 事件过滤：拾取期间点击外部不关闭；表格视口悬停高亮收尾
    // ------------------------------------------------------------------
    bool mDiscreteDataControl::eventFilter(QObject* watched, QEvent* event)
    {
        // 宿主销毁期防御、弹窗 Close/外部点击/跟随等公共路径由基类承担；
        // 此处只处理本控件特有分支
        if (isShuttingDown())
            return QWidget::eventFilter(watched, event);

        // 视口鼠标离开：清除删除列悬停高亮
        if (watched == (_table ? _table->viewport() : nullptr) && event->type() == QEvent::Leave)
        {
            if (_tableModel) _tableModel->setHoverRow(-1);
            return false;
        }

        // 拾取期间点击外部不关闭：拾取完毕或主动关闭前弹窗一直钉在按钮旁
        if (_picking && _popup && _popup->isVisible() && event->type() == QEvent::MouseButtonPress)
        {
            QWidget* w = qobject_cast<QWidget*>(watched);
            if (w && w != _popup && !_popup->isAncestorOf(w))
                return false;
        }

        // 宿主顶层窗口移动时锚点不可见（入口被嵌入宿主行），拾取期间保持弹窗不关闭
        if (_picking && event->type() == QEvent::Move)
        {
            QWidget* w = qobject_cast<QWidget*>(watched);
            if (w && w->isWindow() && w == this->window())
            {
                QWidget* anchor = entryButton();
                if (anchor && !anchor->isVisible())
                    return false;
            }
        }

        return mAttachedPopupBase::eventFilter(watched, event);
    }

    // ------------------------------------------------------------------
    // 表格
    // ------------------------------------------------------------------
    QString mDiscreteDataControl::componentHeader(int index) const
    {
        if (index >= 0 && index < _valueHeaders.size())
            return _valueHeaders.at(index);
        return QStringLiteral("分量 %1").arg(index + 1);
    }

    void mDiscreteDataControl::rebuildTable()
    {
        const int idCols = _picking ? 1 : (_hasPick ? _pickSpec.idHeaders.size() : 0);
        const int dataCols = _picking ? 0 : _componentCount;
        const int opCol = _picking ? 0 : 1;
        const int totalCols = qMax(1, idCols + dataCols + opCol);

        // 弹窗未建（构造期/setRows 在首次打开前调用）只保留数据，打开时一次性刷新
        if (!_popup || !_tableModel || !_table) return;

        // 弹窗隐藏期间不做视图刷新（万行级下即使模型驱动，通知/布局也有开销），
        // 只把行数据留在权威来源 _rows，下次打开时一次性 notifyReset + rebuild；
        // 删除列委托绑定只取决于列结构，与可见性无关，隐藏期也必须完成
        // （此前绑定只在可见分支执行，而打开先于 show 调用本函数，导致
        // 「导入 mx 后直接打开面板」场景委托从未绑定：行末删除按钮不绘制、点击无响应）。
        if (!_popup->isVisible())
        {
            if (_deleteDelegate)
            {
                _table->setItemDelegateForColumn(opCol ? (totalCols - 1) : 0, _deleteDelegate);
                _tableModel->setHoverRow(-1);
            }
            // 空态占位行合并为一整行，使首次打开（弹窗尚未可见）也居中显示提示；
            // setSpan/clearSpans 不依赖视图可见性，在此同步处理保证两态一致
            if (_rows.isEmpty())
                _table->setSpan(0, 0, 1, totalCols);
            else
                _table->clearSpans();
            return;
        }

        _syncingTable = true;

        // 模型结构可能变化（拾取窄列表 1 列 / 完整表 N 列）：整体重置让视图重取行/列数与表头。
        // 删除列委托必须在本次 reset 之后绑定：QHeaderView 对"绑定到最后一列的列委托"
        // 会在模型 reset/结构变化时静默摘除（委托只按旧列索引保存，列数变化后索引失效
        // 即弃用），表现为导入大数据文件后直接打开弹窗删除列不绘制、点击无响应。
        _tableModel->notifyReset();

        // 列宽/拉伸模式（reset 后视图保留列宽设置，这里按模式重设一次）
        for (int c = 0; c < totalCols; ++c)
        {
            const bool isOp = (opCol && c == totalCols - 1);
            if (_picking)
            {
                // 窄列表单列拉伸填满，消除弹窗宽度取整/滚动条换算带来的右侧空白
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
            }
            else if (isOp)
            {
                // 删除列固定宽度（D8：操作列固定 38px），不随弹窗拉伸
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
                _table->setColumnWidth(c, kOpColWidth);
            }
            else
            {
                // 数据列 Stretch：多余宽度均分（D8），弹窗拉大时列宽随动，缩小时按比例压缩
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
            }
        }

        if (_deleteDelegate)
        {
            _table->setItemDelegateForColumn(opCol ? (totalCols - 1) : 0, _deleteDelegate);
            _tableModel->setHoverRow(-1);
        }

        // 空态占位行：合并所有列为一整行，提示文案在整个表格区水平+垂直居中。
        // 必须在 notifyReset 之后调用 setSpan——beginResetModel 会清空视图的 span 表，
        // reset 之前设置的 span 在视图重取数据时被丢弃，这正是首次打开不居中的原因。
        if (_rows.isEmpty())
            _table->setSpan(0, 0, 1, totalCols);
        else
            _table->clearSpans();

        _syncingTable = false;
        // 空态占位行加高（提示文案垂直居中）；有数据时恢复统一数据行高
        _table->verticalHeader()->setDefaultSectionSize(_rows.isEmpty() ? kEmptyRowHeight : kRowHeight);
        // 列结构变化后重新对齐末行操作格
        syncActionCellWidths();
        repositionPopup();
        // notifyReset 后布局（LayoutRequest）在下一次事件循环才把 Stretch 列的实际
        // 宽度分配给表头，此刻 sectionSize 仍是未定值，操作格固定宽度随之失真
        //（首开/进入拾取时按钮文字被压没，直到下一次 resize 才恢复）。
        // 延迟到布局完成后按最终列宽再对齐一次。
        QTimer::singleShot(0, this, [this]() {
            if (_popup && _popup->isVisible())
                syncActionCellWidths();
        });
    }

    void mDiscreteDataControl::onPopupShown()
    {
        // 首开 show 之后视图才真正完成列布局（Stretch 列在控件可见时才分配实际宽度）。
        // 隐藏期 rebuildTable 设置的 setSpan/列宽基于尚未布局的尺寸，show 后视图重排
        // 会把空态占位行的 span 高度/宽度重新按单列计算，导致提示退回首列顶部、
        // 操作行被挤压。show 完成后强制再走一次 rebuildTable（此时弹窗可见，走可见分支），
        // 让 span/行高/列宽基于最终布局生效，保证首次打开即整块居中。
        rebuildTable();
        // 操作行文本/可见性随拾取态变化（拾取X/完成拾取），隐藏期间的状态可能在
        // 交叉时序下滞后；每次打开时按当前状态统一补一次，保证底部行即刻完整
        updateActionStates();
        // 弹窗打开即按当前行回种选中态高亮（表格所见即高亮）；
        // 拾取会话进行中容器即会话状态，内部自行跳过
        syncSelectionToRows();
    }

    void mDiscreteDataControl::onPopupClosed()
    {
        // 弹窗经任意路径关闭（派生/基类 closePopup 均发出 popupClosed）后
        // 拾取高亮随之熄灭：行数据保留，重开弹窗或开始拾取时再回种。
        // 关闭路径若结束过拾取会话（prepareClosePopup→finishPicking），
        // 此刻 _picking 已为 false；仍处拾取态时容器是会话状态，不动。
        if (!_picking)
            clearPickingSelection();
    }

    void mDiscreteDataControl::syncActionCellWidths()
    {
        if (!_table || !_pickBtn || !_clearBtn) return;
        QHeaderView* header = _table->horizontalHeader();
        // 末行操作格与表格列对齐（2.2.6 数据区）：
        // 拾取格对齐对象标识列、清空格对齐删除列、＋合并数值列（拉伸）。
        // 表格横向溢出时按可视区换算：sectionViewportPosition + sectionSize。
        const int idCols = _tableModel ? _tableModel->idColumnCount() : 0;
        int pickWidth = 0;
        for (int c = 0; c < idCols; ++c)
            pickWidth += header->sectionSize(c);
        int clearWidth = kOpColWidth;
        if (!_picking)
        {
            const int opCol = header->count() - 1;
            if (opCol >= 0)
                clearWidth = header->sectionSize(opCol);
        }
        // 拾取态下拾取格铺满全部标识列（D6：完成拾取占满全部标识列），
        // 且必须保证按钮文字完整可见，不被固定宽度截断
        const int minTextWidth = _pickBtn->fontMetrics().horizontalAdvance(
            _picking ? QStringLiteral("完成拾取") : QStringLiteral("拾取%1").arg(_pickSpec.typeName)) + 12;
        _pickBtn->setFixedWidth(qMax(qMax(pickWidth, minTextWidth), 0));
        _clearBtn->setFixedWidth(qMax(clearWidth, 0));
    }

    void mDiscreteDataControl::removeRowAt(int row)
    {
        if (row < 0 || row >= _rows.size()) return;
        // 行删除按 begin → 改 _rows → end 三段式：endRemoveRows 内视图会立即
        // 回查 rowCount()/data()，数据必须先于 end 同步，否则越界崩溃
        const bool notify = _popup && _popup->isVisible() && _tableModel;
        if (notify) _tableModel->beginRemoveRow(row);
        _rows.removeAt(row);
        if (notify) _tableModel->endRemoveRow();
        // 删到空表时占位行（「尚未添加…」）需要一次整体重置才能出现：
        // begin/endRemoveRows 只表达行数 1→0，占位行 0→1 视图无从得知，
        // 不重置则空态下视图无行、删除列/提示文案均不绘制
        if (notify && _rows.isEmpty())
        {
            _tableModel->notifyReset();
            // notifyReset 清空视图 span 表，空态合并行需在其后重建，
            // 否则提示退回只画在第一列；行高同步恢复为空态加高
            const int totalCols = _tableModel->columnCount();
            _table->setSpan(0, 0, 1, totalCols);
            _table->verticalHeader()->setDefaultSectionSize(kEmptyRowHeight);
        }
        // 行数变化后按内容重新自适应弹窗高度（与 appendRow 对称：增量路径不走 rebuildTable）
        repositionPopup();
        updateEntrySummary();
        updateActionStates();
        syncSelectionToRows(); // 高亮与行数据同步：被删除的行对应的选中态一并移除
        emit dataChanged();
    }

    QString mDiscreteDataControl::idTextOf(const QPair<int, int>& key) const
    {
        if (!_hasPick) return QString();
        if (_pickSpec.idHeaders.size() > 1)
        {
            const QString a = key.first > 0 ? QString::number(key.first) : QString();
            const QString b = key.second > 0 ? QString::number(key.second) : QString();
            return a + QStringLiteral(" / ") + b;
        }
        return key.first > 0 ? QString::number(key.first) : QString();
    }

    void mDiscreteDataControl::appendRow(const QPair<int, int>& key, bool focus)
    {
        Row row;
        row.first = key;
        row.second = QStringList();
        for (int i = 0; i < _componentCount; ++i) row.second.append(QString());
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
            // 新行编号未填时聚焦首个标识列，先录编号再填分量（分量写回依赖已填编号）；
            // 编号已存在（如拾取回填的行）才直接聚焦首个分量列
            const int focusCol = (key.first <= 0 && _tableModel->idColumnCount() > 0)
                ? 0 : _tableModel->idColumnCount();
            const QModelIndex idx = _tableModel->index(_rows.size() - 1, focusCol);
            if (idx.isValid())
            {
                _table->scrollTo(idx);
                _table->setCurrentIndex(idx);
                _table->edit(idx);
            }
        }
    }

    void mDiscreteDataControl::changeComponents(int delta)
    {
        // 已按需求移除分量增减入口；保留接口供个别面板程序式调整（无 UI 暴露）
        const int count = _componentCount + delta;
        if (count < 1 || count > kMaxComponents) return;
        _componentCount = count;
        for (Row& row : _rows)
        {
            while (row.second.size() > count) row.second.removeLast();
            while (row.second.size() < count) row.second.append(QString());
        }
        rebuildTable();
        updateActionStates();
        emit dataChanged();
    }

    void mDiscreteDataControl::updateEntrySummary()
    {
        if (QLabel* summary = entrySummaryLabel())
        {
            summary->setText(_rows.isEmpty()
                ? QStringLiteral("点击编辑数据表")
                : QStringLiteral("已定义 %1 行数据").arg(_rows.size()));
        }
        setMetaText(QStringLiteral("%1 行").arg(_rows.size()));
    }

    void mDiscreteDataControl::updateActionStates()
    {
        const bool hasRows = !_rows.isEmpty();
        if (_clearBtn) _clearBtn->setEnabled(hasRows);
        if (_exportBtn) _exportBtn->setEnabled(hasRows);
        setActionButtonEnabled(hasRows);
        if (_importBtn) _importBtn->setEnabled(!_picking);
        if (_pickBtn)
        {
            _pickBtn->setText(_picking
                ? QStringLiteral("完成拾取")
                : QStringLiteral("拾取%1").arg(_pickSpec.typeName));
            _pickBtn->setVisible(_hasPick);
        }
        if (_addRowBtn) _addRowBtn->setVisible(!_picking);
        if (_clearBtn) _clearBtn->setVisible(!_picking);
    }

    // ------------------------------------------------------------------
    // 数据接口
    // ------------------------------------------------------------------
    void mDiscreteDataControl::setRows(const QVector<Row>& rows)
    {
        _rows = rows;
        for (Row& row : _rows)
        {
            while (row.second.size() < _componentCount) row.second.append(QString());
            while (row.second.size() > _componentCount) row.second.removeLast();
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
        // 高亮与行数据同步：外部替换行数据后，选中态随之整体刷新
        syncSelectionToRows();
    }

    QVector<mDiscreteDataControl::Row> mDiscreteDataControl::rows() const
    {
        return _rows;
    }

    // ------------------------------------------------------------------
    // 拾取（直接驱动视口选择容器，不经过通用拾取控件，不渲染拾取高亮）
    // ------------------------------------------------------------------
    void mDiscreteDataControl::startPicking()
    {
        if (!_hasPick) return;
        // 入口一次性取视口局部变量：后续隔着多个同步信号，任何二次 getCVM
        // 取值期间用户切换/关闭模型都会得到悬空/空指针
        auto* vm = getCVM;
        auto* preWidget = vm ? vm->getPreRenderWidget() : nullptr;
        if (!preWidget)
        {
            showInlineMessage(QStringLiteral("当前无可用的模型视口，无法拾取。"), true);
            return;
        }

        // 多实例互斥：已有其它实例在拾取时先结束其会话，
        // 共享的视口选择容器/高亮层不支持并发会话
        if (g_pickingOwner && g_pickingOwner != this)
            g_pickingOwner->stopPicking();

        QPointer<mDiscreteDataControl> guard(this);
        _picking = true;
        g_pickingOwner = this;

        // 订阅视口选择容器：必须先于任何同步信号完成连接——
        // 信号槽重入可能 delete 本控件，guard 失效后不得再写任何成员
        if (_viewportConn) disconnect(_viewportConn);
        _viewportConn = connect(
            preWidget->getSelectionContainer(),
            &mxr::SelectionDataContainer::ReadyToReadContainer,
            this, &mDiscreteDataControl::onViewportPickFinished);

        // 限定拾取工具栏白名单，从根上禁用白名单外类型
        if (!_pickSpec.pickTypes.isEmpty())
        {
            QVector<bool> enabled(10, false);
            if (_pickSpec.pickTypes.contains(PickDataType::GeoPoint))    enabled[1] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::GeoEdge))     enabled[2] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::GeoFace))     enabled[3] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::GeoSolid))    enabled[4] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::Node))        enabled[5] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::ElementEdge)) enabled[6] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::ElementFace)) enabled[7] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::Element))     enabled[8] = true;
            if (_pickSpec.pickTypes.contains(PickDataType::Part))        enabled[9] = true;
            enabled[0] = enabled[1] && enabled[2] && enabled[3] && enabled[4];
            emit mGlobalSignals::getInstance()->setPickTypesEnabledSig(enabled);
            if (!guard) return;
        }

        // 强制切换到目标拾取过滤器（如仅节点）
        if (_pickSpec.pickFilter >= 0)
        {
            emit mMechGlobalSignals::getInstance()->activePickSig(_pickSpec.pickFilter);
            if (!guard) return;
        }

        // 把已有行写回视口选择容器：恢复这些对象的选中态高亮，
        // 使再次拾取时在已有基础上累加，而不是清空重新拾取
        // （内部 updatePaint 同样是同步路径，可能重入 delete 本控件）
        seedSelectionFromRows();
        if (!guard) return;

        rebuildTable();
        updateActionStates();
        if (_popup && !_popup->isVisible())
            togglePopup();
    }

    void mDiscreteDataControl::finishPicking()
    {
        if (!_picking) return;
        _picking = false;
        if (g_pickingOwner == this)
            g_pickingOwner = nullptr;

        if (_viewportConn) disconnect(_viewportConn);
        _viewportConn = QMetaObject::Connection();

        // 主窗口销毁期收尾：不触碰视口/渲染/全局单例，也不补发 dataChanged
        // （宿主面板槽内会做 map 重建与符号渲染，此时视口已销毁）
        if (isShuttingDown())
        {
            return;
        }

        // 恢复拾取工具栏为全部可用：同步信号槽可能重建面板并 delete 本控件
        QPointer<mDiscreteDataControl> guard(this);
        emit mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>());
        if (!guard) return;
        // 拾取结束（完成拾取/直接关闭）：清空会话残留后按弹窗可见性收尾——
        // 弹窗仍打开（点了「完成拾取」回到完整表格）时按当前行回种，
        // 高亮与表格所见保持一致；弹窗随之关闭时回种会被 popupClosed 清理。
        clearPickingSelection();
        if (_popup && _popup->isVisible())
            seedSelectionFromRows();

        rebuildTable();
        updateActionStates();
        // 拾取期间为不中断视口交互，会话内每次拾取回调都不发 dataChanged（宿主渲染/
        // 重建 nodeForceMap_ 等重操作随之冻结）；确认拾取后统一补发一次，宿主此时才
        // 执行渲染等收尾操作。
        emit dataChanged();
        // dataChanged 槽内宿主可能重建面板并 delete 本控件，此后不得再访问成员
    }

    void mDiscreteDataControl::stopPicking()
    {
        if (_picking)
            finishPicking();
    }

    void mDiscreteDataControl::seedSelectionFromRows()
    {
        if (!_hasPick) return;
        auto* vm = getCVM;
        auto* preWidget = vm ? vm->getPreRenderWidget() : nullptr;
        if (!preWidget) return;

        auto* container = preWidget->getSelectionContainer();
        if (!container) return;

        auto infoTypeFor = [this]() -> mxr::SelectionInfoType {
            if (_pickSpec.pickTypes.contains(PickDataType::Node))        return mxr::SelectionInfoType::MeshNode;
            if (_pickSpec.pickTypes.contains(PickDataType::ElementEdge)) return mxr::SelectionInfoType::MeshLine;
            if (_pickSpec.pickTypes.contains(PickDataType::ElementFace)) return mxr::SelectionInfoType::MeshFace;
            if (_pickSpec.pickTypes.contains(PickDataType::Element))     return mxr::SelectionInfoType::MeshElement;
            return mxr::SelectionInfoType::MeshTypeEnd;
        };
        const mxr::SelectionInfoType infoType = infoTypeFor();
        if (infoType == mxr::SelectionInfoType::MeshTypeEnd) return;

        std::vector<mxr::MeshSelectInfoData> infos;
        infos.reserve(_rows.size());
        // 渲染侧 getVertexByFaceIndex 依赖 part_id 定位单元面顶点，
        // 行 key 不含 part_id，按单元 ID 反查部件（同部件内逐单元哈希命中，
        // 跨部件才整表遍历；万行级同部件场景接近 O(N)），避免 part_id=-1 时越界崩溃
        // 模型已卸载但渲染部件仍在的中间态下 getModelData() 为空
        auto* modelData = vm->getModelData();
        if (!modelData) return;
        mMeshData* meshData = modelData->getMeshData();
        // 界面无模型/网格未提交（xyz 未分配或节点数为 0）时直接跳过回种：
        // 渲染层 getVertexByPointID 对空网格做 nodeidmap[id]*3 解引用，
        // 无防护即空指针/越界崩溃；行数据保留，待模型加载后再次同步时恢复高亮
        if (!meshData || !meshData->_nodedata.xyz || meshData->_nodedata.nodeamount <= 0)
            return;
        // 单元 ID → part_id 会话级缓存：同一次回种内大量行共享同一单元时只查一次，
        // 避免对同一单元反复整表遍历（万行级下单部件场景从 O(N×部件数) 降为 O(N)）
        QHash<int, int> partIdByCellId;
        partIdByCellId.reserve(static_cast<int>(_rows.size()));
        // 节点拾取下渲染层 getVertexByPointID 直接以节点 id 索引 nodeidmap/xyz，
        // 手动填写不存在的节点 id 会越界崩溃；单元/面/线拾取则有 part_id 反查防护
        const bool isNodePick = (infoType == mxr::SelectionInfoType::MeshNode);
        for (const Row& row : _rows)
        {
            if (row.first.first <= 0) continue;
            // 手动填写的节点 id 可能不在当前网格中（先填编号后加载模型/填错编号）：
            // nodeidmap.value(id) 对缺失 key 返回默认 0，会把第 0 个节点错误高亮，
            // 网格为空时更是直接空指针解引用；先校验 id 存在再进选择容器
            if (isNodePick && !meshData->_nodedata.nodeidmap.contains(row.first.first))
                continue;
            mxr::MeshSelectInfoData info;
            info.data_id.element_id = row.first.first;
            info.data_index.line_index = row.first.second; // 面/线序号，无则为 -1
            auto it = partIdByCellId.constFind(row.first.first);
            if (it == partIdByCellId.constEnd())
            {
                const int partId = meshData->getPartIdByEleId(row.first.first);
                it = partIdByCellId.insert(row.first.first, partId);
            }
            info.part_id = it.value();
            infos.push_back(info);
        }
        if (infos.empty()) return;

        container->getMeshSelectData()->UpdateSelectDataByType(
            mxr::SelectionOperate::ADD, infoType, infos);
        // 主动刷新一次高亮渲染，让已有对象立即显示选中态
        MechPreData::UpdatePreWidgetHighlightWithoutRTRC(preWidget);
        preWidget->updatePaint();
    }

    void mDiscreteDataControl::syncSelectionToRows()
    {
        // 拾取高亮与弹窗可见性同步：非拾取期间任何行变化（删除/清空/外部设值）后，
        // 弹窗打开时清空残留并按当前行回种（表格所见即高亮）；弹窗关闭时只清空
        // 不回种（行数据保留，重开弹窗或开始拾取时再回种）。
        // 拾取会话进行中不调用（此时容器即会话状态，由视口交互驱动）。
        if (_picking || !_hasPick) return;
        auto* vm = getCVM;
        if (!vm || !vm->getPreRenderWidget()) return;
        clearPickingSelection();
        if (_popup && _popup->isVisible())
            seedSelectionFromRows();
    }

    void mDiscreteDataControl::clearPickingSelection()
    {
        // 拾取会话进行中选择容器即会话状态，由视口交互驱动，不打断
        if (_picking) return;
        auto* vm = getCVM;
        auto* preWidget = vm ? vm->getPreRenderWidget() : nullptr;
        if (!preWidget) return;
        // 先清容器数据，再清高亮渲染层：只清容器时渲染层可能留有上次刷新的高亮残影
        MechPreData::ClearPreWidgetMeshPick(preWidget);
        MechPreData::ClearPreWidgetGeoPick(preWidget);
        MechPreData::ClearPreWidgetPickHighlightKeepData(preWidget);
        preWidget->updatePaint();
    }

    void mDiscreteDataControl::onViewportPickFinished()
    {
        if (!_picking || !_hasPick) return;
        auto* vm = getCVM;
        auto* preWidget = vm ? vm->getPreRenderWidget() : nullptr;
        if (!preWidget) return;

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> pickedData;
        SaveSelectMemoryData(preWidget, pickedData);

        // 视口当前选中集合即拾取结果（取消拾取的对象自然不在其中，对应行随之移除）
        QSet<QPair<int, int>> pickedKeys;
        QVector<QPair<int, int>> orderedKeys;
        for (const auto& pair : pickedData)
        {
            if (!_pickSpec.pickTypes.isEmpty() && !_pickSpec.pickTypes.contains(pair.first))
                continue;
            for (const auto& d : pair.second)
            {
                const QPair<int, int> key = _pickSpec.keyOf(d);
                if (key.first <= 0) continue;
                if (!pickedKeys.contains(key))
                {
                    pickedKeys.insert(key);
                    orderedKeys.append(key);
                }
            }
        }

        // 按当前拾取集合重建行：仍在拾取中的行保留已填分量；新对象追加空分量行。
        // 先建 key→行号索引，避免逐行线性查找（连续拾取大数据量时 O(N²) → O(N)）
        QHash<QPair<int, int>, int> rowIndexByKey;
        rowIndexByKey.reserve(_rows.size());
        for (int i = 0; i < _rows.size(); ++i)
            rowIndexByKey.insert(_rows.at(i).first, i);

        QVector<Row> next;
        next.reserve(orderedKeys.size());
        for (const auto& key : orderedKeys)
        {
            const auto it = rowIndexByKey.constFind(key);
            if (it != rowIndexByKey.constEnd())
                next.append(_rows.at(it.value()));
            else
            {
                QStringList emptyValues;
                for (int i = 0; i < _componentCount; ++i) emptyValues.append(QString());
                next.append(qMakePair(key, emptyValues));
            }
        }
        _rows = next;

        rebuildTable();
        updateEntrySummary();
        updateActionStates();
        // 拾取会话进行中不发 dataChanged：宿主槽内的渲染/符号重建属于重操作，
        // 每拾取一个对象就全量执行会打断拾取节奏；拾取确认（finishPicking）时
        // 统一补发一次完成渲染。
        if (!_picking)
        {
            emit dataChanged();
            // 此后不得再访问成员：dataChanged 槽内宿主可能重建面板并 delete 本控件
        }
    }

    int mDiscreteDataControl::findDuplicateIdRow(const QPair<int, int>& key, int exceptRow) const
    {
        // 比较已填的标识位：全部已填位相等（含单标识相等、另一列都未填）即视为重复。
        // 逐列整体比较会把新行占位 (-1) 与新值区分开，倒序补录编号时漏检重复。
        for (int i = 0; i < _rows.size(); ++i)
        {
            if (i == exceptRow) continue;
            const QPair<int, int>& other = _rows.at(i).first;
            const bool firstDup = (key.first > 0 && key.first == other.first)
                || (key.first <= 0 && other.first <= 0);
            const bool secondDup = (key.second > 0 && key.second == other.second)
                || (key.second <= 0 && other.second <= 0);
            if (firstDup && secondDup)
                return i;
        }
        return -1;
    }
}
