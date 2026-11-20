#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// NVH 工具集 - 选择贡献项对话框（结果研究·部分和，openspec §7.2/design D8）
//
// 部分和"显示"流程先弹本框确认排除集合（design D8：先弹贡献项对话框（确认
// 排除集合）→ 按 D2 数据函数合成 → 产出）。
//
// 表格两列（原型 2026-11-11 实测：整窗 480×468、行号竖头灰带宽 41、行节距 26、
// 表头带 28、%列宽 200）：Contributor（"Mode N : <频率> Hz"，EIGEN MODES 模态
// 频率表）+ % To Exclude（可编辑列默认取页内"去除比例(%)"值——页面语义：选中
// 模态按该比例缩放后剔除/保留，design D2 partialSumSeries 口径）。选择语义=
// 行选中（ExtendedSelection，spec「选中 Mode N」；无勾选框列——原型即无）。
// % 列纯文本观感（1 位小数右对齐，原型 100.0），双击/编辑键经 delegate 现出
// QDoubleSpinBox 编辑器（ThemeManager 有 QDoubleSpinBox 整树重发分支，控件级
// QSS 文本化三件套会被抹——ui-spec §2/§3，故按需创建编辑器而非常驻控件）；
// 模态号存 Contributor 单元格 Qt::UserRole、% 值存单元格文本（ui-spec §4：
// 存档落 item、控件只做呈现与交互）。
//
// 2026-11-17 第六批 D1 裁定：表=调用方传入子集（页内模态列表多选集过滤后的
// pickedMap，调用侧空选警退），preselectedModes=传入子集全部键（整表预选）——
// 对话框机制零改动，仅内容口径换源；preselectedModes 语义即"表内要预选的键"
// （旧注释"页内模态列表选中集"作废——选中过滤已上移调用侧）。
//
// 形态：MechMainWindow::mFramelessDialog 白卡家族（标题栏常数与主钮接线照
// mNVHDisplayOptionsDlg 同款），模态 exec，取消不改、确定经 selectedRatios
// 带出选中模态→% 映射；preselectedModes=页内模态列表选中集（回显预选——
// spec 场景「选中 Mode N → 显示 → 对话框确定」，行选中下不预选则空确定）。
// =============================================================================

#include "mFramelessDialog.h"

#include <map>
#include <vector>

class QPushButton;
class QTableWidget;

namespace PostWindow {

class mNVHContributorPickDlg : public MechMainWindow::mFramelessDialog
{
	Q_OBJECT
public:
	//modeFreqs=传入子集模态号→频率（Hz，D1 裁定=页内多选集过滤后的 pickedMap）；
	//defaultPct=% To Exclude 列默认值（页内"去除比例(%)"带入）；
	//preselectedModes=表内要预选的键（D1 裁定=传入子集全部键，整表预选）
	mNVHContributorPickDlg(const std::map<int, double>& modeFreqs,
		double defaultPct,
		const std::vector<int>& preselectedModes = std::vector<int>(),
		QWidget* parent = nullptr);

	//确定时的选中模态→%To Exclude（0~100）映射（取消路径不取）
	std::map<int, double> selectedRatios() const;

private slots:
	void onAccept();

private:
	void buildUi(const std::map<int, double>& modeFreqs,
		const std::vector<int>& preselectedModes);

	double _defaultPct = 100.0;   //% 列默认值（页内去除比例，编辑器与解析兜底）
	QTableWidget* _table = nullptr;
	QPushButton* _okBtn = nullptr;
	QPushButton* _cancelBtn = nullptr;
};

} // namespace PostWindow
