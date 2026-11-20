#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mMaterialProps.h"

namespace MechMainWindow
{
	/**
	 * @brief 自定义材料库只读属性控件管理类
	 *
	 * 功能与 mMaterialProps 类似，但每个 mCustomMaterialWindow 拥有独立实例，
	 * 避免与编辑面板的单例属性控件共享状态。
	 */
	class MMAINWINDOW_EXPORT mCustomMaterialProps : public QWidget
	{
		Q_OBJECT

	public:
		explicit mCustomMaterialProps(QWidget* parent = nullptr);
		~mCustomMaterialProps();

		/**
		 * @brief 根据材料类型获取对应的属性控件对象（复用内部缓存实例）
		 */
		mMaterialPropBase* getPropByType(const MxFunctions::MaterialType& proptype);

		/**
		 * @brief 新建一个对应类型的属性控件对象（所有权归调用方）
		 *        用于只读展示场景，避免复用实例导致内部控件被级联删除后悬空。
		 */
		mMaterialPropBase* createPropByType(const MxFunctions::MaterialClass& proptype, QWidget*parent);

		/**
		 * @brief 获取默认属性（颜色等）控件对象（复用内部缓存实例）
		 */
		mDefaultProperty* getDefaultProp();

		/**
		 * @brief 新建一个默认属性控件对象（所有权归调用方）
		 */
		mDefaultProperty* createDefaultProp();

	private:
		// 各材料属性页控件实例（与 mMaterialProps 保持一致）
		mDensityProp* m_DensityProp = nullptr;
		mThermalExpansivityProp* m_ThermalExpansivityProp = nullptr;
		mMaterialDampingProp* m_MaterialDampingProp = nullptr;
		mLoseEfficacyProp* m_LoseEfficacyProp = nullptr;
		mFailureEffectProp* m_FailureEffectProp = nullptr;
		mHeatGenerationRateProp* m_HeatGenerationRateProp = nullptr;
		mHeatProductionRateProp* m_HeatProductionRateProp = nullptr;
		mCreepBehaviorProp* m_CreepBehaviorProp = nullptr;
		mCyclicHardeningProp* m_CyclicHardeningProp = nullptr;
		mCyclicHardeningWithTableProp* m_CyclicHardeningWithTableProp = nullptr;
		mBulkModulusProp* m_BulkModulusProp = nullptr;
		mAcousticsDampingProp* m_AcousticsDampingProp = nullptr;
		mTimeDomainPronyProp* m_TimeDomainPronyProp = nullptr;
		mThermalEquivalenceProp* m_ThermalEquivalenceProp = nullptr;
		mShimProp* m_ShimProp = nullptr;
		mFlexProp* m_FlexProp = nullptr;
		mPlasticityProp* m_PlasticityProp = nullptr;
		mPlasticityOrderProp* m_PlasticityOrderProp = nullptr;
		mThicknessBehaviorProp* m_ThicknessBehaviorProp = nullptr;
		mFilmBehaviorProp* m_FilmBehaviorProp = nullptr;
		mCutBehaviorProp* m_CutBehaviorProp = nullptr;
		mHeatProp* m_HeatProp = nullptr;
		mHeatConductivityCoefficientProp* m_HeatConductivityCoefficientProp = nullptr;
		mHeatCapacityDensityProp* m_HeatCapacityDensityProp = nullptr;
		mSoundVelocityProp* m_SoundVelocityProp = nullptr;

		mIsotropyProp* m_IsotropyProp = nullptr;
		mAnisotropyProp* m_AnisotropyProp = nullptr;
		mOrthotropyProp* m_OrthotropyProp = nullptr;
		mIsotropicThermalProp* m_IsotropicThermalProp = nullptr;
		mIsotropicHardeningProp* m_IsotropicHardeningProp = nullptr;
		mKinematicHardeningProp* m_KinematicHardeningProp = nullptr;
		mCompressiveHardeningProp* m_CompressiveHardeningProp = nullptr;
		mChabocheProp* m_ChabocheProp = nullptr;
		mNeoHookeanProp* m_NeoHookeanProp = nullptr;
		mNeoHookean2Prop* m_NeoHookean2Prop = nullptr;
		mMooneyRivlinProp* m_MooneyRivlinProp = nullptr;
		mOgdenProp* m_OgdenProp = nullptr;
		mArrudaBoyceProp* m_ArrudaBoyceProp = nullptr;
		mYeohProp* m_YeohProp = nullptr;
		mPolynomialProp* m_PolynomialProp = nullptr;
		mReducedPolynomialProp* m_ReducedPolynomialProp = nullptr;
		mHyperfoamProp* m_HyperfoamProp = nullptr;
		mRigidProp* m_RigidProp = nullptr;
		mAeroacousticsProp* m_AeroacousticsProp = nullptr;
		mDeformationPlasticityProp* m_DeformationPlasticityProp = nullptr;
		mViscoplasticProp* m_ViscoplasticProp = nullptr;
		mViscousElasticProp* m_ViscousElasticProp = nullptr;
		mBrittleLikeProp* m_BrittleLikeProp = nullptr;
		mPiezoelectricProp* m_PiezoelectricProp = nullptr;
		mFiberReinforcedProp* m_FiberReinforcedProp = nullptr;

		mMatFromLibCommon* m_MatFromLibCommon = nullptr;
		mMatFromLibStructural* m_MatFromLibStructural = nullptr;
		mMatFromLibThermal* m_MatFromLibThermal = nullptr;
		mMatFromLibElectic* m_MatFromLibElectic = nullptr;

		mDefaultProperty* m_DefaultProperty = nullptr;
		mFailureHashinProp* m_FailureHashinProp = nullptr;
		mEnergyDamageProp* m_EnergyDamageProp = nullptr;
		mBreakProp* m_BreakProp = nullptr;
		mBreakParamProp* m_BreakParamProp = nullptr;
		mPhaseFieldProp* m_PhaseFieldProp = nullptr;
	};

} // namespace MechMainWindow
