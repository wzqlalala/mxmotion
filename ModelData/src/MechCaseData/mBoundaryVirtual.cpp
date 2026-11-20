#include "mBoundaryVirtual.h"

namespace MechCaseData
{
	mBoundaryVirtual::mBoundaryVirtual(int id,MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id,type,name)
	{
		_U1 = false;
		_U2 = false;
		_U3 = false;
		_UR1 = false;
		_UR2 = false;
		_UR3 = false;
	}

	mBoundaryVirtual::~mBoundaryVirtual()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mBoundaryVirtual::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundaryVirtual> boundary = std::make_shared<mBoundaryVirtual>(id, getLoadBoundaryType(), name);
		boundary->_U1 = _U1;
		boundary->_U1 = _U2;
		boundary->_U1 = _U3;
		boundary->_UR1 = _UR1;
		boundary->_UR2 = _UR2;
		boundary->_UR3 = _UR3;

		return boundary;
	}
}