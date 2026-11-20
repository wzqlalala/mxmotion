//=============================================================================
// BarChartScaleDraw.h
// 柱状图 X 轴自定义刻度标签绘制（供主窗口各模块复用）
//=============================================================================
#ifndef BARCHART_SCALE_DRAW_H
#define BARCHART_SCALE_DRAW_H

#include <qwt_scale_draw.h>
#include <qwt_text.h>
#include <QStringList>
#include <QtMath>

class BarChartScaleDraw : public QwtScaleDraw
{
public:
    explicit BarChartScaleDraw(const QStringList& labels) : _labels(labels) {}

    QwtText label(double value) const override
    {
        int idx = qRound(value);
        if (idx >= 0 && idx < _labels.size())
            return _labels.at(idx);
        return QwtScaleDraw::label(value);
    }

private:
    QStringList _labels;
};

#endif // BARCHART_SCALE_DRAW_H
