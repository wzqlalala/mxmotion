#include "mSolveTaskData.h"

namespace MechSolveTaskData
{
	mSolveTaskData::mSolveTaskData()
	{
	}

	mSolveTaskData::~mSolveTaskData()
	{
	}

	CaseRelation mSolveTaskData::getCaseRelation(int caseID) const
	{
		//未设置接续关系的工况默认独立计算
		return _caseRelations.contains(caseID) ? (CaseRelation)_caseRelations.value(caseID) : Relation_Independent;
	}

	void mSolveTaskData::setCaseRelation(int caseID, CaseRelation relation)
	{
		_caseRelations[caseID] = (int)relation;
	}
}
