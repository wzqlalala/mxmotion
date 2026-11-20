#ifndef MCURVEWEIGHTING_H
#define MCURVEWEIGHTING_H

// =============================================================================
// 曲线计权（加权）换算（CurveCore 层纯函数，模型/视图/工厂共用）
//
// 计权语义（对齐绘图选项"加权"下拉：无/A/B/C/U）：
//   0 无 / 1 A / 2 B / 3 C / 4 U；U = unweighted（无计权），与"无"等效，
//   保留独立枚举仅为绘图选项所选值在属性页回显一致。
// 计权按线性增益 10^(修正dB/20) 乘到原始值，位于轴刻度模式变换之前：
//   dB20 刻度下即 "20log10(原值) + 修正dB(f)"（标准声学计权语义），
//   Linear/Log10/dB10 经同一增益公式自然成立（全刻度模式生效——**口径已作废**
//   2026-11-16 用户裁定①：增益仅在 dB 档生效，Linear/Log 档渲染原始值，
//   通用闸见下方 effectiveWeightGain；本函数保留给 dB 档委托与显示读数）。
// A/B/C 修正为 IEC 60651/61672 标准有理分式（1kHz 归一 0dB）；
// f<=0 无频率意义，增益取 1（不加权）。
// =============================================================================

#include <cmath>

namespace PostCurve
{

//频率 freqHz(Hz) 处的计权增益（线性域倍数，1.0 = 不修正）
inline double weightGain(int weightMode, double freqHz)
{
	if (freqHz <= 0.0)
		return 1.0;
	const double f2 = freqHz * freqHz;
	double db = 0.0;   //修正 dB 值（20*log10(R)）
	switch (weightMode)
	{
	case 1:   //A 计权：12194²f⁴/[(f²+20.6²)√((f²+107.7²)(f²+737.9²))(f²+12194²)]，+2.00dB
	{
		const double num = 12194.0 * 12194.0 * f2 * f2;
		const double den = (f2 + 20.6 * 20.6)
			* std::sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9))
			* (f2 + 12194.0 * 12194.0);
		db = 20.0 * std::log10(num / den) + 2.00;
		break;
	}
	case 2:   //B 计权：12194²f³/[(f²+20.6²)√(f²+737.9²)(f²+12194²)]；偏移按
	          //1kHz 归一化推导（B(1kHz)≡0dB，与 A/C 同一约定）
	{
		const double num = 12194.0 * 12194.0 * f2 * freqHz;
		const double den = (f2 + 20.6 * 20.6)
			* std::sqrt(f2 + 737.9 * 737.9)
			* (f2 + 12194.0 * 12194.0);
		db = 20.0 * std::log10(num / den) + 1.9489;
		break;
	}
	case 3:   //C 计权：12194²f²/[(f²+20.6²)(f²+12194²)]，+0.062dB
	{
		const double num = 12194.0 * 12194.0 * f2;
		const double den = (f2 + 20.6 * 20.6) * (f2 + 12194.0 * 12194.0);
		db = 20.0 * std::log10(num / den) + 0.062;
		break;
	}
	default:  //0 无 / 4 U(unweighted)：不修正
		return 1.0;
	}
	return std::pow(10.0, db / 20.0);
}

//计权档位闸（2026-11-16 用户裁定①，nvh-log-scale-prototype-align）：增益仅在
//dB10/dB20 刻度档生效；Linear/Logarithmic 档一律恒 1.0（渲染原始值，存量存档
//重开同权——屏蔽落渲染契约处，存档数据零迁移）。scaleMode 语义同
//mCurveChartView::transformValue：0 Linear / 1 Log / 2 dB20 / 3 dB10，非法值
//按非 dB 处理（与 axisScaleModeOf 的未知档回落口径一致）
inline double effectiveWeightGain(int scaleMode, int weightMode, double freqHz)
{
	if (scaleMode != 2 && scaleMode != 3)
		return 1.0;
	return weightGain(weightMode, freqHz);
}

} // namespace PostCurve

#endif // MCURVEWEIGHTING_H
