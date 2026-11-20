#include "mNodeFace.h"

using namespace MxFunctions;
namespace MechSetData
{
	mNodeFace::mNodeFace(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mNodeFace::~mNodeFace()
	{
	}

	mNodeFace* mNodeFace::copySet(const int& id, const QString& name)
	{
		mNodeFace* set = new mNodeFace(id, getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<RenderPickResultData> mNodeFace::getNodeData()
	{
		return _ids;
	}

	void mNodeFace::setNodeData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	MxFunctions::BrowserTreeStatus mNodeFace::getNodeStatus(QVector<QPair<QString, QString>>& info)
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