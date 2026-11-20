#include "mPostBrowserWindow.h"
#include "DerivedCase/mPostDerivedCaseStore.h"
#include "mCommonFunctions.h"   // S5：G5 台账寻址助手（PostCommon）
#include "mCenterWindowProvider.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mCaseBase.h"	//mCaseBase完整类型（原经mTreeView.h传递提供，断链后自含）
#include "mTreeDelegate.h"
#include "mTreeModel.h"
#include "mTreeItemUtil.h"
#include "mPanelHostProvider.h"	//Info dock容器经IPanelHost接口解耦（原直调mInfoWindow）
#include "mMechGlobalSignals.h"
#include "mPostDockProvider.h"
#include "mPostInfoHub.h"
#include "mPostMeasureTool.h"	//测量树联动：重建后高亮查询面板编辑对象（完整类型）
#include "mPostCoordinatePanel.h"	//坐标系树联动：删除/清空前退面板编辑态（完整类型）
#include "mAppConfiguration.h"
#include "mOpenCurveDlg.h"
#include "mDockLayoutManager.h"
#include <QFileDialog>
#include <QDateTime>
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mPostParseMesh.h"
#include "mModelStruct.h"
#include "mViewTreeFunction.h"
#include "mPhysicalFieldEnum.h"

//与 mTreeView.cpp 的搬迁代码环境保持一致
using namespace MxFunctions;
using namespace MechModelData;
using namespace MechCommon;
using namespace MechPreData;
using namespace MechCaseData;
using namespace PostData;

//Qt
#include <QVBoxLayout>
#include <QGridLayout>
#include <QToolBar>
#include <QLabel>
#include <QCollator>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSet>

using namespace MxFunctions;

//部件/分组/材料/集合显隐操作族与集合子树重建（含段内匿名命名空间辅助：
//restoreShownPostPartElements/collectAllPostPartIds——仅本文件消费；
//自 mPostBrowserWindow.cpp 拆出，批次C1：同类多cpp物理拆分，零行为变化）
namespace PostWindow
{

    //////////////////////////////////////////////////////////////////////////
    // 部件单元显隐辅助（自 mTreeView 匿名命名空间移植）
    //////////////////////////////////////////////////////////////////////////
    namespace
    {
        // 集合隐藏写在单元级 cell_info.is_show 上，部件级显示不会清除。
        // 部件级"显示"操作涉及存在隐藏单元的部件时，恢复这些部件的全部单元，
        // 调用方需随后 rebuildPostElementRender() 重建渲染；仅恢复本次显示的部件，
        // 其余部件的单元级隐藏保持不变。返回是否发生了写入。
        bool restoreShownPostPartElements(const std::set<int>& parts)
        {
            auto postRend = getCVM->getPostRenderWidget();
            if (!postRend) return false;
            PostData::mPostDataBase* postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase();
            if (!postDB) return false;
            QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
            if (!postModelData) return false;

            bool restored = false;
            const QString viewName = postRend->getViewportName();
            for (int partId : parts)
            {
                QPointer<PostData::mPostPartData> partData = postModelData->getPartData(partId);
                if (partData == nullptr || partData->getMaskEleNum() == 0)
                    continue;
                QList<int> allEles;
                allEles.reserve(static_cast<int>(partData->_parteledata.cell_rid.size()));
                for (int rid : partData->_parteledata.cell_rid)
                    allEles.append(rid);
                if (!allEles.isEmpty())
                {
                    postDB->setEleVisual(viewName, partId, allEles, true);
                    restored = true;
                }
            }
            return restored;
        }

        std::set<int> collectAllPostPartIds()
        {
            PostData::mPostDataBase* postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase();
            if (!postDB) return {};
            QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
            if (!postModelData) return {};
            return postModelData->getAllPartIDs();
        }

        QStringList collectAllRigidWallNames()
        {
            PostData::mPostDataBase* postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase();
            if (!postDB) return {};
            return postDB->getAllRigidWallNames();
        }
    }

    void mPostTreeView::splitSelectedPostChildren(const QModelIndexList& idxs, std::set<int>& parts, std::set<QString>& wallNames)
    {
        QStringList allWallNames = collectAllRigidWallNames();
        for (auto idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != PostModelChildItem)
                continue;
            QString text = getTextByModelIndex(idx);
            if (allWallNames.contains(text))
                wallNames.insert(text);
            else
                parts.insert(getPhyIdByModelIndex(idx));
        }
    }

    void mPostTreeView::slot_menuShowAllPostPart()
    {
        std::set<int> allParts = collectAllPostPartIds();
        if (allParts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, allParts });
        // S5 G5：RigidWall=登记型（渲染库裁决②）——先寻/登台账 id，updateObject 空部分名=全墙操作
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::ShowAllPartOperate);   // S5 G5（D9 载体）：空 partNames=全墙操作

        //集合隐藏写在单元级 cell_info.is_show 上，部件级显示不会清除；有隐藏单元时一并恢复并重建
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        QPointer<PostData::mPostModelData> postModelData = postDB ? postDB->getModelData(1) : nullptr;
        bool hasHiddenEle = false;
        if (postModelData)
        {
            for (int partId : allParts)
            {
                QPointer<PostData::mPostPartData> partData = postModelData->getPartData(partId);
                if (partData != nullptr && partData->getMaskEleNum() > 0)
                {
                    hasHiddenEle = true;
                    break;
                }
            }
        }
        if (hasHiddenEle)
        {
            postDB->setAllEleShow(postRend->getViewportName());
            rebuildPostElementRender(collectAllPostPartIds());
            return;
        }

        postRend->updatePaint();
    }

    void mPostTreeView::slot_menuHideAllPostPart()
    {
        std::set<int> allParts = collectAllPostPartIds();
        if (allParts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        postRender->updateOneModelOperate({ mxr::HideOnePartOperate, allParts });
        // S5 G5：RigidWall=登记型（渲染库裁决②）——先寻/登台账 id，updateObject 空部分名=全墙操作
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::HideAllPartOperate);   // S5 G5（D9 载体）：空 partNames=全墙操作
        postRend->updatePaint();
    }

    void mPostTreeView::slot_menuReverseShowPostPart()
    {
        std::set<int> allParts = collectAllPostPartIds();
        if (allParts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return;
        QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
        if (!postModelData) return;

        // 反向显示：先隐藏全部，再显示当前未显示的部分
        if (!allParts.empty())
        {
            postRender->updateAllModelOperate(mxr::HideOnePartOperate);
        }
        // S5 G5：RigidWall=登记型（渲染库裁决②）——先寻/登台账 id，updateObject 空部分名=全墙操作
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::HideAllPartOperate);   // S5 G5（D9 载体）：空 partNames=全墙操作
        std::set<int> reverseParts;
        for (int id : allParts)
        {
            if (postModelData->getPartData(id) != nullptr)
                reverseParts.insert(id);
        }
        if (reverseParts.empty()) return;

        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, reverseParts });
        postRend->updatePaint();
    }

    void mPostTreeView::slot_menuShowCurrentPostPart()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;
        std::set<int> parts;
        std::set<QString> wallNames;
        splitSelectedPostChildren(idxs, parts, wallNames);
        if (parts.empty() && wallNames.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        if (!parts.empty())
            postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, parts });
        if (!wallNames.empty())
            // S5 G5：RigidWall 操作型分派（partNames 非空=单墙操作）
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::ShowOnePartOperate,
	wallNames);   // S5 G5（D9 载体）：partNames 非空=单墙操作
        if (restoreShownPostPartElements(parts))
        {
            rebuildPostElementRender(parts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::slot_menuHideCurrentPostPart()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;
        std::set<int> parts;
        std::set<QString> wallNames;
        splitSelectedPostChildren(idxs, parts, wallNames);
        if (parts.empty() && wallNames.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        if (!parts.empty())
            postRender->updateOneModelOperate({ mxr::HideOnePartOperate, parts });
        if (!wallNames.empty())
            // S5 G5：RigidWall 操作型分派（partNames 非空=单墙操作）
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::HideOnePartOperate,
	wallNames);   // S5 G5（D9 载体）：partNames 非空=单墙操作
        postRend->updatePaint();
    }

    void mPostTreeView::slot_menuHideOtherPostPart()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;
        std::set<int> selectedParts;
        std::set<QString> selectedWalls;
        splitSelectedPostChildren(idxs, selectedParts, selectedWalls);
        if (selectedParts.empty() && selectedWalls.empty()) return;

        std::set<int> allParts = collectAllPostPartIds();
        std::set<int> otherParts;
        std::set_difference(allParts.begin(), allParts.end(), selectedParts.begin(), selectedParts.end(), std::inserter(otherParts, otherParts.begin()));

        std::set<QString> otherWalls;
        for (const QString& name : collectAllRigidWallNames())
        {
            if (selectedWalls.find(name) == selectedWalls.end())
                otherWalls.insert(name);
        }

        if (otherParts.empty() && otherWalls.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        if (!otherParts.empty())
            postRender->updateOneModelOperate({ mxr::HideOnePartOperate, otherParts });
        if (!otherWalls.empty())
            // S5 G5：RigidWall 操作型分派（partNames 非空=单墙操作）
PostCommon::updateRigidWallOperate(postRender.get(), (int)mxr::HideOnePartOperate,
	otherWalls);   // S5 G5（D9 载体）：partNames 非空=单墙操作
        postRend->updatePaint();
    }

    void mPostTreeView::slot_createPostModelGroup()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        // 仅允许对后处理部件子节点创建分组
        for (auto idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != PostModelChildItem)
                return;
        }

        mPostTreeItem* groupRootItem = _treeModel->getParentItem(PostModelGroupRootItem);
        if (!groupRootItem) return;

        // 生成唯一分组名
        int index = 1;
        QString groupName;
        do
        {
            groupName = QString("分组-%1").arg(index++);
        } while (_treeModel->findPostModelGroup(groupRootItem, groupName));

        QVector<int> partIds;
        QVector<QString> partNames;
        partIds.reserve(idxs.size());
        partNames.reserve(idxs.size());
        for (auto idx : idxs)
        {
            partIds.push_back(getPhyIdByModelIndex(idx));
            partNames.push_back(getTextByModelIndex(idx));
        }

        _treeModel->addPostModelGroup(groupRootItem, groupName, partIds, partNames);
        expandItem(groupRootItem);
    }

    void mPostTreeView::slot_showPostModelGroup()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        mPostTreeItem* groupItem = nullptr;
        if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupParentItem)
        {
            groupItem = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
        }
        else if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupChildItem)
        {
            mPostTreeItem* child = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
            if (child) groupItem = child->getParentItem();
        }
        if (!groupItem) return;

        QVector<int> partIds = _treeModel->getPostModelGroupPartIds(groupItem);
        if (partIds.isEmpty()) return;

    auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        std::set<int> parts;
        for (int id : partIds) parts.insert(id);
        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, parts });
        if (restoreShownPostPartElements(parts))
        {
            rebuildPostElementRender(parts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::slot_hidePostModelGroup()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        mPostTreeItem* groupItem = nullptr;
        if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupParentItem)
        {
            groupItem = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
        }
        else if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupChildItem)
        {
            mPostTreeItem* child = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
            if (child) groupItem = child->getParentItem();
        }
        if (!groupItem) return;

        QVector<int> partIds = _treeModel->getPostModelGroupPartIds(groupItem);
        if (partIds.isEmpty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        std::set<int> parts;
        for (int id : partIds) parts.insert(id);
        postRender->updateOneModelOperate({ mxr::HideOnePartOperate, parts });
        postRend->updatePaint();
    }

    void mPostTreeView::slot_deletePostModelGroup()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()) return;

        mPostTreeItem* groupItem = nullptr;
        if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupParentItem)
        {
            groupItem = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
        }
        else if (getTreeTypeByModelIndex(idxs[0]) == PostModelGroupChildItem)
        {
            mPostTreeItem* child = _treeModel->itemFromIndex(toSourceIndex(idxs[0]));
            if (child) groupItem = child->getParentItem();
        }
        if (!groupItem) return;

        _treeModel->removePostModelGroup(groupItem);
    }

    void mPostTreeView::slot_showPostMaterialSectionParts()
    {
        std::set<int> parts = collectPostMaterialSectionPartIds();
        if (parts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, parts });
        if (restoreShownPostPartElements(parts))
        {
            rebuildPostElementRender(parts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::slot_hidePostMaterialSectionParts()
    {
        std::set<int> parts = collectPostMaterialSectionPartIds();
        if (parts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        postRender->updateOneModelOperate({ mxr::HideOnePartOperate, parts });
        postRend->updatePaint();
    }

    void mPostTreeView::slot_hideOtherPostMaterialSectionParts()
    {
        std::set<int> selectedParts = collectPostMaterialSectionPartIds();
        if (selectedParts.empty()) return;

        std::set<int> allParts = collectAllPostPartIds();
        std::set<int> otherParts;
        std::set_difference(allParts.begin(), allParts.end(), selectedParts.begin(), selectedParts.end(),
            std::inserter(otherParts, otherParts.begin()));
        if (otherParts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;
        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, selectedParts });
        postRender->updateOneModelOperate({ mxr::HideOnePartOperate, otherParts });
        if (restoreShownPostPartElements(selectedParts))
        {
            rebuildPostElementRender(selectedParts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::slot_showPostElementSet()
    {
        showPostElementSets(selectedPostSetNames(PostSetElementChildItem));
    }

    void mPostTreeView::slot_hidePostElementSet()
    {
        hidePostElementSets(selectedPostSetNames(PostSetElementChildItem));
    }

    void mPostTreeView::slot_hideOtherPostElementSet()
    {
        isolatePostElementSets(selectedPostSetNames(PostSetElementChildItem));
    }

    //节点/面/边集显隐槽已随显隐功能取消（用户定案 2026-11-12）整组移除；
    //单元集（slot_show/hide/hideOtherPostElementSet）与部件集（slot_show/hide/hideOtherPostPartSet）保留

    void mPostTreeView::slot_showPostPartSet()
    {
        showPostPartSets(selectedPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_hidePostPartSet()
    {
        hidePostPartSets(selectedPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_hideOtherPostPartSet()
    {
        isolatePostPartSets(selectedPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_showAllPostElementSet()
    {
        showPostElementSets(allPostSetNames(PostSetElementChildItem));
    }

    void mPostTreeView::slot_hideAllPostElementSet()
    {
        hidePostElementSets(allPostSetNames(PostSetElementChildItem));
    }

    void mPostTreeView::slot_showOnlyPostElementSet()
    {
        isolatePostElementSets(allPostSetNames(PostSetElementChildItem));
    }

    //节点/面/边集的"全部显隐"槽已随显隐功能取消（用户定案 2026-11-12）整组移除

    void mPostTreeView::slot_showAllPostPartSet()
    {
        showPostPartSets(allPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_hideAllPostPartSet()
    {
        hidePostPartSets(allPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_showOnlyPostPartSet()
    {
        isolatePostPartSets(allPostSetNames(PostSetPartChildItem));
    }

    void mPostTreeView::slot_deletePostSet()
    {
        //删除选中集合（五类集合子节点通用；右键分发已保证多选同类型）：
        //名字在库内五张哈希唯一，从各哈希按名移除即可；面板若正编辑被删集合由 hub 通知复位
        QStringList names;
        QModelIndexList idxs = getSelectIndex();
        for (auto idx : idxs)
        {
            MxFunctions::BrowserTreeType type = getTreeTypeByModelIndex(idx);
            if (type != PostSetElementChildItem && type != PostSetNodeChildItem
                && type != PostSetFaceChildItem && type != PostSetEdgeChildItem
                && type != PostSetPartChildItem)
                continue;
            names.append(getTextByModelIndex(idx));
        }
        if (names.isEmpty()) return;

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return;

        auto sets = postDB->getElementSets();
        for (const QString& name : names) sets.remove(name);
        postDB->setElementSets(sets);
        auto nodeSets = postDB->getNodeSets();
        for (const QString& name : names) nodeSets.remove(name);
        postDB->setNodeSets(nodeSets);
        auto faceSets = postDB->getFaceSets();
        for (const QString& name : names) faceSets.remove(name);
        postDB->setFaceSets(faceSets);
        auto edgeSets = postDB->getEdgeSets();
        for (const QString& name : names) edgeSets.remove(name);
        postDB->setEdgeSets(edgeSets);
        auto partSets = postDB->getPartSets();
        for (const QString& name : names) partSets.remove(name);
        postDB->setPartSets(partSets);

        //面板编辑中的集合被删 → 同型重开新集
        mPostInfoHub::getInstance()->notifySetsDeleted(names);
        //树集合子树按库重建（被删集合节点消失）
        MechCommon::mMechGlobalSignals::getInstance()->updatePostModelTree();
    }

    void mPostTreeView::slot_updatePostModelTree()
    {
        int modelID = getCVM->getModelData()->getModelDataID();

        //获取当前模型存储的数据
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (postDB == nullptr) return;
        QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
        if (postModelData == nullptr) return;

        // 保存后处理分组根节点下的现有分组（非持久化，在重建时保留）
        mPostTreeItem* groupRootItem = _treeModel->getParentItem(PostModelGroupRootItem);
        QHash<QString, QVector<int>> existingGroups;
        if (groupRootItem)
        {
            existingGroups = _treeModel->getPostModelGroups(groupRootItem);
            // 清空分组根节点的子节点，稍后会重新根据现有部件创建引用
            _treeModel->removeTreeChildrenItem(groupRootItem);
        }

        mPostTreeItem* partItem, * childitem;
        int phyID; vector<int> phyIDs; QString phyName;
        //部件
        QStringList partNames = postModelData->getAllPartsNames();
        QVector<mPostTreeItem*> partChildItems;
        partChildItems.reserve(partNames.size());
        partItem = _treeModel->getParentItem(PostModelParentItem);
        // 先清空旧部件子节点，避免重建时重复累积并覆盖 m_index 中的 key
        if (partItem && partItem->getChildrenSize() > 0)
        {
            _treeModel->removeTreeChildrenItem(partItem);
        }
        if (partNames.size() > 0)
        {
            for (int i = 0; i < partNames.size(); i++)
            {
                phyID = postModelData->getPartIDByPartName(partNames[i]);
                childitem = new mPostTreeItem(modelID, PostModelChildItem, partNames[i], phyID);
                partChildItems.push_back(childitem);
            }
            _treeModel->appendChildren(partItem, std::move(partChildItems));
        }

        // 重建分组：以部件 ID 和名称为匹配依据，若对应部件仍存在则重建
        for (auto it = existingGroups.begin(); it != existingGroups.end(); ++it)
        {
            QVector<int> validPartIds;
            QVector<QString> validPartNames;
            for (int partId : it.value())
            {
                // 仅保留仍存在于模型中的部件引用
                if (postModelData->getPartData(partId) != nullptr)
                {
                    validPartIds.push_back(partId);
                    validPartNames.push_back(postModelData->getPartNameByPartId(partId));
                }
            }
            if (!validPartIds.isEmpty())
                _treeModel->addPostModelGroup(groupRootItem, it.key(), validPartIds, validPartNames);
        }

        //刚性墙
        //结果库对刚性墙按名称索引、无数字ID：以"最大部件ID+1"为基址铸造合成 phyID，
        //保证与部件不撞 key（原实现误用部件循环残留的 phyID——每个墙节点都与最后一个
        //部件同 key，m_index 被 QHash::insert 静默覆盖，按 key 查部件/对墙做显隐都会错位）
        QStringList rwnames = postDB->getAllRigidWallNames();
        QVector<mPostTreeItem*> rwChildItems;
        rwChildItems.reserve(rwnames.size());
        int wallPhyID = 0;
        for (int partId : postModelData->getAllPartIDs())
        {
            if (partId + 1 > wallPhyID) wallPhyID = partId + 1;
        }
        for (QString rwname : rwnames)
        {
            childitem = new mPostTreeItem(modelID, PostModelChildItem, rwname, wallPhyID++);
            rwChildItems.push_back(childitem);
        }
        _treeModel->appendChildren(partItem, std::move(rwChildItems));

        //工况-分析工况子节点："工况"根下"分析工况"分类按结果库工况名重建
        mPostTreeItem* caseAnalysisRootItem = _treeModel->getParentItem(PostCaseAnalysisRootItem);
        if (caseAnalysisRootItem)
        {
            if (caseAnalysisRootItem->getChildrenSize() > 0)
                _treeModel->removeTreeChildrenItem(caseAnalysisRootItem);

            const QStringList caseNames = postDB->getAllCaseNames();
            QVector<mPostTreeItem*> caseChildItems;
            caseChildItems.reserve(caseNames.size());
            for (const QString& caseName : caseNames)
            {
                //工况名无数字ID：phyID 用名称记忆稳定ID（同材料/属性口径，重建不漂移）
                caseChildItems.push_back(new mPostTreeItem(modelID, PostCaseAnalysisChildItem,
                    caseName, _treeModel->stableNameItemId(caseName)));
            }
            _treeModel->appendChildren(caseAnalysisRootItem, std::move(caseChildItems));
        }

        //工况-派生工况子节点：按 store 定义集重建（批次A0；全 kind 混排，phyID=定义id）
        rebuildDerivedCaseTree(modelID);

        //测量子节点：按数据管理器测量历史重建（记录变更时另经 postMeasureRecordsChangedSig 同步）
        rebuildMeasureTree(modelID);

        //标签子节点：按标签面板会话记录重建（记录变更时另经 postTagRecordsChangedSig 同步）
        rebuildTagTree(modelID);

        //材料/属性/集合（文件级数据，读取器导入时填充；根节点为固定骨架，此处仅重建子节点）
        rebuildPostQueryTree(modelID);
    }

    //派生工况分支子节点重建（"派生工况"根 ← store 定义集）：导入完成链、store 变更
    //防抖（slot_derivedStoreChanged）、定义删除（slot_removeDerivedCaseDef）三路共用；
    //phyID=定义 id（store 进程内全局单调唯一，改名不漂移；图标暂用占位见 getBrowserTreePic）
    void mPostTreeView::rebuildDerivedCaseTree(int modelID)
    {
        mPostTreeItem* derivedRootItem = _treeModel->getParentItem(PostCaseDerivedRootItem);
        if (derivedRootItem == nullptr) return;

        if (derivedRootItem->getChildrenSize() > 0)
            _treeModel->removeTreeChildrenItem(derivedRootItem);

        const QList<DerivedCaseDefinition> defs =
            mPostDerivedCaseStore::instance().definitions(modelID);
        if (defs.isEmpty()) return;

        QVector<mPostTreeItem*> defChildItems;
        defChildItems.reserve(defs.size());
        for (const DerivedCaseDefinition& def : defs)
        {
            defChildItems.push_back(new mPostTreeItem(modelID, PostCaseDerivedChildItem,
                def.name, def.id));
        }
        _treeModel->appendChildren(derivedRootItem, std::move(defChildItems));
    }

    //store 定义集变更防抖超时：重建"派生工况"分支（构造处 addChangeListener 置键启表）
    void mPostTreeView::slot_derivedStoreChanged()
    {
        if (_pendingDerivedKey < 0) return;
        rebuildDerivedCaseTree(_pendingDerivedKey);
        _pendingDerivedKey = -1;
    }

    //测量分支子节点重建（"测量"根 ← 数据管理器测量历史）：
    //导入完成链路（slot_updatePostModelTree）与测量记录变更（slot_updateMeasureTree）共用
    void mPostTreeView::rebuildMeasureTree(int modelID)
    {
        mPostTreeItem* measureRootItem = _treeModel->getParentItem(PostMeasureRootItem);
        if (measureRootItem == nullptr) return;

        if (measureRootItem->getChildrenSize() > 0)
            _treeModel->removeTreeChildrenItem(measureRootItem);

        auto* modelData = getCVM->getModelData();
        if (modelData == nullptr) return;
        //测量历史随模型存续（面板未开过为空列表，分支保持空）
        const QList<PostData::mMeasureRecord> records =
            modelData->getManager<PostData::mPostDataManager>(Physical_Post).measureHistory();

        QVector<mPostTreeItem*> measureChildItems;
        measureChildItems.reserve(records.size());
        for (const PostData::mMeasureRecord& record : records)
        {
            //phyID = 记录id（面板内唯一），与测量工具面板一一对应；
            //隐藏记录节点置 Tree_Wrong（✗ 徽标，树显隐惯例；与面板眼睛列同源）
            measureChildItems.push_back(new mPostTreeItem(modelID, PostMeasureChildItem,
                record.name, record.id,
                record.visible ? MxFunctions::Tree_Yes : MxFunctions::Tree_Wrong));
        }
        _treeModel->appendChildren(measureRootItem, std::move(measureChildItems));

        if (measureRootItem->getChildrenSize() > 0)
            expandItem(measureRootItem);	//原型测量分支默认展开

        //重建后高亮恢复：节点全量重建丢选中，按面板当前编辑对象重新点亮
        if (auto* panel = mPostInfoHub::getInstance()->existingMeasureToolPanel())
            syncMeasureHighlight(panel->currentEditingMeasureId());
    }

    //测量记录变更（postMeasureRecordsChangedSig，测量面板镜像写回后发射）：
    //重建当前模型"测量"分支子节点
    void mPostTreeView::slot_updateMeasureTree()
    {
        if (!getCVM->isPostModel())
            return;
        auto* modelData = getCVM->getModelData();
        if (modelData == nullptr) return;
        rebuildMeasureTree(modelData->getModelDataID());
    }

    //树"测量"分支高亮同步：高亮永远跟随面板编辑对象（V1.3"延迟切换后高亮与
    //编辑对象一致"）——单击被三选一"继续编辑"拦截时调用方传回原编辑 id 还原选中；
    //recordId<0 清测量分支选中（不影响其它分支选中）
    void mPostTreeView::syncMeasureHighlight(int recordId)
    {
        mPostTreeItem* measureRootItem = _treeModel->getParentItem(PostMeasureRootItem);
        if (measureRootItem == nullptr) return;
        QItemSelectionModel* sel = selectionModel();
        if (sel == nullptr) return;

        for (auto item : measureRootItem->getChildrenItem())
        {
            const QModelIndex proxyIdx = _proxy->mapFromSource(_treeModel->indexFromItem(item));
            if (!proxyIdx.isValid()) continue;
            const bool shouldSelect = recordId >= 0 && getPhyIdByItem(item) == recordId;
            if (shouldSelect && !sel->isSelected(proxyIdx))
                sel->select(proxyIdx, QItemSelectionModel::Select | QItemSelectionModel::Current);
            else if (!shouldSelect && sel->isSelected(proxyIdx))
                sel->select(proxyIdx, QItemSelectionModel::Deselect);
        }
    }

    //面板编辑对象变化（measureEditingChangedSig）→ 测量分支高亮跟随
    void mPostTreeView::slot_measureEditingChanged(int recordId)
    {
        if (!getCVM->isPostModel())
            return;
        syncMeasureHighlight(recordId);
    }

    //标签分支子节点重建（"标签"根 ← 标签面板会话记录快照）：
    //导入完成链路（slot_updatePostModelTree）与标签记录变更（slot_updateTagTree）共用
    void mPostTreeView::rebuildTagTree(int modelID)
    {
        mPostTreeItem* tagRootItem = _treeModel->getParentItem(PostMarkerRootItem);
        if (tagRootItem == nullptr) return;

        if (tagRootItem->getChildrenSize() > 0)
            _treeModel->removeTreeChildrenItem(tagRootItem);

        //记录为标签面板会话级（未开过面板为空，分支保持空）；前处理视口创建的
        //标签同属会话记录一并上树（与面板管理表口径一致）
        const QVector<QPair<int, QString>> records = mPostInfoHub::getInstance()->tagTreeRecords();
        if (records.isEmpty()) return;

        QVector<mPostTreeItem*> tagChildItems;
        tagChildItems.reserve(records.size());
        for (const QPair<int, QString>& record : records)
        {
            //phyID = 记录id（面板内唯一），与结果标签面板管理表一一对应
            tagChildItems.push_back(new mPostTreeItem(modelID, PostMarkerChildItem,
                record.second, record.first));
        }
        _treeModel->appendChildren(tagRootItem, std::move(tagChildItems));
    }

    //标签记录变更（postTagRecordsChangedSig，标签面板记录结构变化[新建/转正/删除]后发射）：
    //重建当前模型"标签"分支子节点
    void mPostTreeView::slot_updateTagTree()
    {
        if (!getCVM->isPostModel())
            return;
        auto* modelData = getCVM->getModelData();
        if (modelData == nullptr) return;
        rebuildTagTree(modelData->getModelDataID());
    }

    /*
    * 坐标系分支子节点重建（"坐标系"根 ← 面板会话记录，hub coordinateTreeRecords 快照；
    * 后处理与前处理数据完全分离——用户裁定 2026-11-18 不复用 MechCoordinateData 管理器）：
    * 全局坐标系节点恒为首位（骨架同款 phyID=PostGlobalCoordinatePhyID，拒删拒改名），
    * 用户坐标系按记录 id 序追加（phyID=记录id，单击/右键按 id 回编面板）
    */
    void mPostTreeView::rebuildCoordinateTree(int modelID)
    {
        mPostTreeItem* coordinateRootItem = _treeModel->getParentItem(PostCoordinateRootItem);
        if (coordinateRootItem == nullptr) return;

        //全局坐标系骨架节点受 removeRows"含保护节点整批拒删"守卫保护，removeTreeChildrenItem
        //整批清空恒失败（旧写法每次重建净增一个"全局坐标系"）→ 逐个移除非保护子节点，
        //全局节点保留原实例不重建
        const QVector<mPostTreeItem*> oldChildren = coordinateRootItem->getChildrenItem();
        bool hasGlobalNode = false;
        for (mPostTreeItem* child : oldChildren)
        {
            if (getTreeTypeByItem(child) == PostCoordinateChildItem
                && getPhyIdByItem(child) == PostGlobalCoordinatePhyID)
            {
                hasGlobalNode = true;
                continue;
            }
            _treeModel->removeTreeItem(child);
        }
        if (!hasGlobalNode)   //骨架缺失才补建（正常恒存在，防呆）
            _treeModel->appendChildren(coordinateRootItem, new mPostTreeItem(modelID, PostCoordinateChildItem,
                QString::fromUtf8("全局坐标系"), PostGlobalCoordinatePhyID));

        //会话记录快照（面板未创建返回空，分支仅剩全局坐标系节点）；
        //记录暂未携带类型列，统一默认直角图（真实数据层立项补齐类型列后恢复三图分图）
        const QVector<QPair<int, QString>> records =
            mPostInfoHub::getInstance()->coordinateTreeRecords();
        for (const auto& rec : records)
        {
            _treeModel->appendChildren(coordinateRootItem, new mPostTreeItem(modelID, PostCoordinateChildItem,
                rec.second, rec.first, MxFunctions::Tree_Yes,
                ":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg"));
        }
    }

    //坐标系数据变更（createUpdateCoordinateSig：面板创建/重命名、菜单删除/复制后发射；
    //isCreate/coordID 参数按测量族口径不参与重建判定，全量按管理器重建）：
    //重建当前模型"坐标系"分支子节点
    void mPostTreeView::slot_updatePostCoordinateTree()
    {
        if (!getCVM->isPostModel())
            return;
        auto* modelData = getCVM->getModelData();
        if (modelData == nullptr) return;
        rebuildCoordinateTree(modelData->getModelDataID());
    }

    //右键"编辑"：用户坐标系节点回编（hub editCoordinate 打开面板回填；同单击语义）
    void mPostTreeView::slot_editCoordinateNode()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != PostCoordinateChildItem)
            return;
        const int phyID = getPhyIdByModelIndex(idxs[0]);
        if (phyID == PostGlobalCoordinatePhyID)
            return;
        mPostInfoHub::getInstance()->editCoordinate(phyID);
    }

    //右键"删除"：会话记录删除选中坐标系（面板内退编辑态+发信号重建，单一数据源）
    void mPostTreeView::slot_removeCoordinateNode()
    {
        //先收集 phyID（删除触发树重建会使选中索引失效，不可边遍历边删）
        QList<int> coordIds;
        for (const QModelIndex& idx : getSelectIndex())
        {
            if (getTreeTypeByModelIndex(idx) == PostCoordinateChildItem
                && getPhyIdByModelIndex(idx) != PostGlobalCoordinatePhyID)
                coordIds.append(getPhyIdByModelIndex(idx));
        }
        if (coordIds.isEmpty())
            return;

        mPostInfoHub::getInstance()->deleteCoordinateRecords(coordIds);

        mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("已删除 %1 个坐标系").arg(coordIds.size()), true);
    }

    //右键"复制"：会话记录拷贝（名称带_副本后缀/类型保留；面板内发信号重建）
    void mPostTreeView::slot_copyCoordinateNode()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != PostCoordinateChildItem)
            return;
        const int phyID = getPhyIdByModelIndex(idxs[0]);
        if (phyID == PostGlobalCoordinatePhyID)
            return;

        mPostInfoHub* hub = mPostInfoHub::getInstance();
        const int newId = hub->copyCoordinateRecord(phyID);
        if (newId < 0)
            return;

        const auto records = hub->coordinateTreeRecords();
        QString newName = QString::number(newId);
        for (const auto& rec : records)
        {
            if (rec.first == newId)
            {
                newName = rec.second;
                break;
            }
        }
        mGlobalSignals::getInstance()->outputMessageSig(
            QString::fromUtf8("已复制坐标系 %1").arg(newName), true);
    }

    //右键"创建坐标系"：走模型页三入口同款链路（含结果守卫；默认直角坐标系）
    void mPostTreeView::slot_menuCoordinateNew()
    {
        slot_createPostCoordinate(0);
    }

    //右键"删除所有坐标系"：清空会话记录（面板内退编辑+发信号重建；全局节点树骨架自有）
    void mPostTreeView::slot_menuCoordinateDeleteAll()
    {
        if (mPostInfoHub::getInstance()->coordinateTreeRecords().isEmpty())
            return;
        mPostInfoHub::getInstance()->clearCoordinateRecords();
        mGlobalSignals::getInstance()->outputMessageSig(QString::fromUtf8("已删除所有用户坐标系！"), true);
    }

    void mPostTreeView::slot_updatePostBrowserTreeStatus(MxFunctions::BrowserTreeType type, int phyID, int caseID, MxFunctions::BrowserTreeStatus statu)
    {
        //独立后处理树仅在后处理模型上下文有效（前处理工况根分支随树独立不可达，已删）
        if (!getCVM->isPostModel())
        {
            return;
        }

        //结果类节点统一挂"云图"根（PostCloudRootPhyID）——"结果"根已随 2D 类
        //去节点化移除（曲线/报告/路径不再有树节点），仅扫描云图根
        mPostTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, PostCloudRootPhyID);

        if (parentItem == nullptr)
            return;

        for (auto item : parentItem->getChildrenItem())
        {
            if (getTreeTypeByItem(item) == type && getPhyIdByItem(item) == phyID && statu != getStatusByItem(item))
                setTreeItemStatus(item, statu);
        }
    }

    void mPostTreeView::rebuildPostQueryTree(int modelID)
    {
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (postDB == nullptr) return;
        QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
        if (postModelData == nullptr) return;

        //材料/属性（截面数据）：名 -> 部件ID列表；排序保证树节点顺序稳定
        auto sortedKeys = [](const QHash<QString, QVector<int>>& data)
        {
            QStringList keys = data.keys();
            std::sort(keys.begin(), keys.end());
            return keys;
        };

        //材料/属性根为固定骨架（addPostModelTree 创建）：此处仅重建子节点，无数据时保持空根
        auto rebuildPartGroupTree = [&](MxFunctions::BrowserTreeType rootType, MxFunctions::BrowserTreeType childType,
            MxFunctions::BrowserTreeType partType, const QHash<QString, QVector<int>>& nameParts)
        {
            mPostTreeItem* rootItem = _treeModel->getParentItem(rootType);
            if (rootItem == nullptr) return;
            if (rootItem->getChildrenSize() > 0)
            {
                _treeModel->removeTreeChildrenItem(rootItem);
            }

            for (const QString& name : sortedKeys(nameParts))
            {
                const QVector<int>& partIds = nameParts.value(name);
                //名称节点 phyID 用名称记忆稳定ID（批次B1）：重建顺序变化不漂移，
                //视口选中记忆按 key 存取不受重建影响（原用遍历序号）
                mPostTreeItem* nameItem = new mPostTreeItem(modelID, childType, name, _treeModel->stableNameItemId(name));
                _treeModel->appendChildren(rootItem, nameItem);

                QVector<mPostTreeItem*> partChildItems;
                partChildItems.reserve(partIds.size());
                for (int partId : partIds)
                {
                    QString partName = postModelData->getPartNameByPartId(partId);
                    partChildItems.push_back(new mPostTreeItem(modelID, partType, partName, partId));
                }
                _treeModel->appendChildren(nameItem, std::move(partChildItems));
            }
        };

        rebuildPartGroupTree(PostMaterialRootItem, PostMaterialChildItem, PostMaterialPartItem, postDB->getMaterialParts());
        rebuildPartGroupTree(PostSectionRootItem, PostSectionChildItem, PostSectionPartItem, postDB->getSectionParts());

        //集合根为固定骨架：分类子根按数据重建（空分类不显示），无数据时保持空根
        mPostTreeItem* setRootItem = _treeModel->getParentItem(PostSetRootItem);
        if (setRootItem == nullptr) return;
        if (setRootItem->getChildrenSize() > 0)
        {
            _treeModel->removeTreeChildrenItem(setRootItem);
        }

        QHash<QString, std::vector<int>> elementSets = postDB->getElementSets();
        QHash<QString, std::vector<int>> nodeSets = postDB->getNodeSets();
        QHash<QString, std::vector<std::pair<int, int>>> faceSets = postDB->getFaceSets();
        QHash<QString, std::vector<std::pair<int, int>>> edgeSets = postDB->getEdgeSets();

        auto rebuildSetTree = [&](MxFunctions::BrowserTreeType subRootType, MxFunctions::BrowserTreeType childType,
            const QString& subRootName, const auto& nameIds)
        {
            if (nameIds.isEmpty()) return;

            mPostTreeItem* subRootItem = new mPostTreeItem(modelID, subRootType, subRootName);
            _treeModel->appendChildren(setRootItem, subRootItem);

            QStringList names = nameIds.keys();
            std::sort(names.begin(), names.end());
            QVector<mPostTreeItem*> setChildItems;
            setChildItems.reserve(names.size());
            for (const QString& name : names)
            {
                //名称记忆稳定ID（批次B1）：集合按名称索引无数字ID，原用重建序号
                //会在重建顺序变化时使 key 漂移（视口选中记忆失效）
                setChildItems.push_back(new mPostTreeItem(modelID, childType, name, _treeModel->stableNameItemId(name)));
            }
            _treeModel->appendChildren(subRootItem, std::move(setChildItems));
        };

        rebuildSetTree(PostSetElementRootItem, PostSetElementChildItem, "单元集合", elementSets);
        rebuildSetTree(PostSetNodeRootItem, PostSetNodeChildItem, "节点集合", nodeSets);
        rebuildSetTree(PostSetFaceRootItem, PostSetFaceChildItem, "面集合", faceSets);
        rebuildSetTree(PostSetEdgeRootItem, PostSetEdgeChildItem, "边集合", edgeSets);
        //部件集合：仅 GUI 面板创建（mxdb 集合块无该类型，不随文件导入）
        rebuildSetTree(PostSetPartRootItem, PostSetPartChildItem, "部件集合", postDB->getPartSets());
    }

    std::set<int> mPostTreeView::collectPostMaterialSectionPartIds()
    {
        std::set<int> partIds;
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return partIds;
        QHash<QString, QVector<int>> materialParts = postDB->getMaterialParts();
        QHash<QString, QVector<int>> sectionParts = postDB->getSectionParts();

        QModelIndexList idxs = getSelectIndex();
        for (auto idx : idxs)
        {
            MxFunctions::BrowserTreeType type = getTreeTypeByModelIndex(idx);
            if (type == PostMaterialChildItem || type == PostSectionChildItem)
            {
                //材料名/属性名（截面名）节点按文本从数据库反查部件列表
                QString name = getTextByModelIndex(idx);
                const QHash<QString, QVector<int>>& nameParts =
                    (type == PostMaterialChildItem) ? materialParts : sectionParts;
                for (int partId : nameParts.value(name))
                    partIds.insert(partId);
            }
            else if (type == PostMaterialPartItem || type == PostSectionPartItem)
            {
                partIds.insert(getPhyIdByModelIndex(idx));
            }
        }
        return partIds;
    }

    QStringList mPostTreeView::selectedPostSetNames(BrowserTreeType childType)
    {
        QStringList names;
        QModelIndexList idxs = getSelectIndex();
        for (auto idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) != childType) continue;
            names.append(getTextByModelIndex(idx));
        }
        return names;
    }

    QStringList mPostTreeView::allPostSetNames(BrowserTreeType childType)
    {
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return QStringList();
        switch (childType)
        {
        case PostSetElementChildItem: return postDB->getElementSets().keys();
        case PostSetNodeChildItem:    return postDB->getNodeSets().keys();
        case PostSetFaceChildItem:    return postDB->getFaceSets().keys();
        case PostSetEdgeChildItem:    return postDB->getEdgeSets().keys();
        case PostSetPartChildItem:    return postDB->getPartSets().keys();
        default:                      return QStringList();
        }
    }

    QHash<int, QList<int>> mPostTreeView::collectPostElementSetEleIds(const QStringList& setNames)
    {
        QHash<int, QList<int>> partEles;
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return partEles;
        QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
        if (!postModelData) return partEles;

        QHash<QString, std::vector<int>> elementSets = postDB->getElementSets();
        for (const QString& setName : setNames)
        {
            if (!elementSets.contains(setName)) continue;
            for (int eleId : elementSets.value(setName))
            {
                int partId = postModelData->getPartIDByEleID(eleId);
                if (partId < 0) continue;
                partEles[partId].append(eleId);
            }
        }
        return partEles;
    }

    void mPostTreeView::showPostElementSets(const QStringList& setNames)
    {
        QHash<int, QList<int>> partEles = collectPostElementSetEleIds(setNames);
        if (partEles.isEmpty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        //集合所在部件若处于部件级隐藏（如部件"隐藏全部"），恢复部件显示后仅显示集合单元：
        //先把隐藏部件的全部单元设为隐藏，再恢复部件显示，最后由下方仅显示集合单元
        auto frameProxy = postRender->frameProxy(postRender->currentFrame());
        auto queryProxy = frameProxy ? frameProxy->getRendQueryDataProxy() : nullptr;
        std::set<int> hiddenPartIds;
        if (queryProxy != nullptr)
        {
            for (auto it = partEles.constBegin(); it != partEles.constEnd(); ++it)
            {
                if (!queryProxy->getPartVisible(it.key()))
                    hiddenPartIds.insert(it.key());
            }
        }
        if (!hiddenPartIds.empty())
        {
            mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
            QPointer<PostData::mPostModelData> postModelData = postDB ? postDB->getModelData(1) : nullptr;
            QString viewName = postRend->getViewportName();
            for (int partId : hiddenPartIds)
            {
                QPointer<PostData::mPostPartData> partData = postModelData ? postModelData->getPartData(partId) : nullptr;
                if (partData == nullptr) continue;
                QList<int> allEles;
                allEles.reserve(static_cast<int>(partData->_parteledata.cell_rid.size()));
                for (int rid : partData->_parteledata.cell_rid)
                    allEles.append(rid);
                if (!allEles.isEmpty())
                    postDB->setEleVisual(viewName, partId, allEles, false);
            }
            postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, hiddenPartIds });
        }

        applyPostElementVisual(partEles, true);
        //受影响部件=集合所在部件∪被整包隐藏后恢复的部件（两类部件的单元显隐都变了）
        std::set<int> affected = hiddenPartIds;
        for (auto it = partEles.constBegin(); it != partEles.constEnd(); ++it)
            affected.insert(it.key());
        rebuildPostElementRender(affected);
    }

    void mPostTreeView::hidePostElementSets(const QStringList& setNames)
    {
        QHash<int, QList<int>> partEles = collectPostElementSetEleIds(setNames);
        if (partEles.isEmpty()) return;
        applyPostElementVisual(partEles, false);
        std::set<int> affected;
        for (auto it = partEles.constBegin(); it != partEles.constEnd(); ++it)
            affected.insert(it.key());
        rebuildPostElementRender(affected);
    }

    void mPostTreeView::isolatePostElementSets(const QStringList& setNames)
    {
        QHash<int, QList<int>> setEles = collectPostElementSetEleIds(setNames);
        if (setEles.isEmpty()) return;

        //仅显示集合内单元：先隐藏全部单元，再显示集合单元
        std::set<int> allParts = collectAllPostPartIds();
        if (allParts.empty()) return;
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        QPointer<PostData::mPostModelData> postModelData = postDB ? postDB->getModelData(1) : nullptr;
        if (!postModelData) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;

        QString viewName = postRend->getViewportName();
        for (int partId : allParts)
        {
            QPointer<PostData::mPostPartData> partData = postModelData->getPartData(partId);
            if (!partData) continue;
            QList<int> allEles;
            allEles.reserve(static_cast<int>(partData->_parteledata.cell_rid.size()));
            for (int rid : partData->_parteledata.cell_rid)
                allEles.append(rid);
            if (!allEles.isEmpty())
                postDB->setEleVisual(viewName, partId, allEles, false);
        }
        applyPostElementVisual(setEles, true);
        //全部件单元显隐都变了：整模型重分类（边界/内部按当前显隐重算，空洞消除）
        rebuildPostElementRender(allParts);
    }

    void mPostTreeView::rebuildPostElementRender(const std::set<int>& reclassifyParts)
    {
        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return;
        mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
        if (guidata == nullptr) return;
        QPointer<PostData::mPostModelData> modelData = postDB->getModelData(guidata->_cloudFrame);
        if (!modelData) return;

        /*
        * 受影响部件面拓扑重分类（修：单元集隐藏后模型出现空洞）：
        * 边界/内部面分类在 ParseMesh 时按当时单元显隐生成——单元显隐变化后不重建，
        * 被藏单元的邻接面仍是"内部面"（渲染只画边界面表）→ 空洞。
        * ParseMesh 的 faceBuilt 守卫（防多帧共享部件重复构建的内存保护）会跳过已建
        * 部件，故先清受影响部件的面数据使其重新收录；_curHiddenFaces/_viewFaceDatamap/
        * _viewEdgeDatamap 按 (cid,局部面号) 键跨拓扑重建稳定（classifyAndWriteFaces 末尾
        * refreshFaceIsShow 回放），不清。默认空集=仅重建渲染不改拓扑（部件级显隐路径）。
        */
        if (!reclassifyParts.empty())
        {
            for (int partId : reclassifyParts)
            {
                QPointer<PostData::mPostPartData> part = modelData->getPartData(partId);
                if (!part) continue;
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
        }

        mPostParseMesh* parse = new mPostParseMesh(this);
        parse->ParseMesh(modelData);

        postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy()->RebuildModelData();
        postRender->updateAllModelOperate(mxr::ImportOperate);
        postRender->updateAllModelOperate(mxr::UpdateMinMax);
        //update() 只是异步调度，GL 后端要等下一次事件才重绘；updatePaint 立即渲染
        postRend->updatePaint();

        delete parse;
        parse = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // mPostBrowserWindow
    //////////////////////////////////////////////////////////////////////////
    mPostBrowserWindow* mPostBrowserWindow::_instance = nullptr;

    //////////////////////////////////////////////////////////////////////////
    // 节点/面/边集显隐实现已随显隐功能取消（用户定案 2026-11-12）整组移除：
    // 面/边级隐藏在新渲染器无消费链（渲染库不读 face_is_show/hidden_edge_pairs），
    // 节点点显示机制随之一并下线；单元/部件集显隐保留（单元级/部件级通道工作正常）
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // 部件集合显隐（GUI 面板创建；mxdb 集合块无部件集类型，库内 _partSets 仅有会话数据）
    // 部件级显隐走 Show/HideOnePartOperate，显示时恢复集合部件被单元级隐藏的单元
    //////////////////////////////////////////////////////////////////////////
    namespace
    {
        std::set<int> collectPostPartSetIds(const QStringList& setNames)
        {
            std::set<int> partIds;
            mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
            if (!postDB) return partIds;

            QHash<QString, QVector<int>> partSets = postDB->getPartSets();
            for (const QString& setName : setNames)
            {
                for (int partId : partSets.value(setName))
                    partIds.insert(partId);
            }
            return partIds;
        }
    }

    void mPostTreeView::showPostPartSets(const QStringList& setNames)
    {
        std::set<int> parts = collectPostPartSetIds(setNames);
        if (parts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, parts });
        //部件显示不清除单元级隐藏（cell_info.is_show），集合部件有隐藏单元时一并恢复
        if (restoreShownPostPartElements(parts))
        {
            rebuildPostElementRender(parts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::hidePostPartSets(const QStringList& setNames)
    {
        std::set<int> parts = collectPostPartSetIds(setNames);
        if (parts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        postRender->updateOneModelOperate({ mxr::HideOnePartOperate, parts });
        postRend->updatePaint();
    }

    void mPostTreeView::isolatePostPartSets(const QStringList& setNames)
    {
        //仅显示集合内部件：先隐藏全部部件，再显示集合部件（并恢复其单元级隐藏）
        std::set<int> setParts = collectPostPartSetIds(setNames);
        if (setParts.empty()) return;

        std::set<int> allParts = collectAllPostPartIds();
        if (allParts.empty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;
        auto postRender = postRend->getPostRender();
        if (!postRender) return;

        postRender->updateAllModelOperate(mxr::HideOnePartOperate);
        postRender->updateOneModelOperate({ mxr::ShowOnePartOperate, setParts });
        if (restoreShownPostPartElements(setParts))
        {
            rebuildPostElementRender(setParts);
            return;
        }
        postRend->updatePaint();
    }

    void mPostTreeView::applyPostElementVisual(const QHash<int, QList<int>>& partEles, bool isShow)
    {
        if (partEles.isEmpty()) return;

        auto postRend = getCVM->getPostRenderWidget();
        if (!postRend) return;

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase();
        if (!postDB) return;

        QString viewName = postRend->getViewportName();
        for (auto it = partEles.constBegin(); it != partEles.constEnd(); ++it)
        {
            postDB->setEleVisual(viewName, it.key(), it.value(), isShow);
        }
    }
}
