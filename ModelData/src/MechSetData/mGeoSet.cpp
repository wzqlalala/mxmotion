#include "mGeoSet.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mGeoSet::mGeoSet(int id,MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mGeoSet::~mGeoSet()
	{
	}

	mGeoSet* mGeoSet::copySet(const int& id, const QString& name)
	{
		mGeoSet* set = new mGeoSet(id,getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<int> mGeoSet::getGeoSetData()
	{
		return _ids;
	}

	void mGeoSet::setGeoSetData(std::vector<int> data)
	{
		_ids = data;
	}

	MxFunctions::BrowserTreeStatus mGeoSet::getNodeStatus(QVector<QPair<QString, QString>>& info)
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
