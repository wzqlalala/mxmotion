#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QVector3D>
#include <QHash>
#include <QPair>
#include <map>
#include <vector>
#include <array>

#include "mPanelBase.h"
#include "PosPickWidget1.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"
#include "mPostPathSolver.h"
#include "PlotDataCache.h"
#include "ZsTempRender.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PlotView
{
	class mPlotView;
}

namespace PostWindow
{
	class mPostPlotFloatWindow;

	// 后处理结果路径面板：实现“两点连线并高亮与线段相交节点”的功能
	class MPOSTWINDOW_EXPORT mPostResultPath : public mPanelBase
	{
		Q_OBJECT

	public:
		// 构造函数
		explicit mPostResultPath(QWidget* parent = nullptr);

		// 新建/打开结果路径面板：初始化状态并绑定当前后处理视口和数据
		void newPathResult(int caseid = -1);

	protected:
		// 面板隐藏时触发：清理临时线条、高亮及拾取状态
		void hideEvent(QHideEvent* event) override;
		// 面板显示时触发：补挂帧/变形信号连接（dock 布局恢复显示不经 newPathResult 入口，QY-06）
		void showEvent(QShowEvent* event) override;

	private slots:
		// 点击“起始点”拾取按钮
		void slot_pickStartPoint();
		// 点击“终止点”拾取按钮
		void slot_pickEndPoint();
		// 点击“起始点”清除按钮
		void slot_clearStartPoint();
		// 点击“终止点”清除按钮
		void slot_clearEndPoint();
		// 点击“路径结果”按钮：按模型拓扑求解起终点间全部等长最短路径
		void slot_generatePath();
		// 点击列表条目：切换当前高亮路径（FR-05），并做显隐指纹惰性重检（R6）
		void slot_pathItemSelected(int row, int column);
		// 云图帧变化：路径显示位置随帧刷新（FR-08，不重算）
		void slot_postFrameChanged();
		// 变形系数变化：路径显示位置随变形刷新（FR-08，不重算）
		void slot_postDeformChanged();
		// 点击“曲线生成”按钮：沿当前选中路径的节点序列取样生成曲线（FR-09，X=路径点序号 1～N，Y=节点真实结果值）
		void slot_generateNodeCurve();

	private:
		// 构建界面布局
		void setupUI();
		// 连接信号槽
		void setupConnections();
		// 设置拾取按钮的视觉状态（高亮/普通）
		void setPickButtonSelecting(QWidget* btn, bool selecting);

		// 开始节点拾取（isStart=true 表示起始点，false 表示终止点）
		void startPointPick(bool isStart);
		// 节点拾取完成后的处理
		void finishPointPick(bool isStart);
		// 清除指定点的拾取状态
		void clearPointPick(bool isStart);

		// 获取指定节点在当前动画帧下的变形后坐标（原始坐标 + 位移缩放）
		QVector3D getDeformedNodePosition(int nodeID) const;
		// 计算线段 startNodeID-endNodeID 与网格相交的单元节点 ID 集合
		std::vector<int> findIntersectionNodeIDs(int startNodeID, int endNodeID) const;
		// 计算线段 startNodeID-endNodeID 与单元面的交点坐标序列
		std::vector<QVector3D> findIntersectionPoints(int startNodeID, int endNodeID) const;
		// 计算线段 startNodeID-endNodeID 实际击中的所有单元（返回 <part_id, ele_rid> 列表）
		std::vector<std::pair<int, int>> findIntersectionElements(int startNodeID, int endNodeID) const;
		// 计算包含指定节点的所有单元（返回 <part_id, ele_rid> 列表）
		std::vector<std::pair<int, int>> findElementsByNode(int nodeID) const;
		// 清理临时线条、临时节点和高亮状态
		void clearResult();

		// ---------------- 拓扑最短路径（03-技术路线 §3.2） ----------------
		// 遍历可见部件/可见单元提取棱边建图（含多面体 poly_face_* 面周长分支与空数据守卫）
		void buildTopologyGraph(PostWindow::PathSolver& solver, mPostModelData* modeldata);
		// FR-09：建立“路径边(连续 ID 对) → 关联单元”映射，棱边提取口径与 buildTopologyGraph 一致
		void buildEdgeElementMap(mPostModelData* modeldata, QHash<qint64, QPair<int, int>>& edgeToElement);
		// “计算中”状态切换：路径结果按钮置灰改文本，拾取/列表/曲线生成按钮禁用（02 §3.3）
		void setComputingState(bool computing);
		// singleShot 延迟执行：建图 + 求解 + 结果分发（列表/3D/消息/指纹）
		void solveAndShowPath();
		// 绘制指定序号路径：temp 折线（节点真实 ID 对）+ 路径节点标记（当前帧变形后坐标）
		void showPath(int index);
		// FR-08：按当前帧刷新起终点记录坐标与路径显示位置（不重算、列表不动）
		void refreshPathDisplay();
		// 保留起终点高亮，清除其他拾取类型（原 drawTempLineAndHighlight 逻辑迁移；Q6=A 不再写 MeshElement 容器）
		void keepEndPointHighlights();
		// 依据 _paths 整体刷新列表控件（行号即路径下标；tooltip 为节点真实 ID 序列）；modeldata 供 ID 映射
		void fillPathList(mPostModelData* modeldata);
		// 清空路径结果：_paths/当前序号/列表/显隐指纹
		void clearPathResults();
		// R6 惰性显隐指纹重检：不一致则按当前显隐自动重算并整体刷新（FR-07 口径）
		void checkVisibilityFingerprint();
		// 计算显隐指纹：模型指针 + Σ(部件可见位 + 隐藏单元计数)；失败（数据不可用）返回 false
		// 注意复杂度为 O(总单元数)：getMaskEleNum 逐 cell_info 扫描，仅低频重检时机调用（评审 C3）
		bool computeVisibilityFingerprint(qint64& fp) const;

		// 在浮动窗口中打开曲线视图
		void openPlotInFloatWindow(const QString& vname, PlotView::mPlotView* plotView);
		// 缓存曲线数据
		void cachePlotData(const QString& vname, const PlotDataCache& cache);

	private:
		// 当前后处理数据库
		QPointer<mPostDataBase> _db = nullptr;
		// 当前后处理渲染窗口(视口删除时随QObject析构自动置空)
		QPointer<mxr::REBUILD_PostRend> _rWidget = nullptr;

		// 主布局
		QVBoxLayout* _mainLayout = nullptr;
		// “结果路径”折叠分区
		FoldSection* _numandpos = nullptr;
		// 起始点/终止点拾取面板的容器
		QWidget* _pointContainer = nullptr;
		QWidget* _pointContainer2 = nullptr;

		// 起始点和终止点拾取面板
		PosPickWidget1 _startPointPanel;
		PosPickWidget1 _endPointPanel;
		// “路径结果”按钮：用于绘制连线并高亮
		QPushButton* _btnPathResult = nullptr;
		// “曲线生成”按钮：预留扩展按钮
		QPushButton* _btnCurveGeneration = nullptr;

		// 起始点/终止点拾取信号连接
		QMetaObject::Connection _startPointPickConn;
		QMetaObject::Connection _endPointPickConn;
		// 是否处于起始点/终止点拾取状态
		bool _startPointSelecting = false;
		bool _endPointSelecting = false;

		// 起始点和终止点的节点 ID（-1 表示未选择）
		int _startNodeID = -1;
		int _endNodeID = -1;
		// 起始点和终止点在当前帧下的变形后坐标
		QVector3D _startNodePos;
		QVector3D _endNodePos;

		// ---------------- 拓扑最短路径（FR-03～FR-08） ----------------
		// 汇总行：未计算/共 N 条/超上限提示（02 §3.1）
		QLabel* _summaryLabel = nullptr;
		// 最短路径列表：序号|节点数|路径长度，单选、不可编辑、无条目禁用；行号即 _paths 下标
		QTableWidget* _pathList = nullptr;
		// 本次计算的等长最短路径集合（节点为连续 ID，绘制时经 _nodedata.ids 映射真实 ID）
		std::vector<PostWindow::PathEntry> _paths;
		// 当前高亮的路径下标（-1 表示无）
		int _currentPathIndex = -1;
		// 视口帧/变形系数变化信号连接（FR-08，仿 mPostMeasureTool 挂接；clearResult 断开）
		QMetaObject::Connection _frameConn;
		QMetaObject::Connection _deformConn;
		// R6 惰性显隐指纹缓存：求解成功时记录，低频时机重检（false 表示无有效基准）
		qint64 _fingerValue = 0;
		bool _fingerValid = false;
		// 上次 frameChanged 时刻（ms）：间隔 <250ms 视为动画播放中，跳过指纹重检（评审 C3 写死策略）
		qint64 _lastFrameChangeMs = 0;
		// 求解排队防重入标记：点击或自动重算已入队 singleShot 时置位，solveAndShowPath 开始时复位
		bool _solvePending = false;

		// 曲线视图相关
		QString _nodeCurveViewName;                                           // 节点 ID-X 曲线视图名
        QHash<QString, QPointer<mPostPlotFloatWindow>> _plotFloatWindows;     // 浮动窗口映射
		QHash<QString, PlotDataCache> _plotDataCache;                         // 每个视图的数据缓存
	};
}
