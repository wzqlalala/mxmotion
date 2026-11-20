#ifdef _WIN32
#define _OPENMP 200203
#endif
#include "MXMesh.h"
//#include "mMeshMain.h"
#include "mMeshThread.h"
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"

//#include "mMeshManager.h"

#include <set>
#include "qset.h"
#include "mModelData.h"

#include "mInfoWindow.h"
#include "mFaceMeshPanel.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include <QMutex>

//MMeshEngine
//#include "PreInterface.h"
//#include "MTetrahedron.h"
//#include "MTriangle.h"
//#include "MLine.h"
//#include "MQuadrangle.h"
#ifdef _WIN32
#ifndef _OPENMP
#define _OPENMP 200203
#endif
#if defined(_OPENMP)
#include <omp.h>
#endif
#else
#include <omp.h>
#endif

//MDatabase
//#include "mDatabaseSingleton.h"
//#include "mDatabase.h"
//#include "mMeshData.h"

//MMeshView
//#include "mMeshStaticData.h"
//#include "mTempMeshData1.h"

//MDataMesh
//#include "mMeshData1.h"
//#include "mMeshPartData1.h"
//#include "mMeshGeoFaceData1.h"
//#include "mMeshFaceData1.h"
//#include "mMeshNodeData1.h"
//#include "mMeshLineData1.h"
//#include "mMeshGeoLineData1.h"


//MBasicFunction
#include "mGlobalSignals.h"                                    
//#include "mOutputMessage.h"

//MDataGeo
//#include "mGeoModelData1.h"
//#include "mGeoPartData1.h"
//Qt
#include <QDebug>

//#include"MeshMessage.h"
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC迁移·链A续清配套】命名空间级 CalculateGeometricAttr 显式自含（原经 mGeoManager.h 传递） //【OCC摘除·S10/P0-1】调用已切 engine().geoCalculate() 同语法视图
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mFontRender.h"

#include "mMeshPanelManager.h"
#include "mDataElement.h"
#include "mElementBase.h"
#include <QMessageBox>
#include "mMechGlobalSignals.h"
#include "GlobalMeshMessage.h"
#include "mOtherFunction.h"
#include "MeshEntity.h"
//using namespace MDatabase;
//using namespace MMeshView;
using namespace MBasicFunction;
using namespace MeshPanelData;
using namespace MechElementData;
using namespace MechModelData;
using namespace MechMainWindow;

namespace
{
	//【空窗收口·G3】边显示类型写入：老 GeoDataBase::setGeoDiscreteDataEdgeTopoType 空壳，
	//改写 GN 部件离散表 edgeTopoTypeMap（2=虚拓扑边→渲染虚线；表已随撤消/重做快照序列化）
	void setEdgeTopoTypeToNeutralData(GeoNeutralData::mGeoPartManager* partMgr,
		const std::set<int>& partIds, const std::map<int, int>& edgeTypeMap)
	{
		if (partMgr == nullptr || edgeTypeMap.empty())
		{
			return;
		}
		for (int partId : partIds)
		{
			GeoNeutralData::GN_Part* part = partMgr->getPartPtrById(partId);
			if (part == nullptr)
			{
				continue;
			}
			GeoNeutralData::GN_TessellationMesh disc = part->getGeoDiscreteData();
			for (const auto& p : edgeTypeMap)
			{
				disc.edgeTopoTypeMap[p.first] = p.second;
			}
			part->setGeoDiscreteData(disc);
		}
	}
}

namespace MGUIMesh
{
	mMeshThread::mMeshThread(QObject *parent)
	{
	}

	mMeshThread::~mMeshThread()
	{

	}
	void mMeshThread::slot_createMeshAuto()
	{
		if (getCVM->getPreRenderWidget() == nullptr)
			return;

		MechModelData::mModelData* modelData = getCVM->getModelData();
		if (!modelData)
		{
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			emit done("Error");
			return;
		}
		mMeshData* meshdata = modelData->getMeshData();
		if (!meshdata)
		{
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			emit done("Error");
			return;
		}
		auto geoDatabase = modelData->getGeoData();
		if (!geoDatabase)
		{
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			emit done("Error");
			return;
		}

		MeshEntity* entity = meshdata->getMeshEntity();
		if (!entity)
		{
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			emit done("Error");
			return;
		}
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
		// 步骤1: 获取所有维度ID
		std::set<int> allSolidIds = getGEOServer.geoCalculate().getAllGeoID(3);
		std::set<int> allFaceIds  = getGEOServer.geoCalculate().getAllGeoID(2);
		std::set<int> allEdgeIds  = getGEOServer.geoCalculate().getAllGeoID(1);

		// 步骤2: 3D→2D精确去重（遍历所有3D体，收集其表面的面ID）
        std::set<int> remainSolidIds = allSolidIds;
        for (auto id : allSolidIds)
        {
            //【OCC摘除·S9】旧栈 set 反查退役（恒 null 解引用崩溃隐患），改 GN 部件名
            auto name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, id);
            meshdata->deleteMeshPart(name);
        }
        std::set<int> facesBelongToSolid;
        for (int solidId : remainSolidIds)
        {
            MXGeoSolid* solid = entity->getGeoSolidById(solidId);
            if (!solid)
                continue;
            if (solid->getMeshStatic() != NoMesh)
            {
                allSolidIds.erase(allSolidIds.find(solidId));
                continue;
            }
			for (MXGeoFace* face : solid->getface())
			{
				if (face)
					facesBelongToSolid.insert(face->getGeoTag());
			}
		}

		// 从2D列表中剔除属于3D体的面，保留独立的2D面

        std::set<int> meshGeoId_2D;
        std::set<int>remainFaceIds = allFaceIds;
        for (int faceId : remainFaceIds)
        {
            MXGeoFace* face = entity->getGeoFaceById(faceId);
            if (!face)
            {
                continue;
            }
            if (face->getMeshStatic() != NoMesh)
            {
                allFaceIds.erase(allFaceIds.find(faceId));
                continue;
            }
            if (facesBelongToSolid.count(faceId) == 0)
            {
                meshGeoId_2D.insert(faceId);
            }
        }

		// 步骤3: 2D→1D去重（Part级别，优化为 O(N_edge + N_face)）
		// 如果一个Part中存在2D面，则该Part的1D边由2D剖分自动处理
		std::set<int> partsWithFace; //【OCC摘除·S9】部件身份改 ID（原 set 指针）
		for (int faceId : allFaceIds)
		{
			const int facePartId = MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, faceId); //【OCC摘除·S9】GN 部件 ID（原 set 反查退役）
			if (facePartId >= 0)
				partsWithFace.insert(facePartId);
		}

		std::set<int> meshGeoId_1D;
        std::set<int>remainEdgeIds = allEdgeIds;
        for (int edgeId : remainEdgeIds)
        {
            const int edgePartId = MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, edgeId); //【OCC摘除·S9】GN 部件 ID
            if (edgePartId < 0)
            	continue;
            MXGeoEdge* edge = entity->getGeoEdgeById(edgeId);
            if (!edge)
            {
                continue;
            }
            if (edge->getMeshStatic() != NoMesh)
            {
                allEdgeIds.erase(allEdgeIds.find(edgeId));
                continue;
            }
            if (partsWithFace.count(edgePartId) == 0)
                meshGeoId_1D.insert(edgeId);
        }
		for (int edgeId : allEdgeIds)
		{
			const int edgePartId = MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, edgeId); //【OCC摘除·S9】GN 部件 ID
			if (edgePartId < 0)
				continue;
            MXGeoEdge* edge = entity->getGeoEdgeById(edgeId);
            if (!edge)
            {
                continue;
            }
            if (edge->getMeshStatic() != NoMesh)
            {
                allEdgeIds.erase(allEdgeIds.find(edgeId));
                continue;
            }
			if (partsWithFace.count(edgePartId) == 0)
				meshGeoId_1D.insert(edgeId);
		}
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }

		// 步骤4: 按低维→高维顺序剖分，维度之间需同步；
		// 最后一批剖完的同步交给 done 回调（mMeshPanel.cpp）里的 ensureMesh，避免连续重复执行
		auto splitByDim = [entity, meshdata](int dim, const std::set<int>& meshGeoId, bool syncAfter)
			{
				if (meshGeoId.empty())
					return;
				entity->resetPreMeshSize(dim, MAX_LC);
				entity->setMeshPattern(MeshEntity::MeshPattern::AUTOMATED_MESHING);
				std::shared_ptr<MXMesh> mesh = std::make_shared<MXMesh>();
				mesh->setMeshData(meshdata);
				mesh->MXMeshInterface(dim, entity, meshGeoId, {});
				if (syncAfter)
					meshdata->ensureMesh();  // 仅当还有更高维度要剖时才同步
			};

		const std::set<int>* dimSets[3] = { &meshGeoId_1D, &meshGeoId_2D, &allSolidIds };
		const std::set<int>* lastNonEmpty = nullptr;
		for (auto* s : dimSets)
			if (!s->empty()) lastNonEmpty = s;

		bool hasAnyGeo = (lastNonEmpty != nullptr);
		for (int i = 0; i < 3; ++i)
			splitByDim(i + 1, *dimSets[i], dimSets[i] != lastNonEmpty);
        mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
        // 仅在 GUI 模式下访问 mInfoWindow，headless 批量模式下无 PreRenderWidget
        if (getCVM->getPreRenderWidget() != nullptr)
        {
            mInfoWindow::getInstance()->isMeshing = false;
        }
        if (!hasAnyGeo)
        {
            //mGlobalSignals::getInstance()->outputMessageSig(1, tr("模型无几何部件，可能未导入"), true);
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            emit done("NoGeometry");
            return;
        }
		vector<int> partIds = meshdata->getAllPartIds();
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
		emit done("OK", partIds);
	}

	void mMeshThread::slot_createMeshByPartId(vector<int> partIds)
	{
		/* 3. 同步剖分：保证实体被装进 _meshEntity */
		MechModelData::mModelData* modelData = getCVM->getModelData();
		mMeshData* meshdata = modelData->getMeshData();

		auto geoDatabase = getCVM->getModelData()->getGeoData();
		vector<int> solidids1D;
		vector<int> solidids2D;
		vector<int> solidids3D;

		for (auto partId : partIds)
		{
			vector<int> ids1D = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 1);
			vector<int> ids2D = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 2);
			vector<int> ids3D = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 3);
			solidids1D.insert(solidids1D.end(), ids1D.begin(), ids1D.end());
			solidids2D.insert(solidids2D.end(), ids2D.begin(), ids2D.end());
			solidids3D.insert(solidids3D.end(), ids3D.begin(), ids3D.end());
            if (!solidids1D.size() && !solidids2D.size() && !solidids3D.size())
            {
                continue;
            }
            meshdata->deleteMeshPart(meshdata->getPartNameByPartId(partId));
		}

		vector<set<int>> meshGeoId;
		meshGeoId.resize(3);

		//set<int> meshGeoId;
		for (auto mesh_geo_id : solidids1D)
		{
			meshGeoId[0].insert(mesh_geo_id);
		}
		for (auto mesh_geo_id : solidids2D)
		{
			meshGeoId[1].insert(mesh_geo_id);
		}
		for (auto mesh_geo_id : solidids3D)
		{
			meshGeoId[2].insert(mesh_geo_id);
		}

        if (!meshGeoId[0].size()&& !meshGeoId[1].size()&& !meshGeoId[2].size())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, tr("无几何部件，不支持网格生成操作"), true);
            emit done("OK", partIds);
            return;
        }

		// 获取对应几何维度的所有几何ID并进行一键剖分
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }

		/*vector<int> meshGeoIdByPart = MGeoNeutralQuery::geoIdsByPartId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), dim, partId);
		set<int> meshGeoId;
		meshGeoId.insert(meshGeoIdByPart.begin(), meshGeoIdByPart.end());*/
		auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
			{

				MeshEntity* entity = meshdata->getMeshEntity();
				std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
				if (isMesh)
				{
                    entity->resetPreMeshSize(dim, MAX_LC);   // 重置尺寸！！！！
					entity->setMeshPattern(MeshEntity::MeshPattern::AUTOMATED_MESHING);
					mesh->setMeshData(meshdata);
					mesh->MXMeshInterface(10e10, 0.1, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, meshGeoId[dim - 1], {});           // 真正剖分
					//meshdata->ensureMesh();//网格剖分后里存底层数据的接口
				}
				return meshGeoId[dim - 1];
			};
        
		if (!solidids1D.empty())
        {
            getAndSplitMeshIDsByDim(1, true);           
        }
				if (!solidids2D.empty())
        {
					getAndSplitMeshIDsByDim(2, true);
        }

				if (!solidids3D.empty())
        {
					getAndSplitMeshIDsByDim(3, true);
        }


		/*if (meshGeoId.size() == 0)
		{
			mGlobalSignals::getInstance()->outputMessageSig(1, tr("模型无几何部件，可能未拾取"), true);
		}*/

		/* 4. 刷新视图 */

		/*QVector<QString> names;
		for (int id : NoMeshSolid)
			names << MeshMessage::getInstance()->getPartNameByID(id);

		MeshMessage::getInstance()->setRenderState(names);*/
		
		mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
	/*	getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
		getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();*/

		// 关闭几何拾取高亮s
		//mViewportManager::getInstance()->clearAllPickData();
		//emit mToPoGlobalSignals2::getInstance()->updateBrowserTreeSig(Common::MeshParentItem2);//更新网格树
		//mViewportManager::getInstance()->setPickFilter(PickFilter::PickNothing);//网格剖分后，关闭拾取模式。
		emit done("OK", partIds);

	}

	void mMeshThread::slot_createMesh(int id,QStringList list)
	{
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
		// 将面板中的中文网格类型转化为MeshShapeType
		auto getMeshShape = [&](const QString& shape, int dim, MeshShapeType& current_type)
		{
			switch (dim)
			{
				// 二维网格
				case 2:
				{
					if (strcmp(shape.toStdString().c_str(), "三角形") == 0)
					{
						current_type = MeshShapeType::TRI_MESH;
					}
					else if (strcmp(shape.toStdString().c_str(), "四边形主导") == 0)
					{
						current_type = MeshShapeType::QUAD_DOMINT_MESH;
					}
                    else if (strcmp(shape.toStdString().c_str(), "全四边形") == 0)
                    {
                        current_type = MeshShapeType::QUAD_MESH;
                    }

				}break;

				// 三维网格
				case 3:
				{
					if (strcmp(shape.toStdString().c_str(), "四面体") == 0)
					{
						current_type = MeshShapeType::TET_MESH;
					}
					else if (strcmp(shape.toStdString().c_str(), "六面体") == 0)
					{
						current_type = MeshShapeType::HEX_MESH;
					}
				}break;

				default:break;
			}
		};

		// 从网格面板中获取网格尺寸和网格类型
		double size = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id)->getMeshSize();
		QString shape = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id)->getMeshShape();
        QString order = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(id)->getMeshOrder();

		// 获取modeldata和meshdata
		MechModelData::mModelData* modelData = getCVM->getModelData();
		mMeshData* meshdata = modelData->getMeshData();
		MGeoData::GeoDataBase* geoDatabase = modelData->getGeoData();

		// 过滤已不存在的剖分几何面对象：几何删除后 _list 中仍残留"几何面_ID"旧引用，
		// 与面板回显共用同一存在性判断（m_idToGFace 查不到或部件归属全部失效），避免对已删除几何继续生成网格
		QStringList aliveList;
		for (const QString& s : list)
		{
			if (s.section('_', 0, 0) == QString("几何面")
				&& !MechMainWindow::mFaceMeshPanel::isFaceMeshObjectAlive(s.mid(s.lastIndexOf('_') + 1).toInt()))
			{
				continue;
			}
			aliveList << s;
		}
		if (aliveList.size() < list.size())
		{
			mGlobalSignals::getInstance()->outputMessageSig(1, tr("已跳过%1个已不存在的剖分对象").arg(list.size() - aliveList.size()), true);
		}
		if (aliveList.empty())
		{
			//与正常完成路径一致：复位界面禁用状态（模型树、菜单栏）后再结束
			mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
			emit done("OK");
			return;
		}
		list = aliveList;

		// 获取之前一键剖分完成的几何ID
		std::set<QString> setA(_finishMeshList.begin(), _finishMeshList.end());
		// 获取当前剖分列表的几何ID
		std::set<QString> setB(list.begin(), list.end());

		// 取交集获取重复剖分的几何实体名称
		std::set<QString> intersect;
		std::set_intersection(setB.begin(), setB.end(),
			setB.begin(), setB.end(),
			std::inserter(intersect, intersect.begin()));

		// 获取重复剖分的几何ID
		std::set<int> geoIds;
		for (const QString& s : intersect) 
		{
			// 从最后一个 '_' 往后就是数字
			int id = s.mid(s.lastIndexOf('_') + 1).toInt();
			geoIds.insert(id);
		}

		std::vector<int> solidids;
		for (const QString& s : list)
		{
			solidids.push_back(s.section('_', 1, 1).toInt());
		}
			
		// 根据几何实体名称获取网格维度
		int dim = 3;
		if (list[0].contains("实体"))
			dim = 3;
		else if (list[0].contains("几何面"))
			dim = 2;
		else if (list[0].contains("几何线"))
			dim = 1;

        //考虑虚拓扑
        std::set<int> vToggleGeoIds;
        std::set<int> toggleEdgeIds;
        auto meshEntity = meshdata->getMeshEntity();
        if (meshEntity != nullptr&& dim >1)
        {

            set<int>meshGfaceid ;

            if (dim == 2)
            {
                meshGfaceid = geoIds;

                for (auto ita : meshGfaceid)
                {
                    auto gfaces = meshEntity->getGeoFaces();
                    for (auto gface : gfaces)
                    {
                        if (gface == nullptr)
                        {
                            continue;
                        }
                        if (!gface->toggleFace)
                        {
                            if (gface->getGeoTag() == ita && gface->TOGGLE != 2)
                            {
                                vToggleGeoIds.insert(ita);
                            }
                        }
                        else
                        {
                            if (gface->toggleFace->getGeoTag() == ita)
                            {
                                vToggleGeoIds.insert(gface->getGeoTag());

                                MXGeoFace* toggleFace = gface->toggleFace;
                                for (auto gedge : gface->getEdgesOfGeoFaces())
                                {
                                    if (gedge == nullptr)
                                    {
                                        continue;
                                    }
                                    if (gedge->getEdgeType() == 1)
                                    {
                                        toggleEdgeIds.insert(gedge->getGeoTag());
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (dim == 3)
            {
                for (auto itb : meshEntity->getGeoSolids())
                {
                    if (geoIds.find(itb->getGeoTag()) != geoIds.end())
                    {
                        for (auto itc : itb->getface())
                        {
                            for (auto itd : itc->getEdgesOfGeoFaces())
                            {
                                if (itd->getEdgeType() == 1)
                                {
                                    toggleEdgeIds.insert(itd->getGeoTag());
                                }
                            }
                        }
                    }
                }
            }
        }
        if (dim == 2)
        {
            solidids.clear();
            for (auto ita : vToggleGeoIds)
            {
                solidids.push_back(ita);
            }
        }
		// 删除重复剖分几何实体中原来的网格
        if (dim == 2)
        {
            //判断待剖分几何面所在的实体是否完成实体剖分，若完成则不能对几何面进行生成
            QVector<int>delTags;
            for (auto ita : geoIds)
            {
                MXGeoFace* gface = meshdata->getMeshEntity()->m_idToGFace[ita];
                for (auto itb : meshdata->getMeshEntity()->getGeoSolids())
                {
                    if (itb->getMeshStatic() == MeshDone)
                    {
                        if (itb->getface().contains(gface))
                        {
                            delTags.append(ita);
                            break;
                        }
                    }
                }
            }
            for (auto ita : delTags)
            {
                geoIds.erase(geoIds.find(ita));
                auto itb = find(solidids.begin(), solidids.end(), ita);
                if (itb != solidids.end())
                {
                    solidids.erase(itb);
                }
                QString facePartName = tr("未知部件");
                MXGeoFace* errFace = meshdata->getMeshEntity()->getGeoFaceById(ita);
                if (errFace && !QString::fromUtf8(errFace->getPartName().c_str()).isEmpty())
                {
                    facePartName = QString::fromUtf8(errFace->getPartName().c_str());
            }
                else if (geoDatabase) {
                    //【OCC摘除·S9】旧栈反查退役，改 GN 部件名
                    facePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, ita);
                }
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何面-%2所在实体已完成实体剖分，不支持进行面网格重划分").arg(facePartName).arg(ita), true);
            }
          
			meshdata->deleteElementByGeoFace(geoIds, QHash<int, std::vector<int>>());
            for (auto itc : geoIds)
            {
                std::vector<RenderPickResultData>es= meshdata->get2DElementByGeoFace(itc);
                int tt = 0;
            }

		}
		else if (dim == 3)
		{
			for (auto id : geoIds)
			{
                //【OCC摘除·S9】旧栈 set 反查退役（恒 null 解引用崩溃隐患），改 GN 部件名
                auto name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, id);
				meshdata->deleteMeshPart(name);
			}
		}
		else if (dim == 1)
		{
		    QVector<int>delTags;
		    for (auto ita : geoIds)
		    {
		        MXGeoEdge* gedge = meshdata->getMeshEntity()->m_idToGEdge[ita];
		        if (!gedge)
		        {
		            continue;
		        }
		        for (auto itb : gedge->getLinkFaces())
		        {
		            if (itb->getMeshStatic() == MeshDone)
		            {
		                delTags.append(ita);
		                break;
		            }
		        }
		    }
		    for (auto ita : delTags)
		    {
		        geoIds.erase(geoIds.find(ita));
		        auto itb = find(solidids.begin(), solidids.end(), ita);
		        if (itb != solidids.end())
		        {
		            solidids.erase(itb);
		        }
		        QString edgePartName = tr("未知部件");
		        MXGeoEdge* errEdge = meshdata->getMeshEntity()->getGeoEdgeById(ita);
		        if (errEdge && !QString::fromUtf8(errEdge->getPartName().c_str()).isEmpty())
		        {
		            edgePartName = QString::fromUtf8(errEdge->getPartName().c_str());
		        }
		        else if (geoDatabase)
		        {
		            { //【OCC摘除·S9】旧栈遍历退役，改 GN 部件名（按边 ID 反查）
		            edgePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, ita);
	            }
		        }
		        mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何边-%2所在几何面已完成面剖分，不支持进行线网格重划分").arg(edgePartName).arg(ita), true);
		    }
			meshdata->deleteElementByGeoEdge(geoIds);
		}
		//meshdata->resetAllMeshStatic();
	
		std::unordered_map<uint, MeshShapeType>mesh_shape_type;
		MeshShapeType current_type;
		getMeshShape(shape, dim, current_type);

		set<int> meshGeoId;
		for (auto mesh_geo_id : solidids)
		{

			mesh_shape_type[mesh_geo_id] = current_type;
			meshGeoId.insert(mesh_geo_id);
		}
        // 全部待剖分对象均被上层已完成状态过滤剔除时直接结束，
        // 与正常完成路径一致复位界面禁用状态并发出终态信号，避免线程悬挂
        if (meshGeoId.size() == 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, tr("没有可剖分的几何对象，剖分结束"), true);
            // 进度条只认 100 才会延时隐藏（mStatusBar::progressBarHideOrShow），
            // 早退路径网格引擎不会再上报进度，必须在此收尾
            mGlobalSignals::getInstance()->setProgressBarValue(100);
            mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
            emit done("OK");
            return;
        }
        if (strcmp(order.toStdString().c_str(), "二阶") == 0)
        {
            _meshorder = 2;
        }
        else
        {
            _meshorder = 1;
        }
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }

		// 获取对应几何维度的所有几何ID并进行一键剖分
		auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
			{
				//std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(dim, geoDatabase);
				MeshEntity* entity = meshdata->getMeshEntity();
				std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
				if (isMesh&& meshGeoId.size())
				{
                    MXMeshParameters::getInstance()->_meshOrder = _meshorder;
					entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
				    entity->resetPreMeshSize(dim, MAX_LC);   // 重置尺寸！！！！
					entity->setPreMeshSize(dim, size);
					mesh->saveMeshShape(dim, mesh_shape_type);
					mesh->setMeshData(meshdata);
					mesh->MXMeshInterface(dim, entity, meshGeoId, toggleEdgeIds);  // 真正剖分
					//meshdata->ensureMesh();//网格剖分后里存底层数据的接口
				}
				return meshGeoId;
			};
		getAndSplitMeshIDsByDim(dim, true);
        if (meshGeoId.size())
        {
            mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
        }
		_finishMeshList = list;
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
		getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
		getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("biastNum");
		getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("adjustNum");
		set<int> partSet;
		if (dim == 3)
		{
			for (auto id : solidids)
			{
				partSet.insert(meshdata->getPartIdByGeoSolid(id));
			}
		}
		else if (dim == 2)
		{
			for (auto id : solidids)
			{
				partSet.insert(meshdata->getPartIdByGeoFace(id));
			}
		}
		else if (dim == 1)
		{
			for (auto id : solidids)
			{
				partSet.insert(meshdata->getPartIdByGeoEdge(id));
			}
		}
		vector<int> partIds;
		for (auto id : partSet)
		{
			partIds.push_back(id);
		}
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
		emit done("OK",partIds);
	}

	void mMeshThread::slot_createMeshBySetting()
	{
        //全局尺寸
        double globalSize = getCVM->getModelData()->getManager<MeshPanelData::mMeshPanelManager>(Physical_MeshPanel)._globalSize;
        //获取所有子节点参数表
        getAllMeshParamMap();
		//判断三维实体与面的MeshShapeType是否冲突
		MechModelData::mModelData* modelData = getCVM->getModelData();
		mMeshData* meshdata = modelData->getMeshData();
		MeshEntity* mesh_entity = meshdata->getMeshEntity();
		mesh_entity->buildGeoIdMap();
		// 三维
		for (auto solid_iter : solidResultMap)
		{
			MXGeoSolid* solid = mesh_entity->getGeoSolidById(solid_iter.first);
			if (solid_iter.second.second == MeshShapeType::TET_MESH)
			{
				QVector<MXGeoFace*> faces = solid->getface();
				for (auto face : faces)
				{
					auto id = face->getGeoTag();
					auto it = faceResultMap.find(id);
					if (it != faceResultMap.end()&&it->second.second!= MeshShapeType::TRI_MESH) {			
						emit done("Error");
						return;
					}
				}
			}		
		}

        // 设置剖分模式
        mesh_entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
        mesh_entity->resetPreMeshSize(3, MAX_LC);   // 重置尺寸！！！！
        mesh_entity->resetPreMeshSize(2, MAX_LC);   // 重置尺寸！！！！
        mesh_entity->resetPreMeshSize(1, MAX_LC);   // 重置尺寸！！！！

        // 根据维度以及GeoTag 设置网格参数 <尺寸> <网格剖分形状>
        auto geoDatabase = getCVM->getModelData()->getGeoData();

        // 剖分Id容器
        std::set<int> meshGeoId_solid, meshGeoId_face,meshGeoId_line;

        // 所有待剖分的一维几何线Id
        for (const auto & edge_iter: lineResultMap)
        {
            MXGeoEdge * edge = mesh_entity->getGeoEdgeById(edge_iter.first);
            bool HAVE = false;
            for (auto itb : edge->getLinkFaces())
            {
                if (itb->getMeshStatic() == MeshDone)
                {
                    HAVE = true;
                    break;
                }
            }
            if (HAVE)
            {
                QString edgePartName = tr("未知部件");
                MXGeoEdge* edge = mesh_entity->getGeoEdgeById(edge_iter.first);
                if (edge && !QString::fromUtf8(edge->getPartName().c_str()).isEmpty())
                {
                    edgePartName = QString::fromUtf8(edge->getPartName().c_str());
                }
                else if (geoDatabase)
                {
                    { //【OCC摘除·S9】旧栈遍历退役，改 GN 部件名（按边 ID 反查）
		            edgePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, edge_iter.first);
	            }
                }
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何边-%2所在几何面已完成面剖分，不支持进行线网格重划分").arg(edgePartName).arg(edge_iter.first), true);
                continue;
            }
            double smaller = std::min(edge_iter.second, globalSize);
            mesh_entity->setPreMeshSizeOnGeoEdge(edge, smaller);
            meshGeoId_line.insert(edge_iter.first);
        }
        for (const auto& edge_iter : localResultMap)
        {
            MXGeoEdge* edge = mesh_entity->getGeoEdgeById(edge_iter.first);
            double smaller = std::min(edge_iter.second, globalSize);
            mesh_entity->setPreMeshSizeOnGeoEdge(edge, smaller);
        }

        // 所有待剖分的二维几何面Id
        std::set<int> geofaceIds;
        for (const auto& face_iter : faceResultMap)
        {
            MXGeoFace* face = mesh_entity->getGeoFaceById(face_iter.first);
            bool HAVE = false;
            for (auto itb : meshdata->getMeshEntity()->getGeoSolids())
            {
                if (itb->getMeshStatic() == MeshDone)
                {
                    if (itb->getface().contains(face))
                    {
                        HAVE = true;
                        break;
                    }
                }
            }
            if (HAVE)
            {
                QString facePartName = tr("未知部件");
                MXGeoFace* face = mesh_entity->getGeoFaceById(face_iter.first);
                if (face && !QString::fromUtf8(face->getPartName().c_str()).isEmpty())
                {
                    facePartName = QString::fromUtf8(face->getPartName().c_str());
                }
                else if (geoDatabase)
                {
                    //【OCC摘除·S9】旧栈反查退役，改 GN 部件名
                    facePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, face_iter.first);
                }
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何面-%2所在实体已完成实体剖分，不支持进行面网格重划分").arg(facePartName).arg(face_iter.first), true);
                continue;
            }

            double smaller = std::min(face_iter.second.first, globalSize);
            mesh_entity->setPreMeshSizeOnGeoFace(face, smaller);
            auto param = face->getMeshParam();  // 开启尺寸光滑 否则没有尺寸渐变效果
            param.smoothness.enable = true;
            face->setMeshParam(param);
            face->setMeshShape(face_iter.second.second);
            meshGeoId_face.insert(face_iter.first);
            geofaceIds.insert(face->getGeoTag());
        }
        meshdata->deleteElementByGeoFace(geofaceIds, QHash<int, std::vector<int>>());

        // 所有待剖分的三维实体Id
        for (const auto& solid_iter : solidResultMap)
        {
            MXGeoSolid* solid = mesh_entity->getGeoSolidById(solid_iter.first);
            double smaller = std::min(solid_iter.second.first, globalSize);
            mesh_entity->setPreMeshSizeOnGeoSolid(solid, smaller);
            solid->setMeshShape(solid_iter.second.second);
            meshGeoId_solid.insert(solid_iter.first);
            auto a = std::make_pair(MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, solid_iter.first),
    MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, solid_iter.first)); //【OCC摘除·S9】GN 等价 pair
            auto name = a.first;
            meshdata->deleteMeshPart(name);
        }
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
       // 剖分维度顺序： 低维度->高维度
       // TODO:在剖分前剔除高维度下面所有的低维度id
        MeshEntity* entity = meshdata->getMeshEntity();
        std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
        auto getAndSplitMeshIDsByDimTest = [&](int dim, const std::set<int>& meshGeoId)
        {
            std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
            mesh->setMeshData(meshdata);
            mesh->MXMeshInterface(dim, entity, meshGeoId, {});          // 真正剖分
            meshdata->ensureMesh();                                     // 网格剖分后里存底层数据的接口
        };
        if (meshGeoId_line.size())
        {
            getAndSplitMeshIDsByDimTest(1, meshGeoId_line);
        }
        if (meshGeoId_face.size())
        {
            getAndSplitMeshIDsByDimTest(2, meshGeoId_face);
        }
        if (meshGeoId_solid.size())
        {
            getAndSplitMeshIDsByDimTest(3, meshGeoId_solid);
        }

        // 收集所有被剖分影响的部件ID
        set<int> partSet;
        for (auto id : meshGeoId_line)
        {
            partSet.insert(meshdata->getPartIdByGeoEdge(id));
        }
        for (auto id : meshGeoId_face)
        {
            partSet.insert(meshdata->getPartIdByGeoFace(id));
        }
        for (auto id : meshGeoId_solid)
        {
            partSet.insert(meshdata->getPartIdByGeoSolid(id));
        }
        vector<int> partIds;
        for (auto id : partSet)
        {
            partIds.push_back(id);
        }

        //mBrowserWindow::getInstance()->getBrowserTree()->updateMeshStatus();
  emit done("OK", partIds);
	}

	void mMeshThread::slot_createMesh1()
	{
		//生成网格
		double size = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel)._globalSize;
		/* 3. 同步剖分：保证实体被装进 _meshEntity */
		MechModelData::mModelData* modelData = getCVM->getModelData();
		mMeshData* meshdata = modelData->getMeshData();

		auto geoDatabase = getCVM->getModelData()->getGeoData();
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
		// 获取对应几何维度的所有几何ID并进行一键剖分
		auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
			{
				std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(dim);
				MeshEntity* entity = meshdata->getMeshEntity();
				std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
				if (isMesh)
				{
					entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
					entity->resetPreMeshSize(dim, MAX_LC);  // 重置尺寸！！！
					entity->setPreMeshSize(dim, size);
					mesh->setMeshData(meshdata);
					mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, meshGeoId, {});           // 真正剖分
					//meshdata->ensureMesh();//网格剖分后里存底层数据的接口
				}
				return meshGeoId;
			};
		if (!getGEOServer.geoCalculate().getAllGeoID(3).empty())
			getAndSplitMeshIDsByDim(3, true);
		else if (!getGEOServer.geoCalculate().getAllGeoID(2).empty())
			getAndSplitMeshIDsByDim(2, true);
		else if (!getGEOServer.geoCalculate().getAllGeoID(1).empty())
			getAndSplitMeshIDsByDim(1, true);


		mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
		vector<int> partIds = meshdata->getAllPartIds();
		emit done("OK", partIds);
	}

	void mMeshThread::slot_createMeshAutoAdapt(int dim, const std::set<int>& geo_ids, MeshParam mesh_param)
	{
		if (geo_ids.size() == 0)
		return;
		
		if (geo_ids.size() == 0)
			return;

		// 调试专用 by ysq 勿删！！！！
		// mesh_param.proximity.enable = true;
		// mesh_param.proximity.min_layer = 2;
		// mesh_param.curvature.enable = true;
		// mesh_param.curvature.per_cycle_num = 15;
		// mesh_param.smoothness.enable = true;
		// mesh_param.smoothness.growth_rate = 1.2;
		// mesh_param.max_size = 100;
		// mesh_param.min_size = 5;

		MeshEntity* entity = meshdata->getMeshEntity();
		std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
		if (mesh_param.proximity.enable) {
			MXMeshParameters::getInstance()->OnlyInitialTri = true;
			// 如果开启几何自适应后可以给几何边设置一个离散尺寸用于生成初始三角形
			// 使用合适的离散尺寸可以带来接近100%的性能提升
			// 注意这里的尺寸给的不合适（偏大）会导致线剖分性能急剧下降！！
			// entity->resetPreMeshSize(1, 5);  
			// entity->getPreMeshSize(1);
		}
		else
			MXMeshParameters::getInstance()->OnlyInitialTri = false;

        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
		entity->resetPreMeshSize(dim, MAX_LC);  // 每次都需要重置尺寸！！！
		entity->setMeshPattern(MeshEntity::MeshPattern::ADAPT_MESH);
		entity->setMeshParam(dim, geo_ids, mesh_param);
		mesh->setMeshData(meshdata);
        std::set<int> filtered_geo_ids = geo_ids;
        // 删除重复剖分几何实体中原来的网格
        if (dim == 2)
        {
            //判断待剖分几何面所在的实体是否完成实体剖分，若完成则不能对几何面进行生成
            QVector<int>delTags;
            for (auto ita : geo_ids)
            {
                MXGeoFace* gface = meshdata->getMeshEntity()->m_idToGFace[ita];
                for (auto itb : meshdata->getMeshEntity()->getGeoSolids())
                {
                    if (itb->getMeshStatic() == MeshDone)
                    {
                        if (itb->getface().contains(gface))
                        {
                            delTags.append(ita);
                            break;
                        }
                    }
                }
            }
            for (auto ita : delTags)
            {
                filtered_geo_ids.erase(ita);
                QString facePartName = tr("未知部件");
                MXGeoFace* errFace = meshdata->getMeshEntity()->getGeoFaceById(ita);
                if (errFace && !QString::fromUtf8(errFace->getPartName().c_str()).isEmpty())
                {
                    facePartName = QString::fromUtf8(errFace->getPartName().c_str());
                }
                else
                {
                    auto* adaptGeoDatabase = getCVM->getModelData()->getGeoData();
                    if (adaptGeoDatabase)
                    {
                        //【OCC摘除·S9】旧栈反查退役，改 GN 部件名
                    facePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, ita);
                    }
                }
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何面-%2所在实体已完成实体剖分，不支持进行面网格重划分").arg(facePartName).arg(ita), true);
            }

            meshdata->deleteElementByGeoFace(filtered_geo_ids, QHash<int, std::vector<int>>());
            for (auto itc : filtered_geo_ids)
            {
                std::vector<RenderPickResultData>es = meshdata->get2DElementByGeoFace(itc);
                int tt = 0;
            }

        }
        else if (dim == 3)
        {
            for (auto id : geo_ids)
            {
                //【OCC摘除·S9】旧栈 set 反查退役（恒 null 解引用崩溃隐患），改 GN 部件名
                auto name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, id);
                meshdata->deleteMeshPart(name);
            }
        }
        else if (dim == 1)
        {
            QVector<int>delTags;
            for (auto ita : geo_ids)
            {
                MXGeoEdge* gedge = meshdata->getMeshEntity()->m_idToGEdge[ita];
                if (!gedge)
                {
                    continue;
                }
                for (auto itb : gedge->getLinkFaces())
                {
                    if (itb->getMeshStatic() == MeshDone)
                    {
                        delTags.append(ita);
                        break;
                    }
                }
            }
            std::set<int> filtered_geo_ids = geo_ids;
            for (auto ita : delTags)
            {
                filtered_geo_ids.erase(filtered_geo_ids.find(ita));
                QString edgePartName = tr("未知部件");
                MXGeoEdge* errEdge = meshdata->getMeshEntity()->getGeoEdgeById(ita);
                if (errEdge && !QString::fromUtf8(errEdge->getPartName().c_str()).isEmpty())
                {
                    edgePartName = QString::fromUtf8(errEdge->getPartName().c_str());
                }
                else
                {
                    auto* adaptGeoDatabase = getCVM->getModelData()->getGeoData();
                    if (adaptGeoDatabase)
                    {
                        { //【OCC摘除·S9】旧栈遍历退役，改 GN 部件名（按边 ID 反查）
		            edgePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, ita);
	            }
                    }
                }
                mGlobalSignals::getInstance()->outputMessageSig(1, tr("%1-几何边-%2所在几何面已完成面剖分，不支持进行线网格重划分").arg(edgePartName).arg(ita), true);
            }
            meshdata->deleteElementByGeoEdge(filtered_geo_ids);
        }
        if (filtered_geo_ids.size())
        {
            mesh->MXMeshInterface(dim, entity, filtered_geo_ids, {});
            //meshdata->ensureMesh();
            mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);
        }
		set<int> partSet;
		if (dim == 3)
		{
			for (auto id : geo_ids)
			{
				partSet.insert(meshdata->getPartIdByGeoSolid(id));
			}
		}
		else if (dim == 2)
		{
			for (auto id : geo_ids)
			{
				partSet.insert(meshdata->getPartIdByGeoFace(id));
			}
		}
		else if (dim == 1)
		{
			for (auto id : geo_ids)
			{
				partSet.insert(meshdata->getPartIdByGeoEdge(id));
			}
		}
		vector<int> partIds;
		for (auto id : partSet)
		{
			partIds.push_back(id);
		}
		emit done("OK", partIds);
	}

	void mMeshThread::slot_createMeshCustomEncryption()
	{
		cout << 1;
	}

    static void LoopFindToggleFace(QVector<MXGeoFace*>faces, QHash<int, MXGeoFace*>& lface, int tag, set<int>& toggleID) {
        for (auto& itf : faces) {
            lface.insert(itf->getGeoTag(), itf);
            for (auto& ite : itf->getEdgesOfGeoFaces()) {
                if (ite->getGeoTag() != tag) {
                    auto its1 = toggleID.find(ite->getGeoTag());
                    if (its1 != toggleID.end()) {
                        toggleID.erase(its1);
                        QVector<MXGeoFace*>faces1;
                        for (auto& itg : ite->getLinkFaces()) {
                            if (!lface.contains(itg->getGeoTag())) {
                                faces1.append(itg);
                            }
                        }
                        LoopFindToggleFace(faces1, lface, ite->getGeoTag(), toggleID);
                        //ite->_mLines.clear();

                    }
                }
            }
        }
    }

    static void LoopFindToggleFace1(MXGeoFace* face, QHash<int, MXGeoFace*>& lface, int tag) {

        lface.insert(face->getGeoTag(), face);
        for (auto ita : face->getEdgesOfGeoFaces())
        {
            if (ita->getGeoTag() == tag)
            {
                continue;
            }
            if (ita->getEdgeType() == 1)
            {
                for (auto itb : ita->getLinkFaces())
                {
                    if (itb->getGeoTag() != face->getGeoTag() && !lface.contains(itb->getGeoTag()))
                    {
                        LoopFindToggleFace1(itb, lface, ita->getGeoTag());
                    }
                }
            }
        }


    }

    static void buildToggleMessage(bool OK, set<int>nowToggleID, set<int> toggleID, QVector<QHash<int, MXGeoFace*>>& lfaces, mModelData* modeldata,
        QVector<MXGeoFace*>* outNewToggleFaces = nullptr, QVector<MXGeoEdge*>* outNewCombineEdges = nullptr)
    {


        if (!modeldata->getMeshData()->getMeshEntity()->GeoEdgeTag)
        {
            for (auto ita : modeldata->getMeshData()->getMeshEntity()->getGeoEdges())
            {
                if (modeldata->getMeshData()->getMeshEntity()->GeoEdgeTag < ita->getGeoTag())
                {
                    modeldata->getMeshData()->getMeshEntity()->GeoEdgeTag = ita->getGeoTag();
                }
            }
        }
        if (!modeldata->getMeshData()->getMeshEntity()->GeoFaceTag)
        {
            for (auto ita : modeldata->getMeshData()->getMeshEntity()->getGeoFaces())
            {
                if (modeldata->getMeshData()->getMeshEntity()->GeoFaceTag < ita->getGeoTag())
                {
                    modeldata->getMeshData()->getMeshEntity()->GeoFaceTag = ita->getGeoTag();
                }
            }
        }
    


        //获取toggle边连接的几何面
        //QVector<QHash<int, MXGeoFace*>>lfaces;
        QVector<MXGeoEdge*>toggleEdges;
        for (auto& it : modeldata->getMeshData()->getMeshEntity()->getGeoEdges()) {
            QHash<int, MXGeoFace*>lface;
            auto its = toggleID.find(it->getGeoTag());
            if (its != toggleID.end()) {
                toggleID.erase(its);
                QVector<MXGeoFace*>faces = it->getLinkFaces();
                LoopFindToggleFace(faces, lface, it->getGeoTag(), toggleID);
                lfaces.append(lface);
                toggleEdges.append(it);
                //it->_mLines.clear();
            }
        }
        if (!OK)
        {
            set<int>tempnowToggleID = nowToggleID;
            QVector<MXGeoEdge*>deleteCombineEdges;
            QVector<QHash<int, MXGeoFace*>> cancelfaces;
            //取消toggle边后更新虚拓扑面
            for (auto& it : modeldata->getMeshData()->getMeshEntity()->getGeoEdges())
            {
                QVector<QHash<int, MXGeoFace*>>lface;
                auto its = tempnowToggleID.find(it->getGeoTag());
                if (its != tempnowToggleID.end()) {
                    QVector<MXGeoFace*>faces = it->getLinkFaces();
                    for (auto itb : faces)
                    {
                        QHash<int, MXGeoFace*>linklface;
                        LoopFindToggleFace1(itb, linklface, it->getGeoTag());
                        lface.append(linklface);
                    }
                    tempnowToggleID.erase(its);
                }
                if (!lface.size())
                {
                    continue;
                }
                //判断是否要取消该虚拓扑面
                MXGeoFace* toggleface = nullptr;
                for (auto itk : lface)
                {
                    bool CANCEL = true;
                    for (auto itf : itk)
                    {
                        for (auto ite : itf->getEdgesOfGeoFaces())
                        {
                            toggleface = itf->toggleFace;
                            if (ite->getEdgeType() == 1)
                            {
                                CANCEL = false;
                            }
                        }
                    }
                    if (CANCEL)
                    {
                        for (auto itf : itk)
                        {
                            itf->TOGGLE = 0;
                            itf->toggleFace = nullptr;
                        }
                    }
                }
                if (!toggleface)
                {
                    continue;
                }
                for (auto& ite : toggleface->getEdgesOfGeoFaces())
                {
                    if (ite->getCombineEdge())
                    {
                        if (!deleteCombineEdges.contains(ite->getCombineEdge()))
                        {
                            bool IS = false;
                            for (auto ita : ite->getCombineEdge()->subGEdges)
                            {
                                if (ita->getBeginVertex() != it->getBeginVertex() &&
                                    ita->getBeginVertex() != it->getEndVertex())
                                {
                                    IS = true;
                                }
                                if (ita->getEndVertex() != it->getBeginVertex() &&
                                    ita->getEndVertex() != it->getEndVertex())
                                {
                                    IS = true;
                                }

                            }
                            if (IS)
                            {
                                deleteCombineEdges.append(ite->getCombineEdge());
                            }
                        }
                    }
                }

                //取消虚拓扑面
                set<int> facetag;
                facetag.insert(toggleface->getGeoTag());
                QHash<int, vector<int>> faceToedge;
                vector<int>edgetags;
                for (auto& ite : toggleface->getEdgesOfGeoFaces())
                {
                    edgetags.push_back(ite->getGeoTag());
                }
                faceToedge.insert(toggleface->getGeoTag(), edgetags);
                modeldata->getMeshData()->deleteElementByGeoFace(facetag, faceToedge);

                if (!tempnowToggleID.size())
                {
                    break;
                }
            }
            {
                set<int> geoEdgetag;
                for (auto it : deleteCombineEdges)
                {

                    for (auto ita : it->subGEdges)
                    {
                        //if (ita->getMeshStatic() != NoMesh)
                        {
                            if (ita->getBeginVertex() != it->getBeginVertex() &&
                                ita->getBeginVertex() != it->getEndVertex())
                            {
                                if (ita->getBeginVertex()->meshGeo_vertexs.size())
                                {
                                    ita->getBeginVertex()->setMeshStatic(Meshing);
                                }
                            }
                            if (ita->getEndVertex() != it->getBeginVertex() &&
                                ita->getEndVertex() != it->getEndVertex())
                            {
                                if (ita->getEndVertex()->meshGeo_vertexs.size())
                                {
                                    ita->getEndVertex()->setMeshStatic(Meshing);
                                }
                            }


                            ita->_mLines.clear();
                            ita->meshGeo_vertexs.clear();
                            ita->setMeshStatic(NoMesh);
                            geoEdgetag.insert(ita->getGeoTag());
                            ita->setCombineEdge(nullptr);
                        }
                    }
                }
                modeldata->getMeshData()->deleteElementByGeoEdge(geoEdgetag);
            }

        }

        //构造虚拓扑面
        {
            //set<MXGeoFace*, MXEntityPtrLessThan>togglefaces;
            QVector<MXGeoFace*>togglefaces;
            QVector<MXGeoFace*>deleteToggleFaces;
            QVector<MXGeoEdge*>deleteCombineEdges;

            for (auto it : lfaces) {
                bool IS = false;
                QVector<MXGeoFace*>currenttoggleFaces;
                for (auto& itf : it) {
                    if (itf->getGeoTag() == 2) {
                        int tt = 0;
                    }
                    if (itf->toggleFace) {
                        if (!currenttoggleFaces.contains(itf->toggleFace) && itf->toggleFace)
                        {
                            currenttoggleFaces.append(itf->toggleFace);
                        }
                        if (!togglefaces.contains(itf->toggleFace) && !IS) {
                            if (itf->toggleFace)
                            {
                                togglefaces.append(itf->toggleFace);
                            }
                            IS = true;
                            //break;
                        }
                        if (!togglefaces.contains(itf->toggleFace) && IS) {

                            //itf->toggleFace->_mTriangles.clear();
                            //itf->toggleFace->meshGeo_vertexs.clear();
                            if (!deleteToggleFaces.contains(itf->toggleFace)) {
                                deleteToggleFaces.append(itf->toggleFace);
                            }
                            modeldata->getMeshData()->getMeshEntity()->RemoveGeoFace(itf->toggleFace);

                        }
                    }
                }

                if (!IS) {
                    togglefaces.append(nullptr);
                }
                {

                    for (auto ita : currenttoggleFaces)
                    {

                        //删除之前虚拓扑面上的网格
                        bool HAVE = false;
                        vector<int>edgetags;
                        for (auto itb : ita->getEdgesOfGeoFaces())
                        {
                            auto itc = nowToggleID.find(itb->getGeoTag());
                            if (itc != nowToggleID.end())
                            {
                                HAVE = true;
                            }
                            if (itb->getCombineEdge())
                            {
                                auto itc = find(edgetags.begin(), edgetags.end(), itb->getCombineEdge()->getGeoTag());
                                if (itc != edgetags.end()) {
                                    continue;
                                }
                                edgetags.push_back(itb->getCombineEdge()->getGeoTag());
                                continue;
                            }
                            edgetags.push_back(itb->getGeoTag());
                        }
                        if (HAVE)
                        {
                            set<int> geoFace;
                            QHash<int, vector<int>> faceToedge;
                            geoFace.insert(ita->getGeoTag());

                            faceToedge.insert(ita->getGeoTag(), edgetags);
                            if (ita->getMeshStatic() == MeshDone)
                            {
                                modeldata->getMeshData()->deleteElementByGeoFace(geoFace, faceToedge);
                            }

                        }
                    }
                }
            }

            if (togglefaces.size() > 0) {
                for (int k = 0; k < lfaces.size(); k++) {
                    for (auto& itf : lfaces.at(k)) {
                        if (itf->toggleFace) {
                            itf->toggleFace = togglefaces.at(k);
                        }
                        else
                        {
                            itf->toggleFace = togglefaces.at(k);
                            for (auto ite : itf->getEdgesOfGeoFaces()) {
                                if (ite->getCombineEdge()) {
                                    if (!deleteCombineEdges.contains(ite->getCombineEdge())) {
                                        deleteCombineEdges.append(ite->getCombineEdge());
                                    }
                                }

                                ite->setCombineEdge(nullptr);
                            }
                        }
                    }
                }


            }

            for (int m = 0; m < deleteCombineEdges.size(); m++) {
                for (int i = 0; i < deleteCombineEdges[m]->_mLines.size(); i++) {
                    delete  deleteCombineEdges[m]->_mLines[i];
                    deleteCombineEdges[m]->_mLines[i] = nullptr;
                }
                deleteCombineEdges[m]->_mLines.clear();
                deleteCombineEdges[m]->_mLines.shrink_to_fit();
                for (int i = 0; i < deleteCombineEdges[m]->meshGeo_vertexs.size(); i++) {
                    delete  deleteCombineEdges[m]->meshGeo_vertexs[i];
                    deleteCombineEdges[m]->meshGeo_vertexs[i] = nullptr;
                }
                deleteCombineEdges[m]->meshGeo_vertexs.clear();
                deleteCombineEdges[m]->meshGeo_vertexs.shrink_to_fit();

                for (int j = 0; j < deleteCombineEdges[m]->subGEdges.size(); j++) {
                    for (int i = 0; i < deleteCombineEdges[m]->subGEdges[j]->_mLines.size(); i++) {
                        delete  deleteCombineEdges[m]->subGEdges[j]->_mLines[i];
                        deleteCombineEdges[m]->subGEdges[j]->_mLines[i] = nullptr;
                    }
                    deleteCombineEdges[m]->subGEdges[j]->_mLines.clear();
                    deleteCombineEdges[m]->subGEdges[j]->_mLines.shrink_to_fit();
                    for (int i = 0; i < deleteCombineEdges[m]->subGEdges[j]->meshGeo_vertexs.size(); i++) {
                        delete  deleteCombineEdges[m]->subGEdges[j]->meshGeo_vertexs[i];
                        deleteCombineEdges[m]->subGEdges[j]->meshGeo_vertexs[i] = nullptr;
                    }
                    deleteCombineEdges[m]->subGEdges[j]->meshGeo_vertexs.clear();
                    deleteCombineEdges[m]->subGEdges[j]->meshGeo_vertexs.shrink_to_fit();
                    deleteCombineEdges[m]->subGEdges[j]->setMeshStatic(NoMesh);
                }
            }

            for (int i = 0; i < deleteToggleFaces.size(); i++) {
                if (deleteToggleFaces[i]->_mTriangles.size()) {
                    for (int m = 0; m < deleteToggleFaces[i]->_mTriangles.size(); m++) {
                        delete deleteToggleFaces[i]->_mTriangles[m];
                        deleteToggleFaces[i]->_mTriangles[m] = nullptr;
                    }
                    deleteToggleFaces[i]->_mTriangles.clear();
                    deleteToggleFaces[i]->_mTriangles.shrink_to_fit();
                    for (int m = 0; m < deleteToggleFaces[i]->meshGeo_vertexs.size(); m++) {
                        delete deleteToggleFaces[i]->meshGeo_vertexs[m];
                        deleteToggleFaces[i]->meshGeo_vertexs[m] = nullptr;
                    }
                    deleteToggleFaces[i]->meshGeo_vertexs.clear();
                    deleteToggleFaces[i]->meshGeo_vertexs.shrink_to_fit();

                    for (int m = 0; m < deleteToggleFaces[i]->MeshEdgesInGface.size(); m++) {
                        modeldata->getMeshData()->getMeshEntity()->IDToInteriorEdge.remove(deleteToggleFaces[i]->MeshEdgesInGface[m]->getEdgeId());
                        delete deleteToggleFaces[i]->MeshEdgesInGface[m];
                        deleteToggleFaces[i]->MeshEdgesInGface[m] = nullptr;
                    }
                    deleteToggleFaces[i]->MeshEdgesInGface.clear();
                    deleteToggleFaces[i]->MeshEdgesInGface.shrink_to_fit();


                    deleteToggleFaces[i]->boundaryMeshEdgesInGface.clear();
                    deleteToggleFaces[i]->boundaryMeshEdgesInGface.shrink_to_fit();
                }
            }


        }
        for (auto it : lfaces) {
            bool IS = false;
            MXGeoFace* gface = nullptr;
            for (auto& itf : it) {
                //判断所有面是否都已经确定剖分
                if (itf->getMeshStatic() != MeshDone) {
                    IS = true;
                    //gface = itf->toggleFace;
                }
                gface = itf->toggleFace;
            }
            if (!IS && gface) {
                continue;
            }
            bool HAVE = true;
            if (!gface) {
                gface = new MXGeoFace();
                gface->setPartName(it.begin().value()->getPartName());
                gface->setPartID(it.begin().value()->getPartID());
                int tag = ++modeldata->getMeshData()->getMeshEntity()->GeoFaceTag;

                gface->setGeoTag(tag);
                HAVE = false;
                if (outNewToggleFaces) {
                    outNewToggleFaces->append(gface);
                }
            }
            //QVector<MXGeoEdge*>addGedges;
            //if (!HAVE) {
            string partName;
            for (auto& itf : it) {
                /*for (auto &ite : itf->getEdgesOfGeoFaces()) {
                    if (addGedges.contains(ite)) {
                        addGedges.removeOne(ite);
                    }
                    else
                    {
                        addGedges.append(ite);
                    }
                }*/

                partName = itf->getPartName();
                itf->TOGGLE = 1;
                itf->toggleFace = gface;
            }

            gface->setPartName(partName);
            //添加网格边
            //gface->setEdgesOfGeoFaces(addGedges.toStdVector());
            gface->TOGGLE = 2;
            modeldata->getMeshData()->getMeshEntity()->AddGeoFace(gface);
            //}

        }
        for (auto& it : modeldata->getMeshData()->getMeshEntity()->getGeoEdges())
        {
            if (it->getGeoTag() == 5395) {
                int t = 0;
            }
        }


        for (auto it : lfaces) {
            //构造虚拓扑边
            MXGeoFace* gface = nullptr;
            for (auto& itf : it) {
                gface = itf->toggleFace;
            }
            if (!gface) { continue; }

            //QVector<MXGeoEdge*> addGedges = gface->getEdgesOfGeoFaces();

            QVector<MXGeoEdge*> addGedges;
            for (auto& itf : it) {
                for (auto& ite : itf->getEdgesOfGeoFaces()) {
                    if (addGedges.contains(ite)) {
                        addGedges.removeOne(ite);
                    }
                    else
                    {
                        addGedges.append(ite);
                    }
                }
                if (itf->toggleFace) {
                    if (itf->toggleFace != gface) {
                        modeldata->getMeshData()->getMeshEntity()->RemoveGeoFace(itf->toggleFace);
                    }
                }
                itf->toggleFace = gface;
            }
            //gface->RemoveEdgeOfGeoFaces(addGedges.toStdVector());
            gface->RemoveEdgeOfGeoFaces(gface->getEdgesOfGeoFaces().toStdVector());
            gface->setEdgesOfGeoFaces(addGedges.toStdVector());

            QHash<int, QVector<MXGeoEdge*>>mvLinkGEdges;
            for (auto ita : addGedges) {
                QVector<MXGeoEdge*>linkGEdge0 = mvLinkGEdges.value(ita->getBeginVertex()->getGeoTag());
                linkGEdge0.append(ita);
                mvLinkGEdges.insert(ita->getBeginVertex()->getGeoTag(), linkGEdge0);
                QVector<MXGeoEdge*>linkGEdge1 = mvLinkGEdges.value(ita->getEndVertex()->getGeoTag());
                linkGEdge1.append(ita);
                mvLinkGEdges.insert(ita->getEndVertex()->getGeoTag(), linkGEdge1);
            }


            //几何边排序
            MXGeoEdge* firstEdge = addGedges[0];
            addGedges.clear();
            MXGeoPoint* PT = firstEdge->getEndVertex();
            while (!addGedges.contains(firstEdge))
            {
                addGedges.append(firstEdge);
                for (auto it : mvLinkGEdges.value(PT->getGeoTag())) {
                    if (it->getGeoTag() != firstEdge->getGeoTag()) {
                        if (it->getBeginVertex() == PT) {
                            PT = it->getEndVertex();
                        }
                        else if (it->getEndVertex() == PT) {
                            PT = it->getBeginVertex();
                        }
                        firstEdge = it;
                        break;
                    }
                }
            }



            for (int k = 0; k < addGedges.size(); k++) {
                MXGeoEdge* gedge0 = nullptr;
                MXGeoEdge* gedge1 = nullptr;
                if (k == addGedges.size() - 1) {
                    gedge0 = addGedges[k];
                    gedge1 = addGedges[0];
                }
                else
                {
                    gedge0 = addGedges[k];
                    gedge1 = addGedges[k + 1];
                }
                if (gedge0->getGeoTag() == 5395 || gedge1->getGeoTag() == 5395) {
                    int t = 0;
                }
                /*if (gedge0->getGeoTag() == 25|| gedge0->getGeoTag() == 46|| gedge0->getGeoTag() == 47||
                    gedge0->getGeoTag() == 48 || gedge0->getGeoTag() == 49 || gedge0->getGeoTag() == 50||
                    gedge0->getGeoTag() == 51) {
                    int t = 0;
                }*/
                QVector<MXGeoFace*>linkGeoFaces_0 = gedge0->getLinkFaces();
                QVector<MXGeoFace*>linkGeoFaces_1 = gedge1->getLinkFaces();
                if (linkGeoFaces_0.size() != 2 || linkGeoFaces_1.size() != 2) {
                    if (linkGeoFaces_0.size() == 1 && linkGeoFaces_1.size() == 1) {
                        if (linkGeoFaces_0[0]->toggleFace->getGeoTag() == linkGeoFaces_1[0]->toggleFace->getGeoTag()) {
                            //判断两点是否连接取消的toggle边
                            if (!OK) {
                                //获取取消的toggle边的几何点 
                                QHash<int, MXGeoEdge*>tagToGEdge;
                                for (auto itm : modeldata->getMeshData()->getMeshEntity()->getGeoEdges()) {
                                    tagToGEdge.insert(itm->getGeoTag(), itm);
                                }
                                set<MXGeoPoint*, MXEntityPtrLessThan>togglePoints;
                                for (auto itn : nowToggleID) {
                                    MXGeoEdge* edge = tagToGEdge.value(itn);
                                    togglePoints.insert(edge->getBeginVertex());
                                    togglePoints.insert(edge->getEndVertex());
                                }

                                auto itp0 = togglePoints.find(gedge0->getBeginVertex());
                                if (itp0 != togglePoints.end()) {
                                    if (gedge0->getCombineEdge()) {
                                        gedge0->getCombineEdge()->subGEdges.removeOne(gedge0);
                                        gedge0->setCombineEdge(nullptr);
                                    }
                                    if (gedge1->getCombineEdge()) {
                                        gedge1->getCombineEdge()->subGEdges.removeOne(gedge1);
                                        gedge1->setCombineEdge(nullptr);
                                    }

                                    continue;
                                }
                                auto itp1 = togglePoints.find(gedge0->getEndVertex());
                                if (itp1 != togglePoints.end()) {
                                    if (gedge0->getCombineEdge()) {
                                        gedge0->getCombineEdge()->subGEdges.removeOne(gedge0);
                                        gedge0->setCombineEdge(nullptr);
                                    }
                                    if (gedge1->getCombineEdge()) {
                                        gedge1->getCombineEdge()->subGEdges.removeOne(gedge1);
                                        gedge1->setCombineEdge(nullptr);
                                    }
                                    continue;
                                }
                            }

                            //通过角度判断两边是否要合并
                            MXGeoPoint* commonPt = nullptr;
                            if (gedge0->getBeginVertex() == gedge1->getBeginVertex()) {
                                commonPt = gedge0->getBeginVertex();
                                SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).low()).unit();
                                SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).low()).unit();
                                double value = pv0.x() * pv1.x() + pv0.y() * pv1.y() + pv0.z() * pv1.z();
                                if (value > -0.85) {
                                    continue;
                                }
                            }
                            else if (gedge0->getBeginVertex() == gedge1->getEndVertex())
                            {
                                commonPt = gedge0->getBeginVertex();
                                SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).low()).unit();
                                SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).high()).unit();
                                double value = pv0.x() * (-pv1.x()) + pv0.y() * (-pv1.y()) + pv0.z() * (-pv1.z());
                                if (value > -0.85) {
                                    continue;
                                }

                            }
                            else if (gedge0->getEndVertex() == gedge1->getBeginVertex()) {
                                commonPt = gedge0->getEndVertex();
                                SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).high()).unit();
                                SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).low()).unit();
                                double value = pv0.x() * (-pv1.x()) + pv0.y() * (-pv1.y()) + pv0.z() * (-pv1.z());
                                if (value > -0.85) {
                                    continue;
                                }
                            }
                            else if (gedge0->getEndVertex() == gedge1->getEndVertex()) {
                                commonPt = gedge0->getEndVertex();
                                SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).high()).unit();
                                SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).high()).unit();
                                double value = pv0.x() * pv1.x() + pv0.y() * pv1.y() + pv0.z() * pv1.z();
                                if (value > -0.85) {
                                    continue;
                                }
                            }

                            int tag = 0;
                            MXGeoEdge* combineEdge0 = nullptr;
                            MXGeoEdge* combineEdge1 = nullptr;
                            if (gedge0->getCombineEdge()) {
                                combineEdge0 = gedge0->getCombineEdge();
                            }
                            if (gedge1->getCombineEdge()) {
                                combineEdge1 = gedge1->getCombineEdge();
                            }
                            MXGeoEdge* combineEdge = nullptr;
                            if (!combineEdge0 && !combineEdge0) {
                                combineEdge = new MXGeoEdge();
                                tag = ++modeldata->getMeshData()->getMeshEntity()->GeoEdgeTag;
                                combineEdge->setGeoTag(tag);
                                combineEdge->setEdgeType(2);
                                combineEdge->setPartName(gedge0->getPartName());
                                //MeshMessage::getInstance()->getMeshEntity()->AddGeoEdge(combineEdge);
                                if (outNewCombineEdges) {
                                    outNewCombineEdges->append(combineEdge);
                                }
                            }
                            else
                            {
                                if (combineEdge0)
                                {
                                    combineEdge = combineEdge0;
                                    if (combineEdge1)
                                    {
                                        for (auto& itp : combineEdge1->subGEdges)
                                        {
                                            if (!combineEdge->subGEdges.contains(itp))
                                            {
                                                combineEdge->subGEdges.append(itp);
                                            }
                                            itp->setCombineEdge(combineEdge);
                                        }
                                        /*delete combineEdge1;
                                        combineEdge1 = nullptr;*/
                                    }
                                }
                                else {
                                    combineEdge = combineEdge1;
                                }

                            }
                            gedge0->setCombineEdge(combineEdge);
                            gedge1->setCombineEdge(combineEdge);
                            if (!combineEdge->subGEdges.contains(gedge0)) {
                                combineEdge->subGEdges.append(gedge0);
                            }
                            if (!combineEdge->subGEdges.contains(gedge1)) {
                                combineEdge->subGEdges.append(gedge1);
                            }
                        }
                    }
                    continue;
                }
                if ((linkGeoFaces_0[0]->TOGGLE == 1 || linkGeoFaces_0[1]->TOGGLE == 1) &&
                    (linkGeoFaces_1[0]->TOGGLE == 1 || linkGeoFaces_1[1]->TOGGLE == 1)) {

                    int mintag0 = 0, maxtag0 = 0;
                    /*if (linkGeoFaces_0[0]->TOGGLE == 1 && linkGeoFaces_0[1]->TOGGLE == 1) {
                        mintag0 = std::min(linkGeoFaces_0[0]->toggleFace->getGeoTag(), linkGeoFaces_0[1]->toggleFace->getGeoTag());
                        maxtag0 = std::max(linkGeoFaces_0[0]->toggleFace->getGeoTag(), linkGeoFaces_0[1]->toggleFace->getGeoTag());
                    }*/

                    int faceTag0 = 0;
                    if (linkGeoFaces_0[0]->TOGGLE == 1) {
                        faceTag0 = linkGeoFaces_0[0]->toggleFace->getGeoTag();
                    }
                    else
                    {
                        faceTag0 = linkGeoFaces_0[0]->getGeoTag();
                    }
                    int faceTag1 = 0;
                    if (linkGeoFaces_0[1]->TOGGLE == 1) {
                        faceTag1 = linkGeoFaces_0[1]->toggleFace->getGeoTag();
                    }
                    else
                    {
                        faceTag1 = linkGeoFaces_0[1]->getGeoTag();
                    }
                    mintag0 = std::min(faceTag0, faceTag1);
                    maxtag0 = std::max(faceTag0, faceTag1);
                    QStringList linkfaceTag0 = { MxFunctions::mNumberToString(mintag0),MxFunctions::mNumberToString(maxtag0) };


                    int mintag1 = 1, maxtag1 = 1;
                    /*if (linkGeoFaces_1[0]->TOGGLE == 1 && linkGeoFaces_1[1]->TOGGLE == 1) {
                        mintag1 = std::min(linkGeoFaces_1[0]->toggleFace->getGeoTag(), linkGeoFaces_1[1]->toggleFace->getGeoTag());
                        maxtag1 = std::max(linkGeoFaces_1[0]->toggleFace->getGeoTag(), linkGeoFaces_1[1]->toggleFace->getGeoTag());
                    }*/
                    int faceTag2 = 0;
                    if (linkGeoFaces_1[0]->TOGGLE == 1) {
                        faceTag2 = linkGeoFaces_1[0]->toggleFace->getGeoTag();
                    }
                    else
                    {
                        faceTag2 = linkGeoFaces_1[0]->getGeoTag();
                    }
                    int faceTag3 = 0;
                    if (linkGeoFaces_1[1]->TOGGLE == 1) {
                        faceTag3 = linkGeoFaces_1[1]->toggleFace->getGeoTag();
                    }
                    else
                    {
                        faceTag3 = linkGeoFaces_1[1]->getGeoTag();
                    }
                    mintag1 = std::min(faceTag2, faceTag3);
                    maxtag1 = std::max(faceTag2, faceTag3);
                    QStringList linkfaceTag1 = { MxFunctions::mNumberToString(mintag1), MxFunctions::mNumberToString(maxtag1) };


                    if (linkfaceTag0 == linkfaceTag1) {

                        //判断两点是否连接取消的toggle边
                        if (!OK) {
                            //获取取消的toggle边的几何点 
                            QHash<int, MXGeoEdge*>tagToGEdge;
                            for (auto itm : modeldata->getMeshData()->getMeshEntity()->getGeoEdges()) {
                                tagToGEdge.insert(itm->getGeoTag(), itm);
                            }
                            set<MXGeoPoint*, MXEntityPtrLessThan>togglePoints;
                            for (auto itn : nowToggleID) {
                                MXGeoEdge* edge = tagToGEdge.value(itn);
                                togglePoints.insert(edge->getBeginVertex());
                                togglePoints.insert(edge->getEndVertex());
                            }

                            auto itp0 = togglePoints.find(gedge0->getBeginVertex());
                            if (itp0 != togglePoints.end()) {
                                if (gedge0->getCombineEdge()) {
                                    gedge0->getCombineEdge()->subGEdges.removeOne(gedge0);
                                    gedge0->setCombineEdge(nullptr);
                                }
                                if (gedge1->getCombineEdge()) {
                                    gedge1->getCombineEdge()->subGEdges.removeOne(gedge1);
                                    gedge1->setCombineEdge(nullptr);
                                }

                                continue;
                            }
                            auto itp1 = togglePoints.find(gedge0->getEndVertex());
                            if (itp1 != togglePoints.end()) {
                                if (gedge0->getCombineEdge()) {
                                    gedge0->getCombineEdge()->subGEdges.removeOne(gedge0);
                                    gedge0->setCombineEdge(nullptr);
                                }
                                if (gedge1->getCombineEdge()) {
                                    gedge1->getCombineEdge()->subGEdges.removeOne(gedge1);
                                    gedge1->setCombineEdge(nullptr);
                                }
                                continue;
                            }
                        }

                        //通过角度判断两边是否要合并
                        MXGeoPoint* commonPt = nullptr;
                        if (gedge0->getBeginVertex() == gedge1->getBeginVertex()) {
                            commonPt = gedge0->getBeginVertex();
                            SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).low()).unit();
                            SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).low()).unit();
                            double value = pv0.x() * pv1.x() + pv0.y() * pv1.y() + pv0.z() * pv1.z();
                            if (value > -0.85) {
                                continue;
                            }
                        }
                        else if (gedge0->getBeginVertex() == gedge1->getEndVertex())
                        {
                            commonPt = gedge0->getBeginVertex();
                            SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).low()).unit();
                            SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).high()).unit();
                            double value = pv0.x() * (-pv1.x()) + pv0.y() * (-pv1.y()) + pv0.z() * (-pv1.z());
                            if (value > -0.85) {
                                continue;
                            }

                        }
                        else if (gedge0->getEndVertex() == gedge1->getBeginVertex()) {
                            commonPt = gedge0->getEndVertex();
                            SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).high()).unit();
                            SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).low()).unit();
                            double value = pv0.x() * (-pv1.x()) + pv0.y() * (-pv1.y()) + pv0.z() * (-pv1.z());
                            if (value > -0.85) {
                                continue;
                            }
                        }
                        else if (gedge0->getEndVertex() == gedge1->getEndVertex()) {
                            commonPt = gedge0->getEndVertex();
                            SVector3 pv0 = gedge0->getFirstDer(gedge0->parBounds(0).high()).unit();
                            SVector3 pv1 = gedge1->getFirstDer(gedge1->parBounds(0).high()).unit();
                            double value = pv0.x() * pv1.x() + pv0.y() * pv1.y() + pv0.z() * pv1.z();
                            if (value > -0.85) {
                                continue;
                            }
                        }

                        if (gedge0->getCombineEdge() && gedge1->getCombineEdge() && gedge0->getCombineEdge()->getGeoTag() != gedge1->getCombineEdge()->getGeoTag())
                        {
                            for (auto& its : gedge1->getCombineEdge()->subGEdges)
                            {
                                its->setCombineEdge(gedge0->getCombineEdge());
                                gedge0->getCombineEdge()->subGEdges.append(its);
                            }
                        }

                        int tag = 0;
                        MXGeoEdge* combineEdge = nullptr;
                        if (gedge0->getCombineEdge()) {
                            combineEdge = gedge0->getCombineEdge();
                        }
                        else if (gedge1->getCombineEdge()) {
                            combineEdge = gedge1->getCombineEdge();
                        }

                        if (!combineEdge) {
                            combineEdge = new MXGeoEdge();
                            tag = ++modeldata->getMeshData()->getMeshEntity()->GeoEdgeTag;
                            combineEdge->setGeoTag(tag);
                            combineEdge->setEdgeType(2);
                            combineEdge->setPartName(gedge0->getPartName());
                            combineEdge->setBeginVertex(nullptr);
                            combineEdge->setEndVertex(nullptr);
                            //MeshMessage::getInstance()->getMeshEntity()->AddGeoEdge(combineEdge);
                            if (outNewCombineEdges) {
                                outNewCombineEdges->append(combineEdge);
                            }
                        }
                        gedge0->setCombineEdge(combineEdge);
                        gedge1->setCombineEdge(combineEdge);
                        if (!combineEdge->subGEdges.contains(gedge0)) {
                            combineEdge->subGEdges.append(gedge0);
                        }
                        if (!combineEdge->subGEdges.contains(gedge1)) {
                            combineEdge->subGEdges.append(gedge1);
                        }
                    }
                }
            }
            //更新合并几何边的子边
            set<MXGeoEdge*, MXEntityPtrLessThan>combineEdges;
            for (auto ite : gface->getEdgesOfGeoFaces()) {
                if (ite->getBeginVertex()->getGeoTag() == 1092 || ite->getEndVertex()->getGeoTag() == 1092)
                {
                    int tt = 0;
                }
                if (ite->getGeoTag() == 5395) {
                    int t = 0;
                }
                if (ite->getCombineEdge()) {
                    combineEdges.insert(ite);
                    for (auto its = ite->getCombineEdge()->subGEdges.begin(); its != ite->getCombineEdge()->subGEdges.end();) {
                        if ((*its)->getBeginVertex()->getGeoTag() == 522 && (*its)->getEndVertex()->getGeoTag() == 523)
                        {
                            int tt = 0;
                        }
                        if (!gface->getEdgesOfGeoFaces().contains(*its)) {

                            if ((*its)->getCombineEdge() == ite->getCombineEdge())
                            {
                                (*its)->setCombineEdge(nullptr);
                            }
                            its = ite->getCombineEdge()->subGEdges.erase(its);
                            continue;
                        }
                        its++;
                    }
                }
            }
        }
    }

    void mMeshThread::slot_createMeshEdgeSuppress(int partid, QStringList list, bool isSuppress)
    {
        // 获取当前剖分列表的几何ID
        std::set<QString> setB(list.begin(), list.end());

        // 取交集获取重复剖分的几何实体名称
        std::set<QString> intersect;
        std::set_intersection(setB.begin(), setB.end(),
            setB.begin(), setB.end(),
            std::inserter(intersect, intersect.begin()));
        // 获取重复剖分的几何ID
        std::set<int> geoIds;
        for (const QString& s : intersect)
        {
            // 从最后一个 '_' 往后就是数字
            int id = s.mid(s.lastIndexOf('_') + 1).toInt();
            geoIds.insert(id);
        }
        set<MXGeoEdge*>subGeoEdges;
        for (auto ite : meshdata->getMeshEntity()->getGeoEdges()) {
            if (ite->getCombineEdge())
            {
                subGeoEdges.insert(ite);
            }
        }
        
        //判断选择边是否为合并边
        set<int>tempIds;
        QVector<MXGeoEdge*>deleteCombineEdges;
        QVector<MXGeoFace*>deleteToggleFaces;
        if (isSuppress) {
            //设置toggle边
            for (auto it : geoIds) {
                for (auto ite : meshdata->getMeshEntity()->getGeoEdges()) {                
                    if (ite->getGeoTag() == it) {
                        if (ite->getEdgeType() == 1)
                        {
                            continue;
                        }
                        //判断抑制边连接的几何面个数
                        if (ite->getLinkFaces().size() == 1)
                        {
                            QString edgePartName = tr("未知部件");
                            if (ite && !QString::fromUtf8(ite->getPartName().c_str()).isEmpty())
                            {
                                edgePartName = QString::fromUtf8(ite->getPartName().c_str());
                            }
                            else
                            {
                                auto* suppressGeoDatabase = getCVM->getModelData()->getGeoData();
                                if (suppressGeoDatabase)
                                {
                                    { //【OCC摘除·S9】旧栈遍历退役，改 GN 部件名（按边 ID 反查）
		            edgePartName = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, (ite ? ite->getGeoTag() : -1));
	            }
                                }
                            }
                            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("%1为边界，抑制操作不合理").arg(QString("%1-几何边-%2").arg(edgePartName).arg(it)), true);
                        }

                        if (ite->getCombineEdge()) {
                            MXGeoEdge* combineEdge = ite->getCombineEdge();
                            for (auto& its : combineEdge->subGEdges) {
                                if (!deleteCombineEdges.contains(ite->getCombineEdge())) {
                                    deleteCombineEdges.append(ite->getCombineEdge());
                                }
                                tempIds.insert(its->getGeoTag());
                                its->setCombineEdge(nullptr);
                            }

                        }
                        else
                        {
                            tempIds.insert(it);
                        }
                    }
                }
            }
            geoIds.clear();
            geoIds = tempIds;
        }


        setSutureID(geoIds);
        if (isSuppress) {
            meshdata->setToggleID(geoIds);
            for (auto& it : meshdata->getMeshEntity()->getGeoEdges()) {
                auto its = geoIds.find(it->getGeoTag());
                if (its != geoIds.end()) {
                    it->setEdgeType(1);
                }
            }
        }
        else {
            meshdata->removeToggleID(geoIds);
            for (auto& it : meshdata->getMeshEntity()->getGeoEdges()) {
                auto its = geoIds.find(it->getGeoTag());
                if (its != geoIds.end()) {
                    it->_mLines.clear();
                    it->meshGeo_vertexs.clear();
                    it->setMeshStatic(NoMesh);
                    it->setEdgeType(0);
                    it->getBeginVertex()->setMeshStatic(Meshing);
                    it->getEndVertex()->setMeshStatic(Meshing);
                    it->getBeginVertex()->addLinkGeoEdgess(it);
                    it->getEndVertex()->addLinkGeoEdgess(it);
                }
            }
        }
        for (auto& it : meshdata->getMeshEntity()->getGeoEdges())
        {
            if (it->getGeoTag() == 5395) {
                int t = 0;
            }
        }
        QVector<QHash<int, MXGeoFace*>>lfaces;
        QVector<MXGeoFace*> newToggleFaces;
        QVector<MXGeoEdge*> newCombineEdges;
        try
        {
            buildToggleMessage(isSuppress, geoIds, meshdata->getToggleID(), lfaces, modelData, &newToggleFaces, &newCombineEdges);
        }
        catch (...)
        {

        }
        //判断构建的虚拓扑面是否是自封闭的周期性面
        set<MXGeoEdge*> selfClosedEdges;
        bool ALLTOGGLE = false;
        for (auto ita : lfaces)
        {
            //获取未抑制的边界几何边
            set<MXGeoEdge*> boundaryEdges;
            for (auto itf : ita)
            {
                for (auto ite : itf->getEdgesOfGeoFaces())
                {
                    //跳过已抑制的边
                    if (ite->getEdgeType() == 1)
                    {
                        continue;
                    }                  
                    boundaryEdges.insert(ite);
                }
            }
            if (!boundaryEdges.size())
            {
                ALLTOGGLE = true;
            }

            //判断未抑制的边界几何边中是否有造成虚拓扑面自封闭的几何边
            for (auto ite : boundaryEdges)
            {
                QVector<MXGeoFace*> linkFaces = ite->getLinkFaces();
                bool allInVirtualFace = true;
                for (auto itf : linkFaces)
                {
                    if (!ita.contains(itf->getGeoTag()))
                    {
                        allInVirtualFace = false;
                        break;
                    }
                }
                if (allInVirtualFace && linkFaces.size() >= 2)
                {
                    selfClosedEdges.insert(ite);
                }
            }
        }
        //若检测到自封闭边，输出提示信息并回滚本次抑制操作
        if (selfClosedEdges.size()|| ALLTOGGLE)
        {
            if (ALLTOGGLE)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    QString("抑制操作不合理"), true);
            }
            else
            {
                QString edgeIds;
                for (auto ite : selfClosedEdges)
                {
                    if (!edgeIds.isEmpty())
                    {
                        edgeIds += ", ";
                    }
                    edgeIds += MxFunctions::mNumberToString(ite->getGeoTag());
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    QString("几何边 %1 造成虚拓扑面自封闭，抑制操作不合理").arg(edgeIds), true);
            }
            //回滚：恢复geoIds中的边为未抑制状态
            for (auto& it : meshdata->getMeshEntity()->getGeoEdges())
            {
                auto its = geoIds.find(it->getGeoTag());
                if (its != geoIds.end())
                {
                    it->setEdgeType(0);
                }
            }
            meshdata->removeToggleID(geoIds);

            //回滚：撤销虚拓扑面构建
            for (auto ita : lfaces)
            {
                for (auto itf : ita)
                {
                    itf->TOGGLE = 0;
                    itf->toggleFace = nullptr;
                }
            }
            for (auto itf : newToggleFaces)
            {
                meshdata->getMeshEntity()->RemoveGeoFace(itf);
                delete itf;
            }
            //回滚：清理合并边
            for (auto ite : newCombineEdges)
            {
                for (auto its : ite->subGEdges)
                {
                    its->setCombineEdge(nullptr);
                }
                ite->subGEdges.clear();
                delete ite;
            }
            return;
        }

        for (auto& it : meshdata->getMeshEntity()->getGeoEdges())
        {
            if (it->getGeoTag() == 5395) {
                int t = 0;
            }
        }

        //若缝合边连接的拓扑面已经确定剖分
        QVector<set<MXGeoFace*>>noMeshGFaces;
        QVector<set<MXGeoFace*>>meshGFaces;
        try
        {
            meshdata->GetTogEdgeLinkGFaces(geoIds, noMeshGFaces, meshGFaces);
        }
        catch (...)
        {

        }

        for (auto ita : meshdata->getMeshEntity()->getGeoSolids())
        {
            //判断已剖分的几何面是否在实体上
            QVector<MXGeoFace*> solidFaces = ita->getface();
            bool isSolidFace = false;
            for (auto& itg : meshGFaces)
            {
                for (auto itf : itg)
                {
                    if (solidFaces.contains(itf))
                    {
                        isSolidFace = true;
                        break;
                    }
                }
                if (isSolidFace)
                {
                    break;
                }
            }
            if (isSolidFace)
            {
                std::map<int, int>typeOfEdge;
                std::set<int>pickPardID;
                for (auto ita : geoIds)
                {
                    pickPardID.insert(MGeoNeutralQuery::partIdByGeoId((modelData ? modelData->getGeoManager() : nullptr), 1, ita));
                    if (isSuppress)
                    {
                        typeOfEdge.insert(make_pair(ita, 2));
                    }
                    else
                    {
                        typeOfEdge.insert(make_pair(ita, 1));
                    }

                }
                //【空窗收口·G3】边类型改写 GN 离散表（老接口空壳）
                setEdgeTopoTypeToNeutralData(
                    (modelData ? modelData->getGeoManager()->getGeoPartMgr() : nullptr), pickPardID, typeOfEdge);
                getCVM->updateAllModelOperate(ReplaceOnePart, pickPardID);
                getCVM->clearPick();
                return;
            }
        }


        if (isSuppress) {
            MXMesh* mesh = new MXMesh();

            {
                //判断抑制边节点连接的网格边的共享面是否已剖分
                set<MXGeoPoint*>endPts;
                for (auto itd : geoIds)
                {
                    for (auto& it : meshdata->getMeshEntity()->getGeoEdges()) {
                        auto its = geoIds.find(it->getGeoTag());
                        if (its != geoIds.end())
                        {
                            endPts.insert(it->getBeginVertex());
                            endPts.insert(it->getEndVertex());
                        }
                    }
                }
                set<MXGeoEdge*>cedges;
                for (auto itd : endPts)
                {
                    for (auto ite : itd->getLinkGeoEdges())
                    {
                        if (geoIds.find(ite->getGeoTag()) != geoIds.end())
                        {
                            continue;
                        }
                        if (ite->getCombineEdge())
                        {

                            //合并边需包含抑制点连接的几何边
                            bool HAVE = true;
                            for (auto itk : itd->getLinkGeoEdges())
                            {
                                if (itk->getEdgeType() == 1)
                                {
                                    continue;
                                }
                                if (geoIds.find(itk->getGeoTag()) != geoIds.end())
                                {
                                    continue;
                                }
                                if (!ite->getCombineEdge()->subGEdges.contains(itk))
                                {
                                    HAVE = false;
                                    break;
                                }
                            }
                            if (!HAVE)
                            {
                                continue;
                            }
                            cedges.insert(ite);                       
                            if (ite->getMeshStatic() == MeshDone)
                            {
                                mesh->getMeshEntity()->AddGeoEdge(ite);
                                ite->setMeshStatic(Meshing);
                            }
                            ite->getCombineEdge()->_mLines.clear();
                            ite->getCombineEdge()->_mLines_temp.clear();
                            ite->getCombineEdge()->meshGeo_vertexs.clear();
                        }

                    }
                }
                for (auto& itd : cedges) {
                    for (auto& ite : itd->getLinkFaces()) {

                        QHash<int, MXGeoFace*>InFaces;
                        for (auto itf : lfaces)
                        {
                            if (itf.contains(ite->getGeoTag()))
                            {
                                InFaces = itf;
                            }
                        }
                        if (!InFaces.size())
                        {
                            InFaces.insert(ite->getGeoTag(), ite);
                        }

                        for (auto itg = InFaces.begin(); itg != InFaces.end(); itg++)
                        {
                            if (itg.value()->getMeshStatic() == NoMesh)
                            {
                                continue;
                            }
                            itg.value()->setMeshStatic(NoMesh);
                            set<int> facetag;
                            QHash<int, vector<int>> faceToedge;
                            MXGeoFace* currentGface = itg.value();
                            mesh->getMeshEntity()->AddGeoFace(itg.value());
                            if (itg.value()->toggleFace)
                            {
                                currentGface = itg.value()->toggleFace;
                            }
                            currentGface->_mTriangles.clear();
                            currentGface->meshGeo_vertexs.clear();

                            facetag.insert(currentGface->getGeoTag());
                            vector<int>edgetags;
                            for (auto itk : currentGface->getEdgesOfGeoFaces())
                            {
                                edgetags.push_back(itk->getGeoTag());
                            }
                            faceToedge.insert(currentGface->getGeoTag(), edgetags);
                            edgetags.clear();
                            facetag.insert(currentGface->getGeoTag());
                            for (auto itk : currentGface->getEdgesOfGeoFaces())
                            {
                                edgetags.push_back(itk->getGeoTag());
                            }
                            faceToedge.insert(currentGface->getGeoTag(), edgetags);

                            meshdata->deleteElementByGeoFace(facetag, faceToedge);
                        }
                    }
                }
            }

            set<QString>partNames;
            for (int i = 0; i < meshGFaces.size(); i++) {

                if (meshGFaces.at(i).size()) {
                    //MXMesh* mesh = new MXMesh();
                    set<MXGeoEdge*, MXEntityPtrLessThan>combineGEdges;
                    MeshEntity* _meshEntity = new MeshEntity();
                    MXGeoFace* GFace = nullptr;
                    for (auto it : meshGFaces.at(i)) {
                        for (auto ite : it->getEdgesOfGeoFaces()) {
                            if (ite->getCombineEdge()) {
                                combineGEdges.insert(ite->getCombineEdge());
                            }
                        }
                        it->_mTriangles.clear();
                        it->meshGeo_vertexs.clear();
                        it->setMeshStatic(NoMesh);
                        mesh->getMeshEntity()->AddGeoFace(it);
                        GFace = it->toggleFace;
                        partNames.insert(QString::fromStdString(it->getPartName()));

                    }
                    //if (0)
                    {
                        if (noMeshGFaces.size())
                        {
                            for (int j = 0; j < noMeshGFaces.size(); j++)
                            {
                                for (auto it : noMeshGFaces.at(j)) {
                                    //获取未剖分几何边
                                    for (auto ite : it->getEdgesOfGeoFaces()) {
                                        if (ite->getCombineEdge()) {
                                            combineGEdges.insert(ite->getCombineEdge());
                                        }
                                    }
                                    mesh->getMeshEntity()->AddGeoFace(it);
                                    it->toggleFace = GFace;
                                    partNames.insert(QString::fromStdString(it->getPartName()));
                                }
                            }
                        }


                        for (auto itc : combineGEdges) {
                            for (auto& itd : itc->subGEdges) {


                                auto ite = geoIds.find(itd->getGeoTag());
                                if (ite == geoIds.end()) {
                                    continue;
                                }

                                for (auto& ite : itd->getLinkFaces()) {
                                    auto itk = noMeshGFaces.at(i).find(ite);
                                    if (itk != noMeshGFaces.at(i).end()) {
                                        continue;
                                    }
                                    itk = meshGFaces.at(i).find(ite);
                                    if (itk != meshGFaces.at(i).end()) {
                                        continue;
                                    }

                                    for (auto itm : lfaces) {
                                        if (itm.contains(ite->getGeoTag())) {
                                            for (auto itn1 = itm.begin(); itn1 != itm.end(); itn1++) {
                                                for (auto itn2 : itn1.value()->getEdgesOfGeoFaces()) {
                                                    if (itn2->getMeshStatic() == MeshDone && !itn2->_mLines.size()) {
                                                        itn2->meshGeo_vertexs.clear();
                                                        itn2->_mLines.clear();
                                                        itn2->medges.clear();
                                                        itn2->setMeshStatic(NoMesh);
                                                        //mesh->getMeshEntity()->AddGeoEdge(itn2);

                                                    }
                                                }

                                                itn1.value()->meshGeo_vertexs.clear();
                                                itn1.value()->_mTriangles.clear();
                                                itn1.value()->setMeshStatic(NoMesh);
                                                mesh->getMeshEntity()->AddGeoFace(itn1.value());
                                            }
                                        }
                                    }



                                }
                            }
                        }
                    }

                    set<int>toggleID = getSutereID();

                    if (mesh->getMeshEntity()->getGeoFaces().size()) {
                        //如果合并边连接的两个面都是待剖分的面,删除边上网格
                        if (combineGEdges.size()) {
                            for (auto& itc : combineGEdges) {
                                bool IS = true;
                                for (auto& itd : itc->subGEdges) {
                                    for (auto& itf : itd->getLinkFaces()) {
                                        auto itg = mesh->getMeshEntity()->getGeoFaces().find(itf);
                                        if (itg == mesh->getMeshEntity()->getGeoFaces().end()) {
                                            IS = false;
                                        }
                                    }
                                }
                                if (IS) {
                                    itc->_mLines.clear();
                                    itc->meshGeo_vertexs.clear();
                                    itc->medges.clear();
                                    itc->setMeshStatic(NoMesh);

                                    for (auto& itd : itc->subGEdges) {
                                        itd->_mLines.clear();
                                        itd->meshGeo_vertexs.clear();
                                        itd->medges.clear();
                                    }
                                }
                            }
                        }

                        for (auto& itc : mesh->getMeshEntity()->getGeoEdges())
                        {
                            itc->_mLines.clear();
                            itc->meshGeo_vertexs.clear();
                            itc->medges.clear();
                            itc->setMeshStatic(NoMesh);
                        }



                        //删除虚拓扑面数据
                        set<int> facetag;
                        QHash<int, vector<int>> faceToedge;
                        for (auto itk : mesh->getMeshEntity()->getGeoFaces())
                        {
                            if (itk->toggleFace)
                            {
                                facetag.insert(itk->toggleFace->getGeoTag());
                                vector<int>edgetags;
                                for (auto ite : itk->toggleFace->getEdgesOfGeoFaces())
                                {
                                    edgetags.push_back(ite->getGeoTag());
                                }
                                faceToedge.insert(itk->toggleFace->getGeoTag(), edgetags);
                                edgetags.clear();
                                facetag.insert(itk->getGeoTag());
                                for (auto ite : itk->getEdgesOfGeoFaces())
                                {
                                    edgetags.push_back(ite->getGeoTag());
                                }
                                faceToedge.insert(itk->getGeoTag(), edgetags);
                            }
                        }

                        meshdata->deleteElementByGeoFace(facetag, faceToedge);
                        //删除虚拓扑边数据
                        set<int> geoEdgetag;
                        for (auto itk : mesh->getMeshEntity()->getGeoEdges())
                        {
                            geoEdgetag.insert(itk->getGeoTag());
                        }                      
                    }

                }

            }
            if (mesh->getMeshEntity()->getGeoFaces().size())
            {
                //添加剖分几何边
                for (auto itf : mesh->getMeshEntity()->getGeoFaces()) {
                    for (auto ite : itf->getEdgesOfGeoFaces()) {
                        if (!ite->_mLines.size()) {
                            ite->setMeshStatic(NoMesh);
                            mesh->getMeshEntity()->AddGeoEdge(ite);
                        }
                        else
                        {
                            //ite->setMeshStatic(Meshing);
                            mesh->getMeshEntity()->AddGeoEdge(ite);
                        }
                    }
                    if (itf->toggleFace)
                    {
                        for (auto ite : itf->toggleFace->getEdgesOfGeoFaces()) {
                            if (!ite->_mLines.size()) {
                                ite->setMeshStatic(NoMesh);
                                mesh->getMeshEntity()->AddGeoEdge(ite);
                            }
                            else
                            {
                                //ite->setMeshStatic(Meshing);
                                mesh->getMeshEntity()->AddGeoEdge(ite);
                            }
                        }
                    }

                  
                }
                if (meshdata)
                {
                    if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
                    {
                        MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
                    }
                    MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
                }
                else
                {
                    MXMeshParameters::getInstance()->_nodeNum = 0;
                    MXMeshParameters::getInstance()->_elementNum = 0;
                }
                mesh->MXMeshGenerateMesh(2, meshdata->getToggleID());
                try
                {
                    meshdata->ensureMesh();
                }
                catch (...) {

                }
            }

            if (0)
            {
                for (int i = 0; i < noMeshGFaces.size(); i++) {
                    if (noMeshGFaces.size()) {
                        //判断网格边是否包含合并边
                        set<MXGeoEdge*, MXEntityPtrLessThan>combineEdge;
                        if (noMeshGFaces.at(i).size()) {
                            for (auto it : noMeshGFaces.at(i)) {
                                for (auto ite : it->getEdgesOfGeoFaces()) {
                                    if (ite->getCombineEdge()) {
                                        combineEdge.insert(ite->getCombineEdge());
                                        ite->getCombineEdge()->meshGeo_vertexs.clear();
                                        ite->getCombineEdge()->_mLines.clear();
                                        ite->getCombineEdge()->medges.clear();
                                    }
                                }
                            }
                        }

                        if (combineEdge.size()) {
                            //判断合并边的子边连接的几何面是否已经完成剖分
                            QVector<QHash<int, MXGeoFace*>>nfaces;
                            for (auto itc : combineEdge) {
                                for (auto itd : itc->subGEdges) {
                                    for (auto itf : itd->getLinkFaces()) {
                                        //if (itf->getMeshStatic() == MeshDone) {
                                        MXGeoFace* toggleGFace = itf->toggleFace;
                                        for (auto& itg : lfaces) {
                                            for (auto itk = itg.begin(); itk != itg.end(); itk++) {
                                                if (itk.value()->toggleFace == toggleGFace) {
                                                    nfaces.append(itg);
                                                    break;
                                                }
                                            }
                                        }
                                        //}
                                    }
                                    break;
                                }
                            }
                            MXMesh* mesh = new MXMesh();
                            for (auto it : nfaces) {
                                for (auto itf = it.begin(); itf != it.end(); itf++) {
                                    for (auto itg : itf.value()->getEdgesOfGeoFaces()) {
                                        /*if (itf.value()->getMeshStatic() != MeshDone) {
                                            itg->meshGeo_vertexs.clear();
                                            itg->_mLines.clear();
                                            itg->setMeshStatic(NoMesh);
                                            mesh->getMeshEntity()->AddGeoEdge(itg);
                                        }*/
                                        if (itg->getCombineEdge()) {
                                            auto its = combineEdge.find(itg->getCombineEdge());
                                            if (its != combineEdge.end()) {
                                                itg->meshGeo_vertexs.clear();
                                                itg->_mLines.clear();
                                                itg->setMeshStatic(NoMesh);
                                                itg->medges.clear();
                                                //mesh->getMeshEntity()->AddGeoEdge(itg);
                                            }
                                        }
                                        else {
                                            if (itg->getMeshStatic() == MeshDone && !itg->_mLines.size()) {
                                                itg->setMeshStatic(NoMesh);
                                                itg->medges.clear();
                                                itg->meshGeo_vertexs.clear();
                                                //mesh->getMeshEntity()->AddGeoEdge(itg);
                                            }
                                        }


                                    }
                                    if (itf.value()->getMeshStatic() == MeshDone) {
                                        itf.value()->meshGeo_vertexs.clear();
                                        itf.value()->_mTriangles.clear();
                                        itf.value()->setMeshStatic(NoMesh);
                                        mesh->getMeshEntity()->AddGeoFace(itf.value());
                                    }

                                }
                            }
                            if (mesh->getMeshEntity()->getGeoFaces().size()) {
                                //如果合并边连接的两个面都是待剖分的面,删除边上网格
                                if (combineEdge.size()) {
                                    for (auto& itc : combineEdge) {
                                        bool IS = true;
                                        for (auto& itd : itc->subGEdges) {
                                            for (auto& itf : itd->getLinkFaces()) {
                                                auto itg = mesh->getMeshEntity()->getGeoFaces().find(itf);
                                                if (itg == mesh->getMeshEntity()->getGeoFaces().end()) {
                                                    IS = false;
                                                }
                                            }
                                        }
                                        if (IS) {
                                            itc->_mLines.clear();
                                            itc->meshGeo_vertexs.clear();
                                            itc->medges.clear();
                                            itc->setMeshStatic(NoMesh);

                                            for (auto& itd : itc->subGEdges) {
                                                itd->_mLines.clear();
                                                itd->meshGeo_vertexs.clear();
                                                itd->medges.clear();
                                            }
                                        }
                                    }
                                }

                                //添加剖分几何边
                                for (auto itf : mesh->getMeshEntity()->getGeoFaces()) {
                                    for (auto ite : itf->getEdgesOfGeoFaces()) {
                                        if (!ite->_mLines.size()) {
                                            ite->setMeshStatic(NoMesh);
                                            mesh->getMeshEntity()->AddGeoEdge(ite);
                                        }
                                    }
                                }
                                try
                                {
                                    //删除虚拓扑面数据
                                    set<int> facetag;
                                    QHash<int, vector<int>> faceToedge;
                                    for (auto itk : mesh->getMeshEntity()->getGeoFaces())
                                    {
                                        if (itk->toggleFace)
                                        {
                                            facetag.insert(itk->toggleFace->getGeoTag());
                                            vector<int>edgetags;
                                            for (auto ite : itk->toggleFace->getEdgesOfGeoFaces())
                                            {
                                                edgetags.push_back(ite->getGeoTag());
                                            }
                                            faceToedge.insert(itk->toggleFace->getGeoTag(), edgetags);
                                        }
                                    }

                                    meshdata->deleteElementByGeoFace(facetag, faceToedge);
                                    //删除虚拓扑边数据
                                    set<int> geoEdgetag;
                                    for (auto itk : mesh->getMeshEntity()->getGeoEdges())
                                    {
                                        geoEdgetag.insert(itk->getGeoTag());
                                    }
                                    mesh->MXMeshGenerateMesh(2, meshdata->getToggleID());
                                }
                                catch (...)
                                {

                                }

                                meshdata->ensureMesh();
                            }
                        }
                    }
                }
            }
            //MeshMessage::getInstance()->updateRenderState(ReplaceOnePart, partNames);
        }
        else
        {
            MXMesh* mesh = new MXMesh();
            set<MXGeoEdge*, MXEntityPtrLessThan>combineGEdges;
            for (auto it : meshGFaces) {
                if (it.size()) {

                    for (auto itf : it)
                    {
                        itf->setMeshStatic(NoMesh);
                        mesh->getMeshEntity()->AddGeoFace(itf);
                        for (auto ite : itf->getEdgesOfGeoFaces())
                        {
                            if (ite->getCombineEdge())
                            {
                                combineGEdges.insert(ite->getCombineEdge());
                            }
                            if (ite->getMeshStatic() != MeshDone)
                            {
                                mesh->getMeshEntity()->AddGeoEdge(ite);
                            }
                        }
                    }

                }
            }

            ////判断抑制边节点连接的网格边的共享面是否已剖分
            set<MXGeoPoint*>endPts;
            for (auto itd : geoIds)
            {
                for (auto& it : meshdata->getMeshEntity()->getGeoEdges()) {

                    auto its = geoIds.find(it->getGeoTag());
                    if (its != geoIds.end())
                    {
                        endPts.insert(it->getBeginVertex());
                        endPts.insert(it->getEndVertex());
                    }
                }
            }
            set<MXGeoEdge*>cedges;
            for (auto itd : endPts)
            {
                for (auto ite : itd->getLinkGeoEdges())
                {
                    if (ite->getEdgeType() == 1)
                    {
                        continue;
                    }
                    if (geoIds.find(ite->getGeoTag()) != geoIds.end())
                    {
                        continue;
                    }
                    if (subGeoEdges.find(ite)!= subGeoEdges.end())
                    {
                        cedges.insert(ite);
                        if (ite->getCombineEdge())
                        {
                            ite->getCombineEdge()->_mLines.clear();
                            ite->getCombineEdge()->_mLines_temp.clear();
                            ite->getCombineEdge()->meshGeo_vertexs.clear();
                        }
                    }
                }
            }
            for (auto& itd : cedges) {
                for (auto& ite : itd->getLinkFaces()) {

                    QHash<int, MXGeoFace*>InFaces;
                    for (auto itf : lfaces)
                    {
                        if (itf.contains(ite->getGeoTag()))
                        {
                            InFaces = itf;
                        }
                    }
                    if (!InFaces.size())
                    {
                        if (ite->getMeshStatic() != NoMesh)
                        {
                            InFaces.insert(ite->getGeoTag(), ite);
                        }
                    }
                    if (itd->getCombineEdge())
                    {
                        for (auto itg : itd->getCombineEdge()->subGEdges)
                        {
                            itg->_mLines.clear();
                            itg->_mLines_temp.clear();
                            itg->meshGeo_vertexs.clear();
                            itg->setMeshStatic(NoMesh);
                            mesh->getMeshEntity()->AddGeoEdge(itg);
                        }
                    }
                    else
                    {
                        itd->_mLines.clear();
                        itd->_mLines_temp.clear();
                        itd->meshGeo_vertexs.clear();
                        itd->setMeshStatic(NoMesh);
                        mesh->getMeshEntity()->AddGeoEdge(itd);
                    }


                    for (auto itg = InFaces.begin(); itg != InFaces.end(); itg++)
                    {
                        if(itg.value()->getMeshStatic() == NoMesh)
                        {
                            continue;
                        }
                        itg.value()->setMeshStatic(NoMesh);
                        set<int> facetag;
                        QHash<int, vector<int>> faceToedge;
                        MXGeoFace* currentGface = itg.value();
                        mesh->getMeshEntity()->AddGeoFace(itg.value());
                        if (itg.value()->toggleFace)
                        {
                            currentGface = itg.value()->toggleFace;
                        }
                        currentGface->_mTriangles.clear();
                        currentGface->meshGeo_vertexs.clear();

                        facetag.insert(currentGface->getGeoTag());
                        vector<int>edgetags;
                        for (auto itk : currentGface->getEdgesOfGeoFaces())
                        {
                            edgetags.push_back(itk->getGeoTag());
                        }
                        faceToedge.insert(currentGface->getGeoTag(), edgetags);
                        edgetags.clear();
                        facetag.insert(currentGface->getGeoTag());
                        for (auto itk : currentGface->getEdgesOfGeoFaces())
                        {
                            edgetags.push_back(itk->getGeoTag());
                        }
                        faceToedge.insert(currentGface->getGeoTag(), edgetags);

                        meshdata->deleteElementByGeoFace(facetag, faceToedge);
                    }
                }
            }

          
            if (mesh->getMeshEntity()->getGeoFaces().size()) {

                try
                {
                    //删除虚拓扑面数据
                    set<int> facetag;
                    QHash<int, vector<int>> faceToedge;
                    for (auto itk : mesh->getMeshEntity()->getGeoFaces())
                    {
                        if (itk->toggleFace)
                        {
                            facetag.insert(itk->toggleFace->getGeoTag());
                            vector<int>edgetags;
                            for (auto ite : itk->toggleFace->getEdgesOfGeoFaces())
                            {
                                edgetags.push_back(ite->getGeoTag());
                            }
                            faceToedge.insert(itk->toggleFace->getGeoTag(), edgetags);
                        }
                    }

                    meshdata->deleteElementByGeoFace(facetag, faceToedge);
                    //删除虚拓扑边数据
                    set<int> geoEdgetag;
                    for (auto itk : mesh->getMeshEntity()->getGeoEdges())
                    {
                        geoEdgetag.insert(itk->getGeoTag());
                    }
                    meshdata->deleteElementByGeoEdge(geoEdgetag);

                    if (meshdata)
                    {
                        if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
                        {
                            MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
                        }
                        MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
                    }
                    else
                    {
                        MXMeshParameters::getInstance()->_nodeNum = 0;
                        MXMeshParameters::getInstance()->_elementNum = 0;
                    }

                    mesh->MXMeshGenerateMesh(2, meshdata->getToggleID());
                }
                catch (...)
                {

                }

                meshdata->ensureMesh();
            }

        }

        //删除数据内存
        for (int i = 0; i < deleteToggleFaces.size(); i++) {
            if (deleteToggleFaces[i]->_mTriangles.size()) {
                for (int m = 0; m < deleteToggleFaces[i]->_mTriangles.size(); m++) {
                    delete deleteToggleFaces[i]->_mTriangles[m];
                    deleteToggleFaces[i]->_mTriangles[m] = nullptr;
                }
                deleteToggleFaces[i]->_mTriangles.clear();
                deleteToggleFaces[i]->_mTriangles.shrink_to_fit();
                for (int m = 0; m < deleteToggleFaces[i]->meshGeo_vertexs.size(); m++) {
                    delete deleteToggleFaces[i]->meshGeo_vertexs[m];
                    deleteToggleFaces[i]->meshGeo_vertexs[m] = nullptr;
                }
                deleteToggleFaces[i]->meshGeo_vertexs.clear();
                deleteToggleFaces[i]->meshGeo_vertexs.shrink_to_fit();

                for (int m = 0; m < deleteToggleFaces[i]->MeshEdgesInGface.size(); m++) {
                    meshdata->getMeshEntity()->IDToInteriorEdge.remove(deleteToggleFaces[i]->MeshEdgesInGface[m]->getEdgeId());
                    delete deleteToggleFaces[i]->MeshEdgesInGface[m];
                    deleteToggleFaces[i]->MeshEdgesInGface[m] = nullptr;
                }
                deleteToggleFaces[i]->MeshEdgesInGface.clear();
                deleteToggleFaces[i]->MeshEdgesInGface.shrink_to_fit();


                deleteToggleFaces[i]->boundaryMeshEdgesInGface.clear();
                deleteToggleFaces[i]->boundaryMeshEdgesInGface.shrink_to_fit();
            }
        }
        //emit meshSutureRequested(isSuppress, geoIds);
        std::map<int, int>typeOfEdge;
        std::set<int>pickPardID;
        for (auto ita : geoIds)
        {
            pickPardID.insert(MGeoNeutralQuery::partIdByGeoId((modelData ? modelData->getGeoManager() : nullptr), 1, ita));
            if (isSuppress)
            {
                typeOfEdge.insert(make_pair(ita, 2));
            }
            else
            {
                typeOfEdge.insert(make_pair(ita, 1));
            }

        }
        //【空窗收口·G3】边类型改写 GN 离散表（老接口空壳）
        setEdgeTopoTypeToNeutralData(
            (modelData ? modelData->getGeoManager()->getGeoPartMgr() : nullptr), pickPardID, typeOfEdge);
        getCVM->updateAllModelOperate(ReplaceOnePart, pickPardID);
        getCVM->clearPick();

    }
    void mMeshThread::slot_createMeshLocalSize(int id, QStringList list,std::map<int,double> ids2SizesMap)
    {
        // 将面板中的中文网格类型转化为MeshShapeType



        // 获取modeldata和meshdata
        MechModelData::mModelData* modelData = getCVM->getModelData();
        mMeshData* meshdata = modelData->getMeshData();

        // 获取之前一键剖分完成的几何ID
        std::set<QString> setA(_finishMeshList.begin(), _finishMeshList.end());
        // 获取当前剖分列表的几何ID
        std::set<QString> setB(list.begin(), list.end());

        // 取交集获取重复剖分的几何实体名称
        std::set<QString> intersect;
        std::set_intersection(setA.begin(), setA.end(),
            setB.begin(), setB.end(),
            std::inserter(intersect, intersect.begin()));

        // 获取重复剖分的几何ID
        std::set<int> geoIds;
        for (const QString& s : intersect)
        {
            // 从最后一个 '_' 往后就是数字
            int id = s.mid(s.lastIndexOf('_') + 1).toInt();
            geoIds.insert(id);
        }

        std::vector<int> solidids;
        for (const QString& s : list)
        {
            solidids.push_back(s.section('_', 1, 1).toInt());
        }

        // 根据几何实体名称获取网格维度
        int dim = 1;
        // 删除重复剖分几何实体中原来的网格
        meshdata->deleteElementByGeoEdge(geoIds);
        meshdata->resetAllMeshStatic();



        set<int> meshGeoId;
        for (auto mesh_geo_id : solidids)
        {
            meshGeoId.insert(mesh_geo_id);
        }
        std::vector<std::set<int>> vec;
        vec.reserve(meshGeoId.size());
        for (int v : meshGeoId)
        {
            vec.emplace_back(std::set<int>{v});
        }
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
        // 获取对应几何维度的所有几何ID并进行一键剖分
        auto getAndSplitMeshIDsByDim = [&](int dim, bool isMesh)->std::set<int>
            {
                //std::set<int> meshGeoId = getGEOServer.geoCalculate().getAllGeoID(dim, geoDatabase);
                MeshEntity* entity = meshdata->getMeshEntity();
                std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
                if (isMesh)
                {
                    entity->setMeshPattern(MeshEntity::MeshPattern::STANDRAD_MESHING);
                    mesh->setMeshData(meshdata);
                    for (int i = 0; i < meshGeoId.size(); ++i)
                    {
                        int id = *vec[i].begin();
                        vector<int> res; res.push_back(id);
                        double size = ids2SizesMap[id];
                        mesh->MXMeshInterface(size, size, false, false, false, 0.5, 0.3, 0.2, 0, dim, entity, vec[i], {});           // 真正剖分
                    }
                }
                return meshGeoId;
            };
        getAndSplitMeshIDsByDim(dim, true);


        mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分结束"), true);

        _finishMeshList = list;
        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->ClearAll();
        getCVM->getPreRenderWidget()->getHighlightController()->clearHighlightGeoModel();
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("biastNum");
        getCVM->getPreRenderWidget()->getFontRender()->clearCommonFont("adjustNum");
        set<int> partSet;

   
        for (auto id : solidids)
        {
            partSet.insert(meshdata->getPartIdByGeoEdge(id));
        }
        
        vector<int> partIds;
        for (auto id : partSet)
        {
            partIds.push_back(id);
        }
        emit done("OK", partIds);
    }
	void mMeshThread::slot_meshForMidSurface(QHash<int, double> IdAndSize, int index, int Dim, int order)
	{
		MXMesh* test = new MXMesh;
		MeshEntity*entity = meshdata->getMeshEntity(); 
		for (auto it : IdAndSize)
		{
			set<int>meshGeoId;
			meshGeoId.insert(IdAndSize.key(it));
			/*set<int>ToggleID = mMeshMain::getInstance()->_SuturelineId;*/
			set<int>ToggleID /*= meshdata->getToggleID()*/;
			//test->MXMeshInterface(it, Dim, entity, meshGeoId, ToggleID, _meshorder);
		}

		MxFunctions::mGlobalSignals::getInstance()->finishedMeshForMidSurfaceSig();
	}

	void mMeshThread::slot_generateDirectMesh(int mode)
	{
		MXMesh* test = new MXMesh;
		MeshEntity* entity = meshdata->getMeshEntity();
		set<int>meshGeoId = _GeoIDs;
		set<int>ToggleID /*= meshdata->getToggleID()*/;
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
		test->MXMeshInterface(_lcMax, _lcMin, _smoothBool, _curvatureBool, _proximityBool, _smoothFactor, _curvatureFac, _proximityFac, _type, _dim, entity, meshGeoId, ToggleID);
		emit setMeshEntity(test->_meshEntity);
		meshdata->ensureMesh();
		emit finishedDirectMeshSig(_dim);
	}

	void mMeshThread::slot_generateMesh(int mode)
	{

		MXMesh* test = new MXMesh;
		//MeshEntity*entity = mMeshMain::getInstance()->mxmesh->_meshEntity;
		MeshEntity*entity = meshdata->getMeshEntity();
		set<int>meshGeoId = _GeoIDs;
		//set<int>ToggleID = mMeshMain::getInstance()->_SuturelineId;
		set<int>ToggleID /*= meshdata->getToggleID()*/;
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }
		test->MXMeshInterface(_lcMax, _lcMin, _smoothBool, _curvatureBool, _proximityBool, _smoothFactor, _curvatureFac, _proximityFac, _type,_dim, entity, meshGeoId, ToggleID);

		emit setMeshEntity(test->_meshEntity);

		emit finishedMeshSig(_dim);

		MxFunctions::mGlobalSignals::getInstance()->finishedMeshForMidSurfaceSig();  // 发出剖分完成的信号（几何参数化建模中接收） create by ysq 20250807
	}

	void mMeshThread::slot_adjustLineNodeNum(int mode,int lineId)
	{
		if (_dim == 1)
		{
			//if (meshdata->isGeoEdgeEnsureMesh(lineId))
			//{
			//	emit gameOverSig(_dim);
			//	//输出信息
			//	QString msg = meshdata->FindLanguageValue("这条线不能进行动态调整!");
			//	mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
			//	return;
			//}
			int num = meshdata->GEdgeIDTodynaNodeNum[lineId];
			MXMesh* test = new MXMesh;
			if (mode == 0)//左键加
			{
				try
				{
					test->dynamicEdgeNode(lineId, ++num, meshdata->GEdgeIDToBiasValue[lineId]);
					++meshdata->GEdgeIDTodynaNodeNum[lineId];
											
				}
				catch (...)
				{

				}
				
			}
			else//右键减
			{
				if (--num < 2)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					test->dynamicEdgeNode(lineId, num, meshdata->GEdgeIDToBiasValue[lineId]);
					--meshdata->GEdgeIDTodynaNodeNum[lineId];
				}
				catch (...)
				{

				}
				
			}
		}
		else if (_dim == 2)
		{
			//if (meshdata->isGeoEdgeEnsureMesh(lineId))
			//{
			//	emit gameOverSig(_dim);
			//	//输出信息
			//	QString msg = meshdata->FindLanguageValue("这条线不能进行动态调整!");
			//	mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
			//	return;
			//}
			int num = meshdata->GEdgeIDTodynaNodeNum[lineId];
			MXMesh* test = new MXMesh;
			if (mode == 0)//左键加
			{
				try
				{
					test->dynamicEdgeNode(lineId, ++num, meshdata->GEdgeIDToBiasValue[lineId]);
					++meshdata->GEdgeIDTodynaNodeNum[lineId];
				}
				catch (...)
				{

				}
				
			}
			else//右键减
			{
				if (--num < 2)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					test->dynamicEdgeNode(lineId, num, meshdata->GEdgeIDToBiasValue[lineId]);
					--meshdata->GEdgeIDTodynaNodeNum[lineId];
				}
				catch (...)
				{

				}
				
			}
		}

		emit finishedAdjust(_dim , 0 , lineId);

		//if (_mesh == nullptr)
		//{
		//	emit finishedMeshSig(_dim);
		//	return;
		//}
		//if (_dim == 1)
		//{
			//if (mode == 0)//左键加
			//{
			//	_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
			//	_mesh->SetDynamicEdgeNumber(mTempMeshData1::getInstance()->_pickLineNum.second + 1);
			//}
			//else//右键减
			//{
			//	_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
			//	_mesh->SetDynamicEdgeNumber(mTempMeshData1::getInstance()->_pickLineNum.second - 1);
			//}
			//bool dynamicSwitch = true;
			//int GmodelNumber = 0;
			//_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, mode);
			//_mesh->DynamicMessageConvey(GmodelNumber);
			////mMeshManager::getInstance()->clearMeshData();
			//mTempMeshData1::getInstance()->deleteAllMeshPart();
			//SaveMeshDataToMeshData();
			//SaveTempNodeData(_vMesh.size() - 1);
		//}
		//else if (_dim == 2)//二维的时候调整边的时候判断是否这条边在数据库中已经存在（确定剖分的线）
		//{
		//	int lineid = mtempmeshdata1::getinstance()->_picklinenum.first;
		//	mmeshgeolinedata1 *geolinedata = mmeshmodeldata1::getinstance()->getgeolinedatabyid(lineid);
		//	if (geoLineData != nullptr)
		//	{
		//		//输出信息
		//		QString msg{ "这条线不能进行动态调整" };
		//		mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//		emit outputMessageSig(msg);
		//		emit finishedMeshSig(_dim);
		//		return;
		//	}
		//	if (mode == 0)//左键加
		//	{
		//		_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
		//		_mesh->SetDynamicEdgeNumber(mTempMeshData1::getInstance()->_pickLineNum.second + 1);
		//	}
		//	else//右键减
		//	{
		//		_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
		//		_mesh->SetDynamicEdgeNumber(mTempMeshData1::getInstance()->_pickLineNum.second - 1);
		//	}

		//	bool dynamicSwitch = true;
		//	int GmodelNumber = 0;
		//	_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, mode);
		//	_mesh->DynamicMessageConvey(GmodelNumber);

		//	SaveTempNodeData(_vMesh.size() - 1);
		//}
		//mGlobalSignals::getInstance()->updateTempNodeRendSig();
		//emit finishedMeshSig(_dim);
	}

	void mMeshThread::slot_adjustFromNodeNum(int oneorall, int num, int lineId)
	{
		if (num < 2)
		{
			emit gameOverSig(_dim);
			return;
		}

		int temp = 0;
		MXMesh* test = new MXMesh;
		if (_dim == 1)
		{
			if (oneorall == 0)//一条边
			{
				temp = lineId;
				try
				{
					test->dynamicEdgeNode(lineId, num, meshdata->GEdgeIDToBiasValue[lineId]);
					meshdata->GEdgeIDTodynaNodeNum[lineId] = num;
				}
				catch (...)
				{

				}
				
			}
			else//全部边
			{
				QHash<int,QVector3D> ids = meshdata->IDToCenterOfGEdge;
				for (auto it = ids.begin(); it != ids.end(); it++)
				{
					try
					{
						test->dynamicEdgeNode(it.key(), num, meshdata->GEdgeIDToBiasValue[it.key()]);
						meshdata->GEdgeIDTodynaNodeNum[it.key()] = num;
					}
					catch (...)
					{

					}
					
				}
			}
		}
		else if (_dim == 2)
		{
			if (oneorall == 0)//一条边
			{
				//if (meshdata->isGeoEdgeEnsureMesh(lineId))
				//{
				//	emit gameOverSig(_dim); 
				//	//输出信息
				//	QString msg = meshdata->FindLanguageValue("这条线不能进行动态调整!");
				//	mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
				//	return;
				//}
				temp = lineId;
				try
				{
					test->dynamicEdgeNode(lineId, num, meshdata->GEdgeIDToBiasValue[lineId]);
					meshdata->GEdgeIDTodynaNodeNum[lineId] = num;
				}
				catch (...)
				{

				}
				
			}
			else//全部边
			{
				QHash<int, QVector3D> ids = meshdata->IDToCenterOfGEdge;
				for (auto it = ids.begin(); it != ids.end(); it++)
				{
					/*if (meshdata->isGeoEdgeEnsureMesh(it.key()))
					{
						continue;
					}*/
					try
					{
						test->dynamicEdgeNode(it.key(), num, meshdata->GEdgeIDToBiasValue[it.key()]);
						meshdata->GEdgeIDTodynaNodeNum[it.key()] = num;
					}
					catch (...)
					{

					}
				
				}
			}
		}
		emit finishedAdjust(_dim, 0, temp);

	}

	void mMeshThread::slot_adjustFromAdjustSizeNum(int oneorall, double size)
	{
		//if (_mesh == nullptr)
		//{
		//	emit finishedMeshSig(_dim);
		//	return;
		//}
		//if (_dim == 1)
		//{
		//	if (oneorall == 0)
		//	{
		//		_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
		//		_mesh->SetDynamicEdgeSize(size);
		//		bool dynamicSwitch = true;
		//		int GmodelNumber = 0;
		//		_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, oneorall);
		//		_mesh->DynamicMessageConvey(GmodelNumber);
		//		//mMeshManager::getInstance()->clearMeshData();
		//		mTempMeshData1::getInstance()->deleteAllMeshPart();
		//		SaveMeshDataToMeshData();
		//		SaveTempNodeData(_vMesh.size() - 1);
		//	}
		//	else
		//	{
		//		QList<int> LineIDs{};
		//		LineIDs = mTempMeshData1::getInstance()->_lineNum.keys();
		//		for (int i = 0; i < LineIDs.size(); ++i)
		//		{
		//			mMeshGeoLineData1 *geoLineData = mMeshModelData1::getInstance()->getGeoLineDataByID(LineIDs.at(i));
		//			if (geoLineData != nullptr)
		//			{
		//				//输出信息
		//				QString msg = QString("线编号为%1的不能进行动态调整").arg(LineIDs.at(i));
		//				mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//				emit outputMessageSig(msg);
		//				// mOutputMessage tepMsg(WarningMessage, msg);
		//				// mGlobalSignals::getInstance()->outputMessageSig(tepMsg);
		//				continue;
		//			}
		//			_mesh->SetDynamicEdgeIndex(LineIDs.at(i));
		//			_mesh->SetDynamicEdgeSize(size);
		//			bool dynamicSwitch = true;
		//			int GmodelNumber = 0;
		//			_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, oneorall);
		//			_mesh->DynamicMessageConvey(GmodelNumber);
		//			//mMeshManager::getInstance()->clearMeshData();
		//			SaveTempNodeData(_vMesh.size() - 1);
		//		}
		//		mTempMeshData1::getInstance()->deleteAllMeshPart();
		//		SaveMeshDataToMeshData();
		//	}
		//	mGlobalSignals::getInstance()->updateTempMeshRendSig();
		//}
		//else if (_dim == 2)//二维的时候调整边的时候判断是否这条边在数据库中已经存在（确定剖分的线）
		//{
		//	if (oneorall == 0)
		//	{
		//		int LineId = mTempMeshData1::getInstance()->_pickLineNum.first;
		//		mMeshGeoLineData1 *geoLineData = mMeshModelData1::getInstance()->getGeoLineDataByID(LineId);
		//		if (geoLineData != nullptr)
		//		{
		//			//输出信息
		//			QString msg{ "这条线不能进行动态调整" };
		//			mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//			emit outputMessageSig(msg);
		//			emit finishedMeshSig(_dim);
		//			return;
		//		}

		//		_mesh->SetDynamicEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
		//		_mesh->SetDynamicEdgeSize(size);
		//		bool dynamicSwitch = true;
		//		int GmodelNumber = 0;
		//		_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, oneorall);
		//		_mesh->DynamicMessageConvey(GmodelNumber);
		//		SaveTempNodeData(_vMesh.size() - 1);
		//	}
		//	else
		//	{
		//		QList<int> LineIDs{};
		//		LineIDs = mTempMeshData1::getInstance()->_lineNum.keys();
		//		set<int> geoLineIDs{ LineIDs.begin(),LineIDs.end() };
		//		for (int geoLineID : geoLineIDs)
		//		{
		//			mMeshGeoLineData1 *geoLineData = mMeshModelData1::getInstance()->getGeoLineDataByID(geoLineID);
		//			if (geoLineData != nullptr)
		//			{
		//				//输出信息
		//				QString msg = QString("线编号为%1的不能进行动态调整").arg(geoLineID);
		//				mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//				emit outputMessageSig(msg);
		//				continue;
		//			}
		//			_mesh->SetDynamicEdgeIndex(geoLineID);
		//			_mesh->SetDynamicEdgeSize(size);
		//			bool dynamicSwitch = true;
		//			int GmodelNumber = 0;
		//			_mesh->DynamicMesh(dynamicSwitch, GmodelNumber, oneorall);
		//			_mesh->DynamicMessageConvey(GmodelNumber);
		//			SaveTempNodeData(_vMesh.size() - 1);
		//		}
		//	}
		//}

		//mGlobalSignals::getInstance()->updateTempNodeRendSig();
		//emit finishedMeshSig(_dim);
	}

	void mMeshThread::slot_biasLineFactor(int mode, int lineId)
	{
		//边上节点个数只有两个
		if (meshdata->GEdgeIDTodynaNodeNum[lineId] == 2)
		{
			emit gameOverSig(_dim);
			return;
		}
		if (_dim == 1)
		{
			MXMesh* test = new MXMesh;
			if (mode == 0)//左键加
			{
				double value = meshdata->GEdgeIDToBiasValue[lineId] + 0.1;
				value = floor(value*10.000f + 0.5) / 10.000f;
				if (value > 20.0)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					meshdata->GEdgeIDToBiasValue[lineId] = value;
					test->biasEdgeNode(lineId, value);
				}
				catch (...)
				{

				}
			
			}
			else//右键减
			{
				double value = meshdata->GEdgeIDToBiasValue[lineId] - 0.1;
				value = floor(value*10.000f + 0.5) / 10.000f;
				if (value < -20.0)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					meshdata->GEdgeIDToBiasValue[lineId] = value;
					test->biasEdgeNode(lineId, value);
				}
				catch (...)
				{

				}

			}
		}
		else if (_dim == 2)
		{
			//if (meshdata->isGeoEdgeEnsureMesh(lineId))
			//{
			//	emit gameOverSig(_dim);
			//	//输出信息
			//	QString msg = meshdata->FindLanguageValue("这条线不能进行动态调整!");
			//	mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
			//	return;
			//}
			MXMesh* test = new MXMesh;
			if (mode == 0)//左键加
			{
				double value = meshdata->GEdgeIDToBiasValue[lineId] + 0.1;
				value = floor(value*10.000f + 0.5) / 10.000f;
				if (value > 20.0)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					meshdata->GEdgeIDToBiasValue[lineId] = value;
					test->biasEdgeNode(lineId, value);
				}
				catch (...)
				{

				}
				
			}
			else//右键减
			{
				double value = meshdata->GEdgeIDToBiasValue[lineId] - 0.1;
				value = floor(value*10.000f + 0.5) / 10.000f;
				if (value < -20.0)
				{
					emit gameOverSig(_dim);
					return;
				}
				try
				{
					meshdata->GEdgeIDToBiasValue[lineId] = value;
					test->biasEdgeNode(lineId, value);
				}
				catch (...)
				{

				}
				
			}
		}
		emit finishedAdjust(_dim, 1, lineId);

	}

	void mMeshThread::slot_biasFromFactor(int oneorall, double factor, int LineId)
	{
		if (LineId != 0)
		{
			//边上节点个数只有两个
			if (meshdata->GEdgeIDTodynaNodeNum[LineId] == 2)
			{
				emit gameOverSig(_dim);
				return;
			}
		}
		int temp = 0;
		if (_dim == 1)
		{
			MXMesh* test = new MXMesh;
			if (oneorall == 0)//一条边
			{
				temp = LineId;
				try
				{
					meshdata->GEdgeIDToBiasValue[LineId] = factor;
					test->biasEdgeNode(LineId, factor);
				}
				catch (...)
				{

				}
				
			}
			else//全部边
			{
				QHash<int, double> ids = meshdata->GEdgeIDToBiasValue;
				for (auto it = ids.begin(); it != ids.end(); it++)
				{
					if (meshdata->GEdgeIDTodynaNodeNum[it.key()] == 2)
					{
						continue;
					}
					try
					{
						meshdata->GEdgeIDToBiasValue[it.key()] = factor;
						test->biasEdgeNode(it.key(), factor);
					}
					catch (...)
					{

					}
					
				}
			}
		}
		else if (_dim == 2)
		{
			MXMesh* test = new MXMesh;
			if (oneorall == 0)//一条边
			{
				//if (meshdata->isGeoEdgeEnsureMesh(LineId))
				//{
				//	emit gameOverSig(_dim);
				//	//输出信息
				//	QString msg = meshdata->FindLanguageValue("这条线不能进行动态调整!");
				//	mGlobalSignals::getInstance()->outputMessageSig(1, msg, true);
				//	return;
				//}
				temp = LineId;
				try
				{
					meshdata->GEdgeIDToBiasValue[LineId] = factor;
					test->biasEdgeNode(LineId, factor);
				}
				catch (...)
				{

				}
			
			}
			else//全部边
			{
				QHash<int, double> ids = meshdata->GEdgeIDToBiasValue;
				for (auto it = ids.begin(); it != ids.end(); it++)
				{
					/*if (meshdata->isGeoEdgeEnsureMesh(it.key()))
					{
						continue;
					}*/
					if (meshdata->GEdgeIDTodynaNodeNum[it.key()] == 2)
					{
						continue;
					}
					try
					{
						meshdata->GEdgeIDToBiasValue[it.key()] = factor;
						test->biasEdgeNode(it.key(), factor);
					}
					catch (...)
					{

					}

				}
			}
		}
		emit finishedAdjust(_dim, 1, temp);

		//if (_mesh == nullptr)
		//{
		//	emit finishedMeshSig(_dim);
		//	return;
		//}
		//if (_dim == 1)
		//{
		//	if (oneorall == 0)
		//	{
		//		int index = mTempMeshData1::getInstance()->_pickLineNum.first;
		//		mTempMeshData1::getInstance()->_lineBias[index] = factor;
		//		_mesh->SetBiasEdgeIndex(index);
		//		_mesh->SetBiasEdgeValue(factor);
		//		int GmodelNumber = 0;
		//		_mesh->BiasMesh(GmodelNumber);
		//		_mesh->DynamicMessageConvey(GmodelNumber);
		//		//mMeshManager::getInstance()->clearMeshData();
		//		SaveTempNodeData(_vMesh.size() - 1);
		//		mTempMeshData1::getInstance()->deleteAllMeshPart();
		//		SaveMeshDataToMeshData();
		//	}
		//	else
		//	{
		//		QList<int> LineIDs{};
		//		LineIDs = mTempMeshData1::getInstance()->_lineNum.keys();
		//		for (int i = 0; i < LineIDs.size(); ++i)
		//		{
		//			mMeshGeoLineData1 *geoLineData = mMeshModelData1::getInstance()->getGeoLineDataByID(LineIDs.at(i));
		//			if (geoLineData != nullptr)
		//			{
		//				//输出信息
		//				QString msg = QString("线编号为%1的不能进行动态调整").arg(LineIDs.at(i));
		//				mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//				emit outputMessageSig(msg);
		//				// mOutputMessage tepMsg(WarningMessage, msg);
		//				// mGlobalSignals::getInstance()->outputMessageSig(tepMsg);
		//				continue;
		//			}
		//			mTempMeshData1::getInstance()->_lineBias[LineIDs.at(i)] = factor;
		//			_mesh->SetBiasEdgeIndex(LineIDs.at(i));
		//			_mesh->SetBiasEdgeValue(factor);
		//			int GmodelNumber = 0;
		//			_mesh->BiasMesh(GmodelNumber);
		//			_mesh->DynamicMessageConvey(GmodelNumber);
		//			//mMeshManager::getInstance()->clearMeshData();
		//			SaveTempNodeData(_vMesh.size() - 1);
		//		}
		//		mTempMeshData1::getInstance()->deleteAllMeshPart();
		//		SaveMeshDataToMeshData();
		//	}
		//	mGlobalSignals::getInstance()->updateTempMeshRendSig();
		//}
		//else if (_dim == 2)//二维的时候调整边的时候判断是否这条边在数据库中已经存在（确定剖分的线）
		//{
		//	if (oneorall == 0)
		//	{
		//		int LineId = mTempMeshData1::getInstance()->_pickLineNum.first;
		//		QList<int> Linelist = mDatabaseSingleton::getInstance()->getMeshData()->_node_Line_name.values();
		//		if (Linelist.contains(LineId))
		//		{
		//			//输出信息
		//			QString msg{ "这条线不能进行动态调整" };
		//			mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//			emit outputMessageSig(msg);
		//			emit finishedMeshSig(_dim);
		//			return;
		//		}
		//		mTempMeshData1::getInstance()->_lineBias[LineId] = factor;
		//		_mesh->SetBiasEdgeIndex(mTempMeshData1::getInstance()->_pickLineNum.first);
		//		_mesh->SetBiasEdgeValue(factor);
		//		int GmodelNumber = 0;
		//		_mesh->BiasMesh(GmodelNumber);
		//		_mesh->DynamicMessageConvey(GmodelNumber);
		//		SaveTempNodeData(_vMesh.size() - 1);
		//	}
		//	else
		//	{
		//		QList<int> LineIDs{};
		//		LineIDs = mTempMeshData1::getInstance()->_lineNum.keys();
		//		for (int i = 0; i < LineIDs.size(); ++i)
		//		{
		//			mMeshGeoLineData1 *geoLineData = mMeshModelData1::getInstance()->getGeoLineDataByID(LineIDs.at(i));
		//			if (geoLineData != nullptr)
		//			{
		//				//输出信息
		//				QString msg = QString("线编号为%1的不能进行动态调整").arg(LineIDs.at(i));
		//				mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//				emit outputMessageSig(msg);
		//				continue;
		//			}
		//			mTempMeshData1::getInstance()->_lineBias[LineIDs.at(i)] = factor;
		//			_mesh->SetBiasEdgeIndex(LineIDs.at(i));
		//			_mesh->SetBiasEdgeValue(factor);
		//			int GmodelNumber = 0;
		//			_mesh->BiasMesh(GmodelNumber);
		//			_mesh->DynamicMessageConvey(GmodelNumber);
		//			SaveTempNodeData(_vMesh.size() - 1);
		//		}
		//	}
		//}

		//mGlobalSignals::getInstance()->updateTempNodeRendSig();

		//emit finishedMeshSig(_dim);
	}

	void mMeshThread::slot_saveMeshDataToDataBase(int dim)
	{
		//if (_mesh == nullptr)
		//{
		//	return;
		//}
		//mMeshManager::getInstance()->saveMeshDataToDataBase(dim, _meshorder);
		//MMeshView::mTempMeshData1::getInstance()->saveData();
		//mGlobalSignals::getInstance()->updateTempNodeRendSig();
		//mGlobalSignals::getInstance()->updateTempMeshRendSig();
		//MBasicFunction::mGlobalSignals::getInstance()->updateTheMeshTreeItem();

		//_lastNumberGmodel = PreInterface::getInstance()->meshGModeNumber;
		////MMeshView::mTempMeshData1::getInstance()->clearTempNodeData();
		//_mesh->UpdateMessageAfterDynamicMesh(_lastNumberGmodel);
		////Save2D_1DMeshData();
		//_setList.clear();
		//_partIDName.clear();

		////输出信息
		//QString msg = QString("保存网格");
		//mGlobalSignals::getInstance()->outputStatusBarSig(msg);
		//emit outputMessageSig(msg);
		////更新面板上网格零件信息
		//mGlobalSignals::getInstance()->updatePartInfoSig();

		//完成信号
		emit finishedOKSig(dim);
	}

	void mMeshThread::slot_rejectMesh(int dim)
	{
	//	if (_mesh == nullptr)
	//	{
	//		return;
	//	}
	//	mTempMeshData1::getInstance()->rejectData();
	//	//MMeshView::mTempMeshData1::getInstance()->deleteAllMeshPart();
	//	mGlobalSignals::getInstance()->updateTempNodeRendSig();
	//	mGlobalSignals::getInstance()->updateTempMeshRendSig();


	//	_setList.clear();
	//	_partIDName.clear();
	//	_mesh->GiveUpMesh(_lastNumberGmodel, _meshorder);

	//	for (PreInterface* dv : _vMesh)
	//	{
	//		delete dv;
	//		dv = NULL;
	//	}
	//	_vMesh.clear();

	//	//输出信息
	//	QString msg = QString("撤销生成网格");
	//	mGlobalSignals::getInstance()->outputStatusBarSig(msg);
	//	emit outputMessageSig(msg);
	//}

	//void mMeshThread::SaveMeshDataToMeshData(MDataGeo::GeoDataModelSet* set, int i)
	//{
	//	if (_vMesh[i] == nullptr)
	//	{
	//		return;
	//	}

	//	QString PartName = set->getPartNameTag().first;
	//	int PartID = set->getPartNameTag().second;

	//	mMeshPartData1 *partData = mTempMeshData1::getInstance()->getMeshPartDataByPartName(PartName);
	//	if (partData == nullptr)
	//	{
	//		partData = new mMeshPartData1(PartName, PartID);
	//		//mTempMeshData1::getInstance()->appendMeshPartData(PartName, partData);

	//		//设置颜色
	//		if (MDataGeo::mGeoModelData1::getInstance()->isExistGeoPart(PartName))
	//		{
	//			QVector3D color = MDataGeo::mGeoModelData1::getInstance()->getGeoPartColor(PartName);
	//			partData->setPartColor(color);
	//		}
	//	}

	//	_partIDName[PartID] = PartName;
	//	//节点信息
	//	QVector<MVertex*>::iterator it;
	//	for (it = _vMesh[i]->PreMeshVertex->begin(); it != _vMesh[i]->PreMeshVertex->end(); it++)
	//	{
	//		int ID = (*it)->getIndex();
	//		QVector3D temp = QVector3D((*it)->x(), (*it)->y(), (*it)->z());
	//		mMeshNodeData1 *nodeData = new mMeshNodeData1(ID, temp);
	//		mTempMeshData1::getInstance()->appendNodeData(ID, nodeData);
	//	}

	//	//临时变量
	//	int elementID, index0, index1, index2, index3, index4, index5, index6, index7, index8, index9;
	//	if (_dim == 1)
	//	{
	//		//获取线的编号
	//		int geoLineID = set->getEdgePartTag().first;

	//		//网格线信息
	//		QVector<GEdge*>::iterator itge;
	//		std::vector<MLine*>::iterator itml;
	//		for (itge = _vMesh[i]->PreMeshEdge->begin(); itge != _vMesh[i]->PreMeshEdge->end(); itge++)
	//		{
	//			int geoLineID = (*itge)->tag();//获得几何线的编号
	//			mMeshGeoLineData1 *geoLineData = mTempMeshData1::getInstance()->getGeoLineDataByID(geoLineID);
	//			if (geoLineData == nullptr)
	//			{
	//				partData->appendGeoLineID(geoLineID);
	//				geoLineData = new mMeshGeoLineData1(geoLineID);
	//				mTempMeshData1::getInstance()->appendGeoLineData(geoLineID, geoLineData);
	//			}
	//			for (itml = (*itge)->lines.begin(); itml != (*itge)->lines.end(); itml++)
	//			{
	//				elementID = (*itml)->getNum();//获得网格线的编号
	//				index0 = (*itml)->getVertex(0)->getIndex();
	//				index1 = (*itml)->getVertex(1)->getIndex();

	//				//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);

	//				mMeshData1 *meshData = new mMeshData1(elementID, MeshBeam, PartName, _meshorder);

	//				meshData->setNodeIndex(QVector<int>{index0, index1});

	//				mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

	//				geoLineData->appendMeshID(elementID);
	//			}
	//		}

	//	}
	//	else if (_dim == 2)
	//	{

	//		//面信息
	//		QVector<GFace*>::iterator itgf;
	//		std::vector<MTriangle*>::iterator itmt;
	//		std::vector<MQuadrangle*>::iterator itmq;
	//		for (itgf = _vMesh[i]->PreMeshFace->begin(); itgf != _vMesh[i]->PreMeshFace->end(); itgf++)
	//		{
	//			int geoFaceID = (*itgf)->tag();//获得几何面的编号
	//			mMeshGeoFaceData1 *geoFaceData = mTempMeshData1::getInstance()->getGeoFaceDataByID(geoFaceID);
	//			if (geoFaceData == nullptr)
	//			{
	//				partData->appendGeoFaceID(geoFaceID);
	//				geoFaceData = new mMeshGeoFaceData1(geoFaceID);
	//				mTempMeshData1::getInstance()->appendGeoFaceData(geoFaceID, geoFaceData);
	//			}
	//			for (itmt = (*itgf)->triangles.begin(); itmt != (*itgf)->triangles.end(); itmt++)
	//			{
	//				elementID = (*itmt)->getNum();

	//				index0 = (*itmt)->getVertex(0)->getIndex();
	//				index1 = (*itmt)->getVertex(1)->getIndex();
	//				index2 = (*itmt)->getVertex(2)->getIndex();

	//				//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index2)->addMesh(elementID);

	//				mMeshData1 *meshData = new mMeshData1(elementID, MeshTri, PartName, _meshorder);

	//				if ((*itmt)->getOrder() == 1)
	//				{
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2});
	//				}
	//				else
	//				{
	//					index3 = (*itmt)->getVertex(3)->getIndex();
	//					index4 = (*itmt)->getVertex(4)->getIndex();
	//					index5 = (*itmt)->getVertex(5)->getIndex();
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3, index4, index5});

	//					//if (meshData->getNodeIndex().contains(0))
	//					//{
	//						//qDebug() << elementID << meshData->getNodeIndex();
	//					//}
	//					//qDebug() << elementID << meshData->getNodeIndex();
	//				}

	//				mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

	//				geoFaceData->appendMeshID(elementID);

	//			}
	//			for (itmq = (*itgf)->quadrangles.begin(); itmq != (*itgf)->quadrangles.end(); itmq++)
	//			{
	//				QVector<QVector3D> tempQvector;
	//				elementID = (*itmq)->getNum();

	//				index0 = (*itmq)->getVertex(0)->getIndex();
	//				index1 = (*itmq)->getVertex(1)->getIndex();
	//				index2 = (*itmq)->getVertex(2)->getIndex();
	//				index3 = (*itmq)->getVertex(3)->getIndex();

	//				//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index2)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index3)->addMesh(elementID);

	//				mMeshData1 *meshData = new mMeshData1(elementID, MeshQuad, PartName, _meshorder);

	//				if ((*itmq)->getOrder() == 1)
	//				{
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3});
	//				}
	//				else
	//				{
	//					index4 = (*itmq)->getVertex(4)->getIndex();
	//					index5 = (*itmq)->getVertex(5)->getIndex();
	//					index6 = (*itmq)->getVertex(6)->getIndex();
	//					index7 = (*itmq)->getVertex(7)->getIndex();
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3, index4, index5, index6, index7});
	//				}

	//				mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

	//				geoFaceData->appendMeshID(elementID);
	//			}
	//		}

	//		////两点线信息，存在了单元线中
	//		QVector<GEdge*>::iterator itge;
	//		std::vector<MLine*>::iterator itml;
	//		for (itge = _vMesh[i]->PreMeshEdge->begin(); itge != _vMesh[i]->PreMeshEdge->end(); itge++)
	//		{
	//			int geoLineID = (*itge)->tag();//获得几何线的编号
	//			mMeshGeoLineData1 *geoLineData = mTempMeshData1::getInstance()->getGeoLineDataByID(geoLineID);
	//			if (geoLineData == nullptr)
	//			{
	//				partData->appendGeoLineID(geoLineID);
	//				geoLineData = new mMeshGeoLineData1(geoLineID);
	//				mTempMeshData1::getInstance()->appendGeoLineData(geoLineID, geoLineData);
	//			}
	//			for (itml = (*itge)->lines.begin(); itml != (*itge)->lines.end(); itml++)
	//			{
	//				int meshLineID = (*itml)->getNum();//获得单元线的编号
	//				index0 = (*itml)->getVertex(0)->getIndex();
	//				index1 = (*itml)->getVertex(1)->getIndex();

	//				mMeshNodeData1 *nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index0);
	//				if (nodeData == nullptr)
	//				{
	//					continue;
	//				}
	//				nodeData->addMeshLine(meshLineID);
	//				nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index1);
	//				if (nodeData == nullptr)
	//				{
	//					continue;
	//				}
	//				nodeData->addMeshLine(meshLineID);

	//				for (std::map<MTriangle*, int>::iterator itMTri = (*itml)->LineInTri.begin(); itMTri != (*itml)->LineInTri.end(); itMTri++)
	//				{
	//					int elementID = itMTri->first->getNum();
	//					mMeshLineData1 *meshLineData = new mMeshLineData1(meshLineID, std::set<int>{index0, index1}, elementID, itMTri->second, PartName, geoLineData->getGeoLineID());
	//					if (_meshorder == 2)
	//					{
	//						meshLineData->setMeshLineNodeIndex3((*itml)->getVertex(2)->getIndex());
	//					}
	//					mTempMeshData1::getInstance()->appendMeshLineData(meshLineID, meshLineData);
	//					geoLineData->appendMeshLineID(meshLineID);
	//					mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
	//					if (meshData != nullptr)
	//					{
	//						meshData->appendMeshLine(meshLineID);
	//					}

	//				}
	//				for (std::map<MQuadrangle*, int>::iterator itMQuad = (*itml)->LineInQuad.begin(); itMQuad != (*itml)->LineInQuad.end(); itMQuad++)
	//				{
	//					int elementID = itMQuad->first->getNum();
	//					mMeshLineData1 *meshLineData = new mMeshLineData1(meshLineID, std::set<int>{index0, index1}, elementID, itMQuad->second, PartName, geoLineData->getGeoLineID());
	//					if (_meshorder == 2)
	//					{
	//						meshLineData->setMeshLineNodeIndex3((*itml)->getVertex(2)->getIndex());
	//					}
	//					mTempMeshData1::getInstance()->appendMeshLineData(meshLineID, meshLineData);
	//					geoLineData->appendMeshLineID(meshLineID);
	//					mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
	//					if (meshData != nullptr)
	//					{
	//						meshData->appendMeshLine(meshLineID);
	//					}
	//				}

	//			}
	//		}
	//	}

	//	else if (_dim == 3)
	//	{
	//		//C3D4信息
	//		QVector<GRegion*>::iterator itgr;
	//		std::vector<MTetrahedron*>::iterator itmtet;
	//		if (_vMesh[i]->PreMeshRegion->size() == 0) {
	//			int a = 0;
	//		}
	//		for (itgr = _vMesh[i]->PreMeshRegion->begin(); itgr != _vMesh[i]->PreMeshRegion->end(); itgr++)
	//		{
	//			for (std::vector<MTetrahedron*>::iterator itmtet = (*itgr)->tetrahedra.begin();
	//				itmtet != (*itgr)->tetrahedra.end(); itmtet++)
	//			{
	//				QVector<QVector3D> tempQvector;
	//				elementID = (*itmtet)->getNum();

	//				index0 = (*itmtet)->getVertex(0)->getIndex();
	//				index1 = (*itmtet)->getVertex(1)->getIndex();
	//				index2 = (*itmtet)->getVertex(2)->getIndex();
	//				index3 = (*itmtet)->getVertex(3)->getIndex();

	//				//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index2)->addMesh(elementID);
	//				//mTempMeshData1::getInstance()->getNodeDataByID(index3)->addMesh(elementID);

	//				mMeshData1 *meshData = new mMeshData1(elementID, MeshTet, PartName, _meshorder);

	//				if ((*itmtet)->getOrder() == 1)
	//				{
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3});
	//				}
	//				else
	//				{
	//					index4 = (*itmtet)->getVertex(4)->getIndex();
	//					index5 = (*itmtet)->getVertex(5)->getIndex();
	//					index6 = (*itmtet)->getVertex(6)->getIndex();
	//					index7 = (*itmtet)->getVertex(7)->getIndex();
	//					index8 = (*itmtet)->getVertex(8)->getIndex();
	//					index9 = (*itmtet)->getVertex(9)->getIndex();
	//					meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3, index4, index5, index6, index7, index8, index9});
	//				}
	//				meshData->resizeFaceSize(4);

	//				mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

	//				partData->appendMeshID(elementID);
	//			}
	//		}


	//		//三角形面信息
	//		QVector<GFace*>::iterator itgf;
	//		std::vector<MTriangle*>::iterator itmt;
	//		for (itgf = _vMesh[i]->PreMeshFace->begin(); itgf != _vMesh[i]->PreMeshFace->end(); itgf++)
	//		{
	//			int geoFaceID = (*itgf)->tag();//获得几何面的编号
	//			mMeshGeoFaceData1 *geoFaceData = mTempMeshData1::getInstance()->getGeoFaceDataByID(geoFaceID);
	//			if (geoFaceData == nullptr)
	//			{
	//				partData->appendGeoFaceID(geoFaceID);
	//				geoFaceData = new mMeshGeoFaceData1(geoFaceID);
	//				mTempMeshData1::getInstance()->appendGeoFaceData(geoFaceID, geoFaceData);
	//			}
	//			for (itmt = (*itgf)->triangles.begin(); itmt != (*itgf)->triangles.end(); itmt++)
	//			{
	//				int meshfaceID = (*itmt)->getNum();

	//				index0 = (*itmt)->getVertex(0)->getIndex();
	//				index1 = (*itmt)->getVertex(1)->getIndex();
	//				index2 = (*itmt)->getVertex(2)->getIndex();
	//				if ((*itmt)->TriInTetra.size() == 2)
	//				{
	//					continue;
	//				}
	//				std::map<MTetrahedron*, int>::iterator itmtet = (*itmt)->TriInTetra.begin();
	//				mMeshFaceData1 *meshFaceData1 = nullptr;
	//				for (; itmtet != (*itmt)->TriInTetra.end(); itmtet++)
	//				{
	//					int elementID = itmtet->first->getNum();
	//					meshFaceData1 = new mMeshFaceData1(meshfaceID, QVector<int>{index0, index1, index2}, elementID, itmtet->second, PartName, geoFaceID);
	//					if (_meshorder == 2)
	//					{
	//						meshFaceData1->setHighOrderNodeIndex(QVector<int>{(*itmt)->getVertex(3)->getIndex(), (*itmt)->getVertex(4)->getIndex(), (*itmt)->getVertex(5)->getIndex()});
	//					}
	//					mTempMeshData1::getInstance()->appendMeshFaceData(meshfaceID, meshFaceData1);
	//					geoFaceData->appendMeshFaceID(meshfaceID);
	//					partData->appendMeshFaceIDs(meshfaceID);
	//					mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
	//					if (meshData != nullptr)
	//					{
	//						meshData->setFacePositionID(itmtet->second - 1, meshfaceID);
	//					}
	//					break;
	//				}
	//			}
	//		}

	//		//两点线信息，存在了单元线中
	//		QVector<GEdge*>::iterator itge;
	//		std::vector<MLine*>::iterator itml;
	//		for (itge = _vMesh[i]->PreMeshEdge->begin(); itge != _vMesh[i]->PreMeshEdge->end(); itge++)
	//		{
	//			int geoLineID = (*itge)->tag();//获得几何线的编号
	//			mMeshGeoLineData1 *geoLineData = mTempMeshData1::getInstance()->getGeoLineDataByID(geoLineID);
	//			if (geoLineData == nullptr)
	//			{
	//				partData->appendGeoLineID(geoLineID);
	//				geoLineData = new mMeshGeoLineData1(geoLineID);
	//				mTempMeshData1::getInstance()->appendGeoLineData(geoLineID, geoLineData);
	//			}
	//			for (itml = (*itge)->lines.begin(); itml != (*itge)->lines.end(); itml++)
	//			{
	//				int meshLineID = (*itml)->getNum();//获得单元线的编号
	//				index0 = (*itml)->getVertex(0)->getIndex();
	//				index1 = (*itml)->getVertex(1)->getIndex();

	//				mMeshNodeData1 *nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index0);
	//				if (nodeData == nullptr)
	//				{
	//					continue;
	//				}
	//				nodeData->addMeshLine(meshLineID);
	//				nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index1);
	//				if (nodeData == nullptr)
	//				{
	//					continue;
	//				}
	//				nodeData->addMeshLine(meshLineID);

	//				for (std::map<MTetrahedron*, int>::iterator itmtet = (*itml)->LineInTetra.begin();
	//					itmtet != (*itml)->LineInTetra.end(); itmtet++)
	//				{
	//					int elementID = itmtet->first->getNum();
	//					mMeshLineData1 *meshLineData = new mMeshLineData1(meshLineID, std::set<int>{index0, index1}, elementID, itmtet->second, PartName, geoLineData->getGeoLineID());
	//					if (_meshorder == 2)
	//					{
	//						meshLineData->setMeshLineNodeIndex3((*itml)->getVertex(2)->getIndex());
	//					}
	//					mTempMeshData1::getInstance()->appendMeshLineData(meshLineID, meshLineData);
	//					geoLineData->appendMeshLineID(meshLineID);
	//					mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
	//					if (meshData != nullptr)
	//					{
	//						meshData->appendMeshLine(meshLineID);
	//					}
	//				}
	//			}
	//		}

	//	}

	//	mTempMeshData1::getInstance()->appendMeshPartData(PartName, partData);
	}

    void mMeshThread::slot_createMeshBatch(const MGUIMesh::BatchMeshTask& task)
    {
        // 所有异常分支都必须 emit done，否则批处理 worker 永远收不到终态信号，
        // 后续任务（t2 等）将一直滞留待派发队列，表现为批量流程卡死
        MechModelData::mModelData* modelData = getCVM->getModelData();
        if (!modelData)
        {
            emit done("NoModelData");
            return;
        }
        mMeshData* meshdata = modelData->getMeshData();
        if (!meshdata)
        {
            emit done("NoMeshData");
            return;
        }
        auto geoDatabase = modelData->getGeoData();
        if (!geoDatabase)
        {
            emit done("NoGeoData");
            return;
        }

        MeshEntity* entity = meshdata->getMeshEntity();
        if (!entity)
        {
            emit done("NoMeshEntity");
            return;
        }
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(true);
        // 按任务 dim 决定剖分哪些维度：dim=2 只剖面（含实体表面），dim=3 只剖实体（四面体），
        // dim 为其他值（<=0 或 >3，旧配置缺省）保持原行为，自动剖分 2D 与 3D 全部几何
        const int taskDim = task.dim;
        const bool mesh2DEnabled = (taskDim == 2 || taskDim == 3 || taskDim <= 0 || taskDim > 3);
        const bool mesh3DEnabled = (taskDim == 3 || taskDim <= 0 || taskDim > 3);

        // 步骤1: 获取所有维度ID
        std::set<int> allSolidIds = getGEOServer.geoCalculate().getAllGeoID(3);
        std::set<int> allFaceIds = getGEOServer.geoCalculate().getAllGeoID(2);
        std::set<int> allEdgeIds = getGEOServer.geoCalculate().getAllGeoID(1);

        // 步骤2: 3D→2D精确去重（遍历所有3D体，收集其表面的面ID）
        std::set<int> remainSolidIds = allSolidIds;
        std::set<int> facesBelongToSolid;
        for (int solidId : remainSolidIds)
        {
            MXGeoSolid* solid = entity->getGeoSolidById(solidId);
            if (!solid)
                continue;
            if (solid->getMeshStatic() != NoMesh)
            {
                allSolidIds.erase(allSolidIds.find(solidId));
                continue;
            }
            for (MXGeoFace* face : solid->getface())
            {
                if (face)
                    facesBelongToSolid.insert(face->getGeoTag());
            }
        }

        // 从2D列表中剔除属于3D体的面，保留独立的2D面。
        // 例外：dim=2 的任务不剖实体，实体的未剖表面也应纳入面剖分集合，
        // 否则实体几何模型在 dim=2 任务下 meshGeoId_2D 为空，无事可做。
        const bool keepSolidFaces = (taskDim == 2 && !mesh3DEnabled);

        std::set<int> meshGeoId_2D;
        std::set<int>remainFaceIds = allFaceIds;
        for (int faceId : remainFaceIds)
        {
            MXGeoFace* face = entity->getGeoFaceById(faceId);
            if (!face)
            {
                continue;
            }
            if (face->getMeshStatic() != NoMesh)
            {
                allFaceIds.erase(allFaceIds.find(faceId));
                continue;
            }
            if (keepSolidFaces || facesBelongToSolid.count(faceId) == 0)
            {
                meshGeoId_2D.insert(faceId);
            }
        }

        // 步骤3: 2D→1D去重（Part级别，优化为 O(N_edge + N_face)）
        // 如果一个Part中存在2D面，则该Part的1D边由2D剖分自动处理
        std::set<int> partsWithFace; //【OCC摘除·S9】部件身份改 ID（原 set 指针）
        for (int faceId : allFaceIds)
        {
            const int facePartId = MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, faceId); //【OCC摘除·S9】GN 部件 ID（原 set 反查退役）
            if (facePartId >= 0)
            	partsWithFace.insert(facePartId);
        }

        std::set<int> meshGeoId_1D;
        std::set<int>remainEdgeIds = allEdgeIds;
        for (int edgeId : remainEdgeIds)
        {
            const int edgePartId = MGeoNeutralQuery::partIdByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 1, edgeId); //【OCC摘除·S9】GN 部件 ID
            if (edgePartId < 0)
            	continue;
            MXGeoEdge* edge = entity->getGeoEdgeById(edgeId);
            if (!edge)
            {
                continue;
            }
            if (edge->getMeshStatic() != NoMesh)
            {
                allEdgeIds.erase(allEdgeIds.find(edgeId));
                continue;
            }
            if (partsWithFace.count(edgePartId) == 0)
                meshGeoId_1D.insert(edgeId);
        }
        if (meshdata)
        {
            if (meshdata->getMaxNodeId() > MXMeshParameters::getInstance()->_nodeNum)
            {
                MXMeshParameters::getInstance()->_nodeNum = meshdata->getMaxNodeId();
            }
            MXMeshParameters::getInstance()->_elementNum = meshdata->getMaxEleId();
        }
        else
        {
            MXMeshParameters::getInstance()->_nodeNum = 0;
            MXMeshParameters::getInstance()->_elementNum = 0;
        }

        auto getMeshShape = [&](const QString& shape, int dim, MeshShapeType& current_type)
            {
                switch (dim)
                {
                    // 二维网格
                case 2:
                {
                    if (shape == QString("三角形"))
                    {
                        current_type = MeshShapeType::TRI_MESH;
                    }
                    else if (shape == QString("四边形主导"))
                    {
                        current_type = MeshShapeType::QUAD_DOMINT_MESH;
                    }
                    else if (shape == QString("全四边形"))
                    {
                        current_type = MeshShapeType::QUAD_MESH;
                    }

                }break;

                // 三维网格
                case 3:
                {
                    if (shape == QString("四面体"))
                    {
                        current_type = MeshShapeType::TET_MESH;
                    }
                    else if (shape == QString("六面体"))
                    {
                        current_type = MeshShapeType::HEX_MESH;
                    }

                }break;

                default:break;
                }
            };


        // 步骤4: 按低维→高维顺序剖分，每次剖分后调用ensureMesh()
        auto splitByDim = [entity, meshdata](int dim, const std::set<int>& meshGeoId, double defaultmeshSize,unordered_map<uint, MeshShapeType>mesh_shape_type, MeshEntity::MeshPattern meshpattern)
            {
                if (meshGeoId.empty())
                    return;
                std::shared_ptr<MXMesh>mesh = std::make_shared<MXMesh>();
                entity->setMeshPattern(meshpattern);               
                entity->setPreMeshSize(dim, defaultmeshSize);
                mesh->saveMeshShape(dim, mesh_shape_type);
                mesh->setMeshData(meshdata);
                mesh->MXMeshInterface(dim, entity, meshGeoId, {});
                meshdata->ensureMesh();  // 每次剖分后同步
            };

        bool hasAnyGeo = false;
       /* if (!meshGeoId_1D.empty())
        {
            hasAnyGeo = true;
            splitByDim(1, meshGeoId_1D);
        }*/
        if (mesh2DEnabled && !meshGeoId_2D.empty())
        {
            bool sizeDiff = false;
            vector<int>solidids;
            for (auto ita : meshGeoId_2D)
            {
                solidids.push_back(ita);
            }
            std::unordered_map<uint, MeshShapeType>mesh_shape_type;
            MeshShapeType current_type;
            // 任务未指定剖分类型时，二维默认四边形主导
            QString shape2D = task.meshShape;
            if (shape2D.isEmpty() || (shape2D != QString("三角形") && shape2D != QString("四边形主导") && shape2D != QString("全四边形")))
            {
                shape2D = QString("四边形主导");
            }
            getMeshShape(shape2D, 2, current_type);
            for (auto ita : meshGeoId_2D)
            {
                mesh_shape_type[ita] = current_type;
            }
            double defaultsizes = 2.0;
            double defaultmeshSize = 0.0;
            //任务显式指定剖分尺寸时覆盖自动计算值
            if (task.defaultSize)
            {
                defaultmeshSize = entity->calculateDefaultMeshsizeBaseAttributeForFaceMesh(solidids, defaultsizes);
            }
            else
            {
                defaultmeshSize = task.meshSize;
            }
            hasAnyGeo = true;
            if (task.autoMesh)
            {
                //均匀剖分
                
                splitByDim(2, meshGeoId_2D, defaultmeshSize, mesh_shape_type, MeshEntity::MeshPattern::STANDRAD_MESHING);
            }           
            else
            {
                //自适应剖分
                MeshParam mesh_param;
                //邻近特征已在模型清理路径复位网格侧全局状态（resetModelScopedState），
                //批处理可安全使用任务配置
                mesh_param.proximity.enable = task.proximity;
                mesh_param.proximity.min_layer = task.proximityValue;
                mesh_param.curvature.enable = task.curvature;
                mesh_param.curvature.per_cycle_num = task.curvatureEigenValue;
                mesh_param.smoothness.enable = task.smoothness;
                mesh_param.smoothness.growth_rate = task.smoothnessRatio;
                mesh_param.max_size = task.maxSize;
                mesh_param.min_size = task.minSize;
                entity->setMeshParam(2, meshGeoId_2D, mesh_param);
                splitByDim(2, meshGeoId_2D, defaultmeshSize, mesh_shape_type, MeshEntity::MeshPattern::ADAPT_MESH);

            }

        }
        // 实体剖分仅在任务 dim == 3 时执行；dim == 2 的任务只剖面（含实体表面），
        // 避免对面任务误将实体剖成四面体
        if (mesh3DEnabled && !allSolidIds.empty())
        {
            bool sizeDiff = false;
            vector<double>defaultsizes;
            vector<int>solidids;
            for (auto ita : allSolidIds)
            {
                solidids.push_back(ita);
            }
            std::unordered_map<uint, MeshShapeType>mesh_shape_type;
            MeshShapeType current_type;
            // 任务未指定剖分类型时，三维默认四面体
            QString shape3D = task.meshShape;
            if (shape3D.isEmpty() || (shape3D != QString("四面体") && shape3D != QString("六面体")))
            {
                shape3D = QString("四面体");
            }
            getMeshShape(shape3D, 3, current_type);
            for (auto ita : allSolidIds)
            {
                mesh_shape_type[ita] = current_type;
            }
            double defaultmeshSize = 0.0;
            //任务显式指定剖分尺寸时覆盖自动计算值
            if (task.defaultSize)
            {
                defaultmeshSize = entity->calculateDefaultMeshsizeBaseAttributeForSolidMesh(solidids, defaultsizes, sizeDiff);
            }
            else
            {
                defaultmeshSize = task.meshSize;
            }
            hasAnyGeo = true;
            if (task.autoMesh)
            {
                //均匀剖分

                splitByDim(3, allSolidIds, defaultmeshSize, mesh_shape_type, MeshEntity::MeshPattern::STANDRAD_MESHING);
            }
            else
            {
                //自适应剖分
                MeshParam mesh_param;
                //邻近特征已在模型清理路径复位网格侧全局状态（同 2D 分支）
                mesh_param.proximity.enable = task.proximity;
                mesh_param.proximity.min_layer = task.proximityValue;
                mesh_param.curvature.enable = task.curvature;
                mesh_param.curvature.per_cycle_num = task.curvatureEigenValue;
                mesh_param.smoothness.enable = task.smoothness;
                mesh_param.smoothness.growth_rate = task.smoothnessRatio;
                mesh_param.max_size = task.maxSize;
                mesh_param.min_size = task.minSize;
                entity->setMeshParam(3, allSolidIds, mesh_param);
                splitByDim(3, allSolidIds, defaultmeshSize, mesh_shape_type, MeshEntity::MeshPattern::ADAPT_MESH);

            }
        }
        mGlobalSignals::getInstance()->outputMessageSig(0, tr("剖分完成"), true);
        // 仅在 GUI 模式下访问 mInfoWindow，headless 批量模式下无 PreRenderWidget
        if (getCVM->getPreRenderWidget() != nullptr)
        {
            mInfoWindow::getInstance()->isMeshing = false;
        }
        if (!hasAnyGeo)
        {
            //mGlobalSignals::getInstance()->outputMessageSig(1, tr("模型无几何部件，可能未导入"), true);
            emit done("NoGeometry");
            return;
        }
        vector<int> partIds = meshdata->getAllPartIds();
        mGlobalSignals::getInstance()->modelDataThreadStatusChangedSig(false);
        emit done("OK", partIds);
    }

	void mMeshThread::SaveMeshDataToMeshData()
	{
		//if (_mesh == nullptr)
		//{
		//	return;
		//}

		////节点信息
		//QVector<MVertex*>::iterator it;
		//for (it = _mesh->DynMeshVertex->begin(); it != _mesh->DynMeshVertex->end(); it++)
		//{
		//	int ID = (*it)->getIndex();
		//	QVector3D temp = QVector3D((*it)->x(), (*it)->y(), (*it)->z());
		//	mMeshNodeData1 *nodeData = new mMeshNodeData1(ID, temp);
		//	mTempMeshData1::getInstance()->appendNodeData(ID, nodeData);
		//}

		////临时变量
		//int elementID, index0, index1, index2, index3, index4, index5, index6, index7, index8, index9;
		//QVector<QVector3D> pQvector;
		//QVector<int> pint;
		//QVector<GEdge*>::iterator itge;
		//std::vector<MLine*>::iterator itml;
		////一维网格信息
		//if (_dim == 1)
		//{
		//	for (itge = _mesh->DynMeshEdge->begin(); itge != _mesh->DynMeshEdge->end(); itge++)
		//	{
		//		int geoLineID = (*itge)->tag();//获得几何线的编号
		//		int PartID = (*itge)->LineInGRegion;//获得几何线所在的体的编号
		//		QString PartName = _partIDName[PartID];////获得几何线所在的部件的名称
		//		mMeshPartData1 *partData = mTempMeshData1::getInstance()->getMeshPartDataByPartName(PartName);
		//		if (partData == nullptr)
		//		{
		//			partData = new mMeshPartData1(PartName, PartID);
		//			mTempMeshData1::getInstance()->appendMeshPartData(PartName, partData);

		//			//设置颜色
		//			if (MDataGeo::mGeoModelData1::getInstance()->isExistGeoPart(PartName))
		//			{
		//				QVector3D color = MDataGeo::mGeoModelData1::getInstance()->getGeoPartColor(PartName);
		//				partData->setPartColor(color);
		//			}
		//		}
		//		mMeshGeoLineData1 *geoLineData = mTempMeshData1::getInstance()->getGeoLineDataByID(geoLineID);
		//		if (geoLineData == nullptr)
		//		{
		//			partData->appendGeoLineID(geoLineID);
		//			geoLineData = new mMeshGeoLineData1(geoLineID);
		//			mTempMeshData1::getInstance()->appendGeoLineData(geoLineID, geoLineData);
		//		}
		//		for (itml = (*itge)->lines.begin(); itml != (*itge)->lines.end(); itml++)
		//		{
		//			QVector<QVector3D> tempQvector;
		//			elementID = (*itml)->getNum();//获得网格线的编号
		//			index0 = (*itml)->getVertex(0)->getIndex();
		//			index1 = (*itml)->getVertex(1)->getIndex();

		//			//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
		//			//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);

		//			mMeshData1 *meshData = new mMeshData1(elementID, MeshBeam, PartName, _meshorder);

		//			meshData->setNodeIndex(QVector<int>{index0, index1});

		//			mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

		//			geoLineData->appendMeshID(elementID);
		//		}
		//	}
		//}
		//else if (_dim == 2)
		//{
		//	//三角形网格信息
		//	QVector<GFace*>::iterator itgf;
		//	std::vector<MTriangle*>::iterator itmt;
		//	std::vector<MQuadrangle*>::iterator itmq;
		//	for (itgf = _mesh->DynMeshFace->begin(); itgf != _mesh->DynMeshFace->end(); itgf++)
		//	{
		//		int geoFaceID = (*itgf)->tag();//获得几何线的编号
		//		int PartID = (*itgf)->FaceInGRegion;//获得几何线所在的部件的编号
		//		QString PartName = _partIDName[PartID];////获得几何线所在的部件的名称
		//		mMeshPartData1 *partData = mTempMeshData1::getInstance()->getMeshPartDataByPartName(PartName);
		//		if (partData == nullptr)
		//		{
		//			partData = new mMeshPartData1(PartName, PartID);
		//			mTempMeshData1::getInstance()->appendMeshPartData(PartName, partData);

		//			//设置颜色
		//			if (MDataGeo::mGeoModelData1::getInstance()->isExistGeoPart(PartName))
		//			{
		//				QVector3D color = MDataGeo::mGeoModelData1::getInstance()->getGeoPartColor(PartName);
		//				partData->setPartColor(color);
		//			}
		//		}
		//		mMeshGeoFaceData1 *geoFaceData = mTempMeshData1::getInstance()->getGeoFaceDataByID(geoFaceID);
		//		if (geoFaceData == nullptr)
		//		{
		//			partData->appendGeoFaceID(geoFaceID);
		//			geoFaceData = new mMeshGeoFaceData1(geoFaceID);
		//			mTempMeshData1::getInstance()->appendGeoFaceData(geoFaceID, geoFaceData);
		//		}
		//		for (itmt = (*itgf)->triangles.begin(); itmt != (*itgf)->triangles.end(); itmt++)
		//		{
		//			elementID = (*itmt)->getNum();

		//			index0 = (*itmt)->getVertex(0)->getIndex();
		//			index1 = (*itmt)->getVertex(1)->getIndex();
		//			index2 = (*itmt)->getVertex(2)->getIndex();

		//			mMeshNodeData1 *nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index0);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index0);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index1);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index1);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index2);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index2);
		//			//mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
		//			//mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);
		//			//mTempMeshData1::getInstance()->getNodeDataByID(index2)->addMesh(elementID);

		//			mMeshData1 *meshData = new mMeshData1(elementID, MeshTri, PartName, _meshorder);

		//			if ((*itmt)->getOrder() == 1)
		//			{
		//				meshData->setNodeIndex(QVector<int>{index0, index1, index2});
		//			}
		//			else
		//			{
		//				index3 = (*itmt)->getVertex(3)->getIndex();
		//				index4 = (*itmt)->getVertex(4)->getIndex();
		//				index5 = (*itmt)->getVertex(5)->getIndex();
		//				meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3, index4, index5});
		//			}

		//			mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

		//			geoFaceData->appendMeshID(elementID);
		//		}
		//		for (itmq = (*itgf)->quadrangles.begin(); itmq != (*itgf)->quadrangles.end(); itmq++)
		//		{
		//			elementID = (*itmq)->getNum();

		//			index0 = (*itmq)->getVertex(0)->getIndex();
		//			index1 = (*itmq)->getVertex(1)->getIndex();
		//			index2 = (*itmq)->getVertex(2)->getIndex();
		//			index3 = (*itmq)->getVertex(3)->getIndex();
		//			mMeshNodeData1 *nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index0);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index0);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index1);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index1);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index2);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index2);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index2);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			//nodeData->addMesh(index3);
		//			/*mTempMeshData1::getInstance()->getNodeDataByID(index0)->addMesh(elementID);
		//			mTempMeshData1::getInstance()->getNodeDataByID(index1)->addMesh(elementID);
		//			mTempMeshData1::getInstance()->getNodeDataByID(index2)->addMesh(elementID);
		//			mTempMeshData1::getInstance()->getNodeDataByID(index3)->addMesh(elementID);*/

		//			mMeshData1 *meshData = new mMeshData1(elementID, MeshQuad, PartName, _meshorder);

		//			if ((*itmq)->getOrder() == 1)
		//			{
		//				meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3});
		//			}
		//			else
		//			{
		//				index4 = (*itmq)->getVertex(4)->getIndex();
		//				index5 = (*itmq)->getVertex(5)->getIndex();
		//				index6 = (*itmq)->getVertex(6)->getIndex();
		//				index7 = (*itmq)->getVertex(7)->getIndex();
		//				meshData->setNodeIndex(QVector<int>{index0, index1, index2, index3, index4, index5, index6, index7});
		//			}

		//			mTempMeshData1::getInstance()->appendMeshData(elementID, meshData);

		//			geoFaceData->appendMeshID(elementID);
		//		}
		//	}

		//	//两点线信息，存在了单元线中
		//	QVector<GEdge*>::iterator itge;
		//	std::vector<MLine*>::iterator itml;
		//	for (itge = _mesh->DynMeshEdge->begin(); itge != _mesh->DynMeshEdge->end(); itge++)
		//	{
		//		int geoLineID = (*itge)->tag();//获得几何线的编号
		//		int PartID = (*itge)->LineInGRegion;//获得几何线所在的体的编号
		//		QString PartName = _partIDName[PartID];////获得几何线所在的部件的名称
		//		mMeshPartData1 *partData = mTempMeshData1::getInstance()->getMeshPartDataByPartName(PartName);
		//		if (partData == nullptr)
		//		{
		//			partData = new mMeshPartData1(PartName, PartID);
		//			mTempMeshData1::getInstance()->appendMeshPartData(PartName, partData);
		//		}
		//		mMeshGeoLineData1 *geoLineData = mTempMeshData1::getInstance()->getGeoLineDataByID(geoLineID);
		//		if (geoLineData == nullptr)
		//		{
		//			partData->appendGeoLineID(geoLineID);
		//			geoLineData = new mMeshGeoLineData1(geoLineID);
		//			mTempMeshData1::getInstance()->appendGeoLineData(geoLineID, geoLineData);
		//		}
		//		for (itml = (*itge)->lines.begin(); itml != (*itge)->lines.end(); itml++)
		//		{
		//			int meshLineID = (*itml)->getNum();//获得单元线的编号
		//			index0 = (*itml)->getVertex(0)->getIndex();
		//			index1 = (*itml)->getVertex(1)->getIndex();


		//			mMeshNodeData1 *nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index0);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			nodeData->addMeshLine(meshLineID);
		//			nodeData = mTempMeshData1::getInstance()->getNodeDataByID(index1);
		//			if (nodeData == nullptr)
		//			{
		//				continue;
		//			}
		//			nodeData->addMeshLine(meshLineID);

		//			for (std::map<MTriangle*, int>::iterator itMTri = (*itml)->LineInTri.begin(); itMTri != (*itml)->LineInTri.end(); itMTri++)
		//			{
		//				int elementID = itMTri->first->getNum();
		//				mMeshLineData1 *meshLineData = new mMeshLineData1(meshLineID, std::set<int>{index0, index1}, elementID, itMTri->second, PartName, geoLineData->getGeoLineID());
		//				if (_meshorder == 2)
		//				{
		//					meshLineData->setMeshLineNodeIndex3((*itml)->getVertex(2)->getIndex());
		//				}
		//				mTempMeshData1::getInstance()->appendMeshLineData(meshLineID, meshLineData);
		//				geoLineData->appendMeshLineID(meshLineID);
		//				mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
		//				if (meshData != nullptr)
		//				{
		//					meshData->appendMeshLine(meshLineID);
		//				}
		//			}

		//			for (std::map<MQuadrangle*, int>::iterator itMQuad = (*itml)->LineInQuad.begin(); itMQuad != (*itml)->LineInQuad.end(); itMQuad++)
		//			{
		//				int elementID = itMQuad->first->getNum();
		//				mMeshLineData1 *meshLineData = new mMeshLineData1(meshLineID, std::set<int>{index0, index1}, elementID, itMQuad->second, PartName, geoLineData->getGeoLineID());
		//				if (_meshorder == 2)
		//				{
		//					meshLineData->setMeshLineNodeIndex3((*itml)->getVertex(2)->getIndex());
		//				}
		//				mTempMeshData1::getInstance()->appendMeshLineData(meshLineID, meshLineData);
		//				geoLineData->appendMeshLineID(meshLineID);
		//				mMeshData1 *meshData = mTempMeshData1::getInstance()->getMeshDataByID(elementID);
		//				if (meshData != nullptr)
		//				{
		//					meshData->appendMeshLine(meshLineID);
		//				}
		//			}
		//		}
		//	}

		//}

	}

	void mMeshThread::SaveTempNodeData(int i)
	{
		/*if (PreInterface::getInstance()->ClickAdjust) {
			_vMesh[i] = _mesh;
		}

		QHashIterator<int, QVector<QVector3D>> iter(_vMesh[i]->NodeMesssageOfEdge);
		while (iter.hasNext())
		{
			iter.next();
			int LineID = iter.key();
			mTempMeshData1::getInstance()->_lineNodeVertex[LineID] = iter.value();
			mTempMeshData1::getInstance()->_lineNum[LineID] = iter.value().size();
			if (!mTempMeshData1::getInstance()->_lineCenter.contains(LineID))
			{
				QVector3D tempCenter{};
				int center = mTempMeshData1::getInstance()->_lineNum.value(LineID) / 2;
				if (iter.value().size() > 1)
				{
					mTempMeshData1::getInstance()->_lineCenter[LineID] = (iter.value().at(center) + iter.value().at(center - 1)) / 2.0;
				}
				else
				{
					mTempMeshData1::getInstance()->_lineCenter[LineID] = iter.value().at(center);
				}
			}
			if (!mTempMeshData1::getInstance()->_lineBias.contains(LineID))
			{
				mTempMeshData1::getInstance()->_lineBias[LineID] = 0.00;
			}
		}*/
	}

	void mMeshThread::setSolidMeshParameter(double lcMax, int meshorder, set<int> GeoIDs,int dim)
	{
		_lcMax = lcMax;
		_meshorder = meshorder;//1
		_GeoIDs = GeoIDs;
		_dim = dim;
	}

    void mMeshThread::getAllMeshParamMap()
    {
        //获取网格子节点;
        std::vector<mMeshPanelBase*> solidDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(SolidMesh);
        std::vector<mMeshPanelBase*> faceDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(FaceMesh);
        std::vector<mMeshPanelBase*> lineDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(LineMesh);
        std::vector<mMeshPanelBase*> localDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(MxFunctions::LocalSize);
        std::vector<mMeshPanelBase*> customDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(CustomEncryption);
        std::vector<mMeshPanelBase*> autoDatas = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getAllEffectiveDataByType(AutoAdapt);
        std::vector<mMeshPanelBase*> solidAutoDatas;//网格自适应
        std::vector<mMeshPanelBase*> faceAutoDatas;
        std::vector<mMeshPanelBase*> lineAutoDatas;
        for (const auto& obj : autoDatas) {
            auto a = obj->getMeshGeo();
            if (a == 0)
                faceAutoDatas.push_back(obj);
            else if (a == 1)
                lineAutoDatas.push_back(obj);
            else if (a == 2)
                solidAutoDatas.push_back(obj);
        }
        std::unordered_map<int, std::vector<std::pair<double, MeshShapeType>>> solidMap;
        std::unordered_map<int, std::vector<std::pair<double, MeshShapeType>>> faceMap;
        std::unordered_map<int, std::vector<double>> lineMap;
        std::unordered_map<int, std::vector <double>> localMap;
        std::unordered_map<int, std::vector <MeshParam>> solidAutoMap;
        std::unordered_map<int, std::vector <MeshParam>> faceAutoMap;
        std::unordered_map<int, std::vector <MeshParam>> lineAutoMap;
        solidResultMap.clear();
        faceResultMap.clear();
        lineResultMap.clear();
        localResultMap.clear();
        solidAutoResultMap.clear();
        faceAutoResultMap.clear();
        lineAutoResultMap.clear();
        //自适应剖分-体
        for (auto solidAutoData : solidAutoDatas)
        {
            std::set<int> idSet = solidAutoData->getSetByList();
            MeshParam mesh_param(solidAutoData->getMaxSize(), solidAutoData->getMinSize(),
                solidAutoData->getSmoothChecked(), solidAutoData->getDimSmoothnessRatio(),
                solidAutoData->getGeoProimityChecked(), solidAutoData->getGeoProximityValue(),
                solidAutoData->getCurvatureChecked(), solidAutoData->getCurvatureEigenValue());
            for (int id : idSet)
            {
                solidAutoMap[id].push_back(mesh_param);
            }
        }
        getAutoResultMap(solidAutoMap,solidAutoResultMap);
        //自适应剖分-面
        for (auto faceAutoData : faceAutoDatas)
        {
            std::set<int> idSet = faceAutoData->getSetByList();
            MeshParam mesh_param(faceAutoData->getMaxSize(), faceAutoData->getMinSize(),
                faceAutoData->getSmoothChecked(), faceAutoData->getDimSmoothnessRatio(),
                faceAutoData->getGeoProimityChecked(), faceAutoData->getGeoProximityValue(),
                faceAutoData->getCurvatureChecked(), faceAutoData->getCurvatureEigenValue());
            for (int id : idSet)
            {
                faceAutoMap[id].push_back(mesh_param);
            }
        }
        getAutoResultMap(faceAutoMap, faceAutoResultMap);
        //自适应剖分-线
        for (auto lineAutoData : lineAutoDatas)
        {
            std::set<int> idSet = lineAutoData->getSetByList();
            MeshParam mesh_param(lineAutoData->getMaxSize(), lineAutoData->getMinSize(),
                lineAutoData->getSmoothChecked(), lineAutoData->getDimSmoothnessRatio(),
                lineAutoData->getGeoProimityChecked(), lineAutoData->getGeoProximityValue(),
                lineAutoData->getCurvatureChecked(), lineAutoData->getCurvatureEigenValue());
            for (int id : idSet)
            {
                lineAutoMap[id].push_back(mesh_param);
            }
        }
        getAutoResultMap(lineAutoMap, lineAutoResultMap);
        //体网格
        for (auto solidData : solidDatas)
        {
            std::set<int> idSet = solidData->getSetByList();
            double size = solidData->getMeshSize();
            QString shape = solidData->getMeshShape();
            MeshShapeType type;
            if (shape == "四面体")
                type = MeshShapeType::TET_MESH;
            else
                type = MeshShapeType::HEX_MESH;
            for (int id : idSet)
            {
                solidMap[id].push_back({ size,type });
            }
        }
        for (const auto& entry : solidMap)
        {
            int id = entry.first;
            const auto& vec = entry.second;
            if (vec.empty()) continue;  // 跳过空vector
            // 找到 double 最小的 pair
            auto minIt = std::min_element(vec.begin(), vec.end(),
                [](const std::pair<double, MeshShapeType>& a,
                    const std::pair<double, MeshShapeType>& b) {
                        return a.first < b.first;  // 比较 double
                });
            // 存入结果
            solidResultMap[id] = *minIt;
        }
        //面网格
        for (auto faceData : faceDatas)
        {
            std::set<int> idSet = faceData->getSetByList();
            double size = faceData->getMeshSize();
            QString shape = faceData->getMeshShape();
            MeshShapeType type;
            if (shape == "三角形")
                type = MeshShapeType::TRI_MESH;
            else
                type = MeshShapeType::QUAD_DOMINT_MESH;
            for (int id : idSet)
            {
                faceMap[id].push_back({ size,type });
            }
        }
        for (const auto& entry : faceMap)
        {
            int id = entry.first;
            const auto& vec = entry.second;
            if (vec.empty()) continue;  // 跳过空vector
            // 找到 double 最小的 pair
            auto minIt = std::min_element(vec.begin(), vec.end(),
                [](const std::pair<double, MeshShapeType>& a,
                    const std::pair<double, MeshShapeType>& b) {
                        return a.first < b.first;  // 比较 double
                });
            // 存入结果
            faceResultMap[id] = *minIt;
        }
        //线网格
        for (auto lineData : lineDatas)
        {
            std::set<int> idSet = lineData->getSetByList();
            double size = lineData->getMeshSize();
            for (int id : idSet)
            {
                lineMap[id].push_back(size);
            }
        }
        for (const auto& entry : lineMap)
        {
            int id = entry.first;
            const auto& vec = entry.second;
            if (vec.empty()) continue;  // 跳过空vector
            // 找到 double 最小的 pair
            auto minIt = std::min_element(vec.begin(), vec.end());
            // 存入结果
            lineResultMap[id] = *minIt;
        }
        //局部尺寸
        for (auto localData : localDatas)
        {
            std::map<int, double> idMap;
            if (localData->getCtrlMethod() == "根据尺寸")
                idMap = localData->_ids2SizesMap;
            else
                idMap = localData->_idNums2SizesMap;
            for (auto it : idMap)
            {
                localMap[it.first].push_back(it.second);
            }
        }
        for (const auto& entry : localMap)
        {
            int id = entry.first;
            const auto& vec = entry.second;
            if (vec.empty()) continue;  // 跳过空vector
            // 找到 double 最小的 pair
            auto minIt = std::min_element(vec.begin(), vec.end());
            // 存入结果
            localResultMap[id] = *minIt;
        }
    }

    void mMeshThread::getAutoResultMap(std::unordered_map<int, std::vector<MeshParam>>& autoMap, std::unordered_map<int, MeshParam>& autoResultMap)
    {
        for (const auto& entry : autoMap)
        {
            int id = entry.first;
            const auto& vec = entry.second;
            if (vec.empty()) continue;  // 跳过空vector
            std::vector<double> maxSizeVec;
            std::vector<double> minSizeVec;
            std::vector<double> smoothVec;
            std::vector<double> proVec;
            std::vector<double> curVec;
            bool smoothEnable = false;
            bool proEnable = false;
            bool curEnable = false;
            for (auto mesh_param : vec)
            {
                maxSizeVec.push_back(mesh_param.max_size);
                minSizeVec.push_back(mesh_param.min_size);
                if (mesh_param.smoothness.enable)
                    smoothEnable = true;
                if (mesh_param.proximity.enable)
                    proEnable = true;
                if (mesh_param.curvature.enable)
                    curEnable = true;
                smoothVec.push_back(mesh_param.smoothness.growth_rate);
                proVec.push_back(mesh_param.proximity.min_layer);
                curVec.push_back(mesh_param.curvature.per_cycle_num);
            }
            // 最大尺寸取最小，最小尺寸取最大，光滑、几何邻近、以及曲率的布尔值若有真则为真;找到最大的光滑、几何邻近、曲率参数;
            auto min_maxSize = std::min_element(maxSizeVec.begin(), maxSizeVec.end());
            auto max_minSize = std::max_element(minSizeVec.begin(), minSizeVec.end());
            auto max_smooth = std::max_element(smoothVec.begin(), smoothVec.end());
            auto max_pro = std::max_element(proVec.begin(), proVec.end());
            auto max_cur = std::max_element(curVec.begin(), curVec.end());
            // 存入结果
            MeshParam mesh_param(*min_maxSize, *max_minSize,
                smoothEnable, *max_smooth,
                proEnable, *max_pro,
                curEnable, *max_cur);
            autoResultMap[id] = mesh_param;
        }
    }

	void mMeshThread::setMeshParameter(double lcMax, double lcMin, bool smoothBool, bool curvatureBool, bool proximityBool, double smoothFactor, double curvatureFac, double proximityFac, int type, int dim, int meshorder, set<int> GeoIDs)
	{
		_lcMax = lcMax;
		_lcMin = lcMin;
		_smoothBool = smoothBool;
		_curvatureBool = curvatureBool;
		_proximityBool = proximityBool;
		_smoothFactor = smoothFactor;
		_curvatureFac = curvatureFac;
		_proximityFac = proximityFac;
		_type = type;//0
		_dim = dim;  //1
		_meshorder = meshorder;//1
		_GeoIDs = GeoIDs;
	}

	void mMeshThread::setMeshParameter(double lenth, int type, int dim, int meshorder, set<int> GeoIDs)
	{

		_lcMax = lenth;
		_type = type;
		_dim = dim;
		_meshorder = meshorder;
		_GeoIDs = GeoIDs;

	}
}
