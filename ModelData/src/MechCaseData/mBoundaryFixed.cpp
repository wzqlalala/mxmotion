#include "mBoundaryFixed.h"

namespace MechCaseData
{
	mBoundaryFixed::mBoundaryFixed(int id,MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase( id,type, name)
	{
		_fixedType = 0;
	}

	mBoundaryFixed::~mBoundaryFixed()
	{

	}
	
	std::shared_ptr<mLoadBoundaryBase> mBoundaryFixed::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundaryFixed> boundary = std::make_shared<mBoundaryFixed>(id, getLoadBoundaryType(), name);
		boundary->_fixedType = _fixedType;
		return boundary;
	}
}