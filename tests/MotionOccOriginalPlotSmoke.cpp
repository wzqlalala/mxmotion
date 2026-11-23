#include "mCurveWindow.h"
#include "mPlotView.h"
#include "mPlotViewStyleWidget.h"
#include <QApplication>
#include <QMouseEvent>
#include <QTextStream>
#include <qwt_plot_curve.h>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
}
int runMotionOccOriginalPlotSmoke(QApplication& app)
{
    try {
        auto* window=MechMainWindow::mCurveWindow::getInstance();
        auto* view=window->getPlotView();
        window->resize(1000,480);window->show();app.processEvents();
        // The actual original CurveWindow path must preserve independent X
        // arrays: it may not resample a second curve against the first one.
        window->updateMultiCurves(QVector<QVector<double>>{{0,0.25,1},{0,0.5,2}},
            QVector<QVector<double>>{{1,4,2},{10,15,12}},PlotView::LineChart,{"A","B"},true,true);
        require(view->getCurveSamples("A")[1]==QPointF(0.25,4),"first curve samples changed");
        require(view->getCurveSamples("B")[1]==QPointF(0.5,15),"independent time series collapsed");
        auto style=view->getCurveSettings("A");style.lineColor=Qt::magenta;style.lineWidth=3;style.lineStyle=Qt::DashLine;
        view->setCurveStyle("A",style);
        window->updateCurve({0,1},{3,5},PlotView::LineChart,"A",true,true);
        require(view->getCurveSettings("A").lineColor==Qt::magenta&&view->getCurveSettings("A").lineWidth==3,
            "original clear/replot lost saved curve style");
        view->addMarkerLine("time",PlotView::mPlotMarkerLine::Vertical,0.25);
        view->getMarkerLine("time")->setValue(0.75);
        require(std::abs(view->getMarkerLine("time")->value()-0.75)<1e-12,"time marker did not move");
        const double nan=std::numeric_limits<double>::quiet_NaN();
        window->updateCurve({0,1,2,3,4},{0,1,nan,3,4},PlotView::LineChart,"gap",true,true);
        int segments=0;
        for(auto* item:view->getQwtPlot()->itemList(QwtPlotItem::Rtti_PlotCurve)) {
            auto* curve=static_cast<QwtPlotCurve*>(item);++segments;
            require(curve->dataSize()==2,"curve bridged an undefined interval");
        }
        require(segments==2,"finite runs were not separate Qwt curves");
        require(view->getMarkerLine("time")!=nullptr,"replot erased playback time marker");
        auto axis=view->getAxisSettings(PlotView::X_Bottom);axis.autoScale=false;axis.minimum=1;axis.maximum=3;axis.title="时间(s)";
        view->setAxisSettings(PlotView::X_Bottom,axis);view->rescaleAxes();
        const auto range=view->getAxisSettings(PlotView::X_Bottom);
        require(std::abs(range.minimum-1)<1e-10&&std::abs(range.maximum-3)<1e-10,"rescale replaced a manual range");
        view->removeMarkerLine("time");require(!view->getMarkerLine("time"),"time marker deletion failed");
        view->clearAll();require(view->getCurveNames().isEmpty(),"clear left stale series");
        QTextStream(stdout)<<"PASS original CurveWindow + Qwt: independent samples, styles, discontinuities, axes, playback marker\n";
        delete window;
        return 0;
    } catch(const std::exception& error) {QTextStream(stderr)<<error.what()<<'\n';return 1;}
}
#ifndef MX_OCC_DESKTOP_SMOKE
int main(int argc,char** argv)
{
    QApplication app(argc,argv);
    return runMotionOccOriginalPlotSmoke(app);
}
#endif
