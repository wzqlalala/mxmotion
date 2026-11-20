#include "mPostStreamLinePanel.h"

#include <QFormLayout>
#include <algorithm>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDockLayoutManager.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"

//Post
#include "mPostDataBase.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）
#include "mIMxdbFile1.h"
#include "mMxdbThread.h"
#include "mPostDeformation.h"   // readNodeVectorField：native/vtk/新格式 mxdb 的速度场共用读入口
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostStreamLineRender.h"


//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MIOFile;

namespace PostWindow
{
    mPostStreamLinePanel::mPostStreamLinePanel(QWidget* parent) : mPanelBase(parent)
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

        QLabel* labelObject = new QLabel("单元");
        rowH->addWidget(labelObject);
        _labelSelectStatu = new QLabel("请选择一个单元（仅一个）");
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

        /********************流线生成********************/
        QWidget* widgetGenerate = new QWidget();
        QHBoxLayout* rowGen = new QHBoxLayout(widgetGenerate);
        rowGen->setContentsMargins(0, 0, 0, 0);
        rowGen->setSpacing(4);

        _btnGenerate = new QPushButton("生成流线");
        rowGen->addWidget(_btnGenerate);

        objectFold->addRow(widgetGenerate);

        objectFold->refreshLayout();
		m_foldWidgets.append(objectFold);
		_mainLayout->addWidget(objectFold);
		_mainLayout->addStretch();

        connect(_btnAdd, &QPushButton::clicked, this, &mPostStreamLinePanel::slot_addPick);
        connect(_btnReplace, &QPushButton::clicked, this, &mPostStreamLinePanel::slot_addPick);
        connect(_btnClear, &QPushButton::clicked, this, &mPostStreamLinePanel::slot_clearPick);
        connect(_btnGenerate, &QPushButton::clicked, this, &mPostStreamLinePanel::slot_createStreamLine);
	}

	void mPostStreamLinePanel::newStreamLinePanel(int caseid)
	{
		//创建流线图节点
		MechCommon::mMechGlobalSignals::getInstance()->createStreamLineSig();
	}

	QString mPostStreamLinePanel::getCloudViewName(QString view)
	{
        //源云图定位：优先管理器关系（StreamLineInfo.viewName → resultId → CloudItem.viewName）
        //——显示名接管后视口名不再携带 "_StreamLine<N>" 后缀，名字解析只作兜底
        //（派生视口创建信号先于 addItem 登记的窗口期，彼时视口名仍带后缀）
        const QString byInfo = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
            .getCloudViewNameByView(PostData::ItemType::StreamLineItem, view);
        if (!byInfo.isEmpty())
            return byInfo;
        QRegularExpression re("^(.*)(_StreamLine\\d+)$");
        QRegularExpressionMatch match = re.match(view);
        if (match.hasMatch()) {
            return match.captured(1);
        }
        return view;
	}

	void mPostStreamLinePanel::setCurViewName(const QString& viewName)
	{
	       _curViewName = viewName;

	       // 确保当前视口对应的数据存在
	       QPointer<PostData::mPostGuiData> slData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getStreamLineData(_curViewName);
	       if (slData == nullptr)
	       {
	           QPointer<PostData::mPostGuiData> guiData = new PostData::mPostGuiData();
	           getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addStreamLineData(_curViewName, guiData);
	       }

	       //界面更新
	       updatePanel();
	}

    bool mPostStreamLinePanel::haveStreamLine(QString cloudView)
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(cloudView);
        if (postRend == nullptr) return false;

        MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return false;

        QString postName = postRend->getPostDataName();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)
            return false;

        QString varName = guidata->_cloudVarName;
        if ("Velocity" == varName)//判断是否为速度分量
        {
            return true;
        }
        
        return false;
    }

    void mPostStreamLinePanel::updatePanel()
    {
        PostData::mPostGuiData* slData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getStreamLineData(_curViewName);
        if (slData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        if (slData->_streamLineElementType == "拾取" && !slData->_streamLineElementIDs.empty())
        {
            std::set<int> ids(slData->_streamLineElementIDs.begin(), slData->_streamLineElementIDs.end());

            // 流线生成只需要一个单元，恢复旧数据时强制截断
            if (ids.size() > 1)
            {
                int firstId = *ids.begin();
                ids.clear();
                ids.insert(firstId);
                slData->_streamLineElementIDs = ids;
            }

            _labelSelectStatu->setText(QString("已选择对象ID%1").arg(*ids.begin()));
            _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");


            QString postName = postRend->getPostDataName();
            QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
            if (postDB == nullptr)	return;


            QString postPathName = postDB->getFile();
            //获取云图视口存储的信息
            QString cloudview = getCloudViewName(_curViewName);
            MechPreData::mPostGUIData* cloudGuidata = getCVM->getGUIDataByViewName(cloudview);
            if (cloudGuidata == nullptr) return;


            QString varName = cloudGuidata->_cloudVarName;
            if ("Velocity" != varName)//判断是否为速度分量
            {
                return;
            }

            //读取速度数据，为流线计算提供速度场：旧格式 mxdb 走 readDisplacementData（内写矢量哈希）；
            //native/vtk/新格式 mxdb 经共用读场 readNodeVectorField 组装后写矢量哈希（键域=连续节点id，同域）
            if (postDB->getDataFlag())
            {
                mIMxdbFile1* mxdbReader = new mIMxdbFile1;
                mxdbReader->readDisplacementData(postPathName, cloudGuidata->_cloudFrame, cloudGuidata->_cloudVarName);
                delete mxdbReader;
            }
            else
            {
                const QHash<int, QVector3D> field = readNodeVectorField(postDB, postName,
                    cloudGuidata->_cloudVarName, cloudGuidata->_cloudFrame, cloudGuidata);
                if (!field.isEmpty())
                    postDB->setVectorDataByFrameID(cloudGuidata->_cloudFrame, field);
            }

            QVector3D p = { 0,0,0 };
            if (getCVM->getPostRenderWidget() == nullptr)return;
            auto postRenderFacade = getCVM->getPostRenderWidget()->getPostRender();   // S5 门面直接替换：shared_ptr 门面
            auto mrdproxy = postRenderFacade->frameProxy(postRenderFacade->currentFrame())->getModelRenderDataProxy();   // F8：换帧/重装载后重取
            if (mrdproxy == nullptr)
                return;
            auto dp = dynamic_cast<mxr::IPostModelRenderDataProxy*>(mrdproxy)->postDataView();
            if (dp == nullptr)
                return;

            std::set<int> num; std::vector<QVector3D>Lp;
            for (const auto& id : ids)
            {
                if (!_allOldPickEleData.count(id))
                    continue;
                auto& ele = _allOldPickEleData[id];

                auto nids = dp->nodeIdsByElement(ele.part_id, ele.data_id.element_id);
                for (int i : nids)
                {
                    num.insert(i);
                }
            }
            for (int i : num)
            {
                QVector3D p0{ 0,0,0 }; { const auto v = dp->nodePosition(i); p0 = { v.x, v.y, v.z }; }
                p += p0;
                Lp.push_back(p0);
            }
            if (num.size() == 0)return;
            p /= (float)(num.size());
            // S5 门面直接替换：旧 getOneFrameRender→getPostStreamLineRender 实例级下钻
            // 通道随门面移除集消亡——流线积分球=G5 StreamlineSphere 单实例改参替换
            // （重复 addObject=改参替换；隐除球+清流线=removeObject(id)）；
            // setIntergrateDirection/drawPostStreamLine 暂无 G 通道等价物，流线
            // 绘制挂起待渲染批次补通道（移交项登记，05 实施记录"新发现调用点"章）
            mxr::StreamlineSphereParams sphere;
            sphere.center = p;
            sphere.radius = 1.f * (Lp[0] - Lp[1]).length();
            sphere.isShow = true;   // 旧 setSphereData 即创建即显示；isShow 承载旧 setIsShowSphere
            postRenderFacade->addObject(sphere);
            // S5 移交项（引擎缺通道）：SL->setIntergrateDirection(StreamLineIntergrateDir::forward);
            // S5 移交项（引擎缺通道）：SL->drawPostStreamLine();

            slData->_hasStreamLineView = true;

            postRend->updatePaint();

        }
        else
        {
            _labelSelectStatu->setText("请选择一个单元（仅一个）");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
            postRend->updatePaint();
        }
    }

    void mPostStreamLinePanel::slot_addPick()
    {
        PostData::mPostGuiData* slData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getStreamLineData(_curViewName);
        if (slData == nullptr)
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
        if (pickDataType != PickDataType::Element)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("请选择单元。"));
            return;
        }

        if (num == 0) return;

        res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
        if (res.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("请选择一个单元。"));
            return;
        }

        // 流线生成只需要一个单元，若一次拾取多个，提示用户并仅保留第一个
        if (res.size() > 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("流线生成仅需一个单元，已自动选择第一个单元。"));
            MeshSelectInfoData first = res.front();
            res.clear();
            res.push_back(first);
        }

        getCVM->clearPick();

        _allOldPickEleData[res[0].data_id.element_id] = res[0];//记录被拾取的单元信息
        
        
        int count = static_cast<int>(res.size() + _pickIds.size());
        if (count == 0)
        {
            _labelSelectStatu->setText("请选择一个单元（仅一个）");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        }
        else
        {
            _labelSelectStatu->setText(QString("已选择对象ID%1").arg(res[0].data_id.element_id));
            _labelSelectStatu->setStyleSheet("color: black; font-weight: bold;");
        }

        for (auto&r : res)
        {
            _pickIds.insert(r.data_id.element_id);
        }

        // 再次确保最终只保留一个单元
        if (_pickIds.size() > 1)
        {
            int firstId = *_pickIds.begin();
            _pickIds.clear();
            _pickIds.insert(firstId);
        }

        std::set<int> ids(_pickIds.begin(), _pickIds.end());

        slData->_streamLineElementType = "拾取";
        slData->_streamLineElementIDs = _pickIds;
    }

    void mPostStreamLinePanel::slot_clearPick()
    {
        PostData::mPostGuiData* slData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getStreamLineData(_curViewName);
        if (slData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        _pickIds.clear();
        _labelSelectStatu->setText("请选择一个单元（仅一个）");
        _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
        getCVM->clearPick();

        slData->_streamLineElementIDs = _pickIds;
        slData->_hasStreamLineView = false;

        PostCommon::removeAllSceneObjects(postRender.get(), mxr::SceneObjectType::VectorGraph);   // S5 G5：deleteVectorGraph→台账家族摘除（内部承担家族渲染删除+清台账）
        postRend->updatePaint();
    }

    void mPostStreamLinePanel::slot_createStreamLine()
    {
        PostData::mPostGuiData* slData = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getStreamLineData(_curViewName);
        if (slData == nullptr)
            return;

        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;
        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // 只有拾取到单元时才生成流线
        if (slData->_streamLineElementType != "拾取" || slData->_streamLineElementIDs.empty())
        {

            slData->_hasStreamLineView = false;
            _labelSelectStatu->setText("请选择一个单元（仅一个）");
            _labelSelectStatu->setStyleSheet("color: red; font-style: italic;");
            return;
        }

        QString postName = postRend->getPostDataName();
        QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
        if (postDB == nullptr)	return;


        QString postPathName = postDB->getFile();
        //获取云图视口存储的信息
        QString cloudview = getCloudViewName(_curViewName);
        MechPreData::mPostGUIData* cloudGuidata = getCVM->getGUIDataByViewName(cloudview);
        if (cloudGuidata == nullptr) return;


        QString varName = cloudGuidata->_cloudVarName;
        if ("Velocity" != varName)//判断是否为速度分量
        {
            return;
        }

        //读速度数据，为流线计算提供速度场：旧格式 mxdb 走 readDisplacementData（内写矢量哈希）；
        //native/vtk/新格式 mxdb 经共用读场 readNodeVectorField 组装后写矢量哈希（键域=连续节点id，同域）
        if (postDB->getDataFlag())
        {
            mIMxdbFile1* mxdbReader = new mIMxdbFile1;
            mxdbReader->readDisplacementData(postPathName, cloudGuidata->_cloudFrame, cloudGuidata->_cloudVarName);
            delete mxdbReader;
        }
        else
        {
            const QHash<int, QVector3D> field = readNodeVectorField(postDB, postName,
                cloudGuidata->_cloudVarName, cloudGuidata->_cloudFrame, cloudGuidata);
            if (!field.isEmpty())
                postDB->setVectorDataByFrameID(cloudGuidata->_cloudFrame, field);
        }

        std::set<int> ids(slData->_streamLineElementIDs.begin(), slData->_streamLineElementIDs.end());

        if (ids.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("无选中的单元，流线生成结束"));
            return;
        }
        // 流线生成只需要一个单元，防止旧数据残留多个
        if (ids.size() > 1)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("流线生成仅需一个单元，已自动选择第一个单元。"));
            int firstId = *ids.begin();
            ids.clear();
            ids.insert(firstId);
            slData->_streamLineElementIDs = ids;
        }

        QVector3D p = { 0,0,0 };
        if (getCVM->getPostRenderWidget() == nullptr)return;
        //auto res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
        auto postRenderFacade = getCVM->getPostRenderWidget()->getPostRender();   // S5 门面直接替换：shared_ptr 门面
        auto mrdproxy = postRenderFacade->frameProxy(postRenderFacade->currentFrame())->getModelRenderDataProxy();   // F8：换帧/重装载后重取
        if (mrdproxy == nullptr)
            return;
        auto dp = dynamic_cast<mxr::IPostModelRenderDataProxy*>(mrdproxy)->postDataView();
        if (dp == nullptr)
            return;

        std::set<int> num; std::vector<QVector3D>Lp;
        for (const auto& id : ids)
        {
            if (!_allOldPickEleData.count(id))
                continue;
            auto& ele = _allOldPickEleData[id];

            auto nids = dp->nodeIdsByElement(ele.part_id, ele.data_id.element_id);
            for (int i : nids)
            {
                num.insert(i);
            }
        }
        for (int i : num)
        {
            QVector3D p0{ 0,0,0 }; { const auto v = dp->nodePosition(i); p0 = { v.x, v.y, v.z }; }
            p += p0;
            Lp.push_back(p0);
        }
        if (num.size() == 0)return;
        p /= (float)(num.size());
        // S5 门面直接替换：实例级流线下钻通道消亡——流线积分球=G5 StreamlineSphere
        // 单实例改参替换（重复 addObject=改参替换；隐除球+清流线=removeObject(id)）；
        // drawPostStreamLine 暂无 G 通道等价物，绘制挂起待渲染批次补通道（移交项）
        mxr::StreamlineSphereParams sphere;
        sphere.center = p;
        sphere.radius = 1.f * (Lp[0] - Lp[1]).length();
        sphere.isShow = true;   // 旧 setSphereData 即创建即显示；isShow 承载旧 setIsShowSphere
        postRenderFacade->addObject(sphere);
        // S5 移交项（引擎缺通道）：SL->setIntergrateDirection(StreamLineIntergrateDir::forward);
        // S5 移交项（引擎缺通道）：SL->drawPostStreamLine();

        slData->_hasStreamLineView = true;

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, QString("流线生成成功"));
       
    }

    void mPostStreamLinePanel::hideEvent(QHideEvent* event)
    {
        if (!getCVM) return;
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        // S5 移交项（引擎缺通道）：旧 getOneFrameRender→getPostStreamLineRender 实例级
        // 通道随门面直接替换消亡，deletePostStreamLine 暂无 G 通道等价物——流线清理
        // 挂起待渲染批次补通道（隐藏面板时残留流线随会话换绑/帧重建自然回收）
        postRend->updatePaint();
    }
}
