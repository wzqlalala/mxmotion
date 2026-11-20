#include "mPostAnimation.h"
#include <cmath> //【OCC摘除·S10/P0-1】sin/M_PI 显式自含（原经头网传递）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <QFormLayout>
#include <QCoreApplication>
#include <QDir>
#include <QApplication>
#include <QCollator>
#include <QElapsedTimer>
#include <iostream>
#include <fstream>
#include <QThreadPool>
#include "mCommonFunctions.h"   // unify-addobject-arch：C4 仅窗值域段经 ColorLegend 台账下发（PostCommon 助手）

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

#include "mDockLayoutManager.h"
#include "mPostInfoHub.h"
#include "mPostValueProvider.h"
#include "mViewTreeFunction.h"

//MxFunctions
#include "mGlobalSignals.h"
#include "mContainerOperation.h"
#include "mAppConfiguration.h"

//BasicData
#include "mPhysicalFieldEnum.h"

//Post
#include "mPostDataBase.h"
#include "mPost3DResult.h"
#include "mIMxdbFile1.h"
// 须在 mIMxdbFile1.h 之后：mIONativeFile.h 的裸 include "mIOEnum.h" 靠 include 栈
// （mIMxdbFile1.h 所在的 PostIOFile 目录）命中 MIOFile 版枚举，先于本行会被 Common 版遮蔽
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "CameraManager.h"
#include "mPostParseMesh.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
// S5 门面直接替换：REBUILD_PostRenderSession 转私有实现宿主（mxrPrivate），公开会话头
// 已删除——会话访问唯一通道=REBUILD_PostRend::getPostRender()（值类型在 REBUILD_PostRenderTypes.h，
// 已随公开门面头带入）
#include "mPostRigidWallData.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mxr/view/mPostFrameText.h"
#include "mPostOneFrameRendData.h"
 
#include "mPostRigidWallData.h"

//窗口
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MIOFile;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;

namespace
{
	struct PerfTimer
	{
		QElapsedTimer timer;
		QByteArray name;
		PerfTimer(const char* n) : name(n) { timer.start(); qDebug() << "[Enter]" << name.constData(); }
		~PerfTimer() { qDebug() << "[Exit]" << name.constData() << "took" << timer.elapsed() << "ms"; }
	};

	// mxdb 新旧分支读数据失败的历史行为是静默（帧数据全空、图例 ±FLT_MAX 无从排查），
	// 统一落输出窗口；文案口径与 native 分支一致。mxdb 侧失败不中断动画（继续后续帧）
	void reportAniReadFailure(int frame, const QString& varstr, const QString& compostr,
		const QString& postPathName)
	{
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
		mGlobalSignals::getInstance()->outputMessageSig(2,
			isChinese ? QStringLiteral("动画生成读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
					  : QStringLiteral("Failed to read result component for animation: frame %1, variable %2, component %3 (%4); see the debug log for details")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
			true);
	}
}

namespace PostWindow
{
	mPostAnimationPanel::mPostAnimationPanel(QWidget* parent) : mPanelBase(parent)
	{
		_curPostRend = nullptr;

		_mainLayout = new QVBoxLayout(this);
		this->setStyleSheet(
			"mPostAnimationPanel { background-color: white; }"

			"QComboBox QAbstractItemView {"
			"    background-color: white;"     /* 白底   */
			"    color: black;"                /* 黑字   */
			"    selection-background-color: #3399ff;" /* 悬停高亮，可自定 */
			"    selection-color: black;"      /* 悬停时文字保持黑色 */
			"}");

		/********************帧控制********************/
		FoldSection* frameFold = new FoldSection("帧控制", this);
		_spinCurFrame = new QSpinBox();
		_lineEditCurTime = new QLineEdit();
		_spinStartFrame = new QSpinBox();
		_lineEditStartTime = new QLineEdit();
		_spinEndFrame = new QSpinBox();
		_lineEditEndTime = new QLineEdit();
		_spinIntervalFrame = new QSpinBox();
		_lineEditIntervalTime = new QLineEdit();

		//当前帧/时间
		frameFold->addRow(new PanelLabel("当前帧/时间"), _spinCurFrame, _lineEditCurTime);
		_spinCurFrame->setValue(1);
		_spinCurFrame->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinCurFrame->setFixedWidth(80);

		//起始帧/时间
		frameFold->addRow(new PanelLabel("起始帧/时间"), _spinStartFrame, _lineEditStartTime);
		_spinStartFrame->setValue(1);
		_spinStartFrame->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinStartFrame->setFixedWidth(80);

		//结束帧/时间
		frameFold->addRow(new PanelLabel("结束帧/时间"), _spinEndFrame, _lineEditEndTime);
		_spinEndFrame->setValue(1);
		_spinEndFrame->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinEndFrame->setFixedWidth(80);

		//帧/时间间隔
		frameFold->addRow(new PanelLabel("帧/时间间隔"), _spinIntervalFrame, _lineEditIntervalTime);
		_spinIntervalFrame->setValue(1);
		_spinIntervalFrame->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinIntervalFrame->setFixedWidth(80);
		_lineEditIntervalTime->setText("0");

		frameFold->refreshLayout();
		m_foldWidgets.append(frameFold);
		_mainLayout->addWidget(frameFold);

		/********************动画信息********************/
		FoldSection* infoFold = new FoldSection("动画信息", this);
		_spinFrameNum = new QSpinBox();
		_spinFrameNum->setReadOnly(true);
		_spinFrameNum->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_spinFrameNum->setAlignment(Qt::AlignCenter);

		//动画帧数
		infoFold->addRow(new PanelLabel("动画帧数"), _spinFrameNum);

		infoFold->refreshLayout();
		m_foldWidgets.append(infoFold);
		_mainLayout->addWidget(infoFold);
		_mainLayout->addStretch();

		//connect
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::createAnimationViewport, this, &mPostAnimationPanel::slot_createAnimationViewport);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationPanelSigManger, this, &mPostAnimationPanel::slot_aniTimer);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationInfoGUI, this, &mPostAnimationPanel::updateAnimationGUI);

		//起始帧
		connect(_spinStartFrame, SIGNAL(valueChanged(int)), this, SLOT(slot_setStartFrame(int)));
		connect(_lineEditStartTime, &QLineEdit::editingFinished, this, &mPostAnimationPanel::slot_setStartFrameTime);

		//结束帧
		connect(_spinEndFrame, SIGNAL(valueChanged(int)), this, SLOT(slot_setEndFrame(int)));
		connect(_lineEditEndTime, &QLineEdit::editingFinished, this, &mPostAnimationPanel::slot_setEndFrameTime);

		//当前帧
		connect(_spinCurFrame, SIGNAL(valueChanged(int)), this, SLOT(slot_setCurrentFrame(int)));
		connect(_lineEditCurTime, &QLineEdit::editingFinished, this, &mPostAnimationPanel::slot_setCurrentFrameTime);

		//帧间隔
		connect(_spinIntervalFrame, SIGNAL(valueChanged(int)), this, SLOT(slot_setFrameInterval(int)));
		connect(_lineEditIntervalTime, &QLineEdit::editingFinished, this, &mPostAnimationPanel::slot_setFrameIntervalTime);

		//【FR-07】旧 GUI 定时器直驱链（定时器创建+连接旧 tick 槽）已删除：全文件无
		//start 调用=永不执行路径（unify-anim-playback 死代码清理）

	}

	void mPostAnimationPanel::newAniPanel(int caseid)
	{
		QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseid);
		if (postDB == nullptr) return;

		MechCommon::mMechGlobalSignals::getInstance()->createAnimationSig(1);

		//启用动画菜单栏
		MechCommon::mMechGlobalSignals::getInstance()->setAnimationMenuStatus(true);

		//生成动画
		bool res = autoGenerateAnimation();
		//生成失败（如云图停在默认伪变量 SolidColor）：回滚上方先行下发的菜单启用，
		//避免"开始"可点却无动画可播（未生成动画时控件态以按钮级全禁收口）
		if (!res)
			MechCommon::mMechGlobalSignals::getInstance()->setAnimationMenuStatus(false);
	}

	void mPostAnimationPanel::slot_createAnimationViewport(QString viewport, int type)
	{
		//FR-01/FR-05 单视口形态：语义改为"进入动画模式（同视口）"。原第二视口创建链
		//（newViewport 派生动画视口 + 云图→动画图例最值继承 + FitView）随拆除删除——
		//动画与云图共用同一视口会话（同一 postRend、同一帧表），此处的 viewport
		//已是云图视口名（发射点 slot_updateAnimationTree 直传 resViewName）
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(viewport);
		if (postRend == nullptr) return;
		mPostGUIData* guidata = getCVM->getGUIDataByViewName(viewport);
		if (guidata == nullptr) return;

		//校验通过才落面板状态（与原链失败即早退、不污染 _curViewName 的口径一致）
		_curViewName = viewport;
		_curPostRend = postRend;   //单一事实源：即云图视口 postRend

		//根据动画生成类型设置参数页显示
		if (type == 1)
		{
			guidata->_generateAnimate = true;
		}
		else if (type == 2)
		{
			guidata->_generateAnimate = false;
		}

		//恢复当前视口的失效单元隐藏开关状态，保持与持久化状态一致
		_isHideFailedElement = guidata->_aniHideFailedElement;

		//Q4：模式切换相机不动——原 FitView 属"切到第二视口"语义，随链删除；
		//首次载入模型的 FitView 属另一链，不受影响
		_curPostRend->updatePaint();

		MechCommon::mMechGlobalSignals::getInstance()->setAnimationOutputViewport(_curViewName);
	}

		void mPostAnimationPanel::applyHideFailedElementForFrame(PostData::mPostDataBase* postDB,
			mxr::REBUILD_PostRend* postRend,
			mxr::PostFrameDataProxy* frameDataProxy, int frame, const std::set<int>& lastLoseEles)
		{
			if (!_isHideFailedElement || postDB == nullptr || postRend == nullptr || frameDataProxy == nullptr)
				return;

			PerfTimer _t("mPostAnimationPanel::applyHideFailedElementForFrame");
			bool hasFailed = hideFailedElementsForFrameWithCache(postDB, postRend, frame, lastLoseEles);
			if (hasFailed)
			{
				auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameDataProxy->getModelRenderDataProxy());
				if (model_proxy != nullptr)
					model_proxy->RebuildModelData();
			}
			qDebug() << "[Perf] applyHideFailedElementForFrame frame=" << frame;
		}

	bool mPostAnimationPanel::autoGenerateAnimation()
	{
		PerfTimer _t("mPostAnimationPanel::autoGenerateAnimation");
		//获取当前激活视图
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return false;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return false;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return false;

		QList<int> frameids = postDB->getFrameIds();

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return false;

		//FR-01/FR-05 单视口形态：动画与云图共用同一视口/同一份GUIData——原
		//getCloudViewName 反查云图 GUIData（双名消费）已核销；cloudGuidata 保留为
		//guidata 别名（同一对象），下游云图状态消费点零改动
		mPostGUIData* cloudGuidata = guidata;

		//SolidColor为伪变量，结果文件中不存在，禁止生成动画
		if (guidata->_cloudVarName == QString("SolidColor"))
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("动画生成失败！当前云图变量为纯色显示模式(SolidColor)，请先选择云图变量后再生成动画！"));
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("Animation generation failed! The current contour variable is SolidColor. Please select a result variable first!"));
			}
			return false;
		}

		//总帧数
		int frameamount = 0;

		_lineEditCurTime->show();
		_lineEditStartTime->show();
		_lineEditEndTime->show();
		_lineEditIntervalTime->show();

		//动画类型
		QString anitype = "瞬态动画";
		guidata->_aniType = anitype;
        guidata->_aniControl = "帧";
		// B1b(M2)：GUI 不再 setPostMode(Animation)——模式由 generateAnimation 内部置
		// （自动进入动画形态）；外壳转译入口保留给无会话回退路径
		_animationType = Animation;

		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig("动画生成中！");
		}
		else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
		{
			mGlobalSignals::getInstance()->outputMessageSig("Animation being generated！");
		}

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

		_animationRendData = new PostData::mPostAnimationRendData();
		QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

		//清空容器
		_frameIndexes.clear();
		const int nframe = postDB->getTotalFrameCount();
		mPostInfoHub::getInstance()->slot_videoFrameRange(1, nframe);
		guidata->_aniShotFrame = 1;
		guidata->_aniVideoStartFrame = 1;
		guidata->_aniVideoEndFrame = nframe;

		//删除动画
		this->slot_deleteAnimate();

		for (int i = 0; i < nframe; ++i)
		{
			//存储索引和帧号
			_frameIndexes.append(i + 1);

			double time = postDB->getFrameTime(i + 1);
			_frameTimes[i + 1] = time;
		}

		// 总帧数
		frameamount = _frameIndexes.size();
		_endFrameId = frameamount;

		// S5 门面直接替换：绑定改 attachModel（装载工厂随绑定注入，GUI 不再手造代理入壳）；
		// 本地探测代理保留=首帧可读性前置守卫（语义同旧 frameData 空判），不入渲染壳
		mxr::PostFrameDataProxy* probeProxy = mxr::createFrameDataProxy(cloudGuidata->_cloudFrame, postRend->getViewportName(), postDB);
		if (probeProxy == nullptr)
		{
			// R25 修复（审计逻辑-M4a，既有缺陷顺手修）：上方已置全局等待光标，
			// 此早退原样 return 泄漏光标（全局等待态卡死）——补恢复
			QApplication::restoreOverrideCursor();
			return false;
		}
		// C5 泄漏收口：探针仅作首帧可读性前置守卫（空判后不再使用、不入壳不留存），
		// 即查即释——天然覆盖本函数后续全部早退点与函数尾
		delete probeProxy;
		probeProxy = nullptr;
        // ========== 新增：读取磨平状态 ==========
        bool needPlanish = cloudGuidata->_cloudPlanish;

		postRender->attachModel(postDB, postRend->getViewportName());
		postRender->seekFrame(cloudGuidata->_cloudFrame);

		mGlobalSignals::getInstance()->setProgressBarValue(20);

		//图例范围三模式（HyperView Fixed/Dynamic Scale 对应）：手动=guidata 锁定值域、
		//全程=全帧统一值域（经云图面板缓存）——两者各帧色表统一写稳定值域，动画期间
		//图例稳定、颜色跨帧可比；逐帧=各帧实际最值（allRangeValid=false 走回退分支）
		QPair<double, double> allRange{};
		bool allRangeValid = false;
		if (cloudGuidata->_legendRange == QString("手动"))
		{
			if (cloudGuidata->_cloudMaxValue > cloudGuidata->_cloudMinValue)
			{
				allRange = qMakePair(double(cloudGuidata->_cloudMinValue), double(cloudGuidata->_cloudMaxValue));
				allRangeValid = true;
			}
		}
		else if (cloudGuidata->_legendRange == QString("全程"))
		{
			if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
				allRangeValid = cloudPanel->allFrameRange(postRend->getPostDataName(), postDB,
					cloudGuidata->_cloudVarName, cloudGuidata->_cloudCompo,
					cloudGuidata->_cloudLayerName, cloudGuidata->_cloudComplex, allRange);
		}

		for (int frame : _frameIndexes)
		{
			QElapsedTimer frameTimer;
			frameTimer.start();

			//根据界面的数据来读取数据并刷新
			QString postPathName = QString(postDB->getFile());

			QString varstr = cloudGuidata->_cloudVarName;
			QString compostr = cloudGuidata->_cloudCompo;
			QString layerstr = cloudGuidata->_cloudLayerName;
			QString complex = cloudGuidata->_cloudComplex;

            // 新增：坐标系统
            QString coordinateName;
            QVector<double> userCoord{};

            //统一取值 provider（阶段2 二批）：四格式发布式分支收敛；失败策略保留历史
            //契约（native/vtk 失败中止动画并恢复光标，mxdb 失败报告继续）；
            //磨平走 needPlanish、ElementNodalForce 特判在适配器内
            if (IPostValueProvider* provider = valueProviderFor(postDB))
            {
                ValueRequest req;
                req.frame = frame;
                req.var = varstr;
                req.compo = compostr;
                req.layer = layerstr;
                req.complexMode = complex;
                req.coordinateName = coordinateName;
                req.userCoord = userCoord;
                req.flattenStress = needPlanish;
                const bool ok = provider->publish(req);
                const bool abortOnFail =
                    provider->sourceKind() == IPostValueProvider::SourceKind::Native
                    || provider->sourceKind() == IPostValueProvider::SourceKind::Vtk;
                delete provider;
                if (!ok)
                {
                    if (abortOnFail)
                    {
                        const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
                        mGlobalSignals::getInstance()->outputMessageSig(2,
                            isChinese ? QStringLiteral("动画生成读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
                                            .arg(frame).arg(varstr).arg(compostr).arg(postPathName)
                                      : QStringLiteral("Failed to read result component for animation: frame %1, variable %2, component %3 (%4); see the debug log for details")
                                            .arg(frame).arg(varstr).arg(compostr).arg(postPathName),
                            true);
                        QApplication::restoreOverrideCursor();
                        return false;
                    }
                    reportAniReadFailure(frame, varstr, compostr, postPathName);
                }
            }
            else
            {
                reportAniReadFailure(frame, varstr, compostr, postPathName);
            }

            mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(frame, postRend->getViewportName(), postDB);
			// S5 门面直接替换：代理不再入壳登记（装载工厂随 attachModel 托管）——
			// 本地代理仅供读数后置处理（updateValue/最值/刚体墙/失效单元）

			auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frame_data_proxy->getModelRenderDataProxy());
			if (model_proxy != nullptr)
				model_proxy->updateValue();

			// 设置颜色表每帧值域：全帧统一值域可得时统一写全局（HyperView 语义——
			// 动画期间图例稳定、颜色跨帧可比），否则逐帧各自实际最值
			if (allRangeValid)
			{
			{
			    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = frame;
			    _lg.valueMin = float(allRange.first);
			    _lg.valueMax = float(allRange.second);
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			}
			else
			{
				mxr::IPostRendQueryDataProxy* frame_query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frame_data_proxy->getRendQueryDataProxy());
				if (frame_query_proxy != nullptr)
				{
					float frameMin = frame_query_proxy->getCurrentMinData();
					float frameMax = frame_query_proxy->getCurrentMaxData();
				{
				    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
				    mxr::ColorLegendParams _lg;
				    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				    _lg.targetFrame = frame;
				    _lg.valueMin = frameMin;
				    _lg.valueMax = frameMax;
				    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
				}
				}
				else
				{
					// 如果获取失败，使用云图最值作为备用
				{
				    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
				    mxr::ColorLegendParams _lg;
				    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				    _lg.targetFrame = frame;
				    _lg.valueMin = cloudGuidata->_cloudMinValue;
				    _lg.valueMax = cloudGuidata->_cloudMaxValue;
				    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
				}
				}
			}

			QPointer <mPostModelData> modeldb = postDB->getModelData(frame);
			if (modeldb == nullptr)
			{
				delete frame_data_proxy;   // C5 泄漏收口：本帧本地代理树迭代内释放（continue 早退分支）
				continue;
			}

			mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frame_data_proxy->getRendQueryDataProxy());
			PostData::PostRigidWallType type;
			if (query_proxy != nullptr)
			{
				for (auto it : postDB->getAllRigidWallDatas())
				{
					double pos[6];
					QVector3D startPos = it->getRigidWallStartPos();
					pos[0] = startPos.x();
					pos[1] = startPos.y();
					pos[2] = startPos.z();

					QVector3D endPos = it->getRigidWallEndPos();
					pos[3] = endPos.x();
					pos[4] = endPos.y();
					pos[5] = endPos.z();

					PostData::PostRigidWallType type = it->getRigidWallType();
					switch (type)
					{
					case PostData::PostRigidWallType::InfinitelyLargePlaneRigidWall:
					{
						PostData::mPostInfinitelyLargePlaneRigidWall* rigidWall = new PostData::mPostInfinitelyLargePlaneRigidWall(it->getRigidWallID(), pos);
						QString name = it->getRigidWallName();
						QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
						rigidWall->updatePos(p);
						query_proxy->appendRigidWallData(rigidWall);
					}
					break;
					case PostData::PostRigidWallType::LimitedLargePlanesRigidWall:
					{
						PostData::mPostLimitedLargePlaneRigidWall* rigidWall = new PostData::mPostLimitedLargePlaneRigidWall(it->getRigidWallID(), pos);
						auto wall = dynamic_cast<PostData::mPostLimitedLargePlaneRigidWall*>(it);
						memcpy(rigidWall->_lineEndVector, wall->_lineEndVector, 3 * sizeof(double));
						rigidWall->_L = wall->_L;
						rigidWall->_M = wall->_M;

						QString name = it->getRigidWallName();
						QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
						rigidWall->updatePos(p);
						query_proxy->appendRigidWallData(rigidWall);
					}
					break;
					case PostData::PostRigidWallType::RectangularRigidWall:
					{
						PostData::mPostRectangularRigidWall* rigidWall = new PostData::mPostRectangularRigidWall(it->getRigidWallID(), pos);
						auto wall = dynamic_cast<PostData::mPostRectangularRigidWall*>(it);
						memcpy(rigidWall->_lineEndVector, wall->_lineEndVector, 3 * sizeof(double));
						rigidWall->_L = wall->_L;
						rigidWall->_M = wall->_M;
						rigidWall->_P = wall->_P;

						QString name = it->getRigidWallName();
						QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
						rigidWall->updatePos(p);
						query_proxy->appendRigidWallData(rigidWall);
					}
					break;
					case PostData::PostRigidWallType::CylindricalRigidWall:
					{
						PostData::mPostCylindricalRigidWall* rigidWall = new PostData::mPostCylindricalRigidWall(it->getRigidWallID(), pos);
						auto wall = dynamic_cast<PostData::mPostCylindricalRigidWall*>(it);
						rigidWall->_radius = wall->_radius;
						rigidWall->_length = wall->_length;

						QString name = it->getRigidWallName();
						QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
						rigidWall->updatePos(p);
						query_proxy->appendRigidWallData(rigidWall);
					}
					break;
					case PostData::PostRigidWallType::SphericalRigidWall:
					{
						PostData::mPostSphericalRigidWall* rigidWall = new PostData::mPostSphericalRigidWall(it->getRigidWallID(), pos);
						auto wall = dynamic_cast<PostData::mPostSphericalRigidWall*>(it);
						rigidWall->_radius = wall->_radius;

						QString name = it->getRigidWallName();
						QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
						rigidWall->updatePos(p);
						query_proxy->appendRigidWallData(rigidWall);
					}
					break;
					default:
						break;
					}
				}
			}
			// 处理当前帧失效单元：生成阶段以“全显示”为基准，仅隐藏当前帧失效单元。
			// 每帧独立处理，避免播放/切帧时基于上一帧增量处理导致基准错乱。
			// 通过面缓存复用减少 ParseMesh 重复调用。
			std::set<int> lastLoseEles;
			if (frame > 1)
			{
				QPointer<mPostModelData> lastModelData = postDB->getModelData(frame - 1);
				if (lastModelData != nullptr)
					lastLoseEles = lastModelData->getLoseElems();
			}
			applyHideFailedElementForFrame(postDB, postRend, frame_data_proxy, frame, lastLoseEles);
			delete frame_data_proxy;   // C5 泄漏收口：每帧本地代理树（4 子代理聚合）迭代末尾统一释放
			setProgressBarValue(frame , _frameIndexes.size());
		}
		// S5 门面直接替换（B1b(M2/M9) 口径延续）：GUI 读数+造代理链保留（装载器工厂
		// 仅从 modeldb 造代理、不挂文件读数链，读数仍由 GUI 侧完成，见实施记录
		// 偏离注记）；登记/绑定/渐进实例构建/播放语义全在门面：attachModel 幂等换绑
		// （工厂注入，帧表登记随工厂托管）→ generateAnimation 五约束（防并发闸/
		// 首帧同步构建/参数快照/先停后生成/失败保留旧绑定）。
		// 注：registerAnimationTable/setRendAnimationFrame 为引擎暂留面/移除集方法
		//（门面已无此二者）——代理表登记随工厂托管消亡；门面恒在，legacy 回退分支退役
		{
			mxr::AnimationParams params;
			params.type = mxr::AnimationParams::Type::Transient;
			params.startFrame = 1;
			params.endFrame = frameamount;
			params.frameRate = _rateFrame;
			params.loop = _animationLoopPlay;
			if (postRender->generateAnimation(params))
			{
				// M14：调用门面后回填 guidata 镜像（值域唯一写者=会话）
				const auto last = postRender->lastAnimationParams();
				guidata->_aniFrameRate = last.frameRate;
				guidata->_aniLoopPlay = last.loop;
				guidata->_aniStartFrame = last.startFrame;
				guidata->_aniEndFrame = last.endFrame;
			}
		}
		//设置帧数据后应用云图点大小（无帧数据时渲染器内部单帧对象未创建，不可调用）
		if (postRender->frameProxy(postRender->currentFrame()) != nullptr)
			postRender->setPointSize(cloudGuidata->_pointSize);

		// 根据变量类型设置节点/单元着色，与云图效果保持一致
		{
			if (needPlanish) {
                postRender->updateAllModelOperate(ShowNodeColor);
            }
            else {
                // 根据变量类型判断
                // R25 修复（审计逻辑-M4b，既有缺陷顺手修）：零帧结果库时 _frameIndexes
                // 为空表，.first() 越界 UB（frameamount>0 守卫在收尾处，晚于此处）——
                // 空表跳过着色判定，交由收尾 frameamount==0 失败分支报错
                QPointer<mPostModelData> firstModelData;
                if (!_frameIndexes.isEmpty())
                    firstModelData = postDB->getModelData(_frameIndexes.first());
                if (firstModelData != nullptr) {
                    QList<PostVariableInfo> varInfos = firstModelData->_frameInfo._frameVars;
                    for (auto it : varInfos) {
                        if (it._varName == cloudGuidata->_cloudVarName) {
                            if (it._varType == 0) {
                                postRender->updateAllModelOperate(ShowNodeColor);
                            }
                            else if (it._varType == 1) {
                                postRender->updateAllModelOperate(ShowElementColor);
                            }
                            break;
                        }
                    }
                }
            }
		}

		// 刷新颜色映射、最值并着色所有部件
		postRender->updateAllModelOperate(UpdateVariableValue);
		postRender->updateAllModelOperate(UpdateMinMax);
		postRender->updateAllModelOperate(ColorAllPartOperate);

		QApplication::restoreOverrideCursor();

		postRend->FitView();
		postRend->updatePaint();

		//由于存在某些帧可能没有当前变量的情况，因此生成完毕后需要更新总帧数
		if (frameamount > 0)
		{
			mPostInfoHub::getInstance()->slot_videoEndFrame(frameamount);
			_spinCurFrame->setRange(1, frameamount);
			_spinCurFrame->blockSignals(true);
			_spinCurFrame->setValue(1);
			_spinCurFrame->blockSignals(false);
			_spinStartFrame->setRange(1, frameamount);
			_spinStartFrame->blockSignals(true);
			_spinStartFrame->setValue(1);
			_spinStartFrame->blockSignals(false);
			_spinEndFrame->setRange(1, frameamount);
			_spinEndFrame->blockSignals(true);
			_spinEndFrame->setValue(frameamount);
			_spinEndFrame->blockSignals(false);
			_spinIntervalFrame->setRange(1, frameamount);
			_spinFrameNum->setRange(1, frameamount);

			//球单元显示
			postRender->updateAllModelOperate(ShowDiscreteBall);

			//显示动画
			this->slot_setStartFrame(1);
			_startFrameId = 1;
			this->slot_setEndFrame(frameamount);
			_endFrameId = frameamount;
			this->slot_setCurrentFrame(1);
			_currentFrameId = 1;
		}
		else
		{
			//输出信息
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->setProgressBarValue(100);
				mGlobalSignals::getInstance()->outputMessageSig("动画生成失败！");
				return false;
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				mGlobalSignals::getInstance()->setProgressBarValue(100);
				mGlobalSignals::getInstance()->outputMessageSig("Animation generation failure！");
				return false;
			}
		}

		mGlobalSignals::getInstance()->setProgressBarValue(95);

		//显示云图信息
		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
		if (guidata->_cloudLayerName.isEmpty())
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s", cloudGuidata->_cloudVarName.toStdString().c_str(), cloudGuidata->_cloudCompo.toStdString().c_str());
		}
		else
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s:%s", cloudGuidata->_cloudVarName.toStdString().c_str(), cloudGuidata->_cloudCompo.toStdString().c_str(), cloudGuidata->_cloudLayerName.toStdString().c_str());
		}

		guidata->_generateAnimate = true;
		guidata->_aniFrameAmount = frameamount;

		//播放时设置不可编辑
		slot_setStatus(true);
		//设置一键生成帧速率及间隔
		_intervalFrame = 1;
		_rateFrame = 10;
		// B1b(M4)：播放 tick 归会话自有定时器（真循环回绕修 P10），GUI _rateFrame
		// 定时器链退化为参数镜像（interval=1000/rate 换算在会话内，帧率经
		// setFrameRate 下发）；S5 门面直接替换后门面恒在，GUI 定时器回退分支退役
		postRender->setFrameRate(_rateFrame);
		postRender->setLoop(_animationLoopPlay);
		postRender->setPlayRange(1, frameamount);
		//【审计 B-03】预生成播放入口守卫（slot_PlayAnimation :2337 同款先例）：浮层引擎档
		//在播→先停浮层（pausePlayback→toggled(false)→宿主引擎停机分支会话 pause），
		//防生成尾自动播放抢占会话后被宿主 updateAnimationMenuGUI 旁听链反杀（幽灵互斥）
		if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
			cloudPanel->stopRealtimeEngineIfActive();
		postRender->play();
		_viewPlayStatus[_curViewName] = true;
		guidata->_aniIsStart = true;

		//刷新Ribbon工具条界面，使播放时控件禁用状态生效
		MechCommon::mMechGlobalSignals::getInstance()->updateAnimationMenuGUI(_curViewName);

		//【单视口形态收尾同步】原"云图→动画"两步视口激活随第二视口拆除失效（同视口
		//自激活幂等早退、不发 currentViewportChanged，也无第二视口可跳），其承载的两
		//项联动改为直接派发：① editAnimation——信息坞挂载播放控制参数页、面板
		//setCurViewName 与参数页刷新（同视口化后生成全程 _generateAnimate 即终态，
		//收尾直派所见即最终态；2026-11-05 视口绑定收敛后此即"首次生成后参数栏停留
		//生成页"的收尾修复：editAnimation 按 _generateAnimate=1 挂播放控制页）；
		//② refreshPostMenuEnableSig——Ribbon"动画"页启停重算（原依赖激活链
		//currentViewportChanged 触发，单视口全程无切换；theirs 新规则下重算结果
		//幂等不变，保留使页启停规则再演进时收尾仍被通知）。
		//仍在播放态写回之后执行（[AniGray] 修复语义保持）
		mPostInfoHub::getInstance()->editAnimation(_curViewName);
		MechCommon::mMechGlobalSignals::getInstance()->refreshPostMenuEnableSig();


		//输出信息
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->setProgressBarValue(100);
			mGlobalSignals::getInstance()->outputMessageSig("动画生成成功！");
		}
		else
		{
			mGlobalSignals::getInstance()->setProgressBarValue(100);
			mGlobalSignals::getInstance()->outputMessageSig("Animation generated successfully!");;
		}

        // 接上原有动画截屏逻辑
        // 仅响应来源为本动画视口的帧变化信号，避免切换节点后其它视口的信号把帧数据写进当前节点；
        // 重新生成同一动画时先断开旧连接，防止重复触发
        QString frameViewName = postRend->getViewportName();
        auto connIt = _frameChangedConns.find(frameViewName);
        if (connIt != _frameChangedConns.end() && connIt.value())
        {
            disconnect(connIt.value());
        }
        // S5 门面直接替换：帧事件单通道接门面 frameChanged(frameId, FrameChangeSource)。
        // B1b(R17/H1) 过渡双驱动（外壳 updateAnimationPanelSig(int) 别名信号作播放期
        // 每 tick 驱动源）随旧信号删除自然退役——别名连接与 _aliasTickConns 一并拆除。
        // 【语义披露/引擎观察项】渲染库 playTick 绑定翻转路径仍零发射 frameChanged
        //（发射点仅 seekFrame/step），播放期逐 tick 总线镜像依赖渐进装载 frameReady
        // 与 seek/step 事件；播放逐 tick 驱动缺口待渲染库移交项（playTick 补发
        // PlayAdvance frameChanged）落地后回归单通道全覆盖（05 实施记录登记）
        // 【移交项闭环 2026-11-13】渲染库已落地停机点补发（自然停机 emit 同帧也发，
        // 含 BUG-03 同帧口径修正），停机同步缺口关闭；播放期逐 tick 镜像由既有通道
        // （seek/step/frameReady+停机补发）覆盖，本移交项销项。
        _frameChangedConns[frameViewName] = connect(
            postRend->getPostRender().get(), &mxr::REBUILD_PostRender::frameChanged, this,
            [this, frameViewName](int frame, mxr::FrameChangeSource source)
            {
                Q_UNUSED(source);
                if (frameViewName != _curViewName)
                    return;
                // M11：动画帧事件→动画面板刷新总线（slot_aniTimer 镜像入口）
                emit MechCommon::mMechGlobalSignals::getInstance()->updateAnimationPanelSigManger(frame);
                // 非循环播放自然结束回写（会话播放器至尾帧自动 Stopped）
                REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(_curViewName);
                if (rend != nullptr && rend->getPostRender()->playbackState() != mxr::PlaybackState::Playing
                    && _viewPlayStatus.value(_curViewName, false))
                {
                    slot_PlayAnimation(false);
                }
            },
            Qt::DirectConnection);

		//等值面随动画自动恢复（HyperView 语义：逐帧跟随变形提取）：动画生成链
		//setRendCurrentFrameData 重建渲染器已销毁源视口等值面，生成完成后按
		//等值面面板存储态（阈值/生成形式）立即重建——广播落静态帧+各动画帧；
		//该视口未生成过等值面则无操作
		mPostInfoHub::getInstance()->restoreIsosurfaceAfterAnimation(_curViewName);

		return true;
	}

	bool mPostAnimationPanel::customGenerateAnimation(QString aniType, QString varType, QString start, QString end)
	{
		PerfTimer _t("mPostAnimationPanel::customGenerateAnimation");
		//启用动画菜单栏
		MechCommon::mMechGlobalSignals::getInstance()->setAnimationMenuStatus(true);

		//显示动画控制参数页
		mPostGUIData* guidata = getCVM->getGUIDataByViewName(_curViewName);
		if (guidata == nullptr) return false;
			guidata->_generateAnimate = true;
		mPostInfoHub::getInstance()->editAnimation(_curViewName);

		//获取当前激活视图
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return false;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return false;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return false;

		//FR-01/FR-05 单视口形态：动画与云图共用同一视口/同一份GUIData——原
		//getCloudViewName 反查云图 GUIData（双名消费）已核销；cloudGuidata 保留为
		//guidata 别名（同一对象），下游云图状态消费点零改动
		mPostGUIData* cloudGuidata = guidata;

		//SolidColor为伪变量，结果文件中不存在，禁止生成动画
		if (guidata->_cloudVarName == QString("SolidColor"))
		{
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("动画生成失败！当前云图变量为纯色显示模式(SolidColor)，请先选择云图变量后再生成动画！"));
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("Animation generation failed! The current contour variable is SolidColor. Please select a result variable first!"));
			}
			return false;
		}

		// S5 门面直接替换：绑定改 attachModel+seekFrame（同链一口径；本地探测代理
		// 保留=首帧可读性前置守卫）
		mxr::PostFrameDataProxy* probeProxy = mxr::createFrameDataProxy(guidata->_cloudFrame, postRend->getViewportName(), postDB);
		if (probeProxy == nullptr) return false;
		delete probeProxy;   // C5 泄漏收口：探针即查即释（空判后不再使用），覆盖后续全部早退点与函数尾
		probeProxy = nullptr;
		postRender->attachModel(postDB, postRend->getViewportName());
		postRender->seekFrame(guidata->_cloudFrame);

		QList<int> frameids = postDB->getFrameIds();
		int frameSize = frameids.size();

		//总帧数
		int frameamount = 0;

		//获取动画类型
		guidata->_aniType = aniType;

		if (guidata->_aniType == QString("瞬态动画"))
		{
			_lineEditCurTime->show();
			_lineEditStartTime->show();
			_lineEditEndTime->show();
			_lineEditIntervalTime->show();
		}
		else
		{
			_lineEditCurTime->hide();
			_lineEditStartTime->hide();
			_lineEditEndTime->hide();
			_lineEditIntervalTime->hide();
		}

		//专门为动画图设置后处理模式（S5：PostMode 五值→DisplayMode 三值映射，
		//Animation→TransientAnimation；Linear/Sin→ParametricAnimation；幂等不触发播放器）
		if (guidata->_aniType == "瞬态动画" || guidata->_aniType == "Transient")
		{
			postRend->getPostRender()->setDisplayMode(mxr::DisplayMode::TransientAnimation);
			_animationType = Animation;
		}
		else if (guidata->_aniType == "线性动画" || guidata->_aniType == "Linear")
		{
			postRend->getPostRender()->setDisplayMode(mxr::DisplayMode::ParametricAnimation);
			_animationType = OneFrameLinearAnimation;
		}
		else
		{
			postRend->getPostRender()->setDisplayMode(mxr::DisplayMode::ParametricAnimation);
			_animationType = OneFrameSinAnimation;
		}

		mGlobalSignals::getInstance()->setProgressBarValue(20);

		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig("动画生成中！");
		}
		else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
		{
			mGlobalSignals::getInstance()->outputMessageSig("Animation being generated！");
		}

		if (aniType == QString("瞬态动画") || aniType == QString("Transient"))
		{
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

			guidata->_aniRange = QString("自定义");

			//控制变量 帧和时间
			if (varType == "帧")
			{
				guidata->_aniControl = QString("帧");

				int f1 = 1, f2 = 1;

				QString valstr = start.simplified();
				if (!valstr.isEmpty())
				{
					f1 = valstr.toInt();
					if (f1 > frameSize)
					{
						mGlobalSignals::getInstance()->outputMessageSig(QString("动画生成失败！选择最大帧超过范围,当前最大帧数为%1！").arg(frameSize));
						// R25 修复（审计逻辑-M3a，既有缺陷顺手修）：上方瞬态分支入口已置
						// 全局等待光标，此早退原样 return 泄漏光标——补恢复
						QApplication::restoreOverrideCursor();
						return false;
					}
					guidata->_aniRangeVal1 = valstr;
				}
				valstr = end.simplified();
				if (!valstr.isEmpty())
				{
					f2 = valstr.toInt();
					if (f2 > frameSize)
					{
						mGlobalSignals::getInstance()->outputMessageSig(QString("动画生成失败！选择最大帧超过范围,当前最大帧数为%1！").arg(frameSize));
						// R25 修复（审计逻辑-M3a，既有缺陷顺手修）：同上，早退补恢复等待光标
						QApplication::restoreOverrideCursor();
						return false;
					}
					guidata->_aniRangeVal2 = valstr;
				}

				if (f1 > f2)
				{
					// R25 修复（审计逻辑-M3a，既有缺陷顺手修）：同上，早退补恢复等待光标
					QApplication::restoreOverrideCursor();
					return false;
				}
				//设置模式（S5：Animation→TransientAnimation 映射）
				postRender->setDisplayMode(mxr::DisplayMode::TransientAnimation);
				_animationRendData = new PostData::mPostAnimationRendData();
				QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

				//清空容器
				_frameIndexes.clear();
				mPostInfoHub::getInstance()->slot_videoFrameRange(f1, f2);
				guidata->_aniShotFrame = f1;
				guidata->_aniVideoStartFrame = f1;
				guidata->_aniVideoEndFrame = f2;

				//删除动画
				this->slot_deleteAnimate();
				for (int i = f1; i <= f2; ++i)
				{
					//存储索引和帧号
					_frameIndexes.append(i);
				}
			}
			else
			{
				//设置模式（S5：Animation→TransientAnimation 映射）
				postRender->setDisplayMode(mxr::DisplayMode::TransientAnimation);
				_animationRendData = new PostData::mPostAnimationRendData();
				QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

				//清空容器
				_frameIndexes.clear();
				guidata->_aniControl = QString("时间");

				double t1 = 0, t2 = 0;
				QString valstr = start.simplified();
				if (!valstr.isEmpty())
				{
					t1 = valstr.toDouble();
				}
				guidata->_aniRangeVal1 = valstr;
				valstr = end.simplified();
				if (!valstr.isEmpty())
				{
					t2 = valstr.toDouble();
				}
				guidata->_aniRangeVal2 = valstr;
				const int nframe = postDB->getTotalFrameCount();
				for (int i = 0; i < nframe; ++i)
				{
					//判断该帧的时间是否在范围内
					double ft = postDB->getFrameTime(i + 1);
					if (ft >= t1 && ft <= t2)
					{
						_frameIndexes.append(i + 1);
					}
				}

				// R25 修复（审计逻辑-M3b，既有缺陷顺手修）：时间控 [t1,t2] 无命中帧时
				// _frameIndexes 为空表，下方 [0]/[count-1] 取值越界 UB（同函数参数块处
				// 有 isEmpty 守卫而此处漏）——空表按生成失败处理：报错+恢复等待光标+
				// return false（对齐 auto 链失败语义，后续 .first() 取值随之不可达空表）
				if (_frameIndexes.isEmpty())
				{
					mGlobalSignals::getInstance()->outputMessageSig(QString("动画生成失败！指定时间范围内无数据帧，请检查时间范围设置！"));
					QApplication::restoreOverrideCursor();
					return false;
				}

				int count = _frameIndexes.size();
				mPostInfoHub::getInstance()->slot_videoFrameRange(_frameIndexes[0], _frameIndexes[count - 1]);
				guidata->_aniShotFrame = _frameIndexes[0];
				guidata->_aniVideoStartFrame = _frameIndexes[0];
				guidata->_aniVideoEndFrame = _frameIndexes[count - 1];
			}

			// 总帧数
			frameamount = _frameIndexes.size();
			_endFrameId = frameamount;

			//图例范围三模式（同上一分支）：手动/全程=各帧色表统一写稳定值域，逐帧=各帧实际最值
			QPair<double, double> allRange{};
			bool allRangeValid = false;
			if (cloudGuidata->_legendRange == QString("手动"))
			{
				if (cloudGuidata->_cloudMaxValue > cloudGuidata->_cloudMinValue)
				{
					allRange = qMakePair(double(cloudGuidata->_cloudMinValue), double(cloudGuidata->_cloudMaxValue));
					allRangeValid = true;
				}
			}
			else if (cloudGuidata->_legendRange == QString("全程"))
			{
				if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
					allRangeValid = cloudPanel->allFrameRange(postRend->getPostDataName(), postDB,
						cloudGuidata->_cloudVarName, cloudGuidata->_cloudCompo,
						cloudGuidata->_cloudLayerName, cloudGuidata->_cloudComplex, allRange);
			}

			for (int frame : _frameIndexes)
			{
				QElapsedTimer frameTimer;
				frameTimer.start();

				//根据界面的数据来读取数据并刷新
				QString postPathName = QString(postDB->getFile());

				QString varstr = cloudGuidata->_cloudVarName;
				QString compostr = cloudGuidata->_cloudCompo;
				QString layerstr = cloudGuidata->_cloudLayerName;
				QString complex = cloudGuidata->_cloudComplex;

				//统一取值 provider（阶段2 二批）：四格式发布式分支收敛；失败策略保留历史
				//契约（native/vtk 失败中止动画并恢复光标，mxdb 失败报告继续）；
				//ElementNodalForce 特判在适配器内（同 autoGenerateAnimation 口径）
				if (IPostValueProvider* provider = valueProviderFor(postDB))
				{
					ValueRequest req;
					req.frame = frame;
					req.var = varstr;
					req.compo = compostr;
					req.layer = layerstr;
					req.complexMode = complex;
					req.coordinateName = QString();   //原分支传空坐标系名（区别于云图菜单的"Global"），保真
					const bool ok = provider->publish(req);
					const bool abortOnFail =
						provider->sourceKind() == IPostValueProvider::SourceKind::Native
						|| provider->sourceKind() == IPostValueProvider::SourceKind::Vtk;
					delete provider;
					if (!ok)
					{
						if (abortOnFail)
						{
							const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
							mGlobalSignals::getInstance()->outputMessageSig(2,
								isChinese ? QStringLiteral("动画生成读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
												.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
										  : QStringLiteral("Failed to read result component for animation: frame %1, variable %2, component %3 (%4); see the debug log for details")
												.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
								true);
							QApplication::restoreOverrideCursor();
							return false;
						}
						reportAniReadFailure(frame, varstr, compostr, postPathName);
					}
				}
				else
				{
					reportAniReadFailure(frame, varstr, compostr, postPathName);
				}
				mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(frame, postRend->getViewportName(), postDB);
				// S5 门面直接替换：代理不再入壳登记（同链一口径，装载工厂随 attachModel 托管）

				auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frame_data_proxy->getModelRenderDataProxy());
				if (model_proxy != nullptr)
					model_proxy->updateValue();

				// 设置颜色表每帧值域：全帧统一值域可得时统一写全局（HyperView 语义——
				// 动画期间图例稳定、颜色跨帧可比），否则逐帧各自实际最值
				if (allRangeValid)
				{
				{
				    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
				    mxr::ColorLegendParams _lg;
				    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				    _lg.targetFrame = frame;
				    _lg.valueMin = float(allRange.first);
				    _lg.valueMax = float(allRange.second);
				    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
				}
				}
				else
				{
					mxr::IPostRendQueryDataProxy* frame_query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frame_data_proxy->getRendQueryDataProxy());
					if (frame_query_proxy != nullptr)
					{
						float frameMin = frame_query_proxy->getCurrentMinData();
						float frameMax = frame_query_proxy->getCurrentMaxData();
					{
					    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
					    mxr::ColorLegendParams _lg;
					    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
					    _lg.targetFrame = frame;
					    _lg.valueMin = frameMin;
					    _lg.valueMax = frameMax;
					    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
					}
					}
					else
					{
						// 如果获取失败，使用云图最值作为备用
					{
					    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
					    mxr::ColorLegendParams _lg;
					    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
					    _lg.targetFrame = frame;
					    _lg.valueMin = cloudGuidata->_cloudMinValue;
					    _lg.valueMax = cloudGuidata->_cloudMaxValue;
					    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
					}
					}
				}

				QPointer <mPostModelData> modeldb = postDB->getModelData(frame);
				if (modeldb == nullptr)
				{
					delete frame_data_proxy;   // C5 泄漏收口：本帧本地代理树迭代内释放（continue 早退分支）
					continue;
				}

				mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frame_data_proxy->getRendQueryDataProxy());
				PostData::PostRigidWallType type;
				if (query_proxy != nullptr)
				{
					for (auto it : postDB->getAllRigidWallDatas())
					{
						double pos[6];
						QVector3D startPos = it->getRigidWallStartPos();
						pos[0] = startPos.x();
						pos[1] = startPos.y();
						pos[2] = startPos.z();

						QVector3D endPos = it->getRigidWallEndPos();
						pos[3] = endPos.x();
						pos[4] = endPos.y();
						pos[5] = endPos.z();

						PostData::PostRigidWallType type = it->getRigidWallType();
						switch (type)
						{
						case PostData::PostRigidWallType::InfinitelyLargePlaneRigidWall:
						{
							PostData::mPostInfinitelyLargePlaneRigidWall* rigidWall = new PostData::mPostInfinitelyLargePlaneRigidWall(it->getRigidWallID(), pos);
							QString name = it->getRigidWallName();
							QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
							rigidWall->updatePos(p);
							query_proxy->appendRigidWallData(rigidWall);
						}
						break;
						case PostData::PostRigidWallType::LimitedLargePlanesRigidWall:
						{
							PostData::mPostLimitedLargePlaneRigidWall* rigidWall = new PostData::mPostLimitedLargePlaneRigidWall(it->getRigidWallID(), pos);
							auto wall = dynamic_cast<PostData::mPostLimitedLargePlaneRigidWall*>(it);
							memcpy(rigidWall->_lineEndVector, wall->_lineEndVector, 3 * sizeof(double));
							rigidWall->_L = wall->_L;
							rigidWall->_M = wall->_M;

							QString name = it->getRigidWallName();
							QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
							rigidWall->updatePos(p);
							query_proxy->appendRigidWallData(rigidWall);
						}
						break;
						case PostData::PostRigidWallType::RectangularRigidWall:
						{
							PostData::mPostRectangularRigidWall* rigidWall = new PostData::mPostRectangularRigidWall(it->getRigidWallID(), pos);
							auto wall = dynamic_cast<PostData::mPostRectangularRigidWall*>(it);
							memcpy(rigidWall->_lineEndVector, wall->_lineEndVector, 3 * sizeof(double));
							rigidWall->_L = wall->_L;
							rigidWall->_M = wall->_M;
							rigidWall->_P = wall->_P;

							QString name = it->getRigidWallName();
							QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
							rigidWall->updatePos(p);
							query_proxy->appendRigidWallData(rigidWall);
						}
						break;
						case PostData::PostRigidWallType::CylindricalRigidWall:
						{
							PostData::mPostCylindricalRigidWall* rigidWall = new PostData::mPostCylindricalRigidWall(it->getRigidWallID(), pos);
							auto wall = dynamic_cast<PostData::mPostCylindricalRigidWall*>(it);
							rigidWall->_radius = wall->_radius;
							rigidWall->_length = wall->_length;

							QString name = it->getRigidWallName();
							QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
							rigidWall->updatePos(p);
							query_proxy->appendRigidWallData(rigidWall);
						}
						break;
						case PostData::PostRigidWallType::SphericalRigidWall:
						{
							PostData::mPostSphericalRigidWall* rigidWall = new PostData::mPostSphericalRigidWall(it->getRigidWallID(), pos);
							auto wall = dynamic_cast<PostData::mPostSphericalRigidWall*>(it);
							rigidWall->_radius = wall->_radius;

							QString name = it->getRigidWallName();
							QVector3D p = modeldb->_frameInfo.getRigidWallEndPos(name);
							rigidWall->updatePos(p);
							query_proxy->appendRigidWallData(rigidWall);
						}
						break;
						default:
							break;
						}
					}
				}

			// 处理当前帧失效单元：与一键生成动画保持一致，每帧独立处理
			// 通过面缓存复用减少 ParseMesh 重复调用。
			std::set<int> lastLoseEles;
			if (frame > 1)
			{
				QPointer<mPostModelData> lastModelData = postDB->getModelData(frame - 1);
				if (lastModelData != nullptr)
					lastLoseEles = lastModelData->getLoseElems();
			}
			applyHideFailedElementForFrame(postDB, postRend, frame_data_proxy, frame, lastLoseEles);
			delete frame_data_proxy;   // C5 泄漏收口：每帧本地代理树（4 子代理聚合）迭代末尾统一释放
			qDebug() << "[Perf] customGenerateAnimation frame=" << frame << "frameTotal=" << frameTimer.elapsed() << "ms";

			setProgressBarValue(frame, frameamount);
			}
			// S5 门面直接替换（B1b(M2/M3) 口径延续，同链一口径）：attachModel 幂等换绑
			// （工厂托管，registerAnimationTable/setRendAnimationFrame 暂留面/移除集
			// 消亡）→ generateAnimation 五约束；门面恒在，legacy 回退分支退役
			{
				mxr::AnimationParams params;
				params.type = mxr::AnimationParams::Type::Transient;
				params.startFrame = _frameIndexes.isEmpty() ? 1 : _frameIndexes.first();
				params.endFrame = _frameIndexes.isEmpty() ? 1 : _frameIndexes.last();
				params.frameRate = _rateFrame;
				params.loop = _animationLoopPlay;
				if (postRender->generateAnimation(params))
				{
					// M14：调用门面后回填 guidata 镜像（值域唯一写者=会话）
					const auto last = postRender->lastAnimationParams();
					guidata->_aniFrameRate = last.frameRate;
					guidata->_aniLoopPlay = last.loop;
					guidata->_aniStartFrame = last.startFrame;
					guidata->_aniEndFrame = last.endFrame;
				}
			}
			//设置帧数据后应用云图点大小（无帧数据时渲染器内部单帧对象未创建，不可调用）
			if (postRender->frameProxy(postRender->currentFrame()) != nullptr)
				postRender->setPointSize(cloudGuidata->_pointSize);

			// 根据变量类型设置节点/单元着色，与云图效果保持一致
			{
				QPointer<mPostModelData> firstModelData = postDB->getModelData(_frameIndexes.first());
				if (firstModelData != nullptr)
				{
					QList<PostVariableInfo> varInfos = firstModelData->_frameInfo._frameVars;
					for (auto it : varInfos)
					{
						if (it._varName == cloudGuidata->_cloudVarName)
						{
							if (it._varType == 0)
							{
								postRender->updateAllModelOperate(ShowNodeColor);
							}
							else if (it._varType == 1)
							{
								postRender->updateAllModelOperate(ShowElementColor);
							}
							break;
						}
					}
				}
			}

			// 刷新颜色映射、最值并着色所有部件
			postRender->updateAllModelOperate(UpdateVariableValue);
			postRender->updateAllModelOperate(UpdateMinMax);
			postRender->updateAllModelOperate(ColorAllPartOperate);

			QApplication::restoreOverrideCursor();
		}
		else if (aniType == QString("线性动画") || aniType == QString("Linear"))
		{
			// S5 门面直接替换：绑定改 attachModel；参数化生成需当前帧绑定（G4
			// boundProxy 门），首绑经 1 帧瞬态生成同步构建引导（ensureInstance 首帧
			// 同步路径=G4 唯一同步绑定入口），随后按类型生成参数化动画
			mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(cloudGuidata->_cloudFrame, postRend->getViewportName(), postDB);
			postRender->attachModel(postDB, postRend->getViewportName());
			if (frame_data_proxy != nullptr)
			{
				mxr::AnimationParams bootstrap;
				bootstrap.type = mxr::AnimationParams::Type::Transient;
				bootstrap.startFrame = cloudGuidata->_cloudFrame;
				bootstrap.endFrame = cloudGuidata->_cloudFrame;
				bootstrap.frameRate = _rateFrame;
				bootstrap.loop = false;
				postRender->generateAnimation(bootstrap);   // 首帧同步构建+绑定
			}
			delete frame_data_proxy;   // C5 泄漏收口：本地引导代理树即用即释（分支内其后仅注释引用）
			postRender->seekFrame(cloudGuidata->_cloudFrame);

			// 设置颜色图每帧最大值最小值
			{
			    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = cloudGuidata->_cloudFrame;
			    _lg.valueMin = cloudGuidata->_cloudMinValue;
			    _lg.valueMax = cloudGuidata->_cloudMaxValue;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}

			mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
			if (model_proxy != nullptr) { model_proxy->updateValue(); }

            // 设置颜色图每帧最大值最小值
            {
                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                _lg.targetFrame = cloudGuidata->_cloudFrame;
                _lg.valueMin = cloudGuidata->_cloudMinValue;
                _lg.valueMax = cloudGuidata->_cloudMaxValue;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
            //frame_data_proxy->getRendQueryDataProxy()->setColorTableWindow(postRender->getColorTableWindow());

			//获取总帧数
			int totalf = 20;
			mPostInfoHub::getInstance()->slot_videoFrameRange(1, totalf);
			guidata->_aniShotFrame = 1;
			guidata->_aniVideoStartFrame = 1;
			guidata->_aniVideoEndFrame = totalf;
			guidata->_aniLinearCount = totalf;
			_frameCountLinear = guidata->_aniLinearCount;
			frameamount = totalf;
			_endFrameId = totalf;
			// S5 门面直接替换（B1b(M3) 口径延续）：setAniLinearCount/createParametric/
			// createLinearAnimation 收敛为 generateAnimation 参数化形态——
			// 线性/正弦=单条目插值生成即就绪；totalf 入 endFrame（插值档位数），
			// _aniLinearCount 会话侧默认 20 与 totalf 同值（插值公式消费）
			{
				mxr::AnimationParams params;
				params.type = mxr::AnimationParams::Type::Linear;
				params.startFrame = 1;
				params.endFrame = totalf;
				params.frameRate = _rateFrame;
				params.loop = _animationLoopPlay;
				postRender->generateAnimation(params);
			}
			//帧数据就绪后应用云图点大小
			postRender->setPointSize(cloudGuidata->_pointSize);
			postRend->updatePaint();
			_isGenerate = true;
			mGlobalSignals::getInstance()->setProgressBarValue(55);
		}
		else if (aniType == QString("正弦动画") || aniType == QString("Sine"))
		{
			// S5 门面直接替换：绑定引导同线性链口径（attachModel+1 帧瞬态引导+seek）
			mxr::PostFrameDataProxy* frame_data_proxy = mxr::createFrameDataProxy(cloudGuidata->_cloudFrame, postRend->getViewportName(), postDB);
			postRender->attachModel(postDB, postRend->getViewportName());
			if (frame_data_proxy != nullptr)
			{
				mxr::AnimationParams bootstrap;
				bootstrap.type = mxr::AnimationParams::Type::Transient;
				bootstrap.startFrame = cloudGuidata->_cloudFrame;
				bootstrap.endFrame = cloudGuidata->_cloudFrame;
				bootstrap.frameRate = _rateFrame;
				bootstrap.loop = false;
				postRender->generateAnimation(bootstrap);   // 首帧同步构建+绑定
			}
			delete frame_data_proxy;   // C5 泄漏收口：本地引导代理树即用即释（分支内其后仅注释引用）
			postRender->seekFrame(cloudGuidata->_cloudFrame);

			// 设置颜色图每帧最大值最小值
			{
			    // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = cloudGuidata->_cloudFrame;
			    _lg.valueMin = cloudGuidata->_cloudMinValue;
			    _lg.valueMax = cloudGuidata->_cloudMaxValue;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}

			mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
			if (model_proxy != nullptr) { model_proxy->updateValue(); }

            // 设置颜色图每帧最大值最小值
            {
                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                mxr::ColorLegendParams _lg;
                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                _lg.targetFrame = cloudGuidata->_cloudFrame;
                _lg.valueMin = cloudGuidata->_cloudMinValue;
                _lg.valueMax = cloudGuidata->_cloudMaxValue;
                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
            }
            //frame_data_proxy->getRendQueryDataProxy()->setColorTableWindow(postRender->getColorTableWindow());

			//获取总帧数
			int totalf = 20;
			mPostInfoHub::getInstance()->slot_videoFrameRange(1, totalf);
			guidata->_aniShotFrame = 1;
			guidata->_aniVideoStartFrame = 1;
			guidata->_aniVideoEndFrame = totalf;
			guidata->_aniLinearCount = totalf;
			_frameCountLinear = guidata->_aniLinearCount;
			frameamount = totalf;
			_endFrameId = totalf;
			// S5 门面直接替换（B1b(M3) 口径延续）：正弦=参数化 Sinusoidal 形态
			{
				mxr::AnimationParams params;
				params.type = mxr::AnimationParams::Type::Sinusoidal;
				params.startFrame = 1;
				params.endFrame = totalf;
				params.frameRate = _rateFrame;
				params.loop = _animationLoopPlay;
				postRender->generateAnimation(params);
			}
			//帧数据就绪后应用云图点大小
			postRender->setPointSize(cloudGuidata->_pointSize);
			postRend->updatePaint();
			_isGenerate = true;
			mGlobalSignals::getInstance()->setProgressBarValue(55);
		}
		guidata->_aniFrameAmount = frameamount;

		postRend->FitView();
		postRend->updatePaint();

		//由于存在某些帧可能没有当前变量的情况，因此生成完毕后需要更新总帧数
		if (frameamount > 0)
		{
			mPostInfoHub::getInstance()->slot_videoEndFrame(frameamount);
			_spinCurFrame->setRange(1, frameamount);
			_spinCurFrame->blockSignals(true);
			_spinCurFrame->setValue(1);
			_spinCurFrame->blockSignals(false);
			_spinStartFrame->setRange(1, frameamount);
			_spinStartFrame->blockSignals(true);
			_spinStartFrame->setValue(1);
			_spinStartFrame->blockSignals(false);
			_spinEndFrame->setRange(1, frameamount);
			_spinEndFrame->blockSignals(true);
			_spinEndFrame->setValue(frameamount);
			_spinEndFrame->blockSignals(false);
			_spinIntervalFrame->setRange(1, frameamount);
			_spinFrameNum->setRange(1, frameamount);

			//球单元显示
			postRender->updateAllModelOperate(ShowDiscreteBall);

			//显示动画
			this->slot_setStartFrame(1);
			_startFrameId = 1;
			this->slot_setEndFrame(frameamount);
			this->slot_setCurrentFrame(1);
			_currentFrameId = 1;

			guidata->_generateAnimate = true;

			//播放时设置不可编辑
			slot_setStatus(true);

			// B1b(R17/H2)：链二生成尾自动播放迁会话门面（与链一同口径）——GUI 定时器
			// 驱动下首个 tick 经 slot_setCurrentFrame 发 UserSeek frameChanged，会被
			// 回写判据（playbackState==Stopped≠Playing）误判"已自然结束"而误停；
			// S5 门面直接替换后门面恒在，GUI 定时器回退分支退役
			postRender->setFrameRate(_rateFrame);
			postRender->setLoop(_animationLoopPlay);
			postRender->setPlayRange(1, frameamount);
			//【审计 B-03】链二生成尾自动播放守卫（链一同款，防宿主旁听链反杀幽灵互斥）
			if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
				cloudPanel->stopRealtimeEngineIfActive();
			postRender->play();

			_viewPlayStatus[_curViewName] = true;
			//生成过程中slot_deleteAnimate已将_aniIsStart置false，播放启动后必须同步回写，
			//否则切换节点回来时动画在播放而按钮显示"开始"
			guidata->_aniIsStart = true;
			MechCommon::mMechGlobalSignals::getInstance()->updateAnimationMenuGUI(_curViewName);
			mGlobalSignals::getInstance()->setProgressBarValue(95);
		}
		else
		{
			//输出信息
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
			{
				mGlobalSignals::getInstance()->setProgressBarValue(100);
				mGlobalSignals::getInstance()->outputMessageSig("动画生成失败！");
			}
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
			{
				mGlobalSignals::getInstance()->setProgressBarValue(100);
				mGlobalSignals::getInstance()->outputMessageSig("Animation generation failure！");
			}

			guidata->_generateAnimate = true;
		}

		//显示云图信息
		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
		if (guidata->_cloudLayerName.isEmpty())
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s", cloudGuidata->_cloudVarName.toStdString().c_str(), cloudGuidata->_cloudCompo.toStdString().c_str());
		}
		else
		{
			postRender->getColorTableWindow()->SetTitle(1, "%s-%s:%s", cloudGuidata->_cloudVarName.toStdString().c_str(), cloudGuidata->_cloudCompo.toStdString().c_str(), cloudGuidata->_cloudLayerName.toStdString().c_str());
		}

		//输出信息
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			mGlobalSignals::getInstance()->setProgressBarValue(100);
			mGlobalSignals::getInstance()->outputMessageSig("动画生成成功！");

		}
		else
		{
			mGlobalSignals::getInstance()->setProgressBarValue(100);
			mGlobalSignals::getInstance()->outputMessageSig("Animation generated successfully!");
		}

		//等值面随动画自动恢复（同 autoGenerateAnimation 收尾，见其注释）
		mPostInfoHub::getInstance()->restoreIsosurfaceAfterAnimation(_curViewName);
		return true;
	}

	//【getCloudViewName 已随 FR-01/FR-05 第二视口链拆除删除（2026-11-04）】该函数
	//原按"动画派生视口名→源云图名"反查（管理器关系优先+剥 _Animation\d+ 正则兜底）。
	//单视口形态下动画与云图共用视口名，双名映射不复存在；且实读
	//mPostDataManager::getCloudViewNameByView 发现其按 AnimationItem.viewName 精确
	//匹配后经 resultId 反查 CloudItem——对"已是云图名"的输入不保证幂等（同一结果
	//节点挂多张云图时可能映射到另一云图名），故所有消费点直接改用 _curViewName。

	void mPostAnimationPanel::updateAimationFrameInfo()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		int num = ((_endFrameId - _startFrameId) / _intervalFrame) + 1;
		_spinFrameNum->setValue(num);

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniFrameNum = num;
		}
	}

	int mPostAnimationPanel::getClosestFrame(double inputTime)
	{
		if (_frameTimes.isEmpty())
			return -1;

		// 获取首帧和末帧
		int firstFrame = _frameTimes.firstKey();
		int lastFrame = _frameTimes.lastKey();
		double firstTime = _frameTimes.value(firstFrame);
		double lastTime = _frameTimes.value(lastFrame);

		if (inputTime <= firstTime)
			return firstFrame;
		if (inputTime >= lastTime)
			return lastFrame;

		// 正常查找
		int bestFrame = -1;
		double minDiff = std::numeric_limits<double>::max();

		for (auto it = _frameTimes.begin(); it != _frameTimes.end(); ++it)
		{
			double diff = std::abs(it.value() - inputTime);
			if (diff < minDiff)
			{
				minDiff = diff;
				bestFrame = it.key();
			}
			if (diff == 0.0)
				return bestFrame;
		}
		return bestFrame;
	}

	QPair<int, double> mPostAnimationPanel::getClosestIntervalFrameNum(double time)
	{
		if (_frameTimes.size() < 2) return {1, 0};

		QVector<int> frames;
		QVector<double> times;
		for (auto it = _frameTimes.begin(); it != _frameTimes.end(); ++it) {
			frames.append(it.key());
			times.append(it.value());
		}

		double minDiff = 0;

		int N = frames.size();
		QPair<int, double> frameInterval{1,0};

		// 枚举所有合法的[start, end]区间（帧号升序，且不等）
		for (int i = 0; i < N - 1; ++i) {
			for (int j = i + 1; j < N; ++j) {
				double delta = times[j] - times[i];
				double diff = std::abs(delta - time);
				if (diff < minDiff) {
					minDiff = diff;
					frameInterval.first = frames[j] - frames[i] + 1;
					frameInterval.second = times[j] - times[i];
				}
				// 完全相等可以提前返回
				if (diff == 0.0)
					return frameInterval;
			}
		}
		return frameInterval;
	}

	void mPostAnimationPanel::setCurViewName(const QString& viewName)
	{
		//同名视口无需切换（自定义生成时会以当前视口名重复调用）
		if (viewName == _curViewName) return;

		//【FR-07】共享定时器停止兜底已随旧直驱链删除（unify-anim-playback：
		//播放推进归会话播放器，各节点播放状态记录链不变）
		_lastViewname = _curViewName;

		_curViewName = viewName;
		MechCommon::mMechGlobalSignals::getInstance()->setAnimationOutputViewport(_curViewName);

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

		//恢复当前节点的帧设置、动画类型和帧索引，避免残留上一个节点的值覆盖本节点数据
		restoreAnimationState(guidata, postDB);

		//按当前节点记录的播放状态决定是否继续播放；无记录或已停止时统一落定为暂停，保证按钮显示与实际一致
		if (guidata->_generateAnimate && _viewPlayStatus.value(_curViewName, false))
		{
			// B1b(R17/H2)：节点切换恢复播放迁会话门面（同 H2 判据互斥理由——GUI 定时器
			// 驱动首 tick 即被回写判据误停）；S5 门面直接替换后门面恒在，回退分支退役
			auto resumeRender = postRend->getPostRender();
			//【审计 B-03】恢复播放入口守卫：浮层引擎档在播→先停浮层（该链不发
			//updateAnimationMenuGUI，无旁听链兜底——不停则浮层双旗悬空、镜像通道误喂
			//预生成事件、浮层暂停误停预生成=持续幽灵互斥）
			if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
				cloudPanel->stopRealtimeEngineIfActive();
			//【审计 A-01/A-02 补口】播放态组显式归位：本链不经 generateAnimation/stop
			//（无渲染侧复位点），浮层暂停残留的往返/反向/步长会粘滞进预生成播放——
			//预生成链只发 rate/loop（无方向/步长/往返控件），显式归位=其既有语义显式化
			resumeRender->setPlaybackDirection(1);
			resumeRender->setFrameInterval(1);
			resumeRender->setPingPong(false);
			resumeRender->setFrameRate(_rateFrame);
			resumeRender->setLoop(_animationLoopPlay);
			resumeRender->play();
		}
		else
		{
			_viewPlayStatus[_curViewName] = false;
			guidata->_aniIsStart = false;
		}
	}

	void mPostAnimationPanel::restoreAnimationState(MechPreData::mPostGUIData* guidata, PostData::mPostDataBase* postDB)
	{
		if (guidata == nullptr) return;

		//恢复帧设置成员
		_startFrameId = guidata->_aniStartFrame;
		_endFrameId = guidata->_aniEndFrame;
		_currentFrameId = guidata->_aniCurrentFrame;
		_intervalFrame = guidata->_aniFrameInterval;
		_rateFrame = guidata->_aniFrameRate;
		_animationLoopPlay = guidata->_aniLoopPlay;

		//恢复动画类型
		if (guidata->_aniType == QString("瞬态动画") || guidata->_aniType == QString("Transient"))
		{
			_animationType = Animation;
		}
		else if (guidata->_aniType == QString("线性动画") || guidata->_aniType == QString("Linear"))
		{
			_animationType = OneFrameLinearAnimation;
		}
		else
		{
			_animationType = OneFrameSinAnimation;
		}

		//重建帧索引和时间映射，构建逻辑与生成动画时保持一致
		_frameIndexes.clear();
		_frameTimes.clear();
		if (guidata->_aniType == QString("瞬态动画") || guidata->_aniType == QString("Transient"))
		{
			if (guidata->_aniRange == QString("自定义") && guidata->_aniControl == QString("帧"))
			{
				int f1 = guidata->_aniRangeVal1.simplified().toInt();
				int f2 = guidata->_aniRangeVal2.simplified().toInt();
				for (int i = f1; i <= f2; ++i)
				{
					_frameIndexes.append(i);
				}
			}
			else if (guidata->_aniRange == QString("自定义") && guidata->_aniControl == QString("时间"))
			{
				double t1 = guidata->_aniRangeVal1.simplified().toDouble();
				double t2 = guidata->_aniRangeVal2.simplified().toDouble();
				if (postDB != nullptr)
				{
					const int nframe = postDB->getTotalFrameCount();
					for (int i = 0; i < nframe; ++i)
					{
						double ft = postDB->getFrameTime(i + 1);
						if (ft >= t1 && ft <= t2)
						{
							_frameIndexes.append(i + 1);
						}
					}
				}
			}
			else
			{
				if (postDB != nullptr)
				{
					const int nframe = postDB->getTotalFrameCount();
					for (int i = 0; i < nframe; ++i)
					{
						_frameIndexes.append(i + 1);
					}
				}
			}

			if (postDB != nullptr)
			{
				for (int frame : _frameIndexes)
				{
					_frameTimes[frame] = postDB->getFrameTime(frame);
				}
			}
		}
		else
		{
			//线性/正弦动画按生成的固定帧数恢复
			_frameCountLinear = guidata->_aniLinearCount;
		}
	}

	void mPostAnimationPanel::setProgressBarValue(int i, int size)
	{
		double num = (double)i / size;
		quint64 tempNum = num * 70 + 20;
		int value = tempNum;
		mGlobalSignals::getInstance()->setProgressBarValue(value);
	}

	void mPostAnimationPanel::slot_setEndFrame(int f)
	{
		if (_spinCurFrame->value() > f)
		{
			_spinCurFrame->setValue(f);
		}
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniEndFrame = f;
			_endFrameId = f;
		}

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return;

		//显示GUI上的时间
		if (guidata != nullptr && guidata->_aniType == QString("瞬态动画"))
		{
			int timef = 1;
			if (f > 0 && f <= _frameIndexes.size())
			{
				timef = _frameIndexes.at(f - 1);
			}
			_lineEditEndTime->blockSignals(true);
			_lineEditEndTime->setText(MxFunctions::mNumberToString(postDB->getFrameTime(timef)));
			_lineEditEndTime->blockSignals(false);
			guidata->_aniEndFrameTime = postDB->getFrameTime(timef);
		}

		//更新帧显示
		updateAimationFrameInfo();
	}

	void mPostAnimationPanel::slot_setCurrentFrame(int f)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//判断当前帧是否在范围内
		if (f < _spinStartFrame->value())
		{
			f = _spinStartFrame->value();
		}
		else if (f > _spinEndFrame->value())
		{
			f = _spinEndFrame->value();
		}

		int timef = f;
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		//存储GUI数据
		if (guidata == nullptr) return;
		guidata->_aniCurrentFrame = f;

		//FR-01/FR-05 单视口形态：guidata 即云图 GUIData（同一份），_cloudFrame 无需
		//再从云图条目反查同步（原双名消费核销）；cloudGuidata 保留为 guidata 别名，
		//下游点大小等云图状态消费点零改动
		mPostGUIData* cloudGuidata = guidata;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return;

		//【审计 C-08】参数化动画标记：手动改帧走档位域定位（positionInterpolation），
		//不走 seekFrame 帧号域换绑（见下方定位点）
		bool isParametricAnim = false;
		if (guidata != nullptr && guidata->_aniType == QString("线性动画"))
		{
			postRender->setDisplayMode(mxr::DisplayMode::ParametricAnimation);   // S5：Linear→ParametricAnimation
			isParametricAnim = true;
		}
		else if (guidata != nullptr && guidata->_aniType == QString("正弦动画"))
		{
			postRender->setDisplayMode(mxr::DisplayMode::ParametricAnimation);   // S5：Sin→ParametricAnimation
			isParametricAnim = true;
		}
		else
		{
			if (f > 0 && f <= _frameIndexes.size())
			{
				timef = _frameIndexes.at(f - 1);
			}
			postRender->setDisplayMode(mxr::DisplayMode::TransientAnimation);   // S5：Animation→TransientAnimation
            //同步云图当前帧号，确保标注面板等能获取到动画实际帧
            if (guidata->_aniControl == "帧")
            {
                guidata->_cloudFrame = timef;
            }
		}

		// 同步云图当前帧号，确保标注面板等能获取到动画实际帧
		//guidata->_cloudFrame = timef;

		//切换帧
		int fi = f;
		if (f > 0 && f <= _frameIndexes.size())
		{
			fi = _frameIndexes.at(f - 1);
		}

		// 将动画索引映射为实际帧号
		int actualStart = guidata->_aniStartFrame;
		int actualEnd = guidata->_aniEndFrame;
		if (actualStart > 0 && actualStart <= _frameIndexes.size())
		{
			actualStart = _frameIndexes.at(actualStart - 1);
		}
		if (actualEnd > 0 && actualEnd <= _frameIndexes.size())
		{
			actualEnd = _frameIndexes.at(actualEnd - 1);
		}

		// B1b(M4)：定位走会话——seekFrame 默认同帧去重+未就绪保持上一帧（V-3 驱动点
		// 就绪整改：未就绪时会话广播 frameNotReady 并保持当前画面，不落空帧）；
		// 播放范围经 setPlayRange 下发（幂等，同值不重触发）。
		// S5 门面直接替换：setAnimationFrame→setPlayRange+seekFrame 收敛（B2 幂等继承），
		// 门面恒在 legacy 分支退役
		postRender->setPlayRange(actualStart, actualEnd);
		//【审计 C-08】参数化手动改帧=档位域定位（positionInterpolation：clamp 到播放域+
		//按 _postMode 单点重算插值，Playing 拒绝门=播放中手动改档不生效）——原
		//seekFrame(档位值) 将 1..20 档位误作帧号：真实帧数≥档位时换绑底层帧（改插值
		//基底），不足时越域装载+frameNotReady——均违"参数化基于当前绑定帧单条目插值"
		//口径（02 §6）。帧动画照走 seekFrame 帧号域（fi=映射实际帧号）
		if (isParametricAnim)
			postRender->positionInterpolation(f);
		else
			postRender->seekFrame(fi);

		double time = postDB->getFrameTime(timef);
		_lineEditCurTime->blockSignals(true);
		_lineEditCurTime->setText(MxFunctions::mNumberToString(time));
		_lineEditCurTime->blockSignals(false);
		guidata->_aniCurrentFrameTime = time;

		//图例范围三模式（HyperView Fixed/Dynamic Scale 对应）：手动=guidata 锁定值域、
		//全程=全帧统一值域——播放期间图例值域保持稳定（脚标仍为当前帧实际最值）；
		//逐帧=维持各帧各自最值
		QPair<double, double> allRange{};
		bool allRangeValid = false;
		if (cloudGuidata->_legendRange == QString("手动"))
		{
			if (cloudGuidata->_cloudMaxValue > cloudGuidata->_cloudMinValue)
			{
				allRange = qMakePair(double(cloudGuidata->_cloudMinValue), double(cloudGuidata->_cloudMaxValue));
				allRangeValid = true;
			}
		}
		else if (cloudGuidata->_legendRange == QString("全程"))
		{
			if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
				allRangeValid = cloudPanel->allFrameRange(postname, postDB,
					cloudGuidata->_cloudVarName, cloudGuidata->_cloudCompo,
					cloudGuidata->_cloudLayerName, cloudGuidata->_cloudComplex, allRange);
		}

		if (_animationType == Animation)
		{
			//更新颜色表最值（B1b(M4)：帧代理经门面 frameProxy 现取——F8 纪律：
			//换帧/重装载/淘汰后必须重取，不缓存句柄）
			mxr::PostFrameDataProxy* aniFrameProxy = postRender->frameProxy(fi);
			if (aniFrameProxy == nullptr)
				return;
			mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(aniFrameProxy->getRendQueryDataProxy());
			if (query_proxy == nullptr)
				return;

			mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(aniFrameProxy->getModelRenderDataProxy());
			if (model_query_proxy != nullptr)
			{
				if (!_isSelRange)
				{
					mxr::PostNodeOrElement nodeOrElement = query_proxy->getNodeOrElement();
					std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
					std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();

					if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
					{
						postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
						postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
						postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
						postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
                        if (allRangeValid)
                            {
                                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                                mxr::ColorLegendParams _lg;
                                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                                _lg.targetFrame = fi;
                                _lg.valueMin = float(allRange.first);
                                _lg.valueMax = float(allRange.second);
                                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                            }
                        else
                            {
                                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                                mxr::ColorLegendParams _lg;
                                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                                _lg.targetFrame = fi;
                                _lg.valueMin = query_proxy->getCurrentMinData();
                                _lg.valueMax = query_proxy->getCurrentMaxData();
                                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                            }
					}
					else
					{
						postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
						postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
						postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
						postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
                        if (allRangeValid)
                            {
                                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                                mxr::ColorLegendParams _lg;
                                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                                _lg.targetFrame = fi;
                                _lg.valueMin = float(allRange.first);
                                _lg.valueMax = float(allRange.second);
                                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                            }
                        else
                            {
                                // 色表值域收编（C4 仅窗单效：动画读数链现状只写窗，不触发实例写）
                                mxr::ColorLegendParams _lg;
                                _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
                                _lg.targetFrame = fi;
                                _lg.valueMin = query_proxy->getCurrentMinData();
                                _lg.valueMax = query_proxy->getCurrentMaxData();
                                PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
                            }
					}
				}
			}
		}

		//切帧后保持云图设置的点大小（无帧数据时渲染器内部单帧对象未创建，不可调用）
		if (postRender->frameProxy(postRender->currentFrame()) != nullptr)
			postRender->setPointSize(cloudGuidata->_pointSize);

		//更新视图
		postRend->updatePaint();

		// 通知标注面板等跟随动画当前帧刷新
		MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(timef);
	}

	void mPostAnimationPanel::slot_setFrameInterval(int f)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		postRender->setFrameRate(1000 / qMax(1, f));   // S5：旧 interval 经 1000/f 换算入帧率
		_intervalFrame = f;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		//存储GUI数据
		if (guidata == nullptr) return;
		guidata->_aniFrameInterval = f;
		_spinIntervalFrame->setValue(f);

		//更新帧显示
		updateAimationFrameInfo();
	}

	void mPostAnimationPanel::slot_aniTimer(int frame)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)	return;
		if (!guidata->_aniIsStart)
		{
			return;
		}

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return;

		//更新当前帧（显式调用，避免valueChanged信号反向写入）
		_spinCurFrame->blockSignals(true);
		_spinCurFrame->setValue(frame);
		_spinCurFrame->blockSignals(false);
		slot_setCurrentFrame(frame);
		double timeval = postDB->getFrameTime(frame);
		_lineEditCurTime->blockSignals(true);
		_lineEditCurTime->setText(MxFunctions::mNumberToString(timeval));
		_lineEditCurTime->blockSignals(false);

		// B1b(M4/M11 定案)：循环回绕与非循环停止由会话播放器承担（真循环回绕修
		// P10；至尾帧 playTick 自动 Stopped，发射端连接内回写 GUI 播放态镜像），
		// 本槽仅做面板显示镜像（帧号/时间），GUI 不再二次推进/回绕
	}

	void mPostAnimationPanel::slot_setStartFrameTime()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		double time = _lineEditStartTime->text().toDouble();
		int frame = getClosestFrame(time);
		_spinStartFrame->setValue(frame);

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniStartFrame = frame;
			guidata->_aniStartFrameTime = time;
		}
	}

	void mPostAnimationPanel::slot_setEndFrameTime()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		double time = _lineEditEndTime->text().toDouble();
		int frame = getClosestFrame(time);
		_spinEndFrame->setValue(frame);

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniEndFrame = frame;
			guidata->_aniEndFrameTime = time;
		}
	}

	void mPostAnimationPanel::slot_setCurrentFrameTime()
	{
		double time = _lineEditCurTime->text().toDouble();
		int frame = getClosestFrame(time);
		_spinCurFrame->setValue(frame);
	}

	void mPostAnimationPanel::slot_setFrameIntervalTime()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		double time = _lineEditIntervalTime->text().toDouble();
		QPair<int, double> pair = getClosestIntervalFrameNum(time);
		slot_setFrameInterval(pair.first);
		_lineEditIntervalTime->setText(MxFunctions::mNumberToString(pair.second));

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniFrameInterval = pair.first;
			guidata->_aniFrameIntervalTime = pair.second;
		}
	}

	void mPostAnimationPanel::slot_deleteAnimate()
	{
		slot_deleteAnimateFor(_curViewName);
	}

	void mPostAnimationPanel::slot_deleteAnimateFor(const QString& viewname)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(viewname);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		// 停止播放（旧共享定时器停止兜底已随旧直驱链删除，FR-07）
		_animationIsStart = false;
		_viewPlayStatus[viewname] = false;

		// B1b(M16)：删除动画=stop（回 StaticFrame+当前帧=停止时帧；删动画不删数据——
		// 帧表保留回放链可复用）。S5 门面直接替换：deleteAnimationKeepData 为引擎
		// 暂留面、deleteAnimation 为移除集——门面 stop() 统一承载；门面恒在无回退分支
		// FR-01/FR-05 单视口形态：stop 后视口显示回到停止时帧（云图会话同源），视口
		// 本体不关闭——原删除链 deletePostViewport(动画视口) 随第二视口拆除删除
		postRender->stop();
		postRend->updatePaint();
		_isGenerate = false;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_generateAnimate = false;
			guidata->_aniIsStart = false;
		}

		//若删除的是当前面板视口上的动画，收尾同步菜单控件态（动画页按钮随
		//_generateAnimate=false 全禁用）；非当前视口时该发射按视口名取数、无副作用
		MechCommon::mMechGlobalSignals::getInstance()->updateAnimationMenuGUI(viewname);
	}

	void mPostAnimationPanel::slot_firstFrame()
	{
		_currentFrameId = _spinStartFrame->value();
		_spinCurFrame->setValue(_spinStartFrame->value());
	}

	void mPostAnimationPanel::slot_lastFrame()
	{
		int curf = _spinCurFrame->value();
		int intervalf = _spinIntervalFrame->value();
		int f = curf - intervalf;
		if (f < _spinStartFrame->value())
		{
			_spinCurFrame->setValue(_spinStartFrame->value());
			_currentFrameId = _spinStartFrame->value();
		}
		else
		{
			_currentFrameId = f;
			_spinCurFrame->setValue(f);
		}
	}

	void mPostAnimationPanel::slot_PlayAnimation(bool play)
	{
		//引擎合一互斥（2026-11-16）：浮层引擎态在播（会话播放器正被浮层配置驱动）→
		//先停浮层（pausePlayback→toggled(false)→云图面板引擎停机分支会话 pause），
		//避免同一会话播放器被两处配置抢推
		if (auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel())
			cloudPanel->stopRealtimeEngineIfActive();
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		if (postRend->getViewportName() != _curViewName)
			return;
		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		_animationIsStart = play;
		// B1b(M4)：播放控制走门面（play/pause；真循环回绕修 P10）。S5 门面直接替换：
		// 门面恒在，GUI 定时器回退分支退役（旧定时器 stop 兜底已随 FR-07 死代码清理
		//删除——推进唯一引擎=会话播放器 playTick）
		if (play)
		{
			//【审计 A-01/A-02 补口】播放态组显式归位：浮层暂停残留的往返/反向/步长不经
			//stop()/generateAnimation 复位点会粘滞进预生成播放（本入口可自浮层暂停态
			//直接接管）——预生成链只发 rate/loop（无方向/步长/往返控件），显式归位=
			//其既有语义显式化；对"用户暂停后续播"为幂等 no-op（此前即默认值）
			postRender->setPlaybackDirection(1);
			postRender->setFrameInterval(1);
			postRender->setPingPong(false);
			postRender->setFrameRate(_rateFrame);
			postRender->setLoop(_animationLoopPlay);
			postRender->play();
			_animationIsStart = true;
			_viewPlayStatus[_curViewName] = true;
		}
		else
		{
			_animationIsStart = false;
			postRender->pause();
			_viewPlayStatus[_curViewName] = false;
		}

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniIsStart = play;
			if (!play)
			{
				//Q3-B 落帧（用户裁定）：停止/暂停即落帧事件——播放期间逐帧通知被
				//测量面板播放门控吸收，停止后补发一次换帧通知使标注对齐停止时刻
				//画面帧；帧参与播放 tick emit（slot_setCurrentFrame 尾）同款映射帧，
				//语义一致（评审 S5）
				int stopFrame = guidata->_aniCurrentFrame;
				if (stopFrame > 0 && stopFrame <= _frameIndexes.size())
					stopFrame = _frameIndexes.at(stopFrame - 1);
				MechCommon::mMechGlobalSignals::getInstance()->postCloudFrameChanged(stopFrame);
			}
		}
	}

	void mPostAnimationPanel::slot_nextFrame()
	{
		int curf = _spinCurFrame->value();
		int intervalf = _spinIntervalFrame->value();
		int f = curf + intervalf;
		if (f > _spinEndFrame->value())
		{
			_spinCurFrame->setValue(_spinEndFrame->value());
			_currentFrameId = _spinEndFrame->value();
		}
		else
		{
			_spinCurFrame->setValue(f);
			_currentFrameId = f;
		}
	}

	void mPostAnimationPanel::slot_endFrame()
	{
		_currentFrameId = _spinEndFrame->value();
		_spinCurFrame->setValue(_spinEndFrame->value());
	}

	void mPostAnimationPanel::slot_setRenderRange(const QString& type)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		if (type == QString("当前帧范围"))
		{
			//postRend->setAnimationFrameRange(false);
		}
		else if (type == QString("所有帧最大范围"))
		{
			//postRend->setAnimationFrameRange(true);
		}

		//更新视图
		postRend->update();

		//存储GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniEveryRange = type;
		}
	}

	void mPostAnimationPanel::slot_setHideFailedElement(bool checked)
	{
		_isHideFailedElement = checked;

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend != nullptr)
		{
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata != nullptr)
				guidata->_aniHideFailedElement = checked;
		}

		// 同步 Ribbon 工具条开关状态
		MechCommon::mMechGlobalSignals::getInstance()->updateAnimationHideFailedElementGUI(checked);

		// 仅瞬态动画需要处理失效单元显隐
		if (_animationType != Animation || _curViewName.isEmpty() || postRend == nullptr)
			return;

		// 停止播放，避免处理过程中与定时器切帧冲突
		slot_PlayAnimation(false);

		auto postRender = postRend->getPostRender();
		if (postRender == nullptr)
			return;

		QString postname = postRend->getPostDataName();
		PostData::mPostDataBase* postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

		// 遍历所有动画帧就地隐藏/恢复失效单元，避免重新读取文件生成动画
		std::set<int> lastLoseEles;
		for (int frame : _frameIndexes)
		{
			bool changed = false;
			if (checked)
			{
				changed = hideFailedElementsForFrameWithCache(postDB, postRend, frame, lastLoseEles);
				QPointer<mPostModelData> curModelData = postDB->getModelData(frame);
				if (curModelData != nullptr)
					lastLoseEles = curModelData->getLoseElems();
			}
			else
			{
				changed = restoreFailedElementsForFrame(postDB, postRend, frame);
			}
			if (changed)
			{
				mxr::PostFrameDataProxy* frameProxy = postRender->frameProxy(frame);   // S5：frameProxy 现取（F8）
				if (frameProxy != nullptr)
				{
					auto model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameProxy->getModelRenderDataProxy());
					if (model_proxy != nullptr)
						model_proxy->RebuildModelData();
				}
			}
		}

		QApplication::restoreOverrideCursor();

		// 刷新当前帧显示
		postRend->updatePaint();
	}

	void mPostAnimationPanel::slot_setLoopPlay(bool istrue)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniLoopPlay = istrue;
			_animationLoopPlay = istrue; 
		}
	}

	void mPostAnimationPanel::slot_setFrameRate(int fr)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		postRend->getAnimationWindow()->setAnimationFrameRate(fr);

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniFrameRate = fr;
		}
		_rateFrame = fr;
	}

	void mPostAnimationPanel::slot_setStatus(bool isEnable)
	{
		_spinCurFrame->setEnabled(!isEnable);
		_spinStartFrame->setEnabled(!isEnable);
		_spinEndFrame->setEnabled(!isEnable);
		_spinIntervalFrame->setEnabled(!isEnable);
		_lineEditCurTime->setEnabled(!isEnable);
		_lineEditStartTime->setEnabled(!isEnable);
		_lineEditEndTime->setEnabled(!isEnable);
		_lineEditIntervalTime->setEnabled(!isEnable);
	}

	void mPostAnimationPanel::updateAnimationGUI()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
			return;

		_spinCurFrame->blockSignals(true);
		_lineEditCurTime->blockSignals(true);
		_spinStartFrame->blockSignals(true);
		_lineEditStartTime->blockSignals(true);
		_spinEndFrame->blockSignals(true);
		_lineEditEndTime->blockSignals(true);
		_spinIntervalFrame->blockSignals(true);
		_lineEditIntervalTime->blockSignals(true);
		_spinFrameNum->blockSignals(true);

		if (guidata->_aniType == QString("瞬态动画"))
		{
			_lineEditCurTime->show();
			_lineEditStartTime->show();
			_lineEditEndTime->show();
			_lineEditIntervalTime->show();
		}
		else
		{
			_lineEditCurTime->hide();
			_lineEditStartTime->hide();
			_lineEditEndTime->hide();
			_lineEditIntervalTime->hide();
		}

		//恢复当前视口的失效单元隐藏开关状态，保持与持久化状态一致
		_isHideFailedElement = guidata->_aniHideFailedElement;

		//按当前动画节点的生成帧数重设各帧控件范围，避免沿用其它节点的范围截断数值
		_spinCurFrame->setRange(1, guidata->_aniFrameAmount);
		_spinStartFrame->setRange(1, guidata->_aniFrameAmount);
		_spinEndFrame->setRange(1, guidata->_aniFrameAmount);
		_spinIntervalFrame->setRange(1, guidata->_aniFrameAmount);
		_spinFrameNum->setRange(1, guidata->_aniFrameAmount);
		_spinCurFrame->setValue(guidata->_aniCurrentFrame);
		_lineEditCurTime->setText(MxFunctions::mNumberToString(guidata->_aniCurrentFrameTime));
		_spinStartFrame->setValue(guidata->_aniStartFrame);
		_lineEditStartTime->setText(MxFunctions::mNumberToString(guidata->_aniStartFrameTime));
		_spinEndFrame->setValue(guidata->_aniEndFrame);
		_lineEditEndTime->setText(MxFunctions::mNumberToString(guidata->_aniEndFrameTime));
		_spinIntervalFrame->setValue(guidata->_aniFrameInterval);
		_lineEditIntervalTime->setText(MxFunctions::mNumberToString(guidata->_aniFrameIntervalTime));
		_spinFrameNum->setValue(guidata->_aniFrameNum);

		_spinCurFrame->blockSignals(false);
		_lineEditCurTime->blockSignals(false);
		_spinStartFrame->blockSignals(false);
		_lineEditStartTime->blockSignals(false);
		_spinEndFrame->blockSignals(false);
		_lineEditEndTime->blockSignals(false);
		_spinIntervalFrame->blockSignals(false);
		_lineEditIntervalTime->blockSignals(false);
		_spinFrameNum->blockSignals(false);

		//更新菜单栏界面
		MechCommon::mMechGlobalSignals::getInstance()->updateAnimationMenuGUI(_curViewName);
	}

	int mPostAnimationPanel::getCurrentFrameID()
	{
		return _currentFrameId;
	}

	int mPostAnimationPanel::getStartFrameID()
	{
		return _startFrameId;
	}

	int mPostAnimationPanel::getEndFrameID()
	{
		return _endFrameId;
	}

	void mPostAnimationPanel::slot_setStartFrame(int f)
	{
		if (_spinCurFrame->value() < f)
		{
			_spinCurFrame->setValue(f);
		}

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			guidata->_aniStartFrame = f;
			_startFrameId = f;
		}

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr) return;

		//显示GUI上的时间
		if (guidata != nullptr && guidata->_aniType == QString("瞬态动画"))
		{
			int timef = 1;
			if (f > 0 && f <= _frameIndexes.size())
			{
				timef = _frameIndexes.at(f - 1);
			}
			_lineEditStartTime->blockSignals(true);
			_lineEditStartTime->setText(MxFunctions::mNumberToString(postDB->getFrameTime(timef)));
			_lineEditStartTime->blockSignals(false);
			guidata->_aniStartFrameTime = timef;
		}

		//更新帧显示
		updateAimationFrameInfo();
	}

}
