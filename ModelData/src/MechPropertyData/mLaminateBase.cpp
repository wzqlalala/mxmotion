#include "mLaminateBase.h"
namespace MechPropertyData
{
	mLaminateBase::mLaminateBase(int id, MxFunctions::LaminateType type, const QString& name)
		: _LaminateType(type), _LaminateName(name), _id(id)
	{
		if (name.isEmpty())
		{
			_LaminateName = QString("%1_%2").arg(getLaminateTypeStr()).arg(_id);
		}
	}

	mLaminateBase::~mLaminateBase()
	{
	}

	void mLaminateBase::setLaminateId(int id)
	{
		_id = id;
	}

	int mLaminateBase::getLaminateId()
	{
		return _id;
	}

	void mLaminateBase::setLaminateName(const QString& name)
	{
		_LaminateName = name;
	}

	QString mLaminateBase::getLaminateName()
	{
		return _LaminateName;
	}

	void mLaminateBase::setLaminateType(MxFunctions::LaminateType type)
	{
		_LaminateType = type;
	}

	MxFunctions::LaminateType mLaminateBase::getLaminateType()
	{
		return _LaminateType;
	}

	QString mLaminateBase::laminateTypeToStringEn()
	{
		switch (_LaminateType)
		{
		case MxFunctions::SglLayerPlate:
			return QString("Ply");
		case MxFunctions::GlobalSglLam:
			return QString("GlobalPly");
		case MxFunctions::LamPlate:
			return QString("Laminate");
		default:
			return QString("");
		}
	}

	QString mLaminateBase::getLaminateTypeStr()
	{
		switch (_LaminateType)
		{
		case MxFunctions::SglLayerPlate:
			return QString("单层板");
		case MxFunctions::GlobalSglLam:
			return QString("全局单层板");
		case MxFunctions::LamPlate:
			return QString("层合板");
		default:
			return QString("");
		}
	}

}