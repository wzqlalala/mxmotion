#include "mPostBrowserWindow.h"
#include "DerivedCase/mPostDerivedCaseStore.h"
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
#include "mPostMeasureTool.h"	//测量树联动：右键显隐/删除经面板公开接口（完整类型）
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
#include "mPostGUIData.h"
#include "mPost3DResult.h"
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

//创建/删除/树更新槽族（Ribbon 菜单信号与面板完成信号的接收端；
//自 mPostBrowserWindow.cpp 拆出，批次C1：同类多cpp物理拆分，零行为变化）
namespace PostWindow
{
    void mPostTreeView::slot_switchCloudPanel()
    {
        //仅切换到云图参数面板，不新建云图子节点（新建入口在"云图"根右键菜单的"云图"）。
        //视口选择优先级：当前激活的后处理视口（面板跟随现状，不切换视口）→ 当前选中的
        //云图节点 → "云图"根下首个云图 → 导入时的初始视口（无则忽略）
        QString viewport;
        if (auto* curPriv = VMManager->getCurrentViewportPrivate())
        {
            if (getCVM->isPostModel() && curPriv->getViewportType() == MxFunctions::ViewportPost3D)
            {
                viewport = curPriv->getViewportName();
            }
        }
        if (viewport.isEmpty())
        {
            QModelIndexList idxs = getSelectIndex();
            if (idxs.size() == 1 && getTreeTypeByModelIndex(idxs[0]) == threeDResultParentItem
                && getPhyIdByModelIndex(idxs[0]) >= 0)
            {
                viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByModelIndex(idxs[0]));
            }
        }
        if (viewport.isEmpty())
        {
            mPostTreeItem* cloudRootItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
            if (cloudRootItem != nullptr && cloudRootItem->getChildrenSize() > 0)
            {
                //首个"真实"云图（缓存节点仅参数快照无独立视口，viewName 为空自然落空，
                //显式跳过以免误停在其后无真实云图时提前返回）
                for (mPostTreeItem* child : cloudRootItem->getChildrenItem())
                {
                    if (isCloudPresetNode(child))
                        continue;
                    viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, mTreeItemUtil::phyIdOfItem(_treeModel, child));
                    if (!viewport.isEmpty())
                        break;
                }
            }
        }
        if (viewport.isEmpty())
        {
            //尚无云图节点时回退到导入时创建的初始视口（该视口无树节点，仅承载面板编辑）。
            //初始视口可能已被显示名接管（"模型名· 云图"），经结构定位取当前名
            viewport = solePost3DViewportName(getCVM->getModelData()->getModelDataID());
            if (viewport.isEmpty() || getCVM->getGUIDataByViewName(viewport) == nullptr) return;
        }
        if (!viewport.isEmpty())
            mPostInfoHub::getInstance()->edit3DResult(viewport);
    }

    //slot_create3DResult 随"新建云图入口取消"移除（用户定案 2026-11-04：云图实例只由
    //结果导入与"缓存云图"产生）；hub create3DResult 保留（导入链/mUiShotController 使用）

    int mPostTreeView::contextCaseId()
    {
        //宽松工况上下文的单一收口点（原爆炸视图/截面力/标注三槽各自的回退逻辑收编）：
        //结果父节点取其工况，结果子节点（云图等）取父节点工况，其余（含无选中）
        //按默认上下文 -1；云图根容器哨兵（-2）归一（数据注册在 -1 下）
        int caseId = -1;
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 1)
        {
            MxFunctions::BrowserTreeType type = getTreeTypeByModelIndex(idxs[0]);
            if (type == ResultParentItem)
                caseId = getPhyIdByModelIndex(idxs[0]);
            else if (type == threeDResultParentItem && idxs[0].parent().isValid())
                caseId = getPhyIdByModelIndex(idxs[0].parent());
        }
        return normalizeCaseId(caseId);
    }

    void mPostTreeView::slot_createCurve()
    {
        //功能区按钮以激活视口/选中上下文为准（不要求选中"结果"根）：
        //面板打开即经 createCurveSig 建节点并回绑 _curViewName（见 slot_updatePostCurveTree）
        int caseId = contextCaseId();
        PostWindow::mPostInfoHub::getInstance()->createCurve(caseId);
    }

    void mPostTreeView::slot_createExportWord()
    {
        //同曲线：宽松上下文打开报告面板，建节点经 createExportWordSig（slot_updatePostExportWordTree）
        int caseId = contextCaseId();
        PostWindow::mPostInfoHub::getInstance()->createExportWord(caseId);
    }

    void mPostTreeView::slot_createParrayMimage()
    {
        int caseId = contextCaseId();
        PostWindow::mPostInfoHub::getInstance()->createParrayMimage(caseId);
    }

    void mPostTreeView::slot_createCompVideo()
    {
        int caseId = contextCaseId();
        PostWindow::mPostInfoHub::getInstance()->createCompVideo(caseId);
    }

    void mPostTreeView::slot_createExplode()
    {
        //爆炸视图面板作用于当前后处理视口，工况仅作上下文记录：宽松取选中上下文
        mPostInfoHub::getInstance()->createExplode(contextCaseId());
    }

    void mPostTreeView::slot_createMeasureTool()
    {
        int caseId = contextCaseId();
        mPostInfoHub::getInstance()->createMeasureTool(caseId);
    }

    void mPostTreeView::slot_createResultPath()
    {
        int caseId = contextCaseId();
        mPostInfoHub::getInstance()->createResultPath(caseId);
    }

    void mPostTreeView::slot_createSectionForce()
    {
        //截面力面板作用于当前后处理视口，工况仅作上下文记录：宽松取选中上下文
        mPostInfoHub::getInstance()->createSectionForce(contextCaseId());
    }

    void mPostTreeView::slot_createLabel()
    {
        //标注面板作用于当前视口（applyToCurrentViewport），caseId仅作上下文记录：
        //宽松取选中上下文（见 contextCaseId），导入后自动选中云图子节点时首次点击同样有效
        mPostInfoHub::getInstance()->createNote(contextCaseId());
    }

    void mPostTreeView::slot_createTag()
    {
        //结果标签面板作用于当前后处理视口，工况仅作上下文记录：宽松取选中上下文
        mPostInfoHub::getInstance()->createTagPanel(contextCaseId());
    }

    mPostTreeItem* mPostTreeView::resolveActiveCloudNode()
    {
        //激活视口→源云图节点解析（用户定案 2026-11-04：派生功能绑定激活视口，不要求
        //选中云图节点）。优先级：① 激活 Post3D 视口按名反查节点——云图节点直接命中，
        //动画等派生节点命中后取其父=源云图；② 树当前选中的云图节点（右键菜单路径经此
        //保持直觉——右键会先把选中收敛到被右键行）；③ 云图根下首个云图（slot_switchCloudPanel
        //同款回退）。无任何云图返回 nullptr，调用方提示
        if (auto* curPriv = VMManager->getCurrentViewportPrivate())
        {
            if (curPriv->getViewportType() == MxFunctions::ViewportPost3D)
            {
                if (mPostTreeItem* node = findNodeByViewportName(curPriv->getViewportName()))
                {
                    if (getTreeTypeByItem(node) == threeDResultParentItem)
                        return node;
                    mPostTreeItem* parentNode = node->getParentItem();
                    if (parentNode != nullptr && getTreeTypeByItem(parentNode) == threeDResultParentItem)
                        return parentNode;   //派生节点（动画/切面等）→源云图
                }
            }
        }

        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 1 && getTreeTypeByModelIndex(idxs[0]) == threeDResultParentItem)
        {
            if (mPostTreeItem* selected = getTreeItemByIndex(idxs[0]);
                selected != nullptr && !isCloudPresetNode(selected))
            {
                return selected;
            }
        }

        mPostTreeItem* cloudRootItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
        if (cloudRootItem != nullptr)
        {
            for (auto* child : cloudRootItem->getChildrenItem())
            {
                //缓存节点（仅参数快照、无独立视口）不是可绑定的渲染源，跳过
                if (getTreeTypeByItem(child) == threeDResultParentItem && !isCloudPresetNode(child))
                    return child;
            }
        }
        return nullptr;
    }

    QString mPostTreeView::resolveActiveCloudViewName()
    {
        //源云图视口名解析（派生功能源定位的单一收口，2026-11-04 二修去收编副作用）：
        //有云图节点→条目视口名；无云图节点（刚导入、只有裸初始视口在显示结果）→
        //唯一 Post3D 视口自身即源（不建云图节点）；两者皆无返回空串（调用方提示）
        if (mPostTreeItem* cloudItem = resolveActiveCloudNode())
        {
            return getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
                .getViewNameByID(PostData::CloudItem, getPhyIdByItem(cloudItem));
        }
        return solePost3DViewportName(getCVM->getModelData()->getModelDataID());
    }

    void mPostTreeView::slot_createAnimation(int type)
    {
        //激活视口绑定（用户定案 2026-11-04）：源云图经 resolveActiveCloudNode 解析
        //（激活视口→选中云图→首个云图），不要求选中云图节点；建节点收尾时同链重解析
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createAnimation(type, contextCaseId());
    }

    void mPostTreeView::slot_createLinearSuperposition()
    {
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createLinearSuperposition(contextCaseId());
    }

    void mPostTreeView::slot_createScalarOperation()
    {
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createScalarOperation(contextCaseId());
    }

    void mPostTreeView::slot_createSection()
    {
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createSection(contextCaseId());
    }

    void mPostTreeView::slot_createIsosurface()
    {
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createIsosurface(contextCaseId());
    }

    void mPostTreeView::slot_createVector()
    {
        //判断当前云图变量是否能够生成矢量图（源=激活视口解析出的云图视口名）
        const QString srcViewName = resolveActiveCloudViewName();
        if (srcViewName.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        bool res = mPostInfoHub::getInstance()->haveVector(srcViewName);

        if (res)
            mPostInfoHub::getInstance()->createVector(contextCaseId());
        else
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前云图变量不支持生成矢量图！", true);
    }

    void mPostTreeView::slot_createStreamLine()
    {
        //判断当前云图变量是否能够生成流线图（源=激活视口解析出的云图视口名）
        const QString srcViewName = resolveActiveCloudViewName();
        if (srcViewName.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        bool res = mPostInfoHub::getInstance()->haveStreamLine(srcViewName);

        if (res)
            mPostInfoHub::getInstance()->createStreamLine(contextCaseId());
        else
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前云图变量不支持生成流线图！", true);
    }

    void mPostTreeView::slot_createPartition()
    {
        //分区显示去节点化（用户定案 2026-11-04）：直接对当前激活视口应用/取消分区着色，
        //不再建树节点——重复点击切换开/关（原"点击其它节点退出"随节点消失改为按钮自切换，
        //退出逻辑同款迁移到 _partitionOverrideActive 旗标，见 slot_singleClickItem 尾部）
        auto* curPriv = VMManager->getCurrentViewportPrivate();
        if (curPriv == nullptr || curPriv->getViewportType() != MxFunctions::ViewportPost3D)
        {
            return;
        }

        if (_partitionOverrideActive)
        {
            const auto& r = getCVM->getPostRenderWidget()->getPostRender();
            if (r)
            {
                r->setElementColorOverrideEnabled(false);
            }
            _partitionOverrideActive = false;
            mGlobalSignals::getInstance()->outputMessageSig("已取消分区显示！");
            return;
        }

        int caseId = contextCaseId();
        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(caseId);
        if (postDB == nullptr) return;
        QPointer<PostData::mPostModelData> postModelData = postDB->getModelData(1);
        if (postModelData == nullptr) return;

        if (postModelData->_partitionElements.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前模型不存在分区数据！", true);
            return;
        }

        updatePartitionElementColor(postModelData->_partitionElements);
        _partitionOverrideActive = true;

        mGlobalSignals::getInstance()->outputMessageSig("分区显示已应用，再次点击取消！");
    }

    void mPostTreeView::slot_createPostSet()
    {
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        mPostInfoHub::getInstance()->createSet(contextCaseId());
    }

    void mPostTreeView::slot_createPostSetTyped(int setType)
    {
        //模型页按类型创建集合（V1.2原型"模型"页集合组）：与slot_createPostSet同守卫；
        //按钮序与集合面板类型序一致（0部件集/1节点集/2单元边集/3单元面集/4单元集），
        //面板（对齐前处理集合面板）按类型直接新建，恒等透传
        static const int kPanelTypeByButton[5] = { 0, 1, 2, 3, 4 };
        const int panelType = (setType >= 0 && setType < 5) ? kPanelTypeByButton[setType] : -1;

        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果或缓存云图！", true);
            return;
        }
        if (panelType < 0)
            return;

        mPostInfoHub::getInstance()->createSet(contextCaseId(), panelType);
    }

    void mPostTreeView::slot_createPostCoordinate(int coordType)
    {
        //模型页创建后处理坐标系（V1.2原型"模型"页坐标系组）：本期界面搭建——打开坐标系
        //参数面板按类型预设；数据落库/树子节点/轴符号渲染为后续功能接入（链路已在
        //面板内预留）。coordType 0直角/1柱/2球（对应MxFunctions::CoordinateType）
        if (resolveActiveCloudViewName().isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "当前无可用云图，请先导入结果！", true);
            return;
        }
        mPostInfoHub::getInstance()->createCoordinate(coordType);
    }

    void mPostTreeView::slot_createLine()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != NoteParentItem)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "请先在树中选中一个注释节点！", true);
            return;
        }

        int caseId = getPhyIdByModelIndex(idxs[0].parent());

        mPostInfoHub::getInstance()->createLine(caseId);
    }

    void mPostTreeView::slot_createArrow()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != NoteParentItem)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "请先在树中选中一个注释节点！", true);
            return;
        }

        int caseId = getPhyIdByModelIndex(idxs[0].parent());

        mPostInfoHub::getInstance()->createArrow(caseId);
    }

    void mPostTreeView::slot_createText()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != NoteParentItem)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "请先在树中选中一个注释节点！", true);
            return;
        }

        int caseId = getPhyIdByModelIndex(idxs[0].parent());

        mPostInfoHub::getInstance()->createText(caseId);
    }

    void mPostTreeView::slot_createLog()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != NoteParentItem)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "请先在树中选中一个注释节点！", true);
            return;
        }

        int caseId = getPhyIdByModelIndex(idxs[0].parent());

        mPostInfoHub::getInstance()->createLog(caseId);
    }

    void mPostTreeView::slot_createPath()
    {
        //曲线面板"创建路径"按钮入口：宽松取选中上下文（原守卫要求选中曲线节点，
        //去节点化后曲线节点不存在；路径面板打开即自登记新实例，见 mPostPath::newPathPanel）
        int caseId = contextCaseId();
        mPostInfoHub::getInstance()->createPath(caseId);
    }

    void mPostTreeView::slot_menu3DResultDelete()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0)  return;

        //记录被删节点所属模型：删除云图视口后激活可能漂移到前处理视口，
        //左侧树随工作模式切换成前处理模型树（表现为"整棵模型树被删除"），
        //删除完成后需显式切回该后处理模型并激活其剩余云图视口钉住上下文
        int nodeModelID = -1;
        bool viewportDeleted = false;
        //被删云图视口名下的缓存快照名：快照随源视口 guidata 消亡，对应缓存节点级联清理
        QStringList dyingPresetNames;

        disconnectSig();
        mPostTreeItem* parentItem = nullptr;
        QModelIndexList modelIndexs;
        for (auto& idx : idxs)
        {
            if (getTreeTypeByModelIndex(idx) == threeDResultParentItem)
            {
                nodeModelID = idx.data(MechPreData::Roles::ModelRole).toInt();

                //获取结果父节点
                parentItem = getTreeItemByIndex(idx.parent());   //多选跨父安全：取当前项父节点

                QString viewport = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByModelIndex(idx));

                //删除节点信息
                QString itemName = getTextByModelIndex(idx);
                getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).deleteItem(ItemType::CloudItem, itemName);

                modelIndexs.push_back(idx);
                //缓存节点（viewName 空）无独立视口：仅同步清理 guidata 中的参数快照
                if (viewport.isEmpty())
                    syncCloudPresetName(itemName, QString());
                else
                {
                    if (MechPreData::mPostGUIData* gui = getCVM->getGUIDataByViewName(viewport))
                    {
                        for (const MechPreData::CloudPresetSnapshot& s : gui->_cloudPresets)
                            dyingPresetNames << s._presetName;
                    }
                    mViewTreeFunction::deletePostViewport(viewport);
                    viewportDeleted = true;
                }
            }
        }
        connectSig();

        //钉回后处理上下文：切回被删节点所属模型并激活其导入初始视口
        //（无云图节点也常驻；激活后工作模式/左侧树保持在后处理）。
        //仅删缓存节点（无视口死亡）时跳过：当前激活视口仍有效，强制钉回反而跳视口
        if (viewportDeleted && nodeModelID > 0 && VMManager->isPostModel(nodeModelID))
        {
            VMManager->setCurrentViewportModel(nodeModelID);
            auto* vm = VMManager->getViewportModel(nodeModelID);
            if (vm != nullptr && vm->getViewportIDByName(mViewportName::initialCloud(nodeModelID)) >= 0)
            {
                mViewTreeFunction::activatePostViewport(mViewportName::initialCloud(nodeModelID));
            }
            else
            {
                //初始视口已被首个云图收编改名（显示名接管）：激活该模型任一
                //后处理视口钉住上下文，避免按旧协议名激活落空导致漂移回前处理
                VMManager->activatePostViewport();
            }
        }

        if (parentItem == nullptr) return;

        removeProxyIndexs(modelIndexs);

        //级联清理亡故缓存节点：源云图视口已删，其名下快照随 guidata 消亡，缓存节点
        //不再可恢复（单击只会报"已失效"），随源一并移除（条目+节点）
        if (!dyingPresetNames.isEmpty())
        {
            mPostTreeItem* cloudRoot = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
            if (cloudRoot != nullptr)
            {
                const auto cacheNodes = cloudRoot->getChildrenItem();
                for (mPostTreeItem* child : cacheNodes)
                {
                    if (!isCloudPresetNode(child)) continue;
                    const QString text = mTreeItemUtil::textOfItem(_treeModel, child);
                    if (!dyingPresetNames.contains(text)) continue;
                    getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).deleteItem(ItemType::CloudItem, text);
                    _treeModel->removeTreeItem(child);
                }
            }
        }
        //删除数据(分析工况下的结果)：仅前处理树的工况上下文（父节点 phyID=工况ID>=0）
        //才级联删结果库；后处理树父节点为"云图/结果"容器根（phyID 为 -2/-1），不得触发
        if (parentItem->getChildrenSize() == 0 && getPhyIdByItem(parentItem) >= 0
            && getTreeTypeByItem(parentItem->getParentItem()) != PostTreeRootItem)
            getCVM->getModelData()->deleteByPhysicalType(Physical_Post, getPhyIdByItem(parentItem));

        if (parentItem->getChildrenSize() > 0)
        {
            slot_itemSelectionChanged(QItemSelection(), QItemSelection());
        }
        else
        {
            _treeModel->setCurrentModelID(getCVM->getModelData()->getModelDataID());
            if (getCVM->isPostModel())
            {
                selectedItem(_treeModel->getParentItem(PostTreeRootItem));
            }
            //非后处理模型（视口已漂移到前处理）：后处理树无前处理根（ModelRootItem）
            //可选中，原兜底恒为 nullptr 空操作已删——清选中交由视口激活链处理
        }
    }

    void mPostTreeView::slot_menu3DResultUpdate()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        mPostTreeItem* parentItem = getTreeItemByIndex(idxs[0]);

        //读取更新数据
        QStringList updateViews;
        for (auto& item : parentItem->getChildrenItem())
        {
            if (getStatusByItem(item) != MxFunctions::Tree_Yes)
            {
                QString viewport = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(item));
                updateViews.push_back(viewport);
                setTreeItemStatus(item, Tree_Yes);
            }
        }
        //云图根（phyID=PostCloudRootPhyID）为容器节点，工况上下文按 -1 传递
        int caseId = getPhyIdByItem(parentItem);
        caseId = normalizeCaseId(caseId);   //云图根哨兵归一（-2→-1）
        mPostInfoHub::getInstance()->update3DResult(caseId, updateViews);

        selectedItem(parentItem);
    }

    
    
    
    //slot_menuAnimationDelete/LinearDelete/ScalarOperationDelete/SectionDelete/IsoDelete/VectorDelete
//随派生去节点化移除（用户定案 2026-11-04）：派生节点不存在，删除经视图树删派生视口
//slot_menuCurveDelete/slot_menuExportWordDelete/slot_menuPathDelete 随去节点化移除：
    //曲线/Word报告/路径不再建树节点，删除入口在各参数面板"已建列表"旁

    
    
    
    //slot_updatePostCurveTree/slot_updatePostExportWordTree/slot_updatePostPathTree 随去节点化
    //移除：实例登记与面板绑定由各面板 newXxxPanel 自完成（mPostCurve/mPostExportWord/mPostPath），
    //createCurveSig/createExportWordSig/createPathSig 不再有接收端

    void mPostTreeView::slot_updateScalarOperationTree()
    {
        //标量运算去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、
        //不登记条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：
        //云图节点条目视口名，无节点时裸初始视口自身即源），生成结果经
        //slot_generateScalar 按 _curViewName 作用到源视口渲染
        //（getCloudViewName 无 _Scalar 后缀回退自身=源，全链兼容）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editScalarOperation(resViewName);
    }

    void mPostTreeView::slot_updateLineTree()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        if (getTreeTypeByModelIndex(idxs[0]) != NoteParentItem) return;
        mPostTreeItem* parentItem = _treeModel->getParentItem(NoteParentItem, getPhyIdByModelIndex(idxs[0]));

        //命名取数据层ID（getItemId 唯一递增不重用）：用户改名不影响后续编号、
        //删除后新建跳号不复用（原按兄弟节点文本 split("_") 扫描，改名即污染计数）
        int lineID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::LineItem);
        const QString treeName = QString("二维线_%1").arg(lineID);

        //获取结果父节点
        auto resultParentItem = _treeModel->getParentItem(ResultParentItem, getPhyIdByModelIndex(idxs[0].parent()));

        //工况ID（云图子项挂"云图"根，其 phyID 为容器标识而非工况，按 -1 归一）
        int caseID = getPhyIdByItem(resultParentItem);
        caseID = normalizeCaseId(caseID);   //云图根哨兵归一（-2→-1）

        //获取结果ID
        QString resName = getTextByItem(parentItem);
        int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).get3DResultItemId(caseID, resName);

        //云图父节点视图名称
        QString resViewName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(parentItem));

        //创建视图
        QString viewport = mViewportName::derived(resViewName, QStringLiteral("Line"), lineID);

        //存储节点信息
        LineInfo info;
        info.caseId = caseID;
        info.resultId = resultID;
        info.itemName = treeName;
        info.viewName = viewport;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::LineItem, info);

        mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), LineChildItem, treeName, lineID);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mPostTreeView::slot_updateArrowTree()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        if (getTreeTypeByModelIndex(idxs[0]) != NoteParentItem) return;
        mPostTreeItem* parentItem = _treeModel->getParentItem(NoteParentItem, getPhyIdByModelIndex(idxs[0]));

        //命名取数据层ID（getItemId 唯一递增不重用）：用户改名不影响后续编号、
        //删除后新建跳号不复用（原按兄弟节点文本 split("_") 扫描，改名即污染计数）
        int arrowID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::ArrowItem);
        const QString treeName = QString("箭头_%1").arg(arrowID);

        //获取结果父节点
        auto resultParentItem = _treeModel->getParentItem(ResultParentItem, getPhyIdByModelIndex(idxs[0].parent()));

        //工况ID（云图子项挂"云图"根，其 phyID 为容器标识而非工况，按 -1 归一）
        int caseID = getPhyIdByItem(resultParentItem);
        caseID = normalizeCaseId(caseID);   //云图根哨兵归一（-2→-1）

        //获取结果ID
        QString resName = getTextByItem(parentItem);
        int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).get3DResultItemId(caseID, resName);

        //云图父节点视图名称
        QString resViewName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(parentItem));

        //创建视图
        QString viewport = mViewportName::derived(resViewName, QStringLiteral("Arrow"), arrowID);

        //存储节点信息
        ArrowInfo info;
        info.caseId = caseID;
        info.resultId = resultID;
        info.itemName = treeName;
        info.viewName = viewport;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::ArrowItem, info);

        mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), ArrowChildItem, treeName, arrowID);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mPostTreeView::slot_updateTextTree()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        if (getTreeTypeByModelIndex(idxs[0]) != NoteParentItem) return;
        mPostTreeItem* parentItem = _treeModel->getParentItem(NoteParentItem, getPhyIdByModelIndex(idxs[0]));

        //命名取数据层ID（getItemId 唯一递增不重用）：用户改名不影响后续编号、
        //删除后新建跳号不复用（原按兄弟节点文本 split("_") 扫描，改名即污染计数）
        int textID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::TextItem);
        const QString treeName = QString("文本_%1").arg(textID);

        //获取结果父节点
        auto resultParentItem = _treeModel->getParentItem(ResultParentItem, getPhyIdByModelIndex(idxs[0].parent()));

        //工况ID（云图子项挂"云图"根，其 phyID 为容器标识而非工况，按 -1 归一）
        int caseID = getPhyIdByItem(resultParentItem);
        caseID = normalizeCaseId(caseID);   //云图根哨兵归一（-2→-1）

        //获取结果ID
        QString resName = getTextByItem(parentItem);
        int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).get3DResultItemId(caseID, resName);

        //云图父节点视图名称
        QString resViewName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(parentItem));

        //创建视图
        QString viewport = mViewportName::derived(resViewName, QStringLiteral("Text"), textID);

        //存储节点信息
        TextInfo info;
        info.caseId = caseID;
        info.resultId = resultID;
        info.itemName = treeName;
        info.viewName = viewport;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::TextItem, info);

        mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), TextChildItem, treeName, textID);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mPostTreeView::slot_updateLogTree()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1)  return;

        if (getTreeTypeByModelIndex(idxs[0]) != NoteParentItem) return;
        mPostTreeItem* parentItem = _treeModel->getParentItem(NoteParentItem, getPhyIdByModelIndex(idxs[0]));

        //命名取数据层ID（getItemId 唯一递增不重用）：用户改名不影响后续编号、
        //删除后新建跳号不复用（原按兄弟节点文本 split("_") 扫描，改名即污染计数）
        int logID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getItemId(ItemType::LogItem);
        const QString treeName = QString("Log_%1").arg(logID);

        //获取结果父节点
        auto resultParentItem = _treeModel->getParentItem(ResultParentItem, getPhyIdByModelIndex(idxs[0].parent()));

        //工况ID（云图子项挂"云图"根，其 phyID 为容器标识而非工况，按 -1 归一）
        int caseID = getPhyIdByItem(resultParentItem);
        caseID = normalizeCaseId(caseID);   //云图根哨兵归一（-2→-1）

        //获取结果ID
        QString resName = getTextByItem(parentItem);
        int resultID = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).get3DResultItemId(caseID, resName);

        //云图父节点视图名称
        QString resViewName = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, getPhyIdByItem(parentItem));

        //创建视图
        QString viewport = mViewportName::derived(resViewName, QStringLiteral("Log"), logID);

        //存储节点信息
        LogInfo info;
        info.caseId = caseID;
        info.resultId = resultID;
        info.itemName = treeName;
        info.viewName = viewport;
        getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).addItem(ItemType::LogItem, info);

        mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), LogChildItem, treeName, logID);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mPostTreeView::slot_updateAnimationTree(int type)
    {
        //动画去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editAnimation(resViewName);
    }

    void mPostTreeView::slot_updateLinearSuperpositionTree()
    {
        //线性叠加去派生视口（用户定案 2026-11-04 二修）：不建视口、不登记条目、不建节点
        //——面板直接绑定源云图视口（resolveActiveCloudViewName：云图节点条目视口名，
        //无节点时裸初始视口自身即源），生成结果经 slot_generateLinear 按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Linear 后缀回退自身=源，全链兼容）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editLinear(resViewName);
    }

    void mPostTreeView::slot_updateSectionTree()
    {
        //切面图去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editSection(resViewName);
    }

    void mPostTreeView::slot_updateIsosurfaceTree()
    {
        //等值面图去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editIsosurface(resViewName);
    }

    void mPostTreeView::slot_updateCloudPresetTree(QString viewName, int presetIndex)
    {
        //保存云图缓存（用户定案 2026-11-05 去视口化）：仅缓存界面参数快照，在"云图"
        //根下登记缓存节点（CloudInfo.viewName 留空＝缓存标记）；不创建视口、不回放
        //重绘、不打扰当前面板编辑状态。恢复入口＝单击缓存节点（restoreCloudPreset：
        //回填+重绘）与面板"应用渲染"按钮（按回填后的面板参数重绘）
        if (viewName.isEmpty() || presetIndex < 0) return;

        //快照登记在源云图 guidata 中，作为单击恢复的数据源
        MechPreData::mPostGUIData* srcGui = getCVM->getGUIDataByViewName(viewName);
        if (srcGui == nullptr || presetIndex >= srcGui->_cloudPresets.size()) return;

        mPostTreeItem* parentItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
        if (parentItem == nullptr) return;

        //命名：变量_分量（无分量时为 变量_无分量，变量缺失回退"云图"）；
        //与"云图"根下既有子节点名查重，同名追加序号（_2、_3…）
        QSet<QString> usedNames;
        for (auto& item : parentItem->getChildrenItem())
        {
            usedNames.insert(mTreeItemUtil::textOfItem(_treeModel, item));
        }
        const MechPreData::CloudPresetSnapshot& snap = srcGui->_cloudPresets.at(presetIndex);
        QString baseName;
        if (!snap._cloudVarName.isEmpty() && !snap._cloudCompo.isEmpty())
            baseName = QString("%1_%2").arg(snap._cloudVarName).arg(snap._cloudCompo);
        else if (!snap._cloudVarName.isEmpty())
            baseName = QString("%1_无分量").arg(snap._cloudVarName);
        else
            baseName = QString("云图");
        QString treeName = baseName;
        for (int suffix = 2; usedNames.contains(treeName); ++suffix)
        {
            treeName = QString("%1_%2").arg(baseName).arg(suffix);
        }
        //快照登记最终节点名（缓存名与树上节点一致，restoreCloudPreset 按名反查快照）
        srcGui->_cloudPresets[presetIndex]._presetName = treeName;

        //登记缓存条目并挂树节点：getItemId 先取（peek）再由 addItem 消耗（同一配方，
        //见 updateResultTree 右键创建路径），节点 phyID 用消耗后的 id 与真实云图同序；
        //viewName 留空＝缓存标记（无独立视口）：getViewNameByID 按此区分缓存/云图，
        //会话采集 collectCloudItems 按 viewName 非空过滤，缓存节点不进 .mxses
        int modelID = getCVM->getModelData()->getModelDataID();
        int resultID = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getItemId(PostData::CloudItem);
        PostData::CloudInfo info;
        info.caseId = -1;
        info.itemName = treeName;
        info.viewName = QString();
        info.resultId = resultID;
        getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).addItem(PostData::CloudItem, info);

        mPostTreeItem* childitem = new mPostTreeItem(modelID, MxFunctions::threeDResultParentItem, treeName, resultID);
        _treeModel->appendChildren(parentItem, childitem);
        //不选中新节点、不切面板：缓存是静默登记，当前编辑状态与激活视口均不变
    }

    bool mPostTreeView::isCloudPresetNode(mPostTreeItem* item)
    {
        //缓存节点判定：云图类型节点 + 条目 viewName 为空（slot_updateCloudPresetTree
        //登记缓存时留空作标记；真实云图条目 viewName 恒非空）
        if (item == nullptr || getTreeTypeByItem(item) != MxFunctions::threeDResultParentItem)
            return false;
        return getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
            .getViewNameByID(PostData::CloudItem, getPhyIdByItem(item)).isEmpty();
    }

    int mPostTreeView::realCloudChildCount(mPostTreeItem* cloudRootItem)
    {
        if (cloudRootItem == nullptr)
            return 0;
        int count = 0;
        for (auto* child : cloudRootItem->getChildrenItem())
        {
            if (getTreeTypeByItem(child) == MxFunctions::threeDResultParentItem && !isCloudPresetNode(child))
                ++count;
        }
        return count;
    }

    void mPostTreeView::restoreCloudPreset(const QString& itemName)
    {
        //单击缓存节点恢复（去视口化后的回放端）：按节点名在当前模型各 Post3D 视口的
        //guidata._cloudPresets 中定位快照，回放到目标云图视口并经 edit3DResult 绑定
        //重绘（链尾 setCurViewName 内 updateResultInfoGUI+drawCloudMap，语义同旧
        //"云图缓存"恢复的后半段：回填+重绘）；此后"应用渲染"按同一面板参数可再重绘
        if (itemName.isEmpty()) return;

        //① 定位快照与所属视口（快照仍登记在缓存时的源云图 guidata 中）
        MechPreData::CloudPresetSnapshot snapshot;
        MechPreData::mPostGUIData* srcGui = nullptr;
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        if (vm == nullptr) return;
        for (int id : vm->getAllViewportIDs())
        {
            QWidget* sub = vm->getSubWindowByViewportID(id);
            auto* priv = sub ? vm->getViewportPrivate(sub) : nullptr;
            if (priv == nullptr || priv->getViewportType() != MxFunctions::ViewportPost3D)
                continue;
            MechPreData::mPostGUIData* gui = getCVM->getGUIDataByViewName(priv->getViewportName());
            if (gui == nullptr) continue;
            for (const MechPreData::CloudPresetSnapshot& s : gui->_cloudPresets)
            {
                if (s._presetName == itemName)
                {
                    snapshot = s;
                    srcGui = gui;
                    break;
                }
            }
            if (srcGui != nullptr) break;
        }
        if (srcGui == nullptr)
        {
            mGlobalSignals::getInstance()->outputMessageSig(QString("云图缓存已失效，无法恢复！"), true);
            return;
        }

        //② 回放目标：当前激活上下文的源云图视口（resolveActiveCloudViewName：激活
        //Post3D 视口反查云图节点，动画等派生视口取其源云图；缓存节点选中态在
        //resolveActiveCloudNode 内已跳过）；空则退回快照所属源视口
        QString target = resolveActiveCloudViewName();
        REBUILD_PostRend* postRend = target.isEmpty() ? nullptr
            : getCVM->getPostRenderByViewName(target);
        if (postRend == nullptr)
        {
            //源视口兜底：快照登记在它的 guidata 上，遍历定位其当前视口名
            for (int id : vm->getAllViewportIDs())
            {
                QWidget* sub = vm->getSubWindowByViewportID(id);
                auto* priv = sub ? vm->getViewportPrivate(sub) : nullptr;
                if (priv == nullptr || priv->getViewportType() != MxFunctions::ViewportPost3D)
                    continue;
                if (getCVM->getGUIDataByViewName(priv->getViewportName()) == srcGui)
                {
                    target = priv->getViewportName();
                    postRend = getCVM->getPostRenderByViewName(target);
                    break;
                }
            }
            if (postRend == nullptr)
            {
                mGlobalSignals::getInstance()->outputMessageSig(QString("无可用云图视口，无法恢复云图缓存！"), true);
                return;
            }
        }

        mPostDataBase* postDB = getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post)
            .getPostDatabase(postRend->getPostDataName());
        MechPreData::mPostGUIData* gui = getCVM->getGUIDataByViewName(target);
        if (gui == nullptr || postDB == nullptr) return;

        //③ 回放快照（与保存端 saveCloudPresetSnapshot 字段一一对应）：候选列表自源
        //云图同步（目标视口可能从未绑定过面板，缺失则回填后面板下拉为空）
        gui->_cloudCases = srcGui->_cloudCases;
        gui->_cloudVarNames = srcGui->_cloudVarNames;
        gui->_cloudCompos = srcGui->_cloudCompos;
        gui->_cloudLayerNames = srcGui->_cloudLayerNames;
        gui->_cloudHaveComplex = srcGui->_cloudHaveComplex;
        gui->_cloudHaveLayer = srcGui->_cloudHaveLayer;
        gui->_cloudFrames = postDB->getCaseFrameNames(snapshot._cloudCase);
        gui->_cloudShortFrames = postDB->getCaseFrameShortNames(snapshot._cloudCase);
        gui->_cloudCase = snapshot._cloudCase;
        gui->_cloudVarName = snapshot._cloudVarName;
        gui->_cloudCompo = snapshot._cloudCompo;
        gui->_cloudLayerName = snapshot._cloudLayerName;
        gui->_cloudComplex = snapshot._cloudComplex;
        gui->_smoothMethod = snapshot._smoothMethod;
        gui->_smoothAcrossParts = snapshot._smoothAcrossParts;
        gui->_smoothVariationEnabled = snapshot._smoothVariationEnabled;
        gui->_smoothVariationLimit = snapshot._smoothVariationLimit;
        gui->_rangeIndex = (snapshot._rangeMode == QString("显示部件")) ? 1 : 0;
        gui->_cloudMaxValue = snapshot._cloudMaxValue;
        gui->_cloudMinValue = snapshot._cloudMinValue;
        gui->_cloudDiscrete = snapshot._cloudDiscrete;
        gui->_cloudDiscreteNum = snapshot._cloudDiscreteNum;
        gui->_cloudIsEquivariance = snapshot._cloudIsEquivariance;
        int frameIndex = gui->_cloudFrames.indexOf(snapshot._cloudFrame);
        if (frameIndex >= 0) gui->_cloudFrameIndex = frameIndex;
        //帧名→帧号（换算同 slot_showCloud：取":"前的帧名按工况查帧号）
        QString fName = snapshot._cloudFrame.contains(":") ? snapshot._cloudFrame.split(":").at(0) : snapshot._cloudFrame;
        int fi = postDB->getFrameNumByCaseAndFrameName(snapshot._cloudCase, fName);
        if (fi > 0) gui->_cloudFrame = fi;
        //——对象及坐标系——
        gui->_cloudCoordinate = snapshot._cloudCoordinate;
        gui->_currentCoordinateName = snapshot._currentCoordinateName;
        gui->_renderCoordSystem = snapshot._renderCoordSystem;
        //——图例与显隐标注——
        gui->_legendRange = snapshot._legendRange;
        gui->_cloudShowMax = snapshot._cloudShowMax;
        gui->_cloudShowMin = snapshot._cloudShowMin;
        gui->_cloudShowMaxLine = snapshot._cloudShowMaxLine;
        gui->_cloudShowMinLine = snapshot._cloudShowMinLine;
        //回放重绘即完成一次渲染，清除按钮按已应用态显示（setCurViewName 内消费）
        gui->_cloudRenderApplied = true;

        //④ 激活绑定并重绘（等价"回填+应用渲染"一次完成；已渲染视口不 FitView，保持机位）
        mPostInfoHub::getInstance()->edit3DResult(target);
    }

    void mPostTreeView::syncCloudPresetName(const QString& oldName, const QString& newName)
    {
        //缓存快照名同步（节点改名/删除两端调用）：按节点名在当前模型各 Post3D 视口的
        //guidata._cloudPresets 中反查并改名（newName 空＝删除快照）。restoreCloudPreset
        //按"_presetName==节点名"反查快照，改名不同步则恢复失效；未命中说明快照随源
        //视口已亡，静默返回
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        if (vm == nullptr || oldName.isEmpty()) return;
        for (int id : vm->getAllViewportIDs())
        {
            QWidget* sub = vm->getSubWindowByViewportID(id);
            auto* priv = sub ? vm->getViewportPrivate(sub) : nullptr;
            if (priv == nullptr || priv->getViewportType() != MxFunctions::ViewportPost3D)
                continue;
            MechPreData::mPostGUIData* gui = getCVM->getGUIDataByViewName(priv->getViewportName());
            if (gui == nullptr) continue;
            for (int i = gui->_cloudPresets.size() - 1; i >= 0; --i)
            {
                if (gui->_cloudPresets.at(i)._presetName != oldName)
                    continue;
                if (newName.isEmpty())
                    gui->_cloudPresets.removeAt(i);
                else
                    gui->_cloudPresets[i]._presetName = newName;
            }
        }
    }

    void mPostTreeView::slot_updateVectorTree()
    {
        //矢量图去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editVector(resViewName);
    }

    void mPostTreeView::slot_updateStreamLineTree()
    {
        //流线图去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editStreamLine(resViewName);
    }

    void mPostTreeView::slot_updatePostSetTree()
    {
        //后处理-集合去派生视口（用户定案 2026-11-04）：与线性叠加同款——不建视口、不登记
        //条目、不建节点，面板直接绑定源云图视口（resolveActiveCloudViewName：云图
        //节点条目视口名，无节点时裸初始视口自身即源），生成/操作按 _curViewName
        //作用到源视口渲染（getCloudViewName 无 _Kind 后缀回退自身=源）
        const QString resViewName = resolveActiveCloudViewName();
        if (resViewName.isEmpty()) return;
        mPostInfoHub::getInstance()->editSet(resViewName);
    }
    //右键"编辑"：派生工况定义节点回编（单击派发同链；批次A0）
    void mPostTreeView::slot_editDerivedCaseDef()
    {
        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (srcIdxs.size() != 1) return;
        if (mTreeItemUtil::treeTypeOf(_treeModel, srcIdxs.first()) != MxFunctions::PostCaseDerivedChildItem)
            return;
        mPostInfoHub::getInstance()->editDerivedCase(mTreeItemUtil::phyIdOf(_treeModel, srcIdxs.first()));
    }

    //右键"删除"：store 删除定义（被其它定义引用时拦截并提示引用方名称）+ 分支即时重建
    void mPostTreeView::slot_removeDerivedCaseDef()
    {
        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (srcIdxs.size() != 1) return;
        if (mTreeItemUtil::treeTypeOf(_treeModel, srcIdxs.first()) != MxFunctions::PostCaseDerivedChildItem)
            return;
        const int definitionId = mTreeItemUtil::phyIdOf(_treeModel, srcIdxs.first());
        const int modelKey = _treeModel->getCurrentModelID();
        if (modelKey < 0) return;

        //批次A1联动：删除前留名——结果库同名派生工况（已物化帧）需一并墓碑清理
        DerivedCaseDefinition removedDef;
        const bool hadDef = mPostDerivedCaseStore::instance().definition(modelKey, definitionId, &removedDef);

        QList<int> referrers;
        const mPostDerivedCaseStore::RemoveResult result =
            mPostDerivedCaseStore::instance().removeDefinition(modelKey, definitionId, &referrers);
        if (result == mPostDerivedCaseStore::RemoveResult::NotFound)
            return;
        if (result == mPostDerivedCaseStore::RemoveResult::Referenced)
        {
            QStringList refNames;
            for (int refId : referrers)
            {
                DerivedCaseDefinition refDef;
                if (mPostDerivedCaseStore::instance().definition(modelKey, refId, &refDef))
                    refNames << refDef.name;
            }
            const bool isChinese = MAppConfiguration::mAppConfiguration::getInstance()->_language == 0;
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                isChinese ? QStringLiteral("派生工况被 %1 引用，请先删除或取消引用它的定义。")
									.arg(refNames.join(QStringLiteral("、")))
                          : QStringLiteral("The derived case is referenced by %1. Remove or unreference those definitions first.")
									.arg(refNames.join(QStringLiteral(", "))),
                true);
            return;
        }
        if (hadDef && !removedDef.name.isEmpty())
        {
            //结果库清理：定义所属模型的全部结果库逐个清（生成物化进当时的 postDB；
            //多结果文件模型按名清扫，未命中的库零动作）
            if (mViewportModel* vpModel = mViewportModelManager::getInstance()->getViewportModel(modelKey))
            {
                mPostDataManager* postMgr = &vpModel->getModelData()
                    ->getManager<mPostDataManager>(Physical_Post);
                const QStringList dbNames = (postMgr != nullptr) ? postMgr->getAllDatabaseName() : QStringList();
                for (const QString& dbName : dbNames)
                {
                    if (QPointer<mPostDataBase> db = postMgr->getPostDatabase(dbName))
                        db->removeDerivedCase(removedDef.name);
                }
            }

            //当前云图视口正显示该派生工况时回退到首个分析工况（墓碑工况不可再选；
            //其它视口的下拉在下次激活/刷新时自然过滤墓碑名）
            const QString activeCloud = resolveActiveCloudViewName();
            if (!activeCloud.isEmpty())
            {
                REBUILD_PostRend* rend = getCVM->getPostRenderByViewName(activeCloud);
                mPostGUIData* guidata = (rend != nullptr)
                    ? getCVM->getGUIDataByViewName(activeCloud) : nullptr;
                if (rend != nullptr && guidata != nullptr && guidata->_cloudCase == removedDef.name)
                {
                    QPointer<mPostDataBase> db = getCVM->getModelData()
                        ->getManager<mPostDataManager>(Physical_Post)
                        .getPostDatabase(rend->getPostDataName());
                    const QStringList analysisNames = (db != nullptr) ? db->getAllCaseNames() : QStringList();
                    if (!analysisNames.isEmpty())
                        mPostInfoHub::getInstance()->cloudResultPanel()
                            ->selectCaseByName(activeCloud, analysisNames.first());
                }
            }
        }
        //即时重建（store 通知的防抖重建随超时再跑一次，幂等无害）
        rebuildDerivedCaseTree(modelKey);
    }

    //右键"编辑"：测量记录节点回编（hub editMeasure 打开面板并定位记录进编辑，
    //统一编辑契约三选一确认内建；单击派发同链）
    void mPostTreeView::slot_editMeasureRecord()
    {
        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (srcIdxs.size() != 1) return;
        if (mTreeItemUtil::treeTypeOf(_treeModel, srcIdxs.first()) != MxFunctions::PostMeasureChildItem)
            return;
        mPostInfoHub::getInstance()->editMeasure(mTreeItemUtil::phyIdOf(_treeModel, srcIdxs.first()));
        //延迟切换高亮：dirty 被"继续编辑"拦截时还原到当前编辑记录节点
        if (auto* panel = mPostInfoHub::getInstance()->existingMeasureToolPanel())
            syncMeasureHighlight(panel->currentEditingMeasureId());
    }

    //右键显示/隐藏：批量作用于全部选中测量记录节点（经面板公开接口）
    void mPostTreeView::slot_showMeasureRecords()
    {
        setMeasureRecordsVisible(true);
    }

    void mPostTreeView::slot_hideMeasureRecords()
    {
        setMeasureRecordsVisible(false);
    }

    //选中测量记录节点 phyID 收集（面板调用触发树重建使选中索引失效，先收后作用）
    QList<int> mPostTreeView::selectedMeasureRecordIds() const
    {
        QList<int> ids;
        const QModelIndexList srcIdxs = selectedSourceIndexes();
        for (const QModelIndex& idx : srcIdxs)
        {
            if (mTreeItemUtil::treeTypeOf(_treeModel, idx) != MxFunctions::PostMeasureChildItem)
                continue;
            ids.append(mTreeItemUtil::phyIdOf(_treeModel, idx));
        }
        return ids;
    }

    void mPostTreeView::setMeasureRecordsVisible(bool visible)
    {
        auto* panel = mPostInfoHub::getInstance()->existingMeasureToolPanel();
        if (panel == nullptr) return;	//面板未开过即无会话记录
        const QList<int> ids = selectedMeasureRecordIds();
        for (int id : ids)
            panel->setMeasureRecordVisible(id, visible);
    }

    //右键"删除"：批量删除选中测量记录（编辑记录被删的面板回落与树重建由
    //面板信号驱动；先收 id 再删——逐条删除即时重建树，选中索引不可边遍历边用）
    void mPostTreeView::slot_removeMeasureRecords()
    {
        auto* panel = mPostInfoHub::getInstance()->existingMeasureToolPanel();
        if (panel == nullptr) return;
        const QList<int> ids = selectedMeasureRecordIds();
        for (int id : ids)
            panel->deleteMeasureRecord(id);
    }

}
