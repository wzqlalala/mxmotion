#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 属性编辑器弹窗专属页面族
//
// 与参数栏页面族（mCurvePropPages）是两套内容：弹窗是面向"图上对象"的
// 快捷编辑器，分组与参数项按设计图逐类型补齐——
//   mCurveEditorPropPage      曲线（标准/曲线标签/线条属性/符号属性）
//   mChartEditorPropPage      图表（参考软件图表属性编辑器：基础[表头可见性/
//                              表头/图表类型]+高级[背景颜色/网格线颜色]；
//                              双击图表空白区域/右键空白"编辑"激活）
//   mAnnotationEditorPropPage 注释（标注/显示，右键注释"编辑"弹出）
//   mAxisEditorPropPage       坐标轴（标准/缩放设置/坐标刻度设置，三轴
//                              共用；双击纵轴标题激活纵坐标属性编辑器弹出）
	//   mBaselineEditorPropPage   基准线（标准/显示与参数栏同内容，
	//                              双击基准线或右键基准线"编辑"弹出，弹窗落点视窗左上角）
// 经 mCurvePropPageManager::registerEditorPage 注册（未注册类型回退参数栏页）。
//
// 编写模式与参数栏页面一致：buildUi() 用 mCurveFoldSection 分组建控件，
// 控件信号 -> commit()；loadFromModel() 从 document()->nodeProperty 回填。
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
#include "mPostAnnotationDialog.h"

namespace PostCurve
{
	class mCurveColorButton;
	class mCurveFontButton;
	class mCurveTextButton;
	class mCurvePenStyleCombo;
	class mCurveBorderStyleCombo;
	class mCurveTemplateCombo;
	class mCurveLineWidthCombo;
	class mCurveSymbolStyleCombo;

	//曲线快捷编辑页：标准/曲线标签/线条属性/符号属性。
	//曲线标签组与参数栏曲线页同交互：标签内容/前缀/后缀为模板下拉（mCurveTemplateCombo，
	//点箭头勾选来源变量按"~"拼接、编辑区可直接输入，文本入 kLabelText/kLabelPrefix/
	//kLabelSuffix），前缀/后缀带显示开关（kLabelPrefixVisible/kLabelSuffixVisible）；
	//取消"显示标签"则下方标签设置项置灰。标签渲染暂未实现（模型与编辑UI先行）；
	//区域填充即时生效
	class POSTCURVE_EXPORT mCurveEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mCurveEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		void updateLabelStates();   //标签设置项置灰联动（显示标签/前缀/后缀开关）
		void updateSymbolRows();    //符号设置行显隐联动（选非空标记后现出颜色/密度/缩放尺寸）
		QCheckBox* _visibleCheck = nullptr;              //可见性（kVisible）
		mCurveFoldSection* _labelFold = nullptr;         //曲线标签组（置灰联动用）
		mCurveFoldSection* _symbolFold = nullptr;        //符号属性组（行显隐联动用）
		QCheckBox* _labelVisibleCheck = nullptr;         //显示标签（kLabelVisible）
		mCurveTemplateCombo* _labelContentCombo = nullptr; //标签内容（kLabelText，模板下拉）
		QCheckBox* _labelPrefixCheck = nullptr;          //显示标签前缀（kLabelPrefixVisible）
		mCurveTemplateCombo* _labelPrefixEdit = nullptr;    //标签前缀（kLabelPrefix，模板下拉）
		QCheckBox* _labelSuffixCheck = nullptr;          //显示标签后缀（kLabelSuffixVisible）
		mCurveTemplateCombo* _labelSuffixEdit = nullptr;    //标签后缀（kLabelSuffix，模板下拉）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr;  //线型（kLineStyle，TecLinePattern 0..3）
		mCurveColorButton* _colorBtn = nullptr;          //线条颜色（kColor）
		QDoubleSpinBox* _lineWidthSpin = nullptr;        //线宽（kLineWidth）
		QCheckBox* _areaFillCheck = nullptr;             //区域填充（kAreaFill，渲染层映射 showFill）
		mCurveSymbolStyleCombo* _symbolStyleCombo = nullptr; //标识样式（kSymbolStyle 0..9，绘图选项同款符号图标）
		mCurveColorButton* _symbolColorBtn = nullptr;    //符号颜色（kSymbolColor）
		QSpinBox* _densitySpin = nullptr;                //符号密度（kSymbolDensity，1=逐点）
		QDoubleSpinBox* _scaleSpin = nullptr;            //缩放尺寸（kSymbolScale，实际大小=基准×系数）
	};

	//图表快捷编辑页（参考软件图表属性编辑器）：基础[表头可见性/表头/图表类型] +
	//高级[背景颜色/网格线颜色]。全部属性绑定 Chart 节点提交（页眉数据本就挂在
	//图表节点上，kHeaderVisible/kHeaderText/kChartType/kBackgroundColor/kGridColor
	//在 Chart 节点均有分发）；表头两行与背景/网格色的渲染均已接线，编辑即时生效。
	//表头=按钮弹"编辑字符"弹窗编辑（与参数栏图表页同款）。
	//图表类型 5 项（XY/柱状图/复数图/极坐标图/蛛网图），除 XY 外渲染占位。
	//入口：双击图表空白区域 / 右键空白"编辑"（空白=非曲线/图例/注释/轴标题）
	class POSTCURVE_EXPORT mChartEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mChartEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _headerVisibleCheck = nullptr;    //表头可见性（kHeaderVisible）
		mCurveTextButton* _headerTextBtn = nullptr;  //表头（kHeaderText，点击弹"编辑字符"弹窗）
		QComboBox* _typeCombo = nullptr;             //图表类型（kChartType 0..4）
		mCurveColorButton* _bgColorBtn = nullptr;    //背景颜色（kBackgroundColor）
		mCurveColorButton* _gridColorBtn = nullptr;  //网格线颜色（kGridColor）
	};

	//注释快捷编辑页：标准[可见性/名称/文本/标量值格式化(格式/精度)/
	//附着于/附着曲线/附着点/位置] + 选项[最小化(半径/颜色)] + 显示[字体设置
	//(kFont，弹窗选择，渲染下一期)/对齐方式/边框线宽/边框颜色(选择边框颜色)/
	//自动定位(策略/偏移)/背景颜色(选择背景颜色)/文本颜色(选择文本颜色)]。
	//名称/文本/位置/文本颜色/对齐/边框模式/自动定位接既有模型属性即时生效
	//（增量刷新）；"文本"按钮弹mPostAnnotationDialog（与视图双击注释一致，
	//保存写回 kText）；边框线宽/边框色/背景色/文本色/字体经命令栈提交、
	//渲染层即时生效；条件行由updateConditionalRows按勾选/下拉状态整行显隐；
	//对齐方式为设计占位
	class POSTCURVE_EXPORT mAnnotationEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mAnnotationEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		void updateConditionalRows();   //按当前勾选/下拉状态刷新各条件行显隐
		void syncPosEdit();             //位置框回显："X Y"（固定 5 位小数，空格分隔，与属性页同语义）
		mCurveFoldSection* _annoFold = nullptr;        //标准分组（条件行显隐用）
		mCurveFoldSection* _optFold = nullptr;         //选项分组
		mCurveFoldSection* _dispFold = nullptr;        //显示分组
		QCheckBox* _visibleCheck = nullptr;            //可见性（kVisible，默认勾选，取消即隐藏注释）
		QLineEdit* _nameEdit = nullptr;                //名称（kName）
        QPushButton* _textBtn = nullptr;
		//QLineEdit* _textEdit = nullptr;                //文本（kText，支持 {X}/{Y}/{Label} 模板）
        QCheckBox* _scalarCBox = nullptr;              //标量值格式化（kScalarFmtOn，勾选后显示格式/精度）
		QComboBox* _scalarFmtCombo = nullptr;          //格式（kScalarFmtMode：0科学计数法 1定点格式）
		QSpinBox* _scalarPrecisionSpin = nullptr;      //精度（kScalarFmtPrecision，小数位数）
		QComboBox* _dependOnCombo = nullptr;           //附着于=注释类别（kAttachMode：0曲线类 1视图类）
		QComboBox* _dependCurveCombo = nullptr;        //附着曲线（kAttachCurve，曲线类吸附/求值的目标曲线）
		QSpinBox* _attachIndexSpin = nullptr;          //附着点顺序（kAttachIndex，1基；改值即附着到该点）
		QLineEdit* _posEdit = nullptr;                 //位置"X Y"（kPosX/kPosY，数据坐标，空格分隔单框）
		QCheckBox* _minimizeCheck = nullptr;           //最小化（勾选后显示半径/颜色）
		QSpinBox* _minimizeRadiusSpin = nullptr;       //最小化半径（占位）
		mCurveColorButton* _minimizeColorBtn = nullptr;  //最小化颜色（占位）
        mCurveFontButton* _fontBtn = nullptr;          //字体设置（kFont，弹窗选择，渲染下一期）
		QComboBox* _alignCombo = nullptr;              //对齐方式（占位：左/中/右）
		QComboBox* _borderWidthCombo = nullptr;        //边框线宽（kBorderWidth：线宽样本图标1..4px）
		QComboBox* _borderColorCombo = nullptr;        //边框颜色（kBorderMode：跟随曲线颜色/自定义，附着视图时整行隐藏）
		mCurveColorButton* _borderColorBtn = nullptr;  //选择边框颜色（kBorderColor，附着视图时常显）
		QCheckBox* _autoPosCheck = nullptr;            //自动定位（kAutoPos，勾选后显示策略/偏移）
		QComboBox* _autoPosToleranceCombo = nullptr;   //自动定位策略（不重叠/各锚位，"不重叠"时隐藏偏移）
		QDoubleSpinBox* _autoPosOffsetSpin = nullptr;  //自动定位偏移（kAutoOffset，策略非"不重叠"时显示）
		QComboBox* _bgColorCombo = nullptr;            //背景颜色（默认=无效色/自定义）
		mCurveColorButton* _bgColorBtn = nullptr;      //选择背景颜色（kBgColor，自定义时显示）
		QComboBox* _textColorCombo = nullptr;          //文本颜色（默认=无效色/自定义）
		mCurveColorButton* _textColorBtn = nullptr;    //选择文本颜色（kColor，自定义时显示）
	};

	//坐标轴快捷编辑页（XAxis/YAxis/YAxis2 三轴共用）：内容对齐
	//参考软件的坐标轴属性编辑器，与参数栏轴页（mAxisPropPage）同构共键——
	//标准[可见性·名称可见性·名称·文本·类型(只读回显:垂直/水平)] /
	//显示[标签字体设置(kTitleFont，弹窗选择)·颜色] /
	//缩放设置[缩放方式·参考值(kDbRef,dB20/dB10 共用一项仅 dB 模式显示,行标签随
	//模式切换)·计权(kWeightMode,仅 dB 模式显示)·最小值·最大值（改范围即退出
	//自动范围）] /
	//坐标轴刻度设置[格式(kLabelFormat:自动/科学计数法/定点格式) + 仅 linear 轴:
	//刻度生成方式(kTickGenMode:均匀分步法→刻度总数(kTickCount)/固定步长法→固定
	//步长(kMajorStep))/每主刻度网格数(kMinorCount) + 仅非 linear 轴(log/dB):
	//每十倍刻度数(kDecadeTickCount)/每十倍网格线数(kDecadeMinorCount)] +
	//精度(kPrecision)/刻度字体设置(kLabelFont，弹窗选择) 恒显]。
	//条目多于弹窗默认高度（280x400），整页放滚动区。
	//入口：双击纵/横轴标题（标题变主题蓝+加粗）
	class POSTCURVE_EXPORT mAxisEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mAxisEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//条件行显隐：缩放设置（参考值/计权仅 dB 模式显示，参考值行标签随模式
		//切换）+ 刻度设置（刻度生成方式组仅 linear，每十倍组仅非 linear）
		void updateRowStates();
		//缩放/平移联动：自动范围轴的最小值/最大值随当前显示范围刷新（阻塞
		//信号回显，不退出自动）
		void onAxisDisplayRangeChanged(int axisNodeId);
		//把自动范围轴的最小值/最大值回显为引擎当前显示范围（重建落定后延后
		//一拍调用，避免拿到旧刻度空间的范围）
		void echoDisplayRange();
		QCheckBox* _visibleCheck = nullptr;          //可见性（kVisible）
		QCheckBox* _nameVisibleCheck = nullptr;      //名称可见性（kTitleVisible，名称=轴标题）
		QLineEdit* _nameEdit = nullptr;              //名称（kName，树显示名，同参数栏轴页）
		QLineEdit* _textEdit = nullptr;              //文本（kTitle）
		QComboBox* _typeCombo = nullptr;             //类型（只读回显：X轴=水平，Y/Y2轴=垂直）
		mCurveFontButton* _labelFontBtn = nullptr;   //标签字体设置（kTitleFont，弹窗选择）
		mCurveColorButton* _colorBtn = nullptr;      //颜色（kTitleColor，名称颜色）
		mCurveFoldSection* _rangeFold = nullptr;     //缩放设置分组（条件行显隐用）
		QComboBox* _scaleModeCombo = nullptr;        //缩放方式（kScaleMode：0 Linear 1 Log 2 dB20 3 dB10）
		QLabel* _dbRefLabel = nullptr;               //参考值行标签（文本随缩放方式切换 dB20/dB10）
		QDoubleSpinBox* _dbRefSpin = nullptr;        //参考值（kDbRef，仅 dB 模式显示）
		QComboBox* _weightCombo = nullptr;           //计权（kWeightMode：0无/1A/2B/3C/4U，U=unweighted 与无等效，仅 dB 模式显示）
		QDoubleSpinBox* _minSpin = nullptr;          //最小值（kMin，改范围即退出自动）
		QDoubleSpinBox* _maxSpin = nullptr;          //最大值（kMax，同上）
		mCurveFoldSection* _tickFold = nullptr;      //坐标轴刻度设置分组（条件行显隐用）
		QComboBox* _formatCombo = nullptr;           //格式（kLabelFormat：0自动/1科学计数法/2定点格式）
		QComboBox* _tickGenCombo = nullptr;          //刻度生成方式（kTickGenMode：0均匀分步法/1固定步长法，仅 linear 轴显示）
		QDoubleSpinBox* _tickCountSpin = nullptr;    //刻度总数（kTickCount，均匀分步法）
		QDoubleSpinBox* _fixedStepSpin = nullptr;    //固定步长（kMajorStep，固定步长法）
		QSpinBox* _gridPerMajorSpin = nullptr;       //每主刻度网格数（kMinorCount，仅 linear 轴显示）
		QSpinBox* _decadeTickSpin = nullptr;         //每十倍刻度数（kDecadeTickCount，仅非 linear 轴显示）
		QSpinBox* _decadeGridSpin = nullptr;         //每十倍网格线数（kDecadeMinorCount，仅非 linear 轴显示）
		QSpinBox* _precisionSpin = nullptr;          //精度（kPrecision）
		mCurveFontButton* _tickFontBtn = nullptr;    //刻度字体设置（kLabelFont，弹窗选择）
	};

	//图例快捷编辑页：标准[可见性·自动定位(勾选隐藏图例位置行)·
	//图例位置(9选项：图内四角/图内-用户自定义/图外四向)·X坐标/Y坐标(选"图内-
	//用户自定义"才显示，画布百分比)] + 显示[边框样式(图形下拉5式)·边框颜色·
	//字体设置(kLegendFont，弹窗选择)·引线位置(左侧/下方)·背景填充色·曲线文本色
	//(跟随曲线颜色/自定义)·文本颜色(选"自定义"才显示)]。
	//入口：双击图例空白区域 / 右键图例"编辑"（图例项双击/右键仍是曲线编辑器）。
	//图外四向位置渲染待引擎外置图例方案（模型与UI先行）
	class POSTCURVE_EXPORT mLegendEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mLegendEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//条件行显隐：自动定位勾选->隐藏图例位置行；位置选"图内-用户自定义"->
		//显示X/Y坐标行；曲线文本色选"自定义"->显示文本颜色行
		void updateRowStates();
		QCheckBox* _visibleCheck = nullptr;          //可见性（kVisible）
		QCheckBox* _autoPosCheck = nullptr;          //自动定位（kAutoPosition，tooltip说明避让策略）
		QComboBox* _positionCombo = nullptr;         //图例位置（kPosition 0..8）
		QDoubleSpinBox* _xSpin = nullptr;            //X坐标（kPosX，画布百分比0-100）
		QDoubleSpinBox* _ySpin = nullptr;            //Y坐标（kPosY）
		mCurveBorderStyleCombo* _borderStyleCombo = nullptr; //边框样式（kBorderStyle 0..4，图形下拉）
		mCurveColorButton* _borderColorBtn = nullptr;     //边框颜色（kBorderColor）
		mCurveFontButton* _fontBtn = nullptr;        //字体设置（kLegendFont，弹窗选择）
		QComboBox* _leaderPosCombo = nullptr;        //引线位置（kLeaderPos：0左侧 1下方）
		mCurveColorButton* _bgColorBtn = nullptr;    //背景填充色（kBgColor）
		QComboBox* _textColorModeCombo = nullptr;    //曲线文本色（kTextColorMode：0跟随曲线颜色 1自定义）
		mCurveColorButton* _textColorBtn = nullptr;  //文本颜色（kTextColor，自定义模式生效）
		//条件显隐走"块"（addBlock 挂 VBox 层，隐藏完全收起；QFormLayout 行隐藏仍占位）
		QWidget* _positionBlock = nullptr;      //图例位置+X/Y坐标块（勾选自动定位整体隐藏）
		QWidget* _xyBlock = nullptr;            //X/Y坐标块（位置选"图内-用户自定义"时显示）
		QWidget* _textColorBlock = nullptr;     //文本颜色行块（曲线文本色选"自定义"时显示）
	};

	//基准线快捷编辑页：
	//标准[可见性/名称/标签/图层(占位)/位置/关联轴/类型(只读回显:垂直/水平)] +
	//显示[字体设置(kLabelFont，弹窗选择，渲染下一期)/标签位置(9宫格，默认右中)/
	//线型/线条颜色/线宽]，与参数栏 mBaselinePropPage 行集一致。全部属性绑定
	//Baseline 节点提交，改动经增量刷新即时生效（保缩放）；标签是线上文字
	//（渲染下一期，模型与编辑UI先行）。
	//入口：双击基准线 / 右键基准线"编辑"（弹窗落点=视窗左上角）
	class POSTCURVE_EXPORT mBaselineEditorPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mBaselineEditorPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		QCheckBox* _visibleCheck = nullptr;          //可见性（kVisible，取消即隐藏该线）
		QLineEdit* _nameEdit = nullptr;              //名称（kName，树显示名）
		QLineEdit* _labelEdit = nullptr;             //标签（kLabel，线上文字，渲染下一期）
		QLineEdit* _layerEdit = nullptr;             //图层（kLayer，占位）
		QDoubleSpinBox* _valueSpin = nullptr;        //位置（kValue，横向=Y值 纵向=X值）
		QComboBox* _linkCombo = nullptr;             //关联轴（kLink+kOrientation：X侧纵向/Y侧横向）
		QLineEdit* _typeEdit = nullptr;              //类型（只读回显 垂直/水平，置灰）
		mCurveFontButton* _fontBtn = nullptr;        //字体设置（kLabelFont，弹窗选择，渲染下一期）
		QComboBox* _labelPosCombo = nullptr;         //标签位置（kLabelPos：9宫格，默认右中）
		mCurvePenStyleCombo* _lineStyleCombo = nullptr; //线型（kLineStyle，Qt::PenStyle 1..4）
		mCurveColorButton* _colorBtn = nullptr;      //线条颜色（kColor，默认蓝）
		mCurveLineWidthCombo* _lineWidthCombo = nullptr; //线宽（kLineWidth，1..4px图标档）
	};
}
