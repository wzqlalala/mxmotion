#include "mPostDerivedCaseStore.h"

#include <QDataStream>
#include <QPair>
#include <QSet>
#include <QStack>

#include <algorithm>
#include <functional>

namespace PostWindow
{
	mPostDerivedCaseStore& mPostDerivedCaseStore::instance()
	{
		static mPostDerivedCaseStore store;
		return store;
	}

	QList<DerivedCaseDefinition> mPostDerivedCaseStore::definitions(int modelKey) const
	{
		return _buckets.value(modelKey);
	}

	QList<DerivedCaseDefinition> mPostDerivedCaseStore::definitions(int modelKey, DerivedCaseKind kind) const
	{
		QList<DerivedCaseDefinition> result;
		const QList<DerivedCaseDefinition>& all = _buckets.value(modelKey);
		for (const DerivedCaseDefinition& def : all)
			if (def.kind == kind)
				result.append(def);
		return result;
	}

	bool mPostDerivedCaseStore::definition(int modelKey, int definitionId, DerivedCaseDefinition* out) const
	{
		const QList<DerivedCaseDefinition>& all = _buckets.value(modelKey);
		for (const DerivedCaseDefinition& def : all)
		{
			if (def.id != definitionId)
				continue;
			if (out != nullptr)
				*out = def;
			return true;
		}
		return false;
	}

	bool mPostDerivedCaseStore::bucketNameOk(int modelKey, const QString& name, int exceptId) const
	{
		if (name.trimmed().isEmpty())
			return false;
		return !nameExists(modelKey, name.trimmed(), exceptId);
	}

	int mPostDerivedCaseStore::appendDefinition(int modelKey, const DerivedCaseDefinition& def)
	{
		if (!bucketNameOk(modelKey, def.name, -1))
			return -1;
		DerivedCaseDefinition added = def;
		added.id = _nextId++;
		added.name = added.name.trimmed();
		_buckets[modelKey].append(added);
		notifyChanged(modelKey);
		return added.id;
	}

	bool mPostDerivedCaseStore::updateDefinition(int modelKey, const DerivedCaseDefinition& def)
	{
		if (def.id <= 0 || !bucketNameOk(modelKey, def.name, def.id))
			return false;
		QList<DerivedCaseDefinition>& all = _buckets[modelKey];
		for (DerivedCaseDefinition& existing : all)
		{
			if (existing.id != def.id)
				continue;
			DerivedCaseDefinition updated = def;
			updated.name = updated.name.trimmed();
			existing = updated;
			notifyChanged(modelKey);
			return true;
		}
		return false;
	}

	mPostDerivedCaseStore::RemoveResult mPostDerivedCaseStore::removeDefinition(
		int modelKey, int definitionId, QList<int>* referencedBy)
	{
		QList<DerivedCaseDefinition>& all = _buckets[modelKey];
		const auto it = std::find_if(all.begin(), all.end(),
			[definitionId](const DerivedCaseDefinition& d) { return d.id == definitionId; });
		if (it == all.end())
			return RemoveResult::NotFound;

		//引用完整性：其它定义的派生来源行仍引用本定义时拒绝删除（先删引用方）
		QList<int> referrers;
		for (const DerivedCaseDefinition& def : all)
		{
			if (def.id == definitionId)
				continue;
			for (const DerivedSourceRow& row : def.rows)
			{
				if (row.isDerived && row.derivedId == definitionId)
				{
					referrers.append(def.id);
					break;
				}
			}
		}
		if (!referrers.isEmpty())
		{
			if (referencedBy != nullptr)
				*referencedBy = referrers;
			return RemoveResult::Referenced;
		}

		all.erase(it);
		notifyChanged(modelKey);
		return RemoveResult::Removed;
	}

	void mPostDerivedCaseStore::removeModel(int modelKey)
	{
		_buckets.remove(modelKey);
	}

	bool mPostDerivedCaseStore::nameExists(int modelKey, const QString& name, int exceptId) const
	{
		const QList<DerivedCaseDefinition>& all = _buckets.value(modelKey);
		for (const DerivedCaseDefinition& def : all)
		{
			if (def.id != exceptId && def.name == name)
				return true;
		}
		return false;
	}

	QString mPostDerivedCaseStore::uniqueName(int modelKey, const QString& base) const
	{
		int index = 1;
		QString candidate = base + QStringLiteral("-%1").arg(index);
		while (nameExists(modelKey, candidate, -1))
		{
			++index;
			candidate = base + QStringLiteral("-%1").arg(index);
		}
		return candidate;
	}

	mPostDerivedCaseStore::ReferenceError mPostDerivedCaseStore::checkReferences(
		const QList<DerivedCaseDefinition>& defs, int editingId,
		const QList<int>& draftRefs, int candidateDerivedId)
	{
		/*
		* 原型 validateDefinition（自步序列/包络/线性叠加三面板重复实现收敛）：
		* 以"editingId 的草稿启用引用 + candidate"模拟编辑中定义，对全部定义建
		* 引用图做 DFS——环 → Cycle；depth(定义) = 1 + max(其派生来源定义 depth)，
		* 纯原生来源 depth=1，> 2 → DepthExceeded；引用目标不在图内 → MissingTarget。
		*/
		QHash<int, QList<int>> graph;
		for (const DerivedCaseDefinition& def : defs)
		{
			QList<int> refs;
			if (def.id != editingId)
			{
				for (const DerivedSourceRow& row : def.rows)
					if (row.isDerived && row.derivedId > 0)
						refs << row.derivedId;
			}
			graph.insert(def.id, refs);
		}
		if (editingId > 0 && graph.contains(editingId))
		{
			QList<int> refs = draftRefs;
			if (candidateDerivedId > 0)
				refs << candidateDerivedId;
			graph.insert(editingId, refs);
		}

		//引用目标不存在（防御：存量集被外部改坏时的兜底口径）
		for (auto it = graph.cbegin(); it != graph.cend(); ++it)
		{
			for (int ref : it.value())
			{
				if (!graph.contains(ref))
					return ReferenceError::MissingTarget;
			}
		}

		//DFS：环检测 + 深度计算（迭代式，避免深链递归栈溢出）
		enum class Mark { None, Active, Done };
		QHash<int, int> mark;	//Mark 枚举值
		QHash<int, int> depth;
		for (auto it = graph.cbegin(); it != graph.cend(); ++it)
		{
			if (mark.value(it.key()) != static_cast<int>(Mark::None))
				continue;
			QStack<QPair<int, int>> stack;	//<defId, nextChildIndex>
			stack.push(qMakePair(it.key(), 0));
			mark[it.key()] = static_cast<int>(Mark::Active);
			while (!stack.isEmpty())
			{
				QPair<int, int>& top = stack.top();
				const QList<int>& refs = graph.value(top.first);
				if (top.second < refs.size())
				{
					const int next = refs.at(top.second++);
					if (mark.value(next) == static_cast<int>(Mark::Active))
						return ReferenceError::Cycle;
					if (mark.value(next) == static_cast<int>(Mark::None))
					{
						mark[next] = static_cast<int>(Mark::Active);
						stack.push(qMakePair(next, 0));
					}
				}
				else
				{
					mark[top.first] = static_cast<int>(Mark::Done);
					int level = 1;
					for (int ref : refs)
						level = qMax(level, 1 + depth.value(ref, 1));
					depth[top.first] = level;
					if (level > 2)
						return ReferenceError::DepthExceeded;
					stack.pop();
				}
			}
		}
		return ReferenceError::None;
	}

	bool mPostDerivedCaseStore::expandDefinition(const QList<DerivedCaseDefinition>& defs,
		int definitionId, QList<ExpandedSource>* out, ReferenceError* errorKind)
	{
		/*
		* 展开为"按定义行序的原生来源列表"，不合并同帧——线性叠加的"同帧比例
		* 相加"语义由消费方按 frameId/rowId 自行归并（步序列同帧可重复出现为
		* 独立步骤，禁止在此合并）。
		*/
		if (out == nullptr)
			return false;
		out->clear();
		QHash<int, int> indexOf;
		for (int i = 0; i < defs.size(); ++i)
			indexOf.insert(defs.at(i).id, i);

		ReferenceError fail = ReferenceError::None;
		QSet<int> onPath;
		std::function<void(int, double, int)> walk = [&](int defId, double multiplier, int depth)
		{
			if (fail != ReferenceError::None)
				return;
			if (depth > 2)
			{
				fail = ReferenceError::DepthExceeded;
				return;
			}
			const int index = indexOf.value(defId, -1);
			if (index < 0)
			{
				fail = ReferenceError::MissingTarget;
				return;
			}
			if (onPath.contains(defId))
			{
				fail = ReferenceError::Cycle;
				return;
			}
			onPath.insert(defId);
			const DerivedCaseDefinition& def = defs.at(index);
			for (const DerivedSourceRow& row : def.rows)
			{
				bool ok = false;
				const double scale = row.scale.trimmed().toDouble(&ok);
				if (!ok || !qIsFinite(scale))
				{
					fail = ReferenceError::InvalidScale;
					break;
				}
				if (row.isDerived)
				{
					walk(row.derivedId, scale * multiplier, depth + 1);
					if (fail != ReferenceError::None)
						break;
				}
				else
				{
					ExpandedSource source;
					source.caseName = row.name;
					source.frameName = row.frame;
					source.rowId = row.id;
					source.frameId = row.frameId;
					source.scale = scale * multiplier;
					out->append(source);
				}
			}
			onPath.remove(defId);
		};
		walk(definitionId, 1.0, 1);

		if (fail != ReferenceError::None)
		{
			out->clear();
			if (errorKind != nullptr)
				*errorKind = fail;
			return false;
		}
		return true;
	}

	QByteArray mPostDerivedCaseStore::serialize(int modelKey) const
	{
		QByteArray buffer;
		QDataStream out(&buffer, QIODevice::WriteOnly);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		out.setVersion(QDataStream::Qt_5_13);
#else
		out.setVersion(QDataStream::Qt_5_12);
#endif
		const QList<DerivedCaseDefinition>& all = _buckets.value(modelKey);
		out << quint32(all.size());
		for (const DerivedCaseDefinition& def : all)
		{
			out << qint32(def.id) << qint32(static_cast<int>(def.kind))
				<< def.name << qint32(def.envelopeType) << def.configured;
			out << quint32(def.rows.size());
			for (const DerivedSourceRow& row : def.rows)
			{
				out << row.id << row.name << row.frame << row.scale
					<< row.isDerived << qint32(row.derivedId) << qint32(row.frameId);
			}
		}
		return buffer;
	}

	int mPostDerivedCaseStore::deserialize(int modelKey, const QByteArray& blob)
	{
		QDataStream in(blob);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
		in.setVersion(QDataStream::Qt_5_13);
#else
		in.setVersion(QDataStream::Qt_5_12);
#endif
		quint32 count = 0;
		in >> count;
		QList<DerivedCaseDefinition> parsed;
		for (quint32 i = 0; i < count; ++i)
		{
			if (in.status() != QDataStream::Ok)
				break;	//载荷截断/垃圾：count 可能为天文数字，读失败即跳出（尾部 status 拒绝）
			DerivedCaseDefinition def;
			qint32 kind = 0, envelopeType = 1;
			quint32 rowCount = 0;
			in >> def.id >> kind >> def.name >> envelopeType >> def.configured >> rowCount;
			def.kind = static_cast<DerivedCaseKind>(kind);
			def.envelopeType = envelopeType;
			for (quint32 r = 0; r < rowCount; ++r)
			{
				DerivedSourceRow row;
				qint32 derivedId = -1, frameId = 0;
				in >> row.id >> row.name >> row.frame >> row.scale >> row.isDerived >> derivedId >> frameId;
				row.derivedId = derivedId;
				row.frameId = frameId;
				def.rows.append(row);
			}
			parsed.append(def);
		}
		if (in.status() != QDataStream::Ok)
			return -1;

		//整桶替换（会话恢复时为新建空桶）；id 计数器抬升防新发号撞已恢复 id
		_buckets[modelKey] = parsed;
		for (const DerivedCaseDefinition& def : parsed)
			_nextId = qMax(_nextId, def.id + 1);
		notifyChanged(modelKey);
		return parsed.size();
	}

	void mPostDerivedCaseStore::addChangeListener(const std::function<void(int)>& listener)
	{
		_listeners.append(listener);
	}

	void mPostDerivedCaseStore::clearAll_forTest()
	{
		_buckets.clear();
		_nextId = 1;
		_listeners.clear();
	}

	void mPostDerivedCaseStore::notifyChanged(int modelKey)
	{
		const QList<std::function<void(int)>> listeners = _listeners;
		for (const std::function<void(int)>& listener : listeners)
			listener(modelKey);
	}
}
