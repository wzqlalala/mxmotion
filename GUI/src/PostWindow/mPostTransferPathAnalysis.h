#pragma once
#include "mpostwindow_global.h"
#include <QWidget>
#include <QVector>
#include <QHash>
#include <set>
#include <QPair>
#include <QPointer>
#include <QColor>
#include <QTableWidgetItem>

#include <qwt_plot_marker.h>
#include "PlotDataCache.h"
#include "PchReader/PchReader.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace Ui
{
    class mPostTransferPathAnalysis;
}

namespace MPostRend
{
    class REBUILD_PostRend;
}

namespace PlotView
{
    class mPlotView;
}

using namespace std;
namespace PostWindow
{
    class mPostDataTable;
    class mPostTransferPathAnalysisData;
    class mPostPlotFloatWindow;
    class MPOSTWINDOW_EXPORT mPostTransferPathAnalysis : public QWidget
    {
        Q_OBJECT

    public:
        /** 构造函数 */
        mPostTransferPathAnalysis(QWidget *parent = Q_NULLPTR);

        /** 析构函数 */
        ~mPostTransferPathAnalysis();

        /** 当前面板是否显示 */
        void setPanelPickAndSignal(bool isshow);

        //设置导入模块控件可选状态
        void initialImportEnabledUI(bool isEnabled);

        //设置显示模块控件可选状态
        void initialShowEnabledUI(bool isEnabled);

        //转化幅值相位数据
        QPair<double, double> convertToRealImag(double magnitude, double phaseInDegrees);

        //将幅值相位数据转化为实部虚部数据
        QHash<QString, QVector<QPair<double, double>>> convertMagnitudePhaseToRealImag(const QHash<QString, QVector<QPair<double, double>>>& inputHash);

        //计算所有输入力数据与传递函数数据的乘积（实部、虚部）
        QHash<QString, QVector<QPair<double, double>>> calProductForcaDataAndTransferData(const QHash<QString, QVector<QPair<double, double>>> forceData,
            const QHash<QString, QVector<QPair<double, double>>> transferData);

        //通过实部、虚部格式的输入力*传递函数计算幅值、相位（转化为正角度）格式
        QHash<QString, QVector<QPair<double, double>>> calForcaAndTransferMPData(const QHash<QString, QVector<QPair<double, double>>> froceTransRIData);

        //计算所有频率输入力*传递函数数据所有路径实部、虚部之和
        QVector < QPair<double, double>> calForceAndTransferRISum(QHash<QString, QVector<QPair<double, double>>> forceTransferRIData);

        //计算所有频率输入力*传递函数数据所有路径之和的幅值、相位
        QVector < QPair<double, double>> calForceAndTransferMPSum(QVector<QPair<double, double>> RIdata);

        //添加表格视图项
        void addTableWidgetView(QString viewname);

        //设置列表当前选中
        void setCurrentSelectView(QString itemname);

        //移除列表内容
        void removeTableView(QString viewname);

        //根据频率获取所有路径的百分比
        QList<QPair<QString, double>> getAllPathPercentsByFrequency(double fre);

        //根据频率和路径获取传递函数数据的幅值
        QVector<double> getTransferMagnitudeByFrequencyAndPath(double fre, QStringList pathNames);

        //根据频率和路径获取输入力数据的幅值
        QVector<double> getForceMagnitudeByFrequencyAndPath(double fre, QStringList pathNames);

        //获取传递函数和输入力总和的幅值
        double getMagSum(double fre);

        //获取总幅值百分比投影
        QVector<double> getMagProjectByFre(double fre, QVector<double> percents);

        //截取只包含单元ID和自由度的路径名称
        QString getShortPathName(QString pathname);

        //创建自定义显示图例
        void createCustomLegend(QString viewname, QStringList legendInformation, QColor backColor = QColor(Qt::transparent), QColor textColor = QColor(Qt::black));

        //设置路径贡献柱状图图例
        void setPathContributionLegend(double showFre, QVector<double> showPercents, QStringList showPaths);

        //设置幅值类型路径贡献柱状图图例
        void setMagPathContributionLegend(double showFre, QVector<double> showPercents, QStringList showPaths);

        //设置传递函数柱状图图例
        void setTransferFunctionLegend(QVector<double> showMags, QStringList showPaths);

        //设置输入力柱状图图例
        void setInputForceLegend(QVector<double> showMags, QStringList showPaths);

        //根据频率获取所有路径幅值占总幅值的百分比
        QList<QPair<QString, double>> getAllPathMagnitudePercentsByFrequency(double fre);

        //获取未显示路径实部、虚部求和取幅值后占总幅值的百分比(幅值柱状图Others)
        double getMagPercentOthers(double fre, QStringList showPaths);

        // 获取差异数据的百分比
        QVector<double> getDifferenceData(QVector<double> oldFre, QVector<double> newFre);

    private:
        //读取传递函数文件数据
        bool readTransferFuntionData(QString filePath);

        //读取输入力文件数据
        bool readForceData(QString filePath);

    private slots:

        /** 切换中英文 */
        void slot_changeLanguage(int language);

        //选择tf传递函数文件
        void chooseTFFile();

        //选择force输入力文件
        void chooseForceFile();

        //导入文件读取信息
        void loadFile();

        //传递函数文件路径修改
        void tfFilePathChanged(const QString& text);

        //输入力文件路径修改
        void forceFilePathChanged(const QString& text);

        //导入界面切换工况id
        void chooseSubcaseID(const QString& caseID);

        //导入界面切换结果类型
        void chooseResultType(const QString& resultType);

        //导入界面切换响应ID
        void chooseResponseID(const QString& responseID);

        //绘制频响曲线
        void plotFrequencyResponseCurve();

        //绘制研究部分曲线
        void plotStudyCurve();

        //点击激活曲线视图
        void slot_onCellClicked(QTableWidgetItem *item);

        //绘制显示部分柱状图、曲线
        void plotShowCurve();

        //应用坐标轴类型
        void applyAxisType(PlotView::mPlotView* plotView, int axisType);

        //在图上设置图例信息文本
        void setLegendInfoTextOnPlot(PlotView::mPlotView* plotView, const QStringList& infoLines);

        //绘制柱状图
        void plotBarChart(PlotView::mPlotView* plotView, const QString& name,
            const QStringList& xLabels, const QVector<double>& yData, const QString& title);

        //在柱状图柱顶添加数值标签
        void addBarValueLabels(PlotView::mPlotView* plotView, const QString& curveName,
            const QVector<double>& xValues, const QVector<double>& yData);

        //缓存曲线数据
        void cachePlotData(const QString& vname, const PlotDataCache& cache);

        /** 创建新曲线视图 */
        PlotView::mPlotView* createNewCurvePlot(QString& viewName);

        /** 在浮动窗口中打开曲线视图 */
        void openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView);

        /** 当前激活窗口改变 */
        void slot_postActiveViewChanged();

        /** 删除曲线图 */
        void slot_deleteCurvePlot();

        //显示数据表格
        void showDataTable();

        //删除曲线视图
        void deleteCurveView();

        //切换tab
        void currentTabChanged(int index);

        //保存读取数据
        void saveFileData();

        //保存传递函数数据
        void saveTransmitDate();

        //保存力文件数据
        void saveForceDate();

    signals:
        /** 返回到主面板 */
        void returnSig();

    private:
        Ui::mPostTransferPathAnalysis*             _ui;		                                  //UI
        bool                                     isOpenFile_ = false;                             //文件导入成功标志
        QString                                  _tfFilePath;                                     //传递函数文件路径
        QString                                  _forceFilePath;                                  //输入力文件路径
        QPointer<PostWindow::mPostTransferPathAnalysisData>     _transferPathAnalysisData{};      //所有TPA数据
        int                                      _subcaseID;                                      //工况ID
        QString                                  _resultType;                                     //结果类型
        int                                      _responseID;                                     //响应ID
        QVector<double>                          _allFrequence;                                   //所有频率
        QString                                  _component;                                      //自由度
        int                                      _componentIndex;                                 //自由度索引
        QStringList                              _allTransferPath;                                //节点对应的所有路径
        QHash<QString, QVector<QPair<double, double>>> _allTransferPathData;                      //所有路径对应的传递函数数据（QVector:按对应频率值顺序）（实部虚部）
        QHash<QString, QVector<QPair<double, double>>> _allPathForceData;                         //所有路径对应的输入力数据（实部虚部）
        QHash<QString, QVector<QPair<double, double>>> _allForceAndTransferRIdata;                //所有路径对应的输入力*传递函数数据（实部虚部）
        QHash<QString, QVector<QPair<double, double>>> _allForceAndTransferMPdata;                //所有路径对应的输入力*传递函数数据（幅值相位）
        QVector<QPair<double, double>>                 _forceAndTransferRISum;                    //输入力*传递函数每条路径实部、虚部求和数据
        QVector<QPair<double, double>>                 _forceAndTrandferMPSum;                    //输入力*传递函数每条路径求和后幅值、相位
        QString                                        _importMagnitudeCurveView;                 //导入视图（幅值曲线）
        QString                                        _importPhaseCurveView;                     //导入视图（相位曲线）
        QString                                        _importMagnitudeStudyCurveView;            //导入视图（幅值研究曲线）
        QString                                        _showPathContributionCurveView;            //显示视图（路径贡献柱状图）
        QString                                        _showTransferFunctionCurveView;            //显示视图（传递函数柱状图）
        QString                                        _showInputForceCurveView;                  //显示视图（输入力柱状图）
        QStringList                                    _showViewName;                             //显示的视图名称
        mPostDataTable*                            _dataTable{};			              //数据表窗口

        QHash<QString, QPointer<mPostPlotFloatWindow>> _plotFloatWindows;                            //浮动窗口映射
        QString                                  _activePlotViewName;                       //当前激活的视图名
        QHash<QString, PlotDataCache>            _plotDataCache;                            //曲线数据缓存
        QHash<PlotView::mPlotView*, QList<QwtPlotMarker*>> _barValueLabelMarkers;            //柱状图数值标签缓存

        PchReader::PchFile                      _transmitFile;                                            //传递函数数据
        PchReader::PchFile                      _forceFile;                                            //力文件数据

        // 每个 plotView 的 y=0 参考线 Marker，用于重绘时清理旧线
        // pair: QwtPlot 弱引用 + Marker 指针，防止 plot 先销毁后 detach 悬空崩溃
        QHash<PlotView::mPlotView*, QPair<QPointer<QwtPlot>, QwtPlotMarker*>> _zeroLineMarkers;
    };
}
