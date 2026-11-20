#include "mMaterialProps.h"

#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "mInfoWindow.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMaterialBase.h"
#include "mMechGlobalSignals.h"
#include "mParamCurveControl.h"

#include <cmath>
#include <limits>

//QT
#include <QGuiApplication>
#include <QColorDialog>
#include <QButtonGroup>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
using namespace MechMaterialData;
namespace MechMainWindow
{
    // 名称到 CalcMode 的静态映射
    static QHash<QString, CalcMode> initNameToCalcMode()
    {
        QHash<QString, CalcMode> map;
        //1、密度mDensityProp
        map.insert(QString("密度"), CalcMode::Positive);

        //2、材料阻尼mMaterialDampingProp
        map.insert(QString("材料阻尼"), CalcMode::Positive);

        //3、热膨胀系数mThermalExpansivityProp
        map.insert(QString("参考温度  "), CalcMode::Standard);
        map.insert(QString("A1"), CalcMode::Standard);
        map.insert(QString("A2"), CalcMode::Standard);
        map.insert(QString("A3"), CalcMode::Standard);
        map.insert(QString("A12"), CalcMode::Standard);
        map.insert(QString("A13"), CalcMode::Standard);
        map.insert(QString("A23"), CalcMode::Standard);

        //3、热传导系数mHeatConductivityCoefficientProp
        map.insert(QString("Val1"), CalcMode::Positive);
        map.insert(QString("Val2"), CalcMode::Positive);
        map.insert(QString("Val3"), CalcMode::Positive);
        map.insert(QString("Val4"), CalcMode::Positive);
        map.insert(QString("Val5"), CalcMode::Positive);
        map.insert(QString("Val6"), CalcMode::Positive);

        //4、比热容系数mHeatCapacityDensityProp
        map.insert(QString("比热容系数"), CalcMode::Positive);

        //5、应力失效mLoseEfficacyProp
        map.insert(QString("Xt"), CalcMode::Standard);
        map.insert(QString("纵向拉伸强度"), CalcMode::Standard);
        map.insert(QString("Xc"), CalcMode::Standard);
        map.insert(QString("纵向压缩强度"), CalcMode::Standard);
        map.insert(QString("Yt"), CalcMode::Standard);
        map.insert(QString("横向拉伸强度"), CalcMode::Standard);
        map.insert(QString("Yc"), CalcMode::Standard);
        map.insert(QString("横向压缩强度"), CalcMode::Standard);
        map.insert(QString("S"), CalcMode::Standard);
        map.insert(QString("面内剪切强度"), CalcMode::Standard);
        map.insert(QString("F12"), CalcMode::Standard);
        map.insert(QString("相互作用项"), CalcMode::Standard);
        map.insert(QString("Biax"), CalcMode::Standard);
        map.insert(QString("双轴应力项"), CalcMode::Standard);
        map.insert(QString("St"), CalcMode::Standard);
        map.insert(QString("拉伸强度"), CalcMode::Standard);
        map.insert(QString("Sc"), CalcMode::Standard);
        map.insert(QString("压缩强度"), CalcMode::Standard);
        map.insert(QString("Ss"), CalcMode::Standard);
        map.insert(QString("剪切强度"), CalcMode::Standard);

        //6、应变失效mFailureEffectProp 与第5个输入校验一致,默认不同材料类型同输入参数同校验
        map.insert(QString("St"), CalcMode::Standard);
        map.insert(QString("拉伸应变极限"), CalcMode::Standard);
        map.insert(QString("Sc"), CalcMode::Standard);
        map.insert(QString("压缩应变极限"), CalcMode::Standard);
        map.insert(QString("Ss"), CalcMode::Standard);
        map.insert(QString("剪切应变极限"), CalcMode::Standard);

        map.insert(QString("Xt"), CalcMode::Standard);
        map.insert(QString("纵向拉伸应变极限"), CalcMode::Standard);
        map.insert(QString("Xc"), CalcMode::Standard);
        map.insert(QString("纵向压缩应变极限"), CalcMode::Standard);
        map.insert(QString("Yt"), CalcMode::Standard);
        map.insert(QString("横向拉伸应变极限"), CalcMode::Standard);
        map.insert(QString("Yc"), CalcMode::Standard);
        map.insert(QString("横向压缩应变极限"), CalcMode::Standard);
        map.insert(QString("S"), CalcMode::Standard);
        map.insert(QString("面内剪切应变极限"), CalcMode::Standard);

        //7、Hashin失效mFailureHashinProp
        map.insert(QString("alpha"), CalcMode::Standard);
        map.insert(QString("Sl  "), CalcMode::Standard);
        map.insert(QString("纵向剪切强度"), CalcMode::Standard);
        map.insert(QString("St"), CalcMode::Standard);
        map.insert(QString("横向剪切强度"), CalcMode::Standard);

        //8、Hashin渐进损伤mEnergyDamageProp
        map.insert(QString("Gft"), CalcMode::Standard);
        map.insert(QString("纤维拉伸断裂能"), CalcMode::Standard);
        map.insert(QString("Gfc"), CalcMode::Standard);
        map.insert(QString("纤维压缩断裂能"), CalcMode::Standard);
        map.insert(QString("Gmt"), CalcMode::Standard);
        map.insert(QString("基体拉伸断裂能"), CalcMode::Standard);
        map.insert(QString("Gmc"), CalcMode::Standard);
        map.insert(QString("基体压缩断裂能"), CalcMode::Standard);

        //9、各向同性线弹性材料mIsotropyProp
        map.insert(QString("杨氏模量"), CalcMode::Positive);
        map.insert(QString("泊松比  "), CalcMode::Positive);

        //10、（二维正交各向、三维工程常数、三维正交各向）mOrthotropyProp
        map.insert(QString("Ex  "), CalcMode::Positive);
        map.insert(QString("Ey  "), CalcMode::Positive);
        map.insert(QString("NUxy"), CalcMode::Positive);
        map.insert(QString("Gxy "), CalcMode::Positive);
        map.insert(QString("Gxz "), CalcMode::Positive);
        map.insert(QString("Gyz "), CalcMode::Positive);

        map.insert(QString("Ez  "), CalcMode::Positive);
        map.insert(QString("NUyz"), CalcMode::Positive);
        map.insert(QString("NUxz"), CalcMode::Positive);
        map.insert(QString("Gxy "), CalcMode::Positive);

        map.insert(QString("D1111"), CalcMode::Positive);
        map.insert(QString("D1122"), CalcMode::Positive);
        map.insert(QString("D2222"), CalcMode::Positive);
        map.insert(QString("D1133"), CalcMode::Positive);
        map.insert(QString("D2233"), CalcMode::Positive);
        map.insert(QString("D3333"), CalcMode::Positive);
        map.insert(QString("D1212"), CalcMode::Positive);
        map.insert(QString("D1313"), CalcMode::Positive);
        map.insert(QString("D2323"), CalcMode::Positive);

        //11、(二维各向异性、三维各向异性)mAnisotropyProp
        map.insert(QString("G11"), CalcMode::Positive);
        map.insert(QString("G12"), CalcMode::Positive);
        map.insert(QString("G13"), CalcMode::Positive);
        map.insert(QString("G22"), CalcMode::Positive);
        map.insert(QString("G23"), CalcMode::Positive);
        map.insert(QString("G33"), CalcMode::Positive);

        map.insert(QString("G14"), CalcMode::Positive);
        map.insert(QString("G15"), CalcMode::Positive);
        map.insert(QString("G16"), CalcMode::Positive);
        map.insert(QString("G24"), CalcMode::Positive);
        map.insert(QString("G25"), CalcMode::Positive);
        map.insert(QString("G26"), CalcMode::Positive);
        map.insert(QString("G34"), CalcMode::Positive);
        map.insert(QString("G35"), CalcMode::Positive);
        map.insert(QString("G36"), CalcMode::Positive);
        map.insert(QString("G44"), CalcMode::Positive);
        map.insert(QString("G45"), CalcMode::Positive);
        map.insert(QString("G46"), CalcMode::Positive);
        map.insert(QString("G55"), CalcMode::Positive);
        map.insert(QString("G56"), CalcMode::Positive);
        map.insert(QString("G66"), CalcMode::Positive);

        //12、弹塑性 - 组合硬化mCompressiveHardeningProp
        map.insert(QString("极限屈服应力"), CalcMode::Positive);
        map.insert(QString("硬化速率控制"), CalcMode::Positive);

        //13、弹塑性 - 随动硬化mKinematicHardeningProp

        //14、弹塑性 - 各向同性硬化mIsotropicHardeningProp

        //15、循环弹塑性材料mChabocheProp
        map.insert(QString("零塑性时初始屈服应力"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C1"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R1"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C2"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R2"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C3"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R3"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C4"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R4"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C5"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R5"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C6"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R6"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C7"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R7"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C8"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R8"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C9"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R9"), CalcMode::Positive);
        map.insert(QString("随动硬化参数C10"), CalcMode::Positive);
        map.insert(QString("随动硬化参数R10"), CalcMode::Positive);

        //16、变形塑性材料mDeformationPlasticityProp
        map.insert(QString("弹性模量"), CalcMode::Positive);
        map.insert(QString("屈服应力"), CalcMode::Positive);
        map.insert(QString("非线性塑性硬化指数"), CalcMode::Positive);
        map.insert(QString("材料参数α"), CalcMode::Positive);

        //17、Neohookean超弹性材料mNeoHookeanProp

        //18、Ogden超弹性材料mOgdenProp
        map.insert(QString("MU1"), CalcMode::Standard);
        map.insert(QString("MU2"), CalcMode::Standard);
        map.insert(QString("MU3"), CalcMode::Standard);
        map.insert(QString("MU4"), CalcMode::Standard);
        map.insert(QString("MU5"), CalcMode::Standard);
        map.insert(QString("MU6"), CalcMode::Standard);

        map.insert(QString("Alpha1"), CalcMode::Standard);
        map.insert(QString("Alpha2"), CalcMode::Standard);
        map.insert(QString("Alpha3"), CalcMode::Standard);
        map.insert(QString("Alpha4"), CalcMode::Standard);
        map.insert(QString("Alpha5"), CalcMode::Standard);
        map.insert(QString("Alpha6"), CalcMode::Standard);

        map.insert(QString("D1"), CalcMode::NonNegative);
        map.insert(QString("D2"), CalcMode::NonNegative);
        map.insert(QString("D3"), CalcMode::NonNegative);
        map.insert(QString("D4"), CalcMode::NonNegative);
        map.insert(QString("D5"), CalcMode::NonNegative);
        map.insert(QString("D6"), CalcMode::NonNegative);

        //19、Arruda-Boyce超弹性材料mArrudaBoyceProp
        map.insert(QString("MU"), CalcMode::Standard);
        map.insert(QString("LambdaM"), CalcMode::Standard);
        map.insert(QString("D"), CalcMode::NonNegative);

        //20、MooneyRivlin超弹性材料mMooneyRivlinProp
        map.insert(QString("C10"), CalcMode::Standard);
        map.insert(QString("C01"), CalcMode::Standard);

        //21、Polynomial超弹性材料mPolynomialProp
        map.insert(QString("C10"), CalcMode::Standard);
        map.insert(QString("C01"), CalcMode::Standard);
        map.insert(QString("C20"), CalcMode::Standard);
        map.insert(QString("C11"), CalcMode::Standard);
        map.insert(QString("C02"), CalcMode::Standard);
        map.insert(QString("C30"), CalcMode::Standard);
        map.insert(QString("C21"), CalcMode::Standard);
        map.insert(QString("C12"), CalcMode::Standard);
        map.insert(QString("C03"), CalcMode::Standard);
        map.insert(QString("C40"), CalcMode::Standard);
        map.insert(QString("C31"), CalcMode::Standard);
        map.insert(QString("C22"), CalcMode::Standard);
        map.insert(QString("C13"), CalcMode::Standard);
        map.insert(QString("C04"), CalcMode::Standard);
        map.insert(QString("C50"), CalcMode::Standard);
        map.insert(QString("C41"), CalcMode::Standard);
        map.insert(QString("C32"), CalcMode::Standard);
        map.insert(QString("C23"), CalcMode::Standard);
        map.insert(QString("C14"), CalcMode::Standard);
        map.insert(QString("C05"), CalcMode::Standard);
        map.insert(QString("C60"), CalcMode::Standard);
        map.insert(QString("C51"), CalcMode::Standard);
        map.insert(QString("C42"), CalcMode::Standard);
        map.insert(QString("C33"), CalcMode::Standard);
        map.insert(QString("C24"), CalcMode::Standard);
        map.insert(QString("C15"), CalcMode::Standard);
        map.insert(QString("C06"), CalcMode::Standard);

        map.insert(QString("D4 "), CalcMode::NonNegative);
        map.insert(QString("D5 "), CalcMode::NonNegative);
        map.insert(QString("D6 "), CalcMode::NonNegative);
        //22、Yeoh超弹性材料mYeohProp
        map.insert(QString("D1 "), CalcMode::NonNegative);
        map.insert(QString("D2 "), CalcMode::NonNegative);
        map.insert(QString("D3 "), CalcMode::NonNegative);
        //23、粘弹性材料mViscousElasticProp
        map.insert(QString("杨氏模量    "), CalcMode::Positive);
        map.insert(QString("泊松比      "), CalcMode::Positive);
        //24、粘塑性材料mViscoplasticProp
        map.insert(QString("幂律乘数"), CalcMode::Standard);
        map.insert(QString("应力幂指数"), CalcMode::Standard);
        map.insert(QString("时间幂指数"), CalcMode::Standard);

        //25、热学材料mHeatProp
        //26、声学材料mAeroacousticsProp
        map.insert(QString("声速"), CalcMode::Positive);


        //27、超弹泡沫材料mHyperfoamProp
        // names1: mu1/alpha1 ... mu6/alpha6 -> 实数 Standard
        map.insert(QString("mu1   "), CalcMode::Standard);
        map.insert(QString("alpha1"), CalcMode::Standard);
        map.insert(QString("mu2   "), CalcMode::Standard);
        map.insert(QString("alpha2"), CalcMode::Standard);
        map.insert(QString("mu3   "), CalcMode::Standard);
        map.insert(QString("alpha3"), CalcMode::Standard);
        map.insert(QString("mu4   "), CalcMode::Standard);
        map.insert(QString("alpha4"), CalcMode::Standard);
        map.insert(QString("mu5   "), CalcMode::Standard);
        map.insert(QString("alpha5"), CalcMode::Standard);
        map.insert(QString("mu6   "), CalcMode::Standard);
        map.insert(QString("alpha6"), CalcMode::Standard);
        // names2: nu1 ... nu6 -> 非负实数 NonNegative
        map.insert(QString("nu1   "), CalcMode::NonNegative);
        map.insert(QString("nu2   "), CalcMode::NonNegative);
        map.insert(QString("nu3   "), CalcMode::NonNegative);
        map.insert(QString("nu4   "), CalcMode::NonNegative);
        map.insert(QString("nu5   "), CalcMode::NonNegative);
        map.insert(QString("nu6   "), CalcMode::NonNegative);

        //28、垫片材料mShimProp
        map.insert(QString("剪切刚度"), CalcMode::Positive);

        //29、体积模量mBulkModulusProp
        map.insert(QString("体积模量"), CalcMode::Positive);

        //30、流体阻尼系数mAcousticsDampingProp
        map.insert(QString("流体阻尼系数"), CalcMode::Positive);

        //31、热生成率mHeatGenerationRateProp
        map.insert(QString("热生成率"), CalcMode::Positive);

        //32、Johnson-Cook塑性mPlasticityJohnsonProp
        map.insert(QString("初始屈服应力"), CalcMode::NonNegative);
        map.insert(QString("应变乘子"), CalcMode::NonNegative);
        map.insert(QString("应变幂指数"), CalcMode::NonNegative);
        map.insert(QString("温度幂指数"), CalcMode::NonNegative);
        map.insert(QString("融化温度"), CalcMode::NonNegative);
        map.insert(QString("转变温度"), CalcMode::NonNegative);

        //33、率相关mRateDependentProp
        map.insert(QString("应变率乘子"), CalcMode::NonNegative);
        map.insert(QString("参考应变率"), CalcMode::NonNegative);

        //34、剪切损伤mShearDamageProp
        map.insert(QString("压力系数"), CalcMode::NonNegative);
        //35、损伤演化MAT_DamageEvolution
        map.insert(QString("断裂位移"), CalcMode::NonNegative);

        return map;
    }
    QHash<QString, CalcMode> s_nameToCalcMode = initNameToCalcMode();

    mMaterialProps* mMaterialProps::m_pInstance = nullptr;

    mMaterialProps* mMaterialProps::getInstance()
    {
        if (m_pInstance == nullptr)
        {
            m_pInstance = new mMaterialProps;
        }
        return m_pInstance;
    }

    mMaterialProps::mMaterialProps(QWidget* parent)
        :QObject(parent)
    {
    }

    mMaterialProps::~mMaterialProps()
    {
    }

    mMaterialPropBase* mMaterialProps::getPropByType(const MxFunctions::MaterialClass& propType, QWidget* parent/* = nullptr*/)
    {
        if (m_FoldCache.contains(propType) && m_FoldCache[propType] != nullptr)
            return m_FoldCache[propType];
        else
        {
            mMaterialPropBase* pPropbase = nullptr;
            switch (propType)
            {
            case MxFunctions::MAT_MaterialDefine:
                pPropbase = new mMaterialDefine(parent);
                break;
            case MxFunctions::MAT_Isotropy:
                pPropbase = new mIsotropyProp(parent);
                break;
            case MxFunctions::MAT_Density:
                pPropbase = new mDensityProp(parent);
                break;
            case MxFunctions::MAT_Density_UnCheck:
                pPropbase = new mDensityProp(parent, false, MxFunctions::MAT_Density_UnCheck);
                break;
            case MxFunctions::MAT_CoefficientOfThermalExpansion:
                pPropbase = new mThermalExpansivityProp(parent);
                break;
            case MxFunctions::MAT_MaterialDamping:
                pPropbase = new mMaterialDampingProp(parent);
                break;
            case MxFunctions::MAT_YieldStress:
                pPropbase = new mLoseEfficacyProp(parent);
                break;
            case MxFunctions::MAT_YieldStrain:
                pPropbase = new mFailureEffectProp(parent);
                break;
                //case MxFunctions::HeatGenerationRate:
                //	pPropbase = m_HeatGenerationRateProp;
                //	break;
            case MxFunctions::MAT_HeatGenerationRate:
                pPropbase = new mHeatGenerationRateProp(parent);
                break;
            case MxFunctions::MAT_CreepBehavior:
                pPropbase = new mCreepBehaviorProp(parent);
                break;
            case MxFunctions::MAT_CreepBehavior_Check:
                pPropbase = new mCreepBehaviorProp(parent, true);
                break;
            case MxFunctions::MAT_CyclicHardening:
                pPropbase = new mCyclicHardeningProp(parent);
                break;
            case MxFunctions::MAT_CyclicHardeningWithTable:
                pPropbase = new mCyclicHardeningWithTableProp(parent);
                break;
            case MxFunctions::MAT_BulkModulus:
                pPropbase = new mBulkModulusProp(parent);
                break;
            case MxFunctions::MAT_AcousticMaterialDamping:
                pPropbase = new mAcousticsDampingProp(parent);
                break;
            case MxFunctions::MAT_TimeDomain:
                pPropbase = new mTimeDomainPronyProp(parent);
                break;
            case MxFunctions::MAT_TimeTemperatureEquivalence:
                pPropbase = new mThermalEquivalenceProp(parent);
                break;
                //case MxFunctions::Shim:
                //    pPropbase = m_ShimProp;
                //    break;
            case MxFunctions::MAT_Flex:
                pPropbase = new mFlexProp(parent);
                break;
            case MxFunctions::MAT_Plasticity:
                pPropbase = new mPlasticityProp(parent);
                break;
            case MxFunctions::MAT_PlasticityJohnson:
                pPropbase = new mPlasticityJohnsonProp(parent);
                break;
            case MxFunctions::MAT_RateDependent:
                pPropbase = new mRateDependentProp(parent);
                break;
            case MxFunctions::MAT_ThicknessBehavior:
                pPropbase = new mThicknessBehaviorProp(parent);
                break;
            case MxFunctions::MAT_FilmBehavior:
                pPropbase = new mFilmBehaviorProp(parent);
                break;
            case MxFunctions::MAT_CutBehavior:
                pPropbase = new mCutBehaviorProp(parent);
                break;
                //case MxFunctions::Heat:
                //    pPropbase = m_HeatProp;
                //    break;
            case MxFunctions::MAT_HeatConductivityCoefficient:
                pPropbase = new mHeatConductivityCoefficientProp(parent, true, MxFunctions::MAT_HeatConductivityCoefficient);
                break;
            case MxFunctions::MAT_HeatConductivityCoefficient_UnCheck:
                pPropbase = new mHeatConductivityCoefficientProp(parent, false, MxFunctions::MAT_HeatConductivityCoefficient_UnCheck);
                break;
            case MxFunctions::MAT_Heat:
                pPropbase = new mHeatCapacityDensityProp(parent);
                break;
            case MxFunctions::MAT_SoundVelocity:
                pPropbase = new mSoundVelocityProp(parent);
                break;
            case MxFunctions::MAT_Orthotropy:
                pPropbase = new mOrthotropyProp(parent);
                break;
            case MxFunctions::MAT_Anisotropy:
                pPropbase = new mAnisotropyProp(parent);
                break;
            case MxFunctions::MAT_IsotropicThermalMaterial:
                pPropbase = new mIsotropicThermalProp(parent);
                break;
            case MxFunctions::MAT_IsotropicHardening:
                pPropbase = new mIsotropicHardeningProp(parent);
                break;
            case MxFunctions::MAT_KinematicHardening:
                pPropbase = new mKinematicHardeningProp(parent);
                break;
            case MxFunctions::MAT_CompressiveHardening:
                pPropbase = new mCompressiveHardeningProp(parent);
                break;
                //case MxFunctions::MAT_Chaboche:
                //    pPropbase = new mChabocheProp(parent);
                //    break;
            case MxFunctions::MAT_Neohookean:
                pPropbase = new mNeoHookeanProp(parent);
                break;
            case MxFunctions::MAT_Neohookean2:
                pPropbase = new mNeoHookean2Prop(parent);
                break;
            case MxFunctions::MAT_MooneyRivlin:
                pPropbase = new mMooneyRivlinProp(parent);
                break;
            case MxFunctions::MAT_Ogden:
                pPropbase = new mOgdenProp(parent);
                break;
            case MxFunctions::MAT_Arrudaboyce:
                pPropbase = new mArrudaBoyceProp(parent);
                break;
            case MxFunctions::MAT_Yeoh:
                pPropbase = new mYeohProp(parent);
                break;
            case MxFunctions::MAT_Polynomial:
                pPropbase = new mPolynomialProp(parent);
                break;
            case MxFunctions::MAT_Rpolynomial:
                pPropbase = new mReducedPolynomialProp(parent);
                break;
            case MxFunctions::MAT_Hyperhorm:
                pPropbase = new mHyperfoamProp(parent);
                break;
                //case MxFunctions::MAT_Rigid:
                //	pPropbase = m_RigidProp;
                //	break;
                //case MxFunctions::MAT_Piezoelectric:
                //	pPropbase = m_PiezoelectricProp;
                //	break;
            case MxFunctions::MAT_Deformation:
                pPropbase = new mDeformationPlasticityProp(parent);
                break;
                //case MxFunctions::Elasticfiber:
                //	pPropbase = m_FiberReinforcedProp;
                //	break;
                //case MxFunctions::Viscoplasticity:
                //	pPropbase = m_ViscoplasticProp;
                //	break;
            case MxFunctions::MAT_Viscoelasticity:
                pPropbase = new mViscousElasticProp(parent);
                break;
            case MxFunctions::MAT_HardBrittle:
                pPropbase = new mBrittleLikeProp(parent);
                break;
            case MxFunctions::MAT_LibCommon:
                pPropbase = new mMatFromLibCommon(parent);
                break;
            case MxFunctions::MAT_LibStructural:
                pPropbase = new mMatFromLibStructural(parent);
                break;
            case MxFunctions::MAT_LibThermal:
                pPropbase = new mMatFromLibThermal(parent);
                break;
                //case MxFunctions::MAT_LibElectic:
                //	pPropbase = m_MatFromLibElectic;
                //	break;
            case MxFunctions::MAT_FailureHashin:
                pPropbase = new mFailureHashinProp(parent);
                break;
            case MxFunctions::MAT_EnergyDamage:
                pPropbase = new mEnergyDamageProp(parent);
                break;
            case MxFunctions::MAT_PlasticityWithOrder:
                pPropbase = new mPlasticityOrderProp(parent);
                break;
                //case MxFunctions::MAT_Break:
                //    pPropbase = m_BreakProp;
                //    break;
            case MxFunctions::MAT_BreakParam:
                pPropbase = new mBreakParamProp(parent);
                break;
            case MxFunctions::MAT_PhaseFieldParam:
                pPropbase = new mPhaseFieldProp(parent);
                break;
            case MxFunctions::NONETYPE:
                pPropbase = new mDefaultProperty(parent);
                break;
            case MxFunctions::MAT_ShearDamage:
                pPropbase = new mShearDamageProp(parent);
                break;
            case MxFunctions::MAT_DamageEvolution:
                pPropbase = new mDamageEvolutionProp(parent);
                break;
            default:
                pPropbase = nullptr;
                break;
            }

            m_FoldCache[propType] = pPropbase;
        }

        return m_FoldCache[propType];
    }


    void mMaterialProps::updateAllPropUnit()
    {
        for (auto fold : m_FoldCache)
        {
            fold->updatePropUnit();
        }
    }

    /***********************************材料定义***************************************/
    mMaterialDefine::mMaterialDefine(QWidget* parent)
        :mMaterialPropBase(MAT_MaterialDefine, QString("材料定义"), parent)
    {
        initInputs();
    }

    mMaterialDefine::~mMaterialDefine()
    {
    }

    void mMaterialDefine::initInputs()
    {
        m_qComboBoxType = new QComboBox();
        m_qComboBoxType->addItems({ "线弹性", "塑性", "超弹性", "粘性", "热学", "其它材料", "声学材料"});
        m_qLabelType = new PanelLabel("材料种类");
        this->addRow(m_qLabelType, m_qComboBoxType);
        m_qComboBoxType->setCurrentIndex(0);
        connect(m_qComboBoxType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                refreshTypePanel(index);

            });

        m_qLabeClass = new PanelLabel("材料类型");
        m_qComboBoxClass = new QComboBox();
        m_qComboBoxClass->addItems({ "各向同性", "正交异性", "各向异性" });
        this->addRow(m_qLabeClass, m_qComboBoxClass);
        m_qComboBoxClass->setCurrentIndex(0);
        // ============ 核心映射表：大类索引 -> 子类型列表 ============
        m_materialTypeMap = {
            {0, {"各向同性", "正交异性", "各向异性"}},                                            // 线弹性
            {1, {"常规塑性", "循环塑性", "变形塑性", "Johnson-Cook塑性"}},                        // 塑性
            {2, {"Mooney-Rivlin", "Neo-Hookean", "Ogden", "Yeoh", "Arruda-Boyce", "Polynomial"}},  //超弹性
            {3, {"粘弹性", "粘塑性"}},                                                            // 粘性
            {5, {"垫片材料", "脆性材料", "超弹泡沫", "断裂材料"}}                                 // 其它材料
            // 4热学、6声学：无子类型，不配置
            // 损伤大类已移除：剪切损伤并入Johnson-Cook塑性的可选折叠框
        };

        connect(m_qComboBoxClass, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                if (m_bAutoRefreshUI)
                    return;

                int curTypeIdx = m_qComboBoxType->currentIndex();
                // 当前大类存在子类型，才保存选择下标
                if (m_materialTypeMap.contains(curTypeIdx))
                {
                    m_lastSelectIndex[curTypeIdx] = index;
                }
                m_curType = Index2Type(curTypeIdx, index);
                emit sig_qComBoxTypeChanged(m_curType);
            });

    }

    void mMaterialDefine::refreshTypePanel(int TypeIndex)
    {
        if (m_materialTypeMap.contains(TypeIndex))
        {
            m_qComboBoxClass->blockSignals(true);
            m_qComboBoxClass->clear();
            m_qComboBoxClass->addItems(m_materialTypeMap[TypeIndex]);
            m_qComboBoxClass->setCurrentIndex(-2);
            m_qComboBoxClass->blockSignals(false);
            if (m_lastSelectIndex.contains(TypeIndex))
            {
                int savedIdx = m_lastSelectIndex[TypeIndex];
                // 边界保护：防止缓存下标超出当前列表范围
                if (savedIdx >= 0 && savedIdx < m_qComboBoxClass->count())
                    m_qComboBoxClass->setCurrentIndex(savedIdx);
                else
                    m_qComboBoxClass->setCurrentIndex(0);
            }
            else
            {
                m_qComboBoxClass->setCurrentIndex(0);
            }
            m_qLabeClass->setVisible(true);
            m_qComboBoxClass->setVisible(true);
        }
        else
        {
            // 热学、声学材料：没有子类型，隐藏整行
            m_qLabeClass->setVisible(false);
            m_qComboBoxClass->setVisible(false);

            m_curType = Index2Type(TypeIndex, 0);
            emit sig_qComBoxTypeChanged(m_curType);
        }
    }

    void mMaterialDefine::InitFoldSection()
    {
        return;
    }

    void mMaterialDefine::clearInputs()
    {

    }

    void mMaterialDefine::updatePropUnit()
    {
    }

    void mMaterialDefine::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        if (prop.contains(MatProp_MaterialType))
        {
            // 读取QVariant
            QVariant var = prop.value(MatProp_MaterialType);
            if (var.canConvert<int>())
            {
                int typeVal = var.toInt();
                // 安全强转
                m_curType = static_cast<MxFunctions::MaterialType>(typeVal);
                std::pair<int, int> type = GetIndexFromType(m_curType);
                m_bAutoRefreshUI = true;
                m_qComboBoxType->setCurrentIndex(type.first);
                m_qComboBoxClass->setCurrentIndex(type.second);
                m_bAutoRefreshUI = false;
            }
        }
    }


    void mMaterialDefine::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mMaterialDefine::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mMaterialDefine::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mMaterialDefine::slot_unitSystemResponse);
    }

    void mMaterialDefine::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        Q_UNUSED(type);
        Q_UNUSED(idx);
    }



    /***********************************密度***************************************/
    mDensityProp::mDensityProp(QWidget* parent, bool isCheckable, MxFunctions::MaterialClass classKey)
        :mMaterialPropBase(classKey, "密度", parent), m_bCheckable(isCheckable)
    {
        initInputs();
    }

    mDensityProp::~mDensityProp()
    {
        clearInputs();
    }

    void mDensityProp::initInputs()
    {
        QStringList names = {
            "密度",
        };

        QStringList header;
        header << "温度";
        for (int i = 0; i < names.size(); ++i)
        {
            header << names[i];
        }
        //初始化输入控件	
        inputWidgets.clear();
        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_Density;
            // 设置文本
            widgetSet.labelname->setText(names[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });

            inputWidgets.append(widgetSet);
        }
        checkboxtemp_.initWidget("密度");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Density);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-密度曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("密度") },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });
        updatePropUnit();
        InitFoldSection();
    }

    void mDensityProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "密度")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mDensityProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mDensityProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mDensityProp::slot_connTable);
    }

    void mDensityProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mDensityProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString denunit = UnitsLibrary[(int)Unit::Density][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(denunit);
        inputWidgets[0].lineeditvalue->setUnit(denunit);
    }

    void mDensityProp::clearInputs()
    {
        checkboxtemp_.clear();
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mDensityProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Density, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_Density, ratio} };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mDensityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        bool bIsFold = false;
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Density, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MxFunctions::MaterialPropType::MatProp_Density))//单值给linedit设置
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_Density];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MxFunctions::MaterialPropType::MatProp_Curve_Density))//曲线给btn设置
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Density];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mDensityProp::InitFoldSection()
    {
        if (m_bCheckable)
            this->setCheckable(true);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    void mDensityProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mDensityProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mDensityProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mDensityProp::slot_unitSystemResponse);
    }

    void mDensityProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & UnitSystem))return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }

        //auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        //QString denunit = UnitsLibrary[(int)Unit::Density][idx[0]];
        //double ratio = mgr->getSwitchUnitSystemRatio(Unit::Density);   //旧→新乘数，内部用 his/cur

        //inputWidgets[0].lineeditvalue->setUnit(denunit);
        //QString text = inputWidgets[0].lineeditvalue->text().trimmed();
        //if (!text.isEmpty())
        //{
        //    bool ok = false;
        //    double v = text.toDouble(&ok);
        //    if (ok)
        //        inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio));
        //}
        //if (m_matData)
        //{
        //    auto all = m_matData->getAllProps();
        //    all[getPropertyClass()].unite(collectProperties());
        //    m_matData->setProperty(all);
        //}
    }


    /***********************************热膨胀系数***************************************/
    mThermalExpansivityProp::mThermalExpansivityProp(QWidget* parent)
        :mMaterialPropBase(MAT_CoefficientOfThermalExpansion, "热膨胀系数", parent)
    {
        initInputs();
    }

    mThermalExpansivityProp::~mThermalExpansivityProp()
    {
        //clearInputs();
    }

    void mThermalExpansivityProp::initInputs()
    {
        QStringList names = {
            "参考温度  ",
            "A1",
            "A2",
            "A3",
            "A12",
            "A13",
            "A23",
        };
        QVector<MaterialPropType> proptypes = {
            MatProp_ReferenceTemperature,
            MatProp_CoefficientOfThermalExpansion_A1,
            MatProp_CoefficientOfThermalExpansion_A2,
            MatProp_CoefficientOfThermalExpansion_A3,
            MatProp_CoefficientOfThermalExpansion_A12,
            MatProp_CoefficientOfThermalExpansion_A13,
            MatProp_CoefficientOfThermalExpansion_A23,
        };
        combobox_.initWidget("热膨胀系数");
        combobox_.proptype_ = MatProp_CoefficientOfThermalExpansion;
        //combobox_.combobox->addItems({ "各向同性热膨胀系数","二维正交各向异性热膨胀系数",
        //    "三维正交各向异性热膨胀系数","二维完全各向异性热膨胀系数","三维完全各向异性热膨胀系数"});

        combobox_.combobox->addItems({ "各向同性热膨胀系数",
            "三维正交各向异性热膨胀系数","三维各向异性热膨胀系数" });

        combobox_.combobox->setCurrentIndex(0);
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mThermalExpansivityProp::slot_typeChanged);


        //初始化输入控件
        inputWidgetsTmp.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            // 设置文本
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];

            if (i != 0)type2inp[proptypes[i]] = i - 1;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            if (i == 0)
            {
                inputWidgets1.append(widgetSet);
                widgetSet.lineeditvalue->setExpression("0");
            }
            else inputWidgetsTmp.append(widgetSet);
        }
        inputWidgets2.append(inputWidgetsTmp[0]);
        checkboxtemp_.initWidget("热膨胀系数");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_CoefficientOfThermalExpansion);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随类型切换变化（各向同性 1 列/正交 3 列/各向异性 6 列），
        // 初始按当前 inputWidgets2 生成，slot_typeChanged 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-热膨胀系数曲线"),
            QStringLiteral("温度 / °C"),
            { inputWidgets2[0].labelname->text().trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            for (auto inp : inputWidgetsTmp)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });
        updatePropUnit();
        InitFoldSection();
    }

    void mThermalExpansivityProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets2)
        {
            params << inp.labelname->text().trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-热膨胀系数曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mThermalExpansivityProp::slot_typeChanged(int i)
    {
        inputWidgets2.clear();
        if (i == 0)
        {
            inputWidgets2.append(inputWidgetsTmp[0]);
        }
        /*else if (i == 1)
        {
            inputWidgets2.append(inputWidgetsTmp[0]);
            inputWidgets2.append(inputWidgetsTmp[1]);
        }*/
        else if (i == 1)
        {
            inputWidgets2.append(inputWidgetsTmp[0]);
            inputWidgets2.append(inputWidgetsTmp[1]);
            inputWidgets2.append(inputWidgetsTmp[2]);
        }
        /* else if (i == 3)
         {
             inputWidgets2.append(inputWidgetsTmp[0]);
             inputWidgets2.append(inputWidgetsTmp[1]);
             inputWidgets2.append(inputWidgetsTmp[3]);
         }*/
        else if (i == 2)
        {
            for (int j = 0; j <= 5; j++)
            {
                inputWidgets2.append(inputWidgetsTmp[j]);
            }
        }

        for (int j = 0; j <= 5; j++)
        {
            inputWidgets.append(inputWidgetsTmp[j]);
        }

        this->removeAll();
        this->addRow(inputWidgets1[0].propwid);
        inputWidgets1[0].propwid->show();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        for (auto inp : inputWidgets2)
        {
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        // 类型切换后参数列数变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mThermalExpansivityProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "热膨胀系数")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mThermalExpansivityProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mThermalExpansivityProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mThermalExpansivityProp::slot_connTable);
    }

    void mThermalExpansivityProp::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
        inputWidgets1[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(0.0));
    }

    void mThermalExpansivityProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit1 = "";
        QString unit2 = "";
        if (sysunitidxs[0] >= 6)
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][2];
            unit2 = UnitsLibrary[(int)Unit::TempReciprocal][2];
        }
        else
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][sysunitidxs[3]];
            unit2 = UnitsLibrary[(int)Unit::TempReciprocal][sysunitidxs[3]];
        }
        //inputWidgets1[0].labelunit->setText(unit1);
        inputWidgets1[0].lineeditvalue->setUnit(unit1);
        for (auto inp : inputWidgetsTmp)
        {
            //inp.labelunit->setText(unit2);
            inp.lineeditvalue->setUnit(unit2);
        }
    }

    void mThermalExpansivityProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();

        // inputWidgets2 只是 inputWidgetsTmp 的浅拷贝引用（视图），
        // 不拥有控件，不要在这里调用 clear()，否则会同一份 widget 被重复释放。
        inputWidgets2.clear();

        for (auto& widgetSet : inputWidgetsTmp)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mThermalExpansivityProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TempReciprocal, sysunitidxs, { 3,0,0,0 });
        double shift1 = getCVM->getModelData()->getUnitSystemManager()->getTempShiftFromTo(sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_ReferenceTemperature, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A1, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A2, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A3, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A12, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A13, ratio1},
                {MatProp_CoefficientOfThermalExpansion_A23, ratio1},

        };
        collectProps(props, inputWidgets1, ratios);
        if (props.contains(MatProp_ReferenceTemperature))
            props[MatProp_ReferenceTemperature] = props[MatProp_ReferenceTemperature].toDouble() + shift1;

        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_ReferenceTemperature, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A1, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A2, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A3, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A12, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A13, ratio2},
                {MatProp_CoefficientOfThermalExpansion_A23, ratio2},

            };
            collectProps(props, inputWidgets2, ratios);

        }
        return props;
    }

    void mThermalExpansivityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QVector<int> cur = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QVector<int> old = QVector<int>{ 3, 0, 0, 0 };
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, old, cur);
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TempReciprocal, old, cur);
        double shift1 = getCVM->getModelData()->getUnitSystemManager()->getTempShiftFromTo(old, cur);
        if (prop.contains(MatProp_CoefficientOfThermalExpansion))
        {
            int idx = prop[MatProp_CoefficientOfThermalExpansion].toInt();
            //combobox_.combobox->blockSignals(true);
            //if(fa)
            combobox_.combobox->setCurrentIndex(idx);
            //combobox_.combobox->blockSignals(false);
            inputWidgets2.clear();
            if (idx == 0)
            {
                inputWidgets2.append(inputWidgetsTmp[0]);
            }
            /*if (idx == 1)
            {
                inputWidgets2.append(inputWidgetsTmp[0]);
                inputWidgets2.append(inputWidgetsTmp[1]);
            }*/
            if (idx == 1)
            {
                inputWidgets2.append(inputWidgetsTmp[0]);
                inputWidgets2.append(inputWidgetsTmp[1]);
                inputWidgets2.append(inputWidgetsTmp[2]);
            }
            /* if (idx == 3)
             {
                 inputWidgets2.append(inputWidgetsTmp[0]);
                 inputWidgets2.append(inputWidgetsTmp[1]);
                 inputWidgets2.append(inputWidgetsTmp[3]);
             }*/
            if (idx == 2)
            {
                for (int i = 0; i <= 5; ++i)
                {
                    inputWidgets2.append(inputWidgetsTmp[i]);
                }

            }

        }
        //参考温度
        if (prop.contains(MatProp_ReferenceTemperature))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_ReferenceTemperature];
            if (data.isValid())
            {
                double v = data.toDouble() * ratio1 + shift1;
                inputWidgets1[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }

        //热膨胀系数
        if (prop.contains(MatProp_Curve_CoefficientOfThermalExpansion))
        {
            inputWidgets2[0].lineeditvalue->setExpression("");
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_CoefficientOfThermalExpansion];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = it.value();
                if (data.isValid())
                {
                    inputWidgetsTmp[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() * ratio2));
                }
            }
        }
        updatePropUnit();
    }

    void mThermalExpansivityProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mThermalExpansivityProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mThermalExpansivityProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mThermalExpansivityProp::slot_unitSystemResponse);
    }

    void mThermalExpansivityProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
        //auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        //QVector<int> cur = mgr->getCurUnitsIndex();
        //QVector<int> old = m_lastUnitsIdx.isEmpty() ? QVector<int>{ 3, 0, 0, 0 } : m_lastUnitsIdx;
        //QString unit1 = "";
        //QString unit2 = "";
        //if (cur[0] >= 6)
        //{
        //    unit1 = UnitsLibrary[(int)Unit::Temp][2];
        //    unit2 = UnitsLibrary[(int)Unit::TempReciprocal][2];
        //}
        //else
        //{
        //    unit1 = UnitsLibrary[(int)Unit::Temp][idx[3]];
        //    unit2 = UnitsLibrary[(int)Unit::TempReciprocal][idx[3]];
        //}
        //double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, old, cur);
        //double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TempReciprocal, old, cur);
        ////double ratio1 = mgr->getSwitchUnitSystemRatio(Unit::Temp);
        ////double ratio2 = mgr->getSwitchUnitSystemRatio(Unit::TempReciprocal);
        //double shift1 = mgr->getTempShiftFromTo(old, cur);
        ////double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, sysunitidxs, { 3,0,0,0 });
        ////double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TempReciprocal, sysunitidxs, { 3,0,0,0 });

        //inputWidgets1[0].lineeditvalue->setUnit(unit1);
        //QString data = inputWidgets1[0].lineeditvalue->text().trimmed();
        //if (!data.isEmpty())
        //{
        //    bool ok = false;
        //    double v = data.toDouble(&ok);
        //    if (ok)
        //        inputWidgets1[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio1 + shift1));
        //}
        //for (auto inp : inputWidgetsTmp)
        //{
        //    //inp.labelunit->setText(unit2);
        //    inp.lineeditvalue->setUnit(unit2);
        //    data = inp.lineeditvalue->text().trimmed();
        //    if (!data.isEmpty())
        //    {
        //        bool ok = false;
        //        double v = data.toDouble(&ok);
        //        if (ok)
        //            inp.lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio2));
        //    }
        //}
        //m_lastUnitsIdx = cur;
        //if (m_matData)
        //{
        //    auto all = m_matData->getAllProps();
        //    all[getPropertyClass()].unite(collectProperties());
        //    m_matData->setProperty(all);
        //}
    }

    void mThermalExpansivityProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets1)
        {
            this->addRow(widgetSet.propwid);
        }
        this->addRow(combobox_.propwid);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);

        for (auto& widgetSet : inputWidgets2)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************材料阻尼***************************************/
    mMaterialDampingProp::mMaterialDampingProp(QWidget* parent)
        :mMaterialPropBase(MAT_MaterialDamping, "材料阻尼", parent)
    {
        initInputs();
    }

    mMaterialDampingProp::~mMaterialDampingProp()
    {
        clearInputs();
    }

    void mMaterialDampingProp::initInputs()
    {
        QStringList names = {
            "材料阻尼"
        };
        QStringList units = {
            "s",
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            // 设置文本
            widgetSet.proptype_ = MatProp_MaterialDamping;
            //widgetSet.labelunit->setText(units[i]);
            widgetSet.lineeditvalue->setUnit(units[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        InitFoldSection();
    }

    void mMaterialDampingProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mMaterialDampingProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mMaterialDampingProp::updatePropUnit()
    {

    }

    QHash<MxFunctions::MaterialPropType, QVariant> mMaterialDampingProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets);
        return props;
    }

    void mMaterialDampingProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_MaterialDamping))
        {
            QVariant data = prop[MatProp_MaterialDamping];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mMaterialDampingProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************失效应力***************************************/
    mLoseEfficacyProp::mLoseEfficacyProp(QWidget* parent)
        :mMaterialPropBase(MAT_YieldStress, "失效应力", parent)
    {
        initInputs();
    }

    mLoseEfficacyProp::~mLoseEfficacyProp()
    {
        clearInputs();
    }

    void mLoseEfficacyProp::initInputs()
    {
        QStringList names2 = {
          "纵向拉伸强度",
          "纵向压缩强度",
          "横向拉伸强度",
          "横向压缩强度",
          "面内剪切强度",
          "相互作用项",
          "双轴应力项",
        };
        QStringList names1 = {
            "拉伸强度",
            "压缩强度",
            "剪切强度",
            "相互作用项",
            "双轴应力项",
        };
        proptypes_2 = {
            MatProp_Xt,
            MatProp_Xc,
            MatProp_Yt,
            MatProp_Yc,
            MatProp_S,
            MatProp_F12,
            MatProp_Biax
        };
        proptypes_ = {
            MatProp_St,
            MatProp_Sc,
            MatProp_Ss,
            MatProp_F12,
            MatProp_Biax
        };

        //初始化输入控件
        combobox_.initWidget("应力失效模型");
        combobox_.proptype_ = MatProp_FailStressType;
        combobox_.combobox->addItem("各向同性");
        combobox_.combobox->addItem("二维正交各向异性");
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoseEfficacyProp::slot_switchInputs);

        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();


        for (int i = 0; i < 5; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown,
                s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_[i];
            type2inp_[proptypes_[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }

        for (int i = 0; i < 7; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown,
                s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_2[i];
            type2inp2_[proptypes_2[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp2.append(widgetSet);
        }

        checkboxtemp_.initWidget("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_FailStress);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随失效类型切换（双曲线/幂律），初始按默认类型生成，
        // slot_switchInputs 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-失效应力曲线"),
            QStringLiteral("温度 / °C"),
            { inputWidgetsTmp1[0].name_.trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets_)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        inputWidgets_ = inputWidgetsTmp1;
        updatePropUnit();
        InitFoldSection();
    }

    void mLoseEfficacyProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets_)
        {
            params << inp.name_.trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-失效应力曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mLoseEfficacyProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgets_.clear();
        combobox_.clear();
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
            m_tempCurveRow->clearCurve();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mLoseEfficacyProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mLoseEfficacyProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "失效应力")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mLoseEfficacyProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }

        inputWidgets_.clear();
        if (i == 0)
        {
            inputWidgets_ = inputWidgetsTmp1;
        }
        else
        {
            inputWidgets_ = inputWidgetsTmp2;
        }
        for (int j = 0; j < inputWidgets_.size(); ++j)
        {
            this->addRow(inputWidgets_[j].propwid);
            inputWidgets_[j].propwid->show();
        }
        // 类型切换后参数列变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();
        this->refreshLayout();

        //emit sg_editFinished();
    }

    void mLoseEfficacyProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mLoseEfficacyProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mLoseEfficacyProp::slot_connTable);
    }

    void mLoseEfficacyProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        for (auto inp : inputWidgetsTmp1)
        {
            //inp.labelunit->setText(unit);
            inp.lineeditvalue->setUnit(unit);
        }
        for (auto inp : inputWidgetsTmp2)
        {
            //inp.labelunit->setText(unit);
            inp.lineeditvalue->setUnit(unit);
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mLoseEfficacyProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());

        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_Xt, ratio},
                {MatProp_Xc, ratio},
                {MatProp_Yt, ratio},
                {MatProp_Yc, ratio},
                {MatProp_S, ratio},
                {MatProp_F12, ratio},
                {MatProp_Biax, ratio},//
                {MatProp_St, ratio},
                {MatProp_Sc, ratio},
                {MatProp_Ss, ratio},

            };
            collectProps(props, inputWidgets_, ratios);

        }
        return props;
    }

    void mLoseEfficacyProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mLoseEfficacyProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoseEfficacyProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mLoseEfficacyProp::slot_unitSystemResponse);
    }

    void mLoseEfficacyProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mLoseEfficacyProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        int idx = 0;
        if (prop.contains(MatProp_FailStressType))
        {
            QVariant data = prop[MatProp_FailStressType];
            idx = data.toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            combobox_.combobox->blockSignals(false);
            if (idx == 0)
            {
                inputWidgets_ = inputWidgetsTmp1;
            }
            else
            {
                inputWidgets_ = inputWidgetsTmp2;
            }
        }

        //参考温度
        if (!prop.contains(MatProp_Curve_FailStress))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            if (idx == 0)
            {
                for (auto p : proptypes_)
                {
                    QVariant data = prop[p];
                    if (data.isValid())
                    {
                        inputWidgets_[type2inp_[p]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                    }
                }
            }
            else
            {
                for (auto p : proptypes_2)
                {
                    QVariant data = prop[p];
                    if (data.isValid())
                    {
                        inputWidgets_[type2inp2_[p]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                    }
                }
            }
        }
        else
        {
            for (auto inp : inputWidgets_)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_FailStress];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mLoseEfficacyProp::InitFoldSection()
    {
        this->setCheckable(true);
        this->addRow(combobox_.propwid);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        for (auto& widgetSet : inputWidgets_)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************失效应变***************************************/
    mFailureEffectProp::mFailureEffectProp(QWidget* parent)
        :mMaterialPropBase(MAT_YieldStrain, "失效应变", parent)
    {
        initInputs();
    }

    mFailureEffectProp::~mFailureEffectProp()
    {
        clearInputs();
    }

    void mFailureEffectProp::initInputs()
    {

        QStringList names1 = {
            "拉伸应变极限",
            "压缩应变极限",
            "剪切应变极限",
        };
        QStringList names2 = {
            "纵向拉伸应变极限",
            "纵向压缩应变极限",
            "横向拉伸应变极限",
            "横向压缩应变极限",
            "面内剪切应变极限",
        };
        proptypes_ = {
           MatProp_St,
           MatProp_Sc,
           MatProp_Ss,
        };

        proptypes_2 = {
            MatProp_Xt,
            MatProp_Xc,
            MatProp_Yt,
            MatProp_Yc,
            MatProp_S,
        };

        //初始化输入控件
        combobox_.initWidget("应变失效模型");
        combobox_.proptype_ = MatProp_FailStrainType;
        combobox_.combobox->addItem("各向同性");
        combobox_.combobox->addItem("二维正交各向异性");
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mFailureEffectProp::slot_switchInputs);

        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();

        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_[i];
            type2inp_[proptypes_[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < 5; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_2[i];
            type2inp2_[proptypes_2[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp2.append(widgetSet);
        }

        checkboxtemp_.initWidget("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_FailStrain);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随失效类型切换，初始按默认类型生成，
        // slot_switchInputs 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-失效应变曲线"),
            QStringLiteral("温度 / °C"),
            { inputWidgetsTmp1[0].name_.trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets_)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        inputWidgets_ = inputWidgetsTmp1;
        InitFoldSection();
    }

    void mFailureEffectProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets_)
        {
            params << inp.name_.trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-失效应变曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mFailureEffectProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgets_.clear();
        combobox_.clear();
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
            m_tempCurveRow->clearCurve();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mFailureEffectProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mFailureEffectProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "失效应变")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mFailureEffectProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }

        inputWidgets_.clear();
        if (i == 0)
        {
            inputWidgets_ = inputWidgetsTmp1;
        }
        else
        {
            inputWidgets_ = inputWidgetsTmp2;
        }
        for (int j = 0; j < inputWidgets_.size(); ++j)
        {
            this->addRow(inputWidgets_[j].propwid);
            inputWidgets_[j].propwid->show();
        }
        // 类型切换后参数列变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();
        this->refreshLayout();

        emit sg_editFinished();
    }

    void mFailureEffectProp::connTable(bool conn)
    {
        if (conn)
            //通用属性 这里连接时都判断一下是不是已经连接了
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFailureEffectProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFailureEffectProp::slot_connTable);
    }

    void mFailureEffectProp::updatePropUnit()
    {
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mFailureEffectProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            collectProps(props, inputWidgets_);
        }
        return props;
    }

    void mFailureEffectProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        int idx = 0;
        if (prop.contains(MatProp_FailStrainType))
        {
            QVariant data = prop[MatProp_FailStrainType];
            idx = data.toInt();
            this->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            this->blockSignals(false);
            if (idx == 0)
            {
                inputWidgets_ = inputWidgetsTmp1;
            }
            else
            {
                inputWidgets_ = inputWidgetsTmp2;
            }
        }
        //参考温度
        if (!prop.contains(MatProp_Curve_FailStrain))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            if (idx == 0)
            {
                for (auto p : proptypes_)
                {
                    QVariant data = prop[p];
                    if (data.isValid())
                    {
                        inputWidgets_[type2inp_[p]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                    }
                }
            }
            else
            {
                for (auto p : proptypes_2)
                {
                    QVariant data = prop[p];
                    if (data.isValid())
                    {
                        inputWidgets_[type2inp2_[p]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                    }
                }
            }
        }
        else
        {
            for (auto inp : inputWidgets_)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_FailStrain];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mFailureEffectProp::InitFoldSection()
    {
        this->setCheckable(true);
        this->addRow(combobox_.propwid);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        for (auto& widgetSet : inputWidgets_)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************Hashin失效***************************************/
    mFailureHashinProp::mFailureHashinProp(QWidget* parent)
        :mMaterialPropBase(MAT_FailureHashin, "Hashin失效", parent)
    {
        initInputs();
    }

    mFailureHashinProp::~mFailureHashinProp()
    {
        clearInputs();
    }

    void mFailureHashinProp::initInputs()
    {
        QStringList names = {
            "alpha",
            "纵向拉伸强度",
            "纵向压缩强度",
            "横向拉伸强度",
            "横向压缩强度",
            "纵向剪切强度",
            "横向剪切强度",
        };
        proptypes_ = {
            MatProp_Alpha,
            MatProp_Xt,
            MatProp_Xc,
            MatProp_Yt,
            MatProp_Yc,
            MatProp_Sl,
            MatProp_St,
        };

        //初始化输入控件
        inputWidgets_.clear();

        for (int i = 0; i < 7; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_[i];
            type2inp[proptypes_[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets_.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mFailureHashinProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets_)
        {
            widgetSet.clear();
        }
        inputWidgets_.clear();
    }

    void mFailureHashinProp::clearData()
    {
        for (auto inp : inputWidgets_)
        {
            inp.lineeditvalue->setExpression("");
        }
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mFailureHashinProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets_);
        return props;
    }

    void mFailureHashinProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = prop[it.key()];
            if (data.isValid())
            {
                inputWidgets_[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        updatePropUnit();
    }

    void mFailureHashinProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets_)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    void mFailureHashinProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString noneunit = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        QString denunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(denunit);
        for (auto inp : inputWidgets_)
        {
            inp.lineeditvalue->setUnit(denunit);
        }
        inputWidgets_[0].lineeditvalue->setUnit(noneunit);
    }

    /***********************************Hashin渐进损伤***************************************/
    mEnergyDamageProp::mEnergyDamageProp(QWidget* parent)
        :mMaterialPropBase(MAT_EnergyDamage, "Hashin渐进损伤", parent)
    {
        initInputs();
    }

    mEnergyDamageProp::~mEnergyDamageProp()
    {
        clearInputs();
    }

    void mEnergyDamageProp::initInputs()
    {
        QStringList names = {
            "纤维拉伸断裂能",
            "纤维压缩断裂能",
            "基体拉伸断裂能",
            "基体压缩断裂能",
        };
        proptypes_ = {
            MatProp_Gft,
            MatProp_Gfc,
            MatProp_Gmt,
            MatProp_Gmc,
        };

        //初始化输入控件
        inputWidgets_.clear();

        for (int i = 0; i < 4; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_[i];
            type2inp[proptypes_[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets_.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mEnergyDamageProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets_)
        {
            widgetSet.clear();
        }
        inputWidgets_.clear();
    }

    void mEnergyDamageProp::clearData()
    {
        for (auto inp : inputWidgets_)
        {
            inp.lineeditvalue->setExpression("");
        }
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mEnergyDamageProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets_);
        return props;
    }

    void mEnergyDamageProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = prop[it.key()];
            if (data.isValid())
            {
                inputWidgets_[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        updatePropUnit();
    }

    void mEnergyDamageProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets_)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    void mEnergyDamageProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString denunit = UnitsLibrary[(int)Unit::Rigidity][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(denunit);
        for (auto inp : inputWidgets_)
        {
            inp.lineeditvalue->setUnit(denunit);
        }

    }

    /***********************************热生成率***************************************/
    mHeatGenerationRateProp::mHeatGenerationRateProp(QWidget* parent)
        :mMaterialPropBase(MAT_HeatGenerationRate, "热生成率", parent)
    {
        initInputs();
    }

    mHeatGenerationRateProp::~mHeatGenerationRateProp()
    {
        clearInputs();
    }

    void mHeatGenerationRateProp::initInputs()
    {
        QStringList names = {
            "热生成率",
        };

        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_HeatGenerationRate;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mHeatGenerationRateProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mHeatGenerationRateProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mHeatGenerationRateProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString denunit = UnitsLibrary[(int)Unit::PowerDensity][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(denunit);
        inputWidgets[0].lineeditvalue->setUnit(denunit);
    }

    void mHeatGenerationRateProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mHeatGenerationRateProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::PowerDensity, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_HeatGenerationRate, ratio} };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mHeatGenerationRateProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::PowerDensity, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_HeatGenerationRate))
        {
            QVariant data = prop[MatProp_HeatGenerationRate];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mHeatGenerationRateProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************蠕变行为***************************************/
    mCreepBehaviorProp::mCreepBehaviorProp(QWidget* parent, bool isCheckable)
        :mMaterialPropBase(MAT_CreepBehavior, "蠕变行为", parent), m_bIsCheckable(isCheckable)
    {
        initInputs();
    }

    mCreepBehaviorProp::~mCreepBehaviorProp()
    {
        clearInputs();
    }

    void mCreepBehaviorProp::initInputs()
    {
        QStringList names = {
            "幂律乘数",
            "应力指数  ",
            "时间指数  ",
        };
        QVector<MaterialPropType>proptypes = {
            MatProp_NortonCoefficient,
            MatProp_StressExponent,
            MatProp_TimeIndex,
        };
        QStringList header;
        header << "温度";
        for (int i = 0; i < names.size(); ++i)
        {
            header << names[i];
        }
        //初始化输入控件
        combobox_.initWidget("硬化类型");
        combobox_.proptype_ = MatProp_HardeningType;
        combobox_.combobox->addItem("时间");
        combobox_.combobox->addItem("应变");
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mCreepBehaviorProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);

        inputWidgets.clear();
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        //粘性-粘塑性入口(MAT_CreepBehavior)独有：Anand硬化类型；
        //常规塑性入口为MAT_CreepBehavior_Check(isCheckable=true)，不追加
        if (!m_bIsCheckable)
        {
            combobox_.combobox->addItem("Anand");

            QStringList anandNames = {
                "初始变形阻力S1",
                "活化能Q/R",
                "幂律乘数A",
                "应力乘子Xi",
                "应力幂指数m",
                "硬化常数A0",
                "阻力幂律乘数s^",
                "阻力幂指数n",
                "应变速率指数a",
                "阻力演化系数S2",
                "阻力演化系数S3",
                "硬化系数A1",
                "硬化系数A2",
                "硬化系数A3",
                "硬化系数A4",
            };
            QVector<MaterialPropType> anandPropTypes = {
                MatProp_Anand_DeformationResistance,
                MatProp_Anand_ActivationEnergy,
                MatProp_Anand_PowerLawMultiplier1,
                MatProp_Anand_StressMultiplier,
                MatProp_Anand_StressExponent,
                MatProp_Anand_HardeningConstant,
                MatProp_Anand_ResistancePowerLawMultiplier,
                MatProp_Anand_ResistancePowerExponent,
                MatProp_Anand_RateExponent,
                MatProp_Anand_TempCoefficientS2,
                MatProp_Anand_TempCoefficientS3,
                MatProp_Anand_TempCoefficientA1,
                MatProp_Anand_TempCoefficientA2,
                MatProp_Anand_CreepCoefficientA3,
                MatProp_Anand_CreepCoefficientA4,
            };
            for (int i = 0; i < anandNames.size(); ++i)
            {
                InputWidgetSet widgetSet;
                widgetSet.initWidget(anandNames[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(anandNames[i], CalcMode::Standard));
                widgetSet.proptype_ = anandPropTypes[i];
                type2inpAnand[anandPropTypes[i]] = i;
                connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                    [this]() {
                        emit sg_editFinished();
                    });
                inputWidgetsAnand.append(widgetSet);
            }
        }

        checkboxtemp_.initWidget("蠕变行为");
        checkboxtemp_.setPropType(MatProp_Curve_CreepBehavior);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-蠕变行为曲线"),
            QStringLiteral("温度 / °C"),
            { names[0].trimmed(), names[1].trimmed(), names[2].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mCreepBehaviorProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "蠕变行为")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mCreepBehaviorProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mCreepBehaviorProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mCreepBehaviorProp::slot_connTable);
    }

    void mCreepBehaviorProp::slot_switchInputs(int i)
    {
        //Anand(索引2)：解析模型参数，参数自带温度系数，不提供温度相关表格
        bool isAnand = (i == 2);
        if (isAnand && m_tempCurveRow)
        {
            m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
        }

        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();

        QVector<InputWidgetSet>& curWidgets = isAnand ? inputWidgetsAnand : inputWidgets;
        for (auto& widgetSet : curWidgets)
        {
            this->addRow(widgetSet.propwid);
            widgetSet.propwid->show();
        }

        if (!isAnand && m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        else if (m_tempCurveRow)
        {
            m_tempCurveRow->hide();
        }
        this->refreshLayout();

        emit sg_editFinished();
    }

    void mCreepBehaviorProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgetsAnand)
        {
            widgetSet.clear();
        }
        inputWidgetsAnand.clear();
        type2inpAnand.clear();
        checkboxtemp_.clear();
        combobox_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mCreepBehaviorProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsAnand)
        {
            inp.lineeditvalue->setExpression("");
        }
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mCreepBehaviorProp::updatePropUnit()
    {
        //Anand参数为系数类，不随单位制换算
        if (combobox_.combobox->currentIndex() == 2)
            return;
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(unit);
        inputWidgets[0].lineeditvalue->setUnit(unit);
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mCreepBehaviorProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());

        //Anand：15个模型参数，系数类不做单位换算
        if (combobox_.combobox->currentIndex() == 2)
        {
            collectProps(props, inputWidgetsAnand);
            return props;
        }

        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_NortonCoefficient, ratio},
        };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mCreepBehaviorProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_HardeningType))
        {
            QVariant data = prop[MatProp_HardeningType];
            int idx = data.toInt();
            combobox_.combobox->setCurrentIndex(idx);
        }
        if (prop.contains(MatProp_Curve_CreepBehavior))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);

            QVariant data = prop[MatProp_Curve_CreepBehavior];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (type2inpAnand.contains(it.key()))
                {
                    QVariant dataAnand = prop[it.key()];
                    if (dataAnand.isValid())
                    {
                        inputWidgetsAnand[type2inpAnand[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(dataAnand.toDouble()));
                    }
                    continue;
                }
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    if (type2inp[it.key()] == 0)
                        inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                    else
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
            }

        }
        updatePropUnit();
    }

    void mCreepBehaviorProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mCreepBehaviorProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mCreepBehaviorProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mCreepBehaviorProp::slot_unitSystemResponse);
    }

    void mCreepBehaviorProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mCreepBehaviorProp::InitFoldSection()
    {
        if (m_bIsCheckable)
        {
            this->setCheckable(true);
        }
        this->addRow(combobox_.propwid);
        if (combobox_.combobox->currentIndex() == 2)
        {
            //Anand：仅模型参数，无温度相关表格
            for (auto& widgetSet : inputWidgetsAnand)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            for (auto& widgetSet : inputWidgets)
            {
                this->addRow(widgetSet.propwid);
            }
            if (m_tempCurveRow)
                this->addRow(m_tempCurveRow);
        }
        return;
    }

    /***********************************循环硬化*两个输入框**************************************/
    mCyclicHardeningProp::mCyclicHardeningProp(QWidget* parent)
        :mMaterialPropBase(MAT_CyclicHardening, "循环硬化", parent)
    {
        initInputs();
    }

    mCyclicHardeningProp::~mCyclicHardeningProp()
    {
        clearInputs();
    }

    void mCyclicHardeningProp::initInputs()
    {
        QStringList names = {
            "极限屈服应力",
            "硬化速率控制",
        };
        QVector<MaterialPropType>proptypes = {
            MatProp_YieldStress,
            MatProp_HardeningRateControl,
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            // 设置文本
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mCyclicHardeningProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mCyclicHardeningProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mCyclicHardeningProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString unit_none = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(unit);
        //inputWidgets[1].labelunit->setText(unit);
        inputWidgets[0].lineeditvalue->setUnit(unit);
        inputWidgets[1].lineeditvalue->setUnit(unit_none);
    }
    void mCyclicHardeningProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mCyclicHardeningProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mCyclicHardeningProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mCyclicHardeningProp::slot_unitSystemResponse);
    }

    void mCyclicHardeningProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }
    QHash<MxFunctions::MaterialPropType, QVariant> mCyclicHardeningProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;


        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_YieldStress, ratio},
        };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mCyclicHardeningProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_YieldStress))
        {
            QVariant data = prop[MatProp_YieldStress];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_HardeningRateControl))
        {
            QVariant data = prop[MatProp_HardeningRateControl];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void  mCyclicHardeningProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************循环硬化带表格***************************************/
    mCyclicHardeningWithTableProp::mCyclicHardeningWithTableProp(QWidget* parent)
        :mMaterialPropBase(MAT_CyclicHardeningWithTable, "循环硬化", parent)
    {
        initInputs();
    }

    mCyclicHardeningWithTableProp::~mCyclicHardeningWithTableProp()
    {
        clearInputs();
    }

    void mCyclicHardeningWithTableProp::initInputs()
    {
        header_ << "温度";
        header_ << "等效屈服应力";
        header_ << "等效塑性应变";

        headerNoTemp_ << "等效屈服应力" << "等效塑性应变";

        tableName_ = new QLabel("等效应力-等效塑性应变曲线");
        checkboxtemp_.initWidget("循环硬化带表格");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_CyclicHardeningWithTable);
        // 勾选与否决定曲线是否含温度列（TemperatureCorrelation / NoCorrelation）。
        // 封装控件固定为温度相关形态（首列温度），未勾选时同样以温度列存储，
        // 数据协议与 collectProps(singleCheckBox, correlation) 保持一致
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-循环硬化曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("等效屈服应力"), QStringLiteral("等效塑性应变") },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mCyclicHardeningWithTableProp::clearInputs()
    {
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mCyclicHardeningWithTableProp::clearData()
    {
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mCyclicHardeningWithTableProp::updatePropUnit()
    {
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mCyclicHardeningWithTableProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, checkboxtemp_,
            checkboxtemp_.cb->isChecked()
            ? CurveType::TemperatureCorrelation : CurveType::NoCorrelation);
        return props;
    }

    void mCyclicHardeningWithTableProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_Curve_CyclicHardeningWithTable))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_Curve_CyclicHardeningWithTable];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
                if (curves.correlation == CurveType::TemperatureCorrelation)
                {
                    if (m_tempCurveRow)
                        m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
                }
            }
        }
        updatePropUnit();
    }

    void  mCyclicHardeningWithTableProp::InitFoldSection()
    {
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        return;
    }

    void mCyclicHardeningWithTableProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mCyclicHardeningWithTableProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mCyclicHardeningWithTableProp::slot_connTable);
    }

    void mCyclicHardeningWithTableProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "循环硬化带表格")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    /***********************************体积模量***************************************/
    mBulkModulusProp::mBulkModulusProp(QWidget* parent)
        :mMaterialPropBase(MAT_BulkModulus, "体积模量", parent)
    {
        initInputs();
    }

    mBulkModulusProp::~mBulkModulusProp()
    {
        clearInputs();
    }

    void mBulkModulusProp::initInputs()
    {
        QStringList names = {
            "体积模量",
        };
        units_ = QStringList{
            "Pa",                // 1  m-kg-N-s
            "dyne/cm²",          // 2  cm-g-dyne-s
            "MPa",               // 3  mm-kg-N-s
            "MPa",               // 4  mm-t-N-s
            "MPa",               // 5  mm-dat-N-s
            "kPa",               // 6  vm-kg-vN-s
            "lbf/ft²",           // 7  ft-lbm-lbf-s
            "psi"                // 8  in-lbm-lbf-s
        };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = units_[sysunitidxs[0]];
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_BulkModulus;
            // 设置文本
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mBulkModulusProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mBulkModulusProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mBulkModulusProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(unit);
        inputWidgets[0].lineeditvalue->setUnit(unit);
    }

    void mBulkModulusProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mBulkModulusProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_BulkModulus, ratio} };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mBulkModulusProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_BulkModulus))
        {
            QVariant data = prop[MatProp_BulkModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void  mBulkModulusProp::InitFoldSection()
    {
        this->setCheckable(true);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************声学材料阻尼（流体阻尼系数）***************************************/
    mAcousticsDampingProp::mAcousticsDampingProp(QWidget* parent)
        :mMaterialPropBase(MAT_AcousticMaterialDamping, "声学材料阻尼", parent)
    {
        initInputs();
    }

    mAcousticsDampingProp::~mAcousticsDampingProp()
    {
        clearInputs();
    }

    void mAcousticsDampingProp::initInputs()
    {
        QStringList names = {
            "流体阻尼系数",
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_AcousticMaterialDamping;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        InitFoldSection();
    }

    void mAcousticsDampingProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mAcousticsDampingProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mAcousticsDampingProp::updatePropUnit()
    {

    }

    QHash<MxFunctions::MaterialPropType, QVariant> mAcousticsDampingProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets);
        return props;
    }

    void mAcousticsDampingProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_AcousticMaterialDamping))
        {
            QVariant data = prop[MatProp_AcousticMaterialDamping];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void  mAcousticsDampingProp::InitFoldSection()
    {
        this->setCheckable(true);

        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************时域prony***************************************/
    mTimeDomainPronyProp::mTimeDomainPronyProp(QWidget* parent)
        :mMaterialPropBase(MAT_TimeDomain, "时域prony", parent)
    {
        initInputs();
    }

    mTimeDomainPronyProp::~mTimeDomainPronyProp()
    {
        clearInputs();
    }

    void mTimeDomainPronyProp::initInputs()
    {
        /*QStringList names = {
            "g_i Prony  ",
            "k_i Prony  ",
            "tau_i Prony",
        };*/
        QVector<MaterialPropType>proptypes = {
            MatProp_GIProny,
            MatProp_KIProny,
            MatProp_TuaIProny,
        };
        QStringList units = {
            "",
            "",
            "s",
        };
        //初始化输入控件
        inputWidgets.clear();

        //for (int i = 0; i < 3; ++i)
        //{
        //	InputWidgetSet widgetSet;

        //	// 初始化单个输入组
        //	widgetSet.initWidget(names[i]);
        //	// 设置文本
        //	widgetSet.proptype_ = proptypes[i];
        //	type2inp[proptypes[i]] = i;
        //	connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
        //		[this, widgetSet]() {
        //			emit sg_editFinished();
        //		});
        //	inputWidgets.append(widgetSet);
        //}


        //时温等效

        QStringList names1 = {
            "Theta 0",
            "C1     ",
            "C2     ",
        };
        QVector<MaterialPropType>proptypes1 = {
            MatProp_Theta0,
            MatProp_C1,
            MatProp_C2,
        };
        QStringList names2 = {
            "Theta",
            "E0   ",
        };
        QVector<MaterialPropType>proptypes2 = {
            MatProp_Theta,
            MatProp_E0,
        };
        units_ = QStringList{
            "°C",
            "K",
            "°F",
        };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = "";
        if (sysunitidxs[0] >= 6)
        {
            unit = units_[2];
        }
        else
        {
            unit = units_[sysunitidxs[3]];
        }

        //初始化输入控件
        combobox_.initWidget("转换函数");
        combobox_.proptype_ = MatProp_TransferFunction;
        combobox_.combobox->addItem("WLF");
        combobox_.combobox->addItem("Arrhenius");
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgets2.clear();

        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            type2inp1[proptypes1[i]] = i;
            // 设置文本
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            type2inp2[proptypes2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp2.append(widgetSet);
        }
        inputWidgets2 = inputWidgetsTmp1;
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mTimeDomainPronyProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);
        checkbox_.initWidget("时温等效", false);

        connect(checkbox_.cb, &QCheckBox::stateChanged, this, [this](int i) {
            if (i == 0)
            {
                combobox_.combobox->setDisabled(true);
                for (auto inp : inputWidgetsTmp1)
                {
                    inp.lineeditvalue->setDisabled(true);
                }
                for (auto inp : inputWidgetsTmp2)
                {
                    inp.lineeditvalue->setDisabled(true);
                }
            }
            else
            {
                combobox_.combobox->setDisabled(false);
                for (auto inp : inputWidgetsTmp1)
                {
                    inp.lineeditvalue->setDisabled(false);
                }
                for (auto inp : inputWidgetsTmp2)
                {
                    inp.lineeditvalue->setDisabled(false);
                }
            }
            });

        combobox_.combobox->setDisabled(true);
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setDisabled(true);
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setDisabled(true);
        }
        checkbox_.cb->setChecked(true);
        updatePropUnit();
    }

    void mTimeDomainPronyProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp2.clear();
        inputWidgets2.clear();
        combobox_.clear();
    }

    void mTimeDomainPronyProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mTimeDomainPronyProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit1 = "";
        if (sysunitidxs[0] >= 6)
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][2];
        }
        else
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][sysunitidxs[3]];
        }
        for (auto inp : inputWidgetsTmp1)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
        for (auto inp : inputWidgetsTmp2)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
    }

    void mTimeDomainPronyProp::slot_switchInputs(int i)
    {
        this->removeAll();
        /*for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
            widgetSet.propwid->show();
        }*/
        /*this->addRow(checkbox_.propwid);
        checkbox_.propwid->show();*/
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();

        inputWidgets2.clear();
        if (i == 0)
        {
            inputWidgets2 = inputWidgetsTmp1;
        }
        else
        {
            inputWidgets2 = inputWidgetsTmp2;
        }
        for (int j = 0; j < inputWidgets2.size(); ++j)
        {
            this->addRow(inputWidgets2[j].propwid);
            inputWidgets2[j].propwid->show();
        }
        this->refreshLayout();

        emit sg_editFinished();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mTimeDomainPronyProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets);
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        if (checkbox_.cb->isChecked())
            collectProps(props, inputWidgets2);
        return props;
    }

    void mTimeDomainPronyProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_TransferFunction))
        {
            int idx = prop[MatProp_TransferFunction].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            combobox_.combobox->blockSignals(false);
            if (idx == 0)
            {
                inputWidgets2 = inputWidgetsTmp1;
            }
            else
            {
                inputWidgets2 = inputWidgetsTmp2;
            }
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            bool ischecked = false;
            if (type2inp.contains(it.key()))
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
            }
            if (type2inp1.contains(it.key()))
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp1[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
                ischecked = true;
            }
            if (type2inp2.contains(it.key()))
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp2[type2inp2[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
                ischecked = true;
            }

            checkbox_.cb->setChecked(ischecked);

        }
        updatePropUnit();
    }

    void  mTimeDomainPronyProp::InitFoldSection()
    {

        QVector<QWidget*>wds;
        /*for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }*/
        //this->addRow(checkbox_.propwid);
        this->addRow(combobox_.propwid);
        for (auto& widgetSet : inputWidgets2)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************时温等效***************************************/
    mThermalEquivalenceProp::mThermalEquivalenceProp(QWidget* parent)
        :mMaterialPropBase(MAT_TimeTemperatureEquivalence, "时温等效", parent)
    {
        initInputs();
    }

    mThermalEquivalenceProp::~mThermalEquivalenceProp()
    {
        clearInputs();
    }

    void mThermalEquivalenceProp::initInputs()
    {
        QStringList names1 = {
            "Theta 0",
            "C1     ",
            "C2     ",
        };
        QVector<MaterialPropType>proptypes1 = {
            MatProp_Theta0,
            MatProp_C1,
            MatProp_C2,
        };
        QStringList names2 = {
            "Theta",
            "E0   ",
        };
        QVector<MaterialPropType>proptypes2 = {
            MatProp_Theta,
            MatProp_E0,
        };
        units_ = QStringList{
            "°C",
            "K",
            "°F",
        };

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = "";
        if (sysunitidxs[0] >= 6)
        {
            unit = units_[2];
        }
        else
        {
            unit = units_[sysunitidxs[3]];
        }

        //初始化输入控件
        combobox_.initWidget("转换函数");
        combobox_.proptype_ = MatProp_TransferFunction;
        combobox_.combobox->addItem("WLF");
        combobox_.combobox->addItem("Arrhenius");
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgets.clear();

        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            type2inp1[proptypes1[i]] = i;
            // 设置文本
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            type2inp2[proptypes2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp2.append(widgetSet);
        }
        inputWidgets = inputWidgetsTmp1;
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mThermalEquivalenceProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);

        updatePropUnit();
    }

    void mThermalEquivalenceProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();

        inputWidgets.clear();
        if (i == 0)
        {
            inputWidgets = inputWidgetsTmp1;
        }
        else
        {
            inputWidgets = inputWidgetsTmp2;
        }
        for (int j = 0; j < inputWidgets.size(); ++j)
        {
            this->addRow(inputWidgets[j].propwid);
            inputWidgets[j].propwid->show();
        }
        this->refreshLayout();

        emit sg_editFinished();
    }

    void mThermalEquivalenceProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp2.clear();
        inputWidgets.clear();
        combobox_.clear();
    }

    void mThermalEquivalenceProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mThermalEquivalenceProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit1 = "";
        if (sysunitidxs[0] >= 6)
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][2];
        }
        else
        {
            unit1 = UnitsLibrary[(int)Unit::Temp][sysunitidxs[3]];
        }
        for (auto inp : inputWidgetsTmp1)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
        for (auto inp : inputWidgetsTmp2)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mThermalEquivalenceProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());

        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_Theta0, ratio},
            {MatProp_C1, ratio},
            {MatProp_C2, ratio},
            {MatProp_Theta, ratio},
            {MatProp_E0, ratio},
        };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mThermalEquivalenceProp::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mThermalEquivalenceProp::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mThermalEquivalenceProp::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mThermalEquivalenceProp::slot_unitSystemResponse);
    }

    void mThermalEquivalenceProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mThermalEquivalenceProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_TransferFunction))
        {
            int idx = prop[MatProp_TransferFunction].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            combobox_.combobox->blockSignals(false);
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (type2inp1.contains(it.key()))
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp1[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                }
            }
            if (type2inp2.contains(it.key()))
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp2[type2inp2[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                }
            }
        }
        updatePropUnit();
    }

    void  mThermalEquivalenceProp::InitFoldSection()
    {
        QVector<QWidget*>wds;

        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mThermalEquivalenceProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        this->addRow(combobox_.propwid);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************热传导系数***************************************/

    mHeatConductivityCoefficientProp::mHeatConductivityCoefficientProp(QWidget* parent, bool isCheckable, MxFunctions::MaterialClass classKey)
        :mMaterialPropBase(classKey, "热传导系数", parent), m_bIsCheckable(isCheckable)
    {
        initInputs();

    }

    mHeatConductivityCoefficientProp::~mHeatConductivityCoefficientProp()
    {
        clearInputs();
    }

    void mHeatConductivityCoefficientProp::initInputs()
    {
        QStringList names1 = {
            "Val1",
            "Val2",
            "Val3",
            "Val4",
            "Val5",
            "Val6",
        };
        QVector<MaterialPropType> proptypes = {
            MatProp_HeatConductivityCoefficient_A1,
            MatProp_HeatConductivityCoefficient_A2,
            MatProp_HeatConductivityCoefficient_A3,
            MatProp_HeatConductivityCoefficient_A4,
            MatProp_HeatConductivityCoefficient_A5,
            MatProp_HeatConductivityCoefficient_A6,
        };
        combobox_.initWidget("热传导系数");
        combobox_.proptype_ = MatProp_HeatConductivityCoefficient;
        combobox_.combobox->addItems({ "各向同性热传导系数","正交各向异性热传导系数","各向异性热传导系数" });
        combobox_.combobox->setCurrentIndex(0);
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                slot_typeChanged(index);
                emit sg_editFinished();
            });
        //this, &mHeatConductivityCoefficientProp::slot_typeChanged);

    //初始化输入控件
        inputWidgetsTmp.clear();
        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp.append(widgetSet);
        }
        inputWidgets.append(inputWidgetsTmp[0]);

        checkboxtemp_.initWidget("各向同性热材料-热传导系数");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_HeatConductivityCoefficient);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随传导类型切换（各向同性 1 列/正交 3 列/各向异性 6 列），
        // 初始按当前 inputWidgets 生成，slot_typeChanged 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-热传导系数曲线"),
            QStringLiteral("温度 / °C"),
            { inputWidgetsTmp[0].labelname->text().trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });
        updatePropUnit();
        InitFoldSection();
    }

    void mHeatConductivityCoefficientProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.labelname->text().trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-热传导系数曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mHeatConductivityCoefficientProp::slot_typeChanged(int i)
    {
        inputWidgets.clear();
        if (i == 0)
        {
            inputWidgets.append(inputWidgetsTmp[0]);
        }
        else if (i == 1)
        {
            for (int i = 0; i < 3; ++i)
            {
                inputWidgets.append(inputWidgetsTmp[i]);
            }
        }
        else
        {
            for (int i = 0; i < 6; ++i)
            {
                inputWidgets.append(inputWidgetsTmp[i]);
            }
        }

        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        // 温度相关勾选未解除时，类型切换新增的 Val2/Val3... 行须一并置灰
        bool tempDependent = m_tempCurveRow && m_tempCurveRow->checkBox()->isChecked();
        for (auto inp : inputWidgets)
        {
            inp.setDisableAll(tempDependent);
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        // 类型切换后参数列数变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mHeatConductivityCoefficientProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "热传导系数")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mHeatConductivityCoefficientProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHeatConductivityCoefficientProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHeatConductivityCoefficientProp::slot_connTable);
    }

    void mHeatConductivityCoefficientProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp.clear();
        inputWidgets.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mHeatConductivityCoefficientProp::clearData()
    {
        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mHeatConductivityCoefficientProp::updatePropUnit()
    {
        QString unit1 = "";
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        unit1 = sysunitidxs[3] == 0 ? UnitsLibrary[(int)Unit::ThermalConductivity_C][sysunitidxs[0]] : UnitsLibrary[(int)Unit::ThermalConductivity][sysunitidxs[0]];
        for (auto inp : inputWidgetsTmp)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
    }

    void mHeatConductivityCoefficientProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mHeatConductivityCoefficientProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::ThermalConductivity, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_HeatConductivityCoefficient_A1, ratio},
                {MatProp_HeatConductivityCoefficient_A2, ratio},
                {MatProp_HeatConductivityCoefficient_A3, ratio},
                {MatProp_HeatConductivityCoefficient_A4, ratio},
                {MatProp_HeatConductivityCoefficient_A5, ratio},
                {MatProp_HeatConductivityCoefficient_A6, ratio},
            };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mHeatConductivityCoefficientProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::ThermalConductivity, { 3,0,0,0 }, sysunitidxs);
        if (prop.contains(MatProp_HeatConductivityCoefficient))
        {
            int idx = prop[MatProp_HeatConductivityCoefficient].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            combobox_.combobox->blockSignals(false);
            inputWidgets.clear();
            if (idx == 0)
            {
                inputWidgets.append(inputWidgetsTmp[0]);
            }
            else if (idx == 1)
            {
                for (int i = 0; i < 3; ++i)
                {
                    inputWidgets.append(inputWidgetsTmp[i]);
                }
            }
            else
            {
                for (int i = 0; i < 6; ++i)
                {
                    inputWidgets.append(inputWidgetsTmp[i]);
                }
            }
            //if(fa)
            slot_typeChanged(idx);
        }
        if (prop.contains(MatProp_Curve_HeatConductivityCoefficient))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);

            QVariant data = prop[MatProp_Curve_HeatConductivityCoefficient];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() * ratio));
                }
            }
        }
        updatePropUnit();
    }

    void mHeatConductivityCoefficientProp::InitFoldSection()
    {
        if (m_bIsCheckable)
            this->setCheckable(true);

        combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        this->addRow(combobox_.propwid);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        for (const auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************比热容系数（热容密度）***************************************/

    mHeatCapacityDensityProp::mHeatCapacityDensityProp(QWidget* parent)
        :mMaterialPropBase(MAT_Heat, "比热容系数", parent)
    {
        initInputs();

    }

    mHeatCapacityDensityProp::~mHeatCapacityDensityProp()
    {
        clearInputs();
    }

    void mHeatCapacityDensityProp::initInputs()
    {
        QStringList names2 = {
            "比热容系数"
        };
        //初始化输入控件
        inputWidgets2.clear();
        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_HeatCapacityDensity;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }
        checkboxtemp2_.initWidget("比热容系数");
        checkboxtemp2_.cb->setText("温度相关");
        checkboxtemp2_.setPropType(MatProp_Curve_HeatCapacityDensity);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp2_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-比热容系数曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("比热容系数") },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp2_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets2)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp2_, i != 0);
            emit sg_editFinished();
            });
        updatePropUnit();
        InitFoldSection();
    }

    void mHeatCapacityDensityProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "比热容系数")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);

        emit sg_editFinished();
    }

    void mHeatCapacityDensityProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHeatCapacityDensityProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHeatCapacityDensityProp::slot_connTable);
    }

    void mHeatCapacityDensityProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        checkboxtemp2_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mHeatCapacityDensityProp::clearData()
    {

        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp2_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp2_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mHeatCapacityDensityProp::updatePropUnit()
    {
        QString unit2 = "";
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        unit2 = sysunitidxs[3] == 0 ? UnitsLibrary[(int)Unit::HeatCapacityDensity_C][sysunitidxs[0]] : UnitsLibrary[(int)Unit::HeatCapacityDensity][sysunitidxs[0]];
        //inputWidgets2[0].labelunit->setText(unit2);
        inputWidgets2[0].lineeditvalue->setUnit(unit2);
    }

    void mHeatCapacityDensityProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mHeatCapacityDensityProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::HeatCapacityDensity, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp2_.cb->isChecked())
        {
            collectProps(props, checkboxtemp2_);
        }
        else
        {

            QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_HeatCapacityDensity, ratio} };
            collectProps(props, inputWidgets2, ratios);
        }
        return props;
    }

    void mHeatCapacityDensityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::HeatCapacityDensity, { 3,0,0,0 }, sysunitidxs );
        if (prop.contains(MatProp_Curve_HeatCapacityDensity))
        {
            for (auto inp : inputWidgets2)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp2_, true);

            QVariant data = prop[MatProp_Curve_HeatCapacityDensity];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else if (prop.contains(MatProp_HeatCapacityDensity))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp2_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                QVariant data = prop[MatProp_HeatCapacityDensity];
                if (data.isValid())
                {
                    double v = data.toDouble() * ratio;
                    inputWidgets2[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }
        updatePropUnit();
    }

    void mHeatCapacityDensityProp::InitFoldSection()
    {
        this->setCheckable(true);

        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        this->addRow(inputWidgets2[0].propwid);
        return;
    }

    /***********************************声速***************************************/

    mSoundVelocityProp::mSoundVelocityProp(QWidget* parent)
        :mMaterialPropBase(MAT_SoundVelocity, "声速", parent)
    {
        initInputs();
    }

    mSoundVelocityProp::~mSoundVelocityProp()
    {
        clearInputs();
    }

    void mSoundVelocityProp::initInputs()
    {

        QStringList names = {
            "声速",
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = MatProp_SoundVelocity;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mSoundVelocityProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mSoundVelocityProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mSoundVelocityProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::VelocityOfSound][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mSoundVelocityProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mSoundVelocityProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::VelocityOfSound, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_SoundVelocity, ratio} };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mSoundVelocityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::VelocityOfSound, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_SoundVelocity))
        {
            QVariant data = prop[MatProp_SoundVelocity];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mSoundVelocityProp::InitFoldSection()
    {
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************各向同性***************************************/
    mIsotropyProp::mIsotropyProp(QWidget* parent)
        :mMaterialPropBase(MAT_Isotropy, "各向同性", parent)
    {
        initInputs();
    }

    mIsotropyProp::~mIsotropyProp()
    {
        clearInputs();
    }

    void mIsotropyProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  "
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson
        };

        header_ << "温度";
        for (int i = 0; i < names.size(); ++i)
        {
            header_ << names[i];
        }
        auto* validator = new QDoubleValidator(0.0, 9999.0, 5, this);
        //validator->setNotation(QDoubleValidator::StandardNotation); 禁止科学计数法
        for (int i = 0; i < 2; ++i)
        {
            // 初始化单个输入组
            inputWidgets[i].initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            inputWidgets[i].proptype_ = proptypes[i];
            // 设置文本
            inputWidgets[i].labelname->setText(names[i]);

            inputWidgets[i].lineeditvalue->setValidator(validator);

            connect(inputWidgets[i].lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, i]() {
                    emit sg_editFinished();
                });
            this->addRow(inputWidgets[i].propwid);
        }
        checkboxtemp_.initWidget("各向同性");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Y_B);
        // 温度相关行改用封装控件后，勾选行的 cb/tablebtn 不再入界面：
        // checkboxtemp_ 仅保留为数据载体（proptype_ + cb 的 "curve" 属性），
        // 其行部件不加入折叠块
        checkboxtemp_.propwid->hide();

        // 温度相关行（封装：标签「温度相关」+ 勾选框 + 吸附式曲线弹窗入口按钮）：
        // 自变量列「温度」+ 参数列「杨氏模量 / 泊松比」，行数据双向同步到
        // 封装内勾选框的 "curve" 属性；为兼容 collectProps(singleCheckBox) 的
        // 既有协议，数据变化时镜像一份到 checkboxtemp_.cb
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-各向同性参数曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("杨氏模量 / MPa"), QStringLiteral("泊松比") },
            this);
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        // 勾选/取消勾选：禁用/启用常量输入行（按钮联动置灰由封装内部处理）
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });
        InitFoldSection();
        updatePropUnit();
    }

    void mIsotropyProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "各向同性")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件（勾选状态不变，
        // 入口按钮在未勾选时置灰，此处仅同步数据）
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mIsotropyProp::connTable(bool conn)
    {
        // 温度相关曲线已切换到吸附式弹窗控件（mParamCurveControl），
        // 不再占用 mTableDataWindow 停靠表，保留接口以兼容外部调用
        Q_UNUSED(conn);
    }

    void mIsotropyProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mIsotropyProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mIsotropyProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mIsotropyProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mIsotropyProp::collectProperties()
    {
        //存值转换成默认值
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 2,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_YoungsModulus, ratio} };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mIsotropyProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 2,0,0,0 });
        clearData();
        if (prop.contains(MatProp_YoungsModulus))
        {
            setTempRelatedChecked(false);
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            setTempRelatedChecked(false);
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Curve_Y_B))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            setTempRelatedChecked(true);
            QVariant data = prop[MatProp_Curve_Y_B];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mIsotropyProp::setTempRelatedChecked(bool checked)
    {
        // 勾选状态以界面上的封装勾选框为准（触发输入行禁用/按钮置灰联动），
        // 隐藏的数据载体 checkboxtemp_.cb 同步供 collectProperties 判定
        if (m_tempCurveRow)
            m_tempCurveRow->checkBox()->setChecked(checked);
        checkboxtemp_.cb->setChecked(checked);
    }

    void mIsotropyProp::InitFoldSection()
    {
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);

        }
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        return;
    }

    /***********************************各向异性***************************************/
    mAnisotropyProp::mAnisotropyProp(QWidget* parent)
        :mMaterialPropBase(MAT_Anisotropy, "各向异性", parent)
    {
        initInputs();

    }

    mAnisotropyProp::~mAnisotropyProp()
    {
        clearInputs();
    }

    void mAnisotropyProp::initInputs()
    {
        QStringList names1 = {
            "G11",
            "G12",
            "G13",
            "G22",
            "G23",
            "G33"
        };
        QVector<MaterialPropType>types1 = {
            MatProp_G11,
            MatProp_G12,
            MatProp_G13,
            MatProp_G22,
            MatProp_G23,
            MatProp_G33,
        };
        QStringList names2 = {
            "G11",
            "G12",
            "G13",
            "G14",
            "G15",
            "G16",
            "G22",
            "G23",
            "G24",
            "G25",
            "G26",
            "G33",
            "G34",
            "G35",
            "G36",
            "G44",
            "G45",
            "G46",
            "G55",
            "G56",
            "G66"
        };
        QVector<MaterialPropType>types2 = {
            MatProp_G11,
            MatProp_G12,
            MatProp_G13,
            MatProp_G14,
            MatProp_G15,
            MatProp_G16,
            MatProp_G22,
            MatProp_G23,
            MatProp_G24,
            MatProp_G25,
            MatProp_G26,
            MatProp_G33,
            MatProp_G34,
            MatProp_G35,
            MatProp_G36,
            MatProp_G44,
            MatProp_G45,
            MatProp_G46,
            MatProp_G55,
            MatProp_G56,
            MatProp_G66,
        };
        //初始化输入控件
        /*combobox_.initWidget("类型");
        combobox_.proptype_ = MatProp_Dimensionality;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("二维");
        combobox_.combobox->addItem("三维");*/

        QStringList strs{ "二维", "三维" };
        radioButBox_.initWidget(strs, MatProp_Dimensionality);


        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgets.clear();
        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = types1[i];
            type2inp1[types1[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = types2[i];
            type2inp2[types2[i]] = i;

            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            inputWidgetsTmp2.append(widgetSet);
        }
        inputWidgets = inputWidgetsTmp1;
        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mAnisotropyProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        connect(radioButBox_.group, QOverload<int>::of(&QButtonGroup::buttonClicked), this,
            [this](int id) {
                radioButBox_.currentId = id;
                slot_switchInputs(id);
            });
        radioButBox_.radioVec[0]->setChecked(true);


        checkboxtemp_.initWidget("各向异性");
        checkboxtemp_.setPropType(MatProp_Curve_Anisotropy_Gn);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随单选类型切换（6 列/21 列），初始按当前 inputWidgets 生成，
        // slot_switchInputs 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-各向异性参数曲线"),
            QStringLiteral("温度 / °C"),
            { names1[0], names1[1], names1[2], names1[3], names1[4], names1[5] },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mAnisotropyProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.name_.trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-各向异性参数曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mAnisotropyProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "各向异性")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mAnisotropyProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mAnisotropyProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mAnisotropyProp::slot_connTable);
    }

    void mAnisotropyProp::slot_switchInputs(int i)
    {

        this->removeAll();
        //fa->addRow(combobox_.propwid);
        //combobox_.propwid->show();
        this->addRow(radioButBox_.propwid);
        radioButBox_.propwid->show();

        if (i == 0)
        {
            inputWidgets = inputWidgetsTmp1;
        }
        else
        {
            inputWidgets = inputWidgetsTmp2;
        }
        for (int j = 0; j < inputWidgets.size(); ++j)
        {
            this->addRow(inputWidgets[j].propwid);
            inputWidgets[j].propwid->show();
        }

        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        // 类型切换后参数列数变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();

        this->refreshLayout();

        for (auto inp : inputWidgets)
        {
            inp.setDisableAll(checkboxtemp_.cb->isChecked());
        }
        /*	if (checkboxtemp_.cb->isChecked())
            {
                checkboxtemp_.tablebtn->click();
            }*/

            //emit sg_editFinished();
    }

    void mAnisotropyProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp2.clear();
        inputWidgets.clear();
        //combobox_.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mAnisotropyProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        for (auto inp : inputWidgetsTmp1)
        {
            //inp.labelunit->setText(stressunit);
            inp.lineeditvalue->setUnit(stressunit);

        }
        for (auto inp : inputWidgetsTmp2)
        {
            //inp.labelunit->setText(stressunit);
            inp.lineeditvalue->setUnit(stressunit);
        }
    }

    void mAnisotropyProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mAnisotropyProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mAnisotropyProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        //collectProps(props, combobox_.proptype_,combobox_.combobox->currentIndex());
        collectProps(props, radioButBox_.proptype_, radioButBox_.currentId);

        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_G11, ratio},
                {MatProp_G12, ratio},
                {MatProp_G13, ratio},
                {MatProp_G22, ratio},
                {MatProp_G23, ratio},
                {MatProp_G33, ratio},//
                {MatProp_G14, ratio},
                {MatProp_G15, ratio},
                {MatProp_G16, ratio},
                {MatProp_G24, ratio},
                {MatProp_G25, ratio},
                {MatProp_G26, ratio},
                {MatProp_G34, ratio},
                {MatProp_G35, ratio},
                {MatProp_G36, ratio},
                {MatProp_G44, ratio},
                {MatProp_G45, ratio},
                {MatProp_G46, ratio},
                {MatProp_G55, ratio},
                {MatProp_G56, ratio},
                {MatProp_G66, ratio},
            };
            collectProps(props, inputWidgets, ratios);

        }
        return props;
    }

    void mAnisotropyProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        //维度回退：模型缺 Dimensionality 键时按当前 UI 维度处理，避免三维数据被按二维映射清空
        int d = radioButBox_.currentId;
        if (prop.contains(MatProp_Dimensionality))
            d = prop[MatProp_Dimensionality].toInt();

        radioButBox_.group->blockSignals(true);
        for (auto it : radioButBox_.radioVec)
        {
            it->setChecked(false);
        }
        radioButBox_.group->blockSignals(false);

        radioButBox_.currentId = d;
        radioButBox_.radioVec.at(d)->setChecked(true);
        slot_switchInputs(d);

        inputWidgets = d == 0 ? inputWidgetsTmp1 : inputWidgetsTmp2;
        if (prop.contains(MatProp_Curve_Anisotropy_Gn))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);

            QVariant data = prop[MatProp_Curve_Anisotropy_Gn];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QHash<MaterialPropType, int>type2inp = type2inp1;
            if (d == 1)
                type2inp = type2inp2;
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                }
            }
        }
        updatePropUnit();
    }

    void  mAnisotropyProp::InitFoldSection()
    {
        this->addRow(radioButBox_.propwid);
        for (const auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        return;
    }

    /***********************************正交各向异性***************************************/
    mOrthotropyProp::mOrthotropyProp(QWidget* parent)
        :mMaterialPropBase(MAT_Orthotropy, "正交各向异性", parent)
    {
        initInputs();

    }

    mOrthotropyProp::~mOrthotropyProp()
    {
        clearInputs();
    }

    void mOrthotropyProp::initInputs()
    {
        QStringList names1 = {
            "Ex  ",
            "Ey  ",
            "NUxy",
            "Gxy ",
            "Gxz ",
            "Gyz "
        };
        QVector<MaterialPropType>types1 = {
            MatProp_Ex,
            MatProp_Ey,
            MatProp_NUxy,
            MatProp_Gxy,
            MatProp_Gxz,
            MatProp_Gyz,
        };
        QStringList names2 = {
            "Ex  ",
            "Ey  ",
            "Ez  ",
            "NUyz",
            "NUxz",
            "NUxy",
            "Gyz ",
            "Gxz ",
            "Gxy "
        };
        QVector<MaterialPropType>types2 = {
            MatProp_Ex,
            MatProp_Ey,
            MatProp_Ez,
            MatProp_NUyz,
            MatProp_NUxz,
            MatProp_NUxy,
            MatProp_Gyz,
            MatProp_Gxz,
            MatProp_Gxy,
        };
        QStringList names3 = {
            "D1111",
            "D1122",
            "D2222",
            "D1133",
            "D2233",
            "D3333",
            "D1212",
            "D1313",
            "D2323"
        };
        QVector<MaterialPropType>types3 = {
            MatProp_D1111,
            MatProp_D1122,
            MatProp_D2222,
            MatProp_D1133,
            MatProp_D2233,
            MatProp_D3333,
            MatProp_D1212,
            MatProp_D1313,
            MatProp_D2323,
        };
        //初始化输入控件
        /*combobox_.initWidget("类型");
        combobox_.combobox->clear();
        combobox_.combobox->addItem("二维");
        combobox_.combobox->addItem("三维-工程常数");
        combobox_.combobox->addItem("三维-正交");
        combobox_.proptype_ = MatProp_Dimensionality;*/

        QStringList radioButStrs{ "二维", "三维-工程常数", "三维正交" };
        radioButBox_.initWidget(radioButStrs, MatProp_Dimensionality);

        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();
        inputWidgetsTmp3.clear();
        inputWidgets.clear();
        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;
            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = types1[i];
            type2inp1[types1[i]] = i;
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;
            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = types2[i];
            type2inp2[types2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            inputWidgetsTmp2.append(widgetSet);
        }
        for (int i = 0; i < names3.size(); ++i)
        {
            InputWidgetSet widgetSet;
            // 初始化单个输入组
            widgetSet.initWidget(names3[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names3[i], CalcMode::Standard));
            widgetSet.proptype_ = types3[i];
            type2inp3[types3[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            // 设置文本
            widgetSet.labelname->setText(names3[i]);
            inputWidgetsTmp3.append(widgetSet);
        }
        inputWidgets = inputWidgetsTmp1;

        //把combobox换为radioButBox
        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mOrthotropyProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        connect(radioButBox_.group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int id) {
                radioButBox_.currentId = id;
                slot_switchInputs(id);
                emit hideFold(id);
            });
        radioButBox_.radioVec[0]->setChecked(true);

        checkboxtemp_.initWidget("正交各向异性");
        checkboxtemp_.setPropType(MatProp_Curve_Orthotropy);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随单选类型切换（6/9/21 列），初始按当前 inputWidgets 生成，
        // slot_switchInputs 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-正交各向异性参数曲线"),
            QStringLiteral("温度 / °C"),
            { names1[0].trimmed(), names1[1].trimmed(), names1[2].trimmed(),
              names1[3].trimmed(), names1[4].trimmed(), names1[5].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        InitFoldSection();
        updatePropUnit();
    }

    void mOrthotropyProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.name_.trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-正交各向异性参数曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mOrthotropyProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "正交各向异性")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mOrthotropyProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mOrthotropyProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mOrthotropyProp::slot_connTable);
    }

    void mOrthotropyProp::slot_switchInputs(int i)
    {
        this->blockSignals(true);
        this->removeAll();
        this->blockSignals(false);

        this->addRow(radioButBox_.propwid);
        radioButBox_.propwid->show();

        inputWidgets.clear();
        if (i == 0)
        {
            inputWidgets = inputWidgetsTmp1;
        }
        else if (i == 1)
        {
            inputWidgets = inputWidgetsTmp2;
        }
        else
        {
            inputWidgets = inputWidgetsTmp3;
        }
        for (int j = 0; j < inputWidgets.size(); ++j)
        {
            this->addRow(inputWidgets[j].propwid);
            inputWidgets[j].propwid->show();
        }

        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        // 类型切换后参数列数变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();

        this->refreshLayout();

        for (auto inp : inputWidgets)
        {
            inp.setDisableAll(checkboxtemp_.cb->isChecked());
        }

        //重渲染期间（s_unitSyncing）值处于清空/回填中间态，不发保存信号；
        //用户点击 radio 切维度时（非渲染态）正常发出
        if (!s_unitSyncing)
            emit sg_editFinished();
    }

    void mOrthotropyProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp3)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mOrthotropyProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QVector<int> nullParams = { MatProp_NUxy, MatProp_NUxz, MatProp_NUyz };
        QString nullunit = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        for (auto inp : inputWidgetsTmp1)
        {
            //inp.labelunit->setText(stressunit);
            if (nullParams.contains(inp.proptype_))
                inp.lineeditvalue->setUnit(nullunit);
            else
                inp.lineeditvalue->setUnit(stressunit);
        }
        for (auto inp : inputWidgetsTmp2)
        {
            //inp.labelunit->setText(stressunit);
            if (nullParams.contains(inp.proptype_))
                inp.lineeditvalue->setUnit(nullunit);
            else
                inp.lineeditvalue->setUnit(stressunit);
        }
        for (auto inp : inputWidgetsTmp3)
        {
            //inp.labelunit->setText(stressunit);
            inp.lineeditvalue->setUnit(stressunit);
        }
    }

    void mOrthotropyProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;
            QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
            QVector<int> nullParams = { MatProp_NUxy, MatProp_NUxz, MatProp_NUyz };
            double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
            QHash<MaterialPropType, int> type2inp = type2inp1;
            int d = radioButBox_.currentId;            //单位切换不改维度，按当前视图映射即可
            if (d == 1) type2inp = type2inp2;
            else if (d == 2) type2inp = type2inp3;
            auto prop = m_matData->getAllProps()[getPropertyClass()];
            for (auto it = prop.begin(); it != prop.end(); ++it)
            {
                if (!type2inp.contains(it.key())) continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    if (nullParams.contains(it.key()))
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(mNumberToString(data.toDouble()));
                    else
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(mNumberToString(data.toDouble() / ratio));
                }
            }
            updatePropUnit();
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mOrthotropyProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgetsTmp3)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mOrthotropyProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        //改成raidobutton
        //collectProps(props, combobox_.proptype_,combobox_.combobox->currentIndex());
        int id = radioButBox_.currentId;
        collectProps(props, radioButBox_.proptype_, radioButBox_.currentId);

        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_Ex, ratio},
                {MatProp_Ey, ratio},
                {MatProp_Ez, ratio},
                {MatProp_NUxy, 1.0},
                {MatProp_Gxy, ratio},
                {MatProp_Gxz, ratio},
                {MatProp_Gyz, ratio},//
                {MatProp_NUyz, 1.0},
                {MatProp_NUxz, 1.0},//
                {MatProp_D1111, ratio},
                {MatProp_D1122, ratio},
                {MatProp_D2222, ratio},
                {MatProp_D1133, ratio},
                {MatProp_D2233, ratio},
                {MatProp_D3333, ratio},
                {MatProp_D1212, ratio},
                {MatProp_D1313, ratio},
                {MatProp_D2323, ratio},
            };
            collectProps(props, inputWidgets, ratios);

        }
        return props;
    }

    void mOrthotropyProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QVector<int> nullParams = { MatProp_NUxy, MatProp_NUxz, MatProp_NUyz };
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        //维度回退：模型缺 Dimensionality 键时按当前 UI 维度处理，避免三维数据被按二维映射清空
        int d = radioButBox_.currentId;
        if (prop.contains(MatProp_Dimensionality))
            d = prop[MatProp_Dimensionality].toInt();

        radioButBox_.group->blockSignals(true);
        for (auto it : radioButBox_.radioVec)
        {
            it->setChecked(false);
        }
        radioButBox_.group->blockSignals(false);

        radioButBox_.currentId = d;
        radioButBox_.radioVec.at(d)->setChecked(true);
        slot_switchInputs(d);
        emit hideFold(d);

        inputWidgets = d == 0 ? inputWidgetsTmp1 : (d == 1 ? inputWidgetsTmp2 : inputWidgetsTmp3);

        if (prop.contains(MatProp_Curve_Orthotropy))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);

            QVariant data = prop[MatProp_Curve_Orthotropy];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QHash<MaterialPropType, int>type2inp = type2inp1;
            if (d == 1)
                type2inp = type2inp2;
            else if (d == 2)
                type2inp = type2inp3;
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    if (nullParams.contains(it.key()))
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                    else
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                }
            }
        }
        updatePropUnit();
    }

    void mOrthotropyProp::InitFoldSection()
    {
        this->addRow(radioButBox_.propwid);
        for (const auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        return;
    }

    /***********************************各向同性热材料***************************************/
    mIsotropicThermalProp::mIsotropicThermalProp(QWidget* parent)
        :mMaterialPropBase(MAT_IsotropicThermalMaterial, "各向同性热材料", parent)
    {
        initInputs();

    }

    mIsotropicThermalProp::~mIsotropicThermalProp()
    {
        clearInputs();
    }

    void mIsotropicThermalProp::initInputs()
    {
        QStringList names1 = {
            "Val1",
            "Val2",
            "Val3",
            "Val4",
            "Val5",
            "Val6",
        };
        QVector<MaterialPropType> proptypes = {
            MatProp_HeatConductivityCoefficient_A1,
            MatProp_HeatConductivityCoefficient_A2,
            MatProp_HeatConductivityCoefficient_A3,
            MatProp_HeatConductivityCoefficient_A4,
            MatProp_HeatConductivityCoefficient_A5,
            MatProp_HeatConductivityCoefficient_A6,
        };

        QStringList names2 = {
            "比热容系数"
        };

        combobox_.initWidget("热传导系数");
        combobox_.proptype_ = MatProp_HeatConductivityCoefficient;
        combobox_.combobox->addItems({ "各向同性热传导系数","正交各向异性热传导系数","各向异性热传导系数" });
        combobox_.combobox->setCurrentIndex(0);
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mIsotropicThermalProp::slot_typeChanged);


        //初始化输入控件
        inputWidgets.clear();
        inputWidgets2.clear();
        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            // 设置文本
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp.append(widgetSet);
        }
        inputWidgets.append(inputWidgetsTmp[0]);
        for (int i = 0; i < names2.size(); ++i)
        {
            InputWithCheckBox widgetSet;

            widgetSet.initWidget();
            widgetSet.proptype_ = MatProp_HeatCapacityDensity;
            widgetSet.checkbox->setChecked(false);
            widgetSet.lineeditvalue->setDisabled(true);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            // 连接checkbox的勾选信号
            connect(widgetSet.checkbox, &QCheckBox::stateChanged,
                [=](int state) {
                    bool isChecked = !(state == Qt::Checked);
                    widgetSet.lineeditvalue->setDisabled(isChecked);
                    if (m_tempCurveRow2)
                        m_tempCurveRow2->checkBox()->setDisabled(isChecked);
                    checkboxtemp2_.cb->setDisabled(isChecked);
                });

            // 设置文本
            widgetSet.checkbox->setText(names2[i]);
            inputWidgets2.append(widgetSet);
        }
        checkboxtemp_.initWidget("各向同性热材料-热传导系数");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_HeatConductivityCoefficient);
        checkboxtemp2_.initWidget("各向同性热材料-比热容系数");
        checkboxtemp2_.cb->setText("温度相关");
        checkboxtemp2_.setPropType(MatProp_Curve_HeatCapacityDensity);
        // 两个温度相关行均改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();
        checkboxtemp2_.propwid->hide();

        // 热传导系数曲线参数列随传导类型切换（1/3/6 列），
        // 初始按当前 inputWidgets 生成，slot_typeChanged 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-热传导系数曲线"),
            QStringLiteral("温度 / °C"),
            { inputWidgets[0].labelname->text().trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        // 比热容系数曲线行：勾选框可用状态跟随「比热容系数」常量输入行的勾选
        m_tempCurveRow2 = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-比热容系数曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("比热容系数") },
            this);
        m_tempCurveRow2->setHeaderHasUnit(false);
        m_tempCurveRow2->checkBox()->setDisabled(true);
        connect(m_tempCurveRow2, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp2_.cb->setProperty("curve", m_tempCurveRow2->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow2->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets2)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp2_, i != 0);
            emit sg_editFinished();
            });

        widget1 = new QWidget;
        widget2 = new QWidget;
        group1 = new ClickableGroupBox("热传导系数");
        group2 = new ClickableGroupBox("比热容系数");
        layout1 = new QVBoxLayout(group1);
        layout2 = new QVBoxLayout(group2);

        layout1->addWidget(combobox_.propwid);
        layout1->addWidget(m_tempCurveRow);
        layout1->addWidget(inputWidgets[0].propwid);
        layout2->addWidget(m_tempCurveRow2);
        layout2->addWidget(inputWidgets2[0].propwid);

        updatePropUnit();
    }

    void mIsotropicThermalProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.labelname->text().trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-热传导系数曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mIsotropicThermalProp::slot_typeChanged(int i)
    {
        inputWidgets.clear();
        if (i == 0)
        {
            inputWidgets.append(inputWidgetsTmp[0]);
        }
        else if (i == 1)
        {
            for (int i = 0; i < 3; ++i)
            {
                inputWidgets.append(inputWidgetsTmp[i]);
            }
        }
        else
        {
            for (int i = 0; i < 6; ++i)
            {
                inputWidgets.append(inputWidgetsTmp[i]);
            }
        }

        this->removeAll();
        while (layout1->count() > 0)
        {
            QLayoutItem* item = layout1->itemAt(0);
            if (item && item->widget())
            {
                auto wid = item->widget();
                wid->setParent(nullptr);
                layout1->removeWidget(wid);
                wid->hide();
            }
        }
        layout1->addWidget(combobox_.propwid);
        combobox_.propwid->show();
        if (m_tempCurveRow)
        {
            layout1->addWidget(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        // 温度相关勾选未解除时，类型切换新增的 Val2/Val3... 行须一并置灰
        bool tempDependent = m_tempCurveRow && m_tempCurveRow->checkBox()->isChecked();
        for (auto inp : inputWidgets)
        {
            inp.setDisableAll(tempDependent);
            layout1->addWidget(inp.propwid);
            inp.propwid->show();
        }
        // 类型切换后参数列数变化：先刷新曲线表头，再触发重排
        refreshCurveHeaders();
        this->addRow(group1);
        group1->show();
        this->addRow(group2);
        group2->show();
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mIsotropicThermalProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "各向同性热材料-热传导系数" && title != "各向同性热材料-比热容系数")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        if (title == "各向同性热材料-热传导系数")
        {
            checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
            if (m_tempCurveRow)
                m_tempCurveRow->setCurveData(curves);
        }
        else
        {
            checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves));
            if (m_tempCurveRow2)
                m_tempCurveRow2->setCurveData(curves);
        }

        emit sg_editFinished();
    }

    void mIsotropicThermalProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mIsotropicThermalProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mIsotropicThermalProp::slot_connTable);
    }

    void mIsotropicThermalProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        checkboxtemp_.clear();
        checkboxtemp2_.clear();
        combobox_.clear();
        // m_tempCurveRow/m_tempCurveRow2 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
        m_tempCurveRow2 = nullptr;
        if (group1)
        {
            delete group1;
            group1 = nullptr;
        }
        if (group2)
        {
            delete group2;
            group2 = nullptr;
        }
        if (layout1)
        {
            delete layout1;
            layout1 = nullptr;
        }
        if (layout2)
        {
            delete layout2;
            layout2 = nullptr;
        }
        if (widget1)
        {
            delete widget1;
            widget1 = nullptr;
        }
        if (widget2)
        {
            delete widget2;
            widget2 = nullptr;
        }
    }

    void mIsotropicThermalProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        checkboxtemp2_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
        if (m_tempCurveRow2)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow2->syncCheckedToCarrier(checkboxtemp2_, false);
            m_tempCurveRow2->clearCurve();
        }
    }

    void mIsotropicThermalProp::updatePropUnit()
    {
        QString unit1 = "";
        QString unit2 = "";
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        unit1 = sysunitidxs[3] == 0 ? UnitsLibrary[(int)Unit::ThermalConductivity_C][sysunitidxs[0]] : UnitsLibrary[(int)Unit::ThermalConductivity][sysunitidxs[0]];
        unit2 = sysunitidxs[3] == 0 ? UnitsLibrary[(int)Unit::HeatCapacityDensity_C][sysunitidxs[0]] : UnitsLibrary[(int)Unit::HeatCapacityDensity][sysunitidxs[0]];
        for (auto inp : inputWidgetsTmp)
        {
            //inp.labelunit->setText(unit1);
            inp.lineeditvalue->setUnit(unit1);
        }
        //inputWidgets2[0].labelunit->setText(unit2);
        inputWidgets2[0].lineeditvalue->setUnit(unit2);
    }

    void mIsotropicThermalProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mIsotropicThermalProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::ThermalConductivity_C, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_HeatConductivityCoefficient_A1, ratio},
                { MatProp_HeatConductivityCoefficient_A2, ratio },
                { MatProp_HeatConductivityCoefficient_A3, ratio },
                { MatProp_HeatConductivityCoefficient_A4, ratio },
                { MatProp_HeatConductivityCoefficient_A5, ratio },
                { MatProp_HeatConductivityCoefficient_A6, ratio },
            };
            collectProps(props, inputWidgets, ratios);

        }
        if (checkboxtemp2_.cb->isChecked())
        {
            collectProps(props, checkboxtemp2_);
        }
        else if (inputWidgets2[0].checkbox->isChecked())
        {
            collectProps(props, inputWidgets2);
        }
        return props;
    }

    void mIsotropicThermalProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::ThermalConductivity_C, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_HeatConductivityCoefficient))
        {
            int idx = prop[MatProp_HeatConductivityCoefficient].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(idx);
            combobox_.combobox->blockSignals(false);
            inputWidgets.clear();
            if (idx == 0)
            {
                inputWidgets.append(inputWidgetsTmp[0]);
            }
            else if (idx == 1)
            {
                for (int i = 0; i < 3; ++i)
                {
                    inputWidgets.append(inputWidgetsTmp[i]);
                }
            }
            else
            {
                for (int i = 0; i < 6; ++i)
                {
                    inputWidgets.append(inputWidgetsTmp[i]);
                }
            }
        }
        if (prop.contains(MatProp_Curve_HeatConductivityCoefficient))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_HeatConductivityCoefficient];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else if (prop.contains(MatProp_HeatConductivityCoefficient))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgetsTmp[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                }
            }
        }
        inputWidgets2[0].checkbox->setChecked(false);
        if (prop.contains(MatProp_Curve_HeatCapacityDensity))
        {
            inputWidgets2[0].checkbox->setChecked(true);
            for (auto inp : inputWidgets2)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow2)
                m_tempCurveRow2->syncCheckedTo(checkboxtemp2_, true);
            QVariant data = prop[MatProp_Curve_HeatCapacityDensity];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow2)
                    m_tempCurveRow2->setCurveData(curves.curves);
            }
        }
        else if (prop.contains(MatProp_HeatCapacityDensity))
        {
            inputWidgets2[0].checkbox->setChecked(true);
            if (m_tempCurveRow2)
                m_tempCurveRow2->syncCheckedTo(checkboxtemp2_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgets2[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
            }
        }
        updatePropUnit();
    }

    void mIsotropicThermalProp::InitFoldSection()
    {

        QVector<QWidget*>wds;
        this->addRow(group1);
        this->addRow(group2);
        return;
    }

    /***********************************各向同性硬化***************************************/
    //新材料布局 ****************塑性***常规塑性******************************
    mIsotropicHardeningProp::mIsotropicHardeningProp(QWidget* parent)
        :mMaterialPropBase(MAT_IsotropicHardening, "各向同性硬化", parent)
    {
        initInputs();
    }

    mIsotropicHardeningProp::~mIsotropicHardeningProp()
    {
        clearInputs();
    }

    void mIsotropicHardeningProp::initInputs()
    {
    }


    void mIsotropicHardeningProp::clearInputs()
    {
    }

    void mIsotropicHardeningProp::clearData()
    {
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mIsotropicHardeningProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        return props;
    }

    void mIsotropicHardeningProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        updatePropUnit();
    }

    void mIsotropicHardeningProp::InitFoldSection()
    {
        QVector<QWidget*>wds;
        return;
    }

    QVector<foldDetail*> mIsotropicHardeningProp::
        getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    /***********************************随动硬化***************************************/

    mKinematicHardeningProp::mKinematicHardeningProp(QWidget* parent)
        :mMaterialPropBase(MAT_KinematicHardening, "随动硬化", parent)
    {
        initInputs();
    }

    mKinematicHardeningProp::~mKinematicHardeningProp()
    {
        clearInputs();
    }

    void mKinematicHardeningProp::initInputs()
    {
    }

    void mKinematicHardeningProp::clearInputs()
    {
    }

    void mKinematicHardeningProp::clearData()
    {
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mKinematicHardeningProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        return props;
    }

    void mKinematicHardeningProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        updatePropUnit();
    }

    void mKinematicHardeningProp::InitFoldSection()
    {
        QVector<QWidget*>wds;
        return;
    }

    QVector<foldDetail*> mKinematicHardeningProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    /***********************************组合硬化***************************************/

    mCompressiveHardeningProp::mCompressiveHardeningProp(QWidget* parent)
        :mMaterialPropBase(MAT_CompressiveHardening, "组合硬化", parent)
    {
        initInputs();
    }

    mCompressiveHardeningProp::~mCompressiveHardeningProp()
    {
        clearInputs();
    }

    void mCompressiveHardeningProp::initInputs()
    {
    }

    void mCompressiveHardeningProp::clearInputs()
    {
    }

    void mCompressiveHardeningProp::clearData()
    {
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mCompressiveHardeningProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        return props;
    }

    void mCompressiveHardeningProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        updatePropUnit();
    }

    void mCompressiveHardeningProp::InitFoldSection()
    {
        QVector<QWidget*>wds;
        return;
    }

    QVector<foldDetail*> mCompressiveHardeningProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    /***********************************NeoHookean***************************************/
    mNeoHookeanProp::mNeoHookeanProp(QWidget* parent)
        :mMaterialPropBase(MAT_Neohookean, "Neo-Hookean", parent)
    {
        initInputs();
    }

    mNeoHookeanProp::~mNeoHookeanProp()
    {
        clearInputs();
    }

    void mNeoHookeanProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  "
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson
        };
        QStringList header;
        header << "温度";
        for (int i = 0; i < names.size(); ++i)
        {
            header << names[i];
        }
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            // 设置文本
            widgetSet.labelname->setText(names[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        checkboxtemp_.initWidget("Neo-Hookean");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Y_B);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-Neo-Hookean参数曲线"),
            QStringLiteral("温度 / °C"),
            { names[0].trimmed(), names[1].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mNeoHookeanProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mNeoHookeanProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mNeoHookeanProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mNeoHookeanProp::slot_connTable);
    }

    void mNeoHookeanProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mNeoHookeanProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mNeoHookeanProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mNeoHookeanProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mNeoHookeanProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_YoungsModulus, ratio} };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mNeoHookeanProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_YoungsModulus))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Curve_Y_B))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Y_B];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mNeoHookeanProp::InitFoldSection()
    {
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************NeoHookean2***************************************/
    mNeoHookean2Prop::mNeoHookean2Prop(QWidget* parent)
        :mMaterialPropBase(MAT_Neohookean2, "Neo-Hookean2", parent)
    {
        initInputs();
    }

    mNeoHookean2Prop::~mNeoHookean2Prop()
    {
        clearInputs();
    }

    void mNeoHookean2Prop::initInputs()
    {
        QStringList names1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList names2 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        QStringList names3 =
        {
            "C10",
            "D1"
        };

        QVector<MxFunctions::MaterialPropType>proptypes3 = {
            MatProp_C10,
            MatProp_D1
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("neohookean2参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mNeoHookean2Prop::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });


        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }
        //inputWidgets = inputWidgets1;

        inputWidgets3.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names3[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names3[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes3[i];
            // 设置文本
            widgetSet.labelname->setText(names3[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets3.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mNeoHookean2Prop::slot_paraswitchInputs(int i)
    {
        //emit sg_editFinished();
        if (!this) return;
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        comboboxType_.propwid->show();
        if (i == 0)
        {
            for (int j = 0; j < inputWidgets3.size(); ++j)
            {
                this->addRow(inputWidgets3[j].propwid);
                inputWidgets3[j].propwid->show();
            }
        }
        else
        {
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();
        }
        this->refreshLayout();

    }

    void mNeoHookean2Prop::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        comboboxType_.propwid->show();

        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);
        labelbtn_.propwid->show();
        combobox_.propwid->show();*/

        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mNeoHookean2Prop::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        //labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }

        emit sg_editFinished();
    }

    void mNeoHookean2Prop::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mNeoHookean2Prop::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mNeoHookean2Prop::slot_connTable);
    }

    void mNeoHookean2Prop::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgets3)
        {
            widgetSet.clear();
        }
        inputWidgets3.clear();
        comboboxType_.clear();
        //combobox_.clear();
        //labelbtn_.clear();
    }

    void mNeoHookean2Prop::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets3)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mNeoHookean2Prop::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        //inputWidgets3[0].labelunit->setText(stressunit);
        //inputWidgets3[1].labelunit->setText(stressreciprocalunit);
        inputWidgets3[0].lineeditvalue->setUnit(stressunit);
        inputWidgets3[1].lineeditvalue->setUnit(stressreciprocalunit);
    }

    void mNeoHookean2Prop::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mNeoHookean2Prop::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        collectProps(props, inputWidgets);
        /* collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
       collectProps(props, labelbtn_);*/

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios3{ {MatProp_C10, ratio1}, {MatProp_D1, ratio2} };
        collectProps(props, inputWidgets3, ratios3);


        return props;
    }

    void mNeoHookean2Prop::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        int d = 0;
        // 1. 恢复"分类"下拉框：参数模式(0) 还是 试验数据模式(1)
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();

            comboboxType_.combobox->setCurrentIndex(d);
            slot_paraswitchInputs(d);
        }
        // 2. 恢复"试验数据类型"下拉框：单轴(0) 还是 体积(1)
        if (prop.contains(MatProp_TestDataType))
        {
            int idx = prop[MatProp_TestDataType].toInt();

            slot_switchInputs(idx);
        }
        if (prop.contains(MatProp_Curve_Testdatainput))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }

        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }

        }
        if (prop.contains(MatProp_C10))
        {
            QVariant data = prop[MatProp_C10];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D1))
        {
            QVariant data = prop[MatProp_D1];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }

        // 1. 恢复"分类"下拉框：参数模式(0) 还是 试验数据模式(1)
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();

            comboboxType_.combobox->setCurrentIndex(d);
            if (this)
                slot_paraswitchInputs(d);
        }
        updatePropUnit();
    }

    void mNeoHookean2Prop::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        //int i = combobox_.combobox->currentIndex();
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            for (auto& widgetSet : inputWidgets3)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            //  this->addRow(combobox_.propwid);
              //this->addRow(labelbtn_.propwid);
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }
        return;
    }

    /***********************************Mooney-Rivlin***************************************/
    mMooneyRivlinProp::mMooneyRivlinProp(QWidget* parent)
        :mMaterialPropBase(MAT_MooneyRivlin, "Mooney-Rivlin", parent)
    {
        initInputs();
    }

    mMooneyRivlinProp::~mMooneyRivlinProp()
    {
        clearInputs();
    }

    void mMooneyRivlinProp::initInputs()
    {
        QStringList names1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList names2 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        QStringList names3 =
        {
            "C10",
            "C01",
            "D1"
        };

        QVector<MxFunctions::MaterialPropType>proptypes3 = {
            MatProp_C10,
            MatProp_C01,
            MatProp_D1
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Mooney-Rivlin参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mMooneyRivlinProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        /*combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");*/
        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });


        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }

        //inputWidgets = inputWidgets1;

        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mMooneyRivlinProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        //labelbtn_.initWidget("Mooney-Rivlin");
        //labelbtn_.lablename->setText("类型曲线");
        //labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        //labelbtn_.tablebtn->setDisabled(false);
        //connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int i) {
        //    QString tabletitle = "Mooney-Rivlin";
        //    QStringList header;
        //    for (auto inp : inputWidgets)
        //    {
        //        header << inp.name_;
        //    }
        //    QVariant value = labelbtn_.tablebtn->property("curve");
        //    //编辑表格
        //    //mMaterialPropBase::tableEdit(true, tabletitle, value, header, false);

        //    //打开表格
        //    //mDockLayoutManager::getInstance()->activateDock("Table");

        //    QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

        //    mTableDataWindow::getInstance()->resetInitialTable();
        //    mTableDataWindow::getInstance()->setUseCurve(false);
        //    mTableDataWindow::getInstance()->setTableTitle(tabletitle);
        //    mTableDataWindow::getInstance()->setTableData(curves);
        //    mTableDataWindow::getInstance()->setHorizontalHeaders(header);
        //    mDockLayoutManager::getInstance()->activateDock("Table");
        //    });

        inputWidgets3.clear();
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names3[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names3[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes3[i];
            // 设置文本
            widgetSet.labelname->setText(names3[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets3.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mMooneyRivlinProp::slot_paraswitchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        comboboxType_.propwid->show();
        if (i == 0)
        {
            for (int j = 0; j < inputWidgets3.size(); ++j)
            {
                this->addRow(inputWidgets3[j].propwid);
                inputWidgets3[j].propwid->show();
            }
        }
        else
        {
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();*/

            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mMooneyRivlinProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);*/
        comboboxType_.propwid->show();
        /*labelbtn_.propwid->show();
        combobox_.propwid->show();*/

        this->addRow(checkboxNeoPress_.propwid);
        this->addRow(checkboxNeoV_.propwid);
        checkboxNeoPress_.propwid->show();
        checkboxNeoV_.propwid->show();

        this->refreshLayout();


        emit sg_editFinished();
    }

    void mMooneyRivlinProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        //labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }

        emit sg_editFinished();
    }

    void mMooneyRivlinProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mMooneyRivlinProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mMooneyRivlinProp::slot_connTable);
    }

    void mMooneyRivlinProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgets3)
        {
            widgetSet.clear();
        }
        inputWidgets3.clear();
        comboboxType_.clear();
        /* combobox_.clear();
         labelbtn_.clear();*/
    }

    void mMooneyRivlinProp::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets3)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mMooneyRivlinProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        //inputWidgets3[0].labelunit->setText(stressunit);
        //inputWidgets3[1].labelunit->setText(stressunit);
        //inputWidgets3[2].labelunit->setText(stressreciprocalunit);
        inputWidgets3[0].lineeditvalue->setUnit(stressunit);
        inputWidgets3[1].lineeditvalue->setUnit(stressunit);
        inputWidgets3[2].lineeditvalue->setUnit(stressreciprocalunit);
    }

    void mMooneyRivlinProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mMooneyRivlinProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        /*collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, labelbtn_);*/
        collectProps(props, inputWidgets);

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios3{ {MatProp_C10, ratio1}, {MatProp_C01, ratio1}, {MatProp_D1, ratio2} };
        collectProps(props, inputWidgets3, ratios3);

        return props;
    }

    void mMooneyRivlinProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        int d = 0;
        if (prop.contains(MatProp_TestDataType))
        {
            d = prop[MatProp_TestDataType].toInt();
            /*combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(d);
            combobox_.combobox->blockSignals(false);*/

            //inputWidgets = d == 0 ? inputWidgets1 : inputWidgets2;
        }

        // 1. 恢复"分类"下拉框：参数模式(0) 还是 试验数据模式(1)
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();
            comboboxType_.combobox->setCurrentIndex(d);
            slot_paraswitchInputs(d);
        }

        //这里增加一个枚举用来保存两个温度
        if (prop.contains(MatProp_Curve_Testdatainput_Press) || prop.contains(MatProp_Curve_Testdatainput_V))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }
        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }

        }
        if (prop.contains(MatProp_C10))
        {
            QVariant data = prop[MatProp_C10];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_C01))
        {
            QVariant data = prop[MatProp_C01];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D1))
        {
            QVariant data = prop[MatProp_D1];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mMooneyRivlinProp::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            for (auto& widgetSet : inputWidgets3)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            //this->addRow(combobox_.propwid);
            //this->addRow(labelbtn_.propwid);
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }
        return;
    }


    /***********************************Ogden***************************************/
    mOgdenProp::mOgdenProp(QWidget* parent)
        :mMaterialPropBase(MAT_Ogden, "Ogden", parent)
    {
        initInputs();
    }

    mOgdenProp::~mOgdenProp()
    {
        clearInputs();
    }

    void mOgdenProp::initInputs()
    {
        QStringList names1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList names2 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        QStringList namesMU =
        {
            "MU1",
            "MU2",
            "MU3",
            "MU4",
            "MU5",
            "MU6"
        };

        QVector<MxFunctions::MaterialPropType>proptypesMU = {
            MatProp_Mu1,
            MatProp_Mu2,
            MatProp_Mu3,
            MatProp_Mu4,
            MatProp_Mu5,
            MatProp_Mu6
        };

        QStringList namesAlpha =
        {
            "Alpha1",
            "Alpha2",
            "Alpha3",
            "Alpha4",
            "Alpha5",
            "Alpha6"
        };

        QVector<MxFunctions::MaterialPropType>proptypesAlpha = {
            MatProp_Alpha1,
            MatProp_Alpha2,
            MatProp_Alpha3,
            MatProp_Alpha4,
            MatProp_Alpha5,
            MatProp_Alpha6
        };

        QStringList namesD =
        {
            "D1",
            "D2",
            "D3",
            "D4",
            "D5",
            "D6"
        };

        QVector<MxFunctions::MaterialPropType>proptypesD = {
            MatProp_D1,
            MatProp_D2,
            MatProp_D3,
            MatProp_D4,
            MatProp_D5,
            MatProp_D6
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Ogden参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mOgdenProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        /*combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");*/

        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });


        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }


        //inputWidgets = inputWidgets1;


        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mOgdenProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        //labelbtn_.initWidget("Ogden");
        //labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        //labelbtn_.lablename->setText("试验类型");
        //labelbtn_.tablebtn->setDisabled(false);

        //connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int i) {
        //    QString tabletitle = "Ogden";
        //    QStringList header;
        //    for (auto inp : inputWidgets)
        //    {
        //        header << inp.name_;
        //    }
        //    QVariant value = labelbtn_.tablebtn->property("curve");
        //    //编辑表格
        //   // mMaterialPropBase::tableEdit(labelbtn_.tablebtn->isChecked(), tabletitle, value, header, false);

        //    //打开表格
        //    //mDockLayoutManager::getInstance()->activateDock("Table");

        //    QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

        //    mTableDataWindow::getInstance()->resetInitialTable();
        //    mTableDataWindow::getInstance()->setUseCurve(false);
        //    mTableDataWindow::getInstance()->setTableTitle(tabletitle);
        //    mTableDataWindow::getInstance()->setTableData(curves);
        //    mTableDataWindow::getInstance()->setHorizontalHeaders(header);
        //    mDockLayoutManager::getInstance()->activateDock("Table");
        //    });

        //初始化阶数输入控件
        spinbox_.initWidget("应变势能阶数");
        spinbox_.labelname->setText("应变势能阶数");
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(6);
        spinbox_.proptype_ = MatProp_Order;
        inputWidgetsTemp.clear();
        inputWidgetsMU.clear();
        inputWidgetsAlpha.clear();
        inputWidgetsD.clear();

        for (int i = 0; i < namesMU.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(namesMU[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(namesMU[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypesMU[i];
            type2inpMU[proptypesMU[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgetsMU.append(widgetSet);
        }
        for (int i = 0; i < namesAlpha.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(namesAlpha[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(namesAlpha[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypesAlpha[i];
            type2inpAlpha[proptypesAlpha[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgetsAlpha.append(widgetSet);
        }
        for (int i = 0; i < namesD.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(namesD[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(namesD[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypesD[i];
            type2inpD[proptypesD[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgetsD.append(widgetSet);
        }

        inputWidgetsTemp.append(inputWidgetsMU[0]);
        inputWidgetsTemp.append(inputWidgetsAlpha[0]);
        inputWidgetsTemp.append(inputWidgetsD[0]);

        //连接spinbox信号
        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mOgdenProp::slot_stepChanged);

        updatePropUnit();
        InitFoldSection();
    }

    void mOgdenProp::slot_stepChanged(int step)
    {
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->removeAll();
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            comboboxType_.propwid->show();
            spinbox_.propwid->show();
            inputWidgetsTemp.clear();
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsMU[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsAlpha[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsD[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
        }
        else
        {
            connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &mOgdenProp::slot_paraswitchInputs);
            inputWidgetsTemp.clear();
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsMU[i]);

            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsAlpha[i]);

            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsD[i]);

            }
        }
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mOgdenProp::slot_paraswitchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        spinbox_.propwid->show();
        comboboxType_.propwid->show();

        if (i == 0)
        {

            for (int j = 0; j < inputWidgetsTemp.size(); ++j)
            {

                this->addRow(inputWidgetsTemp[j].propwid);
                inputWidgetsTemp[j].propwid->show();
            }
        }
        else
        {
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();*/
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();
        }
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mOgdenProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);*/

        comboboxType_.propwid->show();
        spinbox_.propwid->show();
        /*labelbtn_.propwid->show();
        combobox_.propwid->show();*/

        this->addRow(checkboxNeoPress_.propwid);
        this->addRow(checkboxNeoV_.propwid);
        checkboxNeoPress_.propwid->show();
        checkboxNeoV_.propwid->show();

        this->refreshLayout();

        //emit sg_editFinished();
    }

    void mOgdenProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
    }

    void mOgdenProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mOgdenProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mOgdenProp::slot_connTable);
    }

    void mOgdenProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        /*for (auto& widgetSet : inputWidgetsTemp)
        {
            widgetSet.clear();
        }*/
        inputWidgetsTemp.clear();
        for (auto& widgetSet : inputWidgetsMU)
        {
            widgetSet.clear();
        }
        inputWidgetsMU.clear();
        for (auto& widgetSet : inputWidgetsAlpha)
        {
            widgetSet.clear();
        }
        inputWidgetsAlpha.clear();
        for (auto& widgetSet : inputWidgetsD)
        {
            widgetSet.clear();
        }
        inputWidgetsD.clear();
        spinbox_.clear();
        comboboxType_.clear();
        /*combobox_.clear();
        labelbtn_.clear();*/
    }

    void mOgdenProp::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTemp)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsMU)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsAlpha)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsD)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mOgdenProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];

        for (int i = 0; i < inputWidgetsMU.size(); ++i)
        {
            //inputWidgetsMU[i].labelunit->setText(stressunit);
            inputWidgetsMU[i].lineeditvalue->setUnit(stressunit);
        }
        for (int i = 0; i < inputWidgetsD.size(); ++i)
        {
            //inputWidgetsD[i].labelunit->setText(stressreciprocalunit);
            inputWidgetsD[i].lineeditvalue->setUnit(stressreciprocalunit);
        }
    }

    void mOgdenProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mOgdenProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        //collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, MatProp_Order, spinbox_.spinbox->value());
        //collectProps(props, labelbtn_);

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        collectProps(props, inputWidgets);

        QHash<MxFunctions::MaterialPropType, double> ratiosMU;
        ratiosMU[MatProp_Mu1] = ratio1;
        ratiosMU[MatProp_Mu2] = ratio1;
        ratiosMU[MatProp_Mu3] = ratio1;
        ratiosMU[MatProp_Mu4] = ratio1;
        ratiosMU[MatProp_Mu5] = ratio1;
        ratiosMU[MatProp_Mu6] = ratio1;
        collectProps(props, inputWidgetsMU, ratiosMU);

        collectProps(props, inputWidgetsAlpha);

        QHash<MxFunctions::MaterialPropType, double> ratiosD;
        ratiosD[MatProp_D1] = ratio2;
        ratiosD[MatProp_D2] = ratio2;
        ratiosD[MatProp_D3] = ratio2;
        ratiosD[MatProp_D4] = ratio2;
        ratiosD[MatProp_D5] = ratio2;
        ratiosD[MatProp_D6] = ratio2;
        collectProps(props, inputWidgetsD, ratiosD);
        //collectProps(props, inputWidgetsTemp);

        return props;
    }

    void mOgdenProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        /*int d = 0;
        if (prop.contains(MatProp_TestDataType))
        {
            d = prop[MatProp_TestDataType].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(d);
            combobox_.combobox->blockSignals(false);
            inputWidgets = d == 0 ? inputWidgets1 : inputWidgets2;
        }*/
        int d = 0;
        // 1. 恢复"分类"下拉框，并手动触发布局切换
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();
            comboboxType_.combobox->setCurrentIndex(d);
            slot_paraswitchInputs(d);  // 手动触发参数/试验布局切换
        }
        // 2. 恢复"试验数据类型"下拉框，并手动触发布局切换
        if (prop.contains(MatProp_TestDataType))
        {
            int idx = prop[MatProp_TestDataType].toInt();

            slot_switchInputs(idx);  // 手动触发单轴/体积切换
        }
        if (prop.contains(MatProp_Curve_Testdatainput))
        {
            d = prop[MatProp_ParaDataType].toInt();
            comboboxType_.combobox->setCurrentIndex(d);
            if (this)
            {
                slot_paraswitchInputs(d);  // 手动触发参数/试验布局切换
            }

        }

        if (prop.contains(MatProp_Curve_Testdatainput_Press) || prop.contains(MatProp_Curve_Testdatainput_V))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }

        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }

        }
        if (d == 0)
        {
            if (prop.contains(MatProp_Mu1))
            {
                QVariant data = prop[MatProp_Mu1];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu2))
            {
                QVariant data = prop[MatProp_Mu2];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu3))
            {
                QVariant data = prop[MatProp_Mu3];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu4))
            {
                QVariant data = prop[MatProp_Mu4];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu5))
            {
                QVariant data = prop[MatProp_Mu5];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu6))
            {
                QVariant data = prop[MatProp_Mu6];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha1))
            {
                QVariant data = prop[MatProp_Alpha1];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha2))
            {
                QVariant data = prop[MatProp_Alpha2];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha3))
            {
                QVariant data = prop[MatProp_Alpha3];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha4))
            {
                QVariant data = prop[MatProp_Alpha4];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha5))
            {
                QVariant data = prop[MatProp_Alpha5];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha6))
            {
                QVariant data = prop[MatProp_Alpha6];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsAlpha[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D1))
            {
                QVariant data = prop[MatProp_D1];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D2))
            {
                QVariant data = prop[MatProp_D2];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D3))
            {
                QVariant data = prop[MatProp_D3];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D4))
            {
                QVariant data = prop[MatProp_D4];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D5))
            {
                QVariant data = prop[MatProp_D5];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D6))
            {
                QVariant data = prop[MatProp_D6];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }
        if (prop.contains(MatProp_Order))
        {
            int step = prop[MatProp_Order].toInt();

            inputWidgetsTemp.clear();
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsMU[i]);
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsAlpha[i]);
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsD[i]);
            }
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (type2inpMU.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsMU[type2inpMU[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio1));
                    }
                }
                if (type2inpAlpha.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsAlpha[type2inpAlpha[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                    }
                }
                if (type2inpD.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsD[type2inpD[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio2));
                    }
                }
            }

            //spinbox_.spinbox->blockSignals(true);
            spinbox_.spinbox->setValue(step);
            //spinbox_.spinbox->blockSignals(false);
        }
        updatePropUnit();
    }

    void mOgdenProp::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            for (auto& widgetSet : inputWidgetsTemp)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            //this->addRow(combobox_.propwid);
            //this->addRow(labelbtn_.propwid);
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }
        return;
    }

    /***********************************Arruda-Boyce***************************************/
    mArrudaBoyceProp::mArrudaBoyceProp(QWidget* parent)
        :mMaterialPropBase(MAT_Arrudaboyce, "Arruda-Boyce", parent)
    {
        initInputs();
    }

    mArrudaBoyceProp::~mArrudaBoyceProp()
    {
        clearInputs();
    }

    void mArrudaBoyceProp::initInputs()
    {
        QStringList names1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList names2 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        QStringList names3 =
        {
            "MU",
            "LambdaM",
            "D"
        };

        QVector<MxFunctions::MaterialPropType>proptypes3 = {
            MatProp_MU,
            MatProp_LambdaM,
            MatProp_D
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Arruda-Boyce参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mArrudaBoyceProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        /*combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");*/
        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });

        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            // 设置文本
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            // 设置文本
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }

        //inputWidgets = inputWidgets1;

        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mArrudaBoyceProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        //labelbtn_.initWidget("Arruda-Boyce");
        //labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        //labelbtn_.lablename->setText("类型曲线");
        //labelbtn_.tablebtn->setDisabled(false);

        //connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int i) {
        //    QString tabletitle = "Arruda-Boyce";
        //    QStringList header;
        //    for (auto inp : inputWidgets)
        //    {
        //        header << inp.name_;
        //    }
        //    QVariant value = labelbtn_.tablebtn->property("curve");
        //    //编辑表格
        //   // mMaterialPropBase::tableEdit(labelbtn_.tablebtn->isChecked(), tabletitle, value, header, false);

        //    //打开表格
        //    //mDockLayoutManager::getInstance()->activateDock("Table");

        //    QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

        //    mTableDataWindow::getInstance()->resetInitialTable();
        //    mTableDataWindow::getInstance()->setUseCurve(false);
        //    mTableDataWindow::getInstance()->setTableTitle(tabletitle);
        //    mTableDataWindow::getInstance()->setTableData(curves);
        //    mTableDataWindow::getInstance()->setHorizontalHeaders(header);
        //    mDockLayoutManager::getInstance()->activateDock("Table");
        //    });
        auto* validator = new QDoubleValidator(0.0, 9999.0, 5, this);
        inputWidgets3.clear();
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names3[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names3[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes3[i];
            // 设置文本
            widgetSet.labelname->setText(names3[i]);

            if (names3[i] == "D")
                widgetSet.lineeditvalue->setValidator(validator);

            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets3.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mArrudaBoyceProp::slot_paraswitchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        comboboxType_.propwid->show();
        if (i == 0)
        {
            for (int j = 0; j < inputWidgets3.size(); ++j)
            {
                this->addRow(inputWidgets3[j].propwid);
                inputWidgets3[j].propwid->show();
            }
        }
        else
        {
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();*/
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();

        }
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mArrudaBoyceProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);*/
        comboboxType_.propwid->show();
        /*labelbtn_.propwid->show();
        combobox_.propwid->show();*/

        this->addRow(checkboxNeoPress_.propwid);
        this->addRow(checkboxNeoV_.propwid);
        checkboxNeoPress_.propwid->show();
        checkboxNeoV_.propwid->show();


        this->refreshLayout();



        //emit sg_editFinished();
    }

    void mArrudaBoyceProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        //labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }

        emit sg_editFinished();
    }

    void mArrudaBoyceProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mArrudaBoyceProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mArrudaBoyceProp::slot_connTable);
    }

    void mArrudaBoyceProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgets3)
        {
            widgetSet.clear();
        }
        inputWidgets3.clear();
        comboboxType_.clear();
        /*combobox_.clear();
        labelbtn_.clear();*/
    }

    void mArrudaBoyceProp::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets3)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mArrudaBoyceProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        //inputWidgets3[0].labelunit->setText(stressunit);
        //inputWidgets3[2].labelunit->setText(stressreciprocalunit);
        inputWidgets3[0].lineeditvalue->setUnit(stressunit);
        inputWidgets3[2].lineeditvalue->setUnit(stressreciprocalunit);
    }

    void mArrudaBoyceProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mArrudaBoyceProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        /*collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, labelbtn_);*/
        collectProps(props, inputWidgets);

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios3{ {MatProp_MU, ratio1}, {MatProp_D, ratio2} };
        collectProps(props, inputWidgets3, ratios3);

        return props;
    }

    void mArrudaBoyceProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        int d = 0;
        // 1. 恢复"分类"下拉框：参数模式(0) 还是 试验数据模式(1)
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();

            comboboxType_.combobox->setCurrentIndex(d);
            slot_paraswitchInputs(d);
        }
        // 2. 恢复"试验数据类型"下拉框：单轴(0) 还是 体积(1)
        if (prop.contains(MatProp_TestDataType))
        {
            int idx = prop[MatProp_TestDataType].toInt();

            slot_switchInputs(idx);
        }

        //这里增加一个枚举用来保存两个温度
        if (prop.contains(MatProp_Curve_Testdatainput_Press) || prop.contains(MatProp_Curve_Testdatainput_V))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }

        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }

        }
        if (prop.contains(MatProp_MU))
        {
            QVariant data = prop[MatProp_MU];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_LambdaM))
        {
            QVariant data = prop[MatProp_LambdaM];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets3[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D))
        {
            QVariant data = prop[MatProp_D];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mArrudaBoyceProp::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            for (auto& widgetSet : inputWidgets3)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);*/
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }
        return;
    }

    /***********************************Yeoh***************************************/
    mYeohProp::mYeohProp(QWidget* parent)
        :mMaterialPropBase(MAT_Yeoh, "Yeoh", parent)
    {

        initInputs();
    }

    mYeohProp::~mYeohProp()
    {
        clearInputs();
    }

    void mYeohProp::initInputs()
    {
        QStringList names = {
            "C10",
            "C20",
            "C30",
            "D1 ",
            "D2 ",
            "D3 "
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_C10,
            MatProp_C20,
            MatProp_C30,
            MatProp_D1,
            MatProp_D2,
            MatProp_D3,
        };
        QStringList names1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList names2 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };
        // 1. 分类下拉框：Yeoh参数 / 试验参数
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Yeoh参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mYeohProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);


        // 2. 试验类型下拉框：单轴拉伸试验 / 体积试验
        /*combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");*/
        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });


        inputWidgets1.clear();
        inputWidgets2.clear();

        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            widgetSet.labelname->setText(names1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() { emit sg_editFinished(); });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            widgetSet.labelname->setText(names2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() { emit sg_editFinished(); });
            inputWidgets2.append(widgetSet);
        }
        //inputWidgets = inputWidgets1;


        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mYeohProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        // 3. 类型曲线按钮
        //labelbtn_.initWidget("Yeoh");
        //labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        //labelbtn_.lablename->setText("类型曲线");
        //labelbtn_.tablebtn->setDisabled(false);
        //connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int) {
        //    QString tabletitle = "Yeoh";
        //    QStringList header;
        //    for (auto inp : inputWidgets) header << inp.name_;
        //    QVariant value = labelbtn_.tablebtn->property("curve");
        //    //mMaterialPropBase::tableEdit(labelbtn_.tablebtn->isChecked(), tabletitle, value, header, false);
        //    //mDockLayoutManager::getInstance()->activateDock("Table");

        //    QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

        //    mTableDataWindow::getInstance()->resetInitialTable();
        //    mTableDataWindow::getInstance()->setUseCurve(false);
        //    mTableDataWindow::getInstance()->setTableTitle(tabletitle);
        //    mTableDataWindow::getInstance()->setTableData(curves);
        //    mTableDataWindow::getInstance()->setHorizontalHeaders(header);
        //    mDockLayoutManager::getInstance()->activateDock("Table");
        //    });

        // 4. Yeoh 参数输入框（C10~D3）
        inputWidgets3.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            if (!widgetSet.propwid) {
                qDebug() << "initWidget failed for:" << names[i];
            }

            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() { emit sg_editFinished(); });
            inputWidgets3.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mYeohProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        //labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }

        emit sg_editFinished();
    }

    void mYeohProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mYeohProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mYeohProp::slot_connTable);
    }

    void mYeohProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);*/
        comboboxType_.propwid->show();
        /*combobox_.propwid->show();
        labelbtn_.propwid->show();*/



        if (i == 0)
        {
            inputWidgets = inputWidgets1;
        }
        else
        {
            inputWidgets = inputWidgets2;
        }

        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mYeohProp::slot_paraswitchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        comboboxType_.propwid->show();

        if (i == 0)  // Yeoh参数模式
        {
            inputWidgets = inputWidgets3;
            for (int j = 0; j < inputWidgets3.size(); ++j)
            {
                this->addRow(inputWidgets3[j].propwid);
                inputWidgets3[j].propwid->show();
            }
        }
        else  // 试验参数模式
        {
            //inputWidgets = inputWidgets1;
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();*/
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();
        }

        this->refreshLayout();
        //emit sg_editFinished();
    }



    void mYeohProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        //checkboxtemp_.clear();
    }

    void mYeohProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        //checkboxtemp_.cb->setProperty("curve", QVariant());
    }

    void mYeohProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        for (int i = 0; i < inputWidgets3.size(); ++i)
        {
            //inputWidgets[i].labelunit->setText(i < 3 ? stressunit : stressreciprocalunit);
            inputWidgets3[i].lineeditvalue->setUnit(i < 3 ? stressunit : stressreciprocalunit);
        }
    }

    void mYeohProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mYeohProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        //collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        //collectProps(props, labelbtn_);
        collectProps(props, inputWidgets);

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios3{ {MatProp_C10, ratio1}, {MatProp_C20, ratio1}, {MatProp_C30, ratio1}, {MatProp_D1, ratio2}, {MatProp_D2, ratio2}, {MatProp_D3, ratio2} };
        collectProps(props, inputWidgets3, ratios3);

        return props;
    }

    void mYeohProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        int d = 0;
        // 1. 恢复"分类"下拉框：参数模式(0) 还是 试验数据模式(1)
        if (prop.contains(MatProp_ParaDataType))
        {
            d = prop[MatProp_ParaDataType].toInt();

            comboboxType_.combobox->setCurrentIndex(d);
            if (this)
            {
                slot_paraswitchInputs(d);
            }

        }


        //这里增加一个枚举用来保存两个温度
        if (prop.contains(MatProp_Curve_Testdatainput_Press) || prop.contains(MatProp_Curve_Testdatainput_V))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }

        else
        {
            // 3. 恢复直接输入的试验参数（名义应力/应变 或 压力/体积比）
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }

        // 4. 恢复 Yeoh 参数（C10~D3）
        if (prop.contains(MatProp_C10))
        {
            QVariant data = prop[MatProp_C10];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_C20))
        {
            QVariant data = prop[MatProp_C20];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_C30))
        {
            QVariant data = prop[MatProp_C30];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio1;
                inputWidgets3[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D1))
        {
            QVariant data = prop[MatProp_D1];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D2))
        {
            QVariant data = prop[MatProp_D2];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_D3))
        {
            QVariant data = prop[MatProp_D3];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio2;
                inputWidgets3[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }

        updatePropUnit();
    }

    void mYeohProp::InitFoldSection()
    {
        //QVector<QWidget*>wds;
        ////this->addRow(checkboxtemp_.propwid);
        //for (auto& widgetSet : inputWidgets)
        //{
        //	this->addRow(widgetSet.propwid);
        //}
        // return;
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            for (auto& widgetSet : inputWidgets3)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            //this->addRow(combobox_.propwid);
            //this->addRow(labelbtn_.propwid);
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }
        return;
    }

    /***********************************Polynomial***************************************/
    mPolynomialProp::mPolynomialProp(QWidget* parent)
        :mMaterialPropBase(MAT_Polynomial, "Polynomial", parent)
    {
        initInputs();
    }

    mPolynomialProp::~mPolynomialProp()
    {
        clearInputs();
    }

    void mPolynomialProp::initInputs()
    {
        QStringList names1 =
        {
            "C10",
            "C01",
            "C20",
            "C11",
            "C02",
            "C30",
            "C21",
            "C12",
            "C03",
            "C40",
            "C31",
            "C22",
            "C13",
            "C04",
            "C50",
            "C41",
            "C32",
            "C23",
            "C14",
            "C05",
            "C60",
            "C51",
            "C42",
            "C33",
            "C24",
            "C15",
            "C06"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_C10,
            MatProp_C01,
            MatProp_C20,
            MatProp_C11,
            MatProp_C02,
            MatProp_C30,
            MatProp_C21,
            MatProp_C12,
            MatProp_C03,
            MatProp_C40,
            MatProp_C31,
            MatProp_C22,
            MatProp_C13,
            MatProp_C04,
            MatProp_C50,
            MatProp_C41,
            MatProp_C32,
            MatProp_C23,
            MatProp_C14,
            MatProp_C05,
            MatProp_C60,
            MatProp_C51,
            MatProp_C42,
            MatProp_C33,
            MatProp_C24,
            MatProp_C15,
            MatProp_C06,
        };
        QStringList names2 = {
            "D1 ",
            "D2 ",
            "D3 ",
            "D4 ",
            "D5 ",
            "D6 "
        };
        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_D1,
            MatProp_D2,
            MatProp_D3,
            MatProp_D4,
            MatProp_D5,
            MatProp_D6,
        };

        QStringList namesTD1 =
        {
            "名义应力",
            "名义应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypesTD1 = {
            MatProp_NominalStress,
            MatProp_NominalStrain
        };
        QStringList namesTD2 =
        {
            "压力",
            "体积比"
        };
        QVector<MxFunctions::MaterialPropType>proptypesTD2 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_ParaDataType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Polynomial参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPolynomialProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        /*combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");*/

        checkboxNeoPress_.initWidget("单轴拉伸实验");
        checkboxNeoPress_.proptype_ = MatProp_Curve_Testdatainput_Press;
        checkboxNeoPress_.cb->setText("单轴拉伸实验");
        checkboxNeoPress_.tablebtn->setDisabled(true);
        connect(checkboxNeoPress_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {

                checkboxNeoPress_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoPress_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-Press";
                QStringList header;
                for (auto inp : inputWidgets1)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoPress_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoPress_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");

                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoPress_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");
            });

        checkboxNeoV_.initWidget("体积实验");
        checkboxNeoV_.proptype_ = MatProp_Curve_Testdatainput_V;
        checkboxNeoV_.cb->setText("体积实验");
        checkboxNeoV_.tablebtn->setDisabled(true);
        connect(checkboxNeoV_.cb, &QCheckBox::stateChanged, this,
            [this](int i) {
                checkboxNeoV_.tablebtn->setDisabled(i == 0);
                mTableDataWindow::getInstance()->clearAndReset(0, 0);
            });
        connect(checkboxNeoV_.tablebtn, &QPushButton::clicked, this,
            [this](int i) {
                QString tabletitle = "Neo-Hookean2-V";
                QStringList header;
                for (auto inp : inputWidgets2)
                {
                    header << inp.name_;
                }

                QVariant value = checkboxNeoV_.cb->property("curve");
                ////编辑表格
                //mMaterialPropBase::tableEdit(checkboxNeoV_.cb->isChecked(),
                //    tabletitle, value, header);
                ////打开表格
                //mDockLayoutManager::getInstance()->activateDock("Table");
                QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

                mTableDataWindow::getInstance()->resetInitialTable();
                mTableDataWindow::getInstance()->setUseCurve(checkboxNeoV_.cb->isChecked());
                mTableDataWindow::getInstance()->setTableTitle(tabletitle);
                mTableDataWindow::getInstance()->setTableData(curves);
                mTableDataWindow::getInstance()->setHorizontalHeaders(header);
                // 设置列校验规则
                {
                    QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                    for (int col = 0; col < header.size(); ++col)
                    {
                        QString colName = header[col].trimmed();
                        if (colName == "压力" || colName == "体积比" || colName == "名义应力" || colName == "名义应变" || colName == "名义横向应变")
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                        else
                            vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                    }
                    mTableDataWindow::getInstance()->setColumnValidations(vaMap);
                }
                mDockLayoutManager::getInstance()->activateDock("Table");

            });

        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(namesTD1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(namesTD1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypesTD1[i];
            widgetSet.labelname->setText(namesTD1[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(namesTD2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(namesTD2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypesTD2[i];
            widgetSet.labelname->setText(namesTD2[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }
        //inputWidgets = inputWidgets1;

        /*connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPolynomialProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);*/

        //labelbtn_.initWidget("Polynomial");
        //labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        //labelbtn_.lablename->setText("试验类型");
        //labelbtn_.tablebtn->setDisabled(false);
        //connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int i) {
        //	QString tabletitle = "Polynomial";
        //	QStringList header;
        //	for (auto inp : inputWidgets)
        //	{
        //		header << inp.name_;
        //	}
        //	QVariant value = labelbtn_.tablebtn->property("curve");
        //	//mMaterialPropBase::tableEdit(labelbtn_.tablebtn->isChecked(), tabletitle, value, header, false);
        //	//mDockLayoutManager::getInstance()->activateDock("Table");

  //          QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();

  //          mTableDataWindow::getInstance()->resetInitialTable();
  //          mTableDataWindow::getInstance()->setUseCurve(false);
  //          mTableDataWindow::getInstance()->setTableTitle(tabletitle);
  //          mTableDataWindow::getInstance()->setTableData(curves);
  //          mTableDataWindow::getInstance()->setHorizontalHeaders(header);
  //          mDockLayoutManager::getInstance()->activateDock("Table");
        //	});

        spinbox_.initWidget("应变势能阶数");
        spinbox_.labelname->setText("应变势能阶数");
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(6);
        spinbox_.proptype_ = MatProp_Order;
        inputWidgetsTemp.clear();
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();

        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            type2inp1[proptypes1[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp1.append(widgetSet);
        }
        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            type2inp2[proptypes2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp2.append(widgetSet);
        }

        inputWidgetsTemp.append(inputWidgetsTmp1[0]);
        inputWidgetsTemp.append(inputWidgetsTmp1[1]);
        inputWidgetsTemp.append(inputWidgetsTmp2[0]);

        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mPolynomialProp::slot_stepChanged);

        updatePropUnit();
        InitFoldSection();
    }

    void mPolynomialProp::slot_paraswitchInputs(int i)
    {
        this->blockSignals(true);
        this->removeAll();
        this->blockSignals(false);
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        spinbox_.propwid->show();
        comboboxType_.propwid->show();
        if (i == 0)
        {
            for (int j = 0; j < inputWidgetsTemp.size(); ++j)
            {
                this->addRow(inputWidgetsTemp[j].propwid);
                inputWidgetsTemp[j].propwid->show();
            }
        }
        else
        {
            /*this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();*/

            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
            checkboxNeoPress_.propwid->show();
            checkboxNeoV_.propwid->show();

        }
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mPolynomialProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        /*this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);*/
        comboboxType_.propwid->show();
        spinbox_.propwid->show();
        /*labelbtn_.propwid->show();
        combobox_.propwid->show();*/

        this->addRow(checkboxNeoPress_.propwid);
        this->addRow(checkboxNeoV_.propwid);
        checkboxNeoPress_.propwid->show();
        checkboxNeoV_.propwid->show();

        /* if (i == 0)
         {
             inputWidgets = inputWidgets1;
         }
         else
         {
             inputWidgets = inputWidgets2;
         }*/
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mPolynomialProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Neo-Hookean2-V" && title != "Neo-Hookean2-Press")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        //labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        if (title == "Neo-Hookean2-V")
        {
            checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "Neo-Hookean2-Press")
        {
            checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves));
        }

        emit sg_editFinished();
    }

    void mPolynomialProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPolynomialProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPolynomialProp::slot_connTable);
    }

    void mPolynomialProp::slot_stepChanged(int step)
    {
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->removeAll();
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            comboboxType_.propwid->show();
            spinbox_.propwid->show();
            inputWidgetsTemp.clear();
            int cnt1 = 0, cnt2 = 0;
            if (step == 1)
            {
                cnt1 = 2;
                cnt2 = 1;
            }
            else if (step == 2)
            {
                cnt1 = 5;
                cnt2 = 2;
            }
            else if (step == 3)
            {
                cnt1 = 9;
                cnt2 = 3;
            }
            else if (step == 4)
            {
                cnt1 = 14;
                cnt2 = 4;
            }
            else if (step == 5)
            {
                cnt1 = 20;
                cnt2 = 5;
            }
            else
            {
                cnt1 = 27;
                cnt2 = 6;
            }
            for (int i = 0; i < cnt1; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp1[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
            for (int i = 0; i < cnt2; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp2[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
        }
        else
        {
            connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &mPolynomialProp::slot_paraswitchInputs);
            inputWidgetsTemp.clear();
            int cnt1 = 0, cnt2 = 0;
            if (step == 1)
            {
                cnt1 = 2;
                cnt2 = 1;
            }
            else if (step == 2)
            {
                cnt1 = 5;
                cnt2 = 2;
            }
            else if (step == 3)
            {
                cnt1 = 9;
                cnt2 = 3;
            }
            else if (step == 4)
            {
                cnt1 = 14;
                cnt2 = 4;
            }
            else if (step == 5)
            {
                cnt1 = 20;
                cnt2 = 5;
            }
            else
            {
                cnt1 = 27;
                cnt2 = 6;
            }
            for (int i = 0; i < cnt1; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp1[i]);
            }
            for (int i = 0; i < cnt2; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp2[i]);
            }
        }
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mPolynomialProp::clearData()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTemp)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mPolynomialProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        for (int i = 0; i < inputWidgetsTmp1.size(); ++i)
        {
            inputWidgetsTmp1[i].lineeditvalue->setUnit(stressunit);
        }
        for (int i = 0; i < inputWidgetsTmp2.size(); ++i)
        {
            inputWidgetsTmp2[i].lineeditvalue->setUnit(stressreciprocalunit);
        }
    }

    void mPolynomialProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    void mPolynomialProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets1)
        {
            widgetSet.clear();
        }
        inputWidgets1.clear();
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        inputWidgets2.clear();
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        /*for (auto& widgetSet : inputWidgetsTemp)
        {
            widgetSet.clear();
        }*/
        inputWidgetsTemp.clear();
       /* for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }*/
        inputWidgetsTmp1.clear();
        /*for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }*/
        inputWidgetsTmp2.clear();
        spinbox_.clear();
        comboboxType_.clear();
        /*combobox_.clear();
        labelbtn_.clear();*/
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mPolynomialProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        //collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, MatProp_Order, spinbox_.spinbox->value());
        //collectProps(props, labelbtn_);
        collectProps(props, inputWidgets);

        if (checkboxNeoPress_.cb->isChecked())
        {
            collectProps(props, checkboxNeoPress_, CurveType::NoCorrelation);
        }
        if (checkboxNeoV_.cb->isChecked())
        {
            collectProps(props, checkboxNeoV_, CurveType::NoCorrelation);
        }

        QHash<MxFunctions::MaterialPropType, double> ratios1;
        for (int i = MatProp_C10; i <= MatProp_C06; ++i)
        {
            ratios1[static_cast<MxFunctions::MaterialPropType>(i)] = ratio1;
        }
        collectProps(props, inputWidgetsTmp1, ratios1);

        QHash<MxFunctions::MaterialPropType, double> ratios2;
        for (int i = MatProp_D1; i <= MatProp_D6; ++i)
        {
            ratios2[static_cast<MxFunctions::MaterialPropType>(i)] = ratio2;
        }
        collectProps(props, inputWidgetsTmp2, ratios2);

        //collectProps(props, inputWidgetsTemp);
        return props;
    }

    void mPolynomialProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_Order))
        {
            int step = prop[MatProp_Order].toInt();
            spinbox_.spinbox->blockSignals(true);
            spinbox_.spinbox->setValue(step);
            spinbox_.spinbox->blockSignals(false);
            inputWidgetsTemp.clear();
            int cnt1 = 0, cnt2 = 0;
            if (step == 1)
            {
                cnt1 = 2;
                cnt2 = 1;
            }
            else if (step == 2)
            {
                cnt1 = 5;
                cnt2 = 2;
            }
            else if (step == 3)
            {
                cnt1 = 9;
                cnt2 = 3;
            }
            else if (step == 4)
            {
                cnt1 = 14;
                cnt2 = 4;
            }
            else if (step == 5)
            {
                cnt1 = 20;
                cnt2 = 5;
            }
            else
            {
                cnt1 = 27;
                cnt2 = 6;
            }
            for (int i = 0; i < cnt1; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp1[i]);
            }
            for (int i = 0; i < cnt2; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsTmp2[i]);
            }
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (type2inp1.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                        inputWidgetsTmp1[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio1));
                }
                if (type2inp2.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                        inputWidgetsTmp2[type2inp2[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio2));
                }
            }
        }

        int d = 0;
        if (prop.contains(MatProp_TestDataType))
        {
            d = prop[MatProp_TestDataType].toInt();
            /*combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(d);
            combobox_.combobox->blockSignals(false);
            if (this)
            {
                slot_switchInputs(d);
            }*/
        }
        if (prop.contains(MatProp_ParaDataType))
        {
            int idx = prop[MatProp_ParaDataType].toInt();
            comboboxType_.combobox->blockSignals(true);
            comboboxType_.combobox->setCurrentIndex(idx);
            comboboxType_.combobox->blockSignals(false);
            if (this)
            {
                slot_paraswitchInputs(idx);
            }
        }

        //这里增加一个枚举用来保存两个温度
        if (prop.contains(MatProp_Curve_Testdatainput_Press) || prop.contains(MatProp_Curve_Testdatainput_V))
        {

            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            if (prop.contains(MatProp_Curve_Testdatainput_Press))
            {
                checkboxNeoPress_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_Press];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoPress_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }
            }
            if (prop.contains(MatProp_Curve_Testdatainput_V))
            {
                checkboxNeoV_.cb->setChecked(true);
                QVariant data = prop[MatProp_Curve_Testdatainput_V];
                if (data.isValid())
                {
                    MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                    checkboxNeoV_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                }

            }

        }

        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }
        updatePropUnit();
    }

    void mPolynomialProp::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        //combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            for (auto& widgetSet : inputWidgetsTemp)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else
        {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            //this->addRow(combobox_.propwid);
            //this->addRow(labelbtn_.propwid);
            this->addRow(checkboxNeoPress_.propwid);
            this->addRow(checkboxNeoV_.propwid);
        }

        slot_stepChanged(1);

        return;
    }

    /***********************************Reduced Polynomial***************************************/
    mReducedPolynomialProp::mReducedPolynomialProp(QWidget* parent)
        :mMaterialPropBase(MAT_Rpolynomial, "Reduced Polynomial", parent)
    {
        initInputs();
    }

    mReducedPolynomialProp::~mReducedPolynomialProp()
    {
        clearInputs();
    }

    void mReducedPolynomialProp::initInputs()
    {
        QStringList names1 = {
            "C10",
            "C20",
            "C30"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_C10,
            MatProp_C20,
            MatProp_C30,
        };
        QStringList names2 = {
            "D1 ",
            "D2 ",
            "D3 ",
            "D4 ",
            "D5 ",
            "D6 "
        };
        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_D1,
            MatProp_D2,
            MatProp_D3,
            MatProp_D4,
            MatProp_D5,
            MatProp_D6,
        };

        //初始化输入控件
        spinbox_.initWidget("应变势能阶数");
        spinbox_.labelname->setText("应变势能阶数");
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(6);
        spinbox_.proptype_ = MatProp_Order;
        inputWidgets.clear();
        inputWidgetsTmp1.clear();
        inputWidgetsTmp2.clear();

        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            type2inp1[proptypes1[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgetsTmp1.append(widgetSet);
        }

        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            type2inp2[proptypes2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgetsTmp2.append(widgetSet);
        }
        inputWidgets.append(inputWidgetsTmp1[0]);
        inputWidgets.append(inputWidgetsTmp2[0]);

        checkboxtemp_.initWidget("Reduced Polynomial");
        checkboxtemp_.setPropType(MatProp_Curve_ReducePolynomial);
        checkboxtemp_.cb->setText("温度相关");
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随应变势能阶数变化（step 个 C 系数 + step 个 D 系数），
        // 初始按默认阶数生成，slot_stepChanged 中经 refreshCurveHeaders 更新
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-Reduced Polynomial曲线"),
            QStringLiteral("温度 / °C"),
            { names1[0].trimmed(), names2[0].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            spinbox_.setDisableAll(i != 0);
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        //连接spinbox信号
        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mReducedPolynomialProp::slot_stepChanged);

        updatePropUnit();

    }

    void mReducedPolynomialProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.labelname->text().trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-Reduced Polynomial曲线"),
            QStringLiteral("温度 / °C"), params);
    }

    void mReducedPolynomialProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Reduced Polynomial")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        emit sg_editFinished();
    }

    void mReducedPolynomialProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mReducedPolynomialProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mReducedPolynomialProp::slot_connTable);
    }

    void mReducedPolynomialProp::slot_stepChanged(int step)
    {
        this->removeAll();
        this->addRow(spinbox_.propwid);
        spinbox_.propwid->show();
        inputWidgets.clear();
        for (int i = 0; i < step; ++i)
        {
            inputWidgets.append(inputWidgetsTmp1[i]);
            this->addRow(inputWidgets.back().propwid);
            inputWidgets.back().propwid->show();
        }
        for (int i = 0; i < step; ++i)
        {
            inputWidgets.append(inputWidgetsTmp2[i]);
            this->addRow(inputWidgets.back().propwid);
            inputWidgets.back().propwid->show();
        }
        // 阶数变化后曲线参数列数变化（step 个 C 系数 + step 个 D 系数）
        refreshCurveHeaders();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mReducedPolynomialProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : inputWidgetsTmp1)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp1.clear();
        for (auto& widgetSet : inputWidgetsTmp2)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp2.clear();
        spinbox_.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mReducedPolynomialProp::clearData()
    {
        for (auto inp : inputWidgetsTmp1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsTmp2)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mReducedPolynomialProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString stressreciprocalunit = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        for (int i = 0; i < inputWidgetsTmp1.size(); ++i)
        {
            //inputWidgetsTmp1[i].labelunit->setText(stressunit);
            inputWidgetsTmp1[i].lineeditvalue->setUnit(stressunit);
        }
        for (int i = 0; i < inputWidgetsTmp2.size(); ++i)
        {
            //inputWidgetsTmp2[i].labelunit->setText(stressreciprocalunit);
            inputWidgetsTmp2[i].lineeditvalue->setUnit(stressreciprocalunit);

        }
    }

    void mReducedPolynomialProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mReducedPolynomialProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, MatProp_Order, spinbox_.spinbox->value());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_C10, ratio1},
                {MatProp_C20, ratio1},
                {MatProp_C30, ratio1},
            };
            collectProps(props, inputWidgetsTmp1, ratios);

            QHash<MxFunctions::MaterialPropType, double> ratiosT{
                {MatProp_D1, ratio2},
                {MatProp_D2, ratio2},
                {MatProp_D3, ratio2},
                {MatProp_D4, ratio2},
                {MatProp_D5, ratio2},
                {MatProp_D6, ratio2},
            };
            collectProps(props, inputWidgetsTmp2, ratiosT);


            collectProps(props, inputWidgets);
        }
        return props;
    }

    void mReducedPolynomialProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_Order))
        {
            int step = prop[MatProp_Order].toInt();

            spinbox_.spinbox->blockSignals(true);
            spinbox_.spinbox->setValue(step);
            spinbox_.spinbox->blockSignals(false);

            inputWidgets.clear();
            for (int i = 0; i < step; ++i)
            {
                inputWidgets.append(inputWidgetsTmp1[i]);
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgets.append(inputWidgetsTmp2[i]);
            }
        }
        if (prop.contains(MatProp_Curve_ReducePolynomial))
        {
            for (auto inp : inputWidgetsTmp1)
            {
                inp.lineeditvalue->setExpression("");
            }
            for (auto inp : inputWidgetsTmp2)
            {
                inp.lineeditvalue->setExpression("");
            }
            checkboxtemp_.cb->setChecked(true);
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_ReducePolynomial];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (type2inp1.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsTmp1[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio1));
                    }
                }
                if (type2inp2.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsTmp2[type2inp2[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio2));
                    }
                }
            }
        }
        updatePropUnit();
    }

    void mReducedPolynomialProp::InitFoldSection()
    {
        QVector<QWidget*>wds;

        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mReducedPolynomialProp::slot_stepChanged);
        this->addRow(spinbox_.propwid);
        spinbox_.spinbox->setValue(spinbox_.spinbox->value());
        // 温度相关行由封装控件承载（slot_stepChanged 内重排时统一 addRow）
        for (const auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        return;
    }

    /***********************************Hyperfoam 超弹泡沫材料***************************************/
    mHyperfoamProp::mHyperfoamProp(QWidget* parent)
        :mMaterialPropBase(MAT_Hyperhorm, "Hyperfoam", parent)
    {
        initInputs();
    }

    mHyperfoamProp::~mHyperfoamProp()
    {
        clearInputs();
    }

    void mHyperfoamProp::initInputs()
    {
        QStringList names1 = {
            "mu1   ",
            "alpha1",
            "mu2   ",
            "alpha2",
            "mu3   ",
            "alpha3",
            "mu4   ",
            "alpha4",
            "mu5   ",
            "alpha5",
            "mu6   ",
            "alpha6"
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
            MatProp_Mu1,
            MatProp_Alpha1,
            MatProp_Mu2,
            MatProp_Alpha2,
            MatProp_Mu3,
            MatProp_Alpha3,
            MatProp_Mu4,
            MatProp_Alpha4,
            MatProp_Mu5,
            MatProp_Alpha5,
            MatProp_Mu6,
            MatProp_Alpha6
        };
        QStringList names2 = {
            "D1",
            "D2",
            "D3",
            "D4",
            "D5",
            "D6"
        };
        QVector<MxFunctions::MaterialPropType>proptypes2 = {
            MatProp_Nu1,
            MatProp_Nu2,
            MatProp_Nu3,
            MatProp_Nu4,
            MatProp_Nu5,
            MatProp_Nu6
        };

        QStringList names3 =
        {
            "名义应力",
            "名义应变",
            "名义横向应变"
        };
        QVector<MxFunctions::MaterialPropType>proptypes3 = {
            MatProp_NominalStress,
            MatProp_NominalStrain,
            MatProp_NominalLateralStrain
        };
        QStringList names4 =
        {
            "压力",
            "体积比"
        };

        QVector<MxFunctions::MaterialPropType>proptypes4 = {
            MatProp_Pressure,
            MatProp_VolumeRatio
        };

        //初始化输入控件
        comboboxType_.initWidget("分类");
        comboboxType_.proptype_ = MatProp_HyperfoamType;
        comboboxType_.combobox->clear();
        comboboxType_.combobox->addItem("Hyperfoam参数");
        comboboxType_.combobox->addItem("试验参数");
        connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mHyperfoamProp::slot_paraswitchInputs);
        comboboxType_.combobox->setCurrentIndex(0);

        combobox_.initWidget("试验类型");
        combobox_.proptype_ = MatProp_TestDataType;
        combobox_.combobox->clear();
        combobox_.combobox->addItem("单轴拉伸试验");
        combobox_.combobox->addItem("体积试验");
        inputWidgets1.clear();
        inputWidgets2.clear();
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names3[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names3[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes3[i];
            // 设置文本
            widgetSet.labelname->setText(names3[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets1.append(widgetSet);
        }
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            widgetSet.initWidget(names4[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names4[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes4[i];
            // 设置文本
            widgetSet.labelname->setText(names4[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }
        inputWidgets = inputWidgets1;
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mHyperfoamProp::slot_switchInputs);
        combobox_.combobox->setCurrentIndex(0);

        labelbtn_.initWidget("HyperfoamProp");
        labelbtn_.proptype_ = MatProp_Curve_Testdatainput;
        labelbtn_.lablename->setText("试验类型表格");
        labelbtn_.tablebtn->setDisabled(false);

        connect(labelbtn_.tablebtn, &QPushButton::clicked, this, [this](int i) {
            QString tabletitle = "Hyperfoam";
            QStringList header;
            for (auto inp : inputWidgets)
            {
                header << inp.name_;
            }
            QVariant value = labelbtn_.tablebtn->property("curve");
            //编辑表格
            mMaterialPropBase::tableEdit(labelbtn_.tablebtn->isChecked(), tabletitle, value, header);
            // 设置列校验规则
            {
                QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                for (int col = 0; col < header.size(); ++col)
                {
                    QString colName = header[col].trimmed();
                    if (colName == "D1" || colName == "D2" || colName == "D3" || colName == "D4" || colName == "D5" || colName == "D6")
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::NonNegativeReal);
                    else
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                }
                mTableDataWindow::getInstance()->setColumnValidations(vaMap);
            }

            //打开表格
            mDockLayoutManager::getInstance()->activateDock("Table");
            });

        spinbox_.initWidget("应变势能阶数");
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(6);
        spinbox_.proptype_ = MatProp_Order;
        inputWidgetsTemp.clear();
        inputWidgetsMU_Alpha.clear();
        inputWidgetsD.clear();

        for (int i = 0; i < names1.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            type2inp1[proptypes1[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsMU_Alpha.append(widgetSet);
        }

        for (int i = 0; i < names2.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            type2inp2[proptypes2[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsD.append(widgetSet);
        }
        inputWidgetsTemp.append(inputWidgetsMU_Alpha[0]);
        inputWidgetsTemp.append(inputWidgetsMU_Alpha[1]);
        inputWidgetsTemp.append(inputWidgetsD[0]);

        //checkboxtemp_.initWidget("Hyperfoam");
        //checkboxtemp_.proptype_ = MatProp_Curve_Hyperfoam;
        //checkboxtemp_.cb->setText("温度相关");
        //checkboxtemp_.tablebtn->setDisabled(true);
        //connect(checkboxtemp_.cb, &QCheckBox::stateChanged, this, [this](int i) {
        //	for (auto inp : inputWidgets)
        //	{
        //		inp.setDisableAll(i != 0);
        //	}
        //	spinbox_.setDisableAll(true);
        //	checkboxtemp_.tablebtn->setDisabled(i == 0);
        //	mTableDataWindow::getInstance()->clearAndReset(0, 0);

        //	});
        //connect(checkboxtemp_.tablebtn, &QPushButton::clicked, this, [this](int i) {			
        //	QStringList header;
        //	header << "温度";
        //	for (int i = 0; i < inputWidgets.size(); ++i)
        //	{
        //		header << inputWidgets[i].labelname->text();
        //	}
  //          QString tabletitle = "Hyperfoam";
        //	QVariant value = checkboxtemp_.cb->property("curve");
  //          //编辑表格
  //          mMaterialPropBase::tableEdit(checkboxtemp_.cb->isChecked(), tabletitle, value, header);
        //	//打开表格
        //	mDockLayoutManager::getInstance()->activateDock("Table");
        //	});

        //连接spinbox信号
        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mHyperfoamProp::slot_stepChanged);

        updatePropUnit();
        InitFoldSection();
    }

    void mHyperfoamProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "Hyperfoam")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        emit sg_editFinished();
    }

    void mHyperfoamProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHyperfoamProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mHyperfoamProp::slot_connTable);
    }

    void mHyperfoamProp::slot_stepChanged(int step)
    {
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->removeAll();
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            spinbox_.propwid->show();
            comboboxType_.propwid->show();
            inputWidgetsTemp.clear();
            for (int i = 0; i < step * 2; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsMU_Alpha[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsD[i]);
                this->addRow(inputWidgetsTemp.back().propwid);
                inputWidgetsTemp.back().propwid->show();
            }
        }
        else
        {
            connect(comboboxType_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &mHyperfoamProp::slot_paraswitchInputs);
            inputWidgetsTemp.clear();
            for (int i = 0; i < step * 2; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsMU_Alpha[i]);
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgetsTemp.append(inputWidgetsD[i]);
            }
        }
        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mHyperfoamProp::clearInputs()
    {
        for (auto inp : inputWidgets1)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto& widgetSet : inputWidgetsTemp)
        {
            widgetSet.clear();
        }
        inputWidgetsTemp.clear();
        for (auto& widgetSet : inputWidgetsMU_Alpha)
        {
            widgetSet.clear();
        }
        inputWidgetsMU_Alpha.clear();
        for (auto& widgetSet : inputWidgetsD)
        {
            widgetSet.clear();
        }
        inputWidgetsD.clear();
        spinbox_.clear();
    }

    void mHyperfoamProp::clearData()
    {
        for (auto inp : inputWidgetsMU_Alpha)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgetsD)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mHyperfoamProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        for (int i = 0; i < inputWidgetsMU_Alpha.size(); ++i)
        {
            if (!(i & 1))
                inputWidgetsMU_Alpha[i].lineeditvalue->setUnit(stressunit);
            //inputWidgetsMU_Alpha[i].labelunit->setText(stressunit);
        }
        for (int i = 0; i < inputWidgetsD.size(); ++i)
        {
            inputWidgetsD[i].lineeditvalue->setUnit(stressunit);
            //inputWidgetsD[i].labelunit->setText(stressunit);
        }
    }

    void mHyperfoamProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mHyperfoamProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboboxType_.proptype_, comboboxType_.combobox->currentIndex());
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, MatProp_Order, spinbox_.spinbox->value());
        collectProps(props, labelbtn_);
        collectProps(props, inputWidgets);

        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_Mu1, ratio1},
            {MatProp_Mu2, ratio1},
            {MatProp_Mu3, ratio1},
            {MatProp_Mu4, ratio1},
            {MatProp_Mu5, ratio1},
            {MatProp_Mu6, ratio1},
        };
        collectProps(props, inputWidgetsMU_Alpha, ratios);

        QHash<MxFunctions::MaterialPropType, double> ratiosT{
            {MatProp_Nu1, ratio2},
            {MatProp_Nu2, ratio2},
            {MatProp_Nu3, ratio2},
            {MatProp_Nu4, ratio2},
            {MatProp_Nu5, ratio2},
            {MatProp_Nu6, ratio2},

        };
        collectProps(props, inputWidgetsD, ratiosT);

        //collectProps(props, inputWidgetsTemp);




        return props;
    }

    void mHyperfoamProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        int d = 0;
        if (prop.contains(MatProp_TestDataType))
        {
            d = prop[MatProp_TestDataType].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(d);
            combobox_.combobox->blockSignals(false);
            inputWidgets = d == 0 ? inputWidgets1 : inputWidgets2;
        }
        if (prop.contains(MatProp_Curve_Testdatainput))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            QVariant data = prop[MatProp_Curve_Testdatainput];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                labelbtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves.curves));
            }
        }
        else
        {
            if (prop.contains(MatProp_NominalStress))
            {
                QVariant data = prop[MatProp_NominalStress];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalStrain))
            {
                QVariant data = prop[MatProp_NominalStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_NominalLateralStrain))
            {
                QVariant data = prop[MatProp_NominalLateralStrain];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Pressure))
            {
                QVariant data = prop[MatProp_Pressure];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_VolumeRatio))
            {
                QVariant data = prop[MatProp_VolumeRatio];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu1))
            {
                QVariant data = prop[MatProp_Mu1];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu2))
            {
                QVariant data = prop[MatProp_Mu2];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu3))
            {
                QVariant data = prop[MatProp_Mu3];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu4))
            {
                QVariant data = prop[MatProp_Mu4];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[6].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu5))
            {
                QVariant data = prop[MatProp_Mu5];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[8].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Mu6))
            {
                QVariant data = prop[MatProp_Mu6];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio1;
                    inputWidgetsMU_Alpha[10].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha1))
            {
                QVariant data = prop[MatProp_Alpha1];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha2))
            {
                QVariant data = prop[MatProp_Alpha2];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha3))
            {
                QVariant data = prop[MatProp_Alpha3];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha4))
            {
                QVariant data = prop[MatProp_Alpha4];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[7].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha5))
            {
                QVariant data = prop[MatProp_Alpha5];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[9].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_Alpha6))
            {
                QVariant data = prop[MatProp_Alpha6];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgetsMU_Alpha[11].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D1))
            {
                QVariant data = prop[MatProp_D1];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D2))
            {
                QVariant data = prop[MatProp_D2];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D3))
            {
                QVariant data = prop[MatProp_D3];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[2].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D4))
            {
                QVariant data = prop[MatProp_D4];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[3].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D5))
            {
                QVariant data = prop[MatProp_D5];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[4].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
            if (prop.contains(MatProp_D6))
            {
                QVariant data = prop[MatProp_D6];
                if (data.isValid())
                {
                    double v = data.toDouble() / ratio2;
                    inputWidgetsD[5].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }

        }
        if (prop.contains(MatProp_Order))
        {
            int step = prop[MatProp_Order].toInt();

            spinbox_.spinbox->blockSignals(true);
            spinbox_.spinbox->setValue(step);
            spinbox_.spinbox->blockSignals(false);
            slot_stepChanged(step);
            inputWidgetsTemp.clear();
            for (int i = 0; i < step * 2; ++i)
            {
                inputWidgets.append(inputWidgetsMU_Alpha[i]);
            }
            for (int i = 0; i < step; ++i)
            {
                inputWidgets.append(inputWidgetsD[i]);
            }
        }
        if (prop.contains(MatProp_Curve_Hyperfoam))
        {
            for (auto inp : inputWidgetsMU_Alpha)
            {
                inp.lineeditvalue->setExpression("");
            }
            for (auto inp : inputWidgetsD)
            {
                inp.lineeditvalue->setExpression("");
            }
            QVariant data = prop[MatProp_Curve_Hyperfoam];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
            }
        }
        else
        {

            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (type2inp1.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        if ((type2inp1[it.key()] & 1))
                            inputWidgetsMU_Alpha[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                        else
                            inputWidgetsMU_Alpha[type2inp1[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio1));
                    }
                }
                if (type2inp2.contains(it.key()))
                {
                    QVariant data = prop[it.key()];
                    if (data.isValid())
                    {
                        inputWidgetsD[type2inp2[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio2));
                    }
                }
            }
        }
        updatePropUnit();
    }

    void mHyperfoamProp::InitFoldSection()
    {
        comboboxType_.combobox->setCurrentIndex(comboboxType_.combobox->currentIndex());
        combobox_.combobox->setCurrentIndex(combobox_.combobox->currentIndex());
        if (comboboxType_.combobox->currentIndex() == 0)
        {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            spinbox_.spinbox->setValue(spinbox_.spinbox->value());
            for (auto& widgetSet : inputWidgetsTemp)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else {
            this->addRow(comboboxType_.propwid);
            this->addRow(spinbox_.propwid);
            this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
        }
        return;
    }

    void mHyperfoamProp::slot_paraswitchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        spinbox_.propwid->show();
        comboboxType_.propwid->show();

        if (i == 0)
        {

            for (int j = 0; j < inputWidgetsTemp.size(); ++j)
            {

                this->addRow(inputWidgetsTemp[j].propwid);
                inputWidgetsTemp[j].propwid->show();
            }
        }
        else
        {
            this->addRow(combobox_.propwid);
            this->addRow(labelbtn_.propwid);
            combobox_.propwid->show();
            labelbtn_.propwid->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }
    void mHyperfoamProp::slot_switchInputs(int i)
    {
        this->removeAll();
        this->addRow(comboboxType_.propwid);
        this->addRow(spinbox_.propwid);
        this->addRow(combobox_.propwid);
        this->addRow(labelbtn_.propwid);

        comboboxType_.propwid->show();
        spinbox_.propwid->show();
        labelbtn_.propwid->show();
        combobox_.propwid->show();
        if (i == 0)
        {
            inputWidgets = inputWidgets1;
        }
        else
        {
            inputWidgets = inputWidgets2;
        }

        this->refreshLayout();
        emit sg_editFinished();
    }
    /***********************************刚体材料***************************************/
    mRigidProp::mRigidProp(QWidget* parent)
        :mMaterialPropBase(MAT_Rigid, "刚体材料", parent)
    {
        initInputs();
    }

    mRigidProp::~mRigidProp()
    {
        clearInputs();
    }

    void mRigidProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  "
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        checkboxtemp_.initWidget("刚体材料");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Y_B);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-刚体材料参数曲线"),
            QStringLiteral("温度 / °C"),
            { names[0].trimmed(), names[1].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        updatePropUnit();

    }

    void mRigidProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "刚体材料")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mRigidProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mRigidProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mRigidProp::slot_connTable);
    }

    void mRigidProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        checkboxtemp_.clear();
        inputWidgets.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mRigidProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mRigidProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mRigidProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mRigidProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_YoungsModulus, ratio} };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mRigidProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_YoungsModulus))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Curve_Y_B))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Y_B];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mRigidProp::InitFoldSection()
    {
        QVector<QWidget*>wds;
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************声学***************************************/
    mAeroacousticsProp::mAeroacousticsProp(QWidget* parent)
        :mMaterialPropBase(MAT_Acoustic, "声学材料", parent)
    {
        initInputs();
    }

    mAeroacousticsProp::~mAeroacousticsProp()
    {
        clearInputs();
    }

    void mAeroacousticsProp::initInputs()
    {
        QStringList names = {
            "声速",
        };
        //初始化输入控件
        inputWidgets.clear();

        //for (int i = 0; i < 1; ++i)
        //{
        //	InputWidgetSet widgetSet;

        //	// 初始化单个输入组
        //	widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
        //	widgetSet.proptype_ = MatProp_SoundVelocity;
        //	connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
        //		[this, widgetSet]() {
        //			emit sg_editFinished();
        //		});
        //	inputWidgets.append(widgetSet);
        //}
        updatePropUnit();
    }

    void mAeroacousticsProp::clearInputs()
    {
        /*	for (auto& widgetSet : inputWidgets)
            {
                widgetSet.clear();
            }
            inputWidgets.clear();*/
    }

    void mAeroacousticsProp::clearData()
    {
        //for (auto inp : inputWidgets)
        //{
        //	inp.lineeditvalue->setExpression("");
        //}
    }

    void mAeroacousticsProp::updatePropUnit()
    {
        /*	QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
            QString stressunit = UnitsLibrary[(int)Unit::VelocityOfSound][sysunitidxs[0]];
            inputWidgets[0].labelunit->setText(stressunit);*/
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mAeroacousticsProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        //collectProps(props, inputWidgets);
        return props;
    }

    void mAeroacousticsProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        /*	clearData();
            if (prop.contains(MatProp_SoundVelocity))
            {
                QVariant data = prop[MatProp_SoundVelocity];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }*/
        updatePropUnit();
    }

    void mAeroacousticsProp::InitFoldSection()
    {
        QVector<QWidget*>wds;
        /*	for (auto& widgetSet : inputWidgets)
            {
                this->addRow(widgetSet.propwid);
            }*/
        return;
    }

    /***********************************变形塑性材料***************************************/
    mDeformationPlasticityProp::mDeformationPlasticityProp(QWidget* parent)
        :mMaterialPropBase(MAT_Deformation, "变形塑性材料", parent)
    {
        initInputs();
    }

    mDeformationPlasticityProp::~mDeformationPlasticityProp()
    {
        clearInputs();
    }

    void mDeformationPlasticityProp::initInputs()
    {
        QStringList names = {
            "弹性模量",
            "泊松比  ",
            "屈服应力",
            "非线性塑性硬化指数",
            "材料参数α",
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson,
            MatProp_YieldStress,
            MatProp_StressExponent,
            MatProp_YieldOffset,
        };
        //初始化输入控件
        inputWidgets.clear();

        for (int i = 0; i < 5; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.labelname->setMinimumWidth(120);
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            // 设置文本
            inputWidgets.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mDeformationPlasticityProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "变形塑性材料")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        emit sg_editFinished();
    }

    void mDeformationPlasticityProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mDeformationPlasticityProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mDeformationPlasticityProp::slot_connTable);
    }


    void mDeformationPlasticityProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    void mDeformationPlasticityProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mDeformationPlasticityProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        //inputWidgets[2].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
        inputWidgets[2].lineeditvalue->setUnit(stressunit);
    }

    void mDeformationPlasticityProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mDeformationPlasticityProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_YoungsModulus, ratio},
            {MatProp_YieldStress, ratio},
        };
        collectProps(props, inputWidgets, ratios);

        return props;
    }

    void mDeformationPlasticityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_Curve_Deformation))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }

            QVariant data = prop[MatProp_Curve_Deformation];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
            }
        }
        else
        {
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    if(type2inp[it.key()] == 0 || type2inp[it.key()] == 2)
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
                    else
                        inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                }
            }
        }
        updatePropUnit();
    }

    void mDeformationPlasticityProp::InitFoldSection()
    {
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    /***********************************粘弹性材料***************************************/
    mViscousElasticProp::mViscousElasticProp(QWidget* parent)
        :mMaterialPropBase(MAT_Viscoelasticity, "粘弹性材料", parent)
    {
        initInputs();
    }

    mViscousElasticProp::~mViscousElasticProp()
    {
        clearInputs();
    }

    void mViscousElasticProp::initInputs()
    {
        QStringList names1 = {
            "杨氏模量    ",
            "泊松比      "
        };
        QVector<MxFunctions::MaterialPropType>proptypes1 = {
                MatProp_YoungsModulus,
                MatProp_Botson
        };
        QStringList names2 = {
            "g_i Prony  ",
            "k_i Prony  ",
            "tau_i Prony"
        };
        QVector<MxFunctions::MaterialPropType>proptypes2 = {
                   MatProp_GIProny,
                   MatProp_KIProny,
                   MatProp_TuaIProny
        };
        //初始化输入控件
        combobox_.initWidget("模量定义基准");
        combobox_.proptype_ = MatProp_ModulusTimeScale;
        combobox_.combobox->addItem("长期模量");
        combobox_.combobox->addItem("瞬时模量");
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mViscousElasticProp::slot_switchInputs);
        inputWidgets.clear();

        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names1[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names1[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes1[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }


        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names2[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names2[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes2[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets2.append(widgetSet);
        }

        lablebtn_.initWidget("粘弹性材料");
        lablebtn_.lablename->setText("时间Prony级数");
        lablebtn_.setPropType(MatProp_Curve_TimeDomain);
        lablebtn_.tablebtn->setDisabled(false);
        connect(lablebtn_.tablebtn, &QPushButton::clicked, this, [names2, this](int i) {
            QStringList header;
            for (int i = 0; i < names2.size(); ++i)
            {
                header << names2[i];
            }
            QString tabletitle = "粘弹性材料";
            QVariant value = lablebtn_.tablebtn->property("curve");
            //编辑表格
            mMaterialPropBase::tableEdit(lablebtn_.tablebtn->isChecked(), tabletitle, value, header);
            // 设置列校验规则
            {
                QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                for (int col = 0; col < header.size(); ++col)
                {
                    vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                }
                mTableDataWindow::getInstance()->setColumnValidations(vaMap);
            }
            //打开表格
            mDockLayoutManager::getInstance()->activateDock("Table");
            });

        // TRS 定义类型下拉框
        combobox_trs_.initWidget("时温等效");
        combobox_trs_.proptype_ = MatProp_TRSType;
        combobox_trs_.combobox->addItem("无");
        combobox_trs_.combobox->addItem("WLF");
        combobox_trs_.combobox->addItem("Arrhenius");
        connect(combobox_trs_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mViscousElasticProp::slot_switchInputs);

        // WLF 参数
        QStringList names_wlf = {
            "参考温度 THETA0",
            "校准常数 C1",
            "校准常数 C2"
        };
        QVector<MxFunctions::MaterialPropType> proptypes_wlf = {
            MatProp_Theta0,
            MatProp_C1,
            MatProp_C2
        };
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names_wlf[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names_wlf[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_wlf[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets_trs_wlf_.append(widgetSet);
        }

        // Arrhenius 参数
        QStringList names_arr = {
            "参考温度 THETA0",
            "活化能 E0"
        };
        QVector<MxFunctions::MaterialPropType> proptypes_arr = {
            MatProp_Theta0,
            MatProp_E0
        };
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names_arr[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names_arr[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes_arr[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets_trs_arr_.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mViscousElasticProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "粘弹性材料")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        lablebtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        emit sg_editFinished();
    }

    void mViscousElasticProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mViscousElasticProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mViscousElasticProp::slot_connTable);
    }

    void mViscousElasticProp::slot_switchInputs(int idx)
    {
        if (!this)
            return;
        this->removeAll();
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
            widgetSet.propwid->show();
        }
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        /* for (auto& widgetSet : inputWidgets2)
         {
             this->addRow(widgetSet.propwid);
             widgetSet.propwid->show();
         }*/
        this->addRow(lablebtn_.propwid);
        lablebtn_.propwid->show();

        this->addRow(combobox_trs_.propwid);
        combobox_trs_.propwid->show();
        if (idx == 1)
        {
            for (auto& widgetSet : inputWidgets_trs_wlf_)
            {
                this->addRow(widgetSet.propwid);
                widgetSet.propwid->show();
            }
        }
        else if (idx == 2)
        {
            for (auto& widgetSet : inputWidgets_trs_arr_)
            {
                this->addRow(widgetSet.propwid);
                widgetSet.propwid->show();
            }
        }

        this->refreshLayout();
        //emit sg_editFinished();
    }

    void mViscousElasticProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgets2)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgets_trs_wlf_)
        {
            widgetSet.clear();
        }
        for (auto& widgetSet : inputWidgets_trs_arr_)
        {
            widgetSet.clear();
        }
        lablebtn_.clear();
        inputWidgets.clear();
        inputWidgets2.clear();
        inputWidgets_trs_wlf_.clear();
        inputWidgets_trs_arr_.clear();
        combobox_.clear();
        combobox_trs_.clear();
    }

    void mViscousElasticProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets2)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets_trs_wlf_)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : inputWidgets_trs_arr_)
        {
            inp.lineeditvalue->setExpression("");
        }
        lablebtn_.tablebtn->setProperty("curve", QVariant());
    }

    void mViscousElasticProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(stressunit);
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mViscousElasticProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mViscousElasticProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        collectProps(props, combobox_trs_.proptype_, combobox_trs_.combobox->currentIndex());
        collectProps(props, lablebtn_);

        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_YoungsModulus, ratio} };
        collectProps(props, inputWidgets, ratios);

        //collectProps(props, inputWidgets2);
        int trsIdx = combobox_trs_.combobox->currentIndex();
        if (trsIdx == 1)
        {
            collectProps(props, inputWidgets_trs_wlf_);
        }
        else if (trsIdx == 2)
        {
            collectProps(props, inputWidgets_trs_arr_);
        }

        return props;
    }

    void mViscousElasticProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_ModulusTimeScale))
        {
            int i = prop[MatProp_ModulusTimeScale].toInt();
            combobox_.combobox->blockSignals(true);
            combobox_.combobox->setCurrentIndex(i);
            combobox_.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_TRSType))
        {
            int i = prop[MatProp_TRSType].toInt();
            combobox_trs_.combobox->blockSignals(true);
            combobox_trs_.combobox->setCurrentIndex(i);
            combobox_trs_.combobox->blockSignals(false);
            slot_switchInputs(i);
        }
        if (prop.contains(MatProp_YoungsModulus))
        {
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Curve_TimeDomain))
        {
            /*for (auto inp : inputWidgets2)
            {
                inp.lineeditvalue->setExpression("");
            }*/
            QVariant data = prop[MatProp_Curve_TimeDomain];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                lablebtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves.curves));
            }
        }
        for (auto inp : inputWidgets_trs_wlf_)
        {
            if (prop.contains(inp.proptype_))
            {
                QVariant data = prop[inp.proptype_];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inp.lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }
        for (auto inp : inputWidgets_trs_arr_)
        {
            if (prop.contains(inp.proptype_))
            {
                QVariant data = prop[inp.proptype_];
                if (data.isValid())
                {
                    double v = data.toDouble();
                    inp.lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                }
            }
        }
        updatePropUnit();
    }

    void mViscousElasticProp::InitFoldSection()
    {
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        this->addRow(combobox_.propwid);
        /*for (auto& widgetSet : inputWidgets2)
        {
            this->addRow(widgetSet.propwid);
        }*/
        this->addRow(lablebtn_.propwid);

        this->addRow(combobox_trs_.propwid);
        int trsIdx = combobox_trs_.combobox->currentIndex();
        if (trsIdx == 1)
        {
            for (auto& widgetSet : inputWidgets_trs_wlf_)
            {
                this->addRow(widgetSet.propwid);
            }
        }
        else if (trsIdx == 2)
        {
            for (auto& widgetSet : inputWidgets_trs_arr_)
            {
                this->addRow(widgetSet.propwid);
            }
        }

        return;
    }

    /***********************************(准)脆性材料***************************************/
    mBrittleLikeProp::mBrittleLikeProp(QWidget* parent)
        :mMaterialPropBase(MAT_HardBrittle, "(准)脆性材料", parent)
    {
        initInputs();
    }

    mBrittleLikeProp::~mBrittleLikeProp()
    {
        clearInputs();
    }

    void mBrittleLikeProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  ",
            "抗拉强度",
            "格里菲斯能量释放率",
            "正则化参数长度"
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson,
            MatProp_StrengthOfExtension,
            MatProp_GriffithsAbilityActivationRate,
            MatProp_RegularizationParameterLength,
        };
        //初始化输入控件
        combobox_1.initWidget("模型");
        combobox_1.combobox->addItem("PFAT1");
        combobox_1.combobox->addItem("PFAT2");
        combobox_1.combobox->addItem("PFCZM");
        combobox_1.proptype_ = MatProp_Model;
        combobox_2.initWidget("软化曲线");
        combobox_2.combobox->addItem("线性软化曲线");
        combobox_2.combobox->addItem("指数软化曲线");
        combobox_2.combobox->addItem("blinear软化曲线");
        combobox_2.combobox->addItem("混泥土软化曲线");
        combobox_2.combobox->addItem("双曲线软化曲线");
        combobox_2.proptype_ = MatProp_SofteningCurve;
        combobox_1.labelname->setFixedWidth(115);
        combobox_2.labelname->setFixedWidth(115);
        inputWidgets.clear();

        for (int i = 0; i < 5; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            // 设置文本
            widgetSet.labelname->setFixedWidth(110);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        connect(combobox_1.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBrittleLikeProp::slot_switchInputs1);
        connect(combobox_2.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBrittleLikeProp::slot_switchInputs2);
        updatePropUnit();
        InitFoldSection();
    }

    void mBrittleLikeProp::slot_switchInputs1(int i)
    {
        this->removeAll();
        for (int i = 0; i < 4; ++i)
        {
            this->addRow(inputWidgets[i].propwid);
            inputWidgets[i].propwid->show();
        }
        this->addRow(combobox_1.propwid);
        combobox_1.propwid->show();
        if (i == 2)
        {
            this->addRow(combobox_2.propwid);
            combobox_2.propwid->show();
        }
        this->addRow(inputWidgets[4].propwid);
        inputWidgets[4].propwid->show();

        this->refreshLayout();
        emit sg_editFinished();

    }

    void mBrittleLikeProp::slot_switchInputs2(int i)
    {
        emit sg_editFinished();
    }

    void mBrittleLikeProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        combobox_1.clear();
        combobox_2.clear();

    }

    void mBrittleLikeProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mBrittleLikeProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString energyunit = UnitsLibrary[(int)Unit::SurfaceEnergyDensity][sysunitidxs[0]];
        QString lengthunit = UnitsLibrary[(int)Unit::Length][sysunitidxs[0]];

        //inputWidgets[0].labelunit->setText(stressunit);
        //inputWidgets[2].labelunit->setText(stressunit);
        //inputWidgets[3].labelunit->setText(energyunit);
        //inputWidgets[4].labelunit->setText(lengthunit);

        inputWidgets[0].lineeditvalue->setUnit(stressunit);
        inputWidgets[2].lineeditvalue->setUnit(stressunit);
        inputWidgets[3].lineeditvalue->setUnit(energyunit);
        inputWidgets[4].lineeditvalue->setUnit(lengthunit);
    }

    void mBrittleLikeProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mBrittleLikeProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::SurfaceEnergyDensity, sysunitidxs, { 3,0,0,0 });
        double ratio3 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{
            {MatProp_YoungsModulus, ratio1},
            {MatProp_StrengthOfExtension, ratio1},
            {MatProp_GriffithsAbilityActivationRate, ratio2},
            {MatProp_RegularizationParameterLength, ratio3},
        };
        collectProps(props, inputWidgets, ratios);

        collectProps(props, combobox_1.proptype_, combobox_1.combobox->currentIndex());
        if (combobox_1.combobox->currentText() == "PFCZM")
        {
            collectProps(props, combobox_2.proptype_, combobox_2.combobox->currentIndex());
        }
        return props;
    }

    void mBrittleLikeProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        auto mgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QHash<MxFunctions::MaterialPropType, double> ratios{
        {MatProp_YoungsModulus,                  mgr->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, {3,0,0,0})},
        {MatProp_StrengthOfExtension,            mgr->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, {3,0,0,0})},
        {MatProp_GriffithsAbilityActivationRate, mgr->getUnitSystemFromToRatio(Unit::SurfaceEnergyDensity, sysunitidxs, {3,0,0,0})},
        {MatProp_RegularizationParameterLength,  mgr->getUnitSystemFromToRatio(Unit::Length, sysunitidxs, {3,0,0,0})},
        };

        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = prop[it.key()];
            if (data.isValid())
            {
                double r = ratios.value(it.key(), 1.0);
                inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / r));
            }
        }
        if (prop.contains(MatProp_Model))
        {
            int d = prop[MatProp_Model].toInt();
            combobox_1.combobox->blockSignals(true);
            combobox_1.combobox->setCurrentIndex(d);
            combobox_1.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_SofteningCurve))
        {
            int d = prop[MatProp_SofteningCurve].toInt();
            combobox_2.combobox->blockSignals(true);
            combobox_2.combobox->setCurrentIndex(d);
            combobox_2.combobox->blockSignals(false);
        }
        updatePropUnit();
    }

    void mBrittleLikeProp::InitFoldSection()
    {
        slot_switchInputs1(combobox_1.combobox->currentIndex());
        for (int i = 0; i < 4; ++i)
        {
            this->addRow(inputWidgets[i].propwid);
        }
        this->addRow(combobox_1.propwid);
        if (combobox_1.combobox->currentText() == "PFCZM")
        {
            this->addRow(combobox_2.propwid);
        }
        this->addRow(inputWidgets[4].propwid);
        return;
    }

    /***********************************压电材料***************************************/
    mPiezoelectricProp::mPiezoelectricProp(QWidget* parent)
        :mMaterialPropBase(MAT_Piezoelectric, "压电材料", parent)
    {
        initInputs();
    }

    mPiezoelectricProp::~mPiezoelectricProp()
    {
        clearInputs();
    }

    void mPiezoelectricProp::initInputs()
    {
        QStringList names = {
            "介电矩阵",
            "压电矩阵",
        };
        //初始化输入控件
        combobox_1.initWidget("弹性材料");
        combobox_1.proptype_ = MatProp_ElasticMaterial;
        auto allmats = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getAllMatID();
        for (auto matid : allmats)
        {
            QString matname = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(matid);
            matname2id_[matname] = matid;
            combobox_1.combobox->addItem(matname);
        }
        combobox_1.combobox->setCurrentIndex(0);
        combobox_2.initWidget("介电类型");
        combobox_2.proptype_ = MatProp_DielectricType;
        combobox_2.combobox->addItem("各向同性");
        combobox_2.combobox->addItem("正交各向异性");
        combobox_2.combobox->addItem("各向异性");
        combobox_3.initWidget("压电类型");
        combobox_3.proptype_ = MatProp_PiezoelectricType;
        combobox_3.combobox->addItem("应力");
        combobox_3.combobox->addItem("应变");

        //介电常数
        InputWidgetSet widgetSet;
        widgetSet.initWidget("介电常数", false, MaterialPropType::Unknown, s_nameToCalcMode.value("介电常数", CalcMode::Standard));
        widgetSet.labelname->setText("介电常数");
        widgetSet.proptype_ = MatProp_DielectricConstant;
        connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this, widgetSet]() {
                emit sg_editFinished();
            });
        inputWidgets.append(widgetSet);

        //矩阵
        for (int i = 0; i < 2; ++i)
        {
            singleTableWidget tableWidget;

            // 初始化单个输入组
            tableWidget.initWidget(names[i]);
            tableWidget.tablewidget->setSizeMode(mTableWidget::SizeMode::FixedSize);
            connect(tableWidget.tablewidget, &QTableWidget::cellChanged,
                this, [this, tableWidget](int row, int column) {
                    QTableWidgetItem* item = tableWidget.tablewidget->item(row, column);
                    if (item)
                    {
                        emit sg_editFinished();
                    }
                });
            // 设置文本
            tableWidget.tablewidget->setColumnCount(3);
            for (int c = 0; c < 3; ++c)
            {
                tableWidget.tablewidget->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
            }
            if (i == 0)
            {
                tableWidget.tablewidget->setRowCount(3);
            }
            else
            {
                tableWidget.tablewidget->setRowCount(6);
            }
            for (int row = 0; row < tableWidget.tablewidget->rowCount(); ++row)
            {
                for (int col = 0; col < tableWidget.tablewidget->columnCount(); ++col)
                {
                    tableWidget.tablewidget->setItem(row, col, new QTableWidgetItem(""));
                }
            }
            singleTableWidgets.append(tableWidget);
        }
        singleTableWidgets[0].proptype_ = MatProp_DielectricMatrix;
        singleTableWidgets[1].proptype_ = MatProp_PiezoelectricMatrix;


        connect(combobox_1.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPiezoelectricProp::slot_switchInputs1);
        connect(combobox_2.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPiezoelectricProp::slot_switchInputs2);
        connect(combobox_3.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPiezoelectricProp::slot_switchInputs3);
    }

    void mPiezoelectricProp::slot_switchInputs2(int i)
    {
        this->removeAll();
        this->addRow(combobox_1.propwid);
        this->addRow(combobox_2.propwid);
        combobox_1.propwid->show();
        combobox_2.propwid->show();
        if (i == 0)
        {
            this->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
        }
        else
        {
            this->addRow(singleTableWidgets[0].propwid);
            singleTableWidgets[0].propwid->show();
        }
        this->addRow(combobox_3.propwid);
        combobox_3.propwid->show();
        this->addRow(singleTableWidgets[1].propwid);
        singleTableWidgets[1].propwid->show();

        this->refreshLayout();
        emit sg_editFinished();

    }

    void mPiezoelectricProp::slot_switchInputs1(int i)
    {
        emit sg_editFinished();
    }

    void mPiezoelectricProp::slot_switchInputs3(int i)
    {
        emit sg_editFinished();
    }

    void mPiezoelectricProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        combobox_1.clear();
        combobox_2.clear();
        combobox_3.clear();
        for (auto& tableWidget : singleTableWidgets)
        {
            tableWidget.clear();
        }
    }

    void mPiezoelectricProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        singleTableWidgets[0].clearData(3, 3);
        singleTableWidgets[1].clearData(6, 3);
    }

    void mPiezoelectricProp::updatePropUnit()
    {

    }

    QHash<MxFunctions::MaterialPropType, QVariant> mPiezoelectricProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_1.proptype_, combobox_1.combobox->currentText());
        collectProps(props, combobox_2.proptype_, combobox_2.combobox->currentIndex());
        if (combobox_2.combobox->currentIndex() == 0)
        {
            collectProps(props, inputWidgets);
        }
        else
        {
            collectProps(props, singleTableWidgets[0]);
        }
        collectProps(props, combobox_3.proptype_, combobox_3.combobox->currentIndex());
        collectProps(props, singleTableWidgets[1]);
        return props;
    }

    void mPiezoelectricProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        //重载弹性材料
        matname2id_.clear();
        combobox_1.combobox->clear();
        QVector<MaterialClass>mattypes = {
            MAT_Isotropy,
            MAT_Orthotropy,
            MAT_Anisotropy,
            /*MAT_IsotropicThermalMaterial,*/
        };
        for (int i = 0; i < mattypes.size(); ++i)
        {
            auto matids = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getAllMatID();
            for (auto id : matids)
            {
                QString name = getCVM->getModelData()->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMatNameByID(id);
                combobox_1.combobox->addItem(name);
                matname2id_[name] = id;
            }
        }
        if (prop.contains(MatProp_ElasticMaterial))
        {
            QVariant data = prop[MatProp_ElasticMaterial];
            if (data.isValid())
            {
                QString matstr = data.value<QString>();
                for (auto id = matname2id_.begin(); id != matname2id_.end(); id++)
                {
                    if (matstr == id.key())
                    {
                        combobox_1.combobox->blockSignals(true);
                        combobox_1.combobox->setCurrentIndex(id.value());
                        combobox_1.combobox->blockSignals(false);
                        break;
                    }
                }
            }
        }
        if (prop.contains(MatProp_DielectricType))
        {
            int type = prop[MatProp_DielectricType].toInt();
            combobox_2.combobox->blockSignals(true);
            combobox_2.combobox->setCurrentIndex(type);
            combobox_2.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_PiezoelectricType))
        {
            int type = prop[MatProp_PiezoelectricType].toInt();
            combobox_3.combobox->blockSignals(true);
            combobox_3.combobox->setCurrentIndex(type);
            combobox_3.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_DielectricConstant))
        {
            QVariant data = prop[MatProp_DielectricConstant];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_DielectricMatrix))
        {
            QVariant data = prop[MatProp_DielectricMatrix];
            if (data.isValid())
            {
                QVector<QVector<double>> v = data.value<QVector<QVector<double>>>();
                singleTableWidgets[0].setValue(v);
            }
        }
        if (prop.contains(MatProp_PiezoelectricMatrix))
        {
            QVariant data = prop[MatProp_PiezoelectricMatrix];
            if (data.isValid())
            {
                QVector<QVector<double>> v = data.value<QVector<QVector<double>>>();
                singleTableWidgets[1].setValue(v);
            }
        }
        updatePropUnit();
    }

    void mPiezoelectricProp::InitFoldSection()
    {
        QVector<QWidget*>wds;

        connect(combobox_2.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPiezoelectricProp::slot_switchInputs2);

        combobox_2.combobox->setCurrentIndex(combobox_2.combobox->currentIndex());
        //载入弹性材料
        QVector<MaterialType>mattypes = {
            LinearElastic_Isotropic,
            LinearElastic_Orthotropic,
            LinearElastic_Anisotropic,
            /*MAT_IsotropicThermalMaterial,*/
        };
        QStringList elasticitymat;
        for (int i = 0; i < mattypes.size(); ++i)
        {
            elasticitymat.append(getCVM->getModelData()->
                getManager<MechMaterialData::mMaterialManager>(Physical_Material).getAllMatNamesByTypes(mattypes[i]));
        }
        combobox_1.combobox->clear();
        for (auto mt : elasticitymat)
        {
            combobox_1.combobox->addItem(mt);
        }

        this->addRow(combobox_1.propwid);
        this->addRow(combobox_2.propwid);
        if (combobox_2.combobox->currentIndex() == 0)
        {
            this->addRow(inputWidgets[0].propwid);
        }
        else
        {
            this->addRow(singleTableWidgets[0].propwid);
        }
        this->addRow(combobox_3.propwid);
        this->addRow(singleTableWidgets[1].propwid);
        return;
    }

    /***********************************纤维增强材料***************************************/
    mFiberReinforcedProp::mFiberReinforcedProp(QWidget* parent)
        :mMaterialPropBase(MAT_Elasticfiber, "纤维增强材料", parent)
    {
        initInputs();
    }

    mFiberReinforcedProp::~mFiberReinforcedProp()
    {
        clearInputs();
    }

    void mFiberReinforcedProp::initInputs()
    {
        QStringList names = {
            "C10",
            "D1 ",
            "nx1",
            "ny1",
            "k11",
            "k21",
            "nx2",
            "ny2",
            "k12",
            "k22",
            "nx3",
            "ny3",
            "k13",
            "k23",
            "nx4",
            "ny4",
            "k14",
            "k24",
        };
        QVector<MaterialPropType>proptypes = {
            MatProp_C10,
            MatProp_D1,
            MatProp_Nx1,
            MatProp_Ny1,
            MatProp_K11,
            MatProp_K21,
            MatProp_Nx2,
            MatProp_Ny2,
            MatProp_K12,
            MatProp_K22,
            MatProp_Nx3,
            MatProp_Ny3,
            MatProp_K13,
            MatProp_K23,
            MatProp_Nx4,
            MatProp_Ny4,
            MatProp_K14,
            MatProp_K24,
        };

        //初始化输入控件
        combobox_.initWidget("坐标系");
        combobox_.proptype_ = MatProp_Coordinate;
        combobox_.combobox->addItem("全局坐标系");
        combobox_.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        connect(combobox_.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();
            });
        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mFiberReinforcedProp::slot_coorChanged);

        spinbox_.initWidget("纤维层数");
        spinbox_.proptype_ = MatProp_NumberOfFiberLayers;
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(4);

        for (int i = 0; i < 18; ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            widgetSet.labelname->setText(names[i]);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp.append(widgetSet);
            if (i < 6)
            {
                inputWidgets.append(widgetSet);
            }
        }
        checkboxtemp_.initWidget("纤维增强材料");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Elasticfiber);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        // 曲线参数列随纤维层数变化（C10、D1 + 每层 nx/ny/k1/k2），
        // 初始按当前 inputWidgets 生成，slot_layerChanged 中经 refreshCurveHeaders 更新
        QStringList paramHeaders;
        for (auto inp : inputWidgets)
        {
            paramHeaders << inp.name_.trimmed();
        }
        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-纤维增强材料参数曲线"),
            QStringLiteral("温度 / °C"),
            paramHeaders,
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            spinbox_.setDisableAll(i != 0);
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        //连接spinbox信号
        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mFiberReinforcedProp::slot_layerChanged);

        updatePropUnit();
    }

    void mFiberReinforcedProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "纤维增强材料")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mFiberReinforcedProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFiberReinforcedProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFiberReinforcedProp::slot_connTable);
    }

    void mFiberReinforcedProp::refreshCurveHeaders()
    {
        if (!m_tempCurveRow) return;
        QStringList params;
        for (auto inp : inputWidgets)
        {
            params << inp.name_.trimmed();
        }
        m_tempCurveRow->setCurveHeaders(QStringLiteral("温度-纤维增强材料参数曲线"),
            QStringLiteral("温度 / °C"), params);
    }


    void mFiberReinforcedProp::slot_layerChanged(int value)
    {
        if (value == cnts)
        {
            return;
        }
        this->removeAll();
        this->addRow(combobox_.propwid);
        this->addRow(spinbox_.propwid);
        combobox_.propwid->show();
        spinbox_.propwid->show();
        inputWidgets.clear();
        inputWidgets.append(inputWidgetsTmp[0]);
        inputWidgets.append(inputWidgetsTmp[1]);
        this->addRow(inputWidgetsTmp[0].propwid);
        this->addRow(inputWidgetsTmp[1].propwid);
        inputWidgetsTmp[0].propwid->show();
        inputWidgetsTmp[1].propwid->show();
        for (int i = 0; i < 4 * value; ++i)
        {
            inputWidgets.append(inputWidgetsTmp[2 + i]);
            this->addRow(inputWidgetsTmp[2 + i].propwid);
            inputWidgetsTmp[2 + i].propwid->show();
        }
        // 层数变化后曲线参数列数变化：先刷新表头，再入布局重排
        refreshCurveHeaders();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        this->refreshLayout();
        cnts = value;
    }

    void mFiberReinforcedProp::slot_coorChanged(int i)
    {
        emit sg_editFinished();
    }

    void mFiberReinforcedProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        combobox_.clear();
        spinbox_.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    void mFiberReinforcedProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mFiberReinforcedProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit1 = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString unit2 = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        //inputWidgets[0].labelunit->setText(unit1);
        //inputWidgets[1].labelunit->setText(unit2);
        inputWidgets[0].lineeditvalue->setUnit(unit1);
        inputWidgets[1].lineeditvalue->setUnit(unit2);
    }

    void mFiberReinforcedProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mFiberReinforcedProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, combobox_.proptype_, combobox_.combobox->currentText());
        collectProps(props, spinbox_.proptype_, spinbox_.spinbox->value());
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_C10, ratio1},
                {MatProp_D1, ratio2}
            };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mFiberReinforcedProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        combobox_.combobox->clear();
        auto coorids = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto name : coorids)
        {
            combobox_.combobox->addItem(name);
        }
        if (prop.contains(MatProp_NumberOfFiberLayers))
        {
            int layers = prop[MatProp_NumberOfFiberLayers].toInt();
            spinbox_.spinbox->blockSignals(true);
            spinbox_.spinbox->setValue(layers);
            spinbox_.spinbox->blockSignals(false);
        }
        if (prop.contains(MatProp_Coordinate))
        {
            QString coorname = prop[MatProp_Coordinate].value<QString>();
            combobox_.combobox->setCurrentText(coorname);
        }
        if (prop.contains(MatProp_Curve_Elasticfiber))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Elasticfiber];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        else
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            for (auto it = prop.begin(); it != prop.end(); it++)
            {
                if (!type2inp.contains(it.key()))
                    continue;
                QVariant data = prop[it.key()];
                if (data.isValid())
                {
                    inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio1));
                }
            }
        }
        updatePropUnit();
    }

    void mFiberReinforcedProp::InitFoldSection()
    {
        QVector<QWidget*>wds;

        this->addRow(combobox_.propwid);
        combobox_.combobox->clear();
        QStringList cornames = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto cn : cornames)
        {
            combobox_.combobox->addItem(cn);
        }

        this->addRow(spinbox_.propwid);
        spinbox_.spinbox->setValue(spinbox_.spinbox->value());

        int layers = spinbox_.spinbox->value();
        for (int i = 0; i < 2 + 4 * layers; ++i)
        {
            this->addRow(inputWidgets[i].propwid);
        }
        // 温度相关行由封装控件承载（列随层数经 refreshCurveHeaders 更新）
        refreshCurveHeaders();
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        return;
    }

    ///***********************************通用***************************************/

    mMatFromLibCommon::mMatFromLibCommon(QWidget* parent)
        :mMaterialPropBase(MAT_LibCommon, "通用", parent)
    {
        initInputs();
    }

    mMatFromLibCommon::~mMatFromLibCommon()
    {
        clearInputs();
    }

    void mMatFromLibCommon::initInputs()
    {

    }

    void mMatFromLibCommon::updatePropUnit()
    {

    }

    void mMatFromLibCommon::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = mgr->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            int unitidx = 0;
            if (inp->unit_ == Unit::Angle)
                unitidx = 1;
            else if (inp->unit_ == Unit::Speed)
                unitidx = 2;
            else if (inp->unit_ == Unit::Temp || inp->unit_ == Unit::TempReciprocal)
                unitidx = 3;
            double ratio = mgr->getSwitchUnitSystemRatio(inp->unit_);   //旧→新乘数
            inp->lineeditvalue->setUnit(UnitsLibrary[(int)inp->unit_][sysunitidxs[unitidx]]);
            QString text = inp->lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok = false;
                double v = text.toDouble(&ok);
                if (ok)
                    inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio));
            }
        }
        if (m_matData)
        {
            auto all = m_matData->getAllProps();
            all[getPropertyClass()].unite(collectProperties());
            m_matData->setProperty(all);
        }
    }

    void mMatFromLibCommon::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet->clear();
            delete widgetSet;
            widgetSet = nullptr;
        }
        inputWidgets.clear();
        for (auto& btn : curvebtns)
        {
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
        }
        curvebtns.clear();
    }

    QHash<QString, QPair<QVector<Unit>, QVariant>> mMatFromLibCommon::collectLibProperties()
    {
        QHash<QString, QPair<QVector<Unit>, QVariant>>props;
        auto unitMgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            Unit unit = inp->unit_;
            QString text = inp->lineeditvalue->text().trimmed();
            QString name = inp->name_;
            QVariant value;

            if (!text.isEmpty())
            {
                bool ok;
                double uiLocalVal = text.toDouble(&ok);
                if (ok)
                {
                    // ratio = SI → 界面本地单位
                    double ratio = unitMgr->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                    double siValue = uiLocalVal / ratio; // 关键：还原为国际单位

                    value = QVariant::fromValue<double>(siValue);
                }
                else
                {
                    value = QVariant();
                }
                props[name] = { {unit}, value };
            }
        }
        props.unite(curves_);
        return props;
    }

    void mMatFromLibCommon::setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop)
    {
        int cnt = prop.size();
        if (cnt == 0)
        {
            return;
        }
        clearInputs();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto p = prop.begin(); p != prop.end(); p++)
        {
            QVariant value = p.value().second;
            // 尝试转换为 double
            if (value.canConvert<double>())
            {
                InputWidgetSet* inp = new InputWidgetSet;
                QString name = p.key();
                Unit unit = p.value().first[0];
                inp->initWidget(name, false);
                inp->labelname->setFixedWidth(90);
                int unitidx = 0;
                if (unit == Unit::Angle)
                {
                    unitidx = 1;
                }
                else if (unit == Unit::Speed)
                {
                    unitidx = 2;
                }
                else if (unit == Unit::Temp || unit == Unit::TempReciprocal)
                {
                    unitidx = 3;
                }
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                inp->unit_ = unit;
                //inp->labelunit->setText(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setUnit(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(value.toDouble() * ratio));
                connect(inp->lineeditvalue, &mCalcLineEdit::textChanged,
                    [this, inp]() {
                        emit sg_editFinished();
                    });
                inputWidgets.append(inp);
            }
            // 否则尝试转换为 QVector<QVector<double>>（二维数组）
            else if (value.canConvert<QVector<QVector<double>>>())
            {
                curves_[p.key()] = p.value();
                QVector<QVector<double>> data = value.value<QVector<QVector<double>>>();
                QString name = p.key();
                // 温度相关曲线改用吸附式弹窗行（标签 + 入口按钮），不再占用停靠表；
                // 编辑产生的列主序曲线（首列温度）经回调写回 curves_ 并触发保存
                QWidget* row = createLibCurveRow(name, data,
                    [this, name](const QVector<QVector<double>>& curves) {
                        curves_[name].second = QVariant::fromValue(curves);
                        emit sg_editFinished();
                    });
                curvebtns.append(row);
            }
        }
        InitFoldSection();
        updatePropUnit();
    }

    void mMatFromLibCommon::InitFoldSection()
    {
        this->removeAll();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp->propwid);
        }
        for (auto btn : curvebtns)
        {
            this->addRow(btn);
        }
        return;
    }


    /***********************************结构***************************************/
    mMatFromLibStructural::mMatFromLibStructural(QWidget* parent)
        :mMaterialPropBase(MAT_LibStructural, "结构", parent)
    {
        initInputs();
    }

    mMatFromLibStructural::~mMatFromLibStructural()
    {
        clearInputs();
    }

    void mMatFromLibStructural::initInputs()
    {

    }

    void mMatFromLibStructural::updatePropUnit()
    {

    }

    void mMatFromLibStructural::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = mgr->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            int unitidx = 0;
            if (inp->unit_ == Unit::Angle)
                unitidx = 1;
            else if (inp->unit_ == Unit::Speed)
                unitidx = 2;
            else if (inp->unit_ == Unit::Temp || inp->unit_ == Unit::TempReciprocal)
                unitidx = 3;
            double ratio = mgr->getSwitchUnitSystemRatio(inp->unit_);   //旧→新乘数
            inp->lineeditvalue->setUnit(UnitsLibrary[(int)inp->unit_][sysunitidxs[unitidx]]);
            QString text = inp->lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok = false;
                double v = text.toDouble(&ok);
                if (ok)
                    inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio));
            }
        }
        if (m_matData)
        {
            auto all = m_matData->getAllProps();
            all[getPropertyClass()].unite(collectProperties());
            m_matData->setProperty(all);
        }
    }

    void mMatFromLibStructural::clearInputs()
    {
        for (auto widgetSet : inputWidgets)
        {
            widgetSet->clear();
            delete widgetSet;
            widgetSet = nullptr;
        }
        inputWidgets.clear();
        for (auto& btn : curvebtns)
        {
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
        }
        curvebtns.clear();
        curves_.clear();
    }

    QHash<QString, QPair<QVector<Unit>, QVariant>> mMatFromLibStructural::collectLibProperties()
    {
        QHash<QString, QPair<QVector<Unit>, QVariant>>props;
        auto unitMgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            Unit unit = inp->unit_;
            QString text = inp->lineeditvalue->text().trimmed();
            QString name = inp->name_;
            QVariant value;

            if (!text.isEmpty())
            {
                bool ok;
                double uiLocalVal = text.toDouble(&ok);
                if (ok)
                {
                    // ratio = SI → 界面本地单位
                    double ratio = unitMgr->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                    double siValue = uiLocalVal / ratio; // 关键：还原为国际单位

                    value = QVariant::fromValue<double>(siValue);
                }
                else
                {
                    value = QVariant();
                }
                props[name] = { {unit}, value };
            }
        }
        props.unite(curves_);
        return props;
    }

    void mMatFromLibStructural::setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop)
    {
        int cnt = prop.size();
        if (cnt == 0)
        {
            return;
        }
        clearInputs();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto p = prop.begin(); p != prop.end(); p++)
        {
            QVariant value = p.value().second;
            // 尝试转换为 double
            if (value.canConvert<double>())
            {
                InputWidgetSet* inp = new InputWidgetSet;
                QString name = p.key();
                Unit unit = p.value().first[0];
                inp->initWidget(name, false);
                inp->labelname->setFixedWidth(90);
                int unitidx = 0;
                if (unit == Unit::Angle)
                {
                    unitidx = 1;
                }
                else if (unit == Unit::Speed)
                {
                    unitidx = 2;
                }
                else if (unit == Unit::Temp || unit == Unit::TempReciprocal)
                {
                    unitidx = 3;
                }
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                inp->unit_ = unit;
                //inp->labelunit->setText(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setUnit(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(value.toDouble() * ratio));
                connect(inp->lineeditvalue, &mCalcLineEdit::textChanged,
                    [this, inp]() {
                        emit sg_editFinished();
                    });
                inputWidgets.append(inp);
            }
            // 否则尝试转换为 QVector<QVector<double>>（二维数组）
            else if (value.canConvert<QVector<QVector<double>>>())
            {
                curves_[p.key()] = p.value();
                QVector<QVector<double>> data = value.value<QVector<QVector<double>>>();
                QString name = p.key();
                // 温度相关曲线改用吸附式弹窗行（标签 + 入口按钮），不再占用停靠表；
                // 编辑产生的列主序曲线（首列温度）经回调写回 curves_ 并触发保存
                QWidget* row = createLibCurveRow(name, data,
                    [this, name](const QVector<QVector<double>>& curves) {
                        curves_[name].second = QVariant::fromValue(curves);
                        emit sg_editFinished();
                    });
                curvebtns.append(row);
            }
        }
        InitFoldSection();
        updatePropUnit();
    }

    void mMatFromLibStructural::InitFoldSection()
    {
        this->removeAll();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp->propwid);
        }
        for (auto btn : curvebtns)
        {
            this->addRow(btn);
        }
        return;
    }


    /***********************************传热***************************************/
    mMatFromLibThermal::mMatFromLibThermal(QWidget* parent)
        :mMaterialPropBase(MAT_LibThermal, "传热", parent)
    {
        initInputs();
    }

    mMatFromLibThermal::~mMatFromLibThermal()
    {
        clearInputs();
    }

    void mMatFromLibThermal::initInputs()
    {

    }

    void mMatFromLibThermal::updatePropUnit()
    {

    }

    void mMatFromLibThermal::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = mgr->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            int unitidx = 0;
            if (inp->unit_ == Unit::Angle)
                unitidx = 1;
            else if (inp->unit_ == Unit::Speed)
                unitidx = 2;
            else if (inp->unit_ == Unit::Temp || inp->unit_ == Unit::TempReciprocal)
                unitidx = 3;
            double ratio = mgr->getSwitchUnitSystemRatio(inp->unit_);   //旧→新乘数
            inp->lineeditvalue->setUnit(UnitsLibrary[(int)inp->unit_][sysunitidxs[unitidx]]);
            QString text = inp->lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok = false;
                double v = text.toDouble(&ok);
                if (ok)
                    inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio));
            }
        }
        if (m_matData)
        {
            auto all = m_matData->getAllProps();
            all[getPropertyClass()].unite(collectProperties());
            m_matData->setProperty(all);
        }
    }

    void mMatFromLibThermal::clearInputs()
    {
        for (auto widgetSet : inputWidgets)
        {
            widgetSet->clear();
            delete widgetSet;
            widgetSet = nullptr;
        }
        inputWidgets.clear();
        for (auto& btn : curvebtns)
        {
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
        }
        curvebtns.clear();
    }

    QHash<QString, QPair<QVector<Unit>, QVariant>> mMatFromLibThermal::collectLibProperties()
    {
        QHash<QString, QPair<QVector<Unit>, QVariant>>props;
        auto unitMgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            Unit unit = inp->unit_;
            QString text = inp->lineeditvalue->text().trimmed();
            QString name = inp->name_;
            QVariant value;

            if (!text.isEmpty())
            {
                bool ok;
                double uiLocalVal = text.toDouble(&ok);
                if (ok)
                {
                    // ratio = SI → 界面本地单位
                    double ratio = unitMgr->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                    double siValue = uiLocalVal / ratio; // 关键：还原为国际单位

                    value = QVariant::fromValue<double>(siValue);
                }
                else
                {
                    value = QVariant();
                }
                props[name] = { {unit}, value };
            }
        }
        props.unite(curves_);
        return props;
    }

    void mMatFromLibThermal::setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop)
    {
        int cnt = prop.size();
        if (cnt == 0)
        {
            return;
        }
        clearInputs();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto p = prop.begin(); p != prop.end(); p++)
        {
            QVariant value = p.value().second;
            // 尝试转换为 double
            if (value.canConvert<double>())
            {
                InputWidgetSet* inp = new InputWidgetSet;
                QString name = p.key();
                Unit unit = p.value().first[0];
                inp->initWidget(name, false);
                inp->labelname->setFixedWidth(90);
                int unitidx = 0;
                if (unit == Unit::Angle)
                {
                    unitidx = 1;
                }
                else if (unit == Unit::Speed)
                {
                    unitidx = 2;
                }
                else if (unit == Unit::Temp || unit == Unit::TempReciprocal)
                {
                    unitidx = 3;
                }
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                inp->unit_ = unit;
                //inp->labelunit->setText(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setUnit(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(value.toDouble() * ratio));
                connect(inp->lineeditvalue, &mCalcLineEdit::textChanged,
                    [this, inp]() {
                        emit sg_editFinished();
                    });
                inputWidgets.append(inp);
            }
            // 否则尝试转换为 QVector<QVector<double>>（二维数组）
            else if (value.canConvert<QVector<QVector<double>>>())
            {
                curves_[p.key()] = p.value();
                QVector<QVector<double>> data = value.value<QVector<QVector<double>>>();
                QString name = p.key();
                // 温度相关曲线改用吸附式弹窗行（标签 + 入口按钮），不再占用停靠表；
                // 编辑产生的列主序曲线（首列温度）经回调写回 curves_ 并触发保存
                QWidget* row = createLibCurveRow(name, data,
                    [this, name](const QVector<QVector<double>>& curves) {
                        curves_[name].second = QVariant::fromValue(curves);
                        emit sg_editFinished();
                    });
                curvebtns.append(row);
            }
        }
        InitFoldSection();
        updatePropUnit();
    }

    void mMatFromLibThermal::InitFoldSection()
    {
        this->removeAll();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp->propwid);
        }
        for (auto btn : curvebtns)
        {
            this->addRow(btn);
        }
        return;
    }

    /***********************************导电***************************************/
    mMatFromLibElectic::mMatFromLibElectic(QWidget* parent)
        :mMaterialPropBase(MAT_LibElectic, "导电", parent)
    {
    }

    mMatFromLibElectic::~mMatFromLibElectic()
    {
        clearInputs();
    }

    void mMatFromLibElectic::initInputs()
    {

    }

    void mMatFromLibElectic::updatePropUnit()
    {

    }

    void mMatFromLibElectic::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        auto* mgr = getCVM->getModelData()->getUnitSystemManager();
        QVector<int> sysunitidxs = mgr->getCurUnitsIndex();
        for (auto inp : inputWidgets)
        {
            int unitidx = 0;
            if (inp->unit_ == Unit::Angle)
                unitidx = 1;
            else if (inp->unit_ == Unit::Speed)
                unitidx = 2;
            else if (inp->unit_ == Unit::Temp || inp->unit_ == Unit::TempReciprocal)
                unitidx = 3;
            double ratio = mgr->getSwitchUnitSystemRatio(inp->unit_);   //旧→新乘数
            inp->lineeditvalue->setUnit(UnitsLibrary[(int)inp->unit_][sysunitidxs[unitidx]]);
            QString text = inp->lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok = false;
                double v = text.toDouble(&ok);
                if (ok)
                    inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(v * ratio));
            }
        }
        if (m_matData)
        {
            auto all = m_matData->getAllProps();
            all[getPropertyClass()].unite(collectProperties());
            m_matData->setProperty(all);
        }
    }

    void mMatFromLibElectic::clearInputs()
    {
        for (auto widgetSet : inputWidgets)
        {
            widgetSet->clear();
            delete widgetSet;
            widgetSet = nullptr;
        }
        inputWidgets.clear();
        for (auto& btn : curvebtns)
        {
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
        }
        curvebtns.clear();
    }


    QHash<QString, QPair<QVector<Unit>, QVariant>> mMatFromLibElectic::collectLibProperties()
    {
        QHash<QString, QPair<QVector<Unit>, QVariant>>props;
        collectProps(props, inputWidgets);
        props.unite(curves_);
        return props;
    }

    void mMatFromLibElectic::setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop)
    {
        int cnt = prop.size();
        if (cnt == 0)
        {
            return;
        }
        clearInputs();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        for (auto p = prop.begin(); p != prop.end(); p++)
        {
            QVariant value = p.value().second;
            // 尝试转换为 double
            if (value.canConvert<double>())
            {
                InputWidgetSet* inp = new InputWidgetSet;
                QString name = p.key();
                Unit unit = p.value().first[0];
                inp->initWidget(name, false);
                inp->labelname->setFixedWidth(90);
                int unitidx = 0;
                if (unit == Unit::Angle)
                {
                    unitidx = 1;
                }
                else if (unit == Unit::Speed)
                {
                    unitidx = 2;
                }
                else if (unit == Unit::Temp || unit == Unit::TempReciprocal)
                {
                    unitidx = 3;
                }
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(unit, { 0,0,0,1 }, sysunitidxs);
                inp->unit_ = unit;
                //inp->labelunit->setText(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setUnit(UnitsLibrary[(int)unit][sysunitidxs[unitidx]]);
                inp->lineeditvalue->setExpression(MxFunctions::mNumberToString(value.toDouble() * ratio));
                connect(inp->lineeditvalue, &mCalcLineEdit::textChanged,
                    [this, inp]() {
                        emit sg_editFinished();
                    });
                inputWidgets.append(inp);
            }
            // 否则尝试转换为 QVector<QVector<double>>（二维数组）
            else if (value.canConvert<QVector<QVector<double>>>())
            {
                curves_[p.key()] = p.value();
                QVector<QVector<double>> data = value.value<QVector<QVector<double>>>();
                QString name = p.key();
                // 温度相关曲线改用吸附式弹窗行（标签 + 入口按钮），不再占用停靠表；
                // 编辑产生的列主序曲线（首列温度）经回调写回 curves_ 并触发保存
                QWidget* row = createLibCurveRow(name, data,
                    [this, name](const QVector<QVector<double>>& curves) {
                        curves_[name].second = QVariant::fromValue(curves);
                        emit sg_editFinished();
                    });
                curvebtns.append(row);
            }
        }
        InitFoldSection();
        updatePropUnit();
    }

    void mMatFromLibElectic::InitFoldSection()
    {
        this->removeAll();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp->propwid);
        }
        for (auto btn : curvebtns)
        {
            this->addRow(btn);
        }
        return;
    }

    /***********************************默认属性***************************************/
    mDefaultProperty::mDefaultProperty(QWidget* parent)
        :mMaterialPropBase(NONETYPE, "属性", parent)
    {
        initInputs();
    }

    mDefaultProperty::~mDefaultProperty()
    {
        clearInputs();
    }

    void mDefaultProperty::initInputs()
    {
        _colorBtn.initWidget("颜色");
        setColor(_currentColor);
        // 连接点击信号
        connect(_colorBtn.btn, &QPushButton::clicked, this, [this]() {
            QColor newColor = QColorDialog::getColor(_currentColor, nullptr, "选择颜色");
            if (newColor.isValid()) {
                setColor(newColor);
                emit sg_colorSelected(newColor);
            }
            });
        InitFoldSection();
    }

    void mDefaultProperty::clearInputs()
    {
        _colorBtn.clear();
    }

    void mDefaultProperty::setColor(const QColor& color)
    {
        _currentColor = color;
        _colorBtn.btn->setStyleSheet(QString(
            "background-color: %1; border-radius: 4px; border: 1px solid gray;"
        ).arg(color.name()));
    }

    void mDefaultProperty::InitFoldSection()
    {
        this->addRow(_colorBtn.propwid);
        return;
    }


    void mDefaultProperty::updatePropUnit()
    {

    }

    void mDefaultProperty::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        if (prop.contains(MatProp_MaterialColor))
        {
            QVariant var = prop[MatProp_MaterialColor];
            if (var.canConvert<QColor>())
            {
                QColor loadColor = var.value<QColor>();
                setColor(loadColor);
            }
        }
        updatePropUnit();
    }


    /***********************************弹性参数***************************************/
    mFlexProp::mFlexProp(QWidget* parent)
        : mMaterialPropBase(MAT_Flex, "弹性参数", parent)
    {
        initInputs();
    }

    mFlexProp::~mFlexProp()
    {
        clearInputs();
    }

    void mFlexProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  "
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson
        };

        inputWidgets.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        checkboxtemp_.initWidget("弹性参数-温度相关");
        checkboxtemp_.cb->setText("温度相关");
        checkboxtemp_.setPropType(MatProp_Curve_Y_B);
        // 温度相关行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-弹性参数曲线"),
            QStringLiteral("温度 / °C"),
            { names[0].trimmed(), names[1].trimmed() },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });
        connect(m_tempCurveRow->checkBox(), &QCheckBox::stateChanged, this, [this](int i) {
            for (auto inp : inputWidgets)
            {
                inp.setDisableAll(i != 0);
            }
            // 勾选状态走封装接口同步数据载体并触发保存；内部 blockSignals
            // 双向屏蔽，程序化 setChecked（setInputsValue/切材料残留复位）不再递归
            // 触发保存，避免「材料1勾选→新建材料2也被勾选」的载体状态串扰
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, i != 0);
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mFlexProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mFlexProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_);
        }
        else
        {
            QHash<MxFunctions::MaterialPropType, double> ratios{
                { MatProp_YoungsModulus, ratio }
            };
            collectProps(props, inputWidgets, ratios);
        }
        return props;
    }

    void mFlexProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_YoungsModulus))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, false);
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                double v = data.toDouble();
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        if (prop.contains(MatProp_Curve_Y_B))
        {
            for (auto inp : inputWidgets)
            {
                inp.lineeditvalue->setExpression("");
            }
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Y_B];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        updatePropUnit();
    }

    void mFlexProp::InitFoldSection()
    {
        for (auto& inp : inputWidgets)
        {
            this->addRow(inp.propwid);
        }
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        return;
    }

    void mFlexProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setChecked(false);
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mFlexProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mFlexProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QVector<foldDetail*> mFlexProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }
    void mFlexProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFlexProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mFlexProp::slot_connTable);
    }

    void mFlexProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "弹性参数-温度相关")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mFlexProp::slot_paraswitchInputs(int i)
    {
        emit sg_editFinished();
        emit sg_hardModelChanged(i);
    }


    /***********************************塑性参数***************************************/
    mPlasticityProp::mPlasticityProp(QWidget* parent)
        : mMaterialPropBase(MAT_Plasticity, "塑性参数", parent)
    {
        initInputs();
    }

    mPlasticityProp::~mPlasticityProp()
    {
        clearInputs();
    }

    void mPlasticityProp::initInputs()
    {
        singleComboHardModle_.initWidget("硬化模型");
        singleComboHardModle_.proptype_ = MatProp_Curve_PlasticityProperties_type;
        singleComboHardModle_.combobox->clear();
        singleComboHardModle_.combobox->addItem("各向同性硬化");
        singleComboHardModle_.combobox->addItem("随动硬化");
        singleComboHardModle_.combobox->addItem("组合硬化");
        connect(singleComboHardModle_.combobox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPlasticityProp::slot_paraswitchInputs);
        singleComboHardModle_.combobox->setCurrentIndex(0);

        // 勾选与否决定曲线是否含温度列（TemperatureCorrelation / NoCorrelation）。
        // 封装控件固定为温度相关形态（首列温度），未勾选时同样以温度列存储，
        // 数据协议与 collectProps(singleCheckBox, correlation) 保持一致
        checkboxtemp2_.initWidget("塑性参数-温度相关");
        checkboxtemp2_.cb->setText("温度相关");
        checkboxtemp2_.setPropType(MatProp_Curve_PlasticityProperties);
        checkboxtemp2_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("温度相关"),
            QStringLiteral("温度-塑性参数曲线"),
            QStringLiteral("温度 / °C"),
            { QStringLiteral("应力"), QStringLiteral("塑性应变") },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp2_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mPlasticityProp::clearInputs()
    {
        singleComboHardModle_.clear();
        checkboxtemp2_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mPlasticityProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, singleComboHardModle_.proptype_, singleComboHardModle_.combobox->currentIndex());
        if (checkboxtemp2_.cb->isChecked())
        {
            collectProps(props, checkboxtemp2_);
        }
        else
        {
            collectProps(props, checkboxtemp2_, MechMaterialData::CurveType::NoCorrelation);
        }
        return props;
    }

    void mPlasticityProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_Curve_PlasticityProperties_type))
        {
            int idx = prop[MatProp_Curve_PlasticityProperties_type].toInt();
            singleComboHardModle_.combobox->blockSignals(true);
            singleComboHardModle_.combobox->setCurrentIndex(idx);
            singleComboHardModle_.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_Curve_PlasticityProperties))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp2_, false);
            QVariant data = prop[MatProp_Curve_PlasticityProperties];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
                if (curves.correlation == CurveType::TemperatureCorrelation)
                {
                    if (m_tempCurveRow)
                        m_tempCurveRow->syncCheckedTo(checkboxtemp2_, true);
                }
            }
        }
        updatePropUnit();
    }

    void mPlasticityProp::InitFoldSection()
    {
        this->addRow(singleComboHardModle_.propwid);
        if (m_tempCurveRow)
            this->addRow(m_tempCurveRow);
        return;
    }

    void mPlasticityProp::clearData()
    {
        singleComboHardModle_.combobox->blockSignals(true);
        singleComboHardModle_.combobox->setCurrentIndex(0);
        singleComboHardModle_.combobox->blockSignals(false);
        checkboxtemp2_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp2_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mPlasticityProp::updatePropUnit()
    {
    }

    QVector<foldDetail*> mPlasticityProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    void mPlasticityProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPlasticityProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mPlasticityProp::slot_connTable);
    }

    void mPlasticityProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "塑性参数-温度相关")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件（勾选状态不变）
        checkboxtemp2_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mPlasticityProp::slot_paraswitchInputs(int i)
    {
        emit sg_editFinished();
        emit sg_hardModelChanged(i);
    }

    /***********************************Johnson-Cook塑性参数***************************************/
    mPlasticityJohnsonProp::mPlasticityJohnsonProp(QWidget* parent)
        : mMaterialPropBase(MAT_PlasticityJohnson, "塑性参数", parent)
    {
        initInputs();
    }

    mPlasticityJohnsonProp::~mPlasticityJohnsonProp()
    {
        clearInputs();
    }
    void mPlasticityJohnsonProp::initInputs()
    {
        QStringList names = {
            "初始屈服应力",
            "应变乘子",
            "应变幂指数",
            "温度幂指数",
            "融化温度",
            "转变温度",
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_A,
            MatProp_B,
            MatProp_N,
            MatProp_M,
            MatProp_MeltingTemp,
            MatProp_TransitionTemp,
        };

        inputWidgets.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::NonNegative));
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        updatePropUnit();
        InitFoldSection();
    }

    void mPlasticityJohnsonProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mPlasticityJohnsonProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double temp_ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, sysunitidxs, { 3,0,0,0 });
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        
        QHash<MxFunctions::MaterialPropType, double> ratios{
            { MatProp_A, ratio },
            { MatProp_MeltingTemp, temp_ratio},
            { MatProp_TransitionTemp, temp_ratio}
        };
        collectProps(props, inputWidgets, ratios);
        
        return props;
    }

    void mPlasticityJohnsonProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double temp_ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Temp, sysunitidxs, { 3,0,0,0 });
        double stress_ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        QList<MxFunctions::MaterialPropType> stress_type = { MatProp_A };
        QList<MxFunctions::MaterialPropType> temp_type = { MatProp_MeltingTemp, MatProp_TransitionTemp };
        if (!prop.contains(MatProp_A)) {
            inputWidgets[type2inp[MatProp_A]].lineeditvalue->setExpression("0");
        }
        if (!prop.contains(MatProp_MeltingTemp)) {
            inputWidgets[type2inp[MatProp_MeltingTemp]].lineeditvalue->setExpression("0");
        }
        if (!prop.contains(MatProp_TransitionTemp)) {
            inputWidgets[type2inp[MatProp_TransitionTemp]].lineeditvalue->setExpression("0");
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = it.value();
            if (data.isValid())
            {
                double v = stress_type.contains(it.key()) ? data.toDouble() / stress_ratio :
                    temp_type.contains(it.key()) ? data.toDouble() / temp_ratio : data.toDouble();
                inputWidgets[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }

    }

    void mPlasticityJohnsonProp::InitFoldSection()
    {
        for (auto& widget : inputWidgets)
            this->addRow(widget.propwid);
        return;
    }

    void mPlasticityJohnsonProp::clearData()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.lineeditvalue->setExpression("");
        }
    }

    void mPlasticityJohnsonProp::updatePropUnit()
    {
        QList<MxFunctions::MaterialPropType> stress_type = { MatProp_A };
        QList<MxFunctions::MaterialPropType> temp_type = { MatProp_MeltingTemp, MatProp_TransitionTemp };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit_none = UnitsLibrary[(int)Unit::NoneUnit][0];
        QString unit_stress = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString unit_temp = UnitsLibrary[(int)Unit::Temp][sysunitidxs[3]];
        for (auto& widget : inputWidgets) {
            auto type = widget.proptype_;
            widget.lineeditvalue->setUnit(stress_type.contains(type) ? unit_stress : temp_type.contains(type) ? unit_temp : unit_none);
        }
    }


    /***********************************带阶数的塑性参数***************************************/
    mPlasticityOrderProp::mPlasticityOrderProp(QWidget* parent)
        : mMaterialPropBase(MAT_PlasticityWithOrder, "塑性参数", parent)
    {
        initInputs();
    }

    mPlasticityOrderProp::~mPlasticityOrderProp()
    {
        clearInputs();
    }

    void mPlasticityOrderProp::initInputs()
    {
        QStringList names = {
            "零塑性时初始屈服应力",
            "随动硬化参数C1",
            "随动硬化参数R1",
            "随动硬化参数C2",
            "随动硬化参数R2",
            "随动硬化参数C3",
            "随动硬化参数R3",
            "随动硬化参数C4",
            "随动硬化参数R4",
            "随动硬化参数C5",
            "随动硬化参数R5",
            "随动硬化参数C6",
            "随动硬化参数R6",
            "随动硬化参数C7",
            "随动硬化参数R7",
            "随动硬化参数C8",
            "随动硬化参数R8",
            "随动硬化参数C9",
            "随动硬化参数R9",
            "随动硬化参数C10",
            "随动硬化参数R10",
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_InitialYieldStressAtZeroPlasticity,
            MatProp_KinematicHardeningC1,
            MatProp_KinematicHardeningR1,
            MatProp_KinematicHardeningC2,
            MatProp_KinematicHardeningR2,
            MatProp_KinematicHardeningC3,
            MatProp_KinematicHardeningR3,
            MatProp_KinematicHardeningC4,
            MatProp_KinematicHardeningR4,
            MatProp_KinematicHardeningC5,
            MatProp_KinematicHardeningR5,
            MatProp_KinematicHardeningC6,
            MatProp_KinematicHardeningR6,
            MatProp_KinematicHardeningC7,
            MatProp_KinematicHardeningR7,
            MatProp_KinematicHardeningC8,
            MatProp_KinematicHardeningR8,
            MatProp_KinematicHardeningC9,
            MatProp_KinematicHardeningR9,
            MatProp_KinematicHardeningC10,
            MatProp_KinematicHardeningR10,
        };

        spinbox_.initWidget("背应力分量个数");
        spinbox_.spinbox->setMinimum(1);
        spinbox_.spinbox->setMaximum(10);
        spinbox_.proptype_ = MatProp_Order;

        inputWidgets2.clear();
        inputWidgets2Tmp.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.proptype_ = proptypes[i];
            type2inp[proptypes[i]] = i;
            widgetSet.labelname->setFixedWidth(i == 0 ? 120 : 100);
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                this, [this]() {
                    emit sg_editFinished();
                });
            inputWidgets2Tmp.append(widgetSet);
        }

        for (int i = 0; i < 3; ++i)
        {
            inputWidgets2.append(inputWidgets2Tmp[i]);
        }

        connect(spinbox_.spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &mPlasticityOrderProp::slot_stepChanged);

        spinbox_.spinbox->blockSignals(true);
        spinbox_.spinbox->setValue(1);
        spinbox_.spinbox->blockSignals(false);

        updatePropUnit();
        InitFoldSection();
    }

    void mPlasticityOrderProp::slot_stepChanged(int step)
    {
        this->blockSignals(true);
        this->removeAll();
        this->blockSignals(false);

        inputWidgets2.clear();
        inputWidgets2.append(inputWidgets2Tmp[0]);
        for (int i = 1; i <= step * 2; ++i)
        {
            inputWidgets2.append(inputWidgets2Tmp[i]);
        }

        this->addRow(spinbox_.propwid);
        spinbox_.propwid->show();
        for (auto inp : inputWidgets2)
        {
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        this->refreshLayout();

        emit sg_editFinished();
    }

    void mPlasticityOrderProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets2Tmp)
        {
            widgetSet.clear();
        }
        inputWidgets2Tmp.clear();
        inputWidgets2.clear();
        spinbox_.clear();
    }

    void mPlasticityOrderProp::clearData()
    {
        for (auto& widgetSet : inputWidgets2Tmp)
        {
            widgetSet.lineeditvalue->setExpression("");
        }
    }

    void mPlasticityOrderProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit_none = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        QString unit1 = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        QString unit2 = UnitsLibrary[(int)Unit::StressReciprocal][sysunitidxs[0]];
        for (int i = 0; i < inputWidgets2Tmp.size(); ++i)
        {
            inputWidgets2Tmp[i].lineeditvalue->setUnit(i == 0 ? unit1 : i % 2 == 0 ? unit_none : (i & 1 ? unit1 : unit2));
        }
    }

    void mPlasticityOrderProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mPlasticityOrderProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio1 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        double ratio2 = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::StressReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, spinbox_.proptype_, spinbox_.spinbox->value());

        QHash<MxFunctions::MaterialPropType, double> ratios2;
        ratios2[MatProp_InitialYieldStressAtZeroPlasticity] = ratio1;
        for (int i = 1; i <= 10; ++i)
        {
            ratios2[static_cast<MxFunctions::MaterialPropType>(MatProp_KinematicHardeningC1 + (i - 1) * 2)] = ratio1;
            ratios2[static_cast<MxFunctions::MaterialPropType>(MatProp_KinematicHardeningR1 + (i - 1) * 2)] = 1.0;
        }
        collectProps(props, inputWidgets2, ratios2);
        return props;
    }

    void mPlasticityOrderProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_Order))
        {
            int order = prop[MatProp_Order].toInt();
            spinbox_.spinbox->setValue(order);
            inputWidgets2.clear();
            inputWidgets2.append(inputWidgets2Tmp[0]);
            for (int i = 1; i <= order * 2; ++i)
            {
                inputWidgets2.append(inputWidgets2Tmp[i]);
            }
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = prop[it.key()];
            if (data.isValid())
            {
                if (type2inp[it.key()] != 0 && type2inp[it.key()] % 2 == 0)
                    inputWidgets2[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                else
                    inputWidgets2[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
            }
        }
        updatePropUnit();
    }

    void mPlasticityOrderProp::InitFoldSection()
    {
        this->addRow(spinbox_.propwid);
        for (auto inp : inputWidgets2)
        {
            this->addRow(inp.propwid);
        }
        return;
    }

    QVector<foldDetail*> mPlasticityOrderProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        mainLayout_ = mainLayout;
        folds_ = &folds;

        QVector<foldDetail*> newFold;
        QVector<QWidget*> wds;
        this->addRow(spinbox_.propwid);
        for (auto inp : inputWidgets2)
        {
            this->addRow(inp.propwid);
        }
        foldDetail* detail = new foldDetail();
        detail->foldText = "塑性参数";
        detail->vecWidget = wds;
        newFold.append(detail);
        return newFold;
    }

    void mPlasticityOrderProp::setOrder(int order)
    {
        if (order < 1) order = 1;
        if (order > 10) order = 10;
        spinbox_.spinbox->blockSignals(true);
        spinbox_.spinbox->setValue(order);
        spinbox_.spinbox->blockSignals(false);
        inputWidgets2.clear();
        inputWidgets2.append(inputWidgets2Tmp[0]);
        for (int i = 1; i <= order * 2; ++i)
        {
            inputWidgets2.append(inputWidgets2Tmp[i]);
        }
    }

    int mPlasticityOrderProp::getOrder() const
    {
        return spinbox_.spinbox->value();
    }

    QVector<InputWidgetSet> mPlasticityOrderProp::getAllInputWidgets() const
    {
        return inputWidgets2Tmp;
    }

    QVector<InputWidgetSet> mPlasticityOrderProp::getVisibleInputWidgets() const
    {
        return inputWidgets2;
    }

    singleSpinBox* mPlasticityOrderProp::getSpinBox()
    {
        return &spinbox_;
    }

    QHash<MxFunctions::MaterialPropType, int> mPlasticityOrderProp::getType2Inp() const
    {
        return type2inp;
    }

    /***********************************率相关***************************************/
    mRateDependentProp::mRateDependentProp(QWidget* parent)
        :mMaterialPropBase(MAT_RateDependent, "率相关", parent)
    {
        initInputs();
    }

    mRateDependentProp::~mRateDependentProp()
    {
        //clearInputs();
    }

    void mRateDependentProp::initInputs()
    {
        QStringList names = {
            "应变率乘子",
            "参考应变率",
        };
        QVector<MaterialPropType> proptypes = {
            MatProp_C,
            MatProp_RefStrainRate,
        };
        combobox_.initWidget("类型");
        combobox_.proptype_ = MatProp_RateType;
        combobox_.combobox->addItems({ "Johnson-Cook" });

        combobox_.combobox->setCurrentIndex(0);
        connect(combobox_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mRateDependentProp::slot_typeChanged);


        //初始化输入控件
        inputWidgetsTmp.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::NonNegative));
            // 设置文本
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];

            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp.append(widgetSet);
        }
        inputWidgets = inputWidgetsTmp;

        updatePropUnit();
        InitFoldSection();
    }

    void mRateDependentProp::slot_typeChanged(int i)
    {
        inputWidgets.clear();
        if (i == 0)
        {
            inputWidgets = inputWidgetsTmp;
        }

        this->removeAll();
        this->addRow(combobox_.propwid);
        combobox_.propwid->show();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }


    void mRateDependentProp::clearData()
    {
        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mRateDependentProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit_none = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        QString unit1 = UnitsLibrary[(int)Unit::TimeReciprocal][sysunitidxs[0]];
        
        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setUnit(inp.proptype_ == MatProp_C ? unit_none : unit1);
        }
    }

    void mRateDependentProp::clearInputs()
    {
        inputWidgets.clear();

        for (auto& widgetSet : inputWidgetsTmp)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp.clear();
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mRateDependentProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TimeReciprocal, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{
                {MatProp_RefStrainRate, ratio},

        };
        collectProps(props, inputWidgets, ratios);

        collectProps(props, combobox_.proptype_, combobox_.combobox->currentIndex());
        return props;
    }

    void mRateDependentProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TimeReciprocal, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_RateType))
        {
            int idx = prop[MatProp_RateType].toInt();
            combobox_.combobox->setCurrentIndex(idx);

            inputWidgets.clear();
            if (idx == 0)
            {
                inputWidgets = inputWidgetsTmp;
            }
        }
        if (!prop.contains(MatProp_RefStrainRate)) {
            inputWidgets[type2inp[MatProp_RefStrainRate]].lineeditvalue->setExpression("0");
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = it.value();
            if (data.isValid())
            {
                double v = it.key() == MatProp_C ? data.toDouble() : data.toDouble() / ratio;
                inputWidgetsTmp[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        
    }

    void mRateDependentProp::InitFoldSection()
    {
        this->setCheckable(true);
        this->addRow(combobox_.propwid);

        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }


    /***********************************厚度行为***************************************/
    mThicknessBehaviorProp::mThicknessBehaviorProp(QWidget* parent)
        : mMaterialPropBase(MAT_ThicknessBehavior, "厚度行为", parent)
    {
        initInputs();
    }

    mThicknessBehaviorProp::~mThicknessBehaviorProp()
    {
        clearInputs();
    }

    void mThicknessBehaviorProp::initInputs()
    {
        comboThicknessType_.initWidget("类型");
        comboThicknessType_.proptype_ = MatProp_ShimThicknessType;
        comboThicknessType_.combobox->clear();
        comboThicknessType_.combobox->addItem("弹性损伤");
        comboThicknessType_.combobox->addItem("弹塑性");
        connect(comboThicknessType_.combobox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mThicknessBehaviorProp::slot_paraswitchInputs);
        comboThicknessType_.combobox->blockSignals(true);
        comboThicknessType_.combobox->setCurrentIndex(0);
        comboThicknessType_.combobox->blockSignals(false);

        loadingCurveBtn_.initWidget("加载曲线");
        loadingCurveBtn_.setPropType(MatProp_Curve_ShimLoading);
        loadingHeader_ << "Pressure" << "Closure";
        connect(loadingCurveBtn_.tablebtn, &QPushButton::clicked, this, [this]() {
            QString tabletitle = "加载曲线";
            QVariant value = loadingCurveBtn_.tablebtn->property("curve");
            mMaterialPropBase::tableEdit(true, tabletitle, value, loadingHeader_);
            {
                QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                for (int col = 0; col < loadingHeader_.size(); ++col)
                {
                    vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                }
                mTableDataWindow::getInstance()->setColumnValidations(vaMap);
            }
            mDockLayoutManager::getInstance()->activateDock("Table");
            });

        unloadingCurveCheck_.initWidget("卸载曲线");
        unloadingCurveCheck_.cb->setText("定义卸载");
        unloadingCurveCheck_.setPropType(MatProp_Curve_ShimUnloading);
        unloadingCurveCheck_.tablebtn->setDisabled(true);
        unloadingHeader_ << "Pressure" << "Closure" << "Max.Closure";
        connect(unloadingCurveCheck_.cb, &QCheckBox::stateChanged, this, [this](int i) {
            unloadingCurveCheck_.tablebtn->setDisabled(i == 0);
            if (i == 0) {
                unloadingCurveCheck_.cb->setProperty("curve", QVariant());
            }
            emit sg_editFinished();
            });
        connect(unloadingCurveCheck_.tablebtn, &QPushButton::clicked, this, [this]() {
            QString tabletitle = "卸载曲线";
            QVariant value = unloadingCurveCheck_.cb->property("curve");
            mMaterialPropBase::tableEdit(unloadingCurveCheck_.cb->isChecked(), tabletitle, value, unloadingHeader_);
            {
                QMap<int, mTableDataWindow::ColumnValidationType> vaMap;
                for (int col = 0; col < unloadingHeader_.size(); ++col)
                {
                    QString colName = unloadingHeader_[col].trimmed();
                    if (colName == "Pressure" || colName == "Closure" || colName == "Max.Closure")
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::PositiveReal);
                    else
                        vaMap.insert(col, mTableDataWindow::ColumnValidationType::Real);
                }
                mTableDataWindow::getInstance()->setColumnValidations(vaMap);
            }
            mDockLayoutManager::getInstance()->activateDock("Table");
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mThicknessBehaviorProp::clearInputs()
    {
        comboThicknessType_.clear();
        loadingCurveBtn_.clear();
        unloadingCurveCheck_.clear();
        loadingHeader_.clear();
        unloadingHeader_.clear();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mThicknessBehaviorProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboThicknessType_.proptype_, comboThicknessType_.combobox->currentIndex());
        collectProps(props, loadingCurveBtn_);
        collectProps(props, unloadingCurveCheck_, CurveType::NoCorrelation);
        return props;
    }

    void mThicknessBehaviorProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_ShimThicknessType))
        {
            int idx = prop[MatProp_ShimThicknessType].toInt();
            comboThicknessType_.combobox->blockSignals(true);
            comboThicknessType_.combobox->setCurrentIndex(idx);
            comboThicknessType_.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_Curve_ShimLoading))
        {
            QVariant data = prop[MatProp_Curve_ShimLoading];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                loadingCurveBtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves.curves));
            }
        }
        if (prop.contains(MatProp_Curve_ShimUnloading))
        {
            QVariant data = prop[MatProp_Curve_ShimUnloading];
            if (data.isValid())
            {
                unloadingCurveCheck_.cb->setChecked(true);
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                unloadingCurveCheck_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (curves.correlation != CurveType::NoCorrelation)
                {
                    unloadingCurveCheck_.cb->setChecked(false);
                }
            }
        }
        updatePropUnit();
    }

    void mThicknessBehaviorProp::InitFoldSection()
    {
        this->addRow(comboThicknessType_.propwid);
        this->addRow(loadingCurveBtn_.propwid);
        this->addRow(unloadingCurveCheck_.propwid);
        return;
    }

    void mThicknessBehaviorProp::clearData()
    {
        comboThicknessType_.combobox->blockSignals(true);
        comboThicknessType_.combobox->setCurrentIndex(0);
        comboThicknessType_.combobox->blockSignals(false);
        loadingCurveBtn_.tablebtn->setProperty("curve", QVariant());
        unloadingCurveCheck_.cb->setChecked(false);
        unloadingCurveCheck_.cb->setProperty("curve", QVariant());
    }

    void mThicknessBehaviorProp::updatePropUnit()
    {
    }

    QVector<foldDetail*> mThicknessBehaviorProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    void mThicknessBehaviorProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mThicknessBehaviorProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mThicknessBehaviorProp::slot_connTable);
    }

    void mThicknessBehaviorProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "加载曲线" && title != "卸载曲线")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        if (title == "加载曲线")
        {
            loadingCurveBtn_.tablebtn->setProperty("curve", QVariant::fromValue(curves));
        }
        else if (title == "卸载曲线")
        {
            unloadingCurveCheck_.cb->setProperty("curve", QVariant::fromValue(curves));
        }
        emit sg_editFinished();
    }

    void mThicknessBehaviorProp::slot_paraswitchInputs(int i)
    {
        emit sg_editFinished();
    }

    /***********************************膜行为***************************************/
    mFilmBehaviorProp::mFilmBehaviorProp(QWidget* parent)
        : mMaterialPropBase(MAT_FilmBehavior, "膜行为", parent)
    {
        initInputs();
    }

    mFilmBehaviorProp::~mFilmBehaviorProp()
    {
        clearInputs();
    }

    void mFilmBehaviorProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  "
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson
        };

        inputWidgets.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mFilmBehaviorProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mFilmBehaviorProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        QHash<MxFunctions::MaterialPropType, double> ratios{
            { MatProp_YoungsModulus, ratio }
        };
        collectProps(props, inputWidgets, ratios);
        return props;
    }

    void mFilmBehaviorProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 3,0,0,0 });
        if (prop.contains(MatProp_YoungsModulus))
        {
            QVariant data = prop[MatProp_YoungsModulus];
            if (data.isValid())
            {
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble() / ratio));
            }
        }
        if (prop.contains(MatProp_Botson))
        {
            QVariant data = prop[MatProp_Botson];
            if (data.isValid())
            {
                inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        updatePropUnit();
    }

    void mFilmBehaviorProp::InitFoldSection()
    {
        for (auto& inp : inputWidgets)
        {
            this->addRow(inp.propwid);
        }
        return;
    }

    void mFilmBehaviorProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mFilmBehaviorProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mFilmBehaviorProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QVector<foldDetail*> mFilmBehaviorProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    /***********************************剪切行为***************************************/
    mCutBehaviorProp::mCutBehaviorProp(QWidget* parent)
        : mMaterialPropBase(MAT_CutBehavior, "剪切行为", parent)
    {
        initInputs();
    }

    mCutBehaviorProp::~mCutBehaviorProp()
    {
        clearInputs();
    }

    void mCutBehaviorProp::initInputs()
    {
        QStringList names = {
            "剪切刚度"
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_ShearStiffness
        };

        inputWidgets.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mCutBehaviorProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mCutBehaviorProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets);
        return props;
    }

    void mCutBehaviorProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_ShearStiffness))
        {
            QVariant data = prop[MatProp_ShearStiffness];
            if (data.isValid())
            {
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        updatePropUnit();
    }

    void mCutBehaviorProp::InitFoldSection()
    {
        for (auto& inp : inputWidgets)
        {
            this->addRow(inp.propwid);
        }
        return;
    }

    void mCutBehaviorProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mCutBehaviorProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        inputWidgets[0].lineeditvalue->setUnit(stressunit);
    }

    void mCutBehaviorProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    QVector<foldDetail*> mCutBehaviorProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        return QVector<foldDetail*>();
    }

    /***********************************垫片材料Shim层***************************************/
    mShimProp::mShimProp(QWidget* parent)
        :mMaterialPropBase(MAT_Shim, "垫片材料", parent)
    {
        initInputs();
    }

    mShimProp::~mShimProp()
    {
        clearInputs();
    }

    void mShimProp::initInputs()
    {
    }

    void mShimProp::clearInputs()
    {
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mShimProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        return props;
    }

    void mShimProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        updatePropUnit();
    }

    void mShimProp::InitFoldSection()
    {
        QVector<QWidget*> wds;
        return;
    }

    QVector<foldDetail*> mShimProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        QVector<foldDetail*> newFold;
        return newFold;
    }

    void mShimProp::clearData()
    {
    }

    /***********************************断裂材料-材料参数***************************************/
    mBreakParamProp::mBreakParamProp(QWidget* parent)
        : mMaterialPropBase(MAT_BreakParam, "材料参数", parent)
    {
        initInputs();
    }

    mBreakParamProp::~mBreakParamProp()
    {
        clearInputs();
    }

    void mBreakParamProp::initInputs()
    {
        QStringList names = {
            "杨氏模量",
            "泊松比  ",
            "拉伸极限强度",
            "断裂能"
        };
        QVector<MxFunctions::MaterialPropType> proptypes = {
            MatProp_YoungsModulus,
            MatProp_Botson,
            MatProp_Strech_Limit_Strenth,
            MatProp_Break_Power
        };

        inputWidgets.clear();
        type2inp.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::Standard));
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
            type2inp[proptypes[i]] = i;
        }

        updatePropUnit();
        InitFoldSection();
    }

    void mBreakParamProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        type2inp.clear();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mBreakParamProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 2,0,0,0 });

        QHash<MxFunctions::MaterialPropType, QVariant> props;

        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_YoungsModulus, ratio} };

        collectProps(props, inputWidgets, ratios);
        return props;
    }

    void mBreakParamProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        for (auto it = prop.begin(); it != prop.end(); ++it)
        {
            MxFunctions::MaterialPropType type = it.key();
            if (type2inp.contains(type))
            {
                QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, sysunitidxs, { 2,0,0,0 });

                int idx = type2inp[type];
                QVariant data = it.value();
                if (data.isValid())
                {
                    if (type == MatProp_YoungsModulus)
                    {
                        double v = data.toDouble() / ratio;
                        inputWidgets[idx].lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
                    }
                    else
                    {
                        inputWidgets[idx].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
                    }

                }
            }
        }
        updatePropUnit();
    }

    void mBreakParamProp::InitFoldSection()
    {
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }

    void mBreakParamProp::clearData()
    {
        for (auto& inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mBreakParamProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString stressunit = UnitsLibrary[(int)Unit::Stress][sysunitidxs[0]];
        if (type2inp.contains(MatProp_YoungsModulus))
            inputWidgets[type2inp[MatProp_YoungsModulus]].lineeditvalue->setUnit(stressunit);
    }

    void mBreakParamProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    /***********************************断裂材料-相场参数***************************************/
    mPhaseFieldProp::mPhaseFieldProp(QWidget* parent)
        : mMaterialPropBase(MAT_PhaseFieldParam, "相场参数", parent)
    {
        initInputs();
    }

    mPhaseFieldProp::~mPhaseFieldProp()
    {
        clearInputs();
    }

    void mPhaseFieldProp::initInputs()
    {
        comboPhaseFieldModel_.initWidget("相场模型", MatProp_PhiseFild_Model);
        comboPhaseFieldModel_.combobox->clear();
        comboPhaseFieldModel_.combobox->addItem("AT1");
        comboPhaseFieldModel_.combobox->addItem("AT2");
        comboPhaseFieldModel_.combobox->addItem("内聚率相场模型");
        comboPhaseFieldModel_.combobox->blockSignals(true);
        comboPhaseFieldModel_.combobox->setCurrentIndex(0);
        comboPhaseFieldModel_.combobox->blockSignals(false);
        connect(comboPhaseFieldModel_.combobox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int) {
                emit sg_editFinished();
            });

        comboCohesionModel_.initWidget("内聚率演化模型", MatProp_Cohension_Model);
        comboCohesionModel_.combobox->clear();
        comboCohesionModel_.combobox->addItem("线性软化");
        comboCohesionModel_.combobox->addItem("指数软化曲线");
        comboCohesionModel_.combobox->addItem("双线性软化");
        comboCohesionModel_.combobox->addItem("Cornelissen软化");
        comboCohesionModel_.combobox->blockSignals(true);
        comboCohesionModel_.combobox->setCurrentIndex(0);
        comboCohesionModel_.combobox->blockSignals(false);
        connect(comboCohesionModel_.combobox,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this](int) {
                emit sg_editFinished();
            });

        inputSpecifileW_.initWidget("特征宽度", false, MatProp_Specifile_W, s_nameToCalcMode.value("特征宽度", CalcMode::Standard));
        inputSpecifileW_.labelname->setText("特征宽度");
        connect(inputSpecifileW_.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                emit sg_editFinished();
            });

        updatePropUnit();
        InitFoldSection();
    }

    void mPhaseFieldProp::clearInputs()
    {
        comboPhaseFieldModel_.clear();
        comboCohesionModel_.clear();
        inputSpecifileW_.clear();
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mPhaseFieldProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, comboPhaseFieldModel_.proptype_, comboPhaseFieldModel_.combobox->currentIndex() + 1);
        collectProps(props, comboCohesionModel_.proptype_, comboCohesionModel_.combobox->currentIndex() + 1);

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, sysunitidxs, { 2,0,0,0 });


        QHash<MxFunctions::MaterialPropType, double> ratios{ {MatProp_Specifile_W, ratio} };
        QVector<InputWidgetSet> inpVecs;
        inpVecs.append(inputSpecifileW_);
        collectProps(props, inpVecs, ratios);
        return props;
    }

    void mPhaseFieldProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_PhiseFild_Model))
        {
            int idx = prop[MatProp_PhiseFild_Model].toInt() - 1;
            comboPhaseFieldModel_.combobox->blockSignals(true);
            comboPhaseFieldModel_.combobox->setCurrentIndex(qBound(0, idx, comboPhaseFieldModel_.combobox->count() - 1));
            comboPhaseFieldModel_.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_Cohension_Model))
        {
            int idx = prop[MatProp_Cohension_Model].toInt() - 1;
            comboCohesionModel_.combobox->blockSignals(true);
            comboCohesionModel_.combobox->setCurrentIndex(qBound(0, idx, comboCohesionModel_.combobox->count() - 1));
            comboCohesionModel_.combobox->blockSignals(false);
        }
        if (prop.contains(MatProp_Specifile_W))
        {
            QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
            double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, sysunitidxs, { 2,0,0,0 });


            QVariant data = prop[MatProp_Specifile_W];
            if (data.isValid())
            {
                double v = data.toDouble() / ratio;
                inputSpecifileW_.lineeditvalue->setExpression(MxFunctions::mNumberToString(v));
            }
        }
        updatePropUnit();
    }

    void mPhaseFieldProp::InitFoldSection()
    {
        this->addRow(comboPhaseFieldModel_.propwid);
        this->addRow(comboCohesionModel_.propwid);
        this->addRow(inputSpecifileW_.propwid);
        return;
    }

    void mPhaseFieldProp::clearData()
    {
        comboPhaseFieldModel_.combobox->blockSignals(true);
        comboPhaseFieldModel_.combobox->setCurrentIndex(0);
        comboPhaseFieldModel_.combobox->blockSignals(false);
        comboCohesionModel_.combobox->blockSignals(true);
        comboCohesionModel_.combobox->setCurrentIndex(0);
        comboCohesionModel_.combobox->blockSignals(false);
        inputSpecifileW_.lineeditvalue->setExpression("");
    }

    void mPhaseFieldProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString lengthunit = UnitsLibrary[(int)Unit::Length][sysunitidxs[0]];
        inputSpecifileW_.lineeditvalue->setUnit(lengthunit);
    }

    void mPhaseFieldProp::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        if (!(type & (UnitSystem | TempUnit))) return;
        if (m_matData)
        {
            mMaterialPropBase::s_unitSyncing = true;   //重渲染期间抑制收集回写（clearData→回填窗口期的 finishEdit 会清空 _props）
            setInputsValue(m_matData->getAllProps()[getPropertyClass()]);
             
            mMaterialPropBase::s_unitSyncing = false;
        }
    }

    /***********************************断裂材料***************************************/
    mBreakProp::mBreakProp(QWidget* parent)
        : mMaterialPropBase(MAT_Break, "断裂材料", parent)
    {
        initInputs();
    }

    mBreakProp::~mBreakProp()
    {
        clearInputs();
    }

    void mBreakProp::initInputs()
    {
    }

    void mBreakProp::clearInputs()
    {
    }

    QHash<MxFunctions::MaterialPropType, QVariant> mBreakProp::collectProperties()
    {
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        return props;
    }

    void mBreakProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        updatePropUnit();
    }

    void mBreakProp::InitFoldSection()
    {

        QVector<QWidget*> wds;
        return;
    }

    QVector<foldDetail*> mBreakProp::getAllInputsFold(QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
    {
        QVector<foldDetail*> newFold;
        return newFold;
    }

    void mBreakProp::clearData()
    {

    }

    void mBreakProp::updatePropUnit()
    {

    }

    /***********************************剪切损伤***************************************/
    mShearDamageProp::mShearDamageProp(QWidget* parent)
        :mMaterialPropBase(MAT_ShearDamage, "剪切损伤", parent)
    {
        initInputs();
    }

    mShearDamageProp::~mShearDamageProp()
    {
        clearInputs();
    }

    void mShearDamageProp::initInputs()
    {
        QStringList names = {
            "压力系数",
        };
        QVector<MxFunctions::MaterialPropType>proptypes = {
            MatProp_KS,
        };

        header_ << "断裂应变" <<"剪应力比" <<"应变率";
        inputWidgets.clear();
        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet widgetSet;
            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::NonNegative));
            widgetSet.proptype_ = proptypes[i];
            // 设置文本
            widgetSet.labelname->setText(names[i]);

            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, i]() {
                    emit sg_editFinished();
                });
            inputWidgets.append(widgetSet);
        }
        //不失活其他输入控件：断裂曲线仅作为附加曲线数据，勾选不联动禁用
        checkboxtemp_.initWidget("剪切损伤");
        checkboxtemp_.cb->setText("断裂曲线");
        checkboxtemp_.setPropType(MatProp_Curve_Fs_Ssr_Sr);
        // 断裂曲线行改用封装控件：原勾选行仅作数据载体，不入界面
        checkboxtemp_.propwid->hide();

        m_tempCurveRow = new mTempCurveRowWidget(
            QStringLiteral("断裂曲线"),
            QStringLiteral("断裂曲线"),
            header_[0],
            { header_[1], header_[2] },
            this);
        m_tempCurveRow->setHeaderHasUnit(false); // 与停靠表列标题约定一致：无单位后缀
        connect(m_tempCurveRow, &mTempCurveRowWidget::dataChanged, this, [this]() {
            checkboxtemp_.cb->setProperty("curve", m_tempCurveRow->checkBox()->property("curve"));
            emit sg_editFinished();
            });

        //损伤演化：复选框取代原独立折叠框，勾选后显示类型/模式/断裂位移
        evolutionCheck_.initWidget("损伤演化", false);
        evolutionTypeCombo_.initWidget("类型");
        evolutionTypeCombo_.proptype_ = MatProp_EvolutionType;
        evolutionTypeCombo_.combobox->addItems({ "位移" });
        evolutionTypeCombo_.combobox->setCurrentIndex(0);
        connect(evolutionTypeCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int i) { emit sg_editFinished(); });
        evolutionModeCombo_.initWidget("模式");
        evolutionModeCombo_.proptype_ = MatProp_EvolutionMode;
        evolutionModeCombo_.combobox->addItems({ "线性" });
        evolutionModeCombo_.combobox->setCurrentIndex(0);
        connect(evolutionModeCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int i) { emit sg_editFinished(); });

        QStringList evolutionNames = { "断裂位移" };
        QVector<MxFunctions::MaterialPropType> evolutionPropTypes = { MatProp_Fd };
        for (int i = 0; i < evolutionNames.size(); ++i)
        {
            InputWidgetSet widgetSet;
            widgetSet.initWidget(evolutionNames[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(evolutionNames[i], CalcMode::NonNegative));
            widgetSet.labelname->setText(evolutionNames[i]);
            widgetSet.proptype_ = evolutionPropTypes[i];
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    emit sg_editFinished();
                });
            evolutionWidgets_.append(widgetSet);
        }
        connect(evolutionCheck_.cb, &QCheckBox::stateChanged, this, &mShearDamageProp::slot_evolutionToggled);

        InitFoldSection();
        updatePropUnit();
    }

    void mShearDamageProp::slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title)
    {
        if (title != "剪切损伤")
        {
            return;
        }
        QVector<QVector<double>> curves = getValidTableData(x, y);
        // 停靠表兼容通道：回写数据载体与封装控件
        checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves));
        if (m_tempCurveRow)
            m_tempCurveRow->setCurveData(curves);
        emit sg_editFinished();
    }

    void mShearDamageProp::connTable(bool conn)
    {
        if (conn)
            connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mShearDamageProp::slot_connTable);
        else
            disconnect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig, this, &mShearDamageProp::slot_connTable);
    }

    void mShearDamageProp::slot_evolutionToggled(int state)
    {
        updateFoldRows();
        emit sg_editFinished();
    }

    void mShearDamageProp::updateFoldRows()
    {
        this->removeAll();
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
            widgetSet.propwid->show();
        }
        // 断裂曲线行由封装控件承载（原勾选行仅作数据载体，不入布局）
        if (m_tempCurveRow)
        {
            this->addRow(m_tempCurveRow);
            m_tempCurveRow->show();
        }
        this->addRow(evolutionCheck_.propwid);
        evolutionCheck_.propwid->show();
        if (evolutionCheck_.cb->isChecked())
        {
            this->addRow(evolutionTypeCombo_.propwid);
            evolutionTypeCombo_.propwid->show();
            this->addRow(evolutionModeCombo_.propwid);
            evolutionModeCombo_.propwid->show();
            for (auto& widgetSet : evolutionWidgets_)
            {
                this->addRow(widgetSet.propwid);
                widgetSet.propwid->show();
            }
        }
        else
        {
            evolutionTypeCombo_.propwid->hide();
            evolutionModeCombo_.propwid->hide();
            for (auto& widgetSet : evolutionWidgets_)
            {
                widgetSet.propwid->hide();
            }
        }
        this->refreshLayout();
    }

    void mShearDamageProp::clearData()
    {
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        for (auto inp : evolutionWidgets_)
        {
            inp.lineeditvalue->setExpression("");
        }
        checkboxtemp_.cb->setProperty("curve", QVariant());
        if (m_tempCurveRow)
        {
            // 清数据时勾选态一并复位：跨材料复用的单例属性块不得把上一材料的
            // 勾选残留带入下一材料（静默同步，不触发 stateChanged/sg_editFinished）
            m_tempCurveRow->syncCheckedToCarrier(checkboxtemp_, false);
            m_tempCurveRow->clearCurve();
        }
    }

    void mShearDamageProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];
        inputWidgets[0].lineeditvalue->setUnit(unit);
        for (auto inp : evolutionWidgets_)
        {
            inp.lineeditvalue->setUnit(unit);
        }
    }

    void mShearDamageProp::clearInputs()
    {
        for (auto& widgetSet : inputWidgets)
        {
            widgetSet.clear();
        }
        inputWidgets.clear();
        for (auto& widgetSet : evolutionWidgets_)
        {
            widgetSet.clear();
        }
        evolutionWidgets_.clear();
        evolutionCheck_.clear();
        evolutionTypeCombo_.clear();
        evolutionModeCombo_.clear();
        checkboxtemp_.clear();
        // m_tempCurveRow 以 this 为父对象，随 QObject 父子树自动析构
        m_tempCurveRow = nullptr;
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mShearDamageProp::collectProperties()
    {
        //存值转换成默认值
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QHash<MxFunctions::MaterialPropType, QVariant> props;
        if (checkboxtemp_.cb->isChecked())
        {
            collectProps(props, checkboxtemp_, CurveType::NoCorrelation);
        }
        collectProps(props, inputWidgets, {});

        //损伤演化参数随剪切损伤大类一并收集(勾选损伤演化才写入)
        if (evolutionCheck_.cb->isChecked())
        {
            collectProps(props, evolutionTypeCombo_.proptype_, evolutionTypeCombo_.combobox->currentIndex());
            collectProps(props, evolutionModeCombo_.proptype_, evolutionModeCombo_.combobox->currentIndex());
            collectProps(props, evolutionWidgets_, {});
        }

        return props;
    }

    void mShearDamageProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::TimeReciprocal, sysunitidxs, { 3,0,0,0 });
        clearData();
        if (prop.contains(MatProp_KS))
        {
            QVariant data = prop[MatProp_KS];
            if (data.isValid())
            {
                inputWidgets[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        if (prop.contains(MatProp_Curve_Fs_Ssr_Sr))
        {
            if (m_tempCurveRow)
                m_tempCurveRow->syncCheckedTo(checkboxtemp_, true);
            QVariant data = prop[MatProp_Curve_Fs_Ssr_Sr];
            if (data.isValid())
            {
                MechMaterialData::Curves curves = MechMaterialData::mMaterialBase::transVatint2Curve(data);
                checkboxtemp_.cb->setProperty("curve", QVariant::fromValue(curves.curves));
                if (m_tempCurveRow)
                    m_tempCurveRow->setCurveData(curves.curves);
            }
        }
        //损伤演化(兼容旧MAT_DamageEvolution大类数据，迁移时已并入本大类)
        bool hasEvolution = prop.contains(MatProp_EvolutionType) || prop.contains(MatProp_Fd);
        evolutionCheck_.cb->blockSignals(true);
        evolutionCheck_.cb->setChecked(hasEvolution);
        evolutionCheck_.cb->blockSignals(false);
        if (prop.contains(MatProp_EvolutionType))
        {
            evolutionTypeCombo_.combobox->setCurrentIndex(prop[MatProp_EvolutionType].toInt());
        }
        if (prop.contains(MatProp_EvolutionMode))
        {
            evolutionModeCombo_.combobox->setCurrentIndex(prop[MatProp_EvolutionMode].toInt());
        }
        if (prop.contains(MatProp_Fd))
        {
            QVariant data = prop[MatProp_Fd];
            if (data.isValid())
            {
                evolutionWidgets_[0].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }
        updateFoldRows();
    }

    void mShearDamageProp::InitFoldSection()
    {
        this->setCheckable(true);
        updateFoldRows();
        return;
    }

    /***********************************损伤演化***************************************/
    mDamageEvolutionProp::mDamageEvolutionProp(QWidget* parent)
        :mMaterialPropBase(MAT_DamageEvolution, "损伤演化", parent)
    {
        initInputs();
    }

    mDamageEvolutionProp::~mDamageEvolutionProp()
    {
        clearInputs();
    }

    void mDamageEvolutionProp::initInputs()
    {
        QStringList names = {
            "断裂位移",
        };
        QVector<MaterialPropType> proptypes = {
            MatProp_Fd,
        };
        combobox_type.initWidget("类型");
        combobox_type.proptype_ = MatProp_EvolutionType;
        combobox_type.combobox->addItems({ "位移" });
        combobox_type.combobox->setCurrentIndex(0);
        connect(combobox_type.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mDamageEvolutionProp::slot_typeChanged);

        combobox_mode.initWidget("模式");
        combobox_mode.proptype_ = MatProp_EvolutionMode;
        combobox_mode.combobox->addItems({ "线性" });
        combobox_mode.combobox->setCurrentIndex(0);
        connect(combobox_mode.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mDamageEvolutionProp::slot_modeChanged);

        //初始化输入控件
        inputWidgetsTmp.clear();
        for (int i = 0; i < names.size(); ++i)
        {
            InputWidgetSet widgetSet;

            // 初始化单个输入组
            widgetSet.initWidget(names[i], false, MaterialPropType::Unknown, s_nameToCalcMode.value(names[i], CalcMode::NonNegative));
            // 设置文本
            widgetSet.labelname->setText(names[i]);
            widgetSet.proptype_ = proptypes[i];

            type2inp[proptypes[i]] = i;
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this, widgetSet]() {
                    emit sg_editFinished();
                });
            inputWidgetsTmp.append(widgetSet);
        }
        inputWidgets = inputWidgetsTmp;

        updatePropUnit();
        InitFoldSection();
    }
    void mDamageEvolutionProp::updateDispLinerInputWidgets() {
        int type = combobox_type.combobox->currentIndex();
        int mode = combobox_mode.combobox->currentIndex();
        inputWidgets.clear();
        if (type == 0 && mode == 0) {
            inputWidgets = inputWidgetsTmp;
        }
    }
    void mDamageEvolutionProp::slot_typeChanged(int i)
    {
        updateDispLinerInputWidgets();

        this->removeAll();
        this->addRow(combobox_type.propwid);
        combobox_type.propwid->show();
        this->addRow(combobox_mode.propwid);
        combobox_mode.propwid->show();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }

    void mDamageEvolutionProp::slot_modeChanged(int i)
    {
        //类型和模式暂时无法辨别区分，暂时复用逻辑
        updateDispLinerInputWidgets();

        this->removeAll();
        this->addRow(combobox_type.propwid);
        combobox_type.propwid->show();
        this->addRow(combobox_mode.propwid);
        combobox_mode.propwid->show();
        for (auto inp : inputWidgets)
        {
            this->addRow(inp.propwid);
            inp.propwid->show();
        }
        this->refreshLayout();
        emit sg_editFinished();
    }


    void mDamageEvolutionProp::clearData()
    {
        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setExpression("");
        }
    }

    void mDamageEvolutionProp::updatePropUnit()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit_none = UnitsLibrary[(int)Unit::NoneUnit][sysunitidxs[0]];

        for (auto inp : inputWidgetsTmp)
        {
            inp.lineeditvalue->setUnit(unit_none);
        }
    }

    void mDamageEvolutionProp::clearInputs()
    {
        inputWidgets.clear();

        for (auto& widgetSet : inputWidgetsTmp)
        {
            widgetSet.clear();
        }
        inputWidgetsTmp.clear();
    }


    QHash<MxFunctions::MaterialPropType, QVariant> mDamageEvolutionProp::collectProperties()
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();

        QHash<MxFunctions::MaterialPropType, QVariant> props;
        collectProps(props, inputWidgets, {});

        collectProps(props, combobox_type.proptype_, combobox_type.combobox->currentIndex());
        collectProps(props, combobox_mode.proptype_, combobox_mode.combobox->currentIndex());
        return props;
    }

    void mDamageEvolutionProp::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        clearData();
        if (prop.contains(MatProp_EvolutionType) && prop.contains(MatProp_EvolutionMode))
        {
            int type = prop[MatProp_EvolutionType].toInt();
            combobox_type.combobox->setCurrentIndex(type);
            int mode = prop[MatProp_EvolutionMode].toInt();
            combobox_mode.combobox->setCurrentIndex(mode);

            inputWidgets.clear();
            if (type == 0 && mode == 0)
            {
                inputWidgets = inputWidgetsTmp;
            }
        }
        for (auto it = prop.begin(); it != prop.end(); it++)
        {
            if (!type2inp.contains(it.key()))
                continue;
            QVariant data = it.value();
            if (data.isValid())
            {
                inputWidgetsTmp[type2inp[it.key()]].lineeditvalue->setExpression(MxFunctions::mNumberToString(data.toDouble()));
            }
        }

    }

    void mDamageEvolutionProp::InitFoldSection()
    {
        this->setCheckable(true);
        this->addRow(combobox_type.propwid);
        this->addRow(combobox_mode.propwid);
        for (auto& widgetSet : inputWidgets)
        {
            this->addRow(widgetSet.propwid);
        }
        return;
    }


}
