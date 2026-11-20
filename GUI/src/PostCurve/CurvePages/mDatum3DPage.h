#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图工具页：基准（基准平面管理，R37 建占位、R41 翻活，T41-3）
//
// Ribbon"3D曲线"页"基准"按钮挂载的参数栏工具页（非节点绑定工具页机制不变：
// 不继承 mCurvePropPageBase、不进 mCurvePropPageManager 注册表、宿主
// mPostMenuContributor 挂载链零改动——懒创建 QPointer 缓存复用原样）。
//
// 行集对齐设计稿 design/3d-curve/ref-3dcurve-datum-page.png 三组：
//   ▾基准管理：☑显示基准(图表级总开关 kDatum3DVisible，走命令栈)+名称框
//     (当前选中基准本名，编辑失焦且变更提交 kName)同行/勾选列表(本图表全部
//     DatumPlane3D 子节点树序，条目勾选=该基准 kVisible 即时单键提交，
//     Qt::UserRole 携带节点 id)/[新增][复制][删除] 三钮：
//     新增=addDatumPlane3D 宏+列表选中新项+属性行回显；
//     复制=宏"复制基准面"(新节点+逐键克隆当前选中基准全部属性、名不克隆
//     续计数、插后选中)；删除=removeNode；按钮启停随选中态/列表空态；
//     图表删除兜底=列表清空全钮禁用不崩溃
//   ▾位置与属性：类型(单选项"参考平面"下拉，可用但变更零提交——无第二类型)/
//     法向方向(X轴/Y轴/Z轴→kNormalAxis)/X·Y·Z坐标(双精度 spin→kPosX/kPosY/
//     kPosZ)/移动增量(0.1)+横向滑条/文字(单行编辑，失焦且变更才一次提交
//     →kText，对齐注释页纪律)/标签位置(9 宫格下拉→kLabelPos，默认中心)/
//     基准颜色(mCurveColorButton→kColor)/不透明度(0..1→kDatumOpacity，默认
//     0.25)/☐反向裁剪=占位。
//     占位纪律（R31/R37 口径逐条）：移动增量值框+滑条、反向裁剪勾选、创建
//     截面曲线组四控件=构造即禁用、不连 commit/信号、不进回填
//   ▾创建截面曲线：曲线横坐标(X)/曲线纵坐标(Z)/创建窗口(当前窗口)/
//     [创建截面曲线]钮，全占位
//
// 上下文维护（mCurveStyle3DPage 同构）：构造函数全局监听控制器三信号
// （sig_nodePropertyChanged/nodeAdded/nodeRemoved——本图表后代增删/属性变化
// 即幂等刷新，图表删除→列表清空全钮禁用）；setChart(chartNodeId) 绑定当前
// 三维图。宿主挂载链零改动口径下 setChart 由页面自身调用：构造时与 showEvent
// （挂载=show）各以工厂 currentChartId() 刷新一次，另订阅工厂
// sig_currentChartChanged 跟随三维图切换（仅有效三维图重绑，-1/非三维图保持
// 现绑定不冲空）——等效于 mCurveStyle3DPage 的"挂载前 setChart"。
// 回显全程 QSignalBlocker（R30 教训）；_syncing 双保险防回环。
//
// 渲染触发零额外代码（属 T41-2 键已在 m3DChartView 白名单）；本页不直接触
// 渲染。窄宽自适配 B01/B02 纪律同 mWaterfall3DPage（按钮 setMinimumWidth(1)
// +紧凑内边距，整页 minimumSizeHint 宽 ≈142px，窄 dock 无裁切）。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "../CurveWidgets/mCurveFoldSection.h"

//QT
#include <QWidget>
#include <QList>
#include <QListWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QSlider;

namespace PostCurve
{
	class mCurveColorButton;   //实心色块按钮（CurveWidgets，bar 形态=通栏色条）

	class POSTCURVE_EXPORT mDatum3DPage : public QWidget
	{
		Q_OBJECT
	public:
		explicit mDatum3DPage(QWidget* parent = nullptr);

		//绑定当前三维图（构造/showEvent 以工厂 currentChartId() 自调，语义同
		//mCurveStyle3DPage 挂载前刷新上下文；仅有效三维图（chartType=5）重绑，
		//-1/非三维图保持现绑定——图表删除清空走控制器信号兜底）
		void setChart(int chartNodeId);
		int chartId() const { return _chartId; }

	protected:
		//挂载即刷新上下文（宿主 setMainWidget→show；幂等）
		void showEvent(QShowEvent* event) override;

	private:
		void buildUi();
		//按 _chartId 现状整体刷新（显示基准回显+列表+按钮启停+属性行回显；
		//幂等，可重入）
		void refreshAll();
		//基准列表重建（树序全部 DatumPlane3D 子节点，勾选态=kVisible；全程
		//阻塞信号；尽量保持当前选中基准不变）
		void refreshList();
		//位置与属性回显+启停（按列表当前选中基准 nodeProperty 先查；未选中/
		//图表无效整组禁用；占位行不在回填路径）
		void refreshProps();
		//列表当前选中基准节点 id（无选中/越界返回 -1）
		int currentDatumId() const;
		//对当前选中基准单键提交（未选中/_syncing 期间静默忽略）
		void commitCurrent(const QString& prop, const QVariant& value);
		//新增（addDatumPlane3D 宏+选中新项+属性行回显）
		void addDatum();
		//复制（宏"复制基准面"：新节点+逐键克隆全部属性、名续计数，插后选中）
		void copyCurrent();
		//删除当前选中基准（removeNode，命令栈可撤销）
		void deleteCurrent();

		int _chartId = -1;           //绑定的三维图节点 id（-1=未绑定/图表已删）
		bool _syncing = false;       //回填标志：refreshAll 期间屏蔽提交（防回环）

		mCurveFoldSection* _manageFold = nullptr;    //▾基准管理
		QCheckBox* _showCheck = nullptr;             //显示基准（图表级 kDatum3DVisible）
		QLineEdit* _nameEdit = nullptr;              //名称框（当前选中基准本名，kName）
		QListWidget* _datumList = nullptr;           //基准列表（勾选=kVisible）
		QList<int> _datumIds;                        //列表条目基准 id（树序，与条目一一对应）
		QPushButton* _addBtn = nullptr;              //新增
		QPushButton* _copyBtn = nullptr;             //复制
		QPushButton* _delBtn = nullptr;              //删除

		mCurveFoldSection* _propFold = nullptr;      //▾位置与属性
		QComboBox* _typeCombo = nullptr;             //类型（单选项"参考平面"，可用零提交）
		QComboBox* _normalCombo = nullptr;           //法向方向（kNormalAxis 0=X/1=Y/2=Z）
		QDoubleSpinBox* _posXSpin = nullptr;         //X坐标（kPosX）
		QDoubleSpinBox* _posYSpin = nullptr;         //Y坐标（kPosY）
		QDoubleSpinBox* _posZSpin = nullptr;         //Z坐标（kPosZ）
		QDoubleSpinBox* _stepSpin = nullptr;         //移动增量（占位禁用）
		QSlider* _stepSlider = nullptr;              //移动增量横向滑条（占位禁用）
		QLineEdit* _textEdit = nullptr;              //文字（kText，失焦且变更提交）
		QComboBox* _labelPosCombo = nullptr;         //标签位置（kLabelPos 9 宫格，默认中心）
		mCurveColorButton* _colorBtn = nullptr;      //基准颜色（kColor）
		QDoubleSpinBox* _opacitySpin = nullptr;      //不透明度（kDatumOpacity 0..1 默认 0.25）
		QCheckBox* _reverseClipCheck = nullptr;      //反向裁剪（占位禁用）

		mCurveFoldSection* _createFold = nullptr;    //▾创建截面曲线（整组占位禁用）
	};
}
