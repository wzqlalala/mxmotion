#include "mMechGlobalSignals.h"

namespace MechCommon
{
	mMechGlobalSignals* mMechGlobalSignals::_instance = nullptr;

	mMechGlobalSignals* mMechGlobalSignals::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mMechGlobalSignals;
		}
		return _instance;
	}
}