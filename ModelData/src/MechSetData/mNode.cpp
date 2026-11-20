#include "mNode.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mNode::mNode(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mNode::~mNode()
	{
	}

	mNode* mNode::copySet(const int& id, const QString& name)
	{
		mNode* set = new mNode(id, getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<RenderPickResultData> mNode::getNodeData()
	{
		return _ids;
	}

	void mNode::setNodeData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	bool mNode::getOrder()
	{
		return _isOrder;
	}

	void mNode::setOrder(bool order)
	{
		_isOrder = order;
	}

	MxFunctions::BrowserTreeStatus mNode::getNodeStatus(QVector<QPair<QString, QString>>& info)
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
