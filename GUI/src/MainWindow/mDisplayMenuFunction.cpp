#include "mDisplayMenuFunction.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mTitleBar.h"
#include "mCustomTooltip.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QVector3D>
#include <qmath.h>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/mFontRender.h"
#include "mxr/view/mShapeRender.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/basic/CameraController.h"
#include "mxr/gizmo/GizmoManager.h"

#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
//【镜像六件套摘除】mGeo*Data1.h 已随 RebuildRender 删除；元素显隐读写改走
//前处理契约代理（elementIndex 反查部件）+ GN 部件属性（渲染重建批次的显隐真源）
#include "mxr/data/contracts/IPreDataProxy.h"

//BasicData
#include "mPropertyBase.h"
#include "mPropertyManager.h"
#include "mTorsionBar.h"
#include "mTube.h"
#include "mSolidHomo.h"
#include "mShellHomo.h"
#include "mConstantBeam.h"
#include "mCrossBeam.h"
#include "mShearPlate.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mSectionShapeManager.h"
#include "mSectionShapeBase.h"
#include "mConnectorBase.h"
#include "mConnectorManager.h"
#include "mElementBase.h"
#include "mDataElement.h"
 
//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】死 include（零调用）

//Post
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPostParseMesh.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/data/IPostRenderDataProxy.h"

#define ep 1e-3
using namespace mxr;
using namespace MechPreData;
using namespace PostData;
using namespace MechConnectorData;
using namespace MechElementData;
namespace MechMainWindow
{
	mDisplayMenuFunction* mDisplayMenuFunction::_instance = nullptr;
	mDisplayMenuFunction* mDisplayMenuFunction::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mDisplayMenuFunction;
		}
		return _instance;
	}

	mDisplayMenuFunction::mDisplayMenuFunction(QWidget* parent) :QObject(parent)
	{
		//【集成适配】dev 版信号为 currentViewportChanged(int)（源版为 QMdiSubWindow*）；
		// 槽体仅判空当前视口，int 参数走 lambda 空指针适配
		connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged,
			this, [this](int) { slot_subWindowActivated(nullptr); });

	}

	void mDisplayMenuFunction::clearHighlight()
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
		if (viewportPrivate->getViewportType() == Viewport3D)
		{

			ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
			ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
			getCVM->updateRender();

		}
		else if (viewportPrivate->getViewportType() == ViewportPost3D)
		{

		}
	}

	void mDisplayMenuFunction::slot_changeViewList(bool next)
	{
		if (!getCVM)
		{
			return;
		}
		mBaseRend* rend = nullptr;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
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

	void mDisplayMenuFunction::slot_angleRotation(QString axis, double value)
	{
		if (!getCVM)
		{
			return;
		}
		mBaseRend* rend = nullptr;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}
		auto cameraController = rend->getCameraController();
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

	void mDisplayMenuFunction::slot_directionPan(QString direction, double value)
	{
		if (!getCVM)
		{
			return;
		}
		mBaseRend* rend = nullptr;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
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
		//slotSetTranslate_ByButton内部不发signal_CameraSync，视口联动需由界面补发（同slot_valueZoom）
		VMManager->setCameraSyncTimer(500, 30, [rend]() {
			emit rend->signal_CameraSync();
			});
		rend->updatePaint();
	}

	void mDisplayMenuFunction::slot_valueZoom(bool zoomin, double value)
	{
		if (!getCVM)
		{
			return;
		}
		mBaseRend* rend = nullptr;

		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
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

	void mDisplayMenuFunction::slot_autoRotation(QString axis, bool enable)
	{
		if (!getCVM)
		{
			return;
		}
		if (enable)
		{
			if (conn_autoRotation)
			{
				disconnect(conn_autoRotation);
			}
			double value = 1;

			conn_autoRotation = connect(&_autoRotationTimer, &QTimer::timeout, this, [this, axis, value](){
					slot_angleRotation(axis, value);
				}, Qt::UniqueConnection);
			_autoRotationTimer.start(10);
		}
		else
		{
			if (conn_autoRotation)
			{
				disconnect(conn_autoRotation);
			}
			_autoRotationTimer.stop();
		}
	}

	void mDisplayMenuFunction::slot_setPostAllBeamVisble(bool show)
	{
		if (!getCVM)
		{
			return;
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			std::set<int>  parts = modelData->getAllPartIDs();

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			if (show)
			{
				currentViewportPrivate->_entityBeam.clear();
				currentViewportPrivate->_entityBeam.insert(parts.begin(), parts.end());
				postRender->updateOneModelOperate({ ShowBeamVolume, parts });
			}
			else
			{
				currentViewportPrivate->_entityShell.clear();
				postRender->updateOneModelOperate({ ClearBeamVolume, {} });
			}

			postRend->updatePaint();
		}
	}

	void mDisplayMenuFunction::slot_setPostAllRodVisble(bool show)
	{
		if (!getCVM)
		{
			return;
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			std::set<int>  parts = modelData->getAllPartIDs();

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			if (show)
			{
				currentViewportPrivate->_entityRod.clear();
				currentViewportPrivate->_entityRod.insert(parts.begin(), parts.end());
				postRender->updateOneModelOperate({ ShowBarVolume, parts });
			}
			else
			{
				currentViewportPrivate->_entityRod.clear();
				postRender->updateOneModelOperate({ ClearBarVolume, {} });
			}

			postRend->updatePaint();
		}
	}

	void mDisplayMenuFunction::slot_setAllShellVisble(bool show)
	{
		if (!getCVM)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto viewportPrivate = VMManager->getCurrentViewportPrivate();
			if (viewportPrivate == nullptr)
			{
				return;
			}
			auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
			auto propertyIds = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getAllProperyID();
			if (show)
			{
				viewportPrivate->_entityShell.clear();

				for (auto propertyId : propertyIds)
				{
					auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propertyId);
					if (propertyData == nullptr)
					{
						continue;
					}
					PropertyType type = propertyData->getPropertyType();
					//if (type != PlaneStrainProperty && type != PlaneStressProperty && type != ShellProperty && type != ShearPlateProperty)
					//{
					//	continue;
					//}
					if (type != ShellProperty)
					{
						continue;
					}
					auto partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, propertyId }, Physical_Part);
					double thickness = propertyData->getThickness().toDouble();
					for (auto part_id : partList)
					{
						viewportPrivate->_entityShell.insert(part_id);
						getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateShell(QString("shell_%1").arg(part_id), part_id, thickness);
					}
				}
			}
			else
			{
				for (auto partId : viewportPrivate->_entityShell)
				{
					int propId = mapRelation.getMapRelation<int>({ Physical_Part, partId }, Physical_Property);
					auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propId);
					if (propertyData == nullptr)
					{
						continue;
					}
					getCVM->getPreRenderWidget()->GetBeamSystem()->deleteShell(QString("shell_%1").arg(partId));
				}
				viewportPrivate->_entityShell.clear();
			}

			getCVM->getPreRenderWidget()->updatePaint();
		}
		
		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			std::set<int>  parts = modelData->getAllPartIDs();

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			if (show)
			{
				currentViewportPrivate->_entityShell.clear();
				currentViewportPrivate->_entityShell.insert(parts.begin(), parts.end());
				postRender->updateOneModelOperate({ ShowShellVolume, parts });
			}
			else
			{
				currentViewportPrivate->_entityShell.clear();
				postRender->updateOneModelOperate({ ClearShellVolume, {} });
			}

			postRend->updatePaint();
		}
	}

	void mDisplayMenuFunction::slot_setAllRodBeamVisble(bool show, MxFunctions::PropertyType type)
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
		if (type != RodProperty && type != ResultantProperty)
		{
			return;
		}
        mDataElement* elementData = getCVM->getModelData()->getElementData();
        mMeshData* meshData = getCVM->getModelData()->getMeshData();
		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		auto propertyNames = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getAllPropertyNamesByTypes(type);
		if (show)
		{
			viewportPrivate->_entityRodBeam.clear();

			for (auto propertyName : propertyNames)
			{
				auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propertyName);
				if (propertyData == nullptr)
				{
					continue;
				}
				int id = propertyData->getPropertyId();
				int propShapeId = mapRelation.getMapRelation<int>({ Physical_Property, id }, Physical_PropertyShape);
				if (propShapeId == -1)
				{
					continue;
				}
				auto sectionShape = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(propShapeId);
				if (!sectionShape)
				{
					continue;
				}

				auto partList = mapRelation.getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
				propertyData->getAngle().toDouble();
				sectionShape->getSectionShapeType();
				sectionShape->getParaNoType();
				std::vector<float> params;
				const auto& up_vectors = this->getDefaultUpVector(partList.toVector().toStdVector());
				for (const auto& param : sectionShape->getParaNoType())
				{
					params.push_back(param);
				}
				for (auto part_id : partList)
				{
					viewportPrivate->_entityRodBeam.insert(part_id);
                    QString partName = meshData->getPartNameByPartId(part_id);
                    auto meshIds = meshData->getAllShow1DMeshElementID({ partName });
                    QVector<QVector3D> norms;
                    for (auto meshId : meshIds)
                    {
                        mElementBase* elementBase = elementData->getElementDataByMeshID(meshId.result_id);
                        mBeamElement* element = dynamic_cast<mBeamElement*>(elementBase);
                        if (element == nullptr)
                        {
                            norms.push_back(QVector3D(0, 0, 0));
                        }
                        else
                        {
                            if (!element->_node.isEmpty())
                            {
                                std::vector<QVector3D> eleCoor = meshData->getCoorsOfEle(meshId);
                                QVector3D nodeCoor = meshData->getVectorById(element->_node.toInt());
                                norms.push_back(QVector3D(nodeCoor.x() - eleCoor[0].x(), nodeCoor.y() - eleCoor[0].y(), nodeCoor.z() - eleCoor[0].z()));
                            }
                            else if (!element->_beamValue.isEmpty())
                            {
                                QStringList list = element->_beamValue.split(",");
                                QVector3D coor;
                                if (list.size() >= 3)
                                {
                                    coor.setX(list[0].toDouble());
                                    coor.setY(list[1].toDouble());
                                    coor.setZ(list[2].toDouble());
                                }
                                norms.push_back(coor);
                            }
                        }
                    }
					getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateBeam(QString("rod_%1").arg(part_id), part_id, static_cast<mxr::BeamSectionType>(sectionShape->getSectionShapeType()), params, mxr::pickRefsOf(meshIds), norms);
				}
			}
		}
		else
		{
			for (auto partId : viewportPrivate->_entityRodBeam)
			{
				int propId = mapRelation.getMapRelation<int>({ Physical_Part, partId }, Physical_Property);
				auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propId);
				if (propertyData == nullptr)
				{
					continue;
				}
				MxFunctions::PropertyType propertyType = propertyData->getPropertyType();
				if (type != propertyType)
				{
					continue;
				}
				getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("rod_%1").arg(partId));
			}
			viewportPrivate->_entityRodBeam.clear();
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mDisplayMenuFunction::slot_setAllSpringVisble(bool show, MxFunctions::PropertyType type)
	{
		//if (!getCVM)
		//{
		//	return;
		//}
		//QString name = "spring";
		//if (type == ViscousDampProperty)
		//{
		//	name = "damp";
		//}
		//auto viewportPrivate = VMManager->getCurrentViewportPrivate();

		//auto meshData = getCVM->getModelData()->getMeshData();
		//auto geoDatabase = getCVM->getModelData()->getGeoData();
		//auto geoManager = getCVM->getModelData()->getGeoManager();
		//if (viewportPrivate == nullptr)
		//{
		//	return;
		//}
		//if (show)
		//{
		//	if (type == ViscousDampProperty)
		//	{
		//		viewportPrivate->_entitySpring.clear();
		//	}
		//	else
		//	{
		//		viewportPrivate->_entityDamping.clear();
		//	}

  //          auto propertyNames = getCVM->getModelData()->getAllNames(Physical_Property);
  //          for (auto propertyName : propertyNames)
  //          {
  //              auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propertyName);
  //              if (propertyData == nullptr)
  //              {
  //                  continue;
  //              }

  //              int id = propertyData->getPropertyId();
  //              auto partList = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Property, id }, Physical_Part);
  //              if (propertyData->getPropertyType() == Elas1Property || propertyData->getPropertyType() == Spring1Property ||
  //                  propertyData->getPropertyType() == Spring2Property || propertyData->getPropertyType() == SpringAProperty ||
  //                  propertyData->getPropertyType() == CbushProperty)
  //              {
  //                  for (auto part_id : partList)
  //                  {
  //                      viewportPrivate->_entitySpring.insert(part_id);
  //                      //getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateBeam(QString("%1_%2").arg(name).arg(part_id), part_id);
  //                  }
  //              }
  //              else if (propertyData->getPropertyType() == ViscousDampProperty)
  //              {
  //                  for (auto part_id : partList)
  //                  {
  //                      viewportPrivate->_entityDamping.insert(part_id);
  //                      //getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateBeam(QString("%1_%2").arg(name).arg(part_id), part_id);
  //                  }
  //              }

  //          }
		//}
		//else
		//{
		//	if (type == ViscousDampProperty)
		//	{
  //              for (auto id : viewportPrivate->_entityDamping)
  //              {
  //                  getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("damp_%1").arg(id));
  //              }
  //              viewportPrivate->_entityDamping.clear();
		//	}
		//	else
		//	{
  //              for (auto id : viewportPrivate->_entitySpring)
  //              {
  //                  getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("spring_%1").arg(id));
  //              }
  //              viewportPrivate->_entitySpring.clear();
		//	}
		//}

		//getCVM->getPreRenderWidget()->updatePaint();
	}

	void mDisplayMenuFunction::slot_setEntityVisble(bool show, std::vector<int> partIds)
	{
		if (!getCVM)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto viewportPrivate = VMManager->getCurrentViewportPrivate();
			if (viewportPrivate == nullptr)
			{
				return;
			}
			auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            mDataElement* elementData = getCVM->getModelData()->getElementData();
            mMeshData* meshData = getCVM->getModelData()->getMeshData();
			if (show)
			{
				for (auto partId : partIds)
				{
					int propId = mapRelation.getMapRelation<int>({ Physical_Part, partId }, Physical_Property);
					auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property).getPropertyData(propId);
					if (propertyData == nullptr)
					{
						continue;
					}
					PropertyType type = propertyData->getPropertyType();
					if (type == ShellProperty)
					{
						viewportPrivate->_entityShell.insert(partId);
						getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateShell(QString("shell_%1").arg(partId), partId, propertyData->getThickness().toDouble());
					}
					else if (type == RodProperty || type == ResultantProperty)
					{
						int propShapeId = mapRelation.getMapRelation<int>({ Physical_Property, propId }, Physical_PropertyShape);
						if (propShapeId == -1)
						{
							continue;
						}
						auto sectionShape = getCVM->getModelData()->getManager<MechSectionShapeData::mSectionShapeManager>(Physical_PropertyShape).getSectionShapeData(propShapeId);
						if (!sectionShape)
						{
							continue;
						}
						std::vector<float> params;
						for (const auto& param : sectionShape->getParaNoType())
						{
							params.push_back(param);
						}
						viewportPrivate->_entityRodBeam.insert(partId);
                        QString partName = meshData->getPartNameByPartId(partId);
                        auto meshIds = meshData->getAllShow1DMeshElementID({ partName });
                        QVector<QVector3D> norms;
                        for (auto meshId : meshIds)
                        {
                            mElementBase* elementBase = elementData->getElementDataByMeshID(meshId.result_id);
                            mBeamElement* element = dynamic_cast<mBeamElement*>(elementBase);
                            if (element == nullptr)
                            {
                                norms.push_back(QVector3D(0, 0, 0));
                            }
                            else
                            {
                                if (!element->_node.isEmpty())
                                {
                                    std::vector<QVector3D> eleCoor = meshData->getCoorsOfEle(meshId);
                                    QVector3D nodeCoor = meshData->getVectorById(element->_node.toInt());
                                    norms.push_back(QVector3D(nodeCoor.x() - eleCoor[0].x(), nodeCoor.y() - eleCoor[0].y(), nodeCoor.z() - eleCoor[0].z()));
                                }
                                else if (!element->_beamValue.isEmpty())
                                {
                                    QStringList list = element->_beamValue.split(",");
                                    QVector3D coor;
                                    if (list.size() >= 3)
                                    {
                                        coor.setX(list[0].toDouble());
                                        coor.setY(list[1].toDouble());
                                        coor.setZ(list[2].toDouble());
                                    }
                                    norms.push_back(coor);
                                }
                            }
                        }
                        getCVM->getPreRenderWidget()->GetBeamSystem()->createOrUpdateBeam(QString("rod_%1").arg(partId), partId, static_cast<mxr::BeamSectionType>(sectionShape->getSectionShapeType()), params, mxr::pickRefsOf(meshIds), norms);
					}
				}
			}
			else
			{
				for (auto partId : partIds)
				{
					viewportPrivate->_entityRodBeam.erase(partId);
					getCVM->getPreRenderWidget()->GetBeamSystem()->deleteBeam(QString("rod_%1").arg(partId));

					viewportPrivate->_entityShell.erase(partId);
					getCVM->getPreRenderWidget()->GetBeamSystem()->deleteShell(QString("shell_%1").arg(partId));
				}
			}
			getCVM->getPreRenderWidget()->updatePaint();
		}
		
		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			std::set<int>  parts;
			parts.insert(partIds.begin(), partIds.end());
			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			if (show)
			{
				currentViewportPrivate->_entityShell.insert(parts.begin(), parts.end());
				currentViewportPrivate->_entityBeam.insert(parts.begin(), parts.end());
				currentViewportPrivate->_entityRod.insert(parts.begin(), parts.end());
				postRender->updateOneModelOperate({ ShowShellVolume, parts });
				postRender->updateOneModelOperate({ ShowBeamVolume, parts });
				postRender->updateOneModelOperate({ ShowBarVolume, parts });
			}
			else
			{
				currentViewportPrivate->_entityShell.erase(parts.begin(), parts.end());
				currentViewportPrivate->_entityBeam.erase(parts.begin(), parts.end());
				currentViewportPrivate->_entityRod.erase(parts.begin(), parts.end());

				if (currentViewportPrivate->_entityShell.size() == 0)
				{
					postRender->updateOneModelOperate({ ClearShellVolume ,{ }});
				}
				else
				{
					postRender->updateOneModelOperate({ ShowShellVolume, currentViewportPrivate->_entityShell });
				}

				if (currentViewportPrivate->_entityBeam.size() == 0)
				{
					postRender->updateOneModelOperate({ ClearBeamVolume ,{ } });
				}
				else
				{
					postRender->updateOneModelOperate({ ShowBeamVolume, currentViewportPrivate->_entityBeam });
				}


				if (currentViewportPrivate->_entityRod.size() == 0)
				{
					postRender->updateOneModelOperate({ ClearBarVolume ,{ } });
				}
				else
				{
					postRender->updateOneModelOperate({ ShowBarVolume, currentViewportPrivate->_entityRod });
				}
			}
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_showNodeEleLabel()
	{
		if (getCVM == nullptr)
		{
			return;
		}
		if (getCVM->getPreRenderWidget())
		{
			auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
				double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;

				vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
			};
			auto currentViewportPrivate = VMManager->getCurrentViewportPrivate();
			auto meshData = getCVM->getModelData()->getMeshData();
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickType != PickDataType::Node && pickType != PickDataType::Element)
			{
				return;
			}
			if (pickNum == 0)
			{
				return;
			}
			auto meshSelectData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData();
			if (pickType == PickDataType::Node)
			{
				currentViewportPrivate->_nodeIdVertexs.clear();
				std::vector<MeshSelectInfoData> res = meshSelectData->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.node_id;
					QVector3D vertex;
					getNodeVertexByID(meshData, nodeId, vertex);
					currentViewportPrivate->_nodeIdVertexs[nodeId] = vertex;
				}

			}
			else if (pickType == PickDataType::Element)
			{
				currentViewportPrivate->_eleIdVertexs.clear();
				std::vector<MeshSelectInfoData> res = meshSelectData->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
				for (int i = 0; i < pickNum; i++)
				{
					RenderPickResultData resultData;
					resultData.part_id = res.at(i).part_id;
					resultData.result_id = res.at(i).data_id.element_id;
					currentViewportPrivate->_eleIdVertexs[resultData.result_id] = meshData->getCentroid(resultData);
				}
			}
			updateNodeEleLabel();
		}
		
		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			const QHash<int, QVector3D> dis = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
			QVector3D deformationScale = guidata->_deformFactor;

			auto currentViewportPrivate = VMManager->getCurrentViewportPrivate();

			std::vector<MeshSelectInfoData> res;
			std::set<int> ids;
			int pickNum = 0;
			auto pickType = getCVM->getPostPickDataType(pickNum);
			if (pickType == PickDataType::Node)
			{
				res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				for (auto r : res)
				{
					ids.insert(r.data_id.element_id);
				}

				currentViewportPrivate->_nodeIdVertexs.clear();
				for (int id : ids)
				{
					int cid = modelData->_nodedata.nodeidmap.value(id);
					auto nodecoordinate = modelData->getVector(cid);
					QVector3D vertex = nodecoordinate + deformationScale * dis.value(cid);

					currentViewportPrivate->_nodeIdVertexs[id] = vertex;
				}
			}
			else if (pickType == PickDataType::Element)
			{
				res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
				for (auto r : res)
				{
					ids.insert(r.data_id.element_id);
				}

				currentViewportPrivate->_eleIdVertexs.clear();
				for (int id : ids)
				{
					int element = id;
					int partid = modelData->getPartIDByEleID(element);
					QList<int> nodeid = modelData->getElementNodeId(partid, element);

					QVector3D vertex;
					for (int id : nodeid)
					{
						vertex += modelData->getCoordinates(id) + deformationScale * dis.value(id);
					}
					vertex = vertex / float(nodeid.size());
					currentViewportPrivate->_eleIdVertexs[id] = vertex;
				}
			}
			updateNodeEleLabel();
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_hideNodeEleLabel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			auto currentViewportPrivate = VMManager->getCurrentViewportPrivate();
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickType != PickDataType::Node && pickType != PickDataType::Element)
			{
				return;
			}
			if (pickNum == 0)
			{
				return;
			}
			auto meshSelectData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData();
			if (pickType == PickDataType::Node)
			{
				std::vector<MeshSelectInfoData> res = meshSelectData->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					currentViewportPrivate->_nodeIdVertexs.remove(nodeId);
				}

			}
			else if (pickType == PickDataType::Element)
			{
				std::vector<MeshSelectInfoData> res = meshSelectData->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
				for (int i = 0; i < pickNum; i++)
				{
					currentViewportPrivate->_eleIdVertexs.remove(res.at(i).data_id.element_id);
				}
			}
			updateNodeEleLabel();
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();

			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			const QHash<int, QVector3D> dis = postDB->getDisplacementDataByFrameID(guidata->_cloudFrame);
			QVector3D deformationScale = guidata->_deformFactor;

			auto currentViewportPrivate = VMManager->getCurrentViewportPrivate();

			std::vector<MeshSelectInfoData> res;
			std::set<int> ids;
			int pickNum = 0;
			auto pickType = getCVM->getPostPickDataType(pickNum);
			if (pickType == PickDataType::Node)
			{
				res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					currentViewportPrivate->_nodeIdVertexs.remove(nodeId);
				}
			}
			else if (pickType == PickDataType::Element)
			{
				res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
				for (auto r : res)
				{
					ids.insert(r.data_id.element_id);
				}

				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					currentViewportPrivate->_eleIdVertexs.remove(nodeId);
				}
			}
			updateNodeEleLabel();
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_hideAllNodeEleLabel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr) return;
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr) return;

		viewportPrivate->_nodeIdVertexs.clear();
		viewportPrivate->_eleIdVertexs.clear();
		auto fontRender = rend->getFontRender();
		fontRender->clearCommonFont("NodeID");
		fontRender->clearCommonFont("EleID");
		rend->updatePaint();
	}

	void mDisplayMenuFunction::slot_maskModel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickNum == 0)
			{
				return;
			}
			if (pickType == PickDataType::Element)
			{
				std::vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
				std::vector<RenderPickResultData> maskids; maskids.reserve(pickNum);
				for (auto r : res)
				{
					RenderPickResultData data;
					data.result_id = r.data_id.element_id;
					data.part_id = r.part_id;
					maskids.push_back(data);
				}
				meshData->setElemMask(maskids);
			}
			else if (pickType >= PickDataType::GeoPoint && pickType <= PickDataType::Part)
			{
				this->setGeoHidden(getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint),
					getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge),
					getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace),
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid),
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPart));
			}
			getCVM->clearPick();
		}
		
		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			int pickNum = 0;
			std::vector<MeshSelectInfoData> res;
			PickDataType pickType = getCVM->getPostPickDataType(pickNum);
			if (pickNum == 0) return;

			if (pickType == PickDataType::Element )
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

				auto it = partEles.begin();
				while (it != partEles.end())
				{
					postDB->setEleVisual(postRend->getViewportName(), it.key(), it.value(), false);
					++it;
				}

				QString filePath = postDB->getFile();
				mPostParseMesh* parse = new mPostParseMesh(this);
				parse->ParseMesh(modelData);

				auto* rebuildProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
				if (rebuildProxy != nullptr)
					rebuildProxy->getModelRenderDataProxy()->RebuildModelData();
				postRender->updateAllModelOperate(ImportOperate);
				postRender->updateAllModelOperate(UpdateMinMax);
				postRend->update();
				updateMaxAndMinData();

				delete parse;
				parse = nullptr;
			}
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_maskOtherModel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickNum == 0)
			{
				return;
			}
			if (pickType == PickDataType::Element )
			{
				slot_maskModel();
				slot_reverseMaskModel();
			}
			else if (pickType >= PickDataType::GeoPoint && pickType <= PickDataType::Part)
			{
				this->setGeoHiddenOther(getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint),
					getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge),
					getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace),                  
					getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid),
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPart));
			}
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			postDB->setAllEleShow(postRend->getViewportName());
			modelData->setEleReserveShow(postRend->getViewportName());

			int pickNum = 0;
			std::vector<MeshSelectInfoData> res;
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

				auto it = partEles.begin();
				while (it != partEles.end())
				{
					postDB->setEleVisual(postRend->getViewportName(), it.key(), it.value(), true);
					++it;
				}

				QString filePath = postDB->getFile();
				mPostParseMesh* parse = new mPostParseMesh(this);
				parse->ParseMesh(modelData);

				auto* rebuildProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
				if (rebuildProxy != nullptr)
					rebuildProxy->getModelRenderDataProxy()->RebuildModelData();
				postRender->updateAllModelOperate(ImportOperate);
				postRender->updateAllModelOperate(UpdateMinMax);
				postRend->updatePaint();
				updateMaxAndMinData();

				delete parse;
				parse = nullptr;
			}
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_reverseMaskModel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			meshData->reverseMask();
			setGeoShowReverse();
			getCVM->clearPick();
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;
			modelData->setEleReserveShow(postRend->getViewportName());

			QString filePath = postDB->getFile();
			mPostParseMesh* parse = new mPostParseMesh(this);
			parse->ParseMesh(modelData);

			auto* rebuildProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
			if (rebuildProxy != nullptr)
				rebuildProxy->getModelRenderDataProxy()->RebuildModelData();
			postRender->updateAllModelOperate(ImportOperate);
			postRender->updateAllModelOperate(UpdateMinMax);
			postRender->updateHighLightRender();
			postRend->update();
			updateMaxAndMinData();
			delete parse;
			parse = nullptr;

			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_showAllModel()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			meshData->unReverseAll();
			setGeoShowAll();
			getCVM->clearPick();
		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			if (postRend == nullptr) return;

			auto  postRender = postRend->getPostRender();
			if (postRender == nullptr) return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			QString postname = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
			if (postDB == nullptr)
				return;

			mPostModelData* modelData = postDB->getModelData(guidata->_cloudFrame);
			if (modelData == nullptr) return;

			postDB->setAllEleShow(postRend->getViewportName());

			QString filePath = postDB->getFile();
			mPostParseMesh* parse = new mPostParseMesh(this);
			parse->ParseMesh(modelData);

			auto* rebuildProxy = postRender->frameProxy(postRender->currentFrame());   // S5 门面直接替换：getCurFrameDataProxy→frameProxy(currentFrame())（F8：换帧/重装载后重取）
			if (rebuildProxy != nullptr)
				rebuildProxy->getModelRenderDataProxy()->RebuildModelData();
			postRender->updateAllModelOperate(ImportOperate);
			postRender->updateAllModelOperate(UpdateMinMax);
			postRend->update();
			updateMaxAndMinData();
			delete parse;
			parse = nullptr;

			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_showAllFreeNode()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			auto tempNodes = meshData->getTempNodes();
			auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			currentViewportPrivate->_showFreeNode.clear();
			currentViewportPrivate->_showFreeNode.insert(tempNodes.begin(), tempNodes.end());
			updateFreeNodeShow(currentViewportPrivate);
		}
		else if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			QString postName = postRend->getPostDataName();
			QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			if (modeldb == nullptr)
				return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			auto freeNodes = modeldb->getFreeNodes(guidata->_cloudFrame);

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			currentViewportPrivate->_showFreeNode.clear();
			currentViewportPrivate->_showFreeNode.insert(freeNodes.begin(), freeNodes.end());
			updateFreeNodeShow(currentViewportPrivate);
		}
	}

	void mDisplayMenuFunction::slot_hideAllFreeNode()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			currentViewportPrivate->_showFreeNode.clear();
			updateFreeNodeShow(currentViewportPrivate);
		}
		else if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			QString postName = postRend->getPostDataName();
			QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			if (modeldb == nullptr)
				return;

			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			currentViewportPrivate->_showFreeNode.clear();
			updateFreeNodeShow(currentViewportPrivate);
		}

	}

	void mDisplayMenuFunction::slot_showFreeNode()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			auto tempNodes = meshData->getTempNodes();
			auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickNum == 0)
			{
				return;
			}
			if (pickType == PickDataType::Node)
			{
				std::vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				std::vector<int> nodeIds;
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					if (std::find(tempNodes.begin(), tempNodes.end(), nodeId) != tempNodes.end())
					{
						nodeIds.push_back(nodeId);
					}
				}

				for (auto id : nodeIds)
				{
					currentViewportPrivate->_showFreeNode.insert(id);
				}
			}
			updateFreeNodeShow(currentViewportPrivate);

		}
		
		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			QString postName = postRend->getPostDataName();
			QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			if (modeldb == nullptr)
				return;
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;
			auto freeNodes = modeldb->getFreeNodes(guidata->_cloudFrame);

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());

			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickNum == 0) return;

			if (pickType == PickDataType::Node)
			{
				std::vector<MeshSelectInfoData> res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				std::vector<int> nodeIds;
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					if (std::find(freeNodes.begin(), freeNodes.end(), nodeId) != freeNodes.end())
					{
						nodeIds.push_back(nodeId);
					}
				}

				for (auto id : nodeIds)
				{
					currentViewportPrivate->_showFreeNode.insert(id);
				}
			}
			updateFreeNodeShow(currentViewportPrivate);
			getCVM->clearPick();
		}
	}

	void mDisplayMenuFunction::slot_hideFreeNode()
	{
		if (getCVM == nullptr)
		{
			return;
		}

		if (getCVM->getPreRenderWidget())
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			auto tempNodes = meshData->getTempNodes();
			auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
			int pickNum = 0;
			auto pickType = getCVM->getCurrentPickDataType(pickNum);
			if (pickNum == 0)
			{
				return;
			}
			if (pickType == PickDataType::Node)
			{
				std::vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				std::vector<int> nodeIds;
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					currentViewportPrivate->_showFreeNode.erase(nodeId);
				}
			}
			updateFreeNodeShow(currentViewportPrivate);

		}

		if (getCVM->getPostRenderWidget())
		{
			REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
			QString postName = postRend->getPostDataName();
			QPointer<mPostDataBase> modeldb = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			if (modeldb == nullptr)
				return;
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;
			auto freeNodes = modeldb->getFreeNodes(guidata->_cloudFrame);

			mPostModelData* modeldata = modeldb->getModelData(guidata->_cloudFrame);
			if (modeldata == nullptr) return;

			int pickNum = 0;
			auto pickType = getCVM->getPostPickDataType(pickNum);
			if (pickNum == 0)	return;

			auto currentViewportPrivate = dynamic_cast<MechPreData::mPostViewportPrivate*>(VMManager->getCurrentViewportPrivate());

			std::vector<int> pickIDs;
			if (pickType == PickDataType::Node)
			{
				std::vector<MeshSelectInfoData> res = getCVM->getPostRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
				std::vector<int> nodeIds;
				for (int i = 0; i < pickNum; i++)
				{
					int nodeId = res.at(i).data_id.element_id;
					int cID = modeldata->_nodedata.nodeidmap.value(nodeId);
					if (std::find(freeNodes.begin(), freeNodes.end(), cID) != freeNodes.end())
					{
						nodeIds.push_back(cID);
					}
				}

				for (auto id : nodeIds)
				{
					currentViewportPrivate->_showFreeNode.erase(id);
				}
			}
			updateFreeNodeShow(currentViewportPrivate);
			getCVM->clearPick();
		}
	}

    void mDisplayMenuFunction::slot_deleteAllFreeNode()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        if (getCVM->getPreRenderWidget())
        {
            QList<MechPreData::mViewportPrivate*> viewportPrivates = getCVM->getRendViewportPrivates(true);
            auto meshData = getCVM->getModelData()->getMeshData();
            auto tempNodes = meshData->getTempNodes();
            for (int i = 0; i < viewportPrivates.size(); i++)
            {
                auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivates.at(i));
                if (!currentViewportPrivate)
                    continue;
                currentViewportPrivate->_showFreeNode.clear();
                updateFreeNodeShow(currentViewportPrivate);
            }
            meshData->deleteTempNodeVertex(tempNodes);
        }
        getCVM->clearPick();
    }

    void mDisplayMenuFunction::slot_deleteFreeNode()
    {
        if (getCVM == nullptr)
        {
            return;
        }
        if (getCVM->getPreRenderWidget())
        {
            QList<MechPreData::mViewportPrivate*> viewportPrivates = getCVM->getRendViewportPrivates(true);
            auto meshData = getCVM->getModelData()->getMeshData();
            auto tempNodes = meshData->getTempNodes();
            std::vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
            std::vector<int> nodeIds;
            int pickNum = 0;
            auto pickType = getCVM->getCurrentPickDataType(pickNum);
            if (pickNum == 0)
            {
                return;
            }
            if (pickType != PickDataType::Node)
            {
                return;
            }
            for (int i = 0; i < pickNum; i++)
            {
                int nodeId = res.at(i).data_id.element_id;
                if (std::find(tempNodes.begin(), tempNodes.end(), nodeId) != tempNodes.end())
                {
                    nodeIds.push_back(nodeId);
                }
            }
            for (int i = 0; i < viewportPrivates.size(); i++)
            {
                auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivates.at(i));
                if (!currentViewportPrivate)
                    continue;

                for (auto nodeId : nodeIds)
                {
                    currentViewportPrivate->_showFreeNode.erase(nodeId);
                }
                updateFreeNodeShow(currentViewportPrivate);
            }
            meshData->deleteTempNodeVertex(nodeIds);

        }
        getCVM->clearPick();
    }

	void mDisplayMenuFunction::slot_showFreeNode(std::vector<int> tempIds, bool showHide)
	{
		if (getCVM == nullptr)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		QList<MechPreData::mViewportPrivate*> viewportPrivates = getCVM->getRendViewportPrivates(true);
		if (showHide)
		{
			for (int i = 0; i < viewportPrivates.size(); i++)
			{
				auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivates.at(i));
				currentViewportPrivate->_showFreeNode.insert(tempIds.begin(), tempIds.end());
				updateFreeNodeShow(currentViewportPrivate);
			}
		}
		else
		{
			for (int i = 0; i < viewportPrivates.size(); i++)
			{
				auto currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivates.at(i));
				for (auto id : tempIds)
				{
					currentViewportPrivate->_showFreeNode.erase(id);
				}

				updateFreeNodeShow(currentViewportPrivate);
			}
		}
	}

	void mDisplayMenuFunction::updateNodeEleLabel()
	{
		if (getCVM == nullptr)
		{
			return;
		}
		mBaseRend* rend = nullptr;
		auto viewportPrivate = VMManager->getCurrentViewportPrivate();
		if (viewportPrivate == nullptr)
		{
			return;
		}
		rend = viewportPrivate->getRendWidget();
		if (rend == nullptr)
		{
			return;
		}

		QVector<QVector3D> vertexs;
		QVector<QString> ids;
		auto fontRender = rend->getFontRender();

		QHashIterator<int, QVector3D> iter(viewportPrivate->_nodeIdVertexs);
		while (iter.hasNext())
		{
			iter.next();
			vertexs.append(iter.value());
			ids.append(QString("%1").arg(iter.key()));
		}

		fontRender->appendCommonFont("NodeID", vertexs, ids, QVector3D(41 / 255.0, 157 / 255.0, 1));

		vertexs.clear();
		ids.clear();
		iter = QHashIterator<int, QVector3D>(viewportPrivate->_eleIdVertexs);
		while (iter.hasNext())
		{
			iter.next();
			vertexs.append(iter.value());
			ids.append(QString("%1").arg(iter.key()));
		}
		fontRender->appendCommonFont("EleID", vertexs, ids, QVector3D(157 / 255.0, 41 / 255.0, 1));
		rend->updatePaint();
	}

	void mDisplayMenuFunction::updateFreeNodeShow(MechPreData::mViewportPrivate* viewportPrivate)
	{
		if (viewportPrivate == nullptr)
		{
			return;
		}

		if (viewportPrivate->getViewportType() == Viewport3D)
		{
			auto meshData = getCVM->getModelData()->getMeshData();
			QVector<QVector3D> vertexs;
			for (auto id : viewportPrivate->_showFreeNode)
			{
				vertexs.append(meshData->getVectorById(id));
			}
			viewportPrivate->getRendWidget()->getCommonShapeRender()->appendPoint("freeNode", vertexs, QVector3D(108 / 255.0, 181 / 255.0, 1.0));
			viewportPrivate->getRendWidget()->updatePaint();
		}
		else if (viewportPrivate->getViewportType() == ViewportPost3D)
		{
			REBUILD_PostRend* postRend =  dynamic_cast<mxr::REBUILD_PostRend*>(viewportPrivate->getRendWidget());
			QString postName = postRend->getPostDataName();
			QPointer<mPostDataBase> postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
			
			mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
			if (guidata == nullptr) return;

			mPostModelData* modeldata = postDB->getModelData(guidata->_cloudFrame);
			QVector<QVector3D> vertexs;
			for (auto id : viewportPrivate->_showFreeNode)
			{
				vertexs.append(modeldata->getCoordinates(id));
			}
			viewportPrivate->getRendWidget()->getCommonShapeRender()->appendPoint("freeNode", vertexs, QVector3D(108 / 255.0, 181 / 255.0, 1.0));
			viewportPrivate->getRendWidget()->updatePaint();
		}
	}

	std::vector<QVector3D> mDisplayMenuFunction::getDefaultUpVector(std::vector<int> parIds)
	{
		std::vector<QVector3D> vecs;
		vecs.resize(parIds.size(), QVector3D(1, 0, 0));
		if (getCVM == nullptr)
		{
			return vecs;
		}
		auto areVectorsParallel = [](const QVector3D& v1, const QVector3D& v2) {
			QVector3D crossProd = QVector3D::crossProduct(v1, v2);
			return crossProd.isNull();
			};
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto meshData = getCVM->getModelData()->getMeshData();

		for (int i = 0; i < parIds.size(); ++i)
		{
			int partId = parIds.at(i);
			auto partData = meshData->getPartData(partId);
			if (partData == nullptr)
			{
				continue;
			}
			int partType = partData->getPartType();
			if (partType == 2)
			{
				auto meshIds = meshData->getMeshIdsByPartId(partId, 1);
				if (meshIds.size() > 0)
				{
					RenderPickResultData resultData;
					resultData.result_id = meshIds.at(0);
					resultData.part_id = partId;
					std::vector<QVector3D> nodeVertexs = meshData->getCoorsOfEle(resultData);
					if (nodeVertexs.size() >= 2)
					{
						if (areVectorsParallel(nodeVertexs.at(1) - nodeVertexs.at(0), vecs[i]))
						{
							vecs[i] = QVector3D(0, 1, 0);
                            continue;

						}
					}
				}
			}
			else
			{
				auto lines = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 1);
				auto res = getGEOServer.geoCalculate().CalculateEdgeAttr(lines);
				for (auto vec : res)
				{
					if (areVectorsParallel(vec.m_dir, vecs.at(i)))
					{
						vecs[i] = QVector3D(0, 1, 0);
                        continue;
					}
				}
			}
		}
		return vecs;
	}

	void mDisplayMenuFunction::updateMaxAndMinData()
	{
		REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
		if (postRend == nullptr) return;
		auto  postRender = postRend->getPostRender();

		if (postRender == nullptr) return;
		// S5 门面直接替换：getCurFrameDataProxy 消亡→frameProxy(currentFrame()) 现取（F8：换帧/重装载后重取）
		mxr::PostFrameDataProxy* curFrameProxy = postRender->frameProxy(postRender->currentFrame());
		if (curFrameProxy == nullptr) return;
		mxr::IPostRendQueryDataProxy* query_proxy = dynamic_cast<mxr::IPostRendQueryDataProxy*>(curFrameProxy->getRendQueryDataProxy());

		mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
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
			if (maxval < minval)
			{
				mGlobalSignals::getInstance()->outputMessageSig(QString("最大值不能小于最小值！"));
				return;
			}
			postRender->getColorTableWindow()->setValueMinMax(postRender->currentFrame(), maxval, minval);
			query_proxy->setCurrentMaxData(maxval);
			query_proxy->setCurrentMinData(minval);
			postRender->getColorTableWindow()->setValueMinMax(postRender->currentFrame(), maxval, minval);
			//setMinMaxData 已随渲染仓色表接口统一(41bcb91a)退役，值域经 getColorTableWindow()->setValueMinMax 设置
			postRend->update();
			guidata->_cloudMaxValue = maxval;
			guidata->_cloudMinValue = minval;
		}
	}

	//【集成回补·GN 祖先闭包】前向声明（定义于 setGeoShowReverse 前）
	//keepHiddenEdges/keepHiddenPoints：调用前已隐藏的元素（用户先前的显式隐藏
	//或既有状态），修复只回滚本次级联新隐藏的元素，不得复活它们
	static void repairSharedSubElementVisibilityGN(GeoNeutralData::mGeoPartManager* partMgr,
		const std::set<int>& keepHiddenEdges = {}, const std::set<int>& keepHiddenPoints = {});

	void mDisplayMenuFunction::setGeoHidden(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID, const std::vector<int>& partID)
	{
		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		//快照本次操作前已隐藏的边/点：作为共享修复的保护名单——
		//否则后续操作会把先前显式隐藏的线按"父面可见"规则复活
		std::set<int> preHiddenEdges, preHiddenPoints;
		for (int id : getAllGeoElementIds(1))
		{
			if (!isGeoElementVisible(1, id))
			{
				preHiddenEdges.insert(id);
			}
		}
		for (int id : getAllGeoElementIds(0))
		{
			if (!isGeoElementVisible(0, id))
			{
				preHiddenPoints.insert(id);
			}
		}
		//隐藏选中的面/体（写口自带向下级联），再按体属重刷面状态
		for (const auto& pID : faceID)
			setGeoElementVisible(2, pID, false);
		for (const auto& pID : solidID)
			setGeoElementVisible(3, pID, false);
        // 把部件展开成其下所有 solid
        std::vector<int> allSolidID = solidID;
        for (const auto& pID : partID)
        {
            auto partSolids = getSolidIdsInPart(pID);
            for (int sid : partSolids)
                setGeoElementVisible(3, sid, false);

            allSolidID.insert(allSolidID.end(), partSolids.begin(), partSolids.end());
        }

        calculateGeoFaceHiddenBySolid(allSolidID);
        //【集成回补·GN 祖先闭包】级联隐藏后修复共享边/点（dev 语义：有任一可见祖先则保留显示）；
        //保护名单内是本次操作前已隐藏的元素，不参与修复
        repairSharedSubElementVisibilityGN(getGeoPartMgr(), preHiddenEdges, preHiddenPoints);

        //显式隐藏选中的边/点放在共享修复之后：修复只回滚级联隐藏，
        //不能复活用户点名隐藏的元素
		for (const auto& pID : lineID)
			setGeoElementVisible(1, pID, false);
		for (const auto& pID : pointID)
			setGeoElementVisible(0, pID, false);

        getCVM->updateGeoModelOperate(getCVM->getPreRenderWidget(), HideOrShowPrimitive, getAllGeoPartIds());
	}

	void mDisplayMenuFunction::setGeoHiddenOther(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID, const std::vector<int>& partID)
	{
		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}

		//同维度内"隐藏全部、再显示选中"，仅处理有选中元素的维度；
		//体/面级联在先，边/点的显式写在后，选中项不被级联覆盖
        //快照本次操作前已隐藏的边/点：作为共享修复的保护名单（同 setGeoHidden）
        std::set<int> preHiddenEdges, preHiddenPoints;
        for (int id : getAllGeoElementIds(1))
        {
            if (!isGeoElementVisible(1, id))
            {
                preHiddenEdges.insert(id);
            }
        }
        for (int id : getAllGeoElementIds(0))
        {
            if (!isGeoElementVisible(0, id))
            {
                preHiddenPoints.insert(id);
            }
        }
        std::set<int> keepSolidSet(solidID.begin(), solidID.end());
        for (const auto& pID : partID)
        {
            auto partSolids = getSolidIdsInPart(pID);
            keepSolidSet.insert(partSolids.begin(), partSolids.end());
        }
        // 隐藏所有不在保留集中的 solid（级联隐藏其面/边/点）；
        // 保留集为空时不执行，否则会把全部体连同选中元素一起级联隐藏
        if (!keepSolidSet.empty())
        {
            for (int sid : getAllGeoElementIds(3))
            {
                if (keepSolidSet.find(sid) == keepSolidSet.end())
                    setGeoElementVisible(3, sid, false);
            }
        }

		if (!faceID.empty())
		{
			for (int id : getAllGeoElementIds(2))
				setGeoElementVisible(2, id, false);

			for (const auto& pID : faceID)
				setGeoElementVisible(2, pID, true);
		}
        // 保留体整体可见：重刷保留体下属面的显示
        std::vector<int> solidIDToCalc(keepSolidSet.begin(), keepSolidSet.end());
        calculateGeoFaceHiddenBySolid(solidIDToCalc);
        // 共享边/点祖先闭包：有任一可见祖先则保留显示（同 setGeoHidden 语义）；
        //保护名单内是本次操作前已隐藏的元素，不参与修复
        repairSharedSubElementVisibilityGN(getGeoPartMgr(), preHiddenEdges, preHiddenPoints);

		if (!lineID.empty())
		{
			for (int id : getAllGeoElementIds(1))
				setGeoElementVisible(1, id, false);

			for (const auto& pID : lineID)
				setGeoElementVisible(1, pID, true);
		}
		if (!pointID.empty())
		{
			for (int id : getAllGeoElementIds(0))
				setGeoElementVisible(0, id, false);

			for (const auto& pID : pointID)
				setGeoElementVisible(0, pID, true);
		}

        getCVM->updateGeoModelOperate(getCVM->getPreRenderWidget(), HideOrShowPrimitive, getAllGeoPartIds());
	}

	void mDisplayMenuFunction::setGeoShow(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID)
	{
		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		//显示选中的元素，写口自动向下级联显示子元素
		for (const auto& pID : pointID)
			setGeoElementVisible(0, pID, true);
		for (const auto& pID : lineID)
			setGeoElementVisible(1, pID, true);
		for (const auto& pID : faceID)
			setGeoElementVisible(2, pID, true);
		for (const auto& pID : solidID)
			setGeoElementVisible(3, pID, true);

		calculateGeoFaceHiddenBySolid(solidID);

		getCVM->updateGeoModelOperate(getCVM->getPreRenderWidget(), HideOrShowPrimitive, getAllGeoPartIds());
	}

	void mDisplayMenuFunction::setGeoShowAll()
	{
		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		//四个维度的元素全集逐个写显示
		for (int dim = 0; dim <= 3; ++dim)
		{
			for (int id : getAllGeoElementIds(dim))
				setGeoElementVisible(dim, id, true);
		}

		getCVM->updateGeoModelOperate(getCVM->getPreRenderWidget(), HideOrShowPrimitive, getAllGeoPartIds());
	}

	//【集成回补·GN 祖先闭包】dev"共享子元素可见性保护"的 GN 直连版：
	// 隐藏/取反后，共享边（属任一可见父面）与共享点（属任一可见父边）恢复显示，
	// 防止仍可见面的共享边被连带隐藏。祖先表走 GN_Part::getEdgeFaceMap/getVertexEdgeMap
	//（E→F / V→E），读写都在 GN 部件属性上（渲染重建批次的显隐真源）。
	//keepHiddenEdges/keepHiddenPoints：调用前已隐藏的元素（用户先前的显式隐藏
	//或既有状态），修复只回滚本次级联新隐藏的元素，不得复活它们。
	static void repairSharedSubElementVisibilityGN(GeoNeutralData::mGeoPartManager* partMgr,
		const std::set<int>& keepHiddenEdges, const std::set<int>& keepHiddenPoints)
	{
		if (partMgr == nullptr)
		{
			return;
		}
		for (int partId : partMgr->getPartList())
		{
			//必须取部件指针而非值拷贝：GN_Part 值语义写临时副本不落库
			GeoNeutralData::GN_Part* part = partMgr->getPartPtrById(partId);
			if (part == nullptr)
			{
				continue;
			}
			//共享边：任一父面可见（未隐藏）则恢复边显示
			const auto& edgeFaceMap = part->getEdgeFaceMap();
			for (const auto& kv : edgeFaceMap)
			{
				if (keepHiddenEdges.count(kv.first) > 0)
				{
					continue;
				}
				if (part->isElementVisible(1, kv.first))
				{
					continue;
				}
				for (int faceId : kv.second)
				{
					if (part->isElementVisible(2, faceId))
					{
						part->setVisible(1, kv.first, true);
						break;
					}
				}
			}
			//共享点：任一父边可见则恢复点显示
			const auto& vertexEdgeMap = part->getVertexEdgeMap();
			for (const auto& kv : vertexEdgeMap)
			{
				if (keepHiddenPoints.count(kv.first) > 0)
				{
					continue;
				}
				if (part->isElementVisible(0, kv.first))
				{
					continue;
				}
				for (int edgeId : kv.second)
				{
					if (part->isElementVisible(1, edgeId))
					{
						part->setVisible(0, kv.first, true);
						break;
					}
				}
			}
		}
	}

	void mDisplayMenuFunction::setGeoShowReverse()
	{
		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		auto partMgr = getGeoPartMgr();
		if (!partMgr)
		{
			return;
		}

		//先快照每个元素自身的可见性，写入顺序为 体→面（级联覆盖子元素）→
		//体连带面拉回→共享修复→边→点（各自取反值显式收尾）：
		//最终每个元素的可见性都等于其自身原状态取反，连续两次取反回到原状态
		std::vector<std::pair<int, bool>> pointFlags, lineFlags, faceFlags, solidFlags;
		for (int id : getAllGeoElementIds(0))
			pointFlags.push_back({ id, isGeoElementVisible(0, id) });
		for (int id : getAllGeoElementIds(1))
			lineFlags.push_back({ id, isGeoElementVisible(1, id) });
		//取反前处于显示态的面集合（体连带显示判断用）
		QSet<int> showFaceIds;
		for (int id : getAllGeoElementIds(2))
		{
			bool visible = isGeoElementVisible(2, id);
			faceFlags.push_back({ id, visible });
			if (visible)
			{
				showFaceIds.insert(id);
			}
		}
		for (int id : getAllGeoElementIds(3))
			solidFlags.push_back({ id, isGeoElementVisible(3, id) });

		for (const auto& flag : solidFlags)
			setGeoElementVisible(3, flag.first, !flag.second);
		for (const auto& flag : faceFlags)
			setGeoElementVisible(2, flag.first, !flag.second);

		//取反后变为可见的体，把它下面"取反前并非显示态"的面拉回显示；
		//取反前可见的面此时已被自身的取反隐藏，不再拉回——
		//否则连续两次取反回不到"该面隐藏、其余显示"的状态
		auto proxy = getPreDataProxy();
		auto elementIndex = (proxy != nullptr) ? proxy->elementIndex() : nullptr;
		for (const auto& flag : solidFlags)
		{
			if (flag.second)
			{
				continue; //取反后隐藏的体不处理
			}
			if (elementIndex == nullptr)
			{
				continue;
			}
			std::vector<int> faceids;
			if (!elementIndex->solidFaceIds(flag.first, faceids))
			{
				continue;
			}
			for (int faceid : faceids)
			{
				if (!showFaceIds.contains(faceid))
					setGeoElementVisible(2, faceid, true);
			}
		}

		//【集成回补·GN 祖先闭包】级联取反后修复共享边/点（dev 语义：有任一可见祖先则恢复显示）
		repairSharedSubElementVisibilityGN(partMgr);

		//边/点的显式取反放在级联、连带显示与共享修复全部落定之后：
		//否则被单独隐藏的边（如"隐藏边→取反→再取反"）会被级联/修复复活，
		//连续两次取反回不到原状态
		for (const auto& flag : lineFlags)
			setGeoElementVisible(1, flag.first, !flag.second);
		for (const auto& flag : pointFlags)
			setGeoElementVisible(0, flag.first, !flag.second);

        getCVM->updateGeoModelOperate(getCVM->getPreRenderWidget(), HideOrShowPrimitive, getAllGeoPartIds());

	}

	//优先面，只有体隐藏了才会隐藏全部面
	void mDisplayMenuFunction::calculateGeoFaceHiddenBySolid(const std::vector<int>& solidID)
	{
		auto proxy = getPreDataProxy();
		if (!proxy || !proxy->elementIndex())
		{
			return;
		}
		auto elementIndex = proxy->elementIndex();
		//重新清洗一遍面的隐藏状态
		for (const auto& pID : solidID)
		{
			std::vector<int> faceids;
			if (!elementIndex->solidFaceIds(pID, faceids))
			{
				continue;
			}
			for (int faceid : faceids)
			{
				//GN 建模保证一个面至多属于一个体：所属体隐藏则面隐藏，
				//所属体显示则面恢复显示；查无所属体按显示处理（不误伤独立面）
				int solidOfFace = elementIndex->solidOfFace(faceid);
				bool isFaceNeedHidden = (solidOfFace != -1) && !isGeoElementVisible(3, solidOfFace);
				setGeoElementVisible(2, faceid, !isFaceNeedHidden);
			}
		}
	}

	bool mDisplayMenuFunction::getGeoPointHiddenByPoint(int id)
	{
		//查不到部件时按可见处理（同老逻辑无数据返回 false）
		return !isGeoElementVisible(0, id);
	}

	bool mDisplayMenuFunction::getGeoSolidHiddenBySolid(int id)
	{
		return !isGeoElementVisible(3, id);
	}

	bool mDisplayMenuFunction::getGeoFaceHiddenByFace(int id)
	{
		return !isGeoElementVisible(2, id);
	}

	mxr::IPreDataProxy* mDisplayMenuFunction::getPreDataProxy()
	{
		if (!getCVM)
		{
			return nullptr;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return nullptr;
		}
		return getCVM->getPreRenderWidget()->getPreDataProxy();
	}

	GeoNeutralData::mGeoPartManager* mDisplayMenuFunction::getGeoPartMgr()
	{
		if (!getCVM || !getCVM->getModelData())
		{
			return nullptr;
		}
		auto geoManager = getCVM->getModelData()->getGeoManager();
		return geoManager ? geoManager->getGeoPartMgr() : nullptr;
	}

	GeoNeutralData::GN_Part* mDisplayMenuFunction::getGeoPartByElementId(int dim, int id)
	{
		static const mxr::GeoElementType typeByDim[4] = {
			mxr::GeoElementType::Point, mxr::GeoElementType::Line,
			mxr::GeoElementType::Face, mxr::GeoElementType::Solid };
		if (dim < 0 || dim > 3)
		{
			return nullptr;
		}
		auto partMgr = getGeoPartMgr();
		if (partMgr == nullptr)
		{
			return nullptr;
		}
		//经契约元素表反查所属部件，再定位到 GN 部件
		auto proxy = getPreDataProxy();
		if (proxy != nullptr && proxy->elementIndex() != nullptr)
		{
			mxr::ElementLoc loc;
			if (proxy->elementIndex()->locateElement(typeByDim[dim], id, loc))
			{
				return partMgr->getPartPtrById(loc.partId);
			}
		}
		//兜底：摊平索引尚未建好（如启动后首次操作、索引缺部件）时按 GN 拓扑
		//直查所属部件——显隐写入不能因索引缺失被静默丢弃
		for (int partId : partMgr->getPartList())
		{
			GeoNeutralData::GN_Part* part = partMgr->getPartPtrById(partId);
			if (part != nullptr && part->existShape(dim, id))
			{
				return part;
			}
		}
		return nullptr;
	}

	bool mDisplayMenuFunction::setGeoElementVisible(int dim, int id, bool visible)
	{
		auto part = getGeoPartByElementId(dim, id);
		if (part == nullptr)
		{
			return false;
		}
		part->setVisible(dim, id, visible);
		return true;
	}

	bool mDisplayMenuFunction::isGeoElementVisible(int dim, int id)
	{
		auto part = getGeoPartByElementId(dim, id);
		return part ? part->isElementVisible(dim, id) : true;
	}

	std::set<int> mDisplayMenuFunction::getAllGeoPartIds()
	{
		std::set<int> partIds;
		auto proxy = getPreDataProxy();
		if (proxy != nullptr && proxy->parts() != nullptr)
		{
			//契约视图借用适配器内存，当次消费当次转存
			auto view = proxy->parts()->partIds();
			for (std::size_t i = 0; i < view.count; ++i)
			{
				partIds.insert(view.data[i]);
			}
		}
		if (partIds.empty())
		{
			//兜底：代理未挂载/摊平部件表为空时按 GN 部件表全量重建，
			//避免空清单导致显隐刷新无效（视口不更新）
			auto partMgr = getGeoPartMgr();
			if (partMgr != nullptr)
			{
				for (int partId : partMgr->getPartList())
				{
					partIds.insert(partId);
				}
			}
		}
		return partIds;
	}

	std::set<int> mDisplayMenuFunction::getAllGeoElementIds(int dim)
	{
		//枚举口径与旧渲染镜像一致：体=solidfaces，面=facePoints，
		//线=edgePoints+独立线，点=几何点+独立点
		std::set<int> ids;
		auto partMgr = getGeoPartMgr();
		if (partMgr == nullptr)
		{
			return ids;
		}
		for (int partId : partMgr->getPartList())
		{
			GeoNeutralData::GN_Part* part = partMgr->getPartPtrById(partId);
			if (part == nullptr)
			{
				continue;
			}
			const auto& disc = part->getGeoDiscreteData();
			if (dim == 3)
			{
				for (const auto& kv : disc.solidfaces) ids.insert(kv.first);
			}
			else if (dim == 2)
			{
				for (const auto& kv : disc.facePoints) ids.insert(kv.first);
			}
			else if (dim == 1)
			{
				for (const auto& kv : disc.edgePoints) ids.insert(kv.first);
				for (const auto& kv : disc.isolatedEdges) ids.insert(kv.first);
			}
			else if (dim == 0)
			{
				for (const auto& kv : disc.geoPointIndices) ids.insert(kv.first);
				for (const auto& kv : disc.isolatedPointIndices) ids.insert(kv.first);
			}
		}
		return ids;
	}

	std::vector<int> mDisplayMenuFunction::getSolidIdsInPart(int partId)
	{
		std::vector<int> solids;
		auto partMgr = getGeoPartMgr();
		if (partMgr == nullptr)
		{
			return solids;
		}
		GeoNeutralData::GN_Part* part = partMgr->getPartPtrById(partId);
		if (part == nullptr)
		{
			return solids;
		}
		for (const auto& kv : part->getGeoDiscreteData().solidfaces)
		{
			solids.push_back(kv.first);
		}
		return solids;
	}

	void mDisplayMenuFunction::slot_subWindowActivated(QMdiSubWindow* subWindow)
	{
		if (!getCVM)
		{
			return;
		}
	}

	mDisplayMenuFunction::~mDisplayMenuFunction()
	{

	}
}

