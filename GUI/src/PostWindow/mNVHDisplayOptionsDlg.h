#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// NVH 工具集 - 显示选项对话框（模态贡献量分析，openspec §6.1/design D6）
//
// 入口：三页签"显示选项"钮共用（mPostNVHContributionPanel::onDisplayOptions
// 按调用钮路由五语境，openspec context-display-options-state：加载=NVHCtx_Load、
// 研究=NVHCtx_Research、结果显示页按当前图表类型三档）；模态 exec，确定经
// ctxScaleResult()/commonResult() 双结果取出（应用到后续生成），取消不改。
//
// 行结构（spec 显示选项要求）：
//   - 显示：不显示相位/显示相位两态（控制响应曲线单/双形态）；语境=
//     NVHCtx_ShowPolar 时联动为 非首尾相接/首尾相接（读值同为公共
//     displayPhase）；
//   - 缩放：Linear/Logarithmic/% Plot/dB10/dB20 五单选，dB10/dB20 各内联
//     参考值科学计数框（家族件 PostCurve::mCurveSciDoubleSpin，曲线属性页
//     dB 参考值同件同口径 1e-12~1e9；默认 1.0，可用性随所在行单选钮——
//     不随选中态切换）；
//     语境预选档/置灰集按 D2 规格表（Load=Log 全可选、ShowBar=%Plot 档集
//     {Linear,%Plot}、ShowPolar=Linear 仅一档、Show2D=Log 全可选、
//     Research=%Plot 全可选）；缩放状态按语境独立存取，回显原样无折算；
//   - 加权：默认勾选 + A/B/C/U 下拉 + 设为全局；ShowBar/ShowPolar 语境
//     勾选框与计权下拉置灰（灰显只读，存储计权值仍生效——2026-11-13
//     原型 D30②）；
//   - 单位：勾选启用 + 时间/长度/力三下拉（一期仅存档，换算语义随真机复核）；
//   - 绘图布局：在当前视图页上打开新窗口（默认）/在当前激活的窗口打开/在
//     新视图页上打开新窗口 + 设为全局。
// "设为全局"标记记录在公共行 struct（weightingGlobal/layoutGlobal），三页签
// 同读一份状态（§6.3 项级同步）。
//
// 形态：MechMainWindow::mFramelessDialog 家族基类（自绘标题栏/白卡/主题代数
// 标记路径内建，内容区走 applyDialogTheme 上色），DlgGeom::bind 记忆本次
// 运行内几何；控件不逐个 setStyleSheet（样式随主题/父级继承，ui-spec §2）。
// =============================================================================

#include "mPostNVHDisplayOptions.h"

#include "mFramelessDialog.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QPushButton;
class QRadioButton;

namespace PostCurve { class mCurveSciDoubleSpin; }

namespace PostWindow {

class mNVHDisplayOptionsDlg : public MechMainWindow::mFramelessDialog
{
	Q_OBJECT
public:
	//ctx=语境（NVHDisplayCtx 五档，驱动显示行文案与缩放档/加权行置灰）；
	//ctxScale=该语境缩放三元组（回显底稿，原样无折算）；common=公共行
	//(显示/加权/单位/绘图布局/设为全局，回显底稿)
	mNVHDisplayOptionsDlg(NVHDisplayCtx ctx, const NVHContextScale& ctxScale,
		const NVHDisplayOptions& common, QWidget* parent = nullptr);

	//确定时的语境缩放（取消路径不取——构造入参拷贝，未 onAccept 不变）
	const NVHContextScale& ctxScaleResult() const { return _ctxScale; }
	//确定时的公共行（同上）
	const NVHDisplayOptions& commonResult() const { return _common; }

private slots:
	void onAccept();   //控件态回写两副本后 accept

private:
	void buildUi();

	NVHContextScale _ctxScale;     //语境缩放工作副本（构造拷贝入参，确定时回写）
	NVHDisplayOptions _common;     //公共行工作副本（同上）
	NVHDisplayCtx _ctx = NVHCtx_Load;

	QComboBox* _dispCombo = nullptr;   //不显示相位/显示相位（极坐标语境联动改文案）

	QButtonGroup* _scaleGroup = nullptr;
	QRadioButton* _scaleLinearRadio = nullptr;
	QRadioButton* _scaleLogRadio = nullptr;
	QRadioButton* _scalePercentRadio = nullptr;
	QRadioButton* _scaleDb10Radio = nullptr;
	QRadioButton* _scaleDb20Radio = nullptr;
	PostCurve::mCurveSciDoubleSpin* _db10RefSpin = nullptr;   //dB10 参考值（家族件科学计数框，默认 1.0，随档启用）
	PostCurve::mCurveSciDoubleSpin* _db20RefSpin = nullptr;   //dB20 参考值（同上）

	QCheckBox* _weightCheck = nullptr;       //加权启用（默认勾选）
	QComboBox* _weightCombo = nullptr;       //A/B/C/U（index+1=weightMode）
	QCheckBox* _weightGlobalCheck = nullptr; //设为全局

	QCheckBox* _unitsCheck = nullptr;        //单位启用
	QComboBox* _timeCombo = nullptr;         //时间(s)
	QComboBox* _lengthCombo = nullptr;       //长度(mm)
	QComboBox* _forceCombo = nullptr;        //力(N)

	QComboBox* _layoutCombo = nullptr;   //0 当前视图页新窗口/1 当前激活窗口/2 新视图页
	QCheckBox* _layoutGlobalCheck = nullptr;     //设为全局

	QPushButton* _okBtn = nullptr;
	QPushButton* _cancelBtn = nullptr;
};

} // namespace PostWindow
