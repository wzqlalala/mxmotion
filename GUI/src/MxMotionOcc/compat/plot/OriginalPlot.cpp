#include "mPlotView.h"
#include "mPlotViewStyleWidget.h"
#include "mPlotBasic.h"
#include "UiPalette.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <qwt_legend.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_div.h>
#include <qwt_symbol.h>
#include <algorithm>
#include <cmath>

namespace PlotView {
namespace {
class OriginalLegend final:public QwtLegend {
protected:
    void updateWidget(QWidget* widget,const QwtLegendData& data) override {
        const bool disabled=widget->testAttribute(Qt::WA_ForceUpdatesDisabled);
        QwtLegend::updateWidget(widget,data);
        if(!disabled)widget->setUpdatesEnabled(true);
    }
};
}

mPlotMarkerLine::mPlotMarkerLine(QwtPlot* plot,const QString& name,Direction direction,double value)
    :plot_(plot),marker_(new QwtPlotMarker(name)),direction_(direction)
{
    marker_->setLineStyle(direction==Vertical?QwtPlotMarker::VLine:QwtPlotMarker::HLine);
    marker_->setItemAttribute(QwtPlotItem::AutoScale,false);
    marker_->setLinePen(MxFunctions::UiPalette::primary(),1.5,Qt::DashLine);
    marker_->attach(plot_);setValue(value);
}
mPlotMarkerLine::~mPlotMarkerLine(){delete marker_;}
void mPlotMarkerLine::setValue(double value)
{
    if(!std::isfinite(value))return;value_=value;
    if(direction_==Vertical)marker_->setXValue(value);else marker_->setYValue(value);
    plot_->replot();
}

mPlotView::mPlotView(QWidget* parent):QWidget(parent)
{
    setObjectName(QStringLiteral("mPlotView"));
    auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);
    plot_=new QwtPlot(this);plot_->setObjectName(QStringLiteral("originalMotionQwtPlot"));layout->addWidget(plot_);
    auto* canvas=static_cast<QwtPlotCanvas*>(plot_->canvas());canvas->setFrameShape(QFrame::NoFrame);
    grid_=new QwtPlotGrid;grid_->setMajorPen(MxFunctions::UiPalette::border(),0,Qt::DotLine);grid_->attach(plot_);
    axes_[X_Top].isVisible=false;axes_[Y_Right].isVisible=false;
    axes_[X_Bottom].title=QStringLiteral("时间(s)");axes_[Y_Left].title=QStringLiteral("数值");
    for(int axis=0;axis<4;++axis)setAxisSettings(static_cast<AxisId>(axis),axes_[std::size_t(axis)]);
    settings_.backgroundColor=MxFunctions::UiPalette::surface();setPlotSettings(settings_);
    zoomer_=new QwtPlotZoomer(canvas,false);
    zoomer_->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton,Qt::ShiftModifier);
    zoomer_->setMousePattern(QwtEventPattern::MouseSelect2,Qt::RightButton);
    zoomer_->setTrackerMode(QwtPicker::AlwaysOff);
    zoomer_->setRubberBandPen(QPen(MxFunctions::UiPalette::primary(),1,Qt::DashLine));
    auto* picker=new QwtPlotPicker(canvas);picker->setStateMachine(new QwtPickerClickPointMachine);
    picker->setMousePattern(QwtEventPattern::MouseSelect1,Qt::LeftButton);
    connect(picker,qOverload<const QPointF&>(&QwtPlotPicker::selected),this,[this](const QPointF& point){
        if(!order_.isEmpty()&&std::isfinite(point.x()))emit signalTimeClicked(point.x());
    });
}
mPlotView::~mPlotView(){markers_.clear();for(auto& curve:curves_)qDeleteAll(curve.segments);curves_.clear();}
void mPlotView::setPlotViewStyleWidget(mPlotViewStyleWidget* widget){styleWidget_=widget;}
mPlotViewStyleWidget* mPlotView::getPlotViewStyleWidget()const{return styleWidget_;}

void mPlotView::applyCurveStyle(CurveData& data)
{
    for(auto* segment:data.segments) {
        segment->setPen(QPen(data.settings.lineColor,data.settings.lineWidth,data.settings.lineStyle));
        segment->setVisible(data.settings.isVisible);
        segment->setStyle(type_==ScatterChart?QwtPlotCurve::NoCurve:type_==StepChart?QwtPlotCurve::Steps:QwtPlotCurve::Lines);
        segment->setSymbol(data.settings.showSymbol||type_==ScatterChart
            ?new QwtSymbol(QwtSymbol::Ellipse,QBrush(data.settings.symbolColor),QPen(data.settings.symbolColor),QSize(int(data.settings.symbolSize),int(data.settings.symbolSize))):nullptr);
    }
}
void mPlotView::updateData(const QString& name,const QVector<double>& x,const QVector<double>& y)
{
    if(x.size()!=y.size())return;
    if(!curves_.contains(name)) {
        CurveData data;const auto color=PlotColor[order_.size()%PlotColor.size()];data.settings.lineColor=color;data.settings.symbolColor=color;
        curves_.insert(name,data);order_.append(name);
    }
    auto& data=curves_[name];qDeleteAll(data.segments);data.segments.clear();data.samples.clear();
    QVector<QPointF> run;
    const auto flush=[&] {
        if(run.isEmpty())return;
        auto* curve=new QwtPlotCurve(name);curve->setRenderHint(QwtPlotItem::RenderAntialiased,true);
        curve->setSamples(run);curve->setItemAttribute(QwtPlotItem::Legend,data.segments.isEmpty());curve->attach(plot_);
        data.segments.append(curve);run.clear();
    };
    for(int i=0;i<x.size();++i) {
        const QPointF point(x[i],y[i]);data.samples.append(point);
        if(!std::isfinite(x[i])||!std::isfinite(y[i]))flush();else run.append(point);
    }
    flush();applyCurveStyle(data);rescaleAxes();emit contentsChanged();emit seriesUpdated();
}
void mPlotView::clearAll()
{
    for(auto& curve:curves_)qDeleteAll(curve.segments);curves_.clear();order_.clear();
    // Playback markers are independent from curve series, as in the original
    // AnimationPanel contract. A curve refresh must not erase the time cursor.
    plot_->replot();emit contentsChanged();
}
QStringList mPlotView::getCurveNames()const{return order_;}
QVector<QPointF> mPlotView::getCurveSamples(const QString& name)const{return curves_.value(name).samples;}
CurveSettings mPlotView::getCurveSettings(const QString& name)const{return curves_.value(name).settings;}
void mPlotView::setCurveStyle(const QString& name,const CurveSettings& settings)
{
    auto found=curves_.find(name);if(found==curves_.end()||!std::isfinite(settings.lineWidth)||settings.lineWidth<0)return;
    found->settings=settings;applyCurveStyle(*found);plot_->replot();
}
AxisSettings mPlotView::getAxisSettings(AxisId axis)const
{
    auto result=axes_.at(std::size_t(axis));const auto scale=plot_->axisScaleDiv(int(axis));
    result.minimum=scale.lowerBound();result.maximum=scale.upperBound();return result;
}
void mPlotView::setAxisSettings(AxisId axis,const AxisSettings& settings)
{
    if(int(axis)<0||int(axis)>3)return;
    if(!settings.autoScale&&(!std::isfinite(settings.minimum)||!std::isfinite(settings.maximum)
        ||settings.maximum<=settings.minimum||(settings.isLog&&settings.minimum<=0)))return;
    axes_[std::size_t(axis)]=settings;
    plot_->enableAxis(int(axis),settings.isVisible);plot_->setAxisTitle(int(axis),settings.title);
    plot_->setAxisScaleEngine(int(axis),settings.isLog?static_cast<QwtScaleEngine*>(new QwtLogScaleEngine):new QwtLinearScaleEngine);
    if(settings.autoScale)plot_->setAxisAutoScale(int(axis));else plot_->setAxisScale(int(axis),settings.minimum,settings.maximum,settings.stepSize);
    plot_->replot();
}
void mPlotView::updateAxisScale(AxisId axis,const AxisSettings& settings){setAxisSettings(axis,settings);}
void mPlotView::updateAxisRange(AxisId axis,const AxisSettings& settings){setAxisSettings(axis,settings);}
void mPlotView::updateAxisLog(AxisId axis,const AxisSettings& settings){setAxisSettings(axis,settings);}
PlotSettings mPlotView::getPlotSettings()const{auto result=settings_;result.title=plot_->title().text();return result;}
void mPlotView::setPlotSettings(const PlotSettings& settings)
{
    settings_=settings;plot_->setTitle(settings.title);plot_->setCanvasBackground(settings.backgroundColor);grid_->setVisible(settings.showGrid);
    if(settings.showLegend&&!plot_->legend())plot_->insertLegend(new OriginalLegend,QwtPlot::BottomLegend);
    if(plot_->legend())plot_->legend()->setVisible(settings.showLegend);
    plot_->replot();
}
void mPlotView::setPlotType(PlotType type){type_=type;for(auto& curve:curves_)applyCurveStyle(curve);plot_->replot();}
void mPlotView::setLegendVisual(bool visible){auto settings=getPlotSettings();settings.showLegend=visible;setPlotSettings(settings);}
void mPlotView::rescaleAxes()
{
    for(int i=0;i<4;++i)if(axes_[std::size_t(i)].autoScale)plot_->setAxisAutoScale(i);
    plot_->replot();if(zoomer_)zoomer_->setZoomBase();
}
mPlotMarkerLine* mPlotView::getMarkerLine(const QString& name)const
{
    const auto found=markers_.find(name);return found==markers_.end()?nullptr:found->second.get();
}
void mPlotView::addMarkerLine(const QString& name,mPlotMarkerLine::Direction direction,double value)
{
    if(!std::isfinite(value))return;markers_[name]=std::make_unique<mPlotMarkerLine>(plot_,name,direction,value);
}
void mPlotView::removeMarkerLine(const QString& name){markers_.erase(name);plot_->replot();}

mPlotViewStyleWidget::mPlotViewStyleWidget(mPlotView* plot,QWidget* parent):QWidget(parent),plot_(plot)
{
    setObjectName(QStringLiteral("mPlotViewStyleWidget"));
    auto* layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(4);
    section_=new QComboBox(this);section_->addItems({QStringLiteral("绘图"),QStringLiteral("坐标轴"),QStringLiteral("曲线")});
    item_=new QComboBox(this);layout->addWidget(section_);layout->addWidget(item_);
    table_=new QTableWidget(this);table_->setColumnCount(2);table_->setHorizontalHeaderLabels({QStringLiteral("属性"),QStringLiteral("值")});
    table_->verticalHeader()->hide();table_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);table_->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(table_,1);
    connect(section_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshData();});
    connect(item_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{if(!refreshing_)populate();});
    connect(plot_,&mPlotView::contentsChanged,this,[this]{refreshData();});refreshData();
}
void mPlotViewStyleWidget::setTableGridVisible(bool value){table_->setShowGrid(value);}
void mPlotViewStyleWidget::setTableAlternatingRowColors(bool value){table_->setAlternatingRowColors(value);}
void mPlotViewStyleWidget::refreshAxisRangeData(){if(section_->currentIndex()==1)populate();}
void mPlotViewStyleWidget::refreshData()
{
    if(refreshing_||!plot_)return;refreshing_=true;
    const QString previous=item_->currentText();QSignalBlocker blocked(item_);item_->clear();
    const int section=section_->currentIndex();item_->setVisible(section!=0);
    if(section==1)item_->addItems({QStringLiteral("X轴（下）"),QStringLiteral("Y轴（左）"),QStringLiteral("X轴（上）"),QStringLiteral("Y轴（右）")});
    else if(section==2)item_->addItems(plot_->getCurveNames());
    const int index=item_->findText(previous);if(index>=0)item_->setCurrentIndex(index);
    refreshing_=false;populate();
}
void mPlotViewStyleWidget::populate()
{
    if(!plot_)return;table_->setRowCount(0);
    const auto row=[this](const QString& name,QWidget* editor){const int i=table_->rowCount();table_->insertRow(i);auto* title=new QTableWidgetItem(name);title->setFlags(Qt::ItemIsEnabled);table_->setItem(i,0,title);table_->setCellWidget(i,1,editor);};
    const auto text=[&](const QString& name,const QString& value,auto apply){auto* edit=new QLineEdit(value,table_);row(name,edit);connect(edit,&QLineEdit::editingFinished,this,[edit,apply]{apply(edit->text());});};
    const auto check=[&](const QString& name,bool value,auto apply){auto* edit=new QCheckBox(table_);edit->setChecked(value);row(name,edit);connect(edit,&QCheckBox::toggled,this,apply);};
    const auto number=[&](const QString& name,double value,double low,double high,bool enabled,auto apply){auto* edit=new QDoubleSpinBox(table_);edit->setDecimals(8);edit->setRange(low,high);edit->setValue(value);edit->setEnabled(enabled);row(name,edit);connect(edit,&QDoubleSpinBox::editingFinished,this,[edit,apply]{apply(edit->value());});};
    const auto color=[&](const QString& name,const QColor& value,auto apply){auto* edit=new QPushButton(value.name(),table_);edit->setStyleSheet(QStringLiteral("color:%1").arg(value.name()));row(name,edit);connect(edit,&QPushButton::clicked,this,[this,edit,value,apply]{const auto chosen=QColorDialog::getColor(value,this);if(chosen.isValid()){apply(chosen);edit->setText(chosen.name());edit->setStyleSheet(QStringLiteral("color:%1").arg(chosen.name()));}});};
    const int section=section_->currentIndex();
    if(section==0) {
        const auto settings=plot_->getPlotSettings();
        text(QStringLiteral("标题"),settings.title,[this](const QString& value){auto s=plot_->getPlotSettings();s.title=value;plot_->setPlotSettings(s);});
        color(QStringLiteral("背景颜色"),settings.backgroundColor,[this](const QColor& value){auto s=plot_->getPlotSettings();s.backgroundColor=value;plot_->setPlotSettings(s);});
        check(QStringLiteral("图例"),settings.showLegend,[this](bool value){plot_->setLegendVisual(value);});
        check(QStringLiteral("网格"),settings.showGrid,[this](bool value){auto s=plot_->getPlotSettings();s.showGrid=value;plot_->setPlotSettings(s);});
    } else if(section==1&&item_->currentIndex()>=0) {
        const AxisId axis=std::array<AxisId,4>{X_Bottom,Y_Left,X_Top,Y_Right}[std::size_t(item_->currentIndex())];const auto settings=plot_->getAxisSettings(axis);
        text(QStringLiteral("标题"),settings.title,[this,axis](const QString& value){auto s=plot_->getAxisSettings(axis);s.title=value;plot_->setAxisSettings(axis,s);});
        check(QStringLiteral("显示"),settings.isVisible,[this,axis](bool value){auto s=plot_->getAxisSettings(axis);s.isVisible=value;plot_->setAxisSettings(axis,s);});
        check(QStringLiteral("自动范围"),settings.autoScale,[this,axis](bool value){auto s=plot_->getAxisSettings(axis);s.autoScale=value;plot_->setAxisSettings(axis,s);QTimer::singleShot(0,this,[this]{populate();});});
        check(QStringLiteral("对数坐标"),settings.isLog,[this,axis](bool value){auto s=plot_->getAxisSettings(axis);s.isLog=value;plot_->setAxisSettings(axis,s);});
        number(QStringLiteral("最小值"),settings.minimum,-1e100,1e100,!settings.autoScale,[this,axis](double value){auto s=plot_->getAxisSettings(axis);s.minimum=value;plot_->setAxisSettings(axis,s);});
        number(QStringLiteral("最大值"),settings.maximum,-1e100,1e100,!settings.autoScale,[this,axis](double value){auto s=plot_->getAxisSettings(axis);s.maximum=value;plot_->setAxisSettings(axis,s);});
    } else if(section==2&&!item_->currentText().isEmpty()) {
        const QString name=item_->currentText();const auto settings=plot_->getCurveSettings(name);
        check(QStringLiteral("显示"),settings.isVisible,[this,name](bool value){auto s=plot_->getCurveSettings(name);s.isVisible=value;plot_->setCurveStyle(name,s);});
        color(QStringLiteral("线条颜色"),settings.lineColor,[this,name](const QColor& value){auto s=plot_->getCurveSettings(name);s.lineColor=value;plot_->setCurveStyle(name,s);});
        number(QStringLiteral("线宽"),settings.lineWidth,0.1,20,true,[this,name](double value){auto s=plot_->getCurveSettings(name);s.lineWidth=value;plot_->setCurveStyle(name,s);});
        auto* style=new QComboBox(table_);style->addItems({QStringLiteral("实线"),QStringLiteral("虚线"),QStringLiteral("点线"),QStringLiteral("点划线")});
        const std::array<Qt::PenStyle,4> styles{Qt::SolidLine,Qt::DashLine,Qt::DotLine,Qt::DashDotLine};
        style->setCurrentIndex(int(std::distance(styles.begin(),std::find(styles.begin(),styles.end(),settings.lineStyle))));row(QStringLiteral("线型"),style);
        connect(style,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,name,styles](int index){if(index<0||index>=int(styles.size()))return;auto s=plot_->getCurveSettings(name);s.lineStyle=styles[std::size_t(index)];plot_->setCurveStyle(name,s);});
        check(QStringLiteral("数据点"),settings.showSymbol,[this,name](bool value){auto s=plot_->getCurveSettings(name);s.showSymbol=value;plot_->setCurveStyle(name,s);});
        color(QStringLiteral("数据点颜色"),settings.symbolColor,[this,name](const QColor& value){auto s=plot_->getCurveSettings(name);s.symbolColor=value;plot_->setCurveStyle(name,s);});
        number(QStringLiteral("数据点大小"),settings.symbolSize,1,30,true,[this,name](double value){auto s=plot_->getCurveSettings(name);s.symbolSize=value;plot_->setCurveStyle(name,s);});
    }
    table_->resizeRowsToContents();
}
}
