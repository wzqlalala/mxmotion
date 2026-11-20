#include "mFrpManager.h"
#include "mFrpBaseData.h"
#include "mFrpDerivedData.h"
#include <algorithm>

using namespace MxFunctions;

namespace MechFrpData
{
	/* ---------------- mFrpManager（容器 / 管理者） ---------------- */
	mFrpManager::mFrpManager()
	{
	}

	mFrpManager::~mFrpManager()
	{
		clear();
	}

	const QVector<mFrpBaseData*>& mFrpManager::roots() const
	{
		return m_roots;
	}

	const QStringList mFrpManager::getAllRootsName() const
	{
		QStringList names;
		names.reserve(m_roots.size());
		for (const MechFrpData::mFrpBaseData* root : m_roots)
			names.append(root->name);
		return names;
	}

	mFrpBaseData* mFrpManager::getFrpBaseData(const QString& id)
	{
		for (mFrpBaseData* r : m_roots)
			if (mFrpBaseData* f = r->findById(id)) return f;
		return nullptr;
	}

	void mFrpManager::clear()
	{
		qDeleteAll(m_roots);
		m_roots.clear();
		m_idSeq = m_analysisSeq = m_materialSeq = m_microSeq = m_failureSeq = m_funSeq = 0;
	}

	// 唯一“加子节点”原语：界面先按自身规则表确定 nodeType，再调用本函数。
	// 数据层不承载 UI 新增策略，只做父/子类型匹配与不变量防御（相节点基体唯一）、构造、命名、id 与序号维护。
	mFrpBaseData* mFrpManager::addChildOfType(mFrpBaseData* parent, FrpNodeType nodeType)
	{
		if (!parent) return nullptr;

		bool parentOk = false;
		switch (nodeType)
		{
		case FrpNode_Micro1:                                   // 微观结构分组 -> 微观结构N
			parentOk = (parent->type == FrpNode_Micro);
			break;
		case FrpNode_Material:                                // 仅“材料”分组下可加材料实例
			parentOk = (parent->type == FrpNode_Material && parent->isGroupData());
			break;
		case FrpNode_Failure:                                 // 仅“失效”分组下可加失效准则实例
			parentOk = (parent->type == FrpNode_Failure && parent->isGroupData());
			break;
		case FrpNode_AspectRatio:                             // 仅“长细比函数”分组下可加 Fun 实例
			parentOk = (parent->type == FrpNode_AspectRatio && parent->isGroupData());
			break;
		case FrpNode_Matrix:                                  // 四相：必须挂在微观结构N 下，且基体唯一
		case FrpNode_Inclusion:
		case FrpNode_Cavity:
		case FrpNode_Fiber:
			parentOk = canAttachPhase(parent, nodeType);
			break;
		default:
			return nullptr;
		}
		if (!parentOk) return nullptr;

		mFrpBaseData* data = mFrpBaseData::createByType(nodeType);
		data->name = suggestName(parent, nodeType);
		parent->addChild(data);
		data->id = genId();
		// 维护各类实例的全局递增序号（独立计数，删除后再增不重名；相节点无独立序号）
		switch (nodeType)
		{
		case FrpNode_Material:   ++m_materialSeq; break;
		case FrpNode_Micro1:     ++m_microSeq;    break;
		case FrpNode_Failure:    ++m_failureSeq;  break;
		case FrpNode_AspectRatio:++m_funSeq;      break;
		default: break;
		}
		return data;
	}

	mFrpBaseData* mFrpManager::addAnalysisRoot()
	{
		mFrpAnalysisData* a = new mFrpAnalysisData;
		a->name = QStringLiteral("均质化模型%1").arg(++m_analysisSeq);
		m_roots.append(a);
		buildDefaultGroups(a);
		reassignIds(a);
		return a;
	}
	void mFrpManager::buildDefaultGroups(mFrpBaseData* a)
	{
		if (!a) return;
		a->addChild(new mFrpBaseData(FrpNode_Material, QStringLiteral("材料")));
		a->addChild(new mFrpBaseData(FrpNode_Micro, QStringLiteral("微观结构")));
		mFrpRveData* rve = new mFrpRveData();          // RVE 带数据，必须建派生类（基类实例会导致页面反显/保存与写盘校验失败）
		rve->name = QStringLiteral("RVE");
		a->addChild(rve);
		a->addChild(new mFrpBaseData(FrpNode_Failure, QStringLiteral("失效")));
		a->addChild(new mFrpBaseData(FrpNode_AspectRatio, QStringLiteral("长细比函数")));
	}

	bool mFrpManager::canAttachPhase(const mFrpBaseData* micro, FrpNodeType nodeType, const mFrpBaseData* ignoredPhase)
	{
		if (!micro || micro->type != FrpNode_Micro1)
			return false;
		if (nodeType != FrpNode_Matrix && nodeType != FrpNode_Inclusion
			&& nodeType != FrpNode_Cavity && nodeType != FrpNode_Fiber)
			return false;
		if (nodeType != FrpNode_Matrix)
			return true;
		for (const mFrpBaseData* child : micro->children)
			if (child != ignoredPhase && child->type == FrpNode_Matrix)
				return false;
		return true;
	}

	QString mFrpManager::phaseName(MxFunctions::FrpNodeType phaseKind)
	{
		switch (phaseKind)
		{
		case FrpNode_Inclusion: return QStringLiteral("掺杂");
		case FrpNode_Cavity:    return QStringLiteral("空腔");
		case FrpNode_Fiber:     return QStringLiteral("连续纤维");
		case FrpNode_Matrix:
		default:              return QStringLiteral("基体");
		}
	}

	// 深拷贝节点（含整棵子树）到同级；副本名为“源名_copy”，同级重名则继续追加“_copy”
	mFrpBaseData* mFrpManager::duplicateNode(mFrpBaseData* src)
	{
		if (!src) return nullptr;
		if (src->isGroupData() || src->type == FrpNode_Rve) return nullptr;      // 分组、RVE 不可复制
		if (src->type == FrpNode_Matrix && !canAttachPhase(src->m_pParent, FrpNode_Matrix))
			return nullptr;

		mFrpBaseData* copy = src->clone(src->m_pParent);
		const bool inMicroTree = (src->type == FrpNode_Micro1 ||
			src->type == FrpNode_Matrix || src->type == FrpNode_Inclusion ||
			src->type == FrpNode_Cavity || src->type == FrpNode_Fiber);
		mFrpBaseData* mg = inMicroTree ? src->ancestorOfType(FrpNode_Micro) : nullptr;
		const QVector<mFrpBaseData*>& sibs = src->m_pParent ? src->m_pParent->children : m_roots;
		auto nameExists = [&](const QString& n) {
			if (mg) return microSubtreeNameExists(mg, n, nullptr);
			for (const mFrpBaseData* c : sibs) if (c->name == n) return true;
			return false;
			};
		QString cand = src->name + QStringLiteral("_copy");
		while (nameExists(cand)) cand += QStringLiteral("_copy");
		copy->name = cand;
		copy->renamed = true;        // 副本名视为已定名（相副本切换相类型时不再被自动改名覆盖）

		if (src->m_pParent) src->m_pParent->addChild(copy);
		else             m_roots.append(copy);
		// 复制微观结构N：副本内相名也须在整个微观结构分组子树唯一，重名追加 _copy
		if (src->type == FrpNode_Micro1)
		{
			for (mFrpBaseData* ph : copy->children)
				if (microSubtreeNameExists(mg, ph->name, ph))
				{
					QString pc = ph->name;
					do { pc += QStringLiteral("_copy"); } while (microSubtreeNameExists(mg, pc, ph));
					ph->name = pc;
					ph->renamed = true;
				}
		}
		reassignIds(copy);
		return copy;
	}
	bool mFrpManager::removeNode(mFrpBaseData* data)
	{
		if (!data) return false;
		// 删材料实例：所属模型内引用该材料名的字段回落“请选择”（须在 delete 前、m_pParent 链仍有效时）
		if (data->type == FrpNode_Material && !data->isGroupData())
		{
			if (mFrpBaseData* an = data->ancestorOfType(FrpNode_Analysis1))
				remapMaterialRefs(an, data->name, QString());
		}
		else if (data->type == FrpNode_Micro1)
		{
			if (mFrpBaseData* an = data->ancestorOfType(FrpNode_Analysis1))
				remapMicrostructureRefs(an, data->name, QString());
		}
		else if (data->type == FrpNode_Micro && data->isGroupData())
		{
			if (mFrpBaseData* an = data->ancestorOfType(FrpNode_Analysis1))
				for (mFrpBaseData* micro : data->children)
					if (micro->type == FrpNode_Micro1)
						remapMicrostructureRefs(an, micro->name, QString());
		}
		if (!data->m_pParent)
		{
			if (!m_roots.removeOne(data)) return false;
			delete data;
			return true;
		}
		if (!data->m_pParent->children.removeOne(data)) return false;
		delete data;
		return true;
	}

	void mFrpManager::clearChildren(mFrpBaseData* node)
	{
		if (!node) return;
		// 清空“材料”分组：所属模型内相节点的材料引用全部回落“请选择”
		if (node->type == FrpNode_Material && node->isGroupData())
		{
			if (mFrpBaseData* an = node->ancestorOfType(FrpNode_Analysis1))
				for (mFrpBaseData* m : node->children)
					remapMaterialRefs(an, m->name, QString());
		}
		else if (node->type == FrpNode_Micro && node->isGroupData())
		{
			if (mFrpBaseData* an = node->ancestorOfType(FrpNode_Analysis1))
				for (mFrpBaseData* micro : node->children)
					if (micro->type == FrpNode_Micro1)
						remapMicrostructureRefs(an, micro->name, QString());
		}
		qDeleteAll(node->children);
		node->children.clear();
	}

	// 追加：保留现有根，把 other 的根并入；根级重名自动加 _N，重排 id，各类序号取大
	void mFrpManager::appendFrom(mFrpManager& other)
	{
		if (&other == this)
			return;
		auto rootNameExists = [&](const QString& nm) {
			for (const mFrpBaseData* x : m_roots)
				if (x->name == nm) return true;
			return false;
			};
		for (mFrpBaseData* r : other.m_roots)
		{
			if (rootNameExists(r->name))
			{
				int n = 2;
				while (rootNameExists(QStringLiteral("%1_%2").arg(r->name).arg(n))) ++n;
				r->name = QStringLiteral("%1_%2").arg(r->name).arg(n);
			}
			m_roots.append(r);
			reassignIds(r);
		}
		other.m_roots.clear();
		m_idSeq = qMax(m_idSeq, other.m_idSeq);
		m_analysisSeq = qMax(m_analysisSeq, other.m_analysisSeq);
		m_materialSeq = qMax(m_materialSeq, other.m_materialSeq);
		m_microSeq = qMax(m_microSeq, other.m_microSeq);
		m_failureSeq = qMax(m_failureSeq, other.m_failureSeq);
		m_funSeq = qMax(m_funSeq, other.m_funSeq);
	}

	bool mFrpManager::renameNode(mFrpBaseData* node, const QString& newName)
	{
		if (!node || newName.isEmpty()) return false;
		// 微观结构分组子树（微观结构N + 相节点）全局不重名；其余节点仍按同级判重
		const bool inMicroTree = (node->type == FrpNode_Micro1 ||
			node->type == FrpNode_Matrix || node->type == FrpNode_Inclusion ||
			node->type == FrpNode_Cavity || node->type == FrpNode_Fiber);
		if (inMicroTree)
		{
			if (mFrpBaseData* mg = node->ancestorOfType(FrpNode_Micro))
				if (microSubtreeNameExists(mg, newName, node)) return false;
		}
		else
		{
			const QVector<mFrpBaseData*>& sibs = node->m_pParent ? node->m_pParent->children : m_roots;
			for (const mFrpBaseData* s : sibs)
				if (s != node && s->name == newName) return false;
		}

		const QString oldName = node->name;
		if (oldName == newName) return true;
		node->name = newName;
		if (node->type == FrpNode_Material && !node->isGroupData())
		{
			if (mFrpBaseData* an = node->ancestorOfType(FrpNode_Analysis1))
				remapMaterialRefs(an, oldName, newName);
		}
		else if (node->type == FrpNode_Micro1)
		{
			if (mFrpBaseData* an = node->ancestorOfType(FrpNode_Analysis1))
				remapMicrostructureRefs(an, oldName, newName);
		}
		return true;
	}

	QString mFrpManager::suggestName(const mFrpBaseData* parentData, MxFunctions::FrpNodeType newType)
	{
		auto smallestFree = [&](const QString& prefix) -> QString {
			QStringList used;
			for (const mFrpBaseData* c : parentData->children)
				used << c->name;
			for (int n = 1; ; ++n)
			{
				const QString cand = prefix + QString::number(n);
				if (!used.contains(cand)) return cand;
			}
			};
		switch (newType)
		{
		case FrpNode_Material:  return smallestFree(QStringLiteral("Material_"));
		case FrpNode_Micro1:    return smallestFree(QStringLiteral("微观结构"));
		case FrpNode_Failure:   return smallestFree(QStringLiteral("失效准则_"));
		case FrpNode_AspectRatio:return smallestFree(QStringLiteral("Fun_"));
		case FrpNode_Matrix:    // 四种相节点默认名跟相类型；同名追加序号（作用域为本根微观结构分组子树）
		case FrpNode_Inclusion:
		case FrpNode_Cavity:
		case FrpNode_Fiber:
		{
			QString base = phaseName(newType);
			// 避让范围为整个微观结构分组子树（跨所有微观结构N 的相节点）
			mFrpBaseData* mg = parentData->m_pParent;   // parentData=微观结构N，其父=微观结构分组
			if (!microSubtreeNameExists(mg, base, nullptr)) return base;
			int k = 2;
			QString cand;
			do { cand = QStringLiteral("%1_%2").arg(base).arg(k++); } while (microSubtreeNameExists(mg, cand, nullptr));
			return cand;
		}
		default: return QStringLiteral("Data_%1").arg(m_idSeq + 1);
		}
	}

	QString mFrpManager::genId() { return QStringLiteral("frp_%1").arg(++m_idSeq); }
	void mFrpManager::reassignIds(mFrpBaseData* n)
	{
		if (!n) return;
		n->id = genId();
		for (mFrpBaseData* c : n->children) reassignIds(c);
	}

	// 遍历某均质化模型内全部相节点名称
	void mFrpManager::remapMaterialRefs(mFrpBaseData* root, const QString& oldName, const QString& newName)
	{
		if (!root || oldName.isEmpty()) return;
		auto remap = [&](QString& ref) { if (ref == oldName) ref = newName; };
		for (mFrpBaseData* c : root->children)
		{
			if (c->type != FrpNode_Micro) continue;        // 微观结构分组
			for (mFrpBaseData* ms : c->children)            // 微观结构N
				for (mFrpBaseData* ph : ms->children)        // 相节点
				{
					mFrpPhaseData* d = ph->as<mFrpPhaseData>();
					if (!d) continue;
					remap(d->baseMaterial);
					remap(d->inc.material);
					remap(d->inc.featureMat);
					remap(d->cav.surfaceMat);
					remap(d->fib.material);
					remap(d->fib.coatMat);
				}
		}
	}


	void mFrpManager::remapMicrostructureRefs(mFrpBaseData* root, const QString& oldName, const QString& newName)
	{
		if (!root || oldName.isEmpty()) return;
		for (mFrpBaseData* child : root->children)
		{
			mFrpRveData* rve = child->as<mFrpRveData>();
			if (!rve) continue;
			if (rve->structure == oldName)
				rve->structure = newName;
			for (RveLayer& layer : rve->layers)
				if (layer.micro == oldName)
					layer.micro = newName;
		}
	}

	bool mFrpManager::microSubtreeNameExists(const mFrpBaseData* microGroup,
		const QString& name, const mFrpBaseData* ignore) const
	{
		if (!microGroup) return false;
		for (const mFrpBaseData* ms : microGroup->children)   // 微观结构N
		{
			if (ms != ignore && ms->name == name) return true;
			for (const mFrpBaseData* ph : ms->children)       // 相节点
				if (ph != ignore && ph->name == name) return true;
		}
		return false;
	}

	// 窗口存储序 a11;a12;a13;a22;a23;a33（索引 0..5）-> 求解器序 a11,a22,a33,a23,a13,a12
	QStringList mFrpManager::reorderTensor6ToAbaqus(const QStringList& store)
	{
		static const int kTo[6] = { 0, 3, 5, 4, 2, 1 };
		QStringList out;
		for (int i = 0; i < 6; ++i)
			out << store.value(kTo[i]);
		return out;
	}

	// 求解器序 a11,a22,a33,a23,a13,a12 -> 窗口存储序 a11;a12;a13;a22;a23;a33（kTo 的逆映射）
	QStringList mFrpManager::reorderTensor6FromAbaqus(const QStringList& abaqus)
	{
		static const int kFrom[6] = { 0, 5, 4, 1, 3, 2 };
		QStringList store;
		for (int i = 0; i < 6; ++i)
			store << abaqus.value(kFrom[i]);
		return store;
	}
}
