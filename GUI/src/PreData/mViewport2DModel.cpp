//
//#include "mViewportModel.h"
////Render
//#include "mxr/view/CCSMRendWidget.h"
//#include "mxr/view/REBUILD_PostRend.h"
//#include "mxr/view/CCSMPreRender.h"
//#include "mxr/view/REBUILD_PostRender.h"
//#include "mxr/data/SelectDataStruct.h"
//#include "mxr/view/CCSMMeshModelRender.h"
//#include "mxr/view/mPreGeoModelRender.h"
//#include "mxr/view/SelectionCaculator.h"
//
//
////BasicData
//#include "mMeshData.h"
//
////Qt
//#include <mGeoDataBase.h>
//#include <QMDiArea>
//#include <QTimer>
//#include <QApplication>
//#include <QEvent>
//
//#include "mPlotView.h"
//#include "mMechGlobalSignals.h"
//#include "mDataStatusManager.h"
//#include "PickCommonFunction.h"
//#include "PreViewportWidget.h"
//#include "PostViewportWidget.h"
//
//namespace MechPreData
//{
//    PlotView::mPlotView* mViewportModel::getActivePlotXYPlot()
//    {
//        PlotView::mPlotView* plot2d = nullptr;
//        QMdiSubWindow* subwindow = (QMdiSubWindow*)this->activeSubWindow();
//        if (subwindow != nullptr)
//        {
//            for (auto it = _viewportsPlot.begin(); it != _viewportsPlot.end(); ++it)
//            {
//                if (it.value() == subwindow)
//                {
//                    return it.key();
//                }
//            }
//        }
//        return nullptr;
//    }
//
//    void mViewportModel::removeAllPlotSub()
//    {
//        if (this->subWindowList().size() <= 0)
//            return;
//
//        for (auto it : _viewportsPlot)
//        {
//            this->removeSubWindow(it);
//            it->hide();
//        }
//    }
//
//    QMdiSubWindow* mViewportModel::openViewporPlot(QMdiArea* parent, PlotView::mPlotView* plotview, QString viewName)
//    {
//        if (plotview == nullptr)
//        {
//            return;
//        }
//
//        QMdiSubWindow* subWindow = nullptr;
//        if (_viewportsPlot.keys().contains(plotview))
//        {
//            subWindow = _viewportsPlot.value(plotview);
//        }
//        else
//        {
//            QList<PlotView::mPlotView*> views = _viewportsPlot.keys();
//            for (PlotView::mPlotView* vb : views)
//            {
//                if (vb->title() == viewName)
//                {
//                    QMdiSubWindow* sw = _viewportsPlot.value(vb);
//                    _viewportsPlot.remove(vb);
//                    delete vb;
//                    delete sw;
//                }
//            }
//            //subWindow = (QMdiSubWindow*)this->addSubWindow(plotview);
//            subWindow = new QMdiSubWindow(this);
//            subWindow->setWidget(plotview);
//            _viewportsPlot.insert(plotview, subWindow);
//        }
//        //显示MDI子窗口
//        if (subWindow != nullptr)
//        {
//            subWindow->activateWindow();
//            subWindow->showMaximized();
//            subWindow->setWindowTitle(viewName);
//            //subWindow->setWindowFlags(Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
//            subWindow->setWindowFlags(Qt::FramelessWindowHint);
//        }
//    }
//
//    QMdiSubWindow* mViewportModel::openViewPortPlot2DbyName(const QString& name)
//    {
//        QHash<PlotView::mPlotView*, QMdiSubWindow*>::iterator it = _viewportsPlot.begin();
//        while (it != _viewportsPlot.end())
//        {
//            PlotView::mPlotView* view2d = it.key();
//            if (view2d != nullptr)
//            {
//                QString viewname = view2d->getViewportName();
//                if (viewname == name)
//                {
//                    this->setActiveSubWindow(nullptr);
//                    if (it.value() == nullptr)
//                    {
//                        return nullptr;
//                    }
//                    for (auto sub : this->subWindowList())
//                    {
//                        if (sub == it.value())
//                        {
//                            it.value()->setGeometry(this->viewport()->rect());
//                            it.value()->setWindowFlags(Qt::FramelessWindowHint);
//                            it.value()->show();
//                            it.value()->activateWindow();
//                            this->setActiveSubWindow(it.value());
//                            this->update();
//                            return it.value();
//                        }
//                    }
//
//                    this->addSubWindow(it.value());
//                    it.value()->setGeometry(this->viewport()->rect());
//                    it.value()->setWindowFlags(Qt::FramelessWindowHint);
//                    it.value()->show();
//                    it.value()->activateWindow();
//                    this->setActiveSubWindow(it.value());
//                    this->update();
//                    return it.value();
//                }
//            }
//            it++;
//        }
//        return nullptr;
//    }
//
//    PlotView::mPlotView* mViewportModel::getTimeCurveView(const QString& name)
//    {
//        QHash<PlotView::mPlotView*, QMdiSubWindow*>::iterator it = _viewportsPlot.begin();
//        while (it != _viewportsPlot.end())
//        {
//            PlotView::mPlotView* view2d = it.key();
//            if (view2d != nullptr)
//            {
//                QString viewname = view2d->getViewportName();
//                if (viewname == name)
//                {
//                    return view2d;
//                }
//            }
//            ++it;
//        }
//        return nullptr;
//    }
//
//    QMdiSubWindow* mViewportModel::getTimeCurvesubWindow(const QString& name)
//    {
//        QHash<PlotView::mPlotView*, QMdiSubWindow*>::iterator it = _viewportsPlot.begin();
//        while (it != _viewportsPlot.end())
//        {
//            PlotView::mPlotView* view2d = it.key();
//            if (view2d != nullptr)
//            {
//                QString viewname = view2d->getViewportName();
//                if (viewname == name)
//                {
//                    return it.value();
//                }
//            }
//            ++it;
//        }
//        return nullptr;
//    }
//}