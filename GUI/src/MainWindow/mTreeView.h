#pragma once
#pragma execution_character_set("utf-8")

#include <QMouseEvent>
#include <QTreeView>
#include <QObject>
#include <QPainter>
#include <set>
#include <vector>
#include <functional>
#include <QMenu>
#include <Qt>
#include <QMdiSubWindow>
#include "mModelEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mMotionSensorType.h"

namespace MechPreData
{
    class mTreeModel;
    class mTreeProxyModel;
    class mViewportPrivate;
    struct mTreeItem;
};
namespace MechMotionData
{
    enum class SpringType;
}


namespace MechModelData
{
    class mModelData;
};


namespace MechMainWindow
{
    class mTreeView : public QTreeView {
        Q_OBJECT
    public:
        explicit mTreeView(QWidget* parent = nullptr);

        void connectSig();
        void disconnectSig();
        /*
        * 获取模型树名称
        */
        QString getModelBrowserTreeName();

        /*
        * 增加模型树
        */
        void addModelBrowserTree();

        /*
        * 移除指定模型的树根节点（删除模型时同步调用，清理其残留树节点）
        */
        void removeModelRootItem(int modelDataID);

        /*
        * 清空当前模型：数据/视口/模型树全部还原为按原名称新建的空模型状态
        *（打开工程文件前、右键"清空"共用；不含确认弹窗，由调用方决定是否询问）
        */
        void clearCurrentModelData();


        /*
        * 名称过滤
        */
        void nameFilterTree(const QString& filterStr);

        /*
        * 选中载荷管理器树下指定载荷节点（载荷管理器面板点击编辑时调用，不存在则不操作）
        */
        void selectLoadManagerLoadItem(int loadID);

        /*
        * 选中指定工况下载荷父节点下的指定载荷节点（载荷引用面板点击编辑时调用，不存在则不操作）
        */
        void selectCaseLoadItem(int caseID, int loadID);

        /*
        * 判断指定工况类型是否支持某载荷/边界类型
        */
        static bool isLoadBoundarySupportedByCase(MxFunctions::CaseType caseType, MxFunctions::LoadBoundaryType type);

        /*
        * 判断指定工况类型是否支持某预定义场类型
        */
        static bool isPredefinedFieldSupportedByCase(MxFunctions::CaseType caseType, MxFunctions::PredefinedFieldType type);

        /*
        * 在工况下的载荷父节点挂载/移除载荷子节点（载荷引用面板勾选/取消勾选时调用）
        */
        void addCaseLoadChildItem(int caseID, int loadID);
        void removeCaseLoadChildItem(int caseID, int loadID);

        /*
        * 选中边界管理器树下指定边界节点（边界管理器面板点击编辑时调用，不存在则不操作）
        */
        void selectBoundaryManagerBoundaryItem(int boundaryID);

        /*
        * 选中指定工况下边界父节点下的指定边界节点（边界引用面板点击编辑时调用，不存在则不操作）
        */
        void selectCaseBoundaryItem(int caseID, int boundaryID);

        /*
        * 在工况下的边界父节点挂载/移除边界子节点（边界引用面板勾选/取消勾选时调用）
        */
        void addCaseBoundaryChildItem(int caseID, int boundaryID);
        void removeCaseBoundaryChildItem(int caseID, int boundaryID);

        /*
        * 选中预定义场管理器树下指定预定义场节点（预定义场管理器面板点击编辑时调用，不存在则不操作）
        */
        void selectPredefinedFieldManagerItem(int fieldID);

        /*
        * 选中指定工况下预定义场父节点下的指定预定义场节点（预定义场引用面板点击编辑时调用，不存在则不操作）
        */
        void selectCasePredefinedFieldItem(int caseID, int fieldID);

        /*
        * 在工况下的预定义场父节点挂载/移除预定义场子节点（预定义场引用面板勾选/取消勾选时调用）
        */
        void addCasePredefinedFieldChildItem(int caseID, int fieldID);
        void removeCasePredefinedFieldChildItem(int caseID, int fieldID);

        /*
        * 状态过滤
        */
        void statusFilterTree(MxFunctions::BrowserTreeStatus filterStatus);

        /*
        * 排序
        */
        void sortTree(int sortWay, Qt::SortOrder sortType);

        /*
        * 获取树数据模型（后处理树框架窗口共享同一份数据）
        */
        MechPreData::mTreeModel* getTreeModel() const { return _treeModel; }

        /*
        * 相同网格类型的交集，更新状态
        */
        void slot_updateMeshStatus();

        /*
        * 删除几何，更新网格子节点列表
        */
        void slot_updateMeshList(std::map<int, std::set<int>> map);

        /*
         * 载荷边界管理器，表格点击勾选框时树节点相应删除添加槽,way=0添加 way=1删除
         */
        void slot_updateLoadBoundaryTreeView(int caseId, int loadBoundaryId, int way);

        /*
         * 预定义场管理器，表格点击勾选框时树节点相应删除添加槽,way=0添加 way=1删除
         */
        void slot_updatePredefinedFieldTreeView(int caseId, int fieldID, int way);

        /*
         * 相互作用管理器，表格点击勾选框时树节点相应删除添加槽,way=0添加 way=1删除
         */
        void slot_updateContactTreeView(int caseId, int fieldID, int way);

        /*
         * 模型修复管理器，表格点击勾选框时树节点相应删除添加槽,way=0添加 way=1删除
         */
        void slot_updateModelFixTreeView(int caseId, int modelFixID, int way);

        /*
        * 设置是否草图模式
        */
        void setSketch(bool b) { isSketch = b; }

        /*
       * 退出草图模式时调用，删除空的草图组合体
       */
        void deleteSketchCompound();

        /*
        * 获取父节点key
        */
        mItemKey getParentKey(mItemKey childKey);

        /*
        * 更新指定部件子节点的几何/网格显隐图标，仅重绘对应的节点行（不做全量刷新）。
        * 上层在改变部件的几何/网格显隐后调用；modelID为部件所属模型，partIDs为部件ID集合
        */
        void updatePartDisplayIcon(int modelID, const std::set<int>& partIDs);

    private:
        QRect branchHitRect(const QModelIndex& index) const;

    protected:
        /*
        * 绘制分支区域：保留原生展开/折叠箭头，并叠加父节点连接子节点的层级虚线导引线
        */
        void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

        /*
         * 从当前选中节点推断工况类型（分析工况节点或其子节点）
         */
        MxFunctions::CaseType getCurrentCaseTypeFromSelection();
        /*
         * 统一创建分析工况下的一级菜单节点，保证各节点按固定顺序创建
         */
        void createCaseFirstLevelItems(int modelID, MxFunctions::CaseType caseType, int caseID, MechPreData::mTreeItem* caseParentItem, MxFunctions::BrowserTreeStatus resultStatus = MxFunctions::Tree_Failure);
        /*
         * 根据工况类型获取模型树节点图标（与分析菜单栏图标保持一致）
         */
        QString getCaseIconPath(MxFunctions::CaseType caseType);
        /*
         * 根据分析工况类型删除其下不应显示的父节点（载荷、边界、预定义场、相互作用）
         */
        void removeCaseParentItemsByCaseType(MxFunctions::CaseType caseType, MechPreData::mTreeItem* caseParentItem);
        /*
         * 获取插入点位置
         */
        MechPreData::mTreeItem* getInsertLocation(MxFunctions::BrowserTreeType InsertedItem);

        /*
        *初始化模型树
        */
        void initBrowserTree();

        /*
        * 初始化连接
        */
        void initConnect();

        /*
        * 高亮部件
        */
        void hightPart();

        /*
        * 符号显示
        */
        void setSymbolShow();

        /*
        * 符号删除
        */
        void deleteSymbol();

        /*
        * 全局坐标系创建
        */
        void createGlobalCoordinate();

        /*
        * 选中改变
        */
        void slot_itemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

        /*
        * 判断选中与模型是否一致，不一致更新模型
        */
        void updateModelView(QModelIndex proxyIndex);

        /*
        * 点击选中
        */
        void slot_singleClickItem();

        /*
        * 更新选中
        */
        void updateItemSelected();

        /*
        * 按当前激活视口刷新前处理树窗口：
        * 模型树仅显示当前模型的一棵，并通知布局管理器切换 视图树/前处理树/后处理树/NVH 窗口显隐
        */
        void updateTreeSectionsForViewport();

        /*
        * 新建模型树
        */
        void newModelBrowserTree();

        /*
        * 复制模型树
        */
        void copyModelBrowserTree();

        /*  
        * 注册命令行
        */
        void registCommond();

        /*********************************************************************************右键菜单*********************************************************************************/
        /*
        * 模型根节点右键
        */
        void createModelRootItem();

        /*
        * 几何结构右键
        */
        void createGeoItemMenu();

        /*
        * 几何结构子节点右键
        */
        void createGeoChildMenu();

        /*
        * 装配分组右键
        */
        void createAssemblyParentMenu();

        /*
        * 装配节点右键
        */
        void createAssemblyChildMenu();


       /*
        * 注释节点右键
        */

        /*
        * 集合父节点右键
        */
        void createSetParentMenu();

        /*
        * 集合子节点右键
        */
        void createSetChildMenu();

        /*
        * 坐标系父节点右键
        */
        void createCoordinateParentMenu();

        /*
        * 坐标系子节点右键
        */
        void createCoordinateChildMenu();



        /*
       * 结构优化父节点右键
       */
        void createStructOptionParentMenu();

        /*
        * 结构优化设计域父节点右键
        */
        void createOptimizationDesignParentMenu();

        /*
        * 结构优化设计域子节点右键
        */
        void createOptimizationDesignChildMenu();

        /*
         * 结构优化设计域父节点右键
         */
        void createOptimizationConstraintParentMenu();

        /*
         * 结构优化设计域子节点右键
         */
        void createOptimizationConstraintChildMenu();

        /*
         * 流道优化父节点右键
         */
        void createRunnerOptimizationParentMenu();

        /*
         * 流道优化设计域父节点右键
         */
        void createRunnerOptimizationDesignParentMenu();

        /*
         * 流道优化设计域子节点右键
         */
        void createRunnerOptimizationDesignChildMenu();

        /*
         * 流道优化约束父节点右键
         */
        void createRunnerOptimizationConstraintParentMenu();

        /*
         * 流道优化约束子节点右键
         */
        void createRunnerOptimizationConstraintChildMenu();

        /*
         * 属性父节点右键
         */
        void createPropertyParentMenu();

        void menuCreateProperty(QAction* menuTitle, bool addDelete);

        /*
        * 属性子节点右键
        */
        void createPropertyChildMenu();

        /*
        * 非结构质量父节点右键
        */
        void createNonStructQualityParentMenu();
        /*
        * 非结构质量子节点右键
        */
        void createNonStructQualityChildMenu();

        /*
        * 场父节点右键（离散场/分析场统一入口）
        */
        void createFieldParentMenu();
        /*
        * 场子节点右键
        */
        void createFieldChildMenu();

        /*
        * 函数父节点右键
        */
        void createFunctionParentMenu();

        /*
        * 函数子节点右键
        */
        void createFunctionChildMenu();


        /*
        * 工况父节点右键
        */
        void createCaseParentMenu();

        /*
         * 载荷父节点右键
         */
        void createLoadParentMenu(MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);

        /*
         * 转子分析父节点右键
         */
        void createRotateParentMenu();

        /*
         * 转子分析列表
         */
        void RotateParentMenu(QAction* menuTitle, bool addDelete);

        /*
         * 载荷列表
         */
        void LoadParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);

        /*
        * 载荷子节点右键
        */
        void createLoadChildMenu();

        /*
        * 转子分析子节点右键
        */
        void createRotateChildMenu();


        /*
         * 集合条件列表
         */
        void add2SetParentMenu(QAction* menuTitle, bool addDelete);

        /*
         * 边界条件列表
         */
        void BoundaryParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);


        /*
         * 边界父节点右键
         */
        void createBoundaryParentMenu(MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);

        /*
         * 预定义场父节点右键
         */
        void createPredefinedFieldParentMenu(MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);

        /*
         * 预定义场列表
         */
        void PredefinedFieldParentMenu(QAction* menuTitle, bool addDelete, MxFunctions::CaseType caseType = (MxFunctions::CaseType)0);

        /*
         * 预定义场子节点右键
         */
        void createPredefinedFieldChildMenu();

        /*
         * 边界子节点右键
         */
        void createBoundaryChildMenu();

        /*
         * 工况下非结构质量父节点右键
         */
        void createCaseNonStructuralQualityParentMenu();

        /*
         * 工况下非结构质量子节点右键
         */
        void createCaseNonStructuralQualityChildMenu();

        /*
         * 分析设置子节点右键
         */
        void createCaseSettingChildMenu();

        /*
        * 结果父节点右键
        */

        /*
        * 三维结果父节点右键
        */

        /*
        * 动画子节点右键
        */

        /*
        * 线性叠加子节点右键
        */

        /*
        * 标量运算子节点右键
        */

        /*
        * 曲线子节点右键
        */

        /*
        * 导出Word子节点右键
        */

        /*
        * 路径子节点节点右键
        */

        /*
       * 切面图子节点节点右键
       */

        /*
        * 等值面图子节点节点右键
        */

        /*
        * 矢量图子节点节点右键
        */

       /*
         * 注释节点右键
         */
        void createNoteParentMenu();

        /*
        * 材料父节点右键
        */
        void createMaterialsParentMenu();

        /*
        * 材料子节点右键
        */
        void createMaterialsChildMenu();

        /*
        * 相互作用父节点右键
        */
        void createInteractionsParentMenu();

        /*
        * 连接父节点右键
        */
        void createConnectorsParentMenu();

        /*
        * 连接副父节点右键
        */
        void createDeputiesParentMenu();

        /*
        * 连接列表
        */
        void ConnectorsParentMenu(QAction* menuTitle, bool addDelete);

        /*
        * 连接副列表
        */
        void DeputiesParentMenu(QAction* menuTitle, bool addDelete);

        /*
        * 连接子节点右键
        */
        void createConnectorsChildMenu();

        /*
        * 连接副子节点右键
        */
        void createDeputiesChildMenu();

        /*
        * 接触父节点右键
        */
        void createContactParentMenu();

        /*
         * 接触子节点右键
         */
        void createContactChildMenu();

        /*
        * 周期性边界右键父节点
        */
        void createPeriodicBoundaryParentMenu();

        /*
        * 周期性边界右键子节点
        */
        void createPeriodicBoundaryChildMenu();

        /*
        * 模型修改父节点右键
        */
        void createModelFixParentMenu();

        /*
         * 模型修改子节点右键
         */
        void createModelFixChildMenu();

        /*
         * 工况树模型修改父节点右键
         */
        void createCaseModelFixParentMenu();

        /*
         * 工况树模型修改子节点右键
         */
        void createCaseModelFixChildMenu();

        /*
         * 工况下相互作用父节点右键
         */
        void createCaseInteractionParentMenu();

        /*
         * 工况下相互作用子节点右键
         */
        void createCaseInteractionChildMenu();

        /*
         * 网格父节点右键
        */
        void createMeshPanelParentMenu();

        /*
        * 网格子节点右键
        */
        void createMeshPanelChildMenu();
        void createLocalSizePanelChildMenu();
        void createCustomEncryption();
        void createAutoAdapt();
        /*
        * 网格操作父节点右键
        */
        void createMeshOptPanelParentMenu();
        /*
        * 网格操作子节点右键
        */
        void createMeshOptPanelChildMenu();
        /*
        * 全局尺寸子节点右键
        */
        void createGlobalSimChildMenu();

        /*
        * 截面形状列表
        */
        void SectionShapeParent(QAction* menuTitle, bool addDelete);

        /*
        * 截面形状父节点右键
        */
        void createSectionShapeParentMenu();

        /*
        * 截面形状子节点右键
        */
        void createSectionShapeChildMenu();

        /*
        * 频率点采样方式右键菜单
        */
        void createFreqSamplPointParentMenu();

        /*
        * 频率点采样方式子节点右键菜单
        */
        void createFreqSamplPointChildMenu();

        /*
        * 层合板管理器右键菜单
        */
        void createLaminateManagerMenu();

        /*
        * 列表
        */
        void LaminateManagerMenu(QAction* menuTitle, bool addDelete);

        /*
        * 单层板父节点右键菜单
        */
        void createSglLayerPlateParentMenu();

        /*
        * 层合板父节点右键菜单
        */
        void createLamPlateParentMenu();

        /*
        * 层合结构子节点，三种子节点统一一个右键菜单
        */
        void createLaminateChildMenu();


        /*
        *  全局参数设置右键菜单
        */
        void createGlobalParameterMenu();
        void GlobalParameterMenu(QAction* menuTitle, bool addDelete = true);
        void menuCreateDiagOpt(QAction* menuTitle);
        void menuCreateSEParamSet(QAction* menuTitle);
        /*
        * 求解控制父节点右键菜单
        */
        void createSolveControlMenu();

        /*
        * 全局输出控制父节点右键菜单
        */
        void createGlobalOutputControlMenu();

        /*
        * 超单元参数父节点右键菜单
        */
        void createGenericParamParentMenu();

        /*
        * 超单元参数子节点右键菜单
        */
        void createGenericParamChildMenu();

        /*
        * 超单元调用父节点右键菜单
        */
        void createGenericAssignMenu();

        /*
        * 声固耦合对搜索父节点右键菜单
        */
        void createASCouplingSearchMenu();

        /*
        * 超单元矩阵父节点右键菜单
        */
        void  createGenericMatrixMenu();

        /*
        * 诊断优化父节点右键菜单
        */
        void  createDiagOptMenu();

        /*
         * 贡献量父节点右键菜单
         */
        void createContriParentMenu();
        void ContriParentMenu(QAction* menuTitle, bool addDelete = true);

        /*
         * 贡献量子节点右键菜单
         */
        void createContriChildMenu();

        /*
         * 等效辐射功率父节点右键菜单
         */
        void createERPParentMenu();

        /*
         * 峰值筛选父节点右键菜单
         */
        void createPeakFilterParentMenu();

        /*
         * 峰值筛选子节点右键菜单
         */
        void createPeakFilterChildMenu();

        /*
        * 分析工况下峰值筛选右键
        */
        void createCasePeakFilterMenu();

        /*******************************多体动力学右键**************************************/
        void createMotionRootMenu();
        void createMotionBodyParentMenu();
        void createMotionBodyChildMenu();
        void createMotionJointParentMenu();
        void createMotionJointChildMenu();
        void createMotionCouplingParentMenu();
        void createMotionCouplingChildMenu();
        void createMotionDriveParentMenu();
        void createMotionDriveChildMenu();
        void createMotionMarkerParentMenu();
        void createMotionMarkerChildMenu();
        void createMotionVectorForceParentMenu();
        void createMotionVectorForceChildMenu();
        void createMotionContactParentMenu();
        void createMotionContactChildMenu();
        void createMotionSpringParentMenu();
        void createMotionSpringChildMenu();
        void createMotionScenarioParentMenu();
        void createMotionControlInputParentMenu();
        void createMotionControlInputChildMenu();
        void createMotionElectroImportParentMenu();
        void createMotionElectroImportChildMenu();
        void createMotionElectroExportParentMenu();
        void createMotionElectroExportChildMenu();
        void createMotionHydraulicParentMenu();
        void createMotionHydraulicChildMenu();
        void createMotionRLParentMenu();
        void createMotionRLChildMenu();
        void createMotionSensorParentMenu();
        void createMotionSensorChildMenu();
        void createMotionControllerParentMenu();
        void createMotionControllerChildMenu();
        void createMotionAutoFlexParentMenu();
        void createMotionAutoFlexChildMenu();

        /*
        * 超单元部件父节点右键菜单
        */
        void  createGenericPartMenu();

    private slots:
        /*
        * 切换视口
        */
        void slot_subWindowActivated(int viewportID);

        /*
        * 视口/模型即将切换（切换前、旧模型仍为当前模型时触发）：
        * 跨模型切换按退出面板语义隐藏旧视口的符号显示
        */
        void slot_viewportModelAboutToChange(int newViewportID);

        /*
        * 右键
        */
        void slot_rightClickItem();

        /*
         * 重命名
         */
        void slot_menuChildRename();

        /*
        * 后处理部件隐藏当前
        */

        /*
        * 后处理部件隐藏其他
        */

        /*
        * 后处理部件隐藏全部
        */

        /*
         * 批量重命名
         */
        void slot_menuBatchRename();


        //隐藏包围盒
        void slot_menuBoxHide();

        //预览包围盒
        void slot_menuBoxPreview();

        //显示包围盒
        void slot_menuBoxShow();

        //局部尺寸预览
        void slot_menuLocalSizePreview();

        //删除包围盒
        void slot_menuBoxDelete();

        /*
        * 导入文件，更新模型树
        */
        void slot_updateModelTree();

        /*
        * 更新后处理模型树
        */
        //【集成·死声明清理】void slot_updatePostModelTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理模型创建分组
        */
        //【集成·死声明清理】void slot_createPostModelGroup();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理模型分组显隐
        */
        //【集成·死声明清理】void slot_showPostModelGroup();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019
        //【集成·死声明清理】void slot_hidePostModelGroup();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理模型分组删除
        */
        //【集成·死声明清理】void slot_deletePostModelGroup();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件显示当前
        */
        //【集成·死声明清理】void slot_menuShowCurrentPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件隐藏当前
        */
        //【集成·死声明清理】void slot_menuHideCurrentPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件隐藏其他
        */
        //【集成·死声明清理】void slot_menuHideOtherPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件显示全部
        */
        //【集成·死声明清理】void slot_menuShowAllPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件隐藏全部
        */
        //【集成·死声明清理】void slot_menuHideAllPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理部件反向显示
        */
        //【集成·死声明清理】void slot_menuReverseShowPostPart();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理材料/截面名及其下部件名 显示当前
        */
        //【集成·死声明清理】void slot_showPostMaterialSectionParts();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理材料/截面名及其下部件名 隐藏当前
        */
        //【集成·死声明清理】void slot_hidePostMaterialSectionParts();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理材料/截面名及其下部件名 隐藏其他
        */
        //【集成·死声明清理】void slot_hideOtherPostMaterialSectionParts();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合名 显示当前
        */
        //【集成·死声明清理】void slot_showPostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合名 隐藏当前
        */
        //【集成·死声明清理】void slot_hidePostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合名 隐藏其他
        */
        //【集成·死声明清理】void slot_hideOtherPostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理节点集合名 显示当前（点显示集合节点）
        */
        //【集成·死声明清理】void slot_showPostNodeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理节点集合名 隐藏当前（取消集合节点点显示）
        */
        //【集成·死声明清理】void slot_hidePostNodeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理节点集合名 隐藏其他（仅保留当前集合节点点显示）
        */
        //【集成·死声明清理】void slot_hideOtherPostNodeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合名 显示当前
        */
        //【集成·死声明清理】void slot_showPostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合名 隐藏当前
        */
        //【集成·死声明清理】void slot_hidePostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合名 隐藏其他
        */
        //【集成·死声明清理】void slot_hideOtherPostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合名 显示当前
        */
        //【集成·死声明清理】void slot_showPostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合名 隐藏当前
        */
        //【集成·死声明清理】void slot_hidePostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合名 隐藏其他
        */
        //【集成·死声明清理】void slot_hideOtherPostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合根 显示全部集合
        */
        //【集成·死声明清理】void slot_showAllPostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合根 隐藏全部集合
        */
        //【集成·死声明清理】void slot_hideAllPostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理单元集合根 仅显示集合单元
        */
        //【集成·死声明清理】void slot_showOnlyPostElementSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理节点集合根 显示全部集合
        */
        //【集成·死声明清理】void slot_showAllPostNodeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理节点集合根 隐藏全部集合
        */
        //【集成·死声明清理】void slot_hideAllPostNodeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合根 显示全部集合
        */
        //【集成·死声明清理】void slot_showAllPostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合根 隐藏全部集合
        */
        //【集成·死声明清理】void slot_hideAllPostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理面集合根 仅显示集合面
        */
        //【集成·死声明清理】void slot_showOnlyPostFaceSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合根 显示全部集合
        */
        //【集成·死声明清理】void slot_showAllPostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合根 隐藏全部集合
        */
        //【集成·死声明清理】void slot_hideAllPostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 后处理边集合根 仅显示集合边
        */
        //【集成·死声明清理】void slot_showOnlyPostEdgeSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 模型树状态更新
        */
        void slot_updateBrowserTreeStatus(MxFunctions::PhysicalFieldType phyType, QHash<int, MxFunctions::BrowserTreeStatus> phyData);

        /*
        * 后处理模型树状态更新
        */

        /*
        * 切换云图结果最大帧
        */

        /***********************************************************************************视图*******************************************************************************************/

        /*
        * 视图事件过滤器
        */
        void slot_signal_osgWidgetEvent(QEvent* event);

        /*
        * 视图事件-鼠标右键定时器
        */
        void longPressDetected();

        /*
        * 视图事件-鼠标右键按下
        */
        void osg_mousePressEvent(QMouseEvent* event);

        /*
        * 视图事件-鼠标右键释放
        */
        void osg_mouseReleaseEvent(QMouseEvent* event);

        /**********************************************************************************几何结构***********************************************************************************************/
        /*
        * 生成网格
        */
        void slot_menuCreateMesh();

        /*
        * 删除几何
        */
        void slot_menuDeleteGeo();

        /*
        * 删除其他几何
        */
        void slot_menuDeleteOtherGeo();

        /*
        * 删除网格
        */
        void slot_menuDeleteMesh();

        /*
        * 显示当前
        */
        void slot_menuShowCurrentPart();

        /*
        * 隐藏当前
        */
        void slot_menuHideCurrentPart();

        /*
        * 隐藏其他
        */
        void slot_menuHideOtherPart();

        /*
        * 显示全部
        */
        void slot_menuShowAllPart();

        /*
        * 隐藏全部
        */
        void slot_menuHideAllPart();

        /*
        * 显示模型树中部件
        */
        void slot_menuShowTreePart();

        /*
        * 反向显示
        */
        void slot_menuReverseShow();

        /*
        * 模型树部件 - 反向显示
        */
        void slot_menuTreePartReverseShow();

        /*
        * 基于可见部件过滤模型树
        */
        void slot_filterTreeByPartMask();

        /*
        * 导入几何文件
        */
        void slot_importGeoFile();

        /*
        * 导出几何文件 - 全部
        */
        void slot_exportGeoFileAll();

        /*
        * 导出几何文件 - 当前显示
        */
        void slot_exportGeoFileVisible();

        /*
        * 导出几何文件 - 选中部件
        */
        void slot_exportGeoFileSelected();

        /*
        * 映射到模型树部件
        */
        void slot_mapRelationTree();

        /*
        * 更新几何结构树
        */
        void slot_updatePartTree(bool isCreate, std::set<int> partID);
        void slot_updatePartTree_2(bool isCreate, std::set<int> partID);

        /*
        * 删除几何结构树
        */
        void slot_deletePartTree(std::set<int> partID);
        void slot_deletePartTree_2(std::set<int> partID);

        /************************************************************************************装配************************************************************************************************/

        /*
        * 更新装配树（装配分组整体重建填充：创建/重命名/删除/关系变化后触发）
        */
        void slot_updateAssemblyTree();

        /*
        * 高亮几何创建的实体
        */
        void slot_appendOneGeoModelRenderFinished();

        /*
        * 删除部件
        */
        void slot_deleteGeoTree(std::set<int> partIds);

        //新增导入成功
        void slot_importSuccessful();

        /***************************************************************************************集合***********************************************************************************/
        /*
        * 创建集合
        */
        void slot_menuSetNew();

        /*
        * 删除所有集合
        */
        void slot_menuSetDelete();

        /*
        * 删除某一类型的所有集合
        */
        void slot_menuSetTypeDelete(MxFunctions::SetType setType);

        /*
        * 集合管理器面板勾选删除集合信号的处理槽：同步删除数据与树节点
        */
        void slot_deleteSets(const QVector<int>& setIDs);

        /*
        * 载荷管理器面板勾选删除载荷信号的处理槽：同步删除数据与树节点
        */
        void slot_deleteLoads(const QVector<int>& loadIDs);

        /*
        * 边界管理器面板勾选删除边界信号的处理槽：同步删除数据与树节点
        */
        void slot_deleteBoundarys(const QVector<int>& boundaryIDs);

        /*
        * 预定义场管理器面板勾选删除预定义场信号的处理槽：同步删除数据与树节点
        */
        void slot_deletePredefinedFields(const QVector<int>& fieldIDs);

        /*
        * 删除一批集合并同步树（数据、节点、空类型分组）；
        * reselect=false时不改动树选中项（面板删除路径，避免选中变化触发面板切换）
        */
        void deleteSetsFromModel(const std::vector<int>& phyIDs, bool reselect = true);

        /*
        * 删除集合
        */
        void slot_menuSetChildDel();

        /*
        * 复制集合
        */
        void slot_menuCopySet();

        /*
        * 更新集合树
        */
        void slot_updateSetTree(bool isCreate, int setID);

        /*
        * 更新前处理集合关联数据
        */
        void updatePreSetData(int setID);

        /*
        * 集合类型图片
        */
        QString slot_setTypePic(MxFunctions::SetType type);

        /*
        * 集合类型中文名
        */
        QString getSetTypeNameByType(MxFunctions::SetType type);

        /*
        * 获取"集合"节点下对应类型的分组节点，不存在则按二级菜单类型顺序创建
        */
        MechPreData::mTreeItem* ensureSetTypeGroupItem(MechPreData::mTreeItem* setRootItem, MxFunctions::SetType setType);

        /*
        * 删除空的集合类型分组节点
        */
        void removeEmptySetTypeGroupItem(MechPreData::mTreeItem* groupItem);

        /***************************************************************************************属性***********************************************************************************/

        /*
        * 删除所有属性
        */
        void slot_menuPropertyDelete();

        /*
        * 删除属性
        */
        void slot_menuPropertyChildDel();

        /*
        * 复制属性
        */
        void slot_menuCopyProperty();

        /*
        * 更新属性
        */
        void slot_updatePropertyTree(bool isCreate, int propertyID);

        /******************非结构质量**************/
        /*
        * 更新非结构质量属性
        */
        void slot_updateNonStructuralQualityTree(bool isCreate, int propertyID);

        /********************************场（离散场/分析场）***************************************/
        void slot_updateFieldTree(bool isCreate, int fieldId);

        /*
        * 删除场属性
        */
        void slot_menuFieldDel();

        /*
        * 删除全部场属性（按子类型）
        */
        void slot_menuFieldDelete(MxFunctions::FieldType fieldType);

        /*
         * 复制场属性
         */
        void slot_menuCopyField();

        /*
        * 删除非结构质量属性
        */
        void slot_menuNonStructuralQualityDel();

        /*
        * 删除全部非结构质量属性
        */
        void slot_menuNonStructuralQualityDelete();

        /*
         * 复制非结构质量属性
         */
        void slot_menuCopyNonStructuralQuality();

        /*
         * 删除工况下全部非结构质量（仅移除映射）
         */
        void slot_menuCaseNonStructuralQualityDelete();

        /*
         * 删除工况下非结构质量子节点（仅移除映射）
         */
        void slot_menuCaseNonStructuralQualityChildDel();

        /*
         * 复制工况下非结构质量子节点
         */
        void slot_menuCopyCaseNonStructuralQuality();

        /***************************************************************************************函数（曲线）***********************************************************************************/
        /*
        * 创建函数
        */
        void slot_menuFunctionNew();

        /*
        * 删除所有函数
        */
        void slot_menuFunctionDelete();

        /*
        * 删除函数
        */
        void slot_menuFunctionChildDel();

        /*
        * 复制函数
        */
        void slot_menuCopyFunction();

        /*
        * 更新函数树
        */
        void slot_updateFunctionTree(bool isCreate, int functionID);

        /***************************************************************************************坐标系***********************************************************************************/
        /*
        * 创建坐标系
        */
        void slot_menuCoordinateNew();


       

        /*
        * 删除所有坐标系
        */
        void slot_menuCoordinateDelete();

        /*
        * 删除坐标系
        */
        void slot_menuCoordinateChildDel();

        /*
        * 复制坐标系
        */
        void slot_menuCopyCoordinate();

        /*
        * 更新坐标系树
        */
        void slot_updateCoordinateTree(bool isCreate, int coordinateID);

        /************************************************************************************网格************************************************************************************************/

        /*
        * 更新网格(全局尺寸)树
        */
        void slot_updateGlobalDimTree(bool isCreate);
        /*
        * 更新网格(全局尺寸)树
        */
        void slot_updateLocalSizeTree(bool isCreate, int meshpanelID);

        /*
        * 更新网格(体、面、线网格)树
        */
        void slot_updateMeshTree(bool isCreate, int meshPanelID);

        /*
        * 更新网格操作树
        */
        void slot_updateMeshOptTree(bool isCreate, int meshPanelID);
        /*
        * 更新网格(体、面、线网格)树
        */
        void slot_updateMeshOperationTree(bool isCreate, int meshPanelID);
        /*
        * 删除网格子节点
        */
        void slot_menuMeshPanelChildDel();
        /*
        * 删除自定义加密子节点
        */
        void slot_menuCustomPanelChildDel();
        /*
        * 删除网格c操作子节点
        */
        void slot_menuMeshOptPanelChildDel();

        /*
        * 删除所有网格
        */
        void slot_menuMeshPanelDelete();
        /*
        * 删除所有网格操作
        */
        void slot_menuMeshOptPanelDelete();

        /*
        * 生成网格
        */
        void slot_createMeshByPartId(std::vector<int> partIds);
        /*
        * 生成网格
        */
        void slot_createMesh();
        /*
        * 一键生成
        */
        void slot_createMeshAuto();
        /*
        * 一键删除
        */
        void slot_deleteMeshAuto();
        /*
        * 根据网格设置生成
        */
        void slot_createMeshBySetting();
        /*
        * 生成网格全局尺寸
        */
        void slot_createMesh1();
        /*
       * 删除全局尺寸子节点
       */
        void slot_deleteGlobalMesh();
        /*
        * 生成网格自适应剖分
        */
        void slot_createMeshAutoAdapt();
        /*
        * 生成网格自定义加密
        */
        void slot_createMeshCustomEncryption();
        /*
        * 生成网格局部尺寸
        */
        void slot_createMeshLocalSize();
        /*
        * 执行边抑制
        */
        void slot_createMeshEdgeSuppress(bool isSuppress);

        /***************************************************************************************截面形状***********************************************************************************/
        /*
        * 更新截面形状工况树
        */
        void slot_updateSectionShapeTree(bool isCreate, int sectionshapeID);

        /*
        * 创建截面形状
        */
        void slot_createSectionShape();

        /*
        * 删除所有截面形状
        */
        void slot_menuSectionShapeDelete();

        /*
        * 删除截面形状
        */
        void slot_menuSectionShapeChildDel();

        /*
        * 复制截面形状
        */
        void slot_menuCopySectionShape();

        /***************************************************************************************分析工况***********************************************************************************/
        /*
        * 更新分析工况树
        */
        void slot_updateCaseTree(bool isCreate, int coordinateID);

        /*
        * 更新载荷
        */
        void slot_updateLoadOnCaseTree(bool isCreate, int loadID);

        /*
        * 更新转子分析父节点显隐（复模态+转子动力学勾选）
        */
        void slot_updateRotorParentItem(int caseID);

        void slot_updateRotateAnalyseOnCaseTree(bool isCreate, int rotateAnalyseID);

        /*
        * 更新边界
        */
        void slot_updateBoundaryOnCaseTree(bool isCreate, int boundaryID);

        /*
        * 更新预定义场
        */
        void slot_updatePredefinedFieldOnCaseTree(bool isCreate, int fieldID);

        /*
        * 复制工况
        */
        void slot_menuCopyCase();


        /*
        * 删除工况
        */
        void slot_menuCaseDelete();

        /*
        * 创建载荷子节点
        */
        void slot_createLoad(MxFunctions::LoadBoundaryType type);

        /*
         * 创建转子分析子节点
         */
        void slot_createRotateAnalyse(MxFunctions::RotateAnalysyType type);

        /*
         * 删除载荷
         */
        void slot_menuLoadChildDel();

        /*
         * 删除转子分析子节点
         */
        void slot_menuRotateChildDel();

        /*
        * 抑制/取消抑制载荷边界
        */
        void slot_menuSuppressLoadBoundary();

        /*
         * 删除全部载荷
         */
        void slot_menuLoadDelete();

        /*
         * 删除全部转子分析
         */
        void slot_menuRotateDelete();

        /*
         * 复制载荷
         */
        void slot_menuCopyLoad();

        /*
         * 复制转子分析
         */
        void slot_menuCopyRotate();

        /*
        * 创建边界子节点
        */
        void slot_createBoundary(MxFunctions::LoadBoundaryType type);

        /*
        * 删除边界
        */
        void slot_menuBoundaryChildDel();

        /*
        * 删除全部边界
        */
        void slot_menuBoundaryDelete();

        /*
        * 复制边界
        */
        void slot_menuCopyBoundary();

        /*
         * 创建预定义场子节点
         */
        void slot_createPredefinedField(MxFunctions::PredefinedFieldType type);

        /*
         * 删除预定义场
         */
        void slot_menuPredefinedFieldChildDel();

        /*
         * 删除全部预定义场
         */
        void slot_menuPredefinedFieldDelete();

        /*
         * 复制预定义场
         */
        void slot_menuCopyPredefinedField();

        /***************************************************************************************材料***********************************************************************************/

        void slot_updateMatTree(bool isCreate, int matID);

        /*
        * 删除所有材料
        */
        void slot_menuMaterialDelete();

        /*
        * 显示/隐藏当前材料的部件
        */
        void slot_menuShowHideMaterial(bool isShow);

        /*
        * 删除材料
        */
        void slot_menuMaterialChildDel();

        /*
        * 复制材料
        */
        void slot_menuCopyMaterial();

        /*
         * 创建材料
         */
        void slot_createMaterial();

        /*
         * 添加到自定义材料库
         */
        void slot_menuAddToMaterialDepot();
        /*********************************************************************************结构优化*********************************************************************************/

        /*
        * 更新结构优化树
        */
        void slot_updateOptimizationTree(bool isCreate);

        /*
        * 删除结构优化
        */
        void slot_menuStructOptionDelete();

        /*
        * 创建优化设计域
        */
        void slot_menuOptimizationDesignNew();

        /*
        * 删除所有优化设计域
        */
        void slot_menuOptimizationDesignDelete();

        /*
        * 删除优化设计域
        */
        void slot_menuOptimizationDesignChildDel();

        /*
        * 复制优化设计域
        */
        void slot_menuCopyOptimizationDesign();

        /*
        * 更新优化设计域树
        */
        void slot_updateOptimizationDesignTree(bool isCreate, int ID);

        /*
       * 创建优化约束
       */
        void slot_menuOptimizationConstraintNew();

        /*
        * 删除所有优化约束
        */
        void slot_menuOptimizationConstraintDelete();

        /*
        * 删除优化约束
        */
        void slot_menuOptimizationConstraintChildDel();

        /*
        * 复制优化约束
        */
        void slot_menuCopyOptimizationConstraint();

        /*
        * 更新优化约束
        */
        void slot_updateOptimizationConstraintTree(bool isCreate, int ID);

        /*********************************************************************************流道优化*********************************************************************************/

        /*
         * 更新流道优化树
         */
        void slot_updateRunnerOptimizationTree(bool isCreate);

        /*
         * 删除流道优化
         */
        void slot_menuRunnerStructOptionDelete();

        /*
         * 创建流道优化设计域
         */
        void slot_menuRunnerOptimizationDesignNew();

        /*
         * 删除所有流道优化设计域
         */
        void slot_menuRunnerOptimizationDesignDelete();

        /*
         * 删除流道优化设计域
         */
        void slot_menuRunnerOptimizationDesignChildDel();

        /*
         * 复制流道优化设计域
         */
        void slot_menuCopyRunnerOptimizationDesign();

        /*
         * 更新流道优化设计域树
         */
        void slot_updateRunnerOptimizationDesignTree(bool isCreate, int ID);

        /*
         * 创建流道优化约束
         */
        void slot_menuRunnerOptimizationConstraintNew();

        /*
         * 删除所有流道优化约束
         */
        void slot_menuRunnerOptimizationConstraintDelete();

        /*
         * 删除流道优化约束
         */
        void slot_menuRunnerOptimizationConstraintChildDel();

        /*
         * 复制流道优化约束
         */
        void slot_menuCopyRunnerOptimizationConstraint();

        /*
         * 更新流道优化约束树
         */
        void slot_updateRunnerOptimizationConstraintTree(bool isCreate, int ID);

        /*******************************************************************************全局参数************************************************************************************/
        /*
        * 删除所有全局参数
        */
        void slot_menuGlobalParamDelete();

        /*
         * 更新超单元参数树
         */
         void slot_updateGenericParamTree(bool isCreate, int ID);

         /*
         * 删除所有超单元参数
         */
         void slot_menuGenericParamDelete();

         /*
         * 删除超单元参数
         */
         void slot_menuGenericParamChildDelete();

         /*
         * 复制超单元参数
         */
         void slot_menuGenericParamChildCopy();

         /*
         * 更新超单元调用树
         */
         void slot_updateGenericAssignTree(bool isCreate);

         /*
         * 删除超单元调用
         */
         void slot_menuGenericAssignDelete();

         /*
         * 更新声固耦合对搜索树
         */
         void slot_updateASCouplingSearchTree(bool isCreate);

         /*
         * 删除声固耦合对搜索
         */
         void slot_menuASCouplingSearchDelete();

         /*
         * 更新超单元矩阵名称树
         */
         void slot_updateGenericMatrixTree(bool isCreate);

         /*
         * 删除超单元矩阵名称
         */
         void slot_menuGenericMatrixDelete();

        /*
        * 更新超单元部件树
        */
         void slot_updateGenericPartTree(bool isCreate);

         /*
         * 删除超单元部件
         */
         void slot_menuGenericPartDelete();

         /*
         * 更新求解控制树
         */
         void slot_updateSolveControlTree(bool isCreate);

         /*
         * 更新全局输出控制树
         */
         void slot_updateGlobalOutputControlTree(bool isCreate);

         /*
         * 删除求解控制
         */
         void slot_menuSolveControlDelete();

         /*
         * 删除全局输出控制
         */
         void slot_menuGlobalOutputControlDelete();

        /*********************************************************************************连接*********************************************************************************/

        /*
        * 更新或创建连接
        */
        void slot_updateConnectorTree(bool isCreate, int connID);

        /*
        * 更新或创建连接副
        */
        void slot_updateDeputyTree(bool isCreate, int deputyID);

        /*
        * 删除所有连接
        */
        void slot_menuConnectorDelete();

        /*
        * 删除所有连接副
        */
        void slot_menuDeputyDelete();

        /*
        * 删除连接
        */
        void slot_menuConnectorChildDel();

        /*
        * 抑制/取消抑制连接
        */
        void slot_menuSuppressConnector();

        /*
        * 删除连接副
        */
        void slot_menuDeputyChildDel();

        /*
        * 抑制/取消抑制连接副
        */
        void slot_menuSuppressDeputy();

        /*
        * 复制连接
        */
        void slot_menuCopyConnector();

        /*
        * 复制连接副
        */
        void slot_menuCopyDeputy();

        /*
        * 连接类型图片
        */
        QString slot_connTypePic(MxFunctions::ConnectorType type = MxFunctions::Connector_Spring);

        /*
        * 连接副类型图片
        */
        QString slot_deputyTypePic(MxFunctions::DeputyType type = MxFunctions::Deputy_Connector);

        /*
       * 更新连接名称
       */
        void slot_updateConnChildItemName(const int phyID, const QString& OldName, const QString& NewName, const QString& tarName);

        /*********************************************************************************接触*********************************************************************************/
    
        /*
         * 批量创建接触
         */
        void slot_batchCreateContactTree(std::vector<int> ids);
    
        /*
         * 更新或创建接触
         */
        void slot_updateContactTree(bool isCreate, int connID);
    
        /*********************************************************************************模型修改*********************************************************************************/
    
        /*
        * 创建/更新模型修改树节点
        */
        void slot_updateModelFixTree(bool isCreate, int modelFixID);
    
        /*
         * 删除所有模型修改
         */
        void slot_menuModelFixDelete();
    
        /*
         * 删除模型修改
         */
        void slot_menuModelFixChildDel();
    
        /*
         * 复制模型修改
         */
        void slot_menuCopyModelFix();

        /*
         * 删除工况下所有模型修改
         */
        void slot_menuCaseModelFixDelete();

        /*
         * 删除工况下模型修改
         */
        void slot_menuCaseModelFixChildDel();

        /*
         * 复制工况下模型修改
         */
        void slot_menuCopyCaseModelFix();

        /*
         * 抑制/取消抑制模型修改
         */
        void slot_menuSuppressModelFix();
    
        /*********************************************************************************接触*********************************************************************************/
    
        /*
         * 删除所有接触
         */
        void slot_menuContactDelete();
    
        /*
         * 删除接触
         */
        void slot_menuContactChildDel();
    
        /*
         * 抑制/取消抑制接触
         */
        void slot_menuSuppressContact();
    
        /*
         * 复制接触
         */
        void slot_menuCopyContact();

        /*********************************************************************************周期性边界*********************************************************************************/
        /*
         * 更新或创建周期性边界
         */
        void slot_updatePeriodicBoundaryTree(bool isCreate, int pbID);

        /*
         * 删除所有周期性边界
         */
        void slot_menuPeriodicBoundaryDelete();

        /*
         * 删除周期性边界
         */
        void slot_menuPeriodicBoundaryChildDel();

        /*
         * 抑制/取消抑制周期性边界
         */
        void slot_menuSuppressPeriodicBoundary();

        /*
        * 周期性边界是否抑制
        */
        bool isCurrentSelectedPeriodicBoundarySuppressed();

        /*
         * 复制周期性边界
         */
        void slot_menuCopyPeriodicBoundary();
        /*
         * 工况下相互作用子节点删除
         */
        void slot_menuCaseInteractionChildDel();
        /*
         * 工况下相互作用子节点复制
         */
        void slot_menuCopyCaseInteraction();


        /***************************************************************************************后处理***********************************************************************************/
        /*
        * 创建三维结果
        */
        //【集成·死声明清理】void slot_create3DResult();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建曲线
        */
        //【集成·死声明清理】void slot_createCurve();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建导出word
        */
        //【集成·死声明清理】void slot_createExportWord();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
            创建功能阵列
        */
        //【集成·死声明清理】void slot_createParrayMimage();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
            创建功能阵列
        */
        //【集成·死声明清理】void slot_createCompVideo();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
            创建爆炸视图
        */
        //【集成·死声明清理】void slot_createExplode();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
            创建测量工具
        */
        //【集成·死声明清理】void slot_createMeasureTool();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建结果路径
        */

        //创建注释
        void slot_createNote();

        //创建标注
        void slot_createLabel();

        /*
        * 创建动画
        */
        //【集成·死声明清理】void slot_createAnimation(int type);  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建线性叠加面板
        */
        //【集成·死声明清理】void slot_createLinearSuperposition();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建标量运算面板
        */
        //【集成·死声明清理】void slot_createScalarOperation();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建切面图
        */
        //【集成·死声明清理】void slot_createSection();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建等值面图
        */
        //【集成·死声明清理】void slot_createIsosurface();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建矢量图
        */
        //【集成·死声明清理】void slot_createVector();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建流线图
        */
        //【集成·死声明清理】void slot_createStreamLine();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建分区
        */
        //【集成·死声明清理】void slot_createPartition();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建集合
        */
        //【集成·死声明清理】void slot_createPostSet();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 根据分区数据更新不同进程单元颜色
        */
        //【集成·死声明清理】void updatePartitionElementColor(const QHash<int, std::set<int>>& partitionElements);  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建二维线
        */
        //【集成·死声明清理】void slot_createLine();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建箭头
        */
        //【集成·死声明清理】void slot_createArrow();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建文本
        */
        //【集成·死声明清理】void slot_createText();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建日志
        */
        //【集成·死声明清理】void slot_createLog();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 创建路径
        */
        //【集成·死声明清理】void slot_createPath();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 更新结果树
        */
        void slot_updateResultTree();

        /*
        * 添加曲线节点
        */
        //【集成·死声明清理】void slot_updatePostCurveTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加导出Word报告节点
        */
        //【集成·死声明清理】void slot_updatePostExportWordTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加标量运算节点
        */

        /*
        * 更新后处理结果状态（modelID为结果所属模型，仅当前模型时刷新树）
        */
        void slot_updateStatusPostResult(int modelID, int caseID);

        /*
        * 计算完成导入后处理结果（modelID为结果所属模型，仅当前模型时导入）
        */
        void slot_importPostResult(int modelID, int caseID);

        /*
        * 添加路径节点
        */
        //【集成·死声明清理】void slot_updatePostPathTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加二维线节点
        */
        //【集成·死声明清理】void slot_updateLineTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加箭头节点
        */
        //【集成·死声明清理】void slot_updateArrowTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加文本节点
        */
        //【集成·死声明清理】void slot_updateTextTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加日志节点
        */
        //【集成·死声明清理】void slot_updateLogTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加动画节点
        */
        //【集成·死声明清理】void slot_updateAnimationTree(int type);  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加线性叠加节点
        */
        //【集成·死声明清理】void slot_updateLinearSuperpositionTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加切面图节点
        */
        //【集成·死声明清理】void slot_updateSectionTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加等值面图节点
        */
        //【集成·死声明清理】void slot_updateIsosurfaceTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加矢量图节点
        */
        //【集成·死声明清理】void slot_updateVectorTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加流线图节点
        */
        //【集成·死声明清理】void slot_updateStreamLineTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 添加集合节点
        */
        //【集成·死声明清理】void slot_updatePostSetTree();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 三维结果父节点-删除
        */
        //【集成·死声明清理】void slot_menu3DResultDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 三维结果父节点-更新
        */
        //【集成·死声明清理】void slot_menu3DResultUpdate();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 动画子节点-删除
        */
        //【集成·死声明清理】void slot_menuAnimationDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 线性叠加子节点-删除
        */
        //【集成·死声明清理】void slot_menuLinearDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 标量运算子节点-删除
        */
        //【集成·死声明清理】void slot_menuScalarOperationDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 曲线子节点-删除
        */
        //【集成·死声明清理】void slot_menuCurveDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 导出Word子节点-删除
        */
        //【集成·死声明清理】void slot_menuExportWordDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 路径子节点-删除
        */
        //【集成·死声明清理】void slot_menuPathDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
       * 切面子节点-删除
       */
        //【集成·死声明清理】void slot_menuSectionDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 等值面子节点-删除
        */
        //【集成·死声明清理】void slot_menuIsoDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019

        /*
        * 矢量图子节点-删除
        */

        /*****************************************************************************频率点采样方式******************************************************************************/

        /*
        * 频率点采样方式子节点创建
        */
        void slot_menuFreqSamplPoint();

        /*
        * 删除全部
        */
        void slot_menuFreqSamplPointDelete();

        /*
        * 子点删除
        */
        void slot_menuFreqSamplPointChilDelete();

        /*
        * 复制
        */
        void slot_menuCopyFreqSamplPoint();

        /*
        * 更新频率点采样方式树
        */
        void slot_updateFreqSamplPointTree(bool isCreate, int freqSamplPointID);

        /*****************************************************************************层合结构******************************************************************************/

        /*
        * 创建单层板
        */
        void slot_menuSglLayerPlate();

        /*
        * 删除所有单层板
        */
        void slot_menuSglLayerPlateDelete();

        /*
        * 创建全局单层板
        */
        void slot_menuGlobalSglLam();

        /*
        * 创建层合板
        */
        void slot_menuLamPlate();

        /*
        * 删除所有层合板
        */
        void slot_menuLamPlateDelete();

        /*
        * 删除所有
        */
        void slot_menuDeleteAllLaminate();

        /*
        * 删除层合结构子节点
        */
        void slot_menuLaminateChildDelete();

        /*
        * 复制层合结构子节点
        */
        void slot_menuLaminateChildCopy();

        /*
        * 判断层合板管理器下是否无任何子节点，如果没有则移除层合板管理器
        */
        void isDeleteLaminateManagerItem();

        /*
        * 更新层合结构树树
        */
        void slot_updateLaminateTree(bool isCreate, int laminateID);

        /**********************************************************************诊断优化**************************************************************************/
        /*
         * 更新模型树及工况树->贡献量
         */
        void slot_updateContriTree(bool isCreate, int laminateID, QString name);

        /*
         * 更新工况树->峰值筛选
         */
        void slot_updateCasePeakFilterTree(bool isCreate, int caseID);

        /*
        * 删除所有贡献量
        */
        void slot_menuContriParentDelete();

        /*
         * 删除单个贡献量
         */
        void slot_menuContriChildDelete();

        /*
         * 删除等效辐射功率
         */
        void slot_menuERPDelete();

        /*
         * 更新峰值筛选树
         */
        void slot_updatePeakFilterTree(bool isCreate, int peakFilterID);

        /*
         * 删除所有峰值筛选
         */
        void slot_menuPeakFilterParentDelete();

        /*
         * 删除单个峰值筛选
         */
        void slot_menuPeakFilterChildDelete();

        /*
         * 复制峰值筛选
         */
        void slot_menuPeakFilterChildCopy();

        /*
         * 删除工况下诊断优化所有贡献量
         */
        void slot_menuDiagOptDelete();

        /*
        * 工况下峰值筛选节点删除
        */
        void slot_casePeakFilterDelete();
        /*********************************************************************************撤销重做*********************************************************************************/

        ///*
        //* 材料创建撤销-删除
        //* 材料删除重做-删除
        //*/
        //void slot_commondDeleteMaterial();

        ///*
        //* 材料创建重做-创建
        //* 材料删除撤销-创建
        //*/
        //void slot_commondCreateMaterial();

        ///*
        //* 材料属性撤销重做
        //*/
        //void slot_commondModMatProp();

        ///*
        //* 载荷创建撤销-删除
        //* 载荷删除重做-删除
        //*/
        //void slot_commondDeleteLoad();

        ///*
        //* 载荷创建重做-创建
        //* 载荷删除撤销-创建
        //*/
        //void slot_commondCreateLoad();

        ///*
        //* 边界创建撤销-删除
        //* 边界删除重做-删除
        //*/
        //void slot_commondDeleteBoundary();

        ///*
        //* 边界创建重做-创建
        //* 边界删除撤销-创建
        //*/
        //void slot_commondCreateBoundary();

        ///*
        //* 连接创建撤销-删除
        //* 连接删除重做-删除
        //*/
        //void slot_commondDeleteConnector();

        ///*
        //* 连接创建重做-创建
        //* 连接删除撤销-创建
        //*/
        //void slot_commondCreateConnector();
        //【集成·死声明清理】void slot_menuVectorDelete();  //dev 已迁 mPostTreeView，moc 引用致 LNK2019





        /***************************************************************************************多体动力学-运动体***********************************************************************************/
        /*
        * 创建运动体
        */
        void slot_menuMotionBodyNew();

        /*
        * 删除所有运动体
        */
        void slot_menuMotionBodyDelete();

        /*
        * 删除运动体
        */
        void slot_menuMotionBodyChildDel();

        /*
        * 复制运动体
        */
        void slot_menuCopyMotionBody();

        /*
        * 更新运动体树
        */
        void slot_updateMotionBodyTree(bool isCreate, int ID);

        /*
        * 批量更新运动体树（一次追加全部节点，仅末尾一次选中；状态/信息表走批量口径）
        */
        void slot_updateMotionBodyTreeBatch(QVector<int> IDs);

        /*
        * 多体根"删除运动数据"：清空多体分支全部对象（含求解方案），部件/几何/网格保留
        */
        void slot_menuMotionRootDeleteAll();

        /*
        * 获取运动体图标
        */
        QString getMotionBodyPic(bool isFixed);

        /***************************************************************************************多体动力学-运动副***********************************************************************************/

        /*
        * 创建运动副
        */
        void slot_menuMotionJointNew(MxFunctions::JointType type);

        /*
        * 删除所有运动副
        */
        void slot_menuMotionJointDelete();

        /*
        * 删除运动副
        */
        void slot_menuMotionJointChildDel();

        /*
        * 复制运动副
        */
        void slot_menuCopyMotionJoint();

        /*
        * 更新运动副树
        */
        void slot_updateMotionJointTree(bool isCreate, int ID);

        /*
        * 获取运动副图标
        */
        QString getMotionJointPic(MxFunctions::JointType type);

        /***************************************************************************************多体动力学-耦合副***********************************************************************************/

        void slot_menuMotionCouplingNew(MxFunctions::CouplingType type);

        /*
        * 删除所有耦合副
        */
        void slot_menuMotionCouplingDelete();

        /*
        * 删除耦合副
        */
        void slot_menuMotionCouplingChildDel();

        /*
        * 复制耦合副
        */
        void slot_menuCopyMotionCoupling();

        /*
        * 更新耦合副树
        */
        void slot_updateMotionCouplingTree(bool isCreate, int ID);

        /*
        * 获取耦合副图标
        */
        QString getMotionCouplingPic(MxFunctions::CouplingType type);

        /*
        * 获取传感器图标（按类型）
        */
        QString getMotionSensorPic(MechMotionData::SensorType type);

        /***************************************************************************************多体动力学-驱动***********************************************************************************/

        /*
        * 创建驱动
        */
        void slot_menuMotionDriveNew(MxFunctions::DriveTargetType type);

        /*
        * 删除所有驱动
        */
        void slot_menuMotionDriveDelete();

        /*
        * 删除驱动
        */
        void slot_menuMotionDriveChildDel();

        /*
        * 复制驱动
        */
        void slot_menuCopyMotionDrive();

        /*
        * 更新驱动树
        */
        void slot_updateMotionDriveTree(bool isCreate, int ID);

        /*
        * 获取驱动图标
        */
        QString getMotionDrivePic(MxFunctions::DriveTargetType type);
      
        /***************************************************************************************多体动力学-标记点***********************************************************************************/
      
        /*
        * 创建标记点
        */
        void slot_menuMotionMarkerNew();
      
        /*
        * 删除所有标记点
        */
        void slot_menuMotionMarkerDelete();
      
        /*
        * 删除标记点
        */
        void slot_menuMotionMarkerChildDel();
      
        /*
        * 复制标记点
        */
        void slot_menuCopyMotionMarker();
      
        /*
         * 更新标记点树
         */
        void slot_updateMotionMarkerTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-力元***********************************************************************************/

        /*
         * 创建矢量力
         */
        void slot_menuMotionVectorForceNew();

        /*
         * 创建矢量力矩
         */
        void slot_menuMotionVectorTorqueNew();

        /*
         * 删除所有力元
         */
        void slot_menuMotionVectorForceDelete();

        /*
         * 删除力元
         */
        void slot_menuMotionVectorForceChildDel();

        /*
         * 复制力元
         */
        void slot_menuCopyMotionVectorForce();

        /*
         * 更新力元树
         */
        void slot_updateMotionVectorForceTree(bool isCreate, int ID);


        /***************************************************************************************多体动力学-接触***********************************************************************************/

        /*
         * 新建 3D 接触
         */
        void slot_menuMotionContactNew();

        /*
         * 删除所有接触
         */
        void slot_menuMotionContactDelete();

        /*
         * 删除接触
         */
        void slot_menuMotionContactChildDel();

        /*
         * 复制接触
         */
        void slot_menuCopyMotionContact();

        /*
         * 更新接触树
         */
        void slot_updateMotionContactTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-弹簧***********************************************************************************/
        void slot_menuMotionSpringNew();
        void slot_menuMotionSpringNewTorsion();
        void slot_menuMotionSpringDelete();
        void slot_menuMotionSpringChildDel();
        void slot_menuCopyMotionSpring();
        void slot_updateMotionSpringTree(bool isCreate, int ID);
        /*
        * 获取弹簧图标
        */
        QString getMotionSpringPic(MechMotionData::SpringType type);

        /***************************************************************************************多体动力学-求解方案***********************************************************************************/

        /*
        * 求解
        */
        void slot_solveMotionScenario();

        /*
        * 创建求解方案
        */
        void slot_menuMotionScenarioNew();

        /*
        * 删除所有求解方案
        */
        void slot_menuMotionScenarioDelete();

        /*
        * 删除求解方案
        */
        void slot_menuMotionScenarioChildDel();

        /*
         * 更新求解方案
         */
        void slot_updateMotionScenarioTree(bool isCreate, int ID);

        /*
         * 求解
         */

        /***************************************************************************************多体动力学-控制输入***********************************************************************************/
        void slot_menuMotionControlInputNew();
        void slot_menuMotionControlInputDelete();
        void slot_menuMotionControlInputChildDel();
        void slot_menuCopyMotionControlInput();
        void slot_updateMotionControlInputTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-机电导入***********************************************************************************/
        void slot_menuMotionElectroImportNew();
        void slot_menuMotionElectroImportDelete();
        void slot_menuMotionElectroImportChildDel();
        void slot_menuCopyMotionElectroImport();
        void slot_updateMotionElectroImportTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-机电导出***********************************************************************************/
        void slot_menuMotionElectroExportNew();
        void slot_menuMotionElectroExportDelete();
        void slot_menuMotionElectroExportChildDel();
        void slot_menuCopyMotionElectroExport();
        void slot_updateMotionElectroExportTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-液压协同***********************************************************************************/
        void slot_menuMotionHydraulicNew();
        void slot_menuMotionHydraulicDelete();
        void slot_menuMotionHydraulicChildDel();
        void slot_menuCopyMotionHydraulic();
        void slot_updateMotionHydraulicTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-强化学习***********************************************************************************/
        void slot_menuMotionRLNew();
        void slot_menuMotionRLDelete();
        void slot_menuMotionRLChildDel();
        void slot_menuCopyMotionRL();
        void slot_updateMotionRLTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-传感器***********************************************************************************/
        void slot_menuMotionSensorNew(MechMotionData::SensorType type);
        void slot_menuMotionSensorDelete();
        void slot_menuMotionSensorChildDel();
        void slot_menuCopyMotionSensor();
        void slot_updateMotionSensorTree(bool isCreate, int ID);
        void slot_menuMotionControllerNew();
        void slot_menuMotionControllerDelete();
        void slot_menuMotionControllerChildDel();
        void slot_menuCopyMotionController();
        void slot_updateMotionControllerTree(bool isCreate, int ID);

        /***************************************************************************************多体动力学-自动柔性体***********************************************************************************/
        void slot_menuMotionAutoFlexNew();
        void slot_menuMotionAutoFlexDelete();
        void slot_menuMotionAutoFlexChildDel();
        void slot_menuCopyMotionAutoFlex();
        void slot_updateMotionAutoFlexTree(bool isCreate, int ID);
    private:
        ///////////////////////////////////////////封装接口，可直接使用，方便理解///////////////////////////////////////////////////////////////
        /*
        * 填充装配分组：顶层装配递归构建层级（含部件引用节点），自由部件按名称自然排序附底；
        * 同一装配/部件被多父引用时以 key._parentID 区分树实例
        */
        void fillAssemblyTree(MechModelData::mModelData* modelData);
        //获取选中节点索引
        QModelIndexList getSelectIndex() { return selectionModel()->selectedRows(); }
        //源/代理自适应解析：本视图代理索引映射到源模型；已是源模型索引原样返回
        QModelIndex toSourceIndex(const QModelIndex& idx) const;
        mItemKey getItemKeyByIndex(const QModelIndex& proxyIdx);
        QList<mItemKey> getItemKeyByIndex(const QModelIndexList& proxyIdx);
        void deSelectItem(MechPreData::mTreeItem* item);
        void deSelectItem(QVector<MechPreData::mTreeItem*>& items);
        bool isItemSelect(MechPreData::mTreeItem* item);
        void expandItem(MechPreData::mTreeItem* item);
        void expandItem(QModelIndex proxyIdx);
        /*
        * 选中某一个节点
        */
        void selectedItem(MechPreData::mTreeItem* item);
        void selectedItem(QVector<MechPreData::mTreeItem*>& items);
        void selectedItem(QModelIndexList proxyIdx);

        /*
        * 通过索引获取数据-该索引为_treeProxyModel索引
        */
        int getPhyIdByModelIndex(QModelIndex proxyIdx);
        int getPhyIdByItem(MechPreData::mTreeItem* item);
        int getModelIdByModelIndex(QModelIndex proxyIdx);
        int getModelIdByItem(MechPreData::mTreeItem* item);
        int getCaseIdByModelIndex(QModelIndex proxyIdx);
        int getCaseIdByItem(MechPreData::mTreeItem* item);
        QString getTextByModelIndex(QModelIndex proxyIdx);
        QString getTextByItem(MechPreData::mTreeItem* item);
        MxFunctions::BrowserTreeType getTreeTypeByModelIndex(QModelIndex proxyIdx);
        MxFunctions::BrowserTreeType getTreeTypeByItem(MechPreData::mTreeItem* item);
        MxFunctions::BrowserTreeStatus getStatusByModelIndex(QModelIndex proxyIdx);
        MxFunctions::BrowserTreeStatus getStatusByItem(MechPreData::mTreeItem* item);
        QString getIconStringByModelIndex(QModelIndex proxyIdx);

        /*
        * 判断当前选中的连接是否被抑制（仅支持单选）
        */
        bool isCurrentSelectedConnectorSuppressed();

        /*
        * 判断当前选中的载荷边界是否被抑制（仅支持单选）
        */
        bool isCurrentSelectedLoadBoundarySuppressed();

        /*
        * 判断当前选中的连接副是否被抑制（仅支持单选）
        */
        bool isCurrentSelectedDeputySuppressed();

        /*
         * 判断当前选中的接触是否被抑制（仅支持单选）
         */
        bool isCurrentSelectedContactSuppressed();

        /*
         * 判断当前选中的模型修改是否被抑制（仅支持单选）
         */
        bool isCurrentSelectedModelFixSuppressed();

        QString getIconStringByItem(MechPreData::mTreeItem* item);
        bool getHiddenByModelIndex(QModelIndex proxyIdx);
        bool getHiddenByItem(MechPreData::mTreeItem* item);
        MechPreData::mTreeItem* getTreeItemByIndex(QModelIndex proxyIdx);

        void setTreeItemStatus(MechPreData::mTreeItem* item, MxFunctions::BrowserTreeStatus stauts);
        void setTreeItemPic(MechPreData::mTreeItem* item, QString picString);
        void setTreeItemText(MechPreData::mTreeItem* item, QString text);
        void setTreeItemHidden(MechPreData::mTreeItem* item, bool hidden); 

        //移除节点
        void removeProxyIndexs(QModelIndexList proxyIndexs);

        /*
        * 移除自身及其子节点
        */
        void removeTreeItem(MechPreData::mTreeItem* item);
        void removeTreeChildrenItem(MechPreData::mTreeItem* item);


        /*
        * 获取父节点的子节点个数
        */
        int getChildrenSize(QModelIndex idx);

        /*
        * 判断是否为分析父节点
        */
        bool isCaseParentItem(QModelIndex proxyIdx);

        /*
        * setSymbolShow 跳过条件用的可见符号名快照：
        * 在 slot_singleClickItem 处理各分支（可能创建符号）之前拍摄，
        * setSymbolShow 应用新显隐后与快照一致则跳过 UpdateAllSymbol
        */
        std::set<QString> _preClickVisibleSymbols;

    protected:
        QItemSelectionModel::SelectionFlags selectionCommand(
            const QModelIndex& index, const QEvent* event) const;

        /*
        * 部件子节点的几何/网格图标点击切换显隐（不改变选中状态）
        */
        void mousePressEvent(QMouseEvent* event) override;

        /*
        * 离开视图时恢复默认光标
        */
        void leaveEvent(QEvent* event) override;

    private:
        /*
        * 点击部件子节点的几何/网格显隐图标：
        * 点击项在选中集合内时对所有选中的部件子节点生效，否则仅对点击项生效；
        * 目标部件全部可见则统一隐藏，任一隐藏则统一显示
        */
        void togglePartDisplayByIcon(const QModelIndex& clickedProxyIdx, bool isGeoIcon);

    private:
        QMenu* m_Menu = nullptr;

        //结果导入的目标工况ID（importPostResult信号设置，建云图树节点用）
        int _resultCaseID = -1;

        //模型树数据
        MechPreData::mTreeModel* _treeModel;
        //模型数据与视图交互
        MechPreData::mTreeProxyModel* _treeProxyModel;

        QMetaObject::Connection m_viewportChangeConn;

        MechPreData::mViewportPrivate* m_viewportPrivate = nullptr;

        //是否在草图模式，草图模式下新建几何不设置选中item
        bool isSketch = false;

        //视图事件-鼠标右键定时器
        QTimer* m_osgPressTimer;
        bool m_isPressing = false;
    };


}


