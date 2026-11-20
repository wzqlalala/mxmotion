#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 应用样式对话框（功能弹窗，单例）
// 入口：图表背景区域右键 -> "应用样式"（mCurveChartView::onCanvasContextMenu）。
// 语义：把源图表（右键来源）的样式，按右栏勾选的属性类别，批量复制到左栏
// 勾选的页/图表上：
//   - 左栏"作用对象筛选"：仅可见曲线复选框 + 作用范围（全部页面/当前页面/
//     最后一次选择）+ 全选/全不选/反选图标按钮 + 工程->页->图表复选树
//     （组节点勾选向下级联=应用到其下全部图表；父节点不回算，点任一复选框
//     只影响自身与后代，行无选中/悬停背景；源图表默认不勾，应用时强制跳过）；
//   - 右栏"选项"：Plot属性/图例/页眉/页脚/坐标轴/标记设置/曲线 七组属性
//     叶子，映射 CurveProp 属性键（曲线组默认不勾选——按序配对有错位风险，
//     其余组默认全勾；设计图中模型无对应的子项暂不列入——
//     页眉二级/三级字体、刻度方式、刻度数量、数据格式/精度、清空数据重绘图，
//     待模型扩展后补，避免"勾选无效果"的假象）；
//   - 底部按钮：应用（应用并保持打开）/取消（关闭）/确定（应用并关闭）；
//   - 曲线样式按序配对：目标第 i 条曲线 <- 源第 i 条曲线，源不足时目标
//     剩余不动；勾选"仅可见曲线"时源/目标两侧都先滤掉隐藏曲线再配对。
//
// 写入全部经 mCurveDocController::setNodeProperty（值相同自动跳过命令），
// beginMacro("应用样式") 包裹 -> 一次 Ctrl+Z 整体撤销；属性变化信号驱动
// 各视口自动重渲染（跨页多视口一并生效）。
//
// 风格照 mPropertyEditorPopup：无边框白卡片 + M徽标标题栏 + ✕，非模态、
// 可拖动缩放，DlgGeom::bind 记忆大小/位置。父窗口沿 parentWidget 链走到
// 真正的顶层主窗口（QMdiSubWindow 亦带 Window 标志，直接作父会被 MDI 收编）。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QSet>
#include <QList>
#include <QProxyStyle>
#include <QPainter>
#include <QTreeView>
#include <QStyleOptionViewItem>

class QCheckBox;
class QComboBox;
class QMouseEvent;
class QPushButton;
class QResizeEvent;
class QSizeGrip;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

namespace PostCurve
{
    class POSTCURVE_EXPORT mApplyStyleDlg : public QWidget
    {
        Q_OBJECT
    public:
        //入口：以 sourceChartId 为样式源打开对话框（单例复用，已打开则换绑
        //源图表并重建左栏树）。parent 仅首次创建时使用，可为源视口任意祖先
        //（内部沿 parentWidget 链走到顶层主窗口作为弹窗父窗口）
        static void showForChart(int sourceChartId, QWidget* parent);

        mApplyStyleDlg(const mApplyStyleDlg&) = delete;
        mApplyStyleDlg& operator=(const mApplyStyleDlg&) = delete;

    private:
        explicit mApplyStyleDlg(QWidget* parent);
        ~mApplyStyleDlg() override;

        //--- UI 构建 ---
        void buildUi();
        QVBoxLayout* buildTargetPanel();   //左栏：作用对象筛选
        QVBoxLayout* buildOptionPanel();   //右栏：选项
        void buildOptionTree();            //右栏选项树（构建一次，勾选状态随实例保留）

        //--- 左栏（作用对象筛选） ---
        //按当前作用范围重建树；保留仍存在节点的勾选（失效id自然剔除）。
        //首次重建默认全勾（源图表除外）；"最后一次选择"回放上次应用集
        void rebuildTargetTree();
        //遍历树收集勾选图表id（页/工程节点按级联展开为其下图表）
        QSet<int> collectCheckedCharts() const;

        //--- 右栏（选项） ---
        //选项叶子（UserRole 存的键）是否勾选；键唯一映射一个叶子
        bool optionChecked(const char* key) const;

        //--- 图标按钮的三棵树操作（全选/全不选/反选；反选按叶子翻转后回算组） ---
        void setTreeChecked(QTreeWidget* tree, Qt::CheckState state);
        void invertTreeChecked(QTreeWidget* tree);

        //--- 应用（应用/确定共用）；返回是否实际执行（有作用对象） ---
        bool applyStyles();
        void copyChartProps(int fromChart, int toChart);              //Plot属性
        void copyLegendProps(int fromChart, int toChart);             //图例
        void copyTextProps(int fromChart, int toChart, bool headerSide); //页眉/页脚（挂图表节点）
        void copyAxisProps(int fromChart, int toChart);               //坐标轴外观+标记设置（按"应用到"范围）
        void copyCurveProps(int fromChart, int toChart);              //曲线（按序配对）
        //图表下某类型子节点id（无返回-1）
        int childNodeOfType(int chartId, int type) const;
        //图表下某类型全部子节点id（树序；"全部副轴"按序配对用）
        QList<int> childNodesOfType(int chartId, int type) const;
        //图表下全部曲线id（"仅可见曲线"勾选时滤掉隐藏曲线），顺序=树序
        QList<int> curveNodeIds(int chartId) const;
        //读源节点属性写到目标（走命令栈，值相同自动跳过）
        void copyProp(int fromId, int toId, const char* prop);

	protected:
		//标题栏按下记录按下点 / 移动超阈值进入拖动（双击不挪窗，仅用户拖动改位置）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		//右下角缩放手柄贴角跟随（手柄不占布局行，按钮行距底边才是精确20px）
		void resizeEvent(QResizeEvent* event) override;

    private slots:
        //勾选级联：仅向下（后代跟随），父节点不回算；左栏另同步勾选集
        void onTargetItemChanged(QTreeWidgetItem* item, int column);
        void onOptionItemChanged(QTreeWidgetItem* item, int column);
        void onScopeChanged(int index);
        //源图表（或其祖先页）被删 -> 关闭（此时父子链仍可查询）
        void onNodeAboutToBeRemoved(int nodeId);
        //其他节点被删 -> 剔除勾选集中的失效id；对话框可见时重建左栏树
        void onNodeRemoved(int nodeId);

    private:
        static mApplyStyleDlg* _instance;   //单例

        int _sourceChartId = -1;        //样式源图表节点id
        QSet<int> _checkedChartIds;     //左栏勾选图表id（切换作用范围/重建后保留，超集）
        QSet<int> _lastTargets;         //最后一次应用的作用对象（"最后一次选择"回放，会话内记忆）
        bool _targetInited = false;     //左栏树是否完成首次默认勾选（全勾除源）
        bool _needCenter = true;        //无已存几何时首次打开居中于父窗口

        QWidget* _titleBar = nullptr;   //标题栏（拖动热区）
        QPoint _dragOffset;             //拖动起始偏移
        QPoint _pressPos;               //标题栏按下点（拖动阈值起算，防双击微动挪窗）
        bool _dragging = false;         //拖动中标志

		QCheckBox* _visibleOnlyCheck = nullptr;  //仅可见曲线（曲线样式配对过滤）
		QComboBox* _scopeCombo = nullptr;        //作用范围：0全部页面 1当前页面 2最后一次选择
		QTreeWidget* _targetTree = nullptr;      //作用对象树（工程->页->图表）
		QTreeWidget* _optionTree = nullptr;      //选项树（七组属性叶子）
		QPushButton* _applyBtn = nullptr;        //应用（应用并保持打开）
		QPushButton* _cancelBtn = nullptr;       //取消（关闭不应用）
		QPushButton* _okBtn = nullptr;           //确定（应用并关闭）
		QSizeGrip* _grip = nullptr;              //右下角缩放手柄（覆盖贴角，不占布局行）
	};
}
