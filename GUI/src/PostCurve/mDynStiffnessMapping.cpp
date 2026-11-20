#include "mDynStiffnessMapping.h"
#include "mCurveDataSourceStore.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>

#include <cmath>

namespace PostCurve
{
	namespace
	{
		const char* const kTemplateMetaKey = "Template";
		const char* const kFormMetaKey = "Form";
		//表头（与设计稿逐字一致；VTF 无 Y轴内容列）
		const QString kDynStiffHeaderLine = QStringLiteral(
			"工况（SUBCASE）,结果类型（Types）,对象（Requests）,分量（Components）,目标值（Target）,Y轴内容");
		const QString kVtfHeaderLine = QStringLiteral(
			"工况（SUBCASE）,结果类型（Types）,对象（Requests）,分量（Components）,目标值（Target）");

		//CSV 字段转义：含逗号/引号/换行的字段用引号包裹，内部引号翻倍
		QString csvEscape(const QString& field)
		{
			if (field.contains(QLatin1Char(','))
				|| field.contains(QLatin1Char('"'))
				|| field.contains(QLatin1Char('\n'))
				|| field.contains(QLatin1Char('\r')))
			{
				QString out = field;
				out.replace(QLatin1Char('"'), QStringLiteral("\"\""));
				return QLatin1Char('"') + out + QLatin1Char('"');
			}
			return field;
		}

		//单行 CSV 拆字段（支持引号转义；不跨行）
		QStringList csvSplitLine(const QString& line)
		{
			QStringList fields;
			QString cur;
			bool inQuotes = false;
			for (int i = 0; i < line.size(); ++i)
			{
				const QChar ch = line.at(i);
				if (inQuotes)
				{
					if (ch == QLatin1Char('"'))
					{
						if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"'))
						{
							cur += ch;
							++i;
						}
						else
							inQuotes = false;
					}
					else
						cur += ch;
				}
				else
				{
					if (ch == QLatin1Char('"'))
						inQuotes = true;
					else if (ch == QLatin1Char(','))
					{
						fields.append(cur);
						cur.clear();
					}
					else
						cur += ch;
				}
			}
			fields.append(cur);
			return fields;
		}

		//"Subcase 101 (label)" -> 101（与 mOpenCurveDlg 的 subcaseNumberOf 同规）
		int parseSubcaseId(const QString& text)
		{
			const QString prefix = QStringLiteral("Subcase ");
			if (!text.startsWith(prefix))
				return -1;
			const QString rest = text.mid(prefix.length()).trimmed();
			const int sp = rest.indexOf(QLatin1Char(' '));
			return (sp < 0 ? rest : rest.left(sp)).toInt();
		}

		//"Point id 1001" -> 1001
		int parsePointId(const QString& text)
		{
			const QString prefix = QStringLiteral("Point id ");
			if (!text.startsWith(prefix))
				return -1;
			const QString rest = text.mid(prefix.length()).trimmed();
			const int sp = rest.indexOf(QLatin1Char(' '));
			return (sp < 0 ? rest : rest.left(sp)).toInt();
		}

		//元信息 "Key=Value" 拆键值（# 由调用方剥掉）
		void splitMetaLine(const QString& line, QString& key, QString& value)
		{
			const int eq = line.indexOf(QLatin1Char('='));
			if (eq < 0)
			{
				key.clear();
				value.clear();
				return;
			}
			key = line.left(eq).trimmed();
			value = line.mid(eq + 1).trimmed();
		}

		//从 PRESSURE 块标签提取声压目标值 dB 文本：$PRESSURE 行形如
		//"PRESSURE (58 dB)" → "58dB"；无 dB 信息返回空串（不生成目标线）
		QString pressureTargetDbOfBlock(const PchReader::PchResultBlock& blk)
		{
			static const QRegularExpression dbRe(
				QStringLiteral("(\\d+(?:\\.\\d+)?)\\s*dB"),
				QRegularExpression::CaseInsensitiveOption);
			for (const auto& label : blk.labels)
			{
				const QString cand = QString::fromStdString(label.originalLine)
					+ QLatin1Char(' ') + QString::fromStdString(label.value);
				if (!cand.contains(QStringLiteral("PRESSURE"), Qt::CaseInsensitive))
					continue;
				const QRegularExpressionMatch m = dbRe.match(cand);
				if (m.hasMatch())
					return m.captured(1) + QStringLiteral("dB");
			}
			return QString();
		}
	}

	QString dynStiffFormText(DynStiffForm form)
	{
		switch (form)
		{
		case DynStiffForm::Velocity:
			return QStringLiteral("速度");
		case DynStiffForm::Acceleration:
			return QStringLiteral("加速度");
		case DynStiffForm::SoundPressure:
			return QStringLiteral("声压");
		case DynStiffForm::SoundPressureIpi:
			return QStringLiteral("声压+激励点IPI");
		case DynStiffForm::Displacement:
		default:
			return QStringLiteral("位移");
		}
	}

	PchReader::ResultType dynStiffFormResultType(DynStiffForm form)
	{
		switch (form)
		{
		case DynStiffForm::Velocity:
			return PchReader::ResultType::Velocity;
		case DynStiffForm::Acceleration:
			return PchReader::ResultType::Acceleration;
		case DynStiffForm::SoundPressure:
			return PchReader::ResultType::Pressure;
		case DynStiffForm::SoundPressureIpi:
			//声压+激励点IPI 含 Pressure/Acceleration 两类结果块，无单一映射——
			//行提取与创建侧按行 resultTypeText 分派（resultTypeFromText）
			return PchReader::ResultType::Acceleration;
		case DynStiffForm::Displacement:
		default:
			return PchReader::ResultType::Displacements;
		}
	}

	PchReader::ResultType resultTypeFromText(const QString& typeText)
	{
		const QString t = typeText.toUpper();
		if (t.contains(QStringLiteral("DISPLACEMENT")))
			return PchReader::ResultType::Displacements;
		if (t.contains(QStringLiteral("VELOCITY")))
			return PchReader::ResultType::Velocity;
		if (t.contains(QStringLiteral("ACCELERATION")))
			return PchReader::ResultType::Acceleration;
		if (t.contains(QStringLiteral("PRESSURE")))
			return PchReader::ResultType::Pressure;
		return PchReader::ResultType::Unknown;
	}

	QString dynStiffDefaultTargetText()
	{
		return QStringLiteral("10000N/mm");
	}

	//目标值单位后缀随导出类型（CSV 目标值的物理含义由 kind+form 共同决定）：
	//VTF：目标=常数横线，值即 Y 本身 → 速度 mm/s、加速度 mm/s^2
	//动刚度：CSV 存刚度 K（位移形式目标线画在 kd=1/|H| 空间、速度/加速度形式
	//目标曲线为 2πf/K、(2πf)²/K），三种形式的 K 均为 N/mm 系
	QString dynStiffTargetUnitSuffix(CurveTemplateKind kind, DynStiffForm form)
	{
		if (kind == CurveTemplateKind::Vtf)
			return form == DynStiffForm::Acceleration
				? QStringLiteral(" mm/s^2") : QStringLiteral(" mm/s");
		return QStringLiteral("N/mm");
	}

	QString curveTemplateKindText(CurveTemplateKind kind)
	{
		switch (kind)
		{
		case CurveTemplateKind::Vtf:
			return QStringLiteral("VTF曲线");
		case CurveTemplateKind::Ntf:
			return QStringLiteral("NTF曲线");
		case CurveTemplateKind::DynStiffness:
		default:
			return QStringLiteral("动刚度曲线");
		}
	}

	QString curveDefaultTargetText(CurveTemplateKind kind, DynStiffForm form)
	{
		if (kind == CurveTemplateKind::Vtf)
		{
			//VTF 默认目标值（设计稿：速度 0.1 mm/s、加速度 100 mm/s^2）
			return form == DynStiffForm::Acceleration
				? QStringLiteral("100 mm/s^2") : QStringLiteral("0.1 mm/s");
		}
		if (kind == CurveTemplateKind::Ntf)
		{
			//NTF 无固定默认目标值：声压行提取自 pch $PRESSURE 标签的 dB 值
			//（无则空=不生成目标线）；激励点 IPI 行不生成目标线
			return QString();
		}
		return dynStiffDefaultTargetText();
	}

	QString dynStiffDefaultYAxisText(DynStiffForm form)
	{
		switch (form)
		{
		case DynStiffForm::Velocity:
			return QStringLiteral("Velocity Mobility (m/s)/N");
		case DynStiffForm::Acceleration:
			return QStringLiteral("IPI((m/s^2)/N");
		case DynStiffForm::Displacement:
		default:
			return QStringLiteral("Kd/N/m");
		}
	}

	QList<DynStiffRow> buildRowsFromPch(const PchReader::PchFile& file, CurveTemplateKind kind, DynStiffForm form)
	{
		QList<DynStiffRow> rows;
		constexpr double kTwoPi = 6.283185307179586;
		const PchReader::ResultType wantType = dynStiffFormResultType(form);
		const QString typeText = QString::fromLatin1(PchReader::toString(wantType));
		//每个工况×对象出 X/Y/Z 三个平动分量行（设计稿 CSV 只列平动，旋转分量不导出）
		const QStringList directions = {
			QStringLiteral("X-Trans"), QStringLiteral("Y-Trans"), QStringLiteral("Z-Trans"),
		};

		//NTF 特例：按形式提取声压/激励点行后即返回（不走矢量三分量通用路径）
		if (form == DynStiffForm::SoundPressure || form == DynStiffForm::SoundPressureIpi)
		{
			//声压行：Pressure 块每 工况×对象 一行，分量令牌 "REL|FX"
			//（设计稿令牌，REAL 声压 × FX 激励方向）；目标值提取自块标签
			//$PRESSURE 的 dB 值（如 "PRESSURE (58 dB)" → "58dB"，无则空）
			QSet<qint64> seenSp;
			for (const auto& blk : file.blocks)
			{
				if (blk.resultType != PchReader::ResultType::Pressure)
					continue;
				const qint64 key = (static_cast<qint64>(blk.subcaseId) << 32) | static_cast<quint32>(blk.pointId);
				if (seenSp.contains(key))
					continue;
				seenSp.insert(key);

				DynStiffRow row;
				row.subcaseId = blk.subcaseId;
				row.subcaseText = mCurveDataSourceStore::subcaseDisplayText(file, blk.subcaseId);
				row.resultTypeText = QString::fromLatin1(PchReader::toString(blk.resultType));
				row.pointId = blk.pointId;
				row.objectText = QStringLiteral("Point id %1").arg(blk.pointId);
				row.componentText = QStringLiteral("REL|FX");
				row.targetText = pressureTargetDbOfBlock(blk);
				rows.append(row);
			}
			//声压+激励点IPI：追加激励点加速度行（MAG|X/Y/Z-Trans，目标值空=
			//不生成目标线，对齐设计稿 IPI 图无目标线）
			if (form == DynStiffForm::SoundPressureIpi)
			{
				QSet<qint64> seenAcc;
				for (const auto& blk : file.blocks)
				{
					if (blk.resultType != PchReader::ResultType::Acceleration)
						continue;
					const qint64 key = (static_cast<qint64>(blk.subcaseId) << 32) | static_cast<quint32>(blk.pointId);
					if (seenAcc.contains(key))
						continue;
					seenAcc.insert(key);

					DynStiffRow base;
					base.subcaseId = blk.subcaseId;
					base.subcaseText = mCurveDataSourceStore::subcaseDisplayText(file, blk.subcaseId);
					base.resultTypeText = QString::fromLatin1(PchReader::toString(blk.resultType));
					base.pointId = blk.pointId;
					base.objectText = QStringLiteral("Point id %1").arg(blk.pointId);
					//targetText 保持空：CSV 目标值列空，创建时不生成目标线
					for (const QString& dir : directions)
					{
						DynStiffRow row = base;
						row.componentText = QStringLiteral("MAG|") + dir;
						rows.append(row);
					}
				}
			}
			return rows;
		}

		//同一 工况×对象 可能有多块（多频段拼接等）：QSet 去重保证每组合只出一行
		QSet<qint64> seen;
		for (const auto& blk : file.blocks)
		{
			if (blk.resultType != wantType)
				continue;
			const qint64 key = (static_cast<qint64>(blk.subcaseId) << 32) | static_cast<quint32>(blk.pointId);
			if (seen.contains(key))
				continue;
			seen.insert(key);

			DynStiffRow base;
			base.subcaseId = blk.subcaseId;
			base.subcaseText = mCurveDataSourceStore::subcaseDisplayText(file, blk.subcaseId);
			base.resultTypeText = typeText;
			base.pointId = blk.pointId;
			base.objectText = QStringLiteral("Point id %1").arg(blk.pointId);
			base.yAxisText = dynStiffDefaultYAxisText(form);
			//分量令牌随数值格式取前缀（MAGNITUDE-PHASE→MAG，REAL-*→REAL；与
			//expandedComponents/componentIndex 同一套命名，assembleChannelData 可直接消费）
			const QString fmt = QString::fromLatin1(PchReader::toString(blk.numericFormat));
			const QString token = fmt.contains(QStringLiteral("MAGNITUDE-PHASE"), Qt::CaseInsensitive)
				? QStringLiteral("MAG") : QStringLiteral("REAL");
			const QString schema = QString::fromStdString(blk.schemaName);
			for (const QString& dir : directions)
			{
				DynStiffRow row = base;
				row.componentText = token + QLatin1Char('|') + dir;
				//目标值默认取该行数据 Y 值区间的中点（原为固定 10000N/mm 定值，
				//与实际数据量级脱节），值空间/单位随导出类型：
				//VTF：目标=常数横线（值空间=Y 本身）→ 幅值区间中点（mm/s、mm/s^2）
				//动刚度·位移：目标=常数 K（kd=1/|H| 值空间）→ kd 区间中点（N/mm）
				//动刚度·速度/加速度：目标=2πf/K、(2πf)²/K 曲线 → 反解
				//K=2π·f中点/|H|中点、K=(2π·f中点)²/|H|中点，使目标曲线在
				//频率/幅值区间中点处恰好穿过幅值中点（N/mm）
				//幅值装配失败/无有效样本时回退固定默认文本
				mCurveDataSourceStore::ChannelDataResult data;
				QString chanErr;
				if (mCurveDataSourceStore::assembleChannelMagnitude(file, blk.subcaseId, schema,
					blk.pointId, dir, &data, &chanErr)
					&& !data.yData.isEmpty())
				{
					bool ok = false;
					double targetValue = 0.0;
					if (kind == CurveTemplateKind::Vtf)
					{
						bool has = false;
						double lo = 0.0, hi = 0.0;
						for (double v : data.yData)
						{
							if (!qIsFinite(v) || v <= 0.0)
								continue;
							if (!has) { lo = hi = v; has = true; }
							else { lo = qMin(lo, v); hi = qMax(hi, v); }
						}
						if (has)
						{
							targetValue = (lo + hi) * 0.5;
							ok = true;
						}
					}
					else if (form == DynStiffForm::Displacement)
					{
						//kd = 1/|H|，与创建侧同口径：只取正导纳（v>1e-12 防除零；
						//负导纳的负 kd 在创建侧会被对数轴样本过滤剔除，不进显示
						//区间，目标中点须与显示区间一致）
						bool has = false;
						double lo = 0.0, hi = 0.0;
						for (double v : data.yData)
						{
							if (!qIsFinite(v) || v <= 1e-12)
								continue;
							const double kd = 1.0 / v;
							if (!has) { lo = hi = kd; has = true; }
							else { lo = qMin(lo, kd); hi = qMax(hi, kd); }
						}
						if (has)
						{
							targetValue = (lo + hi) * 0.5;
							ok = true;
						}
					}
					else
					{
						//速度/加速度：统计幅值与频率的有限区间，按目标曲线反解 K
						bool hasH = false;
						double hLo = 0.0, hHi = 0.0;
						bool hasF = false;
						double fLo = 0.0, fHi = 0.0;
						const int n = qMin(data.xData.size(), data.yData.size());
						for (int i = 0; i < n; ++i)
						{
							const double h = data.yData.at(i);
							const double f = data.xData.at(i);
							if (!qIsFinite(h) || h <= 0.0 || !qIsFinite(f) || f <= 0.0)
								continue;
							if (!hasH) { hLo = hHi = h; hasH = true; }
							else { hLo = qMin(hLo, h); hHi = qMax(hHi, h); }
							if (!hasF) { fLo = fHi = f; hasF = true; }
							else { fLo = qMin(fLo, f); fHi = qMax(fHi, f); }
						}
						if (hasH && hasF)
						{
							const double midH = (hLo + hHi) * 0.5;
							const double fMid = (fLo + fHi) * 0.5;
							const double w = kTwoPi * fMid;
							targetValue = (form == DynStiffForm::Velocity)
								? w / midH : w * w / midH;
							ok = targetValue > 0.0;
						}
					}
					if (ok)
						row.targetText = QString::number(targetValue)
							+ dynStiffTargetUnitSuffix(kind, form);
					else
						row.targetText = curveDefaultTargetText(kind, form);
				}
				else
				{
					row.targetText = curveDefaultTargetText(kind, form);
				}
				rows.append(row);
			}
		}
		return rows;
	}

	bool writeMappingFile(const QString& filePath, CurveTemplateKind kind,
		DynStiffForm form, const QList<DynStiffRow>& rows, QString* err)
	{
		//二进制模式写：换行已显式给 "\r\n"，不走 Text 模式（Text 会把 "\n"
		//再翻译成 "\r\n"，与显式 "\r\n" 叠加成 "\r\r\n" 脏数据）
		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			if (err) *err = QStringLiteral("无法写入文件：%1").arg(filePath);
			return false;
		}
		const bool vtf = (kind == CurveTemplateKind::Vtf);
		const bool dyn = (kind == CurveTemplateKind::DynStiffness);
		QTextStream ts(&file);
		ts.setCodec("UTF-8");
		ts.setGenerateByteOrderMark(true);   //BOM：Excel 直接打开不乱码

		ts << QLatin1Char('#') << kTemplateMetaKey << QLatin1Char('=')
			<< curveTemplateKindText(kind) << "\r\n";
		ts << QLatin1Char('#') << kFormMetaKey << QLatin1Char('=') << dynStiffFormText(form) << "\r\n";
		ts << (vtf ? kVtfHeaderLine : kDynStiffHeaderLine) << "\r\n";
		for (const DynStiffRow& row : rows)
		{
			//空目标值按模板默认补齐（设计稿：未手动添加目标值则使用默认参数）；
			//NTF 无固定默认——声压行已带 pch 提取的 dB 值，IPI 行保持空=不生成目标线
			const QString target = row.targetText.isEmpty()
				? curveDefaultTargetText(kind, form) : row.targetText;
			ts << csvEscape(row.subcaseText) << QLatin1Char(',')
				<< csvEscape(row.resultTypeText) << QLatin1Char(',')
				<< csvEscape(row.objectText) << QLatin1Char(',')
				<< csvEscape(row.componentText) << QLatin1Char(',')
				<< csvEscape(target);
			if (!vtf)
			{
				//Y轴内容列动刚度/NTF 模板有（NTF 恒空，保持列结构对齐动刚度）
				QString yAxis = row.yAxisText;
				if (dyn && yAxis.isEmpty())
					yAxis = dynStiffDefaultYAxisText(form);
				ts << QLatin1Char(',') << csvEscape(yAxis);
			}
			ts << "\r\n";
		}
		ts.flush();
		if (ts.status() != QTextStream::Ok)
		{
			if (err) *err = QStringLiteral("写入文件失败：%1").arg(filePath);
			return false;
		}
		return true;
	}

	bool readMappingFile(const QString& filePath, CurveTemplateKind& kind,
		DynStiffForm& form, QList<DynStiffRow>& rows, QString* err)
	{
		kind = CurveTemplateKind::DynStiffness;
		form = DynStiffForm::Displacement;
		rows.clear();

		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			if (err) *err = QStringLiteral("无法打开配置文件：%1").arg(filePath);
			return false;
		}
		const QByteArray raw = file.readAll();

		//解码：默认 UTF-8（自带 BOM 最好）；出现替换符 U+FFFD 说明不是 UTF-8
		//（Excel 另存 ANSI/GBK 的手工编辑场景），回退 GBK 重解码
		QString all = QString::fromUtf8(raw);
		if (all.contains(QChar::ReplacementCharacter))
			all = QString::fromLocal8Bit(raw);
		//fromUtf8 不剥 BOM（首行会带 U+FEFF 前缀），显式去掉
		if (all.startsWith(QChar(0xFEFF)))
			all.remove(0, 1);

		bool formFound = false;
		bool kindFound = false;
		bool headerSeen = false;
		int firstDataRowFields = 0;   //首数据行列数（#Template 缺失时推断模板类型）
		QString firstYAxis;
		const QStringList lines = all.split(QLatin1Char('\n'));
		for (QString line : lines)
		{
			if (line.endsWith(QLatin1Char('\r')))
				line.chop(1);
			if (line.isEmpty())
				continue;

			//元信息行（#Key=Value）
			if (line.startsWith(QLatin1Char('#')))
			{
				QString key, value;
				splitMetaLine(line.mid(1), key, value);
				if (key.compare(QLatin1String(kFormMetaKey), Qt::CaseInsensitive) == 0)
				{
					if (value == QStringLiteral("位移"))
					{
						form = DynStiffForm::Displacement;
						formFound = true;
					}
					else if (value == QStringLiteral("速度"))
					{
						form = DynStiffForm::Velocity;
						formFound = true;
					}
					else if (value == QStringLiteral("加速度"))
					{
						form = DynStiffForm::Acceleration;
						formFound = true;
					}
					else if (value == QStringLiteral("声压"))
					{
						form = DynStiffForm::SoundPressure;
						formFound = true;
					}
					else if (value == QStringLiteral("声压+激励点IPI"))
					{
						form = DynStiffForm::SoundPressureIpi;
						formFound = true;
					}
				}
				else if (key.compare(QLatin1String(kTemplateMetaKey), Qt::CaseInsensitive) == 0)
				{
					if (value == QStringLiteral("VTF曲线"))
					{
						kind = CurveTemplateKind::Vtf;
						kindFound = true;
					}
					else if (value == QStringLiteral("NTF曲线"))
					{
						kind = CurveTemplateKind::Ntf;
						kindFound = true;
					}
					else if (value == QStringLiteral("动刚度曲线"))
					{
						kind = CurveTemplateKind::DynStiffness;
						kindFound = true;
					}
				}
				continue;
			}

			//表头行（首个含"工况"的行；容忍用户改动列名外的空白）
			if (!headerSeen)
			{
				if (line.contains(QStringLiteral("工况")))
				{
					headerSeen = true;
					continue;
				}
				//无表头直接是数据也容忍：继续按数据行解析
			}

			const QStringList fields = csvSplitLine(line);
			if (fields.size() < 5)
				continue;   //不足五列的行（手误空行/注释）跳过

			if (firstDataRowFields == 0)
				firstDataRowFields = fields.size();

			DynStiffRow row;
			row.subcaseText = fields.at(0).trimmed();
			row.resultTypeText = fields.at(1).trimmed();
			row.objectText = fields.at(2).trimmed();
			row.componentText = fields.at(3).trimmed();
			row.targetText = fields.at(4).trimmed();
			//Y轴内容列仅动刚度模板有（第 6 列；VTF 恒空）
			if (fields.size() >= 6)
				row.yAxisText = fields.at(5).trimmed();
			row.subcaseId = parseSubcaseId(row.subcaseText);
			row.pointId = parsePointId(row.objectText);
			//默认值填充后移到循环后（kind/form 归一化完成再定默认参数，避免
			//#Form 缺失时行级默认值用错形式）
			if (firstYAxis.isEmpty())
				firstYAxis = row.yAxisText;
			rows.append(row);
		}

		//#Template 缺失时按首数据行列数推断（5 列=VTF、6 列=动刚度）
		if (!kindFound)
		{
			if (firstDataRowFields == 5)
				kind = CurveTemplateKind::Vtf;
			else
				kind = CurveTemplateKind::DynStiffness;
		}

		//VTF 无位移形式：#Form=位移（手改错误）或缺失时回退速度（设计稿默认形式）
		if (kind == CurveTemplateKind::Vtf && form == DynStiffForm::Displacement)
			form = DynStiffForm::Velocity;
		//NTF 只有 声压/声压+激励点IPI 两种形式：其它取值（含动刚度词）回退声压
		if (kind == CurveTemplateKind::Ntf
			&& form != DynStiffForm::SoundPressure && form != DynStiffForm::SoundPressureIpi)
			form = DynStiffForm::SoundPressure;

		//#Form 缺失时按模板/首行 Y轴内容推断：VTF→速度（设计稿默认）、
		//NTF→声压（设计稿默认）、动刚度按 Y轴内容（Velocity Mobility→速度、
		//IPI→加速度）
		if (!formFound)
		{
			if (kind == CurveTemplateKind::Vtf)
				form = DynStiffForm::Velocity;
			else if (kind == CurveTemplateKind::Ntf)
				form = DynStiffForm::SoundPressure;
			else if (firstYAxis.contains(QStringLiteral("Velocity"), Qt::CaseInsensitive))
				form = DynStiffForm::Velocity;
			else if (firstYAxis.contains(QStringLiteral("IPI"), Qt::CaseInsensitive))
				form = DynStiffForm::Acceleration;
			else
				form = DynStiffForm::Displacement;
		}

		//行级默认参数（目标值/Y轴内容）在 kind/form 归一化完成后统一补齐
		//（设计稿：未手动添加这些内容则使用默认参数）
		for (DynStiffRow& row : rows)
		{
			if (row.targetText.isEmpty())
				row.targetText = curveDefaultTargetText(kind, form);
			if (kind == CurveTemplateKind::DynStiffness && row.yAxisText.isEmpty())
				row.yAxisText = dynStiffDefaultYAxisText(form);
		}

		if (rows.isEmpty())
		{
			if (err) *err = QStringLiteral("配置文件中未解析到有效数据行：%1").arg(filePath);
			return false;
		}
		return true;
	}

	bool parseTargetValue(const QString& targetText, double& value)
	{
		static const QRegularExpression numRe(QStringLiteral("[-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?"));
		const QRegularExpressionMatch m = numRe.match(targetText);
		if (!m.hasMatch())
			return false;
		bool ok = false;
		const double v = m.captured(0).toDouble(&ok);
		if (!ok)
			return false;
		value = v;
		return true;
	}
}
