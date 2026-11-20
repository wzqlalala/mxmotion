#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图注释节点参数栏属性页（chartType=5 的 Annotation 子节点，R34 一期
// 建、R42 二期翻活画布配套行；R46 修订——注释管理组随创建入口改模式打点退役）
//
// 三维图（chartType=5）注释=普通树节点（模型键既有），属性经命令栈入库、可撤销、
// 随 .mxses 会话序列化。**R46 修订**：注释创建入口=Ribbon"3D曲线-标注-注释"按钮
// 检出三维注释模式后画布内容点击（对齐二维曲线注释交互，牵引线+注释框，
// kAttachMode=0 内容锚定带牵引线）——参数栏"注释管理"组（显示注释/注释名下拉/
// 注释列表/新增·剪切·复制·粘贴）整组退役：注释的新增=画布打点、显隐=树勾选、
// 导航=树选中/画布点击注释项（setCurrentNodeId 全链既有），本页聚焦单注释的
// 内容与样式编辑。剪贴板 API（copyAnnotation/cutAnnotation/pasteAnnotation）
// 与共享例程 addWindowAnnotation 仍在控制器（粘贴/存量会话路径可达）。
//
// 行集与生效/占位口径（三组 9 行——R46 去注释管理组后的行集；生效/占位总账=
// 生效 8 件、置灰占位 {U}{V}{W} 三钮+附着方式下拉）：
//   ▾文字（3 行）：文本编辑区(生效 kText，QPlainTextEdit——失焦且文本有变化
//     才 commit 一次，防逐键压撤回栈；"无变化不发"参照 mCurveTextButton 同值
//     不压栈口径)/(X)(Y)(Z)(U)(V)(W)(Label) 七按钮一行(R42 二期 {X}{Y}{Z}{Label}
//     四钮生效——点击=向文本编辑区光标处 insertPlainText 插入 token 文本
//     "{X}"等，不直接 commit 维持失焦提交口径；{U}{V}{W} 维持置灰占位——3D
//     模板不提供 U/V/W 求值字面保留；按钮可收缩化+tooltip 带完整 token 兜底)/
//     对齐(生效 kAlign：0左 1中 2右，注释侧语义，下拉文案 左对齐/居中/右对齐)
//   ▾属性（5 行全生效）：显示边框(生效 kBorderShow，画布注释项边框显隐)/
//     文字颜色(生效 kColor，B02 改通栏色条——mCurveColorButton bar 模式，不设
//     "默认/自定义"组合)/字体设置(生效 kFont，mCurveFontButton 点击弹 2D 同款
//     QFontDialog"选择字体"——用户拍板不单列字体/字号/粗体/斜体四行)/
//     背景颜色(生效 kBgColor，bar 通栏色条，同文字颜色口径)/
//     边框线宽度(生效 kBorderWidth，mCurveLineWidthCombo 1..4px 图标档，提交=
//     档位px、回显就近取整钳 1..4，与 2D 注释页同映射)
//   ▾附着对象（1 行）：附着方式(置灰回显"三维"——R42 修订 3D 注释语义=三维
//     数据空间锚点（锚点随相机投影移动）；R46 起模型两档位 0=内容锚定（模式
//     打点创建，画布渲染带牵引线）/1=窗口（旧直接创建入口存量，框即锚点），
//     两档位均锚定三维、无第二附着语义，可交互切换无意义，按占位行纪律不连
//     commit/不进回填)
//
// 按钮可收缩（B02 根治"窄面板整行裁切"）：行按钮（生效四模板钮）显式
// setMinimumWidth(1) 覆盖 minimumSizeHint + 紧凑横向内边距（行高不缩）——
// 按钮随面板宽度实时收缩，文字极端窄时截断由 tooltip 兜底（用户拍板"控件随
// 大小变化而变化"）；不引入滚动容器（全部参数栏页共同约束=页面 minimumWidth
// 自适配 dock 宽）。
//
// 回显纪律：生效行回显全程 QSignalBlocker（R30 教训：回显触发提交=毒化模型）；
// 置灰占位控件（{U}{V}{W}/附着方式）构造即禁用固定、不连 commit/信号、不进
// loadFromModel；QPushButton 置灰观感自身样式表兜底（FoldSection :disabled 规则
// 只覆盖 Combo/Line/Spin）。页面骨架=pageLayout+addStretch，不设边距。
//
// 列表维护：本节点 kName/属性变化经基类 onNodePropertyChanged→loadFromModel
// 自动刷新；绑定节点被删除时页面随既有 sig_nodeRemoved 链路自动关闭/解绑
// （基类既有行为）。树节点显示/绿√图标/重命名/筛选均走既有通用机制，零额外
// 改动。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "mCurvePropPageBase.h"
#include "../CurveWidgets/mCurveFoldSection.h"   //mCurveFoldSection = 共享 FoldSection 别名（成员指针用）

//QT
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QVector>

namespace PostCurve
{
	class mCurveColorButton;       //实心色块按钮（CurveWidgets，成员指针用）
	class mCurveFontButton;        //字体设置按钮（点击弹"选择字体" QFontDialog）
	class mCurveLineWidthCombo;    //线宽图标下拉（1..4px 档）

	class POSTCURVE_EXPORT mAnnotation3DPropPage : public mCurvePropPageBase
	{
		Q_OBJECT
	public:
		explicit mAnnotation3DPropPage(QWidget* parent = nullptr);
		void loadFromModel() override;

	protected:
		//文本编辑区失焦提交（QPlainTextEdit 无 editingFinished，事件过滤器抓
		//FocusOut：与模型现值不同才 commit 一次）
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		void buildUi();

		mCurveFoldSection* _textFold = nullptr;      //文字分组
		QPlainTextEdit* _textEdit = nullptr;         //文本编辑区（kText，生效，失焦且有变化才提交）
		QVector<QPushButton*> _tmplBtns;             //(X)(Y)(Z)(U)(V)(W)(Label) 七按钮一行（生效 R42={X}{Y}{Z}{Label} 四钮
		                                             //光标处插 token；置灰占位={U}{V}{W}；可收缩）
		QComboBox* _alignCombo = nullptr;            //对齐（kAlign，生效：0左 1中 2右）

		mCurveFoldSection* _propFold = nullptr;      //属性分组
		QCheckBox* _borderCheck = nullptr;           //显示边框（生效 R42：kBorderShow，画布注释项边框显隐）
		mCurveColorButton* _textColorBtn = nullptr;  //文字颜色（kColor，生效，bar 通栏色条）
		mCurveFontButton* _fontBtn = nullptr;        //字体设置（kFont，生效，2D 同款字体弹窗）
		mCurveColorButton* _bgColorBtn = nullptr;    //背景颜色（kBgColor，生效，bar 通栏色条）
		mCurveLineWidthCombo* _borderWidthCombo = nullptr; //边框线宽度（kBorderWidth，生效，1..4px 档）

		mCurveFoldSection* _attachFold = nullptr;    //附着对象分组
		QComboBox* _attachCombo = nullptr;           //附着方式（置灰回显"三维"，R46 起模型 0/1 两档位均不连 commit）
	};
}
