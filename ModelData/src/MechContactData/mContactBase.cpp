#include "mContactBase.h"

using namespace MxFunctions;
namespace MechContactData
{
	

	mContactBase::mContactBase(int id,ContactType type, const QString& name, bool isUserRename)
		:  _contactType(type), _contactName(name), _id(id), _isUserRename(isUserRename), _isSuppressed(false)
	{
		
		if (name.isEmpty())
		{
			_contactName = QString("%1_%2").arg(getContactTypeStr()).arg(_id);
		}

		heat_value = { {0,0}, {0,0} };//初始值为两行（只能为两列）
		_pickStateMaster = 0;
		_pickStateSlave = 0;
	}

	mContactBase::~mContactBase()
	{
	}

	void mContactBase::setContactId(int id)
	{
		_id = id;
	}

	int mContactBase::getContactId()
	{
		return _id;
	}

	bool mContactBase::getIsUserRename()
	{
		return _isUserRename;
	}

	void mContactBase::setContactName(const QString& name, bool isUserRename)
	{
		_contactName = name;
		_isUserRename = isUserRename;
	}

	QString mContactBase::getContactName()
	{
		return _contactName;
	}

	void mContactBase::setContactType(ContactType type)
	{
		_contactType = type;
	}

	ContactType mContactBase::getContactType()
	{
		return _contactType;
	}

	QString mContactBase::contactTypeToStringEn(ContactType type)
	{
		switch (type)
		{
		case MxFunctions::SurfaceToSurfaceContact:   return "SurfaceToSurfaceContact";
		case MxFunctions::NodeToSurfaceContact:   return "NodeToSurfaceContact";
		case MxFunctions::CommonContact:   return "CommonContact";
		case MxFunctions::InterferenceContact:   return "InterferenceContact";
		case MxFunctions::TwoDimensionalContact: return "TwoDimensionalContact";
		case MxFunctions::LineToSurfaceContact: return "LineToSurfaceContact";
		default:                                  return "UnknownContact";
		}
	}

	QString mContactBase::getContactTypeStr()
	{
		switch (_contactType)
		{
		case MxFunctions::SurfaceToSurfaceContact:
			return QString("面面接触");
		case MxFunctions::NodeToSurfaceContact:
			return QString("点面接触");
		case MxFunctions::CommonContact:   
			return QString("通用接触");
		case MxFunctions::SelfContact:
			return QString("自接触");
		case MxFunctions::InterferenceContact:
			return QString("过盈接触");
		case MxFunctions::TwoDimensionalContact:
			return QString("二维接触");
		case MxFunctions::LineToSurfaceContact:
			return QString("线面接触");
		default:
			return QString("");
		}
	}
	void mContactBase::setContactDescribe(const QString & describe)
	{
		_contactDescribe = describe;
	}

	QString mContactBase::getContactDescribe()
	{
		return _contactDescribe;
	}
	mContactBase* mContactBase::copyContact(const int& id, const QString& name) {
		mContactBase* nb = new mContactBase(id, _contactType, name);
		nb->_isSuppressed = _isSuppressed;
		nb->slave_region = slave_region;
		nb->smallSlip = smallSlip;
		nb->selfContactSearch = selfContactSearch;
		nb->overclosure = overclosure;
		nb->constraint = constraint;
		nb->scale_factor = scale_factor;
		nb->penalty_factor = penalty_factor;
		nb->tangential_define = tangential_define;
		nb->tangential_friction = tangential_friction;
		nb->tangential_static_coef = tangential_static_coef;
		nb->heat_define = heat_define;
		nb->heat_value = heat_value;
		nb->gap_define = gap_define;
		nb->gap_overclosure = gap_overclosure;
		nb->geom_define = geom_define;
		nb->geom_value = geom_value;
		nb->_contactDescribe = _contactDescribe;
		nb->clr_fit_typ = clr_fit_typ;
		nb->size = size;
		nb->evaluation = evaluation;
		nb->eva_MP = eva_MP;
		nb->eva_SD = eva_SD;
		nb->eva_CV = eva_CV;
		nb->eva_PC = eva_PC;
		nb->eva_R  = eva_R ;
		return nb;
	}

	void mContactBase::setSuppressed(bool suppressed)
	{
		_isSuppressed = suppressed;
	}

	bool mContactBase::isSuppressed()
	{
		return _isSuppressed;
	}

	void mContactBase::setPickStateMaster(int pickstate)
	{
		_pickStateMaster = pickstate;
	}

	int mContactBase::getPickStateMaster()
	{
		return _pickStateMaster;
	}	
	
	void mContactBase::setPickStateSlave(int pickstate)
	{
		_pickStateSlave = pickstate;
	}

	int mContactBase::getPickStateSlave()
	{
		return _pickStateSlave;
	}
}
