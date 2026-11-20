#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 编辑曲线浮动窗口（功能弹窗，单例）
//
// 入口：Ribbon"曲线"页 ->"编辑曲线"（checkable 模式开关）。开启后由
// 视口工厂锁定"激活视窗 + 目标曲线集（默认其第一条曲线，可经曲线选取器
// 多选）"为会话目标（sig_editCurveStarted），本窗口随之弹出在目标视窗
// 右上角内侧；关闭（✕）即退出编辑模式。
//
// 内容：
//   - 目标曲线（折叠组）：曲线名回显（只读）+ 曲线选择按钮（弹 Qt::Popup
//     列出本图表曲线，本期单选；设计图预留跨视窗勾选树，待扩展）；
//   - 控制（折叠组）：方向（水平=竖直裁剪线沿X截取 / 垂直=水平裁剪线沿Y
//     截取）、起始/终止（无步进按钮数字框 + |< < > >| 四枚步进小按钮：
//     跳到数据端 / 按步长移动）、步长；
//   - 底部按钮：裁剪（保留区间内数据，范围外落盘删除）/删除（删除区间内
//     数据点，与裁剪互逆）/隐藏（区间内数据段隐藏为断口，点保留在模型
//     hiddenRanges）——三者均 setSeriesData/直写不进命令栈（与 pch 导入
//     一致），执行前把目标曲线的当前状态压入其撤回历史栈；撤回按曲线
//     逐级弹出快照恢复（可连续点击退回未编辑状态，栈空复灰），历史随
//     应用级单例跨编辑会话保留——重开面板仍可撤回该曲线之前的操作。
//     参数栏 x/y 数据页"更改…"（更改数据源）同为数据直写，共用本撤回栈
//     （经公开的 snapshotCurve 压栈，见下）。
//
// 状态与联动：会话范围/方向/步长只存视口工厂（单一数据源），本窗口与
// 目标视窗（裁剪线拖拽）都经工厂信号同步（sig_editCurveRangeChanged），
// 输入侧改动经工厂 API 写回，避免双写（_syncing 防程序回填触发写回）。
//
// 风格照 mPropertyEditorPopup：无边框白卡片 + M徽标标题栏 + ✕ + QSizeGrip，
// DlgGeom::bind(SizeOnly) 记宽高（位置每次会话落到目标视窗旁，不持久化）。
// =============================================================================

#include "PostCurveGlobal.h"

//QT
#include <QWidget>
#include <QHash>
#include <QPoint>
#include <QPointer>
#include <QVector>

class QComboBox;
class QDoubleSpinBox;
class QMouseEvent;
class QPushButton;
class QResizeEvent;
class QSizeGrip;
class QTextEdit;
class QToolButton;

namespace PostCurve
{
	class POSTCURVE_EXPORT mEditCurveDlg : public QWidget
	{
		Q_OBJECT
	public:
		static mEditCurveDlg* getInstance();

		mEditCurveDlg(const mEditCurveDlg&) = delete;
		mEditCurveDlg& operator=(const mEditCurveDlg&) = delete;

		//数据破坏性操作前压栈：对单条曲线当前 数据/隐藏区间/来源五元组 做快照，
		//作为单曲线操作组压入操作历史。编辑曲线"裁剪/删除/隐藏"与参数栏
		//x/y 数据页"更改…"（更改数据源）经此共用一套撤回历史，"撤回"逐级
		//恢复。曲线不存在时安全返回
		void snapshotCurve(int curveId);

	protected:
		//标题栏按下记录按下点 / 移动超阈值进入拖动（双击不挪窗，仅用户拖动改位置）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		//右下角缩放手柄贴角跟随（手柄不占布局行，对齐 mApplyStyleDlg）
		void resizeEvent(QResizeEvent* event) override;
		//曲线选取器标题栏拖动（选取器为独立无框窗口，事件过滤实现拖动）
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		//工厂会话信号 -> 回填/显隐（非会话目标变化全部由工厂保证只发必要信号）
		void onEditStarted(int chartId);
		void onEditTargetChanged();
		void onEditRangeChanged(double start, double end);
		void onEditDirectionChanged(int direction);
		void onEditStopped();

	private:
		explicit mEditCurveDlg(QWidget* parent);
		~mEditCurveDlg() override;

		void buildUi();
		//内容区两个折叠分组（目标曲线/控制）
		QWidget* buildTargetSection();
		QWidget* buildControlSection();
		//一行"数字框 + 四枚步进小按钮"的组合编辑器（bound：0起始 1终止）
		QWidget* buildBoundEditor(int bound);
		//按工厂会话状态整体回填（目标曲线名/方向/范围/步长）
		void refillFromSession();
		//弹出曲线选择（Qt::Popup，本图表曲线单选；确定 -> 工厂换目标）
		void showCurvePicker();
		//"裁剪"：范围外数据落盘删除（渲染空间范围反变换回原始空间过滤）
		void applyTruncate();
		//"删除"：删除起始~终止区间内数据点（与裁剪互逆，同为数据直写）
		void applyDelete();
		//"隐藏"：起始~终止区间内数据段隐藏为断口（点保留在模型 hiddenRanges）
		void applyHide();
		//"撤回"：弹出最近一次操作组，整组恢复各目标曲线操作前状态
		//（可连续点击逐级退回未编辑状态）
		void applyUndo();
		//当前会话范围（渲染空间）反变换回原始数据空间，输出升序 [lo,hi]；
		//非会话态/目标曲线无效时返回 false（裁剪/删除/隐藏共用）
		bool currentRawRange(double& lo, double& hi) const;
		//执行操作前把当前目标集各曲线状态压入操作历史（一次操作一组，多级）
		void takeSnapshot(const QList<int>& curveIds);
		//按操作历史是否为空刷新撤回按钮可用态与 tooltip
		void updateUndoButton();
		//按工厂当前目标集回显目标曲线名（多条以"，"连接）
		void refillTargetNames();

		//撤回快照组：数据体量大不进命令栈，由本窗口自管。一次操作对当时的
		//全部目标曲线各存一份操作前状态（一次撤回整组恢复）；历史为时序栈，
		//弹窗为应用级单例故跨编辑会话保留——重开面板仍可逐级撤回；深度上限
		//防内存失控
		struct CurveState
		{
			QVector<double> xData;             //操作前 X 数据
			QVector<double> yData;             //操作前 Y 数据
			QVector<double> hiddenRanges;      //操作前隐藏区间（[轴,lo,hi]*）
			//操作前复数曲线下面板数据（非复数曲线为空）与展开方式：复数曲线
			//裁剪/删除按索引同步两路，撤回一并还原
			QVector<double> complexYData;
			int complexFormat = 0;
			//操作前来源五元组："更改…"（更改数据源）入栈时一并快照，撤回时
			//同步还原，避免数据与来源回显脱节；裁剪/删除/隐藏的快照里存的
			//即当前值（撤回时等值写回，无害）
			QString sourceFile;
			QString subcase;
			QString schema;
			QString object;
			QString component;
			//操作前 x/y 侧独立来源（参数栏 x/y 数据页侧换源入栈一并快照；空 =
			//横坐标/五元组经典形态，等值语义随上方字段一并还原）
			QString xSideSubcase;
			QString xSideSchema;
			QString xSideObject;
			QString xSideComponent;
			QString ySideSubcase;
			QString ySideSchema;
			QString ySideObject;
			QString ySideComponent;
			//操作前复数下面板侧来源（complexSide*，复数曲线成对换源入栈一并快照）
			QString complexSideSubcase;
			QString complexSideSchema;
			QString complexSideObject;
			QString complexSideComponent;
		};
		struct UndoGroup
		{
			QHash<int, CurveState> curves;     //曲线id -> 操作前状态
		};

		static const int kMaxUndoDepth = 50;   //撤回深度上限（组数）

		static mEditCurveDlg* _instance;   //单例

		bool _syncing = false;             //程序回填中：输入侧信号不写回工厂
		QVector<UndoGroup> _opHistory;     //操作历史（撤回弹出末组）

		QWidget* _titleBar = nullptr;      //标题栏（拖动热区）
		QSizeGrip* _grip = nullptr;        //右下角缩放手柄（覆盖贴角，不占布局行）
		QPoint _dragOffset;                //拖动起始偏移
		QPoint _pressPos;                  //标题栏按下点（拖动阈值起算，防双击微动挪窗）
		bool _dragging = false;            //拖动中标志

		QTextEdit* _curveEdit = nullptr;      //目标曲线名回显（多行只读，长名换行）
		QToolButton* _curvePickBtn = nullptr; //曲线选择按钮（框内右下角，弹选取器）

		//曲线选取器（设计图：工程->页->图表->曲线 层级树 + 取消/确定）；
		//存活期内复用同一实例（重复点按钮前置），WA_DeleteOnClose 自清
		QPointer<QWidget> _pickerDlg;
		QPointer<QWidget> _pickerTitleBar;    //选取器标题栏（拖动热区，事件过滤）
		bool _pickerDragging = false;         //选取器拖动中
		QPoint _pickerDragOffset;             //拖动起始偏移
		QPoint _pickerPressPos;               //选取器标题栏按下点（拖动阈值起算）
		QComboBox* _directionCombo = nullptr; //方向：0水平 1垂直
		QDoubleSpinBox* _startSpin = nullptr; //起始（渲染空间）
		QDoubleSpinBox* _endSpin = nullptr;   //终止（渲染空间）
		QDoubleSpinBox* _stepSpin = nullptr;  //步长
		QPushButton* _undoBtn = nullptr;      //撤回（无快照时不可用）
	};
}
