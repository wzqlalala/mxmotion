#include "mPostPanelParticipation.h"
#include "ui_mPostPanelParticipation.h"
#include "UiMetrics.h"

#include <QApplication>
#include <QFileDialog>
#include <QTextCodec>
#include <QPointer>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QTimer>
#include <QFileInfo>
#include <algorithm>
#include <limits>

#include "qwt_plot.h"
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>
#include <qwt_scale_draw.h>
#include <QPen>

#include "BarChartScaleDraw.h"
#include "mPlotView.h"
#include "mPostPlotFloatWindow.h"
#include "mGlobalSignals.h"
#include "mAppConfiguration.h"
#include "mPostPanelParticipationData.h"
#include "mPostDataTable.h"
#include "mDockLayoutManager.h"
#include "mDraggableInfoLabel.h"
#include <QMdiSubWindow>
#include "mOtherFunction.h"
//using namespace MIOFile;

bool compareByValuePanel(const QPair<QString, double> &pair1, const QPair<QString, double> &pair2)
{
    return abs(pair1.second) > abs(pair2.second);
}

//用于幅值总和排序
bool comparePairsPanel(const QPair<QString,double>& pair1, const QPair<QString, double>& pair2)
{
    return pair1.second > pair2.second;
}

using namespace PlotView;
using namespace MxFunctions;
//using namespace MViewBasic;

namespace PostWindow
{
    mPostPanelParticipation::mPostPanelParticipation(QWidget *parent)
        : QWidget(parent)
    {
        
        _ui = new Ui::mPostPanelParticipation;
        _ui->setupUi(this);

        //界面初始化
        _ui->tabWidget->setCurrentIndex(0);
        QStringList nums;
        for (int i = 1; i <= 30; i++)
        {
            nums.append(MxFunctions::mNumberToString(i));
        }
        _ui->comboBox_showOrder1->clear();
        _ui->comboBox_showOrder1->addItems(nums);
        _ui->comboBox_showOrder2->clear();
        _ui->comboBox_showOrder2->addItems(nums);
        _ui->comboBox_showOrder1->setCurrentText("10");
        _ui->comboBox_showOrder2->setCurrentText("10");

        _ui->comboBox_showMode->setCurrentIndex(0);
        _ui->stackedWidget_show->setCurrentIndex(0);

        initialEnabledUI(false);

        //设置列表框选择模式
        _ui->listWidget_mode->setSelectionMode(QAbstractItemView::ExtendedSelection);

        //连接信号与槽
        connect(_ui->pushButton_selectFile, SIGNAL(clicked()), this, SLOT(chooseFile()));
        connect(_ui->pushButton_loadFileOK, SIGNAL(clicked()), this, SLOT(loadFile()));
        connect(_ui->pushButton_loadResponseOK, SIGNAL(clicked()), this, SLOT(plotFrequencyResponseCurve()));
        connect(_ui->lineEdit_filePath, SIGNAL(textChanged(const QString &)), this, SLOT(filePathChanged(const QString &)));
        connect(_ui->comboBox_case, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseSubcaseID(const QString&)));
        connect(_ui->comboBox_case_2, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseSubcaseID(const QString&)));
        connect(_ui->comboBox_result, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseResultType(const QString&)));
        connect(_ui->comboBox_id, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseGrid(const QString&)));
        connect(_ui->comboBox_component, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(chooseComponent(const QString&)));
        connect(_ui->pushButton_studyOK, SIGNAL(clicked()), this, SLOT(plotStudyCurve()));
        connect(_ui->comboBox_showMode, SIGNAL(currentIndexChanged(int)), this, SLOT(chooseShowPanel(int)));
        connect(_ui->pushButton_showOK, SIGNAL(clicked()), this, SLOT(plotShowCurve()));
        connect(_ui->pushButton_showData, SIGNAL(clicked()), this, SLOT(showDataTable()));
        connect(_ui->pushButton_loadDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        connect(_ui->pushButton_showDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        connect(_ui->pushButton_studyData, SIGNAL(clicked()), this, SLOT(showDataTable()));
        connect(_ui->pushButton_studyDelete, SIGNAL(clicked()), this, SLOT(deleteCurveView()));
        connect(_ui->tabWidget, SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
		//connect(_ui->pushButton_return1, SIGNAL(clicked()), this, SIGNAL(returnSig()));
		//connect(_ui->pushButton_return2, SIGNAL(clicked()), this, SIGNAL(returnSig()));
		//connect(_ui->pushButton_return3, SIGNAL(clicked()), this, SIGNAL(returnSig()));

        //切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);

    }


    mPostPanelParticipation::~mPostPanelParticipation()
    {
        // 清理按 plotView 缓存的 Marker 列表
        for (auto it = _barValueLabelMarkers.begin(); it != _barValueLabelMarkers.end(); ++it)
        {
            for (QwtPlotMarker* marker : it.value())
            {
                if (marker)
                {
                    marker->detach();
                    delete marker;
                }
            }
        }
        _barValueLabelMarkers.clear();

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

        // 清理 widget，但 QDockWidget 由 ADS 管理，不需要 close 或 delete
        for (auto it = _plotFloatWindows.begin(); it != _plotFloatWindows.end(); ++it) {
            it.value()->setParent(nullptr);  // 从 dock 中脱离
            delete it.value();              // 删除 mPostPlotFloatWindow 内部持有的 plotView
        }
        _plotFloatWindows.clear();

        if (_ui != nullptr)
            delete _ui;
    }

    void mPostPanelParticipation::setPanelPickAndSignal(bool isshow)
    {
        //if (isshow)
        //{
        //    mPostRend* postRend = mViewportManager::getInstance()->getActivePostRend();
        //    if (postRend == nullptr)
        //    {
        //        return;
        //    }
        //    auto postRender = postRend->getPostRender();
        //    if (postRender == nullptr)
        //    {
        //        return;
        //    }
        //    postRender->setPickFilter(PickFilter::PickNothing);
        //}
    }

    void mPostPanelParticipation::initialEnabledUI(bool isEnabled)
    {
        _ui->comboBox_case->setEnabled(isEnabled);
        _ui->comboBox_case_2->setEnabled(isEnabled);
        _ui->comboBox_result->setEnabled(isEnabled);
        _ui->comboBox_id->setEnabled(isEnabled);
        _ui->comboBox_component->setEnabled(isEnabled);
        _ui->pushButton_loadResponseOK->setEnabled(isEnabled);
        _ui->comboBox_showMode->setEnabled(isEnabled);
        _ui->comboBox_showFrequency->setEnabled(isEnabled);
        _ui->comboBox_showOrder1->setEnabled(isEnabled);
        _ui->comboBox_showType1->setEnabled(isEnabled);
        _ui->pushButton_showOK->setEnabled(isEnabled);
        _ui->comboBox_studyFrequencyMin->setEnabled(isEnabled);
        _ui->comboBox_studyFrequencyMax->setEnabled(isEnabled);
        _ui->lineEdit_excludeRatio->setEnabled(isEnabled);
        _ui->listWidget_mode->setEnabled(isEnabled);
        _ui->pushButton_studyOK->setEnabled(isEnabled);
        _ui->pushButton_showData->setEnabled(isEnabled);
        _ui->pushButton_showDelete->setEnabled(isEnabled);
        _ui->pushButton_studyData->setEnabled(isEnabled);
        _ui->pushButton_studyDelete->setEnabled(isEnabled);

        _ui->comboBox_case->clear();
        _ui->comboBox_case_2->clear();
        _ui->comboBox_result->clear();
        _ui->comboBox_id->clear();
        _ui->comboBox_component->clear();
        _ui->comboBox_showFrequency->clear();
        _ui->comboBox_studyFrequencyMin->clear();
        _ui->comboBox_studyFrequencyMax->clear();
        _ui->lineEdit_excludeRatio->clear();
        _ui->listWidget_mode->clear();
        _ui->comboBox_showOrder1->setCurrentText("10");
    }

    QVector<double> mPostPanelParticipation::getDifferenceData(QVector<double> oldFre, QVector<double> newFre)
    {
        QVector<double> difs;
        for (int i = 0; i < oldFre.size(); i++)
        {
            double dif = 0.0;
            if (qAbs(oldFre[i]) > 1e-12)
            {
                dif = (newFre[i] - oldFre[i]) / oldFre[i] * 100;
            }
            else
            {
                dif = (qAbs(newFre[i]) > 1e-12) ? 100.0 : 0.0;
            }
            // 截断到 [-100, 100] 范围，确保在 yRight 轴固定范围内可见
            if (dif > 100.0) dif = 100.0;
            if (dif < -100.0) dif = -100.0;
            difs.push_back(dif);
        }
        return difs;
    }

    bool mPostPanelParticipation::generateCurveData()
    {
        // 数据已迁移到 _plotDataCache，无需在此处维护旧成员
        return true;
    }

    void mPostPanelParticipation::chooseFile()
    {
        //打开文件
        QString dir;
        if (!filePath_.isEmpty())
            dir = QFileInfo(filePath_).absolutePath();
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
        initialEnabledUI(false);
        _ui->lineEdit_filePath->setText(filePath);
        filePath_ = filePath;
    }

    void mPostPanelParticipation::loadFile()
    {
        if (filePath_.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("未选择文件！"), true);
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "未选择文件！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "Unselected File!", QMessageBox::Ok);
            }
            return;
        }
        else if (!filePath_.contains(".pch"))
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("文件格式错误，请导入 .pch 文件！"), true);
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "文件错误！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "File Error!", QMessageBox::Ok);
            }
            return;
        }

        //读取文件内容
        bool isOpen = readPanelData(filePath_);
        if (isOpen)
        {
            QFileInfo fileInfo(filePath_);
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("面板贡献量文件导入成功：%1").arg(fileInfo.fileName()), true);
            initialEnabledUI(true);
        }
        else
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("面板贡献量文件打开失败！"), true);
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "文件打开失败！", QMessageBox::Ok);
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "File open failed!", QMessageBox::Ok);
			}
			return;
        }

        //填充导入界面
        //获取所有工况id
		if (panelParticipationData_ == nullptr)
		{
            mGlobalSignals::getInstance()->outputMessageSig(0, QString("面板贡献量文件数据错误！"), true);
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "文件错误！", QMessageBox::Ok);
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "File Error!", QMessageBox::Ok);
			}
			return;
		}
        QVector<int> subcaseIDs = panelParticipationData_->getAllSubcaseID();
        QStringList listID;
        for (int id : subcaseIDs)
        {
            QString strCase = "Subcase_";
            listID.append(strCase + MxFunctions::mNumberToString(id));
        }
        listID = panelParticipationData_->removeDuplicates(listID);
        _ui->comboBox_case->blockSignals(true);
        _ui->comboBox_case->clear();
        _ui->comboBox_case->blockSignals(false);
        _ui->comboBox_case->addItems(listID);


        _ui->comboBox_case_2->blockSignals(true);
        _ui->comboBox_case_2->clear();
        _ui->comboBox_case_2->blockSignals(false);
        _ui->comboBox_case_2->addItems(listID);
        //显示界面

        //初始化研究界面
        QRegExp regx("[0-9,.]+$");
        QValidator *validator = new QRegExpValidator(regx, _ui->lineEdit_excludeRatio);
        _ui->lineEdit_excludeRatio->setValidator(validator);
        _ui->lineEdit_excludeRatio->setText("100");
    }

    void mPostPanelParticipation::chooseSubcaseID(const QString& caseID)
    {
        QString strCase = caseID;
        if (strCase.contains("_"))
        {
            QStringList strList = strCase.split("_");
            if (strList.size() == 2)
            {
                subcaseID_ = strList.at(1).toInt();
            }
        }
        //获取工况ID对应的结果类型
        QStringList results = panelParticipationData_->getResultTypeBySubcaseID(subcaseID_);
        QStringList results2 = panelParticipationData_->removeDuplicates(results);//去重
        _ui->comboBox_result->blockSignals(true);
        _ui->comboBox_result->clear();
        _ui->comboBox_result->blockSignals(false);
        _ui->comboBox_result->addItems(results2);
    }

    void mPostPanelParticipation::chooseResultType(const QString& resultType)
    {
        //根据结果类型及工况ID获取gridID和分量
        resultType_ = resultType;
        //获取gridID和分量
        QVector<int> ids = panelParticipationData_->getGridIDBySubcaseIDAndResultType(subcaseID_, resultType_);
        ids = panelParticipationData_->removeDuplicates(ids);

        _ui->comboBox_id->blockSignals(true);
        _ui->comboBox_id->clear();
        _ui->comboBox_id->blockSignals(false);
        for (int id : ids)
        {
            _ui->comboBox_id->addItem(MxFunctions::mNumberToString(id));
        }

        gridID_ = _ui->comboBox_id->currentText().toInt();
    }

    void mPostPanelParticipation::chooseGrid(const QString& gridid)
    {
        gridID_ = gridid.toInt();

        QStringList component = panelParticipationData_->getComponentBySubcaseIDAndResultType(subcaseID_, resultType_, gridID_);
        component = panelParticipationData_->removeDuplicates(component);
        _ui->comboBox_component->blockSignals(true);
        _ui->comboBox_component->clear();
        _ui->comboBox_component->blockSignals(false);
        _ui->comboBox_component->addItems(component);
    }

    void mPostPanelParticipation::chooseComponent(const QString& component)
    {
        component_ = component;

        //显示、研究界面显示频率，研究界面模态选择，与导入下拉框联动
        //获取所有频率
        allFrequence_ = panelParticipationData_->getAllFrequencyBySubcaseIDAndResultType(subcaseID_, resultType_, gridID_,component_);
        QString frequence;
        QStringList freList;
        for (double fre : allFrequence_)
        {
            frequence = MxFunctions::mNumberToString(fre);
            freList.append(frequence);
        }

        //显示部分显示频率
        _ui->comboBox_showFrequencyMin->clear();
        _ui->comboBox_showFrequencyMax->clear();
        _ui->comboBox_showFrequencyMin->addItems(freList);
        _ui->comboBox_showFrequencyMax->addItems(freList);
        _ui->comboBox_showFrequencyMin->setCurrentIndex(0);
        _ui->comboBox_showFrequencyMax->setCurrentIndex(freList.size() - 1);
        _ui->comboBox_showFrequency->clear();
        _ui->comboBox_showFrequency->addItems(freList);

        //研究部分显示频率
        _ui->comboBox_studyFrequencyMin->clear();
        _ui->comboBox_studyFrequencyMax->clear();
        _ui->comboBox_studyFrequencyMin->addItems(freList);
        _ui->comboBox_studyFrequencyMax->addItems(freList);
        _ui->comboBox_studyFrequencyMin->setCurrentIndex(0);
        _ui->comboBox_studyFrequencyMax->setCurrentIndex(freList.size() - 1);

        //获取研究部分可选模态
        QSet<QString> modes = panelParticipationData_->getPanelIndexBySubcaseIDAndResultType(subcaseID_, resultType_);
        //对模态阶数进行排序
        QList<QString> modeIndex = QList<QString>::fromSet(modes);
        qSort(modeIndex);

        QString str = "Panel ";
        _ui->listWidget_mode->clear();
        for (int i = 0; i < modeIndex.size(); i++)
        {
            QString strPanel = modeIndex[i];
            _ui->listWidget_mode->addItem(str + strPanel);
        }
    }

    bool mPostPanelParticipation::readPanelData(QString filePath)
    {
        //读取文件内容
        QTextCodec *codeC = QTextCodec::codecForName("GBK");
        QFile file(filePath);
        QFileInfo fileinfo(filePath);

        //以读的方式打开文件
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;

        panelParticipationData_ = new mPostPanelParticipationData();
        QString currentline;
        while (!file.atEnd())
        {
            currentline = file.readLine().simplified();
            //读取初始模态数据
            if (currentline.contains("EIGEN MODES"))
            {
                //面板模态中不读取该信息
                //QPointer<initialPanelData> iniPanelData = new initialPanelData();
                //QStringList linelist = currentline.split("=");
                //if (linelist.size() == 2)
                //{
                //    iniPanelData->eigenPanel = linelist.at(1).simplified();
                //}
                //currentline = file.readLine().simplified();
                //if (currentline.contains("SUBCASE ID"))
                //{
                //    linelist = currentline.split("=");
                //    if (linelist.size() == 2)
                //    {
                //        iniPanelData->subcaseID = linelist.at(1).simplified().toInt();
                //    }
                //}
                //currentline = file.readLine().simplified();
                //while (!currentline.contains("TITLE"))
                //{
                //    linelist = currentline.split(" ");
                //    if (linelist.size() == 2)
                //    {
                //        int mode = linelist.at(0).toInt();
                //        double frequnece = linelist.at(1).toDouble();
                //        iniPanelData->initialPanelFrequnecy.insert(mode, frequnece);
                //    }
                //    currentline = file.readLine().simplified();
                //}
                //panelParticipationData_->allIniPanelData_.append(iniPanelData);
            }
            //读取频率模态信息
            else if (currentline.contains("PARTICIPATION FACTORS"))
            {
                QPointer<mPostFrequencyPanelData> frequencyPanelData = new mPostFrequencyPanelData();
                QStringList linelist = currentline.split("=");
                if (linelist.size() == 2)
                {
                    frequencyPanelData->participationFactors_ = linelist.at(1).simplified();
                }
                for (int i = 0; i < 3; i++)
                {
                    currentline = file.readLine().simplified();
                }
                if (currentline.contains("SUBCASE ID"))
                {
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        frequencyPanelData->subcaseId_ = linelist.at(1).simplified().toInt();
                    }
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("FREQUENCY"))
                {
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        frequencyPanelData->frequence_ = linelist.at(1).simplified().toDouble();
                    }
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("GRIDID"))
                {
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        if (linelist.at(1).contains("/"))
                        {
                            QStringList gridList = linelist.at(1).split("/");
                            frequencyPanelData->gridID_ = gridList.at(0).simplified().toInt();
                            frequencyPanelData->component_ = gridList.at(1).simplified();
                        }
                        else
                        {
                            frequencyPanelData->gridID_ = linelist.at(1).simplified().toInt();
                        }
                    }
                }
                currentline = file.readLine().simplified();
                if (currentline.contains("TOTAL RESPONSE"))
                {
                    linelist = currentline.split("=");
                    if (linelist.size() == 2)
                    {
                        if (linelist.at(1).contains(" "))
                        {
                            QString response = linelist.at(1).simplified();
                            QStringList responseList = response.split(" ");
                            frequencyPanelData->totalResponse_ = responseList.at(0).simplified().toDouble();
                            frequencyPanelData->totalAngle_ = responseList.at(1).simplified().toDouble();
                        }
                    }
                }
                currentline = file.readLine().simplified();
                while (!currentline.contains("TITLE"))
                {
                    linelist = currentline.split(" ");
                    if (linelist.size() == 3)
                    {
                        QPair<double, double> pairVal;
                        pairVal.first = linelist.at(1).toDouble();
                        pairVal.second = linelist.at(2).toDouble();

                        QString id = linelist.at(0);
                        frequencyPanelData->panelResponse_.insert(id, pairVal);
                    }
                    currentline = file.readLine().simplified();
                    if (currentline.isEmpty())
                    {
                        break;
                    }
                }
                panelParticipationData_->allFrequencyPanelData_.append(frequencyPanelData);
            }
        }
        file.close();
        return true;
    }

    void mPostPanelParticipation::plotFrequencyResponseCurve()
    {
        //根据界面结果选择判断获取哪些频率数据
        //获取所有频率
        allFrequence_ = panelParticipationData_->getAllFrequencyBySubcaseIDAndResultType(subcaseID_, resultType_,gridID_, component_);
        //获取所有响应
        allTotalResponse_ = panelParticipationData_->getAllTotalResponseBySubcaseIDAndResultType(subcaseID_, resultType_, gridID_, component_);
        if (allFrequence_.size() == 0 || allTotalResponse_.size() == 0)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "当前结果没有模态数据！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "The current result has no modal data!", QMessageBox::Ok);
            }
            return;
        }

        //创建新的曲线视口
        PlotView::mPlotView* plotView = nullptr;
        QString vname = "importCurveView_Panel";
        plotView = createNewCurvePlot(vname);
        importCurveView_ = vname;
        if (plotView == nullptr)
            return;

        //获取ID和结果类型
        QString id = _ui->comboBox_id->currentText();
        QString type = _ui->comboBox_result->currentText();

        int axistype = _ui->comboBox_loadAxisType->currentIndex();
        QString strType;
        if (axistype == 0)
        {
            strType = "(Linear)";
        }
        else if (axistype == 1)
        {
            strType = "(Log)";
        }

        //绘制频响曲线
        QString curveName = "Node" + id + " " + type + " Panel-Response";
        QString xTitle = "Frequency";
        QString yTitle = QString("Response%1").arg(strType);
        QString title = QString("Response @ %1").arg(id);

        plotView->clearAll();
        plotView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, xTitle);
        plotView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, yTitle);
        plotView->setTitle(title);

        // 对数坐标下 Y 值必须大于 0，否则 Qwt 对数轴会把下限拉到极小值（如 1e-140）
        QVector<double> plotResponses = allTotalResponse_;
        if (axistype == 1)
        {
            double minPositive = std::numeric_limits<double>::max();
            for (double v : plotResponses)
            {
                if (v > 0 && v < minPositive)
                    minPositive = v;
            }
            double replacement = (minPositive < std::numeric_limits<double>::max()) ? (minPositive * 0.1) : 1e-12;
            for (double& v : plotResponses)
            {
                if (v <= 0)
                    v = replacement;
            }
        }

        plotView->updateCurveData(curveName, allFrequence_, plotResponses);
        applyAxisType(plotView, axistype);
        plotView->setPlotLegendItemVisible(true);
        plotView->rescaleAxes();
        // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
        plotView->setEnableMarker(true);

        // 对数坐标下，QwtPlotCurve 的默认 baseline=0 会干扰自动范围计算，导致下限被拉到极小值，
        // 因此在这里根据实际数据手动限定 Y 轴范围。
        if (axistype == 1 && !plotResponses.isEmpty())
        {
            double yMin = plotResponses.first();
            double yMax = plotResponses.first();
            for (double v : plotResponses)
            {
                if (v < yMin) yMin = v;
                if (v > yMax) yMax = v;
            }
            // 给一点对数边距：下限降到下一个数量级，上限提到上一个数量级
            double lower = yMin * 0.1;
            double upper = qMax(yMax * 10.0, lower * 10.0);

            PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
            yAxis.autoScale = false;
            yAxis.isLog = true;
            yAxis.min = lower;
            yAxis.max = upper;
            plotView->updateAxisRange(PlotView::Y_Left, yAxis);
        }

        PlotDataCache cache;
        cache.viewName = vname;
        cache.xTitle = xTitle;
        cache.yTitles.append(yTitle);
        cache.xValue = allFrequence_;
        cache.curveDatas.append(qMakePair(curveName, allTotalResponse_));
        cache.curveNames.append(curveName);
        cachePlotData(vname, cache);

        // 在独立浮动窗口中显示
        openPlotInFloatWindow(vname, plotView);
        _ui->tabWidget->setCurrentIndex(1);
    }

    void mPostPanelParticipation::plotStudyCurve()
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
                MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "请选择需要去除的模态！", QMessageBox::Ok);
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "Please select the mode you want to remove!", QMessageBox::Ok);
            }
            return;
        }

        // 获取选择的需要去除的面板编号
        QList<QListWidgetItem *> selectItems = _ui->listWidget_mode->selectedItems();
        QVector<QString> excludePanelIndex;
        for (auto listitem : selectItems)
        {
            QString itemText = listitem->text();
            QStringList strList = itemText.split(" ");
            if (strList.size() < 2) { continue; }
            excludePanelIndex.push_back(strList.at(1));
        }

        if (panelParticipationData_ == nullptr) {
            return;
        }

        // 绘制频率范围内的频响曲线
        QVector<double> frequences = panelParticipationData_->getFrequenceByRange(min, max);
        QVector<double> responses = panelParticipationData_->getTotalResponseByFrequence(subcaseID_, resultType_, gridID_, component_, frequences);

        if (responses.size() != frequences.size()) {
            return;
        }
       
        // 创建新的曲线视口
        PlotView::mPlotView* plotView = nullptr;
        QString vname = "studyCurveView_Panel";
        plotView = createNewCurvePlot(vname);
        studyCurveView_ = vname;
        if (plotView == nullptr)
            return;

        // 计算去除面板后的响应与差异曲线
        QVector<double> newResponses = panelParticipationData_->getNewResponseAfterExcludePanel(
            subcaseID_, resultType_, gridID_, component_, frequences, excludePanelIndex, ratio);
        QVector<double> difference;
        for (int i = 0; i < responses.size() && i < newResponses.size(); ++i)
        {
            // 使用相对百分比差异，避免在右 Y 轴上将微小绝对残差放大显示
            double percentDiff = 0.0;
            if (qAbs(responses[i]) > 1e-12)
            {
                percentDiff = ((responses[i] - newResponses[i]) / responses[i]) * 100.0;
            }
            difference.append(qAbs(percentDiff) > 1e-12 ? percentDiff : 0.0);
        }

        // 获取ID和结果类型
        QString id = _ui->comboBox_id->currentText();
        QString type = _ui->comboBox_result->currentText();
        QString CurveName = "Node" + id + " " + type + " Panel-Response";
        int axistype = _ui->comboBox_studyAxisType->currentIndex();
        QString strType;
        if (axistype == 0)
        {
            strType = "(Linear)";
        }
        else if (axistype == 1)
        {
            strType = "(Log)";
        }
        QString title = QString("Partial Sum Response @ %1").arg(id);

        // 绘制频响曲线
        QString curveName = CurveName;
        QString xTitle = "Frequency";
        QString yTitle = QString("Response%1").arg(strType);

        plotView->clearAll();
        plotView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, xTitle);
        plotView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, yTitle);
        plotView->setTitle(title);

        // 为三条曲线显式指定互不相同的颜色，避免 updateCurveData 自动调色板
        // 的颜色与后续硬编码颜色重复（PlotColor 第 1 项为绿色）
        plotView->updateCurveData(curveName, frequences, responses);
        PlotView::CurveSettings curveSettings = plotView->getCurveSettings(curveName);
        curveSettings.lineColor = Qt::red;
        curveSettings.symbolColor = Qt::red;
        curveSettings.barColor = Qt::red;
        plotView->setCurveStyle(curveName, curveSettings);
        applyAxisType(plotView, axistype);

        // 绘制去除面板响应的频响曲线
        QString newCurveName = "Partial sum Response";
        plotView->updateCurveData(newCurveName, frequences, newResponses);
        PlotView::CurveSettings newCurveSettings = plotView->getCurveSettings(newCurveName);
        newCurveSettings.linePatternType = PlotView::Pat_Dash;
        newCurveSettings.lineColor = Qt::blue;
        newCurveSettings.symbolColor = Qt::blue;
        newCurveSettings.barColor = Qt::blue;
        plotView->setCurveStyle(newCurveName, newCurveSettings);

        // 绘制差异曲线（挂载到右 Y 轴）
        QString diffCurveName = "%Difference";
        plotView->updateCurveData(diffCurveName, frequences, difference, PlotView::X_Bottom, PlotView::Y_Right);
        PlotView::CurveSettings diffSettings = plotView->getCurveSettings(diffCurveName);
        diffSettings.lineColor = Qt::green;
        diffSettings.symbolColor = Qt::green;
        diffSettings.barColor = Qt::green;
        plotView->setCurveStyle(diffCurveName, diffSettings);

        plotView->getQwtPlot()->setAxisTitle(QwtPlot::yRight, "%Difference");
        plotView->getQwtPlot()->enableAxis(QwtPlot::yRight, true);

        plotView->setPlotLegendItemVisible(true);
        plotView->rescaleAxes();
        // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
        plotView->setEnableMarker(true);

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
        PlotView::AxisSettings rightAxis = plotView->getAxisSettings(PlotView::Y_Right);
        rightAxis.autoScale = false;
        rightAxis.min = yMin;
        rightAxis.max = yMax;
        plotView->updateAxisRange(PlotView::Y_Right, rightAxis);

        // 设置图例信息文本
        QStringList infoLines;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            infoLines.append(QString("Contributor(s) Excluded %1% in frequency range %2 to %3")
                .arg(ratio).arg(min).arg(max));
        }
        else
        {
            infoLines.append(QString("Contributor(s) Excluded %1% in frequency range %2 to %3")
                .arg(ratio).arg(min).arg(max));
        }
        for (QString mode : excludePanelIndex)
        {
            infoLines.append(QString("Panel %1 ").arg(mode));
        }
        setLegendInfoTextOnPlot(plotView, infoLines);

        // 缓存数据供数据表使用
        PlotDataCache cache;
        cache.viewName = vname;
        cache.xTitle = xTitle;
        cache.yTitles.append(yTitle);
        cache.yTitles.append(yTitle);
        cache.yTitles.append("%Difference");
        cache.xValue = frequences;
        cache.curveDatas.append(qMakePair(curveName, responses));
        cache.curveDatas.append(qMakePair(newCurveName, newResponses));
        cache.curveDatas.append(qMakePair(diffCurveName, difference));
        cache.curveNames << curveName << newCurveName << diffCurveName;
        cachePlotData(vname, cache);

        // 在独立浮动窗口中显示
        openPlotInFloatWindow(vname, plotView);
    }

    void mPostPanelParticipation::chooseShowPanel(int index)
    {
        _ui->stackedWidget_show->setCurrentIndex(index);
    }

    void mPostPanelParticipation::plotShowCurve()
    {
        //获取显示方式
        int showPanel = _ui->comboBox_showMode->currentIndex();
        QString showFre, showMinFre, showMaxFre, showNum;
        int showType;

        //柱状图
        if (showPanel == 0)
        {
            //获取显示频率、阶数、类型
            showFre = _ui->comboBox_showFrequency->currentText();
            showNum = _ui->comboBox_showOrder1->currentText();
            showType = _ui->comboBox_showType1->currentIndex();
            if (showFre.isEmpty() || showNum.isEmpty()) return;
        }
        //曲线图
        else if (showPanel == 1)
        {
            //获取显示频率、阶数、类型
            showMinFre = _ui->comboBox_showFrequencyMin->currentText();
            showMaxFre = _ui->comboBox_showFrequencyMax->currentText();
            showNum = _ui->comboBox_showOrder2->currentText();
            showType = _ui->comboBox_showType2->currentIndex();
            if (showMinFre.isEmpty() || showMaxFre.isEmpty() || showNum.isEmpty()) return;
        }

        //绘制百分比柱状图
        if (showPanel == 0 && showType == 0)
        {
            double frequence = showFre.toDouble();
            int modeIndex = showNum.toInt();

            QList<QPair<QString, double>> modePercents;
            panelParticipationData_->getPanelPercentByFrequence(subcaseID_, resultType_, frequence, gridID_, component_, modePercents);
            qSort(modePercents.begin(), modePercents.end(), compareByValuePanel);

            //获取要显示的阶数
            QStringList xdata;
            QVector<double> ydata;

            //总响应百分比
            xdata.append("Response");
            ydata.push_back(100);
            QString str = "Panel ";
            QString index;
            QString strPanel;
            if (modePercents.size() <= modeIndex)
            {
                for (int i = 0; i < modePercents.size(); i++)
                {
                    index = modePercents.at(i).first;
                    strPanel = str + index;
                    //strPanel = index;
                    xdata.append(strPanel);
                    ydata.push_back(modePercents.at(i).second);
                }
            }
            else if (modePercents.size() > modeIndex)
            {
                for (int i = 0; i < modeIndex; i++)
                {
                    index = modePercents.at(i).first;
                    strPanel = str + index;
                    //strPanel = index;
                    xdata.append(strPanel);
                    ydata.push_back(modePercents.at(i).second);
                }
            }

            //其他模态百分比
            double total = 0;
            for (int i = 1; i < ydata.size(); i++)
            {
                total += ydata[i];
            }
            double others = 100 - total;
            xdata.append("others");
            ydata.push_back(others);

            PlotView::mPlotView* plotView = nullptr;
            QString vname = "showChartView_Percent_Panel";
            QString title = QString("Projected Participation @ %1(Frequency = %2 HZ)").arg(gridID_).arg(frequence);
            //创建新的曲线视图窗口
            plotView = createNewCurvePlot(vname);
            showChartPercentView_ = vname;
            plotBarChart(plotView, "Percent", xdata, ydata, title);

            // 缓存数据供数据表使用
            PlotDataCache cache;
            cache.viewName = vname;
            cache.xTitle = "Contributors";
            cache.yTitles.append("%Participation");
            cache.xStringValue = xdata;
            cache.curveDatas.append(qMakePair(QString("Percent"), ydata));
            cache.curveNames.append("Percent");
            cachePlotData(vname, cache);

            openPlotInFloatWindow(vname, plotView);
        }
        //绘制幅值柱状图
        else if (showPanel == 0 && showType == 1)
        {
            double frequence = showFre.toDouble();
            int modeIndex = showNum.toInt();

            QList<QPair<QString, double>> modeMagnitude;
            panelParticipationData_->getPanelMagnitudeByFrequence(subcaseID_, resultType_, frequence, gridID_, component_, modeMagnitude);
            qSort(modeMagnitude.begin(), modeMagnitude.end(), compareByValuePanel);

            //获取要显示的阶数
            QStringList xdata;
            QVector<double> ydata;
            xdata.append("Response");
            //获取频率对应总响应
            double totalRes = panelParticipationData_->getTotalResponseByFrequence(subcaseID_, resultType_, gridID_, component_, frequence);
            if (totalRes == -1)
            {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    MechMainWindow::mFramelessMessageBox::information(nullptr, "提示", "总响应获取失败！", QMessageBox::Ok);
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    MechMainWindow::mFramelessMessageBox::information(nullptr, "Waring", "Please re-select the frequency range!", QMessageBox::Ok);
                }
                return;
            }
            ydata.push_back(totalRes);

            QString str = "Panel ";
            QString index;
            QString strPanel;
            if (modeMagnitude.size() <= modeIndex)
            {
                for (int i = 0; i < modeMagnitude.size(); i++)
                {
                    index = modeMagnitude.at(i).first;
                    strPanel = str + index;
                    xdata.append(strPanel);
                    ydata.push_back(modeMagnitude.at(i).second);
                }
            }
            else if (modeMagnitude.size() > modeIndex)
            {
                for (int i = 0; i < modeIndex; i++)
                {
                    index = modeMagnitude.at(i).first;
                    strPanel = str + index;
                    xdata.append(strPanel);
                    ydata.push_back(modeMagnitude.at(i).second);
                }
            }

            //其他模态幅值
            double total = 0;
            for (int i = 1; i < ydata.size(); i++)
            {
                total += ydata[i];
            }
            double others = totalRes - total;
            xdata.append("others");
            ydata.push_back(others);

            PlotView::mPlotView* plotView = nullptr;
            QString vname = "showChartView_Magnitude_Panel";
            QString title = QString("Participation Magnitude @ %1(Frequency = %2 HZ)").arg(gridID_).arg(frequence);
            //创建新的曲线视图窗口
            plotView = createNewCurvePlot(vname);
            showChartMagnitudeView_ = vname;
            plotBarChart(plotView, "Magnitude", xdata, ydata, title);

            // 缓存数据供数据表使用
            PlotDataCache cache;
            cache.viewName = vname;
            cache.xTitle = "Contributors";
            cache.yTitles.append("Participation（Scale=linear）");
            cache.xStringValue = xdata;
            cache.curveDatas.append(qMakePair(QString("Magnitude"), ydata));
            cache.curveNames.append("Magnitude");
            cachePlotData(vname, cache);

            openPlotInFloatWindow(vname, plotView);
        }
        //绘制2D曲线图
        else if (showPanel == 1)
        {
            double minFre = showMinFre.toDouble();
            double maxFre = showMaxFre.toDouble();
            int modeIndex = showNum.toInt();

            if (minFre - maxFre > 0.0)
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

            //获取当前文件下所有面板
            QList<QString> modes = panelParticipationData_->getAllPanelName(subcaseID_,resultType_);

            //获取包含模态的频率以及对应的幅值
            QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> modeFreMagnitude;
            for (int i = 0; i < modes.size(); i++)
            {
                QVector<double> fres, mags;
                panelParticipationData_->getAllFreAndMagnitudeByPanel(subcaseID_, resultType_, modes[i], fres, gridID_, component_, mags);
                if (fres.size() == 0 || mags.size() == 0) continue;
                QPair<QVector<double>, QVector<double>> pair1(fres, mags);
                QPair<QString, QPair<QVector<double>, QVector<double>>> pair2(modes[i],pair1);
                modeFreMagnitude.append(pair2);
            }

            //获取在频率范围内的模态
            QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> rangePanel;
            rangePanel = getPanelByRange(minFre,maxFre,modeFreMagnitude);

            //对各个模态的总幅值进行排序
            //存储键与幅值总和
            QList<QPair<QString, double>> keySumList;
            //遍历QHash并计算总和
            for (auto it = rangePanel.begin(); it != rangePanel.end(); ++it)
            {
                double sum = 0.0;
                for (double value : it->second.second)
                {
                    sum += value;
                }
                keySumList.append(QPair<QString, double>(it->first,sum));
            }
            //对键和总和进行排序
            qSort(keySumList.begin(), keySumList.end(), comparePairsPanel);

            //重新建构排序后的list
            QList<QPair<QString, QPair<QVector<double>, QVector<double>>>>  sortedPanel;
            for (auto& key : keySumList)
            {
                QString index = key.first;
                for (int i  = 0; i < rangePanel.size(); i++)
                {
                    if (rangePanel[i].first == index)
                    {
                        QPair<QVector<double>, QVector<double>> mags = rangePanel[i].second;
                        QPair<QString, QPair<QVector<double>, QVector<double>>> pair(index, mags);
                        sortedPanel.append(pair);
                    }
                }
            }

            //根据显示阶数绘制曲线
            //创建新的曲线视口
            PlotView::mPlotView* plotView = nullptr;
            QString vname = "showCurveView_Panel";
            plotView = createNewCurvePlot(vname);
            showCurveView_ = vname;
            if (plotView == nullptr)
                return;

            // 对数坐标下，把 <=0 的 Y 值替换为最小正值的 0.1 倍，避免 Qwt 对数轴下限被拉到极小值
            auto sanitizeForLog = [](QVector<double>& yData) -> void {
                double minPositive = std::numeric_limits<double>::max();
                for (double v : yData)
                {
                    if (v > 0 && v < minPositive)
                        minPositive = v;
                }
                double replacement = (minPositive < std::numeric_limits<double>::max()) ? (minPositive * 0.1) : 1e-12;
                for (double& v : yData)
                {
                    if (v <= 0)
                        v = replacement;
                }
            };

            //绘制总响应曲线
            QString resCurveName = "Response";
            //根据范围获取所有的频率
            QVector<double> frequences = panelParticipationData_->getFrequenceByRange(minFre, maxFre);
            //获取范围频率对应的总响应
            QVector<double> responses = panelParticipationData_->getTotalResponseByFrequence(subcaseID_, resultType_, gridID_, component_, frequences);
            QString xTitle = "Frequency";
            QString yTitle = "Participation（Scale = log）";
            QString title = QString("Participation Magnitude @ %1").arg(gridID_);

            plotView->clearAll();
            plotView->getQwtPlot()->setAxisTitle(QwtPlot::xBottom, xTitle);
            plotView->getQwtPlot()->setAxisTitle(QwtPlot::yLeft, yTitle);
            plotView->setTitle(title);

            QVector<double> plotResponses = responses;
            sanitizeForLog(plotResponses);
            plotView->updateCurveData(resCurveName, frequences, plotResponses);

            //绘制模态曲线
            QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> showPanelData;
            QString str = "Panel ";
            QVector<double> allYValues = plotResponses;
            if (sortedPanel.size() > modeIndex)
            {
                for (int i = 0; i < modeIndex; i++)
                {
                    QString curveName = str + sortedPanel[i].first;
                    QVector<double> panelY = sortedPanel[i].second.second;
                    sanitizeForLog(panelY);
                    allYValues.append(panelY);
                    plotView->updateCurveData(curveName, sortedPanel[i].second.first, panelY);
                }
                showPanelData = sortedPanel.mid(0, modeIndex);
            }
            else if (sortedPanel.size() <= modeIndex)
            {
                for (int i = 0; i < sortedPanel.size(); i++)
                {
                    QString curveName = str + sortedPanel[i].first;
                    QVector<double> panelY = sortedPanel[i].second.second;
                    sanitizeForLog(panelY);
                    allYValues.append(panelY);
                    plotView->updateCurveData(curveName, sortedPanel[i].second.first, panelY);
                }
                showPanelData = sortedPanel;
            }

            // 对数坐标
            PlotView::AxisSettings yAxis = plotView->getAxisSettings(PlotView::Y_Left);
            yAxis.isLog = true;
            yAxis.autoScale = true;
            plotView->updateAxisLog(PlotView::Y_Left, yAxis);
            plotView->rescaleAxes();
            // 启用 PlotView::mPlotView 内置点吸附（mPlotSnapper），鼠标靠近曲线数据点时自动吸附显示
            plotView->setEnableMarker(true);

            // 由于 barCurve 的 baseline=0 会干扰自动范围计算，根据实际数据手动限定 Y 轴范围
            if (!allYValues.isEmpty())
            {
                double yMin = allYValues.first();
                double yMax = allYValues.first();
                for (double v : allYValues)
                {
                    if (v < yMin) yMin = v;
                    if (v > yMax) yMax = v;
                }
                double lower = yMin * 0.1;
                double upper = qMax(yMax * 10.0, lower * 10.0);

                PlotView::AxisSettings yAxisRange = plotView->getAxisSettings(PlotView::Y_Left);
                yAxisRange.autoScale = false;
                yAxisRange.isLog = true;
                yAxisRange.min = lower;
                yAxisRange.max = upper;
                plotView->updateAxisRange(PlotView::Y_Left, yAxisRange);
            }

            plotView->setPlotLegendItemVisible(true);

            // 缓存数据供数据表使用
            PlotDataCache cache;
            cache.viewName = vname;
            cache.xTitle = xTitle;
            cache.yTitles.append(yTitle);
            cache.xValue = frequences;
            cache.curveDatas.append(qMakePair(resCurveName, responses));
            cache.curveNames.append(resCurveName);
            for (const auto& item : showPanelData)
            {
                QString curveName = str + item.first;
                cache.curveDatas.append(qMakePair(curveName, item.second.second));
                cache.curveNames.append(curveName);
                cache.yTitles.append(yTitle);
            }
            cachePlotData(vname, cache);

            // 在独立浮动窗口中显示
            openPlotInFloatWindow(vname, plotView);
        }
    }

    void mPostPanelParticipation::openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView)
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
        //floatWin->show();
        // 使用 QDockWidget 停靠在底部
        mDockLayoutManager::getInstance()->addDockToBottom(floatWin, vname, keyName);

        // 获取 QMainWindow
        //QMainWindow* mainWin = qobject_cast<QMainWindow*>(window());
        //if (!mainWin) {
        //    floatWin->show();  // 退化为独立窗口
        //    return;
        //}

        //// 创建停靠窗口
        //QDockWidget* dock = new QDockWidget(QString("Response @ %1").arg(1), mainWin);
        //dock->setWidget(floatWin);
        //dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        //mainWin->addDockWidget(Qt::BottomDockWidgetArea, dock);
        //dock->show();
        //dock->raise();

        //mDockLayoutManager::getInstance()->addDockToBottom(dock, vname, vname);

    }

    void mPostPanelParticipation::filePathChanged(const QString& text)
    {
        filePath_ = text;
    }

    void mPostPanelParticipation::slot_changeLanguage(int language)
    {
        if (language == 0)
        {
            _ui->label_2->setText("面板贡献量");
            _ui->tabWidget->setTabText(0, "导入");
            _ui->tabWidget->setTabText(1, "显示");
            _ui->tabWidget->setTabText(2, "研究");
            _ui->groupBox_selectFile->setTitle("文件选择");
            _ui->pushButton_loadFileOK->setText("确定");
            _ui->groupBox_selectResult->setTitle("结果选择");
            _ui->label_case->setText("工况:");
            _ui->label_case_2->setText("工况:");
            _ui->label_result->setText("结果:");
            _ui->label_component->setText("分量:");
            _ui->pushButton_loadResponseOK->setText("确定");
            _ui->label_showMode->setText("显示方式:");
            QStringList modes;
            modes << "柱状图" << "2D曲线";
            _ui->comboBox_showMode->clear();
            _ui->comboBox_showMode->addItems(modes);
            _ui->label_showFrequency1->setText("显示频率:");
            _ui->label_showOrder1->setText("显示阶数:");
            _ui->label_showType1->setText("显示类型:");
            QStringList types;
            types << "百分比" << "幅值";
            _ui->comboBox_showType1->clear();
            _ui->comboBox_showType1->addItems(types);
            _ui->comboBox_showType2->clear();
            _ui->comboBox_showType2->addItem("幅值");
            _ui->pushButton_showData->setText("曲线数据");
            _ui->pushButton_showOK->setText("确定");
            _ui->label_showFrequency2->setText("显示频率:");
            _ui->label_showOrder2->setText("显示阶数:");
            _ui->label_showType2->setText("显示类型:");
            _ui->label_studyFrequnecy->setText("显示频率:");
            _ui->label_excludeRatio->setText("去除比例:");
            _ui->label_choosePanel->setText("面板选择:");
            _ui->pushButton_studyData->setText("曲线数据");
            _ui->pushButton_studyOK->setText("确定");
            //_ui->pushButton_return1->setText("返回");
			//_ui->pushButton_return2->setText("返回");
			//_ui->pushButton_return3->setText("返回");
            _ui->label_loadAxisType->setText("坐标轴类型");
            _ui->label_studyAxisType->setText("坐标轴类型");
            QStringList strlist;
            strlist << "线性" << "对数";
            _ui->comboBox_loadAxisType->clear();
            _ui->comboBox_studyAxisType->clear();
            _ui->comboBox_loadAxisType->addItems(strlist);
            _ui->comboBox_studyAxisType->addItems(strlist);
            _ui->comboBox_loadAxisType->setCurrentIndex(1);
            _ui->comboBox_studyAxisType->setCurrentIndex(1);
        }
        else if (language == 1)
        {
            _ui->label_2->setText("Panel contribution quantity");
            _ui->tabWidget->setTabText(0, "Load");
            _ui->tabWidget->setTabText(1, "Display");
            _ui->tabWidget->setTabText(2, "Study");
            _ui->groupBox_selectFile->setTitle("File selection");
            _ui->pushButton_loadFileOK->setText("OK");
            _ui->groupBox_selectResult->setTitle("Result selection");
            _ui->label_case->setText("Subcase:");
            _ui->label_case_2->setText("Subcase:");
            _ui->label_result->setText("Result:");
            _ui->label_component->setText("Component:");
            _ui->pushButton_loadResponseOK->setText("OK");
            _ui->label_showMode->setText("Display mode:");
            QStringList modes;
            modes << "Bar chart" << "2D curve";
            _ui->comboBox_showMode->clear();
            _ui->comboBox_showMode->addItems(modes);
            _ui->label_showFrequency1->setText("Frequency:");
            _ui->label_showOrder1->setText("Display order:");
            _ui->label_showType1->setText("Display type:");
            QStringList types;
            types << "Projected" << "Magnitude";
            _ui->comboBox_showType1->clear();
            _ui->comboBox_showType1->addItems(types);
            _ui->comboBox_showType2->clear();
            _ui->comboBox_showType2->addItem("Magnitude");
            _ui->pushButton_showData->setText("Curve data");
            _ui->pushButton_showOK->setText("OK");
            _ui->label_showFrequency2->setText("Frequency:");
            _ui->label_showOrder2->setText("Display order:");
            _ui->label_showType2->setText("Display type:");
            _ui->label_studyFrequnecy->setText("Display frequency:");
            _ui->label_excludeRatio->setText("%to exclude:");
            _ui->label_choosePanel->setText("Panel selection:");
            _ui->pushButton_studyData->setText("Curve data");
            _ui->pushButton_studyOK->setText("OK");
			//_ui->pushButton_return1->setText("Return");
			//_ui->pushButton_return2->setText("Return");
			//_ui->pushButton_return3->setText("Return");
            _ui->label_loadAxisType->setText("Axis type");
            _ui->label_studyAxisType->setText("Axis type");
            QStringList strlist;
            strlist << "Linear" << "Logarithmic";
            _ui->comboBox_loadAxisType->clear();
            _ui->comboBox_studyAxisType->clear();
            _ui->comboBox_loadAxisType->addItems(strlist);
            _ui->comboBox_studyAxisType->addItems(strlist);
            _ui->comboBox_loadAxisType->setCurrentIndex(1);
            _ui->comboBox_studyAxisType->setCurrentIndex(1);
        }
    }

    PlotView::mPlotView* mPostPanelParticipation::createNewCurvePlot(QString& viewName)
    {
        PlotView::mPlotView* plot = new PlotView::mPlotView();
        plot->setPlotBackground(QColor(255, 255, 255));
        return plot;
    }

    void mPostPanelParticipation::slot_postActiveViewChanged()
    {
        // 已移除 VMManager 依赖，不再需要处理子窗口激活事件
    }

    void mPostPanelParticipation::slot_deleteCurvePlot()
    {
        // 直接删除当前激活的浮动窗口
        if (_activePlotViewName.isEmpty()) return;
        
        if (_plotFloatWindows.contains(_activePlotViewName)) {
            mPostPlotFloatWindow* win = _plotFloatWindows[_activePlotViewName];
            win->setParent(nullptr);  // 从 dock 中脱离
            delete win;
            _plotFloatWindows.remove(_activePlotViewName);
        }
        _activePlotViewName.clear();
    }


    QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> mPostPanelParticipation::getPanelByRange(double min, double max, QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> mode)
    {
        QList<QPair<QString, QPair<QVector<double>, QVector<double>>>> rangePanel;
        QVector<double> fres, mags;
        QString modeIndex;
        for (int i = 0; i < mode.size(); i++)
        {
            QPair<QString, QPair<QVector<double>, QVector<double>>> item = mode.at(i);
            modeIndex = item.first;
            fres = item.second.first;
            mags = item.second.second;

            QVector<double> vecFre;
            QVector<double> vecMag;
            for (int j = 0; j < fres.size(); j++)
            {
                if (fres[j] >= min && fres[j] <= max)
                {
                    vecFre.push_back(fres[j]);
                    vecMag.push_back(mags[j]);
                }
            }
            if (vecFre.size() > 0 && vecMag.size() > 0)
            {
                QPair<QVector<double>, QVector<double>> pair1(vecFre, vecMag);
                QPair<QString, QPair<QVector<double>, QVector<double>>> pair2(modeIndex, pair1);
                rangePanel.append(pair2);
            }
        }
        return rangePanel;
    }

    QVector<double> mPostPanelParticipation::getOtherMagnitudes(QVector<double> response, QVector<double> fres, QList<QPair<int, QPair<QVector<double>, QVector<double>>>> modeData)
    {
        QVector<double> others;
        for (int i = 0; i < fres.size(); i++)
        {
            double fre = fres[i];
            double res = response[i];
            for (int j = 0; j < modeData.size(); j++)
            {
                QVector<double> modeFres;
                modeFres = modeData[j].second.first;
                for (int z = 0; z < modeFres.size(); z++)
                {
                    if (fabs(modeFres[z] - fre) < 0.001)
                    {
                        double mag = modeData[j].second.second[z];
                        res = res - mag;
                        break;
                    }
                }
            }
            others.push_back(res);
        }
        return others;
    }

    void mPostPanelParticipation::showDataTable()
    {
        // 根据当前 tab 获取对应的 plotView
        PlotView::mPlotView* plot2d = nullptr;
        int tabIndex = _ui->tabWidget->currentIndex();
        QString targetName;
        if (tabIndex == 0) {
            targetName = importCurveView_;
        }
        else if (tabIndex == 1) {
            if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 0) {
                targetName = showChartPercentView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 1) {
                targetName = showChartMagnitudeView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 1) {
                targetName = showCurveView_;
            }
        }
        else if (tabIndex == 2) {
            targetName = studyCurveView_;
        }

        if (!targetName.isEmpty() && _plotFloatWindows.contains(targetName)) {
            plot2d = _plotFloatWindows[targetName]->getPlotView();
        }
        if (plot2d == nullptr) return;

        if (!_plotDataCache.contains(targetName)) return;
        const PlotDataCache& cache = _plotDataCache[targetName];

        if (dataTable_ == nullptr)
            dataTable_ = new mPostDataTable(this);

        // 设置数据（从缓存中读取）
        if (cache.xValue.isEmpty())
        {
            dataTable_->setTableData(cache.curveNames, cache.xTitle, cache.yTitles, cache.xStringValue, cache.curveDatas);
        }
        else
        {
            dataTable_->setTableData(cache.curveNames, cache.xTitle, cache.yTitles, cache.xValue, cache.curveDatas);
        }
        dataTable_->move(QCursor::pos());
        dataTable_->show();
    }

    void mPostPanelParticipation::deleteCurveView()
    {
        int index = _ui->tabWidget->currentIndex();
        QString targetName;
        if (index == 0)
        {
            targetName = importCurveView_;
        }
        if (index == 1)
        {
            if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 0)
            {
                targetName = showChartPercentView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 1)
            {
                targetName = showChartMagnitudeView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 1)
            {
                targetName = showCurveView_;
            }
        }
        if (index == 2)
        {
            targetName = studyCurveView_;
        }

        if (!targetName.isEmpty() && _plotFloatWindows.contains(targetName)) {
            mPostPlotFloatWindow* win = _plotFloatWindows[targetName];
            win->setParent(nullptr);  // 从 dock 中脱离
            delete win;
            _plotFloatWindows.remove(targetName);
            if (_activePlotViewName == targetName) {
                _activePlotViewName.clear();
            }
        }
    }

    namespace
    {
        // 为每个贡献者生成一组稳定的绘制颜色（统一淡蓝色）
        QColor getBarColorByIndex(int index)
        {
            Q_UNUSED(index);
            return QColor(173, 216, 230);   // 淡蓝
        }
    }

    void mPostPanelParticipation::applyAxisType(PlotView::mPlotView* plotView, int axisType)
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
    
    void mPostPanelParticipation::setLegendInfoTextOnPlot(PlotView::mPlotView* plotView, const QStringList& infoLines)
    {
        if (!plotView || infoLines.isEmpty()) return;
        QwtPlot* qwtPlot = plotView->getQwtPlot();
        if (!qwtPlot) return;

        QWidget* hostWindow = plotView->window();
        if (!hostWindow) return;

        // 每个 plotView 缓存一个可拖动信息标签，parent 设为顶层窗口避免被 plotView 裁剪
        static QHash<PlotView::mPlotView*, QPointer<mDraggableInfoLabel>> infoLabels;
        QPointer<mDraggableInfoLabel>& label = infoLabels[plotView];
        bool isNewLabel = false;
        if (!label)
        {
            label = new mDraggableInfoLabel(hostWindow);
            isNewLabel = true;
        }

        QString html = QString("<div style='font-family: Arial; font-size: 12px;'>%1</div>")
            .arg(infoLines.join("<br>"));
        label->setText(html);
        label->adjustSize();

        // 首次创建时：默认定位到画布顶部中间（hostWindow 坐标系）
        if (isNewLabel)
        {
            QWidget* canvas = qwtPlot->canvas();
            QPoint canvasGlobalTopLeft = canvas->mapToGlobal(QPoint(0, 0));
            int globalX = canvasGlobalTopLeft.x() + (canvas->width() - label->width()) / 2;
            int globalY = canvasGlobalTopLeft.y() + 10;
            QPoint hostPos = hostWindow->mapFromGlobal(QPoint(globalX, globalY));

            // 若顶部中间超出 hostWindow 右边界，则回退到左侧
            if (hostPos.x() < 0 || hostPos.x() + label->width() > hostWindow->width())
                hostPos.setX(100);
            if (hostPos.y() < 0)
                hostPos.setY(10);

            label->move(hostPos);
        }

        label->raise();
        label->show();
    }
    
    void mPostPanelParticipation::plotBarChart(PlotView::mPlotView* plotView, const QString& name,
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
        plotView->getQwtPlot()->replot();
    }

    void mPostPanelParticipation::addBarValueLabels(PlotView::mPlotView* plotView, const QString& curveName,
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

            // 数值标签：参考第二张图，极小/极大值使用科学计数法，常规值用固定精度
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
            // 标签位置：QwtPlotMarker 的 label 对齐方式以 marker 位置为基准。
            // 对于正值，文字显示在柱顶上方（AlignTop）；
            // 对于负值，文字显示在柱子最下端外侧（AlignBottom）。
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

        _barValueLabelMarkers[plotView] = oldMarkers;
    }
    
    void mPostPanelParticipation::cachePlotData(const QString& vname, const PlotDataCache& cache)
    {
        _plotDataCache[vname] = cache;
    }

    void mPostPanelParticipation::currentTabChanged(int index)
    {
        return;//htx add 取消通过切面切换，切换曲线视图的功能
        //根据不同的显示模式切换曲线视图
        QString targetName;
        if (index == 0)
        {
            targetName = importCurveView_;
        }
        else if (index == 1)
        {
            if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 0)
            {
                targetName = showChartPercentView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 0 && _ui->comboBox_showType1->currentIndex() == 1)
            {
                targetName = showChartMagnitudeView_;
            }
            else if (_ui->comboBox_showMode->currentIndex() == 1)
            {
                targetName = showCurveView_;
            }
        }
        else if(index == 2)
        {
            targetName = studyCurveView_;
        }

        if (!targetName.isEmpty() && _plotFloatWindows.contains(targetName)) {
            // 通过 ADS 激活对应的 dock
            mDockLayoutManager::getInstance()->activateDock(targetName + "PostWidow");
            _activePlotViewName = targetName;
        }
    }
}
