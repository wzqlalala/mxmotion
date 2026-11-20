#include "mPostTransferPathAnalysis.h"
#include "ui_mPostTransferPathAnalysis.h"
#include "UiMetrics.h"

#include <QApplication>
#include <QFileDialog>
#include <QTextCodec>
#include <QPointer>
#include <QToolButton>
#include <QTableWidgetItem>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QTimer>
#include <QFileInfo>
#include <algorithm>
#include <math.h>
#include <limits>

#include "mxr/view/REBUILD_PostRend.h"

#include "qwt_plot.h"
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>
#include <qwt_scale_draw.h>
#include <QPen>

#include "BarChartScaleDraw.h"
#include "mPlotView.h"
#include "mPlotLegendItem.h"
#include "mPostPlotFloatWindow.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mPostTransferPathAnalysisData.h"
#include "mPostDataTable.h"
#include "mDockLayoutManager.h"

#include "mMoveableLabel.h"
#include "mDraggableInfoLabel.h"
#include "mViewportModelManager.h"
#include "mOtherFunction.h"

//using namespace MIOFile;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MovableLabel;

/**
 * @brief 用于百分比排序的比较函数
 * @details 按QPair中第二个值（double类型百分比）的绝对值从大到小排序
 * @param pair1 第一个待比较的路径百分比数据对
 * @param pair2 第二个待比较的路径百分比数据对
 * @return 若pair1的绝对值大于pair2，则返回true
 */
bool comparePercents(const QPair<QString, double> &pair1, const QPair<QString, double> &pair2)
{
    return abs(pair1.second) > abs(pair2.second);
}

namespace PlotView
{
    class mPlotView;
}
namespace Common
{
    class mViewportManager;
}
using namespace PlotView;
using namespace MxFunctions;
//using namespace MViewBasic;

namespace PostWindow
{
    /**
     * @brief 构造函数：初始化传递路径分析界面
     * @param parent 父窗口指针
     * @details 初始化UI界面、设置下拉框选项、连接信号与槽、设置表格属性等
     */
    mPostTransferPathAnalysis::mPostTransferPathAnalysis(QWidget *parent)
        : QWidget(parent)
    {
        _ui = new Ui::mPostTransferPathAnalysis;
        _ui->setupUi(this);

        // 界面初始化
        // 暂时移除研究界面
        // _ui->tabWidget->removeTab(2);
        _ui->tabWidget->setCurrentIndex(0);

        // 初始化自由度（分量）下拉框选项：X/Y/Z/RX/RY/RZ
        QStringList comlist;
        comlist << "X" << "Y" << "Z" << "RX" << "RY" << "RZ";
        _ui->comboBox_component->addItems(comlist);

        // 初始化显示个数下拉框选项：1-20 和 All
        QStringList nums;
        for (int i = 1; i <= 20; i++)
        {
            nums.append(MxFunctions::mNumberToString(i));
        }
        nums.append("All");
        _ui->comboBox_showOrder->clear();
        _ui->comboBox_showOrder->addItems(nums);
        _ui->comboBox_showOrder->setCurrentText("10");

        // 初始化导入和显示区域的UI使能状态为禁用
        initialImportEnabledUI(false);
        initialShowEnabledUI(false);

        // 初始化表格设置
        _ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
        _ui->tableWidget->setColumnCount(2);                                  // 设置2列
        _ui->tableWidget->verticalHeader()->setHidden(true);                  // 隐藏垂直表头
        _ui->tableWidget->horizontalHeader()->setHidden(true);                // 隐藏水平表头
        _ui->tableWidget->resizeColumnsToContents();                          // 根据内容调整列宽
        QStringList headerText1;
        headerText1 << QString("名称") << QString("删除");
        _ui->tableWidget->setHorizontalHeaderLabels(headerText1);
        _ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);           // 第一列拉伸
        _ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 第二列根据内容调整
        _ui->tableWidget->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // 设置列表框选择模式为扩展选择（可多选）
        _ui->listWidget_mode->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // 连接信号与槽：文件选择和导入相关按钮
        connect(_ui->pushButton_selectTFFile, SIGNAL(clicked()), this, SLOT(chooseTFFile()));
        connect(_ui->pushButton_selectforceFile, SIGNAL(clicked()), this, SLOT(chooseForceFile()));
        connect(_ui->pushButton_loadFileOK, SIGNAL(clicked()), this, SLOT(loadFile()));
        connect(_ui->pushButton_loadResponseOK, SIGNAL(clicked()), this, SLOT(plotFrequencyResponseCurve()));

        // 连接信号与槽：文件路径文本变化时更新内部变量
        connect(_ui->lineEdit_tfFilePath, SIGNAL(textChanged(const QString &)), this, SLOT(filePathChanged(const QString &)));
        connect(_ui->lineEdit_forceFilePath, SIGNAL(textChanged(const QString &)), this, SLOT(filePathChanged(const QString &)));

        // 连接信号与槽：下拉框选择变化时触发相应处理
        connect(_ui->comboBox_case, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseSubcaseID(const QString&)));
        connect(_ui->comboBox_result, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseResultType(const QString&)));
        connect(_ui->comboBox_id, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseResponseID(const QString&)));

        // 连接信号与槽：显示相关按钮
        connect(_ui->pushButton_showOK, SIGNAL(clicked()), this, SLOT(plotShowCurve()));
        connect(_ui->pushButton_showData, SIGNAL(clicked()), this, SLOT(showDataTable()));
        connect(_ui->pushButton_loadDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        connect(_ui->pushButton_showDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        connect(_ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));

        //研究
        connect(_ui->pushButton_studyOK, SIGNAL(clicked()), this, SLOT(plotStudyCurve()));
        connect(_ui->pushButton_studyData, SIGNAL(clicked()), this, SLOT(showDataTable()));
        connect(_ui->pushButton_studyDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        // 连接返回按钮信号（已注释掉）
        //connect(_ui->pushButton_return1, SIGNAL(clicked()), this, SIGNAL(returnSig()));
        //connect(_ui->pushButton_return2, SIGNAL(clicked()), this, SIGNAL(returnSig()));
        //connect(_ui->pushButton_return3, SIGNAL(clicked()), this, SIGNAL(returnSig()));

        // 连接语言切换信号，并根据当前应用语言设置界面文本
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
        //connect(VMManager, SIGNAL(currentViewportChanged(QMdiSubWindow*)), this, SLOT(slot_postActiveViewChanged(QMdiSubWindow*)));

    }

    /**
     * @brief 析构函数：释放资源
     * @details 清理所有浮动窗口和UI对象，避免内存泄漏
     */
    mPostTransferPathAnalysis::~mPostTransferPathAnalysis()
    {
        // 清理 widget，但 QDockWidget 由 ADS 管理，不需要 close 或 delete
        for (auto it = _plotFloatWindows.begin(); it != _plotFloatWindows.end(); ++it) {
            it.value()->setParent(nullptr);  // 从 dock 中脱离
            delete it.value();              // 删除 mPostPlotFloatWindow 内部持有的 plotView
        }
        _plotFloatWindows.clear();

        // 清理 y=0 参考线 Marker（用 QPointer 判断 plot 是否仍存活，避免悬空崩溃）
        for (auto it = _zeroLineMarkers.begin(); it != _zeroLineMarkers.end(); ++it)
        {
            QwtPlotMarker* marker = it.value().second;
            QwtPlot* plot = it.value().first;
            if (marker)
            {
                if (plot)
                    marker->detach();
                delete marker;
            }
        }
        _zeroLineMarkers.clear();

        if (_ui != nullptr)
            delete _ui;
    }

    /**
     * @brief 设置面板拾取和信号状态（当前为空实现）
     * @param isshow 是否显示/启用
     */
    void mPostTransferPathAnalysis::setPanelPickAndSignal(bool isshow)
    {


    }

    /**
     * @brief 初始化导入区域的UI使能状态
     * @param isEnabled 是否启用导入区域控件
     * @details 控制工况、结果类型、响应ID等下拉框和按钮的可用状态，并清空相关数据
     */
    void mPostTransferPathAnalysis::initialImportEnabledUI(bool isEnabled)
    {
        _ui->comboBox_case->setEnabled(isEnabled);
        _ui->comboBox_case_2->setEnabled(isEnabled);
        _ui->comboBox_result->setEnabled(isEnabled);
        _ui->comboBox_id->setEnabled(isEnabled);
        _ui->comboBox_component->setEnabled(isEnabled);
        _ui->pushButton_loadResponseOK->setEnabled(isEnabled);

        _ui->comboBox_case->clear();
        _ui->comboBox_case_2->clear();
        _ui->comboBox_result->clear();
        _ui->comboBox_id->clear();
        _ui->comboBox_showFrequency->clear();
        _ui->comboBox_component->setCurrentText("X");
        _ui->comboBox_showOrder->setCurrentText("10");
    }

    /**
     * @brief 初始化显示区域的UI使能状态
     * @param isEnabled 是否启用显示区域控件
     * @details 控制显示频率、显示个数、显示类型等下拉框和按钮的可用状态
     */
    void mPostTransferPathAnalysis::initialShowEnabledUI(bool isEnabled)
    {
        _ui->comboBox_showFrequency->setEnabled(isEnabled);
        _ui->comboBox_showOrder->setEnabled(isEnabled);
        _ui->comboBox_showType->setEnabled(isEnabled);
        _ui->pushButton_showOK->setEnabled(isEnabled);
        _ui->pushButton_showData->setEnabled(isEnabled);
        _ui->pushButton_showDelete->setEnabled(isEnabled);
    }

    /**
     * @brief 获取指定频率下所有路径的投影百分比（考虑相位）
     * @param fre 目标频率值
     * @return 返回路径名与投影百分比（%）的列表
     * @details
     * 投影百分比计算公式：percent = mag * cos(abs(phaseSum - phase) / 180 * PI) / magSum * 100
     * 其中mag为路径的幅值，phase为路径相位，magSum和phaseSum为总和的幅值和相位
     */
    QList<QPair<QString, double>> mPostTransferPathAnalysis::getAllPathPercentsByFrequency(double fre)
    {
        if (_allFrequence.size() == 0 || _forceAndTrandferMPSum.size() == 0) return {};

        int index = _allFrequence.indexOf(fre);
        // 获取该频率下所有路径输入力*传递函数幅值、相位数据
        QHash<QString, QPair<double, double>> allPathMPData;
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecMPData = _allForceAndTransferMPdata[path];
            QPair<double, double> pairval = vecMPData[index];
            allPathMPData.insert(path, pairval);
        }

        // 获取该频率下输入力*传递函数幅值、相位求和数据
        QPair<double, double> allPathSumMPData = _forceAndTrandferMPSum[index];

        QList<QPair<QString, double>> allPathPercents;
        // 计算百分比（投影方式，考虑相位差）
        double mag, phase, magSum, phaseSum, percent;
        for (auto path : _allTransferPath)
        {
            QPair<double, double> pairMPdata = allPathMPData[path];
            mag = pairMPdata.first;
            phase = pairMPdata.second;
            magSum = allPathSumMPData.first;
            phaseSum = allPathSumMPData.second;

            // 投影百分比公式：幅值 * cos(相位差) / 总幅值 * 100
            percent = mag * cos(abs(phaseSum - phase) / 180 * M_PI) / magSum * 100;
            QPair<QString, double> pathPercent = qMakePair(path, percent);
            allPathPercents.append(pathPercent);
        }
        return allPathPercents;
    }

    /**
     * @brief 根据频率和路径列表获取传递函数的幅值
     * @param fre 目标频率值
     * @param pathNames 需要获取的路径名称列表
     * @return 返回各路径对应的传递函数幅值向量
     * @details 根据实部和虚部计算幅值：mag = sqrt(real^2 + imag^2)
     */
    QVector<double> mPostTransferPathAnalysis::getTransferMagnitudeByFrequencyAndPath(double fre, QStringList pathNames)
    {
        if (_allFrequence.size() == 0 || _allTransferPathData.size() == 0) return {};

        int index = _allFrequence.indexOf(fre);
        // 获取该频率下传递函数数据实部、虚部数据计算的幅值
        QVector<double> vecTransferMag;
        for (auto path : pathNames)
        {
            if (path != "Others")
            {
                QVector<QPair<double, double>> vecRIData = _allTransferPathData[path];
                QPair<double, double> pairval = vecRIData[index];
                double real = pairval.first;
                double imag = pairval.second;
                double mag = sqrt(real * real + imag * imag);
                vecTransferMag.push_back(mag);
            }
        }
        return vecTransferMag;
    }

    /**
     * @brief 根据频率和路径列表获取输入力的幅值
     * @param fre 目标频率值
     * @param pathNames 需要获取的路径名称列表
     * @return 返回各路径对应的输入力幅值向量
     * @details 根据实部和虚部计算幅值：mag = sqrt(real^2 + imag^2)
     */
    QVector<double> mPostTransferPathAnalysis::getForceMagnitudeByFrequencyAndPath(double fre, QStringList pathNames)
    {
        if (_allFrequence.size() == 0 || _allPathForceData.size() == 0) return {};

        int index = _allFrequence.indexOf(fre);
        // 获取该频率下输入力数据实部、虚部数据计算的幅值
        QVector<double> vecForceMag;
        for (auto path : pathNames)
        {
            if (path != "Others")
            {
                QVector<QPair<double, double>> vecRIData = _allPathForceData[path];
                QPair<double, double> pairval = vecRIData[index];
                double real = pairval.first;
                double imag = pairval.second;
                double mag = sqrt(real * real + imag * imag);
                vecForceMag.push_back(mag);
            }
        }
        return vecForceMag;
    }

    /**
     * @brief 获取指定频率下的总幅值
     * @param fre 目标频率值
     * @return 返回该频率下的总幅值
     */
    double mPostTransferPathAnalysis::getMagSum(double fre)
    {
        int index = _allFrequence.indexOf(fre);
        return _forceAndTrandferMPSum.at(index).first;
    }

    /**
     * @brief 根据百分比计算投影幅值
     * @param fre 目标频率值
     * @param percents 各路径的百分比列表
     * @return 返回各路径投影幅值列表
     * @details 公式：magProject = percent * totalMag * 0.01
     */
    QVector<double> mPostTransferPathAnalysis::getMagProjectByFre(double fre, QVector<double> percents)
    {
        if (_forceAndTrandferMPSum.size() == 0 || _allFrequence.size() == 0) return {};
        int index = _allFrequence.indexOf(fre);
        double totalMag = _forceAndTrandferMPSum.at(index).first;

        QVector<double> vecMagProject;
        for (double percent : percents)
        {
            double magProject = percent * totalMag * 0.01;
            vecMagProject.push_back(magProject);
        }
        return vecMagProject;
    }

    /**
     * @brief 获取路径的简短名称（提取单元ID和自由度信息）
     * @param pathname 完整路径名称（格式如"path<>elementID"）
     * @return 提取后的简短名称
     * @details 路径名称格式包含"<>"分隔符，提取后半部分作为显示名称
     */
    QString mPostTransferPathAnalysis::getShortPathName(QString pathname)
    {
        QStringList pathparts = pathname.split("<>");
        if (pathparts.size() < 2) return{};
        QString elestr = pathparts[1];
        if (elestr.isEmpty()) return {};
        return elestr;
    }

    /**
     * @brief 创建自定义可移动图例
     * @param viewname 视图名称，用于查找对应的浮动窗口
     * @param legendInformation 图例文本内容列表
     * @param backColor 图例背景色，默认为透明
     * @param textColor 图例字体颜色，默认为黑色
     * @details 在指定的浮动窗口对应的 plotView 上叠加显示可移动图例。
     *          图例的父控件设为对应的 mPostPlotFloatWindow，使每个浮动窗口的图例
     *          在各自窗口坐标系中独立定位，避免多个窗口的图例在顶层主窗口中重叠。
     */
    void mPostTransferPathAnalysis::createCustomLegend(QString viewname, QStringList legendInformation, QColor backColor, QColor textColor)
    {
        // 设置自定义可移动图例
        if (!_plotFloatWindows.contains(viewname))
        {
            return;
        }
        mPostPlotFloatWindow* floatWin = _plotFloatWindows[viewname];
        if (floatWin == nullptr)
        {
            return;
        }
        PlotView::mPlotView* plotView = floatWin->getPlotView();
        if (!plotView) return;

        QwtPlot* qwtPlot = plotView->getQwtPlot();
        if (!qwtPlot) return;

        // 每个浮动窗口缓存一个可拖动信息标签，parent 设为对应窗口，
        // 使各窗口图例在各自坐标系中独立定位，避免重叠
        static QHash<mPostPlotFloatWindow*, QPointer<mDraggableInfoLabel>> infoLabels;
        QPointer<mDraggableInfoLabel>& label = infoLabels[floatWin];
        bool isNewLabel = false;
        if (!label)
        {
            label = new mDraggableInfoLabel(floatWin);
            isNewLabel = true;
        }

        // 支持自定义背景色和字体颜色
        QString styleSheet = QString("QLabel { border: 1px solid black; background-color: %1; color: %2; padding: 4px; }")
            .arg(backColor.name(QColor::HexArgb))
            .arg(textColor.name());
        label->setStyleSheet(styleSheet);
        label->setTextFormat(Qt::PlainText);

        QString text = legendInformation.join("\n");
        label->setText(text);
        label->adjustSize();

        // 首次创建时：默认定位到画布右上角（floatWin 坐标系）
        if (isNewLabel)
        {
            QWidget* canvas = qwtPlot->canvas();
            QPoint canvasGlobalTopLeft = canvas->mapToGlobal(QPoint(0, 0));
            constexpr int margin = 10;
            int globalX = canvasGlobalTopLeft.x() + canvas->width() - label->width() - margin;
            int globalY = canvasGlobalTopLeft.y() + margin;
            QPoint winPos = floatWin->mapFromGlobal(QPoint(globalX, globalY));

            // 确保图例不超出 floatWin 边界
            if (winPos.x() < 0)
                winPos.setX(margin);
            if (winPos.x() + label->width() > floatWin->width())
                winPos.setX(floatWin->width() - label->width() - margin);
            if (winPos.y() < 0)
                winPos.setY(margin);

            label->move(winPos);
        }

        label->raise();
        label->show();
    }

    /**
     * @brief 设置路径贡献图例（投影方式）
     * @param showFre 显示频率
     * @param showPercents 各路径的百分比
     * @param showPaths 各路径名称
     * @details 生成包含总响应和各路径贡献百分比的图例文本
     */
    void mPostTransferPathAnalysis::setPathContributionLegend(double showFre, QVector<double> showPercents, QStringList showPaths)
    {
        QStringList pathContributionLegend;
        // 获取百分比总幅值幅值投影
        QVector<double> vecMagProjected = getMagProjectByFre(showFre, showPercents);
        double magSum = getMagSum(showFre);
        QString val = MxFunctions::mNumberToString(magSum, 'e', 2);
        QString str = val + ":" + "Response" + " " + ":[100.00%]";
        pathContributionLegend.push_back(str);
        QString strMag, strPath, strPercent;
        QString legendStr;
        for (int i = 0; i < showPaths.size(); i++)
        {
            if (showPaths[i] != "Others")
            {
                strMag = MxFunctions::mNumberToString(vecMagProjected[i], 'e', 2);
                strPath = getShortPathName(showPaths[i]);
                strPercent = MxFunctions::mNumberToString(showPercents[i], 'f', 2);
                legendStr = strMag + ":" + strPath + " " + ":" + "[" + strPercent + "%]";
                pathContributionLegend.push_back(legendStr);
            }
        }
        if (showPaths.contains("Others"))
        {
            int index = showPaths.indexOf("Others");
            strMag = MxFunctions::mNumberToString(vecMagProjected[index], 'e', 2);
            strPercent = MxFunctions::mNumberToString(showPercents[index], 'f', 2);
            legendStr = strMag + ":" + "Others" + " " + ":" + "[" + strPercent + "%]";
            pathContributionLegend.push_back(legendStr);
        }
        createCustomLegend(_showPathContributionCurveView, pathContributionLegend);
    }

    /**
     * @brief 设置路径贡献图例（幅值方式）
     * @param showFre 显示频率
     * @param showPercents 各路径的百分比
     * @param showPaths 各路径名称
     * @details 与投影方式类似，但图例中不显示百分比符号
     */
    void mPostTransferPathAnalysis::setMagPathContributionLegend(double showFre, QVector<double> showPercents, QStringList showPaths)
    {
        QStringList pathContributionLegend;
        // 获取百分比总幅值幅值投影
        QVector<double> vecMagProjected = getMagProjectByFre(showFre, showPercents);
        double magSum = getMagSum(showFre);
        QString val = MxFunctions::mNumberToString(magSum, 'e', 2);
        QString str = val + ":" + "Response";
        pathContributionLegend.push_back(str);
        QString strMag, strPath;
        QString legendStr;
        for (int i = 0; i < showPaths.size(); i++)
        {
            if (showPaths[i] != "Others")
            {
                strMag = MxFunctions::mNumberToString(vecMagProjected[i], 'e', 2);
                strPath = getShortPathName(showPaths[i]);
                legendStr = strMag + ":" + strPath ;
                pathContributionLegend.push_back(legendStr);
            }
        }
        if (showPaths.contains("Others"))
        {
            int index = showPaths.indexOf("Others");
            strMag = MxFunctions::mNumberToString(vecMagProjected[index], 'e', 2);
            legendStr = strMag + ":" + "Others";
            pathContributionLegend.push_back(legendStr);
        }
        createCustomLegend(_showPathContributionCurveView, pathContributionLegend);
    }

    /**
     * @brief 设置传递函数图例
     * @param showMags 各路径的传递函数幅值
     * @param showPaths 各路径名称
     * @details 为传递函数柱状图窗口创建图例
     */
    void mPostTransferPathAnalysis::setTransferFunctionLegend(QVector<double> showMags, QStringList showPaths)
    {
        QStringList TransferFunctionLegend;
        QString strMag, strPath;
        QString legendStr;
        for (int i = 0; i < showPaths.size(); i++)
        {
            if (showPaths[i] != "Others")
            {
                strMag = MxFunctions::mNumberToString(showMags[i], 'e', 2);
                strPath = getShortPathName(showPaths[i]);
                legendStr = strMag + ":" + strPath;
                TransferFunctionLegend.push_back(legendStr);
            }
        }
        createCustomLegend(_showTransferFunctionCurveView, TransferFunctionLegend);
    }

    /**
     * @brief 设置输入力图例
     * @param showMags 各路径的输入力幅值
     * @param showPaths 各路径名称
     * @details 为输入力柱状图窗口创建图例
     */
    void mPostTransferPathAnalysis::setInputForceLegend(QVector<double> showMags, QStringList showPaths)
    {
        QStringList forceLegend;
        QString strMag, strPath;
        QString legendStr;
        for (int i = 0; i < showPaths.size(); i++)
        {
            if (showPaths[i] != "Others")
            {
                strMag = MxFunctions::mNumberToString(showMags[i], 'e', 2);
                strPath = getShortPathName(showPaths[i]);
                legendStr = strMag + ":" + strPath;
                forceLegend.push_back(legendStr);
            }
        }
        createCustomLegend(_showInputForceCurveView, forceLegend);
    }

    /**
     * @brief 获取指定频率下所有路径的幅值百分比（不考虑相位）
     * @param fre 目标频率值
     * @return 返回路径名与幅值百分比（%）的列表
     * @details 幅值百分比 = 路径幅值 / 总幅值 * 100
     */
    QList<QPair<QString, double>> mPostTransferPathAnalysis::getAllPathMagnitudePercentsByFrequency(double fre)
    {
        if (_allFrequence.size() == 0 || _forceAndTrandferMPSum.size() == 0) return {};

        int index = _allFrequence.indexOf(fre);
        // 获取该频率下所有路径输入力*传递函数幅值、相位数据
        QHash<QString, QPair<double, double>> allPathMPData;
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecMPData = _allForceAndTransferMPdata[path];
            QPair<double, double> pairval = vecMPData[index];
            allPathMPData.insert(path, pairval);
        }

        // 获取该频率下输入力*传递函数幅值、相位求和数据
        QPair<double, double> allPathSumMPData = _forceAndTrandferMPSum[index];

        QList<QPair<QString, double>> allPathMagPercents;
        // 计算百分比（仅按幅值比例，不考虑相位）
        double mag, totalMag, magPercent;
        for (auto path : _allTransferPath)
        {
            QPair<double, double> pairMPdata = allPathMPData[path];
            mag = pairMPdata.first;
            totalMag = allPathSumMPData.first;
            magPercent = mag / totalMag * 100;
            QPair<QString, double> pathMagPercent = qMakePair(path, magPercent);
            allPathMagPercents.append(pathMagPercent);
        }
        return allPathMagPercents;
    }

    /**
     * @brief 计算未显示路径的"Others"幅值百分比
     * @param fre 目标频率值
     * @param showPaths 当前已显示的路径列表
     * @return 返回Others的幅值百分比
     * @details 将所有未显示路径的实部和虚部分别求和，然后计算幅值与总幅值的比值
     */
    double mPostTransferPathAnalysis::getMagPercentOthers(double fre, QStringList showPaths)
    {
        if (_allFrequence.size() == 0 || _allForceAndTransferRIdata.size() == 0) return {};
        int index = _allFrequence.indexOf(fre);

        // 获取未显示路径实部、虚部求和数据
        double sumReal = 0.0;
        double sumImag = 0.0;
        for (auto path : _allTransferPath)
        {
            if (!showPaths.contains(path))
            {
                QVector<QPair<double, double>> vecRIData = _allForceAndTransferRIdata[path];
                QPair<double, double> pairval = vecRIData[index];
                sumReal += pairval.first;
                sumImag += pairval.second;
            }
        }

        // 当前频率路径总幅值
        double totalMag = _forceAndTrandferMPSum[index].first;
        double mag = sqrt(sumReal * sumReal + sumImag * sumImag);
        double result = mag / totalMag * 100;
        return result;
    }

    /**
     * @brief 加载文件主函数
     * @details 检查文件路径是否合法，读取传递函数和输入力文件，验证数据格式，并初始化导入界面
     */
    void mPostTransferPathAnalysis::loadFile()
    {
        if (_tfFilePath.isEmpty() || _forceFilePath.isEmpty())
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("未选择传递函数或输入力文件！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("Unselected transfer function or input force file!"), true);
            }
            return;
        }
        else if (!_tfFilePath.contains(".pch") || !_forceFilePath.contains(".pch"))
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("文件格式错误，请导入 .pch 文件！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("File format error, please import .pch file!"), true);
            }
            return;
        }


        //读取传递函数和力文件并将其保存
        _transmitFile = PchReader::PchReader().readFile(std::filesystem::u8path(_tfFilePath.toStdString()));
        _forceFile = PchReader::PchReader().readFile(std::filesystem::u8path(_forceFilePath.toStdString()));

        saveFileData();

         // 读取传递函数文件内容
         //bool isTFOpen = readTransferFuntionData(_tfFilePath);

         //// 读取输入力文件内容
         //bool isForceOpen = readForceData(_forceFilePath);

         // 判断文件格式是否正确
        if (_transmitFile.blocks.size() && _forceFile.blocks.size())
        {
            if (_transferPathAnalysisData == nullptr || _transferPathAnalysisData->_allResponseData.isEmpty() ||
                _transferPathAnalysisData->_allTransferFunctionData.isEmpty() || _transferPathAnalysisData->_allForceData.isEmpty())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Transfer path file data error!"), true);
                }
                return;
            }
            else if (_transferPathAnalysisData->_allResponseData[0]->_outputType == -1 ||
                _transferPathAnalysisData->_allTransferFunctionData[0]->_outputType == -1 ||
                _transferPathAnalysisData->_allForceData[0]->_outputType == -1)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Transfer path file data error!"), true);
                }
                return;
            }
            QFileInfo tfInfo(_tfFilePath);
            QFileInfo forceInfo(_forceFilePath);
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量文件导入成功：%1 / %2")
                .arg(tfInfo.fileName()).arg(forceInfo.fileName()), true);
            initialImportEnabledUI(true);
        }
        else
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递路径贡献量文件打开失败！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("Transfer path file open failed!"), true);
            }
            return;
        }

        // 导入界面初始化
        // 获取所有工况id并填充到下拉框
        QVector<int> subcaseIDs = _transferPathAnalysisData->getAllSubcaseID();
        subcaseIDs = _transferPathAnalysisData->removeDuplicates(subcaseIDs);
        QStringList listID;
        for (int id : subcaseIDs)
        {
            QString strCase = "Subcase ";
            listID.append(strCase + MxFunctions::mNumberToString(id));
        }
        _ui->comboBox_case->blockSignals(true);
        _ui->comboBox_case->clear();
        _ui->comboBox_case->blockSignals(false);
        _ui->comboBox_case->addItems(listID);

        _ui->comboBox_case_2->blockSignals(true);
        _ui->comboBox_case_2->clear();
        _ui->comboBox_case_2->blockSignals(false);
        _ui->comboBox_case_2->addItems(listID);

        // 显示界面

        // 初始化研究界面
        QRegExp regx("[0-9,.]+$");
        QValidator *validator = new QRegExpValidator(regx, _ui->lineEdit_excludeRatio);
        _ui->lineEdit_excludeRatio->setValidator(validator);
        _ui->lineEdit_excludeRatio->setText("100");
    }

    /**
     * @brief 传递函数文件路径变化时更新内部变量
     * @param text 新的文件路径文本
     */
    void mPostTransferPathAnalysis::tfFilePathChanged(const QString& text)
    {
        _tfFilePath = text;
    }

    /**
     * @brief 输入力文件路径变化时更新内部变量
     * @param text 新的文件路径文本
     */
    void mPostTransferPathAnalysis::forceFilePathChanged(const QString& text)
    {
        _forceFilePath = text;
    }

    /**
     * @brief 选择工况ID时触发
     * @param caseID 选中的工况文本（如"Subcase 1"）
     * @details 解析工况ID，并根据该ID获取对应的结果类型列表填充到结果类型下拉框
     */
    void mPostTransferPathAnalysis::chooseSubcaseID(const QString& caseID)
    {
        QString strCase = caseID;
        if (strCase.contains(" "))
        {
            QStringList strList = strCase.split(" ");
            if (strList.size() == 2)
            {
                _subcaseID = strList.at(1).toInt();
            }
        }
        // 获取工况ID对应的结果类型
        QStringList results = _transferPathAnalysisData->getResultTypeBySubcaseID(_subcaseID);
        results = _transferPathAnalysisData->removeDuplicates(results);
        _ui->comboBox_result->blockSignals(true);
        _ui->comboBox_result->clear();
        _ui->comboBox_result->blockSignals(false);
        _ui->comboBox_result->addItems(results);
    }

    /**
     * @brief 选择结果类型时触发
     * @param resultType 选中的结果类型文本
     * @details 根据结果类型和当前工况ID获取响应ID列表并填充下拉框
     */
    void mPostTransferPathAnalysis::chooseResultType(const QString& resultType)
    {
        // 根据结果类型及工况ID获取响应ID
        _resultType = resultType;
        QVector<int> ids = _transferPathAnalysisData->getResponseIDBySubcaseIDAndResultType(_subcaseID, _resultType);
        ids = _transferPathAnalysisData->removeDuplicates(ids);
        _ui->comboBox_id->blockSignals(true);
        _ui->comboBox_id->clear();
        _ui->comboBox_id->blockSignals(false);

        QStringList idlist;
        for (int id : ids)
        {
            idlist.append(MxFunctions::mNumberToString(id));
        }
        _ui->comboBox_id->addItems(idlist);
    }

    /**
     * @brief 选择响应ID时触发
     * @param responseID 选中的响应ID文本
     * @details 更新响应ID变量，并获取该响应对应的所有频率填充到显示频率下拉框
     */
    void mPostTransferPathAnalysis::chooseResponseID(const QString& responseID)
    {
        _responseID = responseID.toInt();

        // 显示、研究界面显示频率，研究界面路径选择，与导入下拉框联动
        // 获取所有频率
        _allFrequence = _transferPathAnalysisData->getAllFrequency(_subcaseID, _resultType,_responseID);
        QString frequence;
        QStringList freList;
        for (double fre : _allFrequence)
        {
            frequence = MxFunctions::mNumberToString(fre);
            freList.append(frequence);
        }

        // 显示部分显示频率
        _ui->comboBox_showFrequency->clear();
        _ui->comboBox_showFrequency->addItems(freList);

        //研究部分显示频率
        _ui->comboBox_studyFrequencyMin->clear();
        _ui->comboBox_studyFrequencyMax->clear();
        _ui->comboBox_studyFrequencyMin->addItems(freList);
        _ui->comboBox_studyFrequencyMax->addItems(freList);
        _ui->comboBox_studyFrequencyMin->setCurrentIndex(0);
        _ui->comboBox_studyFrequencyMax->setCurrentIndex(freList.size() - 1);

        //研究部分路径选择
        QStringList pathNames = _transferPathAnalysisData->getAllNodePathName(_resultType, _responseID);
        pathNames=_transferPathAnalysisData->removeDuplicates(pathNames);


        _ui->listWidget_mode->clear();
        _ui->listWidget_mode->addItems(pathNames);

    }

    /**
     * @brief 读取传递函数数据文件（.pch格式）
     * @param filePath 文件路径
     * @return 读取成功返回true，失败返回false
     * @details
     * 解析.pch格式文件，读取各路径的传递函数数据。
     * 支持按频率和按节点两种结果类型，支持MAGNITUDE-PHASE和REAL-IMAGINARY两种输出格式。
     * 数据组织结构：LABEL->类型->输出格式->SUBCASE ID->频率/节点ID->多行数据值
     */
    bool mPostTransferPathAnalysis::readTransferFuntionData(QString filePath)
    {
        // 读取文件内容
        QTextCodec *codeC = QTextCodec::codecForName("GBK");
        QFile file(filePath);
        QFileInfo fileinfo(filePath);

        // 以读的方式打开文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;

        _transferPathAnalysisData = new mPostTransferPathAnalysisData();
        QString currentline;
        while (!file.atEnd())
        {
            currentline = file.readLine().simplified();
            // 读取传递函数数据
            if (currentline.contains("LABEL"))
            {
                QPointer<mPostTransferFunctionData> transferData = new mPostTransferFunctionData();
                QStringList linelist = currentline.split("=");
                QString temp;
                if (linelist.size() == 2)
                {
                    // 传递路径：提取引号中的内容
                    temp = linelist.at(1);
                    int startIndex = temp.indexOf('"');
                    int endIndex = temp.lastIndexOf('"');
                    temp = temp.mid(startIndex + 1, endIndex - startIndex - 1);
                    if (!temp.contains("<>")) return false;
                    transferData->_transferPath = temp;
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("$"))
                {
                    // 类型
                    transferData->_type = currentline.remove('$');
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("MAGNITUDE-PHASE"))
                {
                    // 输出类型：幅值-相位
                    transferData->_outputType = 1;
                }
                else if (currentline.contains("REAL-IMAGINARY"))
                {
                    // 输出类型：实部-虚部
                    transferData->_outputType = 2;
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("SUBCASE ID"))
                {
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        // 工况ID
                        temp = linelist.at(1);
                        transferData->_subcaseID = temp.toInt();
                    }
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("$FREQUENCY"))
                {
                    // 结果类型为按频率
                    transferData->_resultType = 1;
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        // 频率
                        double fre = linelist.at(1).toDouble();
                        transferData->_frequency.push_back(fre);
                    }
                }
                else if (currentline.contains("$POINT ID"))
                {
                    // 结果类型为按节点
                    transferData->_resultType = 2;
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        // 节点ID
                        int id = linelist.at(1).simplified().split(" ").at(0).toInt();
                        transferData->_nodeId.push_back(id);
                    }
                }
                currentline = file.readLine().simplified();
                while(currentline.contains("G"))
                {
                    // 按频率结果类型
                    if (transferData->_resultType == 1)
                    {
                        QList<double> magRealVal;
                        QList<double> phaseImagVal;
                        // 存储数据
                        // 第一行：节点ID + 3个数据值
                        linelist = currentline.split(" ");
                        if (linelist.size() == 5)
                        {
                            // 节点
                            transferData->_nodeId.push_back(linelist.at(0).toInt());
                            for (int j = 2; j < 5; j++)
                            {
                                magRealVal.append(linelist.at(j).toDouble());
                            }
                        }
                        // 第二行：3个数据值
                        currentline = file.readLine().simplified();
                        linelist = currentline.split(" ");
                        if (linelist.size() == 4)
                        {
                            for (int j = 1; j < 4; j++)
                            {
                                magRealVal.append(linelist.at(j).toDouble());
                            }
                        }
                        // 第三、四行：相位/虚部数据（每行3个）
                        for (int i = 0; i < 2; i++)
                        {
                            currentline = file.readLine().simplified();
                            linelist = currentline.split(" ");
                            for (int j = 1; j < 4; j++)
                            {
                                phaseImagVal.append(linelist.at(j).toDouble());
                            }
                        }
                        if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                        {
                            // 数据存储：6个分量（X/Y/Z/RX/RY/RZ）
                            QHash<int, QPair<double, double>> dataMap;
                            for (int i = 0; i < 6; i++)
                            {
                                dataMap.insert(i,QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                            }
                            transferData->_allComponentValue.push_back(dataMap);
                        }
                        else
                        {
                            return false;
                        }
                        currentline = file.readLine().simplified();
                    }
                    // 按节点结果类型
                    if (transferData->_resultType == 2)
                    {
                        QList<double> magRealVal;
                        QList<double> phaseImagVal;
                        // 存储数据
                        // 第一行：频率 + 3个数据值
                        linelist = currentline.simplified().split(" ");
                        if (linelist.size() == 5)
                        {
                            // 节点
                            transferData->_frequency.push_back(linelist.at(0).toDouble());
                            for (int j = 2; j < 5; j++)
                            {
                                magRealVal.append(linelist.at(j).toDouble());
                            }
                        }
                        // 第二行
                        currentline = file.readLine().simplified();
                        linelist = currentline.split(" ");
                        if (linelist.size() == 4)
                        {
                            for (int j = 1; j < 4; j++)
                            {
                                magRealVal.append(linelist.at(j).toDouble());
                            }
                        }
                        // 第三、四行
                        for (int i = 0; i < 2; i++)
                        {
                            currentline = file.readLine().simplified();
                            linelist = currentline.split(" ");
                            for (int j = 1; j < 4; j++)
                            {
                                phaseImagVal.append(linelist.at(j).toDouble());
                            }
                        }
                        if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                        {
                            QHash<int, QPair<double, double>> dataMap;
                            for (int i = 0; i < 6; i++)
                            {
                                dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                            }
                            transferData->_allComponentValue.push_back(dataMap);
                        }
                        else
                        {
                            return false;
                        }
                        currentline = file.readLine().simplified();
                    }
                }
                _transferPathAnalysisData->_allTransferFunctionData.push_back(transferData);
            }
        }
        file.close();
        return true;
    }

    /**
     * @brief 读取输入力数据文件（.pch格式）
     * @param filePath 文件路径
     * @return 读取成功返回true，失败返回false
     * @details
     * 解析.pch格式文件，读取响应数据和输入力数据。
     * 输入力数据分为单元力（ELEMENT FORCES）和节点力（GRID POINT FORCE BALANCE）两种类型。
     * 同时包含响应数据（非FORCE类型）的读取。
     */
    bool mPostTransferPathAnalysis::readForceData(QString filePath)
    {
        // 读取文件内容
        QTextCodec *codeC = QTextCodec::codecForName("GBK");
        QFile file(filePath);
        QFileInfo fileinfo(filePath);

        // 以读的方式打开文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        if (_transferPathAnalysisData == nullptr) return false;

        QString currentline;
        while (!file.atEnd())
        {
            QStringList linelist;
            currentline = file.readLine().simplified();
            if (currentline.contains("LABEL"))
            {
                currentline = file.readLine().simplified();
                if (!currentline.contains("FORCE"))
                {
                    // 读取响应数据（非力数据，用于响应对比）
                    QPointer<mPostResponseData> responseData = new mPostResponseData();
                    QString temp;
                    if (currentline.contains("$"))
                    {
                        // 类型
                        responseData->_type = currentline.remove('$');
                    }
                    currentline = file.readLine().simplified();
                    if (currentline.contains("MAGNITUDE-PHASE"))
                    {
                        // 输出类型：幅值-相位
                        responseData->_outputType = 1;
                    }
                    else if (currentline.contains("REAL-IMAGINARY"))
                    {
                        // 输出类型：实部-虚部
                        responseData->_outputType = 2;
                    }
                    currentline = file.readLine().simplified();
                    linelist = currentline.split("=");
                    if (currentline.contains("SUBCASE ID"))
                    {
                        if (linelist.size() == 2)
                        {
                            // 工况ID
                            temp = linelist.at(1).simplified();
                            responseData->_subcaseID = temp.toInt();
                        }
                    }
                    currentline = file.readLine().simplified();
                    if (currentline.contains("$FREQUENCY"))
                    {
                        // 结果类型为按频率
                        responseData->_resultType = 1;
                        linelist = currentline.split("=");
                        if (linelist.size() == 2)
                        {
                            // 频率
                            double fre = linelist.at(1).simplified().toDouble();
                            responseData->_frequency.push_back(fre);
                        }
                    }
                    else if (currentline.contains("$POINT ID"))
                    {
                        // 结果类型为按节点
                        responseData->_resultType = 2;
                        linelist = currentline.split("=");
                        if (linelist.size() == 2)
                        {
                            // 节点ID
                            int id = linelist.at(1).simplified().split(" ").at(0).toInt();
                            responseData->_nodeId.push_back(id);
                        }
                    }
                    currentline = file.readLine().simplified();
                    while (currentline.contains("G"))
                    {
                        // 按频率结果类型
                        if (responseData->_resultType == 1)
                        {
                            QList<double> magRealVal;
                            QList<double> phaseImagVal;
                            // 存储数据
                            // 第一行
                            linelist = currentline.simplified().split(" ");
                            if (linelist.size() == 5)
                            {
                                // 节点
                                responseData->_nodeId.push_back(linelist.at(0).toInt());
                                for (int j = 2; j < 5; j++)
                                {
                                    magRealVal.append(linelist.at(j).toDouble());
                                }
                            }
                            // 第二行
                            currentline = file.readLine().simplified();
                            linelist = currentline.simplified().split(" ");
                            if (linelist.size() == 4)
                            {
                                for (int j = 1; j < 4; j++)
                                {
                                    magRealVal.append(linelist.at(j).toDouble());
                                }
                            }
                            // 第三、四行
                            for (int i = 0; i < 2; i++)
                            {
                                currentline = file.readLine().simplified();
                                linelist = currentline.split(" ");
                                for (int j = 1; j < 4; j++)
                                {
                                    phaseImagVal.append(linelist.at(j).toDouble());
                                }
                            }
                            if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                            {
                                QHash<int, QPair<double, double>> dataMap;
                                for (int i = 0; i < 6; i++)
                                {
                                    dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                                }
                                responseData->_allComponentValue.push_back(dataMap);
                            }
                            else
                            {
                                return false;
                            }
                            currentline = file.readLine().simplified();
                        }
                        // 按节点结果类型
                        if (responseData->_resultType == 2)
                        {
                            QList<double> magRealVal;
                            QList<double> phaseImagVal;
                            // 存储数据
                            // 第一行
                            linelist = currentline.simplified().split(" ");
                            if (linelist.size() == 5)
                            {
                                // 节点
                                responseData->_frequency.push_back(linelist.at(0).toDouble());
                                for (int j = 2; j < 5; j++)
                                {
                                    magRealVal.append(linelist.at(j).toDouble());
                                }
                            }
                            // 第二行
                            currentline = file.readLine().simplified();
                            linelist = currentline.split(" ");
                            if (linelist.size() == 4)
                            {
                                for (int j = 1; j < 4; j++)
                                {
                                    magRealVal.append(linelist.at(j).toDouble());
                                }
                            }
                            // 第三、四行
                            for (int i = 0; i < 2; i++)
                            {
                                currentline = file.readLine().simplified();
                                linelist = currentline.split(" ");
                                for (int j = 1; j < 4; j++)
                                {
                                    phaseImagVal.append(linelist.at(j).toDouble());
                                }
                            }
                            if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                            {
                                QHash<int, QPair<double, double>> dataMap;
                                for (int i = 0; i < 6; i++)
                                {
                                    dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                                }
                                responseData->_allComponentValue.push_back(dataMap);
                            }
                            else
                            {
                                return false;
                            }
                            currentline = file.readLine().simplified();
                        }
                    }
                    _transferPathAnalysisData->_allResponseData.push_back(responseData);
                }
                if (currentline.contains("FORCE"))
                {
                    QPointer<mPostForceData> forceData = new mPostForceData();
                    // 读取单元力数据
                    if (currentline == "$ELEMENT FORCES")
                    {
                        forceData->_forceType = 1;
                        currentline = file.readLine().simplified();
                        if (currentline.contains("MAGNITUDE-PHASE"))
                        {
                            // 输出类型：幅值-相位
                            forceData->_outputType = 1;
                        }
                        else if (currentline.contains("REAL-IMAGINARY"))
                        {
                            // 输出类型：实部-虚部
                            forceData->_outputType = 2;
                        }
                        currentline = file.readLine().simplified();
                        linelist = currentline.split("=");
                        if (currentline.contains("SUBCASE ID"))
                        {
                            if (linelist.size() == 2)
                            {
                                // 工况ID
                                forceData->_subcaseID = linelist.at(1).toInt();
                            }
                        }
                        currentline = file.readLine().simplified();
                        linelist = currentline.split("=");
                        if (currentline.contains("ELEMENT TYPE"))
                        {
                            if (linelist.size() == 2)
                            {
                                // 单元类型
                                forceData->_elemenType = linelist.at(1).simplified().split(" ").at(0).toInt();
                            }
                        }
                        currentline = file.readLine().simplified();
                        if (currentline.contains("$FREQUENCY"))
                        {
                            // 结果类型为按频率
                            forceData->_resultType = 1;
                            linelist = currentline.split("=");
                            if (linelist.size() == 2)
                            {
                                // 频率
                                double fre = linelist.at(1).toDouble();
                                forceData->_frequency.push_back(fre);
                            }
                        }
                        else if (currentline.contains("$ELEMENT ID"))
                        {
                            // 结果类型为按节点
                            forceData->_resultType = 2;
                            linelist = currentline.split("=");
                            if (linelist.size() == 2)
                            {
                                // 单元id
                                int id = linelist.at(1).simplified().split(" ").at(0).toInt();
                                forceData->_elementId.push_back(id);
                            }
                        }
                        currentline = file.readLine().simplified();
                        while (!currentline.contains("TITLE"))
                        {
                            // 避免最后一行空行造成死循环
                            if (currentline.isEmpty()) break;
                            // 按频率结果类型
                            if (forceData->_resultType == 1)
                            {
                                QList<double> magRealVal;
                                QList<double> phaseImagVal;
                                // 存储数据
                                // 第一行
                                linelist = currentline.split(" ");
                                if (linelist.size() == 4)
                                {
                                    // 单元ID
                                    forceData->_elementId.push_back(linelist.at(0).toInt());
                                    for (int j = 1; j < 4; j++)
                                    {
                                        magRealVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                // 第二行
                                currentline = file.readLine().simplified();
                                linelist = currentline.split(" ");
                                if (linelist.size() == 4)
                                {
                                    for (int j = 1; j < 4; j++)
                                    {
                                        magRealVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                // 第三、四行
                                for (int i = 0; i < 2; i++)
                                {
                                    currentline = file.readLine().simplified();
                                    linelist = currentline.split(" ");
                                    for (int j = 1; j < 4; j++)
                                    {
                                        phaseImagVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                                {
                                    // 数据存储
                                    QHash<int, QPair<double, double>> dataMap;
                                    for (int i = 0; i < 6; i++)
                                    {
                                        dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                                    }
                                    forceData->_allComponentValue.push_back(dataMap);
                                }
                                else
                                {
                                    return false;
                                }
                                currentline = file.readLine().simplified();
                            }
                            // 按ID结果类型
                            if (forceData->_resultType == 2)
                            {
                                QList<double> magRealVal;
                                QList<double> phaseImagVal;
                                // 存储数据
                                // 第一行
                                linelist = currentline.simplified().split(" ");
                                if (linelist.size() == 4)
                                {
                                    // 频率
                                    forceData->_frequency.push_back(linelist.at(0).toDouble());
                                    for (int j = 1; j < 4; j++)
                                    {
                                        magRealVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                // 第二行
                                currentline = file.readLine().simplified();
                                linelist = currentline.split(" ");
                                if (linelist.size() == 4)
                                {
                                    for (int j = 1; j < 4; j++)
                                    {
                                        magRealVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                // 第三、四行
                                for (int i = 0; i < 2; i++)
                                {
                                    currentline = file.readLine().simplified();
                                    linelist = currentline.split(" ");
                                    for (int j = 1; j < 4; j++)
                                    {
                                        phaseImagVal.append(linelist.at(j).toDouble());
                                    }
                                }
                                if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                                {
                                    QHash<int, QPair<double, double>> dataMap;
                                    for (int i = 0; i < 6; i++)
                                    {
                                        dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                                    }
                                    forceData->_allComponentValue.push_back(dataMap);
                                }
                                else
                                {
                                    return false;
                                }
                                currentline = file.readLine().simplified();
                            }
                        }
                    }
                    // 读取节点力数据
                    else if (currentline == "$GRID POINT FORCE BALANCE")
                    {
                        // 输入力类型：节点力
                        forceData->_forceType = 2;
                        currentline = file.readLine().simplified();
                        if (currentline.contains("MAGNITUDE-PHASE"))
                        {
                            // 输出类型：幅值-相位
                            forceData->_outputType = 1;
                        }
                        else if (currentline.contains("REAL-IMAGINARY"))
                        {
                            // 输出类型：实部-虚部
                            forceData->_outputType = 2;
                        }
                        currentline = file.readLine().simplified();
                        linelist = currentline.split("=");
                        if (currentline.contains("SUBCASE ID"))
                        {
                            if (linelist.size() == 2)
                            {
                                // 工况ID
                                forceData->_subcaseID = linelist.at(1).toInt();
                            }
                        }
                        currentline = file.readLine().simplified();
                        if (currentline.contains("$FREQUENCY"))
                        {
                            // 结果类型为按频率
                            forceData->_resultType = 1;
                            linelist = currentline.split("=");
                            if (linelist.size() == 2)
                            {
                                // 频率
                                double fre = linelist.at(1).toDouble();
                                forceData->_frequency.push_back(fre);
                            }
                        }
                        currentline = file.readLine().simplified();
                        while (!currentline.contains("TITLE"))
                        {
                            // 避免最后一行空行造成死循环
                            if (currentline.isEmpty()) break;
                            // 按频率结果类型
                            if (forceData->_resultType == 1)
                            {
                                QList<double> magRealVal;
                                QList<double> phaseImagVal;
                                // 第一行
                                if (currentline.contains("BUSH"))
                                {
                                    linelist = currentline.split(" ");
                                    if (linelist.size() == 3)
                                    {
                                        int nid = linelist[0].toInt();
                                        int eid = linelist[1].toInt();
                                        QPair<double, double> idPair(nid,eid);
                                        // 节点单元ID
                                        forceData->_nodeElementID.push_back(idPair);
                                    }
                                    // 第二、三行数据
                                    for (int i = 0; i < 2; i++)
                                    {
                                        currentline = file.readLine().simplified();
                                        linelist = currentline.split(" ");
                                        for (int j = 1; j < 4; j++)
                                        {
                                            magRealVal.append(linelist.at(j).toDouble());
                                        }
                                    }
                                    // 第三、四行数据
                                    for (int i = 0; i < 2; i++)
                                    {
                                        currentline = file.readLine().simplified();
                                        linelist = currentline.split(" ");
                                        for (int j = 1; j < 4; j++)
                                        {
                                            phaseImagVal.append(linelist.at(j).toDouble());
                                        }
                                    }
                                    if (magRealVal.size() == 6 && phaseImagVal.size() == 6)
                                    {
                                        // 数据存储
                                        QHash<int, QPair<double, double>> dataMap;
                                        for (int i = 0; i < 6; i++)
                                        {
                                            dataMap.insert(i, QPair<double, double>(magRealVal[i], phaseImagVal[i]));
                                        }
                                        forceData->_allComponentValue.push_back(dataMap);
                                    }
                                    else
                                    {
                                        return false;
                                    }
                                }
                                currentline = file.readLine().simplified();
                            }
                        }
                    }
                    _transferPathAnalysisData->_allForceData.push_back(forceData);
                }
            }
        }
        file.close();
        return true;
    }

    /**
     * @brief 绘制频率响应曲线
     * @details
     * 1. 获取求解器的响应数据
     * 2. 获取所有传递路径的传递函数和输入力数据
     * 3. 将幅值-相位格式转换为实部-虚部格式
     * 4. 计算输入力与传递函数的乘积（实部、虚部）
     * 5. 计算各路径乘积的幅值和相位
     * 6. 求和得到总响应的幅值和相位
     * 7. 分别绘制计算响应和求解器响应的幅值曲线和相位曲线
     */
    void mPostTransferPathAnalysis::plotFrequencyResponseCurve()
    {
        _component = _ui->comboBox_component->currentText();
        _componentIndex = _ui->comboBox_component->currentIndex();

        // 文件保存的响应数据（求解器原始响应）
        QVector<QPair<double, double>> allSolverData = _transferPathAnalysisData->getAllSolverResponseData(_subcaseID, _resultType, _responseID, _componentIndex);

        // 通过每个频率下的每条路径的输入力乘以传递函数，对实部、虚部计算幅值、相位后求和
        // 1. 获取节点包含的所有传递路径
        QStringList pathNames = _transferPathAnalysisData->getAllNodePathName(_resultType, _responseID);
        if (pathNames.size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("文件错误！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("File Error!"), true);
            }
            return;
        }
        _allTransferPath = _transferPathAnalysisData->removeDuplicates(pathNames);

        // 根据传递路径获取传递函数数据（按顺序存储频率、节点、路径、index）
        _allTransferPathData = _transferPathAnalysisData->getAllFrequencyTranseferData(_allTransferPath, _responseID, _componentIndex);
        // 测试传递函数数据获取：验证每个路径的数据点数量是否与频率点数一致
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecval = _allTransferPathData.value(path);
            int size = vecval.size();
            if (size != _allFrequence.size())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递函数文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Transfer function file data error!"), true);
                }
                return;
            }
        }
        // 根据传递路径获取输入力数据（截取标签内容获取所有频率对应数据，分单元力和节点力）
        _allPathForceData = _transferPathAnalysisData->getAllFrequencyForceData(_allTransferPath, _subcaseID);
        // 测试输入力数据获取：验证每个路径的数据点数量是否与频率点数一致
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecval = _allPathForceData.value(path);
            int size = vecval.size();
            if (size != _allFrequence.size())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("输入力文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Input force file data error!"), true);
                }
                return;
            }
        }

        if (_transferPathAnalysisData->_allResponseData.size() == 0 ||
            _transferPathAnalysisData->_allTransferFunctionData.size() == 0 ||
            _transferPathAnalysisData->_allForceData.size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("当前结果没有响应数据！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("The current result has no response data!"), true);
            }
            return;
        }

        // 计算输入力*传递函数数据
        int forcetype = _transferPathAnalysisData->getForceFileType();
        int transfertype = _transferPathAnalysisData->getTransferFileType();

        // 将所有输入力与传递函数数据转换为实部、虚部格式，方便相乘后计算
        if (forcetype == 1)
        {
            QHash<QString, QVector<QPair<double, double>>> outForceData;
            outForceData = convertMagnitudePhaseToRealImag(_allPathForceData);
            _allPathForceData = outForceData;
        }
        if (transfertype == 1)
        {
            QHash<QString, QVector<QPair<double, double>>> outTransferData;
            outTransferData = convertMagnitudePhaseToRealImag(_allTransferPathData);
            _allTransferPathData = outTransferData;
        }
        // 计算输入力*传递函数(实部、虚部)
        _allForceAndTransferRIdata = calProductForcaDataAndTransferData(_allPathForceData, _allTransferPathData);

        // 计算输入力*传递函数(幅值、相位)
        _allForceAndTransferMPdata = calForcaAndTransferMPData(_allForceAndTransferRIdata);

        // 计算输入力*传递函数每条路径实部、虚部求和数据
        _forceAndTransferRISum = calForceAndTransferRISum(_allForceAndTransferRIdata);

        // 通过输入力*传递函数每条路径实部、虚部求和数据计算幅值、相位
        _forceAndTrandferMPSum = calForceAndTransferMPSum(_forceAndTransferRISum);

        // 绘制响应幅值、相位曲线
        QVector <double> magnitudeValuesSolver;
        QVector <double> magnitudeValuesCal;
        QVector <double> phaseValuesSolver;
        QVector <double> phaseValuesCal;
        // 求解器文件数据转换
        // 输出类型为幅值相位
        if (forcetype == 1)
        {
            for (auto solver : allSolverData)
            {
                magnitudeValuesSolver.push_back(solver.first);
                phaseValuesSolver.push_back(solver.second);
            }
        }
        // 输出类型为实部虚部
        else if (forcetype == 2)
        {
            for (auto solver : allSolverData)
            {
                double mag = sqrt(solver.first * solver.first + solver.second * solver.second);
                double phase = atan2(solver.second, solver.first) * 180 / M_PI;
                if (phase < 0)
                {
                    phase = 360 - abs(phase);
                }
                magnitudeValuesSolver.push_back(mag);
                phaseValuesSolver.push_back(phase);
            }
        }
        // 计算数据（通过TPA方法计算的总响应）
        for (auto MPpair : _forceAndTrandferMPSum)
        {
            magnitudeValuesCal.push_back(MPpair.first);
            phaseValuesCal.push_back(MPpair.second);
        }

        // 创建新的幅值曲线视口
        PlotView::mPlotView* plotMagnitudeView = nullptr;
        QString vname = "Magnitude";
        plotMagnitudeView = createNewCurvePlot(vname);
        _importMagnitudeCurveView = vname;
        if (plotMagnitudeView == nullptr)
            return;
        addTableWidgetView(_importMagnitudeCurveView);

        int axisType = _ui->comboBox_axistype->currentIndex();

        // 绘制幅值曲线
        QString curveName = "Calculated Response";
        int nodeid = _responseID;
        QString magTitle = QString("Response Comparison - Magnitude @ %1").arg(nodeid);
        QString magInfo = QString("%1, Node %2, %3").arg(_resultType).arg(_responseID).arg(_component);

        plotMagnitudeView->clearAll();
        plotMagnitudeView->setTitle(magTitle);
        plotMagnitudeView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, "Frequency");
        plotMagnitudeView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, "Magnitude");

        // 对数坐标下 Y 值必须大于 0，否则 Qwt 对数轴会把下限拉到极小值（如 1e-140）
        QVector<double> plotMagnitudeValuesCal = magnitudeValuesCal;
        QVector<double> plotMagnitudeValuesSolver = magnitudeValuesSolver;
        if (axisType == 1)
        {
            double minPositive = std::numeric_limits<double>::max();
            for (double v : plotMagnitudeValuesCal)
            {
                if (v > 0 && v < minPositive)
                    minPositive = v;
            }
            for (double v : plotMagnitudeValuesSolver)
            {
                if (v > 0 && v < minPositive)
                    minPositive = v;
            }
            double replacement = (minPositive < std::numeric_limits<double>::max()) ? (minPositive * 0.1) : 1e-12;
            for (double& v : plotMagnitudeValuesCal)
            {
                if (v <= 0)
                    v = replacement;
            }
            for (double& v : plotMagnitudeValuesSolver)
            {
                if (v <= 0)
                    v = replacement;
            }
        }

        plotMagnitudeView->updateCurveData(curveName, _allFrequence, plotMagnitudeValuesCal);
        applyAxisType(plotMagnitudeView, axisType);

        curveName = "Solver Response";
        plotMagnitudeView->updateCurveData(curveName, _allFrequence, plotMagnitudeValuesSolver);
        plotMagnitudeView->setPlotLegendItemVisible(true);
        plotMagnitudeView->rescaleAxes();
        // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
        plotMagnitudeView->setEnableMarker(true);

        // 对数坐标下，QwtPlotCurve 的默认 baseline=0 会干扰自动范围计算，导致下限被拉到极小值，
        // 因此在这里根据实际数据手动限定 Y 轴范围。
        if (axisType == 1 && !plotMagnitudeValuesCal.isEmpty() && !plotMagnitudeValuesSolver.isEmpty())
        {
            double yMin = plotMagnitudeValuesCal.first();
            double yMax = plotMagnitudeValuesCal.first();
            for (double v : plotMagnitudeValuesCal)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
            for (double v : plotMagnitudeValuesSolver)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
            // 给一点对数边距：下限降到下一个数量级，上限提到上一个数量级
            double lower = yMin * 0.1;
            double upper = qMax(yMax * 10.0, lower * 10.0);

            PlotView::AxisSettings yAxis = plotMagnitudeView->getAxisSettings(PlotView::Y_Left);
            yAxis.autoScale = false;
            yAxis.isLog = true;
            yAxis.min = lower;
            yAxis.max = upper;
            plotMagnitudeView->updateAxisRange(PlotView::Y_Left, yAxis);
        }

        PlotDataCache magCache;
        magCache.viewName = vname;
        magCache.xTitle = "Frequency";
        magCache.yTitles.append("Magnitude");
        magCache.xValue = _allFrequence;
        magCache.curveDatas.append(qMakePair(QString("Calculated Response"), magnitudeValuesCal));
        magCache.curveDatas.append(qMakePair(QString("Solver Response"), magnitudeValuesSolver));
        magCache.curveNames << "Calculated Response" << "Solver Response";
        cachePlotData(vname, magCache);

        // 创建新的相位曲线视口
        PlotView::mPlotView* plotPhaseView = nullptr;
        vname = "Phase";
        plotPhaseView = createNewCurvePlot(vname);
        _importPhaseCurveView = vname;
        if (plotPhaseView == nullptr)
            return;
        addTableWidgetView(_importPhaseCurveView);

        // 绘制相位曲线
        QString phaseTitle = QString("Response Comparison - Phase @ %1").arg(nodeid);
        QString phaseInfo = QString("%1, Node %2, %3").arg(_resultType).arg(_responseID).arg(_component);

        plotPhaseView->clearAll();
        plotPhaseView->setTitle(phaseTitle);
        plotPhaseView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, "Frequency");
        plotPhaseView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, "Phase");

        // 对数坐标下 Y 值必须大于 0，否则 Qwt 对数轴会把下限拉到极小值（如 1e-140）
        QVector<double> plotPhaseValuesCal = phaseValuesCal;
        QVector<double> plotPhaseValuesSolver = phaseValuesSolver;
        if (axisType == 1)
        {
            double minPositive = std::numeric_limits<double>::max();
            for (double v : plotPhaseValuesCal)
            {
                if (v > 0 && v < minPositive)
                    minPositive = v;
            }
            for (double v : plotPhaseValuesSolver)
            {
                if (v > 0 && v < minPositive)
                    minPositive = v;
            }
            double replacement = (minPositive < std::numeric_limits<double>::max()) ? (minPositive * 0.1) : 1e-12;
            for (double& v : plotPhaseValuesCal)
            {
                if (v <= 0)
                    v = replacement;
            }
            for (double& v : plotPhaseValuesSolver)
            {
                if (v <= 0)
                    v = replacement;
            }
        }

        plotPhaseView->updateCurveData("Calculated Response", _allFrequence, plotPhaseValuesCal);
        applyAxisType(plotPhaseView, axisType);

        plotPhaseView->updateCurveData("Solver Response", _allFrequence, plotPhaseValuesSolver);
        plotPhaseView->setPlotLegendItemVisible(true);
        plotPhaseView->rescaleAxes();
        // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
        plotPhaseView->setEnableMarker(true);

        // 对数坐标下，QwtPlotCurve 的默认 baseline=0 会干扰自动范围计算，导致下限被拉到极小值，
        // 因此在这里根据实际数据手动限定 Y 轴范围。
        if (axisType == 1 && !plotPhaseValuesCal.isEmpty() && !plotPhaseValuesSolver.isEmpty())
        {
            double yMin = plotPhaseValuesCal.first();
            double yMax = plotPhaseValuesCal.first();
            for (double v : plotPhaseValuesCal)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
            for (double v : plotPhaseValuesSolver)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
            // 给一点对数边距：下限降到下一个数量级，上限提到上一个数量级
            double lower = yMin * 0.1;
            double upper = qMax(yMax * 10.0, lower * 10.0);

            PlotView::AxisSettings yAxis = plotPhaseView->getAxisSettings(PlotView::Y_Left);
            yAxis.autoScale = false;
            yAxis.isLog = true;
            yAxis.min = lower;
            yAxis.max = upper;
            plotPhaseView->updateAxisRange(PlotView::Y_Left, yAxis);
        }

        PlotDataCache phaseCache;
        phaseCache.viewName = vname;
        phaseCache.xTitle = "Frequency";
        phaseCache.yTitles.append("Phase");
        phaseCache.xValue = _allFrequence;
        phaseCache.curveDatas.append(qMakePair(QString("Calculated Response"), phaseValuesCal));
        phaseCache.curveDatas.append(qMakePair(QString("Solver Response"), phaseValuesSolver));
        phaseCache.curveNames << "Calculated Response" << "Solver Response";
        cachePlotData(vname, phaseCache);

        // 在浮动窗口中打开幅值和相位曲线
        openPlotInFloatWindow(_importMagnitudeCurveView, plotMagnitudeView);
        openPlotInFloatWindow(_importPhaseCurveView, plotPhaseView);
        setCurrentSelectView(_importMagnitudeCurveView);

        // 切换到显示Tab并启用显示区域UI
        _ui->tabWidget->setCurrentIndex(1);
        initialShowEnabledUI(true);
    }

    void mPostTransferPathAnalysis::plotStudyCurve()
    {
        if (_ui->listWidget_mode->count() == 0) return;
        double min = _ui->comboBox_studyFrequencyMin->currentText().toDouble();
        double max = _ui->comboBox_studyFrequencyMax->currentText().toDouble();
        double ratio = _ui->lineEdit_excludeRatio->text().toDouble();
        if (min - max > 0.0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "请重新选择显示频率范围！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "Please re-select the frequency range!", QMessageBox::Ok);
            }
            return;
        }
        if (ratio - 100 > 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "去除比例最大为100%！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "The maximum removal ratio is 100%!", QMessageBox::Ok);
            }
            return;
        }
        if (_ui->listWidget_mode->selectedItems().size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "请选择需要去除的路径！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "Please select the mode you want to remove!", QMessageBox::Ok);
            }
            return;
        }

        //获取选择的需要去除的路径
        QList<QListWidgetItem*> selectItems = _ui->listWidget_mode->selectedItems();
        QVector<QString> excludeModeIndex;
        for (auto listitem : selectItems)
        {
            excludeModeIndex.push_back(listitem->text());
        }

        //绘制频率范围内的频响曲线
        //根据范围获取所有的频率
        QVector<double> frequences = _transferPathAnalysisData->getFrequenceByRange(min, max);

        if (frequences.isEmpty()) return;

        _component = _ui->comboBox_component->currentText();
        _componentIndex = _ui->comboBox_component->currentIndex();

        // 文件保存的响应数据（求解器原始响应）
        QVector<QPair<double, double>> allSolverData = _transferPathAnalysisData->getAllSolverResponseDataByRange(_subcaseID, _resultType, _responseID, _componentIndex, min, max);

        // 通过每个频率下的每条路径的输入力乘以传递函数，对实部、虚部计算幅值、相位后求和
        // 1. 获取节点包含的所有传递路径
        QStringList pathNames = _transferPathAnalysisData->getAllNodePathName(_resultType, _responseID);
        if (pathNames.size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("文件错误！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("File Error!"), true);
            }
            return;
        }
        _allTransferPath = _transferPathAnalysisData->removeDuplicates(pathNames);

        // 根据传递路径获取传递函数数据（按顺序存储频率、节点、路径、index）
        _allTransferPathData = _transferPathAnalysisData->getAllFrequencyTranseferDataByRange(_allTransferPath, _responseID, _componentIndex, min, max);
        // 测试传递函数数据获取：验证每个路径的数据点数量是否与频率点数一致
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecval = _allTransferPathData.value(path);
            int size = vecval.size();
            if (size != frequences.size())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("传递函数文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Transfer function file data error!"), true);
                }
                return;
            }
        }
        // 根据传递路径获取输入力数据（截取标签内容获取所有频率对应数据，分单元力和节点力）
        _allPathForceData = _transferPathAnalysisData->getAllFrequencyForceDataByRange(_allTransferPath, min, max);
        // 测试输入力数据获取：验证每个路径的数据点数量是否与频率点数一致
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecval = _allPathForceData.value(path);
            int size = vecval.size();
            if (size != frequences.size())
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("输入力文件数据错误！"), true);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(0, QString("Input force file data error!"), true);
                }
                return;
            }
        }

        if (_transferPathAnalysisData->_allResponseData.size() == 0 ||
            _transferPathAnalysisData->_allTransferFunctionData.size() == 0 ||
            _transferPathAnalysisData->_allForceData.size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("当前结果没有响应数据！"), true);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(0, QString("The current result has no response data!"), true);
            }
            return;
        }

        // 计算输入力*传递函数数据
        int forcetype = _transferPathAnalysisData->getForceFileType();
        int transfertype = _transferPathAnalysisData->getTransferFileType();

        // 将所有输入力与传递函数数据转换为实部、虚部格式，方便相乘后计算
        if (forcetype == 1)
        {
            QHash<QString, QVector<QPair<double, double>>> outForceData;
            outForceData = convertMagnitudePhaseToRealImag(_allPathForceData);
            _allPathForceData = outForceData;
        }
        if (transfertype == 1)
        {
            QHash<QString, QVector<QPair<double, double>>> outTransferData;
            outTransferData = convertMagnitudePhaseToRealImag(_allTransferPathData);
            _allTransferPathData = outTransferData;
        }
        // 计算输入力*传递函数(实部、虚部)
        _allForceAndTransferRIdata = calProductForcaDataAndTransferData(_allPathForceData, _allTransferPathData);

        // 计算输入力*传递函数(幅值、相位)
        _allForceAndTransferMPdata = calForcaAndTransferMPData(_allForceAndTransferRIdata);

        // 计算输入力*传递函数每条路径实部、虚部求和数据
        _forceAndTransferRISum = calForceAndTransferRISum(_allForceAndTransferRIdata);

        // 通过输入力*传递函数每条路径实部、虚部求和数据计算幅值、相位
        _forceAndTrandferMPSum = calForceAndTransferMPSum(_forceAndTransferRISum);

        // 绘制响应幅值、相位曲线
        QVector <double> magnitudeValuesSolver;
        QVector <double> magnitudeValuesCal;
        QVector <double> phaseValuesSolver;
        QVector <double> phaseValuesCal;
        // 求解器文件数据转换
        // 输出类型为幅值相位
        if (forcetype == 1)
        {
            for (auto solver : allSolverData)
            {
                magnitudeValuesSolver.push_back(solver.first);
                phaseValuesSolver.push_back(solver.second);
            }
        }
        // 输出类型为实部虚部
        else if (forcetype == 2)
        {
            for (auto solver : allSolverData)
            {
                double mag = sqrt(solver.first * solver.first + solver.second * solver.second);
                double phase = atan2(solver.second, solver.first) * 180 / M_PI;
                if (phase < 0)
                {
                    phase = 360 - abs(phase);
                }
                magnitudeValuesSolver.push_back(mag);
                phaseValuesSolver.push_back(phase);
            }
        }
        // 计算数据（通过TPA方法计算的总响应）
        for (auto MPpair : _forceAndTrandferMPSum)
        {
            magnitudeValuesCal.push_back(MPpair.first);
            phaseValuesCal.push_back(MPpair.second);
        }

        // 创建新的幅值曲线视口
        PlotView::mPlotView* plotMagnitudeView = nullptr;
        QString vname = "Magnitude Study";
        plotMagnitudeView = createNewCurvePlot(vname);
        _importMagnitudeStudyCurveView = vname;
        if (plotMagnitudeView == nullptr)
            return;

        int axisType = _ui->comboBox_studyAxisType->currentIndex();

        // 绘制幅值研究曲线
        QString studyTitle = QString("Response Study - Magnitude @ %1").arg(_responseID);

        plotMagnitudeView->clearAll();
        plotMagnitudeView->setTitle(studyTitle);
        plotMagnitudeView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, "Frequency");
        plotMagnitudeView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, "Magnitude");
        plotMagnitudeView->updateCurveData("Calculated Response", frequences, magnitudeValuesCal);
        PlotView::CurveSettings Response = plotMagnitudeView->getCurveSettings("Calculated Response");
        Response.lineColor = Qt::red;
        Response.symbolColor = Qt::red;
        Response.barColor = Qt::red;
        plotMagnitudeView->setCurveStyle("Calculated Response", Response);
        applyAxisType(plotMagnitudeView, axisType);

        // 绘制去除路径响应的频响曲线
        // 根据去除比例，对被排除路径的贡献进行衰减后重新计算总和
        QHash<QString, QVector<QPair<double, double>>> filteredRI = _allForceAndTransferRIdata;
        double ratioFactor = (100.0 - ratio) / 100.0; // 保留的比例因子
        for (QString excludedPath : excludeModeIndex)
        {
            if (filteredRI.contains(excludedPath))
            {
                QVector<QPair<double, double>>& pathData = filteredRI[excludedPath];
                for (int i = 0; i < pathData.size(); ++i)
                {
                    pathData[i].first *= ratioFactor;
                    pathData[i].second *= ratioFactor;
                }
            }
        }
        QVector<QPair<double, double>> filteredRISum = calForceAndTransferRISum(filteredRI);
        QVector<QPair<double, double>> filteredMPSum = calForceAndTransferMPSum(filteredRISum);
        QVector<double> newMagnitudeValuesCal;
        for (auto MPpair : filteredMPSum)
        {
            newMagnitudeValuesCal.push_back(MPpair.first);
        }
        // 绘制去除路径响应的曲线
        plotMagnitudeView->updateCurveData("Response Without Path", frequences, newMagnitudeValuesCal);
        PlotView::CurveSettings withoutPathSettings = plotMagnitudeView->getCurveSettings("Response Without Path");
        withoutPathSettings.linePatternType = PlotView::Pat_Dash;
        withoutPathSettings.lineColor = Qt::blue;
        withoutPathSettings.symbolColor = Qt::blue;
        withoutPathSettings.barColor = Qt::blue;
        plotMagnitudeView->setCurveStyle("Response Without Path", withoutPathSettings);

        // 绘制差异曲线（挂载到右 Y 轴）
        QVector<double> difference = getDifferenceData(magnitudeValuesCal, newMagnitudeValuesCal);
        plotMagnitudeView->updateCurveData("%Difference", frequences, difference, PlotView::X_Bottom, PlotView::Y_Right);
        PlotView::CurveSettings diffSettings = plotMagnitudeView->getCurveSettings("%Difference");
        diffSettings.lineColor = Qt::green;
        diffSettings.symbolColor = Qt::green;
        diffSettings.barColor = Qt::green;
        plotMagnitudeView->setCurveStyle("%Difference", diffSettings);

        plotMagnitudeView->getQwtPlot()->setAxisTitle(QwtPlot::yRight, "%Difference");
        plotMagnitudeView->getQwtPlot()->enableAxis(QwtPlot::yRight, true);

        plotMagnitudeView->setPlotLegendItemVisible(true);
        plotMagnitudeView->rescaleAxes();
        // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
        plotMagnitudeView->setEnableMarker(true);

        // 设置右 Y 轴固定范围，保证差异曲线可见
        double yMin = 0.0, yMax = 0.0;
        if (!difference.isEmpty())
        {
            yMin = yMax = difference[0];
            for (int i = 1; i < difference.size(); ++i)
            {
                if (difference[i] < yMin) yMin = difference[i];
                if (difference[i] > yMax) yMax = difference[i];
            }
            double margin = (yMax - yMin) * 0.1;
            if (qFuzzyIsNull(margin)) margin = 10.0;
            yMin -= margin;
            yMax += margin;
        }
        PlotView::AxisSettings rightAxis = plotMagnitudeView->getAxisSettings(PlotView::Y_Right);
        rightAxis.autoScale = false;
        rightAxis.min = yMin;
        rightAxis.max = yMax;
        plotMagnitudeView->updateAxisRange(PlotView::Y_Right, rightAxis);

        // 设置图例信息文本
        QStringList infoLines;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            infoLines.append(QString("Contributor(s) Excluded %1% in all frequency range")
                .arg(ratio));
        }
        else
        {
            infoLines.append(QString("Contributor(s) Excluded %1% in all frequency range")
                .arg(ratio));
        }
        for (QString path : excludeModeIndex)
        {
            infoLines.append(path);
        }


        // 缓存数据供数据表使用
        PlotDataCache studyCache;
        studyCache.viewName = vname;
        studyCache.xTitle = "Frequency";
        studyCache.yTitles.append("Magnitude");
        studyCache.yTitles.append("Magnitude");
        studyCache.yTitles.append("%Difference");
        studyCache.xValue = frequences;
        studyCache.curveDatas.append(qMakePair(QString("Calculated Response"), magnitudeValuesCal));
        studyCache.curveDatas.append(qMakePair(QString("Response Without Path"), newMagnitudeValuesCal));
        studyCache.curveDatas.append(qMakePair(QString("%Difference"), difference));
        studyCache.curveNames << "Calculated Response" << "Response Without Path" << "%Difference";
        cachePlotData(vname, studyCache);

        // 在浮动窗口中打开幅值研究曲线
        openPlotInFloatWindow(_importMagnitudeStudyCurveView, plotMagnitudeView);

        // 在浮动窗口打开后再创建图例，确保窗口已显示、尺寸有效
        createCustomLegend(_importMagnitudeStudyCurveView, infoLines, QColor(Qt::transparent), QColor(Qt::magenta));
        //CurveName = "Solver Response";
        //plotMagnitudeView->setXText("Frequency");
        //plotMagnitudeView->setYText("Magnitude");
        //plotMagnitudeView->setFrequenceResponseSolverPlotData(CurveName, _allFrequence, magnitudeValuesSolver, magTitleInformation, axisType);

    }

    QVector<double> mPostTransferPathAnalysis::getDifferenceData(QVector<double> oldFre, QVector<double> newFre)
    {
        QVector<double> difs;
        for (int i = 0; i < oldFre.size(); i++)
        {
            double dif = 0.0;
            if (!qFuzzyIsNull(oldFre[i]))
            {
                dif = (newFre[i] - oldFre[i]) / oldFre[i] * 100.0;
            }
            else
            {
                // 原值接近零时，若新值非零则差异为极大值（响应从无到有），反之则为0
                dif = (!qFuzzyIsNull(newFre[i])) ? 999.0 : 0.0;
            }
            // 移除硬截断，保留真实差异值；yRight 轴范围由绘图端根据数据动态调整
            difs.push_back(dif);
        }
        return difs;
    }

    /**
     * @brief 表格单元格点击槽函数：激活对应的浮动窗口
     * @param item 被点击的表格项
     * @details 根据表格项文本获取对应的视图名称，并通过ADS停靠布局管理器激活对应的dock窗口
     */
    void mPostTransferPathAnalysis::slot_onCellClicked(QTableWidgetItem *item)
    {
        QString viewname = item->text();
        if (!_plotFloatWindows.contains(viewname))
            return;
        mPostPlotFloatWindow* floatWin = _plotFloatWindows[viewname];
        if (floatWin == nullptr)
            return;
        // 通过 ADS 激活对应的 dock，注意 key 名与 openPlotInFloatWindow 中的 keyName 一致
        mDockLayoutManager::getInstance()->activateDock(viewname + "PostWidow");
        _activePlotViewName = viewname;
    }

    /**
     * @brief 绘制显示曲线（路径贡献、传递函数、输入力）
     * @details
     * 根据用户选择的频率、显示个数和类型（投影/幅值），
     * 绘制三种柱状图：
     * 1. 路径贡献图（Path Contribution）
     * 2. 传递函数图（Transfer Function）
     * 3. 输入力图（Input Force）
     */
    void mPostTransferPathAnalysis::plotShowCurve()
    {
        if (_ui->comboBox_showFrequency->currentText().isEmpty() ||
            _ui->comboBox_showOrder->currentText().isEmpty())
        {
            return;
        }

        // 获取显示频率、阶数、类型
        double showFre = _ui->comboBox_showFrequency->currentText().toDouble();
        QString showOrder = _ui->comboBox_showOrder->currentText();
        int showType = _ui->comboBox_showType->currentIndex();
        QStringList showPaths;

        // 百分比显示方式（投影方式，考虑相位）
        if (showType == 0)
        {
            // 传递函数*输入力
            QList<QPair<QString, double>> pathPercents = getAllPathPercentsByFrequency(showFre);
            qSort(pathPercents.begin(), pathPercents.end(), comparePercents);

            QStringList xdata;
            QVector<double> ydata;
            // 总响应百分比
            xdata.append("Response");
            ydata.push_back(100);

            int showNum;
            if (showOrder == "All")
            {
                showNum = 1000;
            }
            else
            {
                showNum = _ui->comboBox_showOrder->currentText().toInt();
            }
            // 传递函数、输入力柱状图需要显示的路径、百分比
            QVector<double> showPercents;
            if (pathPercents.size() <= showNum)
            {
                for (int i = 0; i < pathPercents.size(); i++)
                {
                    QString pathname = pathPercents[i].first;
                    showPaths.push_back(pathname);
                    showPercents.push_back(pathPercents.at(i).second);
                    // 获取单元ID及自由度
                    QString str = getShortPathName(pathname);
                    xdata.append(str);
                    ydata.push_back(pathPercents.at(i).second);
                }
            }
            else if (pathPercents.size() > showNum)
            {
                for (int i = 0; i < showNum; i++)
                {
                    QString pathname = pathPercents[i].first;
                    showPaths.push_back(pathname);
                    showPercents.push_back(pathPercents.at(i).second);
                    // 获取单元ID及自由度
                    QString str = getShortPathName(pathname);
                    xdata.append(str);
                    ydata.push_back(pathPercents.at(i).second);
                }

                // 其他模态百分比
                double total = 0;
                for (int i = 1; i < ydata.size(); i++)
                {
                    total += ydata[i];
                }
                double others = 100 - total;
                xdata.append("Others");
                ydata.push_back(others);
                showPaths.push_back("Others");
                showPercents.push_back(others);
            }

            // 创建路径贡献曲线视图
            QString pcTitle = QString("Path Contribution -Projected @%1(Frequency = %2Hz)").arg(_responseID).arg(showFre);
            PlotView::mPlotView* plotViewPathContribution = nullptr;
            QString vname = "Path Contribution";
            plotViewPathContribution = createNewCurvePlot(vname);
            _showPathContributionCurveView = vname;
            addTableWidgetView(_showPathContributionCurveView);
            plotBarChart(plotViewPathContribution, "Percent", xdata, ydata, pcTitle);

            // 缓存数据供数据表使用
            PlotDataCache pcCache;
            pcCache.viewName = vname;
            pcCache.xTitle = "Contributors";
            pcCache.yTitles.append("%Path Contribution");
            pcCache.xStringValue = xdata;
            pcCache.curveDatas.append(qMakePair(QString("Percent"), ydata));
            pcCache.curveNames.append("Percent");
            cachePlotData(vname, pcCache);

            openPlotInFloatWindow(_showPathContributionCurveView, plotViewPathContribution);
            // 路径贡献图例内容
            setPathContributionLegend(showFre, showPercents, showPaths);
        }
        // 幅值显示方式（不考虑相位）
        if (showType == 1)
        {
            // 传递函数*输入力每个路径幅值占总幅值幅值比例
            QList<QPair<QString, double>> pathMagPercents = getAllPathMagnitudePercentsByFrequency(showFre);
            qSort(pathMagPercents.begin(), pathMagPercents.end(), comparePercents);

            QStringList xdata;
            QVector<double> ydata;
            // 总响应百分比
            xdata.append("Response");
            ydata.push_back(100);

            int showNum;
            if (showOrder == "All")
            {
                showNum = 1000;
            }
            else
            {
                showNum = _ui->comboBox_showOrder->currentText().toInt();
            }
            // 传递函数、输入力柱状图需要显示的路径、百分比
            QVector<double> showPercents;
            if (pathMagPercents.size() <= showNum)
            {
                for (int i = 0; i < pathMagPercents.size(); i++)
                {
                    QString pathname = pathMagPercents[i].first;
                    showPaths.push_back(pathname);
                    showPercents.push_back(pathMagPercents.at(i).second);
                    // 获取单元ID及自由度
                    QString str = getShortPathName(pathname);
                    xdata.append(str);
                    ydata.push_back(pathMagPercents.at(i).second);
                }
            }
            else if (pathMagPercents.size() > showNum)
            {
                for (int i = 0; i < showNum; i++)
                {
                    QString pathname = pathMagPercents[i].first;
                    showPaths.push_back(pathname);
                    showPercents.push_back(pathMagPercents.at(i).second);
                    // 获取单元ID及自由度
                    QString str = getShortPathName(pathname);
                    xdata.append(str);
                    ydata.push_back(pathMagPercents.at(i).second);
                }

                // 其他模态百分比(计算所有未显示路径的实部、虚部之和求幅值后与总幅值的比例)
                double others = getMagPercentOthers(showFre, showPaths);
                xdata.append("Others");
                ydata.push_back(others);
                showPaths.push_back("Others");
                showPercents.push_back(others);
            }

            // 创建路径贡献曲线视图
            QString magTitle = QString("Path Contribution -Magnitude @%1(Frequency = %2Hz)").arg(_responseID).arg(showFre);
            PlotView::mPlotView* plotViewPathContribution = nullptr;
            QString vname = "Path Contribution";
            plotViewPathContribution = createNewCurvePlot(vname);
            _showPathContributionCurveView = vname;
            addTableWidgetView(_showPathContributionCurveView);
            plotBarChart(plotViewPathContribution, "Magnitude", xdata, ydata, magTitle);

            // 缓存数据供数据表使用
            PlotDataCache magCache;
            magCache.viewName = vname;
            magCache.xTitle = "Contributors";
            magCache.yTitles.append("%Path Contribution");
            magCache.xStringValue = xdata;
            magCache.curveDatas.append(qMakePair(QString("Magnitude"), ydata));
            magCache.curveNames.append("Magnitude");
            cachePlotData(vname, magCache);

            openPlotInFloatWindow(_showPathContributionCurveView, plotViewPathContribution);
            // 路径贡献图例内容
            setMagPathContributionLegend(showFre, showPercents, showPaths);
        }

        // 确定类型名称用于标题
        QString typeName;
        if (showType == 0)
        {
            typeName = "Projected";
        }
        else if (showType == 1)
        {
            typeName = "Magnitude";
        }
        // 创建传递函数曲线视图
        QStringList transferXdata;
        QVector<double> transferYdata;
        // 根据需要显示的路径获取对应的传递函数幅值数据
        QVector<double> transferMag = getTransferMagnitudeByFrequencyAndPath(showFre, showPaths);
        for (auto p : showPaths)
        {
            QString sp = getShortPathName(p);
            transferXdata.append(sp);
        }
        for (auto mag : transferMag)
        {
            transferYdata.push_back(mag);
        }
        QString tftitle = QString("Transfer Function -%1 (Frequency = %2Hz)").arg(typeName).arg(showFre);
        PlotView::mPlotView* plotViewTransfer = nullptr;
        QString vname = "Transfer Function";
        plotViewTransfer = createNewCurvePlot(vname);
        _showTransferFunctionCurveView = vname;
        addTableWidgetView(_showTransferFunctionCurveView);
        plotBarChart(plotViewTransfer, "Transfer", transferXdata, transferYdata, tftitle);

        // 缓存数据供数据表使用
        PlotDataCache tfCache;
        tfCache.viewName = vname;
        tfCache.xTitle = "Contributors";
        tfCache.yTitles.append("Translational Transfer Function");
        tfCache.xStringValue = transferXdata;
        tfCache.curveDatas.append(qMakePair(QString("Transfer"), transferYdata));
        tfCache.curveNames.append("Transfer");
        cachePlotData(vname, tfCache);

        openPlotInFloatWindow(_showTransferFunctionCurveView, plotViewTransfer);
        setTransferFunctionLegend(transferYdata, showPaths);

        // 创建输入力曲线视图
        QStringList forceXdata;
        QVector<double> forceYdata;
        // 根据需要显示的路径获取对应的输入力幅值数据
        QVector<double> forceMag = getForceMagnitudeByFrequencyAndPath(showFre, showPaths);
        for (auto p : showPaths)
        {
            QString sp = getShortPathName(p);
            forceXdata.append(sp);
        }
        for (auto mag : forceMag)
        {
            forceYdata.push_back(mag);
        }
        QString ftitle = QString("Input Force -%1 (Frequency = %2Hz)").arg(typeName).arg(showFre);
        PlotView::mPlotView* plotViewForce = nullptr;
        vname = "Input Force";
        plotViewForce = createNewCurvePlot(vname);
        _showInputForceCurveView = vname;
        addTableWidgetView(_showInputForceCurveView);
        plotBarChart(plotViewForce, "Input Force", forceXdata, forceYdata, ftitle);

        // 缓存数据供数据表使用
        PlotDataCache forceCache;
        forceCache.viewName = vname;
        forceCache.xTitle = "Contributors";
        forceCache.yTitles.append("Translational Input Force");
        forceCache.xStringValue = forceXdata;
        forceCache.curveDatas.append(qMakePair(QString("Input Force"), forceYdata));
        forceCache.curveNames.append("Input Force");
        cachePlotData(vname, forceCache);

        openPlotInFloatWindow(_showInputForceCurveView, plotViewForce);
        setInputForceLegend(forceYdata, showPaths);

        // 设置当前显示视图
        setCurrentSelectView(_showPathContributionCurveView);
    }

    /**
     * @brief 切换界面语言槽函数
     * @param language 语言代码：0-中文，1-英文
     * @details 根据语言设置更新所有UI控件的文字显示
     */
    void mPostTransferPathAnalysis::slot_changeLanguage(int language)
    {
        if (language == 0)
        {
            _ui->tabWidget->setTabText(0, "导入");
            _ui->tabWidget->setTabText(1, "显示");
            _ui->tabWidget->setTabText(2, "研究");
            _ui->groupBox_selectFile->setTitle("文件选择");
            _ui->label_tffile->setText("传递函数:");
            _ui->label_forcefile->setText("输入力:");
            _ui->pushButton_loadFileOK->setText("导入");
            _ui->groupBox_selectResult->setTitle("结果选择");
            _ui->label_case->setText("工况:");
            _ui->label_case_2->setText("工况:");
            _ui->label_result->setText("结果类型:");
            _ui->label_id->setText("响应ID:");
            _ui->label_component->setText("自由度:");
            _ui->pushButton_loadResponseOK->setText("确定");
            //_ui->label_show->setText("显示设置");
            //_ui->label_curvelist->setText("曲线列表");

            _ui->label_showFrequency->setText("显示频率:");
            _ui->label_showOrder->setText("显示个数:");
            _ui->label_showType->setText("显示类型:");
            QStringList types;
            types << "百分比" << "幅值";
            _ui->comboBox_showType->clear();
            _ui->comboBox_showType->addItems(types);
            _ui->pushButton_showData->setText("曲线数据");
            _ui->pushButton_showOK->setText("确定");
            _ui->label_studyFrequnecy->setText("显示频率:");
            _ui->label_excludeRatio->setText("去除比例:");
            _ui->label_chooseMode->setText("路径选择:");
            _ui->pushButton_studyData->setText("曲线数据");
            _ui->pushButton_studyOK->setText("确定");
            //_ui->pushButton_return1->setText("返回");
            //_ui->pushButton_return2->setText("返回");
            //_ui->pushButton_return3->setText("返回");
            _ui->label_axistype->setText("坐标轴类型");
            QStringList strlist;
            strlist << "线性" << "对数";
            _ui->comboBox_axistype->clear();
            _ui->comboBox_studyAxisType->clear();
            _ui->comboBox_axistype->addItems(strlist);
            _ui->comboBox_studyAxisType->addItems(strlist);
            _ui->comboBox_axistype->setCurrentIndex(1);
            _ui->comboBox_studyAxisType->setCurrentIndex(1);
        }
        else if (language == 1)
        {
            _ui->tabWidget->setTabText(0, "Load");
            _ui->tabWidget->setTabText(1, "Display");
            _ui->tabWidget->setTabText(2, "Study");
            _ui->groupBox_selectFile->setTitle("File selection");
            _ui->label_tffile->setText("Transfer function:");
            _ui->label_forcefile->setText("Input force:");
            _ui->pushButton_loadFileOK->setText("Load");
            _ui->groupBox_selectResult->setTitle("Result selection");
            _ui->label_case->setText("Subcase:");
            _ui->label_case_2->setText("Subcase:");
            _ui->label_result->setText("Result type:");
            _ui->label_id->setText("Response ID:");
            _ui->label_component->setText("Direction component:");
            _ui->pushButton_loadResponseOK->setText("Load");
            //_ui->label_show->setText("Display Settings");
            //_ui->label_curvelist->setText("Curve List");

            _ui->label_showFrequency->setText("Frequency:");
            _ui->label_showOrder->setText("Display order:");
            _ui->label_showType->setText("Display type:");
            QStringList types;
            types << "Projected" << "Magnitude";
            _ui->comboBox_showType->clear();
            _ui->comboBox_showType->addItems(types);
            _ui->pushButton_showData->setText("Curve data");
            _ui->pushButton_showOK->setText("OK");
            _ui->label_studyFrequnecy->setText("Display frequency:");
            _ui->label_excludeRatio->setText("%to exclude:");
            _ui->label_chooseMode->setText("Mode selection:");
            _ui->pushButton_studyData->setText("Curve data");
            _ui->pushButton_studyOK->setText("OK");
            //_ui->pushButton_return1->setText("Return");
            //_ui->pushButton_return2->setText("Return");
            //_ui->pushButton_return3->setText("Return");
            _ui->label_axistype->setText("Axis type");
            QStringList strlist;
            strlist << "Linear" << "Logarithmic";
            _ui->comboBox_axistype->clear();
            _ui->comboBox_studyAxisType->clear();
            _ui->comboBox_axistype->addItems(strlist);
            _ui->comboBox_studyAxisType->addItems(strlist);
            _ui->comboBox_axistype->setCurrentIndex(1);
            _ui->comboBox_studyAxisType->setCurrentIndex(1);
        }
    }

    /**
     * @brief 选择传递函数文件按钮槽函数
     * @details 弹出文件对话框让用户选择.pch格式的传递函数文件
     */
    void mPostTransferPathAnalysis::chooseTFFile()
    {
        // 打开文件
        QString dir;
        if (!_tfFilePath.isEmpty())
            dir = QFileInfo(_tfFilePath).absolutePath();
        else
            dir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QString filePath;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            filePath = QFileDialog::getOpenFileName(this, QString("打开文件"), dir, QString("PCH File(*.pch)"));
        }
        else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            filePath = QFileDialog::getOpenFileName(this, QString("Open File"), dir, QString("PCH File(*.pch)"));
        }

        if (filePath.isEmpty())
        {
            return;
        }
        initialImportEnabledUI(false);
        initialShowEnabledUI(false);
        _ui->lineEdit_tfFilePath->setText(filePath);
        _tfFilePath = filePath;
    }

    /**
     * @brief 选择输入力文件按钮槽函数
     * @details 弹出文件对话框让用户选择.pch格式的输入力文件
     */
    void mPostTransferPathAnalysis::chooseForceFile()
    {
        // 打开文件
        QString dir;
        if (!_forceFilePath.isEmpty())
            dir = QFileInfo(_forceFilePath).absolutePath();
        else
            dir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QString filePath;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            filePath = QFileDialog::getOpenFileName(this, QString("打开文件"), dir, QString("PCH File(*.pch)"));
        }
        else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            filePath = QFileDialog::getOpenFileName(this, QString("Open File"), dir, QString("PCH File(*.pch)"));
        }

        if (filePath.isEmpty())
        {
            return;
        }
        initialImportEnabledUI(false);
        initialShowEnabledUI(false);
        _ui->lineEdit_forceFilePath->setText(filePath);
        _forceFilePath = filePath;
    }

    /**
     * @brief 创建新的曲线绘图视口
     * @param viewName 视图名称
     * @return 返回创建的mPlotView指针
     * @details 创建线型图表，设置白色背景
     */
    PlotView::mPlotView* mPostTransferPathAnalysis::createNewCurvePlot(QString& viewName)
    {
        PlotView::mPlotView* plot = new PlotView::mPlotView();
        plot->setPlotBackground(QColor(255, 255, 255));
        return plot;
    }

    /**
     * @brief 在浮动窗口中打开绘图视口
     * @param vname 视图名称
     * @param plotView 绘图视口指针
     * @details 使用ADS停靠布局管理器将浮动窗口添加到界面底部
     */
    void mPostTransferPathAnalysis::openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView)
    {
        mPostPlotFloatWindow* floatWin = nullptr;
        // 如果已存在同名窗口，复用旧的
        if (_plotFloatWindows.contains(vname)) {
            floatWin = _plotFloatWindows[vname];
            if (floatWin == nullptr)
            {
                // 旧窗口已被外部关闭，移除失效条目
                _plotFloatWindows.remove(vname);
                floatWin = new mPostPlotFloatWindow();
                _plotFloatWindows[vname] = floatWin;
            }
            floatWin->setPlotView(plotView);
            floatWin->setWindowTitleText(vname);
        }
        else {
            floatWin = new mPostPlotFloatWindow();
            floatWin->setWindowTitleText(vname);
            floatWin->setPlotView(plotView);
            _plotFloatWindows[vname] = floatWin;
        }
        _activePlotViewName = vname;
        QString keyName = vname + "PostWidow";
        // 使用 ADS 停靠在底部
        mDockLayoutManager::getInstance()->addDockToBottom(floatWin, vname, keyName);
    }

    /**
     * @brief 后处理活动视图变化槽函数（已废弃）
     * @details 已移除VMManager依赖，不再需要处理子窗口激活事件
     */
    void mPostTransferPathAnalysis::slot_postActiveViewChanged()
    {
        // 已移除 VMManager 依赖，不再需要处理子窗口激活事件
    }

    /**
     * @brief 删除曲线绘图槽函数
     * @details 从表格中找到对应的曲线视图，删除其浮动窗口和表格项
     */
    void mPostTransferPathAnalysis::slot_deleteCurvePlot()
    {
        // 获取信号源（删除按钮）
        QToolButton* tb = qobject_cast<QToolButton*>(sender());
        if (tb == nullptr)
        {
            return;
        }

        const int nrow = _ui->tableWidget->rowCount();
        for (int i = 0; i < nrow; ++i)
        {
            if (tb == _ui->tableWidget->cellWidget(i, 1))
            {
                // 获取名称
                QTableWidgetItem* item = _ui->tableWidget->item(i, 0);
                if (item != nullptr)
                {
                    QString name = item->text();
                    if (_plotFloatWindows.contains(name)) {
                        mPostPlotFloatWindow* win = _plotFloatWindows[name];
                        win->setParent(nullptr);  // 从 dock 中脱离
                        delete win;
                        _plotFloatWindows.remove(name);
                        if (_activePlotViewName == name) {
                            _activePlotViewName.clear();
                        }
                    }
                    if (_showViewName.contains(name))
                    {
                        int index = _showViewName.indexOf(name);
                        _showViewName.removeAt(index);
                    }
                    // 删除表格行
                    _ui->tableWidget->removeRow(i);

                    int row = _ui->tableWidget->rowCount();
                    if (row != 0)
                    {
                        QTableWidgetItem* item = _ui->tableWidget->item(row - 1, 0);
                        if (item != nullptr)
                        {
                            _ui->tableWidget->itemClicked(item);
                        }
                    }
                }
                break;
            }
        }
    }

    /**
     * @brief 显示数据表格
     * @details 根据当前tab页获取对应的plotView，弹出数据表格窗口显示曲线数据
     */
    void mPostTransferPathAnalysis::showDataTable()
    {
        // 根据当前 tab 获取对应的 plotView
        int tabIndex = _ui->tabWidget->currentIndex();
        QString targetName;
        if (tabIndex == 0) {
            targetName = _importMagnitudeCurveView;
        }
        else if (tabIndex == 1) {
            targetName = _showPathContributionCurveView;
        }
        else if (tabIndex == 2)//研究
        {
            targetName = _importMagnitudeStudyCurveView;
        }

        if (targetName.isEmpty() || !_plotDataCache.contains(targetName)) return;

        if (_dataTable == nullptr)
            _dataTable = new mPostDataTable(this);

        const PlotDataCache& cache = _plotDataCache[targetName];

        // 设置数据（从缓存中读取）
        if (cache.xValue.isEmpty())
        {
            _dataTable->setTableData(cache.curveNames, cache.xTitle, cache.yTitles, cache.xStringValue, cache.curveDatas);
        }
        else
        {
            _dataTable->setTableData(cache.curveNames, cache.xTitle, cache.yTitles, cache.xValue, cache.curveDatas);
        }
        _dataTable->move(QCursor::pos());
        _dataTable->show();
    }

    /**
     * @brief 删除当前tab页的曲线视图
     * @details 根据当前tab索引删除对应的曲线视图和表格项
     */
    void mPostTransferPathAnalysis::deleteCurveView()
    {
        int index = _ui->tabWidget->currentIndex();
        if (index == 0)
        {
            if (_plotFloatWindows.contains(_importMagnitudeCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_importMagnitudeCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_importMagnitudeCurveView);
            }
            if (_plotFloatWindows.contains(_importPhaseCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_importPhaseCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_importPhaseCurveView);
            }
            // 移除表格内容
            removeTableView(_importMagnitudeCurveView);
            removeTableView(_importPhaseCurveView);
        }
        else if (index == 1)
        {
            if (_plotFloatWindows.contains(_showPathContributionCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_showPathContributionCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_showPathContributionCurveView);
            }
            if (_plotFloatWindows.contains(_showTransferFunctionCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_showTransferFunctionCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_showTransferFunctionCurveView);
            }
            if (_plotFloatWindows.contains(_showInputForceCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_showInputForceCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_showInputForceCurveView);
            }
            // 移除表格内容
            removeTableView(_showPathContributionCurveView);
            removeTableView(_showTransferFunctionCurveView);
            removeTableView(_showInputForceCurveView);
        }
        else if (index == 2)
        {
            if (_plotFloatWindows.contains(_importMagnitudeStudyCurveView)) {
                mPostPlotFloatWindow* win = _plotFloatWindows[_importMagnitudeStudyCurveView];
                win->setParent(nullptr);
                delete win;
                _plotFloatWindows.remove(_importMagnitudeStudyCurveView);
            }
        }
    }

    /**
     * @brief 当前Tab切换时触发
     * @param index 切换到的tab索引
     * @details 根据不同的显示模式切换激活对应的曲线视图
     */
    void mPostTransferPathAnalysis::currentTabChanged(int index)
    {
        return;//htx add 取消通过切面切换，切换曲线视图的功能
        // 根据不同的显示模式切换曲线视图
        QString targetName;
        if (index == 0)
        {
            targetName = _importMagnitudeCurveView;
        }
        else if (index == 1)
        {
            targetName = _showPathContributionCurveView;
        }
        else if (index == 2)
        {
            targetName = _importMagnitudeStudyCurveView;
        }

        if (!targetName.isEmpty() && _plotFloatWindows.contains(targetName)) {
            // 通过 ADS 激活对应的 dock，注意 key 名与 openPlotInFloatWindow 中的 keyName 一致
            mDockLayoutManager::getInstance()->activateDock(targetName + "PostWidow");
            _activePlotViewName = targetName;
        }
    }

    void mPostTransferPathAnalysis::saveFileData()
    {
        saveTransmitDate();

        saveForceDate();
    }

    void mPostTransferPathAnalysis::saveTransmitDate()
    {
        //传递函数中数据赋值
        _transferPathAnalysisData = new mPostTransferPathAnalysisData();
        for (auto file : _transmitFile.blocks)
        {
            QPointer<mPostTransferFunctionData> transferData = new mPostTransferFunctionData();
            transferData->_transferPath = QString::fromStdString(file.label);
            transferData->_type = QString::fromStdString(file.schemaName);
            for (auto i : file.labels)
            {
                if (QString::fromStdString(i.name).contains("MAGNITUDE-PHASE"))
                    transferData->_outputType = 1;//1:幅值-相位  2:实部-虚部
                if (QString::fromStdString(i.name).contains("REAL-IMAGINARY"))
                    transferData->_outputType = 2;
            }
            transferData->_subcaseID = file.subcaseId;

            for (auto i : file.labels)
            {
                if (QString::fromStdString(i.name).contains("FREQUENCY"))
                    transferData->_resultType = 1;//1:幅值-相位  2:实部-虚部
                if (QString::fromStdString(i.name).contains("POINT ID"))
                    transferData->_resultType = 2;
            }

            if (transferData->_resultType == 1)
            {
                for (auto i : file.records)
                    transferData->_frequency.push_back(i.abscissa);
            }
            if (transferData->_resultType == 2)
                transferData->_nodeId.push_back(file.pointId);// 节点ID

            for (auto fi : file.records)
            {
                if (!fi.secondary.has_value())
                    break;

                // 根据结果类型补充标识
                if (transferData->_resultType == 1) {
                    // 每个 record 首行是节点 ID，需要从 fi 或对应结构中获取并 push 到 _nodeId
                    transferData->_nodeId.push_back(int(fi.abscissa));
                }
                else if (transferData->_resultType == 2) {
                    // 每个 record 首行是频率，需要从 fi 或对应结构中获取并 push 到 _frequency
                    transferData->_frequency.push_back(double(fi.abscissa));
                }

                // 构造一个 QHash 保存当前数据块的 6 个分量
                QHash<int, QPair<double, double>> dataMap;
                for (int i = 0; i < 6; ++i) {
                    dataMap.insert(i, QPair<double, double>(fi.primary[i], fi.secondary.value()[i]));
                }
                transferData->_allComponentValue.push_back(dataMap);
            }
            _transferPathAnalysisData->_allTransferFunctionData.append(transferData);
        }
    }

    void mPostTransferPathAnalysis::saveForceDate()
    {
        // 遍历力文件中的所有块
        for (auto file : _forceFile.blocks)
        {
            // 判断块类型：响应数据
            if (!QString::fromStdString(file.schemaName).contains("FORCE"))
            {
                QPointer<mPostResponseData> responseData = new mPostResponseData();

                // 类型
                responseData->_type = QString::fromStdString(file.schemaName);

                // 输出类型：幅值-相位 / 实部-虚部
                for (auto i : file.labels)
                {
                    if (QString::fromStdString(i.name).contains("MAGNITUDE-PHASE"))
                        responseData->_outputType = 1;
                    else if (QString::fromStdString(i.name).contains("REAL-IMAGINARY"))
                        responseData->_outputType = 2;
                }

                // 工况 ID
                responseData->_subcaseID = file.subcaseId;

                // 结果类型：按频率 / 按节点
                for (auto i : file.labels)
                {
                    if (QString::fromStdString(i.name).contains("FREQUENCY"))
                        responseData->_resultType = 1;
                    else if (QString::fromStdString(i.name).contains("POINT ID"))
                        responseData->_resultType = 2;
                }

                // 块头标识
                if (responseData->_resultType == 1)
                    responseData->_frequency;   // 块头频率(无)
                else if (responseData->_resultType == 2)
                    responseData->_nodeId.push_back(file.pointId);        // 块头节点 ID

                // 遍历数据记录
                for (auto fi : file.records)
                {
                    if (!fi.secondary.has_value())
                        break;  // 相位/虚部数据缺失，与原逻辑一致直接中断

                    // 根据结果类型补充标识
                    if (responseData->_resultType == 1) {
                        // 每个记录首行是节点 ID
                        responseData->_nodeId.push_back(int(fi.abscissa));
                    }
                    else if (responseData->_resultType == 2) {
                        // 每个记录首行是频率
                        responseData->_frequency.push_back(double(fi.abscissa));
                    }

                    // 构造 6 分量数据
                    QHash<int, QPair<double, double>> dataMap;
                    for (int i = 0; i < 6; ++i)
                    {
                        dataMap.insert(i, QPair<double, double>(fi.primary[i], fi.secondary.value()[i]));
                    }
                    responseData->_allComponentValue.push_back(dataMap);
                }

                // 添加到响应数据列表
                _transferPathAnalysisData->_allResponseData.push_back(responseData);
            }

            //单元力数据
            else if (QString::fromStdString(file.schemaName) == "ELEMENT FORCES")
            {
                QPointer<mPostForceData> forceData = new mPostForceData();
                forceData->_forceType = 1;

                // 输出类型
                for (auto i : file.labels)
                {
                    if (QString::fromStdString(i.name).contains("MAGNITUDE-PHASE"))
                        forceData->_outputType = 1;
                    else if (QString::fromStdString(i.name).contains("REAL-IMAGINARY"))
                        forceData->_outputType = 2;
                }
                // 工况 ID
                forceData->_subcaseID = file.subcaseId;

                // 单元类型（对应 $ELEMENT TYPE）
                forceData->_elemenType = int(file.elementTypeCode);
                // 结果类型：按频率 / 按单元 ID
                for (auto i : file.labels)
                {
                    if (QString::fromStdString(i.name).contains("FREQUENCY"))
                        forceData->_resultType = 1;
                    else if (QString::fromStdString(i.name).contains("ELEMENT ID"))
                        forceData->_resultType = 2;
                }

                // 块头标识
                if (forceData->_resultType == 1)
                    forceData->_frequency;                                  // (没有进行验证，无法确认该部分是否正确
                else if (forceData->_resultType == 2)
                    forceData->_elementId.push_back(file.elementId);

                // 遍历数据记录
                for (auto fi : file.records)
                {
                    if (!fi.secondary.has_value())
                        break;

                    // 补充标识
                    if (forceData->_resultType == 1) {
                        // 每个记录首行是单元 ID
                        forceData->_elementId.push_back(int(fi.abscissa));
                    }
                    else if (forceData->_resultType == 2) {
                        // 每个记录首行是频率
                        forceData->_frequency.push_back(double(fi.abscissa));
                    }

                    // 构造 6 分量数据
                    QHash<int, QPair<double, double>> dataMap;
                    for (int i = 0; i < 6; ++i)
                    {
                        dataMap.insert(i, QPair<double, double>(fi.primary[i], fi.secondary.value()[i]));
                    }

                    forceData->_allComponentValue.push_back(dataMap);
                }

                _transferPathAnalysisData->_allForceData.push_back(forceData);
            }

            //  节点力数据 
            else if (QString::fromStdString(file.schemaName) == "GRID POINT FORCES")
            {
                QPointer<mPostForceData> forceData = new mPostForceData();
                forceData->_forceType = 2;

                // 结果类型：按频率 / 按节点
                for (auto i : file.labels)
                {
                    if (QString::fromStdString(i.name).contains("MAGNITUDE-PHASE"))
                        forceData->_outputType = 1;
                    else if (QString::fromStdString(i.name).contains("REAL-IMAGINARY"))
                        forceData->_outputType = 2;
                }
                forceData->_subcaseID = file.subcaseId;

                forceData->_frequency.append(file.abscissa.value());

                // 遍历数据记录
                for (auto fi : file.elementRecords)
                {
                    if (fi.gridId==fi.elementId)
                    {
                        forceData->_nodeElementID.push_back(QPair<int, int>(fi.gridId, fi.elementId));

                        // 构造 6 分量数据
                        QHash<int, QPair<double, double>> dataMap;

                        for (int i = 0; i < 6; ++i)
                        {
                            dataMap.insert(i, (QPair<double, double>(fi.values[i], fi.values[i + 6])));
                        }
                        forceData->_allComponentValue.push_back(dataMap);
                    }
                }

                _transferPathAnalysisData->_allForceData.push_back(forceData);
            }

        }

    }

    void mPostTransferPathAnalysis::applyAxisType(PlotView::mPlotView* plotView, int axisType)
    {
        if (!plotView) return;
        PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
        bool wasLog = yAxis.isLog;
        yAxis.isLog = (axisType == 1);
        yAxis.autoScale = true;
        plotView->updateAxisLog(PlotView::Y_Left, yAxis);

        // 切换到对数坐标后，必须根据实际数据重新计算范围，否则可能因旧范围包含 0 或极小值导致显示异常
        if (yAxis.isLog && !wasLog)
        {
            plotView->rescaleAxes();
        }
    }

    void mPostTransferPathAnalysis::setLegendInfoTextOnPlot(PlotView::mPlotView* plotView, const QStringList& infoLines)
    {
        if (!plotView || infoLines.isEmpty()) return;
        QwtPlot* qwtPlot = plotView->getQwtPlot();
        if (!qwtPlot) return;

        static QHash<QwtPlot*, QwtPlotMarker*> infoMarkers;
        QwtPlotMarker* marker = infoMarkers.value(qwtPlot, nullptr);
        if (!marker)
        {
            marker = new QwtPlotMarker();
            marker->setRenderHint(QwtPlotItem::RenderAntialiased, true);
            infoMarkers.insert(qwtPlot, marker);
        }
        marker->detach();
        QString html = QString("<div style='font-family: Arial; font-size: 12px; color: black;'>%1</div>")
            .arg(infoLines.join("<br>"));
        QwtText text(html);
        text.setRenderFlags(Qt::AlignCenter | Qt::AlignTop);
        marker->setLabel(text);
        marker->setLabelAlignment(Qt::AlignCenter | Qt::AlignTop);
        marker->setValue((qwtPlot->axisInterval(QwtPlot::xBottom).minValue() + qwtPlot->axisInterval(QwtPlot::xBottom).maxValue()) * 0.5,
            qwtPlot->axisInterval(QwtPlot::yLeft).maxValue());
        marker->attach(qwtPlot);
        qwtPlot->replot();
    }

    // 为每个贡献者生成一组稳定的绘制颜色（前向声明，定义在文件末尾）
    QColor getBarColorByIndex(int index);

    void mPostTransferPathAnalysis::plotBarChart(PlotView::mPlotView* plotView, const QString& name,
        const QStringList& xLabels, const QVector<double>& yData, const QString& title)
    {
        if (!plotView) return;
        QwtPlot* qwtPlot = plotView->getQwtPlot();
        if (!qwtPlot) return;

        plotView->clearAll();
        plotView->setTitle(title);
        qwtPlot->setAxisTitle(QwtPlot::xBottom, "Contributors");

        const int n = yData.size();
        QVector<double> xValues;
        for (int i = 0; i < n; ++i)
            xValues.append(static_cast<double>(i));

        // 为每个贡献者单独创建一条“柱状曲线”，使图例可按类别显隐
        for (int i = 0; i < n; ++i)
        {
            QString curveName = xLabels.value(i, QString("%1_%2").arg(name).arg(i));
            QVector<double> singleX = { static_cast<double>(i) };
            QVector<double> singleY = { yData[i] };

            plotView->updateCurveData(curveName, singleX, singleY);
            PlotView::CurveSettings cs = plotView->getCurveSettings(curveName);
            cs.showLine = false;
            cs.showSymbol = false;
            cs.showBars = true;
            cs.barColor = getBarColorByIndex(i);
            // 柱子宽度：按类别数量动态计算，使柱子间距更紧凑
            cs.barWidth = qMax(8.0, qMin(40.0, 800.0 / qMax(1, n)));
            plotView->setCurveStyle(curveName, cs);
        }

        // 自定义 X 轴标签
        qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, new BarChartScaleDraw(xLabels));
        qwtPlot->setAxisMaxMajor(QwtPlot::xBottom, xLabels.size());
        qwtPlot->setAxisMaxMinor(QwtPlot::xBottom, 0);
        // 限制 X 轴显示范围，让柱子更紧凑，刻度标签居中
        PlotView::AxisSettings xAxis = plotView->getAxisSettings(PlotView::X_Bottom);
        xAxis.autoScale = false;
        xAxis.min = -0.5;
        xAxis.max = static_cast<double>(n) - 0.5;
        plotView->updateAxisRange(PlotView::X_Bottom, xAxis);

        // 设置 Y 轴范围（参考旧 mBarChart 逻辑），顶部留出空间显示数值标签
        double yMin = 0.0, yMax = 0.0;
        if (!yData.isEmpty())
        {
            yMin = yMax = yData[0];
            for (double v : yData)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
        }
        double minVal, maxVal;
        if (yMin > 0) minVal = 0;
        else minVal = yMin * 1.2;
        maxVal = yMax * 1.15;
        PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
        yAxis.autoScale = false;
        yAxis.min = minVal;
        yAxis.max = maxVal;
        plotView->updateAxisRange(PlotView::Y_Left, yAxis);

        // 添加 y=0 参考线
        auto oldZeroLineIt = _zeroLineMarkers.find(plotView);
        if (oldZeroLineIt != _zeroLineMarkers.end())
        {
            QwtPlotMarker* oldZeroLine = oldZeroLineIt.value().second;
            QwtPlot* oldPlot = oldZeroLineIt.value().first;
            if (oldZeroLine)
            {
                if (oldPlot)
                    oldZeroLine->detach();
                delete oldZeroLine;
            }
            _zeroLineMarkers.erase(oldZeroLineIt);
        }
        QwtPlotMarker* zeroLine = new QwtPlotMarker();
        zeroLine->setLineStyle(QwtPlotMarker::HLine);
        zeroLine->setYValue(0.0);
        QPen zeroPen(Qt::black);
        zeroPen.setWidthF(1.0);
        zeroPen.setStyle(Qt::DashLine);
        zeroLine->setLinePen(zeroPen);
        zeroLine->setZ(10.0);
        zeroLine->attach(qwtPlot);
        _zeroLineMarkers.insert(plotView, qMakePair(QPointer<QwtPlot>(qwtPlot), zeroLine));

        // 在柱顶添加数值标签
        addBarValueLabels(plotView, name, xValues, yData);

        plotView->setPlotLegendItemVisible(true);
        if (plotView->getPlotLegendItem())
        {
            plotView->getPlotLegendItem()->setPosition(LegendPosition::TopRight);
        }
        plotView->getQwtPlot()->replot();
    }

    void mPostTransferPathAnalysis::cachePlotData(const QString& vname, const PlotDataCache& cache)
    {
        _plotDataCache[vname] = cache;
    }

    /**
     * @brief 将幅值和相位转换为实部和虚部
     * @param magnitude 幅值
     * @param phaseInDegrees 相位（角度制）
     * @return 返回实部和虚部组成的QPair
     * @details 转换公式：real = mag * cos(phase), imag = mag * sin(phase)
     */
    QPair<double, double> mPostTransferPathAnalysis::convertToRealImag(double magnitude, double phaseInDegrees)
    {
        // 将相位从角度转换为弧度
        double phaseInRadians = phaseInDegrees * M_PI / 180.0;

        // 计算实部和虚部
        double real = magnitude * std::cos(phaseInRadians);
        double imag = magnitude * std::sin(phaseInRadians);

        return qMakePair(real, imag);
    }

    /**
     * @brief 将Hash中的幅值-相位数据批量转换为实部-虚部数据
     * @param inputHash 输入的幅值-相位数据Hash
     * @return 返回转换后的实部-虚部数据Hash
     */
    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysis::convertMagnitudePhaseToRealImag(const QHash<QString, QVector<QPair<double, double>>>& inputHash)
    {
        QHash<QString, QVector<QPair<double, double>>> outputHash;
        for (auto it = inputHash.begin(); it != inputHash.end(); ++it)
        {
            const QString& key = it.key();
            const QVector<QPair<double, double>>& inputVector = it.value();
            QVector<QPair<double, double>> outputVector;

            for (const auto& pair : inputVector)
            {
                double magnitude = pair.first;
                double phaseInDegrees = pair.second;

                QPair<double, double> realImagPair = convertToRealImag(magnitude, phaseInDegrees);
                outputVector.append(realImagPair);
            }
            outputHash.insert(key, outputVector);
        }
        return outputHash;
    }

    /**
     * @brief 计算输入力与传递函数的乘积（复数乘法）
     * @param forceData 输入力数据（实部、虚部）
     * @param transferData 传递函数数据（实部、虚部）
     * @return 返回各路径的乘积结果（实部、虚部）
     * @details 复数乘法公式：(a+bi)*(c+di) = (ac-bd) + (ad+bc)i
     */
    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysis::calProductForcaDataAndTransferData(const QHash<QString, QVector<QPair<double, double>>> forceData, const QHash<QString, QVector<QPair<double, double>>> transferData)
    {
        QHash<QString, QVector<QPair<double, double>>> forceAndTransferData;
        for (auto pathname : _allTransferPath)
        {
            QVector<QPair<double, double>> vecProduct;
            QVector<QPair<double, double>> pathForce = forceData.value(pathname);
            QVector<QPair<double, double>> pathTransfer = transferData.value(pathname);

            for (int i = 0; i < pathForce.size(); i++)
            {
                QPair<double, double> pairForce = pathForce[i];
                QPair<double, double> pairTransfer = pathTransfer[i];

                // 复数相乘
                double real = pairForce.first * pairTransfer.first - pairForce.second * pairTransfer.second;
                double imag = pairForce.first * pairTransfer.second + pairForce.second * pairTransfer.first;
                QPair<double, double> pairProduct = qMakePair(real, imag);
                vecProduct.append(pairProduct);
            }
            forceAndTransferData.insert(pathname, vecProduct);
        }
        return forceAndTransferData;
    }

    /**
     * @brief 计算输入力与传递函数乘积的幅值和相位
     * @param froceTransRIData 输入力与传递函数乘积的实部-虚部数据
     * @return 返回各路径的幅值-相位数据
     * @details 幅值 = sqrt(real^2 + imag^2)，相位 = atan2(imag, real) * 180 / PI
     */
    QHash<QString, QVector<QPair<double, double>>> mPostTransferPathAnalysis::calForcaAndTransferMPData(const QHash<QString, QVector<QPair<double, double>>> froceTransRIData)
    {
        QHash<QString, QVector<QPair<double, double>>> mpData;
        for (auto path : _allTransferPath)
        {
            QVector<QPair<double, double>> vecMPPair;
            QVector<QPair<double, double>> vecRIPair = froceTransRIData[path];
            for (int i = 0; i < vecRIPair.size(); i++)
            {
                double real = vecRIPair[i].first;
                double imag = vecRIPair[i].second;
                double magnitude = sqrt(real * real + imag * imag);
                double phase = atan2(imag, real) * 180 / M_PI;
                if (phase < 0)
                {
                    phase = 360 - abs(phase);
                }
                QPair<double, double> mpPair = qMakePair(magnitude,phase);
                vecMPPair.push_back(mpPair);
            }
            mpData.insert(path, vecMPPair);
        }
        return mpData;
    }

    /**
     * @brief 计算所有路径在各频率下的实部和虚部总和
     * @param forceTransferRIData 各路径的实部-虚部数据
     * @return 返回各频率点的实部-虚部总和
     * @details 对每个频率点，将所有路径的实部和虚部分别累加
     */
    QVector <QPair<double, double>> mPostTransferPathAnalysis::calForceAndTransferRISum(QHash<QString, QVector<QPair<double, double>>> forceTransferRIData)
    {
        QVector <QPair<double, double>> sumRealImag;
        // 根据传入数据的实际大小确定循环次数，避免与 _allFrequence 大小不一致导致 X/Y 数据点不匹配
        int dataSize = 0;
        if (!_allTransferPath.isEmpty() && forceTransferRIData.contains(_allTransferPath.first()))
        {
            dataSize = forceTransferRIData[_allTransferPath.first()].size();
        }
        for (int i = 0; i < dataSize; i++)
        {
            QPair<double, double> pairSum;
            double realSum = 0;
            double imagSum = 0;
            for (auto pathname : _allTransferPath)
            {
                if (!forceTransferRIData.contains(pathname))
                {
                    continue;
                }
                QVector<QPair<double, double>> vecPair = forceTransferRIData[pathname];
                if (vecPair.size() <= i)
                {
                    continue;
                }
                QPair<double, double> pairval = vecPair[i];
                realSum += pairval.first;
                imagSum += pairval.second;
            }
            pairSum = qMakePair(realSum, imagSum);
            sumRealImag.push_back(pairSum);

        }
        return sumRealImag;
    }

    /**
     * @brief 将实部-虚部总和转换为幅值-相位
     * @param RIdata 实部-虚部数据
     * @return 返回幅值-相位数据
     * @details 幅值 = sqrt(real^2 + imag^2)，相位 = atan2(imag, real) * 180 / PI
     */
    QVector <QPair<double, double>> mPostTransferPathAnalysis::calForceAndTransferMPSum(QVector<QPair<double, double>> RIdata)
    {
        QVector <QPair<double, double>> MPdata;
        for (auto RIpair : RIdata)
        {
            double real = RIpair.first;
            double imag = RIpair.second;
            double magnitude = sqrt(real * real + imag *imag);
            double phase = atan2(imag,real) * 180 / M_PI;
            if (phase < 0)
            {
                phase = 360 - abs(phase);
            }
            QPair<double, double> MPpair = qMakePair(magnitude, phase);
            MPdata.push_back(MPpair);
        }
        return MPdata;
    }

    /**
     * @brief 添加视图到表格控件
     * @param viewname 视图名称
     * @details 在表格中新增一行，显示视图名称和删除按钮，并连接点击信号
     */
    void mPostTransferPathAnalysis::addTableWidgetView(QString viewname)
    {
        if (!_showViewName.contains(viewname))
        {
            _showViewName.append(viewname);
            const int nrow = _ui->tableWidget->rowCount();
            _ui->tableWidget->insertRow(nrow);
            QTableWidgetItem* item = new QTableWidgetItem(viewname);
            item->setToolTip(viewname);
            _ui->tableWidget->setItem(nrow, 0, item);
            connect(_ui->tableWidget, &QTableWidget::itemClicked, this, &mPostTransferPathAnalysis::slot_onCellClicked);

            QToolButton* tb = new QToolButton;
            tb->setIcon(QIcon(":/mMainWidgets/Icon/clear.png"));
            connect(tb, SIGNAL(clicked()), this, SLOT(slot_deleteCurvePlot()));
            _ui->tableWidget->setCellWidget(nrow, 1, tb);
        }
    }

    /**
     * @brief 设置当前选中的视图
     * @param itemname 视图名称
     * @details 在表格中查找对应的项并触发点击事件，使其成为当前活动视图
     */
    void mPostTransferPathAnalysis::setCurrentSelectView(QString itemname)
    {
        for (int row = 0; row < _ui->tableWidget->rowCount(); ++row)
        {
            QTableWidgetItem *item = _ui->tableWidget->item(row, 0);
            if (item && item->text() == itemname)
            {
                _ui->tableWidget->itemClicked(item);
                _ui->tableWidget->setCurrentItem(item);
                break;
            }
        }
    }

    /**
     * @brief 从表格中移除视图
     * @param viewname 要移除的视图名称
     * @details 在表格中查找对应的行并删除，同时更新视图名称列表
     */
    void mPostTransferPathAnalysis::removeTableView(QString viewname)
    {
        for (int row = 0; row < _ui->tableWidget->rowCount(); ++row)
        {
            QTableWidgetItem *item = _ui->tableWidget->item(row, 0);
            if (item && item->text() == viewname)
            {
                _ui->tableWidget->removeRow(row);
                if (_showViewName.contains(viewname))
                {
                    int index = _showViewName.indexOf(viewname);
                    _showViewName.removeAt(index);
                }
                break;
            }
        }
    }

    // 为每个贡献者生成一组稳定的绘制颜色（统一淡蓝色）
    QColor getBarColorByIndex(int index)
    {
        Q_UNUSED(index);
        return QColor(173, 216, 230);   // 淡蓝
    }
    void mPostTransferPathAnalysis::addBarValueLabels(PlotView::mPlotView* plotView, const QString& curveName,
        const QVector<double>& xValues, const QVector<double>& yData)
    {
        Q_UNUSED(curveName);
        if (!plotView || xValues.size() != yData.size()) return;
        QwtPlot* qwtPlot = plotView->getQwtPlot();
        if (!qwtPlot) return;

        // 清理旧的数值标签：当前 plotView 上已有的 barValueLabel Marker
        QList<QwtPlotMarker*> oldMarkers = _barValueLabelMarkers.take(plotView);
        for (QwtPlotMarker* marker : oldMarkers)
        {
            if (marker)
            {
                marker->detach();
                delete marker;
            }
        }
        oldMarkers.clear();

        for (int i = 0; i < yData.size(); ++i)
        {
            double y = yData[i];
            double x = xValues[i];

            QwtPlotMarker* marker = new QwtPlotMarker();
            marker->setRenderHint(QwtPlotItem::RenderAntialiased, true);

            // 数值标签：极小/极大值使用科学计数法，常规值用固定精度
            QString labelText;
            if (qFuzzyIsNull(y))
            {
                labelText = "0";
            }
            else if (qAbs(y) >= 1e4 || qAbs(y) < 1e-3)
            {
                labelText = MxFunctions::mNumberToString(y, 'e', 3);
            }
            else
            {
                labelText = MxFunctions::mNumberToString(y, 'f', 3);
            }
            QwtText text(labelText);
            text.setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 8));
            text.setColor(Qt::black);
            text.setRenderFlags(Qt::AlignCenter | Qt::AlignBottom);
            marker->setLabel(text);
            // 标签位置：正值显示在柱顶上方（AlignTop），负值显示在柱子最下端外侧（AlignBottom）
            if (y >= 0)
            {
                marker->setLabelAlignment(Qt::AlignCenter | Qt::AlignTop);
                marker->setValue(x, y);
            }
            else
            {
                marker->setLabelAlignment(Qt::AlignCenter | Qt::AlignBottom);
                marker->setValue(x, y);
            }
            marker->attach(plotView->getQwtPlot());
            oldMarkers.append(marker);
        }

        _barValueLabelMarkers.insert(plotView, oldMarkers);
    }
}
