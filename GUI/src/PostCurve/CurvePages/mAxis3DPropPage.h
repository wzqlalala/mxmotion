#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图 x/y/z 轴壳参数栏属性页（chartType=5，R28 入树壳节点）
//
// 三壳（AxisX3DData/AxisY3DData/AxisZ3DData）共用一页三实例，构造参轴下标
// 0=X 1=Y 2=Z 区分（仿 mCurveDataPropPage 构造参先例；注册见
// mCurvePropPageManager::registerDefaultPages，弹窗族不注册——壳无双击/右键
// 弹窗入口，showEditorPopup 回退参数栏页=预期行为）。
//
// 行集口径（两次决策留痕）：
//   首拍 20261111"只放生效项"（ce3ff43e：3 组 7 行、无占位行）——已被用户
//   20261111 二次拍板推翻："将图中内容全填入，（不用实现该部分内容）"，即
//   参考软件截图四组全部行填入页面，三维轴真正生效的字段照旧接功能，其余
//   行一律置灰占位、不连 commit、不进回填（对齐"极坐标图置灰占位"先例，
//   后续迭代接活 Plot3D API）。现行集=四组 20 行（序=截图）：
//   ▾坐标轴与范围（6 行）：坐标轴(R43 翻活三选项下拉 X轴/Y轴/Z轴,用户切换=
//     setCurrentNodeId 联动树选中同曲线对应轴壳)/应用到(R43 翻活 所选坐标轴/
//     全部轴——会话级 UI 态不进模型,控制编辑作用范围)/轴名称(生效 kAxis3DLabel,
//     恒单轴不扇出)/刻度类型(R43 翻活 Linear/Logarithmic/dB20/dB10=二维同款
//     文案顺序,kAxis3DScaleType)/最小值/最大值(一行双控件容器,生效
//     kAxis3DMin/kAxis3DMax,互钳与 autoFit 置灰联动保留)/自动适配(生效
//     kAxis3DAutoFit)+适配范围(置灰占位,一行双勾选)
//   ▾刻度与文字（6 行）：刻度方式(置灰占位"每轴刻度数")/主刻度数(生效
//     kAxis3DTickCount,[2,100])/刻度格式(置灰占位,"自动格式"+3 一行双控件)/
//     轴颜色(置灰占位 深蓝)/每刻度网格数(置灰占位 2)/字体大小(置灰占位 12)
//   ▾网格与外观（4 行）：网格样式(R44 翻活 三维框/线框两选项,图表级
//     kChart3DGridStyle 经 commitToChart——线框=宿主侧三轴旗标关断复用引擎
//     setGridVisible 既有语义,引擎零新 API)/网格线颜色(R44 翻活 bar 色条,
//     图表级 kChart3DBackdropGridColor)/背景颜色(R44 翻活 bar 色条,图表级
//     kChart3DBackdropColor;两色行线框期保持可用=休眠状态,切回三维框恢复
//     消费)/显示网格(生效 kAxis3DGridVisible,整行勾选,恒单轴不扇出;
//     线框模式下整行置灰防假编辑——引擎旗标被线框语义压住,切回恢复)
//   ▾坐标轴长度（4 行,图表级占位组,三壳页同构,不绑定任何键）：手动轴长度
//     (置灰占位)/X轴长度/Y轴长度/Z轴长度(三行置灰占位 1.0,截图恒显三行)
//
// 应用范围（R43 用户拍板）：「应用到」=全部轴时仅"范围与刻度类"五键
// （kAxis3DScaleType/kAxis3DMin/kAxis3DMax/kAxis3DAutoFit/kAxis3DTickCount）
// 经 commitScoped 宏提交扇出三兄弟壳（一次撤销整组还原）；轴名称/显示网格
// 恒单轴。档位为会话级 UI 态：文件局部静态，三实例（参数栏按壳类型各缓存
// 一页，同时存活）共享同一范围，不进模型不序列化。
//
// 自动适配交互（对齐 2D"退出自动回写引擎现值"策略）：勾选=范围按全部曲线
// 数据并集现算，最小/最大值置灰回显当前生效范围（m3DChartView::
// queryAxis3DDisplayRange 静态查询，纯文档计算不依赖视口）；取消=先查当前
// 生效范围回填两 spin 并一并提交 min/max+autoFit=false（视图无跳变、手动
// 起点=当前所见），查询失败（无曲线/无数据）只提交 autoFit=false、回显模型
// 休眠值。提交经基类 commit→命令栈→壳重定向→渲染重建，撤销/重做免费获得。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "mCurvePropPageBase.h"
#include "../CurveWidgets/mCurveFoldSection.h"   //mCurveFoldSection = 共享 FoldSection 别名（成员指针用）

//QT
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>

namespace PostCurve
{
	class mCurveColorButton;   //实心色块按钮（CurveWidgets，成员指针用）

	class POSTCURVE_EXPORT mAxis3DPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		//axisIndex：轴下标 0=X 1=Y 2=Z（壳节点类型→axis3d 数组下标，与文档
		//重定向映射同表）
		explicit mAxis3DPropPage(int axisIndex, QWidget* parent = nullptr);
		void loadFromModel() override;
	private:
		void buildUi();
		//取消自动适配：查当前生效范围回填两 spin 并一并提交 min/max +
		//autoFit=false；查询失败只提交 autoFit=false（模型休眠值兜底）。
		//全部轴档（R43）：宏内逐轴提交各自的当前生效范围+autoFit=false，
		//一次撤销整组还原
		void commitManualRange();
		//范围提交（R43）：所选坐标轴 或 prop 不在扇出键范围 → 基类 commit
		//原样；全部轴 且 prop∈{kAxis3DScaleType/kAxis3DMin/kAxis3DMax/
		//kAxis3DAutoFit/kAxis3DTickCount}（用户拍板"仅范围与刻度类"）→ 宏
		//"修改三维轴"包本曲线三兄弟壳逐个 setNodeProperty（一次撤销整组还原；
		//某壳缺失降级为可用壳集合，至少提交本壳）。守卫与基类 commit 一致
		void commitScoped(const QString& prop, const QVariant& value);
		//图表级提交助手（R44）：网格样式/网格线颜色/背景颜色三行=图表级设置，
		//提交目标=本壳所属图表节点（任意轴壳页编辑全图生效）。守卫与基类
		//commit 一致（_loading/_nodeId<0 静默）；chartNodeId 查不到（<=0）静默
		//返回。**不进 commitScoped 扇出键集**——三行不受「应用到」约束（用户
		//拍板"三行不随应用到扇出"）
		void commitToChart(const QString& prop, const QVariant& value);
		//本壳所属曲线的三兄弟壳 id（X/Y/Z 类型序；某壳缺失降级为可用壳集合，
		//本壳自身恒在列保底——异常骨架不丢当前编辑）
		QList<int> siblingShellIds() const;
		int _axisIndex = 0;                          //轴下标 0=X 1=Y 2=Z
		//--- 坐标轴与范围 ---
		mCurveFoldSection* _axisRangeFold = nullptr; //坐标轴与范围分组（原"标准"+"缩放设置"并组）
		QComboBox* _axisCombo = nullptr;             //坐标轴（R43 翻活三选项下拉，用户切换=setCurrentNodeId 联动树选中对应壳；loadFromModel 阻塞信号回显壳下标）
		QComboBox* _applyCombo = nullptr;            //应用到（R43 翻活 所选坐标轴/全部轴；会话级 UI 态不进模型，三实例共享静态档）
		QLineEdit* _labelEdit = nullptr;             //轴名称（kAxis3DLabel，生效，空=渲染不显示标题；恒单轴）
		QComboBox* _tickTypeCombo = nullptr;         //刻度类型（R43 翻活 kAxis3DScaleType，Linear/Logarithmic/dB20/dB10=二维同款）
		QDoubleSpinBox* _minSpin = nullptr;          //最小值（kAxis3DMin，生效，autoFit 勾选时置灰回显现值）
		QDoubleSpinBox* _maxSpin = nullptr;          //最大值（kAxis3DMax，生效，同上）
		QCheckBox* _autoFitCheck = nullptr;          //自动适配（kAxis3DAutoFit，生效，勾选时范围按并集现算）
		QCheckBox* _fitRangeCheck = nullptr;         //适配范围（置灰占位，不勾）
		//--- 刻度与文字 ---
		mCurveFoldSection* _tickTextFold = nullptr;  //刻度与文字分组
		QComboBox* _scaleModeCombo = nullptr;        //刻度方式（置灰占位"每轴刻度数"）
		QSpinBox* _tickCountSpin = nullptr;          //主刻度数（kAxis3DTickCount，生效，[2,100] 与渲染端 qMax(2,值) 同口径）
		QComboBox* _fmtCombo = nullptr;              //刻度格式·格式（置灰占位"自动格式"）
		QSpinBox* _fmtSpin = nullptr;                //刻度格式·位数（置灰占位 3）
		mCurveColorButton* _axisColorBtn = nullptr;  //轴颜色（置灰占位，深蓝）
		QSpinBox* _minorPerTickSpin = nullptr;       //每刻度网格数（置灰占位 2）
		QSpinBox* _fontSpin = nullptr;               //字体大小（置灰占位 12）
		//--- 网格与外观 ---
		mCurveFoldSection* _gridAppearFold = nullptr; //网格与外观分组
		QComboBox* _gridStyleCombo = nullptr;        //网格样式（R44 翻活 三维框/线框，图表级 kChart3DGridStyle 经 commitToChart）
		mCurveColorButton* _gridColorBtn = nullptr;  //网格线颜色（R44 翻活 bar 色条，图表级 kChart3DBackdropGridColor；线框期休眠可用）
		mCurveColorButton* _bgColorBtn = nullptr;    //背景颜色（R44 翻活 bar 色条，图表级 kChart3DBackdropColor；线框期休眠可用）
		QCheckBox* _gridCheck = nullptr;             //显示网格（kAxis3DGridVisible，生效，本壳对应轴，整行勾选；线框模式整行置灰防假编辑）
		//--- 坐标轴长度（图表级占位组，不绑定任何键） ---
		mCurveFoldSection* _lenFold = nullptr;       //坐标轴长度分组
		QCheckBox* _manualLenCheck = nullptr;        //手动轴长度（置灰占位，不勾）
		QDoubleSpinBox* _lenXSpin = nullptr;         //X轴长度（置灰占位 1.0，三壳页同构恒显）
		QDoubleSpinBox* _lenYSpin = nullptr;         //Y轴长度（置灰占位 1.0）
		QDoubleSpinBox* _lenZSpin = nullptr;         //Z轴长度（置灰占位 1.0）
	};
}
