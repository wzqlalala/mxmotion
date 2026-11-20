#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QLineEdit>
#include <QHash>

class QTimer;

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"

//共享 UI 库控件（实现见 mPostIsosurface.cpp；V1.3 原型 #isosurfaceParameters）
namespace MxFunctions
{
	class mPrimaryButton;
	class mChoiceGroup;
	class mRangeSliderRow;
	class mStepAdjustRow;
	class mSlider;
}

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace mxr
{
	class REBUILD_PostRend;
}

namespace PostData
{
	class mPostGuiData;
}

namespace MechPreData
{
	class mViewportPrivate;
}

namespace PostWindow
{
	/*
	* 等值面参数面板（对齐 V1.3 原型 #isosurfaceParameters"等值面图"，单阈值滑块式，
	* 控件收敛共享 UI 库 MxFunctions，差异表见 doc/等值面共享控件替换差异表-20261114.md）：
	* 操作条（复用云图设置/清除等值面/生成等值面=库 mPrimaryButton 恒主色）
	* + 共享折叠组（云图变量/对象及坐标系/数据处理——自云图面板物理接管，复刻原型共享参数块，
	*   变量切换直接走云图既有管线，面板隐藏时归还）
	* + 折叠组"等值面样式"（组头恒显，字段行按生成态显隐——原型 isosurfaceGeneratedFields；
	*   范围滑块行=库 mRangeSliderRow/当前阈值/阈值调整=库 mStepAdjustRow（步进为实际值
	*   增量，±触界禁用）/生成形式=库 mChoiceGroup 共框单选）。
	* 数据：生成态/阈值（实际值）/生成形式/步进值均为面板进程内状态（_uiStates，按视口名）。
	* 阈值归一化基准值域随生成时刻锚定：纯切帧保持不动（HyperView 语义），切变量/复用
	* 云图设置时按当前图例值域重锚（见 updateIsoBounds）。
	* 派生去节点化（2026-11-04）后管理器不再登记 IsosurfaceInfo 条目，addIsosurfaceData
	* 对无条目视口是静默空操作，guidata 通道不可作数据源（阈值原存 _contourViews[0] 的
	* 链路实际从未落盘，面板重开即丢——改存面板态后跨面板切换/重开均可恢复）。
	* 等值面渲染随面板切换保持（用户定案 2026-11-05）：清除唯一入口是"清除等值面"按钮。
	*/
	class MPOSTWINDOW_EXPORT mPostIsosurfacePanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostIsosurfacePanel(QWidget* parent = nullptr);

		void newIsosurfacePanel(int caseid);

		//通过视口名截取云图视口
		QString getCloudViewName(QString view);

		//设置当前视图
		void setCurViewName(const QString& viewName);

		/** 动画生成后等值面自动恢复（HyperView 语义：等值面随动画逐帧提取）。
		* 动画生成链 setRendCurrentFrameData 整体重建渲染器会销毁源视口等值面；
		* 该视口已生成过（_uiStates 生成态）时按存储阈值/生成形式立即重建，
		* contour 广播把等值面落到静态帧与各动画帧；未生成过则无操作 */
		void restoreIsosurfaceIfGenerated(const QString& viewName);

	private slots:
		/** 复用云图设置：按当前云图图例值域重锚基准并重生成（共享折叠组本就显示云图当前值） */
		void slot_reuseContourSettings();

		/** 清除等值面 */
		void slot_clearIsosurface();

		/** 生成等值面 */
		void slot_generateIsosurface();

		/** 滑块拖动：同步阈值并实时重生成 */
		void slot_thresholdSliderMoved(int value);

		/** 阈值输入变化：同步滑块并实时重生成 */
		void slot_thresholdEdited();

		/** 阈值步进减 */
		void slot_decreaseThreshold();

		/** 阈值步进加 */
		void slot_increaseThreshold();

		/** 生成形式切换（小于/等于/大于；已生成时实时按新模式重生成） */
		void slot_comparisonChanged();

		/** 云图重绘完成：重绘本面板绑定视口且已生成等值面时，按存储实际阈值自动重提取
		*（HyperView 语义：纯切帧时归一化基准保持——阈值/滑条/范围栏不动，同一实际值
		* 逐帧提取；切变量时基准重锚新变量值域）；未生成时仅维持现状 */
		void slot_cloudRedrawn(const QString& viewName);

	signals:

	protected:
		void showEvent(QShowEvent* event) override;

		void hideEvent(QHideEvent* event) override;

	private:
		//进程内界面状态（生成态/阈值实际值/生成形式/步进值），按视口名
		struct IsoUIState
		{
			QString comparison{ "等于" };
			double step{ 0.01 };
			bool generated{ false };		//该视口是否已生成等值面（清除按钮/重开面板的恢复判据）
			double threshold{ 0.5 };		//当前阈值（实际值，重开面板恢复用）
			double thresholdNorm{ 0.5 };	//阈值归一化位置（生成时随实际值记录；切变量重锚值域时按此重定位，避免旧值钳到新值域端点空提取）
			int savedShowMode{ -1 };		//生成等值面前绑定视口的显示模式（0-3；清除等值面时恢复依据，-1=未捕获）
			QString basisKey{};				//阈值归一化基准对应的云图变量身份（工况/变量/分量/层/复数，不含帧）
			double basisMin{ 0 };			//归一化基准值域下限（生成/换变量时锚定，纯切帧保持——HyperView 语义）
			double basisMax{ 1 };			//归一化基准值域上限
		};
		IsoUIState& uiState(const QString& viewName);

		//按绑定视口名解析渲染部件/视口数据（激活视口≠绑定视口时仍作用到绑定视口；空名/未命中返回 nullptr）
		REBUILD_PostRend* boundPostRend() const;
		MechPreData::mViewportPrivate* boundViewportPrivate() const;

		//恢复绑定视口的用户显示模式（优先生成时捕获的 savedShowMode，未捕获回退
		//绑定视口当前勾选态；清除等值面/hideEvent 归还时用）
		void restoreBoundShowMode(mxr::REBUILD_PostRender* postRender, const IsoUIState& state);

		//拖动/输入类实时重生成的去抖入口（QTimer 40ms 尾沿；sliderReleased/按钮类入口直接生成）
		void scheduleRegen();

		//接管云图面板共享折叠组（云图变量/对象及坐标系/数据处理）：
		//复刻原型共享参数块的物理移动——等值面面板激活时移入本面板布局（操作条与等值面样式之间）
		void takeSharedSections();

		//归还共享折叠组给云图面板（hideEvent/切回云图时）
		void returnSharedSections();

		/** 阈值归一化基准值域（仅内部值 _rangeMin/_rangeMax，界面回填由 fillIsoBoundsView 执行）。
		* 已生成且云图变量身份未变（纯切帧）时恢复面板存储的基准——阈值（实际值）/
		* 滑条/范围栏随切帧保持不动，同一实际值逐帧提取（HyperView 语义；其图例值域跨帧稳定，
		* 本云图图例逐帧变化，以生成时刻锚定近似该稳定性）；否则（首次/切换变量/forceCapture）
		* 按渲染端图例"当前显示帧"的值域重新锚定（动画停靠帧/静态帧都恒等于用户所见，
		* guidata 最值动画后停留在动画前静态帧，仅作回退） */
		void updateIsoBounds(bool forceCapture = false);

		//范围栏回填/清空（用户逻辑：点击"生成等值面"后范围栏才有内容）
		void fillIsoBoundsView();
		void clearIsoBoundsView();

		//等值面样式字段行显隐（初始隐藏，生成成功后显示；原型组头恒在仅字段区显隐，
		//行级 takeRow/insertRow 同步折叠组高度帽）后显式触发一次宿主 resize
		void setStyleRowsVisible(bool show);

		//±步进按钮触界禁用刷新（原型 updateIsosurfaceGeneration：未生成双禁；
		//阈值到值域端点禁对应侧——按滑条归一化位置判定，不受文本舍入影响）
		void updateAdjustButtons();

		//宿主滚动范围同步（面板最小尺寸变化后显式触发一次宿主 resize，见实现注释）
		void nudgeHostResize();

		//阈值框实际值解析（越界钳到值域内）与回填（文本+滑条归一化位置同步）
		double thresholdActual() const;
		void setThresholdActual(double actual);

		//按当前阈值生成/重建等值面（值变化实时调用；reanchorBasis=先按当前云图图例重锚基准值域）
		void generateIsosurface(bool reanchorBasis = false);

		//应用生成形式 radio 到界面状态
		void applyComparisonFromUI();


	QVBoxLayout*							_mainLayout;
	bool									_sectionsTaken{ false };	//共享折叠组接管状态
	FoldSection*							_styleFold{};				//等值面样式折叠组（组头恒显）

		//--------- 等值面样式（库 MxFunctions 控件） ---------
	MxFunctions::mRangeSliderRow*			_rangeRow{};				//范围行=共框[只读下限|滑块|只读上限]
	MxFunctions::mSlider*					_sliderThreshold{};			//范围行内滑块（0-1000=归一化位置）
	QLineEdit*								_editThreshold{};			//当前阈值（实际值，与范围栏同单位；滑条为其归一化位置）
	MxFunctions::mStepAdjustRow*			_adjustRow{};				//阈值调整行=共框[‹|步长|›]（步长=实际值增量）
	QLineEdit*								_editStep{};					//步长输入（_adjustRow 内件裸出）
	MxFunctions::mChoiceGroup*				_choiceComparison{};		//生成形式：小于(0)/等于(1)/大于(2) 共框单选

		//--------- 操作条 ---------
	QPushButton*							_btnReuse{};
	QPushButton*							_btnClear{};
	MxFunctions::mPrimaryButton*			_btnGenerate{};				//生成等值面（原型 symmetry-execute 恒主色）

		double									_rangeMin{ 0 };
		double									_rangeMax{ 1 };
		bool									_hasGenerated{ false };
		bool									_isSyncing{ false };		//滑块/阈值互同步防递归
		QTimer*									_regenTimer{};				//实时重生成去抖定时器（单次 40ms）
		QHash<QString, IsoUIState>				_uiStates;
	};
}
