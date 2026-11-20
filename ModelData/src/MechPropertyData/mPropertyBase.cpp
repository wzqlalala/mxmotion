#include "mPropertyBase.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	

	mPropertyBase::mPropertyBase(int id,PropertyType type, const QString& name)
		:  _propertyType(type), _propertyName(name), _id(id)
	{
		
		if (name.isEmpty())
		{
			_propertyName = QString("%1_%2").arg(getPropertyTypeStr()).arg(_id);
		}
	}

	mPropertyBase::~mPropertyBase()
	{
	}

	void mPropertyBase::setPropertyId(int id)
	{
		_id = id;
	}

	int mPropertyBase::getPropertyId()
	{
		return _id;
	}

	void mPropertyBase::setPropertyName(const QString& name)
	{
		_propertyName = name;
	}

	QString mPropertyBase::getPropertyName()
	{
		return _propertyName;
	}

	void mPropertyBase::setPropertyType(PropertyType type)
	{
		_propertyType = type;
	}

	PropertyType mPropertyBase::getPropertyType()
	{
		return _propertyType;
	}

	QString mPropertyBase::propertyTypeToStringEn()
	{
		switch (_propertyType)
		{
		case MxFunctions::RodProperty:
			return QString("Rod");
		case MxFunctions::ResultantProperty:
			return QString("Beam");
		case MxFunctions::CrossProperty:
			return QString("Beam");
		case MxFunctions::TubeProperty:
			return QString("Pipe");
		case MxFunctions::PlaneStrainProperty:
			return QString("PlaneStrain");
		case MxFunctions::PlaneStressProperty:
			return QString("PlaneStress");
		case MxFunctions::ShellProperty:
			return QString("Shell");
		case MxFunctions::ShearPlateProperty:
			return QString("ShearPanel");
		case MxFunctions::SolidProperty:
			return QString("Solid");
		case MxFunctions::SolidShellProperty:
			return QString("ContShell");
		case MxFunctions::LayeredShellProperty:
			return QString("复合铺层壳");
		case MxFunctions::Elas1Property:
			return QString("Spring");
		case MxFunctions::Spring1Property:
			return QString("Spring");
		case MxFunctions::Spring2Property:
			return QString("Spring");
		case MxFunctions::SpringAProperty:
			return QString("Spring");
		case MxFunctions::ViscousDampProperty:
			return QString("Damp");
		case MxFunctions::CbushProperty:
			return QString("Cbush");
		case MxFunctions::PlyLaminateProperty:
			return QString("CompositeLayup");
		case MxFunctions::MembraneProperty:
			return QString("Membrane");
		case MxFunctions::GasketProperty:
			return QString("Gasket");
		case MxFunctions::GapuniProperty:
			return QString("Gapnui");
		case MxFunctions::MacroEquivalentSection:
			return QString("宏观等效截面");
		default:
			return QString("");
		}
	}
	
		QString mPropertyBase::getPropertyTypeStr()
	{
		switch (_propertyType)
		{
		case MxFunctions::RodProperty:
			return QString("可扭可拉杆");
		case MxFunctions::ResultantProperty:
			return QString("常截面梁");
		case MxFunctions::CrossProperty:
			return QString("变截面梁");
		case MxFunctions::TubeProperty:
			return QString("管");
		case MxFunctions::PlaneStrainProperty:
			return QString("平面应变");
		case MxFunctions::PlaneStressProperty:
			return QString("平面应力");
		case MxFunctions::ShellProperty:
			return QString("均质壳");
		case MxFunctions::ShearPlateProperty:
			return QString("剪切板");
		case MxFunctions::SolidProperty:
			return QString("均质三维实体");
		case MxFunctions::SolidShellProperty:
			return QString("均质实体壳");
		case MxFunctions::LayeredShellProperty:
			return QString("复合铺层壳");
		case MxFunctions::Elas1Property:
			return QString("单自由度弹簧");
		case MxFunctions::Spring1Property:
			return QString("六自由度接地弹簧");
		case MxFunctions::Spring2Property:
			return QString("六自由度弹簧");
		case MxFunctions::SpringAProperty:
			return QString("三自由度弹簧");
		case MxFunctions::ViscousDampProperty:
			return QString("阻尼");
		case MxFunctions::CbushProperty:
			return QString("六向弹簧阻尼");
		case MxFunctions::PlyLaminateProperty:
			return QString("铺层");
		case MxFunctions::MembraneProperty:
			return QString("膜");
		case MxFunctions::GasketProperty:
			return QString("垫片");
		case MxFunctions::GapuniProperty:
			return QString("间隙");
		default:
			return QString("");
		}
	}
	void mPropertyBase::setPropertyDescribe(const QString & describe)
	{
		_propertyDescribe = describe;
	}

	QString mPropertyBase::getPropertyDescribe()
	{
		return _propertyDescribe;
	}
}
