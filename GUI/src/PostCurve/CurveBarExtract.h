#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 柱状图转换提取数学（纯逻辑，无 GUI 依赖，可无头单测）
//
// 图表类型切换"提取数值转换"（add-chart-type-extract-convert）：把 XY 曲线按
// 用户指定的提取个数 N 转为柱状类目数据——在可见数据范围 [fmin, fmax]（剔除
// 隐藏区间与非有限值后的范围）上均匀取 N 个频率点、含首尾两端（N=5、0~300
// → 0/75/150/225/300，原型口径 300/(5-1)=75 间距、末点在原型图右缘被裁），
// 每点取曲线在该频率处的线性插值，越界防御性取最近端点。
// 可见点<2（无插值基点）返回空表，调用方跳过该曲线保留原样（design 边界②）。
// 类目签取 QString::number(freq)：整数频率自然显示 0/75/150/225/300 同原型。
// 供 mCurveDocController::convertCurvesToBar 装配转换命令用。
// =============================================================================

#include <QStringList>
#include <QVector>

#include "PostCurveGlobal.h"
#include "CurveCore/CurveDefine.h"

namespace CurveBarExtract
{
	//一个提取点：频率（类目签来源）+ 该频率处的曲线值
	struct ExtractedPoint
	{
		double freq = 0.0;
		double value = 0.0;
	};

	//均匀提取：count<2 或可见点<2 返回空；隐藏/非有限点不参与范围与插值
	QVector<ExtractedPoint> POSTCURVE_EXPORT extractUniform(const PostCurve::CurveSeriesData& series, int count);

	//类目签列表（与 pts 一一等长）
	QStringList POSTCURVE_EXPORT categoryLabels(const QVector<ExtractedPoint>& pts);
}
