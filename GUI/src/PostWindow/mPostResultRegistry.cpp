#include "mPostResultRegistry.h"

namespace PostWindow
{
void mPostResultRegistry::insert(const QString& key, const PostJumpRecord& record)
	{
		_table.insert(key, record);
	}

	const mPostResultRegistry::PostJumpRecord* mPostResultRegistry::find(const QString& key) const
	{
		const auto it = _table.constFind(key);
		return it == _table.constEnd() ? nullptr : &it.value();
	}

	bool mPostResultRegistry::takeByKey(const QString& key, PostJumpRecord* outRecord)
	{
		const auto it = _table.find(key);
		if (it == _table.end())
		{
			return false;
		}
		if (outRecord)
		{
			*outRecord = it.value();
		}
		_table.erase(it);
		return true;
	}

	bool mPostResultRegistry::invalidateKey(const QString& key)
	{
		if (!_table.remove(key))
		{
			return false;
		}
		if (onKeyInvalidated)
		{
			onKeyInvalidated(key);
		}
		return true;
	}

	QStringList mPostResultRegistry::removeByPostModelID(int postModelID)
	{
		QStringList removed;
		for (auto it = _table.begin(); it != _table.end(); )
		{
			if (it.value().postModelID == postModelID)
			{
				removed << it.key();
				it = _table.erase(it);
			}
			else
			{
				++it;
			}
		}
		if (onKeyInvalidated)
		{
			for (const QString& key : removed)
			{
				onKeyInvalidated(key);
			}
		}
		return removed;
	}

	const mPostResultRegistry::PostJumpRecord* mPostResultRegistry::findByPostModelID(int postModelID) const
	{
		//按记录内 postModelID 反查（mtime"删旧重建"后 key≠模型ID，不能按 key 查）。
		//同一模型至多一条记录（漏斗每次创建新模型；删旧重建原key覆盖）
		for (auto it = _table.constBegin(); it != _table.constEnd(); ++it)
		{
			if (it.value().postModelID == postModelID)
				return &it.value();
		}
		return nullptr;
	}
}
