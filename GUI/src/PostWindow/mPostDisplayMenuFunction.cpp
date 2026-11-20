#include "mPostDisplayMenuFunction.h"
#include "mCommonFunctions.h"   // unify-addobject-arch：色表值域段经 ColorLegend 台账下发
#include "mpostwindow_global.h"
#include <cmath>

//视口管理器/后处理GUI数据（PreData，前后处理共用）
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostGUIData.h"
#include "PickCommonFunction.h"

//全局信号
#include "mGlobalSignals.h"

//模型数据/结果数据库（RebuildBasicData）
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostParseMesh.h"
#include "mPhysicalFieldEnum.h"

//渲染 SDK
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/mBaseRend.h"
#include "mxr/basic/CameraController.h"
#include "mxr/data/SelectDataStruct.h"

//与 mPostTreeSetOps.cpp 的引用环境保持一致（mPostParseMesh 在 PostData、
//操作枚举与选择数据结构在 mxr、PickDataType 在 MxFunctions）
using namespace MxFunctions;
using namespace mxr;
using namespace PostData;

namespace PostWindow
{
	mPostDisplayMenuFunction* mPostDisplayMenuFunction::getInstance()
	{
		static mPostDisplayMenuFunction instance;
		return &instance;
	}

	mPostDisplayMenuFunction::mPostDisplayMenuFunction(QObject* parent) : QObject(parent)
	{
	}

	mPostDisplayMenuFunction::~mPostDisplayMenuFunction()
	{
	}

	bool mPostDisplayMenuFunction::getPostContext(mxr::REBUILD_PostRend*& postRend, mxr::REBUILD_PostRender*& postRender,
		MechPreData::mPostGUIData*& guidata, QPointer<PostData::mPostDataBase>& postDB,
		PostData::mPostModelData*& modelData)
	{
		postRend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
		if (postRend == nullptr) return false;

		//getPostRender返回shared_ptr（视口成员持有，生命周期随视口），此处取裸指针使用
		postRender = postRend->getPostRender().get();
		if (postRender == nullptr) return false;

		guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return false;

		postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(MxFunctions::Physical_Post)
			.getPostDatabase(postRend->getPostDataName());
		if (postDB == nullptr) return false;

		modelData = postDB->getModelData(guidata->_cloudFrame);
		if (modelData == nullptr) return false;

		return true;
	}

	void mPostDisplayMenuFunction::slot_changeViewList(bool next)
	{
		if (!getCVM)
		{
			return;
		}
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		mxr::mBaseRend* rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		auto cameraController = rend->getCameraController();
		if (next)
		{
			cameraController->nextView();
		}
		else
		{
			cameraController->lastView();
		}
		VMManager->setCameraSyncTimer(500, 30, [rend]() {
			emit rend->signal_CameraSync();
			});
	}

	void mPostDisplayMenuFunction::slot_angleRotation(QString axis, double value)
	{
		if (!getCVM)
		{
			return;
		}
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		mxr::mBaseRend* rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		if (axis.contains("Sz"))
		{
			if (axis.contains("-"))
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::Z_AXIS_MINUS);
			}
			else
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::Z_AXIS_PLUS);
			}
		}
		else if (axis.contains("Sy"))
		{
			if (axis.contains("-"))
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::Y_AXIS_MINUS);
			}
			else
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::Y_AXIS_PLUS);
			}
		}
		else if (axis.contains("Sx"))
		{
			if (axis.contains("-"))
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::X_AXIS_MINUS);
			}
			else
			{
				rend->slotSetRotate_ByButton(value, ModeGeomTransf::X_AXIS_PLUS);
			}
		}
		rend->updatePaint();
	}

	void mPostDisplayMenuFunction::slot_directionPan(QString direction, double value)
	{
		if (!getCVM)
		{
			return;
		}
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		mxr::mBaseRend* rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		if (direction.contains("up"))
		{
			rend->slotSetTranslate_ByButton(0, rend->height() * value / 200.0, ModeGeomTransf::UP_DOWN_MOVE);
		}
		else if (direction.contains("down"))
		{
			rend->slotSetTranslate_ByButton(0, -rend->height() * value / 200.0, ModeGeomTransf::UP_DOWN_MOVE);
		}
		else if (direction.contains("left"))
		{
			rend->slotSetTranslate_ByButton(-rend->width() * value / 200.0, 0, ModeGeomTransf::LEFT_RIGHT_MOVE);
		}
		else if (direction.contains("right"))
		{
			rend->slotSetTranslate_ByButton(rend->width() * value / 200.0, 0, ModeGeomTransf::LEFT_RIGHT_MOVE);
		}
		rend->updatePaint();
	}

	void mPostDisplayMenuFunction::slot_valueZoom(bool zoomin, double value)
	{
		if (!getCVM)
		{
			return;
		}
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		mxr::mBaseRend* rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		auto cameraController = rend->getCameraController();
		auto boundingSphere = cameraController->getBoundingSphere();
		double currentPercent = ((boundingSphere.valid() && boundingSphere.radius() > 0) ? boundingSphere.radius() : 100.0) * 100 / cameraController->getCameraState().ortho_zoom;
		if (zoomin)
		{
			rend->SetZoomAtViewCenter_ByPercent(currentPercent * value);
		}
		else
		{
			rend->SetZoomAtViewCenter_ByPercent(currentPercent / value);
		}
		VMManager->setCameraSyncTimer(500, 30, [rend]() {
			emit rend->signal_CameraSync();
			});
		rend->updatePaint();
	}

	void mPostDisplayMenuFunction::slot_setAllBeamVolume(bool show)
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		std::set<int> parts = modelData->getAllPartIDs();

		auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
		if (currentViewportPrivate == nullptr) return;
		if (show)
		{
			currentViewportPrivate->_entityBeam.clear();
			currentViewportPrivate->_entityBeam.insert(parts.begin(), parts.end());
			postRender->updateOneModelOperate({ mxr::ShowBeamVolume, parts });
		}
		else
		{
			currentViewportPrivate->_entityBeam.clear();
			postRender->updateOneModelOperate({ mxr::ClearBeamVolume, {} });
		}

		postRend->updatePaint();
	}

	void mPostDisplayMenuFunction::slot_setAllRodVolume(bool show)
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		std::set<int> parts = modelData->getAllPartIDs();

		auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
		if (currentViewportPrivate == nullptr) return;
		if (show)
		{
			currentViewportPrivate->_entityRod.clear();
			currentViewportPrivate->_entityRod.insert(parts.begin(), parts.end());
			postRender->updateOneModelOperate({ mxr::ShowBarVolume, parts });
		}
		else
		{
			currentViewportPrivate->_entityRod.clear();
			postRender->updateOneModelOperate({ mxr::ClearBarVolume, {} });
		}

		postRend->updatePaint();
	}

	void mPostDisplayMenuFunction::rebuildModelRender(PostData::mPostModelData* modelData, mxr::REBUILD_PostRender* postRender,
		mxr::REBUILD_PostRend* postRend, const std::set<int>& reclassifyParts)
	{
		//受影响部件面拓扑重分类（修：单元显隐变化后模型出现空洞）。清面数据使
		//ParseMesh 强制重收录（绕过 faceBuilt 守卫），边界/内部按当前显隐重算；
		//空集=仅重建渲染不改拓扑
		for (int partId : reclassifyParts)
		{
			QPointer<PostData::mPostPartData> part = modelData->getPartData(partId);
			if (part == nullptr) continue;
			auto& fd = part->_partfacedata;
			fd.rep_face_node_ids.clear();
			fd.rep_face_elem_ids.clear();
			fd.face_node_ids.clear();
			fd.face_elem_id.clear();
			fd.rep_face_node_counts.clear();
			fd.face_node_starts.clear();
			fd.face_node_counts.clear();
			fd.face_is_polyhedral.clear();
			fd.face_is_show.clear();
			fd.face_owner1.clear();
			fd.face_owner1_local.clear();
			fd.face_owner2.clear();
			fd.face_owner2_local.clear();
			fd.boundary_face_count = 0;
			fd.inner_face_count = 0;
			fd.elem_faces.clear();
			fd.node_faces.clear();
		}

		mPostParseMesh* parse = new mPostParseMesh(this);
		parse->ParseMesh(modelData);

		auto* rebuildProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
		if (rebuildProxy != nullptr)
			rebuildProxy->getModelRenderDataProxy()->RebuildModelData();
		postRender->updateAllModelOperate(mxr::ImportOperate);
		postRender->updateAllModelOperate(mxr::UpdateMinMax);
		//update() 只是异步调度，GL 后端要等下一次事件才重绘；updatePaint 立即渲染
		postRend->updatePaint();

		delete parse;
	}

	void mPostDisplayMenuFunction::slot_maskModel()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		int pickNum = 0;
		PickDataType pickType = getCVM->getPostPickDataType(pickNum);
		if (pickNum == 0) return;

		if (pickType == PickDataType::Element)
		{
			std::vector<MeshSelectInfoData> res = postRend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
			QHash<int, QList<int>> partEles{};
			for (auto r : res)
			{
				if (partEles.constFind(r.part_id) != partEles.cend())
				{
					partEles[r.part_id].append(r.data_id.element_id);
				}
				else
				{
					QList<int> eleids{};
					eleids.append(r.data_id.element_id);
					partEles.insert(r.part_id, eleids);
				}
			}

			//受影响部件=拾取所在部件：面拓扑按新显隐重分类（修：隐藏后邻接面
			//残留"内部面"分类→穿透空洞）
			std::set<int> affected;
			auto it = partEles.begin();
			while (it != partEles.end())
			{
				postDB->setEleVisual(postRend->getViewportName(), it.key(), it.value(), false);
				affected.insert(it.key());
				++it;
			}

			rebuildModelRender(modelData, postRender, postRend, affected);
			updateMaxAndMinData();
		}
		getCVM->clearPick();
	}

	void mPostDisplayMenuFunction::slot_maskOtherModel()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		//先校验拾取：无拾取时不做"先全部隐藏"的中间状态变更
		int pickNum = 0;
		PickDataType pickType = getCVM->getPostPickDataType(pickNum);
		if (pickNum == 0 || pickType != PickDataType::Element) return;

		//隐藏其他 = 全显→反转（全隐）→再显示拾取的单元
		postDB->setAllEleShow(postRend->getViewportName());
		modelData->setEleReserveShow(postRend->getViewportName());

		std::vector<MeshSelectInfoData> res = postRend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
		QHash<int, QList<int>> partEles{};
		for (auto r : res)
		{
			if (partEles.constFind(r.part_id) != partEles.cend())
			{
				partEles[r.part_id].append(r.data_id.element_id);
			}
			else
			{
				QList<int> eleids{};
				eleids.append(r.data_id.element_id);
				partEles.insert(r.part_id, eleids);
			}
		}

		auto it = partEles.begin();
		while (it != partEles.end())
		{
			postDB->setEleVisual(postRend->getViewportName(), it.key(), it.value(), true);
			++it;
		}

		//全部件单元显隐都变了：整模型重分类（边界/内部按当前显隐重算，空洞消除）
		rebuildModelRender(modelData, postRender, postRend, modelData->getAllPartIDs());
		updateMaxAndMinData();

		getCVM->clearPick();
	}

	void mPostDisplayMenuFunction::slot_reverseMaskModel()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		modelData->setEleReserveShow(postRend->getViewportName());
		postRender->updateHighLightRender();

		//全部件单元显隐都变了：整模型重分类（边界/内部按当前显隐重算）
		rebuildModelRender(modelData, postRender, postRend, modelData->getAllPartIDs());
		updateMaxAndMinData();

		getCVM->clearPick();
	}

	void mPostDisplayMenuFunction::slot_showAllModel()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		mxr::REBUILD_PostRender* postRender = nullptr;
		MechPreData::mPostGUIData* guidata = nullptr;
		QPointer<PostData::mPostDataBase> postDB;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostContext(postRend, postRender, guidata, postDB, modelData))
			return;

		//受影响部件=此前存在隐藏单元的部件（与树侧"显示全部"同口径：
		//集合/按钮隐藏写在单元级，全显后这些部件需按新显隐重分类）
		std::set<int> affected;
		for (int partId : modelData->getAllPartIDs())
		{
			QPointer<PostData::mPostPartData> partData = modelData->getPartData(partId);
			if (partData != nullptr && partData->getMaskEleNum() > 0)
				affected.insert(partId);
		}

		postDB->setAllEleShow(postRend->getViewportName());

		rebuildModelRender(modelData, postRender, postRend, affected);
		updateMaxAndMinData();

		getCVM->clearPick();
	}

	void mPostDisplayMenuFunction::updateMaxAndMinData()
	{
		mxr::REBUILD_PostRend* postRend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
		if (postRend == nullptr) return;
		auto postRender = postRend->getPostRender();
		if (postRender == nullptr) return;
		// S5 门面直接替换：getCurFrameDataProxy 消亡→frameProxy(currentFrame()) 现取（F8：换帧/重装载后重取）
		mxr::PostFrameDataProxy* curFrameProxy = postRender->frameProxy(postRender->currentFrame());
		if (curFrameProxy == nullptr) return;
		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(curFrameProxy->getRendQueryDataProxy());

		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata != nullptr)
		{
			float maxval;
			float minval;
			if (guidata->_rangeIndex == 0)
			{
				maxval = query_proxy->getOriginalMaxData();
				minval = query_proxy->getOriginalMinData();
			}
			else if (guidata->_rangeIndex == 1)
			{
				mxr::IPostModelRenderDataProxy* model_proxy = dynamic_cast<mxr::IPostModelRenderDataProxy*>(curFrameProxy->getModelRenderDataProxy());
				if (model_proxy == nullptr)
				{
					return;
				}
				auto cur_min_max = model_proxy->getPartMinMax();
				minval = cur_min_max.first;
				maxval = cur_min_max.second;
			}
			else
			{
				return;
			}
			//值域非法（倒挂/非有限）＝数据未就绪：静默跳过，不弹"最大值不能小于最小值"打扰
			if (!(maxval >= minval) || !std::isfinite(maxval) || !std::isfinite(minval))
			{
				return;
			}
			{   // 现状首笔窗写（原行含 S5 备注则随段注保留）
			    // 色表值域收编（C4 双效点·窗写段，与本函数伴生 setMinMaxData 成对）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRangeWindowOnly;
			    _lg.targetFrame = -1;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			{
			    // 色表值域收编（C4 双效点·实例写段）：旧 setMinMaxData 原体（含窗写+刷新链）
			    mxr::ColorLegendParams _lg;
			    _lg.updateMask = mxr::ColorLegendParams::Mask_ValueRange;
			    _lg.valueMin = minval;
			    _lg.valueMax = maxval;
			    PostCommon::ensureUpdateSceneObject(postRender.get(), _lg);
			}
			postRend->update();
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}
	}
}
