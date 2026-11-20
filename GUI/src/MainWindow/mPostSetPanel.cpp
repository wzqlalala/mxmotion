#include "mPostSetPanel.h"

#include "mOutputMessage.h"
#include "mGlobalSignals.h"

#include "mAppConfiguration.h"
#include "mDataPost1.h"

#include "mPostSetBase.h"

#include "mCommonFunctions.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include "mOneFrameData1.h"
#include "mPostOneFrameRendData.h"
//#include "mPostMeshPickData.h"
#include "mPostGUIData.h"


#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mPostOneFrameRendData.h"
#include <mIOFileManager.h>
#include "mxr/view/REBUILD_PostOneFrameRender.h"
#include "mxr/view/REBUILD_PostModelRender.h"
//#include "IMeshPickResultBase.h"
//#include "mPostDatabaseSingleton.h"
#include "mPostModelData.h"
#include <mMechGlobalSignals.h>
//#include "mOutputDatabase.h"
#include "mInfoWindow.h"

//using namespace Common;  //【集成】零使用且命名歧义（C2871），注释
using namespace PostData;
using namespace MechPreData;
//using namespace MIOFile;
namespace MechMainWindow
{
    mPostSetPanel::mPostSetPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);                                    // 创建垂直布局管理器
        _mainLayout->setContentsMargins(2, 0, 2, 0);                            // 设置布局的左右边距

        /********************集合列表表格********************/
        FoldSection* setList = new FoldSection("集合列表", this);           // 创建可折叠的区块控件

        _tableSet = new QTableWidget(this);                                 // 创建一个表格控件
        _tableSet->setColumnCount(2);                                       // 设置表格列数
        QStringList horizontalHeader = { "集合名字",  "类型" };
        _tableSet->setHorizontalHeaderLabels(horizontalHeader);             // 设置表格的水平表头
        _tableSet->verticalHeader()->setHidden(true);                       // 隐藏表格的垂直表头（行号列）
        //_tableSet->horizontalHeader()->setMinimumSectionSize(50);           // 设置水平表头最小列宽为50
        //_tableSet->resizeColumnsToContents();                               // 自动调整列宽
        _tableSet->horizontalHeader()->setStretchLastSection(true);         // 让表格的最后一列自动拉伸填充剩余空间
        _tableSet->horizontalHeader()->setHighlightSections(false);         // 禁用表头的高亮显示效果
        _tableSet->setSelectionBehavior(QAbstractItemView::SelectRows);     // 设置为整行选中
        _tableSet->setSelectionMode(QAbstractItemView::SingleSelection);    // 设置为单选模式
        _tableSet->setFixedHeight(140);

        setList->addRow(_tableSet);

        /********************功能选择********************/
        FoldSection* selecFunction = new FoldSection("功能选择", this);           // 创建可折叠的区块控件
        _btnNew = new QPushButton("新建");
        _btnNew->setStyleSheet("background-color: red;");
        _btnUpdate = new QPushButton("更新");
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->addWidget(_btnNew);
        hlayout->addWidget(_btnUpdate);
        selecFunction->addRow(hlayout);
        /********************选定参数********************/
        FoldSection* parametersSelect = new FoldSection("选定参数", this);           // 创建可折叠的区块控件
        _nameLabel = new QLabel("名称：");
        _typeLabel = new QLabel("类型：");
        _rangeLabel = new QLabel("范围：");
        _angleLabel = new QLabel("角度：");

        _nameLine = new QLineEdit();
        _angleDoubleSB = new QDoubleSpinBox;
        _typeCom = new QComboBox;
        QStringList typeList = { "节点","单元","单元表面","零件" };
        _typeCom->addItems(typeList);

        _rangeCom = new QComboBox;
        QStringList rangeList = { "拾取","全部","通过零件","ID","通过线角度","通过面角度"};
        _rangeCom->addItems(rangeList);


        parametersSelect->addRow(_nameLabel, _nameLine);
        parametersSelect->addRow(_typeLabel, _typeCom);
        parametersSelect->addRow(_rangeLabel, _rangeCom);
        parametersSelect->addRow(_angleLabel, _angleDoubleSB);
        _angleLabel->hide();
        _angleDoubleSB->hide();
        /********************拓展功能********************/
        //==================================功能页
        FoldSection* expandedFunctions = new FoldSection("拓展功能", this);           // 创建可折叠的区块控件
        _functionStackedWidget = new QStackedWidget;
        QWidget* firstPage = new QWidget();//空白页

        //==================================第二页
        QWidget* secondPage = new QWidget();
        QHBoxLayout* secondPageHLayout = new QHBoxLayout;
        _partNameLabel = new QLabel("零件名过滤");
        _partNameLine = new QLineEdit();
        secondPageHLayout->addWidget(_partNameLabel);
        secondPageHLayout->addWidget(_partNameLine);
        _partNameListWidget = new QListWidget;
        QVBoxLayout* secondPageVLayout = new QVBoxLayout;
        secondPageVLayout->addLayout(secondPageHLayout);
        secondPageVLayout->addWidget(_partNameListWidget);
        secondPageVLayout->setMargin(0);
        secondPage->setLayout(secondPageVLayout);
        //==================================第三页
        QWidget* thirdPage = new QWidget();
        _idCheck = new QCheckBox("连续ID");
        _highlightBtn = new QPushButton("高亮");
        QLabel* label = new QLabel("只有点击高亮后，输入的ID才会生效");
        QHBoxLayout* thirdPageHLayout = new QHBoxLayout;
        thirdPageHLayout->addWidget(_idCheck);
        thirdPageHLayout->addWidget(label);
        thirdPageHLayout->addWidget(_highlightBtn);

        _idStackedWidget= new QStackedWidget;
        //第一页
        _idTextEt = new QTextEdit;
        _idTextEt->setPlaceholderText("各ID之间使用英文字符\",\"进行分割");
        //第二页
        QWidget* idSecondPage = new QWidget();
        QHBoxLayout* idSecondHLayout = new QHBoxLayout;
        QHBoxLayout* idSecondHLayout2 = new QHBoxLayout;
        QHBoxLayout* idSecondHLayout3 = new QHBoxLayout;
        _idStartLabel = new QLabel("起始ID:");
        _idSpaceLabel = new QLabel("ID间隔:");
        _idEndLabel = new QLabel("终止ID:");

        _idStartLine = new QLineEdit;
        _idSpaceLine = new QLineEdit;
        _idEndLine = new QLineEdit;

        _idStartLine->setPlaceholderText("可不填，默认为最小ID");
        _idSpaceLine->setPlaceholderText("可不填，默认为1");
        _idEndLine->setPlaceholderText("可不填，默认为最大ID");
        idSecondHLayout->addWidget(_idStartLabel);
        idSecondHLayout->addWidget(_idStartLine);
        idSecondHLayout2->addWidget(_idSpaceLabel);
        idSecondHLayout2->addWidget(_idSpaceLine);
        idSecondHLayout3->addWidget(_idEndLabel);
        idSecondHLayout3->addWidget(_idEndLine);
        QVBoxLayout* idSecondVLayout = new QVBoxLayout;
        idSecondVLayout->addLayout(idSecondHLayout);
        idSecondVLayout->addLayout(idSecondHLayout2);
        idSecondVLayout->addLayout(idSecondHLayout3);
        idSecondVLayout->addStretch();
        idSecondVLayout->setMargin(0);
        idSecondPage->setLayout(idSecondVLayout);

        _idStackedWidget->addWidget(_idTextEt);//索引为0
        _idStackedWidget->addWidget(idSecondPage);//索引为0
        connect(_idCheck, &QCheckBox::clicked,this, [=]( bool isClicked) {
            if (isClicked)
            {
                _idStackedWidget->setCurrentIndex(1);
            }
            else
            {
                _idStackedWidget->setCurrentIndex(0);
            }
            });

        QVBoxLayout* thirdPageVLayout = new QVBoxLayout;
        thirdPageVLayout->addLayout(thirdPageHLayout);
        thirdPageVLayout->addWidget(_idStackedWidget);
        thirdPageVLayout->setMargin(0);
        thirdPage->setLayout(thirdPageVLayout);

        _functionStackedWidget->addWidget(firstPage);  // 索引为 0
        _functionStackedWidget->addWidget(secondPage);  // 索引为 1
        _functionStackedWidget->addWidget(thirdPage);  // 索引为 2

        expandedFunctions->addRow(_functionStackedWidget);
        _functionStackedWidget->setCurrentIndex(0);
        /********************拓展功能********************/
        FoldSection* operationFlod = new FoldSection("操作", this);           // 创建可折叠的区块控件
        _clearBtn = new QPushButton("清除");
        _confirmBtn = new QPushButton("确定");
        QHBoxLayout* operationHLayout = new QHBoxLayout;
        operationHLayout->addWidget(_clearBtn);
        operationHLayout->addWidget(_confirmBtn);
        operationFlod->addRow(operationHLayout);

        _mainLayout->addWidget(setList);
        _mainLayout->addWidget(selecFunction);
        _mainLayout->addWidget(parametersSelect);
        _mainLayout->addWidget(expandedFunctions);
        _mainLayout->addWidget(operationFlod);

        _mainLayout->addStretch();
        initConnncet();
    }

    mPostSetPanel::~mPostSetPanel()
    {

    }
    void mPostSetPanel::newSetPanel(int caseid)
    {
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
        if (postDB == nullptr) return;
        _db = postDB;
        //创建集合节点
        MechCommon::mMechGlobalSignals::getInstance()->createPostSetSig();

        // 新建集合前，先把渲染器切换到当前激活的后处理视口，
        // 避免 clearPanel() 误删旧模型渲染器上的切面。
        _rWidget = getCVM->getPostRenderWidget();

        // 新建集合时，清空旧数据，确保面板为空
        //clearPanel();

        QPointer<PostData::mPostGuiData> guiData = new PostData::mPostGuiData();
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addSectionData(_curViewName, guiData);

    }
    // B1b(M12/N-B-3)：视口改名跟随——基类默认字符串跟随 + 渲染窗口句柄按新名重取
    void mPostSetPanel::onViewportRenamed(const QString& oldName, const QString& newName)
    {
        mPanelBase::onViewportRenamed(oldName, newName);
        if (_rWidget != nullptr && _rWidget->getViewportName() == oldName)
            _rWidget = getCVM->getPostRenderByViewName(newName);
    }

    void mPostSetPanel::setCurViewName(const QString& viewName)
    {
        _curViewName = viewName;

        // 先根据当前 viewName 重新定位渲染器和数据库，再进行显隐/重建操作
        updateContextByViewName(viewName);

        if (_rWidget == nullptr)
            return;
        auto postRender = _rWidget->getPostRender();
        if (postRender == nullptr)
            return;
    }
    void mPostSetPanel::updateContextByViewName(const QString& viewName)
    {
        // 根据 viewName 重新定位当前应操作的渲染窗口和后处理数据库，
        // 避免在多后处理模型之间切换时仍持有旧模型的 _rWidget/_db。
        mxr::REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(viewName);
        if (rend == nullptr)
            rend = getCVM->getPostRenderWidget();
        _rWidget = rend;

        if (_rWidget != nullptr)
        {
            QString postName = _rWidget->getPostDataName();
            if (!postName.isEmpty())
                _db = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        }
    }

    void mPostSetPanel::slot_clear()
    {
        if (getCVM)
        {
            getCVM->clearPick();
            if (getCVM->getPreRenderWidget())
            {
                getCVM->getPreRenderWidget()->getHighlightController()->clearTempHighlight();
                getCVM->getPreRenderWidget()->getHighlightController()->clearTempMeshHighlight();
            }
        }
    }
    
    void mPostSetPanel::slot_confirm()
    {
        isSelect = false;

        QString name = _nameLine->text();
        QString type_name = _typeCom->currentText();
        if (name.isEmpty())
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("集合名为空，请输入集合名！"), true);
                return;
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name is empty, please enter the set name!"), true);
                return;
            }
        }

        //检查集合名是否已存在
        bool isexist = _countDataInformation.contains(name);
        if (isexist)
        {
            if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("该集合名已存在！"), true);
                return;
            }
            else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
            {
                mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name already exists!"), true);
                return;
            }
        }

        //获取选中的内容
        if (type_name == "节点")
        {
            const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
            if (!nodes.empty())
                //将内容进行保存
                _countDataInformation[name] = nodes;
            else{
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("该集合没有数据！"), true);
                    return;
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name already exists!"), true);
                    return;
                }
            }
        }
        else if (type_name == "单元")
        {
            const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
            if (!nodes.empty())
                _countDataInformation[name] = nodes;
            else {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("该集合没有数据！"), true);
                    return;
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name already exists!"), true);
                    return;
                }
            }

        }
        else if (type_name == "单元表面")
        {
            const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshFace);
            if (!nodes.empty())
                _countDataInformation[name] = nodes;
            else {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("该集合没有数据！"), true);
                    return;
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name already exists!"), true);
                    return;
                }
            }
        }
        else {
            const auto& nodes = _rWidget->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshPart);
            if (!nodes.empty())
                _countDataInformation[name] = nodes;
            else {
                if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("该集合没有数据！"), true);
                    return;
                }
                else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(2, QString("The set name already exists!"), true);
                    return;
                }
            }
        }

        //列表中添加信息
        int row = _tableSet->rowCount();          // 获取当前行数（初始为0）
        _tableSet->insertRow(row);                // 在末尾插入一个新行
        // 创建两个单元格项
        QTableWidgetItem* itemName = new QTableWidgetItem(name);
        QTableWidgetItem* itemType = new QTableWidgetItem(type_name);
        // 将项放入对应列
        _tableSet->setItem(row, 0, itemName);
        _tableSet->setItem(row, 1, itemType);

        slot_clear();
    }

    void mPostSetPanel::slot_type(QString type)
    {
        QStringList rangeList;
        if (type == "节点")
            rangeList<< "拾取"<<"全部"<<"通过零件"<<"ID"<<"通过线角度"<<"通过面角度";
        else if (type == "单元")
            rangeList <<"拾取"<<"全部"<<"通过零件"<<"ID"<<"通过面角度" ;
        else if (type == "单元表面")
            rangeList << "拾取"<< "全部"<< "通过零件"<< "通过面角度" ;
        else
            rangeList <<"拾取"<< "全部"<< "通过零件" ;

        _rangeCom->clear();
        _rangeCom->addItems(rangeList);

    }
    void mPostSetPanel::slot_range(QString type)
    {
        QString _t = _typeCom->currentText();

        slot_clear();
        _angleLabel->hide();
        _angleDoubleSB->hide();
        _functionStackedWidget->setCurrentIndex(0);
        _rWidget->setPickFilter(mxr::PickFilter::PickNothing);

        if (type == "拾取")
        {
            if (_t == "节点")
                _rWidget->setPickFilter(mxr::PickFilter::PickMeshNode);
            else if (_t == "单元")
                _rWidget->setPickFilter(mxr::PickFilter::PickMeshElement);
            else if (_t == "单元表面")
                _rWidget->setPickFilter(mxr::PickFilter::PickMeshFace);
            else
                _rWidget->setPickFilter(mxr::PickFilter::PickMeshPart);
        }
        else if (type == "全部")
        {
            REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
            if (rebuildPostRend == nullptr) return;

            auto postRender = rebuildPostRend->getPostRender();
            if (postRender == nullptr) return;

            mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
            if (guidata == nullptr) return;

            int fi = guidata->_cloudFrame;
            QPointer<mPostModelData> model = _db->getModelData(fi);
            if (!model) return;

            std::vector<mxr::MeshSelectInfoData> information;
            mxr::SelectionInfoType selType;

            if (_t == "节点") {
                selType = mxr::SelectionInfoType::MeshNode;
                std::set<int> ids = model->getAllNodeIDs();
                for (int id : ids) {
                    mxr::MeshSelectInfoData info;
                    info.data_id.node_id = id;
                    info.data_id.element_id = id;
                    information.push_back(info);
                }
            }
            else if (_t == "单元") {
                selType = mxr::SelectionInfoType::MeshElement;
                std::set<int> ids = model->getAllNodeIDs();
                for (int id : ids) {
                    mxr::MeshSelectInfoData info;
                    info.part_id = 0;
                    info.data_id.node_id = id;
                    info.data_id.element_id = id;
                    info.dim = 2;
                    information.push_back(info);
                }

            }
            else if (_t == "单元表面") {
                selType = mxr::SelectionInfoType::MeshFace;
                QList<QPair<int,int>>  ids = model->getAllSurface();
                
                for (const auto& s : ids) {
                    mxr::MeshSelectInfoData info;
                    info.part_id = 0;
                    info.data_id.element_id = s.first;
                    info.data_id.node_id = s.first;
                    info.data_index.line_index= s.first;
                    info.data_index.face_index = s.first;
                    information.push_back(info);
                }
            }
            else
            {
                selType = mxr::SelectionInfoType::MeshPart;

                mxr::MeshSelectInfoData info;
                info.part_id = 0;
                information.push_back(info);
            }

            _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, selType, information);
            MechPreData::UpdatePostWidgetHighlight(_rWidget);
        }
        else if (type == "通过零件")
        {
            _functionStackedWidget->setCurrentIndex(1);

            QStringList PartName = _db->getAllPartName();
            //列表中添加信息
            QListWidgetItem* item = new QListWidgetItem(PartName[0]);
            _partNameListWidget->clear();
            _partNameListWidget->addItem(item);

        }
        else if (type == "ID")
        {
            _functionStackedWidget->setCurrentIndex(2);
            _idStartLine->clear();
            _idSpaceLine->clear();
            _idEndLine->clear();
            _idTextEt->clear();

        }
        else if (type == "通过线角度")
        {
            _angleLabel->show();
            _angleDoubleSB->show();
        }
        else if (type == "通过面角度")
        {
            _angleLabel->show();
            _angleDoubleSB->show();
        }
    }


    void mPostSetPanel::slot_new()
    {
        slot_clear();
        isSelect = false;
        _btnNew->setStyleSheet("background-color: red;");
        _btnUpdate->setStyleSheet("background-color: white;");
        _confirmBtn->show();
    }

    void mPostSetPanel::slot_update()
    {
        if (!isSelect)
        {
            slot_clear();
            isSelect = true;
        }
        
        _btnNew->setStyleSheet("background-color: white;");
        _btnUpdate->setStyleSheet("background-color: red;");
        _confirmBtn->hide();
    }

    void mPostSetPanel::slot_highlight()
    {
        slot_clear();

        QString _t = _typeCom->currentText();

        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return;

        auto postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        int fi = guidata->_cloudFrame;
        QPointer<mPostModelData> model = _db->getModelData(fi);
        if (!model) return;

        std::vector<mxr::MeshSelectInfoData> information;
        mxr::SelectionInfoType selType;

        bool selection = _idCheck->isChecked();
        if (!selection)
        {
            QString id_list = _idTextEt->toPlainText();
            QStringList idArr = id_list.split(",", QString::SkipEmptyParts);

            QList<int> idIntList; // 存储int类型ID

            for (QString strId : idArr)
            {
                strId = strId.trimmed(); // 清除前后空格
                bool ok;
                int numId = strId.toInt(&ok);
                if (ok) // 转换成功才加入列表
                    idIntList.append(numId);
                //【日志清理】else 分支原为无效 ID 的 qDebug 提示，随集成期日志一并移除
            }
            if (_t == "节点") {
                selType = mxr::SelectionInfoType::MeshNode;
                for (int i:idIntList)
                {
                    mxr::MeshSelectInfoData info;
                    info.data_id.node_id = i;
                    info.data_id.element_id = i;
                    information.push_back(info);
                }
            }
            else
            {
                selType = mxr::SelectionInfoType::MeshElement;
                for (int i : idIntList)
                {
                    mxr::MeshSelectInfoData info;
                    info.part_id = 0;
                    info.data_id.node_id = i;
                    info.data_id.element_id = i;
                    info.dim = 2;
                    information.push_back(info);
                }
            }
        }
        else{
            int start_id,interval, terminate_id;
            if (_idStartLine->text().isEmpty())
                start_id = 0;
            else
                start_id = _idStartLine->text().toInt();
           
            if (_idSpaceLine->text().isEmpty())
                interval = 1;
            else
                interval= _idSpaceLine->text().toInt();
            if (_idEndLine->text().isEmpty())
            { 
                std::set<int> ids = model->getAllNodeIDs();
                terminate_id = ids.size();
            }
            else
                terminate_id = _idEndLine->text().toInt();

            if (_t == "节点") {
                selType = mxr::SelectionInfoType::MeshNode;
                for (int i = start_id; i <= terminate_id; i += interval)
                {
                    mxr::MeshSelectInfoData info;
                    info.data_id.node_id = i;
                    info.data_id.element_id = i;
                    information.push_back(info);
                }
            }
            else
            {
                selType = mxr::SelectionInfoType::MeshElement;
                for (int i = start_id; i <= terminate_id; i += interval)
                {
                    mxr::MeshSelectInfoData info;
                    info.part_id = 0;
                    info.data_id.node_id = i;
                    info.data_id.element_id = i;
                    info.dim = 2;
                    information.push_back(info);
                }
            }
        }
        _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, selType, information);
        MechPreData::UpdatePostWidgetHighlight(_rWidget);

        rebuildPostRend->updatePaint();
    }

    void mPostSetPanel::table_updata(int row, int column)
    {
        // 如果处于不可更新状态，直接返回
        if (!isSelect)
            return;

        // 获取该行的名称和类型
        QTableWidgetItem* itemName = _tableSet->item(row, 0);
        QTableWidgetItem* itemType = _tableSet->item(row, 1);
        if (!itemName || !itemType)
            return;

        QString name = itemName->text();
        QString type = itemType->text();
        updateSetInfo(name, type);
    }

    void mPostSetPanel::slot_partname(const QModelIndex& index)
    {
        QString _t = _typeCom->currentText();
        REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderWidget();
        if (rebuildPostRend == nullptr) return;

        auto postRender = rebuildPostRend->getPostRender();
        if (postRender == nullptr) return;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
        if (guidata == nullptr) return;

        int fi = guidata->_cloudFrame;
        QPointer<mPostModelData> model = _db->getModelData(fi);
        if (!model) return;

        std::vector<mxr::MeshSelectInfoData> information;
        mxr::SelectionInfoType selType;

        if (_t == "节点") {
            selType = mxr::SelectionInfoType::MeshNode;
            std::set<int> ids = model->getAllNodeIDs();
            for (int id : ids) {
                mxr::MeshSelectInfoData info;
                info.data_id.node_id = id;
                info.data_id.element_id = id;
                information.push_back(info);
            }
        }
        else if (_t == "单元") {
            selType = mxr::SelectionInfoType::MeshElement;
            std::set<int> ids = model->getAllNodeIDs();
            for (int id : ids) {
                mxr::MeshSelectInfoData info;
                info.part_id = 0;
                info.data_id.node_id = id;
                info.data_id.element_id = id;
                info.dim = 2;
                information.push_back(info);
            }

        }
        else if (_t == "单元表面") {
            selType = mxr::SelectionInfoType::MeshFace;
            QList<QPair<int, int>>  ids = model->getAllSurface();

            for (const auto& s : ids) {
                mxr::MeshSelectInfoData info;
                info.part_id = 0;
                info.data_id.element_id = s.first;
                info.data_id.node_id = s.first;
                info.data_index.line_index = s.first;
                info.data_index.face_index = s.first;
                information.push_back(info);
            }
        }
        else
        {
            selType = mxr::SelectionInfoType::MeshPart;

            mxr::MeshSelectInfoData info;
            info.part_id = 0;
            information.push_back(info);
        }
        _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, selType, information);
        MechPreData::UpdatePostWidgetHighlight(_rWidget);

        rebuildPostRend->updatePaint();
    }

    void mPostSetPanel::slot_linepart(const QString& text)
    {
        // 遍历
        for (int i = 0; i < _partNameListWidget->count(); ++i) {
            QListWidgetItem* item = _partNameListWidget->item(i);
            // 如果文本为空，显示所有项；否则根据是否包含过滤字符串决定显隐
            bool visible = text.isEmpty() || item->text().contains(text);
            item->setHidden(!visible);
        }
    }

    void mPostSetPanel::updateSetInfo(QString name,QString type)
    {
        slot_clear();
        //从哈希表中重新获取信息
        std::vector<mxr::MeshSelectInfoData> information = _countDataInformation[name];
        //重新显示
        mxr::SelectionInfoType selType;
        if(type=="节点")
            selType = mxr::SelectionInfoType::MeshNode;
        else if(type=="单元")
            selType = mxr::SelectionInfoType::MeshElement;
        else if(type=="单元表面")
            selType = mxr::SelectionInfoType::MeshFace;
        else
            selType = mxr::SelectionInfoType::MeshPart;

        _rWidget->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD,selType,information);
        MechPreData::UpdatePostWidgetHighlight(_rWidget);
    }

    void mPostSetPanel::initConnncet()
    {
        //界面中的按钮
        connect(_clearBtn, &QPushButton::clicked, this, &mPostSetPanel::slot_clear);//清除
        connect(_confirmBtn, &QPushButton::clicked, this, &mPostSetPanel::slot_confirm);//确认
        connect(_typeCom, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &mPostSetPanel::slot_type);//类型
        connect(_btnNew, &QPushButton::clicked, this, &mPostSetPanel::slot_new);//新建
        connect(_btnUpdate, &QPushButton::clicked, this, &mPostSetPanel::slot_update);//更新
        connect(_highlightBtn,&QPushButton::clicked,this,&mPostSetPanel::slot_highlight);
        // 在构造函数或初始化函数中
        connect(_tableSet, &QTableWidget::cellClicked,this, &mPostSetPanel::table_updata);
        connect(_rangeCom, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this, &mPostSetPanel::slot_range);        
        connect(_partNameListWidget, &QListWidget::clicked, this, &mPostSetPanel::slot_partname);
        connect(_partNameLine, &QLineEdit::textChanged, this, &mPostSetPanel::slot_linepart);
    }
}