#include "mDOFNode.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mDOFNode::mDOFNode(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mDOFNode::~mDOFNode()
	{
	}

	mDOFNode* mDOFNode::copySet(const int& id, const QString& name)
	{
		mDOFNode* set = new mDOFNode(id, getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<RenderPickResultData> mDOFNode::getNodeData()
	{
		return _ids;
	}

	void mDOFNode::setNodeData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	bool mDOFNode::getOrder()
	{
		return _isOrder;
	}

	void mDOFNode::setOrder(bool order)
	{
		_isOrder = order;
	}

	int mDOFNode::dof() const
	{
		return _idof;
	}

	void mDOFNode::setDof(int idof)
	{
		_idof = idof;
	}

	MxFunctions::BrowserTreeStatus mDOFNode::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_ids.empty())
		{
			info.append({ tip,"集合数据为空" });
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}
}
