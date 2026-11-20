#include "mLoadBoundaryBase.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mLoadBoundaryBase::mLoadBoundaryBase(int id, LoadBoundaryType type, const QString& name)
		: _loadBoundaryType(type), _loadBoundaryName(name), _id(id), _isSuppressed(false)
	{
		//新添加类型后，务必做筛选，否则会默认以载荷命名
		if (name.isEmpty())
		{
			if (type == BOUNDARY_IM_FIXED || type == BOUNDARY_IM_TEMP ||
				type == BOUNDARY_IM_DISPLACEMENT || type == BOUNDARY_IM_SPEED ||
				type == BOUNDARY_IM_FORWARD || type == BOUNDARY_IM_VIRTUAL ||
				type == BOUNDARY_IM_SUPERELEMENT || type == BOUNDARY_IM_ACC)
			{
				_loadBoundaryName = QString("Boundary_%1").arg(_id);
			}
			else
			{
				_loadBoundaryName = QString("Load_%1").arg(_id);
			}
		}
		_pickState = 0;
	}

	mLoadBoundaryBase::~mLoadBoundaryBase()
	{
	}

	void mLoadBoundaryBase::setLoadBoundaryId(int id)
	{
		_id = id;
	}

	int mLoadBoundaryBase::getLoadBoundaryId()
	{
		return _id;
	}

	void mLoadBoundaryBase::setLoadBoundaryName(const QString& name)
	{
		_loadBoundaryName = name;
	}

	QString mLoadBoundaryBase::getLoadBoundaryName()
	{
		return _loadBoundaryName;
	}

	void mLoadBoundaryBase::setLoadBoundaryType(LoadBoundaryType type)
	{
		_loadBoundaryType = type;
	}

	LoadBoundaryType mLoadBoundaryBase::getLoadBoundaryType()
	{
		return _loadBoundaryType;
	}

	int mLoadBoundaryBase::isLoadOrBoundary()
	{
		int type = -1;
		switch (_loadBoundaryType)
		{
		case MxFunctions::LOAD_IM_FORCE:
		case MxFunctions::LOAD_IM_GRAVITY:
		case MxFunctions::LOAD_IM_TORQUE:
		case MxFunctions::LOAD_IM_PRESSURE:
		case MxFunctions::LOAD_IM_HEATFLUX:
		case MxFunctions::LOAD_IM_RADIATION:
		case MxFunctions::LOAD_IM_CONVECTION:
		case MxFunctions::LOAD_IM_TRANSIENT:
		case MxFunctions::LOAD_IM_FREQDOMAIN:
		case MxFunctions::LOAD_IM_COMBINED:
		case MxFunctions::LOAD_IM_BOLT:
		case MxFunctions::LOAD_IM_ACOCONCFORCE:
		case MxFunctions::LOAD_IM_ACOFREQDOMAIN:
		case MxFunctions::LOAD_IM_SPECTRUM:
		case MxFunctions::LOAD_ROTATE_PHYSICAL_POWER:
		case MxFunctions::LOAD_IM_SURFACE:
		case MxFunctions::LOAD_IM_EDGE:
			type = 0;
			break;
		case MxFunctions::BOUNDARY_IM_FIXED:
		case MxFunctions::BOUNDARY_IM_TEMP:
		case MxFunctions::BOUNDARY_IM_DISPLACEMENT:
		case MxFunctions::BOUNDARY_IM_SPEED:
		case MxFunctions::BOUNDARY_IM_ACC:
		case MxFunctions::BOUNDARY_IM_FORWARD:
		case MxFunctions::BOUNDARY_IM_VIRTUAL:
		case MxFunctions::BOUNDARY_IM_SUPERELEMENT:
		case MxFunctions::BOUNDARY_MOVE:
		case MxFunctions::BOUNDARY_TEMP_NEW:
			type = 1;
			break;
		default:
			break;
		}

		return type;
	}

	void mLoadBoundaryBase::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mLoadBoundaryBase::isSuppressed() const
	{
		return _isSuppressed;
	}

	void mLoadBoundaryBase::setPickState(int pickstate)
	{
		_pickState = pickstate;
	}

	int mLoadBoundaryBase::getPickState()
	{
		return _pickState;
	}
}