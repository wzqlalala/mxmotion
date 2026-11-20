#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// =============================================================================
// mPostNVHContributionPanel —— NVH工具集面板（独立窗口页，自管理单例）
//
// 模态贡献量分析（Nastran PFMODE pch）前端：QTabWidget 三页签（加载结果/结果
// 显示/结果研究），入口="曲线"Ribbon 页-诊断组-模态贡献量分析按钮（总线信号→
// mPostInfoHub::createModalContribution → activateDock("NVHToolSet") 唤醒独立
// 窗口页；门控=出现层单层（「曲线」页为曲线工作模式专属页签，可见即可点，
// 2026-11-16 二次裁定）；关页签不销毁，
// 切走再切回状态保留）。
// 数据层：ModalContributionModel（同目录，PFMODE pch 聚合，单测已覆盖）；
// 图表产出经 mCurveDocController 落曲线工作台（openspec §3）。
// 宿主挂载：add-nvh-toolset-window-page（2026-11-16 用户裁定反转 11-13 嵌入参
// 数栏）——NVHToolSet dock 复活：provider 自注册 + 主窗注册 + 预设落底部条
// 「函数与曲线」标签组右侧并排分栏；参数栏挂载链（embed-nvh-toolset-into-info-
// panel）下线。
// 家族基座：继承 mPanelBase（参数栏控件态样式单一来源，构造即套
// applyParamBarStyle，mxParamPanel 标记供切主题重发）——本类 applyTheme
// 覆写额外重发四处 UiPalette 取色 QSS（图标行/滑条/模态列表/右轨），
// 2026-11-16 用户裁定按推荐清单家族对齐（embed 同 change 任务 §5）。
// =============================================================================

#include "mpostwindow_global.h"

#include "mPanelBase.h"                 //MechMainWindow::mPanelBase（参数面板家族基座）

#include <QWidget>

#include <QList>
#include <QSet>
#include <string>

class QTabWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QButtonGroup;
class QSlider;
class QSpinBox;
class QToolButton;
class QWidget;

//NVHDisplayOptions/NVHContextScale 值成员（非指针）需完整类型——直接包含
//状态头（context-display-options-state 结构拆分后两者均为栈值）
#include "mPostNVHDisplayOptions.h"

namespace PostWindow
{
	class ModalContributionModel;

	class MPOSTWINDOW_EXPORT mPostNVHContributionPanel : public MechMainWindow::mPanelBase
	{
		Q_OBJECT

	public:
		//获取单例指针（首次调用时创建；容器形态与曲线树同族，最小宽 265）
		static mPostNVHContributionPanel* getInstance();

		//析构函数
		~mPostNVHContributionPanel() override;

	private:
		//默认构造函数（私有，单例）
		explicit mPostNVHContributionPanel(QWidget* parent = nullptr);

		//单例指针
		static mPostNVHContributionPanel* _instance;

		//四处 UiPalette 取色 QSS 统一重发（图标行/滑条/模态列表/右轨）：构造与
		//applyTheme 覆写共用（切主题后语义槽重取色，2026-11-16 家族对齐轮新增）
		void refreshPaletteStyles();

		//主题/档位重发：基类 applyTheme 非 virtual（mParamPanelBase slots 非虚），
		//按家族元方法解耦手法（ThemeManager mxParamPanel 标记后按名 invokeMethod，
		//实例元对象最派生 slot 胜出）以同名 slot 遮蔽——先调基类版本重发参数栏
		//控件态样式，本面板四处 UiPalette 取色 QSS 随后同步重取色
	public slots:
		void applyTheme();

	private:

		//加载结果页（页签 0）：文件选择/结果选择两可折叠分组 + 显示选项/应用钮
		QWidget* createLoadPage();

		//结果显示页（页签 1，§3）：图表类型三选/特定频率（2D线图下替换为频率
		//范围行）/显示最高贡献量/复数分量/排序依据 + 显示选项/显示钮——控件
		//选项集随图表类型收敛（spec 结果显示页签设置项）
		QWidget* createResultDisplayPage();

		//结果研究页（页签 2，§7）：响应研究组（频率范围回显/计算类型/去除比例/
		//选择操作/显示差值曲线+差值模式下拉）+ 模态列表组（多选+全选/清空/反选
		//工具条）+ 显示选项/显示钮（显示选项复用 §6 对话框，§7.5）
		QWidget* createResearchPage();

		//加载成功后回显文件实际频率范围到研究页范围框（spec：回显文件实际范围
		//如 1 Hz ~ 1000 Hz）
		void refreshResearchFreqRange();

		//研究页"显示"漏斗（§7.2 流程）：部分和=先弹贡献项对话框确认排除集合
		//再产出；贡献量叠加=模态列表多选直接产出
		void onResearchShow();

		//§7.3 部分和图产出（modeRatiosPercent=勾选模态→%To Exclude；excludeOp=
		//页内选择操作 Exclude/Include；左轴 "% Structural Response"，总响应蓝
		//实线恒 100% 平线、部分和红虚线、差值绿线挂 Y2（% of Response 档右轴
		//"% Difference"/Scale same as Response 档 "Difference"），图注粉框
		//"Contributor(s) Excluded …"；返回本次产出 chartId（-1=数据/落点解析失败）
		int producePartialSumChart(const std::map<int, double>& modeRatiosPercent,
			bool excludeOp);

		//§7.4 贡献量叠加图产出（selectedModes=模态列表多选集；总响应+逐选中
		//模态曲线，图例即曲线名 "Mode N : <频率> Hz"；返回本次产出 chartId）
		int produceOverlayChart(const std::vector<int>& selectedModes);

		//加载按钮：浏览选择 pch → ModalContributionModel 装载 → 回填级联选项集
		void slot_loadFile();

		//应用按钮（加载结果页）：覆盖确认（布局=当前激活窗口且激活视口有内容时）
		//→ 图表生成（produceCharts 应用链=只产响应曲线一张，大图01 权威）→
		//自动切"结果显示"页签
		void slot_apply();

		//图表类型三图标钮切换：折叠框标题与行集随类型切换（柱状图/极坐标图/
		//2D线图，单框动态标题）、复数分量/排序依据选项集随类型收敛（spec），
		//特定频率行与频率范围行互斥显隐（2D线图下替换，spec 结果显示页签设置项）
		void slot_chartTypeChanged();

		//当前图表类型（三图标钮互斥组选中档：0 柱状/1 极坐标/2 2D线图；无选中
		//兜底 0 柱状）
		int currentChartType() const;

		//显示按钮（结果显示页）：走产出链显示链（只产当前图表类型一张，大图06
		//交互流程权威；不再顺带产响应曲线）
		void slot_show();

		QTabWidget* _tabs = nullptr;

		//—— §3 图表产出链（应用/显示共用，均假定数据已加载）——

		//当前选择元组解析（下拉文案→模型键；未就绪返回 valid()==false）
		struct PointSelection
		{
			int subcaseId = -1;
			std::string resultType;
			int gridId = -1;
			std::string component;
			bool valid() const
			{
				return subcaseId >= 0 && gridId >= 0
					&& !resultType.empty() && !component.empty();
			}
		};
		PointSelection currentSelection() const;

		//当前工况号（工况下拉 item userData 回取——2026-11-16 工况文案对齐"定义
		//曲线"口径带 "(label)" 后缀，不再从文案剥前缀；空下拉/未加载返回 -1，
		//维持 currentSelection 有效性门语义）
		int currentSubcaseId() const;

		//响应点表头令牌：响应点名称框非空→以名称替换表头行内 ID 令牌
		//（"Response @/… @/Node <ID>" 各段整体换名，2026-11-16 用户裁定；
		//simplified 折叠换行防破坏表头行结构）；空→ID 数字（现行为）。仅表头
		//消费——曲线名/kName/来源五元组/Note 维持 ID 口径
		QString pointHeaderToken(const PointSelection& sel) const;

		//产出入口（§3.1）：覆盖确认→布局映射（§3.2）→ beginMacro 组命令内产出
		//→ 收尾激活。两链拆分（2026-11-13 用户裁定，需求大图 01/06 权威）：
		//showChain=false 应用链=只产响应曲线一张（不再顺带产类型图）；
		//showChain=true 显示链=只产当前图表类型一张（不再顺带产响应曲线）。
		//返回本次产出 chartId 集合（覆盖判定与 §4 频率联动作用域），取消/失败
		//返回空
		QList<int> produceCharts(bool showChain);

		//§3.3 响应曲线（应用链专用；复用布局下 reuseChartId>=0 清空重建）
		int produceResponseCurve(int pageId, int reuseChartId);

		//§3.4 2D线图：模态贡献量频率曲线族（RSS 排序前 N + Response 蓝粗线 +
		//Others 红虚线 + Ranked By 粉框标签）；复用布局下 reuseChartId>=0
		//清空重建（kChartType 复位 XY——clearChartContent 不清节点属性）
		int produce2DLineChart(int pageId, int reuseChartId = -1);

		//§3.5/3.6 柱状图：单柱曲线 M+2 柱（Response+前 N+Others，类目签挂曲线）
		//+ 投影组合附加物（4 条 Datum 基准线 + Note 贡献列表注释）；复用布局下
		//reuseChartId>=0 清空重建（§4 频率刷新同通道）
		int produceBarChart(int pageId, int reuseChartId = -1);

		//§3.7 极坐标图：模态向量（θ=相位 r=幅值）复数三壳曲线族占位产出（渲染
		//依赖 §5，一期渲染回退 XY 骨架）；复用布局下 reuseChartId>=0 清空重建
		//（§4 频率刷新同通道）
		int producePolarChart(int pageId, int reuseChartId = -1);

		//§5.3 极坐标"首尾相接"形态下发（视图态，随显示选项"显示"行）：视图未
		//物化时 chartView 返回空跳过（同频率红线口径，物化后随下次下发补齐）
		void applyPolarChainForm(int chartId);

		//响应曲线 Y 轴标题（"Structural Response (Scale=…)" 后缀随 NVHCtx_Load
		//语境档+公共行计权联动）
		QString responseAxisTitle() const;

		//参与量轴标题：%Plot="% Participation"（无后缀），其余 "Participation
		//(Scale=…)"；ctx=NVHDisplayCtx（柱状产出传 NVHCtx_ShowBar、研究叠加传
		//NVHCtx_Research——后缀读各自语境档）
		QString participationAxisTitle(int ctx) const;

		//排序依据英文名（Note 末行/Ranked By 标签用，spec 柱状 Note 要求：
		//投影绝对值→Abs of Projected、幅值→Magnitude、RSS 档→Abs of Projected
		//RSS / Magnitude RSS）
		QString sortMetricEnglishName(bool rss = false) const;

		//排序依据英文名纯映射（rss=RSS 档后缀、projected=投影绝对值口径）——
		//极坐标表头按真实排序口径（复数分量组合框）取值，排序依据组合框不参与
		//极坐标排序（2026-11-12 晚）
		static QString sortMetricEnglishText(bool rss, bool projected);

		//—— §4 特定频率红线三联动 ——

		//三联动漏斗（任一路输入统一入口）：最近频率点归一 → 三路状态回写
		//（输入框/滑条阻塞互斥）→ 红线下发产出集合内全部视口 → 柱状/极坐标
		//辅助图随频率即时刷新（spec 特定频率红线三联动）
		void applyFrequencyState(double freq);

		//视图→面板（design D4）：画布左键频率拾取（工厂中继带 chartId，X 已在
		//视图侧吸附最近数据点）；仅处理本功能产出集合内事件
		void onCanvasFrequencyPicked(int chartId, double xValue);

		//§6.1 显示选项对话框：模态 exec 回显共享选项（图表类型驱动显示行文案
		//与缩放档置灰联动）；确定回写共享 struct（应用到后续生成），取消不动
		void onDisplayOptions();

		//—— 文件选择组 ——
		QLineEdit* _filePathEdit = nullptr;     //pch 路径回显（只读）
		QPushButton* _browseBtn = nullptr;      //浏览…
		QPushButton* _loadBtn = nullptr;        //加载

		//—— 结果选择组（六项）——
		QComboBox* _subcaseCombo = nullptr;     //工况
		QComboBox* _resultTypeCombo = nullptr;  //结果类型
		QComboBox* _pointIdCombo = nullptr;     //响应点ID
		QComboBox* _componentCombo = nullptr;   //矢量分量
		QComboBox* _resultSetsCombo = nullptr;  //结果集
		QLineEdit* _pointNameEdit = nullptr;    //响应点名称（可编辑，默认空）

		QPushButton* _displayOptionsBtn = nullptr;  //显示选项（§6 接对话框）
		QPushButton* _applyBtn = nullptr;           //应用

		//—— 结果显示页控件（§3）——
		QWidget* _iconRow = nullptr;            //图表类型图标行容器（UiPalette 取色
		                                        //QSS 挂此层，refreshPaletteStyles 重发）
		QToolButton* _chartTypeBtns[3] = { nullptr, nullptr, nullptr };
		                                        //图表类型图标钮（柱状/极坐标/2D线图；
		                                        //三图标直选，原型 04 图标行墨迹）
		QButtonGroup* _chartTypeBtnGroup = nullptr; //互斥组（防再点已选项取消选中）
		MechMainWindow::FoldSection* _rdFold = nullptr;  //图表档折叠框（单框动态标题：
		                                                //随图表类型 setTitle/切行集，
		                                                //见 slot_chartTypeChanged）
		QLineEdit* _freqEdit = nullptr;         //特定频率（数值输入框）
		QSlider* _freqSlider = nullptr;         //特定频率滑条（按频率点索引步进）
		QWidget* _freqRangeRow = nullptr;       //频率范围行（2D线图档框内行，与
		                                        //特定频率/滑条行互斥；内联起始/终止
		                                        //标签+行尾 Hz，权威大图 08 形态）
		QLineEdit* _freqStartEdit = nullptr;    //起始 Hz
		QLineEdit* _freqEndEdit = nullptr;      //终止 Hz
		QLabel* _freqRangeHint = nullptr;       //"频率范围：a Hz ~ b Hz" 说明行
		                                        //（2D线图档框内首行）
		QComboBox* _topNCombo = nullptr;        //显示最高贡献量（1~30 默认 10）
		QComboBox* _complexCombo = nullptr;     //复数分量（选项集随图表类型收敛）
		QComboBox* _complexSortCombo = nullptr; //排序依据（选项集随图表类型收敛）
		QPushButton* _rdOptionsBtn = nullptr;   //显示选项（结果显示页）
		QPushButton* _showBtn = nullptr;        //显示

		//—— 结果研究页（§7）——
		QComboBox* _calcTypeCombo = nullptr;    //计算类型：部分和/贡献量叠加
		QLineEdit* _rFreqStartEdit = nullptr;   //频率范围起（回显文件实际范围）
		QLineEdit* _rFreqEndEdit = nullptr;     //频率范围止（回显文件实际范围）
		QLabel* _rFreqRangeHint = nullptr;      //频率范围提示行（加载后回写实际范围）
		QSpinBox* _excludePctSpin = nullptr;    //去除比例(%)（叠加模式置灰）
		QComboBox* _pickOpCombo = nullptr;      //选择操作 Exclude/Include
		QCheckBox* _diffCheck = nullptr;        //显示差值曲线（默认不勾）
		QComboBox* _diffModeCombo = nullptr;    //差值模式：% of Response / Scale same as Response
		QListWidget* _modeList = nullptr;       //模态列表（多选，item data=模态号）
		QPushButton* _rOptionsBtn = nullptr;    //显示选项（研究页，复用 §6 对话框）
		QPushButton* _rShowBtn = nullptr;       //显示（研究页）
		QWidget* _modeRail = nullptr;           //模态列表右轨容器（UiPalette 取色
		                                        //QSS 挂此层，refreshPaletteStyles 重发）
		QToolButton* _selAllBtn = nullptr;      //模态列表右轨图标钮：全选
		QToolButton* _selNoneBtn = nullptr;     //清空
		QToolButton* _selInvBtn = nullptr;      //反选
		QToolButton* _selBarBtn = nullptr;      //第 4 钮：选中柱状图显示模态（D2，
		                                        //produceBarChart 成柱后启用；未成柱禁用）
		QSet<int> _barChartBarModes;            //最近一次柱状图成柱模态集（D2；第 4 钮
		                                        //点击按集合选中列表命中行，loadFile 清空）
		QToolButton* _selContribBtn = nullptr;  //第 5 钮「…」：选择贡献项对话框入口
		                                        //（局部 moreBtn 升成员，2026-11-18 裁定
		                                        //叠加档随计算类型联动置灰）

		QList<int> _producedChartIds;           //本次产出 chartId 集合（§4 联动作用域）
		int _producedAuxChartId = -1;           //随频率刷新的辅助图 id（柱状/极坐标；-1 无）
		int _producedAuxChartType = -1;         //辅助图类型（图表类型下拉口径 0 柱状/1 极坐标）

		ModalContributionModel* _model = nullptr;   //PFMODE 数据模型（1.2）
		//显示选项：公共行单份 + 缩放三元组按五语境独立（openspec
		//context-display-options-state：2026-11-17 用户裁定，缩放状态跨语境
		//互不覆盖——柱状图语境档位不再被响应曲线 Log 默认/极坐标 Linear 折算污染）
		NVHDisplayOptions _commonOpts;              //公共行（显示/加权/单位/绘图布局/设为全局）
		NVHContextScale _ctxScale[NVHCtx_Count];    //语境缩放（下标=NVHDisplayCtx）
	};
} // namespace PostWindow
