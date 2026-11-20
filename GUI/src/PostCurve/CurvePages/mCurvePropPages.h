#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 各节点类型属性页（对齐参考软件截图的分组与参数项）
//
// 页面与节点类型对应（注册关系见 mCurvePropPageManager::registerDefaultPages；
// 分组/行布局对齐参考软件截图01~08）：
//   mPagePropPage          Page（页，截图01：标准[标题/页面布局]/显示[字体设置]/
//                          基础[发布页面/发布页面标题]）
//   mChartPropPage         Chart（视口，截图02：基础[表头可见/表头/图表类型]/
//                          高级[背景颜色/网格线颜色]）
//   mCurveSeriesPropPage   Curve（截图03：标准[可见性]/曲线标签/线条属性/符号属性）
//   mCurveDataPropPage     CurveXData/CurveYData（截图04/05：数据源回显+
//                          标准[缩放/偏移/起始值/范围回显/坐标轴]/单位）
//   mAxisPropPage          XAxis/YAxis/YAxis2 共用（设计图05~07：标准/缩放设置/坐标刻度设置）
//   mLegendPropPage        Legend（截图08：标准[可见性/自动定位/图例位置]/
//                          显示[边框/字体/引线/背景/曲线文本色]）
//   mHeaderPropPage        Header（截图06：标准[可见性/文本/类型]/显示[一二三级字体/对齐/颜色]）
//   mFooterPropPage        Footer（截图07：同页眉结构）
//   mBaselinePropPage      Baseline（截图09：标准[可见性/名称/标签/图层/位置/
//                          关联轴/类型只读回显]/显示[字体设置/标签位置/线型/
//                          线条颜色/线宽]；默认线色蓝、实线）
//   mAnnotationPropPage    Annotation（设计图15：标准/位置/显示）
//
// 编写模式（每页一致）：
//   buildUi()       用 mCurveFoldSection 分组建控件，控件信号 -> commit()
//   loadFromModel() 从 document()->nodeProperty 回填控件
// =============================================================================

#include "../PostCurveGlobal.h"
#include "mCurvePropPageBase.h"
#include "../CurveCore/CurveDefine.h"
#include "../CurveWidgets/mCurveFoldSection.h"   //mCurveFoldSection = 共享 FoldSection 别名（成员指针用）

//QT
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace MechMainWindow
{
	class mImageGridComboBox;
}

namespace PostCurve
{
	class mCurveColorButton;
	class mCurveFontButton;
	class mCurveTextButton;
	class mCurvePenStyleCombo;
	class mCurveBorderStyleCombo;
	class mCurveBorderWidthCombo;
	class mCurveLineWidthCombo;
	class mCurveSymbolStyleCombo;
	class mCurveTemplateCombo;
	struct ChannelSelection;   //mCurveChannelSelector 的通道选择值（id 形态五元组）

	//页属性页：标准[标题/页面布局] + 显示[字体设置] + 基础[发布页面/发布页面标题]，
	//截图01。页面布局=mImageGridComboBox 视口布局图标网格（与 mOpenCurveDlg
	//布局下拉旁的网格选择同款，共用 curveViewportIconPaths），索引入模型（kLayout），
	//布局渲染下一期接入
	class POSTCURVE_EXPORT mPagePropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mPagePropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		mCurveTextButton* _titleBtn = nullptr; //标题（kPageTitle 页面标题模板，点击弹"编辑字符"
		                                       //弹窗；Y 轴"文本"行同款，区别于 kName 树显示名）
		MechMainWindow::mImageGridComboBox* _layoutGrid = nullptr; //页面布局（kLayout，图标索引）
		mCurveFontButton* _fontBtn = nullptr;  //字体设置（kFont，点击弹"选择字体"对话框）
		QCheckBox* _publishCheck = nullptr;    //发布页面（kPublishLayout）
		QCheckBox* _publishTitleCheck = nullptr; //发布页面标题（kPublishPageTitle）
	};

	//视口属性页：基础[表头可见/表头/图表类型] + 高级[背景颜色/网格线颜色]，截图02。
	//表头数据挂本图表节点（与页眉属性页/树右键同键联动）；背景/网格颜色入模型，
	//渲染已接线（rebuild 背景 PlotSettings / applyAxis 网格 AxisSettings）
	class POSTCURVE_EXPORT mChartPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mChartPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _headerVisibleCheck = nullptr; //表头可见（kHeaderVisible）
		mCurveTextButton* _headerTextBtn = nullptr; //表头（kHeaderText，点击弹"编辑字符"弹窗）
		QComboBox* _typeCombo = nullptr;       //图表类型（kChartType 0..4，setupChartTypeCombo 填充；XY/柱状/复数已接渲染，其余置灰占位）
		mCurveColorButton* _bgColorBtn = nullptr;   //背景颜色（kBackgroundColor，高级组）
		mCurveColorButton* _gridColorBtn = nullptr; //网格线颜色（kGridColor，高级组）
	};

	//曲线属性页：标准[可见性] / 曲线标签[显示标签·标签内容·显示标签前缀·标签前缀·
	//显示标签后缀·标签后缀] / 线条属性[线型·线条颜色·线宽·区域填充] /
	//符号属性[符号样式]，截图03。标签内容/前缀/后缀为模板下拉（mCurveTemplateCombo：
	//点箭头勾选来源变量按"~"拼接模板，编辑区可直接输入），文本入 kLabelText/
	//kLabelPrefix/kLabelSuffix；取消"显示标签"则下方标签设置项置灰（截图交互），
	//前缀/后缀开关各管其下拉可用态。标签渲染待实现（模型与编辑UI先行）；
	//符号样式>0 即绘制符号（渲染层按 kSymbolStyle 派生，见 mCurveChartView）
	class POSTCURVE_EXPORT mCurveSeriesPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mCurveSeriesPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		void updateLabelStates();   //标签设置项置灰联动（显示标签/前缀/后缀开关）
		void updateSymbolRows();    //符号设置行显隐联动（选非空标记后现出颜色/密度/缩放尺寸）
		mCurveFoldSection* _labelFold = nullptr;      //曲线标签组
		mCurveFoldSection* _symbolFold = nullptr;     //符号属性组
		QCheckBox* _visibleCheck = nullptr;           //可见性（kVisible，取消勾选曲线连同标签一并隐藏）
		QCheckBox* _labelVisibleCheck = nullptr;      //显示标签（kLabelVisible）
		mCurveTemplateCombo* _labelContentCombo = nullptr; //标签内容（kLabelText，模板下拉）
		QCheckBox* _labelPrefixCheck = nullptr;       //显示标签前缀（kLabelPrefixVisible）
		mCurveTemplateCombo* _labelPrefixEdit = nullptr;   //标签前缀（kLabelPrefix，模板下拉）
		QCheckBox* _labelSuffixCheck = nullptr;       //显示标签后缀（kLabelSuffixVisible）
		mCurveTemplateCombo* _labelSuffixEdit = nullptr;   //标签后缀（kLabelSuffix，模板下拉）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr; //线型（kLineStyle，TecLinePattern 0..3）
		mCurveColorButton* _colorBtn = nullptr;       //线条颜色（kColor）
		mCurveLineWidthCombo* _lineWidthCombo = nullptr; //线宽（kLineWidth，图标行0..3=1..4px）
		QCheckBox* _areaFillCheck = nullptr;          //区域填充（kAreaFill）
		mCurveSymbolStyleCombo* _symbolStyleCombo = nullptr; //符号样式（kSymbolStyle 0..9）
		mCurveColorButton* _symbolColorBtn = nullptr; //符号颜色（kSymbolColor）
		QSpinBox* _densitySpin = nullptr;             //符号密度（kSymbolDensity，1=逐点）
		QDoubleSpinBox* _scaleSpin = nullptr;         //缩放尺寸（kSymbolScale，实际大小=基准×系数）
	};

//曲线X/Y数据属性页（CurveXData/CurveYData 壳节点共用，ySide 区分侧别），截图04/05：
//数据组[数据源(占位:外部文件)/文件+更改(打开数据文件(pch/csv/txt,"从文件创建曲线"同款文件
//对话框)后按默认通道直接换源,不弹通道选择器,设计图13更改曲线;R22:ascii(csv/txt)数据集
//固定默认通道=[Unknown]/[Block 1]/第 1 列,X 亦默认第 1 列;复数曲线换 csv/txt 拒绝——无复数
//分量对)/子工况/结果类型/
//对象/分量(四级级联
//真下拉：结果类型含解析的横坐标类型
	//"Time[Sec]"/"Frequency[Hz]"(置首)与矢量结果类型，逐级过滤；选横坐标类型
	//该侧取记录横坐标（对象/分量置灰），选实际结果类型 Y 侧整线换源 / X 侧取
	//通道分量值——切换即重绘。R22:ascii 数据集三级口径——子工况占位"-"
	//(禁用,不落假 "Subcase 1")、结果类型=Unknown、对象=Block 1、分量=列名；
	//文件不在仓库时退化为单项回显)] +
//标准组[缩放系数/偏移量/起始值(只读回显)/最大值/最小值(数据范围置灰回显)/
//用户表达式(置灰挂设计图12)/坐标轴(X侧X1/X2、Y侧Y1/Y2；复数曲线 ym/yp 壳不显示)] + 单位组[类型(置灰占位)/
//量纲(kUnit)]
class POSTCURVE_EXPORT mCurveDataPropPage : public mCurvePropPageBase
{
	Q_OBJECT
public:
	//侧别：0=X 数据页（坐标轴 X1/X2）、1=Y 数据页（Y1/Y2；复数曲线 "ym" 壳共用
	//本页，该形态坐标轴行整行隐藏）、2=复数曲线下面板数据页"yp"（相位/虚部侧，
	//复数曲线第三壳；无坐标轴行——下面板不挂轴）。
	//传 bool 兼容旧调用（false=X true=Y）
	explicit mCurveDataPropPage(int side, QWidget* parent = nullptr);
	void loadFromModel() override;
private:
	void buildUi();
//"更改..."：打开数据文件（pch/csv/txt，"从文件创建曲线"同款对话框，解析登记
//入共享仓库）->默认通道直接 commitChannelChange 换源（不弹通道选择器，通道
//随后可经本页四级下拉调整）；ascii 数据集默认通道=[Unknown]/[Block 1]/第 1 列
void changeSource();
//导入成功续接（原 changeSource 内联换源链路抽出共用）：已导入数据集提示 +
//默认通道直接 commitChannelChange 换源（ascii：复数曲线拒绝——无复数分量对）
void applyImportedSource(const QString& filePath);
	//来源四级下拉回填（模型->控件）：结果类型含解析出的横坐标类型
	//（"Time[Sec]"/"Frequency[Hz]"，置首，选中即该侧取记录横坐标）与矢量
	//结果类型；选项内容与"定义曲线"文件页四级列同源对齐（工况带 label、
	//结果类型=schemaName）。R22:ascii 数据集三级口径（子工况占位"-"/Unknown/
	//Block 1/列名）。逐级定位现值；失配层级把模型现值文本补为末项
	//选中（回显兜底），全程信号阻断
	void refillChannelCombos();
	//用户改任一级下拉：重填下级（横坐标类型形态下对象/分量置灰清空）并提交
	//该侧——横坐标类型 -> 该侧取记录横坐标；实际结果类型 -> Y 侧换新通道
	//（X 保持自身形态与数据）/ X 侧取通道分量值（点数与 Y 一致才配对）。
	//切换即重绘
	void onChannelComboChanged();
	//Y 侧通道换源落模（数据直写 + 撤回栈快照 + 手动回填一次）；"更改..."与
	//Y 侧实际结果类型共用。X 侧不受影响：通道形态数据原样保留（点数与
	//新 Y 成对才落模），横坐标形态 X = 新锚通道的记录横坐标
	void commitChannelChange(const ChannelSelection& picked);
	//复数曲线成对换源落模（任一壳侧/任一父曲线为复数曲线时，整线换源走这里）：
	//所选分量经 siblingComplexComponent 取同方向对侧，两路装配后
	//setComplexSeriesData/setComplexSeriesSource 直写 + 撤回快照 + 手动回填
	void commitComplexChannelChange(const ChannelSelection& picked);
	//复数曲线 ym/yp 侧独立换源（D7 三侧全独立）：本页四级下拉只动本侧
	//（_side==1 换锚四元组、complexSide* 保现存；_side==2 只写 complexYData
	//+complexSide*、锚五元组/复数格式原样）
	void commitComplexSideChannel(const ChannelSelection& picked);
	//复数曲线 ym/yp 页横坐标形态（D7）：本侧数据=锚通道块在所选工况下的
	//记录横坐标，他侧不动（XY 曲线同语义写口 commitSideAbscissa 写共享
	//X/Y 壳，对复数 yp 页不可见）
	void commitComplexSideAbscissa(const QString& abscissaType, int subcaseId);
	//该侧取锚通道数据块的记录横坐标（横坐标类型形态提交；结果类型/对象/分量
	//取来源五元组，子工况取自下拉——工况下拉恒可用；点数与对侧现数据一致才落模）
	void commitSideAbscissa(const QString& abscissaType, int subcaseId);
	//X 侧取所选通道分量值（实际结果类型形态提交；点数与 Y 一致才落模）
	void commitSideChannel(const ChannelSelection& picked);
		int _side = 0;                              //侧别 0=X / 1=Y / 2=复数下面板侧"yp"
		QComboBox* _sourceTypeCombo = nullptr;      //数据源（占位：外部文件）
		QLineEdit* _sourceFileEdit = nullptr;       //文件（kSourceFile，只读回显）
		QPushButton* _changeBtn = nullptr;          //打开文件图标按钮（打开 pch 换源，设计图13）
		QComboBox* _subcaseCombo = nullptr;         //子工况（kSubcase，级联下拉）
		QComboBox* _schemaCombo = nullptr;          //结果类型（kSchema，级联下拉）
		QComboBox* _objectCombo = nullptr;          //对象（kObject，级联下拉）
		QComboBox* _componentCombo = nullptr;       //分量（kComponent，级联下拉）
		QDoubleSpinBox* _scaleSpin = nullptr;       //缩放系数（kScale）
		QDoubleSpinBox* _offsetSpin = nullptr;      //偏移量（kOffset）
		QDoubleSpinBox* _startSpin = nullptr;       //起始值（本侧数据首值，只读回显）
		QDoubleSpinBox* _maxSpin = nullptr;         //最大值（本侧数据范围，置灰回显）
		QDoubleSpinBox* _minSpin = nullptr;         //最小值（同上）
		QLineEdit* _expressionEdit = nullptr;       //用户表达式（kExpression，置灰占位）
		mCurveFoldSection* _stdFold = nullptr;      //标准分组（ym 坐标轴行条件显隐用）
		QComboBox* _axisAssignCombo = nullptr;      //坐标轴（kAxisAssign；yp 侧不建行，ym 壳复数形态整行隐藏）
		QLineEdit* _unitTypeEdit = nullptr;         //类型（置灰占位）
		QLineEdit* _unitEdit = nullptr;             //量纲（kUnit）
	};

	//坐标轴属性页（截图11/12）：标准[可见性/名称可见性/名称/文本/类型(只读回显:水平/
	//垂直)] + 显示[标签字体设置(kTitleFont)/颜色(kTitleColor)] + 缩放设置[缩放方式
	//(kScaleMode)/参考值(kDbRef,dB20/dB10 共用一项仅 dB 模式显示,行标签随模式切换)/
	//计权(kWeightMode,已实现,仅 dB 模式显示)/最小值/最大值(改范围即退出自动)] +
	//坐标轴刻度设置[格式(kLabelFormat:自动/科学计数法/定点格式) + 仅 linear 轴:
	//刻度生成方式(kTickGenMode:均匀分步法→刻度总数(kTickCount)/固定步长法→固定
	//步长(kMajorStep))/每主刻度网格数(kMinorCount) + 仅非 linear 轴(log/dB):每十倍
	//刻度数(kDecadeTickCount)/每十倍网格线数(kDecadeMinorCount)] + 精度(kPrecision)/
	//刻度字体设置(kLabelFont) 恒显]。
	//类别轴模式（柱状图 X 主轴即切换，树显示名 Category；不论曲线是否已带
	//类别刻度名，设计图）：标准只留"文本"，缩放设置改显 起始/终止(下拉选项=
	//类别刻度名，提交为 kMin=序号-0.5/kMax=序号+0.5；无类别刻度名时选项=
	//曲线 X 数据值升序去重，提交为所选 X 值±半格——半格=到相邻数据点距离
	//的一半，选中端点柱完整框进，同类别轴；改即退出自动)/
	//标签对齐方式(kCategoryLabelAlign:水平/对角 斜向/垂直,渲染端对数字刻度
	//同样生效)，坐标轴刻度设置整组隐藏（类别刻度不走数字刻度引擎，见
	//applyAxis）；显示组不变。判定与回填见 loadFromModel（图表类型/类别名在
	//他节点上变化经 ctor 全局监听重载）
	//XAxis/YAxis/YAxis2 三种节点共用本页；与弹窗 mAxisEditorPropPage 同构。
	//刻度模式采用"数据预变换+线性轴"（复刻 NVHCurveWgt 语义）：
	//切换后渲染层按新模式变换数据，刻度数字显示变换空间的值
	class POSTCURVE_EXPORT mAxisPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mAxisPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//条件行显隐：类别轴模式整组切换（见类注释）+ 数字模式下缩放设置（参考
		//值/计权仅 dB 模式显示，参考值行标签随模式切换）+ 刻度设置（刻度生成
		//方式组仅 linear，每十倍组仅非 linear）
		void updateRowStates();
		//缩放/平移联动：自动范围轴的最小值/最大值随当前显示范围刷新（阻塞
		//信号回显，不退出自动）
		void onAxisDisplayRangeChanged(int axisNodeId);
		//把自动范围轴的最小值/最大值回显为引擎当前显示范围（重建落定后延后
		//一拍调用，避免拿到旧刻度空间的范围）
		void echoDisplayRange();
		//类别轴：起始/终止随当前显示范围回显（自动=引擎现值，手动=模型
		//min/max；类别 i 的可见区间 [i-0.5,i+0.5]，四舍五入取回序号）
		void echoCategoryRange();
		//类别轴：起始/终止下拉提交（editingStart=改起始；idx=类别序号）。提交为
		//kMin=idx-0.5/kMax=idx+0.5（与 fitChartAxes 类别范围外扩半格同约定），
		//改即退出自动，另一侧先按当前显示范围回写（同数值最小/最大策略），
		//起始>终止时另一端跟随
		void commitCategoryBound(bool editingStart, int idx);
		mCurveFoldSection* _baseFold = nullptr;  //标准分组（条件行显隐用）
		QCheckBox* _visibleCheck = nullptr;      //可见性（kVisible）
		QCheckBox* _nameVisibleCheck = nullptr;  //名称可见性（kTitleVisible，名称=轴标题）
		QLineEdit* _nameEdit = nullptr;          //名称（kName，树显示名，同注释/基准线页）
		mCurveTextButton* _titleEdit = nullptr;  //文本（kTitle，按钮弹"编辑字符"弹窗；支持模板占位符，渲染时解析）
		QLineEdit* _typeEdit = nullptr;          //类型（只读回显：X轴=水平，Y/Y2轴=垂直，置灰）
		mCurveFontButton* _titleFontBtn = nullptr; //标签字体设置（kTitleFont，弹窗选择）
		mCurveColorButton* _colorBtn = nullptr;  //颜色（kTitleColor）
		mCurveFoldSection* _rangeFold = nullptr; //缩放设置分组（条件行显隐用）
		QComboBox* _scaleModeCombo = nullptr;    //缩放方式（kScaleMode：0 Linear/1 Logarithmic/2 dB20/3 dB10）
		QLabel* _dbRefLabel = nullptr;           //参考值行标签（文本随缩放方式切换 dB20/dB10）
		QDoubleSpinBox* _dbRefSpin = nullptr;    //参考值（kDbRef，仅 dB 模式显示）
		QComboBox* _weightCombo = nullptr;       //计权（kWeightMode：0无/1A/2B/3C/4U，U=unweighted 与无等效，仅 dB 模式显示）
		QDoubleSpinBox* _minSpin = nullptr;      //最小值（kMin，原始值空间，改范围即退出自动）
		QDoubleSpinBox* _maxSpin = nullptr;      //最大值（kMax）
		QComboBox* _catStartCombo = nullptr;     //起始（类别下拉，kMin=序号-0.5，仅类别轴显示）
		QComboBox* _catEndCombo = nullptr;       //终止（类别下拉，kMax=序号+0.5，仅类别轴显示）
		QComboBox* _catAlignCombo = nullptr;     //标签对齐方式（kCategoryLabelAlign：0水平/1对角 / 斜向/2垂直，仅类别轴显示）
		QStringList _categoryLabels;             //类别刻度名（数值来源写入；空=无，起始/终止改用曲线 X 数据值）
		QVector<double> _barXValues;             //曲线 X 数据值（无类别刻度名时起始/终止的数字选项，升序去重）
		bool _categoryMode = false;              //类别轴模式（柱状图 X 主轴）
		mCurveFoldSection* _tickFold = nullptr;  //坐标轴刻度设置分组（条件行显隐用）
		QComboBox* _formatCombo = nullptr;       //格式（kLabelFormat：0自动/1科学计数法/2定点格式）
		QComboBox* _tickGenCombo = nullptr;      //刻度生成方式（kTickGenMode：0均匀分步法/1固定步长法，仅 linear 轴显示）
		QDoubleSpinBox* _tickCountSpin = nullptr; //刻度总数（kTickCount，均匀分步法）
		QDoubleSpinBox* _fixedStepSpin = nullptr; //固定步长（kMajorStep，固定步长法）
		QSpinBox* _gridPerMajorSpin = nullptr;   //每主刻度网格数（kMinorCount，仅 linear 轴显示）
		QSpinBox* _decadeTickSpin = nullptr;     //每十倍刻度数（kDecadeTickCount，仅非 linear 轴显示）
		QSpinBox* _decadeGridSpin = nullptr;     //每十倍网格线数（kDecadeMinorCount，仅非 linear 轴显示）
		QSpinBox* _precisionSpin = nullptr;      //精度（kPrecision）
		mCurveFontButton* _labelFontBtn = nullptr; //刻度字体设置（kLabelFont，弹窗选择）
	};

	//图例属性页（参数栏，截图08）：标准[可见性/自动定位/图例位置(9选项，勾选自动
	//定位时隐藏)] + 显示[边框样式(5式)/边框颜色/字体设置/引线位置/背景填充色/
	//曲线文本色/文本颜色(自定义模式)]。X/Y 坐标编辑在弹窗专属页 mLegendEditorPropPage
	//（两族页面编辑同一组模型键，枚举语义一致，防止两族页面写坏彼此的值）
	class POSTCURVE_EXPORT mLegendPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mLegendPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//条件行显隐：勾选自动定位->隐藏图例位置行；曲线文本色选"自定义"->显示文本颜色行
		void updateRowStates();
		mCurveFoldSection* _baseFold = nullptr;  //标准组（条件行显隐用）
		mCurveFoldSection* _dispFold = nullptr;  //显示组（条件行显隐用）
		QCheckBox* _visibleCheck = nullptr;      //可见性（kVisible）
		QCheckBox* _autoPosCheck = nullptr;      //自动定位（kAutoPosition）
		QComboBox* _positionCombo = nullptr;     //图例位置（kPosition 0..8：图内四角/自定义/图外四向）
		mCurveBorderWidthCombo* _borderWidthCombo = nullptr; //边框线宽（kLegendBorderWidth：0无边框 1..4px）
		mCurveColorButton* _borderColorBtn = nullptr; //边框颜色（kBorderColor）
		mCurveFontButton* _fontBtn = nullptr;    //字体设置（kLegendFont，弹窗选择）
		QComboBox* _leaderPosCombo = nullptr;    //引线位置（kLeaderPos：0左侧 1下方）
		mCurveColorButton* _bgColorBtn = nullptr;     //背景填充色（kBgColor）
		QComboBox* _textColorModeCombo = nullptr; //曲线文本色（kTextColorMode：0跟随曲线颜色 1自定义）
		mCurveColorButton* _textColorBtn = nullptr;   //文本颜色（kTextColor）
	};

	//页眉属性页：标准[可见性/文本/类型(只读回显:页眉)] + 显示[一级字体设置/
	//二级字体设置/三级字体设置/对齐方式/颜色]，截图06（数据挂图表节点）。
	//三组字体各为独立字体按钮（kHeaderFont/kHeaderFont2/kHeaderFont3，弹窗
	//选择，二三级未选按一级折算渲染）；文本=按钮弹"编辑字符"弹窗编辑
	//（模板文本长，行内不放输入框）
	class POSTCURVE_EXPORT mHeaderPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mHeaderPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _visibleCheck = nullptr;      //可见性（kHeaderVisible）
		mCurveTextButton* _textBtn = nullptr;    //文本（kHeaderText，点击弹"编辑字符"弹窗）
		QLineEdit* _typeEdit = nullptr;          //类型（只读回显"页眉"，置灰）
		mCurveFontButton* _fontBtn = nullptr;    //一级字体设置（kHeaderFont，弹窗选择）
		mCurveFontButton* _font2Btn = nullptr;   //二级字体设置（kHeaderFont2，弹窗选择）
		mCurveFontButton* _font3Btn = nullptr;   //三级字体设置（kHeaderFont3，弹窗选择）
		QComboBox* _alignCombo = nullptr;        //对齐方式（kHeaderAlign：0中 1左 2右）
		mCurveColorButton* _colorBtn = nullptr;  //颜色（kHeaderColor，已接线渲染：引擎条带逐行颜色，
		                                         //选中态高亮优先、未选中=模型色/无效兜底黑）
	};

	//页脚属性页：标准[可见性/文本/类型(只读回显:页脚)] + 显示[一级字体设置/
	//二级字体设置/三级字体设置/对齐方式/颜色]，截图07（数据挂图表节点）；
	//三组字体各为独立字体按钮（同页眉页）；文本=按钮弹"编辑字符"弹窗编辑
	class POSTCURVE_EXPORT mFooterPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mFooterPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _visibleCheck = nullptr;      //可见性（kFooterVisible）
		mCurveTextButton* _textBtn = nullptr;    //文本（kFooterText，点击弹"编辑字符"弹窗）
		QLineEdit* _typeEdit = nullptr;          //类型（只读回显"页脚"，置灰）
		mCurveFontButton* _fontBtn = nullptr;    //一级字体设置（kFooterFont，弹窗选择）
		mCurveFontButton* _font2Btn = nullptr;   //二级字体设置（kFooterFont2，弹窗选择）
		mCurveFontButton* _font3Btn = nullptr;   //三级字体设置（kFooterFont3，弹窗选择）
		QComboBox* _alignCombo = nullptr;        //对齐方式（kFooterAlign：0中 1左 2右）
		mCurveColorButton* _colorBtn = nullptr;  //颜色（kFooterColor，已接线渲染：引擎条带逐行颜色，
		                                         //选中态高亮优先、未选中=模型色/无效兜底黑）
	};

	//基准线属性页（参考软件截图09）：标准[可见性/名称/标签/图层(占位)/位置/
	//关联轴/类型(只读回显:垂直/水平，置灰)] + 显示[字体设置(kLabelFont，绘图选项
	//同款弹窗选择，标签字体渲染下一期)/标签位置(9宫格，默认右中)/线型/线条颜色/
	//线宽(1~4px图标档)]。
	//关联轴下拉切换方向（X侧=纵向固定X值，Y侧=横向固定Y值），轴名入 kLink、
	//方向入 kOrientation（渲染暂只取方向，按具体轴定位下一期）；
	//线型换算：页面index 0..3 -> Qt::PenStyle 1..4（与曲线侧枚举不同）
	class POSTCURVE_EXPORT mBaselinePropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mBaselinePropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _visibleCheck = nullptr;          //可见性（kVisible）
		QLineEdit* _nameEdit = nullptr;              //名称（kName，树显示名）
		QLineEdit* _labelEdit = nullptr;             //标签（kLabel，渲染下一期）
		QLineEdit* _layerEdit = nullptr;             //图层（kLayer，占位）
		QDoubleSpinBox* _valueSpin = nullptr;        //位置（kValue：横向=Y值，纵向=X值）
		QComboBox* _linkCombo = nullptr;             //关联轴（kLink+kOrientation：X侧纵向/Y侧横向）
		QLineEdit* _typeEdit = nullptr;              //类型（只读回显 垂直/水平，置灰）
		mCurveFontButton* _fontBtn = nullptr;        //字体设置（kLabelFont，弹窗选择，渲染下一期）
		QComboBox* _labelPosCombo = nullptr;         //标签位置（kLabelPos：9宫格，默认右中）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr; //线型（kLineStyle，Qt::PenStyle）
		mCurveColorButton* _colorBtn = nullptr;      //线条颜色（kColor，默认蓝）
		mCurveLineWidthCombo* _lineWidthCombo = nullptr; //线宽（kLineWidth，1..4px图标档）
	};

	//注释属性页（截图10）：标准[可见性/名称/文本/标量值格式化(格式/精度)/附着于/
	//附着曲线/附着点/位置] + 选项[最小化(半径/颜色)] + 显示[字体设置(kFont，弹窗
	//选择，渲染下一期)/对齐方式/边框线宽/边框颜色(跟随曲线颜色/自定义，自定义显示
	//选择边框颜色；视图类整行隐藏，样式同弹窗编辑器)/自动定位(策略/偏移，仅曲线类)/
	//背景颜色(默认|自定义+取色)/文本颜色(默认|自定义
	//+取色)]。名称/文本/位置/可见性/附着/最小化/自动定位/各颜色接既有模型属性；
	//条件行由 updateConditionalRows 整行显隐
	class POSTCURVE_EXPORT mAnnotationPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mAnnotationPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		void updateConditionalRows();   //按勾选/下拉状态刷新各条件行显隐
		void syncPosEdit();             //位置框回显："X Y"（固定 5 位小数，空格分隔，与弹窗编辑页同语义）
		mCurveFoldSection* _annoFold = nullptr;        //标准分组（条件行显隐用）
		mCurveFoldSection* _optFold = nullptr;         //选项分组
		mCurveFoldSection* _dispFold = nullptr;        //显示分组
		QCheckBox* _visibleCheck = nullptr;            //可见性（kVisible，默认勾选，取消即隐藏注释）
		QLineEdit* _nameEdit = nullptr;                //名称（kName）
		QPushButton* _textBtn = nullptr;              //文本（kText，支持 {X}/{Y}/{Time} 模板；
		                                              //点击弹注释文本对话框，按钮回显当前文本）
		QCheckBox* _scalarCheck = nullptr;             //标量值格式化（kScalarFmtOn，勾选后显示格式/精度）
		QComboBox* _scalarFmtCombo = nullptr;          //格式（kScalarFmtMode：0科学计数法 1定点格式）
		QSpinBox* _scalarPrecisionSpin = nullptr;      //精度（kScalarFmtPrecision，小数位数）
		QComboBox* _attachModeCombo = nullptr;         //附着于=注释类别（kAttachMode：0曲线类 1视图类）
		QComboBox* _attachCurveCombo = nullptr;        //附着曲线（kAttachCurve，曲线类吸附/求值的目标曲线）
		QSpinBox* _attachIndexSpin = nullptr;          //附着点顺序（kAttachIndex，1基；改值即附着到该点）
		QLineEdit* _posEdit = nullptr;                 //位置"X Y"（kPosX/kPosY，数据坐标，空格分隔单框）
		QCheckBox* _minimizeCheck = nullptr;           //最小化（勾选后显示半径/颜色）
		QSpinBox* _minimizeRadiusSpin = nullptr;       //最小化半径（kMinimizeRadius）
		mCurveColorButton* _minimizeColorBtn = nullptr; //最小化颜色（kMinimizeColor）
		mCurveFontButton* _fontBtn = nullptr;          //字体设置（kFont，弹窗选择，渲染下一期）
		QComboBox* _alignCombo = nullptr;              //对齐方式（kAlign：0左 1中 2右）
		mCurveLineWidthCombo* _borderWidthCombo = nullptr; //边框线宽（kBorderWidth：线宽样本图标1..4px）
		QComboBox* _borderColorCombo = nullptr;        //边框颜色（kBorderMode：跟随曲线颜色/自定义，与弹窗编辑页同语义）
		mCurveColorButton* _borderColorBtn = nullptr;  //选择边框颜色（kBorderColor，取消勾选后显示）
		QCheckBox* _autoPosCheck = nullptr;            //自动定位（kAutoPos，勾选后显示策略/偏移）
		QComboBox* _autoAnchorCombo = nullptr;         //自动定位策略（kAutoAnchor 0..8，"不重叠"时隐藏偏移）
		QDoubleSpinBox* _autoOffsetSpin = nullptr;     //自动定位偏移（kAutoOffset）
		QComboBox* _bgColorCombo = nullptr;            //背景颜色（默认=无效色/自定义）
		mCurveColorButton* _bgColorBtn = nullptr;      //选择背景颜色（kBgColor，自定义时显示）
		QComboBox* _textColorCombo = nullptr;          //文本颜色（默认=无效色/自定义）
		mCurveColorButton* _textColorBtn = nullptr;    //选择文本颜色（kColor，自定义时显示）
	};
}
