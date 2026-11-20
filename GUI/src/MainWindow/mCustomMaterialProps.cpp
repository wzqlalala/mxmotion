#include "mCustomMaterialProps.h"
#include "mOtherFunction.h"

namespace MechMainWindow
{
	mCustomMaterialProps::mCustomMaterialProps(QWidget* parent)
		: QWidget(parent)
	{
		m_DensityProp = new mDensityProp(this);
		m_ThermalExpansivityProp = new mThermalExpansivityProp(this);
		m_MaterialDampingProp = new mMaterialDampingProp(this);
		m_LoseEfficacyProp = new mLoseEfficacyProp(this);
		m_FailureEffectProp = new mFailureEffectProp(this);
		m_HeatGenerationRateProp = new mHeatGenerationRateProp(this);
		//m_HeatProductionRateProp = new mHeatProductionRateProp(this);
		m_CreepBehaviorProp = new mCreepBehaviorProp(this);
		m_CyclicHardeningProp = new mCyclicHardeningProp(this);
		m_CyclicHardeningWithTableProp = new mCyclicHardeningWithTableProp(this);
		m_BulkModulusProp = new mBulkModulusProp(this);
		m_AcousticsDampingProp = new mAcousticsDampingProp(this);
		m_TimeDomainPronyProp = new mTimeDomainPronyProp(this);
		m_ThermalEquivalenceProp = new mThermalEquivalenceProp(this);
		m_ShimProp = new mShimProp(this);
		m_FlexProp = new mFlexProp(this);
		m_PlasticityProp = new mPlasticityProp(this);
		m_PlasticityOrderProp = new mPlasticityOrderProp(this);
		m_ThicknessBehaviorProp = new mThicknessBehaviorProp(this);
		m_FilmBehaviorProp = new mFilmBehaviorProp(this);
		m_CutBehaviorProp = new mCutBehaviorProp(this);
		m_BreakProp = new mBreakProp(this);
		m_BreakParamProp = new mBreakParamProp(this);
		m_PhaseFieldProp = new mPhaseFieldProp(this);

		//m_HeatProp = new mHeatProp(this);
		m_HeatConductivityCoefficientProp = new mHeatConductivityCoefficientProp(this);
		m_HeatCapacityDensityProp = new mHeatCapacityDensityProp(this);
		m_SoundVelocityProp = new mSoundVelocityProp(this);

		m_IsotropyProp = new mIsotropyProp(this);
		m_AnisotropyProp = new mAnisotropyProp(this);
		m_OrthotropyProp = new mOrthotropyProp(this);
		m_IsotropicThermalProp = new mIsotropicThermalProp(this);
		m_IsotropicHardeningProp = new mIsotropicHardeningProp(this);
		m_KinematicHardeningProp = new mKinematicHardeningProp(this);
		m_CompressiveHardeningProp = new mCompressiveHardeningProp(this);
		//m_ChabocheProp = new mChabocheProp(this);
		m_NeoHookeanProp = new mNeoHookeanProp(this);
		m_NeoHookean2Prop = new mNeoHookean2Prop(this);
		m_MooneyRivlinProp = new mMooneyRivlinProp(this);
		m_OgdenProp = new mOgdenProp(this);
		m_ArrudaBoyceProp = new mArrudaBoyceProp(this);
		m_YeohProp = new mYeohProp(this);
		m_PolynomialProp = new mPolynomialProp(this);
		m_ReducedPolynomialProp = new mReducedPolynomialProp(this);
		m_HyperfoamProp = new mHyperfoamProp(this);
		m_RigidProp = new mRigidProp(this);
		m_AeroacousticsProp = new mAeroacousticsProp(this);
		m_DeformationPlasticityProp = new mDeformationPlasticityProp(this);
		//m_ViscoplasticProp = new mViscoplasticProp(this);
		m_ViscousElasticProp = new mViscousElasticProp(this);
		m_BrittleLikeProp = new mBrittleLikeProp(this);
		m_PiezoelectricProp = new mPiezoelectricProp(this);
		m_FiberReinforcedProp = new mFiberReinforcedProp(this);

		m_MatFromLibCommon = new mMatFromLibCommon(this);
		m_MatFromLibStructural = new mMatFromLibStructural(this);
		m_MatFromLibThermal = new mMatFromLibThermal(this);
		//m_MatFromLibElectic = new mMatFromLibElectic(this);

		m_DefaultProperty = new mDefaultProperty(this);

		m_FailureHashinProp = new mFailureHashinProp(this);
		m_EnergyDamageProp = new mEnergyDamageProp(this);
	}

	mCustomMaterialProps::~mCustomMaterialProps()
	{
		delete m_DensityProp; m_DensityProp = nullptr;
		delete m_ThermalExpansivityProp; m_ThermalExpansivityProp = nullptr;
		delete m_MaterialDampingProp; m_MaterialDampingProp = nullptr;
		delete m_LoseEfficacyProp; m_LoseEfficacyProp = nullptr;
		delete m_FailureEffectProp; m_FailureEffectProp = nullptr;
		delete m_HeatGenerationRateProp; m_HeatGenerationRateProp = nullptr;
		delete m_HeatProductionRateProp; m_HeatProductionRateProp = nullptr;
		delete m_CreepBehaviorProp; m_CreepBehaviorProp = nullptr;
		delete m_CyclicHardeningProp; m_CyclicHardeningProp = nullptr;
		delete m_CyclicHardeningWithTableProp; m_CyclicHardeningWithTableProp = nullptr;
		delete m_BulkModulusProp; m_BulkModulusProp = nullptr;
		delete m_AcousticsDampingProp; m_AcousticsDampingProp = nullptr;
		delete m_TimeDomainPronyProp; m_TimeDomainPronyProp = nullptr;
		delete m_ThermalEquivalenceProp; m_ThermalEquivalenceProp = nullptr;
		delete m_ShimProp; m_ShimProp = nullptr;
		delete m_FlexProp; m_FlexProp = nullptr;
		delete m_PlasticityProp; m_PlasticityProp = nullptr;
		delete m_ThicknessBehaviorProp; m_ThicknessBehaviorProp = nullptr;
		delete m_FilmBehaviorProp; m_FilmBehaviorProp = nullptr;
		delete m_CutBehaviorProp; m_CutBehaviorProp = nullptr;
		delete m_BreakProp; m_BreakProp = nullptr;
		delete m_BreakParamProp; m_BreakParamProp = nullptr;
		delete m_PhaseFieldProp; m_PhaseFieldProp = nullptr;
		delete m_HeatProp; m_HeatProp = nullptr;
		delete m_HeatConductivityCoefficientProp; m_HeatConductivityCoefficientProp = nullptr;
		delete m_HeatCapacityDensityProp; m_HeatCapacityDensityProp = nullptr;
		delete m_SoundVelocityProp; m_SoundVelocityProp = nullptr;

		delete m_IsotropyProp; m_IsotropyProp = nullptr;
		delete m_AnisotropyProp; m_AnisotropyProp = nullptr;
		delete m_OrthotropyProp; m_OrthotropyProp = nullptr;
		delete m_IsotropicThermalProp; m_IsotropicThermalProp = nullptr;
		delete m_IsotropicHardeningProp; m_IsotropicHardeningProp = nullptr;
		delete m_KinematicHardeningProp; m_KinematicHardeningProp = nullptr;
		delete m_CompressiveHardeningProp; m_CompressiveHardeningProp = nullptr;
		delete m_ChabocheProp; m_ChabocheProp = nullptr;
		delete m_NeoHookeanProp; m_NeoHookeanProp = nullptr;
		delete m_NeoHookean2Prop; m_NeoHookean2Prop = nullptr;
		delete m_MooneyRivlinProp; m_MooneyRivlinProp = nullptr;
		delete m_OgdenProp; m_OgdenProp = nullptr;
		delete m_ArrudaBoyceProp; m_ArrudaBoyceProp = nullptr;
		delete m_YeohProp; m_YeohProp = nullptr;
		delete m_PolynomialProp; m_PolynomialProp = nullptr;
		delete m_ReducedPolynomialProp; m_ReducedPolynomialProp = nullptr;
		delete m_HyperfoamProp; m_HyperfoamProp = nullptr;
		delete m_RigidProp; m_RigidProp = nullptr;
		delete m_AeroacousticsProp; m_AeroacousticsProp = nullptr;
		delete m_DeformationPlasticityProp; m_DeformationPlasticityProp = nullptr;
		delete m_ViscoplasticProp; m_ViscoplasticProp = nullptr;
		delete m_ViscousElasticProp; m_ViscousElasticProp = nullptr;
		delete m_BrittleLikeProp; m_BrittleLikeProp = nullptr;
		delete m_PiezoelectricProp; m_PiezoelectricProp = nullptr;
		delete m_FiberReinforcedProp; m_FiberReinforcedProp = nullptr;

		delete m_MatFromLibCommon; m_MatFromLibCommon = nullptr;
		delete m_MatFromLibStructural; m_MatFromLibStructural = nullptr;
		delete m_MatFromLibThermal; m_MatFromLibThermal = nullptr;
		delete m_MatFromLibElectic; m_MatFromLibElectic = nullptr;

		delete m_DefaultProperty; m_DefaultProperty = nullptr;
		delete m_FailureHashinProp; m_FailureHashinProp = nullptr;
		delete m_EnergyDamageProp; m_EnergyDamageProp = nullptr;
		delete m_PlasticityOrderProp; m_PlasticityOrderProp = nullptr;
	}

	mMaterialPropBase* mCustomMaterialProps::getPropByType(const MxFunctions::MaterialType& proptype)
	{
		mMaterialPropBase* propbase = nullptr;
		switch (proptype)
		{
		case MxFunctions::NONETYPE:
			propbase = nullptr;
			break;
		case MxFunctions::MAT_LIBRARY:
			propbase = nullptr;
			break;
		case MxFunctions::MAT_Density:
			propbase = m_DensityProp;
			break;
		case MxFunctions::MAT_CoefficientOfThermalExpansion:
			propbase = m_ThermalExpansivityProp;
			break;
		case MxFunctions::MAT_MaterialDamping:
			propbase = m_MaterialDampingProp;
			break;
		case MxFunctions::MAT_YieldStress:
			propbase = m_LoseEfficacyProp;
			break;
		case MxFunctions::MAT_YieldStrain:
			propbase = m_FailureEffectProp;
			break;
		case MxFunctions::MAT_HeatGenerationRate:
			propbase = m_HeatGenerationRateProp;
			break;
		//case MxFunctions::MAT_HeatProductionRate:
		//	propbase = m_HeatProductionRateProp;
		//	break;
		case MxFunctions::MAT_CreepBehavior:
			propbase = m_CreepBehaviorProp;
			break;
		case MxFunctions::MAT_CyclicHardening:
			propbase = m_CyclicHardeningProp;
			break;
		case MxFunctions::MAT_CyclicHardeningWithTable:
			propbase = m_CyclicHardeningWithTableProp;
			break;
		case MxFunctions::MAT_BulkModulus:
			propbase = m_BulkModulusProp;
			break;
		case MxFunctions::MAT_AcousticMaterialDamping:
			propbase = m_AcousticsDampingProp;
			break;
		case MxFunctions::MAT_TimeDomain:
			propbase = m_TimeDomainPronyProp;
			break;
		case MxFunctions::MAT_TimeTemperatureEquivalence:
			propbase = m_ThermalEquivalenceProp;
			break;
		case MxFunctions::MAT_Shim:
			propbase = m_ShimProp;
			break;
		case MxFunctions::MAT_Flex:
			propbase = m_FlexProp;
			break;
		case MxFunctions::MAT_Plasticity:
			propbase = m_PlasticityProp;
			break;
		case MxFunctions::MAT_ThicknessBehavior:
			propbase = m_ThicknessBehaviorProp;
			break;
		case MxFunctions::MAT_FilmBehavior:
			propbase = m_FilmBehaviorProp;
			break;
		case MxFunctions::MAT_CutBehavior:
			propbase = m_CutBehaviorProp;
			break;
		//case MxFunctions::MAT_Heat:
		//	propbase = m_HeatProp;
		//	break;
		case MxFunctions::MAT_HeatConductivityCoefficient:
			propbase = m_HeatConductivityCoefficientProp;
			break;
		//case MxFunctions::MAT_HeatCapacityDensity:
		//	propbase = m_HeatCapacityDensityProp;
		//	break;
		case MxFunctions::MAT_SoundVelocity:
			propbase = m_SoundVelocityProp;
			break;
		case MxFunctions::MAT_Isotropy:
			propbase = m_IsotropyProp;
			break;
		case MxFunctions::MAT_Orthotropy:
			propbase = m_OrthotropyProp;
			break;
		case MxFunctions::MAT_Anisotropy:
			propbase = m_AnisotropyProp;
			break;
		case MxFunctions::MAT_IsotropicThermalMaterial:
			propbase = m_IsotropicThermalProp;
			break;
		case MxFunctions::MAT_IsotropicHardening:
			propbase = m_IsotropicHardeningProp;
			break;
		case MxFunctions::MAT_KinematicHardening:
			propbase = m_KinematicHardeningProp;
			break;
		case MxFunctions::MAT_CompressiveHardening:
			propbase = m_CompressiveHardeningProp;
			break;
		//case MxFunctions::MAT_Chaboche:
		//	propbase = m_ChabocheProp;
		//	break;
		case MxFunctions::MAT_Neohookean:
			propbase = m_NeoHookeanProp;
			break;
		case MxFunctions::MAT_Neohookean2:
			propbase = m_NeoHookean2Prop;
			break;
		case MxFunctions::MAT_MooneyRivlin:
			propbase = m_MooneyRivlinProp;
			break;
		case MxFunctions::MAT_Ogden:
			propbase = m_OgdenProp;
			break;
		case MxFunctions::MAT_Arrudaboyce:
			propbase = m_ArrudaBoyceProp;
			break;
		case MxFunctions::MAT_Yeoh:
			propbase = m_YeohProp;
			break;
		case MxFunctions::MAT_Polynomial:
			propbase = m_PolynomialProp;
			break;
		case MxFunctions::MAT_Rpolynomial:
			propbase = m_ReducedPolynomialProp;
			break;
		case MxFunctions::MAT_Hyperhorm:
			propbase = m_HyperfoamProp;
			break;
		case MxFunctions::MAT_Rigid:
			propbase = m_RigidProp;
			break;
		case MxFunctions::MAT_Piezoelectric:
			propbase = m_PiezoelectricProp;
			break;
		case MxFunctions::MAT_Acoustic:
			propbase = m_AeroacousticsProp;
			break;
		case MxFunctions::MAT_Deformation:
			propbase = m_DeformationPlasticityProp;
			break;
		case MxFunctions::MAT_Elasticfiber:
			propbase = m_FiberReinforcedProp;
			break;
		//case MxFunctions::MAT_Viscoplasticity:
		//	propbase = m_ViscoplasticProp;
		//	break;
		case MxFunctions::MAT_Viscoelasticity:
			propbase = m_ViscousElasticProp;
			break;
		case MxFunctions::MAT_HardBrittle:
			propbase = m_BrittleLikeProp;
			break;
		case MxFunctions::MAT_LibCommon:
			propbase = m_MatFromLibCommon;
			break;
		case MxFunctions::MAT_LibStructural:
			propbase = m_MatFromLibStructural;
			break;
		case MxFunctions::MAT_LibThermal:
			propbase = m_MatFromLibThermal;
			break;
		case MxFunctions::MAT_LibElectic:
			propbase = m_MatFromLibElectic;
			break;
		case MxFunctions::MAT_FailureHashin:
			propbase = m_FailureHashinProp;
			break;
		case MxFunctions::MAT_EnergyDamage:
			propbase = m_EnergyDamageProp;
			break;
		case MxFunctions::MAT_PlasticityWithOrder:
			propbase = m_PlasticityOrderProp;
			break;
		case MxFunctions::MAT_Break:
			propbase = m_BreakProp;
			break;
		case MxFunctions::MAT_BreakParam:
			propbase = m_BreakParamProp;
			break;
		case MxFunctions::MAT_PhaseFieldParam:
			propbase = m_PhaseFieldProp;
			break;
		default:
			propbase = nullptr;
			break;
		}
		return propbase;
	}

	mDefaultProperty* mCustomMaterialProps::getDefaultProp()
	{
		return m_DefaultProperty;
	}

	mMaterialPropBase* mCustomMaterialProps::createPropByType(const MxFunctions::MaterialClass& proptype, QWidget* parent)
	{
		switch (proptype)
		{
		case MxFunctions::MAT_Density:                       return new mDensityProp(parent);
		case MxFunctions::MAT_CoefficientOfThermalExpansion: return new mThermalExpansivityProp(parent);
		case MxFunctions::MAT_MaterialDamping:               return new mMaterialDampingProp(parent);
		case MxFunctions::MAT_YieldStress:                   return new mLoseEfficacyProp(parent);
		case MxFunctions::MAT_YieldStrain:                   return new mFailureEffectProp(parent);
		case MxFunctions::MAT_HeatGenerationRate:            return new mHeatGenerationRateProp(parent);
		//case MxFunctions::MAT_HeatProductionRate:            return new mHeatProductionRateProp(parent);
		case MxFunctions::MAT_CreepBehavior:                 return new mCreepBehaviorProp(parent);
		case MxFunctions::MAT_CreepBehavior_Check:          return new mCreepBehaviorProp(parent, true);
		case MxFunctions::MAT_CyclicHardening:               return new mCyclicHardeningProp(parent);
		case MxFunctions::MAT_CyclicHardeningWithTable:      return new mCyclicHardeningWithTableProp(parent);
		case MxFunctions::MAT_BulkModulus:                   return new mBulkModulusProp(parent);
		case MxFunctions::MAT_AcousticMaterialDamping:       return new mAcousticsDampingProp(parent);
		case MxFunctions::MAT_TimeDomain:                    return new mTimeDomainPronyProp(parent);
		case MxFunctions::MAT_TimeTemperatureEquivalence:    return new mThermalEquivalenceProp(parent);
		case MxFunctions::MAT_Shim:                          return new mShimProp(parent);
		case MxFunctions::MAT_Flex:                          return new mFlexProp(parent);
		case MxFunctions::MAT_Plasticity:                    return new mPlasticityProp(parent);
		case MxFunctions::MAT_PlasticityJohnson:             return new mPlasticityJohnsonProp(parent);
		case MxFunctions::MAT_RateDependent:                 return new mRateDependentProp(parent);
		case MxFunctions::MAT_ShearDamage:                   return new mShearDamageProp(parent);
		case MxFunctions::MAT_ThicknessBehavior:             return new mThicknessBehaviorProp(parent);
		case MxFunctions::MAT_FilmBehavior:                  return new mFilmBehaviorProp(parent);
		case MxFunctions::MAT_CutBehavior:                   return new mCutBehaviorProp(parent);
		case MxFunctions::MAT_Heat:                          return new mHeatCapacityDensityProp(parent);
		case MxFunctions::MAT_HeatConductivityCoefficient:   return new mHeatConductivityCoefficientProp(parent);
		case MxFunctions::MAT_HeatConductivityCoefficient_UnCheck: return new mHeatConductivityCoefficientProp(parent, false);
		//case MxFunctions::MAT_HeatCapacityDensity:           return new mHeatCapacityDensityProp(parent);
		case MxFunctions::MAT_Density_UnCheck:               return new mDensityProp(parent, false);
		case MxFunctions::MAT_SoundVelocity:                 return new mSoundVelocityProp(parent);
		case MxFunctions::MAT_Isotropy:                      return new mIsotropyProp(parent);
		case MxFunctions::MAT_Orthotropy:                    return new mOrthotropyProp(parent);
		case MxFunctions::MAT_Anisotropy:                    return new mAnisotropyProp(parent);
		case MxFunctions::MAT_IsotropicThermalMaterial:      return new mIsotropicThermalProp(parent);
		case MxFunctions::MAT_IsotropicHardening:            return new mIsotropicHardeningProp(parent);
		case MxFunctions::MAT_KinematicHardening:            return new mKinematicHardeningProp(parent);
		case MxFunctions::MAT_CompressiveHardening:          return new mCompressiveHardeningProp(parent);
		//case MxFunctions::MAT_Chaboche:                      return new mChabocheProp(parent);
		case MxFunctions::MAT_Neohookean:                    return new mNeoHookeanProp(parent);
		case MxFunctions::MAT_Neohookean2:                   return new mNeoHookean2Prop(parent);
		case MxFunctions::MAT_MooneyRivlin:                  return new mMooneyRivlinProp(parent);
		case MxFunctions::MAT_Ogden:                         return new mOgdenProp(parent);
		case MxFunctions::MAT_Arrudaboyce:                   return new mArrudaBoyceProp(parent);
		case MxFunctions::MAT_Yeoh:                          return new mYeohProp(parent);
		case MxFunctions::MAT_Polynomial:                    return new mPolynomialProp(parent);
		case MxFunctions::MAT_Rpolynomial:                   return new mReducedPolynomialProp(parent);
		case MxFunctions::MAT_Hyperhorm:                     return new mHyperfoamProp(parent);
		case MxFunctions::MAT_Rigid:                         return new mRigidProp(parent);
		case MxFunctions::MAT_Piezoelectric:                 return new mPiezoelectricProp(parent);
		case MxFunctions::MAT_Acoustic:                      return new mAeroacousticsProp(parent);
		case MxFunctions::MAT_Deformation:                   return new mDeformationPlasticityProp(parent);
		case MxFunctions::MAT_Elasticfiber:                  return new mFiberReinforcedProp(parent);
		//case MxFunctions::MAT_Viscoplasticity:               return new mViscoplasticProp(parent);
		case MxFunctions::MAT_Viscoelasticity:               return new mViscousElasticProp(parent);
		case MxFunctions::MAT_HardBrittle:                   return new mBrittleLikeProp(parent);
		case MxFunctions::MAT_LibCommon:                     return new mMatFromLibCommon(parent);
		case MxFunctions::MAT_LibStructural:                 return new mMatFromLibStructural(parent);
		case MxFunctions::MAT_LibThermal:                    return new mMatFromLibThermal(parent);
		//case MxFunctions::MAT_LibElectic:                    return new mMatFromLibElectic(parent);
		case MxFunctions::MAT_FailureHashin:                 return new mFailureHashinProp(parent);
		case MxFunctions::MAT_EnergyDamage:                  return new mEnergyDamageProp(parent);
		case MxFunctions::MAT_PlasticityWithOrder:           return new mPlasticityOrderProp(parent);
		case MxFunctions::MAT_Break:                         return new mBreakProp(parent);
		case MxFunctions::MAT_BreakParam:                    return new mBreakParamProp(parent);
		case MxFunctions::MAT_PhaseFieldParam:               return new mPhaseFieldProp(parent);
		default:                                             return nullptr;
		}
	}

	mDefaultProperty* mCustomMaterialProps::createDefaultProp()
	{
		return new mDefaultProperty(this);
	}
}
