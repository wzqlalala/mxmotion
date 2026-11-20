#include "mSetBase.h"

using namespace MxFunctions;
namespace MechSetData
{

	mSetBase::mSetBase(int id, SetType type, const QString& name,bool internal, bool isUserRename)
		: _setType(type), _setName(name), _id(id), _internal(internal), _isUserRename(isUserRename)
	{
		if (name.isEmpty())
		{
			_setName = QString("%1_%2").arg(getSetTypeStr()).arg(_id);
		}
	}

	mSetBase::~mSetBase()
	{
	}

	bool mSetBase::getIsUserRename()
	{
		return _isUserRename;
	}

	bool mSetBase::getInternal()
	{
		return _internal;
	}

	void mSetBase::setInternal(bool isinter)
	{
		_internal = isinter;
	}

	void mSetBase::setSetId(int id)
	{
		_id = id;
	}

	int mSetBase::getSetId()
	{
		return _id;
	}

	void mSetBase::setSetName(const QString& name)
	{
		_setName = name;
		_isUserRename = true;
	}

	QString mSetBase::getSetName()
	{
		return _setName;
	}

	void mSetBase::setSetType(SetType type)
	{
		_setType = type;
	}

	SetType mSetBase::getSetType()
	{
		return _setType;
	}

	QString mSetBase::getSetTypeStr()
	{
		switch (_setType)
		{
		case GeoPoint:
			return QString("几何点集");
		case GeoEdge:
			return QString("几何线集");
		case GeoFace:
			return QString("几何面集");
		case GeoSolid:
			return QString("几何实体集");
		case Node:
			return QString("节点集");
		case FrequencyPoint:
			return QString("频率点集");
		case DOFNode:
			return QString("自由度节点集");
		case Panel:
			return QString("面板集");
		case Element:
			return QString("单元集");
		case Part:
			return QString("部件集");
		case ElementSurface:
			return QString("单元表面集");
		case ElementEdge:
			return QString("单元边集");
		case NodeC:
			return QString("节点自由度");
		case Frequency:
			return QString("频率");
		case FrequencyRange:
			return QString("频率段");
		case ElementSurfacePlane:
			return QString("二维单元表面集");
		case SurfaceAnalysis:
			return QString("解析表面集");
		case NodeSurface:
			return QString("节点面集");
		default:
			return QString();
		}
	}
}
