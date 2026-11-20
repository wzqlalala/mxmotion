#include "mPostCloudMenuWidget.h"
#include <cmath>

//Post
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mIMxdbFile1.h"
// 须在 mIMxdbFile1.h 之后：mIONativeFile.h 的裸 include "mIOEnum.h" 靠 include 栈
// （mIMxdbFile1.h 所在的 PostIOFile 目录）命中 MIOFile 版枚举，先于本行会被 Common 版遮蔽
#include "NativeReadersIO/mIONativeFile.h"
#include "VtkIO/mIOVtkFile.h"
#include "mMxdbThread.h"
#include "mPostInfoHub.h"
#include "mPostValueProvider.h"
#include "mPost3DResult.h"
#include "CameraManager.h"
#include "mPostParseMesh.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）
// S5 门面直接替换：REBUILD_PostRenderSession 转私有实现宿主（mxrPrivate），公开会话头
// 已删除——会话访问唯一通道=REBUILD_PostRend::getPostRender()（值类型在
// REBUILD_PostRenderTypes.h，已随公开门面头带入）
//#include "mPostRigidWallData.h"
#include "mxr/view/REBUILD_PostModelRender.h"
#include "mPostOneFrameRendData.h"

//PreData
#include "mViewportModel.h"
#include "mViewportModelManager.h"

//MechCommon;
#include "mMechGlobalSignals.h"

//CustomTooltip
#include "mCustomTooltip.h"

//function
#include "mAppConfiguration.h"
#include "mOtherFunction.h"
#include "mPostDeformation.h"
#include "mGlobalSignals.h"

using namespace MechPreData;
using namespace PostData;
using namespace MechCommon;
using namespace MIOFile;

namespace
{
	// mxdb 新旧分支读数据失败的历史行为是静默（切变量无图例无颜色变化无从排查），
	// 统一落输出窗口；文案口径与 mPost3DResultPanel::slot_showCloud native 分支一致
	void reportCloudMenuReadFailure(int frame, const QString& varstr, const QString& compostr,
		const QString& postPathName)
	{
		const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
		mGlobalSignals::getInstance()->outputMessageSig(2,
			isChinese ? QStringLiteral("读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName)
					  : QStringLiteral("Failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
							.arg(frame).arg(varstr).arg(compostr).arg(postPathName),
			true);
	}
}

namespace PostWindow
{
	PostWindow::mPostCloudMenuWidget::mPostCloudMenuWidget(QWidget* parent)
		: QWidget(parent)
	{
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updatePostViewName, this, &mPostCloudMenuWidget::setCurViewName);

		//视口改名跟随（本控件为 QWidget 非 mPanelBase，不进面板注册表，单独订阅；
		//批次3——否则 _curViewName 按旧名查找渲染器落空）
		connect(MechPreData::mViewportModelManager::getInstance(),
			&MechPreData::mViewportModelManager::sig_viewportRenamed, this,
			[this](int, const QString& oldName, const QString& newName)
		{
			if (_curViewName == oldName && oldName != newName)
				_curViewName = newName;
		});
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::update3DResultMenu, this, &mPostCloudMenuWidget::update3DResultMenu);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateResultMenuGUI, this, &mPostCloudMenuWidget::updateResultMenuGUI);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateFreeNodeBtnStatus, this, &mPostCloudMenuWidget::updateFreeNodeBtnStatus);

		//【FR-04】停机同步旁听：postCloudFrameChanged 覆盖四停机点——暂停/手动停止
		//（mPostAnimation Q3-B 补发）、浮层停机（宿主 P2-2 补发）、非循环自然播完
		//（渲染库 playTick 停机补发→动画面板探测器链转发）、删除动画（用户交互链）。
		//播放中（任一动源）到达的逐帧通知按 FR-01/FR-02 早退口径忽略；停机后到达的
		//同名事件自调 updateResultMenuGUI 完成一次性完整同步（含失步收敛段）
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::postCloudFrameChanged, this,
			[this](int)
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
			if (postRend == nullptr || postRend->getPostRender() == nullptr) return;
			if (postRend->getPostRender()->playbackState() == mxr::PlaybackState::Playing) return;
			auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
			if (cloudPanel && cloudPanel->isRealtimeAnimPlaying()) return;
			updateResultMenuGUI();
		});
		//【FR-04④】删除动画停机旁证：updateAnimationMenuGUI 在 stop 完成、会话当前帧
		//仍可读的时点到达（帧表保留可读，"删动画不删数据"）。非播放态自调
		//updateResultMenuGUI；生成完成自动播放起点该信号到达时 state==Playing 被守卫
		//跳过，其余发射点为非播放态幂等刷新（µs 级差分同步）
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::updateAnimationMenuGUI, this,
			[this](QString viewname)
		{
			if (viewname != _curViewName) return;
			REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
			if (postRend == nullptr || postRend->getPostRender() == nullptr) return;
			if (postRend->getPostRender()->playbackState() == mxr::PlaybackState::Playing) return;
			auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
			if (cloudPanel && cloudPanel->isRealtimeAnimPlaying()) return;
			updateResultMenuGUI();
		});
	}

	void mPostCloudMenuWidget::slot_chooseCase(const QString & cname)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		// S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
		if (auto* sessionFacade = postRend->getPostRender().get())
			sessionFacade->notifyDataChanged();

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		QString postName = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr)
			return;

		QStringList fnames = postDB->getCaseFrameShortNames(cname);
		fnames = sortQStringList(fnames);
		guidata->_cloudShortFrames = fnames;

		QStringList names = postDB->getCaseFrameNames(cname);
		names = sortQStringList(names);
		guidata->_cloudFrames = names;

		_frameCombo->blockSignals(true);
		_frameCombo->clear();
		_frameCombo->blockSignals(false);
		// addItems 对空下拉插入首项会同步触发 currentIndexChanged→slot_chooseFrame，
		// sender 同为 _frameCombo，无法区分用户与级联（04 评审 C1）——程序性误标在
		// 此源头复位（含级联链未消费的残留，防泄漏到下一次播放中改变量误判为跳帧）
		_frameCombo->addItems(fnames);
		if (!fnames.isEmpty())
			_frameCombo->setCurrentIndex(0);
		_frameJumpPending = false;
	}

	void mPostCloudMenuWidget::slot_chooseFrame(const QString&)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		QString postName = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr)
			return;

		//【FR-05】跳帧意图标记（全部触发源都经 _frameCombo::currentIndexChanged 到达，
		//工况级联的程序性触发由 slot_chooseCase 尾部复位，见上）；
		//slot_showCloud 消费后复位
		_frameJumpPending = true;

		//获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
		int fi = 0;
		QString fName = _frameCombo->currentText();
		fi = postDB->getFrameNumByCaseAndFrameName(_caseCombo->currentText(), fName);

		//获取该帧的变量
		QStringList vnames = postDB->getVariableByFrameID(fi);
		//首项插入伪变量SolidColor：不读取结果数据，按部件自身颜色显示
		vnames.prepend(QString("SolidColor"));
		QString oldVar = _varCombo->currentText();
		int vari = vnames.indexOf(oldVar);

		_varCombo->blockSignals(true);
		_varCombo->clear();
		_varCombo->addItems(vnames);
		_varCombo->blockSignals(false);
		guidata->_cloudVarNames = vnames;
		//变形变量列表同步：当前帧节点变量且前三分量为空间矢量（X/Y/Z 或 T1/T2/T3；
		//首项=文件位移变量，作变形默认；张量/标量/伪变量 SolidColor 不入表）
		{
			QHash<QString, QStringList> composMap;
			QHash<QString, int> varTypes;
			for (const QString& name : vnames)
			{
				if (name == QString("SolidColor"))
					continue;
				composMap.insert(name, postDB->getVariableCompos(name));
				varTypes.insert(name, postDB->getVariableType(name));
			}
			guidata->_deformVarNames = PostWindow::detail::filterDeformVariables(vnames, composMap, varTypes);
		}

		//优先恢复上次的变量；如果没有历史变量，则默认选中第一个变量
		if (vari >= 0 && !oldVar.isEmpty())
			_varCombo->setCurrentText(oldVar);
		else if (!vnames.isEmpty())
			_varCombo->setCurrentIndex(0);
		//【BUG-02 轮2修复】恢复未产生变量下拉索引变化时（如恢复变量==重建后首项
		//SolidColor，setCurrentText/setCurrentIndex 不发射信号），后续链路
		//（slot_chooseVariable→drawCloudMap→slot_showCloud）不触发，选帧链
		//（seekFrame）不执行——FR-05 跳帧意图丢失（SolidColor 态播放中改帧无跳转；
		//静态态同构 quirk 一并闭合，与面板侧 slot_chooseFrame 手动触发
		//slot_chooseVariable 同口径）。此时标记仍处未消费态（链路运行时会经
		//slot_showCloud 同步消费复位）：先按"链路补齐"触发一次 drawCloudMap
		//（标记仍为 true→slot_showCloud 按 FR-05 跳帧意图消费，保留用户所选帧），
		//随后无条件兜底复位防泄漏（drawCloudMap 被 _isDrawCloudMap 短路未消费时
		//标记在此收口；链路已消费时此处为幂等空操作）。与 C1 级联复位（
		//slot_chooseCase 尾部）协同：级联路径本函数返回前标记已收口，
		//slot_chooseCase 尾部复位为幂等二道防线
		if (_frameJumpPending)
			drawCloudMap();
		_frameJumpPending = false;
	}

	void mPostCloudMenuWidget::slot_chooseVariable(const QString & vname)
	{
		_isDrawCloudMap = false;

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		// S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
		if (auto* sessionFacade = postRend->getPostRender().get())
			sessionFacade->notifyDataChanged();

		QString postName = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr)
			return;
		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		QString caseName = _caseCombo->currentText();
		PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

		if (caseInfo == nullptr)
		{
			caseInfo = new PostData::oneCaseInfo();
			caseInfo->caseName = caseName;
			caseInfo->varName = vname;
			_caseInfos.insert(caseName, caseInfo);
		}
		else
		{
			if (caseInfo->varName != vname)
			{
				delete caseInfo;
				caseInfo = nullptr;
				caseInfo = new PostData::oneCaseInfo();
				caseInfo->caseName = caseName;
				caseInfo->varName = vname;
				_caseInfos.insert(caseName, caseInfo);
			}
		}

        guidata->_caseInfos = _caseInfos;
		if (postDB == nullptr)
			return;

		//伪变量SolidColor：无分量/层/复数，不读取结果数据，按部件自身颜色显示
		if (vname == QString("SolidColor"))
		{
			guidata->_cloudCompos.clear();
			guidata->_cloudHaveComplex = false;
			guidata->_cloudHaveLayer = false;
			_compoCombo->blockSignals(true);
			_compoCombo->clear();
			_compoCombo->setDisabled(true);
			_layerCombo->blockSignals(true);
			_layerCombo->clear();
			_layerCombo->setDisabled(true);
			_complexCombo->setDisabled(true);
			//复数组合框重置为初始状态
			_complexCombo->blockSignals(true);
			if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
				_complexCombo->setCurrentText("实部");
			else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
				_complexCombo->setCurrentText("Real");
			_complexCombo->blockSignals(false);
			_compoCombo->blockSignals(false);
			_layerCombo->blockSignals(false);

			//SolidColor无结果数据，禁用自由节点显示
			guidata->_cloudHaveFreeNode = false;
			MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(true);

			_isDrawCloudMap = true;
			drawCloudMap();
			return;
		}

		//获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
		int fi = 0;
		QString fName = _frameCombo->currentText();
		fi = postDB->getFrameNumByCaseAndFrameName(_caseCombo->currentText(), fName);
		//获取该变量的分量
		QStringList cnames = postDB->getVariableCompos(vname);
		QString oldcompo = _compoCombo->currentText();

		_compoCombo->blockSignals(true);
		_compoCombo->clear();
		_compoCombo->blockSignals(false);
		if (cnames.size() == 0)
		{
			_compoCombo->setDisabled(true);
			_complexCombo->setDisabled(true);

			//获取该变量的层
			QStringList lnames = postDB->getVariableLayer(vname);
			lnames.sort();

			QString oldlayer = _layerCombo->currentText();
			_layerCombo->blockSignals(true);
			_layerCombo->clear();
			_layerCombo->blockSignals(false);
			guidata->_cloudLayerNames = lnames;
			if (lnames.size() == 0)
			{
				_layerCombo->setDisabled(true);
			}
			else
			{
				_layerCombo->setDisabled(false);
				_layerCombo->blockSignals(true);
				_layerCombo->addItems(lnames);
				_layerCombo->blockSignals(false);
				if (caseInfo->layerName != "")
				{
					_layerCombo->setCurrentText(caseInfo->layerName);
				}
				else
				{
					QString temp{};
					if (isContainStr(lnames, "Max", temp))
						_layerCombo->setCurrentText(temp);
				}
			}
		}
		else
		{
			_compoCombo->setDisabled(false);
			_compoCombo->blockSignals(true);
			_compoCombo->addItems(cnames);
			_compoCombo->blockSignals(false);
			guidata->_cloudCompos = cnames;

			QString temp{};
			if (caseInfo->compoName != "")
			{
				_compoCombo->setCurrentText(caseInfo->compoName);
				//防止不触发slot_chooseCompo槽函数
				if (caseInfo->compoName == _compoCombo->itemText(0))
				{
					slot_chooseCompo(caseInfo->compoName);
				}
			}
			else
			{
				if (isContainStr(cnames, "Mag", temp))
				{
					_compoCombo->setCurrentText(temp);
					bool iscomplex = postDB->isVarCompoComplex(vname, temp);
					//设置控件显隐
					_complexCombo->setDisabled(iscomplex);
					slot_chooseCompo(temp);
				}
				else if (isContainStr(cnames, "vonMises", temp))
				{
					_compoCombo->setCurrentText(temp);
					slot_chooseCompo(temp);
				}
			}

			//获取该变量的层
			QStringList lnames = postDB->getVariableLayer(vname);
			lnames.sort();

			QString oldlayer = _layerCombo->currentText();
			_layerCombo->blockSignals(true);
			_layerCombo->clear();
			_layerCombo->blockSignals(false);
			if (lnames.size() == 0)
			{
				_layerCombo->setDisabled(true);
			}
			else
			{
				_layerCombo->setDisabled(false);
				_layerCombo->blockSignals(true);
				_layerCombo->addItems(lnames);
				_layerCombo->blockSignals(false);
				guidata->_cloudLayerNames = lnames;
				if (caseInfo->layerName != "")
				{
					_layerCombo->setCurrentText(caseInfo->layerName);
				}
				else
				{
					QString temp{};
					if (isContainStr(lnames, "Max", temp))
						_layerCombo->setCurrentText(temp);
				}
			}
		}

        //控制大小控制的是点单元的大小而不是自由节点的大小（逻辑存疑）
        //根据当前帧是否包含自由节点以及当前变量是否为节点变量判断是否禁用自由节点操作按钮
        //bool hasFreeNode = postDB->isHaveFreeNode(fi);
		int varType = postDB->getVariableType(vname);
		if (/*hasFreeNode &&*/ varType == 0)
		{
			guidata->_cloudHaveFreeNode = true;
			MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(false);
		}
		else
		{
			guidata->_cloudHaveFreeNode = false;
			MechCommon::mMechGlobalSignals::getInstance()->updateFreeNodeBtnStatus(true);
		}

		_isDrawCloudMap = true;
		drawCloudMap();
	}

	void mPostCloudMenuWidget::slot_chooseCompo(const QString & cname)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		// S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
		if (auto* sessionFacade = postRend->getPostRender().get())
			sessionFacade->notifyDataChanged();
		QString viewName = postRend->getViewportName();
		if (viewName != _curViewName)
			return;
		QString postName = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (postDB == nullptr)
			return;
		QString varName = _varCombo->currentText();
		QString caseName = _caseCombo->currentText();

		//找到当前分量对应的工况信息
		PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

		if (caseInfo == nullptr)
		{
			return;
		}
		caseInfo->compoName = _compoCombo->currentText();
		bool iscomplex = postDB->isVarCompoComplex(varName, cname);
		//设置控件显隐
		if (iscomplex)
		{
			_complexCombo->setDisabled(false);
		}
		else
		{
			_complexCombo->setDisabled(true);
		}

		//将复数组合框重置为初始状态
		_complexCombo->blockSignals(true);
		if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		{
			_complexCombo->setCurrentText("实部");
		}
		else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
		{
			_complexCombo->setCurrentText("Real");
		}
		_complexCombo->blockSignals(false);

		if (iscomplex)
		{
			if (caseInfo->complexName != "")
			{
				_complexCombo->setCurrentText(caseInfo->complexName);
			}
			else
			{
				caseInfo->complexName = _complexCombo->currentText();
			}
		}
		else
		{
			caseInfo->complexName = "";
		}
		drawCloudMap();
	}

	void mPostCloudMenuWidget::slot_chooseLayer(const QString&)
	{
		QString varName = _varCombo->currentText();
		QString cName = _compoCombo->currentText();
		QString caseName = _caseCombo->currentText();

		//找到当前分量对应的工况信息
		PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);

		if (caseInfo == nullptr)
		{
			return;
		}
		caseInfo->layerName = _layerCombo->currentText();

		// S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
		if (REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName))
			if (auto* sessionFacade = postRend->getPostRender().get())
				sessionFacade->notifyDataChanged();

		drawCloudMap();
	}

	void mPostCloudMenuWidget::slot_chooseComplex(const QString&)
	{
		QString varName = _varCombo->currentText();
		QString caseName = _caseCombo->currentText();

		//找到当前分量对应的工况信息
		PostData::oneCaseInfo* caseInfo = _caseInfos.value(caseName);
		if (caseInfo == nullptr)
		{
			return;
		}

		caseInfo->complexName = _complexCombo->currentText();

		// S5 D14 数据纪元：值数据将变化——已装载帧条目置待重验（绑定/重建时工厂重读新值）
		if (REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName))
			if (auto* sessionFacade = postRend->getPostRender().get())
				sessionFacade->notifyDataChanged();

		drawCloudMap();
	}

	void mPostCloudMenuWidget::slot_chooseRangeType(int index)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
		if (query_proxy1 == nullptr)
			return;

		mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
		if (model_proxy == nullptr)
			return;
		float minval = FLT_MAX;
		float maxval = -FLT_MAX;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
			return;
		guidata->_rangeIndex = index;

		if (index == 0)
		{
			maxval = query_proxy1->getOriginalMaxData();
			minval = query_proxy1->getOriginalMinData();
			//_editMax->blockSignals(true);
			//_editMin->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMax->blockSignals(false);
			//_editMin->blockSignals(false);
			query_proxy1->setCurrentMaxData(maxval);
			query_proxy1->setCurrentMinData(minval);
			// 色表值域收编（C4 双效点：与同函数尾段 setMinMaxData 伴生——窗写段，实例写段见尾段）
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			this->slot_setMostValue(true);
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}
		else if (index == 1)
		{
			auto cur_min_max = model_proxy->getPartMinMax();
			minval = cur_min_max.first;
			maxval = cur_min_max.second;
			//_editMax->blockSignals(true);
			//_editMin->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMax->blockSignals(false);
			//_editMin->blockSignals(false);
			query_proxy1->setCurrentMinData(minval);
			query_proxy1->setCurrentMaxData(maxval);
			// 色表值域收编（C4 双效点）：默认值域段+窗写段一口两掩码（顺序按现状），实例写段见尾段
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
					| mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			this->slot_setMostValue(true);
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}

		// 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
		{
			mxr::ColorLegendParams _lg;
			_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
			_lg.valueMin = minval;
			_lg.valueMax = maxval;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
		}
		postRender->updateAllModelOperate(ColorAllPartOperate);

		//if (index == 0)
		//{
		//	_editMax->setEnabled(true);
		//	_resetBtnMax->setEnabled(true);
		//	_editMin->setEnabled(true);
		//	_resetBtnMin->setEnabled(true);
		//}
		//else
		//{
		//	_editMax->setEnabled(false);
		//	_resetBtnMax->setEnabled(false);
		//	_editMin->setEnabled(false);
		//	_resetBtnMin->setEnabled(false);
		//}
		mMechGlobalSignals::getInstance()->updateResultInfoGUI();
	}

	void mPostCloudMenuWidget::slot_resetMostValue()
	{
		////获取信号发射者
		//QToolButton* tb = qobject_cast<QToolButton*>(sender());
		//if (tb == nullptr)
		//	return;
		//REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		//if (postRend == nullptr)
		//	return;

		//auto  postRender = postRend->getPostRender();
		//if (postRender == nullptr) return;
		//if (postRender->frameProxy(postRender->currentFrame()) == nullptr) return;
		//mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());

		//float maxval = _editMax->text().trimmed().toFloat();
		//float minval = _editMin->text().trimmed().toFloat();
		//maxval = query_proxy->getOriginalMaxData();
		//minval = query_proxy->getOriginalMinData();
		//if (maxval < minval)
		//{
		//	if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
		//	{
		//		mGlobalSignals::getInstance()->outputMessageSig(2, QString("最大值不能小于最小值！"), true);
		//	}
		//	else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
		//	{
		//		mGlobalSignals::getInstance()->outputMessageSig(2, QString("The maximum value cannot be less than the minimum value!"), true);
		//	}
		//	return;
		//}

		//if (tb == _resetBtnMax)
		//{
		//	query_proxy->setCurrentMaxData(maxval);
		//	_editMax->blockSignals(true);
  //          _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
		//	_editMax->blockSignals(false);
		//}
		//else if (tb == _resetBtnMin)
		//{
		//	query_proxy->setCurrentMinData(minval);
		//	_editMin->blockSignals(true);
  //          _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
		//	_editMin->blockSignals(false);
		//}
		//postRender->getColorTableWindow()->setValueMinMax(postRender->currentFrame(), maxval, minval);
		//this->slot_setMostValue(true);
	}

	void mPostCloudMenuWidget::slot_editValueChanged()
	{
		slot_setMostValue(true);
	}

	void mPostCloudMenuWidget::slot_setPositionVisible(bool isshow)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//存储界面信息
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		//获取信号源
		QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
		if (cb == _checkboxMax)
		{
			// 最值标注收编（unify-addobject-arch）：单侧掩码切换（首建=全量缺省隐藏=现状初始态）
			mxr::MinMaxLabelParams _mm;
			_mm.updateMask = mxr::MinMaxLabelParams::Mask_Max;
			_mm.maxVisible = isshow;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
			guidata->_cloudShowMaxLine = isshow;
		}
		else if (cb == _checkboxMin)
		{
			mxr::MinMaxLabelParams _mm;
			_mm.updateMask = mxr::MinMaxLabelParams::Mask_Min;
			_mm.minVisible = isshow;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _mm);
			guidata->_cloudShowMinLine = isshow;
		}
		postRend->updatePaint();
		mMechGlobalSignals::getInstance()->updateResultInfoGUI();
	}

	void mPostCloudMenuWidget::showOrHideFreeNode()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
		{
			return;
		}

		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (modeldb == nullptr)
			return;

		QPointer<PostData::mPostModelData> modelData = modeldb->getModelData(guidata->_cloudFrame);
		if (modelData == nullptr)
			return;

		auto frameDataProxy = postRender->frameProxy(postRender->currentFrame());
		if (frameDataProxy == nullptr)return;

		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameDataProxy->getRendQueryDataProxy());
		if (query_proxy == nullptr)
			return;
		mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameDataProxy->getModelRenderDataProxy());
		if (model_query_proxy == nullptr)
			return;

		mxr::PostNodeOrElement nodeOrElement = query_proxy->getNodeOrElement();

		float minval = FLT_MAX;
		float maxval = -FLT_MAX;

		_isFreeNodeShow = !_isFreeNodeShow;
		if (_isFreeNodeShow)
		{
			_toolbtnFreeNode->setText("隐藏");
			_toolbtnFreeNode->setIcon(QIcon(":/mMainWidgets/Icon/post/hide.png"));

			bool flag = _checkboxColud->isChecked();
			postDB->showFreeNode(guidata->_cloudFrame, flag);
			postRender->updateAllModelOperate(ShowFreeNode);
			postRend->updatePaint();

			model_query_proxy->updateValue();

			//设置最值
			maxval = query_proxy->getOriginalMaxData();
			minval = query_proxy->getOriginalMinData();
			//_editMax->blockSignals(true);
			//_editMin->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMax->blockSignals(false);
			//_editMin->blockSignals(false);
			// 色表值域收编（C4 仅窗单效：本函数无伴生 setMinMaxData，现状只写窗）
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
			std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
			if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", maxval);
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", minval);
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
			}
			else
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", maxval);
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", minval);
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
			}
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
			guidata->_cloudShowFreeNode = true;
		}
		else
		{
			_toolbtnFreeNode->setText("显示");
			_toolbtnFreeNode->setIcon(QIcon(":/mMainWidgets/Icon/post/display.png"));
			postDB->hideFreeNode(guidata->_cloudFrame);
			postRender->updateAllModelOperate(ClearFreeNode);
			postRend->update();

			model_query_proxy->updateValue();

			//设置最值
			maxval = query_proxy->getCurrentMaxData();
			minval = query_proxy->getCurrentMinData();
			//_editMax->blockSignals(true);
			//_editMin->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMax->blockSignals(false);
			//_editMin->blockSignals(false);
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			// 色表值域收编（C4 仅窗单效：本函数无伴生 setMinMaxData，现状只写窗）
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
			std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
			if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
			}
			else
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
			}
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
			guidata->_cloudShowFreeNode = false;
		}
        mMechGlobalSignals::getInstance()->updateResultInfoGUI();
	}

	void mPostCloudMenuWidget::deleteFreeNode()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
		{
			return;
		}
		QPointer<PostData::mPostModelData> modelData = postDB->getModelData(guidata->_cloudFrame);
		if (modelData == nullptr)
			return;

		auto frameDataProxy = postRender->frameProxy(postRender->currentFrame());
		if (frameDataProxy == nullptr)return;

		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameDataProxy->getRendQueryDataProxy());
		if (query_proxy == nullptr)
			return;
		mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameDataProxy->getModelRenderDataProxy());
		if (model_query_proxy == nullptr)
			return;

		postDB->deleteFreeNode(guidata->_cloudFrame);
		postRender->updateAllModelOperate(ClearFreeNode);
		postRend->update();

		model_query_proxy->updateValue();
		//设置最值
		float maxval = query_proxy->getCurrentMaxData();
		float minval = query_proxy->getCurrentMinData();
		//_editMax->blockSignals(true);
		//_editMin->blockSignals(true);
  //      _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
  //      _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
		//_editMax->blockSignals(false);
		//_editMin->blockSignals(false);
		query_proxy->setCurrentMaxData(maxval);
		query_proxy->setCurrentMinData(minval);
		// 色表值域收编（C4 仅窗单效：flag=false 路径 slot_setMostValue 不触发实例写，现状只写窗）
		{
			mxr::ColorLegendParams _lg;
			_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			_lg.valueMin = minval;
			_lg.valueMax = maxval;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
		}
		this->slot_setMostValue(false);
		guidata->_cloudMaxValue = maxval;
		guidata->_cloudMinValue = minval;
		guidata->_cloudDeleteFreeNode = true;
		_toolbtnFreeNode->setDisabled(true);
		_sizeSpinBox->setDisabled(true);
		_checkboxColud->setDisabled(true);
	}

	void mPostCloudMenuWidget::relatedCloudFreeNode(bool flag)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		guidata->_cloudRelatedCloud = flag;

		if (_isFreeNodeShow)
		{
			_isFreeNodeShow = false;
			showOrHideFreeNode();
		}
	}

	void mPostCloudMenuWidget::slot_ShowOrHidePhysicalization()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return;

		//_isSBRShow = !_isSBRShow;
		//if (_isSBRShow)
		//{
		//	_toolbtnSBR->setText("实体化");
		//	_toolbtnSBR->setIcon(QIcon(":/mMainWidgets/Icon/post/entity hiding.png"));
		//	postRender->updateAllModelOperate(ShowBeamShellVolume);
		//	guidata->_cloudShowSBR = true;
		//}
		//else
		//{
		//	_toolbtnSBR->setText("实体化");
		//	_toolbtnSBR->setIcon(QIcon(":/mMainWidgets/Icon/post/entity display.png"));
		//	postRender->updateAllModelOperate(ClearBeamShellVolume);
		//	guidata->_cloudShowSBR = false;
		//}
		postRend->updatePaint();
	}

	void mPostCloudMenuWidget::slot_setPointSize(double size)
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		postRender->setPointSize(size);
		postRend->updatePaint();

		_sizeSpinBox->setValue(size);
		//存储界面信息
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
			guidata->_pointSize = size;
	}

	void mPostCloudMenuWidget::update3DResultMenu(QString viewname)
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(viewname);
		if (rebuildPostRend == nullptr) return;

		_curViewName = viewname;
	}

	void mPostCloudMenuWidget::updateResultMenuGUI()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//【R13】播放中跳过面板同步：本函数每次换帧被触发，下拉/输入框会被强制回写
		// guidata 值——用户播放中手动改变量/帧下一帧即被弹回。暂停/停止后恢复刷新。
		//【FR-02】判据扩展为"任一动源播放中"：浮层路径会话播放态恒 Stopped（R13 判据
		// 不可达），经宿主公有查询取浮层定时器活跃态——全部停机形态（含让路录制/
		// 形态切换的静默停表）均自动为假，无标志残留
		if (postRender->playbackState() == mxr::PlaybackState::Playing)
			return;
		{
			auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
			if (cloudPanel && cloudPanel->isRealtimeAnimPlaying())
				return;
		}

		QString postname = postRend->getPostDataName();
		mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
		if (postDB == nullptr)
			return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
			return;


		//切换组随菜单模块移除后六个下拉不再创建（见 mPostMenuContributor），为空时
		//跳过其同步与信号开关，其余组（自由节点/过滤）照常回填
		const bool hasSwitchCombos = (_caseCombo != nullptr) && (_varCombo != nullptr)
			&& (_complexCombo != nullptr) && (_frameCombo != nullptr)
			&& (_compoCombo != nullptr) && (_layerCombo != nullptr);

		//【FR-04】失步收敛段：停机同步以会话实际当前帧为准对齐 guidata 帧状态——静态
		//流程两值本一致（幂等 no-op），停机流程在此完成 _cloudFrame↔_cloudFrameIndex
		//与帧下拉三者一次对齐（02 §4.3 收敛源=会话 currentFrame）。
		//【BUG-01 轮2修复】收敛判据覆盖"两状态各自与收敛源的偏差"而非仅帧值差：预生成
		//路径 _cloudFrame 播放期已被动画面板镜像链逐 tick 更新为会话帧（slot_setCurrentFrame
		//帧控分支），_cloudFrameIndex 滞留播放开始前值——原"sessionFi != _cloudFrame"
		//守卫在停机时恒假短路索引回填，帧下拉回填旧帧（TC-06/07a/08/14）。改为：映射
		//命中索引≠存储值即回写（含"帧值已新鲜、索引失步"主场景）；帧值偏差回写独立保留
		//（幂等无害）。帧名→索引映射同 P2-1 口径（"N:"前缀解析），工况名取
		//guidata->_cloudCase（与 slot_showCloud 同源，免疫 UI 时序差）；未命中保持原索引
		//（渐进装载时序下经既有 frameReady 链自愈）。参数化形态 currentFrame 承载插值
		//档域（P4-1 双域混叠移交规格），守卫排除；帧下拉空表（无结果）跳过
		//【合并 2026-11-14】切换组整组移除（远端 2026-11-11 产品决策，六个下拉不再创建）
		//后 _frameCombo 恒空，收敛段随 hasSwitchCombos 守卫整体短路
		if (hasSwitchCombos
			&& postRender->displayMode() != mxr::DisplayMode::ParametricAnimation
			&& _frameCombo->count() > 0)
		{
			const int sessionFi = postRender->currentFrame();
			if (sessionFi > 0)
			{
				int hitIndex = -1;
				for (int i = 0; i < _frameCombo->count(); ++i)
				{
					const QString itemText = _frameCombo->itemText(i);
					const QString itemFrameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
					if (postDB->getFrameNumByCaseAndFrameName(guidata->_cloudCase, itemFrameName) == sessionFi)
					{
						hitIndex = i;
						break;
					}
				}
				if (hitIndex >= 0 && hitIndex != guidata->_cloudFrameIndex)
					guidata->_cloudFrameIndex = hitIndex;
				if (sessionFi != guidata->_cloudFrame)
					guidata->_cloudFrame = sessionFi;
			}
		}

		if (hasSwitchCombos)
		{
			_caseCombo->blockSignals(true);
			_varCombo->blockSignals(true);
			_complexCombo->blockSignals(true);
			_frameCombo->blockSignals(true);
			_compoCombo->blockSignals(true);
			_layerCombo->blockSignals(true);
		}

		//_rangeBG->blockSignals(true);
		//_editMax->blockSignals(true);
		//_editMin->blockSignals(true);
		//_checkboxMax->blockSignals(true);
		//_checkboxMax->blockSignals(true);
		_toolbtnFreeNode->blockSignals(true);
		_sizeSpinBox->blockSignals(true);
		_checkboxColud->blockSignals(true);
		_toolbtnFilter->blockSignals(true);
		_comboRule->blockSignals(true);
		_lineEditRangeMin->blockSignals(true);
		_lineEditRangeMax->blockSignals(true);

		if (hasSwitchCombos)
		{

		//【R12】下拉框列表同步去重建：本函数经 updateResultMenuGUI 信号在**每次换帧**
		// 都被触发（slot_showCloud 尾部），原逻辑每帧对 6 个下拉 clear+addItems 全量
		// 重建（纯 UI 浪费+闪烁，实测 GUI 段卡顿主力之一）。改为列表内容未变时仅
		// 同步选中项（帧号等），列表真变（导入/切工况）才重建
		auto syncComboItems = [](QComboBox* cb, const QStringList& items, const QString& cur) {
			bool same = (cb->count() == items.size());
			if (same) {
				for (int i = 0; i < items.size(); ++i)
					if (cb->itemText(i) != items[i]) { same = false; break; }
			}
			if (same) {
				if (cb->currentText() != cur)
					cb->setCurrentText(cur);
				return;
			}
			cb->clear();
			cb->addItems(items);
			cb->setCurrentText(cur);
		};
		syncComboItems(_caseCombo, guidata->_cloudCases, guidata->_cloudCase);
		syncComboItems(_varCombo, guidata->_cloudVarNames, guidata->_cloudVarName);
		_complexCombo->setCurrentText(guidata->_cloudComplex);
		_complexCombo->setDisabled(!guidata->_cloudHaveComplex);
		{
			// 帧下拉：列表未变仅同步当前索引（播放期帧号每帧变化，同步保留）
			bool same = (_frameCombo->count() == guidata->_cloudShortFrames.size());
			if (same) {
				for (int i = 0; i < guidata->_cloudShortFrames.size(); ++i)
					if (_frameCombo->itemText(i) != guidata->_cloudShortFrames[i]) { same = false; break; }
			}
			if (same) {
				if (_frameCombo->currentIndex() != guidata->_cloudFrameIndex)
					_frameCombo->setCurrentIndex(guidata->_cloudFrameIndex);
			} else {
				_frameCombo->clear();
				_frameCombo->addItems(guidata->_cloudShortFrames);
				_frameCombo->setCurrentIndex(guidata->_cloudFrameIndex);
			}
		}
		syncComboItems(_compoCombo, guidata->_cloudCompos, guidata->_cloudCompo);
		syncComboItems(_layerCombo, guidata->_cloudLayerNames, guidata->_cloudLayerName);
		_layerCombo->setDisabled(!guidata->_cloudHaveLayer);
		}
		int index = guidata->_rangeIndex;
		//QAbstractButton* btn = _rangeBG->button(index);
		//if (btn) btn->setChecked(true);
		//if (index == 1)
		//{
		//	_editMax->setDisabled(true);
		//	_editMin->setDisabled(true);
		//}
		//else
		//{
		//	_editMax->setDisabled(false);
		//	_editMin->setDisabled(false);
		//}
  //      _editMax->setText(MxFunctions::mNumberToString(guidata->_cloudMaxValue, 'f', 12));
  //      _editMin->setText(MxFunctions::mNumberToString(guidata->_cloudMinValue, 'f', 12));
		//_checkboxMax->setChecked(guidata->_cloudShowMaxLine);
		//_checkboxMin->setChecked(guidata->_cloudShowMinLine);
		if (guidata->_cloudShowFreeNode)
		{
			_toolbtnFreeNode->setText("隐藏");
			_toolbtnFreeNode->setIcon(QIcon(":/mMainWidgets/Icon/post/hide.png"));
		}
		else
		{
			_toolbtnFreeNode->setText("显示");
			_toolbtnFreeNode->setIcon(QIcon(":/mMainWidgets/Icon/post/display.png"));
		}
		if (guidata->_cloudHaveFreeNode)
		{
			if (guidata->_cloudDeleteFreeNode)
			{
				_toolbtnFreeNode->setDisabled(true);
				_sizeSpinBox->setDisabled(true);
				_checkboxColud->setDisabled(true);
			}
			else
			{
				_toolbtnFreeNode->setDisabled(false);
				_sizeSpinBox->setDisabled(false);
				_checkboxColud->setDisabled(false);
			}
		}
		else
		{
			_toolbtnFreeNode->setDisabled(true);
			_sizeSpinBox->setDisabled(true);
			_deleteBtn->setDisabled(true);
			_checkboxColud->setDisabled(true);
		}

		_sizeSpinBox->setValue(guidata->_pointSize);
		_checkboxColud->setChecked(guidata->_cloudRelatedCloud);
		
		if (guidata->_filterOpen)
		{
			_toolbtnFilter->setIcon(QIcon(":/mMainWidgets/Icon/post/kai.png"));
			_comboRule->setEnabled(true);
			_lineEditRangeMin->setEnabled(true);
			_lineEditRangeMax->setEnabled(true);
			_lineEditRangeMin->setText(MxFunctions::mNumberToString(guidata->_filterMin, 'e', 4));
			_lineEditRangeMax->setText(MxFunctions::mNumberToString(guidata->_filterMax, 'e', 4));
		}
		else
		{
			_toolbtnFilter->setIcon(QIcon(":/mMainWidgets/Icon/post/guan.png"));
			_comboRule->setEnabled(false);
			_lineEditRangeMin->setEnabled(false);
			_lineEditRangeMax->setEnabled(false);
			_lineEditRangeMin->setText(MxFunctions::mNumberToString(guidata->_cloudMinValue, 'e', 4));
			_lineEditRangeMax->setText(MxFunctions::mNumberToString(guidata->_cloudMaxValue, 'e', 4));
		}
		if (_toolbtnExportSTL)
			_toolbtnExportSTL->setEnabled(guidata->_filterOpen);

		_comboRule->setCurrentText(guidata->_filterRule);
		_filterMin = MxFunctions::mNumberToString(guidata->_cloudMinValue, 'e', 4);
		_filterMax = MxFunctions::mNumberToString(guidata->_cloudMaxValue, 'e', 4);

        _caseInfos = guidata->_caseInfos;

		if (hasSwitchCombos)
		{
			_caseCombo->blockSignals(false);
			_varCombo->blockSignals(false);
			_complexCombo->blockSignals(false);
			_frameCombo->blockSignals(false);
			_compoCombo->blockSignals(false);
			_layerCombo->blockSignals(false);
		}
		//_rangeBG->blockSignals(false);
		//_editMax->blockSignals(false);
		//_editMin->blockSignals(false);
		//_checkboxMax->blockSignals(false);
		//_checkboxMax->blockSignals(false);
		_toolbtnFreeNode->blockSignals(false);
		_sizeSpinBox->blockSignals(false);
		_checkboxColud->blockSignals(false);
		_toolbtnFilter->blockSignals(false);
		_comboRule->blockSignals(false);
		_lineEditRangeMin->blockSignals(false);
		_lineEditRangeMax->blockSignals(false);
	}

	void mPostCloudMenuWidget::updateFreeNodeBtnStatus(bool isDisabled)
	{
		if (isDisabled)
		{
			_toolbtnFreeNode->setDisabled(true);
			_deleteBtn->setDisabled(true);
			_sizeSpinBox->setDisabled(true);
			_checkboxColud->setDisabled(true);
		}
		else
		{
			_toolbtnFreeNode->setDisabled(false);
			_deleteBtn->setDisabled(false);
			_sizeSpinBox->setDisabled(false);
			_checkboxColud->setDisabled(false);
		}
	}

	void mPostCloudMenuWidget::slot_showCloud()
	{
		REBUILD_PostRend* rebuildPostRend = getCVM->getPostRenderByViewName(_curViewName);
		if (rebuildPostRend == nullptr) return;

		auto  postRender = rebuildPostRend->getPostRender();
		if (postRender == nullptr) return;

		QString postName = rebuildPostRend->getPostDataName();
		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
		if (modeldb == nullptr)
			return;

		//获取GUI数据
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(rebuildPostRend);
		if (guidata == nullptr) return;

		//存储工况
		guidata->_cloudCase = _caseCombo->currentText();

		//存储帧名称
		guidata->_cloudFrameIndex = _frameCombo->currentIndex();

		//获取帧
		int fi = 1;
		//【FR-03/FR-05】播放中帧参数分派：变量/分量/层/复数改动（非跳帧意图）帧参数
		//改取会话 currentFrame——按当前帧起生效，不回跳 _frameCombo 冻结帧（防画面
		//倒闪）、_cloudFrameIndex 按同一映射同步（不写坏失步值）；跳帧意图
		//（_frameJumpPending，FR-05）保留用户所选帧走现状解析链。参数化形态不进分派
		//（currentFrame 承载插值档域，P4-1 双域防误伤，维持现状）。标记无条件消费复位
		const bool preGenPlaying = (postRender->playbackState() == mxr::PlaybackState::Playing)
			&& (postRender->displayMode() == mxr::DisplayMode::TransientAnimation);
		auto cloudPanel = mPostInfoHub::getInstance()->cloudResultPanel();
		const bool floatPlaying = (cloudPanel && cloudPanel->isRealtimeAnimPlaying());
		const bool useSessionFrame = (preGenPlaying || floatPlaying) && !_frameJumpPending;
		_frameJumpPending = false;
		if (useSessionFrame)
		{
			fi = postRender->currentFrame();
			//帧名→索引映射（与 updateResultMenuGUI 失步收敛段同口径：P2-1 同款
			//"N:"前缀解析，工况名取 guidata->_cloudCase；未命中保持上方
			//"_frameCombo->currentIndex()" 存储的原值）
			for (int i = 0; i < _frameCombo->count(); ++i)
			{
				const QString itemText = _frameCombo->itemText(i);
				const QString itemFrameName = itemText.contains(":") ? itemText.split(":").at(0) : itemText;
				if (modeldb->getFrameNumByCaseAndFrameName(guidata->_cloudCase, itemFrameName) == fi)
				{
					guidata->_cloudFrameIndex = i;
					break;
				}
			}
		}
		else
		{
			//获取帧号（根据当前帧显示的帧名称与工况名获取帧号）
			QString fName = _frameCombo->currentText();
			fi = modeldb->getFrameNumByCaseAndFrameName(guidata->_cloudCase, fName);
		}

		bool isNew = false;
		if (fi != guidata->_cloudFrame)
			isNew = true;
		guidata->_cloudFrame = fi;
		//获取变量
		QString varstr = _varCombo->currentText();
		guidata->_cloudVarName = varstr;
		//伪变量SolidColor：不读取结果数据，按部件自身颜色显示
		const bool isSolidColor = (varstr == QString("SolidColor"));

		//获取分量
		QString compostr = _compoCombo->currentText();
		guidata->_cloudCompo = compostr;
		//获取层
		QString layerstr = _layerCombo->currentText();
		guidata->_cloudLayerName = layerstr;
		//获取复数
		bool isdisp = false;
		QString complexstr;
		if (_complexCombo->isEnabled())
		{
			complexstr = _complexCombo->currentText();

			if (guidata->_cloudComplex != complexstr)
				isdisp = true;
		}
		guidata->_cloudComplex = complexstr;
		//根据是否切换帧获取数据

		QString coordinateName = QString("Global");
		QVector<double> userCoord{};

		//显示
		QString postPathName = modeldb->getFile();

		//统一取值 provider（阶段2 二批）：四格式发布式分支收敛为单口（isSolidColor 门、
		//坐标系实参、失败策略逐点保持——native/vtk 失败中止重绘，mxdb 失败报告继续）；
		//ElementNodalForce 特判与磨平分支由适配器内处理
		if (!isSolidColor)
		{
			ValueRequest req;
			req.frame = fi;
			req.var = varstr;
			req.compo = compostr;
			req.layer = layerstr;
			req.complexMode = complexstr;
			req.coordinateName = coordinateName;
			req.userCoord = userCoord;
			IPostValueProvider* provider = valueProviderFor(modeldb);
			const bool ok = provider != nullptr && provider->publish(req);
			const bool abortOnFail = provider != nullptr
				&& (provider->sourceKind() == IPostValueProvider::SourceKind::Native
					|| provider->sourceKind() == IPostValueProvider::SourceKind::Vtk);
			delete provider;
			if (!ok)
			{
				if (abortOnFail)
				{
					const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
					mGlobalSignals::getInstance()->outputMessageSig(2,
						isChinese ? QStringLiteral("读取结果分量失败：帧%1，变量%2，分量%3（%4），详见调试日志")
										.arg(fi).arg(varstr).arg(compostr).arg(postPathName)
								  : QStringLiteral("Failed to read result component: frame %1, variable %2, component %3 (%4); see the debug log for details")
										.arg(fi).arg(varstr).arg(compostr).arg(postPathName),
						true);
					return;
				}
				reportCloudMenuReadFailure(fi, varstr, compostr, postPathName);
			}
		}

		// S5 门面直接替换（B1b(M1) 口径延续）：读数/绑定职责在门面装载器——
		// attachModel（幂等换绑）+ requestFrames（渐进装载）+ seekFrame（命中即绑定；
		// 未就绪保持上一帧，frameReady 后一次性重入本槽收敛）。F8 纪律：绑定点代理
		// 经 frameProxy(currentFrame()) 现取（boundProxy 为会话内通道，门面无此名），
		// 换帧/重装载后必须重取。SolidColor 伪变量并入同链：旧外壳
		// setCurFrameDataProxy 注入路径随移除集消亡，装载工厂不依赖变量类型
		auto session = rebuildPostRend->getPostRender();
		mxr::PostFrameDataProxy* frameData = nullptr;
		{
			if (!session->attachModel(modeldb.data(), rebuildPostRend->getViewportName()))
				return;
			session->requestFrames(fi, fi);
			if (!session->seekFrame(fi))
			{
				// V-3 驱动点就绪整改：未就绪帧保持上一帧显示，装载完成后重入收敛
				if (_cloudFrameReadyConn)
					disconnect(_cloudFrameReadyConn);
				_cloudFrameReadyConn = connect(session.get(),
					&mxr::REBUILD_PostRender::frameReady, this,
					[this, fi](int readyFrame)
					{
						if (readyFrame != fi) return;
						if (_cloudFrameReadyConn)
						{
							disconnect(_cloudFrameReadyConn);
							_cloudFrameReadyConn = QMetaObject::Connection();
						}
						slot_showCloud();
					});
				return;
			}
			frameData = session->frameProxy(session->currentFrame());
		}

		QPointer<PostData::mPostModelData> modelData = modeldb->getModelData(fi);
		if (modelData == nullptr)
			return;

		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(frameData->getRendQueryDataProxy());
		if (query_proxy != nullptr)
		{
			for (auto it : modeldb->getAllRigidWallDatas())
			{
				QString name = it->getRigidWallName();
				QVector3D endPos = modelData->_frameInfo.getRigidWallEndPos(name);
				it->updatePos(endPos);
				query_proxy->appendRigidWallData(it);
			}
		}
		if (!isSolidColor)
		{
			// S5 门面直接替换：绑定已由 seekFrame 内部消化（displayCurrentFrame 同构
			// 路径：色标推最值+注册+绑定+副作用重放）
			// B1b(R18-2①)：原 UpdateAllRigidWallOperate(ImportOperate) 重放链实读断裂
			// （会话 UpdateAllRigidWallOperateInternal 调 updateOneRigidWallOperate→
			// updateOneModelOperate 分支表无 ImportOperate=静默 no-op）——改走 GUI 可达
			// 替代入口：外壳 UpdateOneRigidWallOperate(ReplaceOnePartOperate, names)→
			// updateOneModelOperate ReplaceOnePart 分支，从 query_proxy 现读（已含
			// append 数据）逐墙重建 partRender。
			// 【残差移交项（渲染库一行）】无限大平面墙精化面片（calculateRigidWallData
			// 的 PlanPointPos 写回）仍不可达——根治=UpdateAllRigidWallOperateInternal
			// 改调 updateAllRigidWallOperate（触 calculateRigidWallData，05 R18 节）；
			// 渲染库修复落地后本段改回单发 UpdateAllRigidWallOperate(ImportOperate)
			{
				std::set<QString> appendedWallNames;
				for (auto it : modeldb->getAllRigidWallDatas())
					appendedWallNames.insert(it->getRigidWallName());
				if (!appendedWallNames.empty())
					// S5 G5（D9 载体）：RigidWall=登记型——寻/登台账 id 后 updateObject
					// ReplaceOnePartOperate 逐墙重建（幂等覆盖；渲染库修复落地后改回单发
					// UpdateAllRigidWallOperate(ImportOperate)，B1b(R18-2①) 移交依赖不变）
					PostCommon::updateRigidWallOperate(session.get(), (int)mxr::ReplaceOnePartOperate,
						appendedWallNames);
			}
		}

		//显示自由节点
		if (guidata->_cloudShowFreeNode && !guidata->_cloudDeleteFreeNode)
		{
			modeldb->showFreeNode(guidata->_cloudFrame, guidata->_cloudRelatedCloud);
			postRender->updateAllModelOperate(ShowFreeNode);
		}
		else if (!guidata->_cloudShowFreeNode)
		{
			modeldb->hideFreeNode(guidata->_cloudFrame);
			postRender->updateAllModelOperate(ClearFreeNode);
		}
		else if (guidata->_cloudDeleteFreeNode)
		{
			modeldb->deleteFreeNode(guidata->_cloudFrame);
			postRender->updateAllModelOperate(ClearFreeNode);
		}

		//数据读取完成,渲染窗口可接入变量数据（B1b：统一从 frameData 现取，会话路径
		//下 getCurFrameDataProxy 与 boundProxy 同源，但 F8 纪律要求不依赖二次查询）
		mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(frameData->getModelRenderDataProxy());
		if (model_proxy != nullptr) { model_proxy->updateValue(); }

		postRender->updateAllModelOperate(UpdateVariableValue);
		postRender->updateAllModelOperate(UpdateMinMax);
		rebuildPostRend->update();

		if (isSolidColor)
		{
			//伪变量SolidColor：不显示云图，按部件自身颜色渲染并隐藏色标
			postRender->updateAllModelOperate(ShowSolidColor);
			postRender->getColorTableWindow()->setVisiable(false);
			postRender->updateAllModelOperate(ColorAllPartOperate);
		}
		else
		{
			QList<PostVariableInfo> varInfos = modelData->_frameInfo._frameVars;
			for (auto it : varInfos)
			{
				if (it._varName == varstr)
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

			postRender->getColorTableWindow()->setVisiable(true);
		}
		std::set<int> hideParts = getCVM->getModelPartVisualData(rebuildPostRend->getViewportName());
		auto id_all = query_proxy->getAllPartID();

		if (auto* stateEditor = query_proxy->partStateEditor())
		{
			stateEditor->setVisibleParts(id_all);
			stateEditor->setInvisibleParts(hideParts);
		}
		getCVM->updateAllPostView();
		for (auto id : hideParts)
		{
			modelData->setMeshPartVisual(rebuildPostRend->getViewportName(), id, false);
		}

		//SolidColor无结果数据，跳过范围计算与色标数值更新
		if (!isSolidColor)
		{
		//设置显示范围
		float minval = FLT_MAX;
		float maxval = -FLT_MAX;

		int id = guidata->_rangeIndex;
		if (id == 0)
		{
			maxval = query_proxy->getOriginalMaxData();
			minval = query_proxy->getOriginalMinData();
			//_editMax->blockSignals(true);
			//_editMax->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMin->blockSignals(false);
			//_editMin->blockSignals(false);
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			// 色表值域收编（C4 双效点：与同函数尾段 setMinMaxData 伴生——窗写段，实例写段见尾段）
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			this->slot_setMostValue(false);
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}
		else if (id == 1)
		{
			auto cur_min_max = model_proxy->getPartMinMax();
			minval = cur_min_max.first;
			maxval = cur_min_max.second;

			//_editMax->blockSignals(true);
			//_editMin->blockSignals(true);
   //         _editMax->setText(MxFunctions::mNumberToString(maxval, 'f', 12));
   //         _editMin->setText(MxFunctions::mNumberToString(minval, 'f', 12));
			//_editMax->blockSignals(false);
			//_editMin->blockSignals(false);
			query_proxy->setCurrentMinData(minval);
			query_proxy->setCurrentMaxData(maxval);
			// 色表值域收编（C4 双效点）：默认值域段+窗写段一口两掩码（顺序按现状），实例写段见尾段
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_DefaultRange
					| mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			this->slot_setMostValue(false);
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}

		// 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
		{
			mxr::ColorLegendParams _lg;
			_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
			_lg.valueMin = minval;
			_lg.valueMax = maxval;
			PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
		}
		postRender->updateAllModelOperate(ColorAllPartOperate);

		float maxvalue = query_proxy->getOriginalMaxData();
		float minvalue = query_proxy->getOriginalMinData();
		guidata->_cloudDefualtMax = maxvalue;
		guidata->_cloudDefualtMin = minvalue;
		}

		////更新视图文本
		//mPostFrameText* viewtext = postRender->getPostFrameText();
		//if (viewtext != nullptr)
		//{
		//	QString model = modeldb->getName();
		//	QString casetext = modeldb->getFrameCase(fi);
		//	QString framename = modeldb->getFrameNameByFrameNum(fi);
		//	mPostModelData* modeldata = modeldb->getModelData(fi);
		//	QString type, timestr;
		//	if (!modeldata->_frameInfo._isFrequency)
		//	{
		//		type = "Time";
		//		double time = modeldb->getFrameTime(fi);
		//		timestr = MxFunctions::mNumberToString(time, 'E', 6);
		//	}
		//	else
		//	{
		//		type = "Frequency";
		//		double fre = modeldata->_frameInfo._frequency._modalFreq;
		//		timestr = MxFunctions::mNumberToString(fre, 'E', 6);
		//	}

		//	viewtext->setTopRightText(4, QString("Model:%1 Case:%2")
		//		.arg(model).arg(casetext));
		//	viewtext->setTopRightText(5, QString("Frame:%3 %4:%4")
		//		.arg(framename).arg(type).arg(timestr));
		//	postRender->update();
		//}

		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
        if (guidata->_cloudLayerName.isEmpty())
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        else
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            if (!guidata->_cloudLayerName.isEmpty()) {
                title += " : " + guidata->_cloudLayerName;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }

		//桥梁杆实体化显示
		if (guidata->_cloudShowSBR)
			postRender->updateAllModelOperate(ShowBeamShellVolume);
		else
			postRender->updateAllModelOperate(ClearBeamShellVolume);

		//设置显示模式
        setShowMode();

		//设置点大小为10
		this->slot_setPointSize(guidata->_pointSize);

		mMechGlobalSignals::getInstance()->updateResultInfoGUI();

		// 通知标注面板等跟随云图当前帧刷新（与 mPost3DResult slot_showCloud/slot_changedFrame
		// 尾部同款：全部显示状态就绪后补发，测量面板经 Queued + 帧号去重消费）
		mMechGlobalSignals::getInstance()->postCloudFrameChanged(guidata->_cloudFrame);
	}

	void mPostCloudMenuWidget::slot_setMostValue(bool flag)
	{
		////获取最大值
		//double maxval = _editMax->text().trimmed().toFloat();
		//double minval = _editMin->text().trimmed().toFloat();
		//if (minval == FLT_MAX)
		//{
		//	maxval = 0.0f;
		//	minval = 0.0f;
		//}
		//if (maxval < minval) {
		//	mGlobalSignals::getInstance()->outputMessageSig(2, QString("最大值不能小于最小值！"), true);
		//	return;
		//}

		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;

		auto  postRender = postRend->getPostRender();
		if (postRender == nullptr) return;

		//存储界面信息
		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr)
			return;

        //获取最大值
        double maxval = guidata->_cloudMaxValue;
        double minval = guidata->_cloudMinValue;
        if (minval == FLT_MAX)
        {
            maxval = 0.0f;
            minval = 0.0f;
        }
        //非有限值（哨兵/±inf）按无数据处理，不参与上下限比较
        if (!std::isfinite(maxval) || !std::isfinite(minval))
        {
            maxval = 0.0;
            minval = 0.0;
        }
        if (maxval < minval) {
            mGlobalSignals::getInstance()->outputMessageSig(2, QString("最大值不能小于最小值！"), true);
            return;
        }

		mxr::IPostRendQueryDataProxy* query_proxy1 = dynamic_cast<mxr::IPostRendQueryDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getRendQueryDataProxy());
		if (query_proxy1 == nullptr)
			return;
		if (flag)
		{
			query_proxy1->setCurrentMaxData(maxval);
			query_proxy1->setCurrentMinData(minval);
			// 色表值域收编（C4 双效点）：实例写段+窗写段两笔（顺序按现状）
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			{
				mxr::ColorLegendParams _lg;
				_lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
				_lg.valueMin = minval;
				_lg.valueMax = maxval;
				PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			if (guidata != nullptr)
			{
				guidata->_cloudMaxValue = maxval;
				guidata->_cloudMinValue = minval;
			}
		}
		QString postname = postRend->getPostDataName();
		QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);

		if (modeldb == nullptr)
			return;

		postRender->getColorTableWindow()->SetTitle(0, "MxSim");
        if (guidata->_cloudLayerName.isEmpty())
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        else
        {
            QString title = guidata->_cloudVarName;
            if (!guidata->_cloudCompo.isEmpty()) {
                title += " - " + guidata->_cloudCompo;
            }
            if (!guidata->_cloudLayerName.isEmpty()) {
                title += " : " + guidata->_cloudLayerName;
            }
            postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
        }
        QString title = guidata->_cloudVarName;
        if (!guidata->_cloudCompo.isEmpty()) {
            title += " - " + guidata->_cloudCompo;
        }
        postRender->getColorTableWindow()->SetTitle(1, title.toStdString().c_str());
       

		mxr::IPostModelRenderDataProxy* model_query_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy());
		if (model_query_proxy != nullptr)
		{
			mxr::PostNodeOrElement nodeOrElement = query_proxy1->getNodeOrElement();
			std::pair<int, float> maxid = model_query_proxy->getOneTotalMaxID();
			std::pair<int, float> minid = model_query_proxy->getOneTotalMinID();
			if (nodeOrElement == mxr::PostNodeOrElement::PostElement)
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Elem# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Elem# %d", minid.first);
			}
			else
			{
				postRender->getColorTableWindow()->setMaxFootLabel(0, "max: %E", query_proxy1->getCurrentMaxData());
				postRender->getColorTableWindow()->setMaxFootLabel(1, "at Node# %d", maxid.first);
				postRender->getColorTableWindow()->setMinFootLabel(2, "min: %E", query_proxy1->getCurrentMinData());
				postRender->getColorTableWindow()->setMinFootLabel(3, "at Node# %d", minid.first);
			}
		}
		postRend->updatePaint();
		mMechGlobalSignals::getInstance()->updateResultInfoGUI();
	}

	void mPostCloudMenuWidget::setCurViewName(const QString & viewName)
	{
		_curViewName = viewName;
	}

	QStringList mPostCloudMenuWidget::sortQStringList(const QStringList& frameIds)
	{
		// 使用自定义的比较函数进行排序
		QStringList sortedList = frameIds;
		std::sort(sortedList.begin(), sortedList.end(), [](const QString& a, const QString& b) {
			// 将字符串转换为整数，然后比较大小
			int intA = a.split(":").at(0).toInt();
			int intB = b.split(":").at(0).toInt();
			return intA < intB;  // 从小到大排序
		});
		return sortedList;
	}

	void mPostCloudMenuWidget::drawCloudMap()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderByViewName(_curViewName);
		if (postRend == nullptr) return;
		if (_isDrawCloudMap)
			this->slot_showCloud();
	}

	bool mPostCloudMenuWidget::isContainStr(const QStringList& names, QString str, QString& name)
	{
		for (auto it : names)
		{
			if (it.contains(str, Qt::CaseInsensitive))
			{
				name = it;
				return true;
			}
		}
		return false;
	}

    void mPostCloudMenuWidget::setShowMode()
    {
        REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
        if (postRend == nullptr) return;

        auto  postRender = postRend->getPostRender();
        if (postRender == nullptr) return;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr) return;
        switch (viewportPrivate->_showViewCheckedStatus)
        {
        case 0:postRender->setShowFuntion(ShowFuntion::WireEdge);
            break;
        case 1:postRender->setShowFuntion(ShowFuntion::WireFrame);
            break;
        case 2:postRender->setShowFuntion(ShowFuntion::ElementFace);
            break;
        case 3:postRender->setShowFuntion(ShowFuntion::SmoothShaded);
            break;
        }
        postRend->updatePaint();
    }
}
