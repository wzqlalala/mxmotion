#pragma once
#include "mPlotStructs.h"
#include <QMap>
#include <QPointer>
#include <QVector>
#include <QWidget>
#include <memory>
#include <map>
#include <array>
#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class QwtPlotZoomer;

namespace PlotView {
class mPlotViewStyleWidget;
class mPlotMarkerLine {
public:
    enum Direction { Vertical,Horizontal };
    mPlotMarkerLine(QwtPlot*,const QString&,Direction,double);
    ~mPlotMarkerLine();
    void setValue(double);
    double value() const {return value_;}
private:
    QwtPlot* plot_;
    QwtPlotMarker* marker_;
    Direction direction_;
    double value_=0;
};

// Missing PlotView SDK facade: the original mCurveWindow and Motion panels
// keep their source and public calls, while Qwt owns real curves and axes.
class mPlotView : public QWidget {
    Q_OBJECT
public:
    explicit mPlotView(QWidget* parent=nullptr);
    ~mPlotView() override;
    QwtPlot* getQwtPlot() const {return plot_;}
    void setPlotViewStyleWidget(mPlotViewStyleWidget*);
    mPlotViewStyleWidget* getPlotViewStyleWidget() const;
    void updateData(const QString&,const QVector<double>&,const QVector<double>&);
    void clearAll();
    QStringList getCurveNames() const;
    QVector<QPointF> getCurveSamples(const QString&) const;
    CurveSettings getCurveSettings(const QString&) const;
    void setCurveStyle(const QString&,const CurveSettings&);
    AxisSettings getAxisSettings(AxisId) const;
    void setAxisSettings(AxisId,const AxisSettings&);
    void updateAxisScale(AxisId,const AxisSettings&);
    void updateAxisRange(AxisId,const AxisSettings&);
    void updateAxisLog(AxisId,const AxisSettings&);
    PlotSettings getPlotSettings() const;
    void setPlotSettings(const PlotSettings&);
    void setPlotType(PlotType);
    void setLegendVisual(bool);
    void rescaleAxes();
    mPlotMarkerLine* getMarkerLine(const QString&) const;
    void addMarkerLine(const QString&,mPlotMarkerLine::Direction,double);
    void removeMarkerLine(const QString&);
signals:
    void signalTimeClicked(double time);
    void contentsChanged();
    void seriesUpdated();
private:
    struct CurveData {
        QVector<QPointF> samples;
        QVector<QwtPlotCurve*> segments;
        CurveSettings settings;
    };
    void applyCurveStyle(CurveData&);
    QwtPlot* plot_=nullptr;
    QwtPlotGrid* grid_=nullptr;
    QwtPlotZoomer* zoomer_=nullptr;
    QPointer<mPlotViewStyleWidget> styleWidget_;
    QMap<QString,CurveData> curves_;
    QStringList order_;
    std::map<QString,std::unique_ptr<mPlotMarkerLine>> markers_;
    std::array<AxisSettings,4> axes_;
    PlotSettings settings_;
    PlotType type_=LineChart;
};
}
