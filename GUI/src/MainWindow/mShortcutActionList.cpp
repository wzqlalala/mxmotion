#include "mShortcutActionList.h"
#include "mGeoNeutralQuery.h" //【OCC摘除·S9】GN 查询
#include <cmath> //【OCC迁移·S9 恢复】std::hypot / M_PI（拖拽向量模长与角度换算）

#include "PreViewportWidget.h"
#include "InteractionCore/Tool/MoveGeometrySolidTool.h"
#include <mMeshData.h>
#include "mBrowserWindow.h"
#include "mInfoWindow.h" //【直模退出机制】ESC 清拾取退出直模（exitGeoDirectModeling）
#include "mTreeView.h"

namespace
{
    // 部件整体变换目标：对每个选中部件按 体>面>线>点 取最高维子元素 ID。
    // 体/面级变换会带动其全部子拓扑，同部件只取一个维度避免元素被重复变换；
    // 纯网格部件无 GN 子元素，四维皆空即跳过（与旧命令链行为一致）。
    // 返回按维度分组的目标（下标 0..3 对应 Solid/Face/Edge/Vertex）。
    void collectPartTransformTargets(MGeoManager::GeoManager* geoMgr,
        const std::set<int>& partIds, std::vector<std::vector<uint64_t>>& targetsByDim)
    {
        static const int dimOrder[4] = { 3, 2, 1, 0 };
        targetsByDim.assign(4, {});
        for (const auto& id : partIds)
        {
            for (int i = 0; i < 4; ++i)
            {
                std::vector<int> ids = MGeoNeutralQuery::geoIdsByPartId(geoMgr, dimOrder[i], id);
                if (!ids.empty())
                {
                    for (int gid : ids)
                    {
                        targetsByDim[i].push_back(static_cast<uint64_t>(gid));
                    }
                    break;
                }
            }
        }
    }
}

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
using namespace MxFunctions;
using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    // 全局单例指针
    static ShortcutActionList* g_shortcutAction = nullptr;
    ShortcutActionList* ShortcutActionList::instance()
    {
        if (!g_shortcutAction) {
            g_shortcutAction = new ShortcutActionList();
            g_shortcutAction->connectSignals();
        }
          
        return g_shortcutAction;
    }

    void ShortcutActionList::connectSignals()
    {
        connect(mViewportModelManager::getInstance(), &mViewportModelManager::currentViewportChanged,
            g_shortcutAction, &ShortcutActionList::slot_subWindowActivated);
    }

    void ShortcutActionList::connectPartMoveRotate()
    {
        connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
            &mxr::SignalEmitter::signal_GeometryMoveVector, g_shortcutAction,
            &ShortcutActionList::slot_GeometryMoveAction, Qt::UniqueConnection);

        connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
            &mxr::SignalEmitter::signal_GeometryRotateParam, g_shortcutAction,
            &ShortcutActionList::slot_GeometryRotateAction, Qt::UniqueConnection);
    }

    void ShortcutActionList::disconnectPartMoveRotate()
    {
        disconnect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
            &mxr::SignalEmitter::signal_GeometryMoveVector, g_shortcutAction,
            &ShortcutActionList::slot_GeometryMoveAction);

        disconnect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(),
            &mxr::SignalEmitter::signal_GeometryRotateParam, g_shortcutAction,
            &ShortcutActionList::slot_GeometryRotateAction);
    }

    // ─────────────────────────────────────────
    //  槽函数
    // ─────────────────────────────────────────
    void ShortcutActionList::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        ////几何部件移动和旋转
        //if (isGeoPartMove)
        //{
        //    vector<int> res;

        //    res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->
        //        getSelectDataByType(SelectionInfoType::GeoPart);

        //    getCVM->getPreViewportWidget()->interaction()->getToolManager()->
        //        active<mxr::InteractionMoveGeometrySolidTool>()->setGeoTranformType(res, true);

        //    if (res.size() <= 0)
        //    {
        //        ShortcutActionList::instance()->setGeoPartMove(false);
        //        getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
        //    }
        //}
    }

    void ShortcutActionList::slot_GeometryMoveAction(const float pos[3], const float moveVec[3])
    {
        //【OCC迁移·S9 恢复】旧 mGeoCommandFactory 命令链下线时本槽被整体注释，
        // 拖拽移动信号悬空导致「选部件-移动」不生效；现接回引擎变换族
        // transforms().translate（移动语义与旧链一致：选中部件整体平移、不复制）。
        // 旧命令链原体见 git 历史（S9 注释块随恢复删除）。
        Q_UNUSED(pos);
        try
        {
            //获取选中的部件
            std::vector<int> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->
                getSelectDataByType(SelectionInfoType::GeoPart);

            if (res.size() <= 0)
            {
                return;
            }

            std::vector<std::vector<uint64_t>> targetsByDim;
            collectPartTransformTargets(getCVM->getModelData()->getGeoManager(),
                std::set<int>(res.begin(), res.end()), targetsByDim);

            const double dis = std::hypot(std::hypot(moveVec[0], moveVec[1]), moveVec[2]);
            if (dis <= 0.0)
            {
                return; //拖拽向量退化（原地释放），不发起编辑
            }

            static const GeoNeutralData::GN_ShapeType dimTypes[4] = {
                GeoNeutralData::GN_ShapeType::Solid,
                GeoNeutralData::GN_ShapeType::Face,
                GeoNeutralData::GN_ShapeType::Edge,
                GeoNeutralData::GN_ShapeType::Vertex };
            bool anySucceeded = false, anyFailed = false;
            for (int i = 0; i < 4; ++i)
            {
                if (targetsByDim[i].empty())
                {
                    continue;
                }

                EventDispatcher::mGeoTranslationRequest req;
                req.objtype = dimTypes[i];
                req.isCopy = false;
                req.targets = targetsByDim[i];
                req.distance = static_cast<float>(dis);
                req.name = "OperateOfTrans";
                req.direction[0] = moveVec[0];
                req.direction[1] = moveVec[1];
                req.direction[2] = moveVec[2];

                EventDispatcher::mGeoOperationResult transRes = getGEOServer.transforms().translate(req);
                if (transRes.succeeded())
                {
                    anySucceeded = true;
                    //将编辑成功的partID记录
                    for (auto editpart : transRes.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                }
                else
                {
                    anyFailed = true;
                }
            }

            if (anySucceeded)
            {
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
            }
            else if (anyFailed)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑出错！"));
        }
    }

    void ShortcutActionList::slot_GeometryRotateAction(const float pos[3], const float rotateDir[3], const float angle)
    {
        //【OCC迁移·S9 恢复】同 slot_GeometryMoveAction：旧命令链下线时被整体注释，
        // 现接回 transforms().OperateOfRotate。交互工具侧角度为弧度，引擎接口单位为度。
        // 旧命令链原体见 git 历史（S9 注释块随恢复删除）。
        try
        {
            if (std::fabs(angle) <= 0.0)
            {
                return; //原地释放，不发起编辑
            }

            //获取选中的部件
            std::vector<int> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->
                getSelectDataByType(SelectionInfoType::GeoPart);

            if (res.size() <= 0)
            {
                return;
            }

            std::vector<std::vector<uint64_t>> targetsByDim;
            collectPartTransformTargets(getCVM->getModelData()->getGeoManager(),
                std::set<int>(res.begin(), res.end()), targetsByDim);

            static const GeoNeutralData::GN_ShapeType dimTypes[4] = {
                GeoNeutralData::GN_ShapeType::Solid,
                GeoNeutralData::GN_ShapeType::Face,
                GeoNeutralData::GN_ShapeType::Edge,
                GeoNeutralData::GN_ShapeType::Vertex };
            bool anySucceeded = false, anyFailed = false;
            for (int i = 0; i < 4; ++i)
            {
                if (targetsByDim[i].empty())
                {
                    continue;
                }

                EventDispatcher::mGeoOperateOfRotateRequest req;
                req.objtype = dimTypes[i];
                req.isCopy = false;
                req.targets = targetsByDim[i];
                req.angle = static_cast<float>(angle * 180.0 / M_PI);
                req.name = "OperateOfRotate";
                req.direction[0] = rotateDir[0];
                req.direction[1] = rotateDir[1];
                req.direction[2] = rotateDir[2];
                req.modelLocation[0] = pos[0];
                req.modelLocation[1] = pos[1];
                req.modelLocation[2] = pos[2];

                EventDispatcher::mGeoOperationResult transRes = getGEOServer.transforms().OperateOfRotate(req);
                if (transRes.succeeded())
                {
                    anySucceeded = true;
                    //将编辑成功的partID记录
                    for (auto editpart : transRes.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                }
                else
                {
                    anyFailed = true;
                }
            }

            if (anySucceeded)
            {
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
            }
            else if (anyFailed)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }


    // ─────────────────────────────────────────
    //  辅助函数
    // ─────────────────────────────────────────

   
    std::set<int> ShortcutActionList::getCurrentPickType(PickDataType &type)
    {
        std::set<int> ids;
        type = PickDataType::None;

        //获取当前已拾取的类型
        auto pointIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
        auto edgeIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
        auto faceIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
        auto solidIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
        
        if (pointIds.size() > 0)
        {
            ids.insert(pointIds.begin(), pointIds.end());
            type = PickDataType::GeoPoint;
        }
        else if (edgeIds.size() > 0)
        {
            ids.insert(edgeIds.begin(), edgeIds.end());
            type = PickDataType::GeoEdge;
        }
        else if (faceIds.size() > 0)
        {
            ids.insert(faceIds.begin(), faceIds.end());
            type = PickDataType::GeoFace;
        }
        else if (solidIds.size() > 0)
        {
            ids.insert(solidIds.begin(), solidIds.end());
            type = PickDataType::GeoSolid;
        }

        return ids;
    }

    //QList<MGeoCommand::EditShape> ShortcutActionList::createEditShape(const std::set<int>& ids, PickDataType pickType) //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
 //【OCC摘除·S10/P0-1 终局2】孤儿 EditShape 工具函数整函数下线（零调用者路径）    //{
        //QList<MGeoCommand::EditShape> mEditShapeList; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
        //if (pickType != PickDataType::GeoPoint && pickType != PickDataType::GeoEdge &&
            //pickType != PickDataType::GeoFace && pickType != PickDataType::GeoSolid)
        //{
            //return mEditShapeList;
        //}

//        //for (int t : ids)
        //{
            //GeoDataModelSet* set;
            //if (pickType == PickDataType::GeoPoint)
            //{
                //set = getCVM->getModelData()->getGeoData()->getPartSetByGeoPointID(t);
            //}
            //else if (pickType == PickDataType::GeoEdge)
            //{
                //set = getCVM->getModelData()->getGeoData()->getPartSetByGeoLineID(t);
            //}
            //else if (pickType == PickDataType::GeoFace)
            //{
                //set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(t);
            //}
            //else if (pickType == PickDataType::GeoSolid)
            //{
                //set = getCVM->getModelData()->getGeoData()->getPartSetByGeoSolidID(t);
            //}

//            //if (nullptr == set)
            //{
                //MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
                //continue;
            //}
            //MGeoCommand::EditShape mEditShape; //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
            //mEditShape.mSet = set;

//            //if (pickType == PickDataType::GeoPoint)
            //{
                //mEditShape.editPointIDs.insert(t);
            //}
            //else if (pickType == PickDataType::GeoEdge)
            //{
                //mEditShape.editLineIDs.insert(t);
            //}
            //else if (pickType == PickDataType::GeoFace)
            //{
                //mEditShape.editFaceIDs.insert(t);
            //}
            //else if (pickType == PickDataType::GeoSolid)
            //{
                //mEditShape.editSolidIDs.insert(t);
            //}
            //mEditShapeList.append(mEditShape);
        //}

//        //return mEditShapeList;
    //}


    // ─────────────────────────────────────────
    //  Geometry
    // ─────────────────────────────────────────
    void ShortcutActionList::geometryDeleteFeature()
    {
        //【OCC迁移·S9 恢复】旧 mGeoCommandFactory 命令链下线时本函数被整体注释，
        // F 键「特征去除」失效；现接回引擎编辑族 geoEdit().editOfDefeature
        //（移除所选面组并重建实体，语义与旧链一致）。原体见 git 历史。
        try
        {
            auto ids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            if (ids.size() <= 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何面！"));
                return;
            }

            EventDispatcher::mGeoDefeatureRequest req;
            req.name = "featureRemove";
            req.geoIds = ids;

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDefeature(req);
            if (res.succeeded())
            {
                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                //特征移除后部件变空时联动删除应用侧镜像（同 mGeoEditPanel 删除面/体范式）
                if (res.removedObjectIds.size() > 0)
                {
                    std::set<int> partIds;
                    for (const auto& removed : res.removedObjectIds)
                    {
                        partIds.insert(removed.id);
                    }
                    getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                }
                getCVM->clearPick();
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void ShortcutActionList::geometryDelete()
    {
        //【OCC迁移·S9 恢复】旧命令链下线时本函数被整体注释，Delete 键「删除几何体」
        // 失效；现按当前拾取类型接回 geoEdit().editOfDeleteFaces / editOfDeleteSolids
        //（参数与旧链一致：删面不连带删体；删体不保留面/网格）。原体见 git 历史。
        PickDataType pickType;
        std::set<int> ids = getCurrentPickType(pickType);

        if (pickType == PickDataType::GeoEdge)
        {
            //【删除线】拾取几何线时按 Delete 删除独立线；
            //引擎侧拒绝面/体的拓扑边（仅独立线可删）
            try
            {
                EventDispatcher::mGeoLineEditOfDeleteRequest req;
                req.name = "lineDelete";
                req.geoIds.assign(ids.begin(), ids.end());

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteEdges(req);
                if (res.succeeded())
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    //删空部件联动删除应用侧镜像（同 mGeoEditPanel 删除面/体范式）
                    if (res.removedObjectIds.size() > 0)
                    {
                        std::set<int> partIds;
                        for (const auto& removed : res.removedObjectIds)
                        {
                            partIds.insert(removed.id);
                        }
                        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                        getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                    }
                    getCVM->clearPick();
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    //拓扑边等被引擎拒绝时透出具体原因
                    QString errMsg = res.message.empty()
                        ? tr("几何编辑失败!")
                        : QString::fromStdString(res.message);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, errMsg);
                }
            }
            catch (...)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑出错！"));
            }
        }
        else if (pickType == PickDataType::GeoFace)
        {
            try
            {
                EventDispatcher::mGeoFaceEditOfDeleteRequest req;
                req.name = "faceDelete";
                req.delAssociatedSolid = false;
                req.geoIds.assign(ids.begin(), ids.end());

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteFaces(req);
                if (res.succeeded())
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    //删空部件联动删除应用侧镜像（同 mGeoEditPanel 删除面/体范式）
                    if (res.removedObjectIds.size() > 0)
                    {
                        std::set<int> partIds;
                        for (const auto& removed : res.removedObjectIds)
                        {
                            partIds.insert(removed.id);
                        }
                        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                        getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                    }
                    getCVM->clearPick();
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            catch (...)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑出错！"));
            }
        }
        else if (pickType == PickDataType::GeoSolid)
        {
            try
            {
                EventDispatcher::mGeoSolidEditOfDeleteRequest req;
                req.name = "solidDelete";
                req.reserveFace = false;
                req.reserveMesh = false;
                req.geoIds.assign(ids.begin(), ids.end());

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteSolids(req);
                if (res.succeeded())
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    //删空部件联动删除应用侧镜像（同 mGeoEditPanel 删除面/体范式）
                    if (res.removedObjectIds.size() > 0)
                    {
                        std::set<int> partIds;
                        for (const auto& removed : res.removedObjectIds)
                        {
                            partIds.insert(removed.id);
                        }
                        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                        getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                    }
                    getCVM->clearPick();
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            catch (...)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑出错！"));
            }
        }
    }


    // ─────────────────────────────────────────
    //  通用
    // ─────────────────────────────────────────
    void ShortcutActionList::save()
    {
        bool hasThread = MechIOFile::mIOFileManager::getInstance()->hasRunningThread();
        if (hasThread)
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("当前有未完成的文件操作"), true);
            return;
        }

        QString savepath = getCVM->getMxcaePath();
        //判断当前工程文件路径是否为空，若为空，则选择保存路径
        if (savepath.isEmpty())
        {
            QString workingdir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
            savepath = QFileDialog::getSaveFileName(NULL,
                "保存文件",
                workingdir,
                QString("MxSim File(*.mxcae)"));

            if (savepath.isEmpty())
            {
                return;
            }
            //Linux下原生对话框不会自动追加扩展名，需要手动补全
            if (QFileInfo(savepath).suffix().isEmpty())
            {
                savepath += ".mxcae";
            }
            getCVM->setMxcaePath(savepath);
        }

        //保存文件
        MechIOFile::mIOFileManager::getInstance()->writeMxcaeFile(savepath);
        return;
    }

    void ShortcutActionList::clearPick()
    {
        //取消移动状态
        //在某些特殊情况下可能会获取到空，导致报错，所以添加一下判断
        if (getCVM->getPreViewportWidget() &&
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->active())
        {
            mxr::InteractionToolType type =
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->active()->type();

            switch (type)
            {
            case mxr::InteractionToolType::None:
                break;
            case mxr::InteractionToolType::CreateSketch2DGeometry:
            case mxr::InteractionToolType::GeometryDividerTool:
            case mxr::InteractionToolType::MoveGeoPart:
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
                //移动状态完整退出：断开拖拽移动/旋转信号（与「移动」入口的激活路径对称）
                if (type == mxr::InteractionToolType::MoveGeoPart)
                {
                    disconnectPartMoveRotate();
                }
                break;
            case mxr::InteractionToolType::DirectGeoModeling:
                //【直模退出机制】ESC 取消拾取 = 结束直接建模会话（与面板
                // "取消"按钮同语义：清预览/箭头、复位拾取步骤），随后卸下
                // DirectTool。面板保持打开且拾取过滤维持当前步骤可直接重
                // 新拾取，故不走尾部通用 activePickSig(0) 复位。
                mInfoWindow::getInstance()->exitGeoDirectModeling();
                getCVM->getPreViewportWidget()->interaction()->getToolManager()
                    ->setActive(mxr::InteractionToolType::None);
                getCVM->clearPick();
                return;
            default:
                break;
            }

        }
            

        getCVM->clearPick();
        //MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
    }

    void ShortcutActionList::pickAllEqualGeo()
    {
        PickDataType pickType;
        std::set<int> ids = getCurrentPickType(pickType);

        if (pickType == PickDataType::GeoFace)
        {
            auto tagIds = getGEOServer.geoCalculate().getEqualAreaFaceIdByIds(ids);
            set<int> ret{};
            for (auto i : tagIds)
            {
                QString name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, i); //【OCC摘除·S9】GN 部件名
                bool isVisible = !getCVM->getModelData()->getMeshData()->getGeoPartMask(name); // True是隐藏还是False
                if (isVisible)
                    ret.insert(i);
            }

            std::vector<int> highLightFaces;
            highLightFaces.insert(highLightFaces.end(), ret.begin(), ret.end());

            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, highLightFaces);
        }
        else if (pickType == PickDataType::GeoSolid)
        {
            auto tagIds = getGEOServer.geoCalculate().getEqualVolumeSolidByIds(ids);
            set<int> ret{};
            for (auto i : tagIds)
            {
                QString name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, i); //【OCC摘除·S9】GN 部件名
                bool isVisible = !getCVM->getModelData()->getMeshData()->getGeoPartMask(name); // True是隐藏还是False
                if (isVisible)
                    ret.insert(i);
            }

            std::vector<int> highLightFaces;
            highLightFaces.insert(highLightFaces.end(), ret.begin(), ret.end());
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, highLightFaces);
        }



        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void ShortcutActionList::geometryUndo()
    {
        try
        {
            auto& history = getGEOServer.history();
            if (!history.canUndo())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("没有可撤消的几何操作！"));
                return;
            }
            const QString label = history.undoLabel();
            const auto result = history.undo();
            if (result.succeeded())
            {
                getCVM->clearPick();
                refreshAfterHistoryRestore();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("已撤消：%1").arg(label));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("撤消失败：%1").arg(QString::fromStdString(result.message)));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何撤消出错！"));
        }
    }

    void ShortcutActionList::geometryRedo()
    {
        try
        {
            auto& history = getGEOServer.history();
            if (!history.canRedo())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("没有可重做的几何操作！"));
                return;
            }
            const QString label = history.redoLabel();
            const auto result = history.redo();
            if (result.succeeded())
            {
                getCVM->clearPick();
                refreshAfterHistoryRestore();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("已重做：%1").arg(label));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("重做失败：%1").arg(QString::fromStdString(result.message)));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何重做出错！"));
        }
    }

    void ShortcutActionList::refreshAfterHistoryRestore()
    {
        // 快照恢复是整树替换：几何树、视口渲染、网格侧部件镜像数据全部按
        // 当前 PartManager 重建。镜像数据（树节点、渲染部件表、mMeshData::_partdata）
        // 必须先删后建，否则已撤消部件的残留会继续显示且占用颜色/显隐数据。
        auto* modelData = getCVM->getModelData();
        auto* partMgr = modelData->getGeoManager()->getGeoPartMgr();
        if (!partMgr)
        {
            return;
        }

        const QList<int> partList = partMgr->getPartList();
        const std::set<int> livePartIds(partList.begin(), partList.end());

        // 1) 收集镜像侧仍登记、但文档中已不存在的部件（撤消掉的），按正常删除路径清理。
        std::set<int> stalePartIds;
        if (auto* meshData = modelData->getMeshData())
        {
            for (auto it = meshData->_partdata.begin(); it != meshData->_partdata.end(); ++it)
            {
                if (!partMgr->containsPart(it.key()))
                {
                    stalePartIds.insert(it.key());
                }
            }
        }
        if (!stalePartIds.empty())
        {
            // 树节点删除：先删镜像记录再发 deletePartSig——slot_deletePartTree
            // 的删除条件是"部件镜像已不存在"，顺序反过来条件才不成立。
            if (auto* meshData = modelData->getMeshData())
            {
                for (int staleId : stalePartIds)
                {
                    if (mPartData* stale = meshData->_partdata.take(staleId))
                    {
                        delete stale;
                    }
                }
            }
            MxFunctions::mGlobalSignals::getInstance()->deletePartSig(stalePartIds);
        }

        // 2) 几何→网格侧几何对象图重绑定（收口 GeoManager::syncAllMeshEntityFromGN）：
        //    history 快照恢复只回滚引擎侧 GN 文档，MeshEntity 里"几何传给网格剖分"
        //    的 MXGeo* 对象图不进快照——被撤消部件残留旧对象，存活部件的 ref 解析、
        //    端点/环序接线、参数域与 UV 域仍是恢复前的。history 是界面调用的，
        //    成功返回即是应用侧唯一可靠收口点（引擎侧不感知应用镜像，
        //    DocumentRestored 事件当前也无订阅者），故在此重绑。
        if (auto* geoManager = modelData->getGeoManager())
        {
            geoManager->syncAllMeshEntityFromGN();
        }

        // 3) 为存活部件补齐网格侧镜像记录（与 slotCreateGeoModel_2 一致）：
        //    撤消把部件从镜像中删掉后，重做/回退到含部件的状态时必须重建，
        //    否则颜色/显隐/属性失去数据源，渲染侧 setPartVisual 等查询失效。
        if (auto* meshData = modelData->getMeshData())
        {
            auto* geoManager = modelData->getGeoManager();
            for (int liveId : livePartIds)
            {
                if (!meshData->_partdata.contains(liveId))
                {
                    const GeoNeutralData::GN_Part part = partMgr->getPartById(liveId);
                    meshData->_partdata.insert(liveId,
                        new mPartData(part.metadata().name(), liveId, 1));
                }
                //【引擎集成·颜色同步】快照恢复后把 GN 呈现颜色/透明度回读到
                // mPartData（GN 默认纯白哨兵时不覆盖轮换色板，幂等），与
                // registerCreatedPart 注册口径一致。
                if (geoManager)
                {
                    geoManager->syncPartColorFromGN(liveId);
                }
            }
        }

        // 4) 渲染侧整体重建：DeleteAllPart 清空部件数据与渲染表后，
        //    非空文档再按当前离散数据 ImportPart 重建（ImportPart 内部先 delete 再 update，
        //    对存活部件同样做到先删后建）。
        getCVM->updateAllModelOperate(mxr::ModelOperateEnum::DeleteAllPart);
        if (!livePartIds.empty())
        {
            getCVM->updateAllModelOperate(mxr::ModelOperateEnum::ImportPart);
        }

        // 5) 几何树按当前 PartManager 父子关系重建（isCreate=false 分支内部
        //    会先删旧节点再按父子关系重建；撤消掉的部件节点已在第 1 步删除）。
        MxFunctions::mGlobalSignals::getInstance()->createUpdatePartSig_2(false, livePartIds);
    }
}