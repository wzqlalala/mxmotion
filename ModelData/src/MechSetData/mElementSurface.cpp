#include "mElementSurface.h"
#include <QVector>
#include <QPair>
#include <QString>


using namespace MxFunctions;
namespace MechSetData
{
	mElementSurface::mElementSurface(int id, MxFunctions::SetType type, const QString& name, bool internal, bool isUserRename)
		: mSetBase(id, type, name, internal, isUserRename)
	{
	}

	mElementSurface::~mElementSurface()
	{
	}

	mElementSurface* mElementSurface::copySet(const int& id, const QString& name)
	{
		mElementSurface* set = new mElementSurface(id,getSetType(), name);
		set->_ids = _ids;
		return set;
	}

	std::vector<RenderPickResultData> mElementSurface::getElementSurfaceData()
	{
		return _ids;
	}

	void mElementSurface::setElementSurfaceData(std::vector<RenderPickResultData> data)
	{
		_ids = data;
	}

	MxFunctions::BrowserTreeStatus mElementSurface::getNodeStatus(QVector<QPair<QString, QString>>& info)
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
