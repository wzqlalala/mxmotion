#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线图层管理对话框（非模态单例，任务 5.2，需求图 1212"图层"）
//
// 树形呈现模板应用产生的"层"（mCurveLayerManager），两级（需求图形态）：
//   "Page(s): 页号列表" 组节点（可勾选=整组层显/隐，无右键菜单）
//     └ "Layer N: 层名" 层节点（勾选=成员曲线整体可见，右键：重命名/删除）
// D10（2026-10-30 修订；2026-11-02 v3+v4）：组内基准线层置顶（=视图页节点
// 的第一个子节点，按生成序），文本 "Layer -N: Datum Lines and Reference
// Curves"（显示号取负 -N=-datumNo，-1 起独立分序与曲线层正编号视觉分序；
// datumNo 本身仍为正）；基准线层与普通层同权——右键重命名/删除，勾选=成员
// 基准线整体显/隐，勾选态随成员可见状态现算（与曲线层同规则）。
// 交互：
//   - 勾选：层勾=setCurvesVisible 整层显/隐（写既有 kVisible 与曲线树同键，
//     图表/视图联动免费获得；成员=曲线+基准线）；组勾=逐层 setCurvesVisible；
//     勾选态现算不存储（D5），联动后父/子节点勾选态就地刷新，不整树重建；
//   - 重命名：只改层名不动成员节点 kName；删除：removeLayer 包单个撤销宏
//     （一次 Ctrl+Z 整体撤销）后重建树；
//   - "刷新"：读侧现查重建（层/成员/页面被删即剔除）。
// 展开状态按节点 id 记忆，重建后恢复；无记忆时组节点默认全展开。
//
// 实现说明（设计偏差记录）：设计为 QAbstractItemModel 挂接，v1 用 QTreeWidget
// 快速达成（会话内存活数据量小，刷新成本低），后续如需批量联动再迁模型。
// 形态：非模态无边框白卡片（M 徽标标题栏 + ✕，对齐 mTemplateCurveDlg）；
// 原三级树（层下挂成员曲线叶）按需求图收敛为两级，曲线显隐控制收敛到层级。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QDialog>
#include <QSet>

class QMouseEvent;
class QTreeWidget;
class QTreeWidgetItem;

namespace PostCurve
{
	class POSTCURVE_EXPORT mLayerManageDlg : public QDialog
	{
		Q_OBJECT
	public:
		//入口：打开对话框单例（已打开则重建树并恢复展开态）
		static void showInstance(QWidget* parent);

		mLayerManageDlg(const mLayerManageDlg&) = delete;
		mLayerManageDlg& operator=(const mLayerManageDlg&) = delete;

	private:
		explicit mLayerManageDlg(QWidget* parent);

		void buildUi();
		//读侧现查重建树：Page(s) 分组 → 层（两级，勾选态全部现算）
		void rebuild();
		//勾选切换（_building 护栏内 setCheckState 的回声不处理）
		void onItemChanged(QTreeWidgetItem* item, int column);
		//层节点右键菜单：重命名/删除
		void onContextMenu(const QPoint& pos);

	protected:
		//标题栏拖动（无边框窗体，对齐 mTemplateCurveDlg）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;

	private:
		static mLayerManageDlg* _instance;

		bool _building = false;          //重建/就地刷勾期间的 itemChanged 护栏
		QSet<int> _expandedIds;          //展开节点 id 记忆（重建后恢复）
		QTreeWidget* _tree = nullptr;    //Page(s) 组 → 层 两级树
		QWidget* _titleBar = nullptr;    //标题栏（拖动热区）
		QPoint _pressPos;
		QPoint _dragOffset;
		bool _dragging = false;
	};
}
