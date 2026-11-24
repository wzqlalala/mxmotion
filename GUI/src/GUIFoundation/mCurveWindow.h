#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QVector>
#include <QList>
#include <QSplitterHandle>
#include <QToolButton>
#include <QPainter>
#include <QEvent>
#include "mPlotEnum.h"
#include "guifoundation_global.h"
#include <QTimer>
#include <QPointer>
#include "mPlotStructs.h"

namespace PlotView
{
    class mPlotView;
}
using namespace PlotView;
namespace MechMainWindow
{
    /**
     * @brief 曲线显示窗口（单例）
     * 支持两种绘图模式：
     *  - 单曲线：解析/离散/插值
     *  - 多曲线：分段/阶跃（区间断开）
     */
    class GUIFOUNDATION_EXPORT mCurveWindow : public QWidget
    {
        Q_OBJECT
    public:
        static mCurveWindow* getInstance();
        static mCurveWindow* createForProject(QWidget* owner);
        static void setActiveProjectWindow(mCurveWindow* window);
        ~mCurveWindow();

        //窗口与图表画布底色按当前主题重设（切档/切主题时由 ThemeManager
        //经注册的 applier 调用，见 mDockLayoutManager）
        void applyWindowTheme();
        /*
        * @brief 設置標題
        * @param
        */
        void setPlotTitle(QString title);

        /*
        * @brief 清空全部曲線
        * @param 
        */
        void clearAllPlot();

        /*
        * 设置样式窗口是否显示
        */
        void setViewStyleVisible(bool show);

        /*
        * 自适应
        */
        void setAxisAuto(PlotView::AxisId axisId, bool isAuto);

        /**
         * @brief 设置指定轴的对数/线性刻度
         * @param axisId 轴ID
         * @param isLog true=对数, false=线性
         */
        void setAxisLog(PlotView::AxisId axisId, bool isLog);


        /**
         * @brief 单曲线更新
         * @param isClear 是否清空之前的曲線；showLegen 是否顯示圖例
         */
        void updateCurve(const QVector<double>& x,
            const QVector<double>& y, PlotView::PlotType plotType= PlotView::LineChart,
            const QString& title = QStringLiteral("y = f(x)"),
            bool isClear = true, bool showLegend = false);

        /**
         * @brief 多曲线更新（各段独立，不连线）,共用x
         * @param isClear 是否清空之前的曲線；showLegen 是否顯示圖例
         */
        
        void updateMultiCurves(const QVector<double>& xData,
            const QVector<QVector<double>>& yData, PlotView::PlotType plotType = PlotView::LineChart,
            const QStringList& titles = QStringList(),
            bool isClear = true, bool showLegend = false);

        /**
         * @brief 多曲线更新（各段独立，不连线）,不共用x
         * @param isClear 是否清空之前的曲線；showLegen 是否顯示圖例
         */

        void updateMultiCurves(const QVector<QVector<double>>& xData,
            const QVector<QVector<double>>& yData, PlotView::PlotType plotType = PlotView::LineChart,
            const QStringList& titles = QStringList(),
            bool isClear = true, bool showLegend = false);
        /*
        * 获取plotView
        */
        mPlotView* getPlotView();

        /* 设置X轴/Y轴标题 */
        void SetAxisTitle(const QString& title, bool isXAxis);

    private:
        explicit mCurveWindow(QWidget* parent = nullptr);
        Q_DISABLE_COPY(mCurveWindow)

        static mCurveWindow* _instance;
        static QPointer<mCurveWindow> _activeProjectWindow;

        //曲线视图
        mPlotView* _plotView = nullptr;

    private slots:
        /* 语言切换占位槽 */
        void slot_changeLanguage(int);
    };
} // namespace MechMainWindow
