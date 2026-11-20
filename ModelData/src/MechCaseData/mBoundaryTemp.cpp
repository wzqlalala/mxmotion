#include "mBoundaryTemp.h"

namespace MechCaseData
{
	mBoundaryTemp::mBoundaryTemp(int id,MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id,type, name)
	{
		_temp = 0;
	}

	mBoundaryTemp::~mBoundaryTemp()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mBoundaryTemp::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundaryTemp> boundary = std::make_shared<mBoundaryTemp>(id, getLoadBoundaryType(), name);
		boundary->_temp = _temp;
		boundary->isAnalysisFieldMode_ = isAnalysisFieldMode_;
		return boundary;
	}
}