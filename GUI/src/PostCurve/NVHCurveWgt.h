#ifndef NVHCURVEWGT_H
#define NVHCURVEWGT_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QHash>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QtMath>
#include <limits>
#include <QFontMetrics>
#include "mDrawingOptions.h"

// =============================================================================
// NVH 曲线控件（QPainter 自绘原型，历史遗留保留）
//
// 说明：这是 NVH 曲线功能的早期自绘实现（支持对数刻度/加权/平移缩放）。
// 曲线模块框架落地后，正式渲染已切换为 mCurveChartView（PlotView/Qwt 封装），
// pch 导入流程（mOpenCurveDlg）不再使用本控件。
// 保留意义：曲线配色轮换表、刻度/加权换算等逻辑的实现参考；
// 确认无引用后可下线。
// =============================================================================

namespace PostCurve
{

//单条曲线数据（自绘渲染用：原始值空间的点集 + 颜色 + 图例文本）
struct SeriesData {
    QVector<QPointF> points; // 存储【原始值】空间的数据
    QColor color;
    QString label;
};

//自绘曲线控件：paintEvent 绘制曲线/坐标轴/图例，鼠标/滚轮平移缩放
class NVHCurveWgt : public QWidget {
    Q_OBJECT
public:
    explicit NVHCurveWgt(QWidget* parent = nullptr);

    void addSeries(const QVector<QPointF>& points, const QColor& color, const QString& label, bool autoFit = false);
    void addSeries(const QVector<double>& xData, const QVector<double>& yData, const QColor& color, const QString& label, bool autoFit = false);
    void addSeries(const QHash<QString, QVector<double>>& xHash,
        const QHash<QString, QVector<double>>& yHash,
        bool autoFit = true);

    void setDrawingOptions(const DrawingOptionsStr& options) {
        m_drawingOptions = options;
        fitAll(); // 切换刻度后重新适配视野
    }

    void clearSeries();
    void fitAll();

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    // 视口范围：保存【原始值】空间的 min/max
    double m_xMin = -10.0, m_xMax = 10.0;
    double m_yMin = -10.0, m_yMax = 10.0;

    bool m_isPanning = false;
    QPoint m_lastMousePos;

    QVector<SeriesData> m_seriesList;
    DrawingOptionsStr m_drawingOptions;

    double m_marginLeft = 120.0;
    double m_marginRight = 5.0;
    double m_marginTop = 80.0;
    double m_marginBottom = 110.0;

    // 辅助
    double calculateNiceStep(double range) const;
    double transformValue(double val, const QString& scale) const;         // 原始值 -> 变换空间
    double inverseTransform(double tVal, const QString& scale) const;      // 变换空间 -> 原始值（用于刻度显示）
    double applyWeight(double originalValue, const QString& weight, double frequency) const; // 加权接口
    double mapX(double x) const; // 输入原始值，内部转换
    double mapY(double y) const;
    Qt::Alignment getAlign(const QString& align) const;
};

}

#endif // NVHCURVEWGT_H