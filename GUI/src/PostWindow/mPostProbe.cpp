#include "mOtherFunction.h"
//#include "mPostProbe.h"
//
//#include <QFormLayout>
//#include <QTableWidget>
//#include <QFileDialog>
//#include <QTextCodec>
//
//#include "mViewportModelManager.h"
//#include "mViewportModel.h"
//#include "mMechGlobalSignals.h"
//#include "mDockLayoutManager.h"
//#include "mAppConfiguration.h"
//
//#include "mxr/data/SelectDataStruct.h"
//#include "mxr/view/CCSMRendWidget.h"
//#include "mxr/view/REBUILD_PostRend.h"
//#include "mxr/view/REBUILD_PostRender.h"
//#include "REBUILD_DataProxy.h"
//#include "REBUILD_QueryDataProxy.h"
//#include "ZsTempRender.h"
//
////PlotView
//#include "mCurveWindow.h"
//#include "mPlotView.h"
//#include "mPlotViewStyleWidget.h"
//
////MxFunctions
//#include "mGlobalSignals.h"
//
////BasicData
//#include "mPhysicalFieldEnum.h"
//
////post
//#include "mIMxdbFile1.h"
//#include "mMxdbThread.h"
//#include "mPostCurveData.h"
//
////窗口
//#include "mViewportModelManager.h"
//#include "mViewportModel.h"
//#include "mModelData.h"
//
//#include "UI/UI_PostProbes.h"
//
//using namespace MxFunctions;
//using namespace MechCommon;
//using namespace PostData;
//using namespace MechPreData;
//using namespace MIOFile;
//
//namespace PostWindow
//{
//    mPostProbePanel::mPostProbePanel(QWidget* parent) : mPanelBase(parent)
//    {
//        _ui = new Ui::mPostProbes;
//        _ui->setupUi(this);
//
//        //添加快捷键
//        _ui->pushButton_4->setShortcut(Qt::Key_Escape);
//
//        _ui->pushButton_6->setChecked(true);
//
//        //生成视图名称编号 最多999
//        for (int i = 0; i < 1000; i++)
//        {
//            _usableCurveViewNums.insert(i);
//        }
//
//        //初始化表格
//        _ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
//        _ui->tableWidget->setColumnCount(2);
//        _ui->tableWidget->verticalHeader()->setHidden(true);
//        _ui->tableWidget->resizeColumnsToContents();
//        _ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//        //_ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
//        _ui->tableWidget->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
//        //_ui->tableWidget->horizontalHeader()->setHighlightSections(false);
//        //_ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView:section{background:rgb(218,218,218);}");
//
//        //连接信号与槽
//        connect(_ui->pushButton_4, SIGNAL(clicked()), this, SIGNAL(returnSig()));
//        connect(_ui->comboBox_var, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slot_variableChanged(const QString&)));
//        connect(_ui->listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slot_selectedChanged()));
//        connect(_ui->comboBox_4, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slot_pickComboBox(const QString&)));
//        connect(_ui->pushButton_createCurve, SIGNAL(clicked()), this, SLOT(slot_createCurve()));
//        //切换界面语言
//        //connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
//
//        ////切换界面语言
//        //connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)), this, SLOT(slot_changeLanguage(int)));
//        //slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
//
//        //connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(finishedReadPostFileSig(const QString&)), this, SLOT(slot_finishedImportFiles(const QString&)));
//        connect(VMManager, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(slot_postActiveViewChanged(QMdiSubWindow*)));
//
//        _ui->changeLanguage(0);
//    }
//
//    void mPostProbePanel::newProbePanel(int caseid)
//    {
//
//    }
//
//    void mPostProbePanel::updateProbePanel(int caseid)
//    {
//        // 更新变量数据
//        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
//        if (_postDB == nullptr)
//            return;
//
//        //获取所有变量
//        QStringList varNames = _postDB->getAllVariableNames();
//        _ui->comboBox_var->clear();
//        _ui->comboBox_var->addItems(varNames);
//
//        // 更新拾取点数据
//        setPanelPickAndSignal(true);
//    }
//
//    mPostProbePanel::~mPostProbePanel()
//    {
//        if (_ui != nullptr)
//            delete _ui;
//    }
//
//    void mPostProbePanel::setPanelPickAndSignal(bool isshow)
//    {
//        if (isshow)
//        {
//            REBUILD_PostRend* postRend = nullptr;
//            if (_postRend != nullptr)
//            {
//                postRend = _postRend.data();
//            }
//            else
//            {
//                postRend = getCVM->getPostRenderWidget();//VMManager->getActiveREBUILDPostRend();
//            }
//
//            if (postRend == nullptr)
//                return;
//            auto  postRender = postRend->getPostRender();
//            if (postRender == nullptr) return;
//            //postRender->setPickFilter(PickFilter::PickNode);
//            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
//            connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, SLOT(slot_finishedPick()));
//            //获取拾取数据
//            /*IMeshPickResultBase* pickdata = postRender->getMeshPickData();
//            if (pickdata == nullptr)
//            {
//                return;
//            }*/
//            //清空拾取高亮
//            getCVM->clearPick();
//            /*pickdata->clearPickResultDataByType(IMeshPickResultBase::PickInfoType::NodeID);
//            postRender->updateHighLightRender();*/
//        }
//        else
//            disconnect(VMManager, SIGNAL(finishedPickSigManger()), this, SLOT(slot_finishedPick()));
//    }
//
//    bool mPostProbePanel::generateCurveData()
//    {
//        if (_postRend == nullptr)
//        {
//            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("视图为空！"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("视图为空！"), true);
//                return false;
//            }
//            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("View is empty!"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("View is empty!"), true);
//                return false;
//            }
//        }
//        QString postname1 = _postRend->getPostDataName();
//        //mDataPost1* _postData = mIOFileManager::getInstance()->getDataPostFromName(postname1);
//        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname1);
//
//        if (_postRend == nullptr || _postDB == nullptr)
//        {
//            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("没有拾取到节点/单元！"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("没有拾取到节点/单元！"), true);
//                return false;
//            }
//            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("Node/unit not picked up!"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("Node/unit not picked up!"), true);
//                return false;
//            }
//        }
//        //mPostMeshRend1* meshrend = pv3->getPostMainRend()->getPostMeshRend();
//        auto postRender = _postRend->getPostRender();
//        if (postRender == nullptr)
//        {
//            return false;
//        }
//
//        //获取拾取数据
//
//        /*IMeshPickResultBase* pickdata = postRender->getMeshPickData();
//        if (pickdata == nullptr)
//        {
//            return false;
//        }*/
//
//        //清空数据
//        _curveNames.clear();
//        _axisYTitles.clear();
//        _XValue.clear();
//        _YValuesD.clear();
//        _YValuesF.clear();
//
//        //后处理数据库名
//        QString postname = _postDB->getName();
//
//        //获取变量、层、分量、复数
//        QString name = _ui->comboBox_var->currentText();
//        QString layer = _ui->comboBox_layer->currentText();
//        QStringList compo = getSelectedList(_ui->listWidget);
//        QString complex;
//        if (_ui->comboBox_complex->isVisible())
//        {
//            complex = _ui->comboBox_complex->currentText();
//        }
//
//        //获取拾取到的节点或单元
//        QVector<int> ids;
//        QString idstr = _ui->lineEdit->text();
//        QStringList idlist = idstr.split(",");
//        for (QString id : idlist)
//        {
//            if (!id.simplified().isEmpty())
//            {
//                ids.push_back(id.toInt());
//            }
//        }
//        if (ids.size() == 0)
//        {
//            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("没有拾取到节点/单元！"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("没有拾取到节点/单元！"), true);
//                return false;
//            }
//            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("No node/element was picked up！"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("No node/element was picked up！"), true);
//                return false;
//            }
//
//        }
//
//        ////读取文件
//        //mIMxdbFile1* mxdbreader = new mIMxdbFile1;
//        //mxdbreader->readCurveData1(postname, name, compo, layer, complex, ids, _curveNames, _XValue, _YValues);
//
//        QString postPathName = _postDB->getFile();
//        if (_postDB == nullptr) return false;
//
//        if (_postDB->getDataFlag())
//        {
//            mIMxdbFile1* mxdbReader = new mIMxdbFile1;
//            mxdbReader->readCurveData1(postPathName, name, compo, layer, complex, ids, _curveNames, _XValue, _YValuesD);
//            delete mxdbReader;
//            mxdbReader = nullptr;
//        }
//        else
//        {
//            mMxdbThread* mxdbReader = new mMxdbThread(postPathName, _postDB, this);
//            // 读取曲线数据
//            mxdbReader->readTimeVarCurveData(postPathName, name, compo, layer, complex, ids, _curveNames, _XValue, _YValuesD, _YValuesF);
//            delete mxdbReader;
//            mxdbReader = nullptr;
//        }
//
//        //清空拾取
//        getCVM->clearPick();
//        /*pickdata->clearAllPickData();
//        postRender->updateHighLightRender();*/
//        return true;
//    }
//
//    void mPostProbePanel::slot_changeLanguage(int language)
//    {
//        _ui->changeLanguage(language);
//    }
//
//    void mPostProbePanel::slot_finishedPick()
//    {
//        //mPostRend* postRend = VMManager->getActivePostRend();
//        if (_postRend == nullptr)
//        {
//            return;
//        }
//        auto postRender = _postRend->getPostRender();
//        if (postRender == nullptr)
//        {
//            return;
//        }
//        /*IMeshPickResultBase* pickdata = postRender->getMeshPickData();
//        if (pickdata == nullptr)
//        {
//            return;
//        }*/
//
//        set<int> ids;
//        if (_ui->pushButton_6->text() == QString("节点") || _ui->pushButton_6->text() == QString("Node"))
//        {
//            auto& eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
//            for (auto it = eles.begin(); it != eles.end(); it++)
//            {
//                ids.insert(it->data_id.node_id);
//            }
//        }
//        else if (_ui->pushButton_6->text() == QString("单元") || _ui->pushButton_6->text() == QString("Element"))
//        {
//            auto& eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
//            for (auto it = eles.begin(); it != eles.end(); it ++)
//            {
//                ids.insert(it->data_id.element_id);
//            }
//        }
//
//        QStringList idlist;
//        _ui->lineEdit->clear();
//        for (int id : ids)
//        {
//            idlist.append(MxFunctions::mNumberToString(id));
//        }
//        QString idstr = idlist.join(",");
//        _ui->lineEdit->setText(idstr);
//    }
//
//    void mPostProbePanel::slot_finishedImportFiles(const QString& postName)
//    {
//        //mDataPost1* _postData = mIOFileManager::getInstance()->getDataPostFromName(postName);
//        //if (_postData == nullptr)
//        //{
//        //	return;
//        //}
//        if (postName.isEmpty())
//            return;
//
//        QStringList lis = postName.split("_##_");
//
//        if (lis.size() != 2)
//            return;
//
//        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(lis[0]);
//        if (_postDB == nullptr)
//        {
//            return;
//        }
//
//        //获取所有变量
//        QStringList varNames = _postDB->getAllVariableNames();
//        _ui->comboBox_var->clear();
//        _ui->comboBox_var->addItems(varNames);
//    }
//    void mPostProbePanel::slot_variableChanged(const QString& varName)
//    {
//        ////获取当前激活视图
//        //REBUILD_PostRend* postRend = VMManager->getActiveREBUILDPostRend();
//        if (_postRend == nullptr)
//        {
//            return;
//        }
//        auto postRender = _postRend->getPostRender();
//        if (postRender == nullptr)
//        {
//            return;
//        }
//        //获取该变量的分量
//        //mPostRend* postRend = VMManager->getActivePostRend();
//        //if (postRend == nullptr) {
//        //	return;
//        //}
//        QString postName = _postRend->getPostDataName();
//        //mDataPost1* _postData = mIOFileManager::getInstance()->getDataPostFromName(postName);
//        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
//        QStringList cNames = _postDB->getVariableCompos(varName);
//        //获取该变量的层
//        QStringList lNames = _postDB->getVariableLayer(varName);
//        this->slot_selectedChanged();
//        clearListWidget(_ui->listWidget);
//        if (cNames.size() == 0)
//        {
//            _ui->listWidget->hide();
//        }
//        else
//        {
//            _ui->listWidget->show();
//            _ui->listWidget->addItems(cNames);
//        }
//
//        _ui->comboBox_layer->clear();
//        if (lNames.size() == 0)
//        {
//            _ui->comboBox_layer->hide();
//            _ui->label_4->hide();
//        }
//        else
//        {
//            _ui->comboBox_layer->show();
//            _ui->label_4->show();
//            _ui->comboBox_layer->addItems(lNames);
//        }
//    }
//    void mPostProbePanel::slot_selectedChanged()
//    {
//        //mPostRend* postRend = VMManager->getActivePostRend();
//        if (_postRend == nullptr)
//        {
//            return;
//        }
//        QString postname = _postRend->getPostDataName();
//        //mDataPost1* _postData = mIOFileManager::getInstance()->getDataPostFromName(postname);
//        //if (_postData == nullptr)
//        //{
//        //	return;
//        //}
//        _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
//        if (_postDB == nullptr)
//        {
//            return;
//        }
//
//        //获取选择的分量
//        QStringList componames = getSelectedList(_ui->listWidget);
//        for (QString cname : componames)
//        {
//            //获取该分量是否为复数
//            bool iscomplex = _postDB->isVarCompoComplex(_ui->comboBox_var->currentText(), cname);
//            if (iscomplex)
//            {
//                //设置控件显隐
//                _ui->label_5->setVisible(true);
//                _ui->comboBox_complex->setVisible(true);
//                return;
//            }
//        }
//        //设置控件显隐
//        _ui->label_5->setVisible(false);
//        _ui->comboBox_complex->setVisible(false);
//    }
//    void mPostProbePanel::slot_pickComboBox(const QString& pickText)
//    {
//        QString text = _ui->comboBox_4->currentText();
//        if (text == QString("拾取") || text == QString("Pick up"))
//        {
//
//            //获取当前激活视图
//            //mPostRend* postRend = VMManager->getActivePostRend();
//            if (_postRend == nullptr)
//            {
//                return;
//            }
//            auto postRender = _postRend->getPostRender();
//            if (postRender == nullptr)
//            {
//                return;
//            }
//            QString pickText = _ui->pushButton_6->text();
//            if (pickText == QString("节点") || pickText == QString("Node"))
//            {
//                //postRender->setPickFilter(MxFunctions::PickFilter::PickNode);
//                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
//            }
//            else
//            {
//                MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
//                //postRender->setPickFilter(MxFunctions::PickFilter::PickAnyMesh);
//            }
//
//            //连接拾取信号与槽
//            //disconnect(mGlobalSignals::getInstance(), SIGNAL(finishPickingPostSig()), this, SLOT(slot_finishedPick()));
//            //connect(mGlobalSignals::getInstance(), SIGNAL(finishPickingPostSig()), this, SLOT(slot_finishedPick()));
//            disconnect(VMManager, SIGNAL(finishedPickSigManger()), this, SLOT(slot_finishedPick()));
//            connect(VMManager, SIGNAL(finishedPickSigManger()), this, SLOT(slot_finishedPick()));
//        }
//    }
//    Common::mPlotView* mPostProbePanel::createNewCurvePlot(QString& viewName)
//    {
//        //获取视图名称
//        auto noIt = _usableCurveViewNums.begin();
//        if (noIt == _usableCurveViewNums.end())
//        {
//            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("视图已满！"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("视图已满！"), true);
//                return nullptr;
//            }
//            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
//            {
//                mOutputMessage tepmsg(ErrorMessage, QString("View full!"));
//                mGlobalSignals::getInstance()->outputMessageSig(0, QString("View full!"), true);
//                return nullptr;
//            }
//
//            return nullptr;
//        }
//        viewName = QString("HistoryView_%1").arg(*noIt);
//        _usableCurveViewNums.erase(noIt);
//
//
//        if (VMManager->isViewNameExsit2D(viewName))
//        {
//            //如果视图存在，继续往下获取名称
//            while (true)
//            {
//                noIt = _usableCurveViewNums.begin();
//                if (noIt == _usableCurveViewNums.end())
//                {
//                    if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
//                    {
//                        mOutputMessage tepmsg(ErrorMessage, QString("视图已满！"));
//                        mGlobalSignals::getInstance()->outputMessageSig(0, QString("视图已满！"), true);
//                        return nullptr;
//                    }
//                    else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
//                    {
//                        mOutputMessage tepmsg(ErrorMessage, QString("View full!"));
//                        mGlobalSignals::getInstance()->outputMessageSig(0, QString("View full!"), true);
//                        return nullptr;
//                    }
//
//                    return nullptr;
//                }
//
//                viewName = QString("HistoryView_%1").arg(*noIt);
//                _usableCurveViewNums.erase(noIt);
//                if (VMManager->isViewNameExsit2D(viewName))
//                    continue;
//
//                break;
//            }
//        }
//
//        mPlotView* plot = new mPlotView();          //ViewportMonitor
//        plot->setTitle(viewName);
//        plot->setPlotType(MxFunctions::LineChart);
//        plot->setPlotBackGround(QVector3D(255.0, 255.0, 255.0));
//        mPlotView* ret = plot;
//        plot = nullptr;
//        delete plot;
//        return ret;
//    }
//    void mPostProbePanel::slot_createCurve()
//    {
//        PlotType plotType = LineChart;
//        //判断是否光滑曲线
//
//        mPlotView* plotView = nullptr;
//        QString vname = "";
//
//
//        //标识是否创建了新的曲线视图窗口
//        bool isNewWindow = false;
//
//        if (_ui->checkBox_newWindow->isChecked())
//        {
//            plotView = createNewCurvePlot(vname);
//            isNewWindow = true;
//        }
//        else
//        {
//            //获取当前激活的曲线窗口
//            plotView = VMManager->getActivePlotXYPlot();
//            //是否为空
//            if (plotView == nullptr)
//            {
//                plotView = createNewCurvePlot(vname);
//                isNewWindow = true;
//            }
//        }
//        if (plotView == nullptr)
//            return;
//
//        //获取数据
//        bool istrue = this->generateCurveData();
//        if (!istrue)
//        {
//            return;
//        }
//
//        const int ncurve = _curveNames.size();
//        if (ncurve == 0)
//        {
//
//        }
//
//        QHash<QString, QVector<double>>	yValues;
//        if (_YValuesD.size() > 0)
//        {
//            yValues = _YValuesD;
//        }
//        else
//        {
//            // 遍历源哈希表并进行转换
//            for (auto it = _YValuesF.begin(); it != _YValuesF.end(); ++it) {
//                const QString& key = it.key();
//                const QVector<float>& floatVector = it.value();
//
//                QVector<double> doubleVector;
//                // 将 floatVector 中的元素逐个转换为 double 类型
//                for (float value : floatVector) {
//                    doubleVector.append(static_cast<double>(value));
//                }
//                yValues.insert(key, doubleVector);
//            }
//        }
//
//        for (int i = 0; i < ncurve; ++i)
//        {
//            QString name = _curveNames.at(i);
//            plotView->setXText("");
//            plotView->setYText("");
//            plotView->setPlotType(plotType);
//            plotView->setPlotData(plotType, name, _XValue, yValues.value(name), true, true);
//            VMManager->openViewporPlot(plotView);
//        }
//
//        //更新列表
//        if (isNewWindow)
//        {
//            const int nrow = _ui->tableWidget->rowCount();
//            _ui->tableWidget->insertRow(nrow);
//            QTableWidgetItem* item = new QTableWidgetItem(vname);
//            item->setToolTip(vname);
//            _ui->tableWidget->setItem(nrow, 0, item);
//
//            connect(_ui->tableWidget, &QTableWidget::itemClicked, this, &mPostProbePanel::slot_onCellClicked);
//            emit _ui->tableWidget->itemClicked(item);
//
//            QToolButton* tb = new QToolButton;
//            tb->setIcon(QIcon(":/mMainWidgets/Icon/clear.png"));
//            connect(tb, SIGNAL(clicked()), this, SLOT(slot_deleteCurvePlot()));
//            _ui->tableWidget->setCellWidget(nrow, 1, tb);
//        }
//        else
//        {
//            //更新曲线名称QListWidget
//            mPlotView* plotView = VMManager->getActivePlotXYPlot();
//            if (plotView == nullptr)
//                return;
//            QHash<QString, QVector<double>> curveYValue = plotView->getYValue();
//            QStringList list = curveYValue.keys();
//            _ui->listWidget_2->clear();
//
//            _ui->listWidget_2->addItems(list);
//        }
//    }
//
//    void mPostProbePanel::slot_postActiveViewChanged(QMdiSubWindow*)
//    {
//        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();//VMManager->getActiveREBUILDPostRend();
//        if (postRend == nullptr)
//        {
//            _ui->listWidget_2->setVisible(true);
//            //更新曲线名称
//            mPlotView* plotView = VMManager->getActivePlotXYPlot();
//            if (plotView == nullptr)
//                return;
//
//            QString viewName = plotView->getViewportName();
//            int row = _ui->tableWidget->rowCount();
//            for (int i = 0; i < row; ++i)
//            {
//                QTableWidgetItem* item = _ui->tableWidget->item(i, 0);
//                QString name = item->text();
//                //tableWidget包含该视图
//                if (viewName == name)
//                {
//                    QHash<QString, QVector<double>> curveYValue = plotView->getYValue();
//                    QStringList list = curveYValue.keys();
//                    _ui->listWidget_2->clear();
//
//                    _ui->listWidget_2->addItems(list);
//                    break;
//                }
//            }
//            return;
//        }
//        else
//        {
//            _postRend = postRend;
//            QString viewName = postRend->getViewportName();
//            QString postName = postRend->getPostDataName();
//            if (_curViewName != viewName)
//            {
//                _postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
//                if (_postDB == nullptr)
//                {
//                    return;
//                }
//                //获取所有变量
//                QStringList varNames = _postDB->getAllVariableNames();
//                _ui->comboBox_var->clear();
//                _ui->comboBox_var->addItems(varNames);
//            }
//        }
//
//        _ui->listWidget_2->setVisible(false);
//    }
//    void mPostProbePanel::slot_onCellClicked(QTableWidgetItem* item)
//    {
//        if (item)
//        {
//            QString name = item->text();
//            //VMManager->openViewporPlot(name);
//            VMManager->openViewPortPlot2DbyName(name);
//
//            //更新曲线名称QListWidget
//            mPlotView* plotView = VMManager->getActivePlotXYPlot();
//            if (plotView == nullptr)
//                return;
//            QHash<QString, QVector<double>> curveYValue = plotView->getYValue();
//
//            _ui->listWidget_2->setVisible(true);
//            QStringList list = curveYValue.keys();
//            _ui->listWidget_2->clear();
//
//            _ui->listWidget_2->addItems(list);
//        }
//    }
//    void mPostProbePanel::slot_deleteCurvePlot()
//    {
//        //获取信号源
//        QToolButton* tb = qobject_cast<QToolButton*>(sender());
//        if (tb == nullptr)
//        {
//            return;
//        }
//        const int nrow = _ui->tableWidget->rowCount();
//
//        for (int i = 0; i < nrow; ++i)
//        {
//            if (tb == _ui->tableWidget->cellWidget(i, 1))
//            {
//                //获取名称
//                QTableWidgetItem* item = _ui->tableWidget->item(i, 0);
//                if (item != nullptr)
//                {
//                    QString name = item->text();
//                    mPlotView* plotView = VMManager->getActivePlotXYPlot();
//                    if (plotView != nullptr)
//                    {
//                        if (name == plotView->_name)
//                            _ui->listWidget_2->clear();
//                    }
//                    //删除视图
//                    VMManager->deletePostView2DWidget(name);
//                    //删除表格
//                    _ui->tableWidget->removeRow(i);
//                }
//            }
//        }
//    }
//    void mPostProbePanel::on_pushButton_6_clicked()
//    {
//        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();//VMManager->getActiveREBUILDPostRend();
//        if (postRend == nullptr)
//        {
//            return;
//        }
//        if (postRend->getViewportName() != _curViewName)
//        {
//            return;
//        }
//        auto postRender = postRend->getPostRender();
//        if (postRender == nullptr)
//        {
//            return;
//        }
//        //postRender->setPickFilter(PickFilter::PickNode);
//        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
//        postRender->updateHighLightRender();
//        postRend->update();
//    }
//
//}
