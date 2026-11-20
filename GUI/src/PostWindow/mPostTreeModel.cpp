#include "mPostTreeModel.h"
//模型本体无头纯净：不包含视口/数据管理器头——重命名的管理器同步经 renameValidator
//钩子由视图层（mPostBrowserWindow.cpp 的 postItemRenameSync）注入，批次B2
#include "mGlobalSignals.h"

using namespace MxFunctions;
namespace PostWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 构造 / 析构
    //////////////////////////////////////////////////////////////////////////

    mPostTreeModel::mPostTreeModel(QObject* parent) : QAbstractItemModel(parent)
    {
        //与前处理模型不同：本模型构造时不创建任何骨架，
        //后处理模型根由 addPostModelTree 按打开结果的节奏创建
        m_root = new mPostTreeItem;
    }

    mPostTreeModel::~mPostTreeModel()
    {
        deleteTreeItem(m_root);
    }

    void mPostTreeModel::indexItemRecursive(mPostTreeItem* item)
    {
        if (!item) return;
        // 根节点不放入索引
        if (item != m_root) {
            //key 冲突诊断：m_index 为 QHash::insert 静默覆盖，同 key 节点会使先插入者
            //不可按 key 查找（历史实害：刚性墙曾复用部件残留 phyID）。此处告警不阻断
            if (m_index.contains(item->key))
            {
                qWarning("mPostTreeModel: duplicate item key (model=%d type=%d phy=%d) overwrites index",
                    item->key._modelID, item->key._treeType, item->key._phyID);
            }
            m_index.insert(item->key, item);
        }
        for (auto* c : item->children) indexItemRecursive(c);
    }

    void mPostTreeModel::deindexItemRecursive(mPostTreeItem* item)
    {
        if (!item) return;
        if (item != m_root) {
            m_index.remove(item->key);
        }
        for (auto* c : item->children) deindexItemRecursive(c);
    }

    inline void mPostTreeModel::deleteTreeItem(mPostTreeItem* item)
    {
        for (auto* c : item->children) deleteTreeItem(c);
        delete item;
        item = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // 增删节点（自 mTreeModel 同形裁剪）
    //////////////////////////////////////////////////////////////////////////

    void mPostTreeModel::appendChildren(mPostTreeItem* parentItem, QVector<mPostTreeItem*>&& newChildren)
    {
        if (!parentItem) parentItem = m_root;
        if (newChildren.isEmpty()) return;

        const int first = parentItem->children.size();
        const int last = first + newChildren.size() - 1;

        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, first, last);

        // 重要：在插入期间建立一致的 parent/children 关系
        parentItem->children.reserve(parentItem->children.size() + newChildren.size());
        for (mPostTreeItem* c : newChildren) {
            c->parent = parentItem;
            parentItem->children.push_back(c);
            indexItemRecursive(c);
            if (parentItem->parent)
            {
                parentItem->_childStatuCount[c->_status]++;
                revaluateMyStatus(parentItem);
            }
        }

        endInsertRows();
    }

    void mPostTreeModel::appendChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren)
    {
        if (!parentItem) parentItem = m_root;
        if (!newChildren) return;

        const int first = parentItem->children.size();
        const int last = first;

        QModelIndex parentIdx = indexFromItem(parentItem);

        beginInsertRows(parentIdx, first, last);

        newChildren->parent = parentItem;
        parentItem->children.push_back(newChildren);
        indexItemRecursive(newChildren);

        //计数守卫与批量版对齐：插入目标是隐形根（顶层模型根行）时不记账
        //（隐形根无展示语义，记入会使 m_root 的 _childStatuCount 脏账）
        if (parentItem->parent)
        {
            parentItem->_childStatuCount[newChildren->_status]++;
            revaluateMyStatus(parentItem);
        }
        endInsertRows();
    }

    void mPostTreeModel::insertChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren, MxFunctions::BrowserTreeType treeType)
    {
        if (!parentItem) parentItem = m_root;
        if (!newChildren) return;

        int i = 0;
        if (treeType != TreeRootItem)
        {
            //插在指定类型节点之后（锚点定位，维持固定根顺序）
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
        parentItem->children.insert(i, newChildren);
        indexItemRecursive(newChildren);

        parentItem->_childStatuCount[newChildren->_status]++;
        revaluateMyStatus(parentItem);

        endInsertRows();
    }

    void mPostTreeModel::insertChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren, mPostTreeItem* lastItem)
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
        indexItemRecursive(newChildren);

        parentItem->_childStatuCount[newChildren->_status]++;
        revaluateMyStatus(parentItem);

        endInsertRows();
    }

    void mPostTreeModel::revaluateMyStatus(mPostTreeItem* parentItem)
    {
        //状态冒泡的终点：隐形根、后处理模型根不再向上传递
        int stopType = parentItem->key._treeType;
        if (parentItem == m_root || stopType == PostTreeRootItem)
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

                emit dataChanged(idx, idx, { MechPreData::Roles::StatusRole });
            }
        }
        else if (parentItem->_childStatuCount[Tree_Update] > 0)
        {
            if (parentItem->_status != Tree_Update)
            {
                parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
                parentItem->getParentItem()->_childStatuCount[Tree_Update]++;
                parentItem->_status = Tree_Update;
                emit dataChanged(idx, idx, { MechPreData::Roles::StatusRole });
            }
        }
        else if (parentItem->_childStatuCount[Tree_Wrong] > 0)
        {
            if (parentItem->_status != Tree_Yes && parentItem->_status != Tree_Wrong)
            {
                parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
                parentItem->getParentItem()->_childStatuCount[Tree_Yes]++;
                parentItem->_status = Tree_Yes;
                emit dataChanged(idx, idx, { MechPreData::Roles::StatusRole });
            }
        }
        else if (parentItem->_status != Tree_Yes)
        {
            parentItem->getParentItem()->_childStatuCount[parentItem->_status]--;
            parentItem->getParentItem()->_childStatuCount[Tree_Yes]++;
            parentItem->_status = Tree_Yes;
            emit dataChanged(idx, idx, { MechPreData::Roles::StatusRole });
        }
        revaluateMyStatus(parentItem->getParentItem());
    }

    //////////////////////////////////////////////////////////////////////////
    // 后处理模型树骨架
    //////////////////////////////////////////////////////////////////////////

    void mPostTreeModel::addPostModelTree(int modelDataID, const QString& treeName)
    {
        //防双根：该模型的旧根已存在时先移除（含索引外残留的"幽灵根"——重复挂载时
        //m_index 同 key 只指向其一，另一条不在索引里、按 key 删不掉，故按顶层
        //children 逐个比对 _modelID 清理）。正常流程建根前模型必新、不会命中，
        //会话恢复二次导入等清理不净的场景由此兜底
        QVector<mPostTreeItem*> staleRoots;
        for (mPostTreeItem* item : m_root->children)
        {
            if (item->key._modelID == modelDataID)
                staleRoots.append(item);
        }
        for (mPostTreeItem* item : staleRoots)
        {
            removeTreeItem(item);
        }

        //树名由调用方传入（GUI 路径取模型数据名称——模型创建时已由 mViewportModelManager
        //命名并写入 modelData，与 mTreeModel::AddModelTree 命名来源统一）；
        //未传名时回退"后处理模型-N"序号命名（按现有顶层树名扫描递增）
        QString name = treeName;
        if (name.isEmpty())
        {
            int maxIndex = 0;
            //本模型顶层行均为后处理模型根，无需再按 isPostModel 过滤
            for (const auto& it : m_root->children)
            {
                if (it->_text.contains("-"))
                {
                    QStringList list = it->_text.split("-");
                    if (list.size() > 1 && list[1].toInt() > maxIndex)
                    {
                        maxIndex = list[1].toInt();
                    }
                }
            }
            name = QString("后处理模型-%1").arg(maxIndex + 1);
        }

        QVector<mPostTreeItem*> childrens;
        mPostTreeItem* parentItem = new mPostTreeItem(modelDataID, PostTreeRootItem, name);
        childrens.push_back(parentItem);
        appendChildren(m_root, std::move(childrens));

        childrens.clear();
        childrens.reserve(10);
        mPostTreeItem* children = new mPostTreeItem(modelDataID, PostModelParentItem, "部件");
        childrens.push_back(children);
        children = new mPostTreeItem(modelDataID, PostModelGroupRootItem, "分组");
        childrens.push_back(children);
        children = new mPostTreeItem(modelDataID, PostMaterialRootItem, "材料");
        childrens.push_back(children);
        children = new mPostTreeItem(modelDataID, PostSectionRootItem, "属性");
        childrens.push_back(children);
        children = new mPostTreeItem(modelDataID, PostSetRootItem, "集合");
        childrens.push_back(children);
        //坐标系/工况/标签/测量：容器骨架节点。"工况"下分"分析工况/派生工况"两类
        //（原型 V5.2）：分析工况子节点由导入完成链路填充（slot_updatePostModelTree）；
        //测量子节点与测量工具面板记录联动，标签子节点与结果标签面板会话记录联动
        //（rebuildMeasureTree/rebuildTagTree，记录变更经全局信号同步）；
        //"坐标系"根默认挂全局坐标系子节点（见下方 appendChildren）
        mPostTreeItem* coordinateRootItem = new mPostTreeItem(modelDataID, PostCoordinateRootItem, "坐标系");
        childrens.push_back(coordinateRootItem);
        mPostTreeItem* caseRootItem = new mPostTreeItem(modelDataID, PostCaseRootItem, "工况");
        childrens.push_back(caseRootItem);
        children = new mPostTreeItem(modelDataID, PostMarkerRootItem, "标签");
        childrens.push_back(children);
        children = new mPostTreeItem(modelDataID, PostMeasureRootItem, "测量");
        childrens.push_back(children);
        //结果容器根（六节点骨架）："结果"根已随 2D 类功能去节点化移除（曲线/Word报告/
        //路径不再建树节点，切换句柄在各参数面板内"已建列表"）；云图根为结果类唯一容器，
        //云图子节点（云图_N）统一挂此节点下
        children = new mPostTreeItem(modelDataID, ResultParentItem, "云图", PostCloudRootPhyID,
            MxFunctions::Tree_Yes, ":/mMainWidgets/Icon/post/svg/yuntu.svg");
        childrens.push_back(children);

        appendChildren(parentItem, std::move(childrens));

        //"坐标系"根默认挂"全局坐标系"子节点（对齐前处理 createGlobalCoordinate 语义）：
        //phyID=PostGlobalCoordinatePhyID，随模型根生死、不可删除（removeRows 拒删）、
        //不可重命名（setData 判型拦截）；坐标系功能（参数面板/轴符号/用户坐标系）待立项
        appendChildren(coordinateRootItem, new mPostTreeItem(modelDataID, PostCoordinateChildItem,
            "全局坐标系", PostGlobalCoordinatePhyID));

        //"工况"两分类骨架（原型 V5.2）：分析工况（子节点=结果工况名，导入完成链路填充）/
        //派生工况（子节点待派生工况功能立项后填充）
        QVector<mPostTreeItem*> caseGroupItems;
        caseGroupItems.push_back(new mPostTreeItem(modelDataID, PostCaseAnalysisRootItem, "分析工况"));
        caseGroupItems.push_back(new mPostTreeItem(modelDataID, PostCaseDerivedRootItem, "派生工况"));
        appendChildren(caseRootItem, std::move(caseGroupItems));

        setCurrentModelID(modelDataID);
    }

    bool mPostTreeModel::removeTreeItem(mPostTreeItem* item)
    {
        if (!item) return false;
        mPostTreeItem* parentItem = item->parent;
        if (!parentItem) return false;          // 没父节点通常代表 root，不建议这样删

        int row = parentItem->children.indexOf(item);
        if (row < 0) return false;

        QModelIndex parentIdx = indexFromItem(parentItem);
        return removeRows(row, 1, parentIdx);
    }

    bool mPostTreeModel::removeTreeChildrenItem(mPostTreeItem* parentItem)
    {
        if (!parentItem) return false;
        int cout = parentItem->children.size();
        if (cout == 0) return false;

        QModelIndex parentIdx = indexFromItem(parentItem);
        return removeRows(0, cout, parentIdx);
    }

    //////////////////////////////////////////////////////////////////////////
    // 分组
    //////////////////////////////////////////////////////////////////////////

    mPostTreeItem* mPostTreeModel::addPostModelGroup(mPostTreeItem* groupRootItem, const QString& name, const QVector<int>& partIds, const QVector<QString>& partNames)
    {
        if (!groupRootItem || groupRootItem->key._treeType != PostModelGroupRootItem) return nullptr;

        // 查找相同名称的分组
        if (findPostModelGroup(groupRootItem, name)) return nullptr;

        // 新分组的 phyID 使用模型级全局递增 ID，避免删除重建后 ID 回绕导致 key 冲突
        int groupId = m_nextGroupID++;
        int modelDataID = groupRootItem->key._modelID;
        mPostTreeItem* groupItem = new mPostTreeItem(modelDataID, PostModelGroupParentItem, name, groupId);
        groupItem->groupPartIds = partIds;

        // 必须先把 groupItem 挂载到 groupRootItem 下，让 groupItem->parent 正确指向 groupRootItem。
        // 如果先 append childItems，groupItem 还没有 parent，appendChildren 内部调用 indexFromItem(groupItem)
        // 会把它当作 m_root 的子节点，导致 beginInsertRows 的 parentIdx 错误，模型索引与树结构不一致。
        appendChildren(groupRootItem, groupItem);

        // 创建引用子节点：每个被分组部件一个节点，显示对应部件名称
        QVector<mPostTreeItem*> childItems;
        childItems.reserve(partIds.size());
        for (int i = 0; i < partIds.size(); ++i)
        {
            QString partName = partNames.size() > i ? partNames[i] : QString();
            mPostTreeItem* child = new mPostTreeItem(modelDataID, PostModelGroupChildItem, partName, partIds[i]);
            // 引用子节点不直接持有分组列表，由父节点统一保存
            childItems.push_back(child);
        }
        appendChildren(groupItem, std::move(childItems));
        return groupItem;
    }

    bool mPostTreeModel::removePostModelGroup(mPostTreeItem* groupItem)
    {
        if (!groupItem || groupItem->key._treeType != PostModelGroupParentItem) return false;
        return removeTreeItem(groupItem);
    }

    bool mPostTreeModel::renamePostModelGroup(mPostTreeItem* groupItem, const QString& newName)
    {
        if (!groupItem || groupItem->key._treeType != PostModelGroupParentItem) return false;

        mPostTreeItem* groupRootItem = groupItem->getParentItem();
        if (findPostModelGroup(groupRootItem, newName)) return false;

        QModelIndex idx = indexFromItem(groupItem);
        if (!idx.isValid()) return false;
        return setData(idx, newName, Qt::EditRole);
    }

    mPostTreeItem* mPostTreeModel::findPostModelGroup(mPostTreeItem* groupRootItem, const QString& name) const
    {
        if (!groupRootItem || groupRootItem->key._treeType != PostModelGroupRootItem) return nullptr;
        for (mPostTreeItem* child : groupRootItem->getChildrenItem())
        {
            if (child->key._treeType == PostModelGroupParentItem && child->_text == name)
                return child;
        }
        return nullptr;
    }

    QVector<int> mPostTreeModel::getPostModelGroupPartIds(mPostTreeItem* groupItem) const
    {
        if (!groupItem || groupItem->key._treeType != PostModelGroupParentItem) return {};
        return groupItem->groupPartIds;
    }

    QHash<QString, QVector<int>> mPostTreeModel::getPostModelGroups(mPostTreeItem* groupRootItem) const
    {
        QHash<QString, QVector<int>> groups;
        if (!groupRootItem || groupRootItem->key._treeType != PostModelGroupRootItem) return groups;
        for (mPostTreeItem* groupItem : groupRootItem->getChildrenItem())
        {
            if (groupItem->key._treeType == PostModelGroupParentItem)
                groups.insert(groupItem->_text, groupItem->groupPartIds);
        }
        return groups;
    }

    int mPostTreeModel::stableNameItemId(const QString& name)
    {
        auto it = _setNameIds.find(name);
        if (it == _setNameIds.end())
        {
            it = _setNameIds.insert(name, m_nextSetItemId++);
        }
        return it.value();
    }

    //////////////////////////////////////////////////////////////////////////
    // QAbstractItemModel 虚函数
    //////////////////////////////////////////////////////////////////////////

    QModelIndex mPostTreeModel::index(int row, int col, const QModelIndex& parentIdx) const
    {
        if (col != 0 || row < 0) return {};
        mPostTreeItem* p = itemFromIndex(parentIdx);
        if (!p || row >= p->children.size()) return {};
        return createIndex(row, col, p->children[row]);
    }

    QModelIndex mPostTreeModel::parent(const QModelIndex& childIdx) const
    {
        if (!childIdx.isValid()) return {};
        mPostTreeItem* child = itemFromIndex(childIdx);
        mPostTreeItem* p = child ? child->parent : nullptr;
        if (!p || p == m_root) return {};

        mPostTreeItem* gp = p->parent ? p->parent : m_root;
        int rowInGp = gp->children.indexOf(p);
        return createIndex(rowInGp, 0, p);
    }

    int mPostTreeModel::rowCount(const QModelIndex& parentIdx) const
    {
        mPostTreeItem* p = itemFromIndex(parentIdx);
        return p ? p->children.size() : 0;
    }

    QVariant mPostTreeModel::data(const QModelIndex& idx, int role) const
    {
        if (!idx.isValid()) return {};
        mPostTreeItem* it = itemFromIndex(idx);

        switch (role) {
        case Qt::DisplayRole: return it->_text;
        case Qt::EditRole:    return it->_text;
        case MechPreData::Roles::StatusRole: return it->_status;
        case MechPreData::Roles::Icon2Role: return it->_iconString;
        case MechPreData::Roles::TypeRole:  return it->key._treeType;
        case MechPreData::Roles::ModelRole:  return it->key._modelID;
        case MechPreData::Roles::PhyidRole:  return it->key._phyID;
        case MechPreData::Roles::CaseidRole: return it->key._caseID;
        default: return {};
        }
    }

    Qt::ItemFlags mPostTreeModel::flags(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    bool mPostTreeModel::setData(const QModelIndex& idx, const QVariant& value, int role)
    {
        if (!idx.isValid()) return false;
        mPostTreeItem* it = itemFromIndex(idx);

        if (role == Qt::EditRole) {
            QString newName = value.toString();
            if (newName.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, "名称不能为空！");
                return false;
            }

            //重命名能力本地判型：本模型只承载后处理节点，不再借 mapPhyTypeForTreeType 的
            //Physical 映射——该映射缺 标量运算/切面图/流线图/Word报告 四类，借道会把用户
            //正常重命名误报成"代码错误"提示；不可重命名的容器/部件等节点给出明确提示
            switch ((BrowserTreeType)it->key._treeType)
            {
            case threeDResultParentItem:
            case animationChildItem:
            case linearChildItem:
            case scalarOperationChildItem:
            case SectionChildItem:
            case streamlineChildItem:
            case CurveChildItem:
            case WordReportChildItem:
            case PathChildItem:
            case isosurfaceChildItem:
            case vectorChildItem:
            //集合子节点（五类）：查重与库内 key 迁移同样经视图注入的 renameValidator 完成
            case PostSetElementChildItem:
            case PostSetNodeChildItem:
            case PostSetFaceChildItem:
            case PostSetEdgeChildItem:
            case PostSetPartChildItem:
            //用户坐标系节点：管理器查重/resetName 同步经 renameValidator 完成
            //（全局坐标系 phyID=-1 在 validator 内拒改，保持"不可重命名"守卫语义）
            case PostCoordinateChildItem:
                //数据管理器的查重与同步经视图注入的 renameValidator 完成（未注入时
                //合法重命名一律拒绝——模型本体不触碰视口/数据管理器单例，保持无头可测）
                if (!renameValidator || !renameValidator(it, newName))
                    return false;
                break;
            case PostModelGroupParentItem:
            {
                //分组为树内自有数据（无管理器同步），仅做兄弟重名校验
                if (findPostModelGroup(it->getParentItem(), newName))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, "该分组名称已存在！");
                    return false;
                }
                break;
            }
            default:
                mGlobalSignals::getInstance()->outputMessageSig(1, "该节点不支持重命名！");
                return false;
            }

            it->_text = newName;

            emit dataChanged(idx, idx, { Qt::DisplayRole });
            return true;
        }
        else if (role == MechPreData::Roles::StatusRole) {
            if (it->_status == value.toInt())
            {
                return true;
            }
            it->getParentItem()->_childStatuCount[it->_status]--;
            it->_status = value.toInt();
            it->getParentItem()->_childStatuCount[it->_status]++;
            revaluateMyStatus(it->getParentItem());
            emit dataChanged(idx, idx, { MechPreData::Roles::StatusRole });
            return true;
        }
        else if (role == MechPreData::Roles::TypeRole) {
            it->key._treeType = value.toInt();
            emit dataChanged(idx, idx, { MechPreData::Roles::TypeRole });
            return true;
        }
        else if (role == MechPreData::Roles::Icon2Role) {
            it->_iconString = value.toString();
            emit dataChanged(idx, idx, { MechPreData::Roles::Icon2Role });
            return true;
        }
        else if (role == Qt::DisplayRole) {
            it->_text = value.toString();
            emit dataChanged(idx, idx, { Qt::DisplayRole });
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // 索引 / 查找
    //////////////////////////////////////////////////////////////////////////

    mPostTreeItem* mPostTreeModel::itemFromIndex(const QModelIndex& idx) const
    {
        return idx.isValid() ? static_cast<mPostTreeItem*>(idx.internalPointer()) : m_root;
    }

    mItemKey mPostTreeModel::itemKeyFromIndex(const QModelIndex& idx) const
    {
        if (!idx.isValid()) return mItemKey();
        auto item = itemFromIndex(idx);

        mItemKey key;
        key._modelID = item->key._modelID;
        key._treeType = item->key._treeType;
        key._phyID = item->key._phyID;
        key._caseID = item->key._caseID;
        return key;
    }

    QModelIndex mPostTreeModel::indexFromKey(const mItemKey& key) const
    {
        auto it = m_index.find(key);
        if (it == m_index.end()) return {};
        mPostTreeItem* item = it.value();
        if (!item || !item->parent) return {};
        int row = item->parent->children.indexOf(item);
        return createIndex(row, 0, item);
    }

    mPostTreeItem* mPostTreeModel::treeItemFromKey(const mItemKey& key) const
    {
        auto it = m_index.find(key);
        if (it == m_index.end()) return nullptr;
        return it.value();
    }

    mPostTreeItem* mPostTreeModel::getParentItem(MxFunctions::BrowserTreeType treeType, int caseID) const
    {
        mItemKey key;
        key._modelID = _currentModelID;
        key._treeType = treeType;
        if (caseID > -999) key._phyID = caseID;
        return treeItemFromKey(key);
    }

    mPostTreeItem* mPostTreeModel::getChildrenItem(MxFunctions::BrowserTreeType treeType, int phyID, int caseID, int modelDataID) const
    {
        mItemKey key;
        key._modelID = modelDataID > -1 ? modelDataID : _currentModelID;
        if (caseID > -1) key._caseID = caseID;

        key._treeType = treeType;
        key._phyID = phyID;
        return treeItemFromKey(key);
    }

    QModelIndex mPostTreeModel::indexFromItem(mPostTreeItem* item) const
    {
        if (!item || item == m_root) return {};

        mPostTreeItem* p = item->parent ? item->parent : m_root;
        int row = p->children.indexOf(item);
        if (row < 0) return {};              // 说明 parent/children 关系不一致
        return createIndex(row, 0, item);    // internalPointer 统一存 mPostTreeItem*
    }

    void mPostTreeModel::setCurrentModelID(int modelID)
    {
        if (modelID != _currentModelID)
        {
            _currentModelID = modelID;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // 删除
    //////////////////////////////////////////////////////////////////////////

    bool mPostTreeModel::removeRows(int row, int count, const QModelIndex& parentIdx)
    {
        mPostTreeItem* p = itemFromIndex(parentIdx);
        if (!p || row < 0 || row + count > p->children.size()) return false;

        //全局坐标系节点不可删除（removeTreeItem/removeIndexs/removeTreeChildrenItem
        //均收口到本函数，此处单点守卫即覆盖全部删除路径）：整批含保护节点时整批
        //拒绝、不做部分删除；删模型根（PostTreeRootItem 整树拆除）不经此分支，不受影响
        for (int i = 0; i < count; ++i)
        {
            const mPostTreeItem* victim = p->children[row + i];
            if (victim->key._treeType == PostCoordinateChildItem
                && victim->key._phyID == PostGlobalCoordinatePhyID)
                return false;
        }

        beginRemoveRows(parentIdx, row, row + count - 1);
        for (int i = 0; i < count; ++i) {
            mPostTreeItem* victim = p->children[row];
            //按"被删子节点自己的状态"记账：按父节点当前状态减会使计数在子节点
            //状态分布不一致时漂移、状态冒泡失真（自 mTreeModel 原样继承的缺陷，两树同修）
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

    void mPostTreeModel::removeIndexs(QModelIndexList indexs)
    {
        std::sort(indexs.begin(), indexs.end(), [](const QModelIndex& a, const QModelIndex& b) {
            if (a.parent() == b.parent()) return a.row() > b.row();
            return a.parent() < b.parent();
            });

        for (const auto& idx : indexs) {
            removeRows(idx.row(), 1, idx.parent());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // mPostTreeItem
    //////////////////////////////////////////////////////////////////////////

    mPostTreeItem::mPostTreeItem(int modelID, MxFunctions::BrowserTreeType treeType, QString text, int phyID, MxFunctions::BrowserTreeStatus statusPic, QString picString)
    {
        key._modelID = modelID;
        key._treeType = treeType;
        key._phyID = phyID;
        _status = (int)statusPic;
        _text = text;
        if (picString.isEmpty()) _iconString = getBrowserTreePic(treeType);
        else _iconString = picString;
        _childStatuCount.resize(4);
        _childStatuCount = { 0, 0, 0, 0 };
    }

    QString mPostTreeItem::getBrowserTreePic(MxFunctions::BrowserTreeType type)
    {
        //仅后处理段类型（自 mTreeItem::getBrowserTreePic 后处理子集同形裁剪）；
        //后处理对象类节点统一走 Icon/post/svg 矢量家族（24x24 视图框，树16px/Ribbon 30px
        //同源缩放，与 mPostMenuContributor Ribbon 按钮共用同一文件，勿改回 png 双份维护）；
        //结构性节点（模型/材料/截面属性/分析工况）仍用 BrowserTree png 与前处理树保持同族
        switch (type)
        {
        case MxFunctions::PostTreeRootItem:
            return ":/mMainWidgets/Icon/BrowserTree/model3x.png";
        case MxFunctions::PostModelParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe3x.png";
        case MxFunctions::PostModelChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        case MxFunctions::PostModelGroupRootItem:
        case MxFunctions::PostModelGroupParentItem:
            return ":/mMainWidgets/Icon/BrowserTree/jihe.png";
        case MxFunctions::PostModelGroupChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        case MxFunctions::ResultParentItem:
            return ":/mMainWidgets/Icon/post/svg/yuntu.svg";
        case MxFunctions::threeDResultParentItem:
            return ":/mMainWidgets/Icon/post/svg/yuntu.svg";
        case MxFunctions::CurveChildItem:
            return ":/mMainWidgets/Icon/post/svg/quxian.svg";
        case MxFunctions::animationChildItem:
            return ":/mMainWidgets/Icon/post/svg/donghua.svg";
        case MxFunctions::PathParentItem:
        case MxFunctions::PathChildItem:
            return ":/mMainWidgets/Icon/post/svg/lujing.svg";
        case MxFunctions::linearChildItem:
            return ":/mMainWidgets/Icon/post/svg/xianxingdiejia.svg";
        case MxFunctions::scalarOperationChildItem:
            return ":/mMainWidgets/Icon/post/svg/biaoliangyunshuan.svg";
        case MxFunctions::SectionChildItem:
            return ":/mMainWidgets/Icon/post/svg/qiemiantu.svg";
        case MxFunctions::isosurfaceChildItem:
            return ":/mMainWidgets/Icon/post/svg/dengzhimian.svg";
        case MxFunctions::vectorChildItem:
            return ":/mMainWidgets/Icon/post/svg/xiangliangtu.svg";
        case MxFunctions::streamlineChildItem:
            return ":/mMainWidgets/Icon/post/svg/liuxiantu.svg";
        case MxFunctions::WordReportChildItem:
            return ":/mMainWidgets/Icon/post/svg/fenxibaogao.svg";
        case MxFunctions::partitionChildItem:
            return ":/mMainWidgets/Icon/post/svg/fenqu.svg";
        case MxFunctions::setPostChildItem:
            return ":/mMainWidgets/Icon/post/svg/jihe.svg";
        case MxFunctions::PostMaterialRootItem:
        case MxFunctions::PostMaterialChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/cailiao.png";
        case MxFunctions::PostMaterialPartItem:
            return ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        case MxFunctions::PostSectionRootItem:
        case MxFunctions::PostSectionChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/jiemianshuxing.png";
        case MxFunctions::PostSectionPartItem:
            return ":/mMainWidgets/Icon/BrowserTree/moxing3x.png";
        case MxFunctions::PostSetRootItem:
            return ":/mMainWidgets/Icon/post/svg/jihe.svg";
        case MxFunctions::PostSetElementRootItem:
        case MxFunctions::PostSetElementChildItem:
            return ":/mMainWidgets/Icon/post/svg/danyuanji.svg";
        case MxFunctions::PostSetNodeRootItem:
        case MxFunctions::PostSetNodeChildItem:
            return ":/mMainWidgets/Icon/post/svg/jiedianji.svg";
        case MxFunctions::PostSetFaceRootItem:
        case MxFunctions::PostSetFaceChildItem:
            return ":/mMainWidgets/Icon/post/svg/danmianji.svg";
        case MxFunctions::PostSetEdgeRootItem:
        case MxFunctions::PostSetEdgeChildItem:
            return ":/mMainWidgets/Icon/post/svg/danbianji.svg";
        case MxFunctions::PostSetPartRootItem:
        case MxFunctions::PostSetPartChildItem:
            return ":/mMainWidgets/Icon/post/svg/bujianji.svg";
        //全局坐标系子节点默认直角坐标系图（rebuildCoordinateTree 按类型覆写柱/球）
        case MxFunctions::PostCoordinateRootItem:
        case MxFunctions::PostCoordinateChildItem:
            return ":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg";
        case MxFunctions::PostCaseRootItem:
        case MxFunctions::PostCaseAnalysisRootItem:
        case MxFunctions::PostCaseAnalysisChildItem:
            return ":/mMainWidgets/Icon/BrowserTree/fenxi.png";
        case MxFunctions::PostCaseDerivedRootItem:
        case MxFunctions::PostCaseDerivedChildItem:
            return ":/mMainWidgets/Icon/post/svg/paishenggongkuang.svg";
        case MxFunctions::PostMeasureRootItem:
        case MxFunctions::PostMeasureChildItem:
            return ":/mMainWidgets/Icon/post/svg/celianggongju.svg";
        case MxFunctions::PostMarkerRootItem:
        case MxFunctions::PostMarkerChildItem:
            return ":/mMainWidgets/Icon/post/svg/biaoqian.svg";
        //注释分支（Ribbon"注释"页四个创建钮的树侧对应节点）
        case MxFunctions::NoteParentItem:
            return ":/mMainWidgets/Icon/post/svg/zhushi.svg";
        case MxFunctions::LineChildItem:
            return ":/mMainWidgets/Icon/post/svg/erweixian.svg";
        case MxFunctions::ArrowChildItem:
            return ":/mMainWidgets/Icon/post/svg/jiantou.svg";
        case MxFunctions::TextChildItem:
            return ":/mMainWidgets/Icon/post/svg/wenben.svg";
        case MxFunctions::LogChildItem:
            return ":/mMainWidgets/Icon/post/svg/log.svg";
        default:
            break;
        }
        return QString();
    }
}
