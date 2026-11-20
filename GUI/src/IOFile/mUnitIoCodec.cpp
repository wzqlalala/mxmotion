#include "mUnitIoCodec.h"
#include "mUnitSystemManager.h"
#include "mModelData.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

namespace MechIOFile
{
	//内部单位制上下文：索引3 = mm-t-N-s，角度=度，转速=rad/s，温度=摄氏
	static const QVector<int> s_internalUnitsIndex = { 3,0,0,0 };

	const QVector<int>& mUnitIoCodec::internalUnitsIndex()
	{
		return s_internalUnitsIndex;
	}

	QVector<int> mUnitIoCodec::currentUnitsIndex()
	{
		CommonData::mUnitSystemManager* mgr = unitManager();
		if (mgr == nullptr)
		{
			return s_internalUnitsIndex;
		}
		return mgr->getCurUnitsIndex();
	}

	CommonData::mUnitSystemManager* mUnitIoCodec::unitManager()
	{
		if (getCVM == nullptr || getCVM->getModelData() == nullptr)
		{
			return nullptr;
		}
		return getCVM->getModelData()->getUnitSystemManager();
	}

	double mUnitIoCodec::ratio(const MxFunctions::Unit& unit, const QVector<int>& fromidx, const QVector<int>& toidx)
	{
		CommonData::mUnitSystemManager* mgr = unitManager();
		if (mgr == nullptr)
		{
			return 1.0;
		}
		//统一委托 mUnitSystemManager::getUnitSystemFromToRatio：
		//与各面板提交/回显路径完全同口径（含该函数未分发量纲返回1的现状行为），
		//保证文件数值与界面显示严格一致
		return mgr->getUnitSystemFromToRatio(unit, fromidx, toidx);
	}

	double mUnitIoCodec::tempAffine(double value, const QString& fromUnit, const QString& toUnit)
	{
		CommonData::mUnitSystemManager* mgr = unitManager();
		if (mgr == nullptr || fromUnit == toUnit)
		{
			return value;
		}
		CommonData::UnitInfo from;
		from.unit = MxFunctions::Unit::Temp;
		from.unitstr = fromUnit;
		from.value = value;
		CommonData::UnitInfo to;
		to.unit = MxFunctions::Unit::Temp;
		to.unitstr = toUnit;
		to.value = value;
		mgr->unitTrans(from, to);
		return to.value;
	}

	double mUnitIoCodec::decode(const MxFunctions::Unit& unit, double fileValue)
	{
		if (unit == MxFunctions::Unit::Temp)
		{
			return decodeTemp(fileValue);
		}
		return fileValue * ratio(unit, currentUnitsIndex(), s_internalUnitsIndex);
	}

	double mUnitIoCodec::encode(const MxFunctions::Unit& unit, double internalValue)
	{
		if (unit == MxFunctions::Unit::Temp)
		{
			return encodeTemp(internalValue);
		}
		return internalValue * ratio(unit, s_internalUnitsIndex, currentUnitsIndex());
	}

	double mUnitIoCodec::decodeTemp(double fileValue)
	{
		QVector<int> cur = currentUnitsIndex();
		return tempAffine(fileValue, CommonData::TempSystem[cur[3]], CommonData::TempSystem[s_internalUnitsIndex[3]]);
	}

	double mUnitIoCodec::encodeTemp(double internalValue)
	{
		QVector<int> cur = currentUnitsIndex();
		return tempAffine(internalValue, CommonData::TempSystem[s_internalUnitsIndex[3]], CommonData::TempSystem[cur[3]]);
	}

	void mUnitIoCodec::decodeCurve(const MxFunctions::Unit& unit, QVector<double>& values)
	{
		for (double& v : values)
		{
			v = decode(unit, v);
		}
	}

	void mUnitIoCodec::encodeCurve(const MxFunctions::Unit& unit, QVector<double>& values)
	{
		for (double& v : values)
		{
			v = encode(unit, v);
		}
	}

	MxFunctions::Unit mUnitIoCodec::unitOfMaterialProp(MxFunctions::MaterialPropType proType)
	{
		using namespace MxFunctions;
		switch (proType)
		{
		//应力类：弹性模量/剪切模量/刚度张量分量/屈服与失效应力/体积模量等
		case MatProp_YoungsModulus:
		case MatProp_Ex: case MatProp_Ey: case MatProp_Ez:
		case MatProp_Gyz: case MatProp_Gxz: case MatProp_Gxy:
		case MatProp_D1111: case MatProp_D1122: case MatProp_D2222: case MatProp_D1133:
		case MatProp_D2233: case MatProp_D3333: case MatProp_D1212: case MatProp_D1313: case MatProp_D2323:
		case MatProp_InitialYieldStressAtZeroPlasticity:
		case MatProp_YieldStress:
		case MatProp_NominalStress:
		case MatProp_Pressure:
		case MatProp_Xt: case MatProp_Xc: case MatProp_Yt: case MatProp_Yc:
		case MatProp_S: case MatProp_St: case MatProp_Sc: case MatProp_Ss:
		case MatProp_F12: case MatProp_Sl:
		case MatProp_StrengthOfExtension:
		case MatProp_BulkModulus:
		case MatProp_ShearStiffness:
		case MatProp_KinematicHardeningR1: case MatProp_KinematicHardeningR2:
		case MatProp_KinematicHardeningR3: case MatProp_KinematicHardeningR4:
		case MatProp_KinematicHardeningR5: case MatProp_KinematicHardeningR6:
		case MatProp_KinematicHardeningR7: case MatProp_KinematicHardeningR8:
		case MatProp_KinematicHardeningR9: case MatProp_KinematicHardeningR10:
			return Unit::Stress;
		//G11..G66 各向异性分量：应力
		case MatProp_G11: case MatProp_G12: case MatProp_G13: case MatProp_G14:
		case MatProp_G15: case MatProp_G16: case MatProp_G22: case MatProp_G23:
		case MatProp_G24: case MatProp_G25: case MatProp_G26: case MatProp_G33:
		case MatProp_G34: case MatProp_G35: case MatProp_G36: case MatProp_G44:
		case MatProp_G45: case MatProp_G46: case MatProp_G55: case MatProp_G56:
		case MatProp_G66:
			return Unit::Stress;
		//密度
		case MatProp_Density:
			return Unit::Density;
		//热传导系数（当前 FromTo 未分发该量纲，倍数为1，与材料面板提交口径一致）
		case MatProp_HeatConductivityCoefficient:
		case MatProp_HeatConductivityCoefficient_A1: case MatProp_HeatConductivityCoefficient_A2:
		case MatProp_HeatConductivityCoefficient_A3: case MatProp_HeatConductivityCoefficient_A4:
		case MatProp_HeatConductivityCoefficient_A5: case MatProp_HeatConductivityCoefficient_A6:
			return Unit::ThermalConductivity;
		//热容密度
		case MatProp_HeatCapacityDensity:
			return Unit::HeatCapacityDensity;
		//热膨胀系数(1/温度)
		case MatProp_CoefficientOfThermalExpansion:
		case MatProp_CoefficientOfThermalExpansion_A1: case MatProp_CoefficientOfThermalExpansion_A2:
		case MatProp_CoefficientOfThermalExpansion_A3: case MatProp_CoefficientOfThermalExpansion_A12:
		case MatProp_CoefficientOfThermalExpansion_A13: case MatProp_CoefficientOfThermalExpansion_A23:
			return Unit::TempReciprocal;
		//参考温度（绝对温度，仿射换算）
		case MatProp_ReferenceTemperature:
			return Unit::Temp;
		//热生成率/热产率（功率密度）
		case MatProp_HeatGenerationRate:
		case MatProp_HeatProductionRate:
			return Unit::PowerDensity;
		//声速
		case MatProp_SoundVelocity:
			return Unit::VelocityOfSound;
		//特征长度
		case MatProp_RegularizationParameterLength:
		case MatProp_Specifile_W:
			return Unit::Length;
		//断裂能/能量释放率（表面能密度）
		case MatProp_Gft: case MatProp_Gfc: case MatProp_Gmt: case MatProp_Gmc:
		case MatProp_GriffithsAbilityActivationRate:
		case MatProp_Break_Power:
			return Unit::SurfaceEnergyDensity;
		//曲线y列量纲：随所属属性
		case MatProp_Curve_Density:
			return Unit::Density;
		case MatProp_Curve_Y_B:
		case MatProp_Curve_Orthotropy:
		case MatProp_Curve_Anisotropy_Gn:
		case MatProp_Curve_PlasticityProperties:
		case MatProp_Curve_IsotropicHardeningCurve:
		case MatProp_Curve_Cyclichardening:
		case MatProp_Curve_CyclicHardeningWithTable:
		case MatProp_Curve_FailStress:
		case MatProp_Curve_Testdatainput_Press:
			return Unit::Stress;
		case MatProp_Curve_HeatConductivityCoefficient:
			return Unit::ThermalConductivity;
		case MatProp_Curve_HeatCapacityDensity:
			return Unit::HeatCapacityDensity;
		case MatProp_Curve_CoefficientOfThermalExpansion:
			return Unit::TempReciprocal;
		default:
			//无量纲/类型标识/未知：不换算
			return Unit::NoneUnit;
		}
	}
}
