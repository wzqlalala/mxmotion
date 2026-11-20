#include "mTreeModel.h"
#include <QRegularExpression>
//【部件名唯一】部件重命名自动 _2 后缀需经 GeoManager 取 GN 名表
#include "mGeoManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "mModelData.h"
#include "mDataStatusManager.h"
#include "mContainerOperation.h"
#include "mMechGlobalSignals.h"
#include "mLoadBoundaryBase.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace MechCaseData;
namespace MechPreData
{
    static QCollator* collator = nullptr;

    mTreeModel::mTreeModel(int modelDataID, bool partGroupUnderMotionRoot, QObject* parent)
        : QAbstractItemModel(parent), _partGroupUnderMotionRoot(partGroupUnderMotionRoot)
    {
        m_root = new mTreeItem;
        //建树（AddModelTree）读取 _partGroupUnderMotionRoot 决定"部件"分组挂载位置，
        //标志随构造传入保证首建即正确
        AddModelTree(modelDataID);
    }

    mTreeModel::~mTreeModel() {
        deleteTreeItem(m_root);
    }

    void mTreeModel::indexItemRecursive(mTreeItem* item)
    {
        if (!item) return;
        // 根节点一般不放入索引
        if (item != m_root) {
            m_index.insert(item->key, item);
        }
        for (auto* c : item->children) indexItemRecursive(c);
    }

    void mTreeModel::deindexItemRecursive(mTreeItem* item)
    {
        if (!item) return;
        if (item != m_root) {
            m_index.remove(item->key);
        }
        for (auto* c : item->children) deindexItemRecursive(c);
    }


    inline void mTreeModel::deleteTreeItem(mTreeItem* item)
    {
        for (auto* c : item->children) deleteTreeItem(c);
        delete item;
        item = nullptr;
    }

    void mTreeModel::appendChildren(mTreeItem* parentItem, QVector<mTreeItem*>&& newChildren)
    {
        if (!parentItem) parentItem = m_root;
        if (newChildren.isEmpty()) return;

        const int first = parentItem->children.size();
        const int last = first + newChildren.size() - 1;

        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, first, last);

        // 重要：在插入期间建立一致的 parent/children 关系
        parentItem->children.reserve(parentItem->children.size() + newChildren.size());
        for (mTreeItem* c : newChildren) {
            c->parent = parentItem;
            parentItem->children.push_back(c);
            indexItemRecursive(c); // 你原来的索引逻辑
            if (parentItem->parent)
            {
                parentItem->_childStatuCount[c->_status]++;
                revaluateMyStatus(parentItem);
            }
        }

        endInsertRows();
    }

    void mTreeModel::appendChildren(mTreeItem* parentItem, mTreeItem* newChildren)
    {
        if (!parentItem) parentItem = m_root;
        if (!newChildren) return;

        const int first = parentItem->children.size();
        const int last = first;

        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, first, last);

        newChildren->parent = parentItem;
        parentItem->children.push_back(newChildren);
        indexItemRecursive(newChildren); // 你原来的索引逻辑

        parentItem->_childStatuCount[newChildren->_status]++;
        revaluateMyStatus(parentItem);
        endInsertRows();
    }

    void mTreeModel::appendAssemblyItem(mTreeItem* parentItem, mTreeItem* childItem, int parentAssemblyId)
    {
        if (!childItem) return;
        //先记录父装配ID再挂载注册索引，保证 m_index 中装配实例 key 含完整身份
        childItem->key._parentID = parentAssemblyId;
        appendChildren(parentItem, childItem);
    }

    void mTreeModel::insertChildren(mTreeItem* parentItem, mTreeItem* newChildren, MxFunctions::BrowserTreeType treeType)
    {
        if (!parentItem) parentItem = m_root;
        if (!newChildren) return;

        int i = 0;
        if (treeType != TreeRootItem)
        {
            for (; i < parentItem->children.size(); i++)
            {
                if (parentItem->children[i]->key._treeType == treeType)
                {
                    i++;
                    break;
                }
            }
        }
        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, i, i);

        newChildren->parent = parentItem;
        parentItem->children.insert(i ,newChildren);
        indexItemRecursive(newChildren); // 你原来的索引逻辑

        parentItem->_childStatuCount[newChildren->_status]++;
        revaluateMyStatus(parentItem);

        endInsertRows();
    }

    void mTreeModel::insertChildren(mTreeItem* parentItem, mTreeItem* newChildren, mTreeItem* lastItem)
    {
        if (!parentItem) parentItem = m_root;
        if (!newChildren) return;

        int i = 0;
        if (lastItem != nullptr)
        {
            for (; i < parentItem->children.size(); i++)
            {
                if (parentItem->children[i] == lastItem)
                {
                    i++;
                    break;
                }
            }
        }
        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, i, i);

        newChildren->parent = parentItem;
        parentItem->children.insert(i, newChildren);
        indexItemRecursive(newChildren); // 你原来的索引逻辑

        parentItem->_childStatuCount[newChildren->_status]++;
        revaluateMyStatus(parentItem);

        endInsertRows();
    }

    void mTreeModel::revaluateMyStatus(mTreeItem* parentItem)
    {
        //状态冒泡的终点：隐形根、模型根、多体动力学顶层根、后处理模型根不再向上传递
        //（m_root 经私有构造创建，_childStatuCount 为空且无父节点，向上访问会越界；
        //  多体根与模型根同为顶层节点，parent 即隐形根，须一并终止；
        //  视图树本地类型按防御性处理，正常不会出现在本模型中）
        int stopType = parentItem->key._treeType;
        if (parentItem == m_root
            || stopType == ModelRootItem || stopType == Motion_RootItem || stopType == PostTreeRootItem
            || ViewTreeTypes::isLocalTreeType(stopType))
        {
            return;
        }

        QModelIndex idx = indexFromItem(parentItem);
        if (parentItem->_childStatuCount[Tree_Failure] > 0)
        {
            if (parentItem->_status != Tree_Failure)
            {
                parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
                parentItem->getParentItem()->_childStatuCount[Tree_Failure]++;
                parentItem->_status = Tree_Failure;

                emit dataChanged(idx, idx, {Roles::StatusRole});
            }
        }
        else if (parentItem->_childStatuCount[Tree_Update] > 0)
        {
            if (parentItem->_status != Tree_Update)
            {
                parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
                parentItem->getParentItem()->_childStatuCount[Tree_Update]++;
                parentItem->_status = Tree_Update;
                emit dataChanged(idx, idx, { Roles::StatusRole });
            }
        }
        else if (parentItem->_childStatuCount[Tree_Wrong] > 0)
        {
            if (parentItem->_status != Tree_Yes && parentItem->_status != Tree_Wrong)
            {
                parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
                parentItem->getParentItem()->_childStatuCount[Tree_Yes]++;
                parentItem->_status = Tree_Yes;
                emit dataChanged(idx, idx, { Roles::StatusRole });
            }
        }
        else if (parentItem->_status != Tree_Yes)
        {
            parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
            parentItem->getParentItem()->_childStatuCount[Tree_Yes]++;
            parentItem->_status = Tree_Yes;
            emit dataChanged(idx, idx, { Roles::StatusRole });
        }
        revaluateMyStatus(parentItem->getParentItem());
    }


    mTreeItem* mTreeModel::itemFromIndex(const QModelIndex& idx) const
    {
        return idx.isValid() ? static_cast<mTreeItem*>(idx.internalPointer()) : m_root;
    }

    mItemKey mTreeModel::itemKeyFromIndex(const QModelIndex& idx) const
    {
        if(!idx.isValid()) return mItemKey();
        auto item = itemFromIndex(idx);

        mItemKey key;
        key._modelID = item->key._modelID;
        key._treeType = item->key._treeType;
        key._phyID = item->key._phyID;
        //合流：_caseID（dev 工况区分）与 _parentID（中性引擎装配实例父ID）并集保留
        key._caseID = item->key._caseID;
        key._parentID = item->key._parentID;
        return key;
    }

    //模型节点下固定分组唯一来源：AddModelTree 与 clearModelTree 共用，杜绝两份列表不同步。
    //多体形态下"部件"分组改挂多体动力学顶层根（partGroupUnderMotionRoot=true），
    //此处跳过部件——ModelParentItem 键按 (modelID, treeType) 全树唯一，不能建两份
    static QVector<mTreeItem*> createModelGroupItems(int modelDataID, bool partGroupUnderMotionRoot = false)
    {
        QVector<mTreeItem*> childrens;
        childrens.reserve(10);
        mTreeItem* children = nullptr;
        if (!partGroupUnderMotionRoot)
        {
            children = new mTreeItem(modelDataID, ModelParentItem, "部件");
            childrens.push_back(children);
        }
        //装配分组隐藏（后续再开发）：分组不创建，装配树填充/右键菜单随 getParentItem 落空整体失效
        if (AssemblyTreeEnabled)
        {
            children = new mTreeItem(modelDataID, AssemblyParentItem, "装配");
            childrens.push_back(children);
        }
        children = new mTreeItem(modelDataID, MaterialParentItem, "材料");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, PropertyParentItem, "截面属性");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, CoordinateParentItem, "坐标系");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, InteractionParentItem, "相互作用");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, MeshParentItem, "网格剖分");
        childrens.push_back(children);
        //合流：dev 侧新增分组（载荷/边界/全局参数设置）并入本表
        children = new mTreeItem(modelDataID, LoadManagerParentItem, "载荷");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, BoundaryManagerParentItem, "边界");
        childrens.push_back(children);
        children = new mTreeItem(modelDataID, GlobalParamParentItem, "全局参数设置");
        childrens.push_back(children);
        //【产品集成】Mech 求解任务分组并入工厂表（原 dev_Mech 手工列表独有）
        children = new mTreeItem(modelDataID, SolveTaskParentItem, "求解任务");
        childrens.push_back(children);
        return childrens;
    }

    //多体动力学顶层根下固定分组唯一来源：addMotionRootTree 专用（常驻展示，不随对象增删增删）。
    //求解方案不设固定父分组：方案对象节点（Motion_ScenarioParentItem）直接浮动挂多体根下，
    //每方案自带固定的 动画/结果导出/运动曲线 三个子节点（沿用原有行为）；
    //机电导入/机电导出/液压协同/强化学习仍为动态分组（增量更新槽懒建，不进本工厂）。
    //partGroupUnderMotionRoot=true 时首位挂"部件"分组——复用 ModelParentItem（模型根工厂
    //已跳过），部件的全部单击/右键/面板联动走原有 ModelParentItem 链路，无需特判
    static QVector<mTreeItem*> createMotionGroupItems(int modelDataID, bool partGroupUnderMotionRoot)
    {
        QVector<mTreeItem*> childrens;
        childrens.reserve(13);
        if (partGroupUnderMotionRoot)
        {
            childrens.push_back(new mTreeItem(modelDataID, ModelParentItem, "部件"));
        }
        //原型固定组
        childrens.push_back(new mTreeItem(modelDataID, Motion_MovingBodyParentItem, "运动体"));
        //柔性体沿用自动柔性体分组枚举（Motion_AutoFlexParentItem），仅显示名调整为"柔性体"
        childrens.push_back(new mTreeItem(modelDataID, Motion_AutoFlexParentItem, "柔性体"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_ConnectorParentItem, "运动副"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_CouplingParentItem, "耦合副"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_DriveParentItem, "驱动"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_SpringParentItem, "弹簧"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_LiYuanParentItem, "力元"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_SensorParentItem, "传感器"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_ControllerParentItem, "控制器"));
        //产品额外对象分组（原浮动分组，固化为常驻）
        childrens.push_back(new mTreeItem(modelDataID, Motion_MarkerParentItem, "标记点"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_ContactParentItem, "接触"));
        childrens.push_back(new mTreeItem(modelDataID, Motion_ControlInputParentItem, "控制输入"));
        return childrens;
    }

    void mTreeModel::AddModelTree(int modelDataID)
    {
        mItemKey key;
        key._modelID = modelDataID;
        key._treeType = ModelRootItem;

        mTreeItem* parentItem = treeItemFromKey(key);
        if (parentItem)
        {
            removeTreeChildrenItem(parentItem);
        }
        else
        {
            //模型树名称直接取模型数据名称（模型创建时已由 mViewportModelManager 命名并写入 modelData）
            const QString treeName = VMManager->getViewportModel(modelDataID)->getModelData()->getModelDataName();

            QVector<mTreeItem*> childrens;
            parentItem = new mTreeItem(modelDataID, ModelRootItem, treeName);
            childrens.push_back(parentItem);
            appendChildren(m_root, std::move(childrens));
        }

        //把"创建早于树命名"的前处理视口占位名（视口-N）回填为 "模型名· 视口-N"
        //名称取模型数据名称（新建与已存在两种路径均有效）
        if (parentItem && parentItem->key._treeType == ModelRootItem)
        {
            if (auto viewportModel = VMManager->getViewportModel(modelDataID))
            {
                const QString modelName = viewportModel->getModelData()->getModelDataName();
                static const QRegularExpression placeholderRe(QString::fromUtf8("^视口-\\d+$"));
                for (int vpID : viewportModel->getAllViewportIDs())
                {
                    auto sub = viewportModel->getSubWindowByViewportID(vpID);
                    auto priv = sub ? viewportModel->getViewportPrivate(sub) : nullptr;
                    if (priv && priv->getViewportType() == Viewport3D
                        && placeholderRe.match(priv->getViewportName()).hasMatch())
                    {
                        viewportModel->renameViewport(vpID, modelName + QString::fromUtf8("· ") + priv->getViewportName());
                    }
                }
            }
        }

        appendChildren(parentItem, createModelGroupItems(modelDataID, _partGroupUnderMotionRoot));

        addMotionRootTree(modelDataID);
        setCurrentModelID(modelDataID);
    }

    //多体动力学顶层根显示名："多体动力学-N"。N 与模型根同号（取模型数据名"模型-N"的
    //后缀）：多模型并排时按号分辨所属模型，且删除中间模型不重号（与模型根命名口径一致）；
    //模型被用户改名取不到后缀时退用模型ID，仍保持唯一可分辨。节点寻址不依赖该名字。
    static QString motionRootDisplayName(int modelDataID)
    {
        QString suffix;
        if (auto* vm = VMManager->getViewportModel(modelDataID))
        {
            static const QRegularExpression re(QString::fromUtf8("^模型-(\\d+)$"));
            const auto m = re.match(vm->getModelData()->getModelDataName());
            suffix = m.hasMatch() ? m.captured(1) : QString::number(modelDataID);
        }
        return suffix.isEmpty() ? QString::fromUtf8("多体动力学")
            : QString::fromUtf8("多体动力学-%1").arg(suffix);
    }

    //多体动力学顶层根：与模型根平级直挂隐形根（m_root），按 modelID 区分多模型并排；
    //节点寻址按 (modelID, treeType) 哈希与挂载层级无关，原有 getParentItem(Motion_RootItem)
    //等查找在提层后继续有效。已存在（如 clearModelTree 复用）则仅重建固定分组。
    void mTreeModel::addMotionRootTree(int modelDataID)
    {
        mItemKey key;
        key._modelID = modelDataID;
        key._treeType = Motion_RootItem;

        mTreeItem* motionRootItem = treeItemFromKey(key);
        if (motionRootItem)
        {
            removeTreeChildrenItem(motionRootItem);
        }
        else
        {
            QVector<mTreeItem*> childrens;
            motionRootItem = new mTreeItem(modelDataID, Motion_RootItem, motionRootDisplayName(modelDataID));
            childrens.push_back(motionRootItem);
            appendChildren(m_root, std::move(childrens));
        }
        appendChildren(motionRootItem, createMotionGroupItems(modelDataID, _partGroupUnderMotionRoot));
    }

    //AddPostModelTree 及后处理分组 API 已随树模型独立迁出
    //（PostWindow::mPostTreeModel 承接，本模型仅服务前处理树）
    //注：远端 dev_base 对旧 AddPostModelTree 的"树名直接取模型数据名称"修复
    //已同步移植至 mPostTreeModel::addPostModelTree

    void mTreeModel::clearModelTree(mTreeItem* item, int modelDataID)
    {
        //模型根换绑前的旧模型ID：多体顶层根仍以旧ID为键，需同样换绑
        const int oldModelID = item->key._modelID;
        removeTreeChildrenItem(item);

        //节点换绑新模型ID：仅改 key._modelID 会让键值与哈希值不一致
        //（hash(mItemKey) 以 _modelID 为种子），直接 m_index.insert 后
        //旧桶残留垃圾条目，后续查找/删除可能命中失效节点导致视图不刷新。
        //正确做法是解除索引、改键、再重建索引
        deindexItemRecursive(item);
        item->key._modelID = modelDataID;
        indexItemRecursive(item);

        appendChildren(item, createModelGroupItems(modelDataID, _partGroupUnderMotionRoot));

        //多体顶层根与模型根同步换绑新模型ID并重建固定分组
        //（直接 addMotionRootTree 会因新ID查不到旧根而并排出第二棵多体根）
        mItemKey motionKey;
        motionKey._modelID = oldModelID;
        motionKey._treeType = Motion_RootItem;
        if (mTreeItem* motionRootItem = treeItemFromKey(motionKey))
        {
            removeTreeChildrenItem(motionRootItem);
            deindexItemRecursive(motionRootItem);
            motionRootItem->key._modelID = modelDataID;
            indexItemRecursive(motionRootItem);
            appendChildren(motionRootItem, createMotionGroupItems(modelDataID, _partGroupUnderMotionRoot));
        }
        else
        {
            addMotionRootTree(modelDataID);
        }
        setCurrentModelID(modelDataID);
    }

    bool mTreeModel::removeTreeItem(mTreeItem* item)
    {
        if (!item) return false;
        mTreeItem* parentItem = item->parent;   // 假设你有 parent 指针
        if (!parentItem) return false;          // 没父节点通常代表 root，不建议这样删

        int row = parentItem->children.indexOf(item);
        if (row < 0) return false;

        QModelIndex parentIdx = indexFromItem(parentItem);
        return removeRows(row, 1, parentIdx);
    }

    //后处理分组五件套（addPostModelGroup/removePostModelGroup/renamePostModelGroup/
    //findPostModelGroup/getPostModelGroupPartIds/getPostModelGroups）已随树模型独立迁出

    bool mTreeModel::removeTreeChildrenItem(mTreeItem* parentItem)
    {
        if (!parentItem) return false;
        int cout = parentItem->children.size();
        if (cout == 0) return false;

        QModelIndex parentIdx = indexFromItem(parentItem);
        return removeRows(0, cout, parentIdx);
    }

    void mTreeModel::nameFilterTree(const QString& filterStr)
    {
        bool matched;
        for (mTreeItem* item : m_root->children)
        {
            matched = treeItemFilter(filterStr, item);
            item->hidden = !matched;
        }
    }

    void mTreeModel::statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus)
    {
        bool matched;
        for (mTreeItem* item : m_root->children)
        {
            matched = treeItemFilter(filterStatus, item);
            item->hidden = !matched;
        }
    }

    bool mTreeModel::isExistItem(MxFunctions::BrowserTreeType treeType, int modelDataID, int phyID, int caseID) const
    {
        mItemKey key;
        key._treeType = treeType;
        if (modelDataID > -1) key._modelID = modelDataID;
        else  key._modelID = _currentModelID;
        if (phyID > 0) key._phyID = phyID;
        if (caseID > 0) key._caseID = caseID;

        auto it = m_index.find(key);
        if (it != m_index.end()) return true;

        return false;
    }

    QModelIndex mTreeModel::index(int row, int col, const QModelIndex& parentIdx) const
    {
        if (col != 0 || row < 0) return {};
        mTreeItem* p = itemFromIndex(parentIdx);
        if (!p || row >= p->children.size()) return {};
        return createIndex(row, col, p->children[row]);
    }

    QModelIndex mTreeModel::parent(const QModelIndex& childIdx) const
    {
        if (!childIdx.isValid()) return {};
        mTreeItem* child = itemFromIndex(childIdx);
        mTreeItem* p = child ? child->parent : nullptr;
        if (!p || p == m_root) return {};

        mTreeItem* gp = p->parent ? p->parent : m_root;
        int rowInGp = gp->children.indexOf(p);
        return createIndex(rowInGp, 0, p);
    }

    int mTreeModel::rowCount(const QModelIndex& parentIdx) const
    {
        mTreeItem* p = itemFromIndex(parentIdx);
        return p ? p->children.size() : 0;
    }

    QVariant mTreeModel::data(const QModelIndex& idx, int role) const
    {
        if (!idx.isValid()) return {};
        mTreeItem* it = itemFromIndex(idx);

        switch (role) {
        case Qt::DisplayRole: return it->_text;
        case Qt::EditRole:    return it->_text;
        case Roles::StatusRole: return it->_status;
        case Roles::Icon2Role: return it->_iconString;
        case Roles::TypeRole:  return it->key._treeType;
        case Roles::ModelRole:  return it->key._modelID;
        case Roles::PhyidRole:  return it->key._phyID;
        case Roles::CaseidRole: return it->key._caseID;
        case Roles::HiddenRole:return it->hidden;
        default: return {};
        }
    }

    Qt::ItemFlags mTreeModel::flags(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    bool mTreeModel::setData(const QModelIndex& idx, const QVariant& value, int role)
    {
        if (!idx.isValid()) return false;
        mTreeItem* it = itemFromIndex(idx);

        if (role == Qt::EditRole) {
            QString newName = value.toString();
            if (newName.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "名称不能为空！");
                return false;
            }

            //视图树本地节点类型不出现在本模型中，防御性拦截
            if (ViewTreeTypes::isLocalTreeType(it->key._treeType))
            {
                return false;
            }

            MxFunctions::PhysicalFieldType renameType = mContainerOperation::mapPhyTypeForTreeType((BrowserTreeType)it->key._treeType);
            if (it->key._treeType == ModelRootItem)
            {
                if (newName == it->_text)
                {
                    return true;
                }

                //重名检查以模型数据名称为准（模型树即模型名称的镜像）
                if (VMManager->isExistModelDataName(newName, it->key._modelID))
                {
                    mGlobalSignals::getInstance()->outputMessageSig("该名称已存在！");
                    return false;
                }
            }
            else if (renameType == Physical_Error)
            {
                mGlobalSignals::getInstance()->outputMessageSig("代码错误，请检查函数mapPhyTypeForTreeType是否完整！");
                return false;
            }
            else if (renameType == Physical_LoadBoundary)
            {
                return LoadBoundaryRename(it, newName);
            }
            else
            {
                //【部件名唯一】部件重命名遇重名不再拒绝，自动加 _2/_3 后缀；
                // GN 名表与 mesh 侧名表都作为冲突源，树文本/数据层统一用最终名
                //（GN 侧经 PartManager::resetName 兜底自动后缀）
                if (renameType == MxFunctions::Physical_Part)
                {
                    GeoNeutralData::mGeoPartManager* partMgr =
                        getCVM->getModelData()->getGeoManager()->getGeoPartMgr();
                    QString candidate = newName;
                    int suffix = 1;
                    while (getCVM->getModelData()->isExistName(renameType, candidate)
                           || (partMgr != nullptr
                               && partMgr->_partNames.contains(candidate)))
                    {
                        ++suffix;
                        candidate = newName + QStringLiteral("_%1").arg(suffix);
                    }
                    if (candidate != newName)
                    {
                        mGlobalSignals::getInstance()->outputMessageSig(
                            QString("名称已存在，自动重命名为 %1").arg(candidate));
                        newName = candidate;
                    }
                }
                else
                {
                    //判断是否重名
                    if (getCVM->getModelData()->isExistName(renameType, newName) && !_isSelectRenames)
                    {
                        if (newName == it->_text)
                        {
                            return true;
                        }
                        mGlobalSignals::getInstance()->outputMessageSig(1, "该名称已存在！");
                        return false;
                    }
                }

                getCVM->getDataStatusManager()->resetName(renameType, it->_text, newName, it->key._phyID);
            }

            it->_text = newName;

            //模型树重命名：同步模型数据名称，并联动该模型前处理视口名前缀（"旧名· 视口-N" -> "新名· 视口-N"）
            //用户手动改过名的视口不跟随
            if (it->key._treeType == ModelRootItem)
            {
                if (auto viewportModel = VMManager->getViewportModel(it->key._modelID))
                {
                    viewportModel->getModelData()->setModelDataName(newName);
                    const QString tag = QString::fromUtf8("· 视口-");
                    for (int vpID : viewportModel->getAllViewportIDs())
                    {
                        auto sub = viewportModel->getSubWindowByViewportID(vpID);
                        auto priv = sub ? viewportModel->getViewportPrivate(sub) : nullptr;
                        if (!priv || priv->getViewportType() != Viewport3D) continue;
                        if (priv->isUserRenamed()) continue;
                        const QString oldName = priv->getViewportName();
                        const int pos = oldName.indexOf(tag);
                        if (pos > 0)
                        {
                            viewportModel->renameViewport(vpID, newName + oldName.mid(pos));
                        }
                    }
                }
            }

            emit dataChanged(idx, idx, { Qt::DisplayRole});
            //同一装配被多个父装配引用时树中存在多实例，重命名需同步全部实例文本
            if ((BrowserTreeType)it->key._treeType == AssemblyChildItem)
            {
                for (auto iter = m_index.begin(); iter != m_index.end(); ++iter)
                {
                    mTreeItem* inst = iter.value();
                    if (inst != it && inst->key._treeType == AssemblyChildItem
                        && inst->key._phyID == it->key._phyID && inst->_text != newName)
                    {
                        inst->_text = newName;
                        QModelIndex instIdx = indexFromItem(inst);
                        if (instIdx.isValid())
                            emit dataChanged(instIdx, instIdx, { Qt::DisplayRole });
                    }
                }
            }
            return true;
        }
        else if (role == Roles::HiddenRole) {
            it->hidden = value.toBool();
            emit dataChanged(idx, idx, { Roles::HiddenRole });
            return true;
        }
        else if (role == Roles::StatusRole) {
            if (it->_status == value.toInt())
            {
                return true;
            }
            it->getParentItem()->_childStatuCount[it->_status]--;
            it->_status = value.toInt();
            it->getParentItem()->_childStatuCount[it->_status]++;
            revaluateMyStatus(it->getParentItem());
            emit dataChanged(idx, idx, { Roles::StatusRole });
            return true;
        }
        else if (role == Roles::TypeRole) {
            it->key._treeType = value.toInt();
            emit dataChanged(idx, idx, { Roles::TypeRole });
            return true;
        }
        else if (role == Roles::Icon2Role) {
            it->_iconString = value.toString();
            emit dataChanged(idx, idx, { Roles::Icon2Role });
            return true;
        }
        else if (role == Qt::DisplayRole) {
            it->_text = value.toString();
            emit dataChanged(idx, idx, { Qt::DisplayRole });
            return true;
        }

        return false;
    }

    QModelIndex mTreeModel::indexFromKey(const mItemKey& key) const
    {
        auto it = m_index.find(key);
        if (it == m_index.end()) return {};
        mTreeItem* item = it.value();
        if (!item || !item->parent) return {};
        int row = item->parent->children.indexOf(item);
        return createIndex(row, 0, item);
    }

    mTreeItem* mTreeModel::treeItemFromKey(const mItemKey& key) const
    {
        auto it = m_index.find(key);
        if (it == m_index.end()) return nullptr;
        return it.value();
    }

    bool mTreeModel::treeItemFilter(const QString& filterStr, mTreeItem* parentItem)
    {
        bool matched;
        bool parentMask = parentItem->_text.contains(filterStr, Qt::CaseInsensitive);
        for (mTreeItem* childitem : parentItem->children)
        {
            matched = treeItemFilter(filterStr, childitem);
            childitem->hidden = !matched;
            parentMask = parentMask || matched;
        }
        return parentMask;
    }

    bool mTreeModel::treeItemFilter(MxFunctions::BrowserTreeStatus filterStatus, mTreeItem* parentItem)
    {
        bool matched;
        bool parentMask = parentItem->_status == filterStatus ? true : false;
        for (mTreeItem* childitem : parentItem->children)
        {
            matched = treeItemFilter(filterStatus, childitem);
            childitem->hidden = !matched;
            parentMask = parentMask || matched;
        }
        return parentMask;
    }

    //postRename（后处理节点重命名同步数据管理器）已随树模型独立迁出
    //（PostWindow::mPostTreeModel::postRename 承接）

    bool mTreeModel::LoadBoundaryRename(mTreeItem* item, const QString& newName)
    {
        //判断是否重名
        if (getCVM->getModelData()->isExistName(Physical_LoadBoundary, newName) && !_isSelectRenames)
        {
            if (newName == item->_text)
            {
                return true;
            }
            mGlobalSignals::getInstance()->outputMessageSig("该名称已存在！");
            return false;
        }

        QString oldName = item->_text;
        int loadboundaryid = item->key._phyID;

        //获取当前载荷边界实体，判断其是载荷还是边界
        std::shared_ptr<mLoadBoundaryBase> loadboundary = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadboundaryid);
        //根据载荷id获取与其映射的工况id
        std::unordered_map<int, double> caseMap =
            getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, loadboundaryid }, Physical_Case);


        BrowserTreeType treeType;
        if (loadboundary->isLoadOrBoundary() == 0) //载荷
        {
            treeType = CaseLoadChildItem;
        }
        else if (loadboundary->isLoadOrBoundary() == 1) //边界
        {
            treeType = CaseBCChildItem;
        }
        else
        {
            return false;
        }

        //管理器节点
        caseMap[0] = 1;

        mTreeItem* treeItem;
        QModelIndex idx;
        for (auto caseIds : caseMap)
        {
            treeItem = getChildrenItem(treeType, loadboundaryid, caseIds.first);
            if (treeItem == nullptr) continue;
            treeItem->_text = newName;
            idx = indexFromItem(treeItem);
            emit dataChanged(idx, idx, { Qt::DisplayRole });
        }

        getCVM->getDataStatusManager()->resetName(Physical_LoadBoundary, oldName, newName, item->key._phyID);
        return true;
    }

    mTreeItem* mTreeModel::getParentItem(MxFunctions::BrowserTreeType treeType, int caseID) const
    {
        mItemKey key;
        key._modelID = _currentModelID;
        key._treeType = treeType;
        if (caseID > -999) key._phyID = caseID;
        return treeItemFromKey(key);
    }

    QModelIndex mTreeModel::getParentIndex(MxFunctions::BrowserTreeType treeType, int caseID) const
    {
        mItemKey key;
        key._modelID = _currentModelID;
        key._treeType = treeType;
        if (caseID > -999) key._phyID = caseID;
        return indexFromKey(key);
    }

    mTreeItem* mTreeModel::getChildrenItem(MxFunctions::BrowserTreeType treeType, int phyID, int caseID, int modelDataID) const
    {
        mItemKey key;
        key._modelID = modelDataID > -1 ? modelDataID : _currentModelID;
        if (caseID > -1) key._caseID = caseID;

        key._treeType = treeType;
        key._phyID = phyID;
        return treeItemFromKey(key);
    }

    QModelIndex mTreeModel::indexFromItem(mTreeItem* item) const
    {
        if (!item || item == m_root) return {};

        mTreeItem* p = item->parent ? item->parent : m_root;
        int row = p->children.indexOf(item);
        if (row < 0) return {};              // 说明 parent/children 关系不一致
        return createIndex(row, 0, item);    // internalPointer 统一存 mTreeItem*
    }

    void mTreeModel::setHiddenByKey(const mItemKey& key, bool hidden)
    {
        QModelIndex idx = indexFromKey(key);
        if (idx.isValid()) setData(idx, hidden, Roles::HiddenRole);
    }

    void mTreeModel::setCurrentModelID(int modelID)
    {
        if (modelID != _currentModelID)
        {
            _currentModelID = modelID;
            mMechGlobalSignals::getInstance()->updateTableChangedModel(true);
        }
    }

    bool mTreeModel::removeRows(int row, int count, const QModelIndex& parentIdx)
    {
        mTreeItem* p = itemFromIndex(parentIdx);
        if (!p || row < 0 || row + count > p->children.size()) return false;

        beginRemoveRows(parentIdx, row, row + count - 1);
        for (int i = 0; i < count; ++i) {
            mTreeItem* victim = p->children[row];
            //按"被删子节点自己的状态"记账：按父节点当前状态减会使计数在子节点
            //状态分布不一致时漂移、状态冒泡失真（mPostTreeModel 同形缺陷两树同修）
            const int victimStatus = victim->_status;
            deindexItemRecursive(victim);
            deleteTreeItem(victim);
            p->children.removeAt(row);
            if (p->parent)
            {
                p->_childStatuCount[victimStatus]--;
                revaluateMyStatus(p);
            }
        }
        endRemoveRows();
        return true;
    }

    void mTreeModel::removeIndexs(QModelIndexList indexs)
    {
        std::sort(indexs.begin(), indexs.end(), [](const QModelIndex& a, const QModelIndex& b) {
            if (a.parent() == b.parent()) return a.row() > b.row();
            return a.parent() < b.parent();
            });

        for (const auto& idx : indexs) {
            removeRows(idx.row(), 1, idx.parent());
        }
    }

    void mTreeProxyModel::setSortMode(SortMode m, Qt::SortOrder sortType)
    {
        m_mode = m;
        invalidate();
        sort(0, sortType);
    }

    bool mTreeProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!idx.isValid()) return true;
        bool hidden = sourceModel()->data(idx, Roles::HiddenRole).toBool();
        if (hidden) return false;

        //本代理服务于前处理树窗口；后处理树已使用独立模型（PostWindow::mPostTreeModel），
        //本模型不再包含后处理子树，无需再做 PostTreeRootItem 顶层过滤
        return true;
    }

    bool mTreeProxyModel::lessThan(const QModelIndex& a, const QModelIndex& b) const
    {
        std::function<int(const QString&)> extractNumber = [](const QString& str) {
            QRegularExpression re("(\\d+)$");
            QRegularExpressionMatch match = re.match(str);
            if (match.hasMatch()) {
                return match.captured(1).toInt();
            }
            return 0;
            };

        switch (m_mode)
        {
        case mTreeProxyModel::SortMode::DefaultOrder:
            // 保持源模型原始顺序：先比 parent 下的 row；row 相同再比内部 id 防止不稳定
            if (a.row() != b.row()) return a.row() < b.row();
            return a.internalId() < b.internalId();
        case mTreeProxyModel::SortMode::Alpha:
        {
            const QString ka = sourceModel()->data(a, Qt::DisplayRole).toString();
            const QString kb = sourceModel()->data(b, Qt::DisplayRole).toString();
            if (collator == nullptr)
            {
                collator = new QCollator(QLocale(QLocale::Chinese, QLocale::China));
                collator->setNumericMode(true);
                collator->setCaseSensitivity(Qt::CaseInsensitive);
                collator->setIgnorePunctuation(true);
            }
            return collator->compare(ka, kb) < 0;
        }
        case mTreeProxyModel::SortMode::SuffixNumber:
        {
            const QString ka = sourceModel()->data(a, Qt::DisplayRole).toString();
            const QString kb = sourceModel()->data(b, Qt::DisplayRole).toString();
            return extractNumber(ka) < extractNumber(kb);
        }
        default:
            return false;
        }
    }

    mTreeItem::mTreeItem(int modelID, MxFunctions::BrowserTreeType treeType, QString text, int phyID, MxFunctions::BrowserTreeStatus statusPic, QString picString)
    {
        key._modelID = modelID;
        key._treeType = treeType;
        key._phyID = phyID;
        _status = (int)statusPic;
        _text = text;
        if(picString.isEmpty()) _iconString = getBrowserTreePic(treeType);
        else _iconString = picString;
        _childStatuCount.resize(4);
        _childStatuCount = { 0, 0, 0, 0 };
    }

    QString mTreeItem::getBrowserTreePic(MxFunctions::BrowserTreeType type)
    {
        switch (type)
        {
        case MxFunctions::ModelRootItem:
            return ":/mMainWidgets/Icon/BrowserTree/model3x.png";
        case MxFunctions::ModelParentItem:
        case MxFunctions::ModelChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe3x.png";
        //装配图标待 UI 提供，交付前临时复用既有图标（分组-部件分组图标/实例-模型图标/部件引用-部件图标）
        case MxFunctions::AssemblyParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe3x.png";
        case MxFunctions::AssemblyChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        case MxFunctions::AssemblyPartRefItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe3x.png";
        case MxFunctions::MaterialParentItem:
        case MxFunctions::MaterialChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/cailiao.png";
        case MxFunctions::CaseLoadChildItem:
        case MxFunctions::CaseLoadParentItem:
        case MxFunctions::CaseRotateChildItem:
        case MxFunctions::CaseRotateParentItem:
        	return ":/mMainWidgets/Icon/BrowserTree/zaihe.png";
        case MxFunctions::CaseBCChildItem:
        case MxFunctions::CaseBCParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/bianjie.png";
        case MxFunctions::CaseSettingChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/fenxishezhi.png";
        case MxFunctions::PropertyParentItem:
        case MxFunctions::PropertyChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jiemianshuxing.png";
        case MxFunctions::SectionShapeParentItem:
        case MxFunctions::SectionShapeChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jiemianxinzhuang.png";
        case MxFunctions::CoordinateParentItem:
        case MxFunctions::CoordinateChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png";
        case MxFunctions::InteractionParentItem:
        case MxFunctions::CaseInteractionParentItem:
        case MxFunctions::CaseInteractionChildItem:
        case MxFunctions::PeriodicBoundaryParentItem:
        case MxFunctions::PeriodicBoundaryChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/xianghuzuoyongxiao.png";
        case MxFunctions::ConnectorParentItem:
        case MxFunctions::ConnectorChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/lianjie.png";
        case MxFunctions::ContactParentItem:
        case MxFunctions::ContactChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jiechu.png";
        case MxFunctions::DeputyParentItem:
        case MxFunctions::DeputyChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/lianjiefu.png";
        case MxFunctions::PartTreeItem:
            return ":/mMainWidgets/Icon/mesh.png";
        case MxFunctions::SetParentItem:
        case MxFunctions::SetChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe.png";
        case MxFunctions::FunctionParentItem:
        case MxFunctions::FunctionChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/hanshu.png";
        case MxFunctions::MeshParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/wangge.png";
        case MxFunctions::MeshOptParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/wangge.png";
        //后处理段类型图标已随树模型独立迁出（PostWindow::mPostTreeItem::getBrowserTreePic）
        case MxFunctions::MeshChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/wangge.png";
        case MxFunctions::MeshOptChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/wangge.png";
        case MxFunctions::OptimizationParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhua.png";
        case MxFunctions::OptimizationDesignParentItem:
        case MxFunctions::OptimizationDesignChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuasheji.png";
        case MxFunctions::OptimizationGoalParentItem:
        case MxFunctions::OptimizationGoalChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuamubioa.png";
        case MxFunctions::OptimizationConstraintsParentItem:
        case MxFunctions::OptimizationConstraintsChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuayueshu.png";
        case MxFunctions::RunnerOptimizationParentItem:
            return ":/mMainWidgets/Icon/TopologyOptimization/runner.png";
        case MxFunctions::RunnerOptimizationDesignParentItem:
        case MxFunctions::RunnerOptimizationDesignChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuasheji.png";
        case MxFunctions::RunnerOptimizationGoalParentItem:
        case MxFunctions::RunnerOptimizationGoalChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuamubioa.png";
        case MxFunctions::RunnerOptimizationConstraintsParentItem:
        case MxFunctions::RunnerOptimizationConstraintsChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/youhuayueshu.png";
        case MxFunctions::LoadManagerParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/zaiheguanliqi.png";
        case MxFunctions::BoundaryManagerParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/bianjieguanliqi.png";
        case MxFunctions::PredefinedFieldManagerParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/yudingyichang.png";
        case MxFunctions::CasePredefinedFieldParentItem:
        case MxFunctions::CasePredefinedFieldChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/yudingyichang.png";
        case MxFunctions::FrequencySamplingPointParentItem:
        case MxFunctions::FrequencySamplingPointChildItem:
            return ":/mMainWidgets/Icon/homepage/caiyangpinlvdian.png";
        case MxFunctions::GlobalParamParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/globalParam.png";
        case MxFunctions::SolveTaskParentItem:
            return ":/mMainWidgets/Icon/homepage/tijiaojisuan.png";
        case MxFunctions::GenericParamParentItem:
        case MxFunctions::GenericParamChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/genericParam.png";
        case MxFunctions::GenericAssignParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/genericAssign.png";
        case MxFunctions::GenericMatrixParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/genericMatrix.png";
        case MxFunctions::Motion_RootItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion.png";
        case MxFunctions::Motion_MovingBodyParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_movingBodyParentItem.png";
        case MxFunctions::Motion_ConnectorParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png";
        case MxFunctions::Motion_CouplingParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_couplingParentItem.png";
        case MxFunctions::Motion_DriveParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png";
        case MxFunctions::Motion_SpringParentItem:
            return ":/mMainWidgets/Icon/Motion/force_spring.png";
        case MxFunctions::Motion_MarkerParentItem:
            return ":/mMainWidgets/Icon/caotu/dian.png";
        case MxFunctions::Motion_SensorParentItem:
            return ":/mMainWidgets/Icon/Motion/sensor.png";
        case MxFunctions::Motion_ControllerParentItem:
        case MxFunctions::Motion_ControllerChildItem:
            return ":/mMainWidgets/Icon/Motion/controlCalculation.png";
        case MxFunctions::Motion_AutoFlexParentItem:
        case MxFunctions::Motion_AutoFlexChildItem:
            return ":/mMainWidgets/Icon/Motion/mnf.png";
        case MxFunctions::Motion_ContactParentItem:
            return ":/mMainWidgets/Icon/Motion/contact.png";
        case MxFunctions::Motion_LiYuanParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_liYuanParentItem.png";
        case MxFunctions::Motion_ScenarioParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_solveParentItem.png";
        case MxFunctions::Motion_PostParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_postParentItem.png";
        case MxFunctions::Motion_AnimationChildItem:
            return ":/mMainWidgets/Icon/post/animation.png";
        case MxFunctions::Motion_LoadExportChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_solveParentItem.png";
        case MxFunctions::Motion_CurveChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_curveChildItem.png";
        case MxFunctions::Motion_ChartChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_chartChildItem.png";
        case MxFunctions::Motion_ControlInputParentItem:
        case MxFunctions::Motion_ControlInputChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion_driveParentItem.png";
        case MxFunctions::Motion_ElectroImportParentItem:
        case MxFunctions::Motion_ElectroImportChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion.png";
        case MxFunctions::Motion_ElectroExportParentItem:
        case MxFunctions::Motion_ElectroExportChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion.png";
        case MxFunctions::Motion_HydraulicParentItem:
        case MxFunctions::Motion_HydraulicChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion.png";
        case MxFunctions::Motion_RLParentItem:
        case MxFunctions::Motion_RLChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motion.png";
        case MxFunctions::SglLayerPlateParentItem:
        case MxFunctions::SglLayerPlateChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/dancengban.png";
        case MxFunctions::LamPlateParentItem:
        case MxFunctions::LamPlateChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/cengheban.png";
        case MxFunctions::ContriParentItem:
        case MxFunctions::CaseDiagOptParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/gongxianliang.png";
        case MxFunctions::ModalContriChildItem:
        case MxFunctions::CaseModalContriChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/motaigongxianliang.png";
        case MxFunctions::NodeContriChildItem:
        case MxFunctions::CaseNodeContriChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jiediangongxianliang.png";
        case MxFunctions::TPContriChildItem:
        case MxFunctions::CaseTPContriChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/chuandilujinggongxianliang.png";
        case MxFunctions::PanelContriChildItem:
        case MxFunctions::CasePanelContriChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/mianbangongxianliang.png";
        case MxFunctions::ERPParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/dengxiaofushegonglv.png";
        case MxFunctions::PeakFilterParentItem:
        case MxFunctions::PeakFilterChildItem:
        case MxFunctions::CasePeakFilterChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/peakfilter.png";
        case MxFunctions::CaseNSMManagerItem:
        case MxFunctions::NonStructuralQualityParentItem:
        case MxFunctions::NonStructuralQualityChildItem:
            return ":/mMainWidgets/Icon/Case/nsm@3x.png";
        case MxFunctions::FieldParentItem:
        case MxFunctions::FieldChildItem:
            return ":/mMainWidgets/Icon/DiscreteField/discreteField@3x.png";
        case MxFunctions::ASCouplingSearchParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/ascouplingSearch.png";
        case MxFunctions::GenericPartParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/genericPart.png";
        case MxFunctions::CaseModelFixManagerItem:
        case MxFunctions::CaseModelFixChildItem:
        case MxFunctions::ModelFixParentItem:
        case MxFunctions::ModelFixChildItem:
            return ":/mMainWidgets/Icon/load/单元生死_大@3x.png";
        case MxFunctions::SolveControlParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/qiujiekongzhi.png";
        case MxFunctions::GlobalOutputControlParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/qiujiekongzhi.png";
        default:
            break;
        }
        return QString();
    }

}





