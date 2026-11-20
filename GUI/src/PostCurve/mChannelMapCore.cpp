#pragma execution_character_set("utf-8")

// =============================================================================
// mChannelMapCore —— 通道映射核心（实现），接口语义见 mChannelMapCore.h
// =============================================================================

#include "mChannelMapCore.h"
#include "mCurveTemplateManager.h"
#include "mCurveDataSourceStore.h"
#include "CurveCore/CurveDocument.h"

//QT
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QTextStream>

namespace PostCurve
{
	namespace
	{
		//归一化：去分隔符（空白/|/-/_ 等）+ 小写（design D4：大小写/分隔符不敏感）
		QString normalizeToken(const QString& s)
		{
			QString out;
			out.reserve(s.size());
			for (const QChar& c : s)
			{
				if (c.isLetterOrNumber())
					out += c.toLower();
			}
			return out;
		}

		//Levenshtein 距离（两串均短，O(nm) 直算）
		int editDistance(const QString& a, const QString& b)
		{
			if (a.isEmpty())
				return b.size();
			if (b.isEmpty())
				return a.size();
			QVector<int> prev(b.size() + 1), cur(b.size() + 1);
			for (int j = 0; j <= b.size(); ++j)
				prev[j] = j;
			for (int i = 1; i <= a.size(); ++i)
			{
				cur[0] = i;
				for (int j = 1; j <= b.size(); ++j)
				{
					const int cost = (a.at(i - 1) == b.at(j - 1)) ? 0 : 1;
					cur[j] = qMin(qMin(cur[j - 1] + 1, prev[j] + 1), prev[j - 1] + cost);
				}
				prev.swap(cur);
			}
			return prev.at(b.size());
		}

		//Levenshtein 距离（带截止，optimize-channel-map-perf）：dist ≤ cap 时返回
		//真实距离，否则提前放弃返回 cap+1——DP 任一行的行最小值是最终距离的下界
		//（最优路径逐行单调不减），行 min > cap 即无望。缓冲复用前提：本模块仅
		//GUI 线程调用（无重入）
		int editDistanceCapped(const QString& a, const QString& b, int cap)
		{
			if (a.isEmpty())
				return b.size() <= cap ? b.size() : cap + 1;
			if (b.isEmpty())
				return a.size() <= cap ? a.size() : cap + 1;
			static QVector<int> prev, cur;
			prev.resize(b.size() + 1);
			cur.resize(b.size() + 1);
			for (int j = 0; j <= b.size(); ++j)
				prev[j] = j;
			for (int i = 1; i <= a.size(); ++i)
			{
				cur[0] = i;
				int rowMin = i;
				for (int j = 1; j <= b.size(); ++j)
				{
					const int cost = (a.at(i - 1) == b.at(j - 1)) ? 0 : 1;
					cur[j] = qMin(qMin(cur[j - 1] + 1, prev[j] + 1), prev[j - 1] + cost);
					if (cur[j] < rowMin)
						rowMin = cur[j];
				}
				if (rowMin > cap)
					return cap + 1;
				prev.swap(cur);
			}
			return prev.at(b.size());
		}

		//归一化 token 缓存（normalizeToken 逐字符 map+filter ⇒ 拼接串归一化 =
		//分段归一化拼接，token 可安全预计算复用）。进程级常量语义；仅 GUI 线程
		QString normOf(const QString& s)
		{
			static QHash<QString, QString> cache;
			const auto it = cache.constFind(s);
			if (it != cache.constEnd())
				return it.value();
			return cache.insert(s, normalizeToken(s)).value();
		}

		//"Subcase %1"/"Point id %1" 纯格式串按 id 缓存（同上，仅 GUI 线程）
		QString subTextOf(int subcaseId)
		{
			static QHash<int, QString> cache;
			const auto it = cache.constFind(subcaseId);
			if (it != cache.constEnd())
				return it.value();
			return cache.insert(subcaseId,
				mCurveChannelSelector::subcaseText(subcaseId)).value();
		}

		QString objTextOf(int pointId)
		{
			static QHash<int, QString> cache;
			const auto it = cache.constFind(pointId);
			if (it != cache.constEnd())
				return it.value();
			return cache.insert(pointId,
				mCurveChannelSelector::objectText(pointId)).value();
		}

		//单行精确匹配（能装配出数据即命中）。探针判定与 assembleChannelData 成败
		//严格互充（optimize-channel-map-perf）：不再试装配整份通道数据
		void exactMatchRow(mChannelMapCore::Row* row)
		{
			row->exactHit = false;
			row->mapped.complete = false;
			ChannelSelection sel = mCurveChannelSelector::fromModelTexts(
				row->mapped.filePath, row->subcase, row->schema, row->object, row->component);
			QString err;
			if (!mCurveDataSourceStore::getInstance()->hasChannelData(row->mapped.filePath,
				sel.subcaseId, sel.schema, sel.pointId, sel.component, &err))
				return;
			sel.complete = true;
			row->mapped = sel;
			row->exactHit = true;
		}

		//按相似度给单行找最佳通道（optimize-channel-map-perf 优化版）：
		//  - token 预计算：候选归一串 = 分段归一化 token 拼接（与直算版全串归一
		//    逐字符相等），显示串与 token 均缓存；前缀段按对象一次，每组合仅剩
		//    一次前缀+分量拼接（批次5）；
		//  - 长度差下界剪枝（批次5）：|lenT-lenAct| 为 Levenshtein 距离下界，超
		//    cap 零分配跳过；
		//  - 带截止 DP：cap 取 min(按匹配门槛允许的最大距离, 按当前最优允许的
		//    距离)，均为必要条件剪枝（超 cap 必不可能入选/胜出），判定输出与
		//    优化前全枚举直算版逐位一致——语义基准冻结于 tst_mChannelMapCore；
		//  - 迭代结构与替换规则（score > bestScore）保持原样。
		bool bestMatchRow(mChannelMapCore::Row* row, bool suffixMode)
		{
			mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
			const QString norm = mCurveDataSourceStore::normalizePath(row->mapped.filePath);
			const ListWgtStruct* idx = store->index(norm);
			if (!idx)
				return false;

			//行级模板 token（每行一次；后缀模式只比结果类型+分量尾段——对象名
			//点号数字差异是常见噪声，后缀模式把它们剔除）
			const QString normT = suffixMode
				? normOf(row->schema) + normOf(row->component)
				: normOf(row->subcase) + normOf(row->schema)
					+ normOf(row->object) + normOf(row->component);
			if (normT.isEmpty())
				return false;   //与直算版一致：模板 token 空 ⇒ 全部 score=0 < 门槛
			const int lenT = normT.size();

			double bestScore = 0.0;
			ChannelSelection best;
			best.filePath = norm;
			//按 工况 -> 结果类型 -> 对象 -> 分量 全枚举（迭代结构/序与直算版一致）
			for (auto subIt = idx->ycomponent.constBegin();
				subIt != idx->ycomponent.constEnd(); ++subIt)
			{
				const int subcaseId = subIt.key();
				const QString subTok = normOf(subTextOf(subcaseId));
				const QHash<QString, QHash<int, std::set<NumericFmtInfo>>> schemas = subIt.value();
				for (auto schIt = schemas.constBegin(); schIt != schemas.constEnd(); ++schIt)
				{
					const QString schema = schIt.key();
					const QString schTok = normOf(schema);
					const QHash<int, std::set<NumericFmtInfo>> objects = schIt.value();
					for (auto objIt = objects.constBegin(); objIt != objects.constEnd(); ++objIt)
					{
						const int pointId = objIt.key();
						const QString objTok = normOf(objTextOf(pointId));
						//四段前缀按 (工况,结果类型,对象) 现算一次，分量循环内仅 +compTok
						//（拼接结合律 ⇒ 与直算全串归一逐字符相等；批次5：4 次分配→1 次）
						const QString objPrefix = suffixMode ? schTok
							: subTok + schTok + objTok;
						const int prefixLen = objPrefix.size();
						//具名承接 value() 的临时容器（迭代器不得指向临时量）
						const std::set<NumericFmtInfo> fmts = objIt.value();
						for (const NumericFmtInfo& fmt : fmts)
						{
							const QStringList comps = mCurveDataSourceStore::expandedComponents(
								fmt.fmt, fmt.transient);
							for (const QString& comp : comps)
							{
								const QString compTok = normOf(comp);
								const int lenAct = prefixLen + compTok.size();
								//长度差下界剪枝（批次5）：|lenT-lenAct| 是 Levenshtein 距离
								//的下界（每次编辑至多改变长度 1），超 cap 必不胜出——连
								//normAct 串都不必构建（零分配跳过，输出与直算版逐位一致）
								int maxLen = qMax(lenT, lenAct);
								//cap（floor 经 int() 截断，正数；+1e-9 抵浮点回读偏差）
								int cap = int((1.0 - mChannelMapCore::threshold())
									* maxLen + 1e-9);
								if (bestScore > 0.0)
									cap = qMin(cap, int((1.0 - bestScore) * maxLen + 1e-9));
								if (qAbs(lenT - lenAct) > cap)
									continue;
								const QString normAct = objPrefix + compTok;
								if (normAct.isEmpty())
									continue;   //直算版：候选归一串空 ⇒ score=0 不替换
								const int dist = editDistanceCapped(normT, normAct, cap);
								if (dist > cap)
									continue;   //必不胜出（必要条件剪枝）
								const double score = 1.0 - double(dist) / maxLen;
								if (score > bestScore)
								{
									bestScore = score;
									best.subcaseId = subcaseId;
									best.schema = schema;
									best.pointId = pointId;
									best.component = comp;
								}
							}
						}
					}
				}
			}
			if (bestScore < mChannelMapCore::threshold() || best.component.isEmpty())
				return false;
			best.complete = true;
			row->mapped = best;
			return true;
		}
	}

	QString mChannelMapCore::channelKey(const QString& subcase, const QString& schema,
		const QString& object, const QString& component)
	{
		//\x1f 不可见分隔：显示串本身可含 '|' 等字符，拼接键须无歧义
		return subcase + QChar(0x1F) + schema + QChar(0x1F) + object + QChar(0x1F) + component;
	}

	QList<mChannelMapCore::Row> mChannelMapCore::rowsFromTemplate(
		const QString& templatePath, const QStringList& effectiveParams, QString* err)
	{
		QList<Row> rows;
		//恢复到暂存文档（不动会话；曲线 sourceFile 已按参数表回填为实际路径）
		CurveDocument scratch;
		QList<int> pageIds;
		if (!mCurveTemplateManager::restoreTemplateFile(templatePath, &scratch,
			effectiveParams, &pageIds, err))
		{
			if (err && err->isEmpty())
				*err = QStringLiteral("模板恢复失败");
			return rows;
		}
		for (int pageId : pageIds)
		{
			for (int chartId : scratch.childNodeIds(pageId))
			{
				if (scratch.nodeType(chartId) != CurveNodeType::Chart)
					continue;
				for (int curveId : scratch.childNodeIds(chartId))
				{
					if (scratch.nodeType(curveId) != CurveNodeType::Curve)
						continue;
					const CurveSeriesData& s = scratch.node(curveId)->series;
					if (curveSeriesSourceKind(s) != CurveSourceKind::File)
						continue;   //仅文件来源有通道映射语义
					//复数候选判定（语义同应用管线 isComplexCandidate，文件局部副本：
					//数据保存时剥除 isComplex 不可靠，侧来源定义/复数侧表达式在即复数）
					const bool complexCand = !s.complexSideSchema.isEmpty()
						|| !s.complexSideComponent.isEmpty()
						|| !s.complexRef.expression.isEmpty();
					//锚行（ym）：五元组 = series.subcase/schema/object/component
					const QString curveName = scratch.nodeName(curveId);
					Row row;
					row.curveName = curveName;
					row.side = Row::SideAnchor;
					row.subcase = s.subcase;
					row.schema = s.schema;
					row.object = s.object;
					row.component = s.component;
					row.mapped.filePath = s.sourceFile;
					rows << row;
					//yp 侧行（复数候选，complexSide 分量令牌非空即出；缺省按同方向
					//令牌互换配对）：四元组取 complexSide* 原文（实选落模后与锚四元组
					//可独立）。限定名"曲线名 (yp)"为 CSV 对齐键，语义 = 该曲线 yp 侧
					if (complexCand)
					{
						const QString compB = !s.complexSideComponent.isEmpty()
							? s.complexSideComponent
							: mCurveDataSourceStore::siblingComplexComponent(s.component);
						if (!compB.isEmpty())
						{
							Row ypRow;
							ypRow.curveName = curveName + QStringLiteral(" (yp)");
							ypRow.side = Row::SideYp;
							ypRow.subcase = s.complexSideSubcase;
							ypRow.schema = s.complexSideSchema;
							ypRow.object = s.complexSideObject;
							ypRow.component = compB;
							ypRow.mapped.filePath = s.sourceFile;
							rows << ypRow;
						}
					}
					//x 侧行（仅通道形态——横坐标形态无通道语义，不参与映射）：
					//四元组取 xSide* 原文。限定名"曲线名 (x)"同上
					if (mCurveDataSourceStore::isChannelSchema(s.xSideSchema))
					{
						Row xRow;
						xRow.curveName = curveName + QStringLiteral(" (x)");
						xRow.side = Row::SideX;
						xRow.subcase = s.xSideSubcase;
						xRow.schema = s.xSideSchema;
						xRow.object = s.xSideObject;
						xRow.component = s.xSideComponent;
						xRow.mapped.filePath = s.sourceFile;
						rows << xRow;
					}
				}
			}
		}
		return rows;
	}

	void mChannelMapCore::ensureFilesRegistered(const QList<Row>& rows)
	{
		//数据文件预登记（对齐应用管线同名步，mTemplateApplyPipeline）：
		//修复前首次打开通道映射（本会话尚无任何入库路径）时，仓库无该文件，
		//精确匹配/相似预配/行内候选三者全哑（hasChannelData 报"数据文件不在
		//仓库"），整表红叉且下拉无可选项——曲线生成不受影响但映射交互不可用，
		//须先应用一次模板才恢复。此处在打开对话框前统一登记（幂等），首次
		//打开即全功能可用
		mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
		QSet<QString> seen;
		for (const Row& row : rows)
		{
			if (row.mapped.filePath.isEmpty())
				continue;   //参数未填且模板无原始路径：无处可登记
			const QString norm = mCurveDataSourceStore::normalizePath(row.mapped.filePath);
			if (seen.contains(norm) || store->contains(norm))
				continue;   //本批重复 / 已登记：不重复解析（contains 幂等语义）
			seen.insert(norm);
			store->addFile(row.mapped.filePath);   //失败不中断：各行保持未配红叉
		}
	}

	void mChannelMapCore::initExactMatches(QList<Row>* rows)
	{
		for (Row& row : *rows)
			exactMatchRow(&row);
	}

	void mChannelMapCore::autoConnectSimilar(QList<Row>* rows, int autoFillMode)
	{
		if (autoFillMode == AutoFillNone)
			return;
		const bool suffixMode = (autoFillMode == AutoFillSuffix);
		for (Row& row : *rows)
		{
			if (row.mapped.complete || row.exactHit)
				continue;   //仅未匹配行预配（精确命中行不动）
			bestMatchRow(&row, suffixMode);
		}
	}

	void mChannelMapCore::bestMatchUnmatched(QList<Row>* rows)
	{
		for (Row& row : *rows)
		{
			if (row.mapped.complete || row.exactHit)
				continue;
			bestMatchRow(&row, false);
		}
	}

	ChannelSelection mChannelMapCore::bestMatchChannel(const QString& filePath,
		const QString& subcaseText, const QString& schema,
		const QString& objectText, const QString& component)
	{
		Row row;
		row.subcase = subcaseText;
		row.schema = schema;
		row.object = objectText;
		row.component = component;
		row.mapped.filePath = filePath;
		if (!bestMatchRow(&row, false))
			return ChannelSelection();   //无命中（complete=false）
		return row.mapped;
	}

	QStringList mChannelMapCore::subcaseTexts(const QString& filePath)
	{
		QStringList out;
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			mCurveDataSourceStore::normalizePath(filePath));
		if (!idx)
			return out;
		for (auto it = idx->ycomponent.constBegin(); it != idx->ycomponent.constEnd(); ++it)
			out << mCurveChannelSelector::subcaseText(it.key());
		return out;
	}

	QStringList mChannelMapCore::schemaTexts(const QString& filePath, int subcaseId)
	{
		QStringList out;
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			mCurveDataSourceStore::normalizePath(filePath));
		if (!idx)
			return out;
		const std::set<QString> schemas = idx->yresulttype.value(subcaseId);
		for (const QString& s : schemas)
			out << s;
		return out;
	}

	QStringList mChannelMapCore::objectTexts(const QString& filePath, int subcaseId,
		const QString& schema)
	{
		QStringList out;
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			mCurveDataSourceStore::normalizePath(filePath));
		if (!idx)
			return out;
		//const 迭代取目标 schema 的对象集（optimize-channel-map-perf：值拷贝会对
		//std::set<int> 深拷贝 P 个节点——对象列候选现算的高频热点）
		const QHash<QString, std::set<int>> objs = idx->object.value(subcaseId);
		const auto objIt = objs.constFind(schema);
		if (objIt == objs.constEnd())
			return out;
		for (const int pid : objIt.value())
			out << mCurveChannelSelector::objectText(pid);
		return out;
	}

	QStringList mChannelMapCore::componentTexts(const QString& filePath, int subcaseId,
		const QString& schema, int pointId)
	{
		QStringList out;
		const ListWgtStruct* idx = mCurveDataSourceStore::getInstance()->index(
			mCurveDataSourceStore::normalizePath(filePath));
		if (!idx)
			return out;
		const std::set<NumericFmtInfo> fmts = idx->ycomponent.value(subcaseId)
			.value(schema).value(pointId);
		for (const NumericFmtInfo& fmt : fmts)
			out << mCurveDataSourceStore::expandedComponents(fmt.fmt, fmt.transient);
		return out;
	}

	bool mChannelMapCore::saveCsv(const QList<Row>& rows, const QString& filePath,
		QString* err)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
		{
			if (err)
				*err = QStringLiteral("CSV 无法写入: %1").arg(file.errorString());
			return false;
		}
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out.setGenerateByteOrderMark(true);   //UTF-8 BOM（Excel 直开不乱码）
		out << QStringLiteral("曲线名,工况,结果类型,对象,分量\r\n");
		for (const Row& row : rows)
		{
			//映射后实际通道显示串（未匹配行写空列——导入时该行保持未配）
			const QString sub = row.mapped.complete
				? mCurveChannelSelector::subcaseText(row.mapped.subcaseId) : QString();
			const QString obj = row.mapped.complete
				? mCurveChannelSelector::objectText(row.mapped.pointId) : QString();
			out << row.curveName << QLatin1Char(',') << sub << QLatin1Char(',')
				<< (row.mapped.complete ? row.mapped.schema : QString()) << QLatin1Char(',')
				<< obj << QLatin1Char(',')
				<< (row.mapped.complete ? row.mapped.component : QString()) << "\r\n";
		}
		return true;
	}

	bool mChannelMapCore::loadCsv(const QString& filePath, QList<Row>* rows, QString* err)
	{
		if (!rows)
			return false;
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			if (err)
				*err = QStringLiteral("CSV 无法读取: %1").arg(file.errorString());
			return false;
		}
		QTextStream in(&file);
		in.setCodec("UTF-8");
		in.setAutoDetectUnicode(true);   //带 BOM 自动剥

		//逐行解析（逗号不进显示串，直 split 即可；跳过表头）
		QHash<QString, QStringList> byName;
		bool first = true;
		while (!in.atEnd())
		{
			QString line = in.readLine();
			line.remove(QLatin1Char('\r'));
			if (line.isEmpty())
				continue;
			const QStringList cols = line.split(QLatin1Char(','));
			if (first)
			{
				first = false;
				continue;
			}
			if (cols.isEmpty() || cols.at(0).isEmpty())
				continue;
			byName.insert(cols.at(0), cols);
		}

		mCurveDataSourceStore* store = mCurveDataSourceStore::getInstance();
		for (Row& row : *rows)
		{
			const QStringList cols = byName.value(row.curveName);
			if (cols.size() < 5)
				continue;   //无对齐行/列不足：保持现状
			if (cols.at(1).isEmpty() || cols.at(2).isEmpty()
				|| cols.at(3).isEmpty() || cols.at(4).isEmpty())
				continue;   //空映射列：保持未配
			//存在性探针（optimize-channel-map-perf，与装配成败严格互充）
			const ChannelSelection sel = mCurveChannelSelector::fromModelTexts(
				row.mapped.filePath, cols.at(1), cols.at(2), cols.at(3), cols.at(4));
			QString perr;
			if (!store->hasChannelData(row.mapped.filePath, sel.subcaseId, sel.schema,
				sel.pointId, sel.component, &perr))
				continue;   //导入映射装配失败：该行保持未配（不产假匹配）
			ChannelSelection ok = sel;
			ok.complete = true;
			row.mapped = ok;
			row.confirmed = false;
		}
		return true;
	}

	double mChannelMapCore::similarity(const QString& a, const QString& b)
	{
		const QString na = normalizeToken(a);
		const QString nb = normalizeToken(b);
		if (na.isEmpty() || nb.isEmpty())
			return 0.0;
		return 1.0 - double(editDistance(na, nb)) / qMax(na.size(), nb.size());
	}
}
