#include "mFrpBaseData.h"
#include "mFrpManager.h"
#include "mFrpDerivedData.h"

namespace MechFrpData
{
	/* ---------------- mFrpBaseData（基类：节点身份 + 树操作 + 多态深拷贝） ---------------- */
	mFrpBaseData::mFrpBaseData(FrpNodeType t, const QString& n, mFrpBaseData* p)
		: type(t), name(n), m_pParent(p)
	{
	}

	// 拷贝构造只复制节点自身字段：id 不复制（由 reassignIds 重新分配）、
	// m_pParent 走类内默认初值置空、children 默认空（深拷贝子树由 clone/cloneChildrenTo 负责）
	mFrpBaseData::mFrpBaseData(const mFrpBaseData& other)
		: type(other.type), name(other.name), renamed(other.renamed)
	{
	}

	mFrpBaseData::~mFrpBaseData()
	{
		qDeleteAll(children);   // 虚析构：连带释放整棵子树（每个子节点本身就是数据对象）
		children.clear();
	}

	// 按“节点类型”新建对应数据对象（供规则新增用）；无字段节点(Micro 分组、Micro1 微观结构N 等)返回基类实例
	mFrpBaseData* mFrpBaseData::createByType(FrpNodeType nodeType)
	{
		switch (nodeType)
		{
		case FrpNode_Analysis1: return new mFrpAnalysisData;
		case FrpNode_Material:  return new mFrpMaterialData;
		case FrpNode_Matrix:    return new mFrpPhaseData(FrpNode_Matrix);
		case FrpNode_Inclusion:	return new mFrpPhaseData(FrpNode_Inclusion);
		case FrpNode_Cavity:	return new mFrpPhaseData(FrpNode_Cavity);
		case FrpNode_Fiber:		return new mFrpPhaseData(FrpNode_Fiber);
		case FrpNode_Rve:         return new mFrpRveData;
		case FrpNode_Failure:     return new mFrpFailureData;
		case FrpNode_AspectRatio: return new mFrpAspectRatioData;
		default:                  return new mFrpBaseData(nodeType);   // Micro 分组、Micro1 微观结构N 等无字段节点
		}
	}

	void mFrpBaseData::addChild(mFrpBaseData* child)
	{
		if (!child) return;
		child->m_pParent = this;
		children.append(child);
	}

	int mFrpBaseData::childCountOfType(FrpNodeType nodeType) const
	{
		int n = 0;
		for (const mFrpBaseData* c : children)
			if (c->type == nodeType) ++n;
		return n;
	}

	mFrpBaseData* mFrpBaseData::findById(const QString& nodeId)
	{
		if (this->id == nodeId) return this;
		for (mFrpBaseData* c : children)
			if (mFrpBaseData* f = c->findById(nodeId)) return f;
		return nullptr;
	}

	mFrpBaseData* mFrpBaseData::ancestorOfType(FrpNodeType nodeType)
	{
		mFrpBaseData* p = m_pParent;
		while (p) { if (p->type == nodeType) return p; p = p->m_pParent; }
		return nullptr;
	}

	// “无界面无数据”的分组节点：材料/失效/长细比 的分组（父=Analysis1），以及微观结构分组
	bool mFrpBaseData::isGroupData() const
	{
		if (type == FrpNode_Micro) return true;
		if (type == FrpNode_Material || type == FrpNode_Failure || type == FrpNode_AspectRatio)
			return m_pParent && m_pParent->type == FrpNode_Analysis1;
		return false;
	}

	void mFrpBaseData::cloneChildrenTo(mFrpBaseData* copy) const
	{
		for (const mFrpBaseData* child : children)
			copy->children.append(child->clone(copy));
	}

	mFrpBaseData* mFrpBaseData::clone(mFrpBaseData* parentNode) const
	{
		mFrpBaseData* copy = new mFrpBaseData(*this);
		copy->m_pParent = parentNode;
		cloneChildrenTo(copy);
		return copy;
	}
}
