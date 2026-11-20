#pragma once
#pragma execution_character_set("utf-8")

#include <functional>
#include <QSortFilterProxyModel>
#include <QCollator>
#include <QIcon>
#include <QHash>
#include <QVector>
#include "MPreData_global.h"
#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mPhysicalFieldEnum.h"

namespace MechPreData
{
    /*
    * 视图树节点类型（存于视口记忆 key 与视图树专用模型）。
    * BrowserTreeType 枚举位于 MxFunctions 共享库且其取值顺序有语义（前处理 < PostTreeRootItem < 后处理），
    * 故视图树节点类型在本地定义，取值避开枚举区间，存入 mItemKey._treeType(int)。
    */
    namespace ViewTreeTypes
    {
        //视图树页节点（每页对应一组视口）
        constexpr int ViewTreePageItem = 10002;
        //视图树视口子节点（phyID 存视口ID）
        constexpr int ViewTreeViewportItem = 10003;

        inline bool isLocalTreeType(int type)
        {
            return type >= ViewTreePageItem && type <= ViewTreeViewportItem;
        }
    }

    //模型树“装配”分组整体开关：false 时分组不创建，装配树填充/右键菜单随分组缺席整体失效
    //（fillAssemblyTree、createAssemblyParentMenu 等实现保留，置 true 即恢复，后续再开发）
    inline constexpr bool AssemblyTreeEnabled = false;

    //该类请不要增加任何接口！！！！！有问题请咨询 帅哥-SWC
    struct MPREDATA_EXPORT mTreeItem
    {
        //构造函数
        mTreeItem(int modelID, MxFunctions::BrowserTreeType treeType, QString text = "", int phyID = 0,
            MxFunctions::BrowserTreeStatus statusPic = MxFunctions::Tree_Yes, QString picString = "");

        void setCaseID(int caseID) { key._caseID = caseID; }

        //获取子节点
        QVector<mTreeItem*> getChildrenItem() { return children; }

        //获取子节点个数
        int getChildrenSize() { return children.size(); }

        //获取父节点
        mTreeItem* getParentItem() { return parent; }

        friend class mTreeModel;
    private:
        /*
        * 模型树节点类型图片
        */
        QString getBrowserTreePic(MxFunctions::BrowserTreeType type);

        mTreeItem() { key._modelID = -1; key._phyID = -1; key._treeType = MxFunctions::TreeRootItem; }
    private:
        mTreeItem* parent = nullptr;
        QVector<mTreeItem*> children;
        //子节点状态数量
        std::vector<int> _childStatuCount;

        // 展示
        QString _text;
        int _status;
        QString _iconString;

        // 业务字段
        mItemKey key;
        // 隐藏（名称/状态过滤使用）
        bool hidden = false;
    };

    namespace Roles {
        enum {
            TypeRole = Qt::UserRole + 1,
            ModelRole,
            PhyidRole,
            CaseidRole,
            HiddenRole,
            StatusRole,
            Icon2Role
        };
    }

    class MPREDATA_EXPORT mTreeModel : public QAbstractItemModel {
        Q_OBJECT
    public:
        //partGroupUnderMotionRoot：多体形态开关——true 时"部件"分组（ModelParentItem）挂
        //多体根首位而非模型根下。构造函数内即执行 AddModelTree 建树，标志必须随构造传入，
        //事后设置无效（形态进程内不变，由 mTreeView 构造时按 resolveRibbonForm 决定）
        explicit mTreeModel(int modelDataID, bool partGroupUnderMotionRoot = false, QObject* parent = nullptr);
        ~mTreeModel() override;

        QModelIndex index(int row, int col, const QModelIndex& parent) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex&) const override { return 1; }

        QVariant data(const QModelIndex& idx, int role) const override;
        bool setData(const QModelIndex& idx, const QVariant& value, int role) override;
        Qt::ItemFlags flags(const QModelIndex& idx) const override;

        bool removeRows(int row, int count, const QModelIndex& parent) override;
        void removeIndexs(QModelIndexList indexs);

        mTreeItem* getParentItem(MxFunctions::BrowserTreeType treeType, int caseID = -999) const;
        QModelIndex getParentIndex(MxFunctions::BrowserTreeType treeType, int caseID = -999) const;
        mTreeItem* getChildrenItem(MxFunctions::BrowserTreeType treeType, int phyID, int caseID = -1, int modelDataID = -1) const;
        QModelIndex indexFromItem(mTreeItem* item) const;
        mTreeItem* itemFromIndex(const QModelIndex& idx) const;
        mItemKey itemKeyFromIndex(const QModelIndex& idx) const;
        QModelIndex indexFromKey(const mItemKey& key) const;
        mTreeItem* treeItemFromKey(const mItemKey& key) const;
        //增加前处理模型树
        void AddModelTree(int modelDataID);
        //增加多体动力学顶层根树（与模型根平级，固定分组常驻）
        void addMotionRootTree(int modelDataID);
        //清空模型树
        void clearModelTree(mTreeItem* item, int newModelID);
        //移除自身及其子节点
        bool removeTreeItem(mTreeItem* item);
        bool removeTreeChildrenItem(mTreeItem* parentItem);
        //按名称过滤s
        void nameFilterTree(const QString& filterStr);
        void statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus);
        //判断该节点是否存在
        bool isExistItem(MxFunctions::BrowserTreeType treeType, int modelDataID = -1, int phyID = -1, int caseID = -1) const;

        void setHiddenByKey(const mItemKey& key, bool hidden);
        void setCurrentModelID(int modelID);
        int getCurrentModelID() const { return _currentModelID; }
        void setSelectRenames(bool isSelectRenames) { _isSelectRenames = isSelectRenames; }

        // 批量构建（一次性插入很多孩子，效率更高）
        void appendChildren(mTreeItem* parentItem, QVector<mTreeItem*>&& newChildren);
        void appendChildren(mTreeItem* parentItem, mTreeItem* newChildren);
        void insertChildren(mTreeItem* parentItem, mTreeItem* newChildren, MxFunctions::BrowserTreeType treeType);
        void insertChildren(mTreeItem* parentItem, mTreeItem* newChildren, mTreeItem* lastItem);
        // 挂载装配树实例节点并记录父装配ID（同一装配/部件被多父引用时树实例按 key._parentID 区分）
        void appendAssemblyItem(mTreeItem* parentItem, mTreeItem* childItem, int parentAssemblyId);

        //递归更新父节点状态
        void revaluateMyStatus(mTreeItem* parentItem);
    private:
        inline void deleteTreeItem(mTreeItem* item);
        bool treeItemFilter(const QString& filterStr, mTreeItem* parentItem);
        bool treeItemFilter(MxFunctions::BrowserTreeStatus filterStatus, mTreeItem* parentItem);
        bool LoadBoundaryRename(mTreeItem* item, const QString& newName);
    private:
        mTreeItem* m_root = nullptr;
        QHash<mItemKey, mTreeItem*> m_index;
        int _currentModelID;
        bool _isSelectRenames = false;
        //"部件"分组挂多体根（多体形态）开关，见 setPartGroupUnderMotionRoot
        bool _partGroupUnderMotionRoot = false;


        void indexItemRecursive(mTreeItem* item);
        void deindexItemRecursive(mTreeItem* item);
    };

    class MPREDATA_EXPORT mTreeProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
    public:
        using QSortFilterProxyModel::QSortFilterProxyModel;
        enum class SortMode { DefaultOrder = 0, Alpha, SuffixNumber };

        void setSortMode(SortMode m, Qt::SortOrder sortType);
        SortMode sortMode() const { return m_mode; }

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& a, const QModelIndex& b) const override;

    private:
        SortMode m_mode = SortMode::DefaultOrder;
    };
}



