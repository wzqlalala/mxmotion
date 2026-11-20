#include "mPostBrowserWindow.h"
#include "mUiMenu.h"

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
#include "mPostMeasureTool.h"	//测量树联动：单击/高亮查询面板公开接口（完整类型）
#include "mPostSessionCoordinator.h"
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
#include "UiMetrics.h"
#include <QTimer>
#include "DerivedCase/mPostDerivedCaseStore.h"
#include "UiPalette.h"
#include "ThemeManager.h"	//refreshToolBarTheme 经共享入口统一下发（与前处理树窗口同一实现）
#include "mUiStyle.h"

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
namespace PostWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 过滤递归匹配（与 mTreeModel::treeItemFilter 同语义）：
    // 自身或任一后代命中即保留该行
    //////////////////////////////////////////////////////////////////////////
    namespace
    {
        bool matchesTextRecursive(const QAbstractItemModel* model, const QModelIndex& idx, const QString& filter)
        {
            if (model->data(idx, Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive))
                return true;
            const int n = model->rowCount(idx);
            for (int i = 0; i < n; ++i)
            {
                if (matchesTextRecursive(model, model->index(i, 0, idx), filter))
                    return true;
            }
            return false;
        }

        bool matchesStatusRecursive(const QAbstractItemModel* model, const QModelIndex& idx, int status)
        {
            if (model->data(idx, MechPreData::Roles::StatusRole).toInt() == status)
                return true;
            const int n = model->rowCount(idx);
            for (int i = 0; i < n; ++i)
            {
                if (matchesStatusRecursive(model, model->index(i, 0, idx), status))
                    return true;
            }
            return false;
        }

        int extractSuffixNumber(const QString& str)
        {
            static const QRegularExpression re("(\\d+)$");
            QRegularExpressionMatch match = re.match(str);
            if (match.hasMatch())
                return match.captured(1).toInt();
            return 0;
        }

        //重命名对数据管理器的查重与同步（自 mPostTreeModel::postRename 迁入，批次B2）：
        //经 renameValidator 钩子注入模型，模型本体不再触碰视口/数据管理器单例。
        //类型→条目类型映射收敛为一个 switch（原 postRename 十五段重复同构代码）
        bool postItemRenameSync(MxFunctions::BrowserTreeType type, const QString& oldName, const QString& newName)
        {
            PostData::ItemType itemType;
            switch (type)
            {
            case MxFunctions::threeDResultParentItem:   itemType = PostData::CloudItem; break;
            case MxFunctions::animationChildItem:       itemType = PostData::AnimationItem; break;
            case MxFunctions::linearChildItem:          itemType = PostData::LinearItem; break;
            case MxFunctions::scalarOperationChildItem: itemType = PostData::ScalarOperationItem; break;
            case MxFunctions::SectionChildItem:         itemType = PostData::SectionItem; break;
            case MxFunctions::isosurfaceChildItem:      itemType = PostData::IsosurfaceItem; break;
            case MxFunctions::vectorChildItem:          itemType = PostData::VectorItem; break;
            case MxFunctions::streamlineChildItem:      itemType = PostData::StreamLineItem; break;
            case MxFunctions::CurveChildItem:           itemType = PostData::CurveItem; break;
            case MxFunctions::WordReportChildItem:      itemType = PostData::ExportWordItem; break;
            case MxFunctions::PathChildItem:            itemType = PostData::PathItem; break;
            default:
                return false;   //setData 已按可重命名类型预筛，防御
            }
            auto& mgr = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post);
            if (mgr.ExistItemName(itemType, newName))
            {
                mGlobalSignals::getInstance()->outputMessageSig("该结果名称已存在！");
                return false;
            }
            mgr.resetItemName(itemType, oldName, newName);
            return true;
        }

        //视口名反查用：结果节点类型 → 数据管理器条目类型。
        //与 slot_singleClickItem 各 case 的查询口径一致（各类型用各自 ItemType——
        //管理器 getItemId 为分类型独立计数，跨类型查 ID 必然串条目：旧实现
        //Line/Arrow/Text/Log 借 PathItem 查询，"路径_1"与"二维线_1"同 ID 即互串）；
        //非结果类型返回 -1
        PostData::ItemType viewportLookupItemType(MxFunctions::BrowserTreeType type)
        {
            switch (type)
            {
            case MxFunctions::threeDResultParentItem:   return PostData::CloudItem;
            case MxFunctions::animationChildItem:       return PostData::AnimationItem;
            case MxFunctions::linearChildItem:          return PostData::LinearItem;
            case MxFunctions::scalarOperationChildItem: return PostData::ScalarOperationItem;
            case MxFunctions::SectionChildItem:         return PostData::SectionItem;
            case MxFunctions::isosurfaceChildItem:      return PostData::IsosurfaceItem;
            case MxFunctions::vectorChildItem:          return PostData::VectorItem;
            case MxFunctions::streamlineChildItem:      return PostData::StreamLineItem;
            case MxFunctions::setPostChildItem:         return PostData::SetItem;
            case MxFunctions::CurveChildItem:           return PostData::CurveItem;
            case MxFunctions::WordReportChildItem:      return PostData::ExportWordItem;
            case MxFunctions::PathChildItem:            return PostData::PathItem;
            case MxFunctions::LineChildItem:            return PostData::LineItem;
            case MxFunctions::ArrowChildItem:           return PostData::ArrowItem;
            case MxFunctions::TextChildItem:            return PostData::TextItem;
            case MxFunctions::LogChildItem:             return PostData::LogItem;
            default:                                    return static_cast<PostData::ItemType>(-1);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // mPostTreeProxy
    //////////////////////////////////////////////////////////////////////////
    void mPostTreeProxy::setSortMode(SortMode m, Qt::SortOrder sortType)
    {
        _mode = m;
        invalidate();
        sort(0, sortType);
    }

    bool mPostTreeProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!idx.isValid()) return true;

        //源模型为独立后处理树模型：顶层行即各后处理模型根，无需再做类型筛选，
        //非顶级行随父行结果（父行被拒则整棵子树已过滤）

        //名称过滤：自身或任一后代命中（与前处理树 treeItemFilter 同语义）
        if (!_nameFilter.isEmpty() && !matchesTextRecursive(sourceModel(), idx, _nameFilter))
            return false;

        //状态过滤：自身或任一后代命中
        if (_statusFilter >= 0 && !matchesStatusRecursive(sourceModel(), idx, _statusFilter))
            return false;

        return true;
    }

    bool mPostTreeProxy::lessThan(const QModelIndex& a, const QModelIndex& b) const
    {
        switch (_mode)
        {
        case SortMode::DefaultOrder:
            //保持源模型原始顺序：先比 parent 下的 row；row 相同再比内部 id 防止不稳定
            if (a.row() != b.row()) return a.row() < b.row();
            return a.internalId() < b.internalId();
        case SortMode::Alpha:
        {
            const QString ka = sourceModel()->data(a, Qt::DisplayRole).toString();
            const QString kb = sourceModel()->data(b, Qt::DisplayRole).toString();
            if (_collator == nullptr)
            {
                _collator = new QCollator(QLocale(QLocale::Chinese, QLocale::China));
                _collator->setNumericMode(true);
                _collator->setCaseSensitivity(Qt::CaseInsensitive);
                _collator->setIgnorePunctuation(true);
            }
            return _collator->compare(ka, kb) < 0;
        }
        case SortMode::SuffixNumber:
        {
            const QString ka = sourceModel()->data(a, Qt::DisplayRole).toString();
            const QString kb = sourceModel()->data(b, Qt::DisplayRole).toString();
            return extractSuffixNumber(ka) < extractSuffixNumber(kb);
        }
        default:
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // mPostTreeView
    //////////////////////////////////////////////////////////////////////////
    QModelIndexList mPostTreeView::selectedSourceIndexes() const
    {
        QModelIndexList sourceIdxs;
        if (selectionModel() == nullptr) return sourceIdxs;
        const QModelIndexList rows = selectionModel()->selectedRows();
        for (const QModelIndex& r : rows)
            sourceIdxs << _proxy->mapToSource(r);
        return sourceIdxs;
    }

    void mPostTreeView::selectSourceIndex(const QModelIndex& sourceIdx)
    {
        QModelIndex proxyIdx = _proxy->mapFromSource(sourceIdx);
        if (!proxyIdx.isValid()) return;
        selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        setCurrentIndex(proxyIdx);
        scrollTo(proxyIdx);
    }

    void mPostTreeView::nameFilterTree(const QString& filterStr)
    {
        _proxy->setNameFilter(filterStr);
    }

    void mPostTreeView::statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus)
    {
        _proxy->setStatusFilter(static_cast<int>(filterStatus));
    }

    void mPostTreeView::sortTree(int sortWay, Qt::SortOrder sortType)
    {
        _proxy->setSortMode((mPostTreeProxy::SortMode)sortWay, sortType);
    }

    void mPostTreeView::selectedItem(mPostTreeItem* item)
    {
        if (item == nullptr) return;
        selectSourceIndex(_treeModel->indexFromItem(item));
    }

    void mPostTreeView::removeTreeItem(mPostTreeItem* item)
    {
        _treeModel->removeTreeItem(item);
    }

    void mPostTreeView::removeProxyIndexs(QModelIndexList proxyIndexs)
    {
        if (proxyIndexs.isEmpty()) return;
        _treeModel->removeIndexs(proxyIndexs);
    }

    MxFunctions::BrowserTreeStatus mPostTreeView::getStatusByItem(mPostTreeItem* item)
    {
        if (item == nullptr) return MxFunctions::Tree_Yes;
        return mTreeItemUtil::statusOf(_treeModel, _treeModel->indexFromItem(item));
    }

    void mPostTreeView::setTreeItemStatus(mPostTreeItem* item, MxFunctions::BrowserTreeStatus stauts)
    {
        QModelIndex idx = _treeModel->indexFromItem(item);
        _treeModel->setData(idx, (int)stauts, MechPreData::Roles::StatusRole);
    }

    void mPostTreeView::expandItem(mPostTreeItem* item)
    {
        if (item == nullptr) return;
        QModelIndex proxyIdx = _proxy->mapFromSource(_treeModel->indexFromItem(item));
        if (proxyIdx.isValid())
        {
            expand(proxyIdx);
            scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
        }
    }

    void mPostTreeView::slot_itemSelectionChanged(const QItemSelection&, const QItemSelection&)
    {
        //mTreeView 在选中变化时联动符号显示等；后处理树的联动在 slot_singleClickItem 内完成
    }

    void mPostTreeView::slot_viewportActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        if (!getCVM)
        {
            return;
        }

        auto* viewportPrivate = VMManager->getCurrentViewportPrivate();

        //当前无有效渲染视口（如激活的是曲线图表卡片——卡片ID不在渲染视口注册表）：
        //无后处理树同步可做；此判空须在 Post2D 判断之前（同 mTreeView::slot_subWindowActivated）
        if (viewportPrivate == nullptr)
        {
            return;
        }

        //曲线视口（ViewportPost2D）不涉及后处理树
        if (viewportPrivate->getViewportType() == ViewportPost2D)
            return;

        //同步本模型的"当前模型"（ getParentItem 按 {_currentModelID, treeType} 查索引）
        int modelID = getCVM->getModelData()->getModelDataID();
        if (modelID != _treeModel->getCurrentModelID())
        {
            _treeModel->setCurrentModelID(modelID);
        }

        //后处理模型激活：树范围收敛到当前模型（仅显示其根），再按该视口的选中记忆恢复选中。
        //程序化选区不触发单击派发（clicked 仅响应用户点击），selectionChanged 槽为空
        if (!getCVM->isPostModel())
        {
            return;
        }

        scopeToCurrentModel();

        //参数栏跟随视口内容（优先）：按激活视口名反查当前模型的结果节点，命中即选中并
        //重放单击派发——面板即该视口对应项，选中记忆经 updateItemSelected 自愈。
        //不以此为优先时，若选中记忆是容器节点（如"结果"，无面板语义），重放派发不动作，
        //参数栏会残留上一个模型的云图等面板
        if (mPostTreeItem* viewNode = findNodeByViewportName(viewportPrivate->getViewportName()))
        {
            selectSourceIndex(_treeModel->indexFromItem(viewNode));
            slot_singleClickItem();
            return;
        }

        //派生视口绑回（去节点化 2026-11-04）：派生视口（动画/切面/矢量等）无树节点可反查，
        //经条目类型反查（findItemTypesByViewName）命中即打开对应参数面板——与原树节点
        //单击派发 editXxx 等价；2D 视口绑定类（曲线/报告/路径）同理在此唤出面板
        {
            const QString activeViewName = viewportPrivate->getViewportName();
            const QVector<PostData::ItemType> hitTypes = getCVM->getModelData()
                ->getManager<PostData::mPostDataManager>(Physical_Post).findItemTypesByViewName(activeViewName);
            for (PostData::ItemType t : hitTypes)
            {
                QString menuName;
                switch (t)
                {
                case PostData::AnimationItem:
                    mPostInfoHub::getInstance()->editAnimation(activeViewName); menuName = "动画"; break;
                case PostData::LinearItem:
                    mPostInfoHub::getInstance()->editLinear(activeViewName); menuName = "线性叠加"; break;
                case PostData::ScalarOperationItem:
                    mPostInfoHub::getInstance()->editScalarOperation(activeViewName); menuName = "标量运算"; break;
                case PostData::SectionItem:
                    mPostInfoHub::getInstance()->editSection(activeViewName); menuName = "切面图"; break;
                case PostData::IsosurfaceItem:
                    mPostInfoHub::getInstance()->editIsosurface(activeViewName); menuName = "等值面图"; break;
                case PostData::VectorItem:
                    mPostInfoHub::getInstance()->editVector(activeViewName); menuName = "矢量图"; break;
                case PostData::StreamLineItem:
                    mPostInfoHub::getInstance()->editStreamLine(activeViewName); menuName = "流线图"; break;
                case PostData::SetItem:
                    mPostInfoHub::getInstance()->editSet(activeViewName); menuName = "后处理-集合"; break;
                case PostData::CurveItem:
                    mPostInfoHub::getInstance()->editCurve(activeViewName); menuName = "后处理-曲线"; break;
                case PostData::ExportWordItem:
                    mPostInfoHub::getInstance()->editExportWord(activeViewName); menuName = "分析报告"; break;
                case PostData::PathItem:
                    mPostInfoHub::getInstance()->editPath(activeViewName); menuName = "路径"; break;
                default:
                    break;
                }
                if (!menuName.isEmpty())
                {
                    MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(menuName);
                    return;
                }
            }
        }

        //回退：视口名暂无对应结果节点（如导入中途的裸视口），走选中记忆恢复链路
        QList<mItemKey> itemKeys = viewportPrivate->getTreeSelectItems();
        QModelIndexList proxyIndexs;
        for (const auto& key : itemKeys)
        {
            QModelIndex idx = _treeModel->indexFromKey(key);
            if (!idx.isValid()) continue;
            QModelIndex proxyIdx = _proxy->mapFromSource(idx);
            if (!proxyIdx.isValid()) continue;
            proxyIndexs.push_back(proxyIdx);
        }

        //选中记忆恢复成功才重放单击派发：恢复的选中来自本视口的历史，面板随之联动
        //（程序化选区不触发 clicked，须显式重放）
        if (!proxyIndexs.isEmpty())
        {
            for (int i = 0; i < proxyIndexs.size(); ++i)
            {
                QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Rows
                    | (i == 0 ? QItemSelectionModel::ClearAndSelect : QItemSelectionModel::Select);
                selectionModel()->select(proxyIndexs[i], flags);
            }
            slot_singleClickItem();
            return;
        }

        //裸初始视口（导入时创建、无云图子节点承载）绑定"云图"根：激活/双击该视口时
        //选中云图根节点并打开云图参数面板——edit3DResult 对已激活视口只做面板激活，
        //不切换视口（activatePostViewport 同视口幂等早退），不会反向覆盖本次激活。
        //识别走结构判定（协议名锚点已随显示名接管废除）：云图根无真实云图子节点
        //（缓存节点不占位）⇒ 该视口即初始视口（"无云图 ⇒ 唯一 Post3D 视口"不变量，
        //见 solePost3DViewportName）
        mPostTreeItem* bareCloudRootItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
        if (bareCloudRootItem != nullptr && realCloudChildCount(bareCloudRootItem) == 0
            && viewportPrivate->getViewportName() == solePost3DViewportName(_treeModel->getCurrentModelID()))
        {
            selectSourceIndex(_treeModel->indexFromItem(bareCloudRootItem));
            MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow("云图");
            mPostInfoHub::getInstance()->edit3DResult(viewportPrivate->getViewportName());
            return;
        }

        //其余无节点视口：当前选中是与该视口无关的残留选中，重放派发会给残留节点
        //（另一云图）发起 edit3DResult 并激活其视口——反向覆盖用户刚完成的双击/点击激活；
        //清空选中并复位参数面板
        selectionModel()->clearSelection();
        setCurrentIndex(QModelIndex());
        MechCommon::panelHostRef()->unEditInfoWindow();
    }

    mPostTreeItem* mPostTreeView::findNodeByViewportName(const QString& viewportName)
    {
        if (viewportName.isEmpty()) return nullptr;
        mPostTreeItem* rootItem = _treeModel->getParentItem(PostTreeRootItem);
        if (rootItem == nullptr) return nullptr;
        //仅"结果/云图"两棵子树承载视口名映射节点（部件/材料/集合等无面板语义），
        //不做全树深搜——大模型部件/集合节点上千，原实现每节点一次管理器查询（批次C剪枝）
        for (mPostTreeItem* child : rootItem->getChildrenItem())
        {
            if (mTreeItemUtil::treeTypeOfItem(_treeModel, child) == MxFunctions::ResultParentItem)
            {
                if (mPostTreeItem* hit = findNodeByViewportNameRecursive(child, viewportName))
                    return hit;
            }
        }
        return nullptr;
    }

    mPostTreeItem* mPostTreeView::findNodeByViewportNameRecursive(mPostTreeItem* item, const QString& viewportName)
    {
        //深度优先：节点类型映射到条目类型后按 viewName 比对（viewName 与视口名同源）
        const MxFunctions::BrowserTreeType type = mTreeItemUtil::treeTypeOfItem(_treeModel, item);
        const PostData::ItemType itemType = viewportLookupItemType(type);
        if (itemType != static_cast<PostData::ItemType>(-1))
        {
            const QString viewName = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
                .getViewNameByID(itemType, mTreeItemUtil::phyIdOfItem(_treeModel, item));
            if (viewName == viewportName)
            {
                return item;
            }
        }
        for (mPostTreeItem* child : item->getChildrenItem())
        {
            if (mPostTreeItem* hit = findNodeByViewportNameRecursive(child, viewportName))
            {
                return hit;
            }
        }
        return nullptr;
    }

    void mPostTreeView::scopeToCurrentModel()
    {
        //树范围收敛：仅显示当前后处理模型的根，其余顶层根行隐藏。
        //用视图级行隐藏（setRowHidden）而非代理过滤：行不进出代理模型，
        //切回时保持原展开状态（代理过滤会让子树按默认折叠重新映射进来）——同前处理树方案
        const int curModel = _treeModel->getCurrentModelID();
        for (int row = 0; row < _treeModel->rowCount(QModelIndex()); ++row)
        {
            QModelIndex srcIdx = _treeModel->index(row, 0, QModelIndex());
            QModelIndex proxyIdx = _proxy->mapFromSource(srcIdx);
            if (!proxyIdx.isValid())
            {
                continue;
            }
            setRowHidden(proxyIdx.row(), QModelIndex(),
                srcIdx.data(MechPreData::Roles::ModelRole).toInt() != curModel);
        }
    }

    void mPostTreeView::slot_createNote()
    {
        //自 mTreeView::slot_createNote 移植（clickPostNoteSig 当前无 emit 方，休眠路径保留语义）
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() != 1 || getTreeTypeByModelIndex(idxs[0]) != ResultParentItem)
        {
            mGlobalSignals::getInstance()->outputMessageSig(2, "请先在树中选中一个「结果」根节点！", true);
            return;
        }

        int caseId = getPhyIdByModelIndex(idxs[0]);
        mPostTreeItem* parentItem = _treeModel->getParentItem(ResultParentItem, caseId);
        if (parentItem == nullptr) return;

        // 如果已存在注释节点，则不再重复创建
        for (auto& item : parentItem->getChildrenItem())
        {
            if (getTreeTypeByItem(item) == NoteParentItem)
            {
                return;
            }
        }

        // 创建新的注释节点（图标走 getBrowserTreePic 的 zhushi.svg，原 XY chart.png 占位已废弃）
        QString treeName = "注释";
        mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), NoteParentItem, treeName, 1);
        _treeModel->appendChildren(parentItem, childitem);
        selectedItem(childitem);
    }

    void mPostTreeView::slot_removePostModelTree(int modelDataID)
    {
        //派生工况定义桶随模型回收（批次A0；removeModel 幂等、不触发变更通知）
        mPostDerivedCaseStore::instance().removeModel(modelDataID);

        //按 {modelID, PostTreeRootItem} 定位根节点（phyID/caseID 取默认值，与建根时一致）
        mItemKey key;
        key._modelID = modelDataID;
        key._treeType = PostTreeRootItem;
        mPostTreeItem* rootItem = _treeModel->treeItemFromKey(key);
        if (rootItem == nullptr) return;

        _treeModel->removeTreeItem(rootItem);

        //若移除的是当前模型：重置当前模型ID，待视口激活链路（slot_viewportActivated）重新同步
        if (_treeModel->getCurrentModelID() == modelDataID)
        {
            _treeModel->setCurrentModelID(-1);
        }

        //清理结果跳转登记（该模型的再次跳转回退为新建+导入）
        mPostBrowserWindow::getInstance()->unregisterResultJump(modelDataID);

        //刷新树范围（被删根的隐藏行随之移除；剩余根的显隐按当前模型重新收敛）
        scopeToCurrentModel();
    }

    void mPostTreeView::slot_menuChildRename()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.size() == 0) return;

        if (idxs.size() > 1)
            _treeModel->setSelectRenames(true);
        else
            _treeModel->setSelectRenames(false);

        QModelIndex proxyIdx = _proxy->mapFromSource(idxs[0]);
        if (proxyIdx.isValid())
            edit(proxyIdx);
    }

    void mPostTreeView::slot_rightClickItem()
    {
        //后处理树右键菜单分发：多选要求类型一致；按当前节点类型分发到 createXxxMenu
        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (srcIdxs.isEmpty())
            return;

        MxFunctions::BrowserTreeType type = mTreeItemUtil::treeTypeOf(_treeModel, srcIdxs[0]);
        for (const QModelIndex& idx : srcIdxs)
        {
            if (type != mTreeItemUtil::treeTypeOf(_treeModel, idx))
                return;
        }

        if (m_Menu != nullptr)
        {
            delete m_Menu;
            m_Menu = nullptr;
        }
        m_Menu = new MechMainWindow::mUiMenu(this);

        switch (type)
        {
        case MxFunctions::PostTreeRootItem:
            this->createPostModelRootItem();
            break;
        case MxFunctions::PostModelParentItem:
            this->createPostModelParentMenu();
            break;
        case MxFunctions::PostModelChildItem:
            this->createPostModelChildMenu();
            break;
        case MxFunctions::PostModelGroupRootItem:
            this->createPostModelGroupRootMenu();
            break;
        case MxFunctions::PostModelGroupParentItem:
            this->createPostModelGroupParentMenu();
            break;
        case MxFunctions::PostModelGroupChildItem:
            // 分组子节点为引用节点，复用父分组菜单
            this->createPostModelGroupParentMenu();
            break;
        case MxFunctions::PostMaterialRootItem:
        case MxFunctions::PostSectionRootItem:
            this->createPostMaterialRootMenu();
            break;
        case MxFunctions::PostMaterialChildItem:
        case MxFunctions::PostSectionChildItem:
            this->createPostMaterialChildMenu();
            break;
        case MxFunctions::PostMaterialPartItem:
        case MxFunctions::PostSectionPartItem:
            this->createPostMaterialPartMenu();
            break;
        case MxFunctions::PostSetRootItem:
            this->createPostSetRootMenu();
            break;
        case MxFunctions::PostSetElementRootItem:
            this->createPostSetElementRootMenu();
            break;
        case MxFunctions::PostSetNodeRootItem:
            this->createPostSetNodeRootMenu();
            break;
        case MxFunctions::PostSetFaceRootItem:
            this->createPostSetFaceRootMenu();
            break;
        case MxFunctions::PostSetEdgeRootItem:
            this->createPostSetEdgeRootMenu();
            break;
        case MxFunctions::PostSetPartRootItem:
            this->createPostSetPartRootMenu();
            break;
        case MxFunctions::PostSetElementChildItem:
            this->createPostSetElementChildMenu();
            break;
        case MxFunctions::PostSetNodeChildItem:
            this->createPostSetNodeChildMenu();
            break;
        case MxFunctions::PostSetFaceChildItem:
            this->createPostSetFaceChildMenu();
            break;
        case MxFunctions::PostSetEdgeChildItem:
            this->createPostSetEdgeChildMenu();
            break;
        case MxFunctions::PostSetPartChildItem:
            this->createPostSetPartChildMenu();
            break;
        case MxFunctions::ResultParentItem:
            //"结果"根已移除（2D 类去节点化），云图根为该类型唯一节点：
            //右键菜单承接新建云图/结果更新入口
            this->createCloudRootMenu();
            break;
        case MxFunctions::threeDResultParentItem:
            this->create3DResultParentMenu();
            break;
        case MxFunctions::PostCaseDerivedRootItem:
            //派生工况分支头："新建"三入口（复用 Ribbon 结果页总线信号，行为一致）
            this->createPostCaseDerivedRootMenu();
            break;
        case MxFunctions::PostCaseDerivedChildItem:
            //派生工况定义节点：编辑（回编对应面板）/删除（store 删除+分支重建）
            this->createPostCaseDerivedChildMenu();
            break;
        case MxFunctions::PostMeasureChildItem:
            //测量记录节点：编辑（回编面板，同单击）/显示/隐藏/删除（V1.3 测量树联动）
            this->createMeasureChildMenu();
            break;
        case MxFunctions::PostCoordinateRootItem:
            //坐标系根：创建坐标系（默认直角，走模型页入口链）/删除所有用户坐标系
            this->createCoordinateRootMenu();
            break;
        case MxFunctions::PostCoordinateChildItem:
            //用户坐标系节点：编辑/重命名/复制/删除（全局坐标系节点在菜单内拦截不挂菜单）
            this->createCoordinateChildMenu();
            break;
        //派生子节点（动画/线性叠加/标量运算/切面/等值面/矢量）菜单随去节点化移除
        default:
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 后处理导入完成链路（自 mTreeView::dispatchResultTreeUpdate 演进）：
    // 以后处理树选中（无选中时为当前模型的后处理模型根）为上下文创建三维结果节点
    //////////////////////////////////////////////////////////////////////////
    void mPostTreeView::updateResultTree()
    {
        QModelIndexList ctx = selectedSourceIndexes();
        if (ctx.isEmpty())
        {
            mPostTreeItem* root = _treeModel->getParentItem(MxFunctions::PostTreeRootItem);
            if (root != nullptr)
                ctx = { _treeModel->indexFromItem(root) };
        }
        if (ctx.isEmpty()) return;

        if (mTreeItemUtil::treeTypeOf(_treeModel, ctx.first()) == MxFunctions::PostTreeRootItem)
        {
            //导入完成（模型根上下文）："云图"根默认为空，不自动生成云图子节点；
            //仅创建初始云图视口，供导入完成回调激活视口/更新面板使用，
            //用户手动创建的首个云图会收编该视口（见下方 ResultParentItem 分支）。
            //已有云图时本分支无事可做——初始视口已被收编改名，再按协议名幂等创建
            //会误建游离视口（显示名接管前的旧协议名锚点语义，一并废除）。
            //缓存节点不占位：仅有缓存节点时初始视口仍未收编，照常复用
            mPostTreeItem* cloudRootItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);
            if (cloudRootItem == nullptr || realCloudChildCount(cloudRootItem) == 0)
            {
                int modelID = getCVM->getModelData()->getModelDataID();
                QString viewport = mViewportName::initialCloud(modelID);
                //无云图 ⇒ 唯一 Post3D 视口即初始视口：已存在则复用其当前名（不重复创建）
                if (const QString sole = solePost3DViewportName(modelID); !sole.isEmpty())
                {
                    viewport = sole;
                }
                MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);
                //注意：此处不接管显示名——导入收尾链（slot_finishedImportFiles 及面板
                //_curViewName 全链）以本协议名工作，中途改名会使按名查找落空（变量/分量
                //下拉为空的根因）；初始视口改名由 mPost3DResult::slot_finishedImportFiles
                // 在链路完成后执行并同步面板当前名
            }
        }
        else if (mTreeItemUtil::treeTypeOf(_treeModel, ctx.first()) == MxFunctions::ResultParentItem)
        {
            //结果/云图根节点上下文（右键"云图"路径）：创建新云图视口，统一挂到"云图"根下
            mPostTreeItem* parentItem = _treeModel->getParentItem(MxFunctions::ResultParentItem, PostCloudRootPhyID);

            int modelID = getCVM->getModelData()->getModelDataID();
            //命名取数据层ID（getItemId 唯一递增不重用）：用户改名不影响后续编号、
            //删除后新建跳号不复用（原按兄弟节点文本 split("_") 扫描，改名即污染计数）
            int resultID = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getItemId(PostData::CloudItem);
            const QString treeName = QString("云图_%1").arg(resultID);

            //首个云图收编导入时创建的初始视口，后续云图各自新建独立视口。
            //收编按"当前名"定位（初始视口可能已被显示名接管为"模型名· 云图"，
            //协议名锚点已废除；无云图 ⇒ 唯一 Post3D 视口即初始视口）；
            //有无云图按真实云图子节点计（缓存节点不占位）
            QString viewport;
            if (realCloudChildCount(parentItem) == 0)
            {
                viewport = solePost3DViewportName(modelID);
                if (viewport.isEmpty())
                    viewport = mViewportName::initialCloud(modelID);   //兜底：视口尚未创建
            }
            else
            {
                viewport = mViewportName::cloud(modelID, resultID);
            }
            MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewport);

            PostData::CloudInfo info;
            info.caseId = -1;
            info.itemName = treeName;
            info.viewName = viewport;
            getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).addItem(PostData::CloudItem, info);

            mPostTreeItem* childitem = new mPostTreeItem(getCVM->getModelData()->getModelDataID(), MxFunctions::threeDResultParentItem, treeName, resultID);
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);
            //显示名接管："{模型名}· {云图节点名}"（收编的初始视口从此以显示名存在）。
            //改名后立即同步面板当前名——slot_create3DResultViewport 已把面板 _curViewName
            //设为协议名，不同步则用户随后切工况/变量按旧名查找落空（同导入链故障根因）；
            //setCurViewName 各步判空（此刻 postDataName 尚空，早退无害），不干扰外层链
            if (const QString displayName = renameItemViewportDisplay(MxFunctions::threeDResultParentItem, treeName, viewport);
                !displayName.isEmpty())
            {
                MechCommon::mMechGlobalSignals::getInstance()->updatePostViewName(displayName);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 会话保存/恢复（.mxses）：云图条目采集与回放
    //////////////////////////////////////////////////////////////////////////
    QList<QPair<QString, QString>> mPostTreeView::collectCloudItems(int modelDataID)
    {
        QList<QPair<QString, QString>> items;
        mPostTreeItem* cloudRoot = _treeModel->getChildrenItem(
            MxFunctions::ResultParentItem, PostCloudRootPhyID, -1, modelDataID);
        if (cloudRoot == nullptr)
            return items;
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(modelDataID);
        if (vm == nullptr || vm->getModelData() == nullptr)
            return items;
        auto& manager = vm->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post);
        for (mPostTreeItem* child : cloudRoot->getChildrenItem())
        {
            if (mTreeItemUtil::treeTypeOfItem(_treeModel, child) != MxFunctions::threeDResultParentItem)
                continue;
            const int resultID = mTreeItemUtil::phyIdOfItem(_treeModel, child);
            const QString itemName = mTreeItemUtil::textOfItem(_treeModel, child);
            const QString viewName = manager.getViewNameByID(PostData::CloudItem, resultID);
            if (itemName.isEmpty() || viewName.isEmpty())
                continue;
            items.append({ itemName, viewName });
        }
        return items;
    }

    void mPostTreeView::replayCloudItems(int modelDataID, const QList<QPair<QString, QString>>& items)
    {
        if (items.isEmpty())
            return;
        mPostTreeItem* parentItem = _treeModel->getChildrenItem(
            MxFunctions::ResultParentItem, PostCloudRootPhyID, -1, modelDataID);
        if (parentItem == nullptr)
            return;
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(modelDataID);
        if (vm == nullptr || vm->getModelData() == nullptr)
            return;
        auto& manager = vm->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post);

        bool firstBound = false;
        for (const auto& pair : items)
        {
            const QString& itemName = pair.first;
            const QString& viewName = pair.second;
            if (manager.ExistItemName(PostData::CloudItem, itemName))
                continue;   //幂等（重复回放保护）

            //视口按保存名重绑：缺失时补建；首个条目无条件发创建信号——已存在时
            //槽内仅绑定面板 _curPostRend/_curViewName（回放先于 updateResultTree，
            //云图根非空会跳过其初始视口创建分支，面板绑定须由此补上）
            const bool needCreate = !vm->getViewport(viewName);
            if (!firstBound || needCreate)
            {
                MechCommon::mMechGlobalSignals::getInstance()->create3DResultViewport(viewName);
            }
            firstBound = true;

            PostData::CloudInfo info;
            info.caseId = -1;
            info.itemName = itemName;
            info.viewName = viewName;
            //树节点 phyID 用 addItem 将分配的云图id：getItemId 先取（peek）再由
            //addItem 消耗，与 updateResultTree 右键创建路径同一配方
            const int resultID = manager.getItemId(PostData::CloudItem);
            manager.addItem(PostData::CloudItem, info);

            mPostTreeItem* childitem = new mPostTreeItem(modelDataID,
                MxFunctions::threeDResultParentItem, itemName, resultID);
            _treeModel->appendChildren(parentItem, childitem);
            selectedItem(childitem);

            //显示名接管（同手工创建路径；视口已叫该名时为同值幂等）
            if (const QString displayName = renameItemViewportDisplay(
                MxFunctions::threeDResultParentItem, itemName, viewName);
                !displayName.isEmpty())
            {
                MechCommon::mMechGlobalSignals::getInstance()->updatePostViewName(displayName);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 单击派发（自 mTreeView::slot_singleClickItem 的后处理分支移植）
    //////////////////////////////////////////////////////////////////////////
    void mPostTreeView::slot_singleClickItem()
    {
        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (srcIdxs.isEmpty())
            return;

        //多选：部件等多选操作经右键菜单按选中集合工作，这里只做渲染联动
        if (srcIdxs.size() > 1)
        {
            QList<mItemKey> keys;
            for (const QModelIndex& idx : srcIdxs)
                keys << _treeModel->itemKeyFromIndex(idx);
            updateItemSelected();
            MechCommon::mMechGlobalSignals::getInstance()->browsertreeSelectedChanged(keys, true);
            return;
        }

        const QModelIndex idx = srcIdxs.first();
        QString name;
        MxFunctions::BrowserTreeType type = mTreeItemUtil::treeTypeOf(_treeModel, idx);
        int phyID = mTreeItemUtil::phyIdOf(_treeModel, idx);
        switch (type)
        {
        case MxFunctions::ResultParentItem:
        {
            //该类型现仅剩云图根（"结果"根已随 2D 类去节点化移除）：
            //点击仅切换到云图参数面板，不新建云图子节点
            slot_switchCloudPanel();
            name = "云图";
            break;
        }
        case MxFunctions::threeDResultParentItem:
        {
            //云图节点→绑定其视口编辑；缓存节点（条目 viewName 空，2026-11-05 去视口化
            //后仅参数快照、无独立视口）→回放快照到云图面板并重绘恢复云图（后续
            //"应用渲染"按钮按回填后的面板参数可再次重绘）
            QString viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::CloudItem, phyID);
            if (viewport.isEmpty())
                restoreCloudPreset(getTextByModelIndex(idx));
            else
                mPostInfoHub::getInstance()->edit3DResult(viewport);
            name = "云图";
            break;
        }
        case MxFunctions::PostCaseDerivedChildItem:
        {
            //派生工况定义节点（批次A0，V1.3 原型"工况/派生工况"模型树分支）→
            //按 kind 回编对应参数面板（hub editDerivedCase 路由三面板）
            mPostInfoHub::getInstance()->editDerivedCase(phyID);
            name = "派生工况";
            break;
        }
        //派生功能（动画/线性叠加/标量运算/切面/等值面/矢量/流线/集合）已去节点化
        //（用户定案 2026-11-04）：树节点不存在，分支移除——激活派生视口时经
        //slot_viewportActivated 的条目类型反查绑回对应参数面板
        //曲线/Word报告/路径已去节点化（树节点不存在，分支移除）：
        //面板随激活视口绑定，见 mPostCurve/mPostExportWord/mPostPath
        case MxFunctions::LineChildItem:
        {
            QString viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::LineItem, phyID);
            mPostInfoHub::getInstance()->editLine(viewport);
            name = "二维线";
            break;
        }
        case MxFunctions::ArrowChildItem:
        {
            QString viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::ArrowItem, phyID);
            mPostInfoHub::getInstance()->editArrow(viewport);
            name = "箭头";
            break;
        }
        case MxFunctions::TextChildItem:
        {
            QString viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::TextItem, phyID);
            mPostInfoHub::getInstance()->editText(viewport);
            name = "文本";
            break;
        }
        case MxFunctions::LogChildItem:
        {
            QString viewport = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::LogItem, phyID);
            mPostInfoHub::getInstance()->editLog(viewport);
            name = "Log";
            break;
        }
        //集合/坐标系根→联动切到"模型"页（原型为分支双击跳转，按本树单击联动惯例实现；
        //根节点无参数面板动作）
        case MxFunctions::PostSetRootItem:
        case MxFunctions::PostCoordinateRootItem:
        {
            name = "模型";
            break;
        }
        //坐标系子节点→用户坐标系打开面板编辑该坐标系（hub editCoordinate 回填）；
        //全局坐标系只切"模型"页（只读固有定义，无参数面板动作，对齐前处理口径）
        case MxFunctions::PostCoordinateChildItem:
        {
            if (phyID != PostGlobalCoordinatePhyID)
                mPostInfoHub::getInstance()->editCoordinate(phyID);
            name = "模型";
            break;
        }
        //集合子节点→打开集合面板编辑该集合（对齐前处理树点集合节点编辑语义；显隐仍走右键）
        case MxFunctions::PostSetElementChildItem:
        case MxFunctions::PostSetNodeChildItem:
        case MxFunctions::PostSetFaceChildItem:
        case MxFunctions::PostSetEdgeChildItem:
        case MxFunctions::PostSetPartChildItem:
        {
            mPostInfoHub::getInstance()->editSetByName(getTextByModelIndex(idx));
            name = "模型";
            break;
        }
        //测量根/子节点→打开测量工具面板并切"结果"页（V1.3 原型测量树入口）：
        //根=打开面板；子节点=面板定位该记录进编辑（树节点点击=管理表名称格点击
        //同一语义，经 hub editMeasure 路由，面板公开选中接口已接入）
        case MxFunctions::PostMeasureRootItem:
        {
            slot_createMeasureTool();
            name = "结果";
            break;
        }
        case MxFunctions::PostMeasureChildItem:
        {
            mPostInfoHub::getInstance()->editMeasure(phyID);
            //延迟切换高亮（V1.3）：dirty 被三选一"继续编辑"拦截时面板编辑对象不变，
            //树选中还原到当前编辑记录节点；确认通过则高亮跟随新编辑对象
            if (auto* panel = mPostInfoHub::getInstance()->existingMeasureToolPanel())
                syncMeasureHighlight(panel->currentEditingMeasureId());
            name = "结果";
            break;
        }
        //标签根/子节点→打开标签面板并切"结果"页（V1.3 原型 #annotationParameters 树分支）：
        //根=打开面板；子节点=面板选中该记录并回填表单（树节点点击=管理表行点击同一语义，
        //经 hub selectTagRecord 路由，面板公开选中接口已随本轮接入）
        case MxFunctions::PostMarkerRootItem:
        {
            slot_createTag();
            name = "结果";
            break;
        }
        case MxFunctions::PostMarkerChildItem:
        {
            slot_createTag();
            mPostInfoHub::getInstance()->selectTagRecord(phyID);
            name = "结果";
            break;
        }
        //分区显示已去节点化（2026-11-04）：着色经功能区"分区"按钮直作用并自切换
        //（slot_createPartition），原单击着色/退出分支随节点消失移除；
        //点击任意其它节点时若分区着色开启则退出（旗标判定，同原退出语义）
        default:
            break;
        }

        if (_partitionOverrideActive)
        {
            const auto& r = getCVM->getPostRenderWidget()->getPostRender();
            if (r)
            {
                r->setElementColorOverrideEnabled(false);
            }
            _partitionOverrideActive = false;
        }

        updateItemSelected();
        MechCommon::mMechGlobalSignals::getInstance()->browsertreeSelectedChanged(
            { _treeModel->itemKeyFromIndex(idx) }, true);
        MechCommon::mMechGlobalSignals::getInstance()->switchMenuBarWindow(name);
    }

    void mPostTreeView::updateItemSelected()
    {
        auto priv = VMManager->getCurrentViewportPrivate();
        if (!priv)
            return;

        QModelIndexList srcIdxs = selectedSourceIndexes();
        if (priv->toType<mxr::REBUILD_PostRend>() != nullptr)
        {
            if (srcIdxs.isEmpty()) return;
            priv->setTreeSelectItems({ _treeModel->itemKeyFromIndex(srcIdxs[0]) });
        }
        else
        {
            QList<mItemKey> keys;
            for (const QModelIndex& i : srcIdxs)
                keys << _treeModel->itemKeyFromIndex(i);
            priv->setTreeSelectItems(keys);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 分区显示颜色（自 mTreeView 移植）
    //////////////////////////////////////////////////////////////////////////
    namespace
    {
        QColor postPartitionColor(int processIndex)
        {
            // 使用一组区分度较高的颜色
            static const QVector<QColor> colors = {
                QColor(255, 0, 0),
                QColor(0, 255, 0),
                QColor(0, 0, 255),
                QColor(255, 255, 0),
                QColor(0, 255, 255),
                QColor(255, 0, 255),
                QColor(255, 128, 0),
                QColor(128, 0, 255),
                QColor(0, 128, 255),
                QColor(255, 0, 128)
            };
            if (colors.isEmpty()) return QColor(255, 255, 255);
            return colors[processIndex % colors.size()];
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 结果视口显示名接管（参考前处理"模型名· 视口名"）
    //////////////////////////////////////////////////////////////////////////
    QString mPostTreeView::renameItemViewportDisplay(MxFunctions::BrowserTreeType nodeType, const QString& itemName, const QString& viewportName)
    {
        const PostData::ItemType itemType = viewportLookupItemType(nodeType);
        if (itemType == static_cast<PostData::ItemType>(-1) || itemName.isEmpty() || viewportName.isEmpty())
            return QString();
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(
            getCVM->getModelData()->getModelDataID());
        if (vm == nullptr) return QString();
        const int vpID = vm->getViewportIDByName(viewportName);
        if (vpID < 0) return QString();

        const QString displayName = getCVM->getModelData()->getModelDataName()
            + QString::fromUtf8("· ") + itemName;
        //模型内重名等被拒绝时保留协议名返回空串，不阻断业务
        if (!vm->renameViewport(vpID, displayName))
        {
            return QString();
        }
        //条目 viewName 跟随：树↔视口反查（findNodeByViewportName）与单击派发
        //（editXxx(viewName)）都按管理器 viewName 取视口；guidata 挂在视口私有
        //对象上随视口走，renameViewport 已同步，无需迁移
        getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
            .resetItemViewName(itemType, itemName, displayName);
        return displayName;
    }

    QString mPostTreeView::solePost3DViewportName(int modelID)
    {
        //结构化定位（替代协议名字面量识别）：无云图 ⇒ 该模型唯一的 Post3D 视口即
        //初始视口（派生 3D 视口的创建都以云图节点存在为前提；曲线等 2D 视口不在
        //此列）。取其当前名（可能已被显示名接管）
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(modelID);
        if (vm == nullptr) return QString();
        const QList<int> ids = vm->getAllViewportIDs();
        QString found;
        for (int id : ids)
        {
            QWidget* sub = vm->getSubWindowByViewportID(id);
            auto* priv = sub ? vm->getViewportPrivate(sub) : nullptr;
            if (priv == nullptr || priv->getViewportType() != MxFunctions::ViewportPost3D)
                continue;
            if (!found.isEmpty())
                return QString();   //不止一个 Post3D 视口 ⇒ 已非"无云图"场景，交回调用方回退
            found = priv->getViewportName();
        }
        return found;
    }

    void mPostTreeView::updatePartitionElementColor(const QHash<int, std::set<int>>& partitionElements)
    {
        // 获取后处理渲染器，为空直接结束
        const auto& r = getCVM->getPostRenderWidget()->getPostRender();
        if (!r)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, "未获取到正确视口！", true);
            return;
        }

        // 启用单元颜色覆盖，避免在循环中重复调用
        r->setElementColorOverrideEnabled(true);

        // 遍历每个进程及其单元ID，按进程分配不同颜色
        for (auto it = partitionElements.cbegin(); it != partitionElements.cend(); ++it)
        {
            const int processIndex = it.key();
            const std::set<int>& elementIds = it.value();
            const QColor color = postPartitionColor(processIndex);
            std::vector<int> myVec(elementIds.begin(), elementIds.end());
            // 为每个进程设置单元颜色
            r->setElementColorOverride(myVec, { static_cast<float>(color.redF()), static_cast<float>(color.greenF()), static_cast<float>(color.blueF()) });
        }
    }

    mPostTreeView::mPostTreeView(QWidget* parent) : QTreeView(parent)
    {
        //选中背景变白：消除原生选中高亮（branch 缩进区的左侧蓝色块），选中样式由委托绘制（同前处理树）
        setStyleSheet(R"(
        QTreeView{
        border:none;
        }
        )");
        QPalette pal = this->palette();
        pal.setColor(QPalette::Highlight, Qt::white);
        this->setPalette(pal);

        //独立后处理树模型：本视图自建自持（不再经 browserTreeModelProvider 借前处理树实例）
        _treeModel = new mPostTreeModel(this);

        //派生工况定义集变更承接（批次A0）：store 无 QObject，经回调订阅；
        //面板 commit 逐键触发通知，300ms 防抖合并后重建"派生工况"分支
        _derivedRebuildTimer = new QTimer(this);
        _derivedRebuildTimer->setSingleShot(true);
        _derivedRebuildTimer->setInterval(300);
        connect(_derivedRebuildTimer, &QTimer::timeout, this, &mPostTreeView::slot_derivedStoreChanged);
        mPostDerivedCaseStore::instance().addChangeListener([this](int modelKey)
        {
            //非当前模型的桶变更不重建（其分支在其成为当前模型时由导入链重建）
            if (modelKey != _treeModel->getCurrentModelID())
                return;
            _pendingDerivedKey = modelKey;
            _derivedRebuildTimer->start();
        });

        //重命名管理器同步钩子注入（批次B2）：模型保持无头纯净，业务同步留在视图层。
        //云图树节点改名成功后视口显示名跟随（"{模型名}· {新节点名}"，参考前处理联动）
        _treeModel->renameValidator = [this](mPostTreeItem* item, const QString& newName)
        {
            const MxFunctions::BrowserTreeType type = mTreeItemUtil::treeTypeOfItem(_treeModel, item);
            const QString oldName = mTreeItemUtil::textOfItem(_treeModel, item);

            //集合子节点（五类）：库内哈希 key 迁移 + 树重建 + 面板编辑名同步
            switch (type)
            {
            case MxFunctions::PostSetElementChildItem:
            case MxFunctions::PostSetNodeChildItem:
            case MxFunctions::PostSetFaceChildItem:
            case MxFunctions::PostSetEdgeChildItem:
            case MxFunctions::PostSetPartChildItem:
            {
                mPostDataBase* postDB = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getPostDatabase();
                if (!postDB) return false;
                //重名拒绝：库内五张哈希任一命中（排除自身）
                const bool taken = postDB->getElementSets().contains(newName)
                    || postDB->getNodeSets().contains(newName)
                    || postDB->getFaceSets().contains(newName)
                    || postDB->getEdgeSets().contains(newName)
                    || postDB->getPartSets().contains(newName);
                if (taken && newName != oldName)
                {
                    mGlobalSignals::getInstance()->outputMessageSig(QString("集合名称 %1 已存在").arg(newName), false);
                    return false;
                }
                //key 迁移（按原类型哈希；名字在库内唯一，从所在哈希搬家即可）
                auto migrate = [postDB, &oldName, &newName](auto getter, auto setter)
                {
                    auto sets = (postDB->*getter)();
                    if (sets.contains(oldName))
                    {
                        auto data = sets.value(oldName);
                        sets.remove(oldName);
                        sets.insert(newName, data);
                        (postDB->*setter)(sets);
                    }
                };
                migrate(&mPostDataBase::getElementSets, &mPostDataBase::setElementSets);
                migrate(&mPostDataBase::getNodeSets, &mPostDataBase::setNodeSets);
                migrate(&mPostDataBase::getFaceSets, &mPostDataBase::setFaceSets);
                migrate(&mPostDataBase::getEdgeSets, &mPostDataBase::setEdgeSets);
                migrate(&mPostDataBase::getPartSets, &mPostDataBase::setPartSets);

                //树集合子树按库重建（新名生效）；面板若正编辑该集合同步当前名
                MechCommon::mMechGlobalSignals::getInstance()->updatePostModelTree();
                mPostInfoHub::getInstance()->notifySetRenamed(oldName, newName);
                return true;
            }
            //用户坐标系节点：会话记录查重+改名（面板单一数据源，编辑名同步）
            case MxFunctions::PostCoordinateChildItem:
            {
                //全局坐标系不可重命名（对齐 removeRows 拒删同款守卫语义）
                if (mTreeItemUtil::phyIdOfItem(_treeModel, item) == PostGlobalCoordinatePhyID)
                    return false;
                const int coordId = mTreeItemUtil::phyIdOfItem(_treeModel, item);
                const auto records = mPostInfoHub::getInstance()->coordinateTreeRecords();
                for (const auto& rec : records)
                {
                    if (rec.first != coordId && rec.second == newName)
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(
                            QString::fromUtf8("坐标系名称 %1 已存在").arg(newName), false);
                        return false;
                    }
                }
                //树节点文本由 setData 正常路径更新；记录与面板编辑名经 hub 同步
                mPostInfoHub::getInstance()->renameCoordinateRecord(coordId, newName);
                return true;
            }
            default:
                break;
            }

            if (!postItemRenameSync(type, oldName, newName))
                return false;
            if (type == MxFunctions::threeDResultParentItem)
            {
                const QString vp = getCVM->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
                    .getViewNameByID(PostData::CloudItem, mTreeItemUtil::phyIdOfItem(_treeModel, item));
                if (!vp.isEmpty())
                {
                    if (const QString displayName = renameItemViewportDisplay(type, newName, vp);
                        !displayName.isEmpty())
                    {
                        //改名后同步面板当前名：面板 _curViewName 持字符串，不同步则
                        //后续按旧名查找（getPostRenderByViewName 等）落空
                        MechCommon::mMechGlobalSignals::getInstance()->updatePostViewName(displayName);
                    }
                }
                else
                {
                    //缓存节点（无独立视口）：快照 _presetName 跟随节点名，
                    //否则 restoreCloudPreset 按名反查落空
                    syncCloudPresetName(oldName, newName);
                }
            }
            return true;
        };

        _proxy = new mPostTreeProxy(this);
        _proxy->setSourceModel(_treeModel);

        setModel(_proxy);
        setItemDelegate(new mTreeDelegate(this));
        setHeaderHidden(true);
        setUniformRowHeights(true);
        setAnimated(false);
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        //单击/右键均为本视图的后处理专属派发
        connect(this, &QTreeView::clicked, this, [this](const QModelIndex&) {
            slot_singleClickItem();
        });
        connect(this, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
            QModelIndex proxyIdx = indexAt(pos);
            if (!proxyIdx.isValid()) return;
            //右键菜单按选中集合工作：右键行未选中时先把选中收敛到该行（同模型树右键行为）
            if (!selectionModel()->isSelected(proxyIdx))
                setCurrentIndex(proxyIdx);
            slot_rightClickItem();
        });

        //全局信号（自 mTreeView initConnect 迁移）：后处理结果的创建/树更新驱动
        auto* globalSig = MechCommon::mMechGlobalSignals::getInstance();
        connect(globalSig, &MechCommon::mMechGlobalSignals::click3DResultSig, this, &mPostTreeView::slot_switchCloudPanel);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostCurveSig, this, &mPostTreeView::slot_createCurve);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostExportWordSig, this, &mPostTreeView::slot_createExportWord);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostParrayMimageSig, this, &mPostTreeView::slot_createParrayMimage);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostCompVideoSig, this, &mPostTreeView::slot_createCompVideo);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostExplodeSig, this, &mPostTreeView::slot_createExplode);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostMeasureToolSig, this, &mPostTreeView::slot_createMeasureTool);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostResultPathSig, this, &mPostTreeView::slot_createResultPath);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostSectionForceSig, this, &mPostTreeView::slot_createSectionForce);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostLabelSig, this, &mPostTreeView::slot_createLabel);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostTagSig, this, &mPostTreeView::slot_createTag);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPathSig, this, &mPostTreeView::slot_createPath);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickAnimationSig, this, &mPostTreeView::slot_createAnimation);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickLinearSuperpositionSig, this, &mPostTreeView::slot_createLinearSuperposition);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickScalarOperationSig, this, &mPostTreeView::slot_createScalarOperation);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickSectionSig, this, &mPostTreeView::slot_createSection);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickIsosurfaceSig, this, &mPostTreeView::slot_createIsosurface);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickVectorSig, this, &mPostTreeView::slot_createVector);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickStreamLineSig, this, &mPostTreeView::slot_createStreamLine);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPartitionSig, this, &mPostTreeView::slot_createPartition);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostSetSig, this, &mPostTreeView::slot_createPostSet);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostSetCreateSig, this, &mPostTreeView::slot_createPostSetTyped);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostCoordinateCreateSig, this, &mPostTreeView::slot_createPostCoordinate);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickLineSig, this, &mPostTreeView::slot_createLine);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickArrowSig, this, &mPostTreeView::slot_createArrow);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickTextSig, this, &mPostTreeView::slot_createText);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickLogSig, this, &mPostTreeView::slot_createLog);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createAnimationSig, this, &mPostTreeView::slot_updateAnimationTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createLinearSuperpositionSig, this, &mPostTreeView::slot_updateLinearSuperpositionTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createScalarOperationSig, this, &mPostTreeView::slot_updateScalarOperationTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createSectionSig, this, &mPostTreeView::slot_updateSectionTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createIsosurfaceSig, this, &mPostTreeView::slot_updateIsosurfaceTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createCloudPresetSig, this, &mPostTreeView::slot_updateCloudPresetTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createVectorSig, this, &mPostTreeView::slot_updateVectorTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createStreamLineSig, this, &mPostTreeView::slot_updateStreamLineTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createPostSetSig, this, &mPostTreeView::slot_updatePostSetTree);
        //createCurveSig/createPathSig/createExportWordSig 接线随去节点化移除：
        //曲线/路径/Word报告面板自登记实例（newXxxPanel 内完成），不再经树槽建节点
        connect(globalSig, &MechCommon::mMechGlobalSignals::createLineSig, this, &mPostTreeView::slot_updateLineTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createArrowSig, this, &mPostTreeView::slot_updateArrowTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createTextSig, this, &mPostTreeView::slot_updateTextTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::createLogSig, this, &mPostTreeView::slot_updateLogTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::updatePostModelTree, this, &mPostTreeView::slot_updatePostModelTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::updatePostBrowserTreeStatus, this, &mPostTreeView::slot_updatePostBrowserTreeStatus);
        //测量记录变更（测量面板镜像写回后发射）："测量"分支子节点同步
        connect(globalSig, &MechCommon::mMechGlobalSignals::postMeasureRecordsChangedSig, this, &mPostTreeView::slot_updateMeasureTree);
        //测量编辑对象变化（选中切换/新建登记/删除回落）："测量"分支高亮跟随
        connect(globalSig, &MechCommon::mMechGlobalSignals::measureEditingChangedSig, this, &mPostTreeView::slot_measureEditingChanged);
        //标签记录变更（标签面板记录结构变化后发射）："标签"分支子节点同步
        connect(globalSig, &MechCommon::mMechGlobalSignals::postTagRecordsChangedSig, this, &mPostTreeView::slot_updateTagTree);
        //坐标系数据变更（面板创建/重命名、树侧菜单删除/复制后发射；前处理同款全局
        //信号，此前零消费端——后处理树为本信号唯一消费方）："坐标系"分支子节点同步
        connect(globalSig, &MechCommon::mMechGlobalSignals::createUpdateCoordinateSig, this, &mPostTreeView::slot_updatePostCoordinateTree);

        //跨树信号承接（原接于前处理树 mTreeView，树模型独立后改由本树响应）：
        //云图节点创建（按本树选中上下文）/ 注释节点创建 / 后处理模型树根移除
        connect(globalSig, &MechCommon::mMechGlobalSignals::create3DResultSig, this, &mPostTreeView::updateResultTree);
        connect(globalSig, &MechCommon::mMechGlobalSignals::clickPostNoteSig, this, &mPostTreeView::slot_createNote);
        connect(globalSig, &MechCommon::mMechGlobalSignals::removePostModelTreeSig, this, &mPostTreeView::slot_removePostModelTree);

        //视口激活联动：本模型独立后，currentModelID 与选中记忆恢复由本树自管
        //（原由前处理树 mTreeView 在共享模型实例上代管）
        connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged,
            this, &mPostTreeView::slot_viewportActivated);

        //插入后仅展开受影响分支（批次C）：原顶层插入 expandAll 会覆盖用户手动折叠、
        //批量导入大模型时 O(N) 反复全展开；新顶层模型根只展开自身，子层插入展开父链
        connect(_treeModel, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex& parent, int first, int last) {
            if (!parent.isValid())
            {
                for (int r = first; r <= last; ++r)
                {
                    QModelIndex proxyIdx = _proxy->mapFromSource(_treeModel->index(r, 0, QModelIndex()));
                    if (proxyIdx.isValid())
                        expand(proxyIdx);
                }
            }
            else
            {
                QModelIndex proxyParent = _proxy->mapFromSource(parent);
                for (QModelIndex p = proxyParent; p.isValid(); p = p.parent())
                    expand(p);
            }
        });
        expandAll();
    }

    void mPostTreeView::newPostModelBrowserTree()
    {
        //树名取模型数据名称（模型创建时已命名写入 modelData，删除模型后不再编号错乱）
        auto* modelData = getCVM->getModelData();
        _treeModel->addPostModelTree(modelData->getModelDataID(), modelData->getModelDataName());
    }

    void mPostTreeView::addPostModelBrowserTree()
    {
        newPostModelBrowserTree();
        auto* item = _treeModel->getParentItem(PostTreeRootItem);
        this->collapseAll();
        //新开后处理模型即成为当前模型：树范围收敛到它（隐藏既有其它模型的根）
        scopeToCurrentModel();
        selectedItem(item);
    }

    void mPostTreeView::deletePostModelBrowserTree()
    {
        //必须先记录待删模型ID：deleteViewportModel 期间重激活链同步执行
        //（currentViewportChanged(-1) → 视图树 activateFirstAvailableViewport → 本树
        //slot_viewportActivated），currentModelID 已被切回前处理模型——若删除后才按
        //currentModelID 查根（getParentItem 按 {_currentModelID, treeType} 查索引）必落空，
        //removeTreeItem(nullptr) 静默失败，树根残留
        int deleteModelID = getCVM->getModelData()->getModelDataID();
        mGlobalSignals::getInstance()->deleteViewportModel();
        //按显式模型ID移除树根（同 removePostModelTreeSig 承接路径 slot_removePostModelTree，
        //含结果跳转登记清理）；删除后回退到前处理模型的 currentModelID 同步由重激活链
        //（slot_viewportActivated）完成
        slot_removePostModelTree(deleteModelID);
    }

    void mPostTreeView::deletePostModelBrowserTree(int modelDataID)
    {
        //按显式模型ID删除（导入失败回滚路径）：不经 getCVM 当前模型；树根移除经
        //slot_removePostModelTree 同链路（含结果跳转登记清理）
        mViewTreeFunction::deleteViewportModel(modelDataID);
        slot_removePostModelTree(modelDataID);
    }

    mPostBrowserWindow* mPostBrowserWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mPostBrowserWindow;
            _instance->setObjectName("PostTreeDock");
            _instance->setMinimumWidth(220);
        }
        return _instance;
    }

    mPostBrowserWindow::mPostBrowserWindow(QWidget* parent) : QWidget(parent)
    {
        //会话协调器最先实例化：其对 updateResultTreeSig 的连接必须先于下方
        //本窗口的同名连接（导入完成时会话回放先运行，见 mPostSessionCoordinator 头注释）；
        //构造内自注册到 MechCommon::postSessionParticipantRef()
        mPostSessionCoordinator::getInstance();

        QPalette pal;
        pal.setColor(QPalette::Background, QColor(255, 255, 255));
        setAutoFillBackground(true);
        setPalette(pal);

        //打开后处理结果文件（入口在mMenuWidget主页文件菜单/后处理按钮，经全局信号解耦；
        //建/删后处理树入口本就在后处理树窗口，故由此处执行）
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickOpenPostCloudFileSig, this, &mPostBrowserWindow::slot_openPostFile);
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickOpenPostNVHFileSig, this, &mPostBrowserWindow::slot_openPostNVHFile);

        //计算完成跳转后处理（求解接入方 emit solveFinishedSig 携结果文件路径；
        //导入完成后后处理视口自动激活，Ribbon/dock 经 currentViewportChanged 联动切到后处理模式）
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::solveFinishedSig,
            this, &mPostBrowserWindow::openPostResultFiles);

        //幂等再次跳转（requestShowPostResultSig）：命中注册表则激活已有云图视口，不重复建模型
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::requestShowPostResultSig,
            this, &mPostBrowserWindow::slot_showPostResult);

        //对外结果服务注册（前处理经 PostCommon::postResultServiceRef() 判空调用；
        //本单例进程级存活，无需反注册）
        PostCommon::postResultServiceRef() = this;

        //实例失效通知单点接线：注册表条目被作废（模型删除/漂移）→ 总线信号通知前处理作废旧key。
        //mtime"删旧重建该key实例"走 takeByKey 静默取出+原key重登记，不经过本回调
        _resultRegistry.onKeyInvalidated = [](const QString& key)
        {
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultRemovedSig(key);
        };

        //异步导入失败回滚：失败发生在读取线程完成之后（import3DResult 已返回true、
        //模型/树根/key 均已建），漏斗同步段的回滚够不到，由此按失败通知携带的宿主
        //模型ID回收（判据与动作见槽内）。会话恢复协调器对本信号的连接在先（其构造
        //于本窗口构造最先处执行），恢复条目的回滚由协调器自理后本槽按无登记跳过
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::postResultImportFailedSig,
            this, &mPostBrowserWindow::slot_postImportFailedRollback);

        //视口改名三层同步·集中同步器（批次2）：renameViewport 已同步视口层（priv 名/
        //渲染器键/标题栏/视图树文本），此处补管理器层与派生层——①该模型管理器全部
        //条目类型的 viewName old→new（树↔视口反查与树单击派发按它取视口）；②以
        //oldName 为前缀的派生视口（oldName_KindN）前缀替换级联改名。级联 rename 会
        //再次进入本槽：此时①把派生条目的 viewName 一并同步，②对新前缀不再命中
        //oldName 前缀 → 一层递归终止无环。面板层的 _curViewName 跟随由 mPanelBase
        //注册表经 hub 桥接（批次3）
        connect(MechPreData::mViewportModelManager::getInstance(),
            &MechPreData::mViewportModelManager::sig_viewportRenamed,
            this, [](int viewportID, const QString& oldName, const QString& newName)
        {
            if (oldName.isEmpty() || oldName == newName) return;
            auto* vmm = MechPreData::mViewportModelManager::getInstance();
            //按视口ID定位所属模型（改名可能来自任一模型的重命名链），仅后处理模型有管理器数据
            MechPreData::mViewportModel* owner = nullptr;
            for (int mid : vmm->getAllModelDataID())
            {
                if (auto* vm = vmm->getViewportModel(mid);
                    vm != nullptr && vm->getAllViewportIDs().contains(viewportID))
                {
                    owner = vm;
                    break;
                }
            }
            if (owner == nullptr || !owner->isPostModel()) return;
            auto& postMgr = owner->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post);
            const PostData::ItemType types[] = {
                PostData::CloudItem, PostData::AnimationItem, PostData::LinearItem,
                PostData::ScalarOperationItem, PostData::SectionItem, PostData::IsosurfaceItem,
                PostData::VectorItem, PostData::StreamLineItem, PostData::SetItem,
                PostData::CurveItem, PostData::ExportWordItem, PostData::PathItem,
                PostData::LineItem, PostData::ArrowItem, PostData::TextItem, PostData::LogItem };
            for (PostData::ItemType t : types)
            {
                postMgr.resetItemViewNameByView(t, oldName, newName);
            }
            for (int vp : owner->getAllViewportIDs())
            {
                QWidget* sub = owner->getSubWindowByViewportID(vp);
                auto* priv = sub ? owner->getViewportPrivate(sub) : nullptr;
                if (priv == nullptr) continue;
                const QString cascaded = mViewportName::cascadeDerivedName(priv->getViewportName(), oldName, newName);
                if (!cascaded.isEmpty())
                {
                    owner->renameViewport(vp, cascaded);
                }
            }
        });

        //结果树整体刷新（原mPost3DResult直调getPostTree()->updateResultTree，经信号解耦）
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::updateResultTreeSig, this, [this]()
        {
            getPostTree()->updateResultTree();
        });

        //贡献量两族（原mInfoWindow接收端，随面板迁入mPostInfoHub；模态贡献量
        //旧链下线 openspec §8.1——新链=曲线 Ribbon 诊断组）
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickPanelContributionSig, this, [this]() { mPostInfoHub::getInstance()->panelContributionAmount(); });
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickPathContributionSig, this, [this]() { mPostInfoHub::getInstance()->pathContributionAmount(); });

        //曲线页菜单动作（原mInfoWindow接收端，随曲线面板迁入mPostInfoHub改由此接收）
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickShowTableDataSig, this, [this]()
        {
            mPostInfoHub::getInstance()->showTableData();
        });
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::clickUpdateCurveDisplaySig, this, [this]()
        {
            mPostInfoHub::getInstance()->updateDisplayCurve();
        });

        //启动期预热mPostInfoHub：主页截屏双钮/主窗resize·move重绘等MainWindow侧信号的唯一接收端
        //在该懒加载单例构造时才接线；纯前处理会话若不预热则全程无接收端（截屏按钮点击无效）。
        //本窗口经postTreeDockProvider在主窗口dock注册时必然构造，是PostWindow侧的启动期锚点。
        mPostInfoHub::getInstance();

        _treeView = new mPostTreeView(this);

        //过滤/排序工具条：与前处理树窗口（mBrowserWindow）保持一致，
        // 全部按 UiMetrics/UiPalette 派生（规则 0.1/0.2/2.1）：行高 = control = H、
        // 字号 = body、边线/悬停/选中取语义色板，切档切主题随动
        _filterLabel = new QLabel("过滤：", this);
        _filterLabel->setFixedWidth(40);

        _nameComboBox = new QComboBox(this);
        _nameComboBox->addItems({ "名称", "状态" });
        _findLineEdit = new QLineEdit(this);
        _findLineEdit->setPlaceholderText("搜索概要");

        _statusComboBox = new QComboBox(this);
        _statusComboBox->addItems({ "全部", "正确", "警告", "错误" });

        _stackedWidget = new QStackedWidget(this);
        _stackedWidget->addWidget(_findLineEdit);
        _stackedWidget->addWidget(_statusComboBox);

        _sortLabel = new QLabel("排序：", this);
        _sortLabel->setFixedWidth(40);
        _sortTypeComboBox = new QComboBox(this);
        _sortTypeComboBox->addItems({ "拼音/字母", "后缀序号" });

        _sortTool = new QToolBar(this);
        _sortTool->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QAction* action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/moren.png"), "默认");
        connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(0, Qt::AscendingOrder);
            });
        action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/shengxu.png"), "升序");
        connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(_sortTypeComboBox->currentIndex() + 1, Qt::AscendingOrder);
            });
        action = _sortTool->addAction(QIcon(":/mMainWidgets/Icon/CenterMainWindow/jiangxu.png"), "降序");
        connect(action, &QAction::triggered, this, [this]() {
            _treeView->sortTree(_sortTypeComboBox->currentIndex() + 1, Qt::DescendingOrder);
            });

        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(5);
        gridLayout->setContentsMargins(15, 0, 0, 0);

        QHBoxLayout* searchLayout = new QHBoxLayout();
        searchLayout->addWidget(_stackedWidget);
        searchLayout->addStretch(1);

        QHBoxLayout* toolLayout = new QHBoxLayout();
        toolLayout->addWidget(_sortTool);
        toolLayout->addStretch(1);

        gridLayout->addWidget(_filterLabel, 0, 0, Qt::AlignVCenter);
        gridLayout->addWidget(_nameComboBox, 0, 1, Qt::AlignVCenter);
        gridLayout->addLayout(searchLayout, 0, 2, Qt::AlignVCenter);
        gridLayout->addWidget(_sortLabel, 1, 0, Qt::AlignVCenter);
        gridLayout->addWidget(_sortTypeComboBox, 1, 1, Qt::AlignVCenter);
        gridLayout->addLayout(toolLayout, 1, 2, Qt::AlignVCenter);

        gridLayout->setColumnStretch(0, 0);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 1);

        _nameComboBox->setMinimumWidth(70);
        _sortTypeComboBox->setMinimumWidth(70);
        _findLineEdit->setMinimumWidth(90);

        connect(_nameComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                _findLineEdit->clear();
                _statusComboBox->blockSignals(true);
                _statusComboBox->setCurrentIndex(0);
                _statusComboBox->blockSignals(false);

                if (index == 0)
                    _stackedWidget->setCurrentIndex(0);
                else
                    _stackedWidget->setCurrentIndex(1);

                _treeView->nameFilterTree(_findLineEdit->text());
            });

        connect(_findLineEdit, &QLineEdit::editingFinished, this,
            [this]() {
                _treeView->nameFilterTree(_findLineEdit->text());
            });

        connect(_statusComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (index == 0)
                    _treeView->nameFilterTree("");
                else if (index == 1)
                    _treeView->statusFilterTree(Tree_Yes);
                else if (index == 2)
                    _treeView->statusFilterTree(Tree_Failure);
                else if (index == 3)
                    _treeView->statusFilterTree(Tree_Update);
            });

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 5, 0, 0);
        mainLayout->addLayout(gridLayout, 0);
        QHBoxLayout* treeLayout = new QHBoxLayout();
        treeLayout->addSpacing(10);
        treeLayout->addWidget(_treeView);
        mainLayout->addLayout(treeLayout, 1);

        _treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 首次按当前档位/主题下发工具条样式（此后由 ThemeManager::applyAll 统一重发）
        refreshToolBarTheme();
    }

    void mPostBrowserWindow::refreshToolBarTheme()
    {
        // 过滤/排序控件的主题/档位统一下发（规则 0.1/0.2/2.1）：与前处理树窗口
        // mBrowserWindow 同构，共用 ThemeManager::applyTreeFilterBarTheme 一份实现。幂等。
        MechMainWindow::ThemeManager::TreeFilterBar bar;
        bar.nameCombo = _nameComboBox;
        bar.statusCombo = _statusComboBox;
        bar.sortTypeCombo = _sortTypeComboBox;
        bar.findEdit = _findLineEdit;
        bar.filterLabel = _filterLabel;
        bar.sortLabel = _sortLabel;
        bar.sortTool = _sortTool;
        bar.paletteHost = this;
        MechMainWindow::ThemeManager::applyTreeFilterBarTheme(bar);
    }

    mPostBrowserWindow::~mPostBrowserWindow()
    {
        if (_treeView)
        {
            delete _treeView;
            _treeView = nullptr;
        }
    }

    void mPostBrowserWindow::openPostResultFiles(const QStringList& names)
    {
        //信号路径入口（solveFinishedSig/手动打开共用）：激活式创建+登记，
        //返回的实例key由接口调用方持有，本路径不使用
        createAndRegisterResult(names, /*activateNow=*/true);
    }

    QStringList mPostBrowserWindow::importedResultFilesOfModel(int modelDataID) const
    {
        //原始导入清单（对话框多选的 mxdb0+帧文件完整组合）：回放导入行为与首次一致
        if (const auto* record = _resultRegistry.findByPostModelID(modelDataID))
        {
            if (!record->filePaths.isEmpty())
                return record->filePaths;
        }
        //回退：数据层记录的数据库文件（native 格式单文件即完整选择；
        //旧格式缺帧文件，回放会被过滤器拒绝并跳过——注册表缺登记属异常路径）
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(modelDataID);
        if (vm == nullptr || vm->getModelData() == nullptr)
            return QStringList();
        return vm->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post)
            .getAllDatabaseFile();
    }

    void mPostBrowserWindow::registerSessionRestoredResult(int postModelID, const QStringList& filePaths)
    {
        if (filePaths.isEmpty())
            return;
        auto* vm = MechPreData::mViewportModelManager::getInstance()->getViewportModel(postModelID);
        if (vm == nullptr)
            return;

        //与 createAndRegisterResult 同构的登记（key=模型ID串，字段含义见彼处注释）；
        //恢复链直导不经漏斗，缺此登记会使"恢复态再保存"的清单回退到不完整数据层
        mPostResultRegistry::PostJumpRecord record;
        record.filePaths = filePaths;
        record.postModelID = postModelID;
        //与漏斗登记同口径（恢复链无显式伴随网格，modelFilePath 恒空）
        record.fileMtime = record.currentMonitoredMtime();
        const QList<int> ids = vm->getAllViewportIDs();
        if (!ids.isEmpty())
            record.mainViewportID = ids.first();
        if (record.mainViewportID < 0)
            return;
        _resultRegistry.insert(QString::number(postModelID), record);
    }

    QString mPostBrowserWindow::createAndRegisterResult(const QStringList& names, bool activateNow,
        const QString& reuseKey, const QString& modelFilePath, QString* syncErrorDetail)
    {
        if (syncErrorDetail != nullptr)
            syncErrorDetail->clear();   //防调用方旧值残留：出参仅在返回空key（同步失败）时有效

        if (names.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, "未指定后处理结果文件！", true);
            if (syncErrorDetail != nullptr)
                *syncErrorDetail = QStringLiteral("未指定后处理结果文件");
            return QString();
        }

        //会话恢复结果重导入进行中：拒绝新的手动导入——手动导入完成同样发
        //updateResultTreeSig，会被恢复协调器误当作队列条目完成（提前弹条目、
        //切当前模型，可能跨模型串数据）。会话恢复链不经本漏斗（协调器直调
        //import3DResult），此守卫不影响恢复自身
        if (auto* postParticipant = MechCommon::postSessionParticipantRef();
            postParticipant != nullptr && postParticipant->pendingPostRestoreCount() > 0)
        {
            mGlobalSignals::getInstance()->outputMessageSig(
                QString("后处理结果数据仍在恢复中，请稍候再导入结果文件"), true);
            if (syncErrorDetail != nullptr)
                *syncErrorDetail = QStringLiteral("后处理结果数据仍在恢复中，请稍候再导入");
            return QString();
        }

        //仅创建不切换：导入收尾会自动激活新建的云图视口，先记下当前视口以便创建后切回
        const int prevViewportID = activateNow ? -1
            : MechPreData::mViewportModelManager::getInstance()->currentViewportID();

        //创建后处理模型树及视口（后处理树的建/删入口在后处理树窗口）
        const int postModelID = mViewTreeFunction::newPostViewportModel();
        getPostTree()->addPostModelBrowserTree();

        //读取文件
        bool res = mPostInfoHub::getInstance()->import3DResult(names, -1, modelFilePath);

        //文件读取失败删除模型树（按显式ID回滚，规避失败瞬间当前模型漂移的时序风险）
        if (!res)
        {
            getPostTree()->deletePostModelBrowserTree(postModelID);
            if (syncErrorDetail != nullptr)
                *syncErrorDetail = QStringLiteral("结果文件导入失败（格式组合不受支持或文件不可读）");
            return QString();
        }

        //登记结果跳转注册表：key=创建实例的modelDataID十进制串、创建后冻结（reuseKey非空=
        //mtime"删旧重建该key实例"，保留原key——此后key≠记录中postModelID，绑定由注册表维护）。
        //入口视口取刚创建模型中的唯一视口（newPostViewportModel 建的 Post_Cloud 视口；
        //导入收尾激活的也是它），后续用户再建的云图视口不参与"再次跳转"定位
        mPostResultRegistry::PostJumpRecord record;
        record.filePaths = names;
        record.postModelID = postModelID;
        record.modelFilePath = modelFilePath;
        //mtime 口径单一事实源：全部结果文件+显式伴随网格的最大值（bdf重写而op2
        //未变时同样触发重放重建；缺失文件计0）——漂移检测侧同用 currentMonitoredMtime
        record.fileMtime = record.currentMonitoredMtime();
        if (auto* viewportModel = MechPreData::mViewportModelManager::getInstance()->getViewportModel(postModelID))
        {
            const QList<int> ids = viewportModel->getAllViewportIDs();
            if (!ids.isEmpty())
            {
                record.mainViewportID = ids.first();
            }
        }
        if (record.mainViewportID < 0)
        {
            //无入口视口属异常（newPostViewportModel 必建 Post_Cloud 视口）：与导入失败
            //分支同收尾回滚（避免遗留孤儿模型树）、不登记、不返回key
            getPostTree()->deletePostModelBrowserTree(postModelID);
            if (syncErrorDetail != nullptr)
                *syncErrorDetail = QStringLiteral("后处理实例创建异常（未找到入口视口）");
            return QString();
        }

        const QString key = reuseKey.isEmpty() ? QString::number(postModelID) : reuseKey;
        _resultRegistry.insert(key, record);

        if (!activateNow && prevViewportID >= 0)
        {
            //切回创建前的视口：前处理界面保持不动，留待后续 showPostResult 按 key 跳转
            MechPreData::mViewportModelManager::getInstance()->activateViewportByID(prevViewportID);
        }
        return key;
    }

    PostCommon::PostResultError mPostBrowserWindow::showPostResultByKey(const QString& key)
    {
        const auto* record = _resultRegistry.find(key);
        if (record == nullptr)
        {
            return PostCommon::PostResultError::InvalidKey;
        }

        auto* viewportModel = MechPreData::mViewportModelManager::getInstance()->getViewportModel(record->postModelID);
        if (viewportModel == nullptr)
        {
            //登记指向的模型已被删除：作废登记并报错（clearViewportModel 已守卫拒绝后处理
            //模型，就地换ID漂移不可达，删除是唯一种真失效）。
            //作废触发失效信号通知前处理；不自动重建——重新导入是调用方用户的显式动作
            _resultRegistry.invalidateKey(key);
            return PostCommon::PostResultError::InvalidKey;
        }

        //结果文件已更新（前处理重算覆盖）：删旧模型，按原key"删旧重建该实例"后切换。
        //先静默取出登记再删模型——删除链路（removePostModelTreeSig 接收端）会经
        //unregisterResultJump 反查作废，不先取出会让存活的key误发失效信号
        //（mtime 与登记侧同用 currentMonitoredMtime 口径，含显式伴随网格——
        //两侧不对称会造成误重建/漏重建，见 PostJumpRecord 注释）
        if (record->currentMonitoredMtime() != record->fileMtime)
        {
            const int staleModelID = record->postModelID;
            const QStringList paths = record->filePaths;
            const QString modelFile = record->modelFilePath;   //重建重放沿用创建时的显式伴随网格
            _resultRegistry.takeByKey(key);

            //树根移除经总线（与视图树删除同链路），再删模型（内部会转移激活到前处理）
            emit MechCommon::mMechGlobalSignals::getInstance()->removePostModelTreeSig(staleModelID);
            mViewTreeFunction::deleteViewportModel(staleModelID);

            if (createAndRegisterResult(paths, /*activateNow=*/true, /*reuseKey=*/key,
                    /*modelFilePath=*/modelFile).isEmpty())
            {
                return PostCommon::PostResultError::InvalidKey;   //重建失败（如结果文件已丢失）
            }
            return PostCommon::PostResultError::Ok;
        }

        //命中：激活入口云图视口（激活链自动切 Ribbon/dock/树选中记忆，无需额外代码）
        bool activated = false;
        for (int vpID : viewportModel->getAllViewportIDs())
        {
            if (vpID == record->mainViewportID)
            {
                MechPreData::mViewportModelManager::getInstance()->activateViewportByID(vpID);
                activated = true;
                break;
            }
        }
        //入口视口已被单独删除：回退为激活该模型任一后处理视口
        if (!activated)
        {
            MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(record->postModelID);
            MechPreData::mViewportModelManager::getInstance()->activatePostViewport();
        }
        return PostCommon::PostResultError::Ok;
    }

    QString mPostBrowserWindow::createPostResult(const QStringList& resultFilePaths,
        const PostCommon::PostResultCreateOptions& options, QString* syncErrorDetail)
    {
        return createAndRegisterResult(resultFilePaths, options.activateNow,
            /*reuseKey=*/QString(), options.modelFilePath, syncErrorDetail);
    }

    PostCommon::PostResultError mPostBrowserWindow::showPostResult(const QString& resultKey)
    {
        return showPostResultByKey(resultKey);
    }

    void mPostBrowserWindow::slot_showPostResult(const QString& resultKey, const QStringList& resultFilePaths)
    {
        //信号路径服务于无key的求解完成自动跳转：命中即跳；未命中/失效/mtime更新
        //回退为"新建+导入并登记新key"（与接口 showPostResult 的"InvalidKey报错不重建"不同）
        if (showPostResultByKey(resultKey) == PostCommon::PostResultError::Ok)
        {
            return;
        }
        openPostResultFiles(resultFilePaths);
    }

    void mPostBrowserWindow::unregisterResultJump(int postModelID)
    {
        //模型删除路径反查作废（每移除一条登记触发一次失效信号，见 _resultRegistry）
        _resultRegistry.removeByPostModelID(postModelID);
    }

    void mPostBrowserWindow::slot_postImportFailedRollback(int postModelID)
    {
        //捕获缺失（-1）或无登记=按工况更新/会话恢复等向既有模型补导——回滚会误删
        //用户模型，维持只删库现状（删库已在 mPost3DResult 失败分支按捕获ID完成）
        if (postModelID < 0 || _resultRegistry.findByPostModelID(postModelID) == nullptr)
            return;

        //契约顺序"CreateFailed先于Removed"：removeByPostModelID 在移除完成后会在内部
        //逐key触发 onKeyInvalidated（→postResultRemovedSig"实例被删"），先静默取出
        //登记再手动补发，保证"导入失败"语义信号先到（前处理收到任一即作废key）
        const auto savedInvalidated = _resultRegistry.onKeyInvalidated;
        _resultRegistry.onKeyInvalidated = nullptr;
        const QStringList keys = _resultRegistry.removeByPostModelID(postModelID);
        _resultRegistry.onKeyInvalidated = savedInvalidated;
        for (const QString& k : keys)
        {
            emit MechCommon::mMechGlobalSignals::getInstance()->postResultCreateFailedSig(k);
            if (savedInvalidated)
                savedInvalidated(k);   //→总线 postResultRemovedSig
        }

        //树根移除经总线（接收端 unregisterResultJump 会再查注册表，已清空无副作用），
        //再删模型：与 mtime"删旧重建"同款时序（先树信号后删模型，激活转移由删除链
        //处理，回滚仅按捕获ID操作、不碰当前模型）
        emit MechCommon::mMechGlobalSignals::getInstance()->removePostModelTreeSig(postModelID);
        mViewTreeFunction::deleteViewportModel(postModelID);
    }

    void mPostBrowserWindow::slot_openPostFile()
    {
        //自 mMenuWidget::slot_openPostFile 迁入（入口改经clickOpenPostCloudFileSig全局信号）
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString title;
        if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 0)
        {
            title = "打开文件";
        }
        else if (MAppConfiguration::mAppConfiguration::getInstance()->_language == 1)
        {
            title = "Open file";
        }
        // d3plot 主文件无扩展名（文件名即 d3plot），*.d3plot 匹配不到，需用 d3plot* 模式
        QStringList names = QFileDialog::getOpenFileNames(this, title, defaultDir, QString("Results File(*.mxdb0 *.mxdb *.op2 *.bdf *.rst *.rth *.odb *.h3d d3plot* *.vtk *.vtp *.vtu *.vtm *.pvtp *.pvtu)"));
        if (names.size() < 1)
            return;

        openPostResultFiles(names);
    }

    void mPostBrowserWindow::slot_openPostNVHFile()
    {
        //自 mMenuWidget::slot_openPostNVHFile 迁入。此处只弹对话框，不提前切树/切菜单：
        //曲线上下文（菜单落"曲线"页 + 三棵树互斥切到曲线树）延迟到曲线图表真正
        //创建后由激活链自动完成——图表卡片创建即激活（sig_requestChartViewport ->
        //setActiveViewport）-> mMechMainWindow 卡片激活联动 setWorkMode(Curve) +
        //updateTreeWindows(ViewportCurve)。提前切换会在用户取消导入时留下"曲线上下文
        //却无曲线视口"的悬空态：管理器当前视口未变，点回前处理视口被同ID去重早退，
        //联动永不触发（曾致模型树/曲线树并存）。与曲线页"新建"按钮同一入口语义
        PostCurve::mOpenCurveDlg::showInstance(window());
    }

}

//后处理树dock提供者自注册（DLL加载期执行；主窗口注册"PostTree"dock时经
//MechCommon::postTreeDockProvider间接获取本窗口，本文件git mv迁入PostWindow后注册方零改动）
static const bool s_registerPostTreeDockProvider = []()
{
    MechCommon::postTreeDockProvider() = []() -> QWidget*
    {
        return PostWindow::mPostBrowserWindow::getInstance();
    };
    return true;
}();
