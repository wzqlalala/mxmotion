#include "mPartSet.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mPartSet::mPartSet(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mPartSet::~mPartSet()
	{
	}

	mPartSet* mPartSet::copySet(const int& id, const QString& name)
	{
		mPartSet* set = new mPartSet(id,getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<int> mPartSet::getPartSetData()
	{
		return _ids;
	}

	void mPartSet::setPartSetData(std::vector<int> data)
	{
		_ids = data;
	}

	MxFunctions::BrowserTreeStatus mPartSet::getNodeStatus(QVector<QPair<QString, QString>>& info)
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
