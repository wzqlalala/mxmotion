#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 频段统计计数核心（纯逻辑，无 GUI 依赖，可无头单测）
//
// 曲线统计导出的第二张表"频段统计"（add-curve-stats-band-count）：把已收集的
// 超限频段（mCurveStatisticsDlg 统计链路产出的 OverrunBand）按用户填写的频率
// 范围分段计数。2026-11-04 口径修正（真机反馈两轮）：
// ① 按组分段：矩阵布局每组有单独频段——组段宽 =(fMax−fMin)/组列数，列头逐组
//    各自生成，曲线按 columnKey 归组、计数只落本组槽位；行式布局无组概念，
//    维持全局等宽 N 段（单组口径）；
// ② 行合并：矩阵布局同明细行键（工况(方向)）的曲线合成一条频段行，各组分块
//    写入同一行各自的槽位区间（组间槽位不重叠，合并无碰撞）——频段表行数/行
//    标签/行序与明细表一致（不再逐曲线×组数翻倍）；行式布局仍每曲线一行。
// 归段公式 idx = clamp(floor((f1−f_min)/width), 0, N−1)：按段首频率 f1 单点
// 归段，跨段界频段不重复计入多段；f1==f_max 时 floor 得 N，clamp 收进末段
// （末段右端闭合）。
// 空槽哨兵：counts 槽位 =-1 表示该行该组缺席（该明细行无此组曲线），JSON 层
// 拼空串、脚本渲染留空带框格；未知组（columnKey 不在 groups，理论不发生）
// 曲线不落任何槽位。
// 供 mCurveStatisticsDlg::collectStatistics 尾部装配导出 JSON 的 band_stats
// 顶层对象用；渲染脚本按 marks 选样式、按空串留空，不重复任何业务判定。
// =============================================================================

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

#include "PostCurveGlobal.h"

namespace CurveStatsBand
{
	//一条曲线的频段计数输入：明细行键 + 组键（矩阵） + 超限频段段首频率列表
	struct CurveBands
	{
		QString label;             //行式=工况列；矩阵=明细行键（工况(方向)，同键合并）
		QString columnKey;         //矩阵=组键（对象号，归属组定位）；行式=空
		QList<double> bandStarts;  //每个超限频段的段首频率 f1
	};

	//频段统计表装配结果（generated=false 时仅 skipReason 有效）
	struct BandStatsResult
	{
		bool generated = false;
		QString skipReason;            //跳过原因（导出成功提示附注拼接用）
		double fMin = 0.0;             //分段起点（用户填写）
		double fMax = 0.0;             //分段终点（用户填写）
		QList<int> spans;              //各组列数（行式=单组 N）；总槽位=Σspans
		QStringList columns;           //Σspans 个列头（逐组各自段宽生成，'g'6）
		QStringList rowLabels;         //行标签（行式=收集序每曲线一行；矩阵=行键首现序合并）
		QVector<QVector<int>> counts;  //[行][槽位] 计数（-1=空槽哨兵：该行该组缺席）
		QVector<QVector<bool>> marks;  //与 counts 同形：计数>0 即标亮（FFFF00），空槽恒 false
	};

	//归段：idx = clamp(floor((f1−fMin)/width), 0, bandCount−1)
	//bandCount/width 非正值防御性返回 0（装配层保证有效，纯函数不抛错）
	int POSTCURVE_EXPORT bandIndexOf(double f1, double fMin, double width, int bandCount);

	//频段统计表装配：矩阵传组键列表与各组列数（组段宽=范围/组列数，每组单独
	//分段，同明细行键曲线合并一行）；行式传空 groups + 单组 spans={N}（全局
	//等宽，每曲线一行）。fMax==fMin（起止相等）、spans 空/含非正或范围倒置时
	//跳过并给出原因
	BandStatsResult POSTCURVE_EXPORT assembleBandStats(const QList<CurveBands>& curves,
		double fMin, double fMax, const QStringList& groups, const QList<int>& spans);

	//导出 JSON 契约（顶层 band_stats 对象）：f_min/f_max/columns/rows/marks
	//（width 已随按组分段口径移除——各组段宽不同），rows 行=[行标签, 计数或
	//空串...]（仅计数>0 写数字；0 与 -1 缺席槽一律空串——真机 2026-11-04：
	//0 格清空不显示），marks 与 counts 同形无标签列；
	//generated=false 返回空对象（调用方不写键，脚本缺席即不加第二张表）
	QJsonObject POSTCURVE_EXPORT bandStatsToJson(const BandStatsResult& stats);
}
