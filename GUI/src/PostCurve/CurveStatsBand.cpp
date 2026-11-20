#pragma once
//CurveStatsBand.h 对应实现，契约见头文件
#include "CurveStatsBand.h"

#include <QtGlobal>
#include <QtMath>
#include <QHash>
#include <QJsonArray>

namespace CurveStatsBand
{
	int bandIndexOf(double f1, double fMin, double width, int bandCount)
	{
		if (bandCount <= 0 || width <= 0.0)
			return 0;
		//floor 单点归段天然不重复计入；clamp 吸收 f1==fMax 时 floor 得 N 的
		//浮点边界（末段右端闭合）
		const int idx = static_cast<int>(qFloor((f1 - fMin) / width));
		return qBound(0, idx, bandCount - 1);
	}

	BandStatsResult assembleBandStats(const QList<CurveBands>& curves,
		double fMin, double fMax, const QStringList& groups, const QList<int>& spans)
	{
		BandStatsResult result;
		result.fMin = fMin;
		result.fMax = fMax;
		//总槽位与组起始列：spans 空仅当调用方行式传错（行式恒传单组 {N}），
		//含非正同按无效防御
		int slotCount = 0;
		bool spansValid = true;
		for (int span : spans)
		{
			if (span <= 0)
			{
				spansValid = false;
				break;
			}
			slotCount += span;
		}
		//组键与组列数须等长（调用方保证；不等长按无效防御，理论不发生）
		if (!groups.isEmpty() && groups.size() != spans.size())
			spansValid = false;
		if (slotCount <= 0 || !spansValid || fMax < fMin)
		{
			result.skipReason = QStringLiteral("频率范围无效，未生成频段统计表");
			return result;
		}
		if (fMax == fMin)
		{
			result.skipReason = QStringLiteral("频率范围起止相等，未生成频段统计表");
			return result;
		}

		result.generated = true;
		result.spans = spans;

		const auto statNumber = [](double v) {
			return QString::number(v, 'g', 6);
		};
		//列头逐组各自生成（组段宽 =范围/组列数，不取整——取整会覆盖不满范围），
		//组起始列前缀和供计数槽位定位
		QList<int> groupStarts;
		int base = 0;
		for (int span : spans)
		{
			const double width = (fMax - fMin) / span;
			for (int k = 0; k < span; ++k)
			{
				result.columns.append(statNumber(fMin + k * width)
					+ QLatin1Char('-') + statNumber(fMin + (k + 1) * width));
			}
			groupStarts.append(base);
			base += span;
		}

		//行装配：矩阵同明细行键合并为一条频段行（各组分块写同行本组槽位，
		//组间槽位不重叠合并无碰撞，行序=行键首现序与明细表一致）；行式每曲线
		//一行（收集序，标签理论上唯一不合并）。计数槽 -1 哨兵=该行该组缺席，
		//只在曲线所属组的槽位区间落计数；未知组（理论不发生）不落任何槽位
		const bool mergeRows = !groups.isEmpty();
		QHash<QString, int> rowIndexOf;
		for (const CurveBands& curve : curves)
		{
			int r = -1;
			if (mergeRows)
			{
				r = rowIndexOf.value(curve.label, -1);
			}
			if (r < 0)
			{
				r = result.counts.size();
				if (mergeRows)
					rowIndexOf.insert(curve.label, r);
				result.rowLabels.append(curve.label);
				result.counts.append(QVector<int>(slotCount, -1));
				result.marks.append(QVector<bool>(slotCount, false));
			}
			const int group = groups.isEmpty()
				? 0 : groups.indexOf(curve.columnKey);
			if (group < 0)
				continue;
			const int span = spans.at(group);
			const double width = (fMax - fMin) / span;
			const int start = groupStarts.at(group);
			QVector<int>& counts = result.counts[r];   //行已建好，取引用安全
			//组区间首次被本行触碰时整段归零（-1 哨兵→0），再累计——直接 ++ 会
			//从 -1 起步把所有计数压小 1（测试抓出的回归）
			if (counts.at(start) < 0)
			{
				for (int k = 0; k < span; ++k)
					counts[start + k] = 0;
			}
			for (double f1 : curve.bandStarts)
				++counts[start + bandIndexOf(f1, fMin, width, span)];
		}
		//标亮=计数>0（-1 哨兵空槽不标亮），统一末遍与零频段 0 格口径一致
		for (int r = 0; r < result.counts.size(); ++r)
		{
			QVector<bool>& marks = result.marks[r];
			const QVector<int>& counts = result.counts.at(r);
			for (int slot = 0; slot < counts.size(); ++slot)
				marks[slot] = counts.at(slot) > 0;
		}
		return result;
	}

	QJsonObject bandStatsToJson(const BandStatsResult& stats)
	{
		if (!stats.generated)
			return QJsonObject();
		QJsonObject json;
		json.insert(QStringLiteral("f_min"), stats.fMin);
		json.insert(QStringLiteral("f_max"), stats.fMax);
		QJsonArray columnsJson;
		for (const QString& col : stats.columns)
			columnsJson.append(col);
		json.insert(QStringLiteral("columns"), columnsJson);
		QJsonArray rowsJson;
		QJsonArray marksJson;
		for (int r = 0; r < stats.counts.size(); ++r)
		{
			QJsonArray rowJson;
			rowJson.append(stats.rowLabels.value(r));
			//仅计数>0 的槽位写数字（黄底格）；0 与 -1 哨兵一律空串（7.7 真机
			//口径：0 格清空不显示），脚本渲染留空带框格
			for (int count : stats.counts.at(r))
			{
				if (count > 0)
					rowJson.append(count);
				else
					rowJson.append(QJsonValue(QString()));
			}
			rowsJson.append(rowJson);
			QJsonArray markJson;
			for (bool mark : stats.marks.at(r))
				markJson.append(mark);
			marksJson.append(markJson);
		}
		json.insert(QStringLiteral("rows"), rowsJson);
		json.insert(QStringLiteral("marks"), marksJson);
		return json;
	}
}
