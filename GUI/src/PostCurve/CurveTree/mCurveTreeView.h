#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线结构树视图（UI设计图02/01.2）
//
// 数据来源：直接 setModel 控制器的 CurveDocument（经类型过滤代理），
// 文档增删节点时树随 QAbstractItemModel 行变更自动刷新。
//
// 交互职责（对应设计图02）：
//   - 节点状态图标（绿色√=状态正常，仅图标**非复选框**、不参与交互）+
//     类型图标（DecorationRole，程序化绘制）；
//   - 显示/隐藏编辑入口在属性面板的"显示"行与右键菜单（不在树上勾选）；
//   - 树内重命名：双击叶子/F2 进入编辑（可命名类型：页/视口/曲线/基准线/注释；
//     曲线重命名联动图例键 series.name），提交走 kName 命令；
//   - 双击选中节点 -> 控制器选择中枢 setCurrentNodeId
//     -> mMechMainWindow 连接 sig_currentNodeChanged -> 参数栏切换属性页；
//     单击仅移动当前项、不产生选中阴影（防误触，见 selectionCommand）；
//   - 筛选与选中：筛选（类型/名称）只影响可见性，可见行（含骨架节点）
//     全部可选中并写中枢；切换筛选后原选中节点被筛掉（不可见）时清选中
//     并写空中枢；
//   - 反向联动：图上点选曲线/图例项写入中枢后，本树监听 sig_currentNodeChanged
//     回选对应节点（同值短路防回环）；
//   - 右键菜单 -> 菜单项清单按所属图表的类型声明查表（ChartTypeProfile.menus：
//     新建（页/视口/曲线/基准线/注释/坐标轴）/删除/显示隐藏切换/曲线"编辑数据属性"），
//     全部经控制器命令式 API（可撤销）；
//   - Ctrl+Z / Ctrl+Y：树获得焦点时撤销/重做。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "../CurveCore/CurveDefine.h"
#include "../CurveCore/ChartTypeProfile.h"   //图表类型声明（右键菜单清单按此查表）

//QT
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QTimer>

class QMenu;

namespace PostCurve
{
class mCurveDocController;   //右键菜单辅助方法签名用（实现经 mCurveTreeView.cpp 包含）

//结构树过滤代理（UI设计图01.2）：节点类型筛选 + 名称搜索，
//由 mCurveBrowserWindow 顶部的筛选按钮行与搜索框驱动；
//类型筛选时 Project/Page/Chart 骨架节点始终显示，保证树结构完整（只筛选叶子层）；
//名称搜索时保留命中节点及其祖先链（父行被拒则整棵子树不可见）；
//筛选只影响可见性，不改交互能力（可见行全部可选中）
class POSTCURVE_EXPORT mCurveTreeFilterProxy : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit mCurveTreeFilterProxy(QObject* parent = nullptr);
	//允许显示的节点类型（CurveNodeType 的 int 集合）；空集合=不过滤显示全部
	void setAllowedTypes(const QSet<int>& types);
	void clearFilter();   //恢复显示全部
	//名称搜索过滤：显示名称含关键字的节点及其祖先链；与类型筛选叠加生效，空串=关闭
	void setFilterText(const QString& text);
protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
private:
	//行类型是否可通过（骨架节点不受类型筛选限制）
	bool rowAcceptedByType(const QModelIndex& idx) const;
	//行名称是否命中搜索关键字（大小写不敏感包含）
	bool rowNameMatches(const QModelIndex& idx) const;
	//子树中是否存在"类型通过且名称命中"的节点（搜索时保留祖先链）
	bool subtreeHasVisibleMatch(const QModelIndex& idx) const;

	QSet<int> _allowedTypes;   //空集合表示未启用类型过滤
	QString _filterText;       //空串表示未启用名称搜索
};

	//曲线结构树视图（宿主：mCurveBrowserWindow，dock "CurveTree"）
	class POSTCURVE_EXPORT mCurveTreeView : public QTreeView
	{
		Q_OBJECT
	public:
		explicit mCurveTreeView(QWidget* parent = nullptr);
		~mCurveTreeView() override;

		//初始化模型/信号/快捷键（构造中自动调用）
		void initCurveTreeView();

		//当前选中节点id（经代理映射到源模型，无选中返回-1）
		int currentNodeId() const;
		//外部联动选中：设置当前节点并高亮（如图上点选曲线 -> 树同步选中），
		//会触发 onSelectionChanged -> 选择中枢
		void selectNode(int nodeId);

		//过滤代理（供筛选下拉调用 setAllowedTypes/clearFilter）
		mCurveTreeFilterProxy* proxyModel() const;

		//当前中枢选中节点已被筛选隐藏（不在代理可见行内）时，
		//清空树选中并写空选择中枢（参数栏/图上高亮随 sig_currentNodeChanged 复位）；
		//仍可见的节点（含骨架）选中保持，不动作
		void clearSelectionIfFilteredOut();

protected:
//右键菜单（按节点类型出菜单项，见类头注释）
void contextMenuEvent(QContextMenuEvent* event) override;
//单击不选中（防误触）：左键单击仅移动当前项、不出选中阴影，双击才程序化选中
//（见 mouseDoubleClickEvent）；键盘/程序化选中（event 为空）与右键维持默认语义
QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& index, const QEvent* event) const override;
//双击语义（设计图02）：先程序化选中（选中阴影+选择中枢联动），图表节点=激活
//其视口卡片；其余有子节点=仅选中，不展开/收起（张合只由展开箭头负责）；
//叶子节点=进入重命名；F2 对全部可命名节点生效（editTriggers 已含 EditKeyPressed）
void mouseDoubleClickEvent(QMouseEvent* event) override;
//父子节点虚线连接（与视图树/前处理树同款 PaintDashBranches），再画原生展开箭头
void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;

private slots:
	//选中变化 -> 控制器 setCurrentNodeId（选择中枢）
	void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	//节点插入后延迟展开整树（批量插入时事件循环内合并为一次 expandAll）
	void onRowsInserted();
	//全局选中节点变化 -> 树回选该节点（图上点选曲线/图例点击/点空白清选时同步树显示；
	//来源是树自身时 selectNode 落在已选节点上，无实际变化）
	void onCurrentNodeChanged(int nodeId);

	private:
		//右键菜单项清单：图表管辖内的节点查所属图表的类型声明（ChartTypeProfile.menus）；
		//空白处/工程/页不随图表类型变（本地常量）；menus 未列出的类型返回空=不出菜单
		QVector<TreeMenuAction> menuActionsFor(const CurveDocument* doc, int nodeId, CurveNodeType type) const;
		//把菜单动作翻译为菜单项并接入控制器命令（原 contextMenuEvent 各 case 体原样搬入）
		void appendMenuAction(QMenu& menu, TreeMenuAction action,
			mCurveDocController* controller, CurveDocument* doc, int nodeId, CurveNodeType type);

		mCurveTreeFilterProxy* _proxyModel = nullptr;   //类型过滤代理（sourceModel 为 CurveDocument）
		//展开防抖：rowsInserted 逐节点触发（数百图批次 ≈ 数千次插入），restart 语义
		//把同一事件循环内的全部插入合并为事件循环恢复后的一次 expandAll
		//（0ms singleShot 不合并——每次插入各排一个定时器，数千次 expandAll 会
		//在批量导入结束后形成数秒的 UI 冻结尾巴）
		QTimer _expandDebounce;
	};
}
