#include "mDeputyBase.h"

namespace MechConnectorData
{
	mDeputyBase::mDeputyBase(int id, MxFunctions::DeputyType type, const QString& name)
		:_id(id), _deputyType(type), _deputyName(name), _isSuppressed(false)
	{

	}

	mDeputyBase::~mDeputyBase()
	{

	}

	void mDeputyBase::setDeputyId(int id)
	{
		_id = id;
	}

	int mDeputyBase::getDeputyId()
	{
		return _id;
	}

	void mDeputyBase::setDeputyName(const QString& name)
	{
		_deputyName = name;
	}

	QString mDeputyBase::getDeputyName()
	{
		return _deputyName;
	}

	void mDeputyBase::setDeputyType(MxFunctions::DeputyType type)
	{
		_deputyType = type;
	}

	QString mDeputyBase::getDeputyTypeToStringEn()
	{
		switch (_deputyType)
		{
		case MxFunctions::Deputy_Connector:
			return "Conn3d2";
		case MxFunctions::Deputy_Fastener:
			return "Fastener";
		default:
			break;
		}

		return QString();
	}

	MxFunctions::DeputyType mDeputyBase::getDeputyType()
	{
		return _deputyType;
	}

	void mDeputyBase::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mDeputyBase::isSuppressed()
	{
		return _isSuppressed;
	}
}
