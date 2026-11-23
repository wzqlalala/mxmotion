#pragma once
#include "mPlotEnum.h"
#include <QColor>
#include <QString>
#include <Qt>

namespace PlotView {
struct AxisSettings {
    QString title;
    bool isVisible=true,autoScale=true,isLog=false;
    double minimum=0,maximum=1,stepSize=0;
};
struct CurveSettings {
    QColor lineColor{33,118,193},symbolColor{33,118,193};
    double lineWidth=1.5,symbolSize=5;
    Qt::PenStyle lineStyle=Qt::SolidLine;
    bool isVisible=true,showSymbol=false;
};
struct PlotSettings {
    QString title;
    QColor backgroundColor{Qt::white};
    bool showLegend=true,showGrid=true;
};
}
