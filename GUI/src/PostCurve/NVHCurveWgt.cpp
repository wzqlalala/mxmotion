#include "NVHCurveWgt.h"

namespace PostCurve
{

NVHCurveWgt::NVHCurveWgt(QWidget* parent) : QWidget(parent) {
}

// 数据添加
void NVHCurveWgt::addSeries(const QVector<QPointF>& points, const QColor& color, const QString& label, bool autoFit) {
    SeriesData data;
    data.points = points; // 直接存原始点
    data.color = color;
    data.label = label;
    m_seriesList.append(data);
    if (autoFit) fitAll();
    else update();
}

void NVHCurveWgt::addSeries(const QVector<double>& xData, const QVector<double>& yData, const QColor& color, const QString& label, bool autoFit) {
    QVector<QPointF> points;
    int count = qMin(xData.size(), yData.size());
    points.reserve(count);
    for (int i = 0; i < count; ++i) {
        // 顺序：原始值 -> 加权 -> 存储（保持原始空间）
        double rawY = applyWeight(yData[i], m_drawingOptions.yWeight, xData[i]);
        points.append(QPointF(xData[i], rawY));
    }
    addSeries(points, color, label, autoFit);
}

void NVHCurveWgt::addSeries(const QHash<QString, QVector<double>>& xHash,
    const QHash<QString, QVector<double>>& yHash,
    bool autoFit) {
    QVector<QColor> colors = {
        QColor(220, 50, 50), QColor(230, 180, 0), QColor(0, 150, 70),
        QColor(130, 70, 180), QColor(255, 130, 0), QColor(40, 40, 40)
    };

    int index = 0;
    for (const QString& key : xHash.keys()) {
        if (yHash.contains(key)) {
            QColor color = colors[index % colors.size()];
            addSeries(xHash.value(key), yHash.value(key), color, key, false);
            index++;
        }
    }
    if (autoFit) fitAll();
    else update();
}

void NVHCurveWgt::clearSeries() {
    m_seriesList.clear();
    update();
}

//  变换 / 加权 
// 原始值 -> 变换空间
// 返回 -infinity 或者特殊值表示非法，绘图时会跳过
double NVHCurveWgt::transformValue(double val, const QString& scale) const {
    if (scale == "Logarithmic") {
        if (val <= 0) return std::numeric_limits<double>::quiet_NaN();
        return std::log10(val);
    }
    if (scale == "dB20") {
        if (val <= 0) return std::numeric_limits<double>::quiet_NaN();
        return 20.0 * std::log10(val);
    }
    if (scale == "dB10") {
        if (val <= 0) return std::numeric_limits<double>::quiet_NaN();
        return 10.0 * std::log10(val);
    }
    return val;
}

// 变换空间 -> 原始值（用于绘制刻度数字）
double NVHCurveWgt::inverseTransform(double tVal, const QString& scale) const {
    if (scale == "Logarithmic") return std::pow(10.0, tVal);
    if (scale == "dB20") return std::pow(10.0, tVal / 20.0);
    if (scale == "dB10") return std::pow(10.0, tVal / 10.0);
    return tVal;
}

// 加权接口（预留，暂不实现）
double NVHCurveWgt::applyWeight(double originalValue, const QString& weight, double frequency) const {
    Q_UNUSED(frequency);
    if (weight == "无" || weight.isEmpty()) return originalValue;
    // TODO: 这里将来用 A/B/C/U 计权公式修正 originalValue
    return originalValue;
}

double NVHCurveWgt::calculateNiceStep(double range) const {
    if (range <= 0 || qIsNaN(range)) return 1.0;
    double rawStep = range / 10.0;
    if (rawStep <= 0) return 1.0;
    double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
    double residual = rawStep / magnitude;
    double niceResidual = (residual < 1.5) ? 1 : (residual < 3.5) ? 2 : (residual < 7.5) ? 5 : 10;
    return niceResidual * magnitude;
}

// 像素映射 

// 输入原始值 x，返回像素。内部会做变换。
double NVHCurveWgt::mapX(double x) const {
    double tv = transformValue(x, m_drawingOptions.xScale);
    double tmin = transformValue(m_xMin, m_drawingOptions.xScale);
    double tmax = transformValue(m_xMax, m_drawingOptions.xScale);

    // 若变换后范围非法，退化为线性
    if (qIsNaN(tv) || qIsNaN(tmin) || qIsNaN(tmax) || qFuzzyCompare(tmax, tmin)) {
        tv = x; tmin = m_xMin; tmax = m_xMax;
    }
    double plotW = width() - m_marginLeft - m_marginRight;
    return m_marginLeft + (tv - tmin) * (plotW / (tmax - tmin));
}

double NVHCurveWgt::mapY(double y) const {
    double tv = transformValue(y, m_drawingOptions.yScale);
    double tmin = transformValue(m_yMin, m_drawingOptions.yScale);
    double tmax = transformValue(m_yMax, m_drawingOptions.yScale);

    if (qIsNaN(tv) || qIsNaN(tmin) || qIsNaN(tmax) || qFuzzyCompare(tmax, tmin)) {
        tv = y; tmin = m_yMin; tmax = m_yMax;
    }
    double plotH = height() - m_marginTop - m_marginBottom;
    return m_marginTop + (tmax - tv) * (plotH / (tmax - tmin));
}

Qt::Alignment NVHCurveWgt::getAlign(const QString& align) const {
    if (align == "左" || align == "Left") return Qt::AlignLeft;
    if (align == "右" || align == "Right") return Qt::AlignRight;
    return Qt::AlignHCenter;
}

// 自动适配 
// 关键：在【变换空间】里计算范围，然后再 inverseTransform 回原始值存储到 m_xMin/m_xMax
void NVHCurveWgt::fitAll() {
    if (m_seriesList.isEmpty()) {
        m_xMin = -10.0; m_xMax = 10.0;
        m_yMin = -10.0; m_yMax = 10.0;
        update();
        return;
    }

    double tMinX = std::numeric_limits<double>::max();
    double tMaxX = std::numeric_limits<double>::lowest();
    double tMinY = std::numeric_limits<double>::max();
    double tMaxY = std::numeric_limits<double>::lowest();

    for (const SeriesData& series : m_seriesList) {
        for (const QPointF& pt : series.points) {
            double tx = transformValue(pt.x(), m_drawingOptions.xScale);
            double ty = transformValue(pt.y(), m_drawingOptions.yScale);
            if (qIsNaN(tx) || qIsNaN(ty)) continue; // 跳过非法点
            tMinX = qMin(tMinX, tx); tMaxX = qMax(tMaxX, tx);
            tMinY = qMin(tMinY, ty); tMaxY = qMax(tMaxY, ty);
        }
    }

    if (tMinX > tMaxX) { tMinX = -1; tMaxX = 1; }
    if (tMinY > tMaxY) { tMinY = -1; tMaxY = 1; }

    double rangeX = tMaxX - tMinX;
    double rangeY = tMaxY - tMinY;
    if (rangeX < 1e-12) { tMinX -= 1; tMaxX += 1; rangeX = tMaxX - tMinX; }
    if (rangeY < 1e-12) { tMinY -= 1; tMaxY += 1; rangeY = tMaxY - tMinY; }

    double padX = rangeX * 0.05;
    double padY = rangeY * 0.1;

    // 把变换空间的边界反向变换回原始空间保存
    m_xMin = inverseTransform(tMinX - padX, m_drawingOptions.xScale);
    m_xMax = inverseTransform(tMaxX + padX, m_drawingOptions.xScale);
    m_yMin = inverseTransform(tMinY - padY, m_drawingOptions.yScale);
    m_yMax = inverseTransform(tMaxY + padY, m_drawingOptions.yScale);

    update();
}

// 交互 
void NVHCurveWgt::wheelEvent(QWheelEvent* event) {
    // 非 Linear 刻度禁用缩放
    if (m_drawingOptions.xScale != "Linear" || m_drawingOptions.yScale != "Linear") {
        return;
    }
    double factor = event->angleDelta().y() > 0 ? 0.9 : 1.1;
    QPointF mousePos = QPointF(event->pos());
    double dataX = m_xMin + (mousePos.x() / width()) * (m_xMax - m_xMin);
    double dataY = m_yMax - (mousePos.y() / height()) * (m_yMax - m_yMin);

    m_xMin = dataX - (dataX - m_xMin) * factor;
    m_xMax = dataX + (m_xMax - dataX) * factor;
    m_yMin = dataY - (dataY - m_yMin) * factor;
    m_yMax = dataY + (m_yMax - dataY) * factor;
    update();
}

void NVHCurveWgt::mousePressEvent(QMouseEvent* event) {
    // 非 Linear 刻度禁用缩放
    if (m_drawingOptions.xScale != "Linear" || m_drawingOptions.yScale != "Linear") {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
    }
}

void NVHCurveWgt::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        // 非 Linear 刻度禁用缩放
        if (m_drawingOptions.xScale != "Linear" || m_drawingOptions.yScale != "Linear") {
            return;
        }
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        double dx = (delta.x() / (double)width()) * (m_xMax - m_xMin);
        double dy = (delta.y() / (double)height()) * (m_yMax - m_yMin);
        m_xMin -= dx; m_xMax -= dx;
        m_yMin += dy; m_yMax += dy;
        update();
    }
}

void NVHCurveWgt::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) m_isPanning = false;
}

// 绘制

void NVHCurveWgt::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    int w = width(), h = height();
    double left = m_marginLeft;
    double top = m_marginTop;
    double plotW = w - m_marginLeft - m_marginRight;
    double plotH = h - m_marginTop - m_marginBottom;

    // 变换空间下的步长
    double tMinX = transformValue(m_xMin, m_drawingOptions.xScale);
    double tMaxX = transformValue(m_xMax, m_drawingOptions.xScale);
    double tMinY = transformValue(m_yMin, m_drawingOptions.yScale);
    double tMaxY = transformValue(m_yMax, m_drawingOptions.yScale);

    bool xIsLinear = (m_drawingOptions.xScale == "Linear");
    bool yIsLinear = (m_drawingOptions.yScale == "Linear");

    double stepX = xIsLinear ? calculateNiceStep(tMaxX - tMinX) : 1.0;
    double stepY = yIsLinear ? calculateNiceStep(tMaxY - tMinY) : 1.0;

    // 1. 网格
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    for (double v = std::ceil(tMinX / stepX) * stepX; v <= tMaxX; v += stepX) {
        double xPix = left + (v - tMinX) * (plotW / (tMaxX - tMinX));
        painter.drawLine(QPointF(xPix, top), QPointF(xPix, top + plotH));
    }
    for (double v = std::ceil(tMinY / stepY) * stepY; v <= tMaxY; v += stepY) {
        double yPix = top + (tMaxY - v) * (plotH / (tMaxY - tMinY));
        painter.drawLine(QPointF(left, yPix), QPointF(left + plotW, yPix));
    }

    // 2. 外框
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(QRectF(left, top, plotW, plotH));

    // 3. 绘制十字线（仅当 X、Y 都为 Linear 时才绘制）
    if (m_drawingOptions.xScale == "Linear" && m_drawingOptions.yScale == "Linear") {
        double crossX = mapX(0.0), crossY = mapY(0.0);
        if (qIsNaN(crossX)) crossX = left; if (qIsNaN(crossY)) crossY = top;
        crossX = qBound(left, crossX, left + plotW);
        crossY = qBound(top, crossY, top + plotH);
        painter.setPen(QPen(Qt::black, 2));
        painter.drawLine(QPointF(crossX, top), QPointF(crossX, top + plotH));
        painter.drawLine(QPointF(left, crossY), QPointF(left + plotW, crossY));
    }

    // 4. 刻度（X轴）
    QFont tickFont = painter.font();
    tickFont.setPointSize(9);
    tickFont.setBold(false);
    painter.setFont(tickFont);
    painter.setPen(Qt::black);

    for (double v = std::ceil(tMinX / stepX) * stepX; v <= tMaxX; v += stepX) {
        double xPix = left + (v - tMinX) * (plotW / (tMaxX - tMinX));
        // 反向变换回原始值显示
        QString txt = QString::number(v, 'g', 4); // 直接显示变换空间的值
        painter.drawText(QPointF(xPix - 12, top + plotH + 20), txt);
    }
    // Y轴刻度
    for (double v = std::ceil(tMinY / stepY) * stepY; v <= tMaxY; v += stepY) {
        double yPix = top + (tMaxY - v) * (plotH / (tMaxY - tMinY));
        QString txt = QString::number(v, 'g', 4);
        QRectF tickRect(left - 55, yPix - 8, 45, 16);
        painter.drawText(tickRect, Qt::AlignRight | Qt::AlignVCenter, txt);
    }

    // 5. 曲线（裁剪 + 样式 + 符号）
    painter.save();
    painter.setClipRect(QRectF(left, top, plotW, plotH));
    const DrawingOptionsStr& opt = m_drawingOptions;

    for (int i = 0; i < m_seriesList.size(); ++i) {
        const SeriesData& series = m_seriesList[i];
        if (series.points.isEmpty()) continue;

        QColor penColor = opt.selectLineColor ? QColor(opt.lineColor) : series.color;
        int penWidth = opt.selectLineWidthScheme ? opt.lineWidthScheme + 1 : 2;
        Qt::PenStyle penStyle = Qt::SolidLine;
        if (opt.selectLineStyleScheme) {
            if (opt.lineStyleScheme == 1) penStyle = Qt::DashLine;
            else if (opt.lineStyleScheme == 2) penStyle = Qt::DashDotLine;
            else if (opt.lineStyleScheme == 3) penStyle = Qt::DashDotLine;
        }
        painter.setPen(QPen(penColor, penWidth, penStyle));

        QPainterPath path;
        bool started = false;
        for (int j = 0; j < series.points.size(); ++j) {
            double px = mapX(series.points[j].x());
            double py = mapY(series.points[j].y());
            if (qIsNaN(px) || qIsNaN(py)) { started = false; continue; } // 断点
            if (!started) { path.moveTo(px, py); started = true; }
            else path.lineTo(px, py);
        }
        painter.drawPath(path);

        // 符号
        if (opt.selectSymbol && opt.symbol != 0) {
            painter.setPen(QPen(QColor(opt.symbolColor), 1));
            painter.setBrush(QColor(opt.symbolColor));
            double r = 3.0;
            for (const QPointF& pt : series.points) {
                double px = mapX(pt.x()), py = mapY(pt.y());
                if (qIsNaN(px) || qIsNaN(py)) continue;
                switch (opt.symbol) {
                case 1: painter.drawEllipse(QPointF(px, py), r, r); break;
                case 2: painter.drawRect(QRectF(px - r, py - r, r * 2, r * 2)); break;
                case 3: { QPolygonF p; p << QPointF(px, py - r) << QPointF(px + r, py) << QPointF(px, py + r) << QPointF(px - r, py); painter.drawPolygon(p); break; }
                case 4: { QPolygonF p; p << QPointF(px, py - r) << QPointF(px + r, py + r) << QPointF(px - r, py + r); painter.drawPolygon(p); break; }
                case 5: painter.drawLine(QPointF(px - r, py), QPointF(px + r, py)); painter.drawLine(QPointF(px, py - r), QPointF(px, py + r)); break;
                case 6: { QPolygonF p; p << QPointF(px, py + r) << QPointF(px + r, py - r) << QPointF(px - r, py - r); painter.drawPolygon(p); break; }
                case 7: { QPainterPath s; for (int k = 0; k < 10; ++k) { double a = k * M_PI / 5.0 - M_PI / 2.0; double rad = (k % 2 == 0) ? r : r * 0.4; if (k == 0) s.moveTo(px + rad * cos(a), py + rad * sin(a)); else s.lineTo(px + rad * cos(a), py + rad * sin(a)); } s.closeSubpath(); painter.drawPath(s); break; }
                case 8: painter.drawLine(QPointF(px - r, py - r), QPointF(px + r, py + r)); painter.drawLine(QPointF(px - r, py + r), QPointF(px + r, py - r)); break;
                case 9: painter.drawEllipse(QPointF(px, py), 1.0, 1.0); break;
                }
            }
            painter.setBrush(Qt::NoBrush);
        }
    }
    painter.restore();

    // 6. 图例
    if (!m_seriesList.isEmpty()) {
        QFont legendFont = painter.font();
        legendFont.setBold(true);
        painter.setFont(legendFont);
        QFontMetrics fm(legendFont);

        int maxLabelWidth = 0;
        int lineHeight = fm.height() + 8;
        for (const SeriesData& series : m_seriesList) {
            int textW = fm.horizontalAdvance(series.label);
            if (textW > maxLabelWidth) maxLabelWidth = textW;
        }
        int padding = 10, lineLength = 30, gap = 10;
        int legendWidth = padding + lineLength + gap + maxLabelWidth + padding;
        int legendHeight = m_seriesList.size() * lineHeight + padding * 2;
        int legendX = left + plotW - legendWidth - 10;
        int legendY = top + 10;

        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(QColor(255, 255, 255, 220));
        painter.drawRect(legendX, legendY, legendWidth, legendHeight);
        painter.setBrush(Qt::NoBrush);

        for (int i = 0; i < m_seriesList.size(); ++i) {
            int yCenter = legendY + padding + i * lineHeight + lineHeight / 2;
            QColor lc = m_drawingOptions.selectLineColor ? QColor(m_drawingOptions.lineColor) : m_seriesList[i].color;
            painter.setPen(QPen(lc, 2));
            painter.drawLine(legendX + padding, yCenter, legendX + padding + lineLength, yCenter);
            painter.setPen(lc);
            painter.drawText(QRect(legendX + padding + lineLength + gap,
                legendY + padding + i * lineHeight,
                maxLabelWidth, lineHeight),
                Qt::AlignVCenter | Qt::AlignLeft, m_seriesList[i].label);
        }
    }

    // 7. 轴标题 + 页眉页脚
    QString firstLabel = m_seriesList.isEmpty() ? QString() : m_seriesList.first().label;

    painter.setPen(Qt::black);
    painter.setFont(m_drawingOptions.xFont);
    painter.drawText(QRect(left, top + plotH + 30, plotW, 30), Qt::AlignHCenter, m_drawingOptions.xTitle);

    painter.save();
    painter.translate(left - 70, top + plotH / 2);
    painter.rotate(-90);
    painter.setFont(m_drawingOptions.yFont);
    painter.drawText(QRect(-plotH / 2, 0, plotH, 30), Qt::AlignHCenter, m_drawingOptions.yTitle);
    painter.restore();

    if (m_drawingOptions.header && !firstLabel.isEmpty()) {
        painter.setPen(Qt::black);
        painter.setFont(m_drawingOptions.headerFont);
        painter.drawText(QRect(0, 0, w, m_marginTop),
            getAlign(m_drawingOptions.headerAlign) | Qt::AlignVCenter, firstLabel);
    }
    if (m_drawingOptions.footer && !firstLabel.isEmpty()) {
        painter.setPen(Qt::black);
        painter.setFont(m_drawingOptions.footerFont);
        painter.drawText(QRect(0, h - (m_marginBottom * 0.8), w, m_marginBottom),
            getAlign(m_drawingOptions.footerAlign) | Qt::AlignVCenter, firstLabel);
    }
}

} // namespace PostCurve
