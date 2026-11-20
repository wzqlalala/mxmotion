#include "mFunctionBase.h"

using namespace MxFunctions;
namespace MechFunctionData
{
	mFunctionBase::mFunctionBase(int id, const QString& name): _id(id), _functionName(name)
	{
		if (name.isEmpty())
		{
			_functionName = QString("函数_%1").arg(_id);
		}
	}
	mFunctionBase::~mFunctionBase()
	{

	}

	int mFunctionBase::getFunctionId()
	{
		return _id;
	}

	void mFunctionBase::setFunctionName(const QString& name)
	{
		_functionName = name;
	}

	QString mFunctionBase::getFunctionName()
	{
		return _functionName;
	}

	mFunctionBase* mFunctionBase::copyFunction(const int& id, const QString& name)
	{
		mFunctionBase* func = new mFunctionBase(id, name);
		func->FunctionType = FunctionType;
		func->_xRangeMin = _xRangeMin;
		func->_xRangeMax = _xRangeMax;
		func->_varName = _varName;
		func->varRangeMin = varRangeMin;
		func->varRangeMax = varRangeMax;
		func->expression = expression;
		func->_analyticCol = _analyticCol;
		func->_discreteCol = _discreteCol;
		func->_interpCol = _interpCol;
		func->_piecewiseCol = _piecewiseCol;
		func->_stepCol = _stepCol;
		func->_tableCol = _tableCol;
		func->_periodicCol = _periodicCol;
		func->_modulatedCol = _modulatedCol;
		func->_expDecayCol = _expDecayCol;

		return func;
	}

	// FunctionType 的 getter 和 setter
	int mFunctionBase::getFunctionType() const
	{
		return FunctionType;
	}

	void mFunctionBase::setFunctionType(const int index)
	{
		FunctionType = index;

	}
	
	void mFunctionBase::setAnalytic(const AnalyticCol& ana)
	{
		_analyticCol = ana;
	}

	void mFunctionBase::setDiscrete(const DiscreteCol& dis)
	{
		_discreteCol = dis;
	}
	void mFunctionBase::setInterp(const InterpCol& inte)
	{
		_interpCol = inte;
	}
	void mFunctionBase::setPiecewise(const PiecewiseCol& pie)
	{
		_piecewiseCol = pie;
	}
	void mFunctionBase::setStep(const StepCol& step)
	{
		_stepCol = step;
	}
	void mFunctionBase::setTable(const TableCol& tbl)
	{
		_tableCol = tbl;
	}
	void mFunctionBase::setPeriodic(const PeriodicCol& per)
	{
		_periodicCol = per;
	}
	void mFunctionBase::setModulated(const ModulatedCol& mod)
	{
		_modulatedCol = mod;
	}
	void mFunctionBase::setExpDecay(const ExpDecayCol& ed)
	{
		_expDecayCol = ed;
	}
}
