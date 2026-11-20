#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图工具页：数值（寻点，R37 建；R47/T47-1 功能接入）
//
// Ribbon"3D曲线"页"数值"按钮挂载的参数栏工具页（非节点绑定页，mCurveStyle3DPage
// 同族：不继承 mCurvePropPageBase、不进注册表；宿主挂载链零改动——页面自身经
// 工厂 currentChartId 在构造/showEvent 刷新 + sig_currentChartChanged 跟随，
// mDatum3DPage 同款）。行集对齐设计稿 design/3d-curve/ref-3dcurve-values-page.png
// 两组：
//   ▾寻点与坐标：曲线下拉（当前激活三维图全部 Curve 子节点树序，R23 重名允许，
//     itemData=节点 id 按 id 键定；无激活三维图/无曲线=下拉空+整页禁用）+
//     极值/X轴/Y轴三列导航按钮（对齐 HyperGraph"完整语义"，20261117 用户拍板：
//     首行 ◄►=各列步进序前后步进[极值=极值点集切换——局部峰∪谷间跳转，T47-2
//     20261117 对齐修正/X=曲线类型按序号·其余类型升序稳定序，T47-3/Y 同]，
//     止于端点不环绕；∩ 行[极值 ∩ 全局
//     最高 | X ◄∩ ►∩ 局部峰导航 | Y ◄∩ ►∩]；U 行[极值 U 全局最低 | X ◄U ►U
//     局部谷导航 | Y ◄U ►U]；列启停按类型分流——曲线类型（plot3dType==1 三维
//     曲线）Y 列固定整列灰+其余列恒值兜底，灰含 ∩/U 组，20261117 用户拍板）
//     ——算法口径见 CurveCore/mPointSeekCore（单一实现）。
//   ▾坐标数据表：选中曲线全部点四列只读表[序号/横坐标/纵坐标/高度，'f',4；
//     行序=下标序（网格类 rows*cols==N 行主序摊平口径，与定义曲线弹窗数值
//     网格态一致）]；导出表格钮**T47-4 接线**（选中曲线且数据非空启用——
//     保存框默认名=曲线名.csv 非法字符清洗，取消无副作用；CSV=表头
//     "横坐标,纵坐标,高度"+三列数据无序号列，'f',4 同表格显示/非有限空串/
//     UTF-8 BOM+CRLF，装配=mPointSeekCore::exportTableCsv 单一实现，写出
//     对齐 mDynStiffnessMapping 先例，失败警告框不崩溃）；其余三钮（添加行/
//     删除末行/清空）保持置灰不动（20261117 用户拍板：行编辑留待后续单）。
//     寻点滚动联动（T47-4）：任一寻点动作更新当前点后 scrollToItem 当前行
//     （EnsureVisible 最小滚动）；刷新/换曲线无当前点滚回顶部；高亮机制
//     不变。
//
// 寻点动作收口 seekTo：更新当前点 {curveId, pointIndex} → 工厂
// setPointSeekMarker 转发画布橙点标记（视图态：不进命令栈、不写文档、不序列
// 化、不触发 rebuild）→ 表格当前行加粗+主题蓝高亮（QTableWidgetItem 字体/
// 前景实现，不用 selectRow 以免引入编辑语义）。换曲线=标记与高亮清除；
// 文档侧数据改写（数值页编辑提交/定义曲线应用等）后按 id 重取数据，当前
// 下标越界或该点非 finite → 标记与高亮清除；图表切换=下拉/表格刷新（工厂
// 全局仅一个标记，新图寻点时旧图标记被工厂状态替换清除）。
//
// 上下文维护：构造函数全局监听控制器 sig_nodePropertyChanged/nodeAdded/
// nodeRemoved（mCurveStyle3DPage 先例）+ 工厂 sig_currentChartChanged；构造
// 期整页置灰（R31 占位纪律沿用），选中曲线后按规则启停。页面由
// mPostMenuContributor 懒创建缓存复用（QPointer 保活）。
// =============================================================================
#include "../PostCurveGlobal.h"
#include "../CurveWidgets/mCurveFoldSection.h"   //折叠分组（CommonWidgets FoldSection 共享实现的模块别名，不可前向声明）

//QT
#include <QWidget>
#include <QList>

class QComboBox;
class QTableWidget;
class QToolButton;

namespace PostCurve
{
	class POSTCURVE_EXPORT mPointSeek3DPage : public QWidget
	{
		Q_OBJECT
	public:
		explicit mPointSeek3DPage(QWidget* parent = nullptr);

	private:
		void buildUi();
		//绑定当前激活三维图（构造/showEvent 以工厂 currentChartId() 自调，
		//mDatum3DPage 同款；sig_currentChartChanged 跟随为主路径）
		void setChart(int chartId);
		void showEvent(QShowEvent* event) override;
		//按 _chartId 现状整体刷新（下拉重建按 id 保选择/表格重填/按钮启停/
		//当前点有效性复查；幂等可重入，回填全程阻塞信号防回环）
		void refreshAll();
		//曲线下拉重建（当前图表 Curve 子节点树序，itemData=节点 id；保持当前
		//选择按 id 重定位，落空自动选首条/清空）
		void refreshCombo();
		//坐标数据表重填（选中曲线全部点四列 'f',4 只读；行序=下标序；无当前
		//点滚回顶部——T47-4 滚动联动）
		void refreshTable();
		//三列导航按钮启停（选中曲线按 mPointSeekCore::columnEnabled：曲线类型
		//plot3dType==1 三维曲线 Y 列固定整列灰，其余列恒值兜底——20261117
		//用户拍板；灰含该列 ∩/U 组；未选中/无数据全灰）
		void refreshButtons();
		//表格高亮行重摆（当前点行加粗+主题蓝前景；pointIndex<0 全表去高亮）
		void applyHighlight(int pointIndex);
	//寻点动作统一收口（导航按钮/失效清除共用）：更新当前点 → 工厂
	//setPointSeekMarker 转发画布标记 → 表格高亮+滚动联动（T47-4：
	//scrollToItem 当前行 EnsureVisible）；pointIndex<0 = 清除
	void seekTo(int pointIndex);
	//表格滚动到当前寻点行（T47-4；行号越界安全——无当前点/空表不滚动）
	void scrollToCurrentRow();
		//当前点有效性复查（文档侧数据改写后）：曲线已删/下标越界/该点非
		//finite → 标记与高亮清除（refreshAll 尾调用）
		void revalidateCurrentPoint();
		//文档信号 → 本页刷新判定（本图表节点或其后代；删除通知节点已不可查
		//按潜在本图表事件接受——refreshAll 幂等）
		void onDocNodeChanged(int nodeId);

		int _chartId = -1;        //绑定的三维图节点 id（-1=无激活三维图/已删）
		int _curveId = -1;        //当前选中曲线节点 id（-1=未选中）
		int _currentPoint = -1;   //当前寻点下标（-1=无；表格行=下标一一对应）
		bool _syncing = false;    //回填标志：refreshAll 期间屏蔽提交/联动（防回环）
		QList<int> _comboIds;     //下拉条目曲线 id（树序，与条目下标一一对应）

		mCurveFoldSection* _seekFold = nullptr;    //▾寻点与坐标
		QComboBox* _curveCombo = nullptr;          //曲线下拉（激活三维图全部 Curve 子节点）
		QList<QToolButton*> _stepBtns;             //首行 6 步进钮 [极值◄ 极值► X◄ X► Y◄ Y►]
		QToolButton* _peakBtn = nullptr;           //极值 ∩（全局最高点）
		QToolButton* _valleyBtn = nullptr;         //极值 U（全局最低点）
		QList<QToolButton*> _peakStepBtns;         //[X◄∩ X►∩ Y◄∩ Y►∩]（X/Y 局部峰导航）
		QList<QToolButton*> _valleyStepBtns;       //[X◄U X►U Y◄U Y►U]（X/Y 局部谷导航）
		QPushButton* _exportBtn = nullptr;         //导出表格钮（T47-4 接线：选中曲线且数据非空启用）
		mCurveFoldSection* _tableFold = nullptr;   //▾坐标数据表
		QTableWidget* _table = nullptr;            //四列只读数据表（序号/横坐标/纵坐标/高度）
	};
}
