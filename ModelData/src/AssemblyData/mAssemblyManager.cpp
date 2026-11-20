//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mAssemblyManager.h"
#include <QCollator>

namespace AssemblyData
{
	//名称自然排序（数字按大小、字母按顺序，满足自由区/顶层装配显示排序要求）
	static bool assemblyNameLess(const QString& a, const QString& b)
	{
		QCollator collator;
		collator.setNumericMode(true);
		collator.setCaseSensitivity(Qt::CaseInsensitive);
		return collator.compare(a, b) < 0;
	}

	mAssemblyManager::mAssemblyManager()
	{
	}

	mAssemblyManager::~mAssemblyManager()
	{
		clear();
	}

	int mAssemblyManager::createAssembly(const QString& name)
	{
		const int id = _nextAssemblyID++;
		mAssemblyData* data = new mAssemblyData();
		data->_assemblyID = id;
		data->_name = name;
		_assemblys.insert(id, data);
		return id;
	}

	bool mAssemblyManager::renameAssembly(int assemblyId, const QString& name)
	{
		mAssemblyData* data = _assemblys.value(assemblyId, nullptr);
		if (data == nullptr)
			return false;
		data->_name = name;
		return true;
	}

	bool mAssemblyManager::removeAssembly(int assemblyId)
	{
		mAssemblyData* data = _assemblys.value(assemblyId, nullptr);
		if (data == nullptr)
			return false;

		//解除父装配对本装配的引用
		const auto parents = _asmParents.value(assemblyId);
		for (int parentId : parents)
		{
			_asmChildrenAsm[parentId].remove(assemblyId);
		}
		_asmParents.remove(assemblyId);

		//解除本装配对子装配的引用（子装配保留其他父装配，失去全部父装配则转为顶层）
		const auto childAsms = _asmChildrenAsm.value(assemblyId);
		for (int childId : childAsms)
		{
			_asmParents[childId].remove(assemblyId);
		}
		_asmChildrenAsm.remove(assemblyId);

		//解除本装配对部件的引用
		const auto childParts = _asmChildrenPart.value(assemblyId);
		for (int partId : childParts)
		{
			_partParents[partId].remove(assemblyId);
		}
		_asmChildrenPart.remove(assemblyId);

		delete data;
		_assemblys.remove(assemblyId);
		return true;
	}

	int mAssemblyManager::cloneAssembly(int sourceAssemblyId, const QString& newName)
	{
		mAssemblyData* source = _assemblys.value(sourceAssemblyId, nullptr);
		if (source == nullptr)
			return -1;

		const int newId = createAssembly(newName);

		//子装配与子部件均挂为共享引用；新装配无父无子，挂载不可能成环
		const auto childAsms = _asmChildrenAsm.value(sourceAssemblyId);
		for (int childId : childAsms)
		{
			addChildAssembly(newId, childId);
		}
		const auto childParts = _asmChildrenPart.value(sourceAssemblyId);
		for (int partId : childParts)
		{
			addChildPart(newId, partId);
		}
		return newId;
	}

	bool mAssemblyManager::containsAssembly(int assemblyId) const
	{
		return _assemblys.contains(assemblyId);
	}

	QString mAssemblyManager::getAssemblyName(int assemblyId) const
	{
		mAssemblyData* data = _assemblys.value(assemblyId, nullptr);
		return data == nullptr ? QString() : data->_name;
	}

	bool mAssemblyManager::existAssemblyName(const QString& name) const
	{
		for (auto it = _assemblys.constBegin(); it != _assemblys.constEnd(); ++it)
		{
			if (it.value()->_name == name)
				return true;
		}
		return false;
	}

	int mAssemblyManager::getIDByAssemblyName(const QString& name) const
	{
		for (auto it = _assemblys.constBegin(); it != _assemblys.constEnd(); ++it)
		{
			if (it.value()->_name == name)
				return it.key();
		}
		return -1;
	}

	QVector<int> mAssemblyManager::getAllAssemblyIDs() const
	{
		QVector<int> ids;
		ids.reserve(_assemblys.size());
		for (auto it = _assemblys.constBegin(); it != _assemblys.constEnd(); ++it)
		{
			ids.push_back(it.key());
		}
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	QStringList mAssemblyManager::getAllAssemblyNames() const
	{
		QStringList names;
		for (auto it = _assemblys.constBegin(); it != _assemblys.constEnd(); ++it)
		{
			names.push_back(it.value()->_name);
		}
		return names;
	}

	bool mAssemblyManager::addChildAssembly(int parentAssemblyId, int childAssemblyId)
	{
		if (!_assemblys.contains(parentAssemblyId) || !_assemblys.contains(childAssemblyId))
			return false;

		//禁止自引用与循环从属
		if (wouldCreateCycle(parentAssemblyId, childAssemblyId))
			return false;

		_asmChildrenAsm[parentAssemblyId].insert(childAssemblyId);
		_asmParents[childAssemblyId].insert(parentAssemblyId);
		return true;
	}

	bool mAssemblyManager::addChildPart(int parentAssemblyId, int partId)
	{
		if (!_assemblys.contains(parentAssemblyId))
			return false;

		_asmChildrenPart[parentAssemblyId].insert(partId);
		_partParents[partId].insert(parentAssemblyId);
		return true;
	}

	bool mAssemblyManager::removeChildAssembly(int parentAssemblyId, int childAssemblyId)
	{
		if (!_asmChildrenAsm.value(parentAssemblyId).contains(childAssemblyId))
			return false;

		_asmChildrenAsm[parentAssemblyId].remove(childAssemblyId);
		_asmParents[childAssemblyId].remove(parentAssemblyId);
		return true;
	}

	bool mAssemblyManager::removeChildPart(int parentAssemblyId, int partId)
	{
		if (!_asmChildrenPart.value(parentAssemblyId).contains(partId))
			return false;

		_asmChildrenPart[parentAssemblyId].remove(partId);
		_partParents[partId].remove(parentAssemblyId);
		return true;
	}

	QVector<int> mAssemblyManager::childAssemblies(int assemblyId) const
	{
		QVector<int> result;
		const auto children = _asmChildrenAsm.value(assemblyId);
		for (int childId : children)
		{
			result.push_back(childId);
		}
		//按名称排序，保证树显示顺序稳定
		std::sort(result.begin(), result.end(), [this](int a, int b)
		{
			return assemblyNameLess(getAssemblyName(a), getAssemblyName(b));
		});
		return result;
	}

	QVector<int> mAssemblyManager::childParts(int assemblyId) const
	{
		QVector<int> result;
		const auto children = _asmChildrenPart.value(assemblyId);
		for (int partId : children)
		{
			result.push_back(partId);
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	QVector<int> mAssemblyManager::assemblyParents(int assemblyId) const
	{
		QVector<int> result;
		const auto parents = _asmParents.value(assemblyId);
		for (int parentId : parents)
		{
			result.push_back(parentId);
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	QVector<int> mAssemblyManager::partParents(int partId) const
	{
		QVector<int> result;
		const auto parents = _partParents.value(partId);
		for (int parentId : parents)
		{
			result.push_back(parentId);
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	bool mAssemblyManager::isPartReferenced(int partId) const
	{
		return !_partParents.value(partId).isEmpty();
	}

	QSet<int> mAssemblyManager::referencedPartIds() const
	{
		QSet<int> result;
		for (auto it = _partParents.constBegin(); it != _partParents.constEnd(); ++it)
		{
			if (!it.value().isEmpty())
				result.insert(it.key());
		}
		return result;
	}

	bool mAssemblyManager::wouldCreateCycle(int parentAssemblyId, int childAssemblyId) const
	{
		if (parentAssemblyId == childAssemblyId)
			return true;
		//parent 是 child 的后代时，child→parent 已有路径，再挂 parent→child 即成环
		return isReachable(childAssemblyId, parentAssemblyId);
	}

	QVector<int> mAssemblyManager::topAssemblies() const
	{
		QVector<int> result;
		for (auto it = _assemblys.constBegin(); it != _assemblys.constEnd(); ++it)
		{
			if (_asmParents.value(it.key()).isEmpty())
				result.push_back(it.key());
		}
		std::sort(result.begin(), result.end(), [this](int a, int b)
		{
			return assemblyNameLess(getAssemblyName(a), getAssemblyName(b));
		});
		return result;
	}

	QVector<int> mAssemblyManager::descendantAssemblies(int assemblyId) const
	{
		QVector<int> result;
		QSet<int> visited;
		QVector<int> stack;
		const auto roots = _asmChildrenAsm.value(assemblyId);
		for (int childId : roots)
			stack.push_back(childId);
		while (!stack.isEmpty())
		{
			const int current = stack.takeLast();
			if (visited.contains(current))
				continue;
			visited.insert(current);
			result.push_back(current);
			const auto children = _asmChildrenAsm.value(current);
			for (int childId : children)
			{
				if (!visited.contains(childId))
					stack.push_back(childId);
			}
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	QVector<int> mAssemblyManager::descendantPartIds(int assemblyId) const
	{
		QVector<int> result;
		QSet<int> visitedAsm;
		QVector<int> stack{ assemblyId };
		while (!stack.isEmpty())
		{
			const int current = stack.takeLast();
			if (visitedAsm.contains(current))
				continue;
			visitedAsm.insert(current);
			const auto parts = _asmChildrenPart.value(current);
			for (int partId : parts)
				result.push_back(partId);
			const auto children = _asmChildrenAsm.value(current);
			for (int childId : children)
			{
				if (!visitedAsm.contains(childId))
					stack.push_back(childId);
			}
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	void mAssemblyManager::removePartReferences(int partId)
	{
		const auto parents = _partParents.value(partId);
		for (int parentId : parents)
		{
			_asmChildrenPart[parentId].remove(partId);
		}
		_partParents.remove(partId);
	}

	void mAssemblyManager::clear()
	{
		qDeleteAll(_assemblys);
		_assemblys.clear();
		_asmChildrenAsm.clear();
		_asmChildrenPart.clear();
		_asmParents.clear();
		_partParents.clear();
		_nextAssemblyID = 1;
	}

	bool mAssemblyManager::isReachable(int from, int target) const
	{
		QSet<int> visited;
		QVector<int> stack{ from };
		while (!stack.isEmpty())
		{
			const int current = stack.takeLast();
			if (current == target)
				return true;
			if (visited.contains(current))
				continue;
			visited.insert(current);
			const auto children = _asmChildrenAsm.value(current);
			for (int childId : children)
			{
				if (!visited.contains(childId))
					stack.push_back(childId);
			}
		}
		return false;
	}

	QDataStream& operator<<(QDataStream& out, const mAssemblyManager& mgr)
	{
		//按 ID 升序写出实体与关系，保证文件内容稳定
		const QVector<int> ids = mgr.getAllAssemblyIDs();
		out << (qint32)ids.size();
		for (int id : ids)
		{
			out << (qint32)id << mgr.getAssemblyName(id);

			const QVector<int> childAsms = mgr.childAssemblies(id);
			out << (qint32)childAsms.size();
			for (int childId : childAsms)
				out << (qint32)childId;

			const QVector<int> childParts = mgr.childParts(id);
			out << (qint32)childParts.size();
			for (int partId : childParts)
				out << (qint32)partId;
		}
		return out;
	}

	QDataStream& operator>>(QDataStream& in, mAssemblyManager& mgr)
	{
		mgr.clear();

		qint32 count = 0;
		in >> count;
		for (qint32 i = 0; i < count; ++i)
		{
			qint32 id = -1;
			QString name;
			in >> id >> name;

			mAssemblyData* data = new mAssemblyData();
			data->_assemblyID = id;
			data->_name = name;
			mgr._assemblys.insert(id, data);
			if (id >= mgr._nextAssemblyID)
				mgr._nextAssemblyID = id + 1;

			qint32 childAsmCount = 0;
			in >> childAsmCount;
			for (qint32 j = 0; j < childAsmCount; ++j)
			{
				qint32 childId = -1;
				in >> childId;
				mgr._asmChildrenAsm[id].insert(childId);
				mgr._asmParents[childId].insert(id);
			}

			qint32 childPartCount = 0;
			in >> childPartCount;
			for (qint32 j = 0; j < childPartCount; ++j)
			{
				qint32 partId = -1;
				in >> partId;
				mgr._asmChildrenPart[id].insert(partId);
				mgr._partParents[partId].insert(id);
			}
		}
		return in;
	}
}
