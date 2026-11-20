#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "miofile_global.h"

//QT
#include <QFileInfoList>
#include <set>
#include <vector>
#include <iostream>
#include <QTextStream>
//MxFunctions
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mIOEnum.h"
#include "mMapRelationEnum.h"
#include <memory>
#include "mStructOption.h"

#include "mPlyLaminate.h"



namespace MechModelData
{
    class mModelData;
}

namespace MechMaterialData
{
    class mMaterialBase;
}

namespace MechContactData
{
    class mContactBase;
    class mPeriodicBoundary;
}

namespace MechSetData
{
    class mSetBase;
}

namespace MechPropertyData
{
    class mPropertyBase;
}

namespace MechCaseData
{
    class mCaseBase;
    class mLoadBoundaryBase;
    class mRoatateAnalyseBase;
    class mNonStructuralQuality;
    class mPredefinedFieldBase;
}

namespace MechDiscreteFieldData
{
    class mDiscreteField;
}

namespace MechModelFixData
{
    class mModelFixManager;
    class mEleSetCtrl;
}

namespace MechConnectorData
{
    class mConnectorBase;
    class mDeputyBase;
}

namespace MechCoordinateData
{
    class mCoordinateBase;
}

namespace MechStructOptionData
{
    class mStructOption;
    class mOptimizationConstraintData;
}

namespace MechFunctionData
{
    class mFunctionBase;
}

namespace MechGlobalParamData
{
    class mModalContriData;
    class mNodeContriData;
    class mPanelContriData;
    class mTPContriData;
    class mERPData;
    class mPeakFilterData;
    class mASCouplingSearchData;
    class mGlobalOutputControlData;
}

class QTextStream;
class mMeshData;
class RenderPickResultData;
namespace MechIOFile
{
    enum class QVariantType;
    class mOMxFile
    {
    public:
        /*
        * 默认构造函数
        */
        mOMxFile();

        /*
         * 重载构造函数
         */
        mOMxFile(QString fname);

        /*
        * 设置工况过滤列表（只写这些工况；为空表示写全部工况——默认）
        */
        void setCaseFilter(const std::vector<int>& caseIDs);

        /*
         * 析构函数
         */
        ~mOMxFile();

        /*
        * 写mx文件
        */
        bool writeMxFile();

        /*
        * 获取是否成功写文件
        */
        bool isSuccessWriteFile() = delete;
        /*
        * 写自定义库材料数据
        */
        bool writeMaterialToMatFile(QString filePath, std::shared_ptr<MechMaterialData::mMaterialBase> mat);

    private:
        /*
        * 写节点数据
        */
        bool writeNodeData();

        //工况过滤列表（空=写全部工况）
        std::set<int> _caseFilter;

        //写节点数据
        void writeNodeData(std::map<int, int> NodeId);

        /*
         * 写网格单元数据
         */
        bool writeElementData();

        /*
        * 写特殊单元数据
        */
        bool writeSpecialElement();

        /*
         * 单个工况写入函数
         */
        void writeSingleCase(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析工况
        */
        bool writeCaseData();

        /*
        * 写分析工况下预定义场
        */
        void writeCasePredefinedField(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析工况下载荷边界
        */
        void writeCaseLoadBCData(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下过盈接触
        */
        void writeCaseContactinterference(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下输出
        */
        void writeCaseOutPut(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下PCH输出
        */
        void writeCasePchOutPut(MechCaseData::mCaseBase* caseBase);

        /*
        * 写全局输出控制
        */
        bool writeGlobalOutputControl();

        /*
        * 写分析下历史数据输出（频响分析、瞬态分析、声固耦合分析）
        */
        void writeHistoryOutput(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下非结构质量
        */
        void writeNsmData(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下模型修改
        */
        void writeModelFixData(MechCaseData::mCaseBase* caseBase);

        /*
        * 写分析下转子分析
        */
        void writeOmega(MechCaseData::mCaseBase* caseBase);

        /*
        * 写线性静态分析
        */
        void writeImplicitLinearStatic(MechCaseData::mCaseBase* caseBase);

        /*
        * 写非线性静态分析
        */
        void writeImplicitNonLinearStatic(MechCaseData::mCaseBase* caseBase);

        /*
        * 写非线性瞬态分析
        */
        void writeImplicitNonLinearDynamic(MechCaseData::mCaseBase* caseBase);

        /*
        * 写非线性后屈曲分析
        */
        void writeImplicitPostBuckling(MechCaseData::mCaseBase* caseBase);

        /*
        * 写稳态传热分析
        */
        void writeImplicitSteadyHeat(MechCaseData::mCaseBase* caseBase);

        /*
        * 写瞬态传热分析
        */
        void writeImplicitTransientHeat(MechCaseData::mCaseBase* caseBase);

        /*
        * 写结构耦合传热分析
        */
        void writeImplicitStructuralCouplingHeat(MechCaseData::mCaseBase* caseBase);

        /*
        * 写模态分析
        */
        void writeImplicitModal(MechCaseData::mCaseBase* caseBase);

        /*
        * 写线性屈曲分析
        */
        void writeImplicitLinearBuckling(MechCaseData::mCaseBase* caseBase);

        /*
        * 写频响分析
        */
        void writeImplicitFrequency(MechCaseData::mCaseBase* caseBase);

        /*
        * 写声固耦合分析
        */
        void writeImplASCoupling(MechCaseData::mCaseBase* caseBase);

        /*
        * 写响应谱分析
        */
        void writeImplicitResponseSpectrum(MechCaseData::mCaseBase* caseBase);

        /*
        * 写随机振动分析
        */
        void writeImplicitRandomVibration(MechCaseData::mCaseBase* caseBase);

        /*
        * 写线性瞬态分析
        */
        void writeImplicitTransient(MechCaseData::mCaseBase* caseBase);

        /*
        * 写超单元分析
        */
        void writeImplicitGeneric(MechCaseData::mCaseBase* caseBase);

        /*
        * 写模态贡献量
        */
        void writeModalContriData(MechGlobalParamData::mModalContriData* data);

        /*
        * 写节点贡献量
        */
        void writeNodeContriData(MechGlobalParamData::mNodeContriData* data);

        /*
        * 写面板贡献量
        */
        void writePanelContriData(MechGlobalParamData::mPanelContriData* data);

        /*
        * 写传递路径贡献量
        */
        bool writeTPContriData();

        /*
        * 写等效辐射功率
        */
        void writeERPData();

        /*
         * 写结构优化
         */
        bool writeStructOptionData();

        /*
        * 优化参数
        */
        void writeStructOptionTopopara();

        /*
        * 设计域
        */
        void writeStructOptionDesign();

        /*
        * 优化约束
        */
        void writeStructOptionConstraint();

        /*
        * 优化约束-响应类型子块(与优化目标的*response子块格式保持一致)
        */
        void writeStructOptionConstraintResponse(MechStructOptionData::mOptimizationConstraintData* constraint, const QString& target);

        /*
        * 优化目标
        */
        void writeStructOptionObjective();

        /*
         * 写材料数据
         */
        bool writeMaterialData();

        /*
        * 写材料库
        */
        void writeLibraryMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
         * 写各向同性线弹性材料数据
         */
        void writeIsolinearelastic(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  写正交各向异性线弹性材料数据
        */
        void writeOrthotropy(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  写各向异性线弹性材料数据
        */
        void writeAnisotropy(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        *  写热学材料本构
        */
        void writeHeatMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写chaboche（循环塑性）材料本构
        */
        void writeChabocheMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写Johnson-Cook（JC-塑性）材料本构
        */
        void writeJohnsonCookMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写声学材料
        */
        void writeAeroacousticsMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写变形（变形）塑性材料本构
        */
        void writeDeformationplasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写粘塑性材料本构
        */
        void writeViscoPlasticityMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写粘弹性材料本构
        */
        void writeViscoelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写弹塑性材料本构--常规塑性
        */
        void writeElastoplasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写超弹塑性材料本构
        */
        void writeHyperelasticMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写剪切损伤材料本构
        */
        void writeShearDamageMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写垫片材料
        */
        void writeShimMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        /*
        * 写超弹泡沫材料
        */
        void writeHyperfoamMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        //TODO:新增7个写材料
        //写弹性材料 MAT_Flex
        void writeFlexMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写塑性参数 带阶数 MAT_PlasticityWithOrder
        void writePlasticityWithOrderMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写Johnson-Cook塑性参数 MAT_PlasticityJohnson
        void writePlasticityJohnsonMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写塑性参数 下拉框 MAT_Plasticity
        void writePlasticityComboMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写率相关参数（Johnson-Cook塑性） MAT_RateDependent
        void writeRateDependentMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写厚度行为 MAT_ThicknessBehavior
        void writeThicknessBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写膜行为 MAT_FilmBehavior
        void writeFilmBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写剪切行为 MAT_CutBehavior
        void writeCutBehaviorMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        //写循环硬化带表格 MAT_CyclicHardeningWithTable
        void writeCyclicHardeningWithTable(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写断裂材料
        */
        void writeBreakMat(std::shared_ptr<MechMaterialData::mMaterialBase> mat);

        void writeBreakParamMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        void writeBreakPhaseFieldMat(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写密度数据
        */
        void writeDensity(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写Hashin失效
        */
        void writeHashIndamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写Hash渐变损伤
        */
        void writeEnergydamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写剪切损伤
        */
        void writeShearDamage(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写损伤演化
        */
        void writeDamageEvolution(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);


        /*
        * 写声速
        */
        void writeSoundVelocity(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写体积模量
        */
        void writeBulkModulus(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写流体阻尼系数
        */
        void writeAcousticMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写材料阻尼
        */
        void writeMaterialDamping(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写热膨胀系数
        */
        void writeExpansion(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写热学参数
        */
        void writeHeat(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写失效应变
        */
        void writeFailureEffect(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写失效应力
        */
        void writeFailureStress(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写各向同性循环硬化
        */
        void writeCyclichardening(const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写蠕变行为
        */
        void writeCreep(std::shared_ptr<MechMaterialData::mMaterialBase> mat, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& matProp);

        /*
        * 写截面属性数据
        */
        bool WritePropertyData();

        /*
        * 写三维实体截面
        */
        void writeSolidProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写杆截面
        */
        void writeTorsionBarProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写剪切板截面
        */
        void writeShearPlateProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写管截面
        */
        void writeTubeProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写壳截面
        */
        void writeShellProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写常截面梁
        */
        void writeResultantProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写变截面梁
        */
        void writeCrossProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写eles1弹簧截面
        */
        void writeElas1Property(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写spring弹簧截面
        */
        void writeSpringProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写阻尼截面
        */
        void writeViscousdampsection(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写cbush截面
        */
        void writeBushsection(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写平面应变截面
        */
        void writePlaneStrainProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写平面应力截面
        */
        void writePlaneStressProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写垫片截面
        */
        void writeGasketProperty(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写间隙截面
        */
        void writeGapunisection(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写宏观等效截面
        */
        void writeMacroEquivalentsection(MechPropertyData::mPropertyBase* proBase);

        /*
        * 写铺层截面,写铺层->壳，写铺层->实体壳，写铺层->实体
        */
        void writePlyLaminate(MechPropertyData::mPropertyBase* proBase);
        void writeShell(MechPropertyData::mPlyLaminate* prodata);
        void writeSolidShell(MechPropertyData::mPlyLaminate* prodata);
        void writeSolid(MechPropertyData::mPlyLaminate* prodata);
        QString getPlyFinalSysName(MechPropertyData::mPlyLaminate* plyData);

        /*
         * 写集合数据
         */
        bool writeSetData();

        /*
        写非结构质量
        */
        bool writeNonSturcturalQualityData();


        /*
        * 写离散场
        */
        bool writeDiscreteField();

        /*
        * 写分析场（点云/对象离散）
        */
        bool writeAnalysisField();

        /*
        * 写几何集合
        */
        QString writeGeoSet(int setID, MxFunctions::SetType type);

        /*
        * 写几何点集合
        */
        void writeGeoPointData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
        * 写几何边集合
        */
        void writeGeoEdgeData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
        * 写几何面集合
        */
        void writeGeoFaceData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
        * 写几何实体集合
        */
        void writeGeoSolidData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
        * 写几何混合集合
        */
        void writeMixSetData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
        * 写几何部件集合
        */
        void writeGeoPartData(MechSetData::mSetBase* setBase, MxFunctions::SetType type, const QString& newName);

        /*
         * 写节点集合数据
         */
        void writeNodeSetData(MechSetData::mSetBase* setBase);

        /*
         * 写节点面集合数据
         */
        void writeNodeSurfaceData(MechSetData::mSetBase* setBase);

        /*
         * 按指定名称写节点面集合数据
         */
        void writeNodeSurfaceData(const std::vector<RenderPickResultData>& data, const QString& name, bool internal);

        /*
        * 写解析表面集合数据
        */
        void writeSurfaceAnalysisData(MechSetData::mSetBase* setBase);

        /*
         * 写单元集合数据
         */
        void writeElementSetData(MechSetData::mSetBase* setBase);

        /*
         * 写单元表面集合数据
         */
        void writeElementSurfaceData(MechSetData::mSetBase* setBase);

        /*
        * 写单元边集合数据
        */
        void writeElementEdgeData(MechSetData::mSetBase* setBase);

        /*
        * 写频率点集合数据
        */
        void writeFrequencyPointData(MechSetData::mSetBase* setBase);

        /*
        * 写自由度节点集合数据
        */
        void writeDofNodeData(MechSetData::mSetBase* setBase);

        /*
        * 写面板集合数据
        */
        void writePanelData(MechSetData::mSetBase* setBase);

        /*
        * 写接触数据
        */
        bool writeContactData();

        /*
        * 写周期性边界数据
        */
        bool writePeriodicBoundaryData();

        /*
        * 点面接触
        */
        void writeNodeToSurfaceContact(MechContactData::mContactBase* contact);

        /*
        * 线面接触
        */
        void writeLineToSurfaceContact(MechContactData::mContactBase* contact);

        /*
        * 面面接触
        */
        void writeSurfaceToSurfaceContact(MechContactData::mContactBase* contact);

        /*
        * 二维接触
        */
        void writeTwoDimensionalContact(MechContactData::mContactBase* contact);

        /*
        * 通用接触
        */
        void writeCommonContact(MechContactData::mContactBase* contact);

        /*
        * 自接触
        */
        void writeSelfContact(MechContactData::mContactBase* contact);

        /*
        * 过盈接触
        */
        void writeInterferenceContact(MechContactData::mContactBase* contact);
        bool writeInterferenceContactData(MechContactData::mContactBase* contact);

        /*
        * 法向行为
        */
        void writeBehavior(MechContactData::mContactBase* contact);

        /*
        *  接触定义摩擦属性
        */
        void writeFriction(MechContactData::mContactBase* contact);

        /*
        *  热接触间隙传导
        */
        void writeGapconductance(MechContactData::mContactBase* contact);

        /*
        *  几何调整值
        */
        void writeAdjust(MechContactData::mContactBase* contact);

        /*
        * 接触间隙
        */
        void writeClearance(MechContactData::mContactBase* contact);

        /*
        * 写载荷数据
        */
        bool writeLoadData();

        /*
        * 写集中力、力矩
        */
        void writeNodalload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写压强
        */
        void writePressure(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写重力
        */
        void readBodyload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写热流密度
        */
        void writeHeatFlux(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写热辐射
        */
        void writeRadiation(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写对流热换
        */
        void writeConvection(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写时域载荷
        */
        void writeTransient(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写频域载荷
        */
        void writeFreqdomain(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 组合载荷
        */
        void writeCombined(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写声学载荷
        */
        void writeAcousticConcForce(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写声学频域载荷
        */
        void writeAcousticFrequencyDomain(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        /*
       * 谱载荷
       */
        void writeSpectrum(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 螺栓载荷
        */
        void writeBoltload(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        //旋转加速度
        void writeRoACC(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> loadBase);

        //旋转速度
        void writeRoSpeed(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> loadBase);

        //写旋转分析数据
        void writeRotateAnalyse();

        //旋转物理功率（离心力/旋转加速度）
        void writeRotatePower(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写表面载荷
        */
        void writeSurface(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);

        /*
        * 写边载荷
        */
        void writeEdgeLoad(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadBase);
         
        /*
        * 写边界条件数据
        */
        bool writeBoundaryData();

        /*
        * 写固定/对称/反对称
        */
        void writeSpc(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase);

        /*
        * 写虚拟约束
        */
        void writeSuport(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase);

        /*
        * 写子模型位移边界
        */
        void writeBoundaryMove(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase);

        /*
        * 写子模型温度边界
        */
        void writeBoundaryTempNew(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase);

        /*
        * 写超单元边界
        */
        void writeSuperelement(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryBase);

        /*
        * 写连接
        */
        bool writeConnectorData();

        /*
        * 写轴承单元
        */
        bool writeBearingSection();

        /*
        * 写刚性连接
        */
        void writeConnectorRigid(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 写柔性连接
        */
        void writeConnectorFlexible(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 绑定
        */
        void writeConnectorBind(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * MPC
        */
        void writeConnectorMPC(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 离散刚体
        */
        void writeConnectorDisperse(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 解析刚体
        */
        void writeConnectorRigidbody(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 分布式耦合
        */
        void writeConnectorDistributedCoupling(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 轴承单元
        */
        void writeConnectorBearingUnits(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 螺栓约束
        */
        void writePretension(std::shared_ptr<MechConnectorData::mConnectorBase> connectorBase);

        /*
        * 坐标系
        */
        bool writemCoordinate();

        /*
        * 工具函数
        */
        void writeRawCoordinate(MechCoordinateData::mCoordinateBase* coor, const QString& axisLine);
        void writePlyCoordinate(MechCoordinateData::mCoordinateBase* coor, MechPropertyData::mPlyLaminate* plyLam, const QString& axisLine, bool isCopy);

        /*
        * 写直角、圆柱、球坐标系
        */
        void writeCoordinateBase(MechCoordinateData::mCoordinateBase* coor);

        /*
        * 写离散坐标系
        */
        void writeDiscreteCoordinate(MechCoordinateData::mCoordinateBase* coor);

        /*
        * 写采样频率点
        */
        bool writeFreqSamplPointData();

        /*
        * 写层合结构
        */
        bool writeLaminateData();

        /*
        * 写全局参数
        */
        bool writemGlobalParam();

        /*
        * 写峰值筛选数据
        */
        bool writePeakFilterData();

        /*
        * 写工况下的峰值筛选引用
        */
        void writeCasePeakFilterData(MechCaseData::mCaseBase* caseBase);

        /*
       * 写声固耦合对搜索
       */
        bool writeASCouplingSearchData();

        /*
        * 写超单元参数
        */
        bool writemGenericParam();

        /*
        * 写超单元调用
        */
        bool writemGenericAssign();

        /*
        * 写函数
        */
        bool writeFunction();

        /*
        * 写预定义场数据
        */
        bool writePredefinedFieldData();

        //写模型修改
        bool writeModelChangeData();

        /*
        * 写初始温度数据
        */
        void writeInitTempData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldBase, int id);

        /*
         * 写温度载荷数据
         */
        void writeTempLoadData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldBase, int id);

        /*
         * 写表格函数
         */
        void writeFunction1(MechFunctionData::mFunctionBase* func);

        /*
        * 写可平移表格函数
        */
        void writeFunction2(MechFunctionData::mFunctionBase* func);

        /*
        * 写缩放表格函数
        */
        void writeFunction3(MechFunctionData::mFunctionBase* func);

        /*
        * 写幂函数
        */
        void writeFunction4(MechFunctionData::mFunctionBase* func);

        /*
        * 写插值函数
        */
        void writeFunctionInterpCol(MechFunctionData::mFunctionBase* func);

        /*
        * 写周期函数（type = periodic）
        */
        void writeFunctionPeriodic(MechFunctionData::mFunctionBase* func);

        /*
        * 写调制函数（type = modulated）
        */
        void writeFunctionModulated(MechFunctionData::mFunctionBase* func);

        /*
        * 写指数衰减函数（type = decay）
        */
        void writeFunctionDecay(MechFunctionData::mFunctionBase* func);
        /*
        * 写连接副
        */
        bool writeConnectorDeputy();

        /*
        * 写连接副
        */
        void writeDeputy(std::shared_ptr<MechConnectorData::mDeputyBase> base);

        /*
        * 写fastener焊点
        */
        void writeFastener(std::shared_ptr<MechConnectorData::mDeputyBase> base);

    private:
        /*
        * 按格式写集合
        */
        QString getRenameSet();
        void writeSurfaceSetData(const std::vector<RenderPickResultData>& datas);
        void writeEdgeSetData(const std::vector<RenderPickResultData>& datas);
        void writeSetData(const std::vector<RenderPickResultData>& datas);
        void writeSetData(const std::set<int>& datas);

        /*
        * 写材料属性
        */
        void writeMaterialPropValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop, MxFunctions::MaterialPropType);

        /*
        * 写材料曲线属性
        */
        void writeMaterialPropCurve(const QHash<MxFunctions::MaterialPropType, QVariant>& prop, MxFunctions::MaterialPropType);

        /*
        * 写int类型映射数据
        */
        void writeMapIntData(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type);

        /*
        * 写int类型截面属性映射数据
        */
        void writeMapIntPropertyData(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type);

        /*
        * 写int类型映射数据 对热换流专属
        */
        void writeMapIntDataConv(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type, MxFunctions::MapRelationType relationType);

        /*
        * 写分析场映射数据（热学载荷：field/convfield）
        */
        void writeMapIntDataField(MxFunctions::PhysicalFieldType phyType, int phyID, MxFunctions::PhysicalFieldType type, MxFunctions::MapRelationType relationType);

        /*
        * 写QVariant变量
        */
        void writeQVariantValue(QVariant& data, MechCommon::QVariantType type = MechCommon::QVariantType::QVariant_Double);

        /*
        * 通过拾取类型获取节点
        */
        void getNodesByPickType(MxFunctions::PickDataType type, std::vector<RenderPickResultData> data, std::set<int>& nodeIDs);

        /*
        * 写从节点自由度
        */
        void writeConnectorSlaveNodeDof(std::set<int> dof);

        /*
        * 写截面形状
        */
        void writeSectionShapeType(int phyID, const QString& xb = "", bool isPbeami = false);

        /*
        * 写多行数据，每行16个数据
        */
        void writeMultiRowData(MxFunctions::PhysicalFieldType type, QList<int> ids);

        /*
        * 写向量数据
        */
        void writeCoorPointData(QString vec);


    private:
        //数据库
        MechModelData::mModelData* _modelData;
        mMeshData* _meshData;

        //写文件的文件名
        QString _outputFileName;

        //场名字
        QString _fieldName;

        //需要写文件的计算任务名称
        QString _jobName;

        //重命名集合索引
        int _index = 1;

        QFile _file;
        QTextStream _out;

        //写出的部件
        QStringList _partNames;

        //最大单元ID
        int _maxElementID;

        //过盈接触临时数据
        QHash<int, std::pair<QString, QString>> _interferenceContact;
    };
}
