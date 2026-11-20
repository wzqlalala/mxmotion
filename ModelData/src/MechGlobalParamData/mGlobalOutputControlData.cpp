#include "mGlobalOutputControlData.h"

namespace MechGlobalParamData
{
	mGlobalOutputControlData::mGlobalOutputControlData()
	{
	}

	mGlobalOutputControlData::~mGlobalOutputControlData()
	{
	}

	bool mGlobalOutputControlData::isCreated() const
	{
		return _created;
	}

	void mGlobalOutputControlData::setCreated(bool created)
	{
		_created = created;
	}

	QHash<QString, QStringList> mGlobalOutputControlData::getOutputSelections() const
	{
		return _outputSelections;
	}

	void mGlobalOutputControlData::setOutputSelections(const QHash<QString, QStringList>& selections)
	{
		_outputSelections = selections;
	}

	QDataStream& operator<<(QDataStream& out, mGlobalOutputControlData& obj)
	{
		out << obj._created;
		return out;
	}

	QDataStream& operator>>(QDataStream& in, mGlobalOutputControlData& obj)
	{
		in >> obj._created;
		return in;
	}
}
