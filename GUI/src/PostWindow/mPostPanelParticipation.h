#pragma once
#include "mpostwindow_global.h"
#include <QWidget>
#include <QVector>
#include <QHash>
#include <QPointer>
#include <QPair>
#include <set>

#include <qwt_plot_marker.h>
#include "mPostPanelParticipationData.h"
#include "PlotDataCache.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace Ui
{
    class mPostPanelParticipation;
}

namespace PlotView
{
    class mPlotView;
}

using namespace std;
namespace PostWindow
{
    class mPostDataTable;
    class mPostPanelParticipationData;
    class mPostFrequencyPanelData;
    class mPostPlotFloatWindow;

    class MPOSTWINDOW_EXPORT mPostPanelParticipation : public QWidget
    {
        Q_OBJECT

    public:
        /** 构造函数 */
        mPostPanelParticipation(QWidget *parent = Q_NULLPTR);

        /** 析构函数 */
        ~mPostPanelParticipation();

        /** 当前面板是否显示 */
        void setPanelPickAndSignal(bool isshow);

        //设置显示及研究模块控件可选状态
        void initialEnabledUI(bool isEnabled);

        //获取原频响曲线与去除模态响应的频响曲线差异数据
        QVector<double> getDifferenceData(QVector<double> oldFre, QVector<double> newFre);

    private:
        /** 提取数据 */
        bool generateCurveData();

        //读取模态贡献量数据
        bool readPanelData(QString filePath);

    private slots:

        /** 切换中英文 */
        void slot_changeLanguage(int language);

        //选择pch文件
        void chooseFile();

        //导入文件读取信息
        void loadFile();

        //文件路径修改
        void filePathChanged(const QString& text);

        //导入界面切换工况id
        void chooseSubcaseID(const QString& caseID);

        //导入界面切换结果类型
        void chooseResultType(const QString& resultType);

        //导入界面切换gridID
        void chooseGrid(const QString& gridid);

        //导入界面切换分量
        void chooseComponent(const QString& component);

        //绘制频响曲线
        void plotFrequencyResponseCurve();

        //绘制研究部分曲线
        void plotStudyCurve();

        //切换显示模式（百分比、幅值）
        void chooseShowPanel(int index);

        //绘制显示部分柱状图、曲线
        void plotShowCurve();

        /** 创建新曲线视图 */
        PlotView::mPlotView* createNewCurvePlot(QString& viewName);

        /** 在浮动窗口中打开曲线视图 */
        void openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView);

        /** 应用坐标轴类型（0 线性，1 对数） */
        void applyAxisType(PlotView::mPlotView* plotView, int axisType);

        /** 设置图例信息文本（多行说明） */
        void setLegendInfoTextOnPlot(PlotView::mPlotView* plotView, const QStringList& infoLines);

        /** 绘制柱状图 */
        void plotBarChart(PlotView::mPlotView* plotView, const QString& name,
            const QStringList& xLabels, const QVector<double>& yData, const QString& title);

        /** 在柱顶添加数值标签 */
        void addBarValueLabels(PlotView::mPlotView* plotView, const QString& curveName,
            const QVector<double>& xValues, const QVector<double>& yData);

        /** 缓存绘图数据 */
        void cachePlotData(const QString& vname, const PlotDataCache& cache);

        /** 当前激活窗口改变 */
        void slot_postActiveViewChanged();

        /** 删除曲线图 */
        void slot_deleteCurvePlot();


        //获取范围内的模态
        QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> getPanelByRange(double min, double max, QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> mode);

        //获取除显示模态外的其他模态幅值
        QVector<double> getOtherMagnitudes(QVector<double> response, QVector<double> fres, QList<QPair<int, QPair<QVector<double>, QVector<double>>>> modeData);
 
        //显示数据表格
        void showDataTable();

        //删除曲线视图
        void deleteCurveView();

        //切换tab
        void currentTabChanged(int index);

    signals:
        /** 返回到主面板 */
        void returnSig();

    private:
        Ui::mPostPanelParticipation*		         _ui;		                                //UI
        bool                                     isOpenFile_ = false;                       //文件导入成功标志
        QString                                  filePath_;                                 //pch文件路径
        QPointer<PostWindow::mPostPanelParticipationData>     panelParticipationData_{};      //所有模态数据
        int                                      subcaseID_;                                //工况ID
        int                                      gridID_;                                   //gridID
        QString                                  component_;                                //分量
        QString                                  resultType_;                               //结果类型
        QVector<double>                          allFrequence_;                             //所有频率
        QVector<double>                          allTotalResponse_;                         //所有总响应
        mPostDataTable*									 dataTable_{};				                //数据表窗口
        QString                                  importCurveView_;                          //导入视图（频响曲线）
        QString                                  showChartPercentView_;                     //显示视图（百分比柱状图）
        QString                                  showChartMagnitudeView_;                   //显示视图（幅值柱状图）
        QString                                  showCurveView_;                            //显示视图（频率-幅值曲线）
        QString                                  studyCurveView_;                           //研究视图（频响差异曲线）

        QHash<QString, PlotDataCache>            _plotDataCache;                            //每个视图的数据缓存（供数据表使用）

        // 每个 plotView 当前拥有的柱顶数值标签 Marker，用于重绘时清理旧标签
        QHash<PlotView::mPlotView*, QList<QwtPlotMarker*>> _barValueLabelMarkers;

        // 每个 plotView 的 y=0 参考线 Marker，用于重绘时清理旧线
        // pair: QwtPlot 弱引用 + Marker 指针，防止 plot 先销毁后 detach 悬空崩溃
        QHash<PlotView::mPlotView*, QPair<QPointer<QwtPlot>, QwtPlotMarker*>> _zeroLineMarkers;

        QHash<QString, QPointer<mPostPlotFloatWindow>> _plotFloatWindows;                   //浮动窗口映射
        QString                                  _activePlotViewName;                       //当前激活的视图名
    };
}
