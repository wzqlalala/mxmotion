#pragma once
#pragma execution_character_set("utf-8")

//Qt
#include <QWidget>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QMenu>
#include <QHash>
#include <set>

class QToolBar;
class QLabel;
class QTimer;

//MxFunctions
#include "mModelEnum.h"

//独立后处理树模型的索引/项访问辅助
#include "mTreeItemUtil.h"

//对外结果服务契约（本类实现 IPostResultService）与结果跳转注册表
#include "mPostResultServiceProvider.h"
#include "mPostResultRegistry.h"

//后处理视口命名协议（单一铸造源）：全部视口名构造/识别集中于此，禁止散拼
#include "mPostViewportNaming.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace MechMainWindow
{
    class mTreeView;
}

namespace PostWindow
{
    class mPostTreeItem;
    class mPostTreeModel;

    //////////////////////////////////////////////////////////////////////////
    // 后处理树过滤代理：源模型为独立的 mPostTreeModel（顶层行即后处理模型根）；
    // 名称/状态过滤自持于本代理（不写模型标志位），
    // 排序模式与前处理树代理（mTreeProxyModel）一致：默认序/拼音字母/后缀序号
    //////////////////////////////////////////////////////////////////////////
    class mPostTreeProxy : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        using QSortFilterProxyModel::QSortFilterProxyModel;
        enum class SortMode { DefaultOrder = 0, Alpha, SuffixNumber };

        void setNameFilter(const QString& name) { _nameFilter = name; invalidate(); }
        void setStatusFilter(int status) { _statusFilter = status; invalidate(); }   // -1 表示不过滤
        void setSortMode(SortMode m, Qt::SortOrder sortType);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& a, const QModelIndex& b) const override;

    private:
        QString _nameFilter;
        int _statusFilter = -1;
        SortMode _mode = SortMode::DefaultOrder;
        mutable QCollator* _collator = nullptr;
    };

    //////////////////////////////////////////////////////////////////////////
    // 后处理树视图：数据模型为本模块独立的 mPostTreeModel（不再共享前处理树实例）；
    // 单击/右键派发、菜单与树构建/更新均为后处理子树专属交互（自 mTreeView 移植）
    //////////////////////////////////////////////////////////////////////////
    class mPostTreeView : public QTreeView
    {
        Q_OBJECT
    public:
        explicit mPostTreeView(QWidget* parent = nullptr);

        //当前选中映射回共享模型的源索引
        QModelIndexList selectedSourceIndexes() const;

        //选中指定源索引对应的行（供结果树更新后选中新节点使用）
        void selectSourceIndex(const QModelIndex& sourceIdx);

        //过滤/排序（供窗口工具条调用；过滤状态自持于代理，不影响前处理树）
        void nameFilterTree(const QString& filterStr);
        void statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus);
        void sortTree(int sortWay, Qt::SortOrder sortType);

        //树构建（自 mTreeView 移植；后处理模型树的建/删入口）
        void addPostModelBrowserTree();
        void deletePostModelBrowserTree();
        //按显式模型ID删除（导入失败回滚用）：不经 getCVM 当前模型，规避失败瞬间
        //激活已漂移的时序风险（无参版仍按当前模型，供树右键"删除模型"使用）
        void deletePostModelBrowserTree(int modelDataID);

        //树模型访问（会话采集/回放经此遍历后处理树）
        mPostTreeModel* getPostTreeModel() const { return _treeModel; }

        //--- 会话保存/恢复（.mxses）支持：云图条目采集与回放 ---
        //采集指定后处理模型的云图条目（itemName, viewName；按云图根下节点序）
        QList<QPair<QString, QString>> collectCloudItems(int modelDataID);
        //回放云图条目：视口已由会话编排按保存名重建，此处按名重绑条目（缺失视口
        //按保存名补建）；首个条目无条件触发 create3DResultViewport 以绑定面板
        //_curPostRend（回放先于 updateResultTree 运行，云图根非空会使其跳过
        //初始视口创建，面板绑定须由本回放补上，否则导入收尾链早退）
        void replayCloudItems(int modelDataID, const QList<QPair<QString, QString>>& items);

        //后处理导入完成链路入口：按后处理树选中（无选中时为后处理模型根）
        //创建三维结果节点并激活视口（原经 mTreeView 派发的兼容路径）
        void updateResultTree();

        /*
        * "派生工况"分支子节点重建（批次A0）：自 mPostDerivedCaseStore 按模型
        * 实例枚举全部 kind 定义建 PostCaseDerivedChildItem 子节点（phyID=定义id）。
        * 导入完成链（slot_updatePostModelTree）、store 变更防抖、会话恢复三路共用。
        */
        void rebuildDerivedCaseTree(int modelID);

    private slots:
        //单击派发：后处理节点类型 → 参数面板 + ribbon 切换 + 渲染联动
        void slot_singleClickItem();

        //右键派发：后处理节点类型 → 对应右键菜单（菜单实现均在本文件内）
        void slot_rightClickItem();

        //选中变化（mTreeView 侧为符号联动；后处理树联动在单击派发内完成，此处空实现）
        void slot_itemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

        //视口激活（自 mTreeView::slot_subWindowActivated 收编的后处理职责）：
        //同步本模型 currentModelID；后处理模型激活时按视口选中记忆恢复后处理树选中
        void slot_viewportActivated(int viewportID);

        //创建注释节点（clickPostNoteSig；自 mTreeView::slot_createNote 移植，休眠路径保留语义）
        void slot_createNote();

        //移除后处理模型树根（removePostModelTreeSig；前处理侧删除后处理模型时经总线请求）
        void slot_removePostModelTree(int modelDataID);

        //——以下槽自 mTreeView 移植（右键菜单动作 / 全局信号驱动）——
        void slot_menuShowAllPostPart();
        void slot_menuHideAllPostPart();
        void slot_menuReverseShowPostPart();
        void slot_menuShowCurrentPostPart();
        void slot_menuHideCurrentPostPart();
        void slot_menuHideOtherPostPart();
        void slot_createPostModelGroup();
        void slot_showPostModelGroup();
        void slot_hidePostModelGroup();
        void slot_deletePostModelGroup();
        void slot_menuChildRename();
        void slot_showPostMaterialSectionParts();
        void slot_hidePostMaterialSectionParts();
        void slot_hideOtherPostMaterialSectionParts();
        void slot_showPostElementSet();
        void slot_hidePostElementSet();
        void slot_hideOtherPostElementSet();
        void slot_showPostPartSet();
        void slot_hidePostPartSet();
        void slot_hideOtherPostPartSet();
        void slot_showAllPostElementSet();
        void slot_hideAllPostElementSet();
        void slot_showOnlyPostElementSet();
        void slot_showAllPostPartSet();
        void slot_hideAllPostPartSet();
        void slot_showOnlyPostPartSet();
        //节点/面/边集显隐槽已随显隐功能取消（用户定案 2026-11-12）移除
        //删除选中集合（五类集合子节点通用；从库内哈希移除并刷树，面板同步复位）
        void slot_deletePostSet();
        void slot_switchCloudPanel();
        //slot_create3DResult 随"新建云图入口取消"移除（hub create3DResult 保留给导入链）
        void slot_createCurve();
        void slot_createExportWord();
        void slot_createParrayMimage();
        void slot_createCompVideo();
        void slot_createExplode();
        void slot_createMeasureTool();
        void slot_createResultPath();
        void slot_createSectionForce();
        void slot_createLabel();
        void slot_createTag();
        void slot_createAnimation(int type);
        void slot_createLinearSuperposition();
        void slot_createScalarOperation();
        void slot_createSection();
        void slot_createIsosurface();
        void slot_createVector();
        void slot_createStreamLine();
        void slot_createPartition();
        void slot_createPostSet();
        //模型页（页键"模型#Post"）按类型创建：setType 0部件集/1节点集/2单元边集/3单元面集/4单元集
        void slot_createPostSetTyped(int setType);
        //模型页创建后处理坐标系（0直角/1柱/2球），坐标系功能待接入（树/面板/渲染从零建）
        void slot_createPostCoordinate(int coordType);
        void slot_createLine();
        void slot_createArrow();
        void slot_createText();
        void slot_createLog();
        void slot_createPath();
        void slot_menu3DResultDelete();
        void slot_menu3DResultUpdate();
        //slot_menuCurveDelete/slot_menuExportWordDelete/slot_menuPathDelete 随 2D 类
        //去节点化移除（删除入口在各参数面板"已建列表"旁）；
        //slot_menuAnimationDelete/LinearDelete/ScalarOperationDelete/SectionDelete/IsoDelete/
        //VectorDelete 随派生去节点化移除（派生节点不存在，删除经视图树删派生视口）
        void slot_updatePostModelTree();
        //测量分支子节点重建（slot_updatePostModelTree 与 slot_updateMeasureTree 共用）
        void rebuildMeasureTree(int modelID);
        //树"测量"分支高亮同步：高亮永远跟随面板编辑对象（V1.3"延迟切换后高亮
        //与编辑对象一致"）；recordId<0 清测量分支选中
        void syncMeasureHighlight(int recordId);
        //store 定义集变更承接（防抖定时器超时 → rebuildDerivedCaseTree；批次A0）
        void slot_derivedStoreChanged();
        //右键"编辑"：派生工况定义节点单击回编（经 hub editDerivedCase 按 kind 路由）
        void slot_editDerivedCaseDef();
        //右键"删除"：store 删除定义（被引用时提示先删引用方），分支即时重建
        void slot_removeDerivedCaseDef();
        //测量记录变更（postMeasureRecordsChangedSig）：同步"测量"分支子节点
        void slot_updateMeasureTree();
        //面板编辑对象变化（measureEditingChangedSig）：测量分支高亮跟随
        void slot_measureEditingChanged(int recordId);
        //右键"编辑"：测量记录节点回编（hub editMeasure 打开面板并定位记录进编辑）
        void slot_editMeasureRecord();
        //右键"显示/隐藏/删除"：作用于全部选中测量记录节点（经面板公开接口，树不
        //直接改数据——单一数据源；编辑对象回落/树重建由面板信号驱动）
        void slot_showMeasureRecords();
        void slot_hideMeasureRecords();
        void slot_removeMeasureRecords();
        //右键显隐批量辅助：全部选中测量记录节点 phyID 先收集后作用（面板调用会
        //触发树重建使选中索引失效，不可边遍历边删）
        void setMeasureRecordsVisible(bool visible);
        //选中测量记录节点 phyID 收集（批量显隐/删除共用；空=无测量节点选中）
        QList<int> selectedMeasureRecordIds() const;
        //标签分支子节点重建（slot_updatePostModelTree 与 slot_updateTagTree 共用；
        //记录源=结果标签面板会话记录，经 hub tagTreeRecords 快照读取）
        void rebuildTagTree(int modelID);
        //标签记录变更（postTagRecordsChangedSig，标签面板记录结构变化后发射）：
        //同步当前模型"标签"分支子节点
        void slot_updateTagTree();
        //坐标系分支子节点重建（"坐标系"根 ← MechCoordinateData 管理器；全局坐标系节点
        //恒为首位随根重建，用户坐标系按管理器 id 序追加）
        void rebuildCoordinateTree(int modelID);
        //坐标系数据变更（createUpdateCoordinateSig，面板/树侧菜单创建·删除·复制·
        //重命名后发射）：同步当前模型"坐标系"分支子节点
        void slot_updatePostCoordinateTree();
        //右键"编辑"：用户坐标系节点回编（hub editCoordinate 打开面板回填该坐标系）
        void slot_editCoordinateNode();
        //右键"删除"：管理器删除选中坐标系（面板正编辑该对象时先退编辑态），分支重建
        void slot_removeCoordinateNode();
        //右键"复制"：管理器拷贝生成新坐标系，分支重建
        void slot_copyCoordinateNode();
        //右键"创建坐标系"：走模型页同款入口链（含结果守卫，默认直角坐标系）
        void slot_menuCoordinateNew();
        //右键"删除所有坐标系"：清空用户坐标系（管理器保全局），分支重建
        void slot_menuCoordinateDeleteAll();
        void slot_updatePostBrowserTreeStatus(MxFunctions::BrowserTreeType type, int phyID, int caseID, MxFunctions::BrowserTreeStatus statu);
        //slot_updatePostCurveTree/slot_updatePostExportWordTree/slot_updatePostPathTree 随
        //去节点化移除（面板自登记实例，见 mPostCurve/mPostExportWord/mPostPath）
        void slot_updateScalarOperationTree();
        void slot_updateLineTree();
        void slot_updateArrowTree();
        void slot_updateTextTree();
        void slot_updateLogTree();
        void slot_updateAnimationTree(int type);
        void slot_updateLinearSuperpositionTree();
        void slot_updateSectionTree();
        void slot_updateIsosurfaceTree();
        void slot_updateCloudPresetTree(QString viewName, int presetIndex);
        void slot_updateVectorTree();
        void slot_updateStreamLineTree();
        void slot_updatePostSetTree();

    private:
        //选中同步到当前视口（渲染拾取联动）
        void updateItemSelected();

        //树范围收敛：仅显示当前后处理模型的根（视口激活联动，同前处理树
        //updateTreeSectionsForViewport 的 setRowHidden 方案；视图级行隐藏保持展开状态）
        void scopeToCurrentModel();

        //按视口名在当前模型子树内反查结果节点（manager 的 viewName 与视口名同源），
        //命中节点即"该视口对应的结果项"；未命中返回 nullptr（调用方回退选中记忆链路）
        mPostTreeItem* findNodeByViewportName(const QString& viewportName);
        mPostTreeItem* findNodeByViewportNameRecursive(mPostTreeItem* item, const QString& viewportName);

        //分区显示：按分区集合覆盖单元颜色
        void updatePartitionElementColor(const QHash<int, std::set<int>>& partitionElements);

        //结果视口显示名接管（参考前处理"模型名· 视口名"）：云图/动画/切面等 3D 结果
        //视口创建后把协议名换成 "{模型名}· {树节点名}"——renameViewport 自动同步渲染器
        //查找键/标题栏/视图树；条目 viewName 经 resetItemViewName 跟随（树↔视口反查与
        //单击派发按它取视口）；guidata 挂在视口私有对象上随视口走，重命名无需迁移。
        //节点类型经 viewportLookupItemType 映射条目类型（单一口径源）。成功返回显示名，
        //失败返回空串（保留协议名不阻断业务）
        QString renameItemViewportDisplay(MxFunctions::BrowserTreeType nodeType, const QString& itemName, const QString& viewportName);

        //无云图时模型仅有初始视口：返回该模型唯一 Post3D 视口的当前名（可能已被
        //显示名接管，不再是协议名；无则空串）——裸初始视口的结构化定位，
        //替代旧的协议名字面量识别
        QString solePost3DViewportName(int modelID);

        //宽松工况上下文（功能区按钮以激活视口/选中上下文为准，不要求精确选中
        //"结果"根）：结果父节点取其工况，结果子节点（云图等）取父节点工况，
        //其余（含无选中）按默认上下文 -1；云图根容器哨兵（-2）一并归一
        int contextCaseId();

        //激活视口→源云图节点解析（派生功能绑定激活视口的单一收口）：① 激活 Post3D
        //视口按名反查节点（云图直接命中，派生节点取父）；② 选中云图节点；
        //③ 云图根下首个云图。无任何云图返回 nullptr
        mPostTreeItem* resolveActiveCloudNode();

        //源云图视口名解析：有云图节点→条目视口名；无节点（刚导入）→唯一 Post3D 视口
        //（裸初始视口）自身即源（不建云图节点）；皆无返回空串
        QString resolveActiveCloudViewName();

        //——云图缓存节点（去视口化 2026-11-05：仅参数快照，不建视口）——
        //缓存节点判定：云图节点条目 viewName 为空＝缓存标记
        bool isCloudPresetNode(mPostTreeItem* item);
        //云图根下"真实"云图子节点数（缓存节点不占位：收编初始视口/裸视口绑定等
        //以"有无真实云图"为前提的判定不得被缓存节点污染）
        int realCloudChildCount(mPostTreeItem* cloudRootItem);
        //单击缓存节点恢复：按节点名反查快照回填到目标云图视口并重绘（回填+应用渲染）
        void restoreCloudPreset(const QString& itemName);
        //缓存节点改名/删除时同步快照名（newName 空＝删除快照）
        void syncCloudPresetName(const QString& oldName, const QString& newName);

        //——右键菜单（自 mTreeView 移植）——
        void createPostModelRootItem();
        void createPostModelParentMenu();
        void createPostModelChildMenu();
        void createPostModelGroupParentMenu();
        void createPostModelGroupRootMenu();
        void createPostMaterialRootMenu();
        void createPostMaterialChildMenu();
        void createPostMaterialPartMenu();
        void createPostSetRootMenu();
        void createPostSetElementRootMenu();
        void createPostSetNodeRootMenu();
        void createPostSetFaceRootMenu();
        void createPostSetEdgeRootMenu();
        void createPostSetPartRootMenu();
        void createPostSetElementChildMenu();
        void createPostSetNodeChildMenu();
        void createPostSetFaceChildMenu();
        void createPostSetEdgeChildMenu();
        void createPostSetPartChildMenu();
        void createCloudRootMenu();   //云图根菜单（仅条件"更新"——新建云图入口已取消）
        void create3DResultParentMenu();
        //派生工况分支菜单（批次A0）：分支头"新建"三入口（复用 Ribbon 结果页总线信号）；
        //定义节点"编辑/删除"（重命名经面板名称行即时提交，树随 store 通知刷新）
        void createPostCaseDerivedRootMenu();
        void createPostCaseDerivedChildMenu();
        //测量记录节点菜单（V1.3 测量树联动，用户定案加全套）：编辑（回编面板，同单击）/
        //显示/隐藏/删除（批量作用于选中节点，经面板公开接口）——重命名走面板名称格
        void createMeasureChildMenu();
        //坐标系分支菜单（对齐前处理坐标系树口径）：根=创建坐标系（默认直角，走模型页
        //入口链）/删除所有坐标系；子节点=编辑（回编面板，同单击）/重命名（树内联，经
        //renameValidator 同步管理器）/复制/删除；全局坐标系节点不挂菜单
        void createCoordinateRootMenu();
        void createCoordinateChildMenu();
        //createAnimationChildMenu/Linear/ScalarOperation/Section/Isosurface/Vector 六个派生
        //子菜单随派生去节点化移除；createCurveChildMenu/ExportWord/Path 随 2D 去节点化移除

        //——辅助（自 mTreeView 移植）——
        void newPostModelBrowserTree();
        void rebuildPostQueryTree(int modelID);
        std::set<int> collectPostMaterialSectionPartIds();
        QStringList selectedPostSetNames(MxFunctions::BrowserTreeType childType);
        QStringList allPostSetNames(MxFunctions::BrowserTreeType childType);
        QHash<int, QList<int>> collectPostElementSetEleIds(const QStringList& setNames);
        void showPostElementSets(const QStringList& setNames);
        void hidePostElementSets(const QStringList& setNames);
        void isolatePostElementSets(const QStringList& setNames);
        void rebuildPostElementRender(const std::set<int>& reclassifyParts = {});

        //拆分选中的后处理子节点：部件按ID、刚性墙按名称（刚性墙节点用合成phyID，不能当部件ID用）
        void splitSelectedPostChildren(const QModelIndexList& idxs, std::set<int>& parts, std::set<QString>& wallNames);

        //——与 mTreeView 同名同签名的私有辅助（源索引语义）：搬迁代码无需改写——
        QModelIndexList getSelectIndex() const { return selectedSourceIndexes(); }
        QModelIndex toSourceIndex(const QModelIndex& idx) const { return idx; }
        MxFunctions::BrowserTreeType getTreeTypeByModelIndex(QModelIndex idx) { return mTreeItemUtil::treeTypeOf(_treeModel, idx); }
        int getPhyIdByModelIndex(QModelIndex idx) { return mTreeItemUtil::phyIdOf(_treeModel, idx); }
        QString getTextByModelIndex(QModelIndex idx) { return mTreeItemUtil::textOf(_treeModel, idx); }
        MxFunctions::BrowserTreeStatus getStatusByModelIndex(QModelIndex idx) { return mTreeItemUtil::statusOf(_treeModel, idx); }
        MxFunctions::BrowserTreeStatus getStatusByItem(mPostTreeItem* item);
        void setTreeItemStatus(mPostTreeItem* item, MxFunctions::BrowserTreeStatus stauts);
        void expandItem(mPostTreeItem* item);
        //节点/面/边集显隐辅助已随显隐功能取消（用户定案 2026-11-12）移除
        //（showPostNodeSets/hidePostNodeSets/updatePostNodeSetPointShow、
        //  show/hide/isolatePostFaceSets、show/hide/isolatePostEdgeSets、
        //  collectPostFace/EdgeSetMembers、applyPostFace/EdgeVisual、rebuildPostFaceEdgeRender）
        //部件集合显隐（GUI 面板创建；部件级显隐走 Show/HideOnePartOperate）
        void showPostPartSets(const QStringList& setNames);
        void hidePostPartSets(const QStringList& setNames);
        void isolatePostPartSets(const QStringList& setNames);
        void applyPostElementVisual(const QHash<int, QList<int>>& partEles, bool isShow);
        void selectedItem(mPostTreeItem* item);
        void removeTreeItem(mPostTreeItem* item);
        void removeProxyIndexs(QModelIndexList proxyIndexs);
        //mTreeView 在模型增删期间断开视口激活联动；后处理树无此连接，保留接口为空实现
        void disconnectSig() {}
        void connectSig() {}
        mPostTreeItem* getTreeItemByIndex(QModelIndex idx) { return mTreeItemUtil::itemOf(_treeModel, idx); }
        int getChildrenSize(QModelIndex idx) { return mTreeItemUtil::childrenSizeOf(_treeModel, idx); }
        MxFunctions::BrowserTreeType getTreeTypeByItem(mPostTreeItem* item) { return mTreeItemUtil::treeTypeOfItem(_treeModel, item); }
        int getPhyIdByItem(mPostTreeItem* item) { return mTreeItemUtil::phyIdOfItem(_treeModel, item); }
        QString getTextByItem(mPostTreeItem* item) { return mTreeItemUtil::textOfItem(_treeModel, item); }

        mPostTreeModel* _treeModel = nullptr;
        mPostTreeProxy* _proxy = nullptr;
        QMenu* m_Menu = nullptr;
        //store 定义集变更防抖（批次A0）：面板 commit 逐键触发通知，300ms 合并一次分支重建
        QTimer* _derivedRebuildTimer = nullptr;
        int _pendingDerivedKey = -1;
        //最近一次单击派发的节点类型（分区着色退出判定用；原为函数级 static，跨实例共享不合理）
        //分区显示着色开启旗标（去节点化后由功能区"分区"按钮直作用/自切换，
        //替代原 partitionChildItem 节点的 m_lastClickTreeType 退出判定）
        bool _partitionOverrideActive = false;
    };

    //////////////////////////////////////////////////////////////////////////
    // 后处理树窗口（左侧独立 Dock）：过滤/排序工具条 + 后处理树视图
    //////////////////////////////////////////////////////////////////////////
    class mPostBrowserWindow : public QWidget, public PostCommon::IPostResultService
    {
        Q_OBJECT
    public:
        static mPostBrowserWindow* getInstance();

        ~mPostBrowserWindow();

        mPostTreeView* getPostTree() const { return _treeView; }

        //会话保存采集用：指定后处理模型当初导入的结果文件全量清单。优先结果跳转
        //注册表记录的原始选择（旧格式 mxdb 导入须 mxdb0+帧文件完整组合，数据层
        //_dbFile 只记初始构型文件，单独回放会因缺帧文件被过滤器拒绝）；无登记时
        //回退 getAllDatabaseFile（native 单文件格式即为完整选择，回放可用）
        QStringList importedResultFilesOfModel(int modelDataID) const;

        /*
        * 树窗口工具条（过滤/排序）按当前 UiMetrics/UiPalette 重发（切档/切主题时调用，幂等）。
        * 构造期调用一次，ThemeManager::applyAll 遍历树时经 invokeMethod 再调。
        */
        Q_INVOKABLE void refreshToolBarTheme();

        //程序化打开后处理结果文件（创建漏斗的信号路径入口）：
        //新建后处理视口模型 → 建树根 → 导入结果；失败回滚删除本次创建的模型。
        //手动打开（文件对话框）与 solveFinishedSig（计算完成跳转）共用本函数，
        //经 createAndRegisterResult 落地并登记实例key
        void openPostResultFiles(const QStringList& names);

        //会话恢复的重导入登记：恢复链经 mPostInfoHub::import3DResult 直导，不经
        //createAndRegisterResult，须由此补登记完整文件清单——否则从恢复态再保存时
        //importedResultFilesOfModel 查无登记而回退 getAllDatabaseFile（旧格式仅记
        //mxdb0），二代会话缺帧文件，再恢复会被导入过滤器拒绝。同key重复登记幂等覆盖
        void registerSessionRestoredResult(int postModelID, const QStringList& filePaths);

        //移除指定后处理模型的结果跳转登记（模型删除路径调用，见 _resultRegistry）
        void unregisterResultJump(int postModelID);

        //---- IPostResultService（构造时经 PostCommon::postResultServiceRef() 自注册，前处理判空调用）----
        //创建后处理实例并返回实例唯一的不透明key（同一文件多次导入各自不同key）；
        //失败返回空串。options.activateNow=false 时创建后切回原视口。
        //契约细节见 mPostResultServiceProvider.h
        QString createPostResult(const QStringList& resultFilePaths,
            const PostCommon::PostResultCreateOptions& options = {},
            QString* syncErrorDetail = nullptr) override;

        //按key幂等切换到对应模型树/视口；key失效（模型被删/漂移）返回 InvalidKey 报错不重建
        PostCommon::PostResultError showPostResult(const QString& resultKey) override;

    private:
        explicit mPostBrowserWindow(QWidget* parent = nullptr);

        //创建漏斗（手动打开/信号/接口三路共用）：新建模型+建树根+导入+登记实例key，
        //失败回滚返回空串。reuseKey非空用于mtime"删旧重建该key实例"（key保留重登记）；
        //modelFilePath非空时作为op2伴随网格透传给原生读取（显式优先于多选bdf与自动探测）；
        //syncErrorDetail非空时在同步失败出口填入中文原因（异步解析失败无法经此带出）
        QString createAndRegisterResult(const QStringList& names, bool activateNow,
            const QString& reuseKey = QString(),
            const QString& modelFilePath = QString(),
            QString* syncErrorDetail = nullptr);

        //按key切换核心实现（查表→模型存在性→mtime重建→激活四步），接口与信号路径共用
        PostCommon::PostResultError showPostResultByKey(const QString& key);

    private slots:
        //打开后处理结果文件：云图结果（自mMenuWidget::slot_openPostFile迁入，入口经clickOpenPostCloudFileSig）
        void slot_openPostFile();

        //打开后处理结果文件：曲线结果（自mMenuWidget::slot_openPostNVHFile迁入，入口经clickOpenPostNVHFileSig）
        void slot_openPostNVHFile();

        //幂等跳转（requestShowPostResultSig，服务于无key的求解完成自动跳转）：
        //命中注册表激活已有云图视口；未命中/失效/mtime更新回退为新建+导入并登记新key
        //（与接口 showPostResult 的"InvalidKey报错不重建"语义不同，见服务头注释）
        void slot_showPostResult(const QString& resultKey, const QStringList& resultFilePaths);

        //异步导入失败回滚（postResultImportFailedSig，native/mxdb新旧三链共用）：
        //解析失败发生在 import3DResult 返回之后，漏斗同步段回滚够不到——有注册表
        //登记（=创建漏斗/服务流程新建的实例）才回收整个空模型树并作废key（对每个
        //key先发 postResultCreateFailedSig 再发 postResultRemovedSig）；无登记
        //（=按工况更新/会话恢复等向既有模型补导）维持只删库现状不动用户模型
        void slot_postImportFailedRollback(int postModelID);

    private:
        static mPostBrowserWindow* _instance;
        mPostTreeView* _treeView = nullptr;

        //结果跳转注册表：键=后处理铸造的实例唯一不透明key（进程内有效，不持久化；
        //同一文件多次导入各自一条记录）。记录"创建实例 → 后处理模型/入口视口/文件列表"，
        //条目作废（模型删除/漂移）经回调发总线 postResultRemovedSig，见 mPostResultRegistry.h
        mPostResultRegistry _resultRegistry;

        //过滤/排序工具条（与前处理树窗口一致）
        QComboBox* _nameComboBox = nullptr;
        QStackedWidget* _stackedWidget = nullptr;
        QLineEdit* _findLineEdit = nullptr;
        QComboBox* _statusComboBox = nullptr;
        QComboBox* _sortTypeComboBox = nullptr;
        QToolBar* _sortTool = nullptr;      // 排序工具条（图标/条高随档位重发）
        QLabel* _filterLabel = nullptr;
        QLabel* _sortLabel = nullptr;
    };
}
