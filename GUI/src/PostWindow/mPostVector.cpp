#include "mPostVector.h"

#include <QFormLayout>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;
using namespace MIOFile;

namespace PostWindow
{
    // —— S5 G5 迁移（门面直接替换批，D9 载体）：矢量图创建/删除经 G5 对象清单 ——
    // 旧 createVectorGraph 两重载（标签色集变体/节点集变体）随门面移除集消亡，
    // 收敛为 addObject(VectorGraphParams)：labels 直收标签→色对组（0-255 口径，
    // 渲染端 /255 归一同旧门面转换）；source=Nodes+nodeIds 承载节点集变体；
    // size/arrowType/isShowText 同名直传。旧 create 即覆盖语义由面板既有
    // "先家族删后建"序列承接（deleteVectorGraph→台账摘除）。
    static int addVectorGraphObject(mxr::REBUILD_PostRender* facade,
        const QVector<QPair<QString, QVector3D>>& typeColor, double arrowSize,
        const std::set<int>* nodeIds)
    {
        mxr::VectorGraphParams params;
        params.labels = typeColor;
        params.size = arrowSize;
        params.arrowType = 1;
        params.isShowText = false;
        if (nodeIds != nullptr)
        {
            // Qt 5.13.2 无 QVector 迭代器对构造——集合逐个转填
            params.source = mxr::VectorGraphParams::VectorSource::Nodes;
            for (int id : *nodeIds)
                params.nodeIds.append(id);
        }
        return facade->addObject(params);
    }

    mPostVectorPanel::mPostVectorPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;

		_mainLayout = new QVBoxLayout(this);
		_mainLayout->setContentsMargins(2, 0, 2, 0);

		/********************对象********************/
		FoldSection* objectFold = new FoldSection("对象", this);
        //拾取对象
        QWidget* widgetPick = new QWidget();
        QHBoxLayout* rowH = new QHBoxLayout(widgetPick);
        rowH->setContentsMargins(0, 0, 0, 0);
        rowH->setSpacing(4);

        QLabel* labelObject = new QLabel("节点");
        rowH->addWidget(labelObject);
        _labelSelectStatu = new QLabel("默认为全部");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        _labelSelectStatu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        rowH->addWidget(_labelSelectStatu);

        _btnAdd = new QPushButton(); _btnAdd->setToolTip("添加");
        _btnReplace = new QPushButton(); _btnReplace->setToolTip("替换");
        _btnClear = new QPushButton(); _btnClear->setToolTip("清空");
        _btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/post/tianjia.png"));
        _btnReplace->setIcon(QIcon(":/mMainWidgets/Icon/post/tihuan.png"));
        _btnClear->setIcon(QIcon(":/mMainWidgets/Icon/post/qingkong.png"));
        _btnAdd->setFixedWidth(30);
        _btnReplace->setFixedWidth(30);
        _btnClear->setFixedWidth(30);

        rowH->addWidget(_btnAdd);
        rowH->addWidget(_btnReplace);
        rowH->addWidget(_btnClear);

        objectFold->addRow(widgetPick);

        objectFold->refreshLayout();
		m_foldWidgets.append(objectFold);
		_mainLayout->addWidget(objectFold);

        /********************样式********************/
        FoldSection* styleFold = new FoldSection("样式", this);
        _comboSize = new QComboBox();
        _comboSize->addItems({ "矢量大小", "固定大小"});
        _sliderSize = new QSlider(Qt::Horizontal);
        _checkRTG = new QCheckBox("实时生成");
        _btnDefault = new QPushButton("默认");
        _btnUpdate = new QPushButton("更新");
        _sliderSize->setRange(0, 20);
        _sliderSize->setValue(10);

        styleFold->addRow(new QLabel("箭头大小"), _comboSize, _sliderSize);
        styleFold->addRow(_checkRTG, _btnDefault, _btnUpdate);

        styleFold->refreshLayout();
        m_foldWidgets.append(styleFold);
        _mainLayout->addWidget(styleFold);
		_mainLayout->addStretch();

        connect(_btnAdd, &QPushButton::clicked, this, &mPostVectorPanel::slot_addPick);
        connect(_btnReplace, &QPushButton::clicked, this, &mPostVectorPanel::slot_addPick);
        connect(_btnClear, &QPushButton::clicked, this, &mPostVectorPanel::slot_clearPick);
        connect(_comboSize, &QComboBox::currentTextChanged, this, &mPostVectorPanel::slot_chooseArrowType);
        connect(_sliderSize, &QSlider::valueChanged, this, &mPostVectorPanel::slot_changeArrowSize);
        connect(_checkRTG, &QCheckBox::clicked, this, &mPostVectorPanel::slot_isRealTimeGenerate);
        connect(_btnDefault, &QPushButton::clicked, this, &mPostVectorPanel::slot_default);
        connect(_btnUpdate, &QPushButton::clicked, this, &mPostVectorPanel::slot_update);
	}

	void mPostVectorPanel::newVectorPanel(int caseid)
	{
		//创建矢量图节点
		MechCommon::mMechGlobalSignals::getInstance()->createVectorSig();

        QPointer<PostData::mPostGuiData> guiData = new PostData::mPostGuiData();
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addVectorData(_curViewName, guiData);

        //创建矢量图
        slot_createAllVector();

        _comboSize->setCurrentIndex(0);
        _sliderSize->setValue(10);
        _checkRTG->setChecked(false);
	}

	QString mPostVectorPanel::getCloudViewName(QString view)
	{
        //源云图定位：优先管理器关系（VectorInfo.viewName → resultId → CloudItem.viewName）
        //——显示名接管后视口名不再携带 "_Vector<N>" 后缀，名字解析只作兜底
        //（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
        const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
            .getCloudViewNameByView(PostData::ItemType::VectorItem, view);
        if (!byInfo.isEmpty())
            return byInfo;
        QRegularExpression re("^(.*)(_Vector\\d+)$");
        QRegularExpressionMatch match = re.match(view);
        if (match.hasMatch()) {
            return match.captured(1);
        }
        return view;
	}

	void mPostVectorPanel::setCurViewName(const QString& viewName)
	{
        _curViewName = viewName;

        //界面更新
        updatePanel();
	}

    bool mPostVectorPanel::haveVector(QString cloudView)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(cloudView);
        if (postRend == nullptr) return false;

        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return false;

        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return false;

        QString varName = guidata->_cloudVarName;
        int count = postDB->getVariableCompoCount(varName);
        if (count == 3) return true;
        return false;
    }

    void mPostVectorPanel::updatePanel()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）

        _comboSize->blockSignals(true);
        _sliderSize->blockSignals(true);
        _checkRTG->blockSignals(true);

        _comboSize->setCurrentText(vecData->_vectorArrowMethod);
        _sliderSize->setValue(vecData->_vectorArrowSize);
        _checkRTG->setChecked(vecData->_vectorIsRealTimeGenerate);

        if (vecData->_vectorArrowNodeType == "全部")
        {
            _labelSelectStatu->setText("默认为全部");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");

            addVectorGraphObject(postRender.get(), vecData->_vectorArrowTypeColor, vecData->_vectorArrowSize/10.0, nullptr);   // S5 G5：addObject(VectorGraph,{labels,size,type,isShowText})
        }
        else if (vecData->_vectorArrowNodeType == "拾取")
        {
            std::set<int> ids(vecData->_vectorArrowNodeIDs.begin(), vecData->_vectorArrowNodeIDs.end());
            _labelSelectStatu->setText(QString("已选择%1个对象").arg(ids.size()));
            _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");
            addVectorGraphObject(postRender.get(), vecData->_vectorArrowTypeColor, vecData->_vectorArrowSize/10.0, &ids);   // S5 G5：addObject(VectorGraph,{source:nodes,nodeIds,labels,size,type})
        }

        postRend->updatePaint();

        _comboSize->blockSignals(false);
        _sliderSize->blockSignals(false);
        _checkRTG->blockSignals(false);
    }

    void mPostVectorPanel::slot_addPick()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）

        QPushButton* pb = qobject_cast<QPushButton*>(sender());
        if (pb == nullptr) return;

        if (pb == _btnReplace)
            _pickIds.clear();

        std::vector<MeshSelectInfoData> res;
        int num = 0;
        //获取拾取模式
        PickDataType pickDataType = getCVM->getPostPickDataType(num);
        if (pickDataType != PickDataType::Node)
        {
            return;
        }

        if (num == 0) return;

        int count = 0;
        res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
        count = res.size() + _pickIds.size();

        getCVM->clearPick();
        if (count == 0)
        {
            _labelSelectStatu->setText("当前未选择");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        }
        else
        {
            _labelSelectStatu->setText(QString("已选择%1个对象").arg(count));
            _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");
        }

        for (auto r : res)
        {
            _pickIds.insert(r.data_id.element_id);
        }

        std::set<int> ids(_pickIds.begin(), _pickIds.end());

        QVector<QPair<QString, QVector3D>> type_color;
        QPair<QString, QVector3D> tmp;
        tmp.first = "XYZ";
        tmp.second = QVector3D(0, 0, 0);
        type_color.append(tmp);

        double arrowsize = _sliderSize->value();
        arrowsize = arrowsize / 10.0;
        if (vecData->_vectorIsRealTimeGenerate)
        {
            addVectorGraphObject(postRender.get(), type_color, arrowsize, &ids);   // S5 G5：addObject(VectorGraph,{source:nodes,nodeIds,labels,size,type})
            postRend->updatePaint();
        }

        vecData->_vectorArrowNodeType = "拾取";
        vecData->_vectorArrowNodeIDs = _pickIds;
    }

    void mPostVectorPanel::slot_clearPick()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        _pickIds.clear();
        _labelSelectStatu->setText("当前未选择");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        getCVM->clearPick();

        vecData->_vectorArrowNodeType = "拾取";
        vecData->_vectorArrowNodeIDs = _pickIds;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）
        postRend->updatePaint();
    }

    void mPostVectorPanel::slot_createAllVector()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)	return;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）

        QString postPathName = postDB->getFile();
        //获取云图视口存储的信息
        QString cloudview = getCloudViewName(_curViewName);
        mPostGUIData* cloudGuidata = getCVM->getGUIDataByViewName(cloudview);
        if (cloudGuidata == nullptr) return;

        if (postDB->getDataFlag())
        {
            mIMxdbFile1* mxdbReader = new mIMxdbFile1;
            mxdbReader->readDisplacementData(postPathName, cloudGuidata->_cloudFrame, cloudGuidata->_cloudVarName);
            delete mxdbReader;
            mxdbReader = nullptr;
        }
        else
        {
            mMxdbThread* mxdbReader = new mMxdbThread(postPathName, postDB, this);
            mxdbReader->readVectorData(postPathName, cloudGuidata->_cloudFrame, cloudGuidata->_cloudVarName);
            delete mxdbReader;
            mxdbReader = nullptr;
        }
#ifdef DEBUGQHASH
        QHash<int, QVector3D> hashmp = postDB->getVectorDataByFrameID(cloudGuidata->_cloudFrame);
        QList<int> keys = getQHashKeys(hashmp);
        QList<QVector3D> values = getQHashValues(hashmp);
#endif // DEBUGQHASH

        QVector<QPair<QString, QVector3D>> type_color;
        QPair<QString, QVector3D> tmp;
        tmp.first = "XYZ";
        tmp.second = QVector3D(0, 0, 0);
        type_color.append(tmp);

        double arrowsize = 0;
        arrowsize = _sliderSize->value();
        arrowsize = arrowsize / 10.0;

        //生成全部矢量图
        addVectorGraphObject(postRender.get(), type_color, arrowsize, nullptr);   // S5 G5：addObject(VectorGraph,{labels,size,type,isShowText})
        postRend->updatePaint();

        vecData->_vectorArrowMethod = _comboSize->currentText();
        vecData->_vectorArrowNodeType = "全部";
        vecData->_vectorArrowTypeColor = type_color;
        vecData->_vectorArrowSize = _sliderSize->value();

        _labelSelectStatu->setText("默认为全部");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
    }

    void mPostVectorPanel::slot_isRealTimeGenerate()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        vecData->_vectorIsRealTimeGenerate = _checkRTG->isChecked();
    }

    void mPostVectorPanel::slot_default()
    {
        _pickIds.clear();
        slot_createAllVector();
    }

    void mPostVectorPanel::slot_update()
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        double arrowsize = _sliderSize->value();
        arrowsize = arrowsize / 10.0;
        QVector<QPair<QString, QVector3D>> type_color;
        QPair<QString, QVector3D> tmp;
        tmp.first = "XYZ";
        tmp.second = QVector3D(0, 0, 0);
        type_color.append(tmp);

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）
        if (vecData->_vectorArrowNodeType == "全部")
        {
            addVectorGraphObject(postRender.get(), type_color, arrowsize, nullptr);   // S5 G5：addObject(VectorGraph,{labels,size,type,isShowText})
        }
        else if(vecData->_vectorArrowNodeType == "拾取")
        {
            std::set<int> ids(_pickIds.begin(), _pickIds.end());
            addVectorGraphObject(postRender.get(), type_color, arrowsize, &ids);   // S5 G5：addObject(VectorGraph,{source:nodes,nodeIds,labels,size,type})
        }
        postRend->updatePaint();
    }

    void mPostVectorPanel::slot_chooseArrowType(const QString&)
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        QString type = _comboSize->currentText();
        vecData->_vectorArrowMethod = type;
    }

    void mPostVectorPanel::slot_changeArrowSize(double value)
    {
        PostData::mPostGuiData* vecData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getVectorData(_curViewName);
        if (vecData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        double arrowsize = _sliderSize->value();
        arrowsize = arrowsize / 10.0;
        QVector<QPair<QString, QVector3D>> type_color;
        QPair<QString, QVector3D> tmp;
        tmp.first = "XYZ";
        tmp.second = QVector3D(0, 0, 0);
        type_color.append(tmp);

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）
        if (vecData->_vectorIsRealTimeGenerate)
        {
            if (vecData->_vectorArrowNodeType == "全部")
            {
                addVectorGraphObject(postRender.get(), type_color, arrowsize, nullptr);   // S5 G5：addObject(VectorGraph,{labels,size,type,isShowText})
            }
            else if(vecData->_vectorArrowNodeType == "拾取")
            {
                std::set<int> ids(_pickIds.begin(), _pickIds.end());
                addVectorGraphObject(postRender.get(), type_color, arrowsize, &ids);   // S5 G5：addObject(VectorGraph,{source:nodes,nodeIds,labels,size,type})
            }
            postRend->updatePaint();
        }
        vecData->_vectorArrowSize = _sliderSize->value();
    }

    void mPostVectorPanel::hideEvent(QHideEvent* event)
    {
        if (!getCVM) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）
        postRend->updatePaint();
    }
}