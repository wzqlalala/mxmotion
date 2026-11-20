#include "mCbush.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mCbush::mCbush(int id, PropertyType type, const QString& name) : mPropertyBase(id, type, name)
	{
		_isCheck.resize(6);
		for (int i = 0; i < 6; ++i)
		{
			_isCheck[i] = false;
		}

		_value.resize(6);
		for (int i = 0; i < 6; ++i)
		{
			_value[i].resize(6);
			for (int j = 0; j < 6; ++j)
			{
				_value[i][j] = 0.0;
			}
		}
	}

	mCbush::~mCbush()
	{

	}
	mCbush* MechPropertyData::mCbush::copyProperty(const int& id, const QString& name)
	{
		mCbush* caseData = new mCbush(id, getPropertyType(), name);
		caseData->_isCheck = _isCheck;
		caseData->_value = _value;
		return caseData;
	}
}
