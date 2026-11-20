#include "mSolveRunRecord.h"

namespace MechSolveTaskData
{
	mSolveRunRecord::mSolveRunRecord(mSolveTaskData* task)
		: _task(task)
	{
	}

	mSolveRunRecord::~mSolveRunRecord()
	{
	}

	bool mSolveRunRecord::isFinished() const
	{
		return _status == Status_Done || _status == Status_Failed
			|| _status == Status_Interrupted || _status == Status_Cancelled;
	}
}
