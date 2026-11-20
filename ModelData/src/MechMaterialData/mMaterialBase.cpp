#include "mMaterialBase.h"
#include "mOtherFunction.h"
#include "mModelStruct.h"

using namespace MxFunctions;
namespace MechMaterialData
{
	mMaterialBase::mMaterialBase(int id, QColor color, MaterialType type, const QString& name)
		: _materialType(type), _materialName(name), _id(id), _color(color)
	{
		if (name.isEmpty())
		{
			_materialName = QString("Material_%1").arg(_id);
		}
	}

	QColor mMaterialBase::generateColorById(int id)
	{
		int nColor = PartColorLoop.size();
		QVector4D vector4d = PartColorLoop[(id - 1) % nColor];
		return QColor(vector4d.x() * 255, vector4d.y() * 255, vector4d.z() * 255, vector4d.w() * 255);
	}

	mMaterialBase::mMaterialBase(int id, MaterialType type, const QString& name)
		: mMaterialBase(id, generateColorById(id), type, name)
	{
	}

	mMaterialBase::~mMaterialBase()
	{
	}

	void mMaterialBase::registerMeta()
	{
		qRegisterMetaType<MechMaterialData::Curves>("MechMaterialData::Curves");
		qRegisterMetaTypeStreamOperators<Curves>("Curves");
	}

	void mMaterialBase::setMaterialId(int id)
	{
		_id = id;
	}

	int mMaterialBase::getMaterialId()
	{
		return _id;
	}

	void mMaterialBase::setMaterialName(const QString& name)
	{
		_materialName = name;
	}

	QString mMaterialBase::getMaterialName()
	{
		return _materialName;
	}

	void mMaterialBase::setMaterialType(MxFunctions::MaterialType type)
	{
		_materialType = type;
	}

	MaterialType mMaterialBase::getMaterialType()
	{
		return _materialType;
	}

	void mMaterialBase::setMatColor(const QColor& color)
	{
		_color = color;
	}

	QColor mMaterialBase::getMatColor()
	{
		return _color;
	}

	void mMaterialBase::setProperty(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& props)
	{
		_props = props;
	}

	void mMaterialBase::setLibProperty(const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props)
	{
		_libprops = props;
	}

	void mMaterialBase::addProperty(const MxFunctions::MaterialClass& mattype, const MaterialPropType& propname, const QVariant& property)
	{
		_props[mattype][propname] = property;
	}

	void mMaterialBase::addLibProperty(const MxFunctions::MaterialClass& mattype, const QString& propname, const QVector<Unit>& units, const QVariant& property)
	{
		_libprops[mattype][propname] = { units, property };
	}

	QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>> mMaterialBase::getAllProps()
	{
		return _props;
	}

	QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>> mMaterialBase::getAllLibProps()
	{
		return _libprops;
	}

	std::shared_ptr<mMaterialBase> mMaterialBase::copyMaterial(const int& id, const QString& name)
	{
		std::shared_ptr<mMaterialBase> mat = std::make_shared<mMaterialBase>(id, getMaterialType(), name);
		mat->setProperty(_props);
		mat->setLibProperty(_libprops);
		return mat;
	}

	MxFunctions::BrowserTreeStatus mMaterialBase::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		if (_materialType == MAT_LIBRARY)
		{
			return Tree_Yes;
		}
		QString tip = "警告";
		if (_props.isEmpty() || _materialType == NONETYPE)
		{
			info.append({ tip,"属性为空" });
		}
		for (auto propItem = _props.begin(); propItem != _props.end(); propItem++)
		{
			auto prop = propItem.value();
			if (propItem.key() == MAT_Neohookean2)
			{
				bool isExperiment = false;
				for (auto p = prop.begin(); p != prop.end(); p++)
				{
					if (p.key() == MatProp_ParaDataType)
					{
						if (p.value() == 0)
						{
							isExperiment = true;

						}
						else if (p.value() == 1)
						{
							isExperiment = false;
						}
						break;
					}
				}

				for (auto p = prop.begin(); p != prop.end(); p++)
				{
					if (isExperiment)
					{
						if (((p.key() == MatProp_C10) || (p.key() == MatProp_D1))
							&& (p.value().isNull() || !p.value().isValid()))
						{
							QString inf = MxFunctions::mOtherFunction
								::getMaterialPropStr(p.key()) + "属性值未输入";
							info.append({ tip,inf });
						}
					}
					else
					{
						if (((p.key() == MatProp_Curve_Testdatainput_Press) ||
							p.key() == MatProp_Curve_Testdatainput_V
							)
							&& (p.value().isNull() || !p.value().isValid()))
						{
							QString inf = MxFunctions::mOtherFunction
								::getMaterialPropStr(p.key()) + "属性值未输入";
							info.append({ tip,inf });
						}
					}

				}



			}
			else if (propItem.key() == MAT_Polynomial)
			{
				break;
			}
			else if (propItem.key() == MAT_Ogden)
			{
				break;
			}
			else
			{
				for (auto p = prop.begin(); p != prop.end(); p++)
				{
					if (p.value().isNull() || !p.value().isValid())
					{
						QString inf = MxFunctions::mOtherFunction::getMaterialPropStr(p.key()) + "属性值未输入";
						info.append({ tip,inf });
					}
				}
			}

		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}

	void mMaterialBase::responseUnitChanged(const int& unitidx, mUnitSystemManager* unitsystemmanager)
	{
		for (auto& it : _props)
		{
			for (auto prop = it.begin(); prop != it.end(); prop++)
			{
				Unit unit = mOtherFunction::getUnitOfPropType(prop.key());
				QVariant value = prop.value();
				bool ok;
				double data = value.toDouble(&ok);
				if (ok)//单值 
				{
					if ((unitidx == 0 && (unit == Unit::Temp || unit == Unit::Angle || unit == Unit::Speed)) ||
						(unitidx == 1 && (unit != Unit::Speed)) ||
						(unitidx == 2 && (unit != Unit::Angle)) ||
						(unitidx == 3 && (unit != Unit::Temp) && (unit != Unit::TempReciprocal)))
					{
						continue;
					}
					unitsystemmanager->respSwitchUnitSystem(unit, data);
					prop.value() = QVariant::fromValue<double>(data);
				}
				else//曲线
				{
					prop.value() = transCurveData(unitidx, value, prop.key(), unitsystemmanager);
				}
			}
		}

		////材料库属性大类-属性字段-属性单位-属性值
		//QHash<MxFunctions::MaterialClass, QHash<QString, QPair<Unit, QVariant>>> _libprops;
		for (auto& it : _libprops)
		{
			for (auto& prop : it)
			{
				QVariant value = prop.second;
				bool ok;
				double data = value.toDouble(&ok);
				if (ok)//单值 
				{
					Unit unit = Unit::NoneUnit;
					if (prop.first.size() != 0)
						unit = prop.first[0];
					if ((unitidx == 0 && (unit == Unit::Temp || unit == Unit::Angle || unit == Unit::Speed)) ||
						(unitidx == 1 && (unit != Unit::Speed)) ||
						(unitidx == 2 && (unit != Unit::Angle)) ||
						(unitidx == 3 && (unit != Unit::Temp) && (unit != Unit::TempReciprocal)))
					{
						continue;
					}
					unitsystemmanager->respSwitchUnitSystem(unit, data);
					prop.second = QVariant::fromValue<double>(data);
				}
				else//曲线
				{
					QVector<Unit>units = prop.first;
					prop.second = transCurveData(unitidx, value, units, unitsystemmanager);
				}
			}
		}
	}

	Unit mMaterialBase::getTarUnit(const int& unitidx, const Unit& unit)
	{
		if ((unitidx == 0 && (unit == Unit::Temp || unit == Unit::Angle || unit == Unit::Speed)) ||
			(unitidx == 1 && (unit != Unit::Speed)) ||
			(unitidx == 2 && (unit != Unit::Angle)) ||
			(unitidx == 3 && (unit != Unit::Temp) && (unit != Unit::TempReciprocal)))
		{
			return Unit::NoneUnit;
		}
		return unit;
	}

	QVariant mMaterialBase::transCurveData(const int& unitidx, QVariant data, const MaterialPropType& proptype, mUnitSystemManager* unitsystemmanager)
	{
		QVariant res;

		QVector<QVector<double>>curve = data.value<QVector<QVector<double>>>();
		if (curve.isEmpty())
		{
			return res;
		}
		QVector<Unit>units;
		switch (proptype)
		{
		case MxFunctions::MatProp_Curve_Density:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Density) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_CoefficientOfThermalExpansion:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::TempReciprocal) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Y_B:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::NoneUnit) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Anisotropy_Gn:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp) });
			for (int i = 1; i < curve.size(); ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Orthotropy:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp) });
			for (int i = 1; i < curve.size(); ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_HeatConductivityCoefficient:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::ThermalConductivity) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_HeatCapacityDensity:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::HeatCapacityDensity) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_PlasticityProperties:
		{
			units.append({ getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_PlasticityProperties:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_IsotropicHardeningCurve:
		{
			units.append({ getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_IsotropicHardeningCurve:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Cyclichardening:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Yeho:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::Stress),
				getTarUnit(unitidx,Unit::StressReciprocal),getTarUnit(unitidx,Unit::StressReciprocal),getTarUnit(unitidx,Unit::StressReciprocal) });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Polynomial:
		{
			units.append(getTarUnit(unitidx, Unit::Temp));
			int cnt = curve.size();
			int step = 1;
			if (cnt == 4)
			{
				step = 1;
			}
			else if (cnt == 8)
			{
				step = 2;
			}
			else
			{
				step = 3;
			}
			for (int i = 1; i < cnt - step; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
			}
			for (int i = 0; i < step; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::StressReciprocal));
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_ReducePolynomial:
		{
			units.append(getTarUnit(unitidx, Unit::Temp));
			int cnt = (curve.size() - 1) / 2;
			for (int i = 0; i < cnt; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
			}
			for (int i = 0; i < cnt; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::StressReciprocal));
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Hyperfoam:
		{
			units.append(getTarUnit(unitidx, Unit::Temp));
			int cnt = curve.size() - 1;
			int step = cnt / 3;
			for (int i = 0; i < step; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
				units.append(Unit::NoneUnit);
			}
			for (int i = 0; i < step; ++i)
			{
				units.append(getTarUnit(unitidx, Unit::Stress));
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Deformation:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::Stress),Unit::NoneUnit,
				getTarUnit(unitidx,Unit::Stress),Unit::NoneUnit,Unit::NoneUnit });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Elasticfiber:
		{
			units.append({ getTarUnit(unitidx, Unit::Temp),getTarUnit(unitidx,Unit::Stress),getTarUnit(unitidx,Unit::StressReciprocal) });
			for (int i = 3; i < curve.size(); ++i)
			{
				units.append(Unit::NoneUnit);
			}
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_CreepBehavior:
		{
			units.append({ getTarUnit(unitidx,Unit::Temp),getTarUnit(unitidx,Unit::StressReciprocal),Unit::NoneUnit,Unit::NoneUnit });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		case MxFunctions::MatProp_Curve_Fs_Ssr_Sr:
		{
			units.append({ getTarUnit(unitidx,Unit::NoneUnit),getTarUnit(unitidx,Unit::NoneUnit),Unit::NoneUnit,Unit::TimeReciprocal });
			unitsystemmanager->respSwitchUnitSystem(units, curve);
			res = QVariant::fromValue<QVector<QVector<double>>>(curve);
			break;
		}
		default:
			break;
		}
		return res;
	}

	QVariant mMaterialBase::transCurveData(const int& unitidx, QVariant data, const QVector<Unit>& units, mUnitSystemManager* unitsystemmanager)
	{
		QVariant res;
		QVector<QVector<double>>curve = data.value<QVector<QVector<double>>>();
		if (curve.isEmpty())
		{
			return res;
		}
		unitsystemmanager->respSwitchUnitSystem(units, curve);
		res = QVariant::fromValue<QVector<QVector<double>>>(curve);

		return res;
	}

	QVariant mMaterialBase::transCurve2Varint(const QVector<QVector<double>>& curves, const CurveType& correlation)
	{
		if (curves.size() == 0 || curves[0].size() == 0)
		{
			return QVariant();
		}
		Curves data;
		data.correlation = correlation;
		data.curves = curves;
		return QVariant::fromValue<Curves>(data);
	}

	Curves mMaterialBase::transVatint2Curve(const QVariant& curves)
	{
		Curves data = curves.value<Curves>();
		return data;
	}
}

