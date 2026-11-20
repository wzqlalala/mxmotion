#include "mModelFixDataBase.h"

namespace MechModelFixData
{
	mModelFixDataBase::mModelFixDataBase(int id, const QString& name)
		: _id(id), _modelFixName(name), _isSuppressed(false)
	{
	}

	mModelFixDataBase::~mModelFixDataBase()
	{
	}

	void mModelFixDataBase::setId(int id)
	{
		_id = id;
	}

	int mModelFixDataBase::getId() const
	{
		return _id;
	}

	void mModelFixDataBase::setModelFixName(const QString& name)
	{
		_modelFixName = name;
	}

	QString mModelFixDataBase::getModelFixName() const
	{
		return _modelFixName;
	}

	void mModelFixDataBase::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mModelFixDataBase::isSuppressed() const
	{
		return _isSuppressed;
	}
}
