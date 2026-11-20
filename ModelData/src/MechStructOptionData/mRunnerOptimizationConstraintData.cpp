#include "mRunnerOptimizationConstraintData.h"

namespace MechStructOptionData
{
	mRunnerOptimizationConstraintData::mRunnerOptimizationConstraintData(int id, QString name)
	{
		_id = id;
		_name = name;

		_upperBoundStatus = false;
		_lowerBoundStatus = false;
		_constraintType = "质量";
		_constraintMode = "固体";
		_isArea = false;
	}

	mRunnerOptimizationConstraintData::~mRunnerOptimizationConstraintData()
	{


	}
	void mRunnerOptimizationConstraintData::setId(int id)
	{
		_id = id;
	}

	int mRunnerOptimizationConstraintData::getId()
	{
		return _id;
	}

	void mRunnerOptimizationConstraintData::setName(const QString& name)
	{
		_name = name;
	}

	QString mRunnerOptimizationConstraintData::getName()
	{
		return _name;
	}

	void mRunnerOptimizationConstraintData::setDescribe(const QString& describe)
	{
		_describe = describe;
	}

	QString mRunnerOptimizationConstraintData::getDescribe()
	{
		return _describe;
	}

	mRunnerOptimizationConstraintData* mRunnerOptimizationConstraintData::copyData(const int& id, const QString& name)
	{
		mRunnerOptimizationConstraintData* data = new mRunnerOptimizationConstraintData(id, name);
		data->_upperBoundStatus = _upperBoundStatus;
		data->_upperBoundValue = _upperBoundValue;
		data->_lowerBoundStatus = _lowerBoundStatus;
		data->_lowerBoundValue = _lowerBoundValue;
		data->_constraintType = _constraintType;
		data->_constraintMode = _constraintMode;
		data->_isArea = _isArea;
		return data;
	}

}
