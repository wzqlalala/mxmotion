#pragma once
#pragma execution_character_set("utf-8")

//Qt
#include <QAbstractItemModel>
#include <QVector>
#include <QHash>
#include <functional>

//MxFunctions
#include "mModelEnum.h"
#include "mModelStruct.h"

//前处理共享树模型的 Roles 常量：GUIFoundation 的 mTreeDelegate / mTreeItemUtil 按数值读取，
//本模型直接复用 MechPreData::Roles 保证角色数值一致，不引入两套数值漂移
#include "mTreeModel.h"

#include "mpostwindow_global.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
    //////////////////////////////////////////////////////////////////////////
    // 云图根节点（默认骨架，与"结果"同级）：复用 ResultParentItem 类型、以 phyID 区分，
    // "结果"为 -1，"云图"为 PostCloudRootPhyID；云图子节点（云图_N）统一挂"云图"根下
    //////////////////////////////////////////////////////////////////////////
    inline constexpr int PostCloudRootPhyID = -2;

    //////////////////////////////////////////////////////////////////////////
    // 全局坐标系节点 phyID（"坐标系"根下随骨架默认创建的唯一子节点，不可删除）：
    // -1 对齐前处理全局坐标系 ID=-1 惯例（仅取值口径，不涉前处理数据结构）；类型独立
    // （PostCoordinateChildItem）故与"结果"根的 -1 不冲突。removeRows 按此拒删
    //////////////////////////////////////////////////////////////////////////
    inline constexpr int PostGlobalCoordinatePhyID = -1;

    //////////////////////////////////////////////////////////////////////////
    // 父容器根 phyID → 工况ID 归一（单一收口点）：云图根（PostCloudRootPhyID）为纯容器
    // 非工况，结果数据注册在 -1 下。所有"从节点/父节点取工况"的消费点一律经此归一，
    // 不得各自手写 if——历史上漏归一已造成两批缺陷（合并漏改 11 处按钮失效、
    // 分区显示单击着色失效），新增调用点必须走本函数
    //////////////////////////////////////////////////////////////////////////
    inline int normalizeCaseId(int rawCaseId) { return rawCaseId == PostCloudRootPhyID ? -1 : rawCaseId; }

    //////////////////////////////////////////////////////////////////////////
    // 后处理树节点：自持于 PostWindow，独立于前处理共享树模型的 MechPreData::mTreeItem。
    // 节点身份仍用 MxFunctions::mItemKey（modelID/treeType/phyID/caseID），
    // 以保持视口选中记忆（QList<mItemKey>）与全局信号签名不变
    //////////////////////////////////////////////////////////////////////////
    struct MPOSTWINDOW_EXPORT mPostTreeItem
    {
        mPostTreeItem(int modelID, MxFunctions::BrowserTreeType treeType, QString text = "", int phyID = 0,
            MxFunctions::BrowserTreeStatus statusPic = MxFunctions::Tree_Yes, QString picString = "");

        void setCaseID(int caseID) { key._caseID = caseID; }

        QVector<mPostTreeItem*> getChildrenItem() { return children; }

        int getChildrenSize() { return children.size(); }

        mPostTreeItem* getParentItem() { return parent; }

        friend class mPostTreeModel;
    private:
        QString getBrowserTreePic(MxFunctions::BrowserTreeType type);

        mPostTreeItem() { key._modelID = -1; key._phyID = -1; key._treeType = MxFunctions::TreeRootItem; }

    private:
        mPostTreeItem* parent = nullptr;
        QVector<mPostTreeItem*> children;
        //子节点状态数量
        std::vector<int> _childStatuCount;

        // 展示
        QString _text;
        int _status;
        QString _iconString;

        // 业务字段
        mItemKey key;
        // 分组：本节点所属分组列表（用于 PostModelGroupParentItem；非持久化）
        QVector<int> groupPartIds;
    };

    //////////////////////////////////////////////////////////////////////////
    // 后处理树数据模型：后处理模型树的独立模型（自 MechPreData::mTreeModel 裁剪而来），
    // 顶层行即各后处理模型根（PostTreeRootItem），不再借用前处理树实例
    // （browserTreeModelProvider 通道废止）。API 与 mTreeModel 同形，
    // 视图侧既有调用零改写；差异点：
    //   - 构造不创建任何骨架（前处理模型由 AddModelTree 建骨架，本模型由 addPostModelTree 建）；
    //   - 无 hidden 标志（后处理代理 mPostTreeProxy 自持过滤状态）；
    //   - 重命名能力由 setData 内本地判型（renamablePostTreeType 语义，见 cpp），
    //     不再借 mapPhyTypeForTreeType 的 Physical 映射——该映射缺后处理四类节点
    //////////////////////////////////////////////////////////////////////////
    class MPOSTWINDOW_EXPORT mPostTreeModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        explicit mPostTreeModel(QObject* parent = nullptr);
        ~mPostTreeModel() override;

        QModelIndex index(int row, int col, const QModelIndex& parent) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex&) const override { return 1; }

        QVariant data(const QModelIndex& idx, int role) const override;
        bool setData(const QModelIndex& idx, const QVariant& value, int role) override;
        Qt::ItemFlags flags(const QModelIndex& idx) const override;

        bool removeRows(int row, int count, const QModelIndex& parent) override;
        void removeIndexs(QModelIndexList indexs);

        mPostTreeItem* getParentItem(MxFunctions::BrowserTreeType treeType, int caseID = -999) const;
        mPostTreeItem* getChildrenItem(MxFunctions::BrowserTreeType treeType, int phyID, int caseID = -1, int modelDataID = -1) const;
        QModelIndex indexFromItem(mPostTreeItem* item) const;
        mPostTreeItem* itemFromIndex(const QModelIndex& idx) const;
        mItemKey itemKeyFromIndex(const QModelIndex& idx) const;
        QModelIndex indexFromKey(const mItemKey& key) const;
        mPostTreeItem* treeItemFromKey(const mItemKey& key) const;

        // 增加后处理模型树：根（treeName，缺省回退"后处理模型-N"序号）+ 七个固定骨架节点：
        // 部件/分组/材料/属性/集合/结果/云图（根节点恒存在，数据子节点由导入流程填充）
        // 树名由调用方传入（GUI 路径取模型数据名称）；本模型保持无头纯净，不触碰视口/数据管理器单例
        void addPostModelTree(int modelDataID, const QString& treeName = QString());
        // 移除自身及其子节点
        bool removeTreeItem(mPostTreeItem* item);
        bool removeTreeChildrenItem(mPostTreeItem* parentItem);

        // 后处理模型分组相关（非持久化，重建时按名称恢复）
        mPostTreeItem* addPostModelGroup(mPostTreeItem* groupRootItem, const QString& name, const QVector<int>& partIds, const QVector<QString>& partNames = QVector<QString>());
        bool removePostModelGroup(mPostTreeItem* groupItem);
        bool renamePostModelGroup(mPostTreeItem* groupItem, const QString& newName);
        mPostTreeItem* findPostModelGroup(mPostTreeItem* groupRootItem, const QString& name) const;
        QVector<int> getPostModelGroupPartIds(mPostTreeItem* groupItem) const;
        // 获取分组根节点下所有分组的名称与部件 ID 列表（用于非持久化重建）
        QHash<QString, QVector<int>> getPostModelGroups(mPostTreeItem* groupRootItem) const;

        // 名称记忆稳定ID：集合/材料/属性等"按名称索引、无数字ID"的数据源，重建子树时
        // 用遍历序号作 phyID 会在重建顺序变化时使 key 漂移（视口选中记忆失效）——
        // 经此按名称取ID：同名复用旧ID、新名发新ID（模型级进程内稳定，不持久化）
        int stableNameItemId(const QString& name);

        // 重命名校验/同步钩子（视图侧注入）：本模型只承载后处理节点，重命名对数据
        // 管理器的查重与改名同步属视图层职责——模型 setData(EditRole) 对可重命名类型
        // 先调本钩子，返回 false 拒绝改名。未注入（无头测试/模型独立使用）时合法重命名
        // 路径一律返回 false（分组等纯树内数据的改名不依赖钩子，仍可用）
        std::function<bool(mPostTreeItem*, const QString&)> renameValidator;

        void setCurrentModelID(int modelID);
        int getCurrentModelID() const { return _currentModelID; }
        void setSelectRenames(bool isSelectRenames) { _isSelectRenames = isSelectRenames; }

        // 批量构建（一次性插入很多孩子，效率更高）
        void appendChildren(mPostTreeItem* parentItem, QVector<mPostTreeItem*>&& newChildren);
        void appendChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren);
        void insertChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren, MxFunctions::BrowserTreeType treeType);
        void insertChildren(mPostTreeItem* parentItem, mPostTreeItem* newChildren, mPostTreeItem* lastItem);

        //递归更新父节点状态
        void revaluateMyStatus(mPostTreeItem* parentItem);

    private:
        inline void deleteTreeItem(mPostTreeItem* item);

        void indexItemRecursive(mPostTreeItem* item);
        void deindexItemRecursive(mPostTreeItem* item);

    private:
        mPostTreeItem* m_root = nullptr;
        QHash<mItemKey, mPostTreeItem*> m_index;
        QHash<QString, int> _setNameIds;   // stableNameItemId 的名称记忆（见公有区注释）
        int m_nextSetItemId = 1;
        int _currentModelID = -1;
        bool _isSelectRenames = false;
        int m_nextGroupID = 1;
    };
}
