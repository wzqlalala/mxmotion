#include "mRoatateAnalyseBase.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mRoatateAnalyseBase::mRoatateAnalyseBase(int id, RotateAnalysyType type, const QString& name)
		: _rotateAnalyseType(type), _rotateAnalyseName(name), _id(id)
	{
		if (name.isEmpty())
		{
			_rotateAnalyseName = QString("RotateAnalyse_%1").arg(_id);
		}
	}

	mRoatateAnalyseBase::~mRoatateAnalyseBase()
	{
	}

	void mRoatateAnalyseBase::setRotateAnalyseId(int id)
	{
		_id = id;
	}

	int mRoatateAnalyseBase::getRotateAnalyseId()
	{
		return _id;
	}

	void mRoatateAnalyseBase::setRotateAnalyseName(const QString& name)
	{
		_rotateAnalyseName = name;
	}

	QString mRoatateAnalyseBase::getRotateAnalyseName()
	{
		return _rotateAnalyseName;
	}

	void mRoatateAnalyseBase::setRotateAnalyseType(RotateAnalysyType type)
	{
		_rotateAnalyseType = type;
	}

	RotateAnalysyType mRoatateAnalyseBase::getRotateAnalyseType()
	{
		return _rotateAnalyseType;
	}

	QDataStream& operator>>(QDataStream& in, mRoatateAnalyseBase& obj)
	{
		QString name;
		qint32 type;
		in >> name;
		in >> type;
		obj.setRotateAnalyseName(name);
		obj.setRotateAnalyseType((RotateAnalysyType)type);
		return in;
	}

	QDataStream& operator<<(QDataStream& out, mRoatateAnalyseBase& obj)
	{
		out << obj.getRotateAnalyseName();
		out << (qint32)obj.getRotateAnalyseType();
		return out;
	}
}
