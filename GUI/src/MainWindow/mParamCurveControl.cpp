#include "mParamCurveControl.h"

#include <QComboBox>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPolygon>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTableView>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <cmath>
#include <limits>

namespace MechMainWindow
{
    namespace
    {
        // 曲线预览样式常量：对齐函数曲线窗口（mCurveWindow/mPlotView）的默认观感——
        // 首条曲线取 PlotColor[0]（红）、线宽 2 实线；轴/刻度/刻度值黑色，白底无网格
        const QColor kPlotCurveColor(QStringLiteral("#ff0000"));
        const QColor kPlotAxisColor(QStringLiteral("#000000"));

        // Qwt/TecPlot 的 loose labeling：刻度步长吸附到 1/2/5×10^n 的整刻度
        double plotNiceStep(double range, int intervals)
        {
            if (range <= 0.0 || intervals < 1) return 1.0;
            const double raw = range / intervals;
            if (raw <= 0.0) return 1.0;
            const double exponent = std::floor(std::log10(raw));
            const double fraction = raw / std::pow(10.0, exponent);
            double mantissa = 10.0;
            if (fraction < 1.5) mantissa = 1.0;
            else if (fraction < 3.0) mantissa = 2.0;
            else if (fraction < 7.0) mantissa = 5.0;
            return mantissa * std::pow(10.0, exponent);
        }

        // 生成主刻度序列，同时把 [min,max] 向外吸附到刻度网格（Qwt loose 自适应：
        // 轴端点落在整刻度上，数据不贴边）；刻度按整数序号还原，避免浮点累加误差
        QVector<double> plotLooseTicks(double& min, double& max, int maxTicks)
        {
            if (!(max > min)) max = min + 1.0;
            const double step = plotNiceStep(max - min, qMax(1, maxTicks - 1));
            int first = int(std::floor(min / step + 1e-9));
            int last = int(std::ceil(max / step - 1e-9));
            if (last <= first) last = first + 1;
            QVector<double> ticks;
            ticks.reserve(last - first + 1);
            for (int i = first; i <= last; ++i)
                ticks.append(i * step);
            min = first * step;
            max = last * step;
            return ticks;
        }

        // 视图范围内的主刻度序列（不改写视图边界）：滚轮缩放后视图自由浮动，
        // 刻度仍落在整刻度网格上（对齐 QwtScaleEngine::Floating 的观感）
        QVector<double> plotTicksWithin(double min, double max, int maxTicks)
        {
            if (!(max > min)) return QVector<double>();
            const double step = plotNiceStep(max - min, qMax(1, maxTicks - 1));
            const int first = int(std::floor(min / step + 1e-9));
            const int last = int(std::ceil(max / step - 1e-9));
            QVector<double> ticks;
            ticks.reserve(qMax(1, last - first + 1));
            for (int i = first; i <= last; ++i)
                ticks.append(i * step);
            return ticks;
        }

        // 刻度值文本：位数由步长决定（0.5 → 1 位小数），过大/过小退到科学计数法
        QString plotTickLabel(double v, double step)
        {
            if (v == 0.0) return QStringLiteral("0");
            if (qAbs(v) >= 1e6 || qAbs(v) < 0.001) return QString::number(v, 'e', 1);
            const int decimals = qBound(0, int(std::ceil(-std::log10(step) - 1e-9)), 3);
            return QString::number(v, 'f', decimals);
        }
    }

    // ------------------------------------------------------------------
    // 曲线预览画布：样式对齐函数曲线窗口——黑色坐标轴 + 外伸主/次刻度 +
    // 整刻度自适应范围 + 红色 2px 实线抗锯齿折线 + 右上角白底黑框图例，
    // 空数据时居中提示
    // ------------------------------------------------------------------
    void mAttachedPlotCanvas::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.fillRect(rect(), QColor(QStringLiteral("#ffffff")));

        if (_pts.size() < 2)
        {
            p.setPen(QColor(QStringLiteral("#788391")));
            p.drawText(rect(), Qt::AlignCenter,
                _hint.isEmpty() ? QStringLiteral("填写有效数据后显示曲线") : _hint);
            return;
        }

        // 视图范围：数据更新时由 resetView 建立整刻度全貌，滚轮缩放后自由浮动；
        // 刻度始终取整刻度网格
        if (!_viewValid)
            resetView();
        const double xmin = _viewXmin, xmax = _viewXmax;
        const double ymin = _viewYmin, ymax = _viewYmax;
        const QVector<double> xTicks = plotTicksWithin(xmin, xmax, 6);
        const QVector<double> yTicks = plotTicksWithin(ymin, ymax, 5);
        const double xStep = xTicks.value(1) - xTicks.value(0);
        const double yStep = yTicks.value(1) - yTicks.value(0);

        // 刻度字体（参考曲线窗口 Microsoft YaHei）；左边距按最宽刻度值自适应
        QFont tickFont(QStringLiteral("Microsoft YaHei"));
        tickFont.setPixelSize(12);
        const QFontMetrics fm(tickFont);
        int maxLabelW = 0;
        for (double t : yTicks)
            maxLabelW = qMax(maxLabelW, fm.horizontalAdvance(plotTickLabel(t, yStep)));

        // 参考轴设置：主刻度 8px、次刻度 4px，刻度值与轴线间隔 4px
        const int majorLen = 8, minorLen = 4, tickGap = 4;
        const int w = width(), h = height();
        const int left = 6 + majorLen + tickGap + maxLabelW;
        const int right = 16, top = 12;
        const int bottom = 6 + majorLen + tickGap + fm.height() + (_xTitle.isEmpty() ? 2 : 18);
        auto X = [&](double v) { return left + (v - xmin) / (xmax - xmin) * (w - left - right); };
        auto Y = [&](double v) { return h - bottom - (v - ymin) / (ymax - ymin) * (h - top - bottom); };
        const int axisX = left, axisY = h - bottom;

        // 坐标轴：黑色轴线 + 外伸刻度（先次刻度、再主刻度、最后轴线压顶）
        p.setFont(tickFont);
        p.setPen(QPen(kPlotAxisColor, 1));
        auto drawMinorX = [&](double t0, double t1) {
            for (int k = 1; k <= 3; ++k)
            {
                const double xx = X(t0 + (t1 - t0) * k / 4.0);
                p.drawLine(QPointF(xx, axisY), QPointF(xx, axisY + minorLen));
            }
        };
        auto drawMinorY = [&](double t0, double t1) {
            for (int k = 1; k <= 3; ++k)
            {
                const double yy = Y(t0 + (t1 - t0) * k / 4.0);
                p.drawLine(QPointF(axisX - minorLen, yy), QPointF(axisX, yy));
            }
        };
        for (int i = 0; i + 1 < xTicks.size(); ++i) drawMinorX(xTicks[i], xTicks[i + 1]);
        for (int i = 0; i + 1 < yTicks.size(); ++i) drawMinorY(yTicks[i], yTicks[i + 1]);
        for (double t : xTicks)
        {
            const double xx = X(t);
            p.drawLine(QPointF(xx, axisY), QPointF(xx, axisY + majorLen));
            p.drawText(QRectF(xx - 50, axisY + majorLen + tickGap, 100, fm.height()),
                Qt::AlignHCenter | Qt::AlignVCenter, plotTickLabel(t, xStep));
        }
        for (double t : yTicks)
        {
            const double yy = Y(t);
            p.drawLine(QPointF(axisX - majorLen, yy), QPointF(axisX, yy));
            p.drawText(QRectF(0, yy - fm.height() / 2.0, left - majorLen - tickGap, fm.height()),
                Qt::AlignRight | Qt::AlignVCenter, plotTickLabel(t, yStep));
        }
        p.drawLine(QPointF(axisX, top - 2), QPointF(axisX, axisY));
        p.drawLine(QPointF(axisX, axisY), QPointF(w - right, axisY));

        // 折线：红色 2px 实线、抗锯齿（无数据点符号，与函数曲线默认样式一致）；
        // 放大后超出视图的线段裁剪在绘图区内，不侵入轴边距与图例
        QPolygonF poly;
        poly.reserve(_pts.size());
        for (const QPointF& pt : _pts)
            poly << QPointF(X(pt.x()), Y(pt.y()));
        p.save();
        p.setClipRect(QRect(left, top, w - left - right, h - top - bottom));
        p.setPen(QPen(kPlotCurveColor, 2));
        p.drawPolyline(poly);
        p.restore();

        // x 轴标题
        if (!_xTitle.isEmpty())
        {
            p.setPen(QColor(QStringLiteral("#687383")));
            p.drawText(QRectF(left, axisY + majorLen + tickGap + fm.height(), w - left - right, 16),
                Qt::AlignHCenter | Qt::AlignVCenter, _xTitle);
        }

        // 图例：右上角白底半透明 + 1px 黑框 + 红色线样 + 曲线名（对齐曲线窗口默认图例）
        if (!_yTitle.isEmpty())
        {
            QFont legendFont(QStringLiteral("Microsoft YaHei"));
            legendFont.setPixelSize(12);
            const QFontMetrics lfm(legendFont);
            const int pad = 6, iconW = 26, gap = 6, margin = 8;
            const int textW = lfm.horizontalAdvance(_yTitle);
            const int boxW = pad * 2 + iconW + gap + textW;
            const int boxH = pad * 2 + lfm.height();
            const int bx = w - margin - boxW, by = margin;
            p.setPen(QPen(kPlotAxisColor, 1));
            p.setBrush(QColor(255, 255, 255, 200));
            p.drawRect(QRect(bx, by, boxW, boxH));
            p.setBrush(Qt::NoBrush);
            const int cy = by + boxH / 2;
            p.setPen(QPen(kPlotCurveColor, 2));
            p.drawLine(QPointF(bx + pad, cy), QPointF(bx + pad + iconW, cy));
            p.setPen(kPlotAxisColor);
            p.setFont(legendFont);
            p.drawText(QRectF(bx + pad + iconW + gap, by + pad, textW, lfm.height()),
                Qt::AlignLeft | Qt::AlignVCenter, _yTitle);
        }
    }

    // ------------------------------------------------------------------
    // 视图交互：滚轮缩放（对齐曲线窗口 QwtPlotMagnifier 的中心锚定观感，
    // 每格 1.25 步进），双击复位；数据更新时视图随 setData 复位
    // ------------------------------------------------------------------
    void mAttachedPlotCanvas::resetView()
    {
        _viewValid = false;
        if (_pts.size() < 2) return;
        double xmin = _pts[0].x(), xmax = _pts[0].x();
        double ymin = _pts[0].y(), ymax = _pts[0].y();
        for (const QPointF& pt : _pts)
        {
            xmin = qMin(xmin, pt.x()); xmax = qMax(xmax, pt.x());
            ymin = qMin(ymin, pt.y()); ymax = qMax(ymax, pt.y());
        }
        // 数据线为常值时先垫一层范围，再交给整刻度吸附
        if (xmax == xmin) { const double d = qAbs(xmax) * 0.05 + 0.05; xmin -= d; xmax += d; }
        if (ymax == ymin) { const double d = qAbs(ymax) * 0.05 + 0.05; ymin -= d; ymax += d; }
        // 借 plotLooseTicks 的吸附副作用：轴端点吸附到整刻度网格，全貌不贴边
        plotLooseTicks(xmin, xmax, 6);
        plotLooseTicks(ymin, ymax, 5);
        _viewXmin = xmin; _viewXmax = xmax; _viewYmin = ymin; _viewYmax = ymax;
        _baseXSpan = qMax(xmax - xmin, 1e-12);
        _baseYSpan = qMax(ymax - ymin, 1e-12);
        _viewValid = true;
    }

    void mAttachedPlotCanvas::zoomView(double factor)
    {
        if (!_viewValid || !(factor > 0.0)) return;
        const double cx = (_viewXmin + _viewXmax) / 2.0;
        const double cy = (_viewYmin + _viewYmax) / 2.0;
        // 缩放极限：视图跨度不小于基准的 1e-6（防塌缩）、不大于 1e4 倍（防飘走）
        const double spanX = qBound(_baseXSpan * 1e-6, (_viewXmax - _viewXmin) * factor, _baseXSpan * 1e4);
        const double spanY = qBound(_baseYSpan * 1e-6, (_viewYmax - _viewYmin) * factor, _baseYSpan * 1e4);
        _viewXmin = cx - spanX / 2.0; _viewXmax = cx + spanX / 2.0;
        _viewYmin = cy - spanY / 2.0; _viewYmax = cy + spanY / 2.0;
        update();
    }

    void mAttachedPlotCanvas::wheelEvent(QWheelEvent* event)
    {
        const int delta = event->angleDelta().y();
        if (!_viewValid || delta == 0)
        {
            QWidget::wheelEvent(event);
            return;
        }
        zoomView(delta > 0 ? 1.0 / 1.25 : 1.25);
        event->accept();
    }

    void mAttachedPlotCanvas::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && _viewValid)
        {
            resetView();
            update();
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

    // ------------------------------------------------------------------
    // 行表模型实现：数据常驻宿主控件 _rows，模型只暴露访问接口（不复制）
    // ------------------------------------------------------------------
    QVariant mAttachedTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QAbstractTableModel::headerData(section, orientation, role);
        if (_headers && section < _headers->size())
            return _headers->at(section);
        return QVariant(); // 操作列无表头
    }

    QVariant mAttachedTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || !_rows) return QVariant();
        const int r = index.row();
        const int c = index.column();

        // 空态占位行
        if (isEmptyPlaceholderRow(r))
        {
            if (role == Qt::DisplayRole && c == 0)
                return _emptyHint ? *_emptyHint : QStringLiteral("尚未添加数据");
            if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);
            if (role == Qt::ForegroundRole) return QColor(QStringLiteral("#788391"));
            return QVariant();
        }
        if (r >= _rows->size()) return QVariant();
        if (role == Qt::TextAlignmentRole) return int(Qt::AlignCenter);

        // 删除列由自绘委托负责；此处只给悬停标记，不给显示文本
        if (isOpColumn(c))
        {
            if (role == Qt::UserRole)
                return _hoverRow == r;
            return QVariant();
        }

        if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
        const QStringList& cells = _rows->at(r);
        return (c >= 0 && c < cells.size()) ? cells.at(c) : QString();
    }

    bool mAttachedTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || !index.isValid() || !_rows) return false;
        if (isEmptyPlaceholderRow(index.row()) || index.row() >= _rows->size()) return false;
        if (isOpColumn(index.column())) return false;
        // 数据写回由控件层（cellEditedByUser 监听者）统一处理，模型不直接改数据
        emit cellEditedByUser(index.row(), index.column(), value.toString().trimmed());
        return true;
    }

    Qt::ItemFlags mAttachedTableModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid()) return Qt::NoItemFlags;
        if (isEmptyPlaceholderRow(index.row())) return Qt::NoItemFlags;
        // 删除列：可选中（供委托接收点击），不可编辑
        if (isOpColumn(index.column())) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    void mAttachedTableModel::setHoverRow(int row)
    {
        if (_hoverRow == row) return;
        const int oldRow = _hoverRow;
        _hoverRow = row;
        const int opCol = columnCount() - 1;
        if (oldRow >= 0) emit dataChanged(index(oldRow, opCol), index(oldRow, opCol));
        if (row >= 0) emit dataChanged(index(row, opCol), index(row, opCol));
    }

    // ------------------------------------------------------------------
    // 删除列自绘委托：绘制垃圾桶图标 + 处理点击
    // ------------------------------------------------------------------
    void mAttachedDeleteDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const
    {
        auto* rowModel = qobject_cast<const mAttachedTableModel*>(index.model());
        if (!rowModel || !rowModel->isOpColumn(index.column()) || rowModel->isEmptyPlaceholderRow(index.row()))
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyledItemDelegate::paint(painter, option, index);

        const bool hovered = index.data(Qt::UserRole).toBool();
        if (hovered)
            painter->fillRect(option.rect, QColor(QStringLiteral("#fff5f5")));

        painter->setPen(hovered ? QColor(QStringLiteral("#d95151")) : QColor(QStringLiteral("#788391")));
        QFont f = painter->font();
        f.setPixelSize(13);
        painter->setFont(f);
        painter->drawText(option.rect, Qt::AlignCenter, QStringLiteral("🗑"));
    }

    bool mAttachedDeleteDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
        const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        Q_UNUSED(option)
        if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            auto* rowModel = qobject_cast<const mAttachedTableModel*>(index.model());
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
        // 2.2.6 吸附式弹窗（标准档）D1 数据表格关键尺寸
        constexpr int kRowHeight = 30;        // 可编辑数据行高
        constexpr int kHeaderHeight = 26;     // 表头/单行控件高
        constexpr int kActionsHeight = 26;    // 表格末行操作行高
        constexpr int kColWidth = 100;        // 数据列最小宽（每列 100px，10 位有效数字）
        constexpr int kOpColWidth = 38;       // 行操作列固定宽
        constexpr int kPopupMargin = 12;      // 数据区左右边距
        constexpr int kMaxTableHeight = 360;  // 表格区域超过该高度后内部滚动
        constexpr int kEmptyRowHeight = 120;  // 空态占位行高：提示文案垂直居中
        // 曲线预览展开时：轴选择行 24 + 画布 178 + 边距/间距 22
        constexpr int kPlotHeight = 224;

        // D1 表格内容样式：叠加在基类弹窗公共样式之上
        const QString kContentStyle = QStringLiteral(R"(
            /* 2.2.6 吸附式弹窗：末行操作行采用 panel 底色 #f5f7fa，与表格连续共边、无圆角无外框 */
            QWidget#tableBox { border: none; background: #ffffff; }
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
            QWidget#actionsBar {
                background: #f5f7fa;
                border: 1px solid #e4e8ee;
                border-top: none;
            }
            QPushButton#tableAction {
                background: transparent; border: none;
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

    mParamCurveControl::mParamCurveControl(const QString& title,
        const QString& independentHeader,
        const QStringList& paramHeaders,
        QWidget* parent)
        : mAttachedPopupBase(title, parent)
        , _emptyHint(QStringLiteral("尚未添加数据"))
    {
        _headers << independentHeader;
        _headers << paramHeaders;
        // 绘制曲线为该形态的固定能力（2.2.6 D1/D4：底部工具栏含绘制曲线）
        setActionButtonVisible(true, QStringLiteral("绘制曲线"), true);
        setActionHandler([this]() { togglePlotPreview(); });
    }

    mParamCurveControl::~mParamCurveControl()
    {
        // 弹窗子树随基类析构 delete _popup 级联销毁；表格视口上的过滤器须先摘除，
        // 否则级联销毁期间视口 Leave/Close 事件会回调进正在析构的 eventFilter
        if (_popup)
        {
            if (_table && _table->viewport())
                _table->viewport()->removeEventFilter(this);
        }
    }

    void mParamCurveControl::detachEntryButton(QWidget* newParent)
    {
        detachEntryWidget(newParent);
    }

    void mParamCurveControl::setImportHandler(const std::function<void(const ImportParams&)>& handler)
    {
        mAttachedPopupBase::setImportHandler(handler);
    }

    void mParamCurveControl::setExportHandler(const std::function<void(const QString&)>& handler)
    {
        mAttachedPopupBase::setExportHandler(handler);
    }

    // ------------------------------------------------------------------
    // 内容构建（mAttachedPopupBase 钩子：弹窗首次创建时调用，派生类已构造完成）
    // ------------------------------------------------------------------
    void mParamCurveControl::buildContent(QWidget* body, QVBoxLayout* layout)
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
        _tableModel = new mAttachedTableModel(_table);
        _tableModel->bind(&_rows, &_headers, &_emptyHint);
        _table->setModel(_tableModel);
        _table->verticalHeader()->setVisible(false);
        _table->verticalHeader()->setDefaultSectionSize(kRowHeight);
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
        {
            QFont tableFont = _table->font();
            tableFont.setFamily(QStringLiteral("Consolas"));
            _table->setFont(tableFont);
        }
        connect(_table->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &mParamCurveControl::syncActionCellWidths);

        _deleteDelegate = new mAttachedDeleteDelegate(_table);
        _table->setMouseTracking(true);
        connect(_deleteDelegate, &mAttachedDeleteDelegate::deleteRowRequested,
            this, &mParamCurveControl::removeRowAt);
        connect(_table, &QTableView::entered, this, [this](const QModelIndex& idx) {
            if (idx.isValid() && _tableModel->isOpColumn(idx.column()))
                _tableModel->setHoverRow(idx.row());
            else
                _tableModel->setHoverRow(-1);
            });
        _table->viewport()->installEventFilter(this);
        boxLayout->addWidget(_table);

        // 底部操作行（表格末行单元格，2.2.6 数据区）：纯参数表不设拾取格，
        // ＋合并数据列（拉伸）、清空格对齐右侧删除列；与表格连续共边、无圆角
        QWidget* actionsBar = new QWidget(tableBox);
        actionsBar->setObjectName(QStringLiteral("actionsBar"));
        actionsBar->setFixedHeight(kActionsHeight);
        QHBoxLayout* actionsLayout = new QHBoxLayout(actionsBar);
        actionsLayout->setContentsMargins(0, 0, 0, 0);
        actionsLayout->setSpacing(0);
        _addRowBtn = makeContentButton(QStringLiteral("＋"), QStringLiteral("添加数据行"), QStringLiteral("tableAction"));
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
        connect(_clearBtn, &QPushButton::clicked, this, [this]() { clearRows(); });

        // ---- 曲线预览区（绘制曲线按钮展开，默认隐藏）。
        // 与导入/导出文件面板同规格：独立蓝色描边面板，左右与数据区边距对齐，
        // 蓝框整体包住轴选择行和画布
        _plotPreview = new QWidget(body);
        _plotPreview->setObjectName(QStringLiteral("plotPreview"));
        _plotPreview->setStyleSheet(QStringLiteral(
            "QWidget#plotPreview { background: #ffffff; border: 1px solid #91caff;"
            " border-top: 1px solid #e4e8ee; }"));
        QVBoxLayout* previewLayout = new QVBoxLayout(_plotPreview);
        previewLayout->setContentsMargins(kPopupMargin, 8, kPopupMargin, 8);
        previewLayout->setSpacing(6);
        previewLayout->addLayout(buildPlotAxisRow());
        mAttachedPlotCanvas* canvas = new mAttachedPlotCanvas(_plotPreview);
        canvas->setFixedHeight(178);
        _plotChart = canvas;
        previewLayout->addWidget(canvas);
        _plotPreview->setVisible(false);
        layout->addWidget(_plotPreview);

        // 单元格编辑：模型 setData 时同步行数据并发出信号（业务逻辑在控件层处理）
        connect(_tableModel, &mAttachedTableModel::cellEditedByUser, this,
            [this](int row, int col, const QString& text) {
                if (row < 0 || row >= _rows.size()) return;
                QStringList& cells = _rows[row];
                if (col >= 0 && col < cells.size() && cells[col] != text)
                {
                    cells[col] = text;
                    onCellEdited(row, col, text);
                }
            });

        // 隐藏期把行数据/空态一次性灌入模型，首开不再二次 reset
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
        // 数据任何变化（编辑/增删/清空/导入/外部 setRows）后，预览展开则联动重绘
        connect(this, &mParamCurveControl::dataChanged,
            this, &mParamCurveControl::refreshPlotPreview);
    }

    QSize mParamCurveControl::contentSizeHint() const
    {
        // 2.2.6 宽度规则：W = max(480, 数据列数×100 + 38 操作列 + 框架)，
        // 视口约束与最小宽度下限由基类统一处理
        int tableWidth = _headers.size() * kColWidth + kOpColWidth + 2; // +2 表格边框

        const bool empty = _rows.isEmpty();
        const int rowCountN = qMax(_rows.size(), 1); // 空态保留一行提示
        const int rowsHeight = empty ? kEmptyRowHeight : rowCountN * kRowHeight;
        int tableHeight = kHeaderHeight + rowsHeight + 2;
        const bool overflow = tableHeight > kMaxTableHeight;
        tableHeight = qMin(tableHeight, kMaxTableHeight);
        // 只为实际纵向滚动条补偿（无溢出不预留空白槽）
        if (overflow)
            tableWidth += style()->pixelMetric(QStyle::PM_ScrollBarExtent);

        const int plotHeight = _plotPreview && _plotPreview->isVisible() ? kPlotHeight : 0;
        // 数据区左右边距 + 上下边距 + 末行操作行
        return QSize(tableWidth + 2 * kPopupMargin,
            10 + tableHeight + kActionsHeight + 10 + plotHeight);
    }

    void mParamCurveControl::onFilePanelToggled(bool /*importPanel*/, bool open)
    {
        // 绘图与导入、导出互斥（2.2.6 D5）：文件区展开时收起绘图区
        if (open && _plotPreview && _plotPreview->isVisible())
            togglePlotPreview();
    }

    void mParamCurveControl::onPopupResized()
    {
        mAttachedPopupBase::onPopupResized();
        // 弹窗 Resize 时同步末行操作格列宽（Stretch 列宽随动后清空格需重新对齐）
        syncActionCellWidths();
    }

    // ------------------------------------------------------------------
    // 事件过滤：表格视口悬停高亮收尾（弹窗级过滤由基类承担）
    // ------------------------------------------------------------------
    bool mParamCurveControl::eventFilter(QObject* watched, QEvent* event)
    {
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
    void mParamCurveControl::onCellEdited(int row, int col, const QString& text)
    {
        Q_UNUSED(row) Q_UNUSED(col) Q_UNUSED(text)
        emit dataChanged();
    }

    void mParamCurveControl::rebuildTable()
    {
        // 弹窗未建（setRows 在首次打开前调用）只保留数据，打开时一次性刷新
        if (!_popup || !_tableModel || !_table) return;

        const int totalCols = qMax(1, _headers.size() + 1);

        _tableModel->notifyReset();

        // 列宽/拉伸模式：数据列 Stretch 均分，删除列固定 38px。
        // 删除列委托必须在 reset 之后绑定：QHeaderView 会在模型 reset 时
        // 摘除绑定到旧列索引的列委托
        for (int c = 0; c < totalCols; ++c)
        {
            if (c == totalCols - 1)
            {
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
                _table->setColumnWidth(c, kOpColWidth);
            }
            else
            {
                _table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
            }
        }
        _table->setItemDelegateForColumn(totalCols - 1, _deleteDelegate);
        _tableModel->setHoverRow(-1);

        // 空态占位行：合并所有列为一整行（必须在 notifyReset 之后，reset 会清空 span 表）
        if (_rows.isEmpty())
            _table->setSpan(0, 0, 1, totalCols);
        else
            _table->clearSpans();

        _table->verticalHeader()->setDefaultSectionSize(_rows.isEmpty() ? kEmptyRowHeight : kRowHeight);
        syncActionCellWidths();
        repositionPopup();
    }

    void mParamCurveControl::syncActionCellWidths()
    {
        if (!_table || !_clearBtn) return;
        QHeaderView* header = _table->horizontalHeader();
        // 末行操作格与表格列对齐（2.2.6 数据区）：清空格对齐删除列、＋合并数据列（拉伸）
        const int opCol = header->count() - 1;
        int clearWidth = kOpColWidth;
        if (opCol >= 0)
            clearWidth = header->sectionSize(opCol);
        _clearBtn->setFixedWidth(qMax(clearWidth, 0));
    }

    void mParamCurveControl::removeRowAt(int row)
    {
        if (row < 0 || row >= _rows.size()) return;
        // 行删除按 begin → 改数据 → end 三段式：endRemoveRows 内视图会立即
        // 回查 rowCount()/data()，数据必须先于 end 同步
        const bool notify = _popup && _popup->isVisible() && _tableModel;
        if (notify) _tableModel->beginRemoveRow(row);
        _rows.removeAt(row);
        if (notify) _tableModel->endRemoveRow();
        // 删到空表时占位行需要一次整体重置才能出现（begin/endRemoveRows 只表达 1→0，
        // 占位行 0→1 视图无从得知）
        if (notify && _rows.isEmpty())
        {
            _tableModel->notifyReset();
            _table->setSpan(0, 0, 1, _tableModel->columnCount());
            _table->verticalHeader()->setDefaultSectionSize(kEmptyRowHeight);
        }
        repositionPopup();
        updateEntrySummary();
        updateActionStates();
        emit dataChanged();
    }

    void mParamCurveControl::appendRow(bool focus)
    {
        QStringList row;
        for (int i = 0; i < _headers.size(); ++i) row.append(QString());
        const bool notify = _popup && _popup->isVisible() && _tableModel;
        // 空态 → 首行：rowCount 语义 1(占位)→1(数据)，增量 insert 会残留占位行，
        // 必须走 rebuildTable 整体重置
        const bool replacingPlaceholder = notify && _rows.isEmpty();
        if (notify && !replacingPlaceholder) _tableModel->beginInsertRow(_rows.size());
        _rows.append(row);
        if (notify && !replacingPlaceholder) _tableModel->endInsertRow();
        if (replacingPlaceholder)
            rebuildTable();
        repositionPopup();
        updateEntrySummary();
        updateActionStates();
        emit dataChanged();
        if (focus && _tableModel)
        {
            const QModelIndex idx = _tableModel->index(_rows.size() - 1, 0);
            if (idx.isValid())
            {
                _table->scrollTo(idx);
                _table->setCurrentIndex(idx);
                _table->edit(idx);
            }
        }
    }

    void mParamCurveControl::clearRows()
    {
        if (_rows.isEmpty()) return;
        _rows.clear();
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
        emit dataChanged();
    }

    void mParamCurveControl::setRows(const QVector<QStringList>& rows)
    {
        _rows = rows;
        for (QStringList& row : _rows)
        {
            while (row.size() > _headers.size()) row.removeLast();
            while (row.size() < _headers.size()) row.append(QString());
        }
        rebuildTable();
        updateEntrySummary();
        updateActionStates();
    }

    void mParamCurveControl::updateEntrySummary()
    {
        // 入口显示「标题 · N 行」摘要；行数同步到弹窗标题栏
        if (QLabel* summary = entrySummaryLabel())
        {
            summary->setText(_rows.isEmpty()
                ? _title
                : QStringLiteral("%1 · %2 行").arg(_title).arg(_rows.size()));
        }
        setMetaText(QStringLiteral("%1 行").arg(_rows.size()));
    }

    void mParamCurveControl::updateActionStates()
    {
        const bool hasRows = !_rows.isEmpty();
        if (_clearBtn) _clearBtn->setEnabled(hasRows);
        if (_exportBtn) _exportBtn->setEnabled(hasRows);
        setActionButtonEnabled(hasRows);
    }

    // ------------------------------------------------------------------
    // 曲线数据接口（列主序：第一列自变量，其后每列一个参数）
    // ------------------------------------------------------------------
    void mParamCurveControl::setColumnHeaders(const QString& independentHeader, const QStringList& paramHeaders)
    {
        QStringList headers;
        headers << independentHeader;
        headers << paramHeaders;
        if (headers == _headers)
            return;
        _headers = headers;
        for (QStringList& row : _rows)
        {
            while (row.size() > _headers.size()) row.removeLast();
            while (row.size() < _headers.size()) row.append(QString());
        }
        rebuildTable();
        // 表头变更后预览轴选择行的列数随之变化，下次展开时按新列数重建
        invalidatePlotAxisRow();
    }

    void mParamCurveControl::setCurve(const QVector<QVector<double>>& curves)
    {
        QVector<QStringList> rows;
        if (!curves.isEmpty())
        {
            const int cols = _headers.size();
            // 行数取最长的列；其余列短缺的单元格留空（与表格「空 = 未输入」语义一致）
            int maxLen = 0;
            for (const auto& col : curves)
                maxLen = qMax(maxLen, col.size());
            rows.reserve(maxLen);
            for (int r = 0; r < maxLen; ++r)
            {
                QStringList row;
                row.reserve(cols);
                for (int c = 0; c < cols; ++c)
                {
                    if (c < curves.size() && r < curves[c].size())
                        row.append(QString::number(curves[c][r], 'g', 12));
                    else
                        row.append(QString());
                }
                rows.append(row);
            }
        }
        setRows(rows);
    }

    QVector<QVector<double>> mParamCurveControl::curve() const
    {
        const int cols = _headers.size();
        QVector<QVector<double>> curves(cols);
        for (const QStringList& row : _rows)
        {
            // 自变量必填且合法；参数列逐格解析，空/非法以 NaN 占位（宿主按有效点规则过滤）
            bool ok = false;
            const double x = row.value(0).toDouble(&ok);
            if (!ok) continue;
            curves[0].append(x);
            for (int c = 1; c < cols; ++c)
            {
                bool okV = false;
                const double v = row.value(c).toDouble(&okV);
                curves[c].append(okV ? v : std::numeric_limits<double>::quiet_NaN());
            }
        }
        return curves;
    }

    void mParamCurveControl::clearCurve()
    {
        if (_rows.isEmpty()) return;
        clearRows();
    }

    // ------------------------------------------------------------------
    // 曲线预览：x 恒为第 0 列（自变量），y 为所选参数列
    // ------------------------------------------------------------------
    QString mParamCurveControl::plotColumnName(int col) const
    {
        return (col >= 0 && col < _headers.size()) ? _headers.at(col) : QString();
    }

    int mParamCurveControl::plotSelectedColumn() const
    {
        if (_plotVarCombo && _plotVarCombo->isVisible() && _plotVarCombo->currentIndex() >= 0)
            return _plotVarCombo->currentData().toInt();
        return 1;
    }

    QString mParamCurveControl::plotValidate(int xCol, int yCol) const
    {
        Q_UNUSED(xCol);
        Q_UNUSED(yCol);
        if (plotSeriesData(0, qMax(1, yCol)).size() < 2)
            return QStringLiteral("请至少填写 2 个有效数据点。");
        return QString();
    }

    QVector<QPointF> mParamCurveControl::plotSeriesData(int xCol, int yCol) const
    {
        QVector<QPointF> pts;
        pts.reserve(_rows.size());
        for (const QStringList& row : _rows)
        {
            bool okX = false, okY = false;
            const double x = row.value(xCol).toDouble(&okX);
            const double y = row.value(yCol).toDouble(&okY);
            if (okX && okY)
                pts.append(QPointF(x, y));
        }
        return pts;
    }

    QHBoxLayout* mParamCurveControl::buildPlotAxisRow()
    {
        QHBoxLayout* headRow = new QHBoxLayout();
        headRow->setSpacing(6);
        // 轴选择行：X轴 [自变量(固定)]  Y轴 [参数下拉]
        QLabel* xLabel = new QLabel(QStringLiteral("X轴"), _plotPreview);
        xLabel->setStyleSheet(QStringLiteral("color: #5c6674; font-size: 11px; background: transparent;"));
        QComboBox* xCombo = new QComboBox(_plotPreview);
        xCombo->setFixedHeight(24);
        xCombo->addItem(plotColumnName(0), 0);
        xCombo->setEnabled(false); // x 恒为第 0 列，仅展示
        const QString axisComboStyle = QStringLiteral(
            "QComboBox { border: 1px solid #dce2e9; border-radius: 2px; background: #ffffff;"
            " color: #424b57; font-size: 11px; padding: 0 6px; }"
            "QComboBox:disabled { color: #788391; background: #f5f7fa; }");
        xCombo->setStyleSheet(axisComboStyle);
        QLabel* yLabel = new QLabel(QStringLiteral("Y轴"), _plotPreview);
        yLabel->setStyleSheet(QStringLiteral("color: #5c6674; font-size: 11px; background: transparent;"));
        // 参数列多于 1 时可切换；只有 1 列时禁用展示
        _plotVarCombo = new QComboBox(_plotPreview);
        _plotVarCombo->setFixedHeight(24);
        _plotVarCombo->setStyleSheet(axisComboStyle);
        _plotVarCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        const int yCols = _headers.size() - 1;
        for (int c = 1; c <= yCols; ++c)
            _plotVarCombo->addItem(plotColumnName(c), c);
        _plotVarCombo->setEnabled(yCols > 1);
        xCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        headRow->addWidget(xLabel);
        headRow->addWidget(xCombo, 1);
        headRow->addSpacing(8);
        headRow->addWidget(yLabel);
        headRow->addWidget(_plotVarCombo, 1);
        connect(_plotVarCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { refreshPlotPreview(); });
        return headRow;
    }

    void mParamCurveControl::rebuildPlotAxisRow()
    {
        if (!_plotPreview || !_plotPreview->layout()) return;
        auto* lay = qobject_cast<QVBoxLayout*>(_plotPreview->layout());
        if (!lay || lay->count() < 1) return;
        QLayoutItem* old = lay->takeAt(0);
        if (!old) return;

        // 布局壳与子控件必须整体一次性释放，不能「先逐个 delete 子控件再 delete 布局」：
        // 那会导致 QLayout 析构时再次级联删除同一批子控件（双击释放，首开即崩）。
        // 先显式解除子控件与旧布局的父子关系，让 deleteLater 只清理布局壳；
        // 子控件统一由 _plotPreview 作为父对象托管，随弹窗销毁时一起释放。
        if (QLayout* oldLay = old->layout())
        {
            while (QLayoutItem* it = oldLay->takeAt(0))
            {
                if (QWidget* w = it->widget())
                {
                    w->setParent(_plotPreview); // 托管给预览容器，不随布局壳析构
                    // 脱离布局后必须立即隐藏：控件不再被布局管理，会以旧几何
                    // 残留显示（展开时堆在预览区左上角，表现为多出一个 Y 轴下拉框）
                    w->hide();
                }
                delete it;
            }
            oldLay->deleteLater();
        }
        delete old;
        _plotVarCombo = nullptr; // 旧下拉已随上行解除父子关系，仅清空指针
        lay->insertLayout(0, buildPlotAxisRow());
    }

    void mParamCurveControl::togglePlotPreview()
    {
        if (!_plotPreview) return;
        const bool open = !_plotPreview->isVisible();
        if (open)
        {
            // 首次展开前按实际列数重建轴选择行（此前为基类兜底值）
            if (!_axisRowBuilt)
            {
                rebuildPlotAxisRow();
                _axisRowBuilt = true;
            }
            const int yCol = plotSelectedColumn();
            const QString err = plotValidate(0, yCol);
            if (!err.isEmpty())
            {
                showInlineMessage(err, true);
                setActionButtonChecked(false);
                return;
            }
            showInlineMessage(QString(), false);
            // 绘图与导入、导出互斥（2.2.6 D5）：展开绘图时收起文件区
            closeFilePanels();
        }
        _plotPreview->setVisible(open);
        setActionButtonChecked(open);
        if (open)
            refreshPlotPreview();
        repositionPopup();
    }

    void mParamCurveControl::refreshPlotPreview()
    {
        if (!_plotPreview || !_plotPreview->isVisible()) return;
        // _plotChart 只由 buildContent 创建为 mAttachedPlotCanvas，静态转换即可
        auto* canvas = static_cast<mAttachedPlotCanvas*>(_plotChart.data());
        if (!canvas) return;
        const int yCol = plotSelectedColumn();
        const QString err = plotValidate(0, yCol);
        if (!err.isEmpty())
        {
            canvas->setData({}, QString(), QString());
            canvas->setHint(err);
            return;
        }
        canvas->setHint(QString());
        canvas->setData(plotSeriesData(0, yCol), plotColumnName(0), plotColumnName(yCol));
    }
}
