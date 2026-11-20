#include "mElementSet.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mElementSet::mElementSet(int id,MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mElementSet::~mElementSet()
	{
	}

	mElementSet* mElementSet::copySet(const int& id, const QString& name)
	{
		mElementSet* set = new mElementSet(id,getSetType(),name);
		set->_ids = _ids;
		return set;
	}

	std::vector<RenderPickResultData> mElementSet::getElementSetData()
	{
		return _ids;
	}

	void mElementSet::setElementSetData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	bool mElementSet::getOrder()
	{
		return _isOrder;
	}

	void mElementSet::setOrder(bool order)
	{
		_isOrder = order;
	}

	MxFunctions::BrowserTreeStatus mElementSet::getNodeStatus(QVector<QPair<QString, QString>>& info)
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
