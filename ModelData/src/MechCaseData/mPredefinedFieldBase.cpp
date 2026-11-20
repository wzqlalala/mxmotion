#include "mPredefinedFieldBase.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mPredefinedFieldBase::mPredefinedFieldBase(int id, PredefinedFieldType type, const QString& name)
		: _predefinedFieldType(type), _predefinedFieldName(name), _id(id)
	{
		if (name.isEmpty())
		{
			_predefinedFieldName = QString("PredefinedField_%1").arg(_id);
		}
	}

	mPredefinedFieldBase::~mPredefinedFieldBase()
	{
	}

	void mPredefinedFieldBase::setPredefinedFieldId(int id)
	{
		_id = id;
	}

	int mPredefinedFieldBase::getPredefinedFieldId()
	{
		return _id;
	}

	void mPredefinedFieldBase::setPredefinedFieldName(const QString& name)
	{
		_predefinedFieldName = name;
	}

	QString mPredefinedFieldBase::getPredefinedFieldName()
	{
		return _predefinedFieldName;
	}

	void mPredefinedFieldBase::setPredefinedFieldType(PredefinedFieldType type)
	{
		_predefinedFieldType = type;
	}

	PredefinedFieldType mPredefinedFieldBase::getPredefinedFieldType()
	{
		return _predefinedFieldType;
	}

	void mPredefinedFieldBase::setPickState(int pickstate)
	{
		_pickState = pickstate;
	}

	int mPredefinedFieldBase::getPickState()
	{
		return _pickState;
	}
}
